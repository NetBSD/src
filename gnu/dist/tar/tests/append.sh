#! /bin/sh
# Append was just not working.

. ./preset
. $srcdir/before

set -e
touch file1
touch file2
tar cf archive file1
tar rf archive file2
tar tf archive

out="\
file1
file2
"

. $srcdir/after
