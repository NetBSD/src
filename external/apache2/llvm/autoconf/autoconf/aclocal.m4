# generated automatically by aclocal 1.16.3 -*- Autoconf -*-

# Copyright (C) 1996-2020 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

m4_ifndef([AC_CONFIG_MACRO_DIRS], [m4_defun([_AM_CONFIG_MACRO_DIRS], [])m4_defun([AC_CONFIG_MACRO_DIRS], [_AM_CONFIG_MACRO_DIRS($@)])])
# Check for the extension used for executables on build platform.
# This is necessary for cross-compiling where the build platform
# may differ from the host platform.
AC_DEFUN([AC_BUILD_EXEEXT],
[
AC_MSG_CHECKING([for executable suffix on build platform])
AC_CACHE_VAL(ac_cv_build_exeext,
[if test "$CYGWIN" = yes || test "$MINGW32" = yes; then
  ac_cv_build_exeext=.exe
else
  ac_build_prefix=${build_alias}-

  AC_CHECK_PROG(BUILD_CC, ${ac_build_prefix}gcc, ${ac_build_prefix}gcc)
  if test -z "$BUILD_CC"; then
     AC_CHECK_PROG(BUILD_CC, gcc, gcc)
     if test -z "$BUILD_CC"; then
       AC_CHECK_PROG(BUILD_CC, cc, cc, , , /usr/ucb/cc)
     fi
  fi
  test -z "$BUILD_CC" && AC_MSG_ERROR([no acceptable cc found in \$PATH])
  ac_build_link='${BUILD_CC-cc} -o conftest $CFLAGS $CPPFLAGS $LDFLAGS conftest.$ac_ext $LIBS 1>&AS_MESSAGE_LOG_FD'
  rm -f conftest*
  echo 'int main () { return 0; }' > conftest.$ac_ext
  ac_cv_build_exeext=
  if AC_TRY_EVAL(ac_build_link); then
    for file in conftest.*; do
      case $file in
      *.c | *.o | *.obj | *.dSYM) ;;
      *) ac_cv_build_exeext=`echo $file | sed -e s/conftest//` ;;
      esac
    done
  else
    AC_MSG_ERROR([installation or configuration problem: compiler cannot create executables.])
  fi
  rm -f conftest*
  test x"${ac_cv_build_exeext}" = x && ac_cv_build_exeext=blank
fi])
BUILD_EXEEXT=""
test x"${ac_cv_build_exeext}" != xblank && BUILD_EXEEXT=${ac_cv_build_exeext}
AC_MSG_RESULT(${ac_cv_build_exeext})
ac_build_exeext=$BUILD_EXEEXT
AC_SUBST(BUILD_EXEEXT)])

#
# Check for GNU Make.  This is originally from
# http://www.gnu.org/software/ac-archive/htmldoc/check_gnu_make.html
#
AC_DEFUN([AC_CHECK_GNU_MAKE],
[AC_CACHE_CHECK([for GNU make],[llvm_cv_gnu_make_command],
dnl Search all the common names for GNU make
[llvm_cv_gnu_make_command=''
 for a in "$MAKE" make gmake gnumake ; do
  if test -z "$a" ; then continue ; fi ;
  if  ( sh -c "$a --version" 2> /dev/null | grep GNU 2>&1 > /dev/null ) 
  then
   llvm_cv_gnu_make_command=$a ;
   break;
  fi
 done])
dnl If there was a GNU version, then set @ifGNUmake@ to the empty string, 
dnl '#' otherwise
 if test "x$llvm_cv_gnu_make_command" != "x"  ; then
   ifGNUmake='' ;
 else
   ifGNUmake='#' ;
   AC_MSG_RESULT("Not found");
 fi
 AC_SUBST(ifGNUmake)
])

AC_DEFUN([CXX_FLAG_CHECK],
  [AC_SUBST($1, `$CXX -Werror patsubst($2, [^-Wno-], [-W]) -fsyntax-only -xc /dev/null 2>/dev/null && echo $2`)])

# Combine AC_DEFINE and AC_SUBST
AC_DEFUN([LLVM_DEFINE_SUBST], [
AC_DEFINE([$1], [$2], [$3])
AC_SUBST([$1], ['$2'])
])

#
# This function determins if the HUGE_VAL macro is compilable with the 
# -pedantic switch or not. XCode < 2.4.1 doesn't get it right.
#
AC_DEFUN([AC_HUGE_VAL_CHECK],[
  AC_CACHE_CHECK([for HUGE_VAL sanity], [ac_cv_huge_val_sanity],[
    AC_LANG_PUSH([C++])
    ac_save_CXXFLAGS=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS -pedantic"
    AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <math.h>]],
                                   [[double x = HUGE_VAL; return x != x;]])],
                  [ac_cv_huge_val_sanity=yes],[ac_cv_huge_val_sanity=no],
                  [ac_cv_huge_val_sanity=yes])
    CXXFLAGS=$ac_save_CXXFLAGS
    AC_LANG_POP([C++])
    ])
  AC_SUBST(HUGE_VAL_SANITY,$ac_cv_huge_val_sanity)
])

#
# Get the linker version string.
#
# This macro is specific to LLVM.
#
AC_DEFUN([AC_LINK_GET_VERSION],
  [AC_CACHE_CHECK([for linker version],[llvm_cv_link_version],
  [
   version_string="$(${LD:-ld} -v 2>&1 | head -1)"

   # Check for ld64.
   if (echo "$version_string" | grep -q "ld64"); then
     llvm_cv_link_version=$(echo "$version_string" | sed -e "s#.*ld64-\([^ ]*\)\( (.*)\)\{0,1\}#\1#")
   else
     llvm_cv_link_version=$(echo "$version_string" | sed -e "s#[^0-9]*\([0-9.]*\).*#\1#")
   fi
  ])
  AC_DEFINE_UNQUOTED([HOST_LINK_VERSION],"$llvm_cv_link_version",
                     [Linker version detected at compile time.])
])

#
# Determine if the system can handle the -R option being passed to the linker.
#
# This macro is specific to LLVM.
#
AC_DEFUN([AC_LINK_USE_R],
[AC_CACHE_CHECK([for compiler -Wl,-R<path> option],[llvm_cv_link_use_r],
[ AC_LANG_PUSH([C])
  oldcflags="$CFLAGS"
  CFLAGS="$CFLAGS -Wl,-R."
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[]])],
    [llvm_cv_link_use_r=yes],[llvm_cv_link_use_r=no])
  CFLAGS="$oldcflags"
  AC_LANG_POP([C])
])
if test "$llvm_cv_link_use_r" = yes ; then
  AC_DEFINE([HAVE_LINK_R],[1],[Define if you can use -Wl,-R. to pass -R. to the linker, in order to add the current directory to the dynamic linker search path.])
  fi
])

#
# Determine if the system can handle the -rdynamic option being passed
# to the compiler.
#
# This macro is specific to LLVM.
#
AC_DEFUN([AC_LINK_EXPORT_DYNAMIC],
[AC_CACHE_CHECK([for compiler -rdynamic option],
                [llvm_cv_link_use_export_dynamic],
[ AC_LANG_PUSH([C])
  oldcflags="$CFLAGS"
  CFLAGS="$CFLAGS -rdynamic"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[]])],
    [llvm_cv_link_use_export_dynamic=yes],[llvm_cv_link_use_export_dynamic=no])
  CFLAGS="$oldcflags"
  AC_LANG_POP([C])
])
if test "$llvm_cv_link_use_export_dynamic" = yes ; then
  AC_DEFINE([HAVE_LINK_EXPORT_DYNAMIC],[1],[Define if you can use -rdynamic.])
  fi
])

#
# Determine if the system can handle the --version-script option being
# passed to the linker.
#
# This macro is specific to LLVM.
#
AC_DEFUN([AC_LINK_VERSION_SCRIPT],
[AC_CACHE_CHECK([for compiler -Wl,--version-script option],
                [llvm_cv_link_use_version_script],
[ AC_LANG_PUSH([C])
  oldcflags="$CFLAGS"

  # The following code is from the autoconf manual,
  # "11.13: Limitations of Usual Tools".
  # Create a temporary directory $tmp in $TMPDIR (default /tmp).
  # Use mktemp if possible; otherwise fall back on mkdir,
  # with $RANDOM to make collisions less likely.
  : ${TMPDIR=/tmp}
  {
    tmp=`
      (umask 077 && mktemp -d "$TMPDIR/fooXXXXXX") 2>/dev/null
    ` &&
    test -n "$tmp" && test -d "$tmp"
  } || {
    tmp=$TMPDIR/foo$$-$RANDOM
    (umask 077 && mkdir "$tmp")
  } || exit $?

  echo "{" > "$tmp/export.map"
  echo "  global: main;" >> "$tmp/export.map"
  echo "  local: *;" >> "$tmp/export.map"
  echo "};" >> "$tmp/export.map"

  CFLAGS="$CFLAGS -Wl,--version-script=$tmp/export.map"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[]])],
    [llvm_cv_link_use_version_script=yes],[llvm_cv_link_use_version_script=no])
  rm "$tmp/export.map"
  rmdir "$tmp"
  CFLAGS="$oldcflags"
  AC_LANG_POP([C])
])
if test "$llvm_cv_link_use_version_script" = yes ; then
  AC_SUBST(HAVE_LINK_VERSION_SCRIPT,1)
  fi
])


#
# Some Linux machines run a 64-bit kernel with a 32-bit userspace. 'uname -m'
# shows these as x86_64. Ask the system 'gcc' what it thinks.
#
AC_DEFUN([AC_IS_LINUX_MIXED],
[AC_CACHE_CHECK(for 32-bit userspace on 64-bit system,llvm_cv_linux_mixed,
[ AC_LANG_PUSH([C])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[#ifndef __x86_64__
       error: Not x86-64 even if uname says so!
      #endif
      ]])],
      [llvm_cv_linux_mixed=no],
      [llvm_cv_linux_mixed=yes])
  AC_LANG_POP([C])
])
])

#
# Determine if the compiler accepts -fvisibility-inlines-hidden
#
# This macro is specific to LLVM.
#
AC_DEFUN([AC_CXX_USE_VISIBILITY_INLINES_HIDDEN],
[AC_CACHE_CHECK([for compiler -fvisibility-inlines-hidden option],
                [llvm_cv_cxx_visibility_inlines_hidden],
[ AC_LANG_PUSH([C++])
  oldcxxflags="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -O0 -fvisibility-inlines-hidden -Werror"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [template <typename T> struct X { void __attribute__((noinline)) f() {} };],
    [X<int>().f();])],
    [llvm_cv_cxx_visibility_inlines_hidden=yes],[llvm_cv_cxx_visibility_inlines_hidden=no])
  CXXFLAGS="$oldcxxflags"
  AC_LANG_POP([C++])
])
if test "$llvm_cv_cxx_visibility_inlines_hidden" = yes ; then
  AC_SUBST([ENABLE_VISIBILITY_INLINES_HIDDEN],[1])
else
  AC_SUBST([ENABLE_VISIBILITY_INLINES_HIDDEN],[0])
fi
])

