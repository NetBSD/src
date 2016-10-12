dnl Autoconf configure script for GDB, the GNU debugger.
dnl Copyright (C) 1995-2016 Free Software Foundation, Inc.
dnl
dnl This file is part of GDB.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([AM_GDB_WARNINGS],[
AC_ARG_ENABLE(werror,
  AS_HELP_STRING([--enable-werror], [treat compile warnings as errors]),
  [case "${enableval}" in
     yes | y) ERROR_ON_WARNING="yes" ;;
     no | n)  ERROR_ON_WARNING="no" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-werror) ;;
   esac])

# Enable -Werror by default when using gcc.  Turn it off for releases.
if test "${GCC}" = yes -a -z "${ERROR_ON_WARNING}" && $development; then
    ERROR_ON_WARNING=yes
fi

WERROR_CFLAGS=""
if test "${ERROR_ON_WARNING}" = yes ; then
    WERROR_CFLAGS="-Werror"
fi

# These options work in either C or C++ modes.
build_warnings="-Wall -Wpointer-arith \
-Wno-unused -Wunused-value -Wunused-function \
-Wno-switch -Wno-char-subscripts \
-Wempty-body -Wunused-but-set-parameter -Wunused-but-set-variable"

# Now add in C and C++ specific options, depending on mode.
if test "$enable_build_with_cxx" = "yes"; then
   build_warnings="$build_warnings -Wno-sign-compare -Wno-write-strings \
-Wno-narrowing"
else
   build_warnings="$build_warnings -Wpointer-sign -Wmissing-prototypes \
-Wdeclaration-after-statement -Wmissing-parameter-type \
-Wold-style-declaration -Wold-style-definition"
fi

# Enable -Wno-format by default when using gcc on mingw since many
# GCC versions complain about %I64.
case "${host}" in
  *-*-mingw32*) build_warnings="$build_warnings -Wno-format" ;;
  *) build_warnings="$build_warnings -Wformat-nonliteral" ;;
esac

AC_ARG_ENABLE(build-warnings,
AS_HELP_STRING([--enable-build-warnings], [enable build-time compiler warnings if gcc is used]),
[case "${enableval}" in
  yes)	;;
  no)	build_warnings="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${build_warnings} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${t} ${build_warnings}";;
  *)    build_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$build_warnings" != x""; then
  echo "Setting compiler warning flags = $build_warnings" 6>&1
fi])dnl
AC_ARG_ENABLE(gdb-build-warnings,
AS_HELP_STRING([--enable-gdb-build-warnings], [enable GDB specific build-time compiler warnings if gcc is used]),
[case "${enableval}" in
  yes)	;;
  no)	build_warnings="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${build_warnings} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${t} ${build_warnings}";;
  *)    build_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$build_warnings" != x""; then
  echo "Setting GDB specific compiler warning flags = $build_warnings" 6>&1
fi])dnl

# The set of warnings supported by a C++ compiler is not the same as
# of the C compiler.
if test "$enable_build_with_cxx" = "yes"; then
    AC_LANG_PUSH([C++])
fi

WARN_CFLAGS=""
if test "x${build_warnings}" != x -a "x$GCC" = xyes
then
    AC_MSG_CHECKING(compiler warning flags)
    # Separate out the -Werror flag as some files just cannot be
    # compiled with it enabled.
    for w in ${build_warnings}; do
	# GCC does not complain about -Wno-unknown-warning.  Invert
	# and test -Wunknown-warning instead.
	case $w in
	-Wno-*)
		wtest=`echo $w | sed 's/-Wno-/-W/g'` ;;
	*)
		wtest=$w ;;
	esac

	case $w in
	-Werr*) WERROR_CFLAGS=-Werror ;;
	*)
	    # Check whether GCC accepts it.
	    saved_CFLAGS="$CFLAGS"
	    CFLAGS="$CFLAGS $wtest"
	    saved_CXXFLAGS="$CXXFLAGS"
	    CXXFLAGS="$CXXFLAGS $wtest"
	    AC_TRY_COMPILE([],[],WARN_CFLAGS="${WARN_CFLAGS} $w",)
	    CFLAGS="$saved_CFLAGS"
	    CXXFLAGS="$saved_CXXFLAGS"
	esac
    done
    AC_MSG_RESULT(${WARN_CFLAGS} ${WERROR_CFLAGS})
fi
AC_SUBST(WARN_CFLAGS)
AC_SUBST(WERROR_CFLAGS)

if test "$enable_build_with_cxx" = "yes"; then
   AC_LANG_POP([C++])
fi
])
