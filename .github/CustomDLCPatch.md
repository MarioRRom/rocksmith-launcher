# CDLC Patch

**Status:** implemented.

## What it does

Lets the game load custom songs (`.psarc` not signed by Ubisoft) by dropping the
`D3DX9_42.dll` enabler into the game directory.

## How it works

The game refuses to load unsigned `.psarc` files — the enabler DLL hooks the
signature check and bypasses it. The launcher manages the DLL transparently:

1. **`patch add cdlc-enabler`** — downloads the enabler from
   [Lovrom8/RSCDLCEnabler-TooManyCoresFix](https://github.com/Lovrom8/RSCDLCEnabler-TooManyCoresFix)
   (once, cached in `~/.local/share/rocksmith-launcher/patches/cdlc-enabler/`)
   and copies it next to the game executable.
2. **`patch remove cdlc-enabler`** — deletes the DLL from the game directory.
3. **`patch add cdlc-enabler -f`** — forces re-download of the enabler.

## Cache

`~/.local/share/rocksmith-launcher/patches/cdlc-enabler/D3DX9_42.dll` stores the
downloaded binary. Once cached, subsequent `patch add` calls apply from cache
without hitting the network. The `-f` flag forces a fresh download.

## Project files

- **[cdlc_patch.cpp](../rocklaunch-core/src/patches/cdlc_patch.cpp)** — patch logic: download, cache, deploy, remove.
- **[downloader.cpp](../rocklaunch-core/src/utils/downloader.cpp)** — `Fetch()` via `curl` subprocess.

## Thanks

This method exists thanks to [Lovrom8](https://github.com/Lovrom8). He
personally recommended using his
[RSCDLCEnabler-TooManyCoresFix](https://github.com/Lovrom8/RSCDLCEnabler-TooManyCoresFix)
repo over the original geneccx fork — maintained, with the >32-thread crash fix.
The enabler DLL is his work; the launcher simply manages it at launcher level,
credited in the README.

---

## Future plans

- **Multiple enabler sources** — allow choosing between different DLL providers.
- **Manual DLL import** — let the user provide their own enabler binary.
- **CDLC song management** — list, add, remove `.psarc` song files (separate from
  the patch, planned for phase 7). The PSARC reader/writer in `psarc_util.cpp`
  will handle extracting song metadata.
