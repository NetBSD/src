dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

dnl Options available in this module
SIM_AC_OPTION_ENDIAN(BIG)
SIM_AC_OPTION_ALIGNMENT(NONSTRICT_ALIGNMENT)
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
esac
SIM_AC_OPTION_HARDWARE($hw_enabled,$hw_devices,$hw_extra_devices)

SIM_AC_OUTPUT
