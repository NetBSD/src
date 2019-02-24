# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

AC_INIT(BIND, [9.13], [info@isc.org], [], [https://www.isc.org/downloads/BIND/])
AC_PREREQ([2.60])

AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_USE_SYSTEM_EXTENSIONS

# Enable RFC 3542 APIs on macOS
AC_DEFINE([__APPLE_USE_RFC_3542], [1], [Select RFC3542 IPv6 API on macOS])

AC_PROG_MAKE_SET

AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AX_POSIX_SHELL

AC_SUBST(STD_CINCLUDES)
AC_SUBST(STD_CDEFINES)
AC_SUBST(STD_CWARNINGS)
AC_SUBST(CCOPT)
AC_SUBST(CCNOOPT)
AC_SUBST(BACKTRACECFLAGS)

#
# Use pkg-config
#

PKG_PROG_PKG_CONFIG

# Warn if the user specified libbind, which is now deprecated
AC_ARG_ENABLE(libbind, AS_HELP_STRING([--enable-libbind], [deprecated]))

case "$enable_libbind" in
	yes)
		AC_MSG_ERROR(['libbind' is no longer part of the BIND 9 distribution.
It is available from http://www.isc.org as a separate download.])
		;;
	no|'')
		;;
esac

AC_ARG_ENABLE(buffer_useinline,
	      AS_HELP_STRING([--enable-buffer-useinline],
		             [define ISC_BUFFER_USEINLINE when compiling
				[default=yes]]),
	      if test yes = "${enable}"
	      then
		      AC_DEFINE([ISC_BUFFER_USEINLINE], [1],
			        [Define if you want to use inline buffers])
	      fi,
	      AC_DEFINE([ISC_BUFFER_USEINLINE], [1]))

AC_ARG_ENABLE(warn_shadow,
	      AS_HELP_STRING([--enable-warn-shadow],
			     [turn on -Wshadow when compiling]))

AC_ARG_ENABLE(warn_error,
	      AS_HELP_STRING([--enable-warn-error],
			    [turn on -Werror when compiling]))

AC_ARG_ENABLE(developer,
	      AS_HELP_STRING([--enable-developer],
			     [enable developer build settings]))
XTARGETS=
case "$enable_developer" in
yes)
	STD_CDEFINES="$STD_CDEFINES -DISC_MEM_DEFAULTFILL=1 -DISC_LIST_CHECKINIT=1"
	test "${enable_fixed_rrset+set}" = set || enable_fixed_rrset=yes
	test "${enable_querytrace+set}" = set || enable_querytrace=yes
	test "${with_cmocka+set}" = set || with_cmocka=yes
	test "${with_dlz_filesystem+set}" = set || with_dlz_filesystem=yes
	test "${enable_symtable+set}" = set || enable_symtable=all
	test "${enable_warn_error+set}" = set || enable_warn_error=yes
	test "${enable_warn_shadow+set}" = set || enable_warn_shadow=yes
	test "${with_zlib+set}" = set || with_zlib=yes
	XTARGETS='${XTARGETS}'
	;;
esac
AC_SUBST(XTARGETS)

AC_ARG_ENABLE([fuzzing],
	      [AS_HELP_STRING([--enable-fuzzing=<afl|libfuzzer>],
			      [Enable fuzzing using American Fuzzy Lop or libFuzzer (default=no)])],
	      [],
	      [enable_fuzzing=no])

AC_MSG_CHECKING([whether to enable fuzzing mode])
AS_CASE([$enable_fuzzing],
	[no],[AC_MSG_RESULT([no])],
	[afl],[
	  AC_MSG_RESULT([using AFL])
	  AC_DEFINE([ENABLE_AFL], [1],
		    [Define to enable American Fuzzy Lop test harness])
	  CFLAGS="$CFLAGS -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1"
	  LIBS="$LIBS -lpthread"],
	[libfuzzer],[
	  AC_MSG_RESULT([using libFuzzer])
	  CFLAGS="$CFLAGS -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1 -fsanitize=fuzzer,address,undefined"
	  LDFLAGS="$LDFLAGS -fsanitize=fuzzer,address,undefined"],
	[*],[AC_MSG_ERROR([You need to explicitly select the fuzzer])])

AS_IF([test "$enable_fuzzing" = "afl"],
      [AC_MSG_CHECKING("for AFL enabled compiler")
       AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
					  [#ifndef __AFL_COMPILER
					   #error AFL compiler required
					   #endif
					  ])],
			 [AC_MSG_RESULT([yes])],
			 [AC_MSG_ERROR([set CC=afl-<gcc|clang> when --enable-fuzzing=afl is used])])
      ])

#
# Make very sure that these are the first files processed by
# config.status, since we use the processed output as the input for
# AC_SUBST_FILE() substitutions in other files.
#
AC_CONFIG_FILES([make/rules make/includes])

AC_PATH_PROG(AR, ar)
ARFLAGS="cruv"
AC_SUBST(AR)
AC_SUBST(ARFLAGS)

# The POSIX ln(1) program.  Non-POSIX systems may substitute
# "copy" or something.
LN=ln
AC_SUBST(LN)

case "$AR" in
	"")
		AC_MSG_ERROR([
ar program not found.  Please fix your PATH to include the directory in
which ar resides, or set AR in the environment with the full path to ar.
])

		;;
esac

#
# Etags.
#
AC_PATH_PROGS(ETAGS, etags emacs-etags)

#
# Some systems, e.g. RH7, have the Exuberant Ctags etags instead of
# GNU emacs etags, and it requires the -L flag.
#
if test "X$ETAGS" != "X"; then
	AC_MSG_CHECKING(for Exuberant Ctags etags)
	if $ETAGS --version 2>&1 | grep 'Exuberant Ctags' >/dev/null 2>&1; then
		AC_MSG_RESULT(yes)
		ETAGS="$ETAGS -L"
	else
		AC_MSG_RESULT(no)
	fi
fi
AC_SUBST(ETAGS)

#
# Perl is optional; it is used only by some of the system test scripts.
# Note: the backtrace feature (see below) uses perl to build the symbol table,
# but it still compiles without perl, in which case an empty table will be used.
#
AC_PATH_PROGS(PERL, perl5 perl)
AC_SUBST(PERL)

#
# Python is also optional but required by default so that dnssec-keymgr gets
# installed unless explicitly prevented by the user using --without-python.
#
testminvers='import sys
if (sys.version_info < (2,7)) or (sys.version_info < (3,2) and sys.version_info >= (3,0)):
   exit(1)'

testargparse='try: import argparse
except: exit(1)'

testply='try: import ply
except: exit(1)'

default_with_python="python python3 python3.7 python3.6 python3.5 python3.4 python3.3 python3.2 python2 python2.7"

AC_ARG_WITH([python],
	    AS_HELP_STRING([--with-python=PATH],
			   [specify path to Python interpreter]),
	    [], [with_python=$default_with_python])
AC_ARG_WITH([python-install-dir],
	    AS_HELP_STRING([--with-python-install-dir=PATH],
			   [installation directory for Python modules]),
	    [], with_python_install_dir="")

AS_IF([test "$with_python" = "yes"],
      [with_python=$default_with_python])

AS_IF([test "$with_python" = "no"],
      [AC_MSG_CHECKING([for Python support])
       AC_MSG_RESULT([disabled])],
      [for p in $with_python
       do
	 AS_CASE([$p],
		 [/*],[PYTHON="$p"])

	 AC_PATH_PROG([PYTHON], [$p])
	 # Do not cache the result of the check from the previous line.  If the
	 # first found Python interpreter has missing module dependencies and
	 # the result of the above check is cached, subsequent module checks
	 # will erroneously keep on using the cached path to the first found
	 # Python interpreter instead of different ones.
	 unset ac_cv_path_PYTHON

	 AS_IF([test -z "$PYTHON"], [continue])

	 AC_MSG_CHECKING([if $PYTHON is python2 version >= 2.7 or python3 version >= 3.2])
	 AS_IF(["$PYTHON" -c "$testminvers" 2>/dev/null],
	       [AC_MSG_RESULT([yes])],
	       [AC_MSG_RESULT([no])
		unset PYTHON
		continue])

	 AC_MSG_CHECKING([Python module 'argparse'])
	 AS_IF(["$PYTHON" -c "$testargparse" 2>/dev/null],
	       [AC_MSG_RESULT([yes])],
	       [AC_MSG_RESULT([no])
		unset PYTHON
		continue])

	 AC_MSG_CHECKING([Python module 'ply'])
	 AS_IF(["$PYTHON" -c "$testply" 2>/dev/null],
	       [AC_MSG_RESULT([yes])],
	       [AC_MSG_RESULT([no])
		unset PYTHON
		continue])

	 # Stop looking any further once we find a Python interpreter
	 # satisfying all requirements.
	 break
       done

       AS_IF([test "X$PYTHON" = "X"],
	     [AC_MSG_CHECKING([for Python support])
	      AC_MSG_RESULT([no])
	      AC_MSG_ERROR([Python required for dnssec-keymgr])])])

PYTHON_TOOLS=''
CHECKDS=''
COVERAGE=''
KEYMGR=''
AS_IF([test "X$PYTHON" != "X"],
      [PYTHON_TOOLS=python
       CHECKDS=checkds
       COVERAGE=coverage
       KEYMGR=keymgr
       PYTHON_INSTALL_DIR="$with_python_install_dir"
       AS_IF([test -n "$with_python_install_dir"],
	     [PYTHON_INSTALL_LIB="--install-lib=$with_python_install_dir"])])
AC_SUBST(CHECKDS)
AC_SUBST(COVERAGE)
AC_SUBST(KEYMGR)
AC_SUBST(PYTHON_TOOLS)
AC_SUBST(PYTHON_INSTALL_DIR)
AC_SUBST(PYTHON_INSTALL_LIB)

#
# Special processing of paths depending on whether --prefix,
# --sysconfdir or --localstatedir arguments were given.  What's
# desired is some compatibility with the way previous versions
# of BIND built; they defaulted to /usr/local for most parts of
# the installation, but named.boot/named.conf was in /etc
# and named.pid was in /var/run.
#
# So ... if none of --prefix, --sysconfdir or --localstatedir are
# specified, set things up that way.  If --prefix is given, use
# it for sysconfdir and localstatedir the way configure normally
# would.  To change the prefix for everything but leave named.conf
# in /etc or named.pid in /var/run, then do this the usual configure way:
# ./configure --prefix=/somewhere --sysconfdir=/etc
# ./configure --prefix=/somewhere --localstatedir=/var
#
# To put named.conf and named.pid in /usr/local with everything else,
# set the prefix explicitly to /usr/local even though that's the default:
# ./configure --prefix=/usr/local
#
case "$prefix" in
	NONE)
		case "$sysconfdir" in
			'${prefix}/etc')
				sysconfdir=/etc
				;;
		esac
		case "$localstatedir" in
			'${prefix}/var')
				localstatedir=/var
				;;
		esac
		;;
esac
expanded_sysconfdir=`eval echo $sysconfdir`
AC_SUBST(expanded_sysconfdir)

#
# Make sure INSTALL uses an absolute path, else it will be wrong in all
# Makefiles, since they use make/rules.in and INSTALL will be adjusted by
# configure based on the location of the file where it is substituted.
# Since in BIND9 INSTALL is only substituted into make/rules.in, an immediate
# subdirectory of install-sh, This relative path will be wrong for all
# directories more than one level down from install-sh.
#
case "$INSTALL" in
	/*)
		;;
	*)
		#
		# Not all systems have dirname.
		#
		changequote({, })
		ac_dir="`echo $INSTALL | sed 's%/[^/]*$%%'`"
		changequote([, ])

		ac_prog="`echo $INSTALL | sed 's%.*/%%'`"
		test "X$ac_dir" = "X$ac_prog" && ac_dir=.
		test -d "$ac_dir" && ac_dir="`(cd \"$ac_dir\" && pwd)`"
		INSTALL="$ac_dir/$ac_prog"
		;;
esac

AC_PROG_CC
AC_PROG_CC_C99

#
# CCNOOPT defaults to -O0 on gcc and disables optimization when is last
#
if test "X$CCNOOPT" = "X" -a "X$GCC" = "Xyes"; then
	CCNOOPT="-O0"
fi

AC_HEADER_STDC

AC_CHECK_HEADERS(fcntl.h regex.h sys/time.h unistd.h sys/mman.h sys/sockio.h sys/select.h sys/param.h sys/sysctl.h net/if6.h sys/socket.h net/route.h linux/netlink.h linux/rtnetlink.h,,,
[$ac_includes_default
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

#
# Check for thread local storage
#
AC_CHECK_HEADERS([threads.h],
		 [
		     AC_MSG_CHECKING([for C11 Thread-Local Storage using thread_local])
		     AC_COMPILE_IFELSE(
			 [AC_LANG_PROGRAM(
			      [
				  #include <threads.h>
			      ],[
				  static thread_local int tls = 0;
				  return (tls);
			      ])
			 ],[
			     AC_MSG_RESULT([yes])
			     AC_DEFINE([HAVE_THREAD_LOCAL],[1],[Define if thread_local keyword is available])
			     AC_DEFINE([HAVE_TLS],[1],[Define if Thread-Local Storage is available])
			 ],[
			     AC_MSG_RESULT([no])
			 ])
		 ],[
		     AC_MSG_CHECKING([for Thread-Local Storage using __thread])
		     AC_COMPILE_IFELSE(
			 [AC_LANG_PROGRAM(
			      [
			      ],[
				  static __thread int tls = 0;
				  return (tls);
			      ])
			 ],[
			     AC_MSG_RESULT([yes])
			     AC_DEFINE([HAVE___THREAD],[1],[Define if __thread keyword is available])
			     AC_DEFINE([HAVE_TLS],[1],[Define if Thread-Local Storage is available])
			 ],[
			     AC_MSG_RESULT([no])
			 ])
		 ])

AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_FLEXIBLE_ARRAY_MEMBER

#
# Check for yield support on ARM processors
#
AS_CASE([$host],
	[arm*],[
	  AC_MSG_CHECKING([for yield instruction support])
	  AC_COMPILE_IFELSE(
	    [AC_LANG_PROGRAM(
	       [[]],
	       [[__asm__ __volatile__ ("yield")]]
	     )],
	    [AC_MSG_RESULT([yes])],
	    [AC_MSG_ERROR([no, try adding -march=native or -march=armv7-a to CFLAGS (see PLATFORMS.md for more information)])])])

AC_CHECK_FUNCS([sysctlbyname])

#
# Check for the existence of mmap to enable the fast format zones
#
AC_CHECK_FUNCS(mmap)

#
# Older versions of HP/UX don't define seteuid() and setegid()
#
AC_CHECK_FUNCS(seteuid setresuid)
AC_CHECK_FUNCS(setegid setresgid)

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINTPTR_T

AC_HEADER_TIME

#
# check for uname library routine
#
AC_MSG_CHECKING([for uname])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
     [[#include <sys/utsname.h>
       #include <stdio.h>
      ]],
     [[
       struct utsname uts;
       uname(&uts);
       printf("running on %s %s %s for %s\n",
	      uts.sysname, uts.release, uts.version, uts.machine);
     ]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([HAVE_UNAME], [1], [define if uname is available])
  ],
  [AC_MSG_RESULT(no)
   AC_MSG_WARN([uname is not correctly supported])
  ])

#
# check for GCC noreturn attribute
#
AC_MSG_CHECKING(for GCC noreturn attribute)
AC_TRY_COMPILE([],[void foo() __attribute__((noreturn));],
	[AC_MSG_RESULT(yes)
		ISC_PLATFORM_NORETURN_PRE="#define ISC_PLATFORM_NORETURN_PRE"
		ISC_PLATFORM_NORETURN_POST="#define ISC_PLATFORM_NORETURN_POST __attribute__((noreturn))"],
	[AC_MSG_RESULT(no)
		ISC_PLATFORM_NORETURN_PRE="#define ISC_PLATFORM_NORETURN_PRE"
		ISC_PLATFORM_NORETURN_POST="#define ISC_PLATFORM_NORETURN_POST"])
AC_SUBST(ISC_PLATFORM_NORETURN_PRE)
AC_SUBST(ISC_PLATFORM_NORETURN_POST)

#
# check if we have kqueue
#
AC_ARG_ENABLE([kqueue],
	      [AS_HELP_STRING([--enable-kqueue],
			      [use BSD kqueue when available [default=yes]])],
	      [], enable_kqueue="yes")

AS_IF([test "$enable_kqueue" = "yes"],
      [AC_CHECK_FUNCS([kqueue])])

#
# check if we have epoll.  Linux kernel 2.4 has epoll_create() which fails,
# so we need to try running the code, not just test its existence.
#
AC_ARG_ENABLE([epoll],
	      [AS_HELP_STRING([--enable-epoll],
			      [use Linux epoll when available [default=auto]])],
	      [], [enable_epoll="yes"])

AS_IF([test "$enable_epoll" = "yes"],
      [AC_CHECK_FUNCS([epoll_create1])])

#
# check if we support /dev/poll
#
AC_ARG_ENABLE([devpoll],
	      [AS_HELP_STRING([--enable-devpoll],
			      [use /dev/poll when available [default=yes]])],
	      [], [enable_devpoll="yes"])
AS_IF([test "$enable_devpoll" = "yes"],
      [AC_CHECK_HEADERS([sys/devpoll.h devpoll.h])])

#
# Find the machine's endian flavor.
#
AC_C_BIGENDIAN

#
# GeoIP support?
#
GEOIPLINKSRCS=
GEOIPLINKOBJS=
AC_ARG_WITH(geoip,
	    AS_HELP_STRING([--with-geoip=PATH],
			   [Build with GeoIP support (yes|no|path)]),
    use_geoip="$withval", use_geoip="no")

if test "yes" = "$use_geoip"
then
	for d in /usr /usr/local /opt/local
	do
		if test -f $d/include/GeoIP.h
		then
			use_geoip=$d
			break
		fi
	done
fi

case "$use_geoip" in
	no|'')
		AC_MSG_CHECKING([for GeoIP support])
		AC_MSG_RESULT([disabled])
		;;
	*)
		if test -d "$use_geoip" -o -L "$use_geoip"
		then
			CFLAGS="$CFLAGS -I$use_geoip/include"
			CPPFLAGS="$CPPFLAGS -I$use_geoip/include"
			LIBS="$LIBS -L$use_geoip/lib"
			case "$host_os" in
				netbsd*|openbsd*|solaris*)
					LIBS="$LIBS -Wl,-rpath=$use_geoip/lib"
					;;
			esac
		elif test "yes" = "$use_geoip"
		then
			AC_MSG_ERROR([GeoIP path not found])
		else
			AC_MSG_ERROR([GeoIP path $use_geoip does not exist])
		fi
		AC_CHECK_HEADER(GeoIP.h, [],
			[AC_MSG_ERROR([GeoIP header file not found])]
		)
		AC_SEARCH_LIBS(GeoIP_id_by_addr_gl, GeoIP, [],
			[AC_MSG_ERROR([suitable GeoIP library not found])]
		)
		AC_SEARCH_LIBS(fabsf, m, [],
			[AC_MSG_ERROR([Math library not found])]
		)
		AC_DEFINE(HAVE_GEOIP, 1, Build with GeoIP support)
		GEOIPLINKSRCS='${GEOIPLINKSRCS}'
		GEOIPLINKOBJS='${GEOIPLINKOBJS}'
		AC_MSG_CHECKING([for GeoIP support])
		AC_MSG_RESULT([yes])

		AC_MSG_CHECKING([for GeoIP Country IPv6 support])
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([
				#include <GeoIP.h>
				#include <netinet/in.h>
			], [
				struct in6_addr in6;
				GeoIP_country_name_by_ipnum_v6(NULL, in6);
			])],
			[
				AC_MSG_RESULT([yes])
				AC_DEFINE(HAVE_GEOIP_V6, 1, Build with GeoIP Country IPv6 support)
			],
			[AC_MSG_RESULT([no])]
		)

		AC_MSG_CHECKING([for GeoIP City IPv6 support])
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([
				#include <GeoIP.h>
				#include <GeoIPCity.h>
				#include <netinet/in.h>
			], [
				struct in6_addr in6;
				int i = GEOIP_CITY_EDITION_REV0_V6;
				GeoIP_record_by_ipnum_v6(NULL, in6);
			])],
			[
				AC_MSG_RESULT([yes])
				AC_DEFINE(HAVE_GEOIP_CITY_V6, 1, Build with GeoIP City IPv6 support)
			],
			[AC_MSG_RESULT([no])]
		)
		;;
esac
AC_SUBST(GEOIPLINKSRCS)
AC_SUBST(GEOIPLINKOBJS)

#
# Do we have arc4random(), etc ?
#
AC_CHECK_FUNCS(arc4random arc4random_buf arc4random_uniform getrandom)

AX_PTHREAD

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

#
# We'd like to use sigwait() too
#
AC_CHECK_FUNCS([sigwait pthread_attr_getstacksize pthread_attr_setstacksize])

AC_ARG_WITH([locktype],
	    AS_HELP_STRING([--with-locktype=ARG],
			   [Specify mutex lock type
			    (adaptive or standard)]),
	    [], [with_locktype="adaptive"])

AS_CASE([$with_locktype],
	[adaptive],[
	  AC_MSG_CHECKING([for PTHREAD_MUTEX_ADAPTIVE_NP])
	  AC_COMPILE_IFELSE(
	    [AC_LANG_PROGRAM(
	       [[
		 #ifndef _GNU_SOURCE
		 #define _GNU_SOURCE
		 #endif
		 #include <pthread.h>
	       ]],
	       [[
		 return (PTHREAD_MUTEX_ADAPTIVE_NP);
	       ]]
	     )],
	    [AC_MSG_RESULT([using adaptive lock type])
	     AC_DEFINE([HAVE_PTHREAD_MUTEX_ADAPTIVE_NP], 1,
		       [Support for PTHREAD_MUTEX_ADAPTIVE_NP]) ],
	    [AC_MSG_RESULT([using standard lock type])]
	  )],
	[standard],[AC_MSG_RESULT([using standard lock type])],
	[AC_MSG_ERROR([You must specify "adaptive" or "standard" for --with-locktype.])]
       )

AC_CHECK_HEADERS([sched.h])

AC_SEARCH_LIBS([sched_yield],[rt])
AC_CHECK_FUNCS([sched_yield pthread_yield pthread_yield_np])

AC_CHECK_HEADERS([sys/cpuset.h])
AC_CHECK_HEADERS([sys/procset.h])
AC_CHECK_FUNCS([pthread_setaffinity_np cpuset_setaffinity processor_bind sched_setaffinity])

# Look for functions relating to thread naming
AC_CHECK_FUNCS([pthread_setname_np pthread_set_name_np])
AC_CHECK_HEADERS([pthread_np.h], [], [], [#include <pthread.h>])

#
# Look for sysconf to allow detection of the number of processors.
#
AC_CHECK_FUNCS([sysconf])

AC_SUBST(ALWAYS_DEFINES)

AC_MSG_CHECKING(for libtool)
AC_ARG_WITH(libtool, AS_HELP_STRING([--with-libtool], [use GNU libtool]),
	    use_libtool="$withval", use_libtool="no")

case $use_libtool in
	yes)
		AC_MSG_RESULT(yes)
		AM_PROG_LIBTOOL
		O=lo
		A=la
		LIBTOOL_MKDEP_SED='s;\.o;\.lo;'
		LIBTOOL_MODE_COMPILE='--mode=compile'
		LIBTOOL_MODE_INSTALL='--mode=install'
		LIBTOOL_MODE_LINK='--mode=link'
		LIBTOOL_MODE_UNINSTALL='--mode=uninstall'
		INSTALL_LIBRARY='${INSTALL_PROGRAM}'
		;;
	*)
		AC_MSG_RESULT(no)
		O=o
		A=a
		LIBTOOL=
		AC_SUBST(LIBTOOL)
		LIBTOOL_MKDEP_SED=
		LIBTOOL_MODE_COMPILE=
		LIBTOOL_MODE_INSTALL=
		LIBTOOL_MODE_LINK=
		LIBTOOL_MODE_UNINSTALL=
		INSTALL_LIBRARY='${INSTALL_DATA}'
		;;
esac
AC_SUBST(INSTALL_LIBRARY)

#
# If PIC is disabled, shared libraries must also be
#
AS_IF([test "$pic_mode" = "no"],
      [enable_shared="no"])

CRYPTO=OpenSSL

#
# was --with-openssl specified?
#
AX_CHECK_OPENSSL([:],[AC_MSG_FAILURE([OpenSSL/LibreSSL not found])])

save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
save_LDFLAGS="$LDFLAGS"
CFLAGS="$CFLAGS $OPENSSL_INCLUDES"
LIBS="$LIBS $OPENSSL_LIBS"
LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS"

AC_MSG_CHECKING([for OpenSSL >= 1.0.0 or LibreSSL])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <openssl/opensslv.h>]],
		     [[#if !defined(LIBRESSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER < 0x1000000fL)
		       #error OpenSSL >= 1.0.0 or LibreSSL required
		       #endif
		      ]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_FAILURE([not found])])

#
# Check for functions added in OpenSSL or LibreSSL
#

AC_CHECK_FUNCS([CRYPTO_zalloc])
AC_CHECK_FUNCS([EVP_CIPHER_CTX_new EVP_CIPHER_CTX_free])
AC_CHECK_FUNCS([EVP_MD_CTX_new EVP_MD_CTX_free EVP_MD_CTX_reset])
AC_CHECK_FUNCS([HMAC_CTX_new HMAC_CTX_free HMAC_CTX_reset HMAC_CTX_get_md])

#
# Check for algorithm support in OpenSSL
#

AC_CHECK_FUNCS([ECDSA_sign ECDSA_verify], [:],
	       [AC_MSG_FAILURE([ECDSA support in OpenSSL is mandatory.])])

AC_MSG_CHECKING([for ECDSA P-256 support])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <openssl/evp.h>
		       #include <openssl/ec.h>]],
		     [[EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_FAILURE([not found.  ECDSA P-256 support in OpenSSL is mandatory.])])

AC_MSG_CHECKING([for ECDSA P-384 support])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <openssl/evp.h>
		       #include <openssl/ec.h>]],
		     [[EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp384r1);]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_FAILURE([not found.  ECDSA P-384 support in OpenSSL is mandatory.])])

AC_MSG_CHECKING([for Ed25519 support])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <openssl/evp.h>
		       #include <openssl/ec.h>]],
		     [[EC_KEY *key = EC_KEY_new_by_curve_name(NID_ED25519);]])],
    [AC_DEFINE([HAVE_OPENSSL_ED25519], [1], [define if OpenSSL supports Ed25519])
     AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for Ed448 support])
AC_MSG_RESULT([broken])
#AC_COMPILE_IFELSE(
#    [AC_LANG_PROGRAM([[#include <openssl/evp.h>
#		       #include <openssl/ec.h>]],
#		     [[EC_KEY *key = EC_KEY_new_by_curve_name(NID_ED448);]])],
#    [AC_DEFINE([HAVE_OPENSSL_ED448], [1], [define if OpenSSL supports Ed448])
#     AC_MSG_RESULT([yes])],
#    [AC_MSG_RESULT([no])])

#
# Check for OpenSSL SHA-1 support
#
AC_CHECK_FUNCS([EVP_sha1], [:],
	       [AC_MSG_FAILURE([SHA-1 support in OpenSSL is mandatory.])])

#
# Check for OpenSSL SHA-2 support
#
AC_CHECK_FUNCS([EVP_sha224 EVP_sha256 EVP_sha384 EVP_sha512], [:],
	       [AC_MSG_FAILURE([SHA-2 support in OpenSSL is mandatory.])])

#
# Check for OpenSSL AES support
#
AC_CHECK_FUNCS([EVP_aes_128_ecb EVP_aes_192_ecb EVP_aes_256_ecb], [:],
	       [AC_MSG_FAILURE([AES support in OpenSSL is mandatory.])])

#
# Check for OpenSSL 1.1.x/LibreSSL functions
#
AC_CHECK_FUNCS([DH_get0_key ECDSA_SIG_get0 RSA_set0_key])

#
# Check whether FIPS mode is available and whether we should enable it
#
AC_ARG_ENABLE([fips-mode],
	      [AS_HELP_STRING([--enable-fips-mode],
			      [enable FIPS mode in OpenSSL library [default=no]])],
	      [], [enable_fips_mode="no"])

AC_MSG_CHECKING([whether to enable FIPS mode in OpenSSL library])
AS_CASE([$enable_fips_mode],
	[yes], [AC_MSG_RESULT([yes])
		AC_CHECK_FUNCS([FIPS_mode],
			       [], [AC_MSG_FAILURE([OpenSSL FIPS mode requested but not available.])])],
	[no], [AC_MSG_RESULT([no])])

CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"
LDFLAGS="$save_LDFLAGS"

AC_SUBST([OPENSSL_INCLUDES])
AC_SUBST([OPENSSL_LIBS])
AC_SUBST([OPENSSL_LDFLAGS])

#
# Client Cookie algorithm choice
#
AC_ARG_WITH([cc-alg],
	    [AS_HELP_STRING([--with-cc-alg=ALG],
			   [choose the algorithm for Client Cookie
			    [aes|sha1|sha256] (default is aes)])],
	    [:], [with_cc_alg="aes"])

AC_MSG_CHECKING([for the algorithm for Client Cookie])
AS_CASE([$with_cc_alg],
	[sha1|SHA1],[AC_MSG_RESULT([sha1])
		     AC_DEFINE([HMAC_SHA1_CC], [1], [Use HMAC-SHA1 for Client Cookie generation])],
	[sha256|SHA256],[AC_MSG_RESULT([sha256])
			 AC_DEFINE([HMAC_SHA256_CC], [1], [Use HMAC-SHA256 for Client Cookie generation])],
	[aes|AES|auto],[AC_MSG_RESULT([aes])
		       AC_DEFINE([AES_CC], [1], [Use AES for Client Cookie generation])],
	[AC_MSG_ERROR([Invalid $with_cc_alg algorithm for Client Cookie])])


PKCS11_TOOLS=
PKCS11_TEST=
#
# was --enable-native-pkcs11 specified?
#
AC_ARG_ENABLE(native-pkcs11,
	      AS_HELP_STRING([--enable-native-pkcs11],
			     [use native PKCS11 for public-key crypto [default=no]]),
	      [:], [enable_native_pkcs11="no"])

AC_MSG_CHECKING([for PKCS11 for Public-Key Cryptography])
AS_CASE([$enable_native_pkcs11],
	[no],[AC_MSG_RESULT([no])],
	[yes],[PKCS11_TOOLS=pkcs11
	       PKCS11_TEST=pkcs11
	       CRYPTO=pkcs11
	       AS_IF([$use_threads],
		     [:],
		     [AC_MSG_ERROR([PKCS11 requires threading support])])
	       AC_MSG_RESULT([yes])
	       AC_CHECK_FUNCS([getpassphrase])
	      ])
AC_SUBST([PKCS11_TEST])
AC_SUBST([PKCS11_TOOLS])

AS_CASE([$CRYPTO],
	[pkcs11],[AC_DEFINE([USE_PKCS11], [1], [define if PKCS11 is used for Public-Key Cryptography])],
	[AC_DEFINE([USE_OPENSSL], [1], [define if OpenSSL is used for Public-Key Cryptography])])

# preparation for automake
# AM_CONDITIONAL([PKCS11_TOOLS], [test "$with_native_pkcs11" = "yes"])

#
# was --with-pkcs11 specified?
#
AC_ARG_WITH([pkcs11],
	    [AS_HELP_STRING([--with-pkcs11[=PATH]],
			    [Build with PKCS11 support [no|path] (PATH is for the PKCS11 provider)])],
	    [:], [with_pkcs11="undefined"])

AS_CASE([$with_pkcs11],
	[yes|auto],[AC_MSG_ERROR([--with-pkcs11 needs explicit path to the PKCS11 library])],
	[no|undefined],[with_pkcs11="undefined"])
AC_DEFINE_UNQUOTED([PK11_LIB_LOCATION], ["$with_pkcs11"], [define the default PKCS11 library path])

# for PKCS11 benchmarks

have_clock_gt=no
AC_CHECK_FUNC(clock_gettime,have_clock_gt=yes,)
if test "no" = "$have_clock_gt"; then
	AC_CHECK_LIB(rt,clock_gettime,have_clock_gt=rt,)
fi

if test "no" != "$have_clock_gt"; then
	AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Define if clock_gettime is available.])
fi

if test "rt" = "$have_clock_gt"; then
	LIBS="-lrt $LIBS"
fi

AC_MSG_CHECKING(for GSSAPI library)
AC_ARG_WITH(gssapi,
	    AS_HELP_STRING([--with-gssapi=[PATH|[/path/]krb5-config]],
			   [Specify path for system-supplied GSSAPI
				[default=yes]]),
	    use_gssapi="$withval", use_gssapi="yes")

# first try using krb5-config, if that does not work then fall back to "yes" method.

case "$use_gssapi" in
*/krb5-config|krb5-config)
    AC_MSG_RESULT(trying $use_gssapi)
    if test krb5-config = "$use_gssapi"
    then
	AC_PATH_PROG(KRB5_CONFIG, $use_gssapi)
    else
	KRB5_CONFIG="$use_gssapi"
    fi
    gssapi_cflags=`$KRB5_CONFIG --cflags gssapi`
    gssapi_libs=`$KRB5_CONFIG --libs gssapi`
    saved_cppflags="$CPPFLAGS"
    CPPFLAGS="$gssapi_cflags $CPPFLAGS"
    AC_CHECK_HEADERS(gssapi.h gssapi/gssapi.h,
	[ISC_PLATFORM_GSSAPIHEADER="#define ISC_PLATFORM_GSSAPIHEADER <$ac_header>"])
    if test "" = "$ISC_PLATFORM_GSSAPIHEADER"; then
	AC_MSG_RESULT([krb5-config: gssapi.h not found])
	CPPFLAGS="$saved_cppflags"
	use_gssapi="yes"
    else
	AC_CHECK_HEADERS(krb5/krb5.h krb5.h,
	    [ISC_PLATFORM_KRB5HEADER="#define ISC_PLATFORM_KRB5HEADER <$ac_header>"])
	if test "" = "$ISC_PLATFORM_KRB5HEADER"; then
	    AC_MSG_RESULT([krb5-config: krb5.h not found])
	    CPPFLAGS="$saved_cppflags"
	    use_gssapi="yes"
	else
	    CPPFLAGS="$saved_cppflags"
	    saved_libs="$LIBS"
	    LIBS=$gssapi_libs
	    AC_MSG_CHECKING([krb5-config linking as $LIBS])
	    AC_TRY_LINK( , [gss_acquire_cred();krb5_init_context()],
		gssapi_linked=yes, gssapi_linked=no)
	    case $gssapi_linked in
		yes) AC_MSG_RESULT([krb5-config: linked]);;
		no)  AC_MSG_RESULT([krb5-config: could not determine proper GSSAPI linkage])
		    use_gssapi="yes"
		    ;;
	    esac
	    LIBS=$saved_libs
	fi
    fi
    if test "yes" = "$use_gssapi"; then
	AC_MSG_CHECKING([for GSSAPI library, non krb5-config method])
    fi
    ;;
esac

case "$host" in
*darwin*)
	if test "yes" = "$use_gssapi"
	then
		use_gssapi=framework
	fi
	;;
esac

# gssapi is just the framework, we really require kerberos v5, so
# look for those headers (the gssapi headers must be there, too)
# The problem with this implementation is that it doesn't allow
# for the specification of gssapi and krb5 headers in different locations,
# which probably ought to be fixed although fixing might raise the issue of
# trying to build with incompatible versions of gssapi and krb5.
if test "yes" = "$use_gssapi"
then
	# first, deal with the obvious
	if test \( -f /usr/include/kerberosv5/krb5.h -o \
		   -f /usr/include/krb5/krb5.h -o \
		   -f /usr/include/krb5.h \)   -a \
		\( -f /usr/include/gssapi.h -o \
		   -f /usr/include/gssapi/gssapi.h \)
	then
		use_gssapi=/usr
	else
	    krb5dirs="/usr/local /usr/local/krb5 /usr/local/kerberosv5 /usr/local/kerberos /usr/pkg /usr/krb5 /usr/kerberosv5 /usr/kerberos /usr"
	    for d in $krb5dirs
	    do
		if test -f $d/include/gssapi/gssapi_krb5.h -o \
			-f $d/include/krb5.h
		then
			if test -f $d/include/gssapi/gssapi.h -o \
				-f $d/include/gssapi.h
			then
				use_gssapi=$d
				break
			fi
		fi
		use_gssapi="no"
	    done
	fi
fi

case "$use_gssapi" in
	no)
		AC_MSG_RESULT(disabled)
		USE_GSSAPI=''
		;;
	yes)
		AC_MSG_ERROR([--with-gssapi must specify a path])
		;;
	*/krb5-config|krb5-config)
		USE_GSSAPI='-DGSSAPI'
		DST_GSSAPI_INC="$gssapi_cflags"
		DNS_GSSAPI_LIBS="$gssapi_libs"
		;;
	framework)
		USE_GSSAPI='-DGSSAPI'
		ISC_PLATFORM_GSSAPIHEADER="#define ISC_PLATFORM_GSSAPIHEADER <Kerberos/Kerberos.h>"
		ISC_PLATFORM_KRB5HEADER="#define ISC_PLATFORM_KRB5HEADER <Kerberos/Kerberos.h>"
		DNS_GSSAPI_LIBS="-framework Kerberos"
		AC_MSG_RESULT(framework)
		;;

	*)
		AC_MSG_RESULT(looking in $use_gssapi/lib)
		USE_GSSAPI='-DGSSAPI'
		saved_cppflags="$CPPFLAGS"
		CPPFLAGS="-I$use_gssapi/include $CPPFLAGS"
		AC_CHECK_HEADERS(gssapi.h gssapi/gssapi.h,
		    [ISC_PLATFORM_GSSAPIHEADER="#define ISC_PLATFORM_GSSAPIHEADER <$ac_header>"
		     gssapi_hack="#include <$ac_header>"])

		if test "" = "$ISC_PLATFORM_GSSAPIHEADER"; then
		    AC_MSG_ERROR([gssapi.h not found])
		fi

		AC_CHECK_HEADERS(gssapi_krb5.h gssapi/gssapi_krb5.h,
		    [ISC_PLATFORM_GSSAPI_KRB5_HEADER="#define ISC_PLATFORM_GSSAPI_KRB5_HEADER <$ac_header>"
		     gssapi_krb5_hack="#include <$ac_header>"])

		AC_CHECK_HEADERS(krb5.h krb5/krb5.h kerberosv5/krb5.h,
		    [ISC_PLATFORM_KRB5HEADER="#define ISC_PLATFORM_KRB5HEADER <$ac_header>"
		    krb5_hack="#include <$ac_header>"])

		if test "" = "$ISC_PLATFORM_KRB5HEADER"; then
		    AC_MSG_ERROR([krb5.h not found])
		fi

		#
		# XXXDCL This probably doesn't work right on all systems.
		# It will need to be worked on as problems become evident.
		#
		# Essentially the problems here relate to two different
		# areas.  The first area is building with either KTH
		# or MIT Kerberos, particularly when both are present on
		# the machine.  The other is static versus dynamic linking.
		#
		# On the KTH vs MIT issue, Both have libkrb5 that can mess
		# up the works if one implementation ends up trying to
		# use the other's krb.  This is unfortunately a situation
		# that very easily arises.
		#
		# Dynamic linking when the dependency information is built
		# into MIT's libgssapi_krb5 or KTH's libgssapi magically makes
		# all such problems go away, but when that setup is not
		# present, because either the dynamic libraries lack
		# dependencies or static linking is being done, then the
		# problems start to show up.
		saved_libs="$LIBS"
		for TRY_LIBS in \
		    "-lgssapi_krb5" \
		    "-lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err" \
		    "-lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err -lresolv" \
		    "-lgssapi" \
		    "-lgssapi -lkrb5 -ldes -lcrypt -lasn1 -lroken -lcom_err" \
		    "-lgssapi -lkrb5 -lcrypt -lasn1 -lroken -lcom_err" \
		    "-lgssapi -lkrb5 -lgssapi_krb5 -lcrypt -lasn1 -lroken -lcom_err" \
		    "-lgssapi -lkrb5 -lhx509 -lcrypt -lasn1 -lroken -lcom_err" \
		    "-lgss -lkrb5"
		do
		    # Note that this does not include $saved_libs, because
		    # on FreeBSD machines this configure script has added
		    # -L/usr/local/lib to LIBS, which can make the
		    # -lgssapi_krb5 test succeed with shared libraries even
		    # when you are trying to build with KTH in /usr/lib.
		    if test "/usr" = "$use_gssapi"
		    then
			    LIBS="$TRY_LIBS $ISC_OPENSSL_LIBS"
		    else
			    LIBS="-L$use_gssapi/lib $TRY_LIBS $ISC_OPENSSL_LIBS"
		    fi
		    AC_MSG_CHECKING(linking as $TRY_LIBS)
		    AC_TRY_LINK([
#include <sys/types.h>
$gssapi_hack
$gssapi_krb5_hack
$krb5_hack
				] , [gss_acquire_cred(NULL, NULL, 0, NULL, 0, NULL, NULL, NULL);krb5_init_context(NULL);
#if defined(HAVE_GSSAPI_KRB5_H) || defined(HAVE_GSSAPI_GSSAPI_KRB5_H)
gsskrb5_register_acceptor_identity(NULL);
#endif],
				gssapi_linked=yes, gssapi_linked=no)
		    case $gssapi_linked in
		    yes) AC_MSG_RESULT(yes); break ;;
		    no)  AC_MSG_RESULT(no) ;;
		    esac
		done

		CPPFLAGS="$saved_cppflags"

		case $gssapi_linked in
		no) AC_MSG_ERROR(could not determine proper GSSAPI linkage) ;;
		esac

		#
		# XXXDCL Major kludge.  Tries to cope with KTH in /usr/lib
		# but MIT in /usr/local/lib and trying to build with KTH.
		# /usr/local/lib can end up earlier on the link lines.
		# Like most kludges, this one is not only inelegant it
		# is also likely to be the wrong thing to do at least as
		# many times as it is the right thing.  Something better
		# needs to be done.
		#
		if test "/usr" = "$use_gssapi" -a \
			-f /usr/local/lib/libkrb5.a; then
		    FIX_KTH_VS_MIT=yes
		fi

		case "$FIX_KTH_VS_MIT" in
		yes)
		    case "$enable_static_linking" in
		    yes) gssapi_lib_suffix=".a"  ;;
		    *)   gssapi_lib_suffix=".so" ;;
		    esac

		    for lib in $LIBS; do
			case $lib in
			-L*)
			    ;;
			-l*)
			    new_lib=`echo $lib |
				     sed -e s%^-l%$use_gssapi/lib/lib% \
					 -e s%$%$gssapi_lib_suffix%`
			    NEW_LIBS="$NEW_LIBS $new_lib"
			    ;;
			*)
			   AC_MSG_ERROR([KTH vs MIT Kerberos confusion!])
			    ;;
			esac
		    done
		    LIBS="$NEW_LIBS"
		    ;;
		esac

		DST_GSSAPI_INC="-I$use_gssapi/include"
		DNS_GSSAPI_LIBS="$LIBS"

		AC_MSG_RESULT(using GSSAPI from $use_gssapi/lib and $use_gssapi/include)
		LIBS="$saved_libs"
		;;
esac

AC_SUBST(ISC_PLATFORM_HAVEGSSAPI)
AC_SUBST(ISC_PLATFORM_GSSAPIHEADER)
AC_SUBST(ISC_PLATFORM_GSSAPI_KRB5_HEADER)
AC_SUBST(ISC_PLATFORM_KRB5HEADER)

AC_SUBST(USE_GSSAPI)
AC_SUBST(DST_GSSAPI_INC)
AC_SUBST(DNS_GSSAPI_LIBS)
DNS_CRYPTO_LIBS="$DNS_GSSAPI_LIBS"

#
# Applications linking with libdns also need to link with these libraries.
#

AC_SUBST(DNS_CRYPTO_LIBS)

#
# was --with-lmdb specified?
#
AC_MSG_CHECKING(for lmdb library)
AC_ARG_WITH(lmdb,
	    AS_HELP_STRING([--with-lmdb[=PATH]],
			   [build with LMDB library [yes|no|path]]),
    use_lmdb="$withval", use_lmdb="auto")

have_lmdb=""
case "$use_lmdb" in
	no)
		lmdb_libs=""
		;;
	auto|yes)
		for d in /usr /usr/local /opt/local
		do
			if test -f "${d}/include/lmdb.h"
			then
				if test ${d} != /usr
				then
					lmdb_cflags="-I ${d}/include"
					LIBS="$LIBS -L${d}/lib"
				fi
				have_lmdb="yes"
			fi
		done
		;;
	*)
		if test -f "${use_lmdb}/include/lmdb.h"
		then
			lmdb_cflags="-I${use_lmdb}/include"
			LIBS="$LIBS -L${use_lmdb}/lib"
			have_lmdb="yes"
		else
			AC_MSG_ERROR([$use_lmdb/include/lmdb.h not found.])
		fi
		;;
esac

if test "X${have_lmdb}" != "X"
then
	AC_MSG_RESULT(yes)
	AC_SEARCH_LIBS([mdb_env_create], [lmdb], [],
		       [AC_MSG_ERROR([found lmdb include but not library.])
			have_lmdb=""])
elif test "X$use_lmdb" = Xyes
then
	AC_MSG_ERROR([include/lmdb.h not found.])
else
	AC_MSG_RESULT(no)
fi

NZD_TOOLS=""
NZDSRCS=
NZDTARGETS=
if test "X${have_lmdb}" != "X"
then
	CFLAGS="$CFLAGS $lmdb_cflags"
	AC_DEFINE(HAVE_LMDB, 1, [Define if lmdb was found])
	NZD_TOOLS="nzd"
	NZDSRCS='${NZDSRCS}'
	NZDTARGETS='${NZDTARGETS}'
fi
AC_SUBST(NZD_TOOLS)
AC_SUBST(NZDSRCS)
AC_SUBST(NZDTARGETS)

#
# was --with-libxml2 specified?
#
AC_MSG_CHECKING(for libxml2 library)
AC_ARG_WITH(libxml2,
	    AS_HELP_STRING([--with-libxml2[=PATH]],
			   [build with libxml2 library [yes|no|path]]),
    use_libxml2="$withval", use_libxml2="auto")

case "$use_libxml2" in
	no)
		DST_LIBXML2_INC=""
		;;
	auto|yes)
		case X`(xml2-config --version) 2>/dev/null` in
		X2.[[6789]].*)
			libxml2_libs=`xml2-config --libs`
			libxml2_cflags=`xml2-config --cflags`
			;;
		*)
			if test "yes" = "$use_libxml2" ; then
				AC_MSG_RESULT(no)
				AC_MSG_ERROR(required libxml2 version not available)
			else
				libxml2_libs=
				libxml2_cflags=
			fi
			;;
		esac
		;;
	*)
		if test -f "$use_libxml2/bin/xml2-config" ; then
			libxml2_libs=`$use_libxml2/bin/xml2-config --libs`
			libxml2_cflags=`$use_libxml2/bin/xml2-config --cflags`
		fi
		;;
esac

if test "X$libxml2_libs" != "X"
then
	CFLAGS="$CFLAGS $libxml2_cflags"
	LIBS="$LIBS $libxml2_libs"
	#
	# Sanity check xml2-config output.
	#
	AC_TRY_LINK([#include <libxml/xmlwriter.h>],
		    [return(xmlTextWriterStartElement(NULL, NULL));],
		    AC_MSG_RESULT(yes),
		    AC_MSG_ERROR(xml2-config returns badness))
	AC_DEFINE(HAVE_LIBXML2, 1, [Define if libxml2 was found])
	XMLSTATS=1
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(XMLSTATS)

#
# was --with-libjson specified?
#
AC_MSG_CHECKING(for json library)
AC_ARG_WITH(libjson,
	    AS_HELP_STRING([--with-libjson[=PATH]],
			   [build with libjson0 library [yes|no|path]]),
	    use_libjson="$withval", use_libjson="auto")

have_libjson=""
have_libjson_c=""
case "$use_libjson" in
	no)
		libjson_libs=""
		;;
	auto|yes)
		for d in /usr /usr/local /opt/local
		do
			if test -f "${d}/include/json/json.h"
			then
				if test ${d} != /usr
				then
					libjson_cflags="-I ${d}/include"
					LIBS="$LIBS -L${d}/lib"
				fi
				have_libjson="yes"
			elif test -f "${d}/include/json-c/json.h"
			then
				if test ${d} != /usr
				then
					libjson_cflags="-I ${d}/include"
					LIBS="$LIBS -L${d}/lib"
				fi
				have_libjson="yes"
				have_libjson_c="yes"
			fi
		done
		;;
	*)
		if test -f "${use_libjson}/include/json/json.h"
		then
			libjson_cflags="-I${use_libjson}/include"
			LIBS="$LIBS -L${use_libjson}/lib"
			have_libjson="yes"
		elif test -f "${use_libjson}/include/json-c/json.h"
		then
			libjson_cflags="-I${use_libjson}/include"
			LIBS="$LIBS -L${use_libjson}/lib"
			have_libjson="yes"
			have_libjson_c="yes"
		else
			AC_MSG_ERROR([$use_libjson/include/json{,-c}/json.h not found.])
		fi
		;;
esac

if test "X${have_libjson}" != "X"
then
	AC_MSG_RESULT(yes)
	AC_SEARCH_LIBS([json_object_new_int64], [json json-c], [],
		       [AC_MSG_ERROR([found libjson include but not library.])
			have_libjson=""])
elif test "X$use_libjson" = Xyes
then
	AC_MSG_ERROR([include/json{,-c}/json.h not found.])
else
	AC_MSG_RESULT(no)
fi

if test "X${have_libjson}" != "X"
then
	CFLAGS="$CFLAGS $libjson_cflags"
	AC_DEFINE(HAVE_JSON, 1, [Define if libjson was found])
	if test "X${have_libjson_c}" = Xyes
	then
		AC_DEFINE(HAVE_JSON_C, 1, [Define if json-c was found])
	fi
	JSONSTATS=1
fi
AC_SUBST(JSONSTATS)

#
# was --with-zlib specified?
#
AC_MSG_CHECKING(for zlib library)
AC_ARG_WITH(zlib,
	    AS_HELP_STRING([--with-zlib[=PATH]],
			   [build with zlib for HTTP compression
				[default=yes]]),
	    with_zlib="$withval", with_zlib="auto")

have_zlib=""
case "$with_zlib" in
	no)
		zlib_libs=""
		;;
	auto|yes)
		for d in /usr /usr/local /opt/local
		do
			if test -f "${d}/include/zlib.h"
			then
				if test ${d} != /usr
				then
					zlib_cflags="-I ${d}/include"
					LIBS="$LIBS -L${d}/lib"
				fi
				have_zlib="yes"
			fi
		done
		;;
	*)
		if test -f "${with_zlib}/include/zlib.h"
		then
			zlib_cflags="-I${with_zlib}/include"
			LIBS="$LIBS -L${with_zlib}/lib"
			have_zlib="yes"
		else
			AC_MSG_ERROR([$with_zlib/include/zlib.h not found.])
		fi
		;;
esac

if test "X${have_zlib}" != "X"
then
	AC_MSG_RESULT(yes)
	AC_SEARCH_LIBS([deflate], [z], [],
		       [AC_MSG_ERROR([found zlib include but not library.])
			have_zlib=""])
elif test "X$with_zlib" = Xyes
then
	AC_MSG_ERROR([include/zlib.h not found.])
else
	AC_MSG_RESULT(no)
fi

ZLIB=
if test "X${have_zlib}" != "X"
then
	CFLAGS="$CFLAGS $zlib_cflags"
	AC_DEFINE(HAVE_ZLIB, 1, [Define if zlib was found])
	ZLIB=1
fi
AC_SUBST(ZLIB)


#
# In solaris 10, SMF can manage named service
#
AC_CHECK_LIB(scf, smf_enable_instance)

#
# Large File
#
AC_ARG_ENABLE(largefile,
	      AS_HELP_STRING([--enable-largefile], [64-bit file support]),
	      want_largefile="yes", want_largefile="no")
case $want_largefile in
	yes)
		ALWAYS_DEFINES="$ALWAYS_DEFINES -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
		;;
	*)
		;;
esac

#
# Additional compiler settings.
#
MKDEPCC="$CC"
MKDEPCFLAGS="-M"

if test "X$GCC" = "Xyes"; then
	STD_CWARNINGS="$STD_CWARNINGS -W -Wall -Wmissing-prototypes -Wcast-qual -Wwrite-strings -Wformat -Wpointer-arith"
	AC_MSG_CHECKING(if "$CC" supports -fno-strict-aliasing)
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -Werror -fno-strict-aliasing"
	AC_TRY_COMPILE(,, [FNOSTRICTALIASING=yes],[FNOSTRICTALIASING=no])
	CFLAGS="$SAVE_CFLAGS"
	if test "yes" = "$FNOSTRICTALIASING"; then
		AC_MSG_RESULT(yes)
		STD_CWARNINGS="$STD_CWARNINGS -fno-strict-aliasing"
	else
		AC_MSG_RESULT(no)
	fi
	#
	# turn off delete null pointer checks
	#
	AC_MSG_CHECKING(if "$CC" supports -fno-delete-null-pointer-checks)
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -Werror -fno-delete-null-pointer-checks"
	AC_TRY_COMPILE(,, [FNODELETENULLPOINTERCHECKS=yes],
			  [FNODELETENULLPOINTERCHECKS=no])
	CFLAGS="$SAVE_CFLAGS"
	if test "yes" = "$FNODELETENULLPOINTERCHECKS"; then
		AC_MSG_RESULT(yes)
		STD_CWARNINGS="$STD_CWARNINGS -fno-delete-null-pointer-checks"
	else
		AC_MSG_RESULT(no)
	fi
	if test "X$enable_warn_shadow" = Xyes; then
		STD_CWARNINGS="$STD_CWARNINGS -Wshadow"
	fi
	if test "X$enable_warn_error" = Xyes; then
		STD_CWARNINGS="$STD_CWARNINGS -Werror"
	fi
else
	case $host in
	*-solaris*)
		MKDEPCFLAGS="-xM"
		;;
	esac
fi

AC_SUBST(MKDEPCC)
AC_SUBST(MKDEPCFLAGS)
AC_SUBST(MKDEPPROG)

#
# -lxnet buys us one big porting headache...  standards, gotta love 'em.
#
# AC_CHECK_LIB(xnet, socket, ,
#    AC_CHECK_LIB(socket, socket)
# )
#
# Use this for now, instead:
#
case "$host" in
	*-linux*)
		;;
	*)
		AC_CHECK_LIB(socket, socket)
		AC_CHECK_LIB(nsl, inet_addr)
		;;
esac

#
# Work around Solaris's select() limitations.
#
case "$host" in
	*-solaris2.[[89]]|*-solaris2.1?)
	AC_DEFINE(FD_SETSIZE, 65536,
		  [Solaris hack to get select_large_fdset.])
	;;
esac

#
# Purify support
#
AC_MSG_CHECKING(whether to use purify)
AC_ARG_WITH(purify,
	    AS_HELP_STRING([--with-purify[=PATH]],[use Rational purify]),
	    use_purify="$withval", use_purify="no")

case "$use_purify" in
	no)
		;;
	yes)
		AC_PATH_PROG(purify_path, purify, purify)
		;;
	*)
		purify_path="$use_purify"
		;;
esac

case "$use_purify" in
	no)
		AC_MSG_RESULT(no)
		PURIFY=""
		;;
	*)
		if test -f "$purify_path" || test purify = "$purify_path"; then
			AC_MSG_RESULT($purify_path)
			PURIFYFLAGS="`echo $PURIFYOPTIONS`"
			PURIFY="$purify_path $PURIFYFLAGS"
		else
			AC_MSG_ERROR([$purify_path not found.

Please choose the proper path with the following command:

    configure --with-purify=PATH
])
		fi
		;;
esac

AC_SUBST(PURIFY)

#
# Google/Great Performance Tools CPU Profiler
#
AC_MSG_CHECKING(whether to use gperftools profiler)
AC_ARG_WITH(gperftools-profiler,
	    AS_HELP_STRING([--with-gperftools-profiler],
			   [use gperftools CPU profiler]),
	    use_profiler="$withval", use_profiler="no")

case $use_profiler in
	yes)
		AC_MSG_RESULT(yes)
		AC_DEFINE([HAVE_GPERFTOOLS_PROFILER], 1,
		[Define to use gperftools CPU profiler.])
		LIBS="$LIBS -lprofiler"
		;;
	*)
		AC_MSG_RESULT(no)
		;;
esac

#
# enable/disable dumping stack backtrace.  Also check if the system supports
# glibc-compatible backtrace() function.
#
AC_ARG_ENABLE([backtrace],
	      [AS_HELP_STRING([--enable-backtrace],
			      [log stack backtrace on abort [default=yes]])],
	      [], [enable_backtrace="yes"])

AS_IF([test "$enable_backtrace" = "yes"],
      [AC_DEFINE([USE_BACKTRACE], [1], [define if we can use backtrace])
       AC_LINK_IFELSE(
	 [AC_LANG_PROGRAM(
	    [[#include <execinfo.h>]],
	    [[return (backtrace((void **)0, 0));]]
	  )],
	 [AC_DEFINE([HAVE_LIBCTRACE], [1], [define if system have backtrace function])]
       )])

AC_ARG_ENABLE(symtable,
	      AS_HELP_STRING([--enable-symtable],
			     [use internal symbol table for backtrace
			      [all|minimal(default)|none]]),
		want_symtable="$enableval",  want_symtable="minimal")
case $want_symtable in
yes|all|minimal)     # "yes" is a hidden value equivalent to "minimal"
	if test "" = "$PERL"
	then
		AC_MSG_ERROR([Internal symbol table requires perl but no perl is found.
Install perl or explicitly disable the feature by --disable-symtable.])
	fi
	if test "yes" = "$use_libtool"; then
		AC_MSG_WARN([Internal symbol table does not work with libtool.  Disabling symbol table.])
	else
		# we generate the internal symbol table only for those systems
		# known to work to avoid unexpected build failure.  Also, warn
		# about unsupported systems when the feature is enabled
		#  manually.
		case $host_os in
		freebsd*|netbsd*|openbsd*|linux*|solaris*|darwin*)
			MKSYMTBL_PROGRAM="$PERL"
			if test "all" = "$want_symtable"; then
				ALWAYS_MAKE_SYMTABLE="yes"
			fi
			;;
		*)
			if test "yes" = "$want_symtable" -o "all" = "$want_symtable"
			then
				AC_MSG_WARN([this system is not known to generate internal symbol table safely; disabling it])
			fi
		esac
	fi
	;;
*)
	;;
esac
AC_SUBST(MKSYMTBL_PROGRAM)
AC_SUBST(ALWAYS_MAKE_SYMTABLE)

#
# File name extension for static archive files, for those few places
# where they are treated differently from dynamic ones.
#
SA=a

AC_SUBST(O)
AC_SUBST(A)
AC_SUBST(SA)
AC_SUBST(LIBTOOL_MKDEP_SED)
AC_SUBST(LIBTOOL_MODE_COMPILE)
AC_SUBST(LIBTOOL_MODE_INSTALL)
AC_SUBST(LIBTOOL_MODE_LINK)
AC_SUBST(LIBTOOL_MODE_UNINSTALL)

BIND9_CO_RULE=".c.$O:"
AC_SUBST(BIND9_CO_RULE)

#
# Here begins a very long section to determine the system's networking
# capabilities.  The order of the tests is significant.
#

#
# We do the IPv6 compilation checking after libtool so that we can put
# the right suffix on the files.
#
AC_MSG_CHECKING([for IPv6 structures])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
     [[
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netinet/in.h>
     ]],
     [[
       struct sockaddr_in6 sin6;
       struct in6_addr in6;
       struct in6_pktinfo in6_pi;
       struct sockaddr_storage storage;
       in6 = in6addr_any;
       in6 = in6addr_loopback;
       sin6.sin6_scope_id = 0;
       return (0);
     ]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_FAILURE([IPv6 support is mandatory])])

#
# Allow forcibly disabling TCP Fast Open support as autodetection might yield
# confusing results on some systems (e.g. FreeBSD; see set_tcp_fastopen()
# comment in lib/isc/unix/socket.c).
#

AC_ARG_ENABLE([tcp_fastopen],
	      [AS_HELP_STRING([--disable-tcp-fastopen],
			      [disable TCP Fast Open support [default=yes]])],
	     [], [enable_tcp_fastopen="yes"])

AS_IF([test "$enable_tcp_fastopen" = "yes"],
      [AC_DEFINE([ENABLE_TCP_FASTOPEN], [1], [define if you want TCP_FASTOPEN enabled if available])])

#
# Check for some other useful functions that are not ever-present.
#
AC_CHECK_FUNCS([strlcpy strlcat])

AC_SUBST(READLINE_LIB)
AC_ARG_WITH(readline,
	    AS_HELP_STRING([--with-readline[=LIBSPEC]],
			   [specify readline library [default auto]]),
	    use_readline="$withval", use_readline="auto")
case "$use_readline" in
no)	;;
*)
	saved_LIBS="$LIBS"
	case "$use_readline" in
	yes|auto) try_readline="-ledit"; or_readline="-lreadline" ;;
	*) try_readline="$use_readline"
	esac
	for readline in "$try_readline" $or_readline
	do
		LIBS="$readline"
		AC_MSG_NOTICE(checking for readline with $readline)
		AC_CHECK_FUNCS(readline)
		if test "yes" = "$ac_cv_func_readline"
		then
			READLINE_LIB="$readline"
			break
		fi
		for lib in -lterminfo -ltermcap -lncurses -lcurses
		do
			AC_MSG_NOTICE(checking for readline with $readline $lib)
			unset ac_cv_func_readline
			LIBS="$readline $lib"
			AC_CHECK_FUNCS(readline)
			if test "yes" = "$ac_cv_func_readline"
			then
				READLINE_LIB="$readline $lib"
				break
			fi
		done
		if test "yes" = "$ac_cv_func_readline"
		then
			break
		fi
	done
	if test "auto" != "$use_readline" &&
	   test "X$READLINE_LIB" = "X"
	then
		AC_MSG_ERROR([The readline library was not found.])
	fi
	LIBS="$saved_LIBS"
	;;
esac
if test "yes" = "$ac_cv_func_readline"
then
	case "$READLINE_LIB" in
	*edit*)
		AC_CHECK_HEADERS(editline/readline.h)
		AC_CHECK_HEADERS(edit/readline/readline.h)
		AC_CHECK_HEADERS(edit/readline/history.h)
		;;
	esac
	AC_CHECK_HEADERS(readline/readline.h)
	AC_CHECK_HEADERS(readline/history.h)
fi

#
# Use our own SPNEGO implementation?
#
AC_ARG_ENABLE(isc-spnego,
	      AS_HELP_STRING([--disable-isc-spnego],
			     [use SPNEGO from GSSAPI library]))

if test -n "$USE_GSSAPI"
then
	case "$enable_isc_spnego" in
		yes|'')
			USE_ISC_SPNEGO='-DUSE_ISC_SPNEGO'
			DST_EXTRA_OBJS="$DST_EXTRA_OBJS spnego.$O"
			DST_EXTRA_SRCS="$DST_EXTRA_SRCS spnego.c"
			AC_MSG_RESULT(using SPNEGO from lib/dns)
			;;
		no)
			AC_MSG_RESULT(using SPNEGO from GSSAPI library)
			;;
	esac
fi

AC_SUBST(USE_ISC_SPNEGO)

AC_SUBST(DST_EXTRA_OBJS)
AC_SUBST(DST_EXTRA_SRCS)

#
# Security Stuff
#
# Note it is very recommended to *not* disable chroot(),
# this is only because chroot() was made obsolete by Posix.
AC_ARG_ENABLE(chroot, AS_HELP_STRING([--disable-chroot], [disable chroot]))
case "$enable_chroot" in
	yes|'')
		AC_CHECK_FUNCS(chroot)
		;;
	no)
		;;
esac

LIBCAP_LIBS=""
AC_MSG_CHECKING([whether to enable Linux capabilities])
AC_ARG_ENABLE([linux-caps],
	      [AS_HELP_STRING([--disable-linux-caps],
			      [disable Linux capabilities])],
	      [],
	      [AS_CASE([$host],
		       [*-linux*],[enable_linux_caps=yes],
		       [enable_linux_caps=no])])

AS_IF([test "$enable_linux_caps" = "yes"],
      [AC_MSG_RESULT([yes])
       AC_CHECK_HEADERS([sys/capability.h],
			[],
			[AC_MSG_ERROR(m4_normalize([sys/capability.h header is required for Linux capabilities support.
						    Either install libcap or use --disable-linux-caps.]))])
       save_LIBS="$LIBS"
       AC_SEARCH_LIBS([cap_set_proc], [cap],
		      [LIBCAP_LIBS="$ac_cv_search_cap_set_proc"],
		      [AC_MSG_ERROR(m4_normalize([libcap is required for Linux capabilities support.
						  Either install libcap or use --disable-linux-caps.]))])
       LIBS="$save_LIBS"],
      [AC_MSG_RESULT([no])])
AC_SUBST([LIBCAP_LIBS])

AC_CHECK_HEADERS(sys/un.h,
ISC_PLATFORM_HAVESYSUNH="#define ISC_PLATFORM_HAVESYSUNH 1"
,
ISC_PLATFORM_HAVESYSUNH="#undef ISC_PLATFORM_HAVESYSUNH"
)
AC_SUBST(ISC_PLATFORM_HAVESYSUNH)

case "$host" in
*-solaris*)
	AC_DEFINE(NEED_SECURE_DIRECTORY, 1,
		  [Define if connect does not honour the permission on the UNIX domain socket.])
	;;
esac

#
# Time Zone Stuff
#
AC_CHECK_FUNCS([tzset])

AC_MSG_CHECKING(for optarg declaration)
AC_TRY_COMPILE([
#include <unistd.h>
],
[optarg = 0;],
[AC_MSG_RESULT(yes)],
[AC_MSG_RESULT(no)
GEN_NEED_OPTARG="-DNEED_OPTARG=1"
AC_DEFINE(NEED_OPTARG, 1, [Defined if extern char *optarg is not declared.])])

#
# Check for nanoseconds in file stats
#
AC_MSG_CHECKING([for st_mtim.tv_nsec])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
     [[#include <sys/fcntl.h>]],
     [[struct stat s;
       return(s.st_mtim.tv_nsec);
      ]])],
   [AC_DEFINE([HAVE_STAT_NSEC], [1], [define if struct stat has st_mtim.tv_nsec field])])

#
# Check for if_nametoindex() for IPv6 scoped addresses support
#
AC_CHECK_FUNCS([if_nametoindex])

AC_CHECK_FUNCS(nanosleep usleep explicit_bzero)

ISC_ATOMIC_LIBS=""
AC_CHECK_HEADERS(
  [stdatomic.h],
  [AC_MSG_CHECKING([for memory model aware atomic operations])
   AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[[#include <stdatomic.h>]],
	[[atomic_int_fast32_t val = 0; atomic_fetch_add_explicit(&val, 1, memory_order_relaxed);]]
      )],
     [AC_MSG_RESULT([stdatomic.h])
      AC_MSG_CHECKING([whether -latomic is needed for 64-bit stdatomic.h functions])
      AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
	   [[#include <stdatomic.h>]],
	   [[atomic_int_fast64_t val = 0; atomic_fetch_add_explicit(&val, 1, memory_order_relaxed);]]
	 )],
	[AC_MSG_RESULT([no])],
	[AC_MSG_RESULT([yes])
	 ISC_ATOMIC_LIBS="-latomic"
	])
     ],
     [AC_MSG_FAILURE([stdatomic.h header found, but compilation failed, fix your toolchaing.])]
   )],
  [AC_MSG_CHECKING([for memory model aware atomic operations])
   AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[[#include <inttypes.h>]],
	[[int32_t val = 0; __atomic_fetch_add(&val, 1, __ATOMIC_RELAXED);]]
      )],
     [AC_MSG_RESULT([__atomic builtins])
      AC_DEFINE([HAVE___ATOMIC], [1], [define if __atomic builtins are not available])
      AC_MSG_CHECKING([whether -latomic is needed for 64-bit __atomic builtins])
      AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
	   [[#include <inttypes.h>]],
	   [[int64_t val = 0; __atomic_fetch_add(&val, 1, __ATOMIC_RELAXED);]]
	 )],
	[AC_MSG_RESULT([no])],
	[AC_MSG_RESULT([yes])
	 ISC_ATOMIC_LIBS="-latomic"
	])
     ],
     [AC_MSG_RESULT([__sync builtins])
     ])
  ])
LIBS="$LIBS $ISC_ATOMIC_LIBS"

AC_CHECK_HEADERS([uchar.h])

#
# Check for __builtin_unreachable
#
AC_MSG_CHECKING([compiler support for __builtin_unreachable()])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
     [[]],
     [[__builtin_unreachable();]]
   )],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_BUILTIN_UNREACHABLE], [1], [define if the compiler supports __builtin_unreachable().])
  ],
  [AC_MSG_RESULT([no])
  ])

#
# Check for __builtin_expect
#
AC_MSG_CHECKING([compiler support for __builtin_expect])
AC_TRY_LINK(, [
	return (__builtin_expect(1, 1) ? 1 : 0);
], [
	have_builtin_expect=yes
	AC_MSG_RESULT(yes)
], [
	have_builtin_expect=no
	AC_MSG_RESULT(no)
])
if test "yes" = "$have_builtin_expect"; then
	AC_DEFINE(HAVE_BUILTIN_EXPECT, 1, [Define to 1 if the compiler supports __builtin_expect.])
fi

#
# Check for __builtin_clz
#
AC_MSG_CHECKING([compiler support for __builtin_clz])
AC_TRY_LINK(, [
	return (__builtin_clz(0xff) == 24 ? 1 : 0);
], [
	have_builtin_clz=yes
	AC_MSG_RESULT(yes)
], [
	have_builtin_clz=no
	AC_MSG_RESULT(no)
])
if test "yes" = "$have_builtin_clz"; then
	AC_DEFINE(HAVE_BUILTIN_CLZ, 1, [Define to 1 if the compiler supports __builtin_clz.])
fi

#
# Activate "rrset-order fixed" or not?
#
AC_ARG_ENABLE(fixed-rrset,
	      AS_HELP_STRING([--enable-fixed-rrset],
			     [enable fixed rrset ordering [default=no]]),
	      enable_fixed="$enableval", enable_fixed="no")
case "$enable_fixed" in
	yes)
		AC_DEFINE(DNS_RDATASET_FIXED, 1,
			  [Define to enable "rrset-order fixed" syntax.])
		;;
	no)
		;;
	*)
		;;
esac

#
# Activate dnstap?
#
AC_ARG_ENABLE(dnstap,
	      AS_HELP_STRING([--enable-dnstap],
			     [enable dnstap support
				(requires fstrm, protobuf-c)]),
	      use_dnstap=$enableval, use_dnstap=no)

DNSTAP=
DNSTAPSRCS=
DNSTAPOBJS=
DNSTAPTARGETS=
DNSTAP_PB_C_H=
if test "x$use_dnstap" != "xno"; then
	AC_ARG_WITH([protobuf-c],
		    AS_HELP_STRING([--with-protobuf-c=path],
				   [Path where protobuf-c is installed, for dnstap]), [
	    # workaround for protobuf-c includes at old dir
	    # before protobuf-c-1.0.0
	    if test -f $withval/include/google/protobuf-c/protobuf-c.h
	    then
		CFLAGS="$CFLAGS -I$withval/include/google"
	    else
		CFLAGS="$CFLAGS -I$withval/include"
	    fi
	    LDFLAGS="$LDFLAGS -L$withval/lib"
	    AC_PATH_PROG([PROTOC_C], [protoc-c], [],
			 [$PATH$PATH_SEPARATOR$withval/bin])
	], [
	    # workaround for protobuf-c includes at old dir
	    # before protobuf-c-1.0.0
	    if test -f /usr/include/google/protobuf-c/protobuf-c.h
	    then
		CFLAGS="$CFLAGS -I/usr/include/google"
	    else
		if test -f /usr/local/include/google/protobuf-c/protobuf-c.h
		then
		    CFLAGS="$CFLAGS -I/usr/local/include/google"
		    LDFLAGS="$LDFLAGS -L/usr/local/lib"
		fi
	    fi
	    AC_PATH_PROG([PROTOC_C],[protoc-c])
	])
	if test -z "$PROTOC_C"; then
		AC_MSG_ERROR([The protoc-c program was not found.])
	fi
	AC_ARG_WITH([libfstrm], AS_HELP_STRING([--with-libfstrm=path],
		    [Path where libfstrm is installed, for dnstap]), [
	    CFLAGS="$CFLAGS -I$withval/include"
	    LDFLAGS="$LDFLAGS -L$withval/lib"
	    AC_PATH_PROG([FSTRM_CAPTURE], [fstrm_capture], [], [$PATH$PATH_SEPARATOR$withval/bin])
	],[
	    AC_PATH_PROG([FSTRM_CAPTURE], [fstrm_capture])
	])

	AC_SEARCH_LIBS([fstrm_iothr_init], [fstrm], [],
		       AC_MSG_ERROR([The fstrm library was not found. Please install fstrm!]))
	AC_SEARCH_LIBS([protobuf_c_message_pack], [protobuf-c], [],
		       AC_MSG_ERROR([The protobuf-c library was not found. Please install protobuf-c!]))

	AC_DEFINE(HAVE_DNSTAP, 1, [Define to 1 to enable dnstap support])
	DNSTAP=dnstap
	DNSTAPSRCS='${DNSTAPSRCS}'
	DNSTAPOBJS='${DNSTAPOBJS}'
	DNSTAPTARGETS='${DNSTAPTARGETS}'
	DNSTAP_PB_C_H="dnstap.pb-c.h"
fi
AC_SUBST(DNSTAP)
AC_SUBST(DNSTAPSRCS)
AC_SUBST(DNSTAPOBJS)
AC_SUBST(DNSTAPTARGETS)
AC_SUBST(DNSTAP_PB_C_H)

#
#  The following sets up how non-blocking i/o is established.
#  cygwin and solaris 2.x (x<5) require special handling.
#
case "$host" in
*-cygwin*) AC_DEFINE(PORT_NONBLOCK, O_NDELAY);;
*-solaris2.[[01234]])
	AC_DEFINE(PORT_NONBLOCK, O_NONBLOCK)
	AC_DEFINE(USE_FIONBIO_IOCTL, 1,
		  [Defined if you need to use ioctl(FIONBIO) instead a fcntl call to make non-blocking.])
	;;
*) AC_DEFINE(PORT_NONBLOCK, O_NONBLOCK,
	     [Sets which flag to pass to open/fcntl to make non-blocking (O_NDELAY/O_NONBLOCK).])
	;;
esac
#
# Solaris 2.5.1 and earlier cannot bind() then connect() a TCP socket.
# This prevents the source address being set.
#
case "$host" in
*-solaris2.[[012345]]|*-solaris2.5.1)
	AC_DEFINE(BROKEN_TCP_BIND_BEFORE_CONNECT, 1,
		  [Define if you cannot bind() before connect() for TCP sockets.])
	;;
esac
#
# The following sections deal with tools used for formatting
# the documentation.  They are all optional, unless you are
# a developer editing the documentation source.
#

#
# Look for TeX.
#

AC_PATH_PROGS(LATEX, latex, latex)
AC_SUBST(LATEX)

AC_PATH_PROGS(PDFLATEX, pdflatex, pdflatex)
AC_SUBST(PDFLATEX)

AC_PATH_PROGS(DBLATEX, dblatex, dblatex)
AC_SUBST(DBLATEX)

#
# Look for w3m
#

AC_PATH_PROGS(W3M, w3m, w3m)
AC_SUBST(W3M)

#
# Look for pandoc
#
AC_PATH_PROG(PANDOC, pandoc, pandoc)
AC_SUBST(PANDOC)


#
# Look for xsltproc (libxslt)
#

AC_PATH_PROG(XSLTPROC, xsltproc, xsltproc)
AC_SUBST(XSLTPROC)

#
# Look for xmllint (libxml2)
#

AC_PATH_PROG(XMLLINT, xmllint, xmllint)
AC_SUBST(XMLLINT)

#
# Look for Doxygen
#

AC_PATH_PROG(DOXYGEN, doxygen, doxygen)
AC_SUBST(DOXYGEN)

#
# Look for curl
#

AC_PATH_PROG(CURL, curl, curl)
AC_SUBST(CURL)

#
# Subroutine for searching for an ordinary file (e.g., a stylesheet)
# in a number of directories:
#
#   NOM_PATH_FILE(VARIABLE, FILENAME, DIRECTORIES)
#
# If the file FILENAME is found in one of the DIRECTORIES, the shell
# variable VARIABLE is defined to its absolute pathname.  Otherwise,
# it is set to FILENAME, with no directory prefix (that's not terribly
# useful, but looks less confusing in substitutions than leaving it
# empty).  The variable VARIABLE will be substituted into output files.
#

AC_DEFUN([NOM_PATH_FILE], [
$1=""
AC_MSG_CHECKING(for $2)
for d in $3
do
	f=$d/$2
	if test -f $f
	then
		$1=$f
		AC_MSG_RESULT($f)
		break
	fi
done
if test "X[$]$1" = "X"
then
	AC_MSG_RESULT("not found");
	$1=$2
fi
AC_SUBST($1)
])

#
# Look for Docbook-XSL stylesheets.  Location probably varies by system.
# If it's not explicitly specified, guess where it might be found, based on
# where SGML stuff lives on some systems (FreeBSD is the only one we're sure
# of at the moment).
#
AC_MSG_CHECKING(for Docbook-XSL path)
AC_ARG_WITH(docbook-xsl,
	    AS_HELP_STRING([--with-docbook-xsl[=PATH]],
			   [specify path for Docbook-XSL stylesheets]),
   docbook_path="$withval", docbook_path="auto")
case "$docbook_path" in
auto)
	AC_MSG_RESULT(auto)
	docbook_xsl_trees="/usr/pkg/share/xsl/docbook /usr/local/share/xsl/docbook-ns /usr/local/share/xsl/docbook /usr/share/xsl/docbook /opt/local/share/xsl/docbook-xsl /opt/local/share/xsl/docbook-xsl-nons /usr/share/xml/docbook/stylesheet/docbook-xsl"
	;;
*)
	docbook_xsl_trees="$withval"
	AC_MSG_RESULT($docbook_xsl_trees)
	;;
esac

#
# Look for stylesheets we need.
#

NOM_PATH_FILE(XSLT_DOCBOOK_STYLE_HTML, html/docbook.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_STYLE_XHTML, xhtml/docbook.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_STYLE_MAN, manpages/docbook.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_CHUNK_HTML, html/chunk.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_CHUNK_XHTML, xhtml/chunk.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_CHUNKTOC_HTML, html/chunktoc.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_CHUNKTOC_XHTML, xhtml/chunktoc.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_MAKETOC_HTML, html/maketoc.xsl, $docbook_xsl_trees)
NOM_PATH_FILE(XSLT_DOCBOOK_MAKETOC_XHTML, xhtml/maketoc.xsl, $docbook_xsl_trees)

#
# Same dance for dblatex
#
dblatex_xsl_trees="/usr/local/share/xml/docbook/stylesheet/dblatex /usr/pkg/share/xml/docbook/stylesheet/dblatex /usr/share/xml/docbook/stylesheet/dblatex"
NOM_PATH_FILE(XSLT_DBLATEX_STYLE, xsl/docbook.xsl, $dblatex_xsl_trees)
NOM_PATH_FILE(XSLT_DBLATEX_FASTBOOK, xsl/latex_book_fast.xsl, $dblatex_xsl_trees)

#
# IDN support using libidn2
#

LIBIDN2_CFLAGS=
LIBIDN2_LDFLAGS=
LIBIDN2_LIBS=
AC_ARG_WITH([libidn2],
	    [AS_HELP_STRING([--with-libidn2[=PATH]], [enable IDN support using GNU libidn2 [yes|no(default)|path]])],
	    [with_libidn2="$withval"], [with_libidn2="no"])
AS_CASE([$with_libidn2],
	[yes],	[PKG_CHECK_MODULES([LIBIDN2], [libidn2],
				   [AC_DEFINE([HAVE_LIBIDN2], [1], [Define if libidn2 was found])])],
	[no],	[],
	[*],	[save_CFLAGS="$CFLAGS"
		 save_CPPFLAGS="$CPPFLAGS"
		 save_LDFLAGS="$LDFLAGS"
		 save_LIBS="$LIBS"
		 LIBIDN2_CFLAGS="-I$with_libidn2/include"
		 LIBIDN2_LDFLAGS="-L$with_libidn2/lib"
		 CFLAGS="$LIBIDN2_CFLAGS $CFLAGS"
		 CPPFLAGS="$LIBIDN2_CFLAGS $CPPFLAGS"
		 LDFLAGS="$LIBIDN2_LDFLAGS $LDFLAGS"
		 AC_CHECK_HEADERS([idn2.h],
				  [],
				  [AC_MSG_ERROR([idn2.h not found])])
		 AC_SEARCH_LIBS([idn2_to_ascii_lz], [idn2],
				[LIBIDN2_LIBS="$ac_cv_search_idn2_to_ascii_lz"
				 AC_DEFINE([HAVE_LIBIDN2], [1], [Define if libidn2 was found])],
				[AC_MSG_ERROR([libidn2 requested, but not found])])
		 CFLAGS="$save_CFLAGS"
		 CPPFLAGS="$save_CPPFLAGS"
		 LDFLAGS="$save_LDFLAGS"
		 LIBS="$save_LIBS"])
AC_SUBST([LIBIDN2_CFLAGS])
AC_SUBST([LIBIDN2_LDFLAGS])
AC_SUBST([LIBIDN2_LIBS])

#
# Check whether to build with cmocka unit testing framework
#

AC_ARG_WITH([cmocka],
	    [AS_HELP_STRING([--with-cmocka=detect],[enable CMocka based tests (default is detect)])],
	    [],[with_cmocka=detect])

AS_CASE([$with_cmocka],
	[no],[],
	[detect],[PKG_CHECK_MODULES([CMOCKA], [cmocka >= 1.0.0],
				    [AC_DEFINE([HAVE_CMOCKA], [1], [Use CMocka])
				     UNITTESTS=tests],[with_cmocka=no])],
	[yes],[PKG_CHECK_MODULES([CMOCKA], [cmocka >= 1.0.0],
				 [AC_DEFINE([HAVE_CMOCKA], [1], [Use CMocka])])
	                          UNITTESTS=tests],
	[AC_MSG_ERROR([Use PKG_CONFIG_PATH to specify path to CMocka library])]
       )
AC_SUBST([CMOCKA_CFLAGS])
AC_SUBST([CMOCKA_LIBS])
AC_SUBST(UNITTESTS)

#
# Check for kyua execution engine if CMocka was requested
# and bail out if execution engine was not found
#
AC_ARG_VAR([KYUA], [path to kyua execution engine])
AS_IF([test "$with_cmocka" != "no"],
      [AC_PATH_PROGS([KYUA], [kyua], [])
       AS_IF([test -z "$KYUA"],
            [AC_MSG_WARN([kyua test execution engine not found])])])

AC_SUBST([KYUA])

#
# Check for -Wl,--wrap= support
#

save_LDFLAGS=$LDFLAGS
LDFLAGS="--wrap=printf"

LD_WRAP_TESTS=false
enable_ld_wrap=no
AC_MSG_CHECKING([for linker support for --wrap option])
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <stdio.h>], [__wrap_printf("success"); return (0);])],
    [enable_ld_wrap=yes
     LD_WRAP_TESTS=true
     AC_DEFINE([LD_WRAP], [1], [define if the linker supports --wrap option])
     AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])])
# AM_CONDITIONAL([LD_WRAP], [test $enable_ld_wrap = yes])
AC_SUBST([LD_WRAP_TESTS])

LDFLAGS=$save_LDFLAGS

#
# Check for i18n
#
AC_CHECK_HEADERS(locale.h)
AC_CHECK_FUNCS(setlocale)

#
# was --with-tuning specified?
#
AC_ARG_WITH(tuning,
	    AS_HELP_STRING([--with-tuning=ARG],
			   [Specify server tuning (large or default)]),
	    use_tuning="$withval", use_tuning="no")

case "$use_tuning" in
	large)
		AC_DEFINE(TUNE_LARGE, 1, [Define to use large-system tuning.])
		AC_MSG_RESULT(using large-system tuning)
		;;
	no|default)
		AC_MSG_RESULT(using default tuning)
		;;
	yes|*)
		AC_MSG_ERROR([You must specify "large" or "default" for --with-tuning.])
		;;
esac

#
# was --enable-querytrace specified?
#
AC_ARG_ENABLE(querytrace,
	      AS_HELP_STRING([--enable-querytrace],
			     [enable very verbose query trace logging
				[default=no]]),
	      want_querytrace="$enableval", want_querytrace="no")

AC_MSG_CHECKING([whether to enable query trace logging])
case "$want_querytrace" in
yes)
	AC_MSG_RESULT(yes)
	AC_DEFINE(WANT_QUERYTRACE, 1, [Define to enable very verbose query trace logging.])
	;;
no)
	AC_MSG_RESULT(no)
	;;
*)
	AC_MSG_ERROR("--enable-querytrace requires yes or no")
	;;
esac

#
# Was --disable-auto-validation specified?
#
validation_default=auto
AC_ARG_ENABLE(auto-validation,
	      AS_HELP_STRING([--enable-auto-validation],
			     [turn on DNSSEC validation by default, using the IANA root key [default=yes]]),
	      [:],[enable_auto_validation=yes])
AS_IF([test "$enable_auto_validation" = "no"],[validation_default=yes])
AC_DEFINE_UNQUOTED([VALIDATION_DEFAULT], ["$validation_default"], [the default value of dnssec-validation option])

#
# Substitutions
#
AC_SUBST(BIND9_TOP_BUILDDIR)
BIND9_TOP_BUILDDIR=`pwd`

AC_SUBST(BIND9_ISC_BUILDINCLUDE)
AC_SUBST(BIND9_ISCCC_BUILDINCLUDE)
AC_SUBST(BIND9_ISCCFG_BUILDINCLUDE)
AC_SUBST(BIND9_DNS_BUILDINCLUDE)
AC_SUBST(BIND9_NS_BUILDINCLUDE)
AC_SUBST(BIND9_BIND9_BUILDINCLUDE)
AC_SUBST(BIND9_IRS_BUILDINCLUDE)
if test "X$srcdir" != "X"; then
	BIND9_ISC_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/isc/include"
	BIND9_ISCCC_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/isccc/include"
	BIND9_ISCCFG_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/isccfg/include"
	BIND9_DNS_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/dns/include"
	BIND9_NS_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/ns/include"
	BIND9_BIND9_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/bind9/include"
	BIND9_IRS_BUILDINCLUDE="-I${BIND9_TOP_BUILDDIR}/lib/irs/include"
else
	BIND9_ISC_BUILDINCLUDE=""
	BIND9_ISCCC_BUILDINCLUDE=""
	BIND9_ISCCFG_BUILDINCLUDE=""
	BIND9_DNS_BUILDINCLUDE=""
	BIND9_NS_BUILDINCLUDE=""
	BIND9_BIND9_BUILDINCLUDE=""
	BIND9_IRS_BUILDINCLUDE=""
fi

AC_SUBST_FILE(BIND9_MAKE_INCLUDES)
BIND9_MAKE_INCLUDES=$BIND9_TOP_BUILDDIR/make/includes

AC_SUBST_FILE(BIND9_MAKE_RULES)
BIND9_MAKE_RULES=$BIND9_TOP_BUILDDIR/make/rules

. "$srcdir/version"
BIND9_PRODUCT="PRODUCT=\"${PRODUCT}\""
AC_SUBST(BIND9_PRODUCT)
BIND9_DESCRIPTION="DESCRIPTION=\"${DESCRIPTION}\""
AC_SUBST(BIND9_DESCRIPTION)
BIND9_VERSION="${MAJORVER}.${MINORVER}${PATCHVER:+.}${PATCHVER}${RELEASETYPE}${RELEASEVER}${EXTENSIONS}"
AC_SUBST(BIND9_VERSION)
BIND9_MAJOR="MAJOR=${MAJORVER}.${MINORVER}"
AC_SUBST(BIND9_MAJOR)
BIND9_VERSIONSTRING="${PRODUCT} ${MAJORVER}.${MINORVER}${PATCHVER:+.}${PATCHVER}${RELEASETYPE}${RELEASEVER}${EXTENSIONS}${DESCRIPTION:+ }${DESCRIPTION}"
AC_SUBST(BIND9_VERSIONSTRING)
BIND9_VERSIONSHORT="${PRODUCT} ${MAJORVER}.${MINORVER}${PATCHVER:+.}${PATCHVER}${RELEASETYPE}${RELEASEVER}${EXTENSIONS}"
AC_SUBST(BIND9_VERSIONSHORT)

BIND9_SRCID="SRCID=unset_id"
if test -f "${srcdir}/srcid"; then
	. "${srcdir}/srcid"
	BIND9_SRCID="SRCID=$SRCID"
elif test -d "${srcdir}/.git"; then
	BIND9_SRCID="SRCID="`(cd "${srcdir}";git rev-parse --short HEAD)`
fi

AC_SUBST(BIND9_SRCID)

if test -z "$ac_configure_args"; then
	BIND9_CONFIGARGS="defaults"
else
	for a in $ac_configure_args
	do
		BIND9_CONFIGARGS="$BIND9_CONFIGARGS $a"
	done
fi
BIND9_CONFIGARGS="`echo $BIND9_CONFIGARGS | sed 's/^ //'`"
BIND9_CONFIGARGS="CONFIGARGS=${BIND9_CONFIGARGS}"
AC_SUBST(BIND9_CONFIGARGS)

AC_SUBST_FILE(LIBISC_API)
LIBISC_API="$srcdir/lib/isc/api"

AC_SUBST_FILE(LIBISCCC_API)
LIBISCCC_API="$srcdir/lib/isccc/api"

AC_SUBST_FILE(LIBISCCFG_API)
LIBISCCFG_API="$srcdir/lib/isccfg/api"

AC_SUBST_FILE(LIBDNS_API)
LIBDNS_API="$srcdir/lib/dns/api"

AC_SUBST_FILE(LIBDNS_MAPAPI)
LIBDNS_MAPAPI="$srcdir/lib/dns/mapapi"

AC_SUBST_FILE(LIBBIND9_API)
LIBBIND9_API="$srcdir/lib/bind9/api"

AC_SUBST_FILE(LIBIRS_API)
LIBIRS_API="$srcdir/lib/irs/api"

AC_SUBST_FILE(LIBNS_API)
LIBNS_API="$srcdir/lib/ns/api"

#
# Configure any DLZ drivers.
#
# If config.dlz.in selects one or more DLZ drivers, it will set
# CONTRIB_DLZ to a non-empty value, which will be our clue to
# build DLZ drivers in contrib.
#
# This section has to come after the libtool stuff because it needs to
# know how to name the driver object files.
#

CONTRIB_DLZ=""
DLZ_DRIVER_INCLUDES=""
DLZ_DRIVER_LIBS=""
DLZ_DRIVER_SRCS=""
DLZ_DRIVER_OBJS=""
DLZ_SYSTEM_TEST=""
DLZ_DRIVER_MYSQL_INCLUDES=""
DLZ_DRIVER_MYSQL_LIBS=""

#
# Configure support for building a shared library object
#
# Even when libtool is available it can't always be relied upon
# to build an object that can be dlopen()'ed, but this is necessary
# for building the dlzexternal system test, so we'll try it the
# old-fashioned way.
#
SO="so"
SO_CFLAGS=""
SO_LDFLAGS=""
SO_LD=""
SO_TARGETS=""
SO_STRIP="cat"

AC_ARG_WITH([dlopen],
	    AS_HELP_STRING([--with-dlopen=ARG],
			   [support dynamically loadable DLZ and DYNDB drivers]),
	    [], [with_dlopen="auto"])


#
# If PIC is disabled, dlopen must also be
#
AS_IF([test "$pic_mode" = "no"],
      [AS_CASE([$with_dlopen],
	       [auto],[with_dlopen="no"],
	       [yes],[AC_MSG_ERROR([--with-dlopen requires PIC])])])

AS_CASE([$with_dlopen],
	[auto|yes],[
	  AC_SEARCH_LIBS([dlopen],[dl])
	  AC_CHECK_FUNCS([dlopen dlclose dlsym],
			 [with_dlopen="yes"],
			 [with_dlopen="no"])
	])

AS_IF([test "$with_dlopen" = "yes"],
      [AS_CASE([$host],
	       [*-linux*|*-gnu*],[
		 LDFLAGS="${LDFLAGS} -Wl,--export-dynamic"
		 SO_CFLAGS="-fPIC"
		 SO_LDFLAGS=""
		 AS_IF([test "$use_libtool" = "yes"],[
			 SO_LDFLAGS="-Xcompiler -shared"
			 SO_LD="${CC}"
		       ],[
			 SO_LDFLAGS="-shared"
			 SO_LD="${CC}"
		       ])
	       ],
	       [*-freebsd*|*-openbsd*|*-netbsd*],[
		 LDFLAGS="${LDFLAGS} -Wl,-E"
		 SO_CFLAGS="-fpic"
		 AS_IF([test "$use_libtool" = "yes"],[
			 SO_LDFLAGS="-Xcompiler -shared"
			 SO_LD="${CC}"
		       ],[
			 SO_LDFLAGS="-shared"
			 SO_LD="${CC}"
		       ])
	       ],
	       [*-darwin*],[
		 SO_CFLAGS="-fPIC"
                 SO_LD="${CC}"
		 AS_IF([test "$use_libtool" = "yes"],[
			 SO_LDFLAGS="-Xcompiler -dynamiclib -undefined dynamic_lookup"
		       ],[
			 SO_LDFLAGS="-dynamiclib -undefined dynamic_lookup"
		       ])
	       ],
	       [*-solaris*],[
		 SO_CFLAGS="-KPIC"
		 SO_LDFLAGS="-G -z text"
		 SO_LD="ld"
	       ],
	       [
		 SO_CFLAGS="-fPIC"
	       ])
       AS_IF([test "$GCC" = "yes"],[
	       SO_CFLAGS="-fPIC"
	       AS_IF([test -z "$SO_LD"],
		     [AS_IF([test "$use_libtool" = "yes"],[
			      SO_LDFLAGS="-Xcompiler -shared"
			      SO_LD="${CC}"
			    ],[
			      SO_LDFLAGS="-shared"
			      SO_LD="${CC}"
			    ])
		     ])
	     ])
       # If we still don't know how to make shared objects, don't make any.
       AS_IF([test -n "$SO_LD"],
	     [SO_TARGETS="\${SO_TARGETS}"
	      AC_DEFINE([ISC_DLZ_DLOPEN], [1],
			[Define to allow building of objects for dlopen().])
	     ])
      ])

CFLAGS="$CFLAGS $SO_CFLAGS"

AC_SUBST(SO)
AC_SUBST(SO_CFLAGS)
AC_SUBST(SO_LDFLAGS)
AC_SUBST(SO_LD)
AC_SUBST(SO_STRIP)
AC_SUBST(SO_TARGETS)

#
# Response policy rewriting using DNS Response Policy Service (DNSRPS)
# interface.
#
# DNSRPS can be compiled into BIND everywhere with a reasonably
# modern C compiler.  It is enabled on systems with dlopen() and librpz.so.
#
dnsrps_avail=yes
AC_MSG_CHECKING([for librpz __attribute__s])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
     [[]],
     [[
       extern void f(char *p __attribute__((unused)), ...)
       __attribute__((format(printf,1,2))) __attribute__((__noreturn__));
     ]])],
  [
    librpz_have_attr=yes
    AC_DEFINE([LIBRPZ_HAVE_ATTR], [1], [have __attribute__s used in librpz.h])
    AC_MSG_RESULT([yes])
  ],[
    librpz_have_attr=no
    AC_MSG_RESULT([no])
  ])

AC_ARG_ENABLE([dnsrps-dl],
	      [AS_HELP_STRING([--enable-dnsrps-dl],
			      [DNS Response Policy Service delayed link
			       [default=$librpz_dl]])],
	      [enable_librpz_dl="$enableval"], [enable_librpz_dl="$with_dlopen"])

AS_IF([test "$enable_librpz_dl" = "yes" -a "$with_dlopen" = "no"],
      [AC_MSG_ERROR([DNS Response Policy Service delayed link requires dlopen to be enabled])])

AC_ARG_WITH([dnsrps-libname],
	    [AS_HELP_STRING([--with-dnsrps-libname],
			    [DNSRPS provider library name (librpz.so)])],
	    [librpz_name="$withval"], [librpz_name="librpz.so"])
AC_ARG_WITH([dnsrps-dir],
	    [AS_HELP_STRING([--with-dnsrps-dir],
			    [path to DNSRPS provider library])],
	    [librpz_path="$withval/$librpz_name"], [librpz_path="$librpz_name"])
AC_DEFINE_UNQUOTED([DNSRPS_LIBRPZ_PATH], ["$librpz_path"],
		   [dnsrps $librpz_name])
AS_IF([test "$enable_librpz_dl" = "yes"],
      [
	dnsrps_lib_open=2
      ],[
	dnsrps_lib_open=1
	# Add librpz.so to linked libraries if we are not using dlopen()
	AC_SEARCH_LIBS([librpz_client_create], [rpz], [],
		[dnsrps_lib_open=0
		 dnsrps_avail=no])
      ])
AC_DEFINE_UNQUOTED([DNSRPS_LIB_OPEN], [$dnsrps_lib_open],
		   [0=no DNSRPS  1=static link  2=dlopen()])

AC_ARG_ENABLE([dnsrps],
	      AS_HELP_STRING([--enable-dnsrps],
			     [enable DNS Response Policy Service API]),
	      [enable_dnsrps=$enableval], [enable_dnsrps=no])

AS_IF([test "$enable_dnsrps" != "no"],[
	AS_IF([test "$dnsrps_avail" != "yes"],
	      [AC_MSG_ERROR([dlopen and librpz.so needed for DNSRPS])])
	AS_IF([test "$dnsrps_lib_open" = "0"],
	      [AC_MSG_ERROR([dlopen and librpz.so needed for DNSRPS])])
	AC_DEFINE([USE_DNSRPS], [1], [Enable DNS Response Policy Service API])
      ])

sinclude(contrib/dlz/config.dlz.in)
AC_MSG_CHECKING(contributed DLZ drivers)

if test -n "$CONTRIB_DLZ"
then
	AC_MSG_RESULT(yes)
	DLZ_DRIVER_RULES=contrib/dlz/drivers/rules
	AC_CONFIG_FILES([$DLZ_DRIVER_RULES
		contrib/dlz/modules/mysql/Makefile
		contrib/dlz/modules/mysqldyn/Makefile])
else
	AC_MSG_RESULT(no)
	DLZ_DRIVER_RULES=/dev/null
fi

AC_SUBST(CONTRIB_DLZ)
AC_SUBST(DLZ_DRIVER_INCLUDES)
AC_SUBST(DLZ_DRIVER_LIBS)
AC_SUBST(DLZ_DRIVER_SRCS)
AC_SUBST(DLZ_DRIVER_OBJS)
AC_SUBST(DLZ_SYSTEM_TEST)
AC_SUBST(DLZ_DRIVER_MYSQL_INCLUDES)
AC_SUBST(DLZ_DRIVER_MYSQL_LIBS)
AC_SUBST_FILE(DLZ_DRIVER_RULES)

if test "yes" = "$cross_compiling"; then
	if test -z "$BUILD_CC"; then
		AC_MSG_ERROR([BUILD_CC not set])
	fi
	BUILD_CFLAGS="$BUILD_CFLAGS"
	BUILD_CPPFLAGS="$BUILD_CPPFLAGS"
	BUILD_LDFLAGS="$BUILD_LDFLAGS"
	BUILD_LIBS="$BUILD_LIBS"
else
	BUILD_CC="$CC"
	BUILD_CFLAGS="$CFLAGS"
	BUILD_CPPFLAGS="$CPPFLAGS $GEN_NEED_OPTARG"
	BUILD_LDFLAGS="$LDFLAGS"
	BUILD_LIBS="$LIBS"
fi

NEWFLAGS=""
for e in $BUILD_LDFLAGS ; do
    case $e in
	-L*)
	    case $host_os in
		netbsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		freebsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		solaris*)
		    ee=`echo $e | sed -e 's%^-L%-R%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		*)
		    NEWFLAGS="$NEWFLAGS $e"
		    ;;
		esac
	    ;;
	*)
	    NEWFLAGS="$NEWFLAGS $e"
	    ;;
    esac
done
BUILD_LDFLAGS="$NEWFLAGS"

NEWFLAGS=""
for e in $DNS_GSSAPI_LIBS ; do
    case $e in
	-L*)
	    case $host_os in
		netbsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		freebsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		solaris*)
		    ee=`echo $e | sed -e 's%^-L%-R%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		*)
		    NEWFLAGS="$NEWFLAGS $e"
		    ;;
		esac
	    ;;
	*)
	    NEWFLAGS="$NEWFLAGS $e"
	    ;;
    esac
done
DNS_GSSAPI_LIBS="$NEWFLAGS"

NEWFLAGS=""
for e in $ISC_OPENSSL_LIBS ; do
    case $e in
	-L*)
	    case $host_os in
		netbsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		freebsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		solaris*)
		    ee=`echo $e | sed -e 's%^-L%-R%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		*)
		    NEWFLAGS="$NEWFLAGS $e"
		    ;;
		esac
	    ;;
	*)
	    NEWFLAGS="$NEWFLAGS $e"
	    ;;
    esac
done
ISC_OPENSSL_LIBS="$NEWFLAGS"

NEWFLAGS=""
for e in $DNS_CRYPTO_LIBS ; do
    case $e in
	-L*)
	    case $host_os in
		netbsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		freebsd*)
		    ee=`echo $e | sed -e 's%^-L%-Wl,-rpath,%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		solaris*)
		    ee=`echo $e | sed -e 's%^-L%-R%'`
		    NEWFLAGS="$NEWFLAGS $e $ee"
		    ;;
		*)
		    NEWFLAGS="$NEWFLAGS $e"
		    ;;
		esac
	    ;;
	*)
	    NEWFLAGS="$NEWFLAGS $e"
	    ;;
    esac
done
DNS_CRYPTO_LIBS="$NEWFLAGS"

AC_SUBST(BUILD_CC)
AC_SUBST(BUILD_CFLAGS)
AC_SUBST(BUILD_CPPFLAGS)
AC_SUBST(BUILD_LDFLAGS)
AC_SUBST(BUILD_LIBS)

#
# Commands to run at the end of config.status.
# Don't just put these into configure, it won't work right if somebody
# runs config.status directly (which autoconf allows).
#

AC_CONFIG_COMMANDS(
	[chmod],
	[chmod a+x isc-config.sh doc/doxygen/doxygen-input-filter])

#
# Files to configure.  These are listed here because we used to
# specify them as arguments to AC_OUTPUT.  It's (now) ok to move these
# elsewhere if there's a good reason for doing so.
#

AC_CONFIG_FILES([
	make/Makefile
	make/mkdep
	Makefile
	bin/Makefile
	bin/check/Makefile
	bin/confgen/Makefile
	bin/confgen/unix/Makefile
	bin/delv/Makefile
	bin/dig/Makefile
	bin/dnssec/Makefile
	bin/named/Makefile
	bin/named/unix/Makefile
	bin/nsupdate/Makefile
	bin/pkcs11/Makefile
	bin/plugins/Makefile
	bin/python/Makefile
	bin/python/isc/Makefile
	bin/python/isc/utils.py
	bin/python/isc/tests/Makefile
	bin/python/dnssec-checkds.py
	bin/python/dnssec-coverage.py
	bin/python/dnssec-keymgr.py
	bin/python/isc/__init__.py
	bin/python/isc/checkds.py
	bin/python/isc/coverage.py
	bin/python/isc/dnskey.py
	bin/python/isc/eventlist.py
	bin/python/isc/keydict.py
	bin/python/isc/keyevent.py
	bin/python/isc/keymgr.py
	bin/python/isc/keyseries.py
	bin/python/isc/keyzone.py
	bin/python/isc/policy.py
	bin/python/isc/rndc.py
	bin/python/isc/tests/dnskey_test.py
	bin/python/isc/tests/policy_test.py
	bin/rndc/Makefile
	bin/tests/Makefile
	bin/tests/headerdep_test.sh
	bin/tests/optional/Makefile
	bin/tests/pkcs11/Makefile
	bin/tests/pkcs11/benchmarks/Makefile
	bin/tests/system/Makefile
	bin/tests/system/conf.sh
	bin/tests/system/dlz/prereq.sh
	bin/tests/system/dlzexternal/Makefile
	bin/tests/system/dlzexternal/ns1/dlzs.conf
	bin/tests/system/dyndb/Makefile
	bin/tests/system/dyndb/driver/Makefile
	bin/tests/system/pipelined/Makefile
	bin/tests/system/rndc/Makefile
	bin/tests/system/rpz/Makefile
	bin/tests/system/rsabigexponent/Makefile
	bin/tests/system/tkey/Makefile
	bin/tests/virtual-time/Makefile
	bin/tests/virtual-time/conf.sh
	bin/tools/Makefile
	contrib/scripts/check-secure-delegation.pl
	contrib/scripts/zone-edit.sh
	doc/Makefile
	doc/arm/Makefile
	doc/arm/noteversion.xml
	doc/arm/pkgversion.xml
	doc/arm/releaseinfo.xml
	doc/doxygen/Doxyfile
	doc/doxygen/Makefile
	doc/doxygen/doxygen-input-filter
	doc/misc/Makefile
	doc/tex/Makefile
	doc/tex/armstyle.sty
	doc/xsl/Makefile
	doc/xsl/isc-docbook-chunk.xsl
	doc/xsl/isc-docbook-html.xsl
	doc/xsl/isc-manpage.xsl
	doc/xsl/isc-notes-html.xsl
	isc-config.sh
	lib/Makefile
	lib/bind9/Makefile
	lib/bind9/include/Makefile
	lib/bind9/include/bind9/Makefile
	lib/dns/Makefile
	lib/dns/include/Makefile
	lib/dns/include/dns/Makefile
	lib/dns/include/dst/Makefile
	lib/dns/tests/Makefile
	lib/irs/Makefile
	lib/irs/include/Makefile
	lib/irs/include/irs/Makefile
	lib/irs/include/irs/netdb.h
	lib/irs/include/irs/platform.h
	lib/irs/tests/Makefile
	lib/isc/pthreads/Makefile
	lib/isc/pthreads/include/Makefile
	lib/isc/pthreads/include/isc/Makefile
	lib/isc/Makefile
	lib/isc/include/Makefile
	lib/isc/include/isc/Makefile
	lib/isc/include/isc/platform.h
	lib/isc/include/pk11/Makefile
	lib/isc/include/pkcs11/Makefile
	lib/isc/tests/Makefile
	lib/isc/unix/Makefile
	lib/isc/unix/include/Makefile
	lib/isc/unix/include/isc/Makefile
	lib/isc/unix/include/pkcs11/Makefile
	lib/isccc/Makefile
	lib/isccc/include/Makefile
	lib/isccc/include/isccc/Makefile
	lib/isccc/tests/Makefile
	lib/isccfg/Makefile
	lib/isccfg/include/Makefile
	lib/isccfg/include/isccfg/Makefile
	lib/isccfg/tests/Makefile
	lib/ns/Makefile
	lib/ns/include/Makefile
	lib/ns/include/ns/Makefile
	lib/ns/tests/Makefile
	lib/samples/Makefile
	lib/samples/Makefile-postinstall
	unit/unittest.sh
	fuzz/Makefile
])

#
# Do it
#

AC_OUTPUT

#
# Now that the Makefiles exist we can ensure that everything is rebuilt.
#
AC_ARG_WITH(make-clean,
	    AS_HELP_STRING([--with-make-clean],
			   [run "make clean" at end of configure [yes|no]]),
    make_clean="$withval", make_clean="yes")
case "$make_clean" in
yes)
	if test "yes" != "$no_create"
	then
		if test "yes" = "$silent"
		then
			make clean > /dev/null
		else
			make clean
		fi
	fi
	;;
esac

AC_ARG_ENABLE(full-report,
	      AS_HELP_STRING([--enable-full-report],
			     [report values of all configure options]))

report() {
    echo "==============================================================================="
    echo "Configuration summary:"
    echo "-------------------------------------------------------------------------------"
    echo "Optional features enabled:"
    if test "yes" = "$enable_full_report" -o "standard" = "$locktype"; then
	echo "        Mutex lock type: $locktype"
    fi
    test "large" = "$use_tuning" && echo "    Large-system tuning (--with-tuning)"
    test "no" = "$use_dnstap" || \
	    echo "    Allow 'dnstap' packet logging (--enable-dnstap)"
    test "no" = "$use_geoip" || echo "    GeoIP access control (--with-geoip)"
    test "no" = "$use_gssapi" || echo "    GSS-API (--with-gssapi)"
    if test "yes" = "$enable_full_report" -o "aes" != "$with_cc_alg"; then
	echo "        Algorithm: $with_cc_alg"
    fi

    # these lines are only printed if run with --enable-full-report
    if test "yes" = "$enable_full_report"; then
	test "no" = "$found_ipv6" || echo "    IPv6 support (--enable-ipv6)"
	test "X$PYTHON" = "X" || echo "    Python tools (--with-python)"
	test "X$XMLSTATS" = "X" || echo "    XML statistics (--with-libxml2)"
	test "X$JSONSTATS" = "X" || echo "    JSON statistics (--with-libjson)"
	test "X$ZLIB" = "X" || echo "    HTTP zlib compression (--with-zlib)"
	test "X$NZD_TOOLS" = "X" || echo "    LMDB database to store configuration for 'addzone' zones (--with-lmdb)"
	test "no" = "$with_libidn2" || echo "    IDN support (--with-libidn2)"
    fi

    test "yes" = "$enable_dnsrps" && \
	echo "    DNS Response Policy Service interface (--enable-dnsrps)"
    test "yes" = "$enable_fixed" && \
	echo "    Allow 'fixed' rrset-order (--enable-fixed-rrset)"
    test "yes" = "$enable_backtrace" && \
	echo "    Print backtrace on crash (--enable-backtrace)"
    test "minimal" = "$want_symtable" && \
	echo "    Use symbol table for backtrace, named only (--enable-symtable)"
    test "yes" = "$want_symtable" -o "all" = "$want_symtable" && \
	echo "    Use symbol table for backtrace, all binaries (--enable-symtable=all)"
    test "no" = "$use_libtool" || echo "    Use GNU libtool (--with-libtool)"
    test "yes" = "$want_querytrace" && \
	echo "    Very verbose query trace logging (--enable-querytrace)"
    test "no" = "$with_cmocka" || echo "    CMocka Unit Testing Framework (--with-cmocka)"

    test "no" = "$want_autoval" || echo "    DNSSEC validation active by default (--enable-auto-validation)"

    test "$CRYPTO" = "pkcs11" && (
	echo "    Using PKCS#11 for Public-Key Cryptography (--with-native-pkcs11)"
	echo "    PKCS#11 module (--with-pkcs11): $with_pkcs11"
    )

    echo "    Dynamically loadable zone (DLZ) drivers:"
    test "no" = "$use_dlz_bdb" || \
	echo "        Berkeley DB (--with-dlz-bdb)"
    test "no" = "$use_dlz_ldap" || \
	echo "        LDAP (--with-dlz-ldap)"
    test "no" = "$use_dlz_mysql" || \
	echo "        MySQL (--with-dlz-mysql)"
    test "no" = "$use_dlz_odbc" || \
	echo "        ODBC (--with-dlz-odbc)"
    test "no" = "$use_dlz_postgres" || \
	echo "        Postgres (--with-dlz-postgres)"
    test "no" = "$use_dlz_filesystem" || \
	echo "        Filesystem (--with-dlz-filesystem)"
    test "no" = "$use_dlz_stub" || \
	echo "        Stub (--with-dlz-stub)"
    test "$use_dlz_bdb $use_dlz_ldap $use_dlz_mysql $use_dlz_odbc $use_dlz_postgres $use_dlz_filesystem $use_dlz_stub" = "no no no no no no no" && echo "        None"

    echo "-------------------------------------------------------------------------------"

    echo "Features disabled or unavailable on this platform:"
    test "no" = "$found_ipv6" && echo "    IPv6 support (--enable-ipv6)"
    test "large" = "$use_tuning" || echo "    Large-system tuning (--with-tuning)"

    test "no" = "$use_dnstap" && \
	    echo "    Allow 'dnstap' packet logging (--enable-dnstap)"
    test "no" = "$use_geoip" && echo "    GeoIP access control (--with-geoip)"
    test "no" = "$use_gssapi" && echo "    GSS-API (--with-gssapi)"

    test "no" = "$enable_dnsrps" && \
	echo "    DNS Response Policy Service interface (--enable-dnsrps)"

    test "yes" = "$enable_fixed" || \
	echo "    Allow 'fixed' rrset-order (--enable-fixed-rrset)"

    test "no" = "$want_autoval" && echo "    DNSSEC validation requires configuration (--disable-auto-validation)"

    test "$CRYPTO" = "pkcs11" || (
	echo "    Using PKCS#11 for Public-Key Cryptography (--without-native-pkcs11)"
    )

    test "yes" = "$enable_backtrace" || \
	echo "    Print backtrace on crash (--enable-backtrace)"
    test "yes" = "$want_querytrace" || \
	echo "    Very verbose query trace logging (--enable-querytrace)"

    test "yes" = "$use_libtool" || echo "    Use GNU libtool (--with-libtool)"
    test "no" = "$with_cmocka" && echo "    CMocka Unit Testing Framework (--with-cmocka)"

    test "X$PYTHON" = "X" && echo "    Python tools (--with-python)"
    test "X$XMLSTATS" = "X" && echo "    XML statistics (--with-libxml2)"
    test "X$JSONSTATS" = "X" && echo "    JSON statistics (--with-libjson)"
    test "X$ZLIB" = "X" && echo "    HTTP zlib compression (--with-zlib)"
    test "X$NZD_TOOLS" = "X" && echo "    LMDB database to store configuration for 'addzone' zones (--with-lmdb)"
    test "no" = "$with_libidn2" && echo "    IDN support (--with-libidn2)"

    echo "-------------------------------------------------------------------------------"
    echo "Configured paths:"
    echo "    prefix: $prefix"
    echo "    sysconfdir: $sysconfdir"
    echo "    localstatedir: $localstatedir"
    echo "-------------------------------------------------------------------------------"
    echo "Compiler: $CC"
    $CC --version 2>&1 | sed 's/^/    /'

    if test "X$ac_unrecognized_opts" != "X"; then
	echo "Unrecognized options:"
	echo "    $ac_unrecognized_opts"
    fi

    if test "yes" != "$enable_full_report"; then
	echo "-------------------------------------------------------------------------------"
	echo "For more detail, use --enable-full-report."
    fi
    echo "==============================================================================="
}

if test "yes" != "$silent"; then
	report
fi

# Tell Emacs to edit this file in shell mode.
# Local Variables:
# mode: sh
# End:
