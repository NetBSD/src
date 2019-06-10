dnl Process this file with autoconf to produce a configure script.

dnl WARNING! C code starting with # (preprocessor directives) must not
dnl be indented!

AC_COPYRIGHT([
Copyright 1999-2018 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 3 of the License, or (at
your option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
])

dnl Add check-news when it checks for more than 15 lines
AC_INIT([MPFR],[4.0.1])

dnl Older Automake versions than 1.13 may still be supported, but no longer
dnl tested, and many things have changed in 1.13. Moreover the INSTALL file
dnl and MPFR manual assume that MPFR has been built using Automake 1.13+
dnl (due to parallel tests, introduced by default in Automake 1.13).
AM_INIT_AUTOMAKE([1.13 no-define dist-bzip2 dist-xz dist-zip])
AM_MAINTAINER_MODE(enable)

AC_CONFIG_MACRO_DIR([m4])

dnl FIXME: Several AC_RUN_IFELSE programs can return several values
dnl (one for each format to be detected), but the Autoconf manual says:
dnl "This exit status might be that of a failed compilation, or it might
dnl be that of a failed program execution." Unfortunately, we cannot
dnl know whether a non-zero exit status comes from a failed compilation,
dnl so that the detection may be incorrect. To avoid this problem, write
dnl a test program for each possible format? Alternatively, the test
dnl program could output the result of the format detection to a file
dnl and return with the 0 exit status if it could do that successfully.

test_CFLAGS=${CFLAGS+set}

dnl Check if user request its CC and CFLAGS
if test -n "$CFLAGS" || test -n "$CC" ; then
 user_redefine_cc=yes
fi

dnl Basic Autoconf macros. At this point, they must not make Autoconf
dnl choose a compiler because of the CC and CFLAGS setup from gmp.h!

AC_CANONICAL_HOST

AC_PROG_EGREP
AC_PROG_SED

dnl To use a separate config header.
dnl There is still some problem with GMP's HAVE_CONFIG
dnl AC_CONFIG_HEADERS([mpfrconf.h:mpfrconf.in])

dnl Extra arguments to configure

unset gmp_lib_path GMP_CFLAGS GMP_CC

AC_ARG_WITH(gmp_include,
   [  --with-gmp-include=DIR  GMP include directory ],
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   CPPFLAGS="$CPPFLAGS -I$withval")

AC_ARG_WITH(gmp_lib,
   [  --with-gmp-lib=DIR      GMP lib directory ], [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   LDFLAGS="$LDFLAGS -L$withval"
   gmp_lib_path="$withval"
  ])

AC_ARG_WITH(gmp,
   [  --with-gmp=DIR          GMP install directory ], [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   if test -z "$with_gmp_lib" && test -z "$with_gmp_include" ; then
      CPPFLAGS="$CPPFLAGS -I$withval/include"
      LDFLAGS="$LDFLAGS -L$withval/lib"
      gmp_lib_path="$withval/lib"
   else
      AC_MSG_FAILURE([Do not use --with-gmp and --with-gmp-include/--with-gmp-lib options simultaneously.])
   fi
  ])

AC_ARG_WITH(gmp_build,
   [  --with-gmp-build=DIR    GMP build directory (please read INSTALL file)],
   [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   if test -z "$gmp_lib_path" && test -z "$with_gmp_include" ; then
      CPPFLAGS="$CPPFLAGS -I$withval -I$withval/tune"
      LDFLAGS="$LDFLAGS -L$withval -L$withval/.libs -L$withval/tune"
      gmp_lib_path="$withval$PATH_SEPARATOR$withval/.libs$PATH_SEPARATOR$withval/tune"
      if test -r $withval/Makefile ; then
         GMP_CFLAGS=`$SED -n 's/^CFLAGS = //p' $withval/Makefile`
         GMP_CC=`$SED -n 's/^CC = //p' $withval/Makefile`
         GMP_SOURCE=`$SED -n 's/^srcdir = *//p' $withval/Makefile`
         case "$GMP_SOURCE" in
           .)  GMP_SOURCE="" ;;
           /*) ;;
           ?*) GMP_SOURCE="$withval/$GMP_SOURCE" ;;
         esac
         if test -d "$GMP_SOURCE" ; then
            CPPFLAGS="$CPPFLAGS -I$GMP_SOURCE -I$GMP_SOURCE/tune"
         fi
      fi
      use_gmp_build=yes
   else
      AC_MSG_FAILURE([Do not use --with-gmp-build and other --with-gmp options simultaneously.])
   fi
   ])

AC_ARG_WITH(mulhigh_size,
   [  --with-mulhigh-size=NUM internal threshold table for mulhigh],
   AC_DEFINE_UNQUOTED([MPFR_MULHIGH_SIZE],$withval, [Mulhigh size]))

AC_ARG_ENABLE(gmp-internals,
   [  --enable-gmp-internals  enable use of GMP undocumented functions [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([WANT_GMP_INTERNALS],1,[Want GMP internals]) ;;
      no)  ;;
      *) AC_MSG_ERROR([bad value for --enable-gmp-internals: yes or no]) ;;
     esac])

AC_ARG_ENABLE(assert,
   [  --enable-assert         enable ASSERT checking [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_WANT_ASSERT],1,[Want all simple assertions]) ;;
      none) AC_DEFINE([MPFR_WANT_ASSERT],-1,[Do no want any assertion]) ;;
      no)  ;;
      full) AC_DEFINE([MPFR_WANT_ASSERT],2,[Want full assertions]) ;;
      *) AC_MSG_ERROR([bad value for --enable-assert: yes, no, none or full]) ;;
     esac])

AC_ARG_ENABLE(logging,
   [  --enable-logging        enable MPFR logging (the system must support it)
                          [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_USE_LOGGING],1,[Enable MPFR logging support]) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-logging: yes or no]) ;;
     esac])

AC_ARG_ENABLE(thread-safe,
   [  --disable-thread-safe   explicitly disable TLS support
  --enable-thread-safe    build MPFR as thread safe, i.e. with TLS support
                          (the system must support it) [[default=autodetect]]],
   [ case $enableval in
      yes) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-thread-safe: yes or no]) ;;
     esac])

AC_ARG_ENABLE(shared-cache,
   [  --enable-shared-cache   enable use of caches shared by all threads,
                          for all MPFR constants.  It usually makes MPFR
                          dependent on PTHREAD [[default=no]]],
   [ case $enableval in
      yes) mpfr_want_shared_cache=yes
         AC_DEFINE([WANT_SHARED_CACHE],1,[Want shared cache]) ;;
      no)  ;;
      *) AC_MSG_ERROR([bad value for --enable-shared-cache: yes or no]) ;;
     esac])

AC_ARG_ENABLE(warnings,
   [  --enable-warnings       allow MPFR to output warnings to stderr [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_USE_WARNINGS],1,[Allow MPFR to output warnings to stderr]) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-warnings: yes or no]) ;;
     esac])

AC_ARG_ENABLE(tests-timeout,
   [  --enable-tests-timeout=NUM
                          [[for developers]] enable timeout for test programs
                          (NUM seconds, <= 9999) [[default=no]]; if this is
                          enabled, the environment variable $MPFR_TESTS_TIMEOUT
                          overrides NUM (0: no timeout)],
   [ case $enableval in
      no)   ;;
      yes)  AC_DEFINE([MPFR_TESTS_TIMEOUT], 0, [timeout limit]) ;;
      [[0-9]]|[[0-9]][[0-9]]|[[0-9]][[0-9]][[0-9]]|[[0-9]][[0-9]][[0-9]][[0-9]])
       AC_DEFINE_UNQUOTED([MPFR_TESTS_TIMEOUT], $enableval, [timeout limit]) ;;
      *)    AC_MSG_ERROR([bad value for --enable-tests-timeout]) ;;
     esac])

AC_ARG_ENABLE(tune-for-coverage,
   [  --enable-tune-for-coverage
                          [[for developers]] tune MPFR for coverage tests],
   [ case $enableval in
      no)   ;;
      yes)  AC_DEFINE([MPFR_TUNE_COVERAGE], 1, [tune for coverage]) ;;
      *)    AC_MSG_ERROR([bad value for --enable-tune-for-coverage]) ;;
     esac])


dnl
dnl Setup CC and CFLAGS
dnl

dnl ********************************************************************
dnl Check for CC and CFLAGS in gmp.h

dnl Warning! The following tests must be done before Autoconf selects
dnl a compiler. This means that some macros such as AC_PROG_CC and
dnl AM_PROG_AR must be put after the following code.

if test -z "$user_redefine_cc" && test "$cross_compiling" != yes ; then

dnl We need to guess the C preprocessor instead of using AC_PROG_CPP,
dnl since AC_PROG_CPP implies AC_PROG_CC, which chooses a compiler
dnl (before we have the chance to get it from gmp.h) and does some
dnl checking related to this compiler (such as dependency tracking
dnl options); if the compiler changes due to __GMP_CC in gmp.h, one
dnl would have incorrect settings.
dnl FIXME: Move this in acinclude.m4 ?
if test -z "$GMP_CC$GMP_CFLAGS" ; then
   AC_MSG_CHECKING(for CC and CFLAGS in gmp.h)
   GMP_CC=__GMP_CC
   GMP_CFLAGS=__GMP_CFLAGS
   # /lib/cpp under Solaris doesn't support some environment variables
   # used by GCC, such as C_INCLUDE_PATH. If the user has set up such
   # environment variables, he probably wants to use them. So, let us
   # prefer cpp and gcc to /lib/cpp. However, note that this won't
   # work if GCC has been installed with versioned names only (e.g.
   # with cpp-5 and gcc-5).
   for cpp in "cpp -P" "cpp" "gcc -P -E" "gcc -E" "/lib/cpp -P" "/lib/cpp" "cc -P -E" "cc -E" "c99 -P -E" "c99 -E" "clang -E" "cl -E" "icl -E"
   do
     # Get CC
     echo "#include \"gmp.h\"" > conftest.c
     echo "MPFR_OPTION __GMP_CC" >> conftest.c
     $cpp $CPPFLAGS conftest.c 2> /dev/null > conftest.txt
     test $? -ne 0 && continue
     GMP_CC=`$EGREP MPFR_OPTION conftest.txt | $SED -e 's/MPFR_OPTION //g;s/ *" *//g'`
     # Get CFLAGS
     echo "#include \"gmp.h\"" >  conftest.c
     echo "MPFR_OPTION __GMP_CFLAGS" >> conftest.c
     unset rmpedantic
     [test "$enable_logging" = yes && rmpedantic='s/[ "]-pedantic[ "]/ /g;']
     $cpp $CPPFLAGS conftest.c 2> /dev/null > conftest.txt
     test $? -ne 0 && continue
     GMP_CFLAGS=`$EGREP MPFR_OPTION conftest.txt | $SED -e "$rmpedantic"'s/MPFR_OPTION //g;s/ *" *//g'`
     break
   done
   rm -f conftest*
   if test "x$GMP_CC" = "x__GMP_CC" || test "x$GMP_CFLAGS" = "x__GMP_CFLAGS" ; then
      AC_MSG_RESULT(failed)
      GMP_CFLAGS=
      GMP_CC=
   else
      AC_MSG_RESULT([CC=$GMP_CC CFLAGS=$GMP_CFLAGS])
   fi
fi

dnl But these variables may be invalid, so we must check them first.
dnl Note: we do not use AC_RUN_IFELSE, as it implies AC_PROG_CC.
if test -n "$GMP_CC$GMP_CFLAGS" ; then
   AC_MSG_CHECKING(for CC=$GMP_CC and CFLAGS=$GMP_CFLAGS)
   echo "int main (void) { return 0; }" > conftest.c
   if $GMP_CC $GMP_CFLAGS -o conftest conftest.c 2> /dev/null ; then
     AC_MSG_RESULT(yes)
     CFLAGS=$GMP_CFLAGS
     CC=$GMP_CC
   else
     AC_MSG_RESULT(no)
   fi
   rm -f conftest*
fi

fi

dnl ********************************************************************

AC_PROG_CC
AC_PROG_CPP
AC_LANG(C)

dnl AM_PROG_AR is needed so that ar-lib (wrapper for Microsoft lib.exe)
dnl gets installed by "automake -i" (if absent, "automake --warnings=all"
dnl gives a warning).
AM_PROG_AR

dnl This must done before MPFR_CONFIGS.
LT_INIT(win32-dll)

dnl Try to determine the double format
AC_MSG_CHECKING(format of `double' floating point)
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
]], [[
union ieee_double_extract
{
  double d;
  unsigned char x[8];
} t;
t.d = 2.877939254133025759330166692961938679218292236328125; /* exact */
if (sizeof (double) != 8)
   return 0;
if (sizeof (unsigned char) != 1)
   return 0;
if (t.x[0] == 1 && t.x[1] == 2 && t.x[2] == 3 && t.x[3] == 4 &&
    t.x[4] == 5 && t.x[5] == 6 && t.x[6] == 7 && t.x[7] == 64)
   return 1; /* little endian */
else if (t.x[7] == 1 && t.x[6] == 2 && t.x[5] == 3 && t.x[4] == 4 &&
    t.x[3] == 5 && t.x[2] == 6 && t.x[1] == 7 && t.x[0] == 64)
   return 2; /* big endian */
else
   return 0; /* unknown */
]])],
   [mpfr_ieee_double=$?],
   [mpfr_ieee_double=$?],
   [mpfr_ieee_double=0])
case "$mpfr_ieee_double" in
  1) AC_MSG_RESULT([IEEE little endian])
     AC_DEFINE(HAVE_DOUBLE_IEEE_LITTLE_ENDIAN) ;;
  2) AC_MSG_RESULT([IEEE big endian])
     AC_DEFINE(HAVE_DOUBLE_IEEE_BIG_ENDIAN) ;;
  *) AC_MSG_WARN([format of `double' floating-point not recognized]) ;;
esac

dnl For GCC, _Decimal64 was introduced in GCC 4.3 for some targets
dnl (note that it is not guaranteed to be available because it may
dnl be disabled in the GCC build). See:
dnl   https://gcc.gnu.org/gcc-4.3/changes.html
dnl _Decimal64 is not yet defined in GCC for C++:
dnl   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51364
dnl _Decimal64 support is broken with GCC 4.6.3 and 4.7.2 on powerpc64
dnl with the mode32 ABI, e.g. "-m32 -mpowerpc64 -mtune=970 -O3"; this
dnl is detected by the x != x test below.
AC_ARG_ENABLE(decimal-float,
   [  --disable-decimal-float explicitly disable decimal floats support
  --enable-decimal-float  build conversion functions from/to decimal floats
                          [[default=autodetect]]],
   [ case $enableval in
      yes) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-decimal-float: yes or no]) ;;
     esac])

dnl GCC >= 4.3 provides __float128, but since this is not standard,
dnl the default is "autodetect" and not "yes"
AC_ARG_ENABLE(float128,
   [  --disable-float128      explicitly disable __float128 support
  --enable-float128       build conversion functions from/to __float128
                          [[default=autodetect]]],
   [ case $enableval in
      yes) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-float128: yes or no]) ;;
     esac])

test_libgmp=__gmpz_init

AC_ARG_ENABLE(mini-gmp,
   [  --enable-mini-gmp       build MPFR with mini-gmp (experimental) [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_USE_MINI_GMP],1,[Use mini-gmp])
           # With mini-gmp, mp_limb_t is always unsigned long.
           AC_COMPUTE_INT(nettle_cv_gmp_numb_bits, [(sizeof(unsigned long) * CHAR_BIT)],
              [#include <limits.h>],
              [AC_MSG_FAILURE([cannot find value of GMP_NUMB_BITS])])
           AC_MSG_RESULT([$nettle_cv_gmp_numb_bits bits])
           AC_DEFINE_UNQUOTED([GMP_NUMB_BITS], $nettle_cv_gmp_numb_bits, [number of bits in a limb])
           AC_DEFINE([GMP_NAIL_BITS], 0, [number of nails in a limb])
           test_libgmp=mpz_init;;
      no)  ;;
      *) AC_MSG_ERROR([bad value for --enable-mini-gmp: yes or no]) ;;
     esac])

AC_ARG_ENABLE(debug-prediction,
   [  --enable-debug-prediction
                          [[for developers]] enable debug of branch prediction
                          (for x86 and x86-64 with GCC, static libs)],
   [ case $enableval in
      yes)  if test "$enable_shared" != no; then
              AC_MSG_ERROR([--enable-debug-prediction can only work in static mode (--disable-shared)])
            fi
            AC_DEFINE([MPFR_DEBUG_PREDICTION],1,
	       [Enable debug of branch prediction]) ;;
      no)   ;;
      *)    AC_MSG_ERROR([bad value for --enable-debug-prediction: yes or no]) ;;
     esac])

AC_ARG_ENABLE(lto,
   [  --enable-lto            build MPFR with link-time-optimization
                          (experimental) [[default: no]]],
   [ case $enableval in
      yes)  if test "$enable_shared" != "no"; then
              AC_MSG_ERROR([--enable-lto can only work in static mode (--disable-shared)])
            fi
            enable_lto=yes
            ;;
      no)   ;;
      *)    AC_MSG_ERROR([bad value for --enable-lto: yes or no]) ;;
     esac])

dnl Check if compiler is ICC, and if such a case, disable GCC
dnl And add some specific flags.
dnl Don't add Warnings Flags (Otherwise you'll get more than 20000 warnings).
dnl Add -long_double flags? Don't use -pc64 !
dnl Notes (VL):
dnl   * With icc 10.1 20080212 on itanium, the __ICC macro is not defined,
dnl     even when the -icc option is used (contrary to what is documented
dnl     on the icc man page).
dnl   * When ICC is correctly detected (__ICC macro defined), unsetting
dnl     the GCC variable confuses libtool. See:
dnl       https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=485421
dnl   * If need be, the gcc predefined macros __GNUC_* can be disabled
dnl     thanks to the -no-gcc option.
dnl   * Now use -mieee-fp instead of -mp (ICC 13 says: option '-mp' is
dnl     deprecated and will be removed in a future release.). According
dnl     to "icc -help", both options are equivalent; it also suggests
dnl     to use -fp-model <arg> instead of -mp, but what is <arg>? Anyway
dnl     -mieee-fp solves the tset_d failure due to a negative zero. This
dnl     option has been introduced in ICC 9.0 for Linux (2005-06):
dnl     http://www.ncsa.illinois.edu/UserInfo/Resources/Software/Intel/Compilers/9.0/C_ReleaseNotes.htm
AC_MSG_CHECKING(for ICC)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !defined(__ICC)
# error "Not ICC"
error
#endif
]], [[]])],[
 AC_MSG_RESULT(yes)
 CFLAGS="-fp_port -mieee-fp -wd1572 -wd265 -wd186 -wd239 $CFLAGS"
],[AC_MSG_RESULT(no)])

dnl If compiler is gcc, then use some specific flags.
dnl But don't touch user other flags.
if test "$test_CFLAGS" != set && test -n "$GCC"; then
  CFLAGS="-Wpointer-arith $CFLAGS"
  AC_MSG_CHECKING(whether the selected language is C++)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined (__cplusplus)
# error "C++"
error
#endif
  ]], [[]])],[
    AC_MSG_RESULT(no)
    CFLAGS="-Wmissing-prototypes $CFLAGS"
  ],[
    AC_MSG_RESULT(yes)
    CFLAGS="-Wmissing-declarations -Wno-sign-compare $CFLAGS"
  ])
  CFLAGS="-Wall $CFLAGS"
fi

AM_PROG_CC_C_O

case $host in
  *-apple-darwin*)
dnl This allows to take the first GMP library in the library paths,
dnl whether it is dynamic or static. This behavior is more sensible,
dnl in particular because it is the only way to link with a version
dnl only available in static form when another version is available
dnl in dynamic, and also for consistency, because the compiler will
dnl take the first gmp.h found in the include paths (so, we need to
dnl take a library that corresponds to this header file). This is a
dnl common problem with darwin.
    MPFR_LD_SEARCH_PATHS_FIRST ;;
esac

AC_C_CONST
AC_C_VOLATILE
dnl Determine the endianness of integer. But since MPFR doesn't use
dnl AC_CONFIG_HEADERS, configure cannot provide such information for
dnl universal binaries containing variants with different endianness
dnl (i.e. generic code must be used).
AC_C_BIGENDIAN([AC_DEFINE(HAVE_BIG_ENDIAN)],[AC_DEFINE(HAVE_LITTLE_ENDIAN)],
[true],[true])

MPFR_CONFIGS

# (Based on GMP 5.1)
# clock_gettime is in librt on *-*-osf5.1 and on glibc < 2.17, so add -lrt to
# TUNE_LIBS if needed (e.g. if clock_gettime is not already in the C library).
# On linux (tested on x86_32, 2.6.26), clock_getres reports ns accuracy,
# while in a quick test on osf, clock_getres said only 1 millisecond.
old_LIBS="$LIBS"
AC_SEARCH_LIBS(clock_gettime, rt, [
  AC_DEFINE([HAVE_CLOCK_GETTIME],1,[Define to 1 if you have the `clock_gettime' function])])
TUNE_LIBS="$LIBS"
LIBS="$old_LIBS"
AC_SUBST(TUNE_LIBS)

dnl
dnl Setup GMP detection
dnl

dnl Check GMP Header
AC_MSG_CHECKING(for gmp.h)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include "gmp.h"
]])],[AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_MSG_ERROR([gmp.h can't be found, or is unusable.])
])

dnl Configs for Windows DLLs.
dnl libtool requires "-no-undefined" for win32 dll
dnl
dnl "-Wl,output-def" is used to get a .def file for use by MS lib to make
dnl a .lib import library, described in the manual.
dnl
dnl Incidentally, libtool does generate an import library libmpfr.dll.a,
dnl but it's "ar" format and cannot be used by the MS linker.  There
dnl doesn't seem to be any GNU tool for generating or converting to .lib.
dnl
dnl The DLL version (the number in libmpfr-*.dll.def below) must be the
dnl version of the oldest supported interface (i.e. CURRENT - AGE in
dnl src/Makefile.am).
AC_SUBST(MPFR_LDFLAGS)
AC_SUBST(LIBMPFR_LDFLAGS)
case $host in
  *-*-cygwin* | *-*-mingw* | *-*-pw32* | *-*-os2*)
   AC_MSG_CHECKING(for DLL/static GMP)
   if test "$enable_shared" = yes; then
     MPFR_LDFLAGS="$MPFR_LDFLAGS -no-undefined"
     LIBMPFR_LDFLAGS="$LIBMPFR_LDFLAGS -Wl,--output-def,.libs/libmpfr-6.dll.def"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
#if !__GMP_LIBGMP_DLL
# error "Dead man"
error
#endif
     ]], [[]])],[AC_MSG_RESULT(DLL)],[
  AC_MSG_RESULT(static)
  AC_MSG_ERROR([libgmp isn't provided as a DLL: use --enable-static --disable-shared]) ])
   else
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
#if __GMP_LIBGMP_DLL
# error "Dead man"
error
#endif
     ]], [[]])],[AC_MSG_RESULT(static)],[
  AC_MSG_RESULT(DLL)
  AC_MSG_ERROR([libgmp is provided as a DLL: use --disable-static --enable-shared]) ])
  fi
  ;;
esac

dnl Under Linux, make sure that the old dtags are used if LD_LIBRARY_PATH
dnl is defined. The issue is that with the new dtags, LD_LIBRARY_PATH has
dnl the precedence over the run path, so that if a compatible MPFR library
dnl is installed in some directory from $LD_LIBRARY_PATH, then the tested
dnl MPFR library will be this library instead of the MPFR library from the
dnl build tree. Other OS with the same issue might be added later.
dnl
dnl References:
dnl   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=859732
dnl   http://lists.gnu.org/archive/html/libtool/2017-05/msg00000.html
dnl
dnl We need to check whether --disable-new-dtags is supported as alternate
dnl linkers may be used (e.g., with tcc: CC=tcc LD=tcc).
dnl
case $host in
  *-*-linux*)
    if test -n "$LD_LIBRARY_PATH"; then
      saved_LDFLAGS="$LDFLAGS"
      LDFLAGS="$LDFLAGS -Wl,--disable-new-dtags"
      AC_MSG_CHECKING(whether --disable-new-dtags is supported by the linker)
      AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main (void) { return 0; }
      ]])],
      [AC_MSG_RESULT(yes (use it since LD_LIBRARY_PATH is set))],
      [AC_MSG_RESULT(no)
       LDFLAGS="$saved_LDFLAGS"
      ])
    fi
    ;;
esac

dnl
dnl For mpfr-longlong.h - TODO: should be replaced (see acinclude.m4).
dnl

GMP_C_ATTRIBUTE_MODE


dnl
dnl Setup GMP detection (continued)
dnl

dnl Check minimal GMP version
dnl We only guarantee that with a *functional* and recent enough GMP version,
dnl MPFR will compile; we do not guarantee that GMP will compile.
dnl In particular fat builds are broken in GMP 4.3.2 and GMP 5.0.0
dnl (at least on 64-bit Core 2 under Linux),
dnl see <https://gmplib.org/list-archives/gmp-bugs/2011-August/002345.html>.
if test -z "$enable_mini_gmp" ; then
  AC_MSG_CHECKING(for recent GMP)
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include "gmp.h"
#if (__GNU_MP_VERSION*100+__GNU_MP_VERSION_MINOR*10 < 500)
# error "GMP 5.0.0 or newer is required"
error
#endif
  ]])],[AC_MSG_RESULT(yes)],[
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([GMP 5.0.0 or newer is required])
])
fi

dnl Check if gmp.h is usable at link time; this may detect errors such as
dnl with GMP 4.1, which uses "extern __inline__" unconditionally with all
dnl GCC versions, which breaks by default with GCC 5 (this problem with
dnl GMP 4.1 is no longer possible as we now require GMP 5.0.0 or newer,
dnl but the same kind of problem may occur in the future).
dnl Note: No linking is done against the GMP library at this time, as we
dnl do not use any GMP symbol. The goal of this test is to avoid obscure
dnl errors with the following gmp.h tests.
AC_MSG_CHECKING(usable gmp.h at link time)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
]],[[]])],[AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_MSG_ERROR([there is an incompatibility between gmp.h and the compiler.
See 'config.log' for details.])
])

dnl Check if we can use internal header files of GMP (only --with-gmp-build)
if test "$use_gmp_build" = yes ; then
   AC_MSG_CHECKING(for gmp internal files)
   AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
   #include "gmp.h"
   #include "gmp-impl.h"
   #include "longlong.h"
   ]])],[
     AC_MSG_RESULT(yes)
     AC_DEFINE([MPFR_HAVE_GMP_IMPL],1,[Use GMP Internal Files])
   ],[
     AC_MSG_ERROR([header files gmp-impl.h and longlong.h not found])
   ])
fi

dnl Check for GMP_NUMB_BITS and sizeof(mp_limb_t) consistency.
dnl Problems may occur if gmp.h was generated with some ABI
dnl and is used with another ABI (or if nails are used).
dnl This test doesn't need to link with libgmp (at least it shouldn't).
AC_MSG_CHECKING(for GMP_NUMB_BITS and sizeof(mp_limb_t) consistency)
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <limits.h>
#include "gmp.h"
]], [[
  if (GMP_NUMB_BITS == sizeof(mp_limb_t) * CHAR_BIT)
    return 0;
  fprintf (stderr, "GMP_NUMB_BITS     = %ld\n", (long) GMP_NUMB_BITS);
  fprintf (stderr, "sizeof(mp_limb_t) = %ld\n", (long) sizeof(mp_limb_t));
  fprintf (stderr, "sizeof(mp_limb_t) * CHAR_BIT = %ld != GMP_NUMB_BITS\n",
           (long) (sizeof(mp_limb_t) * CHAR_BIT));
  return 1;
]])], [AC_MSG_RESULT(yes)], [
       AC_MSG_RESULT(no)
       AC_MSG_ERROR([GMP_NUMB_BITS and sizeof(mp_limb_t) are not consistent.
You probably need to change some of the GMP or MPFR compile options.
See 'config.log' for details (search for GMP_NUMB_BITS).])],
       [AC_MSG_RESULT([cannot test])])

dnl Check if we can link with GMP
AC_CHECK_LIB(gmp, $test_libgmp, [LIBS="-lgmp $LIBS"],
 [AC_MSG_ERROR([libgmp not found or uses a different ABI (including static vs shared).
Please read the INSTALL file -- see "In case of problem".])])

dnl Check for corresponding 'gmp.h' and libgmp
AC_MSG_CHECKING(if gmp.h version and libgmp version are the same)
dnl We do not set LD_LIBRARY_PATH, as it is not possible to set it just
dnl before the test program is run, and we do not want to affect other
dnl programs (such as the compiler), because the behavior could be
dnl incorrect and even have security implications.
dnl WARNING! LD_RUN_PATH is not taken into account by the GNU gold ld,
dnl e.g. from binutils-gold 2.22-5 under Debian; see
dnl   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=660813
if test -z "$enable_mini_gmp" ; then
  saved_LD_RUN_PATH="$LD_RUN_PATH"
  LD_RUN_PATH="${LD_RUN_PATH:+$LD_RUN_PATH$PATH_SEPARATOR}$gmp_lib_path"
  export LD_RUN_PATH
  AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <string.h>
#include "gmp.h"
  ]], [[
  char buffer[100];
  sprintf (buffer, "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
           __GNU_MP_VERSION_PATCHLEVEL);
  printf ("(%s/%s) ", buffer, gmp_version);
  fflush (stdout);
  /* Also put information in config.log (stderr) */
  fprintf (stderr, "gmp.h: %s / libgmp: %s\n", buffer, gmp_version);
  if (strcmp (buffer, gmp_version) == 0)
    return 0;
  if (__GNU_MP_VERSION_PATCHLEVEL != 0)
    return 1;
  sprintf (buffer, "%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR);
  return (strcmp (buffer, gmp_version) != 0) ? 1 : 0;
  ]])],
   [AC_MSG_RESULT(yes)
    MPFR_CHECK_GMP
    MPFR_CHECK_DBL2INT_BUG
    MPFR_CHECK_PRINTF_SPEC],
   [AC_MSG_RESULT(no)
    AC_MSG_WARN([==========================================================])
    AC_MSG_WARN(['gmp.h' and 'libgmp' seem to have different versions or])
    AC_MSG_WARN([we cannot run a program linked with GMP (if you cannot])
    AC_MSG_WARN([see the version numbers above). A cause may be different])
    AC_MSG_WARN([GMP versions with different ABI's or the use of --with-gmp])
    AC_MSG_WARN([or --with-gmp-include with a system include directory])
    AC_MSG_WARN([(such as /usr/include or /usr/local/include).])
    AC_MSG_WARN([However since we can't use 'libtool' inside the configure,])
    AC_MSG_WARN([we can't be sure. See 'config.log' for details.])
    AC_MSG_WARN([CC="$CC"])
    AC_MSG_WARN([CFLAGS="$CFLAGS"])
    AC_MSG_WARN([CPPFLAGS="$CPPFLAGS"])
    AC_MSG_WARN([LDFLAGS="$LDFLAGS"])
    AC_MSG_WARN([LIBS="$LIBS"])
    AC_MSG_WARN([Temporary LD_RUN_PATH was "$LD_RUN_PATH".])
    AC_MSG_WARN([==========================================================])
    ],AC_MSG_RESULT([cannot test])
  )
  LD_RUN_PATH="$saved_LD_RUN_PATH"
fi

dnl __gmpn_sbpi1_divappr_q is an internal GMP symbol; thus its behavior
dnl may change or this symbol may be removed in the future (without being
dnl handled by the library versioning system, which is even worse, as this
dnl can mean undetected incompatibilities in case of GMP library upgrade,
dnl without rebuilding MPFR). So, this symbol must not be used, unless
dnl WANT_GMP_INTERNALS is defined. Only the GMP public API should be used
dnl by default, in particular by binary distributions. Moreover the check
dnl below may yield an incorrect result as libtool isn't used in configure
dnl (see above).
AC_CHECK_FUNCS([__gmpn_sbpi1_divappr_q])
dnl same for other GMP internal functions
AC_CHECK_FUNCS([__gmpn_invert_limb])
AC_CHECK_FUNCS([__gmpn_rsblsh_n])

MPFR_CHECK_MP_LIMB_T_VS_LONG

dnl The getrusage function is needed for MPFR bench (cf tools/bench)
AC_CHECK_FUNCS([getrusage])

dnl Remove also many MACROS (AC_DEFINE) which are unused by MPFR
dnl and polluate (and slow down because libtool has to parse them) the build.
if test -f confdefs.h; then
  $SED '/#define PACKAGE_/d' <confdefs.h >confdefs.tmp
  $SED '/#define HAVE_STRING/d' <confdefs.tmp >confdefs.h
  $SED '/#define HAVE_ALLOCA /d' <confdefs.h >confdefs.tmp
  $SED '/#define HAVE_DLFCN_H/d' <confdefs.tmp >confdefs.h
  $SED '/#define HAVE_MEM/d' <confdefs.h >confdefs.tmp
  $SED '/#define STDC_HEADERS/d' <confdefs.tmp >confdefs.h
  $SED '/#define HAVE_STRTOL/d' <confdefs.h >confdefs.tmp
  $SED '/#define HAVE_STDLIB_H/d' <confdefs.tmp >confdefs.h
  $SED '/#define HAVE_UNISTD_H/d' <confdefs.h >confdefs.tmp
  $SED '/#define HAVE_STDC_HEADERS/d' <confdefs.tmp >confdefs.h
  $SED '/#define HAVE_LONG_DOUBLE/d' <confdefs.h >confdefs.tmp
  $SED '/#define HAVE_SYS_STAT_H/d' <confdefs.tmp >confdefs.h
  $SED '/#define HAVE_SYS_TYPES_H/d' <confdefs.h >confdefs.tmp
  $SED '/#define PROTOTYPES/d' <confdefs.tmp >confdefs.h
  $SED '/#define __PROTOTYPES/d' <confdefs.h >confdefs.tmp

  mv confdefs.tmp confdefs.h
fi

if $EGREP -q -e '-dev$' $srcdir/VERSION; then
  AC_SUBST([DATAFILES])dnl
  DATAFILES=`echo \`$SED -n \
    's/^ *data_check *("\(data\/[[^"]]*\)".*/tests\/\1/p' \
    $srcdir/tests/*.c\``
fi

dnl Output
AC_CONFIG_FILES([Makefile mpfr.pc doc/Makefile src/Makefile tests/Makefile tune/Makefile src/mparam.h:src/mparam_h.in tools/bench/Makefile])
AC_OUTPUT

dnl NEWS README AUTHORS Changelog
