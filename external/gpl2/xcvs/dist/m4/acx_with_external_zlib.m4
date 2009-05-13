# Allow the builder to specify an external ZLIB rather than the version
# distributed with CVS.

# Copyright (c) 2003
#		Anthon Pang <apang@telux.net>,
#               Derek R. Price, Ximbiot <http://ximbiot.com>,
#               and the Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# ACX_WITH_EXTERNAL_ZLIB
# -------------------
# Determine which ZLIB to use.
AC_DEFUN([ACX_WITH_EXTERNAL_ZLIB],[
#
# Use --with-zlib to build with a zlib other than the version distributed
# with CVS.
#
# defaults to the included snapshot of zlib
#
AC_ARG_WITH([external-zlib],
            AC_HELP_STRING([--with-external-zlib],
                           [Use the specified ZLIB library (defaults to
                            the version distributed with the CVS source)]),
            [with_external_zlib=$withval],
            [with_external_zlib=no])

#
# Try to locate a ZLIB installation if no location was specified, assuming
# external ZLIB was enabled.
# 
if test -n "$acx_zlib_cv_external_zlib"; then
  # Granted, this is a slightly ugly way to print this info, but the
  # AC_CHECK_HEADER used in the search for a ZLIB installation makes using
  # AC_CACHE_CHECK worse
  AC_MSG_CHECKING([for external ZLIB])
else :; fi
AC_CACHE_VAL([acx_zlib_cv_external_zlib], [
  #
  # --with but no location specified
  # assume zlib.h locates our install.
  #
  acx_zlib_save_CPPFLAGS=$CPPFLAGS
  for acx_zlib_cv_external_zlib in yes /usr/local no; do
    if test x$acx_zlib_cv_external_zlib = xno; then
      break
    fi
    if test x$acx_zlib_cv_external_zlib = xyes; then
      AC_MSG_CHECKING([for external ZLIB])
      AC_MSG_RESULT([])
    else
      CPPFLAGS="$acx_zlib_save_CPPFLAGS -I$acx_zlib_cv_external_zlib/include"
      AC_MSG_CHECKING([for external ZLIB in $acx_zlib_cv_external_zlib])
      AC_MSG_RESULT([])
    fi
    unset ac_cv_header_zlib_h
    AC_CHECK_HEADERS([zlib.h])
    if test "$ac_cv_header_zlib_h" = yes; then
      break
    fi
  done
  CPPFLAGS=$acx_zlib_save_CPPFLAGS
AC_MSG_CHECKING([for external ZLIB])
])dnl
AC_MSG_RESULT([$acx_zlib_cv_external_zlib])


#
# Output a pretty message naming our selected ZLIB "external" or "package"
# so that any warnings printed by the version check make more sense.
#
AC_MSG_CHECKING([selected ZLIB])
if test "x$with_external_zlib" = xno; then
  AC_MSG_RESULT([package])
else
  AC_MSG_RESULT([external])
fi


#
# Verify that the ZLIB we aren't using isn't newer than the one we are.
#
if test "x$acx_zlib_cv_external_zlib" != xno; then
  LOCAL_ZLIB_VERSION=`sed -n '/^#define ZLIB_VERSION ".*"$/{
                              s/^#define ZLIB_VERSION "\(.*\)"$/\1/;
                              p;}' <$srcdir/zlib/zlib.h 2>&AS_MESSAGE_LOG_FD`
  ACX_EXTRACT_HEADER_DEFN([ZLIB_VERSION], [zlib.h])
  ZLIB_VERSION=`echo "$ZLIB_VERSION" |sed 's/"//g'`
  ASX_VERSION_COMPARE([$LOCAL_ZLIB_VERSION], [$ZLIB_VERSION],
    [if test "x$with_external_zlib" = xno; then
       AC_MSG_WARN(
         [Found external ZLIB with a more recent version than the
           package version ($ZLIB_VERSION > $LOCAL_ZLIB_VERSION).  configure with the
           --with-external-zlib option to select the more recent version.])
     fi],
    [],
    [if test "x$with_external_zlib" != xno; then
       AC_MSG_WARN(
         [Package ZLIB is more recent than requested external version
           ($LOCAL_ZLIB_VERSION > $ZLIB_VERSION).  configure with the --without-external-zlib
           option to select the more recent version.])
     fi])
fi


# Now set with_external_zlib to our discovered value or the user specified
# value, as appropriate.
if test x$with_external_zlib = xyes; then
  with_external_zlib=$acx_zlib_cv_external_zlib
fi
# $with_external_zlib could still be "no"


#
# Set up ZLIB includes for later use.
#
if test x$with_external_zlib != xyes \
   && test x$with_external_zlib != no; then
  if test -z "$CPPFLAGS"; then
    CPPFLAGS="-I$with_external_zlib/include"
  else
    CPPFLAGS="$CPPFLAGS -I$with_external_zlib/include"
  fi
  if test -z "$LDFLAGS"; then
    LDFLAGS="-L$with_external_zlib/lib"
  else
    LDFLAGS="$LDFLAGS -L$with_external_zlib/lib"
  fi
fi

ZLIB_CPPFLAGS=
ZLIB_LIBS=
ZLIB_SUBDIRS=
if test x$with_external_zlib = xno; then
  # We need ZLIB_CPPFLAGS so that later executions of cpp from configure
  # don't try to interpret $(top_srcdir)
  ZLIB_CPPFLAGS='-I$(top_srcdir)/zlib'
  ZLIB_LIBS='$(top_builddir)/zlib/libz.a'
  # ZLIB_SUBDIRS is only used in the top level Makefiles.
  ZLIB_SUBDIRS=zlib
else
  # We know what to do now, so set up the CPPFLAGS, LDFLAGS, and LIBS for later
  # use.
  if test -z "$LIBS"; then
    LIBS=-lz
  else
    LIBS="$LIBS -lz"
  fi

  #
  # Verify external installed zlib works
  #
  # Ideally, we would also check that the version is newer
  #
  AC_MSG_CHECKING([that ZLIB library works])
  AC_TRY_LINK([#include <zlib.h>],
              [int i = Z_OK; const char *version = zlibVersion();],
              [AC_MSG_RESULT([yes])],
              [AC_MSG_RESULT([no])
               AC_MSG_ERROR([ZLIB failed to link])])
fi

dnl Subst for the local case
AC_SUBST(ZLIB_SUBDIRS)dnl
AC_SUBST(ZLIB_CPPFLAGS)dnl
AC_SUBST(ZLIB_LIBS)dnl
])
