dnl Process this file with autoconf to produce a configure script.
dnl NB: The version here is not used.  If gdb ever changes from generating its
dnl version at build time to autoconf time (like bfd et al do), we can switch.
AC_INIT([sim], [0],
  [https://sourceware.org/bugzilla/enter_bug.cgi?product=gdb&component=sim],
  [], [https://sourceware.org/gdb/wiki/Sim/])

dnl Probably should unify PKGVERSION with PACKAGE_* settings from AC_INIT.
ACX_PKGVERSION([SIM])
AC_DEFINE_UNQUOTED([PKGVERSION], ["$PKGVERSION"], [Additional package description])
dnl PACKAGE_BUGREPORT is provided by AC_INIT.
ACX_BUGURL([$PACKAGE_BUGREPORT])
AC_DEFINE_UNQUOTED([REPORT_BUGS_TO], ["$REPORT_BUGS_TO"], [Bug reporting address])

AC_CONFIG_HEADERS([config.h])

SIM_AC_TOOLCHAIN
SIM_AC_PLATFORM

AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

# If a cpu ever has more than one simulator to choose from, use
# --enable-sim=... to choose.
AC_ARG_ENABLE(sim,
[AS_HELP_STRING([--enable-sim], [Enable the GNU simulator])],
[case "${enableval}" in
yes | no) ;;
*)	AC_MSG_ERROR(bad value ${enableval} given for --enable-sim option) ;;
esac])

AC_ARG_ENABLE([example-sims],
	      [AC_HELP_STRING([--enable-example-sims],
			      [enable example GNU simulators])])

AC_ARG_ENABLE(targets,
[  --enable-targets        alternative target configurations],
[case "${enableval}" in
  yes | "") AC_MSG_ERROR(enable-targets option must specify target names or 'all')
	    ;;
  no)       enable_targets= ;;
  *)        enable_targets=$enableval ;;
esac])

dnl Used to keep track of which target (if any) is the default one.  This is
dnl used when installing files to see if they need to be suffixed.
SIM_PRIMARY_TARGET=
AC_SUBST(SIM_PRIMARY_TARGET)

dnl Directories that we need to recurse into (i.e. add to $SUBDIRS).
SIM_SUBDIRS=
AC_SUBST(SIM_SUBDIRS)

dnl List of enabled arch backends.
SIM_ENABLED_ARCHES=
AC_SUBST(SIM_ENABLED_ARCHES)

dnl Used by common/Make-common.in to see which configure script created it.
SIM_COMMON_BUILD_TRUE=
SIM_COMMON_BUILD_FALSE='#'
AC_SUBST(SIM_COMMON_BUILD_TRUE)
AC_SUBST(SIM_COMMON_BUILD_FALSE)

AC_CONFIG_FILES(Make-common.sim:common/Make-common.in)
dnl Build a particular arch subdir.
dnl arg[1] is the arch subdir name.
dnl arg[2] is whether the arch has a dedicated configure script.
m4_define([SIM_BUILD_TARGET], [dnl
  AS_VAR_APPEND([SIM_ENABLED_ARCHES], [" $1"])
  m4_if($2, [true], [dnl
    AC_CONFIG_SUBDIRS($1)
  ], [dnl
    AC_CONFIG_FILES($1/Makefile.sim:$1/Makefile.in)
    AC_CONFIG_COMMANDS([$1/Makefile],
[sed -n \
	-e '/^## COMMON_PRE_/,/^## End COMMON_PRE_/ {
		/^srcdir = / s:$:/$1:
		p
	}' \
	<Make-common.sim >$1/Makesim1.tmp
 sed -n -e '/^## COMMON_POST_/,/^## End COMMON_POST_/ p' <Make-common.sim >$1/Makesim2.tmp
 sed -e '/^## COMMON_PRE_/ r $1/Makesim1.tmp' \
	-e '/^## COMMON_POST_/ r $1/Makesim2.tmp' \
	<$1/Makefile.sim >$1/Makefile
 rm -f $1/Makesim1.tmp $1/Makesim2.tmp
])
    AS_VAR_APPEND([SIM_SUBDIRS], [" $1"])
  ])
  AC_CONFIG_FILES($1/.gdbinit:common/gdbinit.in)
  dnl Create the depdirs for ports until we can convert them to automake.
  AC_CONFIG_COMMANDS([depdir-$1],
		     [$SHELL $ac_aux_dir/mkinstalldirs $1/$DEPDIR],
		     [ac_aux_dir=$ac_aux_dir DEPDIR=$DEPDIR])
])
dnl Enable a particular arch subdir.
dnl arg[1] is the matching target triple.
dnl arg[2] is the arch subdir name.
dnl arg[3] is whether the arch has a dedicated configure script.
dnl arg[4] is any additional shell code to run for this arch.
m4_define([SIM_TARGET], [dnl
  sim_enable_arch_$2=false
  case "${targ}" in
    all|$1)
      if test "${targ}" = "${target}"; then
        SIM_PRIMARY_TARGET=$2
      fi
      SIM_BUILD_TARGET($2, $3)
      sim_enable_arch_$2=true
      $4
      ;;
  esac
  SIM_AC_TOOLCHAIN_FOR_TARGET($2)
  AM_CONDITIONAL([SIM_ENABLE_ARCH_$2], [${sim_enable_arch_$2}])
])

dnl WHEN ADDING ENTRIES TO THIS MATRIX:
dnl Make sure that the left side always has two dashes.  Otherwise you can get
dnl spurious matches.  Even for unambiguous cases, do this as a convention, else
dnl the table becomes a real mess to understand and maintain.
dnl
dnl NB: Target matching is aligned with gdb/configure.tgt.  Changes must be kept
dnl in sync with that file.
if test "${enable_sim}" != no; then
  sim_igen=no
  for targ in `echo $target $enable_targets | sed 's/,/ /g'`
  do
    SIM_TARGET([aarch64*-*-*], [aarch64])
    SIM_TARGET([arm*-*-*], [arm])
    SIM_TARGET([avr*-*-*], [avr])
    SIM_TARGET([bfin-*-*], [bfin])
    SIM_TARGET([bpf-*-*], [bpf])
    SIM_TARGET([cr16*-*-*], [cr16])
    SIM_TARGET([cris-*-* | crisv32-*-*], [cris])
    SIM_TARGET([d10v-*-*], [d10v])
    SIM_TARGET([frv-*-*], [frv])
    SIM_TARGET([ft32-*-*], [ft32])
    SIM_TARGET([h8300*-*-*], [h8300])
    SIM_TARGET([iq2000-*-*], [iq2000])
    SIM_TARGET([lm32-*-*], [lm32])
    SIM_TARGET([m32c-*-*], [m32c])
    SIM_TARGET([m32r-*-*], [m32r])
    SIM_TARGET([m68hc11-*-*|m6811-*-*], [m68hc11])
    SIM_TARGET([mcore-*-*], [mcore])
    SIM_TARGET([microblaze*-*-*], [microblaze])
    SIM_TARGET([mips*-*-*], [mips], [true], [sim_igen=yes])
    SIM_TARGET([mn10300*-*-*], [mn10300], [], [sim_igen=yes])
    SIM_TARGET([moxie-*-*], [moxie])
    SIM_TARGET([msp430*-*-*], [msp430])
    SIM_TARGET([or1k*-*-*], [or1k])
    SIM_TARGET([powerpc*-*-*], [ppc], [true])
    SIM_TARGET([pru*-*-*], [pru])
    SIM_TARGET([riscv*-*-*], [riscv])
    SIM_TARGET([rl78-*-*], [rl78])
    SIM_TARGET([rx-*-*], [rx])
    SIM_TARGET([sh*-*-*], [sh])
    SIM_TARGET([sparc-*-*], [erc32])
    SIM_TARGET([v850*-*-*], [v850], [], [sim_igen=yes])
  done

  if test "x${enable_example_sims}" = xyes; then
    SIM_AC_TOOLCHAIN_FOR_TARGET(example-synacor)
    SIM_BUILD_TARGET([example-synacor])
  fi
  AM_CONDITIONAL([SIM_ENABLE_ARCH_examples], [test "${enable_example_sims}" = "yes"])
fi
AM_CONDITIONAL([SIM_ENABLE_IGEN], [test "$sim_igen" = "yes"])
AM_CONDITIONAL([ENABLE_SIM], [test -n "$SIM_SUBDIRS"])

dnl Standard (and optional) simulator options.
dnl Eventually all simulators will support these.
SIM_AC_OPTION_ALIGNMENT
SIM_AC_OPTION_ASSERT
SIM_AC_OPTION_CGEN_MAINT
SIM_AC_OPTION_DEBUG
SIM_AC_OPTION_ENDIAN
SIM_AC_OPTION_ENVIRONMENT
SIM_AC_OPTION_HARDWARE
SIM_AC_OPTION_INLINE
SIM_AC_OPTION_PROFILE
SIM_AC_OPTION_RESERVED_BITS
SIM_AC_OPTION_SCACHE
SIM_AC_OPTION_STDIO
SIM_AC_OPTION_TRACE
SIM_AC_OPTION_WARNINGS

AC_MSG_CHECKING(for a known getopt prototype in unistd.h)
AC_CACHE_VAL(sim_cv_decl_getopt_unistd_h,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <unistd.h>], [extern int getopt (int, char *const*, const char *);])],
sim_cv_decl_getopt_unistd_h=yes, sim_cv_decl_getopt_unistd_h=no)])
AC_MSG_RESULT($sim_cv_decl_getopt_unistd_h)
if test $sim_cv_decl_getopt_unistd_h = yes; then
  AC_DEFINE([HAVE_DECL_GETOPT], 1,
	    [Is the prototype for getopt in <unistd.h> in the expected format?])
fi

dnl These are unfortunate.  They are conditionally called by other sim macros
dnl but always used by common/Make-common.in.  So we have to subst here even
dnl when the rest of the code is in the respective macros.  Once we merge the
dnl respective SIM_AC_OPTION_xxx call above, we can drop these.
AC_SUBST(sim_bitsize)
AC_SUBST(sim_float)

dnl Some arches have unique configure flags.
m4_include([frv/acinclude.m4])
m4_include([riscv/acinclude.m4])
m4_include([rx/acinclude.m4])

dnl Hack to output an "include" statement in the Makefile so automake doesn't
dnl try to follow it itself while processing Makefile.am.
include_makefile="include"
AC_SUBST(include_makefile)

AC_CONFIG_FILES([arch-subdir.mk Makefile .gdbinit:gdbinit.in])
AC_OUTPUT
