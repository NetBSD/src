#! /bin/sh
# An old archive was not receiving directories.

. ./preset
. $srcdir/before

set -e
mkdir directory
tar cfvo archive directory
tar tf archive

out="\
directory/
directory/
"

. $srcdir/after
