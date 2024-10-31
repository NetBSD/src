#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.5.8.1 2024/10/31 18:39:58 martin Exp $
#
# Run this script on the extracted libarchive directory and check for
# additional files and directories to prune, only relevant content is included.

set -e
PROG=$(basename "$0")

if [ -z "$1" ]; then
	echo "Usage $PROG <libarchive-directory>" 1>&2
	exit 1
fi

cd "$1"

rm -rf build contrib doc examples autom4te.cache unzip
rm -f INSTALL Makefile.am Makefile.in aclocal.m4 config.h.in
rm -f configure configure.ac CMakeLists.txt */CMakeLists.txt */*/CMakeLists.txt
rm -f */config_freebsd.h  CTestConfig.cmake .gitattributes .gitignore
rm -f .travis.yml */*/.cvsignore
