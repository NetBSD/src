## Makefile for the toplevel directory of the GNU NLS Utilities
## Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.3 gnits
MAINT_CHARSET = latin1
ACLOCAL_AMFLAGS = -I m4

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = ABOUT-NLS

EXTRA_DIST = BUGS DISCLAIM README.gemtext
SUBDIRS = doc lib intl src po m4 misc tests

DISTCLEANFILES = intl/libintl.h

#ABOUT-NLS: $(srcdir)/doc/nls.texi $(srcdir)/doc/matrix.texi
#	rm -f $(srcdir)/ABOUT-NLS \
#	  && $(MAKEINFO) --output=$(srcdir)/ABOUT-NLS --no-header \
#	    -I $(srcdir)/doc --no-validate nls.texi
