#! /bin/sh
# Deleting a member after a big one was destroying the archive.

. ./preset
. $srcdir/before

set -e
genfile -l 50000 > file1
genfile -l 1024 > file2
tar cf archive file1 file2
tar f archive --delete file2
tar tf archive

out="\
file1
"

. $srcdir/after
