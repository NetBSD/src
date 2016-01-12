## Makefile for the gettext-runtime/tests subdirectory of GNU gettext
## Copyright (C) 2005-2006 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.5 gnits no-dependencies
EXTRA_DIST =

TESTS = test-lock

AM_CPPFLAGS = \
  -I.. \
  -I$(srcdir)/../intl

noinst_PROGRAMS = test-lock
test_lock_SOURCES = test-lock.c ../intl/lock.c
test_lock_LDADD = @LIBMULTITHREAD@ @LIBSCHED@

# Clean up after Solaris cc.
clean-local:
	rm -rf SunWS_cache
