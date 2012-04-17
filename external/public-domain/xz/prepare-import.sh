#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.1.6.1 2012/04/17 00:05:02 yamt Exp $

set -e

rm -rf dist tmp
tar xzf xz-5.0.3.tar.gz
mv xz-5.0.3 dist

cd dist
# Binary files derived from distribution files
rm -rf doc/man
# Files under GPL
rm -rf build-aux extra lib m4/[a-s]* m4/[u-z]* src/scripts/xz* Doxyfile.in
# Files not of relevance
rm -rf ABOUT-NLS aclocal.m4 autogen.sh configure COPYING.*GPL* INSTALL.generic
mkdir po.tmp
mv po/*.po po/*.gmo po.tmp/
rm -rf po
mv po.tmp po
rm -rf debug dos windows
rm -rf Makefile* */Makefile* */*/Makefile* */*/*/Makefile*
# Binary files to be encoded
for f in tests/compress_prepared_bcj_sparc tests/compress_prepared_bcj_x86 \
	 tests/files/*.xz; do
	uuencode -m $f $f > $f.base64
	rm $f
done

# Files under GPL/LGPL kept:
# build-aux/* from autoconf
# lib/*
# m4/*
