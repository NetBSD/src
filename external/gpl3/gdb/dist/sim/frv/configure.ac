dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN(BIG)
SIM_AC_OPTION_ALIGNMENT(STRICT_ALIGNMENT)
SIM_AC_OPTION_SCACHE(16384)
SIM_AC_OPTION_DEFAULT_MODEL(fr500)
SIM_AC_OPTION_CGEN_MAINT

#
# Enable making unknown traps dump out registers
#
AC_ARG_ENABLE(sim-trapdump,
[AS_HELP_STRING([--enable-sim-trapdump],
		[Make unknown traps dump the registers])],
[case "${enableval}" in
  yes) sim_trapdump="-DTRAPDUMP=1";;
  no)  sim_trapdump="-DTRAPDUMP=0";;
  *)   AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-trapdump"); sim_trapdump="";;
esac
if test x"$silent" != x"yes" && test x"$sim_trapdump" != x""; then
  echo "Setting sim_trapdump = $sim_trapdump" 6>&1
fi],[sim_trapdump=""])dnl
AC_SUBST(sim_trapdump)

SIM_AC_OPTION_HARDWARE(yes,"","")

SIM_AC_OUTPUT
