## Makefile for the gettext-tools/misc subdirectory of GNU gettext
## Copyright (C) 1995-1997, 2000-2003, 2005 Free Software Foundation, Inc.
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
EXTRA_DIST =
DISTCLEANFILES =


# Scripts for maintainers.

bin_SCRIPTS = gettextize autopoint

DISTCLEANFILES += gettextize autopoint

EXTRA_DIST += add-to-archive cvsuser.c archive.tar.gz

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = archive.tar.gz

# The archive.tar.gz is not in the CVS. Alpha-testers must use the one from an
# already installed release.
archive.tar.gz:
	cp -p $(gettextsrcdir)/archive.tar.gz $@


# Emacs Lisp code.

lisp_LISP = start-po.el po-mode.el po-compat.el

EXTRA_DIST += $(lisp_LISP)


# VMS support.

EXTRA_DIST += Makefile.vms


# Woe32 support.

EXTRA_DIST += Makefile.msvc
