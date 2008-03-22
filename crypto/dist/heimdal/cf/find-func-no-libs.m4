dnl $Heimdal: find-func-no-libs.m4 13338 2004-02-12 14:21:14Z lha $
dnl $NetBSD: find-func-no-libs.m4,v 1.2 2008/03/22 08:36:58 mlelstv Exp $
dnl
dnl
dnl Look for function in any of the specified libraries
dnl

dnl AC_FIND_FUNC_NO_LIBS(func, libraries, includes, arguments, extra libs, extra args)
AC_DEFUN([AC_FIND_FUNC_NO_LIBS], [
AC_FIND_FUNC_NO_LIBS2([$1], ["" $2], [$3], [$4], [$5], [$6])])
