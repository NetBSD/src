#!/usr/bin/env bash
set -ex

BUILDROOT="$(git rev-parse --show-toplevel)"

pushd "$BUILDROOT" &>/dev/null
  ./autogen.sh
  ./configure --disable-silent-rules
  make -j $(nproc) distcheck
popd &>/dev/null
