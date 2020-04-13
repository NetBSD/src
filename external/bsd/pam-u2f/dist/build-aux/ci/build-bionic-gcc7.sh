#!/usr/bin/env bash
set -ex

BUILDROOT="$(git rev-parse --show-toplevel)"

set +x
source $BUILDROOT/build-aux/ci/format-code.sh "$(git rev-parse HEAD~)"
set -x

source $BUILDROOT/build-aux/ci/build-linux.sh
