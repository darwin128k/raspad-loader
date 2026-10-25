# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Automatic tagged release notes: short Highlights plus a collapsible full changelog generated from Git history, including direct commits.

- GitHub Actions builds Windows and Debian-compatible Linux x86 archives for both games and both auth modes, with smoke tests, example config, licenses, and SHA-256 checksums. Tags starting with `v` publish the archives to GitHub Releases.

- Restored optional RevEmu 2013 tickets and legacy identity with `VELLUM_AUTH_REVEMU2013` / `revemu2013` in both build scripts. Vellum v2 remains the default. Listen servers recognize both ticket layouts.

- **`internet` in `config/vellum.cfg`.** `internet = 0` (also `off` / `false` /
  `no`) skips master queries at runtime; favorites and LAN are unchanged. No
  second branch is needed to ship a client without an Internet list.
- **`build.bat nobrowser` / `build.sh nobrowser`** — same as
  `-DVELLUM_NO_SERVER_BROWSER=ON`, compiles the Internet query out.

### Changed

- Name the product Raspad Loader; reserve Vellum for the auth protocol. Use `config/loader.cfg` and `loader.log`; keep previous config names as read-only fallbacks.

- Compile only the selected auth implementation, including its listen-server parser and dependencies. Rename runtime master settings to `config/masterservers.cfg`; `vellum.cfg` remains a fallback for existing deployments.

- Preserve quoted launch arguments on Windows and literal shell arguments on Linux, reject oversized command lines, and avoid dropping arguments after the old Linux standalone 64-token limit.

- **One line of history.** `master` is the only development branch. **v1.0.0**
  tags the last commit before Vellum v2 tickets; **v2.0.0** tags the first
  configurable-master + Ed25519 + Sven Co-op line. Both snapshots stay in git
  (checkout the tag); they are not parallel forks.
- CMake option `VELLUM_STANDALONE` replaces `REVLOADER_STANDALONE` (the old
  name is still accepted).

### Removed

- Leftover RevEmu 2013 sources (`revemu2013.cpp` / `.h`) that were already
  unused after v2 tickets.

## [2.0.0] - 2026-10-24

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

- **`config/vellum.cfg`** — runtime settings of the `steamclient` facade,
  read from the game folder:

  ```
  masters = 10.0.0.5:27010, 10.0.0.6:27010
  ```

  An empty list, or no file at all, means that no master is queried and the
  Internet tab of the server browser stays empty (favorites and LAN are
  unaffected).

### Changed

- **No master is baked into the binary any more.** The GoldSrc UDP master used
  to be compiled in (`37.230.210.218:27010`, a CS 1.6 list); a deployment now
  names its own in `config/vellum.cfg`. The binary does not care which game
  the list belongs to.
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

## [1.0.0] - 2026-09-18

Last snapshot of the pre-crypto line (RevLoader renamed to Vellum, optional
HTTP master list, `VELLUM_NO_SERVER_BROWSER`, RevEmu 2013 still in tree).
See git at tag `v1.0.0`.
