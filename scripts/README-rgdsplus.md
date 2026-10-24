# Picori on RG DS Plus / ROCKNIX

This workflow builds Picori locally with xmake and the Linux GNU AArch64
cross compiler, stages a slim package in `dist/rgdsplus`, uploads it over SSH
to `/storage/roms/ports/picori`, and optionally launches it under Sway.

## Host setup (Ubuntu / Debian)

Install the build tools (sudo is only needed for package installation):

```sh
sudo apt update
sudo apt install xmake build-essential cmake ninja-build pkg-config python3 git curl unzip zip \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu \
    openssh-client sshpass
```

The SDL3 build also needs target graphics/audio development libraries. On a
host already configured for ARM64 multiarch:

```sh
sudo apt install libasound2-dev:arm64 libwayland-dev:arm64 libxkbcommon-dev:arm64 \
    libegl-dev:arm64 libgles-dev:arm64 libgbm-dev:arm64 libdrm-dev:arm64 \
    libudev-dev:arm64 wayland-protocols
```

If ARM64 packages are unavailable, first enable `arm64` with
`sudo dpkg --add-architecture arm64` and configure your distribution's ARM64
package mirror (Ubuntu normally uses ports.ubuntu.com), then `sudo apt update`.
The helpers do not install system packages themselves.

## Build

```sh
BUILD_JOBS=2 scripts/build-rgdsplus.sh
```

The first build requires network access for xmake packages. SDL3 is built from
the project's pinned source; PNG and zlib are built statically to avoid host
or device library version mismatches. The target still uses the device's
glibc, libstdc++, OpenMP and graphics/audio libraries. Build against a libc
no newer than the device's; deployment checks dynamic dependencies before
promoting the upload. The current host/device pair uses glibc 2.43/2.44.
The package includes ARM64 `libxkbcommon.so.0` and its copyright notice:
the device's older xkbcommon lacks `xkb_keymap_mod_get_mask`, which SDL needs
when compiled against the host's newer headers. The launcher uses the private
`lib/` directory; no device system libraries are replaced.

The release uses the existing GPU/SDL_Renderer fallback, a 384-pixel widescreen
capacity, and no RetroAchievements (avoids a cross-built curl/TLS dependency
chain). The USA asset baseline and `multi_region=y` match Picori's checked-in
offset headers; the EU ROM is detected at runtime. No ROM or generated assets
are needed on the build host. The embedded extractor generates assets on the
device's first launch. `default.logic` is staged for the randomizer.

Object files live in `build/rgdsplus-aarch64`; xmake's current project config
is changed to ARM64. The existing target writes its unstripped executable to
`build/pc/tmc_pc`; the helper checks its architecture, stages a stripped copy
in `dist/rgdsplus`, and writes a SHA256 manifest. Reconfigure xmake for native
builds afterward if needed.

## SSH configuration and deployment

```sh
cp scripts/deploy-rgdsplus.conf.example scripts/deploy-rgdsplus.conf
chmod 600 scripts/deploy-rgdsplus.conf
# Edit DEVICE_IP, DEVICE_USER, and optionally DEVICE_PASSWORD / DEVICE_ROM.
scripts/deploy-rgdsplus.sh
scripts/run-rgdsplus.sh
```

The private config is git-ignored. An empty password uses SSH keys/agent;
otherwise `sshpass -e` supplies it without a password argument. SSH accepts
the first host key and rejects changed keys. Set `DEPLOY_CONFIG` to use another
config file. Keep configs trusted: they are sourced as shell code.

Deployment validates the AArch64 executable and manifest, finds a ROM under
`/roms/` with the supported EU SHA1
`cff199b36ff173fb6faf152653d1bccf87c26fb7`, and copies it to
`/storage/roms/ports/picori/baserom_eu.gba` only when missing. Set `DEVICE_ROM`
to an exact `/roms/...` path to skip the search. An existing invalid ROM stops
deployment and is preserved. Saves, configuration and extracted assets stay
in place across deployments. Uploads are staged in `.upload`, checked on the
device, made executable with `chmod +x`, and renamed into place.

The run helper restarts only its own `picori.service` transient unit, detects
the existing Wayland socket, launches `launch.sh`, checks the running
executable's SHA256 and the `AgbMain` startup message, then focuses and
fullscreens the window through Sway. The launcher stops `essway.service`
before starting the game and refuses to start if EmulationStation is still
running, so controller input cannot also reach the frontend. It restores
EmulationStation when the game exits or `picori.service` is stopped, provided
the frontend was active before launch. Sway stays running throughout.
Startup waits up to 120 seconds for first-launch extraction
and checks logs from the current service invocation only.

```sh
# On the device:
journalctl -u picori.service -n 100 --no-pager
systemctl stop picori.service
# Or launch directly from an independent device terminal (outside EmulationStation):
/storage/roms/ports/picori/launch.sh
```

Deployment and launch are separate so another build can be uploaded without
automatically interrupting a running game. Run both helpers after rebuilding
to replace and restart the game.
