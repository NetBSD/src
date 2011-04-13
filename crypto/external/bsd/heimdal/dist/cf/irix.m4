dnl
dnl $Id: irix.m4,v 1.1.1.1 2011/04/13 18:14:32 elric Exp $
dnl

AC_DEFUN([rk_IRIX],
[
irix=no
case "$host" in
*-*-irix*) 
	irix=yes
	;;
esac
AM_CONDITIONAL(IRIX, test "$irix" != no)dnl

])
