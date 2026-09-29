#!/usr/bin/env bash
# ============================================================================
# prepare_source.sh — Prepare the complete corresponding source tree
# ============================================================================
#
# This script creates the full source archive that corresponds to the
# distributed .bin firmware files, as required by GPL-2.0 §3.
#
# It performs the following steps:
#   1. Clones the EdgeTX repository
#   2. Checks out the exact commit used for building (e5784ee5)
#   3. Initialises submodules (required for a buildable tree)
#   4. Applies the full-duplex telemetry mirror patch
#   5. Packages the result into a .tar.gz archive
#
# The resulting archive, together with BUILD_INSTRUCTIONS.md, constitutes
# the "complete corresponding source code" for all seven .bin files.
#
# Usage:
#   chmod +x prepare_source.sh
#   ./prepare_source.sh
#
# Requirements: git, patch, tar
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH_FILE="${SCRIPT_DIR}/edgetx_crsf_vcp_full_duplex.patch"

EDGETX_REPO="https://github.com/BelixRogner/edgetx.git"
BASE_COMMIT="e5784ee5"
BRANCH="feat/crsf-trainer-over-usb-vcp"

OUTPUT_DIR="${SCRIPT_DIR}/edgetx-crsf-vcp-source"
ARCHIVE_NAME="edgetx-crsf-vcp-fullduplex-source.tar.gz"

# ── Preflight checks ────────────────────────────────────────────────────────
if [ ! -f "${PATCH_FILE}" ]; then
    echo "ERROR: Patch file not found: ${PATCH_FILE}" >&2
    exit 1
fi

for cmd in git patch tar; do
    if ! command -v "${cmd}" &>/dev/null; then
        echo "ERROR: Required command '${cmd}' not found." >&2
        exit 1
    fi
done

# ── Step 1: Clone and checkout ──────────────────────────────────────────────
echo "=== Step 1/5: Cloning EdgeTX repository ==="
if [ -d "${OUTPUT_DIR}" ]; then
    echo "Removing existing source directory: ${OUTPUT_DIR}"
    rm -rf "${OUTPUT_DIR}"
fi

git clone --branch "${BRANCH}" "${EDGETX_REPO}" "${OUTPUT_DIR}"

echo "=== Step 2/5: Checking out exact build commit ${BASE_COMMIT} ==="
cd "${OUTPUT_DIR}"
git checkout "${BASE_COMMIT}"

echo "=== Step 3/5: Initialising submodules ==="
git submodule update --init --recursive

# ── Step 2: Apply patch ─────────────────────────────────────────────────────
echo "=== Step 4/5: Applying full-duplex telemetry mirror patch ==="
git apply "${PATCH_FILE}"

# Mark the applied changes clearly (GPL-2.0 §2a: mark modified files)
git add -A
git commit --author="RCSIM Project <noreply@example.com>" \
    -m "Apply CRSF VCP full-duplex telemetry mirror patch

Modifications for RCSIM-GCS project:
- serial.cpp: Enable bidirectional VCP (TX+RX) and telemetrySetMirrorCb
- CMakeLists.txt: Size-optimisation flags for 512KB targets
- bootloader/CMakeLists.txt: LTO for bootloader
- targets/taranis/CMakeLists.txt: Disable PXX1/PXX2/HELI/GHOST on
  512KB RadioMaster targets (Pocket, TX12, TX12MK2, Zorro)
- thirdparty/FatFs/ffconf.h: Conditional features in bootloader context
- system_clock.c, system_stm32f4xx.c: __attribute__((used)) to prevent
  LTO from stripping critical startup functions
- tools/build-common.sh: Pocket target build options

Applied on top of commit e5784ee5 from branch feat/crsf-trainer-over-usb-vcp.
See BUILD_INSTRUCTIONS.md for per-target build commands."

# ── Step 3: Copy build instructions and license info ────────────────────────
echo "=== Step 5/5: Copying build documentation ==="
if [ -f "${SCRIPT_DIR}/BUILD_INSTRUCTIONS.md" ]; then
    cp "${SCRIPT_DIR}/BUILD_INSTRUCTIONS.md" "${OUTPUT_DIR}/"
fi

# ── Step 4: Create archive ──────────────────────────────────────────────────
echo "=== Creating source archive: ${ARCHIVE_NAME} ==="
cd "${SCRIPT_DIR}"
tar czf "${ARCHIVE_NAME}" \
    --exclude='.git' \
    -C "${SCRIPT_DIR}" \
    "$(basename "${OUTPUT_DIR}")"

ARCHIVE_SIZE=$(du -sh "${ARCHIVE_NAME}" | cut -f1)
echo ""
echo "============================================================"
echo "  Source archive created: ${ARCHIVE_NAME} (${ARCHIVE_SIZE})"
echo "============================================================"
echo ""
echo "This archive contains the complete corresponding source code"
echo "for all seven .bin firmware files distributed in this directory."
echo ""
echo "To verify: extract, follow BUILD_INSTRUCTIONS.md, and build"
echo "each target with the documented CMake commands."
echo ""
echo "NOTE: The .git directory is excluded from the archive."
echo "If you prefer to publish a git branch instead of an archive,"
echo "push the '${OUTPUT_DIR}' directory as a branch to your repo."
