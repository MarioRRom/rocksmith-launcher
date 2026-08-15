# CDLC Patch — Design Notes (Phase 5)

**Status: WIP, undecided.** Two possible paths, neither committed yet. Picking
one is blocked on a few open questions below. Companion doc: `NoCablePatch.md`
(no longer related — that patch doesn't need a DLL anymore).

## The problem

Rocksmith 2014 rejects custom song packs (CDLC, `.psarc` files not signed by
Ubisoft) — it checks a signature and refuses anything that doesn't pass. To
support CDLC we need to make the game accept them anyway.

## Plan A — use Lovrom8's DLL directly

Grab the enabler DLL from
[Lovrom8/RSCDLCEnabler-TooManyCoresFix](https://github.com/Lovrom8/RSCDLCEnabler-TooManyCoresFix)
(an actively maintained fork of the original CDLC enabler) and have the launcher
manage it — download it, drop it in the game folder, done.

**Pros:** way less work, no need to build anything ourselves, maintained by
someone else.

**Open questions / risks:**
- **License.** The repo (and the original it's based on) doesn't declare one, so
  we can't just copy the DLL into our own repo and hand it out ourselves. Two
  ways around that:
  - The launcher **downloads it straight from the GitHub repo** on demand,
    instead of bundling it — never our own copy, just automating what the user
    would click themselves.
  - Or the **user provides the DLL themselves** (downloads it manually, points
    the launcher at it) — same "bring your own file" pattern already used
    elsewhere in the project.
- **The >32-thread crash fix this build adds might not even be relevant to us.**
  Still checking whether that specific fix applies under Wine/Proton or if it's
  a Windows-only quirk — undecided, needs more digging before it factors into
  the choice.
- Still need to confirm exactly how the DLL is meant to be installed/activated
  (just drop the file in, or does it need a config file too).

## Plan B — build our own DLL

Write and inject our own proxy DLL that patches the signature check ourselves,
based on how the original enabler works.

**Pros:** full control, no dependency on someone else's project staying online
or maintained, no license question at all.

**Cons:** building a DLL like this from scratch is real work — cross-compiling
with mingw-w64, finding the right memory pattern to patch, getting the injection
right, testing it actually works on the real game. Basically redoing what
already exists elsewhere, just to sidestep a license gray area that's probably
low-risk in practice anyway.

## Decision

Not made yet. Leaning would depend on how annoying Plan A's open questions turn
out to be once actually investigated — if the download-on-demand approach is
clean and the DLL just works, no reason to redo that work in Plan B.
