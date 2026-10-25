#!/bin/sh
# Build a slim, multi-region Linux ARM64 release using Picori's xmake graph.
set -eu
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

for tool in xmake python3 cmake ninja make pkg-config aarch64-linux-gnu-gcc aarch64-linux-gnu-g++ aarch64-linux-gnu-readelf aarch64-linux-gnu-strip sha256sum; do
    command -v "$tool" >/dev/null 2>&1 || {
        printf 'Missing build tool: %s (see scripts/README-rgdsplus.md)\n' "$tool" >&2
        exit 2
    }
done

# Never resolve target libraries through the host's amd64 pkg-config files.
export PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig
unset PKG_CONFIG_PATH XMAKE_USE_SYSTEM_SDL3 IN_NIX_SHELL
xmake f -y -p linux -a arm64 -m release --toolchain=cross \
    --cross=aarch64-linux-gnu- -o build/rgdsplus-aarch64 \
    --game_version=USA --multi_region=y --pc_avx2=n --gpu_renderer=y \
    --widescreen_width=569 --ra=n --repro_harness=n --pc_static_dependencies=y
xmake build -y -j "${BUILD_JOBS:-2}" tmc_pc

OUT="$ROOT/dist/rgdsplus"
mkdir -p "$OUT/assets/rando" "$OUT/lib" "$OUT/licenses"
aarch64-linux-gnu-readelf -h build/pc/tmc_pc | grep -q 'Machine:.*AArch64'
cp build/pc/tmc_pc "$OUT/tmc_pc"
aarch64-linux-gnu-strip --strip-unneeded "$OUT/tmc_pc"
cp assets/rando/default.logic "$OUT/assets/rando/default.logic"
cp scripts/launch-rgdsplus.sh "$OUT/launch.sh"
# SDL compiled against recent xkbcommon headers needs symbols absent from
# ROCKNIX's older library. Bundle only this small runtime, leaving system libs alone.
cp -L /usr/lib/aarch64-linux-gnu/libxkbcommon.so.0 "$OUT/lib/libxkbcommon.so.0"
cp /usr/share/doc/libxkbcommon0/copyright "$OUT/licenses/libxkbcommon-copyright"
chmod +x "$OUT/tmc_pc" "$OUT/launch.sh"
(cd "$OUT" && sha256sum tmc_pc launch.sh assets/rando/default.logic \
    lib/libxkbcommon.so.0 licenses/libxkbcommon-copyright > SHA256SUMS)
printf 'Slim ARM64 package ready: %s\n' "$OUT"
aarch64-linux-gnu-readelf -h "$OUT/tmc_pc" | grep -E 'Class:|Machine:'
