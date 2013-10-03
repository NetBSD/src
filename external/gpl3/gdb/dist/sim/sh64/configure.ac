dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN([], BIG_ENDIAN)
SIM_AC_OPTION_ALIGNMENT(STRICT_ALIGNMENT)
SIM_AC_OPTION_HOSTENDIAN
SIM_AC_OPTION_SCACHE(16384)
SIM_AC_OPTION_DEFAULT_MODEL(sh5)
SIM_AC_OPTION_ENVIRONMENT
SIM_AC_OPTION_INLINE()
SIM_AC_OPTION_CGEN_MAINT

SIM_AC_OPTION_HARDWARE(always,"","")

if test -z "$SIM_DV_SOCKSER_O"; then
	AC_MSG_ERROR([Sorry, but hardware support in this simulator
unconditionally relies on dv-sockser.o which is unavailable for your host.
Please fix this simulator.])
fi

SIM_AC_OUTPUT

