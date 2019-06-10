dnl Process this file with autoconf to produce a configure script.
m4_include([../config/override.m4])
AC_INIT(Makefile.in)

AC_PROG_CC
AC_PROG_INSTALL
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib, :)

AC_CANONICAL_SYSTEM
AC_ARG_PROGRAM
AC_PROG_CC
AC_SUBST(CFLAGS)
AC_SUBST(HDEFINES)
AR=${AR-ar}
AC_SUBST(AR)
AC_PROG_RANLIB

# Put a plausible default for CC_FOR_BUILD in Makefile.
if test -z "$CC_FOR_BUILD"; then
  if test "x$cross_compiling" = "xno"; then
    CC_FOR_BUILD='$(CC)'
  else
    CC_FOR_BUILD=gcc
  fi
fi
AC_SUBST(CC_FOR_BUILD)
CFLAGS_FOR_BUILD=${CFLAGS_FOR_BUILD-${CFLAGS}}
AC_SUBST(CFLAGS_FOR_BUILD)

# If a cpu ever has more than one simulator to choose from, use
# --enable-sim=... to choose.
AC_ARG_ENABLE(sim,
[AS_HELP_STRING([--enable-sim], [Enable the GNU simulator])],
[case "${enableval}" in
yes | no) ;;
*)	AC_MSG_ERROR(bad value ${enableval} given for --enable-sim option) ;;
esac])

m4_define([SIM_ARCH], [
  sim_arch=$1
  AC_CONFIG_SUBDIRS($1)
])
if test "${enable_sim}" != no; then
   sinclude(configure.tgt)
   AC_CONFIG_SUBDIRS(testsuite)
   if test "$sim_igen" = yes; then
      AC_CONFIG_SUBDIRS(igen)
   fi
fi

AC_OUTPUT(Makefile)

exit 0
