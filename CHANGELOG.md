# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **One wrapper per game.** The loader is now built once per game, and every
  edition carries its own name, game directory, Steam AppId, icon and instance
  mutex:

  | Edition | Wrapper | Starts | Steam AppId |
  |---|---|---|---|
  | Counter-Strike 1.6 | `cstrike.exe` | `hl.exe -game cstrike` | 10 |
  | Sven Co-op | `svencoop.exe` | `hl.exe -game svencoop` | 225840 |

  Both editions come in two shapes: the wrapper spawns `hl.exe`, and the
  standalone build compiles the `hl` launcher into the wrapper so the game
  runs in one process and no `hl.exe` is needed.

### Changed

- **Vellum v2 tickets (breaking: servers need qproto 2.1+).** The connect
  ticket is now 120 bytes, Ed25519-signed over the server ip:port the engine
  dials and the local clock. A ticket one server collects no longer works on
  another, and nobody can forge another player's SteamID.
- **SteamID from a profile key.** On first run `steamclient` creates a key pair
  and keeps it in the user profile (`%APPDATA%\Raspad\vellum.key`;
  `~/.local/share/raspad/vellum.key`, mode 0600) — never in the game folder,
  so sharing a game folder no longer shares a SteamID. Lose the key, get a new
  SteamID.
- Listen servers check the ticket signature before trusting its SteamID.
- **An edition owns its game.** A `-game` from the caller is dropped instead
  of being forwarded: the game is fixed at build time, so the wrapper can
  never be talked into starting something else. Every other argument
  (`-connect`, `-console`, map names, …) still reaches the engine.
- `build.bat` builds both editions, and keeps the wrapper and the standalone
  configuration in separate build directories.

### Removed

- Hostname and disk / machine serial are no longer collected or sent
  (v1 ticket `ident`).

### Fixed

- The standalone build no longer re-execs with a command line that still
  carried the caller's `-game`; it re-execs with the one the edition built.

### Build

- Authum added as submodule `lib/authum`; only its Ed25519 wrapper and
  vendored Monocypher 4.0.2 are compiled. `build.bat` pins the C compiler to
  `cl` (C sources are now part of the build).
- New editions: add a line to `build.bat` (name, game directory, AppId) and
  drop `res/<name>.rc` + `res/<name>.ico` next to the cstrike ones.
