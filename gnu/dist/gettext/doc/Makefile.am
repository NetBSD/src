## Makefile for the doc subdirectory of the GNU NLS Utilities
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

SED = sed

info_TEXINFOS = gettext.texi
gettext_TEXINFOS = iso-apdx.texi

EXTRA_DIST = iso-apdx.sed ISO_639 nls.texi matrix.texi

iso-apdx.texi: ISO_639 iso-apdx.sed
	$(SED) -f $(srcdir)/iso-apdx.sed $(srcdir)/ISO_639 > iso-apdx.tmp
	rm -f $(srcdir)/iso-apdx.texi
	mv iso-apdx.tmp $(srcdir)/iso-apdx.texi
