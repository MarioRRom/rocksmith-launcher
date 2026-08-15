# No Cable Patch — Design Notes (Phase 4)

**Status:** method decided, PoC pending (blocked on mingw-w64). Companion doc:
`CustomDLC.md` — the CDLC half of the same injected DLL.

## Inspiration

[No Cable Launcher](https://github.com/ztone16/NoCableLauncher) (Windows). We do
not fork or delegate to it — we reimplement the method at launcher level, credited
in the README.

## What I tried first (and why it failed)

**Virtual audio device over PipeWire — rejected, tested.** The idea: expose a
virtual node that *is* the Real Tone cable to the game
(`device.vendor.id=12BA`, `device.product.id=00FF`) and route the chosen
interface's audio into it. Result on the author's system:

- `Audio=pulse`: `winepulse.drv` enumerates **0** capture devices in-game.
- `Audio=alsa`: `winealsa.drv` only sees real ALSA cards — PipeWire/Pulse virtual
  nodes never reach it.

Wine cannot fabricate devices; it only exposes what the host audio stack provides.
The virtual-device route is a dead end here. **The memory write is the path.**

## How the game detects the cable

Rocksmith 2014 compares the audio endpoint's VID/PID against a **hardcoded value
in its own executable memory**. The Real Tone cable values live in the code as
VID `12BA` (bytes `BA 12`) and PID `00FF` (bytes `FF 00`).

Under Wine the game reads the real device's VID/PID fine (the author's mic,
SmartlinkTech `301A:5555`, was compared in-game correctly) — the only problem is
that the *expected* values are hardcoded to the original cable.

**The fix:** overwrite those expected VID/PID bytes in the running process with
the selected device's VID/PID. The game then compares the real device against the
patched values → match → the input is accepted as the cable.

### Memory values touched

- **Just 4 bytes:** the 2 expected VID bytes + the 2 expected PID bytes
  (little-endian).
- **How they're found:** scan the game's module image for the Real Tone VID/PID
  pattern `BA 12 92 0A 10 C0 11 C0 FF 00`. The PID sits 8 bytes after the VID.
- **The patch re-applies on every game launch — mandatory.** It's a memory-only
  write; the on-disk binary is never touched. Each launch reloads the original
  bytes, so the write must run every time the launcher starts the game.
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

## Implementation plan

**One injected proxy DLL, shared with the CDLC patch (phase 5).** No ptrace, no
external processes, no `launcher.exe -> game.exe` chains.

- Inject a proxy DLL (`D3DX9_42.dll`, 32-bit, re-forwarding its 329 exports to the
  real one in `system32`) into the game's process.
- On load, the DLL **reads environment variables** the launcher set before `exec`:
  `RS_PATCH_NO_CABLE` (on/off) and `RS_PATCH_VID` / `RS_PATCH_PID` (what to write).
- If `RS_PATCH_NO_CABLE=1` it performs the memory write above; otherwise it does
  nothing but forward.
- **The DLL never enumerates devices or resolves names.** The launcher resolves the
  chosen input to VID/PID before launching and passes it via env.

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

### Profile data

The patch lives under `patches.no_cable` in the profile JSON:
- `enabled` (bool): whether the patch is active.
- `player1_audio_device` (string): player 1's audio input (your guitar). Empty = not configured.
- `player2_audio_device` (string): player 2's input (multiplayer). Empty = not configured.
- `multiplayer` (bool): load the second device. Must differ from player 1's.

`player1/2_audio_device` store a human-readable input identifier (name or
ALSA/PipeWire id) that the launcher resolves to VID/PID at launch time. The patch
is runtime-only: it doesn't modify the game install, runs before the game, and is
idempotent.

### Integration with `launch`

Right before the phase-3 `exec`, in order:
1. **Verify `game_id`**: profile must be `rocksmith2014`; any other game never
   receives this patch.
2. **Skip when disabled**: `patches.no_cable.enabled == false` → nothing to do.
3. **Validate settings**: when enabled, `player1_audio_device` must be set and
   resolvable to VID/PID. Multiplayer requires a second, distinct device.
4. **Apply**: resolve VID/PID → set env vars → inject the DLL.
5. **Warnings, not hard failures**: if a device vanished or can't resolve, warn
   and let the game launch anyway.
6. **Log everything** to `logs/rocklaunch.log`.
