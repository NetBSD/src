# Makefile for GNU tar regression tests.

# Copyright (C) 1996, 1997, 1999, 2000, 2001 Free Software Foundation, Inc.

# François Pinard <pinard@iro.umontreal.ca>, 1988.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.

AUTOMAKE_OPTIONS = gnits
check_PROGRAMS = genfile
BUILT_SOURCES = preset

TESTS = version.sh \
  append.sh delete01.sh delete02.sh delete03.sh \
  extrac01.sh extrac02.sh extrac03.sh extrac04.sh \
  gzip.sh incremen.sh ignfail.sh \
  old.sh volume.sh

genfile_SOURCES = genfile.c
EXTRA_DIST = after before preset.in $(TESTS)

localedir = $(datadir)/locale
INCLUDES = -I../intl -I$(top_srcdir)/src -I../lib -I$(top_srcdir)/lib

# Link libtar.a after @INTLLIBS@, since @INTLLIBS@ might invoke rpl_realloc.
# Also, link it before @INTTLIBS@, since unicodeio.c might invoke
# locale_charset.  Sigh.
LDADD = ../lib/libtar.a @INTLLIBS@ ../lib/libtar.a

mostlyclean:
	rm -rf tmp-*

$(TESTS): genfile

# Local Variables:
# coding: iso-latin-1
# End:
