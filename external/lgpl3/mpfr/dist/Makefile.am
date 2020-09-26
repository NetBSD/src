# Copyright 2000-2020 Free Software Foundation, Inc.
# This Makefile.am is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


# Since we do not require GNU "make", let's select the POSIX behavior to
# uniformize the behavior a bit more with various "make" implementations
# and ease maintenance. This target should be removed only if one day,
# we need to require GNU "make".
.POSIX:

AUTOMAKE_OPTIONS = gnu

# ACLOCAL_AMFLAGS will be fully deprecated in Automake 2.0;
# AC_CONFIG_MACRO_DIRS (introduced in Automake 1.13) is now used instead,
# but we still set ACLOCAL_AMFLAGS to avoid a warning message from
# libtoolize and in case some developer needs to switch back to an
# old Automake version.
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc src tests tune tools/bench

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mpfr.pc

nobase_dist_doc_DATA = AUTHORS BUGS COPYING COPYING.LESSER NEWS TODO \
  examples/ReadMe examples/can_round.c examples/divworst.c \
  examples/rndo-add.c examples/sample.c examples/threads.c \
  examples/version.c

EXTRA_DIST = PATCHES VERSION doc/README.dev doc/check-typography doc/mini-gmp \
  tools/check_mparam.c tools/ck-clz_tab tools/ck-copyright-notice \
  tools/ck-mparam tools/ck-news tools/ck-version-info tools/get_patches.sh \
  $(DATAFILES)

bench:
	cd tools/bench && $(MAKE) $(AM_MAKEFLAGS) bench

# Various checks for "make dist" / "make distcheck".
# Warning! With "make distcheck", neither the top-level directory nor
# the tools directory is writable.
# * Check consistency concerning -version-info. Moreover if the VERSION
#   file doesn't end with "-dev", check that the -version-info value is
#   up-to-date. Note: this is a heuristic, to detect some mistakes.
# * Check that copyright notices exist and appear to be correct.
# * Check the NEWS file.
# * Check the __clz_tab sizes.
# * Check the mparam.h files.
# * Check typography in mpfr.texi (Texinfo rules).
dist-hook:
	cd $(srcdir) && tools/ck-version-info
	cd $(srcdir) && tools/ck-copyright-notice
	cd $(srcdir) && tools/ck-news
	cd $(srcdir) && tools/ck-clz_tab
	$(srcdir)/tools/ck-mparam
	cd $(srcdir)/doc && ./check-typography

# Check that MPFR does not use GMP internal symbols. Of course, do not run
# this rule if you use --with-gmp-build or --enable-gmp-internals.
# This test does nothing if no libmpfr.so is generated.
check-gmp-symbols:
	cd src && $(MAKE) check-gmp-symbols

# Check that MPFR does not define symbols with a GMP reserved prefix.
# This test does nothing if no libmpfr.so is generated.
check-exported-symbols:
	cd src && $(MAKE) check-exported-symbols

.PHONY: check-gmp-symbols check-exported-symbols
