# Makefile for GNU tar sources.
# Copyright (C) 1994, 1995, 1996, 1997, 1999, 2000, 2001 Free Software
# Foundation, Inc.

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
bin_PROGRAMS = tar
libexec_PROGRAMS = @RMT@
EXTRA_PROGRAMS = rmt

noinst_HEADERS = arith.h common.h rmt.h system.h tar.h
rmt_SOURCES = rmt.c
tar_SOURCES = buffer.c compare.c create.c delete.c extract.c \
  incremen.c list.c mangle.c misc.c names.c rtapelib.c tar.c update.c

localedir = $(datadir)/locale
INCLUDES = -I../intl -I../lib -I$(top_srcdir)/lib

DISTCLEANFILES = localedir.h
localedir.h : Makefile
	echo '#define LOCALEDIR "$(localedir)"' >$@
rmt.o tar.o : localedir.h

# Link libtar.a after @INTLLIBS@, since @INTLLIBS@ might invoke rpl_realloc.
# Also, link it before @INTTLIBS@, since unicodeio.c might invoke
# locale_charset.  Sigh.
LDADD = ../lib/libtar.a @INTLLIBS@ ../lib/libtar.a
rmt_LDADD = $(LDADD) @rmt_LDADD@
tar_LDADD = $(LDADD) @tar_LDADD@
