# Convenience script for regenerating all aclocal.m4, config.h.in, Makefile.in,
# configure files with new versions of autoconf or automake.
#
# This script requires autoconf-2.58..2.59 and automake-1.8.2..1.9 in the PATH.

# Copyright (C) 2003-2004 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

aclocal
autoconf
automake

(cd autoconf-lib-link
 aclocal -I m4 -I ../m4
 autoconf
 automake
)

(cd gettext-runtime
 aclocal -I m4 -I ../gettext-tools/m4 -I ../autoconf-lib-link/m4 -I ../m4
 autoconf
 autoheader && touch config.h.in
 automake
)

(cd gettext-runtime/libasprintf
 aclocal -I ../../m4 -I ../m4
 autoconf
 autoheader && touch config.h.in
 automake
)

cp -p gettext-runtime/ABOUT-NLS gettext-tools/ABOUT-NLS

(cd gettext-tools
 aclocal -I m4 -I ../gettext-runtime/m4 -I ../autoconf-lib-link/m4 -I ../m4
 autoconf
 autoheader && touch config.h.in
 automake
)

cp -p autoconf-lib-link/config.rpath build-aux/config.rpath
