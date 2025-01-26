# SPDX-License-Identifier: FSFAP
#
# ===========================================================================
#      https://gitlab.isc.org/isc-projects/autoconf-archive/ax_jemalloc.html
# ===========================================================================
#
# SYNOPSIS
#
#   PKG_CHECK_VERSION(VARIABLE, MODULE, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Retrieves the value of the pkg-config version for the given module.
#
# LICENSE
#
#   Copyright (c) 2023 Internet Systems Consortium
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

#
AC_DEFUN([PKG_CHECK_VERSION],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
AC_ARG_VAR([$1], [version of $2, overriding pkg-config])dnl

_PKG_CONFIG([$1], [modversion], [$2])
AS_VAR_COPY([$1], [pkg_cv_][$1])

AS_VAR_IF([$1], [""], [$4], [$3])dnl
])dnl PKG_CHECK_VERSION
