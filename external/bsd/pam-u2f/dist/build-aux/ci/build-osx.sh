#!/usr/bin/env bash
set -ex

BUILDROOT="$(git rev-parse --show-toplevel)"

pushd "/tmp" &>/dev/null
  # Build and install libcbor
  git clone git://github.com/pjk/libcbor
  pushd "/tmp/libcbor" &>/dev/null
    git checkout v0.5.0
    cmake -Bbuild -H.
    cmake --build build -- --jobs=2 VERBOSE=1
    sudo make -j $(sysctl -n hw.logicalcpu) -C build install
  popd &>/dev/null

  # Build and install libfido2
  export PKG_CONFIG_PATH=/usr/local/opt/openssl@1.1/lib/pkgconfig
  git clone git://github.com/Yubico/libfido2
  pushd "/tmp/libfido2" &>/dev/null
    cmake -Bbuild -H.
    cmake --build build -- --jobs=2 VERBOSE=1
    sudo make -j $(sysctl -n hw.logicalcpu) -C build install
  popd &>/dev/null
popd &>/dev/null

pushd "$BUILDROOT" &>/dev/null
  ./autogen.sh
  ./configure --disable-silent-rules --disable-man
  make -j $(sysctl -n hw.logicalcpu)
popd &>/dev/null
