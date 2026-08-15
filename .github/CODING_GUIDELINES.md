# Rocksmith Launcher — Coding Guidelines

This document describes the coding conventions, formatting rules, and architectural practices used throughout Rocksmith Launcher.

The objective is simple:

* keep the codebase readable
* preserve consistency between modules
* simplify long-term maintenance
* make contributions easier to review

These guidelines are not meant to restrict contributors unnecessarily.
They exist to keep the project coherent as it grows.

---

# Language

**English is the default language of the project**: code, identifiers, comments, commit messages, and documentation.

---

# Architecture

The project is layered so every feature can be tested and used before any GUI exists:

1. `rocklaunch-core/` — all business logic (game detection, runtimes, launch, patches, CDLC). **No Qt dependencies.**
2. `rocklaunch-cli/` — thin CLI exercising the core. Subcommands by phase: `profile list/new/show/delete`, `set-path`, `runtime list`, `runtime install`, `launch`, `dlc list/add/delete`.
3. `rocklaunch/` (phase 6) — Qt/QML frontend consuming the core's public API via thin QObject/Q_INVOKABLE wrappers.

Nothing outside the core implements game detection, runtime handling, patching, or CDLC. Tests cover the core via the CLI.

`IGameProfile` represents the behavior of a supported game. A user-facing profile is
instead a `ProfileConfig`: one installation of that game with its own identifier,
install directory, runtime, prefix, and patch settings. The same install directory
must not belong to more than one `ProfileConfig`.

Key abstractions, defined from phase 0:

- `IGameProfile` — `id()`, `validate_install()`, `required_env(LaunchContext)`, `executable()`.
- `ILaunchPatch` — `id()`, `is_enabled(Profile)`, `apply(prefix_dir, install_dir)`. Provisional — the final patch interface is decided in phases 4/5 (possibly split into `ICablePatch`/`ICDLCPatch`).
- `GameSource` — locates the game (`SteamSource` via `libraryfolders.vdf`, `ManualSource` by path). No scattered conditional logic.
- `IRuntimeSource` — discovers local Wine/Proton runtimes. `RuntimeManager` merges
  sources and owns runtime lookup; sources never mutate profile configuration.

**Current phase 1 scope:** profile lifecycle and manually assigned paths are
implemented and tested. `SteamSource` is intentionally separate future work until a
Steam installation is available for validation.

The launcher reimplements the methods of relevant projects (no-cable, CDLC, audio) at launcher level — no literal forks, no `launcher.exe -> game.exe` chains, no files required from the user. Inspiration is credited in the README (`# Credits`).

Data lives under XDG-friendly paths:

```
~/.config/rocksmith-launcher/         → config.json (launcher-wide settings)
                                      → profiles/<profile_id>.json (game instances)
~/.local/share/rocksmith-launcher/
  ├── prefixes/<profile_id>/          → WINEPREFIX / STEAM_COMPAT_DATA_PATH
  ├── runtimes/                       → downloaded GE-Proton versions
  └── logs/
```

Log to file from startup (`logs/`), for every launch flow.

---

# C++ Style

Follow the **Qt Group Coding Conventions** (https://wiki.qt.io/Coding_Conventions). On top of them, this project adds:

- Interfaces use the `I` prefix (`IGameProfile`, `ILaunchPatch`).
- Private members use the `m_` prefix.
- Use `std::filesystem` (`fs`) for paths.
- `#pragma once` as include guard.
- Comments follow the rules in the [Comments](#comments) section.

---

# Comments

Comments apply to C++ today and to QML once the GUI lands (phase 6). The rules are language-agnostic.

Comments should explain intent, not obvious implementation details.

Good:

```cpp
// Only the expected files matter; the launcher does not care how the install was obtained.
bool Rocksmith2014Profile::ValidateInstall(const fs::path &installDir) const
```

Bad:

```cpp
// Increment counter by one
counter++
```

Assume the reader already understands the language syntax.

## Simple Comment Separator

Used for compact or minor sections.

```cpp
// Profile commands
// Path assignment
// Entry point
```

Leave one blank line above the comment.

---

# Config (JSON)

Launcher-wide configuration lives in `~/.config/rocksmith-launcher/config.json`.
Per-installation configuration lives in `profiles/<profile_id>.json`; it includes
the game ID, installation path, runtime ID, and patch settings. Keep keys descriptive
and predictable; new settings are exposed through the CLI first.

---

# QML (future GUI, phase 6)

The Qt/QML frontend consumes the already-tested core; keep business logic out of the UI. Detailed QML conventions will be added when the GUI is being built.
