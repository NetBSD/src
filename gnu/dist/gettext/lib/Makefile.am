## Makefile for the support lib subdirectory of the GNU NLS Utilities
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

AUTOMAKE_OPTIONS = 1.2 gnits

noinst_LIBRARIES = libnlsut.a

EXTRA_DIST = alloca.c error.c getline.c memset.c memmove.c \
printf-prs.c stpcpy.c stpncpy.c strcasecmp.c strcspn.c  strncasecmp.c \
strstr.c strtol.c strtoul.c vasprintf.c

libnlsut_a_SOURCES = basename.c fstrcmp.c getopt.c getopt1.c hash.c obstack.c \
xgetcwd.c xmalloc.c xstrdup.c

libnlsut_a_LIBADD = @ALLOCA@ @LIBOBJS@

noinst_HEADERS = error.h fstrcmp.h getline.h getopt.h hash.h obstack.h \
printf-parse.h printf.h system.h pathmax.h

INCLUDES = -I../intl
