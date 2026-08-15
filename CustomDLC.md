# CDLC Patch — Design Notes (Phase 5)

**Status:** method decided, PoC pending (blocked on mingw-w64). Companion doc:
`NoCablePatch.md` — the no-cable half of the same injected DLL.

## Inspiration

[RSCDLCEnabler](https://github.com/phobos2077/RS2014-CDLC-Installer), the hook
component of phobos2077's Windows CDLC installer. We do not fork or delegate to
it — we reimplement the method at launcher level, credited in the README.

## What I tried first (and why it matters)

- The original installer is a Windows GUI whose only job is copying one DLL into
  the game folder. **Its quirk:** it locates the game via the Steam path, so
  non-Steam installs (like the author's) can't use it as-is. That's exactly the
  case our launcher solves.
- The hook worked fine under Wine in the author's install (game folder, no
  `WINEDLLOVERRIDES`), confirming the proxy-DLL mechanism works in our environment.

**Why the enabler exists:** the game verifies the signature of every song pack
and rejects custom ones. The fix makes `verify_signature` always report a valid
signature, accepting CDLC songs (`.psarc`).

## How the enabler works

Two pieces in one DLL:

1. **Proxy:** the DLL is named `D3DX9_42.dll` and sits in the game folder. Windows
   (and Wine) prefer the executable's directory in the DLL search order, so the
   hook **shadows** the real `D3DX9_42.dll` from `system32` without touching the
   system. All its functions (329 exports, 32-bit) re-forward to the real DLL: the
   game runs normally, just with our `DllMain` loaded.
2. **Patch:** on load, the DLL scans a fixed range of the game's module image for
   the `verify_signature` call site and overwrites the byte that decides whether
   the signature was valid, forcing the result to "valid".

### Memory values touched

- **Just 2 bytes.** The DLL locates `verify_signature`'s call site and overwrites
  the 2 bytes that store the verification result (a 2-byte `mov` that leaves the
  result "invalid") with the 2 bytes equivalent to "valid". The following `ret`
  stays intact.
- **How it's found:** a pattern scan over the game's module image. The pattern is
  the `verify_signature` call bytes, with the 4-byte call displacement as a
  wildcard. The scan range is hardcoded to the game's expected module base (no
  ASLR on that image) — proven under Wine on the author's system.
- **Idempotent:** re-writing the same 2 bytes is a no-op.
- The write uses page-protection → copy → instruction-cache flush → restore. A
  standard in-process patching technique.

## Implementation plan

**One injected proxy DLL, shared with the no-cable patch (phase 4).** No ptrace,
no external processes, no `launcher.exe -> game.exe` chains.

- Inject the proxy DLL (`D3DX9_42.dll`, 32-bit, 329 exports re-forwarding to the
  real one in `system32`) into the game's process.
- On load, the DLL **reads environment variables** the launcher set before `exec`:
  `RS_PATCH_CDLC` (on/off) decides whether to apply the signature patch.
- If `RS_PATCH_CDLC=1` it performs the `verify_signature` memory write above;
  otherwise it does nothing but forward.
- The DLL also reads the no-cable env vars (`RS_PATCH_NO_CABLE`, `RS_PATCH_VID`,
  `RS_PATCH_PID`) — same DLL, both patches independent.

### Env var contract (launcher ↔ DLL)

| Env var | Value | Effect |
|---|---|---|
| `RS_PATCH_CDLC` | `"1"` / `"0"` | Apply the `verify_signature` CDLC patch |
| `RS_PATCH_NO_CABLE` | `"1"` / `"0"` | Apply the no-cable memory write (see `NoCablePatch.md`) |
| `RS_PATCH_VID` | 4 hex chars, e.g. `"301A"` | VID to write (no-cable) |
| `RS_PATCH_PID` | 4 hex chars, e.g. `"5555"` | PID to write (no-cable) |

Rules:
- The launcher validates before injecting: `RS_PATCH_NO_CABLE=1` requires valid
  VID/PID; unset or unknown values = disabled.
- No patch requested → the launcher doesn't even inject the DLL.
- The patches are independent: CDLC without no-cable and vice versa.

### Injection

- The DLL is placed in the game folder (shadowing the `system32` one) — that's how
  it worked in the author's install, without `WINEDLLOVERRIDES`. Adding
  `WINEDLLOVERRIDES` is optional (belt-and-suspenders).
- The launcher manages the DLL's presence: it copies it in when at least one patch
  is enabled, and backs up / restores the game's original `D3DX9_42.dll` so no
  orphan hooks are left when both patches are disabled.

### Profile data

The patch lives under `patches.cdlc` in the profile JSON:
- `enabled` (bool): whether the patch is active.

### Integration with `launch`

Right before the phase-3 `exec`, in order:
1. **Verify `game_id`**: profile must be `rocksmith2014`; any other game never
   receives this patch.
2. **Skip when disabled**: `patches.cdlc.enabled == false` → nothing to do.
3. **Apply**: set env vars → inject the DLL.
4. **Log everything** to `logs/rocklaunch.log`.

## Credits

Inspiration: [phobos2077/RS2014-CDLC-Installer](https://github.com/phobos2077/RS2014-CDLC-Installer)
— the `RSCDLCEnabler` project. Reimplemented at launcher level per the project's
design principles; no forks or external executables.
