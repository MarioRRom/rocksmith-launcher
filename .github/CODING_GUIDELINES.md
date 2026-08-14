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
2. `rocklaunch-cli/` — thin CLI exercising the core. Subcommands by phase: `detect`, `set-path`, `runtime list`, `runtime install`, `launch`, `dlc list/add/delete`.
3. `rocklaunch/` (phase 6) — Qt/QML frontend consuming the core's public API via thin QObject/Q_INVOKABLE wrappers.

Nothing outside the core implements game detection, runtime handling, patching, or CDLC. Tests cover the core via the CLI.

Key abstractions, defined from phase 0:

- `IGameProfile` — `id()`, `validate_install()`, `required_env(LaunchContext)`, `executable()`.
- `ILaunchPatch` — `id()`, `is_enabled(Profile)`, `apply(prefix_dir, install_dir)`. Provisional — the final patch interface is decided in phases 4/5 (possibly split into `ICablePatch`/`ICDLCPatch`).
- `GameSource` — locates the game (`SteamSource` via `libraryfolders.vdf`, `ManualSource` by path). No scattered conditional logic.

The launcher reimplements the methods of relevant projects (no-cable, CDLC, audio) at launcher level — no literal forks, no `launcher.exe -> game.exe` chains, no files required from the user. Inspiration is credited in the README (`# Credits`).

Data lives under XDG-friendly paths:

```
~/.config/rocksmith-launcher/         → config.json, profiles/*.json
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
- Comments explain intent, not implementation details.

---

# Config (JSON)

User configuration and profiles live under `~/.config/rocksmith-launcher/`. Keep keys descriptive and predictable; new settings are exposed through the CLI first.

---

# QML (future GUI, phase 6)

The Qt/QML frontend consumes the already-tested core; keep business logic out of the UI. Detailed QML conventions will be added when the GUI is being built.
