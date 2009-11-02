#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.1 2009/11/02 10:03:56 plunky Exp $

# Extract the new tarball and rename the libevent-X.Y.Z directory
# to dist.  Run this script and check for additional files and
# directories to prune, only relevant content is included.
#
# lib/ is built as SUBDIR from external/lib/Makefile, and
# the regression tests are used from tests/lib/libevent
#
# Use the following template to import
#  cvs import src/external/bsd/file/dist LIBEVENT libevent-X-Y-Z-stable
#
# don't forget to bump the lib/shlib_version and commit the include/ files
#

set -e

if [ -f dist/configure ]; then
    mkdir -p tmp
    cd tmp
    ../dist/configure
    make event-config.h
    mv config.h ../include		# not needed for 2.*
    mv event-config.h ../include
    cd ..
    rm -Rf tmp

    echo "Removing unwanted distfiles .."
    cd dist
    rm -Rf Doxyfile Makefile.am Makefile.in WIN32-Code WIN32-Prj \
	aclocal.m4 autogen.sh compat config.guess config.h.in config.sub \
	configure configure.in devpoll.c epoll.c epoll_sub.c event_rpcgen.py \
	evport.c install-sh ltmain.sh missing mkinstalldirs sample \
	strlcpy.c test/Makefile.am test/Makefile.in test/bench.c \
	test/regress.rpc test/test-eof.c test/test-init.c test/test-time.c \
	test/test-weof.c test/test.sh
    cd ..
fi

echo "Adding RCS tags .."
for f in $(grep -RL '\$NetBSD.*\$' dist include | grep -v CVS); do
    case $f in
    *.[ch])
	cat - ${f} > ${f}_tmp <<- EOF
		/*	\$NetBSD\$	*/
	EOF
	mv ${f}_tmp ${f}
	;;
    *.[0-9])
	cat - ${f} > ${f}_tmp <<- EOF
		.\"	\$NetBSD\$
		.\"
	EOF
	mv ${f}_tmp ${f}
	;;
    *)
	echo "No RCS tag added to ${f}"
	;;
    esac
done

echo "prepare-import done"
