dnl $Id: osfc2.m4,v 1.1.1.1 2000/06/16 18:46:11 thorpej Exp $
dnl
dnl enable OSF C2 stuff

AC_DEFUN(AC_CHECK_OSFC2,[
AC_ARG_ENABLE(osfc2,
[  --enable-osfc2          enable some OSF C2 support])
LIB_security=
if test "$enable_osfc2" = yes; then
	AC_DEFINE(HAVE_OSFC2, 1, [Define to enable basic OSF C2 support.])
	LIB_security=-lsecurity
fi
AC_SUBST(LIB_security)
])
