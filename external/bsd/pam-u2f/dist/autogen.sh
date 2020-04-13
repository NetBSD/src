#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

main() {
  autoreconf --install
}

pushd "$DIR" &>/dev/null
  main "$@"
popd &>/dev/null
