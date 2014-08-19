# Copyright 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
# This Makefile.am is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


AUTOMAKE_OPTIONS = gnu
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc src tests tune

nobase_dist_doc_DATA = AUTHORS BUGS COPYING COPYING.LESSER NEWS TODO \
  examples/ReadMe examples/divworst.c examples/rndo-add.c examples/sample.c \
  examples/version.c

EXTRA_DIST = PATCHES VERSION \
  tools/ck-copyright-notice tools/ck-version-info tools/get_patches.sh \
  $(DATAFILES)

# Various checks for "make dist".
# * Check consistency concerning -version-info. Moreover if the VERSION
#   file doesn't end with "-dev", check that the -version-info value is
#   up-to-date. Note: this is a heuristic, to detect some mistakes.
# * Check that copyright notices exist and appear to be correct.
dist-hook:
	cd $(srcdir) && tools/ck-version-info
	cd $(srcdir) && tools/ck-copyright-notice
