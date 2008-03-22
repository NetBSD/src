dnl
dnl $Heimdal: capabilities.m4 13338 2004-02-12 14:21:14Z lha $
dnl $NetBSD: capabilities.m4,v 1.2 2008/03/22 08:36:58 mlelstv Exp $
dnl

dnl
dnl Test SGI capabilities
dnl

AC_DEFUN([KRB_CAPABILITIES],[

AC_CHECK_HEADERS(capability.h sys/capability.h)

AC_CHECK_FUNCS(sgi_getcapabilitybyname cap_set_proc)
])
