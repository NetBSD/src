# Makefile for GNU tar documentation.

# Copyright 1994, 1995, 1996, 1997, 1999, 2000, 2001 Free Software
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
## along with this program; if not, write to the Free Software Foundation,
## Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

AUTOMAKE_OPTIONS = gnits
info_TEXINFOS = tar.texi

EXTRA_DIST = convtexi.pl fdl.texi freemanuals.texi getdate.texi header.texi

CLEANFILES = tmp-*

# The rendering level is anyone of PUBLISH, DISTRIB or PROOF.
# Just call `make dvi RENDITION=PROOF' if you want PROOF rendition.
RENDITION = DISTRIB

$(srcdir)/tar.info: tar.texi fdl.texi freemanuals.texi getdate.texi \
  $(srcdir)/header.texi version.texi
	@echo "WARNING: \`makeinfo' will not work properly if it comes from a"
	@echo "         Texinfo distribution that is earlier than version 4.0."
	$(MAKEINFO) --no-split -D$(RENDITION) -I$(srcdir) tar.texi -o $@

tar.dvi: tar.texi getdate.texi $(srcdir)/header.texi version.texi
	@echo "WARNING: \`makeinfo' will not work properly if it comes from a"
	@echo "         Texinfo distribution that is earlier than version 4.0."
	$(MAKEINFO) -D$(RENDITION) -Etmp-tar.tmp -otmp-tar.info \
	  -I$(srcdir) tar.texi
	rm -f tmp-tar.sed tmp-tar.info*
	test $(RENDITION) = DISTRIB \
	  || echo >>tmp-tar.sed 's/^@set DISTRIB/@set $(RENDITION)/'
	test $(RENDITION) = PUBLISH \
	  || echo >>tmp-tar.sed '/^@smallbook/d'
	sed -f tmp-tar.sed tmp-tar.tmp > tmp-tar.texi
	rm -f tmp-tar.sed tmp-tar.tmp
	TEXINPUTS=$(srcdir):$$TEXINPUTS $(TEXI2DVI) tmp-tar.texi
	mv tmp-tar.dvi $@

$(srcdir)/header.texi: $(top_srcdir)/src/tar.h
	sed -n '/Archive Format/,/End of Format/p' $(top_srcdir)/src/tar.h \
	  | expand | sed 's/\([{}]\)/@\1/g' >$@
