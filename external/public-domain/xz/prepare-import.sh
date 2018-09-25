#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.5 2018/09/25 05:38:10 joerg Exp $

set -e

rm -rf dist tmp
tar xf xz-5.2.4.tar.xz
mv xz-5.2.4 dist

cd dist
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
for f in tests/compress_prepared_bcj_sparc tests/compress_prepared_bcj_x86 \
	 tests/files/*.xz; do
	uuencode -m $f $f > $f.base64
	rm $f
done

grep -v '^ac_config_files=' configure > configure.new
mv configure.new ../configure
