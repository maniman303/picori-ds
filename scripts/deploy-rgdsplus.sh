#!/bin/sh
set -eu
umask 077
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
# shellcheck source=scripts/device-rgdsplus.sh
. "$ROOT/scripts/device-rgdsplus.sh"
OUT="$ROOT/dist/rgdsplus"
REMOTE=/storage/roms/ports/picori
[ -f "$OUT/SHA256SUMS" ] || { echo 'Run scripts/build-rgdsplus.sh first' >&2; exit 2; }
(cd "$OUT" && sha256sum -c SHA256SUMS)
aarch64-linux-gnu-readelf -h "$OUT/tmc_pc" | grep -q 'Machine:.*AArch64'

# Pass the optional ROM path over stdin, never as remotely evaluated shell text.
printf '%s\n' "${DEVICE_ROM:-}" | ssh_device "mkdir -p $REMOTE; cat > $REMOTE/.rom-source"
ssh_device sh -s <<'REMOTE_PREFLIGHT'
set -eu
[ "$(uname -m)" = aarch64 ] || { echo 'Device is not aarch64' >&2; exit 1; }
cd /storage/roms/ports/picori
expected=cff199b36ff173fb6faf152653d1bccf87c26fb7
valid_rom() { [ -f "$1" ] && [ "$(sha1sum "$1" | awk '{print $1}')" = "$expected" ]; }
if [ -e baserom_eu.gba ]; then
    valid_rom baserom_eu.gba || { echo 'Existing baserom_eu.gba has wrong SHA1; preserving it' >&2; exit 1; }
else
    IFS= read -r source < .rom-source || true
    if [ -n "$source" ]; then
        case "$source" in /roms/*) ;; *) echo 'DEVICE_ROM must be under /roms/' >&2; exit 1;; esac
        valid_rom "$source" || { echo 'DEVICE_ROM is not the supported EU ROM' >&2; exit 1; }
        cp "$source" baserom_eu.gba
    else
        # /roms is itself a symlink on ROCKNIX; trailing slash traverses it.
        find /roms/ -type f -iname '*.gba' -exec sh -c '
            dest=$1; shift
            for source do
                [ ! -e "$dest" ] || exit 0
                hash=$(sha1sum "$source" | cut -d " " -f 1)
                if [ "$hash" = cff199b36ff173fb6faf152653d1bccf87c26fb7 ]; then
                    cp "$source" "$dest"
                fi
            done
        ' sh "$PWD/baserom_eu.gba" {} +
    fi
    valid_rom baserom_eu.gba || { echo 'No SHA1-valid EU Minish Cap ROM found in /roms' >&2; exit 1; }
fi
rm -f .rom-source
mkdir -p .upload/assets/rando .upload/lib .upload/licenses
REMOTE_PREFLIGHT

scp_device "$OUT/tmc_pc" "$OUT/launch.sh" "$OUT/SHA256SUMS" "$DEVICE_USER@$DEVICE_IP:$REMOTE/.upload/"
scp_device "$OUT/assets/rando/default.logic" "$DEVICE_USER@$DEVICE_IP:$REMOTE/.upload/assets/rando/"
scp_device "$OUT/lib/libxkbcommon.so.0" "$DEVICE_USER@$DEVICE_IP:$REMOTE/.upload/lib/"
scp_device "$OUT/licenses/libxkbcommon-copyright" "$DEVICE_USER@$DEVICE_IP:$REMOTE/.upload/licenses/"
ssh_device sh -s <<'REMOTE_DEPLOY'
set -eu
cd /storage/roms/ports/picori/.upload
sha256sum -c SHA256SUMS
chmod +x tmc_pc launch.sh
LD_LIBRARY_PATH="$PWD/lib" ldd ./tmc_pc > dependencies.log 2>&1 || { cat dependencies.log; exit 1; }
if grep -q 'not found' dependencies.log; then cat dependencies.log; exit 1; fi
LD_LIBRARY_PATH="$PWD/lib" ldd lib/libxkbcommon.so.0 > xkb-dependencies.log 2>&1 || { cat xkb-dependencies.log; exit 1; }
if grep -q 'not found' xkb-dependencies.log; then cat xkb-dependencies.log; exit 1; fi
mkdir -p ../assets/rando ../lib ../licenses
mv tmc_pc ../tmc_pc
mv launch.sh ../launch.sh
mv assets/rando/default.logic ../assets/rando/default.logic
mv lib/libxkbcommon.so.0 ../lib/libxkbcommon.so.0
mv licenses/libxkbcommon-copyright ../licenses/libxkbcommon-copyright
mv SHA256SUMS ../SHA256SUMS
cd ..
sha256sum -c SHA256SUMS
printf 'Deployed to %s; ROM, executable permissions and shared libraries verified.\n' "$PWD"
REMOTE_DEPLOY
