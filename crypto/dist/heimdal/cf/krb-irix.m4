dnl
dnl $Id: krb-irix.m4,v 1.1.1.1.2.2 2001/04/05 23:24:29 he Exp $
dnl

dnl requires AC_CANONICAL_HOST
AC_DEFUN(KRB_IRIX,[
irix=no
case "$host_os" in
irix*) irix=yes ;;
esac
AM_CONDITIONAL(IRIX, test "$irix" != no)dnl
])
