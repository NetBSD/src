# Makefile for GNU tar library.

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

noinst_LIBRARIES = libtar.a

EXTRA_DIST = \
  alloca.c fileblocks.c fnmatch.c ftruncate.c lchown.c \
  malloc.c memset.c mktime.c realloc.c rename.c rmdir.c \
  stpcpy.c strcasecmp.c strncasecmp.c strstr.c \
  strtol.c strtoll.c strtoul.c strtoull.c strtoimax.c strtoumax.c utime.c \
  waitpid.c xstrtol.c

noinst_HEADERS = \
  argmatch.h backupfile.h dirname.h error.h exclude.h \
  fnmatch.hin full-write.h getdate.h getline.h getopt.h getstr.h \
  hash.h human.h lchown.h modechange.h prepargs.h print-copyr.h \
  quote.h quotearg.h \
  save-cwd.h savedir.h safe-read.h unicodeio.h xalloc.h xstrtol.h

libtar_a_SOURCES = \
  addext.c argmatch.c backupfile.c basename.c dirname.c error.c \
  exclude.c full-write.c getdate.y getopt.c getopt1.c getstr.c \
  hash.c human.c modechange.c msleep.c prepargs.c print-copyr.c \
  quote.c quotearg.c safe-read.c save-cwd.c savedir.c \
  unicodeio.c xgetcwd.c xmalloc.c xstrdup.c \
  xstrtoimax.c xstrtoul.c xstrtoumax.c

INCLUDES = -I../intl

libtar_a_LIBADD = @ALLOCA@ @LIBOBJS@
libtar_a_DEPENDENCIES = $(libtar_a_LIBADD)

DISTCLEANFILES = fnmatch.h fnmatch.hno

# Say $(srcdir), so GNU make does not report an ambiguity with the .y.c rule.
$(srcdir)/getdate.c: getdate.y
	cd $(srcdir) && \
	  $(YACC) $(YFLAGS) getdate.y && \
	  mv -f y.tab.c getdate.c
