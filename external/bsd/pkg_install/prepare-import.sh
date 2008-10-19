#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.1.4.2 2008/10/19 22:40:48 haad Exp $
#
# Extract the new tarball and rename the libarchive-X.Y.Z directory
# to dist.  Run this script and check for additional files and
# directories to prune, only relevant content is included.

set -e

cd dist
rm -f Makefile.in README config* install-sh tkpkg
rm -f */Makefile.in */*.cat*
rm -rf CVS */CVS
