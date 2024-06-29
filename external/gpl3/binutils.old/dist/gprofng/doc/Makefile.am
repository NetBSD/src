## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2012-2021 Free Software Foundation, Inc.
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

info_TEXINFOS       = gprofng.texi
gprofng_TEXINFOS    = fdl.texi
BUILT_SOURCES       = version.texi
CLEANFILES          = version.texi
TEXINFO_TEX         = .
MAKEINFOHTML        = $(MAKEINFO) --html --no-split

version.texi: 
	@echo "@set EDITION 1.0"                    > $@
	@echo "@set VERSION 1.0"                   >> $@
	@echo "@set UPDATED 22 February 2022"      >> $@
	@echo "@set UPDATED-MONTH February 2022"   >> $@
#	@echo "@set UPDATED `date +"%-d %B %Y"`"   >> $@
#	@echo "@set UPDATED-MONTH `date +"%B %Y"`" >> $@

MAINTAINERCLEANFILES = gprofng.info
