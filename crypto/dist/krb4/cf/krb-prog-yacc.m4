dnl $Id: krb-prog-yacc.m4,v 1.1.1.2 2000/12/29 01:44:09 assar Exp $
dnl
dnl
dnl We prefer byacc or yacc because they do not use `alloca'
dnl

AC_DEFUN(AC_KRB_PROG_YACC,
[AC_CHECK_PROGS(YACC, byacc yacc 'bison -y')])
