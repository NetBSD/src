#! /bin/sh
# tar should detect that its gzip child failed.

. ./preset
. $srcdir/before

tar xfvz /dev/null
test $? = 2 || exit 1

err="\

gzip: stdin: unexpected end of file
tar: Child returned status 1
tar: Error exit delayed from previous errors
"

. $srcdir/after
