## Makefile for the toplevel directory of GNU gettext
## Copyright (C) 1995-1999, 2000-2005 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies

SUBDIRS = autoconf-lib-link gettext-runtime gettext-tools

EXTRA_DIST = \
  version.sh PACKAGING ChangeLog.0 autogen.sh \
  build-aux/config.libpath build-aux/reloc-ldflags build-aux/install-reloc \
  build-aux/ac-help.sed \
  m4/fixautomake.m4 \
  m4/libtool.m4

# Additional dependencies for configure, due to the use of autoconf --trace.
$(srcdir)/configure: $(srcdir)/autoconf-lib-link/configure.ac $(srcdir)/gettext-runtime/configure.ac $(srcdir)/gettext-tools/configure.ac 


# DJGPP port.

EXTRA_DIST += djgpp/COPYING.DJ djgpp/Makefile.maint djgpp/README.DJ \
              djgpp/README.in djgpp/README djgpp/config.bat djgpp/config.in \
              djgpp/config.sed djgpp/config.site djgpp/djpwd.h \
              djgpp/edtests.bat djgpp/fnchange.in djgpp/fnchange.lst \
              djgpp/getpwnam.c djgpp/msginit.sed djgpp/tscript.sed \
              djgpp/unsetenv.c


# OS/2 port.

EXTRA_DIST += os2/README.OS2 os2/Makefile os2/backward.def os2/configure.awk


# VMS support.

EXTRA_DIST += README.vms Makefile.vms \
              vms/link_options.opt \
              vms/with-libiconv.opt vms/without-libiconv.opt


# Woe32 support.

EXTRA_DIST += README.woe32 Makefile.msvc
EXTRA_DIST += README.mingw
