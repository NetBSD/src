dnl $Id: krb-prog-yacc.m4,v 1.1.1.1.4.2 2000/06/16 18:32:18 thorpej Exp $
dnl
dnl
dnl We prefer byacc or yacc because they do not use `alloca'
dnl

AC_DEFUN(AC_KRB_PROG_YACC,
[AC_CHECK_PROGS(YACC, byacc yacc 'bison -y')
if test "$YACC" = ""; then
  AC_MSG_WARN([yacc not found - some stuff will not build])
fi
])
