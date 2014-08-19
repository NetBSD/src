dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN(LITTLE_ENDIAN)
SIM_AC_OPTION_ALIGNMENT(NONSTRICT_ALIGNMENT)
SIM_AC_OPTION_HOSTENDIAN
SIM_AC_OPTION_WARNINGS
SIM_AC_OPTION_RESERVED_BITS
SIM_AC_OPTION_BITSIZE(32,31)
SIM_AC_OPTION_INLINE()
SIM_AC_OPTION_HARDWARE(yes,,mn103cpu mn103int mn103tim mn103ser mn103iop)

AC_CHECK_FUNCS(time chmod utime fork execve execv chown)
AC_CHECK_HEADERS(unistd.h stdlib.h string.h strings.h utime.h time.h)

SIM_AC_OPTION_HARDWARE(always,"","")

if test -z "$SIM_DV_SOCKSER_O"; then
	AC_MSG_ERROR([Sorry, but hardware support in this simulator
unconditionally relies on dv-sockser.o which is unavailable for your host.
Please fix this simulator.])
fi

SIM_AC_OUTPUT
