AC_INIT([DHCP],[4.4.1],[dhcp-users@isc.org])


# we specify "foreign" to avoid having to have the GNU mandated files,
# like AUTHORS, COPYING, and such
AM_INIT_AUTOMAKE([foreign])

# we specify AM_MAINTAINER_MODE to avoid problems with rebuilding
# the configure and makefiles.  Without it users doing things that
# change the timestamps on the code, like checking it into a cvs
# tree, could trigger a rebuild of the infrastructure files which
# might fail if they don't have the correct tools.
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

# We want to turn on warnings if we are using gcc and the user did
# not specify CFLAGS. The autoconf check for the C compiler sets the
# CFLAGS if gcc is used, so we will save it before we run that check.
SAVE_CFLAGS="$CFLAGS"

# Now find our C compiler.
AC_PROG_CC

# Suppress warnings about --datarootdir
AC_DEFUN([AC_DATAROOTDIR_CHECKED])

# If we have gcc, and AC_PROG_CC changed the flags, then we know the
# user did not specify any flags. Add warnings in this case.
if test "$GCC" = "yes"; then
	if test "$CFLAGS" != "$SAVE_CFLAGS"; then
		STD_CWARNINGS="$STD_CWARNINGS -Wall -Werror -fno-strict-aliasing"
	fi
fi

# We can have some flags to pass to bind configure
BINDCONFIG=
if test "$cross_compiling" = "yes"; then
	BINDCONFIG="--host=$host"
fi
# Pass CFLAGS and co. $ac_configure_args looks like "'arg1' 'arg2' ..."
# and as there can be a space inside an argument some magic is required.
# This sets $1 ... $N to my_configure_args, arg1 ... argN
eval "set my_configure_args $ac_configure_args"
# remove my_configure_args, i.e., the guard against empty $ac_configure_args
shift
# iterate on arguments and copying 'arg' when it begins by an upper case
for a
do
	case $a in
		[[A-Z]]*) BINDCONFIG="$BINDCONFIG '$a'" ;;
	esac
done
AC_SUBST(BINDCONFIG)

# POSIX doesn't include the IPv6 Advanced Socket API and glibc hides
# parts of the IPv6 Advanced Socket API as a result.  This is stupid
# as it breaks how the two halves (Basic and Advanced) of the IPv6
# Socket API were designed to be used but we have to live with it.
# Use this to define _GNU_SOURCE to pull in the IPv6 Advanced Socket API.
AC_USE_SYSTEM_EXTENSIONS

AC_PROG_RANLIB

AC_PATH_PROG(AR, ar)
AC_SUBST(AR)

if test "X$AR" = "X"; then
	AC_MSG_ERROR([
ar program not found.  Please fix your PATH to include the directory in
which ar resides, or set AR in the environment with the full path to ar.])
fi

AC_CONFIG_HEADERS([includes/config.h])

# we sometimes need to know byte order for building packets
AC_C_BIGENDIAN(AC_SUBST(byte_order, BIG_ENDIAN),
	       AC_SUBST(byte_order, LITTLE_ENDIAN))
AC_DEFINE_UNQUOTED([DHCP_BYTE_ORDER], [$byte_order],
		   [Define to BIG_ENDIAN for MSB (Motorola or SPARC CPUs)
		    or LITTLE_ENDIAN for LSB (Intel CPUs).])

# Optional compile-time DEBUGging.
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[create a debug-only version of the software (default is no).]),
  [case "${enableval}" in
    yes) enable_debug=yes
        AC_DEFINE([DEBUG], [1],
            [Define to compile debug-only DHCP software.])
        # Just override CFLAGS totally to remove optimization.
        CFLAGS="-g";;
    no)  enable_debug=no ;;
    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
  esac],[enable_debug=no])

# XXX: there are actually quite a lot more DEBUG_ features we could enable,
# but I don't want to pollute the --help space.
#
#/* #define DEBUG_TOKENS */
#/* #define DEBUG_PACKET */
#/* #define DEBUG_EXPRESSIONS */
#/* #define DEBUG_FIND_LEASE */
#/* #define DEBUG_EXPRESSION_PARSE */
#/* #define DEBUG_CLASS_MATCHING */
#/* #define DEBUG_MEMORY_LEAKAGE */
#/* #define DEBUG_MALLOC_POOL */
#/* #define DEBUG_LEASE_STATE_TRANSITIONS */
#/* #define DEBUG_RC_HISTORY */
#/* #define DEBUG_RC_HISTORY_EXHAUSTIVELY */
#/* #define RC_HISTORY_MAX 10240 */
#/* #define POINTER_DEBUG */
#/* #define DEBUG_FAILOVER_MESSAGES */
#/* #define DEBUG_FAILOVER_TIMING */
#/* #define DEBUG_DUMP_ALL_LEASES */

# Failover optional compile-time feature.
AC_ARG_ENABLE(failover,
	AS_HELP_STRING([--enable-failover],[enable support for failover (default is yes)]))
# Failover is on by default, so define if it is not explicitly disabled.
if test "$enable_failover" != "no"; then
    enable_failover="yes"
	AC_DEFINE([FAILOVER_PROTOCOL], [1],
		  [Define to include Failover Protocol support.])
fi

# execute() support.
AC_ARG_ENABLE(execute,
	AS_HELP_STRING([--enable-execute],[enable support for execute() in config (default is yes)]))
# execute() is on by default, so define if it is not explicitly disabled.
if test "$enable_execute" != "no" ; then
    enable_execute="yes"
	AC_DEFINE([ENABLE_EXECUTE], [1],
		  [Define to include execute() config language support.])
fi

# Server tracing support.
AC_ARG_ENABLE(tracing,
	AS_HELP_STRING([--enable-tracing],[enable support for server activity tracing (default is yes)]))
# tracing is on by default, so define if it is not explicitly disabled.
if test "$enable_tracing" != "no" ; then
	AC_DEFINE([TRACING], [1],
		  [Define to include server activity tracing support.])
fi

# Delayed-ack feature support.
AC_ARG_ENABLE(delayed_ack,
	AS_HELP_STRING([--enable-delayed-ack],[queues multiple DHCPACK replies (default is yes)]))
if test "$enable_delayed_ack" != "no"; then
    enable_delayed_ack="yes"
	AC_DEFINE([DELAYED_ACK], [1],
		  [Define to queue multiple DHCPACK replies per fsync.])
fi

# DHCPv6 optional compile-time feature.
AC_ARG_ENABLE(dhcpv6,
	AS_HELP_STRING([--enable-dhcpv6],[enable support for DHCPv6 (default is yes)]))
# DHCPv6 is on by default, so define if it is not explicitly disabled.
if test "$enable_dhcpv6" != "no"; then
    enable_dhcpv6="yes"
	AC_DEFINE([DHCPv6], [1],
		  [Define to 1 to include DHCPv6 support.])
fi

# DHCPv4o6 optional compile-time feature.
AC_ARG_ENABLE(dhcpv4o6,
	AS_HELP_STRING([--enable-dhcpv4o6],[enable support for DHCPv4-over-DHCPv6 (default is no)]))
# DHCPv4o6 is off by default, so define if it is explicitly enabled.
if test "$enable_dhcpv4o6" = "yes"; then
	# DHCPv4o6 requires DHCPv6
	if test "$enable_dhcpv6" = "no"; then
		AC_MSG_ERROR([dhcpv4o6 requires dhcpv6])
	fi
	AC_DEFINE([DHCP4o6], [1], 
		  [Define to 1 to include DHCPv4 over DHCPv6 support.])
else
    # so we can report below
    enable_dhcpv4o6="no"
fi

# Relay port (draft-ietf-dhc-relay-port-10.txt) optional compile-time feature.
AC_ARG_ENABLE(relay-port,
	AS_HELP_STRING([--enable-relay-port],[enable support for relay port (default is no)]))
# Relay port is off by default (for now)
if test "$enable_relay_port" = "yes"; then
	AC_DEFINE([RELAY_PORT], [1],
		  [Define to 1 to include relay port support.])
else
	# so we can report below
	enable_relay_port="no"
fi

# PARANOIA is off by default (until we can test it with all features)
AC_ARG_ENABLE(paranoia,
	AS_HELP_STRING([--enable-paranoia],[enable support for chroot/setuid (default is no)]))
AC_ARG_ENABLE(early_chroot,
	AS_HELP_STRING([--enable-early-chroot],[enable chrooting prior to configuration (default is no)]))
# If someone enables early chroot, but does not enable paranoia, do so for
# them.
if test "$enable_paranoia" != "yes" && \
   test "$enable_early_chroot" = "yes" ; then
	enable_paranoia="yes"
fi

if test "$enable_paranoia" = "yes" ; then
	AC_DEFINE([PARANOIA], [1],
		  [Define to any value to include Ari's PARANOIA patch.])
fi
if test "$enable_early_chroot" = "yes" ; then
	AC_DEFINE([EARLY_CHROOT], [1],
		  [Define to any value to chroot() prior to loading config.])
fi

AC_ARG_ENABLE(ipv4_pktinfo,
	AS_HELP_STRING([--enable-ipv4-pktinfo],[enable use of pktinfo on IPv4 sockets (default is no)]))

if test "$enable_ipv4_pktinfo" = "yes"; then
	AC_DEFINE([USE_V4_PKTINFO], [1],
		[Define to 1 to enable IPv4 packet info support.])
fi

AC_ARG_ENABLE(use_sockets,
	AS_HELP_STRING([--enable-use-sockets],[use the standard BSD socket API (default is no)]))

if test "$enable_use_sockets" = "yes"; then
	AC_DEFINE([USE_SOCKETS], [1],
		[Define to 1 to use the standard BSD socket API.])
fi

# Include the PID in the log messages.  This is useful when there may
# be multiple instances of a program.
# This is off by default
AC_ARG_ENABLE(log_pid,
	AS_HELP_STRING([--enable-log-pid],[Include PIDs in syslog messages (default is no).]))
if test "$enable_log_pid" = "yes" ; then
	AC_DEFINE([USE_LOG_PID], [1],
		[Define to include PIDs in syslog messages.])
fi

# Allow for binary search when inserting v4 leases into queues
AC_ARG_ENABLE(binary_leases,
	AS_HELP_STRING([--enable-binary-leases],[enable support for binary insertion of leases (default is no)]))
# binary_leases is off by default.
if test "$enable_binary_leases" = "yes"; then
	AC_DEFINE([BINARY_LEASES], [1],
		  [Define to support binary insertion of leases into queues.])
else
    enable_binary_leases="no"
fi

# Testing section

DISTCHECK_ATF_CONFIGURE_FLAG=
atf_path="no"
AC_ARG_WITH([atf],
    AS_HELP_STRING([--with-atf=PATH],[specify location where atf was installed (or "bind")]),
    [atf_path="$withval"])
AM_CONDITIONAL(BIND_ATF, test "$atf_path" = "bind")
if test "$atf_path" = "bind" ; then
    DISTCHECK_ATF_CONFIGURE_FLAG="--with-atf=bind"
    atf_pcp="bind"
    atf_path="\${top_srcdir}/bind/atf"
    ATF_CFLAGS="-I$atf_path/include -DUNIT_TEST"
    ATF_LDFLAGS="-L$atf_path/lib -latf-c"
    ATF_BIN=`cd $srcdir; pwd`/bind/atf/bin
    AC_SUBST(ATF_CFLAGS)
    AC_SUBST(ATF_LDFLAGS)
    AC_SUBST(ATF_BIN)
    BINDCONFIG="$BINDCONFIG --with-atf"
elif test "$atf_path" != "no" ; then
    DISTCHECK_ATF_CONFIGURE_FLAG="--with-atf=$atf_path"
    # Config path for pkg-config
    atf_pcp=""
    if test "$atf_path" != "yes" ; then
        if test -f $atf_path/lib/pkgconfig/atf-c.pc ; then
            atf_pcp=$atf_path/lib/pkgconfig
        elif test -f $atf_path/lib64/pkgconfig/atf-c.pc ; then
            atf_pcp=$atf_path/lib64/pkgconfig
        fi
    else
        # Not specified, try some common paths
        atf_dirs="/usr /usr/local /usr/pkg /opt /opt/local"
        for d in $atf_dirs
        do
            if test -f $d/lib/pkgconfig/atf-c.pc ; then
                atf_pcp=$d/lib/pkgconfig
                atf_path=$d
            elif test -f $d/lib64/pkgconfig/atf-c.pc ; then
                atf_pcp=$d/lib64/pkgconfig
                atf_path=$d
            fi
        done
    fi

    if test "$atf_pcp" = "" ; then
        AC_MSG_ERROR([Unable to find atf files in location specified])
    else
        AC_CHECK_PROG([pkgcfg_found],[pkg-config],[pkg-config],[])
        if test "$pkgcfg_found" = ""; then
            AC_MSG_ERROR([Could not locate ATF, pkg-config not installed])
        fi
        ATF_CFLAGS="`PKG_CONFIG_PATH=$atf_pcp pkg-config --cflags atf-c` -DUNIT_TEST"
        ATF_LDFLAGS="`PKG_CONFIG_PATH=$atf_pcp pkg-config --libs atf-c`"
        if  test -f $atf_pcp/atf-sh.pc ; then
                ATF_BIN="`PKG_CONFIG_PATH=$atf_pcp pkg-config --variable=exec_prefix atf-sh`/bin"
        else
                # older versions don't have atf-sh, try usual place
                ATF_BIN=$atf_path/bin
        fi

        UNITTESTS=tests

        AC_SUBST(ATF_CFLAGS)
        AC_SUBST(ATF_LDFLAGS)
        AC_SUBST(ATF_BIN)
        AC_SUBST(UNITTESTS)
    fi
fi

AM_CONDITIONAL(HAVE_ATF, test "$atf_pcp" != "")
AM_COND_IF([HAVE_ATF], [AC_DEFINE([HAVE_ATF], [1], [ATF framework specified?])])
AC_SUBST(DISTCHECK_ATF_CONFIGURE_FLAG)

###
### Path fun.  Older versions of DHCP were installed in /usr/sbin, so we
### need to look there and potentially overwrite by default (but not if
### the user configures an alternate value).  LOCALSTATEDIR is totally
### braindead.  No one uses /usr/local/var/db/ nor /usr/local/var/run, and
### they would be insane for suggesting it.  We need to look in /var/for
### 'db' and 'state/dhcp' for db files, and /var/run for pid files by
### default.
###
AC_PREFIX_PROGRAM(dhcpd)

# XXX - isn't there SOME WAY to default autoconf to /var instead of
# /usr/local/var/no/one/has/this/please/stop/trying?
case "$localstatedir" in
	'${prefix}/var')
		localstatedir=/var
		;;
esac

# Default server configuration file.
AC_ARG_WITH(srv-conf-file,
	AS_HELP_STRING([--with-srv-conf-file=PATH],[Default file containing dhcpd configuration
		        (default is typically /etc/dhcpd.conf)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCPD_CONF], ["$withval"],
			   [Default file containing dhcpd configuration.]))

# Allow specification of alternate state files
AC_ARG_WITH(srv-lease-file,
	AS_HELP_STRING([--with-srv-lease-file=PATH],[File for dhcpd leases
		        (default is LOCALSTATEDIR/db/dhcpd.leases)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCPD_DB], ["$withval"],
			   [File for dhcpd leases.]))

AC_MSG_CHECKING([for dhcpd.leases location])
if [[ "x$with_srv_lease_file" = "x" ]] ; then
	if [[ -d "${localstatedir}/db" ]] ; then
		with_srv_lease_file="${localstatedir}/db/dhcpd.leases"
	elif [[ -d "${localstatedir}/state" ]] ; then
		if [[ -d "${localstatedir}/state/dhcp" ]] ; then
			with_srv_lease_file="${localstatedir}/state/dhcp/dhcpd.leases"
		else
			with_srv_lease_file="${localstatedir}/state/dhcpd.leases"
		fi
	elif [[ -d "${localstatedir}/lib" ]] ; then
		if [[ -d "${localstatedir}/lib/dhcp" ]] ; then
			with_srv_lease_file="${localstatedir}/lib/dhcp/dhcpd.leases"
		else
			with_srv_lease_file="${localstatedir}/lib/dhcpd.leases"
		fi
	elif [[ -d "${localstatedir}/etc" ]] ; then
		with_srv_lease_file="${localstatedir}/etc/dhcpd.leases"
	else
		with_srv_lease_file="/etc/dhcpd.leases"
	fi
fi
AC_MSG_RESULT($with_srv_lease_file)

AC_ARG_WITH(srv6-lease-file,
	AS_HELP_STRING([--with-srv6-lease-file=PATH],[File for dhcpd6 leases
		        (default is LOCALSTATEDIR/db/dhcpd6.leases)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCPD6_DB], ["$withval"],
			   [File for dhcpd6 leases.]))

AC_MSG_CHECKING([for dhcpd6.leases location])
if [[ "x$with_srv6_lease_file" = "x" ]] ; then
	if [[ -d "${localstatedir}/db" ]] ; then
		with_srv6_lease_file="${localstatedir}/db/dhcpd6.leases"
	elif [[ -d "${localstatedir}/state" ]] ; then
		if [[ -d "${localstatedir}/state/dhcp" ]] ; then
			with_srv6_lease_file="${localstatedir}/state/dhcp/dhcpd6.leases"
		else
			with_srv6_lease_file="${localstatedir}/state/dhcpd6.leases"
		fi
	elif [[ -d "${localstatedir}/lib" ]] ; then
		if [[ -d "${localstatedir}/lib/dhcp" ]] ; then
			with_srv6_lease_file="${localstatedir}/lib/dhcp/dhcpd6.leases"
		else
			with_srv6_lease_file="${localstatedir}/lib/dhcpd6.leases"
		fi
	elif [[ -d "${localstatedir}/etc" ]] ; then
		with_srv6_lease_file="${localstatedir}/etc/dhcpd6.leases"
	else
		with_srv6_lease_file="/etc/dhcpd6.leases"
	fi
fi
AC_MSG_RESULT($with_srv6_lease_file)

AC_ARG_WITH(cli-lease-file,
	AS_HELP_STRING([--with-cli-lease-file=PATH],[File for dhclient leases
		        (default is LOCALSTATEDIR/db/dhclient.leases)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCLIENT_DB], ["$withval"],
			   [File for dhclient leases.]))

AC_MSG_CHECKING([for dhclient.leases location])
if [[ "x$with_cli_lease_file" = "x" ]] ; then
	if [[ -d "${localstatedir}/db" ]] ; then
		with_cli_lease_file="${localstatedir}/db/dhclient.leases"
	elif [[ -d "${localstatedir}/state" ]] ; then
		if [[ -d "${localstatedir}/state/dhcp" ]] ; then
			with_cli_lease_file="${localstatedir}/state/dhcp/dhclient.leases"
		else
			with_cli_lease_file="${localstatedir}/state/dhclient.leases"
		fi
	elif [[ -d "${localstatedir}/lib" ]] ; then
		if [[ -d "${localstatedir}/lib/dhcp" ]] ; then
			with_cli_lease_file="${localstatedir}/lib/dhcp/dhclient.leases"
		else
			with_cli_lease_file="${localstatedir}/lib/dhclient.leases"
		fi
	elif [[ -d "${localstatedir}/etc" ]] ; then
		with_cli_lease_file="${localstatedir}/etc/dhclient.leases"
	else
		with_cli_lease_file="/etc/dhclient.leases"
	fi
fi
AC_MSG_RESULT($with_cli_lease_file)

AC_ARG_WITH(cli6-lease-file,
	AS_HELP_STRING([--with-cli6-lease-file=PATH],[File for dhclient6 leases
		        (default is LOCALSTATEDIR/db/dhclient6.leases)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCLIENT6_DB], ["$withval"],
			   [File for dhclient6 leases.]))

AC_MSG_CHECKING([for dhclient6.leases location])
if [[ "x$with_cli6_lease_file" = "x" ]] ; then
	if [[ -d "${localstatedir}/db" ]] ; then
		with_cli6_lease_file="${localstatedir}/db/dhclient6.leases"
	elif [[ -d "${localstatedir}/state" ]] ; then
		if [[ -d "${localstatedir}/state/dhcp" ]] ; then
			with_cli6_lease_file="${localstatedir}/state/dhcp/dhclient6.leases"
		else
			with_cli6_lease_file="${localstatedir}/state/dhclient6.leases"
		fi
	elif [[ -d "${localstatedir}/lib" ]] ; then
		if [[ -d "${localstatedir}/lib/dhcp" ]] ; then
			with_cli6_lease_file="${localstatedir}/lib/dhcp/dhclient6.leases"
		else
			with_cli6_lease_file="${localstatedir}/lib/dhclient6.leases"
		fi
	elif [[ -d "${localstatedir}/etc" ]] ; then
		with_cli6_lease_file="${localstatedir}/etc/dhclient6.leases"
	else
		with_cli6_lease_file="/etc/dhclient6.leases"
	fi
fi
AC_MSG_RESULT($with_cli6_lease_file)

AC_ARG_WITH(srv-pid-file,
	AS_HELP_STRING([--with-srv-pid-file=PATH],[File for dhcpd process information
		        (default is LOCALSTATEDIR/run/dhcpd.pid)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCPD_PID], ["$withval"],
			   [File for dhcpd process information.]))
AC_ARG_WITH(srv6-pid-file,
	AS_HELP_STRING([--with-srv6-pid-file=PATH],[File for dhcpd6 process information
		        (default is LOCALSTATEDIR/run/dhcpd6.pid)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCPD6_PID], ["$withval"],
			   [File for dhcpd6 process information.]))
AC_ARG_WITH(cli-pid-file,
	AS_HELP_STRING([--with-cli-pid-file=PATH],[File for dhclient process information
		        (default is LOCALSTATEDIR/run/dhclient.pid)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCLIENT_PID], ["$withval"],
			   [File for dhclient process information.]))
AC_ARG_WITH(cli6-pid-file,
	AS_HELP_STRING([--with-cli6-pid-file=PATH],[File for dhclient6 process information
		        (default is LOCALSTATEDIR/run/dhclient6.pid)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCLIENT6_PID], ["$withval"],
			   [File for dhclient6 process information.]))
AC_ARG_WITH(relay-pid-file,
	AS_HELP_STRING([--with-relay-pid-file=PATH],[File for dhcrelay process information
		        (default is LOCALSTATEDIR/run/dhcrelay.pid)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCRELAY_PID], ["$withval"],
			   [File for dhcrelay process information.]))
AC_ARG_WITH(relay6-pid-file,
	AS_HELP_STRING([--with-relay6-pid-file=PATH],[File for dhcrelay6 process information
		        (default is LOCALSTATEDIR/run/dhcrelay6.pid)]),
	AC_DEFINE_UNQUOTED([_PATH_DHCRELAY6_PID], ["$withval"],
			   [File for dhcrelay6 process information.]))

# Check basic types.
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T

# Some systems need the u_intX_t types defined across.
AC_CHECK_TYPE([u_int8_t], [], [
  AC_TYPE_UINT8_T
  AC_DEFINE(u_int8_t, [uint8_t], [Define a type for 8-bit unsigned
				  integers.])
])
AC_CHECK_TYPE([u_int16_t], [], [
  AC_TYPE_UINT16_T
  AC_DEFINE(u_int16_t, [uint16_t], [Define a type for 16-bit unsigned
				    integers.])
])
AC_CHECK_TYPE([u_int32_t], [], [
  AC_TYPE_UINT32_T
  AC_DEFINE(u_int32_t, [uint32_t], [Define a type for 32-bit unsigned
				    integers.])
])
AC_CHECK_TYPE([u_int64_t], [], [
  AC_TYPE_UINT64_T
  AC_DEFINE(u_int64_t, [uint64_t], [Define a type for 64-bit unsigned
				    integers.])
])

# see if ifaddrs.h is available
AC_CHECK_HEADERS(ifaddrs.h)

# figure out what IPv4 interface code to use
AC_CHECK_HEADERS(linux/types.h)  # needed for linux/filter.h on old systems

relay_port_supported="no"
AC_CHECK_HEADER(linux/filter.h, DO_LPF=1, ,
[
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
])
if test -n "$DO_LPF"
then
	AC_DEFINE([HAVE_LPF], [1],
		  [Define to 1 to use the Linux Packet Filter interface code.])
	relay_port_supported="yes"
else
	AC_CHECK_HEADER(sys/dlpi.h, DO_DLPI=1)
	if test -n "$DO_DLPI"
	then
		AC_DEFINE([HAVE_DLPI], [1],
			  [Define to 1 to use DLPI interface code.])
	else
		AC_CHECK_HEADER(net/bpf.h, DO_BPF=1)
		if test -n "$DO_BPF"
		then
			AC_DEFINE([HAVE_BPF], [1],
				   [Define to 1 to use the
				   Berkeley Packet Filter interface code.])
			relay_port_supported="yes"
		fi
	fi
fi

if test "$enable_relay_port" = "yes"; then
	if test "$relay_port_supported" != "yes"; then
		AC_MSG_ERROR([--enable-relay-port requires BPF or LPF])
	fi
fi

# SIOCGLIFCONF uses some transport structures.  Trick is not all platforms
# use the same structures.  We like to use 'struct lifconf' and 'struct
# lifreq', but we'll use these other structures if they're present.  HPUX
# does not define 'struct lifnum', but does use SIOCGLIFNUM - they use an
# int value.
#
AC_MSG_CHECKING([for struct lifnum])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ #include <sys/types.h>
  #include <sys/socket.h>
  #include <net/if.h>
]], [[ struct lifnum a;
]])],[AC_MSG_RESULT(yes)
	 AC_DEFINE([ISC_PLATFORM_HAVELIFNUM], [1],
		   [Define to 1 if the system has 'struct lifnum'.])],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for struct if_laddrconf])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ #include <sys/types.h>
  #include <net/if6.h>
]], [[ struct if_laddrconf a;
]])],[AC_MSG_RESULT(yes)
	 AC_DEFINE([ISC_PLATFORM_HAVEIF_LADDRCONF], [1],
		   [Define to 1 if the system has 'struct if_laddrconf'.])],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for struct if_laddrreq])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
 #include <net/if6.h>
]], [[ struct if_laddrreq a;
]])],[AC_MSG_RESULT(yes)
	 AC_DEFINE([ISC_PLATFORM_HAVEIF_LADDRREQ], [1],
		   [Define to 1 if the system has 'struct if_laddrreq'.])],[AC_MSG_RESULT(no)])

#
# check for GCC noreturn attribute
#
AC_MSG_CHECKING(for GCC noreturn attribute)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[void foo() __attribute__((noreturn));]])],[AC_MSG_RESULT(yes)
	 AC_DEFINE([ISC_DHCP_NORETURN], [__attribute__((noreturn))],
		   [Define to the string for a noreturn attribute.])],[AC_MSG_RESULT(no)
	 AC_DEFINE([ISC_DHCP_NORETURN], [],
		   [Define to the string for a noreturn attribute.])])

# Look for optional headers.
AC_CHECK_HEADERS(sys/socket.h net/if_dl.h net/if6.h regex.h)

# Solaris needs some libraries for functions
AC_SEARCH_LIBS(socket, [socket])
AC_SEARCH_LIBS(inet_ntoa, [nsl])

AC_SEARCH_LIBS(inet_aton, [socket nsl], ,
	AC_DEFINE([NEED_INET_ATON], [1],
		  [Define to 1 if the inet_aton() function is missing.]))

# Check for a standalone regex library.
AC_SEARCH_LIBS(regcomp, [regex])

AC_CHECK_FUNCS(strlcat)

# For HP/UX we need -lipv6 for if_nametoindex, perhaps others.
AC_SEARCH_LIBS(if_nametoindex, [ipv6])

# For some Solaris nanosleep is found by BIND in librt
have_nanosleep="no"
AC_CHECK_FUNC(nanosleep, have_nanosleep="yes")
if test "$have_nanosleep" = "no"; then
	AC_CHECK_LIB(rt, nanosleep, have_nanosleep="rt")
fi
if test "$have_nanosleep" = "rt"; then
	LIBS="-lrt $LIBS"
fi

# check for /dev/random (declares HAVE_DEV_RANDOM)
AC_MSG_CHECKING(for random device)
AC_ARG_WITH(randomdev,
	AS_HELP_STRING([--with-randomdev=PATH],[Path for random device
		       (default is /dev/random)]),
	use_randomdev="$withval", use_randomdev="unspec")
if test "$use_randomdev" = "unspec"; then
	if test "$cross_compiling" = "yes"; then
		AC_MSG_RESULT(unspecified)
		AC_MSG_ERROR([ need --with-randomdev=PATH or --with-randomdev=no])
	fi
	use_randomdev="/dev/random"
elif test "$use_randomdev" = "yes"; then
	use_randomdev="/dev/random"
fi
if test "$use_randomdev" = "no"; then
	AC_MSG_RESULT(disabled)
	BINDCONFIG="$BINDCONFIG --with-randomdev=no"
else
	if test "$cross_compiling" = "yes"; then
		AC_MSG_RESULT($use_randomdev (unchecked))
	else
		AC_MSG_RESULT($use_randomdev)
		AC_CHECK_FILE($use_randomdev,
		    AC_DEFINE([HAVE_DEV_RANDOM], [1], 
			[Define to 1 if you have the /dev/random or other configured file.]),
		    AC_MSG_ERROR(cannot find $use_randomdev))
	fi
	BINDCONFIG="$BINDCONFIG --with-randomdev=$use_randomdev"
fi

BINDIOMUX="--disable-kqueue --disable-epoll --disable-devpoll"
# check kqueue/epoll/devpoll alternative to select
AC_ARG_ENABLE(kqueue,
	AS_HELP_STRING([--enable-kqueue],[use BSD kqueue (default is no)]),
	want_kqueue="$enableval", want_kqueue="no")
if test "$want_kqueue" = "yes"; then
	BINDIOMUX="--enable-kqueue"
	AC_MSG_WARN([--enable-kqueue is not supported: it may lead to issues such as server looping])
fi
AC_ARG_ENABLE(epoll,
	AS_HELP_STRING([--enable-epoll],[use Linux epoll (default is no)]),
	want_epoll="$enableval", want_epoll="no")
if test "$want_epoll" = "yes"; then
	BINDIOMUX="--enable-epoll"
	AC_MSG_WARN([--enable-epoll is not supported: it may lead to issues such as server looping])
fi
AC_ARG_ENABLE(devpoll,
	AS_HELP_STRING([--enable-devpoll],[use /dev/poll (default is no)]),
	want_devpoll="$enableval", want_devpoll="no")
if test "$want_devpoll" = "yes"; then
	BINDIOMUX="--enable-devpoll"
	AC_MSG_WARN([--enable-devpoll is not supported: it may lead to issues such as server looping])
fi
AC_SUBST(BINDIOMUX)

# general extra bind configure arguments
AC_ARG_WITH(bind-extra-config,
	AS_HELP_STRING([--with-bind-extra-config],[configure bind librairies
		       with some extra options (default is none)]),
	use_xbindconfig="$withval", use_xbindconfig="")
case "$use_xbindconfig" in
yes|no|'')
	;;
*)
	BINDCONFIG="$BINDCONFIG $use_xbindconfig"
	AC_MSG_WARN([Most options to bind configure are not supported when used by ISC DHCP])
	;;
esac

# see if there is a "sa_len" field in our interface information structure
AC_CHECK_MEMBER(struct sockaddr.sa_len,
	AC_DEFINE([HAVE_SA_LEN], [],
		  [Define to 1 if the sockaddr structure has a length field.]),
	,
	[#include <sys/socket.h>])

# figure out pointer size
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -I$srcdir"
AC_CHECK_SIZEOF(struct iaddr *, , [
#include "includes/inet.h"
#include <stdio.h>
])
CFLAGS="$SAVE_CFLAGS"

# Solaris does not have the msg_control or msg_controlen members
# in the msghdr structure unless you define:
#
#   _XOPEN_SOURCE, _XOPEN_SOURCE_EXTENDED, and __EXTENSIONS__
#
# See the "standards" man page for details.
#
# We check for the msg_control member, and if it is not found, we check
# again with the appropriate defines added to the CFLAGS. (In order to
# do this we have to remove the check from the cache, which is what the
# "unset" is for.)
AC_CHECK_MEMBER(struct msghdr.msg_control,,
	[CFLAGS="$CFLAGS -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED=1"
	 CFLAGS="$CFLAGS -D__EXTENSIONS__"
	 unset ac_cv_member_struct_msghdr_msg_control
	 AC_CHECK_MEMBER(struct msghdr.msg_control,,
	 	[AC_MSG_ERROR([Missing msg_control member in
			       msg_control structure.])],
		[
#include <sys/types.h>
#include <sys/socket.h>
		])
	],
	[
#include <sys/types.h>
#include <sys/socket.h>
	])

AC_CHECK_MEMBER(struct tpacket_auxdata.tp_vlan_tci,
    [AC_DEFINE([VLAN_TCI_PRESENT], [1], [tpacket_auxdata.tp_vlan_tci present])]
    ,, [#include <linux/if_packet.h>])

# bind/Makefile.in is not from automake so we need 2 variables for bind dir
BINDSUBDIR=
BINDDIR=
BINDSRCDIR=
BINDLIBIRSDIR=
BINDLIBDNSDIR=
BINDLIBISCCFGDIR=
BINDLIBISCDIR=
DISTCHECK_LIBBIND_CONFIGURE_FLAG=
AC_ARG_WITH(libbind,
	AS_HELP_STRING([--with-libbind=PATH],[bind includes and libraries are in PATH]),
	use_libbind="$withval", use_libbind="no")
case "$use_libbind" in
yes)
	AC_MSG_ERROR([PATH is required in --with-libbind=PATH])
	;;
no)
	BINDSUBDIR="\${top_srcdir}/bind"
	my_abs_srcdir=`cd $srcdir && pwd`
	BINDDIR="${my_abs_srcdir}/bind"
	if test ! -d "$srcdir/bind"; then
		AC_MSG_ERROR([Where to find or build bind includes and libraries must be specified])
	fi
	if test -d "$srcdir/bind/bind9"; then
		BINDSRCDIR="${my_abs_srcdir}/bind/bind9"
	else
		if test ! -f "$srcdir/bind/version.tmp"; then
			AC_MSG_ERROR([Cannot find $srcdir/bind/version.tmp])
		fi
		. "$srcdir/bind/version.tmp"
		bindversion=${MAJORVER}.${MINORVER}.${PATCHVER}${RELEASETYPE}${RELEASEVER}
		BINDSRCDIR="${my_abs_srcdir}/bind/bind-$bindversion"
	fi
	AC_CONFIG_FILES([$srcdir/bind/Makefile])

	BINDLIBIRSDIR="$BINDSRCDIR/lib/irs"
	BINDLIBDNSDIR="$BINDSRCDIR/lib/dns"
	BINDLIBISCCFGDIR="$BINDSRCDIR/lib/isccfg"
	BINDLIBISCDIR="$BINDSRCDIR/lib/isc"
	;;
*)
	if test ! -d "$use_libbind"; then
		AC_MSG_ERROR([Cannot find bind directory at $use_libbind])
	fi
	if test ! -d "$use_libbind/include" -o \
	        ! -f "$use_libbind/include/isc/buffer.h"
	then
		AC_MSG_ERROR([Cannot find bind includes at $use_libbind/include])
	fi
	if test	! -d "$use_libbind/lib" -o \
	        \( ! -f "$use_libbind/lib/libisc.a" -a \
		   ! -f	"$use_libbind/lib/libisc.la" \)
	then
		AC_MSG_ERROR([Cannot find bind libraries at $use_libbind/lib])
	fi
	BINDDIR="$use_libbind"
	BINDLIBIRSDIR="$BINDDIR/lib"
	BINDLIBDNSDIR="$BINDDIR/lib"
	BINDLIBISCCFGDIR="$BINDDIR/lib"
	BINDLIBISCDIR="$BINDDIR/lib"	
	DISTCHECK_LIBBIND_CONFIGURE_FLAG="--with-libbind=$use_libbind"
	;;
esac
AC_SUBST(BINDSUBDIR)
AC_SUBST(BINDDIR)
AC_SUBST(BINDSRCDIR)
AC_SUBST(BINDLIBIRSDIR)
AC_SUBST(BINDLIBDNSDIR)
AC_SUBST(BINDLIBISCCFGDIR)
AC_SUBST(BINDLIBISCDIR)
AC_SUBST(DISTCHECK_LIBBIND_CONFIGURE_FLAG)
AM_CONDITIONAL(HAVE_BINDDIR, test "$use_libbind" = "no")

#
# GNU libtool support
#
case "$build_os" in
	sunos*)
		# Just set the maximum command line length for sunos
		# as it otherwise takes a exceptionally long time to
		# work it out. Required for libtool.

		lt_cv_sys_max_cmd_len=4096
		;;
esac

want_libtool="no"

BINDLT=
DISTCHECK_LIBTOOL_CONFIGURE_FLAG=
AC_ARG_ENABLE(libtool,
	AS_HELP_STRING([--enable-libtool],
[use GNU libtool for dynamic shared libraries (default is no).]),
	want_libtool="$enableval")

if test "$use_libbind" != "no"; then
	if test "$want_libtool" = "yes" -a \
	        ! -f "$use_libbind/lib/libisc.la"
	then
		AC_MSG_ERROR([Cannot find dynamic libraries at $use_libbind/lib])
	fi
	if test "$want_libtool" = "no" -a \
	        ! -f "$use_libbind/lib/libisc.a"
	then
		AC_MSG_ERROR([Cannot find static libraries at $use_libbind/lib])
	fi
fi


if test "$want_libtool" = "yes"; then
	AC_MSG_WARN([legacy configure is used but libtool is enabled. Trying to recover...])
	# expand $ac_configure_args
	eval "set my_configure_args $ac_configure_args"
	shift
	cd $srcdir; exec ./config+lt "$@"
	AC_MSG_ERROR([Recovering failed])
fi

DHLIBS=LIBRARIES
A=a

AC_SUBST(DHLIBS)
AC_SUBST(A)
AC_SUBST(BINDLT)
AC_SUBST(DISTCHECK_LIBTOOL_CONFIGURE_FLAG)

# quoting in Makefile.am.in
Q=@
AC_SUBST(Q)

# install bind includes and libraries

want_install_bind="no"
if test "$want_libtool" = "yes"; then
	want_install_bind="yes"
fi
if test "$use_libbind" != "no"; then
	want_install_bind="no"
fi
AC_ARG_ENABLE(bind_install,
	AS_HELP_STRING([--enable-bind-install],
[install bind includes and libraries (default is no).]),
	want_install_bind="$enableval")
if test "$want_install_bind" = "yes"; then
        if test "$use_libbind" != "no"; then
		AC_MSG_WARN([--enable-bind-install does nothing when --with-libbind is set])
	fi
elif test "$want_libtool" = "yes" -a "$use_libbind" = "no"; then
	AC_MSG_WARN([embedded dynamic bind libraries must be installed])
fi
AM_CONDITIONAL(INSTALL_BIND, test "$want_install_bind" = "yes")

# OpenLDAP support.
AC_ARG_WITH(ldap,
    AS_HELP_STRING([--with-ldap],[enable OpenLDAP support in dhcpd (default is no)]),
    [ldap=$withval],
    [ldap=no])

# OpenLDAP with SSL support.
AC_ARG_WITH(ldapcrypto,
    AS_HELP_STRING([--with-ldapcrypto],[enable OpenLDAP crypto support in dhcpd (default is no)]),
    [ldapcrypto=$withval],
    [ldapcrypto=no])

# Gssapi to allow LDAP to authenticate with a keytab
AC_ARG_WITH(ldap-gssapi,
    AC_HELP_STRING([--with-ldap-gssapi],
                   [enable krb5/gssapi authentication for OpenLDAP in dhcpd (default is no)]),
    [ldap_gssapi=$withval],
    [ldap_gssapi=no])


# LDAP CASA auth support.
AC_ARG_WITH(ldapcasa,
    AC_HELP_STRING([--with-ldapcasa],
                   [enable LDAP CASA auth support in dhcpd (default is no)]),
    [ldapcasa=$withval],
    [ldapcasa=no])

# OpenLDAP support is disabled by default, if enabled then SSL support is an
# extra optional that is also disabled by default.  Enabling LDAP SSL support
# implies enabling LDAP support. Similarly, KRB5 support implies LDAP support,
# but doesn't include SSL. The two are not dependant.
if test x$ldap = xyes || test x$ldapcrypto = xyes || test x$ldap_gssapi = xyes; then
    saved_LIBS="$LIBS"
    LIBS=""
    AC_SEARCH_LIBS(ldap_initialize, [ldap], ,
		   AC_MSG_FAILURE([*** Cannot find ldap_initialize with -lldap - do you need to install an OpenLDAP2 Devel package?]))
    AC_SEARCH_LIBS(ber_pvt_opt_on, [lber], ,
		   AC_MSG_FAILURE([*** Cannot find ber_pvt_opt_on with -llber - do you need to install an OpenLDAP2 Devel package?]))
    if test x$ldap_gssapi = xyes ; then
    AC_SEARCH_LIBS(krb5_init_context, [krb5], ,
          AC_MSG_FAILURE([*** Cannot find krb5_init_context with -lkrb5 - do you need to install a Kerberos Devel package?]))
    fi

    # Create LDAP_LIBS which we specify them explicitly rather than lumping them in with LIBS
    AC_SUBST(LDAP_LIBS, [$LIBS])
    LIBS="$saved_LIBS"


    AC_CHECK_HEADERS([ldap.h])
    AC_CHECK_FUNCS([inet_pton inet_ntop])


    LDAP_CFLAGS="-DLDAP_CONFIGURATION"

    if test x$ldapcasa = xyes ; then
    AC_CHECK_HEADERS([micasa_mgmd.h],[
           LDAP_CFLAGS="$LDAP_CFLAGS -DLDAP_CASA_AUTH"
    ], AC_MSG_FAILURE([*** Cannot find micasa_mgmd.h for ldap casa auth support]))
    fi

    if test x$ldapcrypto = xyes ; then
    LDAP_CFLAGS="$LDAP_CFLAGS -DLDAP_USE_SSL"
    fi

    if test x$ldap_gssapi = xyes; then
    LDAP_CFLAGS="$LDAP_CFLAGS -DLDAP_USE_GSSAPI"
    fi

    AC_SUBST(LDAP_CFLAGS, [$LDAP_CFLAGS])
fi

# Append selected warning levels to CFLAGS before substitution (but after
# AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[],[]) & etc).
CFLAGS="$CFLAGS $STD_CWARNINGS"

# Try to add the bind and dhcp include directories
CFLAGS="$CFLAGS -I\$(top_srcdir)/includes -I$BINDDIR/include"

case "$host" in
*-darwin*)
    CFLAGS="$CFLAGS -D__APPLE_USE_RFC_3542";;
*-solaris*)
    # As of Solaris 11, ethernet dev files are in /dev/net
    AC_CHECK_FILE(/dev/net,
		    [AC_DEFINE([USE_DEV_NET], [1],
			[Define to 1 if ethernet devices are in /dev/net])])
    ;;
esac

AC_C_FLEXIBLE_ARRAY_MEMBER

AC_CONFIG_FILES([
  Makefile
  client/Makefile
  client/tests/Makefile
  common/Makefile.am
  common/Makefile
  common/tests/Makefile
  dhcpctl/Makefile.am
  dhcpctl/Makefile
  includes/Makefile
  omapip/Makefile.am
  omapip/Makefile
  relay/Makefile
  server/Makefile
  tests/Makefile.am
  tests/Makefile
  tests/unittest.sh
  server/tests/Makefile
  doc/devel/doxyfile
])
AC_OUTPUT


if test "$enable_dhcpv4o6" = "yes"; then
	DHCP_VERSIONS="DHCPv4, DHCPv6 and DHCPv4-over-DHCPv6"
elif test "$enable_dhcpv6" != "no"; then
	DHCP_VERSIONS="DHCPv4 and DHCPv6"
else
	DHCP_VERSIONS="DHCPv4"
fi

cat > config.report << END

     ISC DHCP source configure results:
    -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

Package:
  Name:          $PACKAGE_NAME
  Version:       $PACKAGE_VERSION

C Compiler:      $CC

Flags:
  DEFS:          $DEFS
  CFLAGS:        $CFLAGS

DHCP versions:   $DHCP_VERSIONS

Features:
  debug:         $enable_debug
  failover:      $enable_failover
  execute:       $enable_execute
  binary-leases: $enable_binary_leases
  dhcpv6:        $enable_dhcpv6
  delayed-ack:   $enable_delayed_ack
  dhcpv4o6:      $enable_dhcpv4o6
  relay-port:    $enable_relay_port

Developer:
  ATF unittests : $atf_path

END
# TODO: Add Perl system tests

if test "$atf_path" != "no"
then
echo "ATF_CFLAGS  : $ATF_CFLAGS" >> config.report
echo "ATF_LDFLAGS : $ATF_LDFLAGS" >> config.report
echo "ATF_BIN : $ATF_BIN" >> config.report
echo
fi

cat config.report

echo
echo Now you can type "make" to build ISC DHCP
echo
