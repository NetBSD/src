#!/usr/bin/env bash
#   Copyright (C) 1990-2014 Free Software Foundation
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# This script creates release packages for gdb, binutils, and other
# packages which live in src.  It used to be implemented as the src-release
# Makefile and prior to that was part of the top level Makefile, but that
# turned out to be very messy and hard to maintain.

set -e

BZIPPROG=bzip2
GZIPPROG=gzip
XZPROG=xz
MD5PROG=md5sum
MAKE=make
CC=gcc
CXX=g++

# Default to avoid splitting info files by setting the threshold high.
MAKEINFOFLAGS=--split-size=5000000

#
# Support for building net releases

# Files in devo used in any net release.
DEVO_SUPPORT="README Makefile.in configure configure.ac \
	config.guess config.sub config move-if-change \
	COPYING COPYING.LIB install-sh config-ml.in symlink-tree \
	mkinstalldirs ltmain.sh missing ylwrap \
	libtool.m4 ltsugar.m4 ltversion.m4 ltoptions.m4 \
	Makefile.def Makefile.tpl src-release.sh config.rpath \
	ChangeLog MAINTAINERS README-maintainer-mode \
	lt~obsolete.m4 ltgcc.m4 depcomp mkdep compile \
	COPYING3 COPYING3.LIB"

# Files in devo/etc used in any net release.
ETC_SUPPORT="Makefile.in configure configure.in standards.texi \
	make-stds.texi standards.info* configure.texi configure.info* \
	ChangeLog configbuild.* configdev.* fdl.texi texi2pod.pl gnu-oids.texi"

# Get the version number of a given tool
getver()
{
    tool=$1
    if grep 'AC_INIT.*BFD_VERSION' $tool/configure.ac >/dev/null 2>&1; then
	bfd/configure --version | sed -n -e '1s,.* ,,p'
    elif test -f $tool/common/create-version.sh; then
	$tool/common/create-version.sh $tool 'dummy-host' 'dummy-target' VER.tmp
	cat VER.tmp | grep 'version\[\]' | sed 's/.*"\([^"]*\)".*/\1/' | sed 's/-cvs$//'
        rm -f VER.tmp
    elif test -f $tool/version.in; then
	head -1 $tool/version.in
    else
	echo VERSION
    fi
}

# Setup build directory for building release tarball
do_proto_toplev()
{
    package=$1
    ver=$2
    tool=$3
    support_files=$4
    echo "==> Making $package-$ver/"
    # Take out texinfo from a few places.
    sed -e '/^all\.normal: /s/\all-texinfo //' \
	-e '/^	install-texinfo /d' \
	<Makefile.in >tmp
    mv -f tmp Makefile.in
    #
    ./configure --target=i386-pc-linux-gnu
    $MAKE configure-host configure-target \
	ALL_GCC="" ALL_GCC_C="" ALL_GCC_CXX="" \
	CC_FOR_TARGET="$CC" CXX_FOR_TARGET="$CXX"
    # Make links, and run "make diststuff" or "make info" when needed.
    rm -rf proto-toplev
    mkdir proto-toplev
    dirs="$DEVO_SUPPORT $support_files $tool"
    for d in $dirs ; do
	if [ -d $d ]; then
	    if [ ! -f $d/Makefile ] ; then
		true
	    elif grep '^diststuff:' $d/Makefile >/dev/null ; then
		(cd $d ; $MAKE MAKEINFOFLAGS="$MAKEINFOFLAGS" diststuff) \
		    || exit 1
	    elif grep '^info:' $d/Makefile >/dev/null ; then
		(cd $d ; $MAKE MAKEINFOFLAGS="$MAKEINFOFLAGS" info) \
		    || exit 1
	    fi
	    if [ -d $d/proto-$d.dir ]; then
		ln -s ../$d/proto-$d.dir proto-toplev/$d
	    else
		ln -s ../$d proto-toplev/$d
	    fi
	else
	    if (echo x$d | grep / >/dev/null); then
	      mkdir -p proto-toplev/`dirname $d`
	      x=`dirname $d`
	      ln -s ../`echo $x/ | sed -e 's,[^/]*/,../,g'`$d proto-toplev/$d
	    else
	      ln -s ../$d proto-toplev/$d
	    fi
	  fi
	done
	(cd etc; $MAKE MAKEINFOFLAGS="$MAKEINFOFLAGS" info)
	$MAKE distclean
	mkdir proto-toplev/etc
	(cd proto-toplev/etc;
	    for i in $ETC_SUPPORT; do
		ln -s ../../etc/$i .
		done)
	#
	# Take out texinfo from configurable dirs
	rm proto-toplev/configure.ac
	sed -e '/^host_tools=/s/texinfo //' \
	    <configure.ac >proto-toplev/configure.ac
	#
	mkdir proto-toplev/texinfo
	ln -s ../../texinfo/texinfo.tex	proto-toplev/texinfo/
	if test -r texinfo/util/tex3patch ; then
	    mkdir proto-toplev/texinfo/util && \
		ln -s ../../../texinfo/util/tex3patch proto-toplev/texinfo/util
	else
	    true
	fi
	chmod -R og=u . || chmod og=u `find . -print`
	#
	# Create .gmo files from .po files.
	for f in `find . -name '*.po' -type f -print`; do
	    msgfmt -o `echo $f | sed -e 's/\.po$/.gmo/'` $f
	done
	#
	rm -f $package-$ver
	ln -s proto-toplev $package-$ver
}

CVS_NAMES='-name CVS -o -name .cvsignore'

# Add an md5sum to the built tarball
do_md5sum()
{
    echo "==> Adding md5 checksum to top-level directory"
    (cd proto-toplev && find * -follow \( $CVS_NAMES \) -prune \
	-o -type f -print \
	| xargs $MD5PROG > ../md5.new)
    rm -f proto-toplev/md5.sum
    mv md5.new proto-toplev/md5.sum
}

# Build the release tarball
do_tar()
{
    package=$1
    ver=$2
    echo "==> Making $package-$ver.tar"
    rm -f $package-$ver.tar
    find $package-$ver -follow \( $CVS_NAMES \) -prune \
	-o -type f -print \
	| tar cTfh - $package-$ver.tar
}

# Compress the output with bzip2
do_bz2()
{
    package=$1
    ver=$2
    echo "==> Bzipping $package-$ver.tar.bz2"
    rm -f $package-$ver.tar.bz2
    $BZIPPROG -k -v -9 $package-$ver.tar
}

# Compress the output with gzip
do_gz()
{
    package=$1
    ver=$2
    echo "==> Gzipping $package-$ver.tar.gz"
    rm -f $package-$ver.tar.gz
    $GZIPPROG -k -v -9 $package-$ver.tar
}

# Compress the output with xz
do_xz()
{
    package=$1
    ver=$2
    echo "==> Xzipping $package-$ver.tar.xz"
    rm -f $package-$ver.tar.xz
    $XZPROG -k -v -9 $package-$ver.tar
}

# Compress the output with all selected compresion methods
do_compress()
{
    package=$1
    ver=$2
    compressors=$3
    for comp in $compressors; do
	case $comp in
	    bz2)
		do_bz2 $package $ver;;
	    gz)
		do_gz $package $ver;;
	    xz)
		do_xz $package $ver;;
	    *)
		echo "Unknown compression method: $comp" && exit 1;;
	esac
    done
}

# Add djunpack.bat the tarball
do_djunpack()
{
    package=$1
    ver=$2
    echo "==> Adding updated djunpack.bat to top-level directory"
    echo - 's /gdb-[0-9\.]*/$package-'"$ver"'/'
    sed < djunpack.bat > djunpack.new \
	-e 's/gdb-[0-9][0-9\.]*/$package-'"$ver"'/'
    rm -f proto-toplev/djunpack.bat
    mv djunpack.new proto-toplev/djunpack.bat
}

# Create a release package, tar it and compress it
tar_compress()
{
    package=$1
    tool=$2
    support_files=$3
    compressors=$4
    verdir=${5:-$tool}
    ver=$(getver $verdir)
    do_proto_toplev $package $ver $tool "$support_files"
    do_md5sum
    do_tar $package $ver
    do_compress $package $ver "$compressors"
}

# Create a gdb release package, tar it and compress it
gdb_tar_compress()
{
    package=$1
    tool=$2
    support_files=$3
    compressors=$4
    ver=$(getver $tool)
    do_proto_toplev $package $ver $tool "$support_files"
    do_md5sum
    do_djunpack $package $ver
    do_tar $package $ver
    do_compress $package $ver "$compressors"
}

# The FSF "binutils" release includes gprof and ld.
BINUTILS_SUPPORT_DIRS="bfd gas include libiberty opcodes ld elfcpp gold gprof intl setup.com makefile.vms cpu zlib"
binutils_release()
{
    compressors=$1
    package=binutils
    tool=binutils
    tar_compress $package $tool "$BINUTILS_SUPPORT_DIRS" "$compressors"
}

GAS_SUPPORT_DIRS="bfd include libiberty opcodes intl setup.com makefile.vms zlib"
gas_release()
{
    compressors=$1
    package=gas
    tool=gas
    tar_compress $package $tool "$GAS_SUPPORT_DIRS" "$compressors"
}

GDB_SUPPORT_DIRS="bfd include libiberty opcodes readline sim intl libdecnumber cpu zlib"
gdb_release()
{
    compressors=$1
    package=gdb
    tool=gdb
    gdb_tar_compress $package $tool "$GDB_SUPPORT_DIRS" "$compressors"
}

# Corresponding to the CVS "sim" module.
SIM_SUPPORT_DIRS="bfd opcodes libiberty include intl gdb/version.in gdb/common/create-version.sh makefile.vms zlib"
sim_release()
{
    compressors=$1
    package=sim
    tool=sim
    tar_compress $package $tool "$SIM_SUPPORT_DIRS" "$compressors" gdb
}

usage()
{
    echo "src-release.sh <options> <release>"
    echo "options:"
    echo "  -b: Compress with bzip2"
    echo "  -g: Compress with gzip"
    echo "  -x: Compress with xz"
    exit 1
}

build_release()
{
    release=$1
    compressors=$2
    case $release in
	binutils)
	    binutils_release "$compressors";;
	gas)
	    gas_release "$compressors";;
	gdb)
	    gdb_release "$compressors";;
	sim)
	    sim_release "$compressors";;
	*)
	    echo "Unknown release name: $release" && usage;;
    esac
}

compressors=""

while getopts ":gbx" opt; do
    case $opt in
	b)
	    compressors="$compressors bz2";;
	g)
	    compressors="$compressors gz";;
	x)
	    compressors="$compressors xz";;
	\?)
	    echo "Invalid option: -$OPTARG" && usage;;
  esac
done
shift $((OPTIND -1))
release=$1

build_release $release "$compressors"
