dnl $Heimdal: find-func-no-libs.m4,v 1.5.20.1 2004/04/01 07:27:33 joda Exp $
dnl $NetBSD: find-func-no-libs.m4,v 1.1.1.3.2.1 2004/04/21 04:55:38 jmc Exp $
dnl
dnl
dnl Look for function in any of the specified libraries
dnl

dnl AC_FIND_FUNC_NO_LIBS(func, libraries, includes, arguments, extra libs, extra args)
AC_DEFUN([AC_FIND_FUNC_NO_LIBS], [
AC_FIND_FUNC_NO_LIBS2([$1], ["" $2], [$3], [$4], [$5], [$6])])
