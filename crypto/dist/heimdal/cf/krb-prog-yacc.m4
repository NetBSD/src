dnl $Heimdal: krb-prog-yacc.m4,v 1.3.16.1 2004/04/01 07:27:34 joda Exp $
dnl $NetBSD: krb-prog-yacc.m4,v 1.1.1.3.2.1 2004/04/21 04:55:38 jmc Exp $
dnl
dnl
dnl We prefer byacc or yacc because they do not use `alloca'
dnl

AC_DEFUN([AC_KRB_PROG_YACC],
[AC_CHECK_PROGS(YACC, byacc yacc 'bison -y')
if test "$YACC" = ""; then
  AC_MSG_WARN([yacc not found - some stuff will not build])
fi
])
