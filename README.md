<h1 align="center">Rocksmith Launcher</h1>
<p align="center">Play Rocksmith 2014 on Linux without the headaches</p>

> [!CAUTION]
> **Alpha phase.** This project is in early development. The CLI interface,
> configuration format, and roadmap may change at any time.

## What it does

A Linux launcher for Rocksmith 2014 Edition Remastered that handles the annoying
parts of running the game under Proton/Wine — game detection, runner management,
prefix setup, patches, and audio configuration. You bring the game, the launcher
does the rest.

- **Locate the game** via Steam or point it to any manual installation.
- **Manage runners** — detects installed Proton/Wine; GE-Proton downloader planned.
- **Launch** from a clean prefix, no manual intervention.
- **Patches out of the box** — Direct Connect and audio setup included.
- **Manage CDLC** — toggle the CDLC patch, view/remove/import custom songs.

## Current state

- [x] Profile management and manual game paths
- [x] Proton/Wine runner detection and per-profile selection
- [x] Launch the game from a clean prefix
- [x] Direct Connect patch — plug any audio interface or mic, no Real Tone Cable needed
- [x] CDLC patch (enable/disable) — deploy/remove the CDLC enabler DLL
- [ ] GE-Proton downloader
- [ ] Qt/QML GUI
- [ ] CDLC songs manager
- [ ] Steam game auto-detection

## Patches

- **[Direct Connect](./.github/DirectConnectPatch.md)** — enables the game's
  hidden input mode so any audio interface or mic works directly. No Real Tone
  Cable, no DLL, no spoofing. Tested with a cheap Cube Baby interface, works
  great.
- **[CDLC](./.github/CustomDLCPatch.md)** — lets the game load custom songs by
  deploying the CDLC enabler DLL into the game folder. Togglable.

## Prerequisites

**Build:** CMake, C++17 compiler, OpenSSL, zlib,
[nlohmann/json](https://github.com/nlohmann/json) (fetched automatically).

**Runtime:** `7z` ([p7zip](https://p7zip.sourceforge.net/)) —
`sudo apt install p7zip-full` / `sudo pacman -S p7zip`.
`curl` — usually pre-installed on Linux.

## Building

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

### Usage

```sh
./build/rocklaunch-cli/rocklaunch-cli --help
```

Config and logs are created automatically on first run under
`~/.config/rocksmith-launcher/` and `~/.local/share/rocksmith-launcher/`.

### Contributing

- 📐 **[Coding Guidelines](./.github/CODING_GUIDELINES.md)** — architecture, conventions, folder structure.
- 📖 **[Contributing](./.github/CONTRIBUTING.md)** — rules for issues and PRs.
- 🤖 **[AI Policy](./.github/AI_POLICY.md)** — guidelines for AI-assisted contributions.

## License

GNU General Public License v3.0 — see [LICENSE](./LICENSE).

## Credits

The methods implemented by this launcher are inspired by the following projects. Links are added as each method is integrated:

- **[RSMods](https://github.com/Lovrom8/RSMods)** — Direct Connect Mode
- **[RSCDLCEnabler](https://github.com/geneccx/RSCDLCEnabler)** — CDLC enabler (original concept)
- **[RSCDLCEnabler-TooManyCoresFix](https://github.com/Lovrom8/RSCDLCEnabler-TooManyCoresFix)** — maintained CDLC enabler fork
