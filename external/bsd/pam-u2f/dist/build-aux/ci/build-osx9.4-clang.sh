#!/usr/bin/env bash
set -ex

BUILDROOT="$(git rev-parse --show-toplevel)"

source $BUILDROOT/build-aux/ci/build-osx.sh
