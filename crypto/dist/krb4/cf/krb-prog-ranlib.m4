dnl $Id: krb-prog-ranlib.m4,v 1.1.1.2 2000/12/29 01:44:09 assar Exp $
dnl
dnl
dnl Also look for EMXOMF for OS/2
dnl

AC_DEFUN(AC_KRB_PROG_RANLIB,
[AC_CHECK_PROGS(RANLIB, ranlib EMXOMF, :)])
