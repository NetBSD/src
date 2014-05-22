dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN(BIG_ENDIAN)
SIM_AC_OPTION_ALIGNMENT(STRICT_ALIGNMENT)
SIM_AC_OPTION_HOSTENDIAN
SIM_AC_OPTION_SCACHE(16384)
SIM_AC_OPTION_DEFAULT_MODEL(m32r/d)
SIM_AC_OPTION_ENVIRONMENT
SIM_AC_OPTION_INLINE()
SIM_AC_OPTION_CGEN_MAINT

  case "${target_alias}" in
  m32r*-linux*)
    traps_obj=traps-linux.o
    sim_extra_cflags="-DM32R_LINUX"
    ;;
  *)
    traps_obj=traps.o
    sim_extra_cflags="-DM32R_ELF"
    ;;
  esac
AC_SUBST(traps_obj)
AC_SUBST(sim_extra_cflags)

SIM_AC_OPTION_HARDWARE(yes,"","")
m32r_extra_objs="$SIM_DV_SOCKSER_O"
AC_SUBST(m32r_extra_objs)

SIM_AC_OUTPUT
