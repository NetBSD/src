dnl
dnl $Heimdal: have-types.m4 13338 2004-02-12 14:21:14Z lha $
dnl $NetBSD: have-types.m4,v 1.2 2008/03/22 08:36:58 mlelstv Exp $
dnl

AC_DEFUN([AC_HAVE_TYPES], [
for i in $1; do
        AC_HAVE_TYPE($i)
done
if false;then
	AC_CHECK_FUNCS($1)
fi
])
