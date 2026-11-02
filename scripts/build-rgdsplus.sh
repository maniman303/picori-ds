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

# The release targets the EU ROM (scripts/deploy-rgdsplus.sh), so compile
# against the EU asset/offset baseline instead of the committed USA headers.
# The generated EU headers are not checked in; build them once on the host
# with the EU ROM (python3 build.py --eur, or the xmake asset tasks).
EU_ASSETS=build/EU/assets
if [ ! -f "$EU_ASSETS/map_offsets.h" ] || [ ! -f "$EU_ASSETS/gfx_offsets.h" ]; then
    printf 'Missing EU generated asset headers in %s.\n' "$EU_ASSETS" >&2
    printf 'Place the EU ROM at baserom_eu.gba and run once on this host:\n' >&2
    printf '  python3 build.py --eur\n' >&2
    printf '(or configure --game_version=EU and run xmake extract_assets,\n' >&2
    printf ' convert_assets and build_assets).\n' >&2
    exit 2
fi

xmake f -y -p linux -a arm64 -m release --toolchain=cross \
    --cross=aarch64-linux-gnu- -o build/rgdsplus-aarch64 \
    --game_version=EU --multi_region=y --pc_avx2=n --gpu_renderer=y \
    --widescreen_width=576 --ra=n --repro_harness=n --pc_static_dependencies=y
# The target output path is shared with native configs. Remove a previous
# non-AArch64 binary so xmake relinks instead of reusing it as up to date.
rm -f build/pc/tmc_pc
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
