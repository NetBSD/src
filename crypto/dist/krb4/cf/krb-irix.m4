dnl
dnl $Id: krb-irix.m4,v 1.1.1.2 2002/09/12 12:22:14 joda Exp $
dnl

dnl requires AC_CANONICAL_HOST
AC_DEFUN(KRB_IRIX,[
irix=no
case "$host_os" in
irix*) irix=yes ;;
esac
AM_CONDITIONAL(IRIX, test "$irix" != no)dnl
])
