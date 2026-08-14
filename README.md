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
| 0 | ✅ Project skeleton (CMake, core library + CLI, `IGameProfile`, logging) | — |
| 1 | ✅ Profile management and manual game paths | Phase 2/3 |
| 2 | Proton/Wine runtime manager (list, install GE-Proton) | Phase 3 |
| 3 | Launch the game from a clean prefix (no patches/audio yet) | Phase 4 |
| 4 | No-cable patch: guitar input as the original Real Tone cable (toggleable) | Phase 6 |
| 5 | CDLC patch (enable/disable) | Phase 6 |
| 6 | First Qt/QML GUI | Phase 7 |
| 7 | CDLC songs manager (CLI `dlc list/add/delete` + GUI) | — |

> **Steam detection:** this is intentionally separate future work. Phase 1 manages
> profiles and manually assigned game paths only; Steam auto-detection via
> `libraryfolders.vdf` will be implemented when a Steam installation is available
> for validation.

## Profiles

A profile is one independently configured Rocksmith installation. It owns its game
directory, runtime selection, prefix, and patch settings. The same game directory
cannot be assigned to more than one profile.

The launcher configuration is separate from profiles and only stores launcher-wide
settings. Future launches use the explicit form `rocklaunch-cli launch <profile>`.

The current CLI lifecycle is `profile new` (auto-named `<game>-<n>` if no name is given),
then `set-path <profile> <path>`. Use `profile show <profile>` to inspect an assignment,
`profile list` to view all profiles, and `profile delete <profile>` to remove a profile
configuration.

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

# Create a profile (auto-named rocksmith2014-1 when no name is given)
./build/rocklaunch-cli/rocklaunch-cli profile new

# Point a profile at a game installation (Steam or non-Steam; validates the game files)
./build/rocklaunch-cli/rocklaunch-cli set-path rocksmith2014-1 /path/to/Rocksmith2014

# Inspect, list, or delete profiles
./build/rocklaunch-cli/rocklaunch-cli profile show rocksmith2014-1
./build/rocklaunch-cli/rocklaunch-cli profile list
./build/rocklaunch-cli/rocklaunch-cli profile delete rocksmith2014-1
```

## Credits

The methods implemented by this launcher are inspired by the following projects. Links are added as each method is integrated:

- **[NoCableLauncher](https://github.com/Maxx53/NoCableLauncher)** — no-cable patch
- **[RSCDLCInstaller](https://github.com/rscustom/RSCDLCEnabler)** — CDLC management
