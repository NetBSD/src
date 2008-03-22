dnl $Heimdal: krb-sys-nextstep.m4 13338 2004-02-12 14:21:14Z lha $
dnl $NetBSD: krb-sys-nextstep.m4,v 1.2 2008/03/22 08:36:58 mlelstv Exp $
dnl
dnl NEXTSTEP is not posix compliant by default,
dnl you need a switch -posix to the compiler
dnl

AC_DEFUN([rk_SYS_NEXTSTEP], [
AC_CACHE_CHECK(for NeXTSTEP, rk_cv_sys_nextstep, [
AC_EGREP_CPP(yes, 
[#if defined(NeXT) && !defined(__APPLE__)
	yes
#endif 
], rk_cv_sys_nextstep=yes, rk_cv_sys_nextstep=no)])
if test "$rk_cv_sys_nextstep" = "yes"; then
  CFLAGS="$CFLAGS -posix"
  LIBS="$LIBS -posix"
fi
])
