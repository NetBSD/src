#! /bin/sh
# Paths going up and down were inducing extraction loops.

. ./preset
. $srcdir/before

set -e
mkdir directory
tar -cPvf archive directory/../directory
echo -----
tar -xPvf archive

out="\
directory/../directory/
-----
directory/../directory/
"

. $srcdir/after
