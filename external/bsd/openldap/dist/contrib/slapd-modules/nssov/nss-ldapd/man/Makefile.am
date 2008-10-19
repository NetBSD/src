# Makefile.am - use automake to generate Makefile.in
#
# Copyright (C) 2007 Arthur de Jong
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

dist_man_MANS = nss-ldapd.conf.5 nslcd.8

EXTRA_DIST = nss-ldapd.conf.5.xml nslcd.8.xml

if GENMAN

MAINTAINERCLEANFILES = $(dist_man_MANS)

SUFFIXES = .xml
.xml:
	$(DOCBOOK2X_MAN) \
	  --string-param header-3='$(RELEASE_MONTH)' \
	  --string-param header-4='Version $(VERSION)' \
	  $<

endif
