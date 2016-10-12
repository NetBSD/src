# This file contains common code used by all simulators.
#
# SIM_AC_COMMON invokes AC macros used by all simulators and by the common
# directory.  It is intended to be invoked before any target specific stuff.
# SIM_AC_OUTPUT is a cover function to AC_OUTPUT to generate the Makefile.
# It is intended to be invoked last.
#
# The simulator's configure.ac should look like:
#
# dnl Process this file with autoconf to produce a configure script.
# AC_PREREQ(2.64)dnl
# AC_INIT(Makefile.in)
# sinclude(../common/aclocal.m4)
#
# SIM_AC_COMMON
#
# ... target specific stuff ...
#
# SIM_AC_OUTPUT

# Include global overrides and fixes for Autoconf.
m4_include(../../config/override.m4)
sinclude([../../config/zlib.m4])
m4_include([../../config/plugins.m4])
m4_include([../../libtool.m4])
m4_include([../../ltoptions.m4])
m4_include([../../ltsugar.m4])
m4_include([../../ltversion.m4])
m4_include([../../lt~obsolete.m4])
sinclude([../../config/depstand.m4])

AC_DEFUN([SIM_AC_COMMON],
[
AC_REQUIRE([AC_PROG_CC])
# autoconf.info says this should be called right after AC_INIT.
AC_CONFIG_HEADER(ifelse([$1],,config.h,[$1]):config.in)
AC_CANONICAL_SYSTEM
AC_USE_SYSTEM_EXTENSIONS
AC_C_BIGENDIAN
AC_ARG_PROGRAM
AC_PROG_INSTALL

# Put a plausible default for CC_FOR_BUILD in Makefile.
if test "x$cross_compiling" = "xno"; then
  CC_FOR_BUILD='$(CC)'
else
  CC_FOR_BUILD=gcc
fi
AC_SUBST(CC_FOR_BUILD)

AC_SUBST(CFLAGS)
AC_SUBST(HDEFINES)
AR=${AR-ar}
AC_SUBST(AR)
AC_PROG_RANLIB

# Some of the common include files depend on bfd.h, and bfd.h checks
# that config.h is included first by testing that the PACKAGE macro
# is defined.
PACKAGE=sim
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of this package. ])
AC_SUBST(PACKAGE)

# Dependency checking.
ZW_CREATE_DEPDIR
ZW_PROG_COMPILER_DEPENDENCIES([CC])

# Check for the 'make' the user wants to use.
AC_CHECK_PROGS(MAKE, make)
MAKE_IS_GNU=
case "`$MAKE --version 2>&1 | sed 1q`" in
  *GNU*)
    MAKE_IS_GNU=yes
    ;;
esac
AM_CONDITIONAL(GMAKE, test "$MAKE_IS_GNU" = yes)

dnl We don't use gettext, but bfd does.  So we do the appropriate checks
dnl to see if there are intl libraries we should link against.
ALL_LINGUAS=
ZW_GNU_GETTEXT_SISTER_DIR(../../intl)

# Check for common headers.
# FIXME: Seems to me this can cause problems for i386-windows hosts.
# At one point there were hardcoded AC_DEFINE's if ${host} = i386-*-windows*.
AC_CHECK_HEADERS(stdlib.h string.h strings.h unistd.h time.h)
AC_CHECK_HEADERS(sys/time.h sys/times.h sys/resource.h sys/mman.h)
AC_CHECK_HEADERS(fcntl.h fpu_control.h)
AC_CHECK_HEADERS(dlfcn.h errno.h sys/stat.h)
AC_CHECK_FUNCS(getrusage time sigaction __setfpucw)
AC_CHECK_FUNCS(mmap munmap lstat truncate ftruncate posix_fallocate)
AC_CHECK_MEMBERS([[struct stat.st_dev], [struct stat.st_ino],
[struct stat.st_mode], [struct stat.st_nlink], [struct stat.st_uid],
[struct stat.st_gid], [struct stat.st_rdev], [struct stat.st_size],
[struct stat.st_blksize], [struct stat.st_blocks], [struct stat.st_atime],
[struct stat.st_mtime], [struct stat.st_ctime]], [], [],
[[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif]])
AC_CHECK_TYPES(socklen_t, [], [],
[#include <sys/types.h>
#include <sys/socket.h>
])

# Check for socket libraries
AC_CHECK_LIB(socket, bind)
AC_CHECK_LIB(nsl, gethostbyname)

# BFD conditionally uses zlib, so we must link it in if libbfd does, by
# using the same condition.
AM_ZLIB

# BFD uses libdl when when plugins enabled.
AC_PLUGINS
AM_CONDITIONAL(PLUGINS, test "$plugins" = yes)
LT_INIT([dlopen])
AC_SUBST(lt_cv_dlopen_libs)

. ${srcdir}/../../bfd/configure.host

dnl Standard (and optional) simulator options.
dnl Eventually all simulators will support these.
dnl Do not add any here that cannot be supported by all simulators.
dnl Do not add similar but different options to a particular simulator,
dnl all shall eventually behave the same way.


dnl We don't use automake, but we still want to support
dnl --enable-maintainer-mode.
AM_MAINTAINER_MODE


dnl --enable-sim-debug is for developers of the simulator
dnl the allowable values are work-in-progress
AC_MSG_CHECKING([for sim debug setting])
sim_debug="0"
AC_ARG_ENABLE(sim-debug,
[AS_HELP_STRING([--enable-sim-debug=opts],
		[Enable debugging flags (for developers of the sim itself)])],
[case "${enableval}" in
  yes) sim_debug="7";;
  no)  sim_debug="0";;
  *)   sim_debug="($enableval)";;
esac])dnl
if test "$sim_debug" != "0"; then
  AC_DEFINE_UNQUOTED([DEBUG], [$sim_debug], [Sim debug setting])
fi
AC_DEFINE_UNQUOTED([WITH_DEBUG], [$sim_debug], [Sim debug setting])
AC_MSG_RESULT($sim_debug)


dnl --enable-sim-stdio is for users of the simulator
dnl It determines if IO from the program is routed through STDIO (buffered)
AC_MSG_CHECKING([for sim stdio debug behavior])
sim_stdio="0"
AC_ARG_ENABLE(sim-stdio,
[AS_HELP_STRING([--enable-sim-stdio],
		[Specify whether to use stdio for console input/output])],
[case "${enableval}" in
  yes)	sim_stdio="DO_USE_STDIO";;
  no)	sim_stdio="DONT_USE_STDIO";;
  *)	AC_MSG_ERROR([Unknown value $enableval passed to --enable-sim-stdio]);;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_STDIO], [$sim_stdio], [How to route I/O])
AC_MSG_RESULT($sim_stdio)


dnl --enable-sim-trace is for users of the simulator
dnl The argument is either a bitmask of things to enable [exactly what is
dnl up to the simulator], or is a comma separated list of names of tracing
dnl elements to enable.  The latter is only supported on simulators that
dnl use WITH_TRACE.  Default to all tracing but internal debug.
AC_MSG_CHECKING([for sim trace settings])
sim_trace="~TRACE_debug"
AC_ARG_ENABLE(sim-trace,
[AS_HELP_STRING([--enable-sim-trace=opts],
		[Enable tracing of simulated programs])],
[case "${enableval}" in
  yes)	sim_trace="-1";;
  no)	sim_trace="0";;
  [[-0-9]]*)
	sim_trace="'(${enableval})'";;
  [[[:lower:]]]*)
	sim_trace=""
	for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
	  if test x"$sim_trace" = x; then
	    sim_trace="(TRACE_$x"
	  else
	    sim_trace="${sim_trace}|TRACE_$x"
	  fi
	done
	sim_trace="$sim_trace)" ;;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_TRACE], [$sim_trace], [Sim trace settings])
AC_MSG_RESULT($sim_trace)


dnl --enable-sim-profile
dnl The argument is either a bitmask of things to enable [exactly what is
dnl up to the simulator], or is a comma separated list of names of profiling
dnl elements to enable.  The latter is only supported on simulators that
dnl use WITH_PROFILE.
AC_MSG_CHECKING([for sim profile settings])
profile="1"
sim_profile="-1"
AC_ARG_ENABLE(sim-profile,
[AS_HELP_STRING([--enable-sim-profile=opts], [Enable profiling flags])],
[case "${enableval}" in
  yes)	profile="1" sim_profile="-1";;
  no)	profile="0" sim_profile="0";;
  [[-0-9]]*)
	profile="(${enableval})" sim_profile="(${enableval})";;
  [[a-z]]*)
    profile="1"
	sim_profile=""
	for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
	  if test x"$sim_profile" = x; then
	    sim_profile="(PROFILE_$x"
	  else
	    sim_profile="${sim_profile}|PROFILE_$x"
	  fi
	done
	sim_profile="$sim_profile)" ;;
esac])dnl
AC_DEFINE_UNQUOTED([PROFILE], [$profile], [Sim profile settings])
AC_DEFINE_UNQUOTED([WITH_PROFILE], [$sim_profile], [Sim profile settings])
AC_MSG_RESULT($sim_profile)


SIM_AC_OPTION_ASSERT
SIM_AC_OPTION_ENVIRONMENT
SIM_AC_OPTION_INLINE

ACX_PKGVERSION([SIM])
ACX_BUGURL([http://www.gnu.org/software/gdb/bugs/])
AC_DEFINE_UNQUOTED([PKGVERSION], ["$PKGVERSION"], [Additional package description])
AC_DEFINE_UNQUOTED([REPORT_BUGS_TO], ["$REPORT_BUGS_TO"], [Bug reporting address])

dnl Types used by common code
AC_TYPE_SIGNAL

dnl Detect exe extension
AC_EXEEXT

]) dnl End of SIM_AC_COMMON


dnl Additional SIM options that can (optionally) be configured
dnl For optional simulator options, a macro SIM_AC_OPTION_* is defined.
dnl Simulators that wish to use the relevant option specify the macro
dnl in the simulator specific configure.ac file between the SIM_AC_COMMON
dnl and SIM_AC_OUTPUT lines.


dnl Specify the running environment.
dnl If the simulator invokes this in its configure.ac then without this option
dnl the default is the user environment and all are runtime selectable.
dnl If the simulator doesn't invoke this, only the user environment is
dnl supported.
dnl ??? Until there is demonstrable value in doing something more complicated,
dnl let's not.
AC_DEFUN([SIM_AC_OPTION_ENVIRONMENT],
[
AC_MSG_CHECKING([default sim environment setting])
sim_environment="ALL_ENVIRONMENT"
AC_ARG_ENABLE(sim-environment,
[AS_HELP_STRING([--enable-sim-environment=environment],
		[Specify mixed, user, virtual or operating environment])],
[case "${enableval}" in
  all | ALL)             sim_environment="ALL_ENVIRONMENT";;
  user | USER)           sim_environment="USER_ENVIRONMENT";;
  virtual | VIRTUAL)     sim_environment="VIRTUAL_ENVIRONMENT";;
  operating | OPERATING) sim_environment="OPERATING_ENVIRONMENT";;
  *)   AC_MSG_ERROR([Unknown value $enableval passed to --enable-sim-environment]);;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_ENVIRONMENT], [$sim_environment], [Sim default environment])
AC_MSG_RESULT($sim_environment)
])


dnl Specify the alignment restrictions of the target architecture.
dnl Without this option all possible alignment restrictions are accommodated.
dnl arg[1] is hardwired target alignment
dnl arg[2] is default target alignment
AC_DEFUN([SIM_AC_OPTION_ALIGNMENT],
wire_alignment="[$1]"
default_alignment="[$2]"
[
AC_ARG_ENABLE(sim-alignment,
[AS_HELP_STRING([--enable-sim-alignment=align],
		[Specify strict, nonstrict or forced alignment of memory accesses])],
[case "${enableval}" in
  strict | STRICT)       sim_alignment="-DWITH_ALIGNMENT=STRICT_ALIGNMENT";;
  nonstrict | NONSTRICT) sim_alignment="-DWITH_ALIGNMENT=NONSTRICT_ALIGNMENT";;
  forced | FORCED)       sim_alignment="-DWITH_ALIGNMENT=FORCED_ALIGNMENT";;
  yes) if test x"$wire_alignment" != x; then
	 sim_alignment="-DWITH_ALIGNMENT=${wire_alignment}"
       else
         if test x"$default_alignment" != x; then
           sim_alignment="-DWITH_ALIGNMENT=${default_alignment}"
         else
	   echo "No hard-wired alignment for target $target" 1>&6
	   sim_alignment="-DWITH_ALIGNMENT=0"
         fi
       fi;;
  no)  if test x"$default_alignment" != x; then
	 sim_alignment="-DWITH_DEFAULT_ALIGNMENT=${default_alignment}"
       else
         if test x"$wire_alignment" != x; then
	   sim_alignment="-DWITH_DEFAULT_ALIGNMENT=${wire_alignment}"
         else
           echo "No default alignment for target $target" 1>&6
           sim_alignment="-DWITH_DEFAULT_ALIGNMENT=0"
         fi
       fi;;
  *)   AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-alignment"); sim_alignment="";;
esac
if test x"$silent" != x"yes" && test x"$sim_alignment" != x""; then
  echo "Setting alignment flags = $sim_alignment" 6>&1
fi],
[if test x"$default_alignment" != x; then
  sim_alignment="-DWITH_DEFAULT_ALIGNMENT=${default_alignment}"
else
  if test x"$wire_alignment" != x; then
    sim_alignment="-DWITH_ALIGNMENT=${wire_alignment}"
  else
    sim_alignment=
  fi
fi])dnl
])dnl
AC_SUBST(sim_alignment)


dnl Conditionally compile in assertion statements.
AC_DEFUN([SIM_AC_OPTION_ASSERT],
[
AC_MSG_CHECKING([whether to enable sim asserts])
sim_assert="1"
AC_ARG_ENABLE(sim-assert,
[AS_HELP_STRING([--enable-sim-assert],
		[Specify whether to perform random assertions])],
[case "${enableval}" in
  yes)	sim_assert="1";;
  no)	sim_assert="0";;
  *)	AC_MSG_ERROR([--enable-sim-assert does not take a value]);;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_ASSERT], [$sim_assert], [Sim assert settings])
AC_MSG_RESULT($sim_assert)
])



dnl --enable-sim-bitsize is for developers of the simulator
dnl It specifies the number of BITS in the target.
dnl arg[1] is the number of bits in a word
dnl arg[2] is the number assigned to the most significant bit
dnl arg[3] is the number of bits in an address
dnl arg[4] is the number of bits in an OpenFirmware cell.
dnl FIXME: this information should be obtained from bfd/archure
AC_DEFUN([SIM_AC_OPTION_BITSIZE],
wire_word_bitsize="[$1]"
wire_word_msb="[$2]"
wire_address_bitsize="[$3]"
wire_cell_bitsize="[$4]"
[AC_ARG_ENABLE(sim-bitsize,
[AS_HELP_STRING([--enable-sim-bitsize=N], [Specify target bitsize (32 or 64)])],
[sim_bitsize=
case "${enableval}" in
  64,63 | 64,63,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=63";;
  32,31 | 32,31,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31";;
  64,0 | 64,0,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0";;
  32,0 | 64,0,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0";;
  32) if test x"$wire_word_msb" != x -a x"$wire_word_msb" != x0; then
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31"
      else
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0"
      fi ;;
  64) if test x"$wire_word_msb" != x -a x"$wire_word_msb" != x0; then
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=63"
      else
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=0"
      fi ;;
  *)  AC_MSG_ERROR("--enable-sim-bitsize was given $enableval.  Expected 32 or 64") ;;
esac
# address bitsize
tmp=`echo "${enableval}" | sed -e "s/^[[0-9]]*,*[[0-9]]*,*//"`
case x"${tmp}" in
  x ) ;;
  x32 | x32,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_ADDRESS_BITSIZE=32" ;;
  x64 | x64,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_ADDRESS_BITSIZE=64" ;;
  * ) AC_MSG_ERROR("--enable-sim-bitsize was given address size $enableval.  Expected 32 or 64") ;;
esac
# cell bitsize
tmp=`echo "${enableval}" | sed -e "s/^[[0-9]]*,*[[0-9*]]*,*[[0-9]]*,*//"`
case x"${tmp}" in
  x ) ;;
  x32 | x32,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_CELL_BITSIZE=32" ;;
  x64 | x64,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_CELL_BITSIZE=64" ;;
  * ) AC_MSG_ERROR("--enable-sim-bitsize was given cell size $enableval.  Expected 32 or 64") ;;
esac
if test x"$silent" != x"yes" && test x"$sim_bitsize" != x""; then
  echo "Setting bitsize flags = $sim_bitsize" 6>&1
fi],
[sim_bitsize=""
if test x"$wire_word_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_WORD_BITSIZE=$wire_word_bitsize"
fi
if test x"$wire_word_msb" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_WORD_MSB=$wire_word_msb"
fi
if test x"$wire_address_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_ADDRESS_BITSIZE=$wire_address_bitsize"
fi
if test x"$wire_cell_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_CELL_BITSIZE=$wire_cell_bitsize"
fi])dnl
])
AC_SUBST(sim_bitsize)



dnl --enable-sim-endian={yes,no,big,little} is for simulators
dnl that support both big and little endian targets.
dnl arg[1] is hardwired target endianness.
dnl arg[2] is default target endianness.
AC_DEFUN([SIM_AC_OPTION_ENDIAN],
[
wire_endian="[$1]"
default_endian="[$2]"
AC_ARG_ENABLE(sim-endian,
[AS_HELP_STRING([--enable-sim-endian=endian],
		[Specify target byte endian orientation])],
[case "${enableval}" in
  b*|B*) sim_endian="-DWITH_TARGET_BYTE_ORDER=BFD_ENDIAN_BIG";;
  l*|L*) sim_endian="-DWITH_TARGET_BYTE_ORDER=BFD_ENDIAN_LITTLE";;
  yes)	 if test x"$wire_endian" != x; then
	   sim_endian="-DWITH_TARGET_BYTE_ORDER=BFD_ENDIAN_${wire_endian}"
	 else
	  if test x"$default_endian" != x; then
	     sim_endian="-DWITH_TARGET_BYTE_ORDER=BFD_ENDIAN_${default_endian}"
	   else
	     echo "No hard-wired endian for target $target" 1>&6
	     sim_endian="-DWITH_TARGET_BYTE_ORDER=BFD_ENDIAN_UNKNOWN"
	   fi
	 fi;;
  no)	 if test x"$default_endian" != x; then
	   sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=BFD_ENDIAN_${default_endian}"
	 else
	   if test x"$wire_endian" != x; then
	     sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=BFD_ENDIAN_${wire_endian}"
	   else
	     echo "No default endian for target $target" 1>&6
	     sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=BFD_ENDIAN_UNKNOWN"
	   fi
	 fi;;
  *)	 AC_MSG_ERROR("Unknown value $enableval for --enable-sim-endian"); sim_endian="";;
esac
if test x"$silent" != x"yes" && test x"$sim_endian" != x""; then
  echo "Setting endian flags = $sim_endian" 6>&1
fi],
[if test x"$default_endian" != x; then
  sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=BFD_ENDIAN_${default_endian}"
else
  if test x"$wire_endian" != x; then
    sim_endian="-DWITH_TARGET_BYTE_ORDER=BFD_ENDIAN_${wire_endian}"
  else
    sim_endian=
  fi
fi])dnl
])
AC_SUBST(sim_endian)


dnl --enable-sim-float is for developers of the simulator
dnl It specifies the presence of hardware floating point
dnl And optionally the bitsize of the floating point register.
dnl arg[1] specifies the presence (or absence) of floating point hardware
dnl arg[2] specifies the number of bits in a floating point register
AC_DEFUN([SIM_AC_OPTION_FLOAT],
[
default_sim_float="[$1]"
default_sim_float_bitsize="[$2]"
AC_ARG_ENABLE(sim-float,
[AS_HELP_STRING([--enable-sim-float],
		[Specify that the target processor has floating point hardware])],
[case "${enableval}" in
  yes | hard)	sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT";;
  no | soft)	sim_float="-DWITH_FLOATING_POINT=SOFT_FLOATING_POINT";;
  32)           sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT -DWITH_TARGET_FLOATING_POINT_BITSIZE=32";;
  64)           sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT -DWITH_TARGET_FLOATING_POINT_BITSIZE=64";;
  *)		AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-float"); sim_float="";;
esac
if test x"$silent" != x"yes" && test x"$sim_float" != x""; then
  echo "Setting float flags = $sim_float" 6>&1
fi],[
sim_float=
if test x"${default_sim_float}" != x""; then
  sim_float="-DWITH_FLOATING_POINT=${default_sim_float}"
fi
if test x"${default_sim_float_bitsize}" != x""; then
  sim_float="$sim_float -DWITH_TARGET_FLOATING_POINT_BITSIZE=${default_sim_float_bitsize}"
fi
])dnl
])
AC_SUBST(sim_float)


dnl The argument is the default cache size if none is specified.
AC_DEFUN([SIM_AC_OPTION_SCACHE],
[
default_sim_scache="ifelse([$1],,0,[$1])"
AC_ARG_ENABLE(sim-scache,
[AS_HELP_STRING([--enable-sim-scache=size],
		[Specify simulator execution cache size])],
[case "${enableval}" in
  yes)	sim_scache="-DWITH_SCACHE=${default_sim_scache}";;
  no)	sim_scache="-DWITH_SCACHE=0" ;;
  [[0-9]]*) sim_scache="-DWITH_SCACHE=${enableval}";;
  *)	AC_MSG_ERROR("Bad value $enableval passed to --enable-sim-scache");
	sim_scache="";;
esac
if test x"$silent" != x"yes" && test x"$sim_scache" != x""; then
  echo "Setting scache size = $sim_scache" 6>&1
fi],[sim_scache="-DWITH_SCACHE=${default_sim_scache}"])
])
AC_SUBST(sim_scache)


dnl The argument is the default model if none is specified.
AC_DEFUN([SIM_AC_OPTION_DEFAULT_MODEL],
[
default_sim_default_model="ifelse([$1],,0,[$1])"
AC_ARG_ENABLE(sim-default-model,
[AS_HELP_STRING([--enable-sim-default-model=model],
		[Specify default model to simulate])],
[case "${enableval}" in
  yes|no) AC_MSG_ERROR("Missing argument to --enable-sim-default-model");;
  *)	sim_default_model="-DWITH_DEFAULT_MODEL='\"${enableval}\"'";;
esac
if test x"$silent" != x"yes" && test x"$sim_default_model" != x""; then
  echo "Setting default model = $sim_default_model" 6>&1
fi],[sim_default_model="-DWITH_DEFAULT_MODEL='\"${default_sim_default_model}\"'"])
])
AC_SUBST(sim_default_model)


dnl --enable-sim-hardware is for users of the simulator
dnl arg[1] Enable sim-hw by default? ("yes" or "no")
dnl arg[2] is a space separated list of devices that override the defaults
dnl arg[3] is a space separated list of extra target specific devices.
AC_DEFUN([SIM_AC_OPTION_HARDWARE],
[
if test "[$2]"; then
  hardware="[$2]"
else
  hardware="cfi core pal glue"
fi
hardware="$hardware [$3]"

sim_hw_cflags="-DWITH_HW=1"
sim_hw="$hardware"
sim_hw_objs="\$(SIM_COMMON_HW_OBJS) `echo $sim_hw | sed -e 's/\([[^ ]][[^ ]]*\)/dv-\1.o/g'`"

AC_ARG_ENABLE(sim-hardware,
  [AS_HELP_STRING([--enable-sim-hardware=LIST],
                  [Specify the hardware to be included in the build.])],
  ,[enable_sim_hardware="[$1]"])
case ${enable_sim_hardware} in
  yes|no) ;;
  ,*) hardware="${hardware} `echo ${enableval} | sed -e 's/,/ /'`";;
  *,) hardware="`echo ${enableval} | sed -e 's/,/ /'` ${hardware}";;
  *)  hardware="`echo ${enableval} | sed -e 's/,/ /'`"'';;
esac

if test "$enable_sim_hardware" = no; then
  sim_hw_objs=
  sim_hw_cflags="-DWITH_HW=0"
  sim_hw=
else
  sim_hw_cflags="-DWITH_HW=1"
  # remove duplicates
  sim_hw=""
  sim_hw_objs="\$(SIM_COMMON_HW_OBJS)"
  for i in $hardware ; do
    case " $sim_hw " in
      *" $i "*) ;;
      *) sim_hw="$sim_hw $i" ; sim_hw_objs="$sim_hw_objs dv-$i.o";;
    esac
  done
  # mingw does not support sockser
  case ${host} in
    *mingw*) ;;
    *) # TODO: We don't add dv-sockser to sim_hw as it is not a "real" device
       # that you instatiate.  Instead, other code will call into it directly.
       # At some point, we should convert it over.
       sim_hw_objs="$sim_hw_objs dv-sockser.o"
       AC_DEFINE_UNQUOTED(
         [HAVE_DV_SOCKSER], 1, [Define if dv-sockser is usable.])
       ;;
  esac
  if test x"$silent" != x"yes"; then
    echo "Setting hardware to $sim_hw_cflags, $sim_hw, $sim_hw_objs"
  fi
  dnl Some devices require extra libraries.
  case " $hardware " in
    *" cfi "*) AC_CHECK_LIB(m, log2);;
  esac
fi
])
AC_SUBST(sim_hw_cflags)
AC_SUBST(sim_hw_objs)
AC_SUBST(sim_hw)


dnl --enable-sim-inline is for users that wish to ramp up the simulator's
dnl performance by inlining functions.
dnl Default sims to no inlining.
AC_DEFUN([SIM_AC_OPTION_INLINE],
[
sim_inline="-DDEFAULT_INLINE=m4_ifblank([$1],[0],[$1])"
AC_ARG_ENABLE(sim-inline,
[AS_HELP_STRING([--enable-sim-inline=inlines],
		[Specify which functions should be inlined])],
[sim_inline=""
case "$enableval" in
  no)		sim_inline="-DDEFAULT_INLINE=0";;
  0)		sim_inline="-DDEFAULT_INLINE=0";;
  yes | 2)	sim_inline="-DDEFAULT_INLINE=ALL_C_INLINE";;
  1)		sim_inline="-DDEFAULT_INLINE=INLINE_LOCALS";;
  *) for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
       new_flag=""
       case "$x" in
	 *_INLINE=*)	new_flag="-D$x";;
	 *=*)		new_flag=`echo "$x" | sed -e "s/=/_INLINE=/" -e "s/^/-D/"`;;
	 *_INLINE)	new_flag="-D$x=ALL_C_INLINE";;
	 *)		new_flag="-D$x""_INLINE=ALL_C_INLINE";;
       esac
       if test x"$sim_inline" = x""; then
	 sim_inline="$new_flag"
       else
	 sim_inline="$sim_inline $new_flag"
       fi
     done;;
esac
if test x"$silent" != x"yes" && test x"$sim_inline" != x""; then
  echo "Setting inline flags = $sim_inline" 6>&1
fi])dnl
])
AC_SUBST(sim_inline)


AC_DEFUN([SIM_AC_OPTION_RESERVED_BITS],
[
default_sim_reserved_bits="ifelse([$1],,1,[$1])"
AC_ARG_ENABLE(sim-reserved-bits,
[AS_HELP_STRING([--enable-sim-reserved-bits],
		[Specify whether to check reserved bits in instruction])],
[case "${enableval}" in
  yes)	sim_reserved_bits="-DWITH_RESERVED_BITS=1";;
  no)	sim_reserved_bits="-DWITH_RESERVED_BITS=0";;
  *)	AC_MSG_ERROR("--enable-sim-reserved-bits does not take a value"); sim_reserved_bits="";;
esac
if test x"$silent" != x"yes" && test x"$sim_reserved_bits" != x""; then
  echo "Setting reserved flags = $sim_reserved_bits" 6>&1
fi],[sim_reserved_bits="-DWITH_RESERVED_BITS=${default_sim_reserved_bits}"])dnl
])
AC_SUBST(sim_reserved_bits)


AC_DEFUN([SIM_AC_OPTION_SMP],
[
AC_MSG_CHECKING([number of sim cpus to support])
default_sim_smp="ifelse([$1],,5,[$1])"
sim_smp="$default_sim_smp""
AC_ARG_ENABLE(sim-smp,
[AS_HELP_STRING([--enable-sim-smp=n],
		[Specify number of processors to configure for (default ${default_sim_smp})])],
[case "${enableval}" in
  yes)	sim_smp="5";;
  no)	sim_smp="0";;
  *)	sim_smp="$enableval";;
esac])dnl
sim_igen_smp="-N ${sim_smp}"
AC_DEFINE_UNQUOTED([WITH_SMP], [$sim_smp], [Sim SMP settings])
AC_MSG_RESULT($sim_smp)
])


AC_DEFUN([SIM_AC_OPTION_XOR_ENDIAN],
[
AC_MSG_CHECKING([for xor endian support])
default_sim_xor_endian="ifelse([$1],,8,[$1])"
sim_xor_endian="$default_sim_xor_endian"
AC_ARG_ENABLE(sim-xor-endian,
[AS_HELP_STRING([--enable-sim-xor-endian=n],
		[Specify number bytes involved in XOR bi-endian mode (default ${default_sim_xor_endian})])],
[case "${enableval}" in
  yes)	sim_xor_endian="8";;
  no)	sim_xor_endian="0";;
  *)	sim_xor_endian="$enableval";;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_XOR_ENDIAN], [$sim_xor_endian], [Sim XOR endian settings])
AC_MSG_RESULT($sim_smp)
])


dnl --enable-build-warnings is for developers of the simulator.
dnl it enables extra GCC specific warnings.
AC_DEFUN([SIM_AC_OPTION_WARNINGS],
[
AC_ARG_ENABLE(werror,
  AS_HELP_STRING([--enable-werror], [treat compile warnings as errors]),
  [case "${enableval}" in
     yes | y) ERROR_ON_WARNING="yes" ;;
     no | n)  ERROR_ON_WARNING="no" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-werror) ;;
   esac])

# Enable -Werror by default when using gcc
if test "${GCC}" = yes -a -z "${ERROR_ON_WARNING}" ; then
    ERROR_ON_WARNING=yes
fi

WERROR_CFLAGS=""
if test "${ERROR_ON_WARNING}" = yes ; then
# NOTE: Disabled in the sim dir due to most sims generating warnings.
#    WERROR_CFLAGS="-Werror"
     true
fi

build_warnings="-Wall -Wdeclaration-after-statement -Wpointer-arith \
-Wpointer-sign \
-Wno-unused -Wunused-value -Wunused-function \
-Wno-switch -Wno-char-subscripts -Wmissing-prototypes
-Wdeclaration-after-statement -Wempty-body -Wmissing-parameter-type \
-Wold-style-declaration -Wold-style-definition"

# Enable -Wno-format by default when using gcc on mingw since many
# GCC versions complain about %I64.
case "${host}" in
  *-*-mingw32*) build_warnings="$build_warnings -Wno-format" ;;
  *) build_warnings="$build_warnings -Wformat-nonliteral" ;;
esac

AC_ARG_ENABLE(build-warnings,
AS_HELP_STRING([--enable-build-warnings], [enable build-time compiler warnings if gcc is used]),
[case "${enableval}" in
  yes)	;;
  no)	build_warnings="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${build_warnings} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${t} ${build_warnings}";;
  *)    build_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$build_warnings" != x""; then
  echo "Setting compiler warning flags = $build_warnings" 6>&1
fi])dnl
AC_ARG_ENABLE(sim-build-warnings,
AS_HELP_STRING([--enable-sim-build-warnings], [enable SIM specific build-time compiler warnings if gcc is used]),
[case "${enableval}" in
  yes)	;;
  no)	build_warnings="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${build_warnings} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${t} ${build_warnings}";;
  *)    build_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$build_warnings" != x""; then
  echo "Setting GDB specific compiler warning flags = $build_warnings" 6>&1
fi])dnl
WARN_CFLAGS=""
if test "x${build_warnings}" != x -a "x$GCC" = xyes
then
    AC_MSG_CHECKING(compiler warning flags)
    # Separate out the -Werror flag as some files just cannot be
    # compiled with it enabled.
    for w in ${build_warnings}; do
	case $w in
	-Werr*) WERROR_CFLAGS=-Werror ;;
	*) # Check that GCC accepts it
	    saved_CFLAGS="$CFLAGS"
	    CFLAGS="$CFLAGS $w"
	    AC_TRY_COMPILE([],[],WARN_CFLAGS="${WARN_CFLAGS} $w",)
	    CFLAGS="$saved_CFLAGS"
	esac
    done
    AC_MSG_RESULT(${WARN_CFLAGS} ${WERROR_CFLAGS})
fi
])
AC_SUBST(WARN_CFLAGS)
AC_SUBST(WERROR_CFLAGS)


dnl Generate the Makefile in a target specific directory.
dnl Substitutions aren't performed on the file in AC_SUBST_FILE,
dnl so this is a cover macro to tuck the details away of how we cope.
dnl We cope by having autoconf generate two files and then merge them into
dnl one afterwards.  The two pieces of the common fragment are inserted into
dnl the target's fragment at the appropriate points.

AC_DEFUN([SIM_AC_OUTPUT],
[
dnl Make @cgen_breaks@ non-null only if the sim uses CGEN.
cgen_breaks=""
if grep CGEN_MAINT $srcdir/Makefile.in >/dev/null; then
cgen_breaks="break cgen_rtx_error";
fi
AC_SUBST(cgen_breaks)
AC_CONFIG_FILES(Makefile.sim:Makefile.in)
AC_CONFIG_FILES(Make-common.sim:../common/Make-common.in)
AC_CONFIG_FILES(.gdbinit:../common/gdbinit.in)
AC_CONFIG_COMMANDS([Makefile],
[echo "Merging Makefile.sim+Make-common.sim into Makefile ..."
 rm -f Makesim1.tmp Makesim2.tmp Makefile
 sed -n -e '/^## COMMON_PRE_/,/^## End COMMON_PRE_/ p' <Make-common.sim >Makesim1.tmp
 sed -n -e '/^## COMMON_POST_/,/^## End COMMON_POST_/ p' <Make-common.sim >Makesim2.tmp
 sed -e '/^## COMMON_PRE_/ r Makesim1.tmp' \
	-e '/^## COMMON_POST_/ r Makesim2.tmp' \
	<Makefile.sim >Makefile
 rm -f Makefile.sim Make-common.sim Makesim1.tmp Makesim2.tmp
])
AC_CONFIG_COMMANDS([stamp-h], [echo > stamp-h])
AC_OUTPUT
])

sinclude(../../config/gettext-sister.m4)
sinclude(../../config/acx.m4)

dnl --enable-cgen-maint support
AC_DEFUN([SIM_AC_OPTION_CGEN_MAINT],
[
cgen_maint=no
dnl Default is to use one in build tree.
cgen=guile
cgendir='$(srcdir)/../../cgen'
dnl Having --enable-maintainer-mode take arguments is another way to go.
dnl ??? One can argue --with is more appropriate if one wants to specify
dnl a directory name, but what we're doing here is an enable/disable kind
dnl of thing and specifying both --enable and --with is klunky.
dnl If you reeely want this to be --with, go ahead and change it.
AC_ARG_ENABLE(cgen-maint,
[AS_HELP_STRING([--enable-cgen-maint[=DIR]], [build cgen generated files])],
[case "${enableval}" in
  yes)	cgen_maint=yes ;;
  no)	cgen_maint=no ;;
  *)
	# argument is cgen install directory (not implemented yet).
	# Having a `share' directory might be more appropriate for the .scm,
	# .cpu, etc. files.
	cgendir=${cgen_maint}/lib/cgen
	cgen=guile
	;;
esac])dnl
dnl AM_CONDITIONAL(CGEN_MAINT, test x${cgen_maint} != xno)
if test x${cgen_maint} != xno ; then
  CGEN_MAINT=''
else
  CGEN_MAINT='#'
fi
AC_SUBST(CGEN_MAINT)
AC_SUBST(cgendir)
AC_SUBST(cgen)
])
