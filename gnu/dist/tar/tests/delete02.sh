#! /bin/sh
# Deleting a member with the archive from stdin was not working correctly.

. ./preset
. $srcdir/before

set -e
genfile -l 3073 -p zeros > 1
cp 1 2
cp 2 3
tar cf archive 1 2 3
tar tf archive
cat archive | tar f - --delete 2 > archive2
echo -----
tar tf archive2

out="\
1
2
3
-----
1
3
"

. $srcdir/after
