#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK_PATH="${PICO_SDK_PATH:-$ROOT/pico-sdk}"
SDK_REF="${PICO_SDK_REF:-2.2.0}"
if [[ -d "$SDK_PATH/.git" ]]; then git -C "$SDK_PATH" submodule update --init --recursive; exit 0; fi
mkdir -p "$(dirname "$SDK_PATH")"
git clone --depth 1 --branch "$SDK_REF" --recurse-submodules https://github.com/raspberrypi/pico-sdk.git "$SDK_PATH"
