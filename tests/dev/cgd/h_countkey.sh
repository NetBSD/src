#!/bin/sh

set -Ceu

n=$(cat "$1" 2>/dev/null || echo 0)
n=$((n + 1))
echo $n >"$1".tmp
mv -f "$1".tmp "$1"
shift

echo ${1+"$@"} | base64 -d
