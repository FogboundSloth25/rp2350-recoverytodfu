#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
declare -A BOARD_UF2=(
  [waveshare_rp2350_usb_a]="rp2350-recovery.waveshare_rp2350_usb_a.uf2"
  [waveshare_rp2350_zero]="rp2350-recovery.waveshare_rp2350_zero.uf2"
  [pimoroni_tiny2350]="rp2350-recovery.pimoroni_tiny2350.uf2"
  [pico2]="rp2350-recovery.pico2.uf2"
)
BOARD_ORDER=(waveshare_rp2350_usb_a waveshare_rp2350_zero pimoroni_tiny2350 pico2)
board_title(){ case "$1" in waveshare_rp2350_usb_a) echo "Waveshare RP2350 USB-A";; waveshare_rp2350_zero) echo "Waveshare RP2350 Zero";; pimoroni_tiny2350) echo "Pimoroni TINY2350";; pico2) echo "Raspberry Pi Pico 2";; *) echo "$1";; esac; }
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"
SDK_PATH="${PICO_SDK_PATH:-$ROOT/pico-sdk}"
BUILD_ROOT="${BUILD_ROOT:-$ROOT/build}"
OUTPUT_DIR="$BUILD_ROOT/uf2"
LOG_FILE="${BUILD_LOG:-$ROOT/build.log}"
mkdir -p "$(dirname "$LOG_FILE")"
: > "$LOG_FILE"
exec > >(tee "$LOG_FILE") 2>&1
finish(){ rc=$?; echo; [[ $rc -eq 0 ]] && echo "Build log saved to: $LOG_FILE" || echo "Build failed (exit $rc). Full log saved to: $LOG_FILE" >&2; exit $rc; }
trap finish EXIT
select_boards(){
  echo; echo "=============================================="; echo "        RP2350 Recovery board selector"; echo "=============================================="; echo
  local i=1 board
  for board in "${BOARD_ORDER[@]}"; do printf '  %d) %s\n' "$i" "$(board_title "$board")"; ((i+=1)); done
  printf '  %d) All boards\n' "$i"; echo
  local choice
  while true; do
    read -r -p "Select board [1-$i]: " choice
    case "$choice" in 1|2|3|4) REPLY_BOARDS=("${BOARD_ORDER[$((choice-1))]}"); return;; 5) REPLY_BOARDS=("${BOARD_ORDER[@]}"); return;; *) echo "Invalid choice.";; esac
  done
}
usage(){ echo "Usage: ./build.sh [waveshare_rp2350_usb_a|waveshare_rp2350_zero|pimoroni_tiny2350|pico2 ...]"; }
if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then usage; exit 0; fi
if [[ ! -d "$SDK_PATH" ]]; then PICO_SDK_PATH="$SDK_PATH" "$ROOT/scripts/bootstrap-pico-sdk.sh"; fi
[[ -f "$SDK_PATH/external/pico_sdk_import.cmake" ]] || { echo "ERROR: invalid Pico SDK: $SDK_PATH" >&2; exit 1; }
if [[ "$#" -eq 0 ]]; then select_boards; targets=("${REPLY_BOARDS[@]}"); else targets=("$@"); fi
for board in "${targets[@]}"; do [[ -n "${BOARD_UF2[$board]+x}" ]] || { echo "ERROR: unsupported board: $board" >&2; exit 2; }; done
mkdir -p "$OUTPUT_DIR"
for board in "${targets[@]}"; do
  build_dir="$BUILD_ROOT/$board"; uf2_name="${BOARD_UF2[$board]}"
  echo "Building $(board_title "$board")"
  [[ "${CLEAN:-0}" == "1" ]] && rm -rf "$build_dir"
  cmake -S "$ROOT" -B "$build_dir" -DPICO_SDK_PATH="$SDK_PATH" -DPICO_BOARD="$board" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$build_dir" -j"$JOBS"
  uf2="$build_dir/rp2350_recovery.uf2"
  [[ -f "$uf2" ]] || { echo "ERROR: UF2 missing: $uf2" >&2; exit 1; }
  cp -f "$uf2" "$OUTPUT_DIR/$uf2_name"
  sha256sum "$OUTPUT_DIR/$uf2_name"
done
echo "Build complete."
