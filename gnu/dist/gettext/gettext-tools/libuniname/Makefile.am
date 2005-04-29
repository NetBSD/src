## Makefile for libuniname directory in GNU gettext package.
## Copyright (C) 2002-2003, 2005 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.5 gnits no-dependencies
EXTRA_DIST =


noinst_LIBRARIES = libuniname.a

libuniname_a_SOURCES = uniname.h uniname.c uninames.h

AM_CPPFLAGS = -I.. -I../lib -I$(top_srcdir)/lib

EXTRA_DIST += gen-uninames


TESTS = test-names.sh
TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) $(SHELL)

noinst_PROGRAMS = test-names
# INTL_MACOSX_LIBS is needed because libgettextlib.la depends on libintl.la
# but libtool doesn't put -Wl,-framework options into .la files.
test_names_LDADD = libuniname.a ../lib/libgettextlib.la @INTL_MACOSX_LIBS@

EXTRA_DIST += $(TESTS) UnicodeDataNames.txt


# VMS support.

EXTRA_DIST += Makefile.vms


# Woe32 support.

EXTRA_DIST += Makefile.msvc
