dnl
dnl $Heimdal: have-types.m4,v 1.2 2000/07/15 18:09:16 joda Exp $
dnl $NetBSD: have-types.m4,v 1.1.1.3 2002/09/12 12:41:44 joda Exp $
dnl

AC_DEFUN(AC_HAVE_TYPES, [
for i in $1; do
        AC_HAVE_TYPE($i)
done
if false;then
	AC_CHECK_FUNCS($1)
fi
])
