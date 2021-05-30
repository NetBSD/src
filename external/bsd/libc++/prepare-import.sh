#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.4 2021/05/30 01:56:59 joerg Exp $
#
# Checkout libcxxrt in the corresponding subdirectories of
# dist.  Run this script and check for additional files and
# directories to prune, only relevant content should be included.

set -e

cd ../libcxxrt
rm -rf .git CMakeLists.txt */CMakeLists.txt src/doxygen_config

