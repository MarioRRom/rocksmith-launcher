# Direct Connect Patch

**Status:** implemented and verified in-game.

## What it does

Enables the game's hidden **Input > Direct Connect** mode, which lets the player
pick any audio input (guitar line-in, audio interface, mic) directly from the
game's own menu.

## How it works

Rocksmith 2014 ships with a Direct Connect input mode that was planned for the
original game and left in the Remastered codebase but disabled — gated behind a
data flag, not missing code. Enabling it requires editing two JSON manifests
inside `cache.psarc`, the game's packed data archive:

- **`ui_menu_pillar_startup.database.json`** — adds the `"othercable"` entry to
  `FE_InputSelect.Images` and `FE_InputSelect.Buttons`, which makes the game
  render the Direct Connect option in its input selection menu.
- **`ui_menu_pillar_mission.database.json`** — adds `"3": "DIRECT CONNECT"` to
  `inputMode.AcceptedValues`, which tells the game that input mode 3 (Direct
  Connect) is a valid selection.

The process: extract `cache.psarc` to a temp dir, open `cache7.7z` with `7z`,
edit the two JSON manifests, update the 7z in-place, and repack the PSARC.

The original is backed up as `cache.psarc.bak` on the first edit and never touched.

## Project files

- **[direct_connect_patch.cpp](../rocklaunch-core/src/patches/direct_connect_patch.cpp)** — patch logic: backup, extract, mutate JSON, repack.
- **[psarc_util.cpp](../rocklaunch-core/src/utils/psarc_util.cpp)** — PSARC reader/writer (AES-256-CFB TOC, zlib blocks).
- **[subprocess.cpp](../rocklaunch-core/src/utils/subprocess.cpp)** — calls system `7z` for extraction/update inside PSARC.

## Prefix requirement

`Audio=alsa` must be set in the prefix for the game to enumerate audio devices
via ALSA (PulseAudio reports zero capture devices under Wine/Proton). Applied
automatically by `EnsurePrefix` on every launch — the player does not need to
do anything.

## Thanks

This method exists thanks to [Lovrom8](https://github.com/Lovrom8) and the
[RSMods](https://github.com/Lovrom8/RSMods) project. Lovrom8 personally
recommended this approach over the old memory-patch route — a much simpler and
more reliable path. We reimplement the `cache.psarc` edit at launcher level,
credited in the README.

---

## Discarded: memory-write approach (Plan B)

The original design before Direct Connect Mode was discovered. Kept for
reference.

### How it worked

The game compares the audio endpoint's VID/PID against a **hardcoded value in
its own executable memory**. The Real Tone cable values are VID `12BA` and PID
`00FF` (bytes `BA 12` and `FF 00` in little-endian). Under Wine the game reads
the real device's VID/PID correctly, but the *expected* values are hardcoded —
so only the original cable is accepted.

The fix: overwrite those 4 bytes in the running process with the selected
device's VID/PID. The game then compares the real device against the patched
values → match → input accepted.

The pattern to locate in memory: `BA 12 92 0A 10 C0 11 C0 FF 00`. The PID sits
8 bytes after the VID. Only 4 bytes are modified (the VID and PID values).