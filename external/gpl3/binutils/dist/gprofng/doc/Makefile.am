## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2012-2025 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.
#

AUTOMAKE_OPTIONS = info-in-builddir foreign no-texinfo.tex

DVIS  = gprofng.dvi
PDFS  = gprofng.pdf
HTMLS = gprofng.html

# Options to extract the man page
MANCONF = -Dman

TEXI2POD = perl $(srcdir)/../../etc/texi2pod.pl $(AM_MAKEINFOFLAGS)
POD2MAN = pod2man --center="User Commands" \
	--release="binutils-$(VERSION)" --section=1

info_TEXINFOS       = gprofng_ug.texi
gprofng_ug_TEXINFOS = fdl.texi gp-macros.texi
TEXINFO_TEX         = .
MAKEINFOHTML        = $(MAKEINFO) --html --no-split

man_MANS = gprofng.1 gprofng-archive.1 gprofng-collect-app.1 \
	gprofng-display-html.1 gprofng-display-src.1 gprofng-display-text.1

# Build the man page from the texinfo file
# The sed command removes the no-adjust Nroff command so that
# the man output looks standard.
$(man_MANS): $(srcdir)/gp-macros.texi
	$(AM_V_at)( nm=`basename $@ .1` ; \
	  $(TEXI2POD) $(MANCONF) < $(srcdir)/$$nm.texi > $$nm.pod ; \
	  $(POD2MAN) $$nm.pod | sed -e '/^.if n .na/d' > $$nm.tmp || exit 1 ; \
	  mv -f $$nm.tmp $@ ; \
	  rm -f $$nm.pod )

gprofng.1: $(srcdir)/gprofng.texi
gprofng-archive.1: $(srcdir)/gprofng-archive.texi
gprofng-collect-app.1: $(srcdir)/gprofng-collect-app.texi
gprofng-display-html.1: $(srcdir)/gprofng-display-html.texi
gprofng-display-src.1: $(srcdir)/gprofng-display-src.texi
gprofng-display-text.1: $(srcdir)/gprofng-display-text.texi

MAINTAINERCLEANFILES = gprofng.info $(man_MANS)
EXTRA_DIST = $(man_MANS) version.texi

info: $(man_MANS)

examples.tar.gz:
	$(AM_V_at)( tar czf $@ $(srcdir)/../examples )

.PHONY: install-data-local

install-data-local:  examples.tar.gz
	$(mkinstalldirs) $(DESTDIR)$(docdir)/gprofng
	$(INSTALL_DATA) examples.tar.gz $(DESTDIR)$(docdir)/gprofng

