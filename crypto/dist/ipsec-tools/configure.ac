dnl -*- mode: m4 -*-
dnl Id: configure.ac,v 1.77 2006/07/20 19:19:27 manubsd Exp

AC_PREREQ(2.52)
AC_INIT(ipsec-tools, CVS)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE(dist-bzip2)

AC_ENABLE_SHARED(no)

AC_PROG_CC
AC_HEADER_STDC
AC_PROG_LIBTOOL
AC_PROG_YACC
AM_PROG_LEX
AC_SUBST(LEXLIB)
AC_PROG_EGREP

CFLAGS_ADD="$CFLAGS_ADD -Wall -Werror -Wno-unused"

case $host in
*netbsd*)
	LDFLAGS="-Wl,-R/usr/pkg/lib $LDFLAGS"
	;;
*linux*)
	LIBS="$LIBS -lresolv"
	INSTALL_OPTS="-o bin -g bin"
	INCLUDE_GLIBC="include-glibc"
	RPM="rpm"
	AC_SUBST(INSTALL_OPTS)
	AC_SUBST(INCLUDE_GLIBC)
	AC_SUBST(RPM)
	;;
*darwin*)
	LIBS="$LIBS -lresolv"
	;;
esac

# Look up some IPsec-related headers
AC_CHECK_HEADER(net/pfkeyv2.h, [have_net_pfkey=yes], [have_net_pfkey=no])
AC_CHECK_HEADER(netinet/ipsec.h, [have_netinet_ipsec=yes], [have_netinet_ipsec=no])
AC_CHECK_HEADER(netinet6/ipsec.h, [have_netinet6_ipsec=yes], [have_netinet6_ipsec=no])
AC_CHECK_HEADER(netipsec/ipsec.h, [have_netipsec_ipsec=yes], [have_netipsec_ipsec=no])

# FreeBSD >=7 has only <netipsec/ipsec.h>
# NetBSD has <netinet6/ipsec.h> but not <netinet/ipsec.h>
# XXX some *BSD still have both <netinet6/ipsec.h> and <netipsec/ipsec.h>,
# we can't decide which one to use (actually <netinet6/ipsec.h>)


if test "$have_netinet_ipsec$have_netinet6_ipsec$have_netipsec_ipsec" = nonoyes; then
    have_netinet_ipsec=yes
    AC_DEFINE(PATH_IPSEC_H, [<netipsec/ipsec.h>], [Path to ipsec.h])
else
	if test "$have_netinet_ipsec$have_netinet6_ipsec" = noyes; then
    	have_netinet_ipsec=yes
	    AC_DEFINE(PATH_IPSEC_H, [<netinet6/ipsec.h>], [Path to ipsec.h])
	else
		# have_netinet_ipsec will be checked a few lines below
	    AC_DEFINE(PATH_IPSEC_H, [<netinet/ipsec.h>], [Path to ipsec.h])
	fi
fi

case "$host_os" in
 *linux*)
    AC_ARG_WITH(kernel-headers,
	AC_HELP_STRING([--with-kernel-headers=/lib/modules/<uname>/build/include],
		       [where your Linux Kernel headers are installed]),
	    [ KERNEL_INCLUDE="$with_kernel_headers" 
	      CONFIGURE_AMFLAGS="--with-kernel-headers=$with_kernel_headers"
	      AC_SUBST(CONFIGURE_AMFLAGS) ],
	    [ KERNEL_INCLUDE="/lib/modules/`uname -r`/build/include" ])

    AC_CHECK_HEADER($KERNEL_INCLUDE/linux/pfkeyv2.h, ,
	[ AC_CHECK_HEADER(/usr/src/linux/include/linux/pfkeyv2.h,
	  KERNEL_INCLUDE=/usr/src/linux/include ,
	  [ AC_MSG_ERROR([Unable to find linux-2.6 kernel headers. Aborting.]) ] ) ] )
    AC_SUBST(KERNEL_INCLUDE)
    # We need the configure script to run with correct kernel headers.
    # However we don't want to point to kernel source tree in compile time,
    # i.e. this will be removed from CPPFLAGS at the end of configure.
    CPPFLAGS="-I$KERNEL_INCLUDE $CPPFLAGS"

    AC_CHECK_MEMBER(struct sadb_x_policy.sadb_x_policy_priority, 
    	[AC_DEFINE(HAVE_PFKEY_POLICY_PRIORITY, [],
               	[Are PF_KEY policy priorities supported?])], [],
    	[#include "$KERNEL_INCLUDE/linux/pfkeyv2.h"])

    GLIBC_BUGS='-include ${top_srcdir}/src/include-glibc/glibc-bugs.h -I${top_srcdir}/src/include-glibc -I${top_builddir}/src/include-glibc'
    GLIBC_BUGS_LOCAL="-include ${srcdir-.}/src/include-glibc/glibc-bugs.h -I${srcdir-.}/src/include-glibc -I./src/include-glibc"
    CPPFLAGS="$GLIBC_BUGS_LOCAL $CPPFLAGS"
    CPPFLAGS="-D_GNU_SOURCE $CPPFLAGS"
    AC_SUBST(GLIBC_BUGS)
    ;;
 *)
    if test "$have_net_pfkey$have_netinet_ipsec" != yesyes; then
      if test "$have_net_pfkey" = yes; then
	AC_MSG_ERROR([Found net/pfkeyv2.h but not netinet/ipsec.h. Aborting.])
      else
	AC_MSG_ERROR([Found netinet/ipsec.h but not net/pfkeyv2.h. Aborting.])
      fi
    fi
    ;;
esac

### Some basic toolchain checks

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(limits.h sys/time.h unistd.h stdarg.h varargs.h)
AC_CHECK_HEADERS(shadow.h strings.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday select socket strerror strtol strtoul strlcpy strlcat)
AC_REPLACE_FUNCS(strdup)
RACOON_CHECK_VA_COPY

# Check if printf accepts "%z" type modifier for size_t argument
AC_MSG_CHECKING(if printf accepts %z)
saved_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -Wall -Werror"
AC_TRY_COMPILE([
#include <stdio.h>
], [
printf("%zu\n", (size_t)-1);
],
	[AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no);
	 CFLAGS_ADD="$CFLAGS_ADD -Wno-format";
	 AC_DEFINE(BROKEN_PRINTF, [], [If printf doesn't support %zu.])
	])
CFLAGS=$saved_CFLAGS

# Can we use __func__ macro?
AC_MSG_CHECKING(if __func__ is available)
AC_TRY_COMPILE(
[#include <stdio.h>
], [char *x = __func__;],
	[AC_DEFINE([HAVE_FUNC_MACRO], [], [Have __func__ macro])
	AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])

# Check if readline support is requested
AC_MSG_CHECKING(if readline support is requested)
AC_ARG_WITH(readline,
	[  --with-readline         support readline input (yes by default)],
	[with_readline="$withval"], [with_readline="yes"])
AC_MSG_RESULT($with_readline)

# Is readline available?
if test $with_readline != "no"; then
	AC_CHECK_HEADER([readline/readline.h], 
		[AC_CHECK_LIB(readline, readline, [
				AC_DEFINE(HAVE_READLINE, [],
					[Is readline available?])
				LIBS="$LIBS -lreadline"
		], [])], [])
fi


AC_MSG_CHECKING(if --with-flex option is specified)
AC_ARG_WITH(flexdir,
	[AC_HELP_STRING([--with-flex], [use directiory (default: no)])],
	[flexdir="$withval"])
AC_MSG_RESULT(${flexdir-dirdefault})

if test "x$flexdir" != "x"; then
	LIBS="$LIBS $flexdir/libfl.a"
fi

AC_MSG_CHECKING(if --with-flexlib option is specified)
AC_ARG_WITH(flexlib,
	[  --with-flexlib=<LIB>    specify flex library.],
	[flexlib="$withval"])
AC_MSG_RESULT(${flexlib-default})

if test "x$flexlib" != "x"; then
	LIBS="$LIBS $flexlib"
fi

# Check if a different OpenSSL directory was specified
AC_MSG_CHECKING(if --with-openssl option is specified)
AC_ARG_WITH(openssl, [  --with-openssl=DIR      specify OpenSSL directory],
	[crypto_dir=$withval])
AC_MSG_RESULT(${crypto_dir-default})

if test "x$crypto_dir" != "x"; then
	LIBS="$LIBS -L${crypto_dir}/lib"
	CPPFLAGS="-I${crypto_dir}/include $CPPFLAGS"
fi
AC_MSG_CHECKING(openssl version)

AC_TRY_COMPILE(
[#include <openssl/opensslv.h>
],
[#if OPENSSL_VERSION_NUMBER < 0x0090813fL
#error OpenSSL version is too old ...
#endif],
[AC_MSG_RESULT([ok])],
[AC_MSG_RESULT(too old)
AC_MSG_ERROR([OpenSSL version must be 0.9.8s or higher. Aborting.])
])

AC_CHECK_HEADERS(openssl/engine.h)

# checking rijndael
AC_CHECK_HEADERS([openssl/aes.h], [], 
	[CRYPTOBJS="$CRYPTOBJS rijndael-api-fst.o rijndael-alg-fst.o"])

# checking sha2
AC_MSG_CHECKING(sha2 support)
AC_DEFINE([WITH_SHA2], [], [SHA2 support])
AC_MSG_RESULT(yes)
AC_CHECK_HEADER(openssl/sha2.h, [], [
	AC_MSG_CHECKING(if sha2 is defined in openssl/sha.h)
	AC_TRY_COMPILE([
		#ifdef HAVE_SYS_TYPES_H
		#include <sys/types.h>
		#endif
		#include <openssl/sha.h>
	], [
		SHA256_CTX ctx;
	], [
	    AC_MSG_RESULT(yes)
	    AC_DEFINE([HAVE_SHA2_IN_SHA_H], [], [sha2 is defined in sha.h])
	], [AC_MSG_RESULT(no)
	    AC_LIBOBJ([sha2])
	    CRYPTOBJS="$CRYPTOBJS sha2.o"
	])

	CPPFLAGS_ADD="$CPPFLAGS_ADD -I\${top_srcdir}/src/racoon/missing"
])
AC_SUBST(CRYPTOBJS)

# checking camellia
AC_CHECK_HEADERS([openssl/camellia.h])


# Option --enable-adminport 
AC_MSG_CHECKING(if --enable-adminport option is specified)
AC_ARG_ENABLE(adminport,
	[  --enable-adminport      enable admin port],
	[], [enable_adminport=no])
if test $enable_adminport = "yes"; then
	AC_DEFINE([ENABLE_ADMINPORT], [], [Enable admin port])
fi
AC_MSG_RESULT($enable_adminport)

# Option RC5
AC_MSG_CHECKING(if --enable-rc5 option is specified)
AC_ARG_ENABLE(rc5,
	[  --enable-rc5		enable RC5 encryption (patented)],
	[], [enable_rc5=no])
AC_MSG_RESULT($enable_rc5)

if test $enable_rc5 = "yes"; then
	AC_CHECK_HEADERS([openssl/rc5.h])
	AC_CHECK_LIB([crypto_rc5], [RC5_32_encrypt],
	    [EXTRA_CRYPTO="$EXTRA_CRYPTO -lcrypto_rc5"])
fi

# Option IDEA
AC_MSG_CHECKING(if --enable-idea option is specified)
AC_ARG_ENABLE(idea,
	[  --enable-idea	enable IDEA encryption (patented)],
	[], [enable_idea=no])
AC_MSG_RESULT($enable_idea)

if test $enable_idea = "yes"; then
	AC_CHECK_HEADERS([openssl/idea.h])
	AC_CHECK_LIB([crypto_idea], [idea_encrypt], 
	    [EXTRA_CRYPTO="$EXTRA_CRYPTO -lcrypto_idea"])
fi
AC_SUBST(EXTRA_CRYPTO)

# For dynamic libradius
RACOON_PATH_LIBS([MD5_Init], [crypto])

# Check if we need -lutil for login(3)
RACOON_PATH_LIBS([login], [util])

# Specify libiconv prefix
AC_MSG_CHECKING(if --with-libiconv option is specified)
AC_ARG_WITH(libiconv, 
    [  --with-libiconv=DIR    specify libiconv path (like/usr/pkg)],
    [libiconv_dir=$withval], 
    [libiconv_dir=no])
AC_MSG_RESULT($libiconv_dir)
if test "$libiconv_dir" != "no"; then
	if test "$libiconv_dir" = "yes" ; then
		  libiconv_dir="";
	fi;
	if test "x$libiconv_dir" = "x"; then
		RACOON_PATH_LIBS([iconv_open], [iconv])
	else
		if test -d "$libiconv_dir/lib" -a \
		    -d "$libiconv_dir/include" ; then
			RACOON_PATH_LIBS([iconv_open], [iconv], ["$libiconv_dir/lib"])
			CPPFLAGS_ADD="$CPPFLAGS_ADD -I$libiconv_dir/include"
		else
			AC_MSG_ERROR([ICONV libs or includes not found. Aborting.])
	  	fi
	fi
	LIBS="$LIBS -L$libiconv_dir/lib -R$libiconv_dir/lib -liconv"
	AC_CHECK_FUNCS(iconv_open)
fi

AC_MSG_CHECKING([if --enable-hybrid option is specified])
AC_ARG_ENABLE(hybrid, 
    [  --enable-hybrid	  enable hybrid, both mode-cfg and xauth support],
    [], [enable_hybrid=no])
AC_MSG_RESULT($enable_hybrid)

if test "x$enable_hybrid" = "xyes"; then
	case $host in
		*darwin*)
		;;
	*)
		LIBS="$LIBS -lcrypt";
		;;
	esac
	HYBRID_OBJS="isakmp_xauth.o isakmp_cfg.o isakmp_unity.o throttle.o"
	AC_SUBST(HYBRID_OBJS)
	AC_DEFINE([ENABLE_HYBRID], [], [Hybrid authentication support])
fi

AC_MSG_CHECKING([if --enable-frag option is specified])
AC_ARG_ENABLE(frag, 
    [  --enable-frag           enable IKE fragmentation payload support],
    [], [enable_frag=no])
AC_MSG_RESULT($enable_frag)

if test "x$enable_frag" = "xyes"; then
	case $host in
	*darwin*)
		;;
	*)
		LIBS="$LIBS -lcrypt"; 
		;;
	esac
	FRAG_OBJS="isakmp_frag.o"
	AC_SUBST(FRAG_OBJS)
	AC_DEFINE([ENABLE_FRAG], [], [IKE fragmentation support])
fi

AC_MSG_CHECKING(if --with-libradius option is specified)
AC_ARG_WITH(libradius, 
    [  --with-libradius=DIR    specify libradius path (like/usr/pkg)],
    [libradius_dir=$withval], 
    [libradius_dir=no])
AC_MSG_RESULT($libradius_dir)
if test "$libradius_dir" != "no"; then
	if test "$libradius_dir" = "yes" ; then
		  libradius_dir="";
	fi;
	if test "x$libradius_dir" = "x"; then
		RACOON_PATH_LIBS([rad_create_request], [radius])
	else
		if test -d "$libradius_dir/lib" -a \
		    -d "$libradius_dir/include" ; then
			RACOON_PATH_LIBS([rad_create_request], [radius], ["$libradius_dir/lib"])
			CPPFLAGS_ADD="$CPPFLAGS_ADD -I$libradius_dir/include"
		else
			AC_MSG_ERROR([RADIUS libs or includes not found. Aborting.])
	  	fi
	fi
	AC_DEFINE([HAVE_LIBRADIUS], [], [Hybrid authentication uses RADIUS])
	LIBS="$LIBS -L$libradius_dir/lib -R$libradius_dir/lib -lradius"
	AC_CHECK_FUNCS(rad_create_request)
fi

AC_MSG_CHECKING(if --with-libpam option is specified)
AC_ARG_WITH(libpam, 
    [  --with-libpam=DIR    specify libpam path (like/usr/pkg)],
    [libpam_dir=$withval], 
    [libpam_dir=no])
AC_MSG_RESULT($libpam_dir)
if test "$libpam_dir" != "no"; then
	if test "$libpam_dir" = "yes" ; then
		  libpam_dir="";
	fi;
	if test "x$libpam_dir" = "x"; then
		RACOON_PATH_LIBS([pam_start], [pam])
	else
		if test -d "$libpam_dir/lib" -a \
		    -d "$libpam_dir/include" ; then
			RACOON_PATH_LIBS([pam_start], [pam], ["$libpam_dir/lib"])
			CPPFLAGS_ADD="$CPPFLAGS_ADD -I$libpam_dir/include"
		else
			AC_MSG_ERROR([PAM libs or includes not found. Aborting.])
	  	fi
	fi
	AC_DEFINE([HAVE_LIBPAM], [], [Hybrid authentication uses PAM])
	LIBS="$LIBS -L$libpam_dir/lib -R$libpam_dir/lib -lpam"
	AC_CHECK_FUNCS(pam_start)
fi

AC_MSG_CHECKING(if --with-libldap option is specified)
AC_ARG_WITH(libldap, 
    [  --with-libldap=DIR    specify libldap path (like/usr/pkg)],
    [libldap_dir=$withval], 
    [libldap_dir=no])
AC_MSG_RESULT($libldap_dir)
if test "$libldap_dir" != "no"; then
	if test "$libldap_dir" = "yes" ; then
		  libldap_dir="";
	fi;
	if test "x$libldap_dir" = "x"; then
		RACOON_PATH_LIBS([ldap_init], [ldap])
	else
		if test -d "$libldap_dir/lib" -a \
		    -d "$libldap_dir/include" ; then
			RACOON_PATH_LIBS([ldap_init], [ldap], ["$libldap_dir/lib"])
			CPPFLAGS_ADD="$CPPFLAGS_ADD -I$libldap_dir/include"
		else
			AC_MSG_ERROR([LDAP libs or includes not found. Aborting.])
	  	fi
	fi
	AC_DEFINE([HAVE_LIBLDAP], [], [Hybrid authentication uses LDAP])
	LIBS="$LIBS -L$libldap_dir/lib -R$libldap_dir/lib -lldap"

	saved_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS -Wall -Werror"
	saved_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $CPPFLAGS_ADD"
	AC_TRY_COMPILE(
		[#include <ldap.h>],
		[
			#if LDAP_API_VERSION < 2004
			#error OpenLDAP version is too old ...
			#endif
		],
		[AC_MSG_RESULT([ok])],
		[
			AC_MSG_RESULT(too old)
			AC_MSG_ERROR([OpenLDAP version must be 2.0 or higher. Aborting.])
		])
	CFLAGS=$saved_CFLAGS
	CPPFLAGS=$saved_CPPFLAGS
fi

# Check for Kerberos5 support
# XXX This must come after all --with-* tests, else the
# -liconv checks will not work
AC_MSG_CHECKING(if --enable-gssapi option is specified)
AC_ARG_ENABLE(gssapi,
	[  --enable-gssapi         enable GSS-API authentication],
	[], [enable_gssapi=no])
AC_MSG_RESULT($enable_gssapi)
AC_PATH_PROG(KRB5_CONFIG,krb5-config,no)
if test "x$enable_gssapi" = "xyes"; then
	if test "$KRB5_CONFIG" != "no"; then
		krb5_incdir="`$KRB5_CONFIG --cflags gssapi`"
		krb5_libs="`$KRB5_CONFIG --libs gssapi`"
	else
		# No krb5-config; let's make some assumptions based on
		# the OS.
		case $host_os in
		netbsd*)
			krb5_incdir="-I/usr/include/krb5"
			krb5_libs="-lgssapi -lkrb5 -lcom_err -lroken -lasn1"
			;;
		*)
			AC_MSG_ERROR([krb5-config not found, but needed for GSSAPI support. Aborting.])
			;;
		esac
	fi
	LIBS="$LIBS $krb5_libs"
	CPPFLAGS_ADD="$krb5_incdir $CPPFLAGS_ADD"
	AC_DEFINE([HAVE_GSSAPI], [], [Enable GSS API])

	# Check if iconv 2nd argument needs const 
	saved_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS -Wall -Werror"
	saved_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $CPPFLAGS_ADD"
	AC_CHECK_HEADER([iconv.h], [], [AC_MSG_ERROR([iconv.h not found, but needed for GSSAPI support. Aborting.])])
	AC_MSG_CHECKING([if iconv second argument needs const])
	AC_TRY_COMPILE([
		#include <iconv.h>
		#include <stdio.h>
	], [
		iconv_t cd = NULL;
		const char **src = NULL;
		size_t *srcleft = NULL;
		char **dst = NULL;
		size_t *dstleft = NULL;

		(void)iconv(cd, src, srcleft, dst, dstleft);
	], [AC_MSG_RESULT(yes)
	    AC_DEFINE([HAVE_ICONV_2ND_CONST], [], [Have iconv using const])
	], [AC_MSG_RESULT(no)])
	CFLAGS=$saved_CFLAGS
	CPPFLAGS=$saved_CPPFLAGS

	# libiconv is often integrated into libc. If a with-* option
	# caused a non libc-based iconv.h to be catched instead of
	# the libc-based iconv.h, then we need to link with -liconv
	AC_MSG_CHECKING(if -liconv is required)
	saved_CPPFLAGS=$CPPFLAGS
	saved_LIBS=$LIBS
	CPPFLAGS="$CPPFLAGS $CPPFLAGS_ADD"
	AC_TRY_LINK([
		#include <iconv.h>
	], [
		(void)iconv_open("ascii", "ascii");
	],
		[AC_MSG_RESULT(no)],
		[
			LIBS="$LIBS -liconv"
			AC_TRY_LINK([
				#include <iconv.h>
		], [
				(void)iconv_open("ascii", "ascii");
			],
			[
				AC_MSG_RESULT(yes)
				saved_LIBS=$LIBS
			], [
				AC_MSG_ERROR([cannot use iconv])
			])
		])
	CPPFLAGS=$saved_CPPFLAGS
	LIBS=$saved_LIBS
fi

AC_MSG_CHECKING(if --enable-stats option is specified)
AC_ARG_ENABLE(stats,
        [  --enable-stats          enable statistics logging function],
        [], [enable_stats=no])
if test "x$enable_stats" = "xyes"; then
	AC_DEFINE([ENABLE_STATS], [], [Enable statictics])
fi
AC_MSG_RESULT($enable_stats)

AC_MSG_CHECKING(if --enable-dpd option is specified)
AC_ARG_ENABLE(dpd,
        [  --enable-dpd            enable dead peer detection],
        [], [enable_dpd=no])
if test "x$enable_dpd" = "xyes"; then
	AC_DEFINE([ENABLE_DPD], [], [Enable dead peer detection])
fi
AC_MSG_RESULT($enable_dpd)

AC_MSG_CHECKING(if --enable-samode-unspec option is specified)
AC_ARG_ENABLE(samode-unspec,
        [  --enable-samode-unspec  enable to use unspecified a mode of SA],
        [], [enable_samode_unspec=no])
if test "x$enable_samode_unspec" = "xyes"; then
	case $host_os in
	*linux*)
		cat << EOC
		
ERROR: --enable-samode-unspec is not supported under linux 
because linux kernel do not support it. This option is disabled 
to prevent mysterious problems.

If you REALLY know what your are doing, remove this check.
EOC
		exit 1;
		;;
	esac
	AC_DEFINE([ENABLE_SAMODE_UNSPECIFIED], [], [Enable samode-unspec])
fi
AC_MSG_RESULT($enable_samode_unspec)

# Checks if IPv6 is requested
AC_MSG_CHECKING([whether to enable ipv6])
AC_ARG_ENABLE(ipv6,
[  --disable-ipv6          disable ipv6 support],
[ case "$enableval" in
  no)
       AC_MSG_RESULT(no)
       ipv6=no
       ;;
  *)   AC_MSG_RESULT(yes)
       ipv6=yes
       ;;
  esac ],

  AC_TRY_RUN([ /* AF_INET6 avalable check */
#include <sys/types.h>
#include <sys/socket.h>
main()
{
  exit(0);
 if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
   exit(1);
 else
   exit(0);
}
],
  AC_MSG_RESULT(yes)
  AC_DEFINE([INET6], [], [Support IPv6])
  ipv6=yes,
  AC_MSG_RESULT(no)
  ipv6=no,
  AC_MSG_RESULT(no)
  ipv6=no
))

if test "$ipv6" = "yes"; then
	AC_DEFINE([INET6], [], [Support IPv6])
	AC_MSG_CHECKING(for advanced API support)
	AC_TRY_COMPILE([#ifndef INET6
#define INET6
#endif
#include <sys/types.h>
#include <netinet/in.h>],
		[struct in6_pktinfo a;],
		[AC_MSG_RESULT(yes)
		 AC_DEFINE([INET6_ADVAPI], [], [Use advanced IPv6 API])],
		[AC_MSG_RESULT(no)])
fi

RACOON_CHECK_BUGGY_GETADDRINFO
if test "$buggygetaddrinfo" = "yes"; then
	AC_MSG_ERROR([Broken getaddrinfo() is no longer supported. Aborting.])
fi

# Check if kernel support is available for NAT-T, defaults to no. 
kernel_natt="no"

AC_MSG_CHECKING(kernel NAT-Traversal support)
case $host_os in
linux*)
# Linux kernel NAT-T check
AC_EGREP_CPP(yes, 
[#include <linux/pfkeyv2.h>
#ifdef SADB_X_EXT_NAT_T_TYPE
yes
#endif
], [kernel_natt="yes"])
	;;
freebsd*|netbsd*)
# NetBSD case
# Same check for FreeBSD
AC_CHECK_MEMBER(struct sadb_x_nat_t_type.sadb_x_nat_t_type_len,
       [kernel_natt="yes"],, [
#define _KERNEL
#include <sys/types.h>
#include <net/pfkeyv2.h>
])
	;;
esac
AC_MSG_RESULT($kernel_natt)

AC_MSG_CHECKING(whether to support NAT-T)
AC_ARG_ENABLE(natt,
	[  --enable-natt           enable NAT-Traversal (yes/no/kernel)],
        [ if test "$enable_natt" = "kernel"; then enable_natt=$kernel_natt; fi ],
	[ enable_natt=no ])
AC_MSG_RESULT($enable_natt)

if test "$enable_natt" = "yes"; then
	if test "$kernel_natt" = "no" ; then 
		AC_MSG_ERROR([NAT-T requested, but no kernel support! Aborting.])
	else
		AC_DEFINE([ENABLE_NATT], [], [Enable NAT-Traversal])
		NATT_OBJS="nattraversal.o"
		AC_SUBST(NATT_OBJS)
	fi
fi

# Set up defines for supported NAT-T versions.
natt_versions_default="00,02,rfc"
AC_MSG_CHECKING(which NAT-T versions to support)
AC_ARG_ENABLE(natt_versions,
	[  --enable-natt-versions=list    list of supported NAT-T versions delimited by coma.],
	[ test "$enable_natt_versions" = "yes" && enable_natt_versions=$natt_versions_default ],
	[ enable_natt_versions=$natt_versions_default ])
if test "$enable_natt" = "yes"; then
	AC_MSG_RESULT($enable_natt_versions)
	for i in `echo $enable_natt_versions | tr ',cfr' ' CFR'`; do
		case $i in 
			0|00) AC_DEFINE([ENABLE_NATT_00], [], [Enable NAT-Traversal draft 00]) ;;
			1|01) AC_DEFINE([ENABLE_NATT_01], [], [Enable NAT-Traversal draft 01]) ;;
			2|02) AC_DEFINE([ENABLE_NATT_02], [], [Enable NAT-Traversal draft 02]) ;;
			3|03) AC_DEFINE([ENABLE_NATT_03], [], [Enable NAT-Traversal draft 03]) ;;
			4|04) AC_DEFINE([ENABLE_NATT_04], [], [Enable NAT-Traversal draft 04]) ;;
			5|05) AC_DEFINE([ENABLE_NATT_05], [], [Enable NAT-Traversal draft 05]) ;;
			6|06) AC_DEFINE([ENABLE_NATT_06], [], [Enable NAT-Traversal draft 06]) ;;
			7|07) AC_DEFINE([ENABLE_NATT_07], [], [Enable NAT-Traversal draft 07]) ;;
			8|08) AC_DEFINE([ENABLE_NATT_08], [], [Enable NAT-Traversal draft 08]) ;;
			RFC)  AC_DEFINE([ENABLE_NATT_RFC], [], [Enable NAT-Traversal RFC version]) ;;
			*) AC_MSG_ERROR([Unknown NAT-T version. Aborting.]) ;;
		esac
	done
	unset i
else
	AC_MSG_RESULT([none])
fi

AC_MSG_CHECKING(if --enable-broken-natt option is specified)
AC_ARG_ENABLE(broken-natt,
	[  --enable-broken-natt    broken in-kernel NAT-T],
        [], [enable_broken_natt=no])
if test "x$enable_broken_natt" = "xyes"; then
	AC_DEFINE([BROKEN_NATT], [], [in-kernel NAT-T is broken])
fi
AC_MSG_RESULT($enable_broken_natt)

AC_MSG_CHECKING(whether we support FWD policy)
case $host in
	*linux*)
		AC_TRY_COMPILE([
		#include <inttypes.h>
		#include <linux/ipsec.h>
			], [
			int fwd = IPSEC_DIR_FWD;
			],
			[AC_MSG_RESULT(yes)
			 AC_DEFINE([HAVE_POLICY_FWD], [], [Have forward policy])],
			[AC_MSG_RESULT(no)])
		;;
	*)
		AC_MSG_RESULT(no)
		;;
esac

AC_CHECK_TYPE([ipsec_policy_t], 
	      [AC_DEFINE([HAVE_IPSEC_POLICY_T], [], [Have ipsec_policy_t])],
	      [],
	      [
		#include <sys/types.h>
	      	#include <netinet6/ipsec.h>
	      ])

# Check if kernel support is available for Security Context, defaults to no.
kernel_secctx="no"

AC_MSG_CHECKING(kernel Security Context support)
case $host_os in
linux*)
# Linux kernel Security Context check
AC_EGREP_CPP(yes,
[#include <linux/pfkeyv2.h>
#ifdef SADB_X_EXT_SEC_CTX
yes
#endif
], [kernel_secctx="yes"])
	;;
esac
AC_MSG_RESULT($kernel_secctx)

AC_CHECK_HEADER(selinux/selinux.h,
	[AC_CHECK_LIB(selinux, avc_init, [selinux_support=yes], 
	[selinux_support=no])], [selinux_support=no])

AC_MSG_CHECKING(whether to support Security Context)
AC_ARG_ENABLE(security-context,
	[  --enable-security-context    enable Security Context(yes/no/kernel)],
	[if test "$enable_security_context" = "kernel"; then
		enable_security_context=$kernel_secctx; fi],
	[enable_security_context=$kernel_secctx])
AC_MSG_RESULT($enable_security_context)

if test "$enable_security_context" = "yes"; then
	if test "$kernel_secctx" = "no" ; then
		AC_MSG_ERROR([Security Context requested, but no kernel support! Aborting.])
	else
		if test "$selinux_support" = "no"; then
			AC_MSG_ERROR([Security Context requested, but no selinux support! Aborting.])
		else
			AC_DEFINE([HAVE_SECCTX], [], [Enable Security Context])
			SECCTX_OBJS="security.o"
			AC_SUBST(SECCTX_OBJS)
			LIBS="$LIBS -lselinux"
		fi
	fi
fi

RACOON_PATH_LIBS([clock_gettime], [rt])

AC_MSG_CHECKING(for monotonic system clock)
AC_TRY_COMPILE(
	[#include <time.h>],
	[clock_gettime(CLOCK_MONOTONIC, NULL);],
	[AC_DEFINE([HAVE_CLOCK_MONOTONIC], [], [Have a monotonic clock])
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])

CFLAGS="$CFLAGS $CFLAGS_ADD"
CPPFLAGS="$CPPFLAGS $CPPFLAGS_ADD"

case $host in
	*linux*)
		# Remove KERNEL_INCLUDE from CPPFLAGS. It will
		# be symlinked to src/include-glibc/linux in
		# compile time.
		CPPFLAGS=`echo $CPPFLAGS | sed "s,-I$KERNEL_INCLUDE,,"`
		;;
esac

include_racoondir=${includedir}/racoon
AC_SUBST(include_racoondir)

AC_CONFIG_FILES([
  Makefile
  package_version.h
  src/Makefile
  src/include-glibc/Makefile
  src/libipsec/Makefile
  src/setkey/Makefile
  src/racoon/Makefile
  src/racoon/samples/psk.txt
  src/racoon/samples/racoon.conf
  rpm/Makefile
  rpm/suse/Makefile
  rpm/suse/ipsec-tools.spec
  ])
AC_OUTPUT
