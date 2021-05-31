#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.3.22.1 2021/05/31 22:10:15 cjep Exp $
#
# Checkout libcxxrt in the corresponding subdirectories of
# dist.  Run this script and check for additional files and
# directories to prune, only relevant content should be included.

set -e

cd ../libcxxrt
rm -rf .git CMakeLists.txt */CMakeLists.txt src/doxygen_config

