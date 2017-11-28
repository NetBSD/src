dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN(LITTLE)
SIM_AC_OPTION_ALIGNMENT(NONSTRICT_ALIGNMENT)
SIM_AC_OPTION_WARNINGS
SIM_AC_OPTION_RESERVED_BITS
SIM_AC_OPTION_BITSIZE(32,31)
SIM_AC_OPTION_HARDWARE(yes,,mn103cpu mn103int mn103tim mn103ser mn103iop)

AC_CHECK_FUNCS(time chmod utime fork execve execv chown)
AC_CHECK_HEADERS(utime.h)

SIM_AC_OUTPUT
