dnl SPDX-License-Identifier: LGPL-2.1-only
dnl
dnl Copyright (C) 2021 EfficiOS, Inc.
dnl
dnl Process this file with autoconf to produce a configure script.

# Project version information
m4_define([urcu_version_major], [0])
m4_define([urcu_version_minor], [15])
m4_define([urcu_version_patch], [0])
m4_define([urcu_version_dev_stage], [])
m4_define([urcu_version], urcu_version_major[.]urcu_version_minor[.]urcu_version_patch[]urcu_version_dev_stage)

# Library version information of "liburcu"
# Following the numbering scheme proposed by libtool for the library version
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
m4_define([urcu_lib_version_current], [9])
m4_define([urcu_lib_version_revision], [0])
m4_define([urcu_lib_version_age], [1])
m4_define([urcu_lib_version], urcu_lib_version_current[:]urcu_lib_version_revision[:]urcu_lib_version_age)


##                     ##
## Autoconf base setup ##
##                     ##

AC_PREREQ([2.69])
AC_INIT([userspace-rcu],[urcu_version],[mathieu dot desnoyers at efficios dot com],[],[http://liburcu.org/])

AC_CONFIG_HEADERS([include/config.h include/urcu/config.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
AC_CANONICAL_HOST


##                     ##
## Automake base setup ##
##                     ##

AM_INIT_AUTOMAKE([1.12 foreign dist-bzip2 no-dist-gzip nostdinc -Wall -Wno-portability -Werror])
AM_MAINTAINER_MODE([enable])

# Enable silent rules by default
AM_SILENT_RULES([yes])


##                               ##
## OS and Arch specific defaults ##
##                               ##

AS_CASE([$host],
  [*-cygwin* | *-msys*], [LT_NO_UNDEFINED="-no-undefined"]
)


##                   ##
## C compiler checks ##
##                   ##

# Choose the C compiler
AC_PROG_CC
# AC_PROG_CC_STDC was merged in AC_PROG_CC in autoconf 2.70
m4_version_prereq([2.70], [], [AC_PROG_CC_STDC])

# Make sure the C compiler supports C99
AS_IF([test "$ac_cv_prog_cc_c99" = "no"], [AC_MSG_ERROR([The compiler does not support C99])])

# Enable available system extensions and LFS support
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

# Check if the selected C compiler supports atomic builtins
AE_CC_ATOMIC_BUILTINS


##                     ##
## C++ compiler checks ##
##                     ##

# Require a C++11 compiler without GNU extensions (-std=c++11)
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

# Make sure the C compiler supports __attribute__
AX_C___ATTRIBUTE__
AS_IF([test "x$ax_cv___attribute__" != "xyes"],
  [AC_MSG_ERROR([The compiler does not support __attribute__ extensions])])

# Make sure we have pthread support
AX_PTHREAD([], [AC_MSG_ERROR([Could not configure pthread support])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_TYPEOF
AC_TYPE_INT32_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Detect warning flags supported by the C compiler and append them to
# WARN_CFLAGS.
m4_define([WARN_FLAGS_LIST], [ dnl
  -Wall dnl
  -Wextra dnl
  -Wmissing-prototypes dnl
  -Wmissing-declarations dnl
  -Wnull-dereference dnl
  -Wundef dnl
  -Wshadow dnl
  -Wjump-misses-init dnl
  -Wsuggest-attribute=format dnl
  -Wtautological-constant-out-of-range-compare dnl
  -Wnested-externs dnl
  -Wwrite-strings dnl
  -Wformat=2 dnl
  -Wstrict-aliasing dnl
  -Wmissing-noreturn dnl
  -Winit-self dnl
  -Wduplicated-cond dnl
  -Wduplicated-branches dnl
  -Wlogical-op dnl
  dnl
  dnl-Wredundant-decls dnl
  -Wno-null-dereference dnl
])

# Pass -Werror as an extra flag during the test: this is needed to make the
# -Wunknown-warning-option diagnostic fatal with clang.
AC_LANG_PUSH([C])
AX_APPEND_COMPILE_FLAGS([WARN_FLAGS_LIST], [WARN_CFLAGS], [-Werror])
AC_LANG_POP([C])

AC_LANG_PUSH([C++])
AX_APPEND_COMPILE_FLAGS([WARN_FLAGS_LIST], [WARN_CXXFLAGS], [-Werror])
AC_LANG_POP([C++])

AE_IF_FEATURE_ENABLED([Werror], [WARN_CFLAGS="${WARN_CFLAGS} -Werror"])
AE_IF_FEATURE_ENABLED([Werror], [WARN_CXXFLAGS="${WARN_CXXFLAGS} -Werror"])


##               ##
## Header checks ##
##               ##

AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ \
	limits.h \
	stddef.h \
	sys/param.h \
	sys/time.h \
])


##                 ##
## Programs checks ##
##                 ##

AC_PROG_AWK
AC_PROG_GREP
AC_PROG_MAKE_SET
AC_CHECK_PROGS(NPROC, [nproc gnproc])
AC_CHECK_PROGS(GETCONF, [getconf])
AC_CHECK_PROGS(TIME, [time])

# Initialize and configure libtool
LT_INIT


##                ##
## Library checks ##
##                ##

# Checks for library functions.
AC_FUNC_MMAP
AC_FUNC_FORK
AC_CHECK_FUNCS([ \
	atexit \
	getcpuid \
	gettid \
	gettimeofday \
	memeset \
	memset \
	munmap \
	rand_r \
	sched_getcpu \
	sched_setaffinity \
	strerror \
	strtoul \
	sysconf \
])

# AC_FUNC_MALLOC causes problems when cross-compiling.
#AC_FUNC_MALLOC

# Search for clock_gettime() in -lrt
AC_SEARCH_LIBS([clock_gettime], [rt], [
  AC_DEFINE([CONFIG_RCU_HAVE_CLOCK_GETTIME], [1], [clock_gettime() is detected.])
])


##                             ##
## Optional features selection ##
##                             ##

# Allow to fallback to FIXME if the membarrier syscall is unavailable on the
# running kernel, when disabled, abort if the syscall is unavailable. Applies
# to default and bulletproof flavors.
# Enabled by default
AE_FEATURE_DEFAULT_ENABLE
AE_FEATURE([sys-membarrier-fallback], [Abort if sys-membarrier is needed but not available rather than using a fallback.])

# Use compiler Thread Local Storage, when disabled use pthread_getspecific() to emulate TLS.
# Enabled by default
AE_FEATURE_DEFAULT_ENABLE
AE_FEATURE([compiler-tls], [Use pthread_getspecific() to emulate Thread Local Storage (TLS) variables.])

# smp-support configure option
# Enabled by default
AE_FEATURE_DEFAULT_ENABLE
AE_FEATURE([smp-support], [Disable SMP support. Warning: only use this on uniprocessor systems.])

# RCU debugging option
# Disabled by default
AE_FEATURE_DEFAULT_DISABLE
AE_FEATURE([rcu-debug], [Enable internal debugging self-checks. Introduces a performance penalty.])

# rculfhash iterator debugging
# Disabled by default
AE_FEATURE_DEFAULT_DISABLE
AE_FEATURE([cds-lfht-iter-debug], [Enable extra debugging checks for lock-free hash table iterator traversal. Alters the rculfhash ABI. Make sure to compile both library and application with matching configuration.])

# Use compiler atomic builtins, when disabled use our legacy uatomic implementation.
# Disabled by default
AE_FEATURE_DEFAULT_DISABLE
AE_FEATURE([compiler-atomic-builtins], [Enable the use of compiler atomic builtins.])

# emit legacy memory barriers
# Enable by default
AE_FEATURE_DEFAULT_ENABLE
AE_FEATURE([legacy-mb], [Disable legacy memory barriers.])

# When given, add -Werror to WARN_CFLAGS and WARN_CXXFLAGS.
# Disabled by default
AE_FEATURE_DEFAULT_DISABLE
AE_FEATURE([Werror],[Treat compiler warnings as errors.])

##                                                                    ##
## Set defines for optional features conditionnals in the source code ##
##                                                                    ##

AE_IF_FEATURE_DISABLED([sys-membarrier-fallback], [
  AC_DEFINE([CONFIG_RCU_FORCE_SYS_MEMBARRIER], [1], [Require the operating system to support the membarrier system call for default and bulletproof flavors.])
])

AE_IF_FEATURE_ENABLED([compiler-tls], [
  AC_DEFINE([CONFIG_RCU_TLS], [1], [Use compiler provided Thread Local Storage.])
])

AE_IF_FEATURE_ENABLED([smp-support], [
  AC_DEFINE([CONFIG_RCU_SMP], [1], [Enable SMP support. With SMP support enabled, uniprocessors are also supported. With SMP support disabled, UP systems work fine, but the behavior of SMP systems is undefined.])
])

AE_IF_FEATURE_ENABLED([rcu-debug], [
  AC_DEFINE([CONFIG_RCU_DEBUG], [1], [Enable internal debugging self-checks. Introduces a performance penalty.])
])

AE_IF_FEATURE_ENABLED([cds-lfht-iter-debug], [
  AC_DEFINE([CONFIG_CDS_LFHT_ITER_DEBUG], [1], [Enable extra debugging checks for lock-free hash table iterator traversal. Alters the rculfhash ABI. Make sure to compile both library and application with matching configuration.])
])

AE_IF_FEATURE_ENABLED([compiler-atomic-builtins], [
  AC_DEFINE([CONFIG_RCU_USE_ATOMIC_BUILTINS], [1], [Use compiler atomic builtins.])
])

AE_IF_FEATURE_ENABLED([legacy-mb], [
  AC_DEFINE([CONFIG_RCU_EMIT_LEGACY_MB], [1], [Emit legacy memory barriers that were documented in the APIs.])
])

##                                                                          ##
## Set automake variables for optional feature conditionnals in Makefile.am ##
##                                                                          ##

# Building the examples requires the shared libraries to be enabled
AM_CONDITIONAL([ENABLE_EXAMPLES], AE_IS_FEATURE_ENABLED([shared]))


##                                          ##
## Check for optional features dependencies ##
##                                          ##


AE_IF_FEATURE_ENABLED([compiler-atomic-builtins], [
  AS_IF([test "x$ae_cv_cc_atomic_builtins" != xyes], [
     AC_MSG_ERROR([The compiler does not support atomic builtins.])
  ])
])

##                                             ##
## Substitute variables for use in Makefile.am ##
##                                             ##

# Library versions for libtool
AC_SUBST([URCU_LIBRARY_VERSION], [urcu_lib_version])

AC_SUBST(LT_NO_UNDEFINED)

# The order in which the include folders are searched is important.
# The top_builddir should always be searched first in the event that a build
# time generated file is included.
AM_CPPFLAGS="-I\$(top_builddir)/include -I\$(top_srcdir)/include -include config.h"
AC_SUBST(AM_CPPFLAGS)

AM_CFLAGS="$WARN_CFLAGS $PTHREAD_CFLAGS"
AC_SUBST(AM_CFLAGS)

AM_CXXFLAGS="$WARN_CXXFLAGS $PTHREAD_CFLAGS"
AC_SUBST(AM_CXXFLAGS)


##                                     ##
## Output files generated by configure ##
##                                     ##

AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	doc/examples/Makefile
	extras/Makefile
	include/Makefile
	src/Makefile
	tests/Makefile
	tests/common/Makefile
	tests/unit/Makefile
	tests/benchmark/Makefile
	tests/regression/Makefile
	tests/utils/Makefile
	src/liburcu.pc
	src/liburcu-bp.pc
	src/liburcu-cds.pc
	src/liburcu-qsbr.pc
	src/liburcu-mb.pc
	src/liburcu-memb.pc
])

AC_CONFIG_FILES([tests/utils/env.sh],[chmod +x tests/utils/env.sh])


AC_OUTPUT

#
# Mini-report on what will be built.
#

AE_PPRINT_INIT
AE_PPRINT_SET_INDENT(1)
AE_PPRINT_SET_TS(38)

AS_ECHO
AS_ECHO("${AE_PPRINT_COLOR_BLDBLU}Userspace-RCU $PACKAGE_VERSION${AE_PPRINT_COLOR_RST}")
AS_ECHO

AE_PPRINT_SUBTITLE([Features])

AE_PPRINT_PROP_STRING([Target architecture], $host_cpu)

# SMP support enabled/disabled
AE_IS_FEATURE_ENABLED([smp-support]) && value=1 || value=0
AE_PPRINT_PROP_BOOL([SMP support], $value)

# TLS
AE_IS_FEATURE_ENABLED([compiler-tls]) && value="compiler TLS" || value="pthread_getspecific()"
AE_PPRINT_PROP_STRING([Thread Local Storage (TLS)], [$value])

# clock_gettime() available
test "x$ac_cv_search_function_clock_gettime" != "xno" && value=1 || value=0
AE_PPRINT_PROP_BOOL([clock_gettime()], $value)

# Require membarrier
AE_IS_FEATURE_ENABLED([sys-membarrier-fallback]) && value=0 || value=1
AE_PPRINT_PROP_BOOL([Require membarrier], $value)

# RCU debug enabled/disabled
AE_IS_FEATURE_ENABLED([rcu-debug]) && value=1 || value=0
AE_PPRINT_PROP_BOOL([Internal debugging], $value)

# rculfhash iterator debug enabled/disabled
AE_IS_FEATURE_ENABLED([cds-lfht-iter-debug]) && value=1 || value=0
AE_PPRINT_PROP_BOOL([Lock-free HT iterator debugging], $value)

AE_PPRINT_PROP_BOOL([Multi-flavor support], 1)

# atomic builtins enabled/disabled
AE_IS_FEATURE_ENABLED([compiler-atomic-builtins]) && value=1 || value=0
AE_PPRINT_PROP_BOOL([Use compiler atomic builtins], $value)

# legacy memory barriers
AE_IS_FEATURE_ENABLED([legacy-mb]) && value=1 || value=0
AE_PPRINT_PROP_BOOL([Emit legacy memory barriers], $value)

report_bindir="`eval eval echo $bindir`"
report_libdir="`eval eval echo $libdir`"

# Print the bindir and libdir this 'make install' will install into.
AS_ECHO
AE_PPRINT_SUBTITLE([Install directories])
AE_PPRINT_PROP_STRING([Binaries], [$report_bindir])
AE_PPRINT_PROP_STRING([Libraries], [$report_libdir])

