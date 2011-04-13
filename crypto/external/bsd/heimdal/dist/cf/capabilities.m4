dnl
dnl $Id: capabilities.m4,v 1.1.1.1 2011/04/13 18:14:32 elric Exp $
dnl

dnl
dnl Test SGI capabilities
dnl

AC_DEFUN([KRB_CAPABILITIES],[

AC_CHECK_HEADERS(capability.h sys/capability.h)

AC_CHECK_FUNCS(sgi_getcapabilitybyname cap_set_proc)
])
