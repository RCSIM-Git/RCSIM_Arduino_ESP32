import gzip
import json
import os
import shutil
import sys

workspace_root = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(workspace_root, 'ExpressLRS/src/python'))
import UnifiedConfiguration

release_dir = os.path.join(workspace_root, "release")
os.makedirs(release_dir, exist_ok=True)

target_repo_dir = r"C:\Users\Mateusz\Desktop\RCSIM27.04monacoSLAM\RCSIM_PC\pc_app\ESP32Arduino\RCSIM_Arduino_ESP32\ExpressLRS ER Series Tier 3 mod"
os.makedirs(target_repo_dir, exist_ok=True)

targets_json_path = os.path.join(workspace_root, "ExpressLRS/src/hardware/targets.json")
with open(targets_json_path, "r", encoding="utf-8") as f:
    all_targets = json.load(f)

rm_rx2400 = all_targets.get("radiomaster", {}).get("rx_2400", {})

# Clean defines without user bindphrase / private Wi-Fi
clean_defines = json.dumps({
    "flash-discriminator": 234619391,
    "wifi-on-interval": 60,
    "rcvr-uart-baud": 420000,
    "lock-on-first-connection": True
})

esp8285_dir = os.path.join(workspace_root, r"ExpressLRS\src\.pio\build\Unified_ESP8285_2400_RX_via_WIFI")
esp8285_bin = os.path.join(esp8285_dir, "firmware.bin")
esp8285_gz  = os.path.join(esp8285_dir, "firmware.bin.gz")

if os.path.exists(esp8285_gz):
    print("[*] Decompressing firmware.bin.gz for ESP8285...")
    with gzip.open(esp8285_gz, 'rb') as f_in:
        decompressed_data = f_in.read()
    with open(esp8285_bin, 'wb') as f_out:
        f_out.write(decompressed_data)

esp32_dir   = os.path.join(workspace_root, r"ExpressLRS\src\.pio\build\Unified_ESP32_2400_RX_via_WIFI")
esp32_bin   = os.path.join(esp32_dir, "firmware.bin")

print("=" * 80)
print("[*] GENERATING STANDARDIZED RCSIM TIER 3 RELEASES (GPS + IMU HYBRID)")
print("=" * 80)

# Models to build with hybrid RCSIM_GPS_IMU
MODELS = [
    ("er4", "ELRS_V4.1_RadioMaster_ER4_RCSIM_GPS_IMU"),
    ("er5", "ELRS_V4.1_RadioMaster_ER5A_C_RCSIM_GPS_IMU"),
    ("er5-v2", "ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU"),
    ("er6", "ELRS_V4.1_RadioMaster_ER6_RCSIM_GPS_IMU"),
    ("er6g", "ELRS_V4.1_RadioMaster_ER6G_RCSIM_GPS_IMU"),
    ("er6gv", "ELRS_V4.1_RadioMaster_ER6GV_RCSIM_GPS_IMU"),
    ("er8", "ELRS_V4.1_RadioMaster_ER8_RCSIM_GPS_IMU"),
    ("er8g", "ELRS_V4.1_RadioMaster_ER8G_RCSIM_GPS_IMU"),
    ("er8gv", "ELRS_V4.1_RadioMaster_ER8GV_RCSIM_GPS_IMU"),
]

generated_files = []

for model_key, file_prefix in MODELS:
    if model_key not in rm_rx2400:
        print(f"[!] Skipped {model_key} - not in targets.json")
        continue

    t_info = rm_rx2400[model_key]
    product_name = t_info["product_name"]
    lua_name = t_info["lua_name"]
    platform = t_info["platform"]
    layout_file = t_info["layout_file"]

    base_bin_source = esp8285_bin if platform == "esp8285" else esp32_bin

    if not os.path.exists(base_bin_source):
        print(f"[-] Missing base binary: {base_bin_source}")
        continue

    layout_path = os.path.join(workspace_root, "ExpressLRS/src/hardware/RX", layout_file)
    with open(layout_path, "r", encoding="utf-8") as f:
        hardware = json.load(f)

    if "overlay" in t_info:
        for k, v in t_info["overlay"].items():
            if v is None:
                hardware.pop(k, None)
            else:
                hardware[k] = v

    bin_out = os.path.join(release_dir, f"{file_prefix}.bin")
    gz_out = os.path.join(release_dir, f"{file_prefix}.bin.gz")

    with open(base_bin_source, "rb") as f_src:
        raw_bin = f_src.read()

    with open(bin_out, "wb") as f_dst:
        f_dst.write(raw_bin)

    temp_layout_file = os.path.join(release_dir, f"temp_{model_key}.json")
    with open(temp_layout_file, "w", encoding="utf-8") as f:
        json.dump(hardware, f)

    with open(bin_out, "r+b") as firmware_file:
        UnifiedConfiguration.appendToFirmware(
            firmware_file,
            product_name,
            lua_name,
            clean_defines,
            hardware,
            temp_layout_file,
            None
        )

    if os.path.exists(temp_layout_file):
        os.remove(temp_layout_file)

    with open(bin_out, "rb") as f_in:
        final_bin = f_in.read()

    with gzip.open(gz_out, "wb") as f_out:
        f_out.write(final_bin)

    bin_size = os.path.getsize(bin_out)
    gz_size = os.path.getsize(gz_out)
    print(f"[OK] {product_name} ({platform.upper()}):")
    print(f"     -> {os.path.basename(bin_out)} ({bin_size:,} bytes)")
    print(f"     -> {os.path.basename(gz_out)} ({gz_size:,} bytes)")

    generated_files.append((bin_out, gz_out))

print("\n" + "=" * 80)
print("[*] CLEANING & POPULATING TARGET REPOSITORY DIRECTORY")
print(f"    Path: {target_repo_dir}")
print("=" * 80)

# Remove old ambiguous/unstandardized .bin and .bin.gz files from target_repo_dir
for item in os.listdir(target_repo_dir):
    if item.endswith(".bin") or item.endswith(".bin.gz"):
        # We will replace them cleanly
        old_path = os.path.join(target_repo_dir, item)
        os.remove(old_path)
        print(f"[REMOVED OLD] {item}")

# 1. Copy all newly built RCSIM combo binaries to target_repo_dir
for bin_path, gz_path in generated_files:
    dst_bin = os.path.join(target_repo_dir, os.path.basename(bin_path))
    dst_gz  = os.path.join(target_repo_dir, os.path.basename(gz_path))
    shutil.copy2(bin_path, dst_bin)
    shutil.copy2(gz_path, dst_gz)
    print(f"[COPIED RCSIM] {os.path.basename(bin_path)} & .gz")

# 2. Copy Standalone GPS-only firmware
gps_bin_src = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin")
gps_gz_src  = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin.gz")
if os.path.exists(gps_bin_src) and os.path.exists(gps_gz_src):
    shutil.copy2(gps_bin_src, os.path.join(target_repo_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin"))
    shutil.copy2(gps_gz_src,  os.path.join(target_repo_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin.gz"))
    print("[COPIED GPS ONLY] ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin & .gz")
else:
    print("[!] Warning: GPS only source not found in release/")

# 3. Copy Standalone IMU-only firmware
imu_bin_src = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_IMU.bin")
imu_gz_src  = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_IMU.bin.gz")
if not os.path.exists(imu_bin_src):
    # Fallback to MPU9250 binary if named that way in release
    alt_src = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin")
    if os.path.exists(alt_src):
        shutil.copy2(alt_src, imu_bin_src)
if not os.path.exists(imu_gz_src):
    alt_gz = os.path.join(release_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin.gz")
    if os.path.exists(alt_gz):
        shutil.copy2(alt_gz, imu_gz_src)

if os.path.exists(imu_bin_src) and os.path.exists(imu_gz_src):
    shutil.copy2(imu_bin_src, os.path.join(target_repo_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_IMU.bin"))
    shutil.copy2(imu_gz_src,  os.path.join(target_repo_dir, "ELRS_V4.1_RadioMaster_ER5Cv2_IMU.bin.gz"))
    print("[COPIED IMU ONLY] ELRS_V4.1_RadioMaster_ER5Cv2_IMU.bin & .gz")
else:
    print("[!] Warning: IMU only source not found in release/")

print("\n[OK] SUCCESS: All standardized binaries generated and copied!")
