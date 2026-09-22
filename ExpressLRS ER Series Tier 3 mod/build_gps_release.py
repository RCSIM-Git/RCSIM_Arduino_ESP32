import gzip
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'ExpressLRS/src/python'))
import UnifiedConfiguration

release_dir = r"c:\Users\Mateusz\Desktop\er5c\release"
os.makedirs(release_dir, exist_ok=True)

target_repo_dir = r"c:\Users\Mateusz\Desktop\RCSIM27.04monacoSLAM\RCSIM_PC\pc_app\ESP32Arduino\RCSIM_Arduino_ESP32\ExpressLRS ER Series Tier 3 mod"
os.makedirs(target_repo_dir, exist_ok=True)

build_dir = r"c:\Users\Mateusz\Desktop\er5c\ExpressLRS\src\.pio\build\Unified_ESP8285_2400_RX_via_WIFI"
bin_source = os.path.join(build_dir, "firmware.bin")
gz_source = os.path.join(build_dir, "firmware.bin.gz")

if not os.path.exists(bin_source) and os.path.exists(gz_source):
    print("Decompressing firmware.bin.gz from build...")
    with gzip.open(gz_source, 'rb') as f_in:
        decompressed_data = f_in.read()
    with open(bin_source, 'wb') as f_out:
        f_out.write(decompressed_data)

bin_release = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin")
gz_release = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz")

with open(bin_source, "rb") as f_in:
    firmware_data = f_in.read()

# Write clean base binary
with open(bin_release, "wb") as f_out:
    f_out.write(firmware_data)

# Read standard hardware layout for ER5C V2
product_name = "RadioMaster ER5A/C V2 2.4GHz PWM RX"
lua_name = "RM ER5A/C V2"

hardware = {
    "radio_busy": 5, "radio_dio1": 4, "radio_miso": 12, "radio_mosi": 13,
    "radio_nss": 15, "radio_rst": 2, "radio_sck": 14, "power_min": 0,
    "power_high": 0, "power_max": 0, "power_default": 0, "power_control": 0,
    "power_values": [13], "power_lna_gain": 0, "led": 16,
    "pwm_outputs": [0, 1, 3, 9, 10], "vbat": 17, "vbat_offset": -7,
    "vbat_scale": 291, "vbat_cal_min": 4000, "vbat_cal_max": 35000,
    "vbat_noreading": -1
}

# Clean default defines WITHOUT personal WiFi SSID/Password or custom UID bindphrase
clean_defines = json.dumps({
    "flash-discriminator": 234619391,
    "wifi-on-interval": 60,
    "rcvr-uart-baud": 420000,
    "lock-on-first-connection": True
})

temp_layout_file = os.path.join(release_dir, "temp_official_layout.json")
with open(temp_layout_file, "w") as f:
    json.dump(hardware, f)

with open(bin_release, "r+b") as firmware_file:
    UnifiedConfiguration.appendToFirmware(
        firmware_file,
        product_name,
        lua_name,
        clean_defines,
        hardware,
        temp_layout_file,
        None
    )

with open(bin_release, "rb") as f_in:
    clean_data = f_in.read()

with gzip.open(gz_release, "wb") as f_out:
    f_out.write(clean_data)

print("Clean release generated successfully!")
print(f"BIN: {bin_release} ({os.path.getsize(bin_release)} bytes)")
print(f"GZ:  {gz_release} ({os.path.getsize(gz_release)} bytes)")

# Copy to RCSIM repository folder
repo_bin = os.path.join(target_repo_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin")
repo_gz = os.path.join(target_repo_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz")

shutil.copy2(bin_release, repo_bin)
shutil.copy2(gz_release, repo_gz)

print(f"Copied to repo: {repo_bin}")
print(f"Copied to repo: {repo_gz}")
