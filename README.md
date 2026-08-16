# Rocksmith Launcher

> **Alpha phase.** This project is in early development. Nothing here is stable; the CLI interface, configuration format, and roadmap may change at any time. Use at your own risk.

## About

A Linux launcher for Rocksmith 2014 Edition Remastered that takes care of the annoying parts of running the game under Proton/Wine:

- **Locate the game** automatically via Steam, or point it to any manual installation (Steam and non-Steam are equally supported).
- **Manage Proton/Wine runners** — detects runners already installed on your system; on-demand GE-Proton downloads are planned.
- **Launch the game** from a clean prefix with no manual intervention.
- **Apply patches out of the box** — the no-cable patch and pre-launch audio setup are implemented directly by the launcher, reusing methods inspired by the relevant projects; the CDLC enabler approach is under evaluation (see [Credits](#credits)).
- **Manage CDLC** — toggle the CDLC patch on/off, and manage your custom songs (view, remove, import) directly from the launcher.

All you need is the launcher and the game — nothing else. Steam and non-Steam games are supported on equal footing; for a non-Steam install you simply point the launcher at your game directory.

The core logic lives in a Qt-free C++ library (`rocklaunch-core`), exposed through a thin CLI (`rocklaunch-cli`) used to exercise every feature. A Qt/QML frontend (`rocklaunch`) is planned once the CLI features are stable.

## Roadmap

The project is built in small, self-contained phases — each one is usable on its own, so there is always something working:

| Phase | Goal | Required by |
|---|---|---|
| 0 | ✅ Project skeleton (CMake, core library + CLI, `IGameProfile`, logging) | — |
| 1 | ✅ Profile management and manual game paths | Phase 2/3 |
| 2 | ✅ Proton/Wine runner manager (local discovery and per-profile selection) | Phase 3 |
| 3 | ✅ Launch the game from a clean prefix (no patches/audio yet) | Phase 4 |
| 4 | No-cable patch: enable the game's Direct Connect input mode so any audio input works (toggleable) | Phase 6 |
| 5 | CDLC patch (enable/disable) — method under evaluation | Phase 6 |
| 6 | First Qt/QML GUI | Phase 7 |
| 7 | CDLC songs manager (CLI `cdlc list/add/remove` + GUI) | — |
| 8 | Runner downloader (`runner install` — fetch GE-Proton from GitHub releases) | — |
| 9 | Steam game auto-detection (`SteamSource` via `libraryfolders.vdf`) | — |

Phases 8 and 9 are the least urgent: the typical user already has Proton installed
(via ProtonUp-Qt) and runs the game outside Steam, so they are not needed by any
other phase.

## Profiles

A profile is one independently configured Rocksmith installation. It owns its game
directory, runner selection, prefix, and patch settings. The same game directory
cannot be assigned to more than one profile.

The launcher configuration is separate from profiles and only stores launcher-wide
settings. Future launches use the explicit form `rocklaunch-cli launch <profile>`.

Profiles are created with `profile new` (auto-named `<game>-<n>` when no name is
given) and pointed at a game directory with `set-path`. See the CLI help for the
full command list.

## Patches

Patches are per-profile and toggleable. They apply **just before the game
launches**, are game-specific (a profile of another game never receives Rocksmith
patches), and each has a design document explaining the method and what was tried:

- **No Cable Patch** — enables the game's built-in **Input > Direct Connect** mode
  with a one-time, reversible edit to the game's `cache.psarc` (a data flag, not
  code). Once enabled, the game lets you pick any audio input (guitar line-in,
  interface, mic) directly in its own menu — no DLL, no VID/PID spoofing.
  📄 **[NoCablePatch.md](./NoCablePatch.md)**
- **CDLC Patch** — lets the game load custom songs (`.psarc`) by getting past the
  signature check. **Method not decided yet**: options under evaluation are managing
  a maintained third-party enabler DLL (downloaded on demand or provided by the
  user) or building our own. Independent of the no-cable patch — you can enable one
  without the other.
  📄 **[CustomDLC.md](./CustomDLC.md)**

The two patches are independent and share no infrastructure — the no-cable patch
involves no DLL at all.

## Architecture & Development

Want to contribute? All the details live in two documents so this README doesn't turn into a wall of text:

- 📐 **[CODING_GUIDELINES.md](./.github/CODING_GUIDELINES.md)** — the most important one if you want to understand the project deeply: architecture, folder structure, C++/QML conventions, and how development actually works here.
- 📖 **[CONTRIBUTING.md](./.github/CONTRIBUTING.md)** — the rules to follow when opening issues and PRs.
- 🤖 **[AI_POLICY.md](./.github/AI_POLICY.md)** — what to expect and what is expected when contributing with AI-assisted work.

## Design principles

- Steam and non-Steam installations are supported on equal footing — for non-Steam games you simply point the launcher at the game directory. The launcher only checks that the expected files exist.
- Each game installation belongs to exactly one profile. Patches, runner choice, and
  prefix state are profile-specific; launcher-wide preferences remain global.
- The launcher reimplements the relevant methods (no-cable patch, pre-launch audio setup) itself, inspired by existing projects — no `launcher.exe -> game.exe` chains, and no extra files from the user. **CDLC is the one exception under evaluation:** the enabler binary may be downloaded from upstream on demand or provided by the user, since its upstream has no license (see [CustomDLC.md](./CustomDLC.md)).
- Detection is preferred over download: if you have Steam, use the Proton/GE-Proton already installed before implementing a downloader.

## Building

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Usage

The CLI binary lives at `build/rocklaunch-cli/rocklaunch-cli` after building.
Configuration and logs are created automatically on first run under
`~/.config/rocksmith-launcher/` and `~/.local/share/rocksmith-launcher/`.

```sh
# Show all commands
./build/rocklaunch-cli/rocklaunch-cli --help
```

## License

GNU General Public License v3.0 — see [LICENSE](./LICENSE).

The methods implemented by this launcher are *inspired by* the projects listed below
and reimplemented from scratch. See [Credits](#credits).

## Credits

The methods implemented by this launcher are inspired by the following projects. Links are added as each method is integrated:

- **[RSMods](https://github.com/Lovrom8/RSMods)** — Direct Connect Mode (no-cable patch)
- **[RSCDLCEnabler](https://github.com/geneccx/RSCDLCEnabler)** — CDLC enabler (original concept)
- **[RSCDLCEnabler-TooManyCoresFix](https://github.com/Lovrom8/RSCDLCEnabler-TooManyCoresFix)** — maintained CDLC enabler fork (method under evaluation; no license — downloaded on demand or user-provided, never redistributed)
- **[RS2014-CDLC-Installer](https://github.com/phobos2077/RS2014-CDLC-Installer)** — CDLC management
