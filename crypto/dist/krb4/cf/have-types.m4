dnl
dnl $Id: have-types.m4,v 1.1.1.2 2000/12/29 01:44:09 assar Exp $
dnl

AC_DEFUN(AC_HAVE_TYPES, [
for i in $1; do
        AC_HAVE_TYPE($i)
done
: << END
changequote(`,')dnl
@@@funcs="$funcs $1"@@@
changequote([,])dnl
END
])
