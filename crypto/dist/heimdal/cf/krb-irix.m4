dnl
dnl $Id: krb-irix.m4,v 1.1.1.1 2001/02/11 13:51:51 assar Exp $
dnl

dnl requires AC_CANONICAL_HOST
AC_DEFUN(KRB_IRIX,[
irix=no
case "$host_os" in
irix*) irix=yes ;;
esac
AM_CONDITIONAL(IRIX, test "$irix" != no)dnl
])
