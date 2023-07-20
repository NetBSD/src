#
# Copyright (c) 2023, Luke Mewburn <lukem@NetBSD.org>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
#

#
# _NB_CHECK_CC_FLAG_PREPARE
#	Check for flags to force a compiler (e.g., clang) to fail
#	if given an unknown -WWARN, and set $nb_cv_check_cc_flags
#	to that flag for NB_CHECK_CC_FLAG() to use.
#
AC_DEFUN([_NB_CHECK_CC_FLAG_PREPARE], [dnl
nb_cv_check_cc_flags=
AX_CHECK_COMPILE_FLAG([-Werror=unknown-warning-option],
 [AS_VAR_SET([nb_cv_check_cc_flags], [-Werror=unknown-warning-option])])
]) dnl _NB_CHECK_CC_FLAG_PREPARE

#
# NB_CHECK_CC_FLAG(FLAG, [VAR=FLAG_DERIVED])
#	Determine if the C compiler supports FLAG,
#	and sets output variable VAR to FLAG if FLAG is supported.
#
#	If VAR is not provided, default to FLAG_DERIVED, which is
#	FLAG converted to upper-case and all special characters
#	replaced with "_", and the result prepended with "CC_".
#	FLAG_DERIVED is appended to the m4 macro NB_CHECK_CC_FLAG_VARS.
#	E.g., if FLAG is "-Wexample=yes", FLAG_DERIVED is "CC_WEXAMPLE_YES".
#
#	Compiler-specific notes:
#	clang	Uses _NB_CHECK_CC_FLAG_PREPARE() to determine if
#		-Werror=unknown-warning-option.
#	gcc	Check for -WFLAG if FLAG is -Wno-FLAG, to work around
#		gcc silently ignoring unknown -Wno-FLAG.
#
AC_DEFUN([NB_CHECK_CC_FLAG], [dnl
AC_REQUIRE([_NB_CHECK_CC_FLAG_PREPARE])dnl
m4_ifblank([$1], [m4_fatal([Usage: $0(FLAG,[VAR=FLAG_DERIVED])])])dnl
m4_pushdef([NB_flag], [$1])dnl
m4_ifblank([$2], [dnl
m4_pushdef([NB_var], [CC]m4_translit(NB_flag, [-=a-z], [__A-Z]))dnl
m4_append([NB_CHECK_CC_FLAG_VARS], NB_var, [ ])dnl
], [dnl
m4_pushdef([NB_var], [$2])dnl
])dnl
m4_pushdef([NB_wflag], m4_bpatsubst(NB_flag, [^-Wno-], [-W]))dnl
AX_CHECK_COMPILE_FLAG(NB_wflag, [AS_VAR_SET(NB_var,NB_flag)], [], [$nb_cv_check_cc_flags])
AC_SUBST(NB_var)
m4_popdef([NB_flag])dnl
m4_popdef([NB_wflag])dnl
m4_popdef([NB_var])dnl
]) dnl NB_CHECK_CC_FLAG
