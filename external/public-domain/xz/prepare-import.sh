#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.6 2026/04/08 21:00:50 christos Exp $

set -e
V=5.8.3

if [ -d import ]; then
	echo "import present, please cleanup" 1>&2
	exit 1
fi
tar -xzf xz-$V.tar.*
mv xz-$V import

cd import
# Binary files derived from distribution files
rm -rf doc/man
# Files under GPL
rm -rf extra lib m4/[a-s]* m4/[u-z]* src/scripts/xz* Doxyfile.in
# Files not of relevance
rm -rf ABOUT-NLS aclocal.m4 autogen.sh COPYING.*GPL* INSTALL.generic
mkdir po.tmp
mv po/*.po po/*.gmo po.tmp/
rm -rf po
mv po.tmp po
rm -rf debug dos windows
rm -rf Makefile* */Makefile* */*/Makefile* */*/*/Makefile*
# Files under GPL/LGPL kept:
# build-aux/* from autoconf
# Binary files to be encoded
for f in tests/files/*.xz; do
	uuencode -m $f $f > $f.base64
	rm $f
done

sed -i -e '/^ac_config_files=/d' configure
echo "cd import && cvs -d cvs.netbsd.org:/cvsroot import src/external/public-domain/xz/dist XZ xz-$V"
