# Extract data from preprocessor output using sed expresions.

# Copyright (c) 2003
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

# ACX_EXTRACT_CPP_DEFN(VARIABLE, PROGRAM,
#                      [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -------------------------------------------
# Extract single line definitions from preprocessor output.  If the
# output of the C preprocessor on PROGRAM contains a definition for VARIABLE,
# assign it to shell variable VARIABLE and run ACTION-IF-FOUND.  Otherwise run
# ACTION-IF-NOT-FOUND.
AC_DEFUN([ACX_EXTRACT_CPP_DEFN],
[AC_LANG_PREPROC_REQUIRE()dnl
AC_LANG_CONFTEST([AC_LANG_SOURCE([[$2]])])
dnl eval is necessary to expand ac_cpp.
dnl Ultrix and Pyramid sh refuse to redirect output of eval, so use subshell.
ac_extract_cpp_result=`(eval "$ac_cpp -dM conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD |
dnl m4 quote the argument to sed to prevent m4 from eating character classes
dnl
dnl C89 6.8.3 Specifies that all whitespace separation in macro definitions is
dnl equivalent, so eat leading and trailing whitespace to account for
dnl preprocessor quirks (commands 1.2 & 1.3 in the sed script below).
  sed -n ["/^#define $1 /{
            s/^#define $1 //;
	    s/^ *//;
	    s/ *\$//;
            p;}"] 2>&AS_MESSAGE_LOG_FD`
if test -n "$ac_extract_cpp_result"; then
  $1=$ac_extract_cpp_result
m4_ifvaln([$3], [$3])dnl
m4_ifvaln([$4], [else
  $4])dnl
fi
rm -f conftest*
])# ACX_EXTRACT_CPP_DEFN



# ACX_EXTRACT_HEADER_DEFN(VARIABLE, HEADER-FILE,
#                    [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------
AC_DEFUN([ACX_EXTRACT_HEADER_DEFN],
[ACX_EXTRACT_CPP_DEFN([$1],
[#include <$2>
], [$3], [$4])])
