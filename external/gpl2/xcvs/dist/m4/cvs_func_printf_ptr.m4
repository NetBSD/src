# Determine whether printf supports %p for printing pointers.

# Copyright (c) 2003 Free Software Foundation, Inc.

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

# CVS_FUNC_PRINTF_PTR
# -------------------
# Determine whether printf supports %p for printing pointers.
AC_DEFUN([CVS_FUNC_PRINTF_PTR],
[AC_CACHE_CHECK(whether printf supports %p,
  cvs_cv_func_printf_ptr,
[AC_TRY_RUN([#include <stdio.h>
/* If printf supports %p, exit 0. */
int
main ()
{
  void *p1, *p2;
  char buf[256];
  p1 = &p1; p2 = &p2;
  sprintf(buf, "%p", p1);
  exit(sscanf(buf, "%p", &p2) != 1 || p2 != p1);
}], cvs_cv_func_printf_ptr=yes, cvs_cv_func_printf_ptr=no)
rm -f core core.* *.core])
if test $cvs_cv_func_printf_ptr = yes; then
  AC_DEFINE(HAVE_PRINTF_PTR, 1,
            [Define to 1 if the `printf' function supports the %p format
	     for printing pointers.])
fi
])# CVS_FUNC_PRINTF_PTR
