## Makefile for program src directory in GNU NLS utilities package.
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

bin_PROGRAMS = gettext msgcmp msgfmt msgmerge msgunfmt xgettext msgcomm

noinst_HEADERS = domain.h message.h po-gram.h po-hash.h po-lex.h po.h \
str-list.h xget-lex.h po-gram.gen.h po-hash.gen.h dir-list.h

EXTRA_DIST = po-gram.y po-hash.y

datadir = $(prefix)/@DATADIRNAME@
localedir = $(datadir)/locale

l = @l@

INCLUDES = -I$(srcdir) -I.. -I$(top_srcdir)/lib -I../intl -I$(top_srcdir)/intl
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LDADD = ../lib/libnlsut.a ../intl/libintl.$la

SED = sed
YACC = @YACC@
YFLAGS = -d

# Source dependecies.
gettext_SOURCES = gettextp.c
msgcmp_SOURCES = message.c msgcmp.c open-po.c po-gram.gen.c po-hash.gen.c \
po-lex.c po.c str-list.c dir-list.c
msgfmt_SOURCES = msgfmt.c open-po.c po-gram.gen.c po-hash.gen.c po-lex.c po.c \
str-list.c message.c dir-list.c
msgmerge_SOURCES = message.c msgmerge.c open-po.c po-gram.gen.c po-hash.gen.c \
po-lex.c po.c str-list.c dir-list.c
msgunfmt_SOURCES = message.c msgunfmt.c str-list.c
xgettext_SOURCES = message.c open-po.c po-gram.gen.c po-hash.gen.c po-lex.c \
po.c str-list.c xget-lex.c xgettext.c dir-list.c
msgcomm_SOURCES = msgcomm.c message.c po-gram.gen.c po-hash.gen.c po-lex.c \
open-po.c po.c str-list.c dir-list.c

MAINTAINERCLEANFILES = po-gram.gen.c po-gram.gen.h po-hash.gen.c po-hash.gen.h

# Some rules for yacc handling.
#po-gram.gen.c po-gram.gen.h: po-gram.y
#	$(YACC) $(YFLAGS) $(srcdir)/po-gram.y
#	if test -s y.tab.c && test -s y.tab.h; then \
#	  rm -f $(srcdir)/po-gram.gen.c $(srcdir)/po-gram.gen.h; \
#	  $(SED) 's/[yY][yY]/po_gram_/g' < y.tab.c > $(srcdir)/po-gram.gen.c; \
#	  $(SED) 's/[yY][yY]/po_gram_/g' < y.tab.h > $(srcdir)/po-gram.gen.h; \
#	fi
#	rm -f y.tab.c y.tab.h

#po-hash.gen.c po-hash.gen.h: po-hash.y
#	$(YACC) $(YFLAGS) $(srcdir)/po-hash.y
#	if test -s y.tab.c && test -s y.tab.h; then \
#	  rm -f $(srcdir)/po-hash.gen.c $(srcdir)/po-hash.gen.h; \
#	  $(SED) 's/[yY][yY]/po_hash_/g' < y.tab.c > $(srcdir)/po-hash.gen.c; \
#	  $(SED) 's/[yY][yY]/po_hash_/g' < y.tab.h > $(srcdir)/po-hash.gen.h; \
#	fi
#	rm -f y.tab.c y.tab.h
