# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

### Removed

- Hostname and disk / machine serial are no longer collected or sent
  (v1 ticket `ident`).

### Build

- Authum added as submodule `lib/authum`; only its Ed25519 wrapper and
  vendored Monocypher 4.0.2 are compiled. `build.bat` pins the C compiler to
  `cl` (C sources are now part of the build).
