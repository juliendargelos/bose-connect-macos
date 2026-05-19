# AGENTS.md

This file onboards AI coding agents for this repository.

Trust this document first. Only search the codebase if a task needs details not covered here, or if these instructions are outdated.

## Repository summary

- Project: `Bose Connect macOS` (CLI utility for Bose headphones).
- Upstream base: `https://github.com/airvzxf/bose-connect-app-linux.git`.
- Purpose: reverse-engineered Bose Connect controls over Bluetooth RFCOMM (macOS native transport).
- Type: small C/Objective-C CLI project (~42 tracked files).
- Languages/runtime: C11 + Objective-C (`IOBluetooth`, `Foundation`, `CoreFoundation`).
- Build system: CMake.
- Platform: macOS only (`src/CMakeLists.txt` hard-fails on non-Apple).

## Quick, reliable command sequence (validated)

Run from repo root.

1) Build release (always do this before validating runtime behavior):

```bash
./src/script/build-prod.bash
```

- Validated: works.
- Observed time: ~1.37s on Apple Silicon.
- Postcondition: binary at `src/build/bose-connect`.

2) Smoke test the binary:

```bash
./src/build/bose-connect --help
```

- Validated: works.
- Observed time: ~0.33s.

3) Optional local environment check (no target device required):

```bash
./src/build/bose-connect --list-devices
```

- Validated: works on host with Bluetooth access.
- Observed time: ~0.07s.

4) Tests:

```bash
ctest --test-dir ./src/build --output-on-failure
```

- Validated: command works, but currently prints `No tests were found!!!`.
- Observed time: ~0.43s.

5) Install without sudo (recommended for automation):

```bash
cmake --install ./src/build --config Release --prefix /tmp/bose-install
```

- Validated with a writable temp prefix.
- Postcondition: binary in `/tmp/bose-install/bin/bose-connect`.

## Bootstrap and tool requirements

Always ensure these are available before building:

- Xcode Command Line Tools (`xcode-select -p` must resolve).
- `cmake`.
- `clang` (Apple clang from Xcode works).

Validated on this machine:

- `cmake 4.3.2`
- `Apple clang 21.0.0`

## QA/lint path and failure mode

Debug+QA script:

```bash
./src/script/build-dev.bash
```

- Observed failure in ~1.44s: `/bin/sh: clang-format: command not found`.
- Root cause: QA path in `src/CMakeLists.txt` enables `clang-format`, `clang-tidy`, and `cppcheck` pre-build checks.
- Also validated missing on host: `clang-format`, `clang-tidy`, `cppcheck` were not installed.

If you need QA mode, install those tools first (for macOS, typically via Homebrew `llvm` + `cppcheck`, and ensure LLVM bin path is in `PATH`).

## Known operational gotchas (important)

- `src/script/build-prod.bash` and `src/script/build-dev.bash` always delete `src/build` first.
  - Do not rely on cached build outputs.
- CLI accepts only one command option per invocation (`src/main.c`).
- Device commands can fail intermittently with `Connection refused`; retry is often enough.
- Transport read timeout is 5s and returns `EAGAIN` (`src/library/transport_macos.m`); `--send-packet` treats this as "no response".
- Alias storage is user-local in `~/.bose-connect-devices` (format: `alias=AA:BB:CC:DD:EE:FF`).
- Keep repo clean from `.DS_Store` files.

## Project layout and where to edit

### Core architecture

- `src/main.c`
  - CLI usage text, option parsing (`getopt_long`), command dispatch.
  - Address/alias resolution and high-level command handlers.
- `src/library/based.c` + `src/library/based.h`
  - Reverse-engineered Bose protocol packet definitions and request/response logic.
  - Device status parsing, setter/getter implementations, packet read/write checks.
- `src/library/transport_macos.m`
  - macOS Bluetooth transport over RFCOMM.
  - Opens channel with `IOBluetoothDevice openRFCOMMChannelSync`.
- `src/library/alias_store.c` + `src/library/alias_store.h`
  - Alias CRUD and parsing for `~/.bose-connect-devices`.
- `src/library/bluetooth.c/.h`
  - Bluetooth address parse/format helpers.
- `src/library/util.c/.h`
  - Hex/string/memory helpers.

### Build/config files

- `src/CMakeLists.txt`: targets, Apple-only guard, optional QA commands.
- `src/.clang-format`, `src/.clang-tidy`: formatting and static-analysis configs.
- `src/script/*.bash`: standard build/install scripts.

## Current mode-switch behavior (important for future changes)

- Public CLI option: `-m, --mode` (`quiet`, `aware`, `custom-1`, `custom-2`).
- Implemented in `src/main.c` and delegated to `set_noise_mode`.
- Mode packet family in `src/library/based.c`:
  - Send: `1f 03 05 02 <mode> 01 <checksum>`
  - Checksum: `0xC5 - mode`
  - ACK: `1f 03 07 00`
- `--send-packet` supports comma-separated packet sequences in one connection.

## CI/workflow notes

Workflows in `.github/workflows/`:

- `cmake.yml` (Docker-on-Ubuntu build flow, legacy).
- `codacy-analysis.yml` (security scan).
- `codeql-analysis.yml` (CodeQL).

Important: repository code is now macOS-only (`if (NOT APPLE) FATAL_ERROR`), so Linux-native build expectations from older workflow patterns may not reflect current local truth.

## Practical change checklist (fast path)

1. Edit code in the relevant file(s) above.
2. Always run `./src/script/build-prod.bash`.
3. Always run `./src/build/bose-connect --help`.
4. If CLI options changed, update both:
   - usage text in `src/main.c`
   - `README.md` usage block
5. If Bluetooth behavior changed and hardware is available, validate with `--alias` workflow.
6. Run `git status --short` and ensure no generated junk files are left.

## Root inventory (for quick orientation)

- Root files/dirs: `.github/`, `src/`, `README.md`, `CONTRIBUTING.md`, `DEVELOPMENT.md`, `TODO.md`, `LICENSE`.
- `src/` first level: `main.c`, `main.h`, `library/`, `script/`, `CMakeLists.txt`, `Dockerfile`, `docker-compose.yml`, lint configs.
