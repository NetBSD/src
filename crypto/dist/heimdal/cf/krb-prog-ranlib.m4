dnl $Heimdal: krb-prog-ranlib.m4,v 1.1 1997/12/14 15:59:01 joda Exp $
dnl $NetBSD: krb-prog-ranlib.m4,v 1.1.1.3 2002/09/12 12:41:44 joda Exp $
dnl
dnl
dnl Also look for EMXOMF for OS/2
dnl

AC_DEFUN(AC_KRB_PROG_RANLIB,
[AC_CHECK_PROGS(RANLIB, ranlib EMXOMF, :)])
