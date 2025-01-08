AC_PREREQ([2.69])
AC_INIT([ppp],
        [2.5.2],
        [https://github.com/ppp-project/ppp])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])

AC_LANG(C)
AC_CONFIG_SRCDIR([pppd/main.c])
AC_CONFIG_HEADERS([pppd/config.h pppd/pppdconf.h pppd/plugins/pppoe/config.h])
AC_ENABLE_STATIC(no)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
LT_INIT

PKG_PROG_PKG_CONFIG

AC_CANONICAL_HOST
build_linux=no
build_sunos=no

case "${host_os}" in
    linux*)
	build_linux=yes
	;;
    solaris2*)
	build_sunos=yes
	;;
    *)
	AC_MSG_ERROR(["OS ${host_os} not supported"])
	;;
esac

AM_CONDITIONAL([LINUX], [test "x${build_linux}" = "xyes" ])
AM_CONDITIONAL([SUNOS], [test "x${build_sunos}" = "xyes" ])
AM_COND_IF([SUNOS],
      CFLAGS="$CFLAGS -DSOL2 -DSRV4")

#
# Checks for header files, these will set the HAVE_[FILE]_H macros in config.h
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([  \
    asm/types.h     \
    crypt.h         \
    paths.h         \
    shadow.h        \
    stddef.h        \
    stdarg.h        \
    sys/dlpi.h      \
    sys/ioctl.h     \
    sys/socket.h    \
    sys/time.h      \
    sys/uio.h       \
    time.h          \
    unistd.h        \
    utmp.h])

#
# Check for linux specific headers, required by pppoe, or pppol2tp
AM_COND_IF([LINUX], [
    AC_CHECK_HEADERS([          \
        net/bpf.h               \
        net/if.h                \
        net/if_types.h          \
        net/if_arp.h            \
        linux/if.h              \
        linux/if_ether.h        \
        linux/if_packet.h       \
        netinet/if_ether.h      \
        netpacket/packet.h])

    AC_MSG_CHECKING([for struct sockaddr_ll in <linux/if_packet.h>])
    AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([@%:@include <linux/if_packet.h>], [sizeof(struct sockaddr_ll)])],
	[AC_MSG_RESULT([yes])
	 AC_DEFINE(HAVE_STRUCT_SOCKADDR_LL, 1, [Struct sockaddr_ll is present on system])
	],
	AC_MSG_RESULT([no]))
])


AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned short)

# Checks for library functions.
AC_CHECK_FUNCS([    \
    mmap            \
    logwtmp         \
    strerror])

#
# If libc doesn't provide logwtmp, check if libutil provides logwtmp(), and if so link to it.
AS_IF([test "x${ac_cv_func_logwtmp}" != "xyes"], [
    AC_CHECK_LIB([util], [logwtmp], [
        AC_DEFINE(HAVE_LOGWTMP, 1, [System provides the logwtmp() function])
        AC_SUBST([UTIL_LIBS], ["-lutil"])
    ])
])

#
# Check if libcrypt have crypt() function
AC_CHECK_LIB([crypt], [crypt],
    AC_SUBST([CRYPT_LIBS], ["-lcrypt"]))

#
# Should pppd link with -lsystemd (Linux only)
AC_ARG_ENABLE([systemd],
    AS_HELP_STRING([--enable-systemd], [Enable support for systemd notification]))
AM_CONDITIONAL(WITH_SYSTEMD, test "x${enable_systemd}" = "xyes")
AM_COND_IF([WITH_SYSTEMD],
    AC_DEFINE([SYSTEMD], 1, [Enable support for systemd notifications]))
AS_IF([test "x${enable_systemd}" = "xyes"], [
	PKG_CHECK_MODULES([SYSTEMD], [libsystemd])])

#
# Enable Callback Protocol Support, disabled by default
AC_ARG_ENABLE([cbcp],
    AS_HELP_STRING([--enable-cbcp], [Enable Callback Protocol]))
AM_CONDITIONAL(PPP_WITH_CBCP, test "x${enable_cbcp}" = "xyes")
AM_COND_IF([PPP_WITH_CBCP],
    AC_DEFINE([PPP_WITH_CBCP], 1, [Have Callback Protocol support]))

#
# Disable Microsoft extensions will remove CHAP, MPPE and PEAP support
AC_ARG_ENABLE([microsoft-extensions],
    AS_HELP_STRING([--disable-microsoft-extensions], [Disable Microsoft CHAP / MPPE / PEAP extensions]))

AM_CONDITIONAL(PPP_WITH_CHAPMS, test "x${enable_microsoft_extensions}" != "xno")
AM_COND_IF([PPP_WITH_CHAPMS],
    AC_DEFINE([PPP_WITH_CHAPMS], 1, [Have Microsoft CHAP support]))

AM_CONDITIONAL(PPP_WITH_MPPE, test "x${build_sunos}" != "xyes" && test "x${enable_microsoft_extensions}" != "xno")
AM_COND_IF([PPP_WITH_MPPE],
    AC_DEFINE([PPP_WITH_MPPE], 1, [Have Microsoft MPPE support]))

#
# Enable Microsoft LAN Manager support, depends on Microsoft Extensions
AC_ARG_ENABLE([mslanman],
    AS_HELP_STRING([--enable-mslanman], [Enable Microsoft LAN Manager support]))
AS_IF([test "x${enable_mslanman}" = "xyes" && test "x${enable_microsoft_extensions}" != "xno"],
    AC_DEFINE([PPP_WITH_MSLANMAN], 1, [Have Microsoft LAN Manager support]))

#
# Disable IPv6 support
AC_ARG_ENABLE([ipv6cp],
    AS_HELP_STRING([--disable-ipv6cp], [Disable IPv6 Control Protocol]))
AM_CONDITIONAL(PPP_WITH_IPV6CP, test "x${enable_ipv6cp}" != "xno")
AM_COND_IF([PPP_WITH_IPV6CP],
    AC_DEFINE(PPP_WITH_IPV6CP, 1, [Have IPv6 Control Protocol]))

#
# Disable Multilink support
AC_ARG_ENABLE([multilink],
    AS_HELP_STRING([--enable-multilink], [Enable multilink support]))
AM_CONDITIONAL(PPP_WITH_MULTILINK, test "x${enable_multilink}" = "xyes")
AM_COND_IF([PPP_WITH_MULTILINK],
    AC_DEFINE([PPP_WITH_MULTILINK], 1, [Have multilink support]))
AS_IF([test "x${build_sunos}" = "xyes" && test "x${enable_multilink}" = "xyes"],
    [AC_MSG_ERROR([Multilink is not supported on SunOS])])

#
# Multilink require Trivial Database Support
AM_CONDITIONAL(PPP_WITH_TDB, test "x${enable_multilink}" = "xyes")
AM_COND_IF([PPP_WITH_TDB],
    AC_DEFINE([PPP_WITH_TDB], 1, [Include TDB support]))

#
# Enable support for loadable plugins
AC_ARG_ENABLE([plugins],
    AS_HELP_STRING([--disable-plugins], [Disable support for loadable plugins]))
AS_IF([test "x$enable_plugins" != "xno"],
    AC_DEFINE([PPP_WITH_PLUGINS], 1, [Have support for loadable plugins]))
AM_CONDITIONAL(PPP_WITH_PLUGINS, test "x${enable_plugins}" != "xno")

#
# Disable EAP-TLS support
AC_ARG_ENABLE([eaptls],
    AS_HELP_STRING([--disable-eaptls], [Disable EAP-TLS authentication support]))
AS_IF([test "x$enable_eaptls" != "xno"],
    AC_DEFINE([PPP_WITH_EAPTLS], 1, [Have EAP-TLS authentication support]))
AM_CONDITIONAL(PPP_WITH_EAPTLS, test "x${enable_eaptls}" != "xno")

#
# Disable PEAP support
AC_ARG_ENABLE([peap],
    AS_HELP_STRING([--disable-peap], [Disable PEAP authentication support]))
AS_IF([test "x${enable_peap}" != "xno" && test "x${enable_microsoft_extensions}" != "xno"],
    AC_DEFINE([PPP_WITH_PEAP], 1, [Have PEAP authentication support]))
AM_CONDITIONAL([PPP_WITH_PEAP], test "x${enable_peap}" != "xno" && test "x${enable_microsoft_extensions}" != "xno")

#
# Disable OpenSSL engine support
AC_ARG_ENABLE([openssl-engine],
    AS_HELP_STRING([--disable-openssl-engine], [Disable OpenSSL engine support]))
AS_IF([test "x$enable_openssl_engine" != "xno"], [],
    AC_DEFINE([OPENSSL_NO_ENGINE], 1, [OpenSSL engine support]))

#
# Specify runtime directory
AC_ARG_WITH([plugin-dir],
	AS_HELP_STRING([--with-plugin-dir=DIR],[Specify the plugin directory for pppd]))
AS_IF([test -n "$with_plugin_dir"],
	[PPPD_PLUGIN_DIR="$with_plugin_dir"],
	[PPPD_PLUGIN_DIR="${libdir}/pppd/$VERSION"])
AC_SUBST(PPPD_PLUGIN_DIR, "$PPPD_PLUGIN_DIR", [The pppd plugin directory])

#
# Specify runtime directory
AC_ARG_WITH([runtime-dir],
	AS_HELP_STRING([--with-runtime-dir=DIR],[Specify the runtime directory for pppd]))
AS_IF([test -n "$with_runtime_dir"],
	[PPPD_RUNTIME_DIR="$with_runtime_dir"],
	[PPPD_RUNTIME_DIR="${runstatedir}/pppd"])
AC_SUBST(PPPD_RUNTIME_DIR)

#
# Specify runtime directory
AC_ARG_WITH([logfile-dir],
	AS_HELP_STRING([--with-logfile-dir=DIR],[Specify the log directory for pppd]))
AS_IF([test -n "$with_logfile_dir"],
	[PPPD_LOGFILE_DIR="$with_logfile_dir"],
	[PPPD_LOGFILE_DIR="${localstatedir}/log/ppp"])
AC_SUBST(PPPD_LOGFILE_DIR)

#
# System CA certificates path
AC_ARG_WITH(system-ca-path,
    AS_HELP_STRING([--with-system-ca-path=/path/to/ssl/certs], [path to system CA certificates]),
    [
       case "$withval" in
       "" | y | ye | yes)
            with_system_ca_path="${sysconfdir}/ssl/certs"
            ;;
       n | no)
            ;;
       *)
            with_system_ca_path="$withval"
            ;;
       esac
    ],[with_system_ca_path="${sysconfdir}/ssl/certs"])
AM_CONDITIONAL(PPP_WITH_SYSTEM_CA_PATH, [test "$with_system_ca_path" != "no"])
AM_COND_IF(PPP_WITH_SYSTEM_CA_PATH, [
    SYSTEM_CA_PATH="$with_system_ca_path"
])
AC_SUBST(SYSTEM_CA_PATH)

#
# Check for OpenSSL
AX_CHECK_OPENSSL
AM_CONDITIONAL(PPP_WITH_OPENSSL, test "x${with_openssl}" != "xno")
AM_COND_IF([PPP_WITH_OPENSSL],
    AC_DEFINE([PPP_WITH_OPENSSL], 1, [PPP is compiled with openssl support]))

#
# Check if OpenSSL has compiled in support for various ciphers
AS_IF([test "x${with_openssl}" != "xno" ], [
    AX_CHECK_OPENSSL_DEFINE([OPENSSL_NO_MD4], [md4])
    AX_CHECK_OPENSSL_DEFINE([OPENSSL_NO_MD5], [md5])
    AX_CHECK_OPENSSL_DEFINE([OPENSSL_NO_DES], [des])
    AX_CHECK_OPENSSL_DEFINE([OPENSSL_NO_SHA], [sha])
], [
    AS_IF([test "x${enable_eaptls}" != "xno" || test "x${enable_peap}" != "xno"],
        [AC_MSG_ERROR([OpenSSL not found, and if this is your intention then run configure --disable-eaptls and --disable-peap])])
])

AM_CONDITIONAL([OPENSSL_HAVE_MD4], test "x${ac_cv_openssl_md4}" = "xyes")
AM_COND_IF([OPENSSL_HAVE_MD4],
    AC_DEFINE([OPENSSL_HAVE_MD4], 1, [Use MD4 included with openssl]))

AM_CONDITIONAL([OPENSSL_HAVE_MD5], test "x${ac_cv_openssl_md5}" = "xyes")
AM_COND_IF([OPENSSL_HAVE_MD5],
    AC_DEFINE([OPENSSL_HAVE_MD5], 1, [Use MD5 included with openssl]))

AM_CONDITIONAL([OPENSSL_HAVE_SHA], test "x${ac_cv_openssl_sha}" = "xyes")
AM_COND_IF([OPENSSL_HAVE_SHA],
    AC_DEFINE([OPENSSL_HAVE_SHA], 1, [Use SHA included with openssl]))

AM_CONDITIONAL([OPENSSL_HAVE_DES], test "x${ac_cv_openssl_des}" = "xyes")
AM_COND_IF([OPENSSL_HAVE_DES],
    AC_DEFINE([OPENSSL_HAVE_DES], 1, [Use DES included with openssl]))

#
# With libsrp support
AX_CHECK_SRP([
    AC_DEFINE([PPP_WITH_SRP], 1, [Support for libsrp authentication module])])

#
# With libatm support
AX_CHECK_ATM

#
# With libpam support
AX_CHECK_PAM(AC_DEFINE([PPP_WITH_PAM], 1, [Support for Pluggable Authentication Modules]))
AM_CONDITIONAL(PPP_WITH_PAM, test "x${with_pam}" = "xyes")

#
# With libpcap support, activate pppd on network activity
AX_CHECK_PCAP

#
# SunOS provides a version of libpcap that would work, but SunOS has no support for activity filter
AM_CONDITIONAL([PPP_WITH_FILTER], [ test "x${with_pcap}" = "xyes" && test "x${build_sunos}" != "xyes" ])
AM_COND_IF([PPP_WITH_FILTER], [
    AC_DEFINE([PPP_WITH_FILTER], 1, [Have packet activity filter support])], [
    AS_IF([test "x${build_sunos}" = "xyes"], [
        AC_MSG_WARN([Packet activity filter not supported on SunOS])
        with_pcap="no"
        ])
    ])

AC_DEFINE_UNQUOTED(PPPD_VERSION, "$VERSION", [Version of pppd])

AC_CONFIG_FILES([
    Makefile
    chat/Makefile
    include/Makefile
    pppd/Makefile
    pppd/pppd.pc
    pppd/plugins/Makefile
    pppd/plugins/pppoe/Makefile
    pppd/plugins/pppoatm/Makefile
    pppd/plugins/pppol2tp/Makefile
    pppd/plugins/radius/Makefile
    pppdump/Makefile
    pppstats/Makefile
    scripts/Makefile
    ])
AC_OUTPUT


echo "
$PACKAGE_NAME version $PACKAGE_VERSION
    Prefix...............: $prefix
    Runtime Dir..........: $PPPD_RUNTIME_DIR
    Logfile Dir..........: $PPPD_LOGFILE_DIR
    Plugin Dir...........: $PPPD_PLUGIN_DIR
    System CA Path ......: ${SYSTEM_CA_PATH:-not set}
    With OpenSSL.........: ${with_openssl:-yes}
    With libatm..........: ${with_atm:-no}
    With libpam..........: ${with_pam:-no}
    With libpcap.........: ${with_pcap:-no}
    With libsrp..........: ${with_srp:-no}
    C Compiler...........: $CC $CFLAGS
    Linker...............: $LD $LDFLAGS $LIBS

Features enabled
    Microsoft Extensions.: ${enable_microsoft_extensions:-yes}
    Multilink............: ${enable_multilink:-no}
    Plugins..............: ${enable_plugins:-yes}
    CBCP.................: ${enable_cbcp:-no}
    IPV6CP...............: ${enable_ipv6cp:-yes}
    EAP-TLS..............: ${enable_eaptls:-yes}
    systemd notifications: ${enable_systemd:-no}
"
