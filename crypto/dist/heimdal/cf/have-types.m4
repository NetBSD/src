dnl
dnl $Heimdal: have-types.m4,v 1.2.12.1 2004/04/01 07:27:33 joda Exp $
dnl $NetBSD: have-types.m4,v 1.1.1.4 2004/04/02 14:48:06 lha Exp $
dnl

AC_DEFUN([AC_HAVE_TYPES], [
for i in $1; do
        AC_HAVE_TYPE($i)
done
if false;then
	AC_CHECK_FUNCS($1)
fi
])
