dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

AC_CHECK_HEADERS(unistd.h stdint.h)

COPRO="armcopro.o maverick.o iwmmxt.o"

AC_SUBST(COPRO)

SIM_AC_OUTPUT
