# Rocksmith Launcher

> **Alpha phase.** This project is in early development. Nothing here is stable; the CLI interface, configuration format, and roadmap may change at any time. Use at your own risk.

## About

A Linux launcher for Rocksmith 2014 Edition Remastered that takes care of the annoying parts of running the game under Proton/Wine:

- **Locate the game** automatically via Steam, or point it to any manual installation (Steam and non-Steam are equally supported).
- **Manage Proton/Wine runtimes** — detects runtimes already installed on your system; on-demand GE-Proton downloads are planned.
- **Launch the game** from a clean prefix with no manual intervention.
- **Apply patches out of the box** — the no-cable patch, CDLC management, and pre-launch audio setup are implemented directly by the launcher, reusing methods inspired by the relevant projects (see [Credits](#credits)).
- **Manage CDLC** — toggle the CDLC patch on/off, and manage your custom songs (view, remove, import) directly from the launcher.

All you need is the launcher and the game — nothing else. Steam and non-Steam games are supported on equal footing; for a non-Steam install you simply point the launcher at your game directory.

The core logic lives in a Qt-free C++ library (`rocklaunch-core`), exposed through a thin CLI (`rocklaunch-cli`) used to exercise every feature. A Qt/QML frontend (`rocklaunch`) is planned once the CLI features are stable.

## Roadmap

The project is built in small, self-contained phases — each one is usable on its own, so there is always something working:

| Phase | Goal | Required by |
|---|---|---|
| 0 | ✅ Project skeleton (CMake, core library + CLI, `IGameProfile`, logging) | — |
| 1 | ✅ Profile management and manual game paths | Phase 2/3 |
| 2 | ✅ Proton/Wine runtime manager (local discovery and per-profile selection) | Phase 3 |
| 3 | ✅ Launch the game from a clean prefix (no patches/audio yet) | Phase 4 |
| 4 | No-cable patch: guitar input as the original Real Tone cable (toggleable) | Phase 6 |
| 5 | CDLC patch (enable/disable) | Phase 6 |
| 6 | First Qt/QML GUI | Phase 7 |
| 7 | CDLC songs manager (CLI `dlc list/add/delete` + GUI) | — |
| 8 | Runtime downloader (`runtime install` — fetch GE-Proton from GitHub releases) | — |
| 9 | Steam game auto-detection (`SteamSource` via `libraryfolders.vdf`) | — |

Phases 8 and 9 are the least urgent: the typical user already has Proton installed
(via ProtonUp-Qt) and runs the game outside Steam, so they are not needed by any
other phase.

## Profiles

A profile is one independently configured Rocksmith installation. It owns its game
directory, runtime selection, prefix, and patch settings. The same game directory
cannot be assigned to more than one profile.

The launcher configuration is separate from profiles and only stores launcher-wide
settings. Future launches use the explicit form `rocklaunch-cli launch <profile>`.

Profiles are created with `profile new` (auto-named `<game>-<n>` when no name is
given) and pointed at a game directory with `set-path`. See the CLI help for the
full command list.

## Patches

Patches are per-profile, toggleable, and apply **just before the game launches** —
they are runtime-only and never modify the game install. Both are implemented by
the launcher itself (methods inspired by the projects in [Credits](#credits)), so
you only provide the game.

Each patch has a design document explaining the method, what was tried, and how it
will be applied:

- **No Cable Patch** — makes a regular audio input (guitar line-in, audio
  interface, mic) look like the original Real Tone cable inside the game. The
  launcher resolves the chosen input to its VID/PID and the injected DLL patches
  the expected values in the game's memory.
  📄 **[NoCablePatch.md](./NoCablePatch.md)**
- **CDLC Patch** — lets the game load custom songs (`.psarc`) by patching the
  signature check. Independent of the no-cable patch: you can enable one without
  the other.
  📄 **[CustomDLC.md](./CustomDLC.md)**

Both patches share the same injected proxy DLL and a single env-var protocol
(`RS_PATCH_*`) between the launcher and the DLL — the launcher decides what to
apply before launching, and the DLL applies exactly that.

## Architecture & Development

Want to contribute? All the details live in two documents so this README doesn't turn into a wall of text:

- 📐 **[CODING_GUIDELINES.md](./.github/CODING_GUIDELINES.md)** — the most important one if you want to understand the project deeply: architecture, folder structure, C++/QML conventions, and how development actually works here.
- 📖 **[CONTRIBUTING.md](./.github/CONTRIBUTING.md)** — the rules to follow when opening issues and PRs.
- 🤖 **[AI_POLICY.md](./.github/AI_POLICY.md)** — what to expect and what is expected when contributing with AI-assisted work.

## Design principles

- Steam and non-Steam installations are supported on equal footing — for non-Steam games you simply point the launcher at the game directory. The launcher only checks that the expected files exist.
- Each game installation belongs to exactly one profile. Patches, runtime choice, and
  prefix state are profile-specific; launcher-wide preferences remain global.
- The launcher reimplements the relevant methods (no-cable patch, CDLC management, pre-launch audio setup) itself, inspired by existing projects — no `launcher.exe -> game.exe` chains, and no extra files from the user. You only provide the game.
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

- **[NoCableLauncher](https://github.com/Maxx53/NoCableLauncher)** — no-cable patch
- **[RSCDLCInstaller](https://github.com/rscustom/RSCDLCEnabler)** — CDLC management
