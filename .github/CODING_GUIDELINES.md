# Rocksmith Launcher — Coding Guidelines

Keep the codebase readable, consistent, and easy to review. These are not meant to restrict contributors — they exist to keep the project coherent as it grows.

**English** is the default language: code, identifiers, comments, commit messages, and documentation.

---

## Architecture

The project is layered so every feature can be tested without a GUI:

```
rocklaunch-core/     Business logic. No Qt dependencies.
rocklaunch-cli/      Thin CLI exercising the core.
rocklaunch/          (phase 6) Qt/QML frontend consuming the core.
```

Nothing outside the core implements game detection, runner handling, patching, or CDLC. Tests cover the core via the CLI.

**Key abstractions** (defined from phase 0):

- `IGameProfile` — behavior of a supported game (`Id`, `ValidateInstall`, `RequiredEnv`, `Executable`).
- `ILaunchPatch` — patch interface (`Id`, `GameId`, `Preset`, `IsEnabled`, `Apply`, `Remove`).
- `GameSource` — locates the game (`SteamSource`, `ManualSource`). No scattered conditional logic.
- `IRunnerSource` — discovers local Wine/Proton runners. `RunnerManager` merges sources.

**Profiles** (`ProfileConfig`) represent one installation of a game with its own path, runner, prefix, and patch settings. The same install directory cannot belong to more than one profile.

**Launcher reimplements** the methods of relevant projects at launcher level — no forks, no `launcher.exe -> game.exe` chains, no files required from the user. CDLC is the exception under evaluation (see `.github/CustomDLCPatch.md`). Inspiration credited in the README.

**Data paths:**

```
~/.config/rocksmith-launcher/
    config.json                        launcher-wide settings
                                       (future: runner defaults, colors, language)
~/.local/share/rocksmith-launcher/
    profiles/<profile_id>.json         profile state (game, runner, patches)
    prefixes/<profile_id>/             WINEPREFIX / STEAM_COMPAT_DATA_PATH
    runners/                           downloaded GE-Proton versions
    patches/                           cached downloadable patches
    logs/
```

---

## C++ Style

Follow the [Qt Group Coding Conventions](https://wiki.qt.io/Coding_Conventions). This project adds:

- Interfaces use the `I` prefix (`IGameProfile`, `ILaunchPatch`).
- Private members use the `m_` prefix.
- Use `std::filesystem` (`fs`) for paths.
- `#pragma once` as include guard.

---

## Comments

Comments explain intent, not obvious implementation details. Assume the reader already understands the language syntax.

**Good** — explains why:

```cpp
// Only the expected files matter; the launcher does not care how the install was obtained.
bool Rocksmith2014RemasteredProfile::ValidateInstall(const fs::path &installDir) const
```

**Bad** — restates what the code does:

```cpp
// Increment counter by one
counter++
```

For compact or minor sections, use a simple separator with a blank line above:

```cpp
// Profile commands

// Path assignment
```

---

## Config (JSON)

Per-installation configuration lives in `profiles/<profile_id>.json` (game ID, install path, runner, patches). Launcher-wide settings live in `config.json`. Keep keys descriptive and predictable; new settings are exposed through the CLI first.
