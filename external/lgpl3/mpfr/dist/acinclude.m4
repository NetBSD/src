dnl  MPFR specific autoconf macros

dnl  Copyright 2000, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
dnl  Contributed by the AriC and Caramel projects, INRIA.
dnl
dnl  This file is part of the GNU MPFR Library.
dnl
dnl  The GNU MPFR Library is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU Lesser General Public License as published
dnl  by the Free Software Foundation; either version 3 of the License, or (at
dnl  your option) any later version.
dnl
dnl  The GNU MPFR Library is distributed in the hope that it will be useful, but
dnl  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
dnl  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
dnl  License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public License
dnl  along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
dnl  http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
dnl  51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

dnl  autoconf 2.60 is necessary because of the use of AC_PROG_SED.
dnl  The following line allows the autoconf wrapper (when installed)
dnl  to work as expected.
dnl  If you change the required version, please update README.dev too!
AC_PREREQ(2.60)

dnl ------------------------------------------------------------
dnl You must put in MPFR_CONFIGS everything which configure MPFR
dnl except:
dnl   -everything dealing with CC and CFLAGS in particular the ABI
dnl   but the IEEE-754 specific flags must be set here.
dnl   -GMP's linkage.
dnl   -Libtool stuff.
dnl   -Handling of special arguments of MPFR's configure.
AC_DEFUN([MPFR_CONFIGS],
[
AC_REQUIRE([AC_OBJEXT])
AC_REQUIRE([MPFR_CHECK_LIBM])
AC_REQUIRE([AC_HEADER_TIME])
AC_REQUIRE([AC_CANONICAL_HOST])

AC_CHECK_HEADER([limits.h],, AC_MSG_ERROR([limits.h not found]))
AC_CHECK_HEADER([float.h],,  AC_MSG_ERROR([float.h not found]))
AC_CHECK_HEADER([string.h],, AC_MSG_ERROR([string.h not found]))

dnl Check for locales
AC_CHECK_HEADERS([locale.h])

dnl Check for wide characters (wchar_t and wint_t)
AC_CHECK_HEADERS([wchar.h])

dnl Check for stdargs
AC_CHECK_HEADER([stdarg.h],[AC_DEFINE([HAVE_STDARG],1,[Define if stdarg])],
  [AC_CHECK_HEADER([varargs.h],,
    AC_MSG_ERROR([stdarg.h or varargs.h not found]))])

dnl sys/fpu.h - MIPS specific
AC_CHECK_HEADERS([sys/time.h sys/fpu.h])

dnl Check how to get `alloca'
AC_FUNC_ALLOCA

dnl SIZE_MAX macro
gl_SIZE_MAX

dnl va_copy macro
AC_MSG_CHECKING([how to copy va_list])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdarg.h>
]], [[
   va_list ap1, ap2;
   va_copy(ap1, ap2);
]])], [
   AC_MSG_RESULT([va_copy])
   AC_DEFINE(HAVE_VA_COPY)
], [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdarg.h>
]], [[
   va_list ap1, ap2;
   __va_copy(ap1, ap2);
]])], [AC_DEFINE([HAVE___VA_COPY]) AC_MSG_RESULT([__va_copy])],
   [AC_MSG_RESULT([memcpy])])])

dnl FIXME: The functions memmove, memset and strtol are really needed by
dnl MPFR, but if they are implemented as macros, this is also OK (in our
dnl case).  So, we do not return an error, but their tests are currently
dnl useless.
dnl gettimeofday is not defined for MinGW
AC_CHECK_FUNCS([memmove memset setlocale strtol gettimeofday])

dnl Check for IEEE-754 switches on Alpha
case $host in
alpha*-*-*)
  saved_CFLAGS="$CFLAGS"
  AC_CACHE_CHECK([for IEEE-754 switches], mpfr_cv_ieee_switches, [
  if test -n "$GCC"; then
    mpfr_cv_ieee_switches="-mfp-rounding-mode=d -mieee-with-inexact"
  else
    mpfr_cv_ieee_switches="-fprm d -ieee_with_inexact"
  fi
  CFLAGS="$CFLAGS $mpfr_cv_ieee_switches"
  AC_TRY_COMPILE(,,, mpfr_cv_ieee_switches="none")
  ])
  if test "$mpfr_cv_ieee_switches" = "none"; then
    CFLAGS="$saved_CFLAGS"
  else
    CFLAGS="$saved_CFLAGS $mpfr_cv_ieee_switches"
  fi
esac

dnl check for long long
AC_CHECK_TYPE([long long int],
   AC_DEFINE(HAVE_LONG_LONG, 1, [Define if compiler supports long long]),,)

dnl intmax_t is C99
AC_CHECK_TYPES([intmax_t])
if test "$ac_cv_type_intmax_t" = yes; then
  AC_CACHE_CHECK([for working INTMAX_MAX], mpfr_cv_have_intmax_max, [
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I$srcdir/src"
    AC_TRY_COMPILE([#include "mpfr-intmax.h"], [intmax_t x = INTMAX_MAX;],
      mpfr_cv_have_intmax_max=yes, mpfr_cv_have_intmax_max=no)
    CPPFLAGS="$saved_CPPFLAGS"
  ])
  if test "$mpfr_cv_have_intmax_max" = "yes"; then
    AC_DEFINE(MPFR_HAVE_INTMAX_MAX,1,[Define if you have a working INTMAX_MAX.])
  fi
fi

AC_CHECK_TYPE( [union fpc_csr],
   AC_DEFINE(HAVE_FPC_CSR,1,[Define if union fpc_csr is available]), ,
[
#if HAVE_SYS_FPU_H
#  include <sys/fpu.h>
#endif
])

dnl Check for fesetround
AC_CACHE_CHECK([for fesetround], mpfr_cv_have_fesetround, [
saved_LIBS="$LIBS"
LIBS="$LIBS $MPFR_LIBM"
AC_TRY_LINK([#include <fenv.h>], [fesetround(FE_TONEAREST);],
  mpfr_cv_have_fesetround=yes, mpfr_cv_have_fesetround=no)
LIBS="$saved_LIBS"
])
if test "$mpfr_cv_have_fesetround" = "yes"; then
  AC_DEFINE(MPFR_HAVE_FESETROUND,1,[Define if you have the `fesetround' function via the <fenv.h> header file.])
fi

dnl Check for gcc float-conversion bug; if need be, -ffloat-store is used to
dnl force the conversion to the destination type when a value is stored to
dnl a variable (see ISO C99 standard 5.1.2.3#13, 6.3.1.5#2, 6.3.1.8#2). This
dnl is important concerning the exponent range. Note that this doesn't solve
dnl the double-rounding problem.
if test -n "$GCC"; then
  AC_CACHE_CHECK([for gcc float-conversion bug], mpfr_cv_gcc_floatconv_bug, [
  saved_LIBS="$LIBS"
  LIBS="$LIBS $MPFR_LIBM"
  AC_TRY_RUN([
#include <float.h>
#ifdef MPFR_HAVE_FESETROUND
#include <fenv.h>
#endif
static double get_max (void);
int main() {
  double x = 0.5;
  double y;
  int i;
  for (i = 1; i <= 11; i++)
    x *= x;
  if (x != 0)
    return 1;
#ifdef MPFR_HAVE_FESETROUND
  /* Useful test for the G4 PowerPC */
  fesetround(FE_TOWARDZERO);
  x = y = get_max ();
  x *= 2.0;
  if (x != y)
    return 1;
#endif
  return 0;
}
static double get_max (void) { static volatile double d = DBL_MAX; return d; }
  ], [mpfr_cv_gcc_floatconv_bug="no"],
     [mpfr_cv_gcc_floatconv_bug="yes, use -ffloat-store"],
     [mpfr_cv_gcc_floatconv_bug="cannot test, use -ffloat-store"])
  LIBS="$saved_LIBS"
  ])
  if test "$mpfr_cv_gcc_floatconv_bug" != "no"; then
    CFLAGS="$CFLAGS -ffloat-store"
  fi
fi

dnl Check if denormalized numbers are supported
AC_CACHE_CHECK([for denormalized numbers], mpfr_cv_have_denorms, [
AC_TRY_RUN([
#include <math.h>
#include <stdio.h>
int main() {
  double x = 2.22507385850720138309e-308;
  fprintf (stderr, "%e\n", x / 2.0);
  return 2.0 * (x / 2.0) != x;
}
], mpfr_cv_have_denorms=yes, mpfr_cv_have_denorms=no, mpfr_cv_have_denorms=no)
])
if test "$mpfr_cv_have_denorms" = "yes"; then
  AC_DEFINE(HAVE_DENORMS,1,[Define if denormalized floats work.])
fi

dnl Check the FP division by 0 fails (e.g. on a non-IEEE-754 platform).
dnl In such a case, MPFR_ERRDIVZERO is defined to disable the tests
dnl involving a FP division by 0.
dnl For the developers: to check whether all these tests are disabled,
dnl configure MPFR with "-DMPFR_TEST_DIVBYZERO=1 -DMPFR_ERRDIVZERO=1".
AC_CACHE_CHECK([if the FP division by 0 fails], mpfr_cv_errdivzero, [
AC_TRY_RUN([
int main() {
  volatile double d = 0.0, x;
  x = 0.0 / d;
  x = 1.0 / d;
  return 0;
}
], [mpfr_cv_errdivzero="no"],
   [mpfr_cv_errdivzero="yes"],
   [mpfr_cv_errdivzero="cannot test, assume no"])
])
if test "$mpfr_cv_errdivzero" = "yes"; then
  AC_DEFINE(MPFR_ERRDIVZERO,1,[Define if the FP division by 0 fails.])
  AC_MSG_WARN([The floating-point division by 0 fails instead of])
  AC_MSG_WARN([returning a special value: NaN or infinity. Tests])
  AC_MSG_WARN([involving a FP division by 0 will be disabled.])
fi

dnl Check whether NAN != NAN (as required by the IEEE-754 standard,
dnl but not by the ISO C standard). For instance, this is false with
dnl MIPSpro 7.3.1.3m under IRIX64. By default, assume this is true.
AC_CACHE_CHECK([if NAN == NAN], mpfr_cv_nanisnan, [
AC_TRY_RUN([
#include <stdio.h>
#include <math.h>
#ifndef NAN
# define NAN (0.0/0.0)
#endif
int main() {
  double d;
  d = NAN;
  return d != d;
}
], [mpfr_cv_nanisnan="yes"],
   [mpfr_cv_nanisnan="no"],
   [mpfr_cv_nanisnan="cannot test, assume no"])
])
if test "$mpfr_cv_nanisnan" = "yes"; then
  AC_DEFINE(MPFR_NANISNAN,1,[Define if NAN == NAN.])
  AC_MSG_WARN([The test NAN != NAN is false. The probable reason is that])
  AC_MSG_WARN([your compiler optimizes floating-point expressions in an])
  AC_MSG_WARN([unsafe way because some option, such as -ffast-math or])
  AC_MSG_WARN([-fast (depending on the compiler), has been used.  You])
  AC_MSG_WARN([should NOT use such an option, otherwise MPFR functions])
  AC_MSG_WARN([such as mpfr_get_d and mpfr_set_d may return incorrect])
  AC_MSG_WARN([results on special FP numbers (e.g. NaN or signed zeros).])
  AC_MSG_WARN([If you did not use such an option, please send us a bug])
  AC_MSG_WARN([report so that we can try to find a workaround for your])
  AC_MSG_WARN([platform and/or document the behavior.])
fi

dnl Check if the chars '0' to '9' are consecutive values
AC_MSG_CHECKING([if charset has consecutive values])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
char *number = "0123456789";
char *lower  = "abcdefghijklmnopqrstuvwxyz";
char *upper  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
]],[[
 int i;
 unsigned char *p;
 for (p = (unsigned char*) number, i = 0; i < 9; i++)
   if ( (*p)+1 != *(p+1) ) return 1;
 for (p = (unsigned char*) lower, i = 0; i < 25; i++)
   if ( (*p)+1 != *(p+1) ) return 1;
 for (p = (unsigned char*) upper, i = 0; i < 25; i++)
   if ( (*p)+1 != *(p+1) ) return 1;
]])], [AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_DEFINE(MPFR_NO_CONSECUTIVE_CHARSET,1,[Charset is not consecutive])
], [AC_MSG_RESULT(cannot test)])

dnl Must be checked with the LIBM
dnl but we don't want to add the LIBM to MPFR dependency.
dnl Can't use AC_CHECK_FUNCS since the function may be in LIBM but
dnl not exported in math.h
saved_LIBS="$LIBS"
LIBS="$LIBS $MPFR_LIBM"
dnl AC_CHECK_FUNCS([round trunc floor ceil nearbyint])
AC_MSG_CHECKING(for math/round)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
int f (double (*func)(double)) { return 0;}
]], [[
 double a = 17.42;
 a = f (round);
 return 0;
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_ROUND, 1,[Have ISO-C99 round function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/trunc)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
int f (double (*func)(double)) { return 0;}
]], [[
 double a = 17.42;
 a = f(trunc);
 return 0;
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_TRUNC, 1,[Have ISO-C99 trunc function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/floor)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
int f (double (*func)(double)) { return 0;}
]], [[
 double a = 17.42;
 a = f(floor);
 return 0;
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_FLOOR, 1,[Have ISO-C99 floor function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/ceil)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
int f (double (*func)(double)) { return 0;}
]], [[
 double a = 17.42;
 a = f(ceil);
 return 0;
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_CEIL, 1,[Have ISO-C99 ceil function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/rint)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
int f (double (*func)(double)) { return 0;}
]], [[
 double a = 17.42;
 a = f(nearbyint);
 return 0;
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_NEARBYINT, 1,[Have ISO-C99 rint function])
],[AC_MSG_RESULT(no)])

LIBS="$saved_LIBS"

dnl Now try to check the long double format
MPFR_C_LONG_DOUBLE_FORMAT

if test "$enable_logging" = yes; then
  if test "$enable_thread_safe" = yes; then
    AC_MSG_ERROR([Enable either `Logging' or `thread-safe', not both])
  else
    enable_thread_safe=no
  fi
fi

dnl Check if thread-local variables are supported.
dnl At least two problems can occur in practice:
dnl 1. The compilation fails, e.g. because the compiler doesn't know
dnl    about the __thread keyword.
dnl 2. The compilation succeeds, but the system doesn't support TLS or
dnl    there is some ld configuration problem. One of the effects can
dnl    be that thread-local variables always evaluate to 0. So, it is
dnl    important to run the test below.
if test "$enable_thread_safe" != no; then
AC_MSG_CHECKING(for TLS support)
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$srcdir/src"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define MPFR_USE_THREAD_SAFE 1
#include "mpfr-thread.h"
MPFR_THREAD_ATTR int x = 17;
int main() {
  return x != 17;
}
  ]])],
     [AC_MSG_RESULT(yes)
      AC_DEFINE([MPFR_USE_THREAD_SAFE],1,[Build MPFR as thread safe])
     ],
     [AC_MSG_RESULT(no)
      if test "$enable_thread_safe" = yes; then
        AC_MSG_ERROR([please configure with --disable-thread-safe])
      fi
     ],
     [if test "$enable_thread_safe" = yes; then
        AC_MSG_RESULT([cannot test, assume yes])
        AC_DEFINE([MPFR_USE_THREAD_SAFE],1,[Build MPFR as thread safe])
      else
        AC_MSG_RESULT([cannot test, assume no])
      fi
     ])
CPPFLAGS="$saved_CPPFLAGS"
fi
])
dnl end of MPFR_CONFIGS


dnl  MPFR_C_LONG_DOUBLE_FORMAT
dnl  -------------------------
dnl  Determine the format of a long double.
dnl
dnl  The object file is grepped, so as to work when cross compiling.  A
dnl  start and end sequence is included to avoid false matches, and
dnl  allowance is made for the desired data crossing an "od -b" line
dnl  boundary.  The test number is a small integer so it should appear
dnl  exactly, no rounding or truncation etc.
dnl
dnl  "od -b" is supported even by Unix V7, and the awk script used doesn't
dnl  have functions or anything, so even an "old" awk should suffice.
dnl
dnl  The 10-byte IEEE extended format is generally padded to either 12 or 16
dnl  bytes for alignment purposes.  The SVR4 i386 ABI is 12 bytes, or i386
dnl  gcc -m128bit-long-double selects 16 bytes.  IA-64 is 16 bytes in LP64
dnl  mode, or 12 bytes in ILP32 mode.  The first 10 bytes is the relevant
dnl  part in all cases (big and little endian).
dnl
dnl  Enhancements:
dnl
dnl  Could match more formats, but no need to worry until there's code
dnl  wanting to use them.
dnl
dnl  Don't want to duplicate the double matching from GMP_C_DOUBLE_FORMAT,
dnl  perhaps we should merge with that macro, to match data formats
dnl  irrespective of the C type in question.  Or perhaps just let the code
dnl  use DOUBLE macros when sizeof(double)==sizeof(long double).

AC_DEFUN([MPFR_C_LONG_DOUBLE_FORMAT],
[AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_AWK])
AC_REQUIRE([AC_OBJEXT])
AC_CHECK_TYPES([long double])
AC_CACHE_CHECK([format of `long double' floating point],
                mpfr_cv_c_long_double_format,
[mpfr_cv_c_long_double_format=unknown
if test "$ac_cv_type_long_double" != yes; then
  mpfr_cv_c_long_double_format="not available"
else
  cat >conftest.c <<\EOF
[
/* "before" is 16 bytes to ensure there's no padding between it and "x".
   We're not expecting any "long double" bigger than 16 bytes or with
   alignment requirements stricter than 16 bytes.  */
struct {
  char         before[16];
  long double  x;
  char         after[8];
} foo = {
  { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\001', '\043', '\105', '\147', '\211', '\253', '\315', '\357' },
  -123456789.0,
  { '\376', '\334', '\272', '\230', '\166', '\124', '\062', '\020' }
};
]
EOF
  mpfr_compile="$CC $CFLAGS $CPPFLAGS -c conftest.c >&AC_FD_CC 2>&1"
  if AC_TRY_EVAL(mpfr_compile); then
    cat >conftest.awk <<\EOF
[
BEGIN {
  found = 0
}

# got[] holds a sliding window of bytes read the input.  got[0] is the most
# recent byte read, and got[31] the oldest byte read, so when looking to
# match some data the indices are "reversed".
#
{
  for (f = 2; f <= NF; f++)
    {
      # new byte, shift others up
      for (i = 31; i >= 0; i--)
        got[i+1] = got[i];
      got[0] = $f;

      # end sequence
      if (got[7] != "376") continue
      if (got[6] != "334") continue
      if (got[5] != "272") continue
      if (got[4] != "230") continue
      if (got[3] != "166") continue
      if (got[2] != "124") continue
      if (got[1] != "062") continue
      if (got[0] != "020") continue

      # start sequence, with 8-byte body
      if (got[23] == "001" && \
          got[22] == "043" && \
          got[21] == "105" && \
          got[20] == "147" && \
          got[19] == "211" && \
          got[18] == "253" && \
          got[17] == "315" && \
          got[16] == "357")
        {
          saw = " (" got[15] \
                 " " got[14] \
                 " " got[13] \
                 " " got[12] \
                 " " got[11] \
                 " " got[10] \
                 " " got[9]  \
                 " " got[8] ")"

          if (got[15] == "301" && \
              got[14] == "235" && \
              got[13] == "157" && \
              got[12] == "064" && \
              got[11] == "124" && \
              got[10] == "000" && \
              got[9] ==  "000" && \
              got[8] ==  "000")
            {
              print "IEEE double, big endian"
              found = 1
              exit
            }

          if (got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "124" && \
              got[11] == "064" && \
              got[10] == "157" && \
              got[9] ==  "235" && \
              got[8] ==  "301")
            {
              print "IEEE double, little endian"
              found = 1
              exit
            }
        }

      # start sequence, with 12-byte body
      if (got[27] == "001" && \
          got[26] == "043" && \
          got[25] == "105" && \
          got[24] == "147" && \
          got[23] == "211" && \
          got[22] == "253" && \
          got[21] == "315" && \
          got[20] == "357")
        {
          saw = " (" got[19] \
                 " " got[18] \
                 " " got[17] \
                 " " got[16] \
                 " " got[15] \
                 " " got[14] \
                 " " got[13] \
                 " " got[12] \
                 " " got[11] \
                 " " got[10] \
                 " " got[9]  \
                 " " got[8] ")"

          if (got[19] == "000" && \
              got[18] == "000" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "240" && \
              got[14] == "242" && \
              got[13] == "171" && \
              got[12] == "353" && \
              got[11] == "031" && \
              got[10] == "300")
            {
              print "IEEE extended, little endian"
              found = 1
              exit
            }

          if (got[19] == "300" && \
              got[18] == "031" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "353" && \
              got[14] == "171" && \
              got[13] == "242" && \
              got[12] == "240" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[09] == "000" && \
              got[08] == "000")
            {
              # format found on m68k
              print "IEEE extended, big endian"
              found = 1
              exit
            }
        }

      # start sequence, with 16-byte body
      if (got[31] == "001" && \
          got[30] == "043" && \
          got[29] == "105" && \
          got[28] == "147" && \
          got[27] == "211" && \
          got[26] == "253" && \
          got[25] == "315" && \
          got[24] == "357")
        {
          saw = " (" got[23] \
                 " " got[22] \
                 " " got[21] \
                 " " got[20] \
                 " " got[19] \
                 " " got[18] \
                 " " got[17] \
                 " " got[16] \
                 " " got[15] \
                 " " got[14] \
                 " " got[13] \
                 " " got[12] \
                 " " got[11] \
                 " " got[10] \
                 " " got[9]  \
                 " " got[8] ")"

          if (got[23] == "000" && \
              got[22] == "000" && \
              got[21] == "000" && \
              got[20] == "000" && \
              got[19] == "240" && \
              got[18] == "242" && \
              got[17] == "171" && \
              got[16] == "353" && \
              got[15] == "031" && \
              got[14] == "300")
            {
              print "IEEE extended, little endian"
              found = 1
              exit
            }

          if (got[23] == "300" && \
              got[22] == "031" && \
              got[21] == "326" && \
              got[20] == "363" && \
              got[19] == "105" && \
              got[18] == "100" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "000" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[9]  == "000" && \
              got[8]  == "000")
            {
              # format used on HP 9000/785 under HP-UX
              print "IEEE quad, big endian"
              found = 1
              exit
            }

          if (got[23] == "000" && \
              got[22] == "000" && \
              got[21] == "000" && \
              got[20] == "000" && \
              got[19] == "000" && \
              got[18] == "000" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "100" && \
              got[12] == "105" && \
              got[11] == "363" && \
              got[10] == "326" && \
              got[9]  == "031" && \
	      got[8]  == "300")
            {
              print "IEEE quad, little endian"
              found = 1
              exit
            }

          if (got[23] == "301" && \
              got[22] == "235" && \
              got[21] == "157" && \
              got[20] == "064" && \
              got[19] == "124" && \
              got[18] == "000" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "000" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[9]  == "000" && \
              got[8]  == "000")
            {
              # format used on 32-bit PowerPC (Mac OS X and Debian GNU/Linux)
              print "possibly double-double, big endian"
              found = 1
              exit
            }
        }
    }
}

END {
  if (! found)
    print "unknown", saw
}
]
EOF
    mpfr_cv_c_long_double_format=`od -b conftest.$OBJEXT | $AWK -f conftest.awk`
    case $mpfr_cv_c_long_double_format in
    unknown*)
      echo "cannot match anything, conftest.$OBJEXT contains" >&AC_FD_CC
      od -b conftest.$OBJEXT >&AC_FD_CC
      ;;
    esac
  else
    AC_MSG_WARN([oops, cannot compile test program])
  fi
fi
rm -f conftest*
])

AH_VERBATIM([HAVE_LDOUBLE],
[/* Define one of the following to 1 for the format of a `long double'.
   If your format is not among these choices, or you don't know what it is,
   then leave all undefined.
   IEEE_EXT is the 10-byte IEEE extended precision format.
   IEEE_QUAD is the 16-byte IEEE quadruple precision format.
   LITTLE or BIG is the endianness.  */
#undef HAVE_LDOUBLE_IEEE_EXT_LITTLE
#undef HAVE_LDOUBLE_IEEE_QUAD_BIG])

case $mpfr_cv_c_long_double_format in
  "IEEE extended, little endian")
    AC_DEFINE(HAVE_LDOUBLE_IEEE_EXT_LITTLE, 1)
    ;;
  "IEEE quad, big endian")
    AC_DEFINE(HAVE_LDOUBLE_IEEE_QUAD_BIG, 1)
    ;;
  "IEEE quad, little endian")
    AC_DEFINE(HAVE_LDOUBLE_IEEE_QUAD_LITTLE, 1)
    ;;
  "possibly double-double, big endian")
    AC_MSG_WARN([This format is known on GCC/PowerPC platforms,])
    AC_MSG_WARN([but due to GCC PR26374, we can't test further.])
    AC_MSG_WARN([You can safely ignore this warning, though.])
    # Since we are not sure, we do not want to define a macro.
    ;;
  unknown* | "not available")
    ;;
  *)
    AC_MSG_WARN([oops, unrecognised float format: $mpfr_cv_c_long_double_format])
    ;;
esac
])


dnl  MPFR_CHECK_LIBM
dnl  ---------------
dnl  Determine a math library -lm to use.

AC_DEFUN([MPFR_CHECK_LIBM],
[AC_REQUIRE([AC_CANONICAL_HOST])
AC_SUBST(MPFR_LIBM,'')
case $host in
  *-*-beos* | *-*-cygwin* | *-*-pw32*)
    # According to libtool AC CHECK LIBM, these systems don't have libm
    ;;
  *-*-solaris*)
    # On Solaris the math functions new in C99 are in -lm9x.
    # FIXME: Do we need -lm9x as well as -lm, or just instead of?
    AC_CHECK_LIB(m9x, main, MPFR_LIBM="-lm9x")
    AC_CHECK_LIB(m,   main, MPFR_LIBM="$MPFR_LIBM -lm")
    ;;
  *-ncr-sysv4.3*)
    # FIXME: What does -lmw mean?  Libtool AC CHECK LIBM does it this way.
    AC_CHECK_LIB(mw, _mwvalidcheckl, MPFR_LIBM="-lmw")
    AC_CHECK_LIB(m, main, MPFR_LIBM="$MPFR_LIBM -lm")
    ;;
  *)
    AC_CHECK_LIB(m, main, MPFR_LIBM="-lm")
    ;;
esac
])


dnl  MPFR_LD_SEARCH_PATHS_FIRST
dnl  --------------------------

AC_DEFUN([MPFR_LD_SEARCH_PATHS_FIRST],
[case "$LD $LDFLAGS" in
  *-Wl,-search_paths_first*) ;;
  *) AC_MSG_CHECKING([if the compiler understands -Wl,-search_paths_first])
     saved_LDFLAGS="$LDFLAGS"
     LDFLAGS="-Wl,-search_paths_first $LDFLAGS"
     AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
       [AC_MSG_RESULT(yes)],
       [AC_MSG_RESULT(no)]
        LDFLAGS="$saved_LDFLAGS")
     ;;
 esac
])


dnl  GMP_C_ATTRIBUTE_MODE
dnl  --------------------
dnl  Introduced in gcc 2.2, but perhaps not in all Apple derived versions.
dnl  Needed for mpfr-longlong.h; this is currently necessary for s390.
dnl
dnl  TODO: Replace this with a cleaner type size detection, as this
dnl  solution only works with gcc and assumes CHAR_BIT == 8. Probably use
dnl  <stdint.h>, and <http://gcc.gnu.org/viewcvs/trunk/config/stdint.m4>
dnl  as a fallback.

AC_DEFUN([GMP_C_ATTRIBUTE_MODE],
[AC_CACHE_CHECK([whether gcc __attribute__ ((mode (XX))) works],
               gmp_cv_c_attribute_mode,
[AC_TRY_COMPILE([typedef int SItype __attribute__ ((mode (SI)));], ,
 gmp_cv_c_attribute_mode=yes, gmp_cv_c_attribute_mode=no)
])
if test $gmp_cv_c_attribute_mode = yes; then
 AC_DEFINE(HAVE_ATTRIBUTE_MODE, 1,
 [Define to 1 if the compiler accepts gcc style __attribute__ ((mode (XX)))])
fi
])


dnl  MPFR_FUNC_GMP_PRINTF_SPEC
dnl  ------------------------------------
dnl  MPFR_FUNC_GMP_PRINTF_SPEC(spec, type, [includes], [if-true], [if-false])
dnl  Check if gmp_sprintf supports the conversion specification 'spec'
dnl  with type 'type'.
dnl  Expand 'if-true' if printf supports 'spec', 'if-false' otherwise.

AC_DEFUN([MPFR_FUNC_GMP_PRINTF_SPEC],[
AC_MSG_CHECKING(if gmp_printf supports "%$1")
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
$3
#include <gmp.h>
]], [[
  char s[256];
  $2 a = 17;

  if (gmp_sprintf (s, "(%0.0$1)(%d)", a, 42) == -1) return 1;
  return (strcmp (s, "(17)(42)") != 0);
]])],
  [AC_MSG_RESULT(yes)
  $4],
  [AC_MSG_RESULT(no)
  $5])
])


dnl MPFR_CHECK_PRINTF_SPEC
dnl ----------------------
dnl Check if gmp_printf supports some optional length modifiers.
dnl Defined symbols are negative to shorten the gcc command line.

AC_DEFUN([MPFR_CHECK_PRINTF_SPEC], [
AC_REQUIRE([MPFR_CONFIGS])dnl
if test "$ac_cv_type_intmax_t" = yes; then
 MPFR_FUNC_GMP_PRINTF_SPEC([jd], [intmax_t], [
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
         ],,
         [AC_DEFINE([NPRINTF_J], 1, [gmp_printf cannot read intmax_t])])
fi

MPFR_FUNC_GMP_PRINTF_SPEC([hhd], [char], [
#include <gmp.h>
         ],,
         [AC_DEFINE([NPRINTF_HH], 1, [gmp_printf cannot use `hh' length modifier])])

MPFR_FUNC_GMP_PRINTF_SPEC([lld], [long long int], [
#include <gmp.h>
         ],,
         [AC_DEFINE([NPRINTF_LL], 1, [gmp_printf cannot read long long int])])

MPFR_FUNC_GMP_PRINTF_SPEC([Lf], [long double], [
#include <gmp.h>
         ],,
         [AC_DEFINE([NPRINTF_L], 1, [gmp_printf cannot read long double])])

MPFR_FUNC_GMP_PRINTF_SPEC([td], [ptrdiff_t], [
#if defined (__cplusplus)
#include <cstddef>
#else
#include <stddef.h>
#endif
#include "gmp.h"
    ],,
    [AC_DEFINE([NPRINTF_T], 1, [gmp_printf cannot read ptrdiff_t])])
])
