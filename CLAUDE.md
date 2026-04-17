# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**neoswift** is a FSD-compatible flight simulation network pilot client, forked from [swift pilot client](https://swift-project.org/). It is designed for third-party networks — VATSIM-specific proprietary components have been removed and replaced. Network configuration is via auto-discovery at `https://<your-network>/.well-known/fsd-configuration.json`.

## Build

### Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.26 |
| Qt | 6.10.1 (via aqtinstall or Qt Online Installer), requires `qtmultimedia` |
| Conan | 2.x (`pip install conan`) |
| Ninja | any (`pip install ninja`) |
| MSVC | 2022 (Windows) / GCC ≥ 11 (Linux) / Apple Clang ≥ 15 (macOS) |

### Build Commands

```bash
# 1. Install Conan dependencies
conan profile detect
conan install . --output-folder=build_conan --deployer=full_deploy -pr=ci/profile_win --build=missing
# Linux/macOS: -pr=ci/profile_linux or ci/profile_mac

# 2. Activate Conan environment
# Windows:
build_conan\build\RelWithDebInfo\generators\conanrun.bat
# Linux/macOS:
source build_conan/build/RelWithDebInfo/generators/conanrun.sh

# 3. Configure
cmake --preset dev-debug

# 4. Build
cmake --build build --parallel
```

### Key CMake Options

- `SWIFT_BUILD_UNIT_TESTS` (default ON) — build unit tests
- `SWIFT_VATSIM_SUPPORT` (default ON) — VATSIMAuth challenge-response (requires `vatsim.json`)
- `SWIFT_BUILD_*_PLUGIN` — per-simulator plugin toggles (FSX, P3D, MSFS, MSFS2024, XPlane, FlightGear, Emulated)
- `SWIFT_USE_PCH` (default ON) — precompiled headers; disable if debugging build issues

### VATSIMAuth credentials (optional)

Create `vatsim.json` in repo root (gitignored):
```json
{ "vatsim": { "id": "YOUR_CLIENT_ID", "key": "YOUR_32_CHAR_PRIVATE_KEY" } }
```
Without this file, CMake uses dummy values; JWT-auth networks are unaffected.

### Tests

```bash
cd build
ctest --output-on-failure
# Or individual test binary:
./out/bin/swiftcoretest
```

### Translations

```bash
cmake --build build --target update_translations   # extract strings → .ts
cmake --build build --target release_translations  # compile .ts → .qm
```

## Architecture

### Module Layout

```
src/
├── misc/        # Utility shared library: aviation data, math/physics (pq/), geo, input, weather
├── core/        # Core shared library: FSD protocol, network, AFV voice, simulator contexts
├── gui/         # GUI shared library: Qt widgets, wizard, settings dialogs, editors
├── input/       # Input shared library: platform-specific keyboard/joystick
├── sound/       # Audio shared library: Opus codec, DSP, sample providers
├── plugins/simulator/  # Per-simulator plugins (FSX, P3D, MSFS, MSFS2024, XPlane, FlightGear, Emulated)
├── swiftcore/   # Main pilot client executable
├── swiftdata/   # Aircraft database manager executable
├── swiftlauncher/ # Application launcher executable
└── xswiftbus/   # X-Plane D-Bus integration service (Linux/macOS only)
```

Dependency order: `misc` ← `core` ← `gui` ← `swiftcore/swiftdata`

### Context / Dependency Injection

The central DI mechanism lives in `src/core/context/`. The global context exposes:
- `IContextApplication` — settings, lifecycle
- `IContextAudio` — audio devices
- `IContextNetwork` — FSD network connectivity
- `IContextOwnAircraft` — player aircraft state
- `IContextSimulator` — active simulator plugin

Components obtain context interfaces via `sApp->getIContextXxx()`.

### Settings Pattern

```cpp
struct TMySettings : swift::misc::TSettingTrait<MyDataType> {
    static const char *key() { return "settings/path"; }
    static const QString &humanReadable() { return QStringLiteral("My Setting"); }
    static bool isValid(const MyDataType &, QString &) { return true; }
};
swift::misc::CSetting<TMySettings> m_setting { this };
```

Settings are broadcast via Qt signals when changed.

### Simulator Plugins

All simulator plugins implement a common interface in `src/plugins/simulator/plugincommon/`. The plugin loader discovers and loads them at runtime. Per-simulator path/config UI lives in `src/gui/components/settings*simulator*.{h,cpp,ui}`.

### GUI Wizard Flow

Configuration wizard (`src/gui/components/configurationwizard.*`) pages:
1. Network selection
2. Simulator selection (`configsimulatorcomponent`)
3. Per-simulator path setup (`configsimulatorsetupcomponent` — new component)
4. Initial data load

## Code Style

`.clang-format` (LLVM-based, column limit 120, 4-space indent, namespace indentation All) and `.clang-tidy` (C++17 modernization checks) are authoritative. Naming: classes `CamelCase`, methods/members `camelBack`, member variables `m_` prefix.

## CI/CD

`.github/workflows/build.yml` runs: static analysis (cppcheck + clang-tidy on changed files) → parallel builds on Ubuntu/Windows/macOS → optional installer packaging via InstallBuilder → GitHub release on `main`.

Required secrets for full CI: `BITROCK_LICENSE` (InstallBuilder), `NETWORK_CLIENT_ID`/`NETWORK_CLIENT_KEY` (VATSIMAuth).

## Simulator Support

| Simulator | Win | Lin | Mac |
|-----------|-----|-----|-----|
| FS9 / FSX / P3D | ✓ | — | — |
| MSFS 2020 / 2024 | ✓ | — | — |
| X-Plane | ✓ | ✓ | ✓ |
| FlightGear | ✓ | ✓ | ✓ |
