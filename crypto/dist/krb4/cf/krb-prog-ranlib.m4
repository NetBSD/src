dnl $Id: krb-prog-ranlib.m4,v 1.1.1.1.4.2 2000/06/16 18:46:12 thorpej Exp $
dnl
dnl
dnl Also look for EMXOMF for OS/2
dnl

AC_DEFUN(AC_KRB_PROG_RANLIB,
[AC_CHECK_PROGS(RANLIB, ranlib EMXOMF, :)])
