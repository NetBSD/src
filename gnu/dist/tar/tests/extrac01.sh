#! /bin/sh
# There was a diagnostic when directory already exists.

. ./preset
. $srcdir/before

set -e
mkdir directory
touch directory/file
tar cf archive directory || exit 1
tar xf archive || exit 1

. $srcdir/after
