dnl $Heimdal: find-if-not-broken.m4,v 1.4.8.1 2004/04/01 07:27:33 joda Exp $
dnl $NetBSD: find-if-not-broken.m4,v 1.1.1.4 2004/04/02 14:48:06 lha Exp $
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
