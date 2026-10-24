#!/bin/sh
# Installed beside tmc_pc; run from the SSH helper or an independent terminal.
set -eu
cd /storage/roms/ports/picori
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export HOME=/storage
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/var/run/0-runtime-dir}"
if [ -z "${WAYLAND_DISPLAY:-}" ]; then
    for socket in "$XDG_RUNTIME_DIR"/wayland-*; do
        [ -S "$socket" ] || continue
        WAYLAND_DISPLAY=${socket##*/}
        export WAYLAND_DISPLAY
        break
    done
fi
: "${WAYLAND_DISPLAY:?No running Wayland compositor found}"
export SDL_VIDEODRIVER=wayland
export SDL_APP_ID=picori
export SDL_VIDEO_WAYLAND_WMCLASS=picori
export OMP_NUM_THREADS="${OMP_NUM_THREADS:-4}"

frontend_was_active=false
game_pid=
pidfile=/run/picori-game.pid
# shellcheck disable=SC2329 # Invoked by the EXIT trap.
cleanup() {
    status=$?
    trap - EXIT HUP INT TERM
    if [ -n "$game_pid" ]; then
        kill "$game_pid" 2>/dev/null || true
        wait "$game_pid" 2>/dev/null || true
    fi
    rm -f "$pidfile"
    if [ "$frontend_was_active" = true ]; then
        systemctl start essway.service || {
            echo 'Failed to restore EmulationStation (essway.service)' >&2
            status=1
        }
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

if systemctl is-active --quiet essway.service; then
    frontend_was_active=true
    systemctl stop essway.service
fi
if systemctl is-active --quiet essway.service || pidof emulationstation >/dev/null 2>&1; then
    echo 'EmulationStation is still running; refusing to share controller input' >&2
    exit 1
fi
./tmc_pc --skip-launcher "$@" &
game_pid=$!
printf '%s\n' "$game_pid" > "$pidfile"
status=0
wait "$game_pid" || status=$?
game_pid=
exit "$status"
