#!/usr/bin/env bash
# neoswift Cloud Agent install script.
#
# Idempotent: safe to re-run. Provisions the Linux build toolchain for the
# neoswift pilot client (primary repo) plus the rustfsd test server, then
# performs an end-to-end build of neoswift so fresh agents start ready.
#
# Reproduces the CI-proven Linux build path (.github/workflows/build.yml):
# Qt 6.10.1 (qtmultimedia + qtwebsockets), Conan 2, Ninja, GCC, and the
# dev-debug CMake preset (Debug, VATSIM + xswiftbus disabled -> no secrets or
# ssh-only submodules required).
set -euo pipefail

QT_VERSION=6.10.1
QT_ROOT=/opt/Qt
QT_DIR="$QT_ROOT/$QT_VERSION/gcc_64"

export PATH="$HOME/.local/bin:$QT_DIR/bin:$PATH"

echo ">>> [1/7] System packages"
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build pkg-config git \
  dbus-x11 libdbus-1-dev libglu1-mesa-dev libgl1-mesa-dev \
  libpulse-dev libxkbcommon-dev libxcb-cursor0

echo ">>> [2/7] Python build tooling (conan, aqtinstall)"
pip3 install --user --break-system-packages --upgrade requests conan aqtinstall

echo ">>> [3/7] Qt $QT_VERSION (qtmultimedia + qtwebsockets)"
if [ ! -x "$QT_DIR/bin/qmake6" ]; then
  sudo mkdir -p "$QT_ROOT"
  sudo chown "$(id -u):$(id -g)" "$QT_ROOT"
  ( cd /tmp && aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 \
      -m qtmultimedia qtwebsockets -O "$QT_ROOT" )
else
  echo "    Qt already present at $QT_DIR"
fi

echo ">>> [4/7] Rust toolchain (rustfsd requires >= 1.88)"
if command -v rustup >/dev/null 2>&1; then
  rustup toolchain install stable --profile minimal
  rustup default stable
else
  echo "    rustup not found; skipping (rustfsd build unavailable)"
fi

echo ">>> [5/7] Persist environment for interactive shells"
if ! grep -q 'neoswift cloud env' "$HOME/.bashrc" 2>/dev/null; then
  {
    echo ''
    echo '# neoswift cloud env'
    echo "export PATH=\"\$HOME/.local/bin:$QT_DIR/bin:\$PATH\""
    echo "export CMAKE_PREFIX_PATH=\"$QT_DIR\${CMAKE_PREFIX_PATH:+:\$CMAKE_PREFIX_PATH}\""
  } >> "$HOME/.bashrc"
fi

echo ">>> [6/7] neoswift bootstrap + build"
NEOSWIFT=/agent/repos/neoswift
if [ -d "$NEOSWIFT" ]; then
  cd "$NEOSWIFT"
  # msgpack submodule (https). xplanemp2 is ssh-only and only used by
  # xswiftbus, which the dev-debug preset disables, so it is not initialised.
  git submodule update --init third_party/msgpack
  # Optional proprietary externals (XPLM SDK, SimConnect) when the helper
  # repo is checked out; not required by the dev-debug preset.
  if [ -d /agent/repos/neoswift-externals/XPLM ] && [ ! -e third_party/externals/XPLM ]; then
    rm -rf third_party/externals
    ln -s /agent/repos/neoswift-externals third_party/externals
  fi
  export CC=gcc CXX=g++
  export CMAKE_PREFIX_PATH="$QT_DIR${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
  conan profile detect --exist-ok
  conan install . --output-folder=build_conan --deployer=full_deploy \
    -pr=ci/profile_linux -s build_type=Debug --build=missing
  cmake -S . -B build --preset dev-debug
  cmake --build build --parallel
else
  echo "    $NEOSWIFT not found; skipping neoswift build"
fi

echo ">>> [7/7] rustfsd dependency prefetch"
if [ -f /agent/repos/rustfsd/Cargo.toml ] && command -v cargo >/dev/null 2>&1; then
  ( cd /agent/repos/rustfsd && cargo fetch )
else
  echo "    rustfsd not present or cargo unavailable; skipping"
fi

echo ">>> install complete"
