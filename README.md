# Rocksmith Launcher

> **Alpha phase.** This project is in early development. Nothing here is stable; the CLI interface, configuration format, and roadmap may change at any time. Use at your own risk.

## About

A Linux launcher for Rocksmith 2014 Edition Remastered that takes care of the annoying parts of running the game under Proton/Wine:

- **Locate the game** automatically via Steam, or point it to any manual installation (Steam and non-Steam are equally supported).
- **Manage Proton/Wine runtimes** — detects runtimes already installed on your system and can download GE-Proton versions on demand.
- **Launch the game** from a clean prefix with no manual intervention.
- **Apply patches out of the box** — the no-cable patch, CDLC management, and pre-launch audio setup are implemented directly by the launcher, reusing methods inspired by the relevant projects (see [Credits](#credits)).
- **Manage CDLC** — toggle the CDLC patch on/off, and manage your custom songs (view, remove, import) directly from the launcher.

All you need is the launcher and the game — nothing else. Steam and non-Steam games are supported on equal footing; for a non-Steam install you simply point the launcher at your game directory.

The core logic lives in a Qt-free C++ library (`rocklaunch-core`), exposed through a thin CLI (`rocklaunch-cli`) used to exercise every feature. A Qt/QML frontend (`rocklaunch`) is planned once the CLI features are stable.

## Roadmap

The project is built in small, self-contained phases — each one is usable on its own, so there is always something working:

| Phase | Goal | Required by |
|---|---|---|
| 0 | Project skeleton (CMake, core library + CLI, `IGameProfile`, logging) | — |
| 1 | Game detection (Steam `libraryfolders.vdf` + manual path) | Phase 2/3 |
| 2 | Proton/Wine runtime manager (list, install GE-Proton) | Phase 3 |
| 3 | Launch the game from a clean prefix (no patches/audio yet) | Phase 4 |
| 4 | No-cable patch: guitar input as the original Real Tone cable (toggleable) | Phase 6 |
| 5 | CDLC patch (enable/disable) | Phase 6 |
| 6 | First Qt/QML GUI | Phase 7 |
| 7 | CDLC songs manager (CLI `dlc list/add/delete` + GUI) | — |

## Architecture & Development

Want to contribute? All the details live in two documents so this README doesn't turn into a wall of text:

- 📐 **[CODING_GUIDELINES.md](./.github/CODING_GUIDELINES.md)** — the most important one if you want to understand the project deeply: architecture, folder structure, C++/QML conventions, and how development actually works here.
- 📖 **[CONTRIBUTING.md](./.github/CONTRIBUTING.md)** — the rules to follow when opening issues and PRs.
- 🤖 **[AI_POLICY.md](./.github/AI_POLICY.md)** — what to expect and what is expected when contributing with AI-assisted work.

## Design principles

- Steam and non-Steam installations are supported on equal footing — for non-Steam games you simply point the launcher at the game directory. The launcher only checks that the expected files exist.
- The launcher reimplements the relevant methods (no-cable patch, CDLC management, pre-launch audio setup) itself, inspired by existing projects — no `launcher.exe -> game.exe` chains, and no extra files from the user. You only provide the game.
- Detection is preferred over download: if you have Steam, use the Proton/GE-Proton already installed before implementing a downloader.

## Building

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Credits

The methods implemented by this launcher are inspired by the following projects. Links are added as each method is integrated:

- **[NoCableLauncher](https://github.com/Maxx53/NoCableLauncher)** — no-cable patch
- **[RSCDLCInstaller](https://github.com/rscustom/RSCDLCEnabler)** — CDLC management