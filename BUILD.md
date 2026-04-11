# Building neoswift

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | ≥ 3.26 | |
| Qt | 6.10.1 | via [aqtinstall](https://github.com/miurahr/aqtinstall) or [Qt Online Installer](https://www.qt.io/download) |
| Conan | 2.x | `pip install conan` |
| Ninja | any | `pip install ninja` or system package |
| MSVC | 2022 (v194) | Windows only |
| GCC | ≥ 11 | Linux only |
| Apple Clang | ≥ 15 | macOS only |

Qt modules required: `qtmultimedia`

---

## 1. Conan dependencies

neoswift uses Conan 2 to manage C++ dependencies (opus, libsodium, dbus, libevent, nlohmann_json).

```bash
# First-time setup
conan profile detect

# Install dependencies (pick the right profile)
conan install . --output-folder=build_conan --deployer=full_deploy -pr=ci/profile_win    # Windows
conan install . --output-folder=build_conan --deployer=full_deploy -pr=ci/profile_linux  # Linux
conan install . --output-folder=build_conan --deployer=full_deploy -pr=ci/profile_mac    # macOS
```

> **Note:** The profiles in `ci/` use **conancenter** (the default public Conan registry).
> No private Artifactory access is required.

---

## 2. Network client credentials (optional)

neoswift uses the VATSIMAuth challenge-response protocol when connecting to networks that support it.
The client must be compiled with a `clientId` and `privateKey` issued by the target network.

Create `vatsim.json` in the repository root (gitignored):

```json
{ "vatsim": { "id": "YOUR_CLIENT_ID", "key": "YOUR_32_CHAR_PRIVATE_KEY" } }
```

Without this file CMake falls back to dummy values (`0` / all-zeros).
The build succeeds, but challenge-response auth will fail at runtime.
JWT-authenticated networks are unaffected.

---

## 3. Local build (Windows, development)

```bat
REM Activate Conan environment
build_conan\build\RelWithDebInfo\generators\conanrun.bat

REM Configure
cmake --preset dev-debug

REM Build
cmake --build build --parallel
```

The `dev-debug` preset in `CMakePresets.json` uses Ninja and the Conan toolchain automatically.

### Passing the credentials file

```bat
cmake --preset dev-debug -DVATSIM_KEY_JSON=vatsim.json
```

Or set it once in `CMakeUserPresets.json`:

```json
{
  "version": 6,
  "configurePresets": [{
    "name": "my-debug",
    "inherits": "dev-debug",
    "cacheVariables": { "VATSIM_KEY_JSON": "vatsim.json" }
  }]
}
```

---

## 4. Linux / macOS

```bash
# Activate Conan environment
source build_conan/build/RelWithDebInfo/generators/conanrun.sh

cmake --preset dev-debug -DVATSIM_KEY_JSON=vatsim.json
cmake --build build --parallel
```

---

## 5. Creating an installer (optional)

Installers are produced by **InstallBuilder Qt Professional** (Bitrock).
The CI uses version `qt-professional-25.10.1`, downloadable from
https://releases.installbuilder.com/installbuilder.

You need a paid license (`license.xml`).

```bash
# Full build + package via the build script
python scripts/build.py -w 64 -t msvc          # Windows
python3 scripts/build.py -w 64 -t gcc          # Linux
python3 scripts/build.py -w 64 -t clang        # macOS
```

The script calls CMake, runs `cmake --install`, then invokes InstallBuilder.
Output: `neoswiftinstaller-{os}-64-{version}.{ext}` in the repo root.

---

## 6. GitHub Actions CI

The workflow (`.github/workflows/build.yml`) triggers on every push and produces
installers + xswiftbus archives as artifacts. A draft GitHub Release is created
automatically on pushes to `main`.

### Required secrets

Configure these in **Settings → Secrets and variables → Actions** on your GitHub repo:

| Secret | Required | Description |
|--------|----------|-------------|
| `BITROCK_LICENSE` | **Yes** (for installer) | Full XML content of your InstallBuilder Qt Professional license file. Without this the build step succeeds but the packaging step is skipped. |
| `NETWORK_CLIENT_ID` | Recommended | Integer client ID issued to neoswift by a network operator (e.g. Skylite). Baked into the binary for VATSIMAuth challenge-response. |
| `NETWORK_CLIENT_KEY` | Recommended | 32-character private key matching the client ID above. |
| `EXTERNALS_PAT` | Optional | GitHub PAT with `repo` scope for `${{ github.repository_owner }}/externals`. Only needed if you maintain a private externals repo containing proprietary SDKs (SimConnect etc.). If absent the externals checkout is skipped and SimConnect support is disabled. |
| `BACKTRACE_SYMBOL_TOKEN` | Optional | Upload debug symbols to a Backtrace crash-reporting account. |
| `BACKTRACE_MINIDUMP_TOKEN` | Optional | Backtrace minidump ingestion token. |

> **Not needed (removed from upstream):**
> `VATSIM_ID`, `VATSIM_KEY`, `ARTIFACTORY_USER`, `ARTIFACTORY_TOKEN`, `DISCORD_WEBHOOK`

---

## About InstallBuilder

The CI uses **InstallBuilder Qt Professional** (product line by Bitrock, formerly VMware InstallBuilder).
It is **not** the open-source NSIS or CPack — it is a commercial tool.

- Website: https://installbuilder.com
- Edition required: **Qt Professional** (the `qt-professional-*` builds)
- License format: XML file provided by Bitrock after purchase, placed at `~/license.xml` during CI
