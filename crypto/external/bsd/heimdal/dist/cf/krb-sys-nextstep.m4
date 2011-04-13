dnl $Id: krb-sys-nextstep.m4,v 1.1.1.1 2011/04/13 18:14:32 elric Exp $
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
