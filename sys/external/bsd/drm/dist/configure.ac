#  Copyright 2005 Adam Jackson.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_PREREQ(2.57)
AC_INIT([libdrm], 2.4.11, [dri-devel@lists.sourceforge.net], libdrm)
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER([libdrm/config.h])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

AC_HEADER_STDC
AC_SYS_LARGEFILE

PKG_CHECK_MODULES(PTHREADSTUBS, pthread-stubs)
AC_SUBST(PTHREADSTUBS_CFLAGS)
AC_SUBST(PTHREADSTUBS_LIBS)

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)
AC_ARG_ENABLE(udev,	AS_HELP_STRING([--enable-udev],
				[Enable support for using udev instead of mknod (default: disabled)]),
				[UDEV=$enableval], [UDEV=no])

AC_ARG_ENABLE(nouveau-experimental-api,
	      AS_HELP_STRING([--enable-nouveau-experimental-api],
	      [Enable support for nouveau's experimental API (default: disabled)]),
	      [NOUVEAU=$enableval], [NOUVEAU=no])

dnl ===========================================================================
dnl check compiler flags
AC_DEFUN([LIBDRM_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  libdrm_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_COMPILE_IFELSE([ ], [libdrm_cc_flag=yes], [libdrm_cc_flag=no])
  CFLAGS="$libdrm_save_CFLAGS"

  if test "x$libdrm_cc_flag" = "xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT([$libdrm_cc_flag])
])

dnl We use clock_gettime to check for timeouts in drmWaitVBlank

AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=],
               [AC_CHECK_LIB([rt], [clock_gettime], [CLOCK_LIB=-lrt],
                             [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

dnl Use lots of warning flags with with gcc and compatible compilers

dnl Note: if you change the following variable, the cache is automatically
dnl skipped and all flags rechecked.  So there's no need to do anything
dnl else.  If for any reason you need to force a recheck, just change
dnl MAYBE_WARN in an ignorable way (like adding whitespace)

MAYBE_WARN="-Wall -Wextra \
-Wsign-compare -Werror-implicit-function-declaration \
-Wpointer-arith -Wwrite-strings -Wstrict-prototypes \
-Wmissing-prototypes -Wmissing-declarations -Wnested-externs \
-Wpacked -Wswitch-enum -Wmissing-format-attribute \
-Wstrict-aliasing=2 -Winit-self -Wunsafe-loop-optimizations \
-Wdeclaration-after-statement -Wold-style-definition \
-Wno-missing-field-initializers -Wno-unused-parameter \
-Wno-attributes -Wno-long-long -Winline"

# invalidate cached value if MAYBE_WARN has changed
if test "x$libdrm_cv_warn_maybe" != "x$MAYBE_WARN"; then
	unset libdrm_cv_warn_cflags
fi
AC_CACHE_CHECK([for supported warning flags], libdrm_cv_warn_cflags, [
	echo
	WARN_CFLAGS=""

	# Some warning options are not supported by all versions of
	# gcc, so test all desired options against the current
	# compiler.
	#
	# Note that there are some order dependencies
	# here. Specifically, an option that disables a warning will
	# have no net effect if a later option then enables that
	# warnings, (perhaps implicitly). So we put some grouped
	# options (-Wall and -Wextra) up front and the -Wno options
	# last.

	for W in $MAYBE_WARN; do
		LIBDRM_CC_TRY_FLAG([$W], [WARN_CFLAGS="$WARN_CFLAGS $W"])
	done

	libdrm_cv_warn_cflags=$WARN_CFLAGS
	libdrm_cv_warn_maybe=$MAYBE_WARN

	AC_MSG_CHECKING([which warning flags were supported])])
WARN_CFLAGS="$libdrm_cv_warn_cflags"

if test "x$UDEV" = xyes; then
	AC_DEFINE(UDEV, 1, [Have UDEV support])
fi

AM_CONDITIONAL(HAVE_NOUVEAU, [test "x$NOUVEAU" = xyes])

PKG_CHECK_MODULES(CAIRO, cairo, [HAVE_CAIRO=yes], [HAVE_CAIRO=no])
if test "x$HAVE_CAIRO" = xyes; then
	AC_DEFINE(HAVE_CAIRO, 1, [Have cairo support])
fi
AM_CONDITIONAL(HAVE_CAIRO, [test "x$HAVE_CAIRO" = xyes])

# For enumerating devices in test case
PKG_CHECK_MODULES(LIBUDEV, libudev, [HAVE_LIBUDEV=yes], [HAVE_LIBUDEV=no])
if test "x$HAVE_LIBUDEV" = xyes; then
   	AC_DEFINE(HAVE_LIBUDEV, 1, [Have libudev support])
fi
AM_CONDITIONAL(HAVE_LIBUDEV, [test "x$HAVE_LIBUDEV" = xyes])


AC_SUBST(WARN_CFLAGS)
AC_OUTPUT([
	Makefile
	libdrm/Makefile
	libdrm/intel/Makefile
	libdrm/nouveau/Makefile
	libdrm/nouveau/libdrm_nouveau.pc
	shared-core/Makefile
	tests/Makefile
	tests/modeprint/Makefile
	tests/modetest/Makefile
	libdrm.pc
	libdrm_intel.pc])
