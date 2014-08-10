#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.1.6.1 2014/08/10 07:07:26 tls Exp $
#
# Checkout libc++ and libcxxrt in the corresponding subdirectories of
# dist.  Run this script and check for additional files and
# directories to prune, only relevant content should be included.

set -e

cd dist/libcxx
rm -rf .svn cmake Makefile CMakeLists.txt lib src/support www .arcconfig
rm -rf include/support */CMakeLists.txt
cd ../libcxxrt
rm -rf .git CMakeLists.txt */CMakeLists.txt src/doxygen_config

