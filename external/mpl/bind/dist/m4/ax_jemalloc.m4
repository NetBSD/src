# SPDX-License-Identifier: FSFAP
#
# ===========================================================================
#      https://gitlab.isc.org/isc-projects/autoconf-archive/ax_jemalloc.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_JEMALLOC([, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   Test for the jemalloc library in a path
#
# LICENSE
#
#   Copyright (c) 2021 Internet Systems Consortium
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

#
AC_DEFUN([AX_CHECK_JEMALLOC], [
     found=false
     PKG_CHECK_MODULES(
	[JEMALLOC], [jemalloc],
	[
	    found=true
	], [
	    AC_CHECK_HEADERS([malloc_np.h jemalloc/jemalloc.h],
		[
		    save_LIBS="$LIBS"
		    save_LDFLAGS="$LDFLAGS"
		    save_CPPFLAGS="$CPPFLAGS"
		    AC_SEARCH_LIBS([mallocx], [jemalloc],
			[
			    found=true
			    AS_IF([test "$ac_cv_search_mallocx" != "none required"],
				[JEMALLOC_LIBS="$ac_cv_search_mallocx"])
			])
		    CPPFLAGS="$save_CPPFLAGS"
		    LDFLAGS="$save_LDFLAGS"
		    LIBS="$save_LIBS"
		])
	])

    AS_IF([$found], [$1], [$2])

    AC_SUBST([JEMALLOC_CFLAGS])
    AC_SUBST([JEMALLOC_LIBS])
])
