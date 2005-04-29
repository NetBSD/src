## Makefile for the autoconf-lib-link directory of GNU gettext
## Copyright (C) 1995-1999, 2000-2003, 2005 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies
ACLOCAL_AMFLAGS = -I m4 -I ../m4
EXTRA_DIST =
DISTCLEANFILES =

SUBDIRS = m4 tests

EXTRA_DIST += config.rpath


# We want to maintain only one copy of config.rpath. The one in this directory
# is the source.

all-local: $(srcdir)/../build-aux/config.rpath

$(srcdir)/../build-aux/config.rpath: $(srcdir)/config.rpath
	cp $(srcdir)/config.rpath $(srcdir)/../build-aux/config.rpath


# The AC_CONFIG_AUX_DIR argument in the tests/rpath*/configure.ac files is
# interpreted in two different contexts:
#   1. at "make distrib" time, when we call automake, relative to
#      SRCDIR/tests/rpath*/,
#   2. at "make check" time, when the directory is copied and configured,
#      relative to BUILDDIR/tests/tstdir/rp-*/.
# We choose this argument to be '../../../build-aux', for case 1, and therefore
# we need to make a local copy of the build-aux directory, for case 2.

all-local: build-aux/config.guess build-aux/config.sub build-aux/install-sh build-aux/ltmain.sh build-aux/mkinstalldirs build-aux/missing build-aux/config.rpath

build-aux-dir:
	test -d build-aux || mkdir build-aux
	touch build-aux-dir

build-aux/config.guess: $(srcdir)/../build-aux/config.guess build-aux-dir
	cp $(srcdir)/../build-aux/config.guess build-aux/config.guess

build-aux/config.sub: $(srcdir)/../build-aux/config.sub build-aux-dir
	cp $(srcdir)/../build-aux/config.sub build-aux/config.sub

build-aux/install-sh: $(srcdir)/../build-aux/install-sh build-aux-dir
	cp $(srcdir)/../build-aux/install-sh build-aux/install-sh

build-aux/ltmain.sh: $(srcdir)/../build-aux/ltmain.sh build-aux-dir
	cp $(srcdir)/../build-aux/ltmain.sh build-aux/ltmain.sh

build-aux/mkinstalldirs: $(srcdir)/../build-aux/mkinstalldirs build-aux-dir
	cp $(srcdir)/../build-aux/mkinstalldirs build-aux/mkinstalldirs

build-aux/missing: $(srcdir)/../build-aux/missing build-aux-dir
	cp $(srcdir)/../build-aux/missing build-aux/missing

build-aux/config.rpath: $(srcdir)/config.rpath build-aux-dir
	cp $(srcdir)/config.rpath build-aux/config.rpath

DISTCLEANFILES += build-aux/config.guess build-aux/config.sub build-aux/install-sh build-aux/ltmain.sh build-aux/mkinstalldirs build-aux/missing build-aux/config.rpath

distclean-local:
	rm -rf build-aux build-aux-dir


# Subdirectories with separate autoconfiguration.

CONFIGURES_RPATHX = $(srcdir)/tests/rpathx/Makefile.in $(srcdir)/tests/rpathx/aclocal.m4 $(srcdir)/tests/rpathx/configure
$(srcdir)/tests/rpathx/Makefile.in: $(srcdir)/tests/rpathx/Makefile.am $(srcdir)/tests/rpathx/configure.ac $(srcdir)/tests/rpathx/aclocal.m4
	cd $(srcdir)/tests/rpathx && automake Makefile
$(srcdir)/tests/rpathx/aclocal.m4: $(srcdir)/tests/rpathx/configure.ac $(srcdir)/../m4/libtool.m4
	cd $(srcdir)/tests/rpathx && aclocal -I `cd ../../m4 && pwd` -I `cd ../../../m4 && pwd`
$(srcdir)/tests/rpathx/configure: $(srcdir)/tests/rpathx/configure.ac $(srcdir)/tests/rpathx/aclocal.m4
	cd $(srcdir)/tests/rpathx && autoconf

CONFIGURES_RPATHY = $(srcdir)/tests/rpathy/Makefile.in $(srcdir)/tests/rpathy/aclocal.m4 $(srcdir)/tests/rpathy/configure
$(srcdir)/tests/rpathy/Makefile.in: $(srcdir)/tests/rpathy/Makefile.am $(srcdir)/tests/rpathy/configure.ac $(srcdir)/tests/rpathy/aclocal.m4
	cd $(srcdir)/tests/rpathy && automake Makefile
$(srcdir)/tests/rpathy/aclocal.m4: $(srcdir)/tests/rpathy/configure.ac $(srcdir)/../m4/libtool.m4 $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathy && aclocal -I `cd ../../m4 && pwd` -I `cd ../../../m4 && pwd`
$(srcdir)/tests/rpathy/configure: $(srcdir)/tests/rpathy/configure.ac $(srcdir)/tests/rpathy/aclocal.m4
	cd $(srcdir)/tests/rpathy && autoconf

CONFIGURES_RPATHZ = $(srcdir)/tests/rpathz/Makefile.in $(srcdir)/tests/rpathz/aclocal.m4 $(srcdir)/tests/rpathz/configure
$(srcdir)/tests/rpathz/Makefile.in: $(srcdir)/tests/rpathz/Makefile.am $(srcdir)/tests/rpathz/configure.ac $(srcdir)/tests/rpathz/aclocal.m4
	cd $(srcdir)/tests/rpathz && automake Makefile
$(srcdir)/tests/rpathz/aclocal.m4: $(srcdir)/tests/rpathz/configure.ac $(srcdir)/../m4/libtool.m4 $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathz && aclocal -I `cd ../../m4 && pwd` -I `cd ../../../m4 && pwd`
$(srcdir)/tests/rpathz/configure: $(srcdir)/tests/rpathz/configure.ac $(srcdir)/tests/rpathz/aclocal.m4
	cd $(srcdir)/tests/rpathz && autoconf

CONFIGURES_RPATHLX = $(srcdir)/tests/rpathlx/Makefile.in $(srcdir)/tests/rpathlx/aclocal.m4 $(srcdir)/tests/rpathlx/configure
$(srcdir)/tests/rpathlx/Makefile.in: $(srcdir)/tests/rpathlx/Makefile.am $(srcdir)/tests/rpathlx/configure.ac $(srcdir)/tests/rpathlx/aclocal.m4
	cd $(srcdir)/tests/rpathlx && automake Makefile
$(srcdir)/tests/rpathlx/aclocal.m4: $(srcdir)/tests/rpathlx/configure.ac $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathlx && aclocal -I `cd ../../m4 && pwd`
$(srcdir)/tests/rpathlx/configure: $(srcdir)/tests/rpathlx/configure.ac $(srcdir)/tests/rpathlx/aclocal.m4
	cd $(srcdir)/tests/rpathlx && autoconf

CONFIGURES_RPATHLY = $(srcdir)/tests/rpathly/Makefile.in $(srcdir)/tests/rpathly/aclocal.m4 $(srcdir)/tests/rpathly/configure
$(srcdir)/tests/rpathly/Makefile.in: $(srcdir)/tests/rpathly/Makefile.am $(srcdir)/tests/rpathly/configure.ac $(srcdir)/tests/rpathly/aclocal.m4
	cd $(srcdir)/tests/rpathly && automake Makefile
$(srcdir)/tests/rpathly/aclocal.m4: $(srcdir)/tests/rpathly/configure.ac $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathly && aclocal -I `cd ../../m4 && pwd`
$(srcdir)/tests/rpathly/configure: $(srcdir)/tests/rpathly/configure.ac $(srcdir)/tests/rpathly/aclocal.m4
	cd $(srcdir)/tests/rpathly && autoconf

CONFIGURES_RPATHLYX = $(srcdir)/tests/rpathlyx/Makefile.in $(srcdir)/tests/rpathlyx/aclocal.m4 $(srcdir)/tests/rpathlyx/configure
$(srcdir)/tests/rpathlyx/Makefile.in: $(srcdir)/tests/rpathlyx/Makefile.am $(srcdir)/tests/rpathlyx/configure.ac $(srcdir)/tests/rpathlyx/aclocal.m4
	cd $(srcdir)/tests/rpathlyx && automake Makefile
$(srcdir)/tests/rpathlyx/aclocal.m4: $(srcdir)/tests/rpathlyx/configure.ac $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathlyx && aclocal -I `cd ../../m4 && pwd`
$(srcdir)/tests/rpathlyx/configure: $(srcdir)/tests/rpathlyx/configure.ac $(srcdir)/tests/rpathlyx/aclocal.m4
	cd $(srcdir)/tests/rpathlyx && autoconf

CONFIGURES_RPATHLZ = $(srcdir)/tests/rpathlz/Makefile.in $(srcdir)/tests/rpathlz/aclocal.m4 $(srcdir)/tests/rpathlz/configure
$(srcdir)/tests/rpathlz/Makefile.in: $(srcdir)/tests/rpathlz/Makefile.am $(srcdir)/tests/rpathlz/configure.ac $(srcdir)/tests/rpathlz/aclocal.m4
	cd $(srcdir)/tests/rpathlz && automake Makefile
$(srcdir)/tests/rpathlz/aclocal.m4: $(srcdir)/tests/rpathlz/configure.ac $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathlz && aclocal -I `cd ../../m4 && pwd`
$(srcdir)/tests/rpathlz/configure: $(srcdir)/tests/rpathlz/configure.ac $(srcdir)/tests/rpathlz/aclocal.m4
	cd $(srcdir)/tests/rpathlz && autoconf

CONFIGURES_RPATHLZYX = $(srcdir)/tests/rpathlzyx/Makefile.in $(srcdir)/tests/rpathlzyx/aclocal.m4 $(srcdir)/tests/rpathlzyx/configure
$(srcdir)/tests/rpathlzyx/Makefile.in: $(srcdir)/tests/rpathlzyx/Makefile.am $(srcdir)/tests/rpathlzyx/configure.ac $(srcdir)/tests/rpathlzyx/aclocal.m4
	cd $(srcdir)/tests/rpathlzyx && automake Makefile
$(srcdir)/tests/rpathlzyx/aclocal.m4: $(srcdir)/tests/rpathlzyx/configure.ac $(srcdir)/m4/lib-link.m4 $(srcdir)/m4/lib-prefix.m4 $(srcdir)/m4/lib-ld.m4
	cd $(srcdir)/tests/rpathlzyx && aclocal -I `cd ../../m4 && pwd`
$(srcdir)/tests/rpathlzyx/configure: $(srcdir)/tests/rpathlzyx/configure.ac $(srcdir)/tests/rpathlzyx/aclocal.m4
	cd $(srcdir)/tests/rpathlzyx && autoconf

SUBCONFIGURES = $(CONFIGURES_RPATHX) $(CONFIGURES_RPATHY) $(CONFIGURES_RPATHZ) $(CONFIGURES_RPATHLX) $(CONFIGURES_RPATHLY) $(CONFIGURES_RPATHLYX) $(CONFIGURES_RPATHLZ) $(CONFIGURES_RPATHLZYX)

subconfigures: $(SUBCONFIGURES)

# Hidden from automake, but really activated. Works around an automake-1.5 bug.
#distdir: subconfigures
