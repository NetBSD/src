#
# configure.in
#
#       The Initial Developer of the Original Code is International
#       Business Machines Corporation. Portions created by IBM
#       Corporation are Copyright (C) 2005, 2006 International Business
#       Machines Corporation. All Rights Reserved.
#
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the Common Public License as published by
#       IBM Corporation; either version 1 of the License, or (at your option)
#       any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       Common Public License for more details.
#
#       You should have received a copy of the Common Public License
#       along with this program; if not, a copy can be viewed at
#       http://www.opensource.org/licenses/cpl1.0.php.
#

AC_INIT(tpm-tools, 1.3.9, trousers-tech@lists.sf.net)
AC_PREREQ(2.12)dnl
AC_CONFIG_SRCDIR(Makefile.am)
AM_CONFIG_HEADER(config.h)

AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign 1.6])


DEBUG=""
AC_MSG_CHECKING([for debug-enabled build])
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [create a debug build that outputs debug messages]),
 [if test "$enableval" = "yes"; then
    DEBUG="yes"
    AC_MSG_RESULT([yes])
  else
    DEBUG="no"
    AC_MSG_RESULT([no])
  fi],
 [DEBUG="no"
  AC_MSG_RESULT([no])])

# If the user has not set CFLAGS, do something appropriate
test_CFLAGS=${CFLAGS+set}
if test "$test_CFLAGS" != set; then
	if test "$DEBUG" == "yes"; then
		CFLAGS="-O0 -g -DDEBUG"
	else
		CFLAGS="-g -O2"
	fi
elif test "$DEBUG" == "yes"; then
	CFLAGS="$CFLAGS -O0 -g -DDEBUG"
fi


AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_CHECK_PROGS(COPY, cp)
AC_CHECK_PROGS(ECHO, echo)
AC_CHECK_PROGS(MKDIR, mkdir)
AC_CHECK_PROGS(RM, rm)

AM_GNU_GETTEXT_VERSION([0.15])
AM_GNU_GETTEXT([external])

# Arch specific stuff
case $target in
	*ppc64* | *powerpc64* | *x86_64* | *s390x*)
		CFLAGS="$CFLAGS -m64"
		;;
	*)
		;;
esac



# Non-standard OpenSSL location
AC_MSG_CHECKING([Non-standard OpenSSL location])
AC_ARG_WITH(openssl,
	    AC_HELP_STRING([--with-openssl=PATH], [Location of openssl libs/includes]),
	    [OPENSSL_INCLUDE_DIR="$withval/include"
	     OPENSSL_LIB_DIR="$withval/lib"
	    if test ! -d $OPENSSL_INCLUDE_DIR -o ! -d $OPENSSL_LIB_DIR; then
		    AC_MSG_ERROR([$OPENSSL_INCLUDE_DIR or $OPENSSL_LIB_DIR doen't exist!])
	    else
		    AC_MSG_RESULT([yes])
		    CFLAGS="$CFLAGS -L$OPENSSL_LIB_DIR -I$OPENSSL_INCLUDE_DIR"
		    AC_SUBST([OPENSSL_LIB_DIR])
	    fi],
	    [AC_MSG_RESULT([no])
	    AC_SUBST([OPENSSL_LIB_DIR], []) ] )

AC_CHECK_LIB(tspi, Tspi_Context_Create, [TSS_LIB="1"], [AC_MSG_ERROR([tss lib not found: libtspi.so])])
AC_CHECK_LIB(tspi, Tspi_TPM_SetOperatorAuth, [TSS_12="1"], [TSS_12="0"])
if test "$TSS_12" = "1"; then
	AM_CONDITIONAL(TSS_LIB_IS_12, true)
	AC_MSG_NOTICE([Configuring tpm-tools for TSS 1.2])
else
	AM_CONDITIONAL(TSS_LIB_IS_12, false)
	AC_MSG_NOTICE([Configuring tpm-tools for TSS 1.1])
fi
AC_CHECK_HEADER(trousers/tss.h, [TSS_INC="1"], [AC_MSG_ERROR([tss header not found: trousers/tss.h])])

AC_CHECK_LIB(crypto, PEM_read_X509, [OPENSSL_LIB="1"], [AC_MSG_ERROR([openssl lib not found: libcrypto.so])])
AC_CHECK_HEADER(openssl/evp.h, [OPENSSL_INC="1"], [AC_MSG_ERROR([openssl header not found: openssl/evp.h])])

OPENCRYPTOKI="0"
AC_ARG_ENABLE(pkcs11_support, 
  AC_HELP_STRING([--disable-pkcs11-support], [don't build data_mgmt commands [[default=no]]]),
  [disable_pkcs11_support="yes"
   AC_MSG_RESULT([*** Not building data_mgmt at user request ***])],)

if test "x$disable_pkcs11_support" != "xyes"; then
  AC_CHECK_HEADER(opencryptoki/pkcs11.h, [OPENCRYPTOKI_INC="1"], [OPENCRYPTOKI_INC="0"])
  if test "$OPENCRYPTOKI_INC" = "1"; then
    OPENCRYPTOKI="1"
  fi
fi
AM_CONDITIONAL([P11_SUPPORT], [test "$OPENCRYPTOKI" = "1"])

AC_HEADER_STDC

AC_C_CONST
AC_C_INLINE

AC_SYS_LONG_FILE_NAMES

AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_TYPE_UID_T

CFLAGS="$CFLAGS -Wall -Wreturn-type -Wsign-compare"

AC_CONFIG_FILES(Makefile		\
		po/Makefile.in		\
		m4/Makefile		\
		include/Makefile	\
		lib/Makefile		\
		src/Makefile		\
		src/tpm_mgmt/Makefile	\
		src/cmds/Makefile	\
		src/data_mgmt/Makefile  \
		dist/tpm-tools.spec	\
		man/Makefile            \
		man/man1/Makefile       \
		man/man3/Makefile       \
		man/man8/Makefile)
AC_OUTPUT

echo "CFLAGS=$CFLAGS"
echo "LDFLAGS=$LDFLAGS"
