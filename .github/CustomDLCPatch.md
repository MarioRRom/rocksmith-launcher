# CDLC Patch — Design Notes

**Status:** WIP. The game rejects custom song packs (`.psarc` not signed by
Ubisoft) — we need to bypass that check.

## Alternatives

Two paths under consideration, neither committed yet:

- **Use Lovrom8's DLL** — grab the enabler from
  [Lovrom8/RSCDLCEnabler-TooManyCoresFix](https://github.com/Lovrom8/RSCDLCEnabler-TooManyCoresFix)
  and have the launcher manage it. The repo has no license, so the launcher
  would either download it on demand from GitHub (same as the user would do
  manually) or the user provides the DLL themselves.
- **User provides the DLL** — same "bring your own file" pattern used elsewhere
  in the project.

Building our own DLL is discarded — too much complexity for something that
already exists and works.

## Open questions

- How is the DLL installed? Just drop the file, or does it need a config file?
- Does the >32-thread crash fix in Lovrom8's fork apply under Wine/Proton?

## CDLC song management

Managing the actual `.psarc` song files (list, add, remove) is separate from the
patch and planned for phase 7. The PSARC reader/writer in `psarc_util.cpp`
(reused from the Direct Connect patch) will handle extracting song metadata.
