dnl
dnl $Heimdal: dlopen.m4,v 1.1 2002/08/28 16:32:16 joda Exp $
dnl $NetBSD: dlopen.m4,v 1.1.1.1 2002/09/12 12:41:44 joda Exp $
dnl

AC_DEFUN([rk_DLOPEN], [
	AC_FIND_FUNC_NO_LIBS(dlopen, dl)
	AM_CONDITIONAL(HAVE_DLOPEN, test "$ac_cv_funclib_dlopen" != no)
])
