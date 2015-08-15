dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

dnl Options available in this module
SIM_AC_OPTION_INLINE()
SIM_AC_OPTION_ALIGNMENT(NONSTRICT_ALIGNMENT)
SIM_AC_OPTION_HOSTENDIAN
SIM_AC_OPTION_WARNINGS

#
# Add simulated hardware devices
#
hw_enabled=no
case "${target}" in
  m68hc11-*-*|m6811-*-*)
	hw_enabled=yes
	hw_extra_devices="m68hc11 m68hc11sio m68hc11eepr m68hc11tim m68hc11spi nvram"
	SIM_SUBTARGET="$SIM_SUBTARGET -DTARGET_M68HC11=1"
	;;
  *)
	m68hc11_extra_objs="" 
	;;
esac
SIM_AC_OPTION_HARDWARE($hw_enabled,$hw_devices,$hw_extra_devices)

AC_CHECK_HEADERS(string.h strings.h stdlib.h stdlib.h fcntl.h)

m68hc11_extra_objs="$SIM_DV_SOCKSER_O"
AC_SUBST(m68hc11_extra_objs)

SIM_AC_OUTPUT
