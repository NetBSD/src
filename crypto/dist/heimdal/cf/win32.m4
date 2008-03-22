dnl $Heimdal: win32.m4 13709 2004-04-13 14:29:47Z lha $
dnl $NetBSD: win32.m4,v 1.1 2008/03/22 08:36:58 mlelstv Exp $
dnl rk_WIN32_EXPORT buildsymbol symbol-that-export
AC_DEFUN([rk_WIN32_EXPORT],[AH_TOP([#ifdef $1
#ifndef $2
#ifdef _WIN32_
#define $2 _export _stdcall
#else
#define $2
#endif
#endif
#endif
])])
