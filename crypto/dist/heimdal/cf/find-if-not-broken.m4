dnl $Heimdal: find-if-not-broken.m4 13338 2004-02-12 14:21:14Z lha $
dnl $NetBSD: find-if-not-broken.m4,v 1.2 2008/03/22 08:36:58 mlelstv Exp $
dnl
dnl
dnl Mix between AC_FIND_FUNC and AC_BROKEN
dnl

AC_DEFUN([AC_FIND_IF_NOT_BROKEN],
[AC_FIND_FUNC([$1], [$2], [$3], [$4])
if eval "test \"$ac_cv_func_$1\" != yes"; then 
	rk_LIBOBJ([$1])
fi
])
