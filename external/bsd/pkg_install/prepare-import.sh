#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.5 2024/06/11 10:18:11 wiz Exp $
#
# Copy new pkgsrc/pkgtools/pkg_install/files to dist.
# Run this script and check for additional files and
# directories to prune, only relevant content is included.
#
# Import with:
#
# cd dist
# cvs -d cvs.netbsd.org:/cvsroot import -m "Import pkg_install 20xxxxxx from pkgsrc" src/external/bsd/pkg_install/dist PKGSRC pkg_install-20xxxxxx

set -e

cd dist
rm -f Makefile.in README config* install-sh tkpkg
rm -f */Makefile.in */*.cat*
rm -rf CVS */CVS view
