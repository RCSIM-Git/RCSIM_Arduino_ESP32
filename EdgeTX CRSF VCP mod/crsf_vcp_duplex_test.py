#!/usr/bin/env python3
"""Test bidirectional CRSF over USB-VCP (Trainer RX + Telemetry Mirror TX).

Działa w obie strony:
  1. Nadaje ramki sterujące RC (kanały 1-16) z PC do aparatury (100 Hz).
  2. Odbiera i dekoduje na żywo pakiety telemetrii CRSF wysyłane przez aparaturę do PC:
     - Link Statistics (0x14): RSSI, Link Quality (LQ), SNR, RF Mode
     - Bateria (0x08): Napięcie, Prąd, Zużycie mAh, %
     - GPS (0x02): Pozycja, Prędkość, Kurs
"""

import argparse
import glob
import os
import struct
import sys
import threading
import time

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    sys.exit("Wymagany pyserial: pip install pyserial")

UART_SYNC = 0xC8
CHANNELS_ID = 0x16

# ID ramek CRSF
TYPE_GPS = 0x02
TYPE_BATTERY = 0x08
TYPE_LINK_STATISTICS = 0x14
TYPE_RC_CHANNELS = 0x16
TYPE_ATTITUDE = 0x1E

NUM_CHANNELS = 16
CH_BITS = 11

CRSF_MIN = 172
CRSF_CENTER = 992
CRSF_MAX = 1811

# CRC8 (Wielomian 0xD5)
_CRC8_POLY = 0xD5
_CRC8_TAB = []
for _i in range(256):
    _c = _i
    for _ in range(8):
        _c = ((_c << 1) ^ _CRC8_POLY) & 0xFF if _c & 0x80 else (_c << 1) & 0xFF
    _CRC8_TAB.append(_c)


def crc8(data: bytes) -> int:
    crc = 0
    for b in data:
        crc = _CRC8_TAB[crc ^ b]
    return crc


def encode_channels_frame(channels):
    """Tworzy ramkę kanałów CRSF: [0xC8][len][0x16][22 bajty][crc8]."""
    payload = bytearray()
    bits = 0
    bits_available = 0
    for value in channels:
        bits |= (int(value) & 0x7FF) << bits_available
        bits_available += CH_BITS
        while bits_available >= 8:
            payload.append(bits & 0xFF)
            bits >>= 8
            bits_available -= 8

    frame = bytearray([UART_SYNC, len(payload) + 2, CHANNELS_ID])
    frame += payload
    frame.append(crc8(frame[2:]))
    return bytes(frame)


def triangle(phase):
    return 2 * phase if phase < 0.5 else 2 * (1.0 - phase)


def sweep_channels(elapsed, period):
    channels = [CRSF_CENTER] * NUM_CHANNELS
    for ch in range(4):
        phase = ((elapsed / period) + ch * 0.25) % 1.0
        channels[ch] = int(CRSF_MIN + (CRSF_MAX - CRSF_MIN) * triangle(phase))
    return channels


def parse_telemetry_frame(frame_type: int, payload: bytes):
    """Dekoduje ramki telemetrii CRSF przychodzące z aparatury."""
    if frame_type == TYPE_LINK_STATISTICS and len(payload) >= 10:
        # RSSI: TBS / ELRS specyfikacja
        raw_rssi1 = payload[0]
        rssi1 = raw_rssi1 - 256 if raw_rssi1 > 127 else -raw_rssi1
        lq = payload[2]
        snr = struct.unpack("b", bytes([payload[3]]))[0]
        rf_mode = payload[5]
        return f"[LINK STATS] RSSI: {rssi1} dBm | LQ: {lq}% | SNR: {snr} dB | RF Mode: {rf_mode}"

    elif frame_type == TYPE_BATTERY and len(payload) >= 8:
        voltage = struct.unpack(">H", payload[0:2])[0] / 10.0
        current = struct.unpack(">H", payload[2:4])[0] / 10.0
        capacity = (payload[4] << 16) | (payload[5] << 8) | payload[6]
        remaining = payload[7]
        return f"[BATTERY] {voltage:.1f}V | {current:.1f}A | {capacity} mAh | {remaining}%"

    elif frame_type == TYPE_GPS and len(payload) >= 15:
        lat = struct.unpack(">i", payload[0:4])[0] / 1e7
        lon = struct.unpack(">i", payload[4:8])[0] / 1e7
        speed = struct.unpack(">H", payload[8:10])[0] / 10.0
        sats = payload[14]
        return f"[GPS] Lat: {lat:.6f}, Lon: {lon:.6f} | V: {speed:.1f} km/h | Sats: {sats}"

    return f"[CRSF 0x{frame_type:02X}] {len(payload)}B: {payload.hex()}"


def autodetect_port():
    ports = list(serial.tools.list_ports.comports())
    if sys.platform.startswith("win"):
        if not ports:
            sys.exit("Błąd: Nie znaleziono żadnego portu COM!")
        for p in ports:
            desc = (p.description or "") + " " + (p.manufacturer or "")
            if any(kw in desc for kw in ["STM", "Virtual", "EdgeTX", "RadioMaster", "CDC"]):
                print(f"Wykryto radio na porcie: {p.device} ({p.description})")
                return p.device
        return ports[0].device

    candidates = sorted(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*"))
    if candidates:
        return candidates[0]
    sys.exit("Nie znaleziono portu szeregowego. Podaj --port ręcznie.")


def rx_worker(ser, stop_event):
    """Wątek odbiorczy: ciągły odczyt i parsuje pakiety telemetrii CRSF."""
    buffer = bytearray()
    while not stop_event.is_set():
        try:
            n = ser.in_waiting
            if n > 0:
                data = ser.read(n)
                buffer.extend(data)

                while len(buffer) >= 4:
                    # Szukaj nagłówka synchronizacji (0xC8, 0xEE, 0xEA)
                    if buffer[0] not in (0xC8, 0xEE, 0xEA, 0xEC):
                        buffer.pop(0)
                        continue

                    length = buffer[1]
                    total_len = length + 2

                    if total_len < 4 or total_len > 64:
                        buffer.pop(0)
                        continue

                    if len(buffer) < total_len:
                        break  # Czekaj na resztę ramki

                    frame = buffer[:total_len]
                    buffer = buffer[total_len:]

                    frame_type = frame[2]
                    payload = frame[3:-1]
                    frame_crc = frame[-1]

                    if crc8(frame[2:-1]) == frame_crc:
                        info = parse_telemetry_frame(frame_type, payload)
                        print(f"   <-- TELEMETRIA: {info}")
            else:
                time.sleep(0.005)
        except Exception as e:
            if not stop_event.is_set():
                print(f"[RX Error] {e}")
            break


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Port szeregowy COM (np. COM3 lub /dev/ttyACM0)")
    parser.add_argument("--rate", type=float, default=100.0, help="Częstotliwość sterowania Hz (domyślnie 100)")
    args = parser.parse_args()

    port = args.port or autodetect_port()
    print(f"--- EdgeTX USB-VCP Full-Duplex Test ---")
    print(f"Port: {port}")
    print(f"Sterowanie TX: 100 Hz kanały 1-4 (fala trójkątna)")
    print(f"Nasłuch RX: Telemetria CRSF z radia / pojazdu")
    print("Naciśnij Ctrl+C, aby zakończyć.\n")

    ser = serial.Serial(port, baudrate=115200, timeout=0.1)
    stop_event = threading.Event()
    rx_th = threading.Thread(target=rx_worker, args=(ser, stop_event), daemon=True)
    rx_th.start()

    interval = 1.0 / args.rate
    start = time.monotonic()
    next_send = start
    count = 0

    try:
        while True:
            now = time.monotonic()
            channels = sweep_channels(now - start, 4.0)
            ser.write(encode_channels_frame(channels))
            count += 1

            if count % 100 == 0:
                print(f"[TX] Wysłano {count} ramek sterujących ({args.rate:g} Hz)...")

            next_send += interval
            delay = next_send - time.monotonic()
            if delay > 0:
                time.sleep(delay)
            else:
                next_send = time.monotonic()
    except KeyboardInterrupt:
        print("\nZatrzymywanie...")
    finally:
        stop_event.set()
        rx_th.join(timeout=0.5)
        ser.close()
        print("Zakończono.")


if __name__ == "__main__":
    main()
