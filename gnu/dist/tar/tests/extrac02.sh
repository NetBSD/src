#! /bin/sh
# Could not extract symlinks over an existing file.

. ./preset
. $srcdir/before

set -e
touch file
ln -s file link 2> /dev/null || ln file link
tar cf archive link
rm link
touch link
tar xf archive

. $srcdir/after
