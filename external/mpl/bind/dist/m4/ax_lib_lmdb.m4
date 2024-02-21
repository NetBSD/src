# SPDX-License-Identifier: FSFAP
#
# ===========================================================================
#      https://gitlab.isc.org/isc-projects/autoconf-archive/ax_lib_lmdb.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_LIB_LMDB(PATH[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   Test for the LMDB library in a path
#
#   This macro takes only one argument, a path to the lmdb headers and library.
#
# LICENSE
#
#   Copyright (c) 2020 Internet Systems Consortium
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 19

#
AC_DEFUN([AX_LIB_LMDB],
	 [AS_IF([test -z "$1"],
		[AC_MSG_ERROR([Path cannot be empty])])
	  AC_MSG_CHECKING([for lmdb header in $1])
	  AS_IF([test -r "$1/include/lmdb.h"],
		[AC_MSG_RESULT([yes])
		 LMDB_CFLAGS="-I$1/include"
		 LMDB_LIBS="-L$1/lib"
		 AX_SAVE_FLAGS([lmdb])
		 CFLAGS="$CFLAGS $LMDB_CFLAGS"
		 LIBS="$LIBS $LMDB_LIBS"
		 AC_SEARCH_LIBS([mdb_env_create], [lmdb],
				[LMDB_LIBS="$LMDB_LIBS $ac_cv_search_mdb_env_create"
				 AX_RESTORE_FLAGS([lmdb])
				 $2
				],
				[AC_MSG_RESULT([no])
				 LMDB_CFLAGS=""
				 LMDB_LIBS=""
				 AX_RESTORE_FLAGS([lmdb])
				 $3
				])
		],
		[AC_MSG_RESULT([no])])])
