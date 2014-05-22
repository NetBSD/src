#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.3.12.1 2014/05/22 15:51:03 yamt Exp $
#
# Copy new pkgsrc/pkgtools/pkg_install/files to dist.
# Run this script and check for additional files and
# directories to prune, only relevant content is included.

set -e

cd dist
rm -f Makefile.in README config* install-sh tkpkg
rm -f */Makefile.in */*.cat*
rm -rf CVS */CVS view
