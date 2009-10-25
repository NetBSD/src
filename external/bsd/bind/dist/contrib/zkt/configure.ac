#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
#	@(#) configure.ac 
#
#	2008-06-27	initial setup
#	2008-06-29	add of BIND path checking
#	2008-06-30	add of arg checkings
#	2008-07-02	additional arg checkings
#	2008-07-04	check for getopt_long() added
#	2008-08-30	check for unsigned integer types
#	2008-10-01	if BIND_UTIL_PATH check failed, use config_zkt.h setting as last resort
#	2009-07-30	check for timegm() added
#

AC_PREREQ(2.59)

### Package name and current version
AC_INIT(ZKT, 0.99c, Holger Zuleger hznet.de)
dnl AC_REVISION(Revision: 1.1)

### Files to test to check if src dir contains the package
AC_CONFIG_SRCDIR([dnssec-zkt.c])
AC_CONFIG_HEADER([config.h])


### Checks for programs.
AC_PROG_CC

### find out the path to BIND utils and version
AC_PATH_PROG([SIGNZONE_PROG], dnssec-signzone)
if test -z "$SIGNZONE_PROG" ; then
#        AC_MSG_ERROR([*** 'BIND dnssec-signzone dnssec-keygen' missing, please install or fix your \$PATH ***])
        AC_MSG_WARN([*** 'BIND dnssec-signzone' missing, use default BIND_UTIL_PATH and BIND_VERSION setting out of config_zkt.h ***])
else
	bind_util_path=`dirname "$SIGNZONE_PROG"`
	# define BIND_UTIL_PATH in config.h.in
	AC_DEFINE_UNQUOTED(BIND_UTIL_PATH, "$bind_util_path/", Path to BIND utilities)
	# define BIND_VERSION in config.h.in
	bind_version=`$SIGNZONE_PROG 2>&1 | grep "Version:" | tr -cd "0-9" | sed "s/^\(...\).*/\1/"`
	AC_DEFINE_UNQUOTED(BIND_VERSION, $bind_version, BIND version as integer number without dots)
fi

AC_CHECK_TYPE(uint, unsigned int)
AC_CHECK_TYPE(ulong, unsigned long)
AC_CHECK_TYPE(ushort, unsigned short)
AC_CHECK_TYPE(uchar, unsigned char)

### define configure arguments
AC_ARG_ENABLE([printtimezone], AC_HELP_STRING( [--enable-print-timezone], [print out timezone]), [printtimezone=$enableval])
printtimezone=0
test "$printtimezone" = yes && printtimezone=1
AC_DEFINE_UNQUOTED(PRINT_TIMEZONE, $printtimezone, print out timezone)

AC_ARG_ENABLE([printyear], AC_HELP_STRING( [--enable-print-age], [print age of year]), [printyear=$enableval])
printyear=0
test "$printyear" = yes && printyear=1
AC_DEFINE_UNQUOTED(PRINT_AGE_OF_YEAR, $printyear, print age of year)

AC_ARG_ENABLE([logprogname], AC_HELP_STRING( [--enable-log-progname], [log with progname]), [logprogname=$enableval])
logprogname=0
test "$logprogname" = yes && logprogname=1
AC_DEFINE_UNQUOTED(LOG_WITH_PROGNAME, $logprogname, log with progname)

AC_ARG_ENABLE([logtimestamp], AC_HELP_STRING( [--disable-log-timestamp], [do not log with timestamp]), [logtimestamp=$enableval])
logtimestamp=1
test "$logtimestamp" = no && logtimestamp=0
AC_DEFINE_UNQUOTED(LOG_WITH_TIMESTAMP, $logtimestamp, log with timestamp)

AC_ARG_ENABLE([loglevel], AC_HELP_STRING( [--disable-log-level], [do not log with level]), [loglevel=$enableval])
loglevel=1
test "$loglevel" = no && loglevel=0
AC_DEFINE_UNQUOTED(LOG_WITH_LEVEL, $loglevel, log with level)

AC_ARG_ENABLE([ttl_in_keyfile], AC_HELP_STRING( [--disable-ttl-in-keyfiles], [do not allow TTL values in keyfiles]), [ttl_in_keyfile=$enableval])
ttl_in_keyfile=1
test "$ttl_in_keyfile" = no && ttl_in_keyfile=0
AC_DEFINE_UNQUOTED(TTL_IN_KEYFILE_ALLOWED, $ttl_in_keyfile, TTL in keyfiles allowed)

configpath="/var/named"
AC_ARG_ENABLE([configpath], 
	AC_HELP_STRING( [--enable-configpath=PATH], [set path of config file (defaults to /var/named)]),
	[configpath=$enableval])
case "$configpath" in
yes)
	configpath="/var/named"
	;;
no)
	configpath=""
	;;
*)
	;;
esac
AC_DEFINE_UNQUOTED(CONFIG_PATH, "$configpath/", [set path of config file (defaults to /var/named)])

usetree=1
t="T"
AC_ARG_ENABLE([tree], 
	AC_HELP_STRING( [--disable-tree], [use single linked list instead of binary tree data structure for dnssec-zkt]),
	[usetree=$enableval])
if test "$usetree" = no
then
	usetree=0
	t=""
fi
AC_DEFINE_UNQUOTED(USE_TREE, $usetree, Use TREE data structure for dnssec-zkt)

AC_DEFINE_UNQUOTED(ZKT_VERSION, "v$t$PACKAGE_VERSION (c) Feb 2005 - Aug 2009 Holger Zuleger hznet.de", ZKT version string)

### Checks for libraries.


### Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netdb.h stdlib.h getopt.h string.h strings.h sys/socket.h sys/time.h sys/types.h syslog.h unistd.h utime.h])


### Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UID_T


### Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
# 2008-07-04 getopt_long added
# 2009-07-30 timegm added
AC_CHECK_FUNCS([getopt_long gettimeofday memset putenv socket strcasecmp strchr strdup strerror strncasecmp strrchr tzset utime getuid timegm])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
