dnl
dnl $Id: capabilities.m4,v 1.1.1.1 2000/06/16 18:46:09 thorpej Exp $
dnl

dnl
dnl Test SGI capabilities
dnl

AC_DEFUN(KRB_CAPABILITIES,[

AC_CHECK_HEADERS(capability.h sys/capability.h)

AC_CHECK_FUNCS(sgi_getcapabilitybyname cap_set_proc)
])
