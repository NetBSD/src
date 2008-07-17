# configure.ac - process this file with autoconf to produce configure
#
# Copyright (C) 2006 Luke Howard
# Copyright (C) 2006 West Consulting
# Copyright (C) 2006, 2007, 2008 Arthur de Jong
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

AC_PREREQ(2.59)
AC_COPYRIGHT(
[Copyright (C) 2006 Luke Howard
Copyright (C) 2006 West Consulting
Copyright (C) 2006, 2007, 2008 Arthur de Jong

This configure script is derived from configure.ac which is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. See the
configure.ac file for more details.])

# initialize and set version and bugreport address
AC_INIT([nss-ldapd],[0.6.2],[arthur@ch.tudelft.nl])
RELEASE_MONTH="May 2008"
AC_SUBST(RELEASE_MONTH)
AC_CONFIG_SRCDIR([nslcd.h])

AC_CANONICAL_TARGET
AC_PREFIX_DEFAULT()

# display notice and initialize automake
AC_MSG_NOTICE([configuring AC_PACKAGE_TARNAME AC_PACKAGE_VERSION])
AM_INIT_AUTOMAKE(AC_PACKAGE_TARNAME,AC_PACKAGE_VERSION)

# create a config.h file (Automake will add -DHAVE_CONFIG_H)
AC_CONFIG_HEADERS([config.h])

# check for programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_CC_C_O
AC_USE_SYSTEM_EXTENSIONS

# checks for tool to convert docbook to man
AC_PATH_PROGS(DOCBOOK2X_MAN, docbook2x-man)
if test "x${DOCBOOK2X_MAN}" = x
then
  AC_MSG_WARN([docbook2x-man not found: not generating man pages])
fi
AM_CONDITIONAL([GENMAN], [test "x${DOCBOOK2X_MAN}" != x])

# start with default options
nss_ldap_so_LIBS=
nslcd_LIBS=

# check for debugging options
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug],
                             [enable extensive debugging and logging]),
              [if test "x$enableval" != "xno" ; then CFLAGS="-g -DDEBUG $CFLAGS" ; fi])

DESIRED_CFLAGS=""

# check for extra compiler warnings
AC_ARG_ENABLE(warnings,
              AS_HELP_STRING([--enable-warnings],
                             [enable extra compiler warnings (gcc)]),
              [if test "x$enableval" != "no"
               then
                 CFLAGS="$CFLAGS -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Waggregate-return -Wmissing-declarations -Wunused -Wformat=2 -Wswitch-default -Wswitch-enum -Wfloat-equal -Wbad-function-cast -Wunreachable-code -Wredundant-decls"
                 DESIRED_CFLAGS="$DESIRED_CFLAGS -Wextra -Wdeclaration-after-statement -Werror-implicit-function-declaration"
               fi])

test_gcc_flag() {
  AC_LANG_CONFTEST([int main() {}])
  $CC -c conftest.c $CFLAGS $@ > /dev/null 2> /dev/null
  ret=$?
  rm -f conftest.o
  return $ret
}

for flag in $DESIRED_CFLAGS
do
  AC_MSG_CHECKING([whether $CC accepts $flag])
  if test_gcc_flag $flag
  then
    CFLAGS="$CFLAGS $flag"
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
done

AC_ARG_WITH(ldap-lib,
            AS_HELP_STRING([--with-ldap-lib=TYPE],
                           [select ldap library (auto|netscape5|netscape4|netscape3|umich|openldap) @<:@auto@:>@]))
if test -z "$with_ldap_lib"
then
  with_ldap_lib=auto
fi

AC_ARG_WITH(ldap-conf-file,
            AS_HELP_STRING([--with-ldap-conf-file=PATH],
                           [path to LDAP configuration file @<:@/etc/nss-ldapd.conf@:>@]),
            [ NSS_LDAP_PATH_CONF="$with_ldap_conf_file" ],
            [ NSS_LDAP_PATH_CONF="/etc/nss-ldapd.conf" ])
AC_DEFINE_UNQUOTED(NSS_LDAP_PATH_CONF,"$NSS_LDAP_PATH_CONF",[Path to LDAP configuration file.])
AC_SUBST(NSS_LDAP_PATH_CONF)

AC_ARG_WITH(nslcd-pidfile,
            AS_HELP_STRING([--with-nslcd-pidfile=PATH],
                           [path to pidfile @<:@/var/run/nslcd/nslcd.pid@:>@]),
            [ NSLCD_PIDFILE="$with_nslcd_pidfile" ],
            [ NSLCD_PIDFILE="/var/run/nslcd/nslcd.pid" ])
AC_DEFINE_UNQUOTED(NSLCD_PIDFILE,"$NSLCD_PIDFILE",[The location of the pidfile used for checking availability of the nslcd.])
AC_SUBST(NSLCD_PIDFILE)

AC_ARG_WITH(nslcd-socket,
            AS_HELP_STRING([--with-nslcd-socket=PATH],
                           [path to socket @<:@/var/run/nslcd/socket@:>@]),
            [ NSLCD_SOCKET="$with_nslcd_socket" ],
            [ NSLCD_SOCKET="/var/run/nslcd/socket" ])
AC_DEFINE_UNQUOTED(NSLCD_SOCKET,"$NSLCD_SOCKET",[The location of the socket used for communicating.])
AC_SUBST(NSLCD_SOCKET)

# checks for availability of header files
AC_CHECK_HEADERS(lber.h)
AC_CHECK_HEADERS(ldap.h,,AC_MSG_ERROR([could not locate <ldap.h>]),[
    #if HAVE_LBER_H
    #include <lber.h>
    #endif
    ])
AC_CHECK_HEADERS(ldap_ssl.h)
AC_CHECK_HEADERS(nss.h)
AC_CHECK_HEADERS(pthread.h)
AC_CHECK_HEADERS(shadow.h)
AC_CHECK_HEADERS(aliases.h)
AC_CHECK_HEADERS(netinet/ether.h)
AC_CHECK_HEADERS(ctype.h)
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADERS(sys/un.h)
AC_CHECK_HEADERS(sasl.h sasl/sasl.h)
AC_CHECK_HEADERS(strings.h)
AC_CHECK_HEADERS(gssldap.h)
AC_CHECK_HEADERS(gsssasl.h)
AC_CHECK_HEADERS(gssapi/gssapi_krb5.h gssapi.h)
AC_CHECK_HEADERS(grp.h)
AC_CHECK_HEADERS(sys/socket.h)
AC_CHECK_HEADERS(sys/ucred.h)
AC_CHECK_HEADERS(ucred.h)

# set up directory with compatibility replacement files
AC_CONFIG_LIBOBJ_DIR([compat])

# checks for availability of system libraries for nslcd
save_LIBS="$LIBS"
LIBS="$nslcd_LIBS"
AC_SEARCH_LIBS(gethostbyname,nsl socket)
AC_SEARCH_LIBS(socket,socket)
#AC_CHECK_LIB(resolv,main)
nslcd_LIBS="$LIBS"
LIBS="$save_LIBS"

# check for availability of system libraries for nss part
save_LIBS="$LIBS"
LIBS="$nss_ldap_so_LIBS"
AC_SEARCH_LIBS(socket,socket)
nss_ldap_so_LIBS="$LIBS"
LIBS="$save_LIBS"

# TODO: simplify the above since we need a correct LIBS for the following
#       tests

# checks for availability of functions
AC_CHECK_FUNCS(sigaction)
AC_CHECK_FUNCS(snprintf)
AC_CHECK_FUNCS(gethostbyname)
AC_CHECK_FUNCS(setgroups)
AC_CHECK_FUNCS(getpeereid)
AC_CHECK_FUNCS(getpeerucred)
AC_CHECK_FUNCS(__nss_configure_lookup)


# replace getopt_long() function if it is not on the system
AC_REPLACE_FUNCS(getopt_long)
# replace daemon() function if it is not on the system
AC_SEARCH_LIBS(daemon,bsd)
AC_REPLACE_FUNCS(daemon)
# replace ether_ntoa_r() and ether_aton_r() if they are not found
AC_CHECK_FUNCS(ether_aton_r ether_ntoa_r,,[AC_CHECK_FUNCS(ether_aton ether_ntoa)
                                           AC_LIBOBJ(ether)])

# checks for types
AC_C_CONST
AC_TYPE_SIGNAL
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T

# check for support for the __thread keyword
AC_CACHE_CHECK([whether $CC supports '__thread'], [mn_cv_c___thread_supported],
               [AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[static __thread i;]], []),
                       [mn_cv_c___thread_supported=yes],
                       [mn_cv_c___thread_supported=no])])
if test $mn_cv_c___thread_supported != yes
then
  AC_MSG_WARN([$CC does not support '__thread' keyword])
  AC_DEFINE(__thread,,[Define to empty if compiler does not support `__thread' keyword.])
fi

# check for support for the struct ether_addr structure
AC_CHECK_TYPE(struct ether_addr,
    AC_DEFINE(HAVE_STRUCT_ETHER_ADDR,1,[Define to 1 if you have a `struct ether_addr' definition.]),,[
    #include <sys/socket.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #include <netinet/if_ether.h>])

# check to see if socklen_t is defined
AC_CHECK_TYPE(socklen_t,,
    AC_DEFINE(socklen_t,size_t,[Define to `size_t' if not defined elswhere.]),[
    #include <sys/types.h>
    #include <sys/socket.h>])

# check to see if struct sockaddr_storage is defined
AC_CHECK_TYPE(struct sockaddr_storage,,
    AC_DEFINE(sockaddr_storage,sockaddr_in,[Define to `sockaddr_in' if not defined elsewhere.]),[
    #include <sys/types.h>
    #include <sys/socket.h>])

# check for support for the struct ucred structure
AC_CHECK_TYPE(struct ucred,
    AC_DEFINE(HAVE_STRUCT_UCRED,1,[Define to 1 if you have a `struct ucred' definition.]),,[
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <sys/types.h>])

AC_CHECK_TYPE(sasl_interact_t,
    AC_DEFINE(HAVE_SASL_INTERACT_T,1,[Define to 1 if you have a `sasl_interact_t' definition.]))

# checks for LDAP library
save_LIBS="$LIBS"
LIBS="$nslcd_LIBS"
save_CFLAGS="$CFLAGS"

# check threading stuff
ACX_PTHREAD(,AC_MSG_ERROR([no support for pthreads]))
AC_CHECK_FUNCS(pthread_atfork)

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

AC_CHECK_LIB(gssapi, gss_krb5_ccache_name,[LIBS="-lgssapi $LIBS" found_gssapi_lib=yes],,$LIBS)
if test -z "$found_gssapi_lib"
then
  AC_CHECK_LIB(gssapi_krb5, gss_krb5_ccache_name,[LIBS="-lgssapi_krb5 $LIBS"],,$LIBS)
fi

if test -z "$found_ldap_lib" -a \( $with_ldap_lib = auto -o $with_ldap_lib = umich -o $with_ldap_lib = openldap \)
then
  AC_SEARCH_LIBS(ldap_search_ext,[ldap_r ldap],found_ldap_lib=yes,,)
fi
if test -z "$found_ldap_lib" -a \( $with_ldap_lib = auto -o $with_ldap_lib = netscape5 \)
then
  AC_CHECK_LIB(ldap50, main, LIBS="-lldap50 -lssldap50 -lssl3 -lnss3 -lnspr4 -lprldap50 -lplc4 -lplds4 $LIBS" found_ldap_lib=yes need_pthread=yes,, -lpthread)
fi
if test -z "$found_ldap_lib" -a \( $with_ldap_lib = auto -o $with_ldap_lib = netscape4 \)
then
  AC_CHECK_LIB(ldapssl41, main, LIBS="-lldapssl41 -lplc3 -lplds3 -lnspr3 $LIBS" found_ldap_lib=yes need_pthread=yes,, -lpthread)
  if test -z "$found_ldap_lib"
  then
    AC_CHECK_LIB(ldapssl40, main, LIBS="-lldapssl40 $LIBS" found_ldap_lib=yes need_pthread=yes,, -lpthread)
  fi
  if test -z "$found_ldap_lib"
  then
    AC_CHECK_LIB(ldap41, main, LIBS="-lldap41 $LIBS" found_ldap_lib=yes need_pthread=no,,)
  fi
  if test -z "$found_ldap_lib"
  then
    AC_CHECK_LIB(ldap40, main, LIBS="-lldap40 $LIBS" found_ldap_lib=yes need_pthread=no,,)
  fi
fi
if test -z "$found_ldap_lib" -a \( $with_ldap_lib = auto -o $with_ldap_lib = netscape3 \)
then
  AC_CHECK_LIB(ldapssl30, main, LIBS="-lldapssl30 $LIBS" found_ldap_lib=yes need_pthread=yes,, -lpthread)
fi
if test -z "$found_ldap_lib"
then
  AC_MSG_ERROR(could not locate a valid LDAP library)
fi
if test "$need_pthread" = "yes"
then
  AC_CHECK_LIB(pthread, main)
fi
AC_CHECK_LIB(gssldap, ldap_gss_bind,[LIBS="-lgssldap $LIBS"],,$LIBS)
AC_CHECK_LIB(sasl2, sasl_client_init)

AC_CHECK_FUNCS(sasl_auxprop_request)
AC_CHECK_FUNCS(gss_krb5_ccache_name)
AC_CHECK_FUNCS(ldap_parse_result ldap_memfree ldap_controls_free ldap_control_free)
AC_CHECK_FUNCS(ldap_explode_dn ldap_explode_rdn ldap_set_option ldap_get_option)
AC_CHECK_FUNCS(ldap_abandon ldap_simple_bind_s ldap_unbind ldap_set_rebind_proc)
AC_CHECK_FUNCS(ldap_sasl_interactive_bind_s ldap_initialize ldap_search_ext)
AC_CHECK_FUNCS(ldap_create_control)
AC_CHECK_FUNCS(ldap_domain2hostlist ldap_domain2dn)
AC_CHECK_FUNCS(ldap_get_values ldap_value_free ldap_get_dn)
AC_CHECK_FUNCS(ldap_err2string ldap_msgfree ldap_result)

# replace ldap_create_page_control() and ldap_parse_page_control()
AC_CHECK_FUNCS(ldap_create_page_control ldap_parse_page_control,,[AC_LIBOBJ(pagectrl)])

# check the number of arguments that ldap_set_rebind_proc() uses
AC_CACHE_CHECK(
    [number of arguments to ldap_set_rebind_proc],
    nss_ldapd_cv_ldap_set_rebind_proc,
    [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[
            #include <lber.h>
            #include <ldap.h>]],
            [[ldap_set_rebind_proc(0, 0, 0);]])],
        [nss_ldapd_cv_ldap_set_rebind_proc=3],
        [nss_ldapd_cv_ldap_set_rebind_proc=2]) ])
AC_DEFINE_UNQUOTED(LDAP_SET_REBIND_PROC_ARGS,$nss_ldapd_cv_ldap_set_rebind_proc,
                   [Define to the number of arguments to ldap_set_rebindproc.])

CFLAGS="$save_CFLAGS"
nslcd_LIBS="$LIBS"
LIBS="$save_LIBS"

AC_SUBST(nss_ldap_so_LIBS)
AC_SUBST(nslcd_LIBS)

# generate files
AC_CONFIG_FILES([Makefile compat/Makefile common/Makefile
                 nss/Makefile nslcd/Makefile man/Makefile tests/Makefile])
AC_OUTPUT
