# No Cable Patch — Design Notes (Phase 4)

**Status:** method changed, PoC pending. The memory-write approach was superseded
by **Direct Connect Mode** via a one-time `cache.psarc` edit — no DLL, no memory
patching, no mingw-w64 toolchain for this patch. The old design is kept below as
**Plan B**. Companion doc: `CustomDLC.md` — no longer shares infrastructure with
this patch (see "What changed and why").

## What changed and why

Community feedback (Reddit) pointed out that Rocksmith 2014 already contains a
**Direct Connect input mode**, planned for the original game and apparently left
in the Remastered codebase but disabled — reportedly gated behind a single data
change rather than being missing code. RSMods (Lovrom8) exposes this as a toggle:
enabling it adds an "Input > Direct Connect" option to the game's own menu, where
the player picks any input device directly. No VID/PID spoofing, no hardcoded Real
Tone cable comparison to defeat.

This is strictly better than the memory-write approach for our purposes:

- **No process memory patching.** The old method wrote 4 bytes into the running
  game's image on every launch. This method edits a game *data* file once.
- **No DLL, no injection, no `WINEDLLOVERRIDES`.** Removes the mingw-w64 toolchain
  requirement for this specific patch.
- **The game handles device selection itself**, via its own menu — the launcher
  doesn't need to resolve a chosen input to VID/PID at all.

## Inspiration

[RSMods](https://github.com/Lovrom8/RSMods) (Lovrom8) — the "Direct Connect Mode"
feature. We do not fork or delegate to it — we reimplement the specific
`cache.psarc` edit at launcher level, credited in the README.

## Mechanism

- The game reads its `cache.psarc` archive at startup for various packed data.
  Enabling Direct Connect Mode requires changing a specific entry inside that
  archive (a data flag, not executable code) — the exact entry needs to be
  extracted from the RSMods source.
- **Persistent, one-time edit**, unlike the old memory patch: once written, the
  change survives across launches (on-disk data, not a per-process runtime write).
  Applied once per install, not on every launch.
- `cache.psarc` is itself a PSARC archive (same container format as CDLC song
  packs) — extracting/editing/repacking it needs the same PSARC read/write code
  the CDLC manager (phase 7) will need anyway. Shared infrastructure, worth
  building once.

## Prefix requirement

Direct Connect Mode still needs the prefix to expose capture devices to WASAPI —
this part of the original design is unchanged:

```
wine reg add "HKCU\Software\Wine\Drivers" /v Audio /t REG_SZ /d alsa /f
```

Already applied by `EnsurePrefix` on every launch: `Audio=alsa` is injected for
**both** runner types — wine writes to `<prefix>`, proton to `<prefix>/pfx` (the
real WINEPREFIX the proton script uses) — enabling native detection of all audio
inputs in-game.

## To confirm before implementing

1. **Read `RSMods`' source** to identify exactly which entry/value inside the
   archive toggles Direct Connect Mode. Do not copy a pre-modified `cache.psarc`
   from anywhere — reimplement the specific change against the user's own archive,
   same principle as everything else in this project.
2. Confirm whether the edit is **version-sensitive** (different builds may pack
   `cache.psarc` slightly differently) — validate against the installed version
   before writing.
3. Anecdotal reports mention occasional audio artifacts ("cracking noise") with
   Direct Connect Mode on some setups — likely a buffer/latency tuning issue (same
   family as the `PIPEWIRE_LATENCY` tuning already done for launch), keep in mind
   when this reaches user testing.

## Implementation plan

1. **Backup**: before the first edit, copy the original `cache.psarc` to a backup
   location (atomic: write to `.tmp`, then rename — never leave the game folder in
   a half-written state if the launcher is killed mid-copy).
2. **Extract → edit → repack**: use the PSARC read/write library (shared with
   phase 7) to open `cache.psarc`, change the identified entry, write it back.
3. **Idempotent**: if the entry is already set correctly, skip re-writing.
4. **Restore**: disabling the patch restores the backed-up original file.
5. This is a reversible **install-level modification**, not runtime-only — a
   deliberate difference from the old design.

### Profile data

```json
"patches": {
    "direct_connect": {
        "enabled": false
    }
}
```

No `player1_audio_device` / `player2_audio_device`, no VID/PID resolution — the
game's own Input menu handles device selection once the mode is enabled.
Multiplayer device selection is handled in-game, not by the launcher.

### Integration with `launch`

Right before the phase-3 `exec`, in order:
1. **Verify `game_id`**: profile must be `rocksmith2014remastered`; any other game never
   receives this patch.
2. **Skip when disabled**: `patches.direct_connect.enabled == false` → nothing to
   do (and restore the original `cache.psarc` if a previous enable left it
   patched).
3. **Apply once, not per-launch**: check whether `cache.psarc` already reflects the
   desired state before touching it — this differs from the old design, where the
   memory write had to run on every single launch.
4. **Log everything** to `logs/rocklaunch.log`.

---

## Plan B (superseded, kept for reference) — memory-write approach

The original design: overwrite the hardcoded Real Tone cable VID/PID inside the
running game's memory via an injected proxy DLL (`WINEDLLOVERRIDES`), so the
chosen input's real VID/PID matches what the game expects. Kept here in case
Direct Connect Mode ever becomes unavailable or incompatible with a future game
update.

### How the game detects the cable

Rocksmith 2014 compares the audio endpoint's VID/PID against a **hardcoded value
in its own executable memory**. The Real Tone cable values live in the code as
VID `12BA` (bytes `BA 12`) and PID `00FF` (bytes `FF 00`).

Under Wine the game reads the real device's VID/PID fine (the author's mic,
SmartlinkTech `301A:5555`, was compared in-game correctly) — the only problem is
that the *expected* values are hardcoded to the original cable.

**The fix:** overwrite those expected VID/PID bytes in the running process with
the selected device's VID/PID. The game then compares the real device against the
patched values → match → the input is accepted as the cable.

### What I tried first (and why it failed)

**Virtual audio device over PipeWire — rejected, tested.** The idea: expose a
virtual node that *is* the Real Tone cable to the game
(`device.vendor.id=12BA`, `device.product.id=00FF`) and route the chosen
interface's audio into it. Result on the author's system:

- `Audio=pulse`: `winepulse.drv` enumerates **0** capture devices in-game.
- `Audio=alsa`: `winealsa.drv` only sees real ALSA cards — PipeWire/Pulse virtual
  nodes never reach it.

Wine cannot fabricate devices; it only exposes what the host audio stack provides.
The virtual-device route is a dead end here.

### Memory values touched

- **Just 4 bytes:** the 2 expected VID bytes + the 2 expected PID bytes
  (little-endian).
- **How they're found:** scan the game's module image for the Real Tone VID/PID
  pattern `BA 12 92 0A 10 C0 11 C0 FF 00`. The PID sits 8 bytes after the VID.
- **The patch re-applies on every game launch — mandatory.** It's a memory-only
  write; the on-disk binary is never touched.
- **Only the offsets are cached between launches** (the pattern location is stable
  because the binary and its image base don't change) — the expensive scan runs
  once, later launches write directly at the cached position.
- **Idempotence check applies only within one process run** (e.g. multiplayer
  re-patching for player 2): after the first write the original pattern is gone,
  so a re-scan would find nothing — write straight to the cached offsets.

### Prefix requirement (cannot be skipped)

The prefix must expose capture devices to WASAPI, or the game sees no microphone
and the patch is pointless. `EnsurePrefix` applies **both** keys on every launch:

- `Audio=alsa` (forces `winealsa.drv`) — `winepulse.drv` enumerated 0 capture
  devices; `Audio=alsa` enumerates them all.
- `dsound=alsa` — a separate key, part of the same fix.

### Env var contract (launcher ↔ DLL)

| Env var | Value | Effect |
|---|---|---|
| `RS_PATCH_NO_CABLE` | `"1"` / `"0"` | Apply the memory write (overwrite expected VID/PID) |
| `RS_PATCH_VID` | 4 hex chars, e.g. `"301A"` | VID to write |
| `RS_PATCH_PID` | 4 hex chars, e.g. `"5555"` | PID to write |
| `RS_PATCH_CDLC` | `"1"` / `"0"` | Apply the CDLC `verify_signature` patch (see `CustomDLC.md`) |

Rules:
- The launcher validates before injecting: `RS_PATCH_NO_CABLE=1` requires valid
  VID/PID; unset or unknown values = disabled.
- No patch requested → the launcher doesn't even inject the DLL.

### Injection

The DLL (`D3DX9_42.dll`, 32-bit, re-forwarding its 329 exports to the real one in
`system32`) is placed in the game folder, shadowing the `system32` one. On load it
reads the env vars above and applies only what was asked. The DLL never enumerates
devices or resolves names — the launcher resolves the chosen input to VID/PID
before launching and passes it via env.

### Profile data (old shape)

Under `patches.no_cable`:
- `enabled` (bool): whether the patch is active.
- `player1_audio_device` (string): player 1's audio input (your guitar). Empty = not configured.
- `player2_audio_device` (string): player 2's input (multiplayer). Empty = not configured.
- `multiplayer` (bool): load the second device. Must differ from player 1's.
