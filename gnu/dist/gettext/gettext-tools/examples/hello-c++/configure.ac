dnl Example for use of GNU gettext.
dnl Copyright (C) 2003-2004 Free Software Foundation, Inc.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT
AC_CONFIG_SRCDIR(hello.cc)
AM_INIT_AUTOMAKE(hello-c++, 0)

AC_PROG_CXX
AC_CHECK_HEADERS([unistd.h])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.13.1)
AC_LIB_LINKFLAGS([asprintf])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile.in])
AC_OUTPUT
