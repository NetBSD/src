#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.1.6.1 2009/05/30 16:40:31 snj Exp $
#
# Copy new pkgsrc/pkg_install/files to dist.
# Run this script and check for additional files and
# directories to prune, only relevant content is included.

set -e

cd dist
rm -f Makefile.in README config* install-sh tkpkg
rm -f */Makefile.in */*.cat*
rm -rf CVS */CVS view
