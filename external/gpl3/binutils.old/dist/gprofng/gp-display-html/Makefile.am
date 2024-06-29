## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2021 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I . -I .. -I ../.. 

bin_SCRIPTS   = gp-display-html
CLEANFILES    = $(bin_SCRIPTS)

do_subst = sed -e 's/BINUTILS_VERSION/$(VERSION)/'

gp-display-html: gp-display-html.in Makefile
	$(do_subst) < $(srcdir)/gp-display-html.in > $@
	chmod +x $@

if BUILD_MAN

man_MANS = gp-display-html.1
MAINTAINERCLEANFILES = $(man_MANS)

# Use this if the man pages depend on the version number. 
# common_mandeps = $(top_srcdir)/../bfd/version.m4
#
# Also change the dependence line below to this:
# gp-display-html.1: $(common_mandeps) gp-display-html
#
# Currently, the version number shown in the man page is derived from
# the output printed with --version.

# These variables are used by help2man to generate the man pages.

INFO_PAGE             = "gprofng"
MANUAL                = "User Commands"
TEXT_GP_DISPLAY_HTML  = "generate an HTML based directory structure to browse the profiles"

HELP2MAN_OPT = --libtool --no-info --info-page=$(INFO_PAGE) --manual=$(MANUAL)
H2M_FILTER = | sed 's/\.TP/\.TP\n.B/' | sed 's/Commands:/\.SH COMMANDS/' \
  | sed 's/See also:/\.SH SEE ALSO/' | sed 's/Documentation:/.SH DOCUMENTATION/' \
  | sed 's/Limitations:/.SH LIMITATIONS/'

gp-display-html.1: gp-display-html
	$(AM_V_GEN)_BUILDING_MANPAGE=1 $(HELP2MAN) $(HELP2MAN_OPT) \
	  --name=$(TEXT_GP_DISPLAY_HTML) ./gp-display-html $(H2M_FILTER) > $@

endif

