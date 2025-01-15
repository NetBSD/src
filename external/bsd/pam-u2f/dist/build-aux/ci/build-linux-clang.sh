#!/usr/bin/env bash
set -eux

BUILDROOT="$(git rev-parse --show-toplevel)"
SCAN=(scan-build${CC#clang} --use-cc="${CC}")

pushd "$BUILDROOT" &>/dev/null
  ./autogen.sh
  mkdir build && pushd build &>/dev/null
    ${SCAN[@]} ../configure --disable-silent-rules --disable-man
  popd &>/dev/null
  ${SCAN[@]} --keep-cc --status-bugs make -C build -j $(nproc) check
popd &>/dev/null
