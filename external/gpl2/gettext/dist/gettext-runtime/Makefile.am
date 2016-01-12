## Makefile for the gettext-runtime directory of GNU gettext
## Copyright (C) 1995-1999, 2000-2006 Free Software Foundation, Inc.
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
## along with this program; if not, write to the Free Software Foundation,
## Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies
ACLOCAL_AMFLAGS = -I m4 -I ../autoconf-lib-link/m4 -I ../m4 -I gnulib-m4

MAKEINFO = env LANG= LC_MESSAGES= LC_ALL= LANGUAGE= @MAKEINFO@

# The list of subdirectories depends on whether --disable-libasprintf was
# specified.
if ENABLE_LIBASPRINTF
SUBDIR_libasprintf = libasprintf
else
SUBDIR_libasprintf =
endif
SUBDIRS = doc intl intl-java intl-csharp gnulib-lib $(SUBDIR_libasprintf) src po m4

EXTRA_DIST = BUGS


# Files installed for the user and for use by gettextize.

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = ABOUT-NLS


all-local: ABOUT-NLS

ABOUT-NLS: $(srcdir)/doc/nls.texi $(srcdir)/doc/matrix.texi
	rm -f $(srcdir)/ABOUT-NLS \
	  && $(MAKEINFO) --output=$(srcdir)/ABOUT-NLS --no-header \
	    -I $(srcdir)/doc --no-validate nls.texi


# Allow users to use "gnulib-tool --update".
EXTRA_DIST += gnulib-m4/gnulib-cache.m4


# Woe32 support.

EXTRA_DIST += README.woe32
