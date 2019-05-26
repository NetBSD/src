dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

# For dv-rv and rvdummy.
AC_CHECK_HEADERS(sys/socket.h sys/select.h limits.h sys/param.h)

SIM_AC_OPTION_ENDIAN(LITTLE)
SIM_AC_OPTION_ALIGNMENT(NONSTRICT_ALIGNMENT)
SIM_AC_OPTION_SCACHE(16384)
SIM_AC_OPTION_WARNINGS
SIM_AC_OPTION_HARDWARE(yes,,rv cris cris_900000xx)

# The default model shouldn't matter as long as there's a BFD.
SIM_AC_OPTION_DEFAULT_MODEL(crisv32)
SIM_AC_OPTION_CGEN_MAINT

SIM_AC_OUTPUT
