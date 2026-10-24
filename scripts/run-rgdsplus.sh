#!/bin/sh
set -eu
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
# shellcheck source=scripts/device-rgdsplus.sh
. "$ROOT/scripts/device-rgdsplus.sh"
ssh_device sh -s <<'REMOTE_RUN'
set -eu
cd /storage/roms/ports/picori
sha256sum -c SHA256SUMS
systemctl is-active --quiet sway.service
# A transient service survives the SSH connection and puts logs in the journal.
systemctl stop picori.service 2>/dev/null || true
systemctl reset-failed picori.service 2>/dev/null || true
systemd-run --unit=picori --collect --property=WorkingDirectory="$PWD" "$PWD/launch.sh"
invocation=$(systemctl show -p InvocationID --value picori.service)
[ -n "$invocation" ] || { echo 'Missing service invocation ID' >&2; exit 1; }
ready=false
for attempt in $(seq 1 120); do
    systemctl is-active --quiet picori.service || {
        journalctl "_SYSTEMD_INVOCATION_ID=$invocation" -n 80 --no-pager
        exit 1
    }
    if journalctl "_SYSTEMD_INVOCATION_ID=$invocation" --no-pager | grep -q 'Port layer initialized. Entering AgbMain'; then
        ready=true
        break
    fi
    sleep 1
done
[ "$ready" = true ] || {
    journalctl "_SYSTEMD_INVOCATION_ID=$invocation" -n 80 --no-pager
    echo 'Timed out waiting for AgbMain' >&2
    exit 1
}
# Catch startup failures immediately after entering the game loop as well.
sleep 5
systemctl is-active --quiet picori.service || {
    journalctl "_SYSTEMD_INVOCATION_ID=$invocation" -n 80 --no-pager
    exit 1
}
# MainPID is the lifecycle wrapper; verify/focus its game child.
pid=$(cat /run/picori-game.pid)
case "$pid" in ''|*[!0-9]*) echo 'Invalid game PID' >&2; exit 1;; esac
if systemctl is-active --quiet essway.service || pidof emulationstation >/dev/null 2>&1; then
    systemctl stop picori.service
    echo 'EmulationStation must be stopped while Picori owns controller input' >&2
    exit 1
fi
expected=$(awk '$2 == "tmc_pc" { print $1 }' SHA256SUMS)
actual=$(sha256sum "/proc/$pid/exe" | awk '{print $1}')
[ "$actual" = "$expected" ] || { echo 'Running executable SHA256 mismatch' >&2; exit 1; }
journalctl "_SYSTEMD_INVOCATION_ID=$invocation" --no-pager -n 100
export XDG_RUNTIME_DIR=/var/run/0-runtime-dir
for socket in "$XDG_RUNTIME_DIR"/sway-ipc.*.sock; do
    [ -S "$socket" ] || continue
    SWAYSOCK=$socket swaymsg "[pid=$pid] fullscreen enable" >/dev/null
    SWAYSOCK=$socket swaymsg "[pid=$pid] focus" >/dev/null
    break
done
printf 'Picori running: PID %s, SHA256 %s; AgbMain reached.\n' "$pid" "$actual"
REMOTE_RUN
