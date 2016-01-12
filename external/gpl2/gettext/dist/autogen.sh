#!/bin/sh
# Convenience script for regenerating all aclocal.m4, config.h.in, Makefile.in,
# configure files with new versions of autoconf or automake.
#
# This script requires autoconf-2.60..2.61 and automake-1.10 in the PATH.
# It also requires either
#   - the GNULIB_TOOL environment variable pointing to the gnulib-tool script
#     in a gnulib checkout, or
#   - the cvs program in the PATH and an internet connection.

# Copyright (C) 2003-2006 Free Software Foundation, Inc.
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
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# Usage: ./autogen.sh [--quick]

if test "x$1" = "x--quick"; then
  quick=true
else
  quick=false
fi

if test -z "$GNULIB_TOOL"; then
  # Check out gnulib in a subdirectory 'gnulib'.
  GNULIB_CVS_ROOT=':pserver:anonymous@cvs.savannah.gnu.org:/sources/gnulib'
  GNULIB_CVS_REPOSITORY='gnulib'
  if test -d gnulib; then
    (cd gnulib && cvs update -d -P)
  else
    cvs -d "$GNULIB_CVS_ROOT" checkout $GNULIB_CVS_REPOSITORY
  fi
  # Now it should contain a gnulib-tool.
  if test -f gnulib/gnulib-tool; then
    GNULIB_TOOL=`pwd`/gnulib/gnulib-tool
  else
    echo "** warning: gnulib-tool not found" 1>&2
  fi
fi
# Skip the gnulib-tool step if gnulib-tool was not found.
if test -n "$GNULIB_TOOL"; then
  # In gettext-runtime:
  GNULIB_MODULES_FOR_SRC='
  atexit
  basename
  closeout
  error
  exit
  getopt
  gettext-h
  memmove
  progname
  propername
  relocatable
  relocwrapper
  stdbool
  strtoul
  unlocked-io
  xalloc
  '
  GNULIB_MODULES_OTHER='
  gettext-runtime-misc
  csharpcomp-script
  java
  javacomp-script
  '
  $GNULIB_TOOL --dir=gettext-runtime --lib=libgrt --source-base=gnulib-lib --m4-base=gnulib-m4 --no-libtool --local-dir=gnulib-local \
    --import $GNULIB_MODULES_FOR_SRC $GNULIB_MODULES_OTHER
  # In gettext-tools:
  GNULIB_MODULES_FOR_SRC='
  alloca-opt
  atexit
  backupfile
  basename
  binary-io
  bison-i18n
  byteswap
  c-ctype
  c-strcase
  c-strcasestr
  c-strstr
  clean-temp
  closeout
  copy-file
  csharpcomp
  csharpexec
  error
  error-progname
  execute
  exit
  findprog
  fnmatch-posix
  fstrcmp
  full-write
  fwriteerror
  gcd
  getline
  getopt
  gettext-h
  hash
  iconv
  javacomp
  javaexec
  linebreak
  localcharset
  lock
  memmove
  memset
  minmax
  obstack
  pathname
  pipe
  progname
  propername
  relocatable
  relocwrapper
  sh-quote
  stdbool
  stpcpy
  stpncpy
  strcspn
  xstriconv
  strpbrk
  strtol
  strtoul
  ucs4-utf8
  unistd
  unlocked-io
  utf8-ucs4
  utf16-ucs4
  vasprintf
  wait-process
  xalloc
  xallocsa
  xerror
  xsetenv
  xstriconv
  xvasprintf
  '
  # Not yet used. Add some files to gettext-tools-misc instead.
  GNULIB_MODULES_FOR_LIBGREP='
  error
  exitfail
  gettext-h
  hard-locale
  obstack
  regex
  stdbool
  xalloc
  '
  GNULIB_MODULES_OTHER='
  gettext-tools-misc
  gcj
  java
  '
  $GNULIB_TOOL --dir=gettext-tools --lib=libgettextlib --source-base=gnulib-lib --m4-base=gnulib-m4 --libtool --local-dir=gnulib-local \
    --import $GNULIB_MODULES_FOR_SRC $GNULIB_MODULES_OTHER
  # In gettext-tools/libgettextpo:
  # This is a subset of the GNULIB_MODULES_FOR_SRC.
  GNULIB_MODULES_FOR_LIBGETTEXTPO='
  basename
  c-ctype
  c-strcase
  c-strstr
  error
  error-progname
  exit
  fstrcmp
  fwriteerror
  gcd
  getline
  gettext-h
  hash
  iconv
  linebreak
  minmax
  pathname
  progname
  stdbool
  ucs4-utf8
  unlocked-io
  utf8-ucs4
  utf16-ucs4
  vasprintf
  xalloc
  xallocsa
  xerror
  xstriconv
  xvasprintf
  '
  GNULIB_MODULES_OTHER='
  gettext-tools-libgettextpo-misc
  '
  $GNULIB_TOOL --dir=gettext-tools --source-base=libgettextpo --m4-base=libgettextpo/gnulib-m4 --macro-prefix=gtpo --makefile-name=Makefile.gnulib --libtool --local-dir=gnulib-local \
    --import $GNULIB_MODULES_FOR_LIBGETTEXTPO $GNULIB_MODULES_OTHER
fi

(cd autoconf-lib-link
 aclocal -I m4 -I ../m4
 autoconf
 automake
)

(cd gettext-runtime
 aclocal -I m4 -I ../autoconf-lib-link/m4 -I ../m4 -I gnulib-m4
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
 aclocal -I m4 -I ../gettext-runtime/m4 -I ../autoconf-lib-link/m4 -I ../m4 -I gnulib-m4 -I libgettextpo/gnulib-m4
 autoconf
 autoheader && touch config.h.in
 automake
)

(cd gettext-tools/examples
 aclocal -I ../../gettext-runtime/m4 -I ../../m4
 autoconf
 automake
 # Rebuilding the examples PO files is only rarely needed.
 if ! $quick; then
   ./configure && (cd po && make update-po) && make distclean
 fi
)

aclocal
autoconf
automake

cp -p autoconf-lib-link/config.rpath build-aux/config.rpath
