## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2012-2020 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = info-in-builddir no-texinfo.tex foreign

# What version of the manual you want; "all" includes everything
CONFIG=all

# Options to extract the man page from as.texinfo
MANCONF = -Dman

TEXI2POD = perl $(top_srcdir)/../etc/texi2pod.pl $(AM_MAKEINFOFLAGS)

POD2MAN = pod2man --center="GNU Development Tools" \
	 --release="binutils-$(VERSION)" --section=1

# List of man pages generated from binutils.texi
man_MANS = \
	addr2line.1 \
	ar.1 \
	dlltool.1 \
	nm.1 \
	objcopy.1 \
	objdump.1 \
	ranlib.1 \
	readelf.1 \
	size.1 \
	strings.1 \
	strip.1 \
	elfedit.1 \
	windres.1 \
	windmc.1 \
	$(DEMANGLER_NAME).1

info_TEXINFOS = binutils.texi
binutils_TEXI = $(srcdir)/binutils.texi

AM_MAKEINFOFLAGS = -I "$(srcdir)" -I "$(top_srcdir)/../libiberty" \
		   -I "$(top_srcdir)/../bfd/doc" -I ../../bfd/doc
TEXI2DVI = texi2dvi -I "$(srcdir)" -I "$(top_srcdir)/../libiberty" \
		    -I "$(top_srcdir)/../bfd/doc" -I ../../bfd/doc

# Man page generation from texinfo
addr2line.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_addr2line.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Daddr2line < $(binutils_TEXI) > addr2line.pod
	-($(POD2MAN) addr2line.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f addr2line.pod

ar.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_ar.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dar < $(binutils_TEXI) > ar.pod
	-($(POD2MAN) ar.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f ar.pod

dlltool.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_dlltool.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Ddlltool < $(binutils_TEXI) > dlltool.pod
	-($(POD2MAN) dlltool.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f dlltool.pod

nm.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_nm.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dnm < $(binutils_TEXI) > nm.pod
	-($(POD2MAN) nm.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f nm.pod

objcopy.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_objcopy.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dobjcopy < $(binutils_TEXI) > objcopy.pod
	-($(POD2MAN) objcopy.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f objcopy.pod

objdump.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_objdump.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dobjdump < $(binutils_TEXI) > objdump.pod
	-($(POD2MAN) objdump.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f objdump.pod

ranlib.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_ranlib.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dranlib < $(binutils_TEXI) > ranlib.pod
	-($(POD2MAN) ranlib.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f ranlib.pod

readelf.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_readelf.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dreadelf < $(binutils_TEXI) > readelf.pod
	-($(POD2MAN) readelf.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f readelf.pod

size.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_size.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dsize < $(binutils_TEXI) > size.pod
	-($(POD2MAN) size.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f size.pod

strings.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_strings.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dstrings < $(binutils_TEXI) > strings.pod
	-($(POD2MAN) strings.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f strings.pod

strip.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_strip.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dstrip < $(binutils_TEXI) > strip.pod
	-($(POD2MAN) strip.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f strip.pod

elfedit.1:	$(binutils_TEXI)
	touch $@
	-$(TEXI2POD) $(MANCONF) -Delfedit < $(binutils_TEXI) > elfedit.pod
	-($(POD2MAN) elfedit.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f elfedit.pod

windres.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_windres.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dwindres < $(binutils_TEXI) > windres.pod
	-($(POD2MAN) windres.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f windres.pod

windmc.1:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_windmc.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dwindmc < $(binutils_TEXI) > windmc.pod
	-($(POD2MAN) windmc.pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f windmc.pod

cxxfilt.man:	$(binutils_TEXI)
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_cxxfilt.man:
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dcxxfilt < $(binutils_TEXI) > $(DEMANGLER_NAME).pod
	-($(POD2MAN) $(DEMANGLER_NAME).pod | sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || (rm -f $@.T$$$$ && exit 1)
	rm -f $(DEMANGLER_NAME).pod

MOSTLYCLEANFILES     = $(man_MANS) binutils.info cxxfilt.man

$(DEMANGLER_NAME).1: cxxfilt.man Makefile
	if test -f cxxfilt.man; then \
	  man=cxxfilt.man; \
	else \
	  man=$(srcdir)/cxxfilt.man; \
	fi; \
	sed -e 's/@PROGRAM@/$(DEMANGLER_NAME)/' \
	    -e 's/cxxfilt/$(DEMANGLER_NAME)/' < $$man \
		> $(DEMANGLER_NAME).1

# Maintenance

# We need it for the taz target in ../../Makefile.in.
info-local: $(MANS)
