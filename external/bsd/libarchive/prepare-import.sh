#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.3.12.2 2020/04/13 07:46:02 martin Exp $
#
# Extract the new tarball and rename the libarchive-X.Y.Z directory
# to dist.  Run this script and check for additional files and
# directories to prune, only relevant content is included.

set -e

cd dist

rm -rf build contrib doc examples autom4te.cache
rm -f INSTALL Makefile.am Makefile.in aclocal.m4 config.h.in
rm -f configure configure.ac CMakeLists.txt */CMakeLists.txt */*/CMakeLists.txt
rm -f */config_freebsd.h  CTestConfig.cmake .gitattributes .gitignore
rm -f .travis.yml */*/.cvsignore

