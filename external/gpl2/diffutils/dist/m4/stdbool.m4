# Check for stdbool.h that conforms to C99.

# Copyright (C) 2002 Free Software Foundation, Inc.

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

AC_DEFUN([AC_HEADER_STDBOOL],
  [AC_CACHE_CHECK([for stdbool.h that conforms to C99],
     [ac_cv_header_stdbool_h],
     [AC_TRY_COMPILE(
	[
	  #include <stdbool.h>
	  #ifndef bool
	   "error: bool is not defined"
	  #endif
	  #ifndef false
	   "error: false is not defined"
	  #endif
	  #ifndef true
	   "error: false is not defined"
	  #endif
	  #ifndef __bool_true_false_are_defined
	   "error: __bool_true_false_are_defined is not defined"
	  #endif

	  struct s { _Bool s: 1; _Bool t; } s;

	  char a[true == 1 ? 1 : -1];
	  char b[false == 0 ? 1 : -1];
	  char c[__bool_true_false_are_defined == 1 ? 1 : -1];
	  char d[(bool) -0.5 == true ? 1 : -1];
	  char e[(bool) &s == true ? 1 : -1];
	  char f[(_Bool) -0.0 == false ? 1 : -1];
	  char g[true];
	  char h[sizeof (_Bool)];
	  char i[sizeof s.t];
	],
	[ return !a + !b + !c + !d + !e + !f + !g + !h + !i; ],
	[ac_cv_header_stdbool_h=yes],
	[ac_cv_header_stdbool_h=no])])
   if test $ac_cv_header_stdbool_h = yes; then
     AC_DEFINE(HAVE_STDBOOL_H, 1, [Define to 1 if stdbool.h conforms to C99.])
   fi])
