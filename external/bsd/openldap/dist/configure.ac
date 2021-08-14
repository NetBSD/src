dnl $OpenLDAP$
dnl This work is part of OpenLDAP Software <http://www.openldap.org/>.
dnl
dnl Copyright 1998-2021 The OpenLDAP Foundation.
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted only as authorized by the OpenLDAP
dnl Public License.
dnl
dnl A copy of this license is available in the file LICENSE in the
dnl top-level directory of the distribution or, alternatively, at
dnl <http://www.OpenLDAP.org/license.html>.
dnl
dnl ----------------------------------------------------------------
dnl Disable config.cache!
define([AC_CACHE_LOAD], )dnl
define([AC_CACHE_SAVE], )dnl
dnl ----------------------------------------------------------------
dnl Disable libtool 1.5 support for languages we don't use
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_GCJ_CONFIG], [:])dnl
dnl ================================================================
dnl Configure.in for OpenLDAP
AC_COPYRIGHT([[Copyright 1998-2021 The OpenLDAP Foundation. All rights reserved.
Restrictions apply, see COPYRIGHT and LICENSE files.]])
AC_REVISION([Id: c8892a2009b69f5948d96e4a90b672eeb1ded5e1 ])
AC_INIT([OpenLDAP],,[https://bugs.openldap.org],,[https://www.openldap.org])
AC_CONFIG_SRCDIR(build/version.sh)dnl
dnl ----------------------------------------------------------------
dnl OpenLDAP Autoconf Macros
builtin(include, build/openldap.m4)dnl
dnl ================================================================

m4_ifndef([PKG_PREREQ],
	[m4_fatal([must install pkg-config 0.29 or later before running autoconf/autogen])])

AC_CONFIG_AUX_DIR(build)dnl
AC_CONFIG_MACRO_DIRS([build])

eval `$ac_aux_dir/version.sh`
if test -z "$OL_STRING"; then
	AC_MSG_ERROR([could not determine version])
fi

if test -f "$ac_aux_dir/shtool" && test ! -d $ac_aux_dir/shtool; then
	ac_cv_shtool="$ac_aux_dir/shtool"
else
	AC_MSG_ERROR([no shtool found in $ac_aux_dir])
fi

SHTOOL="$ac_cv_shtool"
dnl AC_SUBST(SHTOOL)dnl

TB="" TN=""
if test -t 1; then
	TB="`$SHTOOL echo -e '%B' 2>/dev/null`"
	TN="`$SHTOOL echo -e '%b' 2>/dev/null`"
fi

OPENLDAP_REPO=""
if test -d $ac_aux_dir/../.git; then
	OPENLDAP_REPO="(from Git clone) "
elif test -d $ac_aux_dir/CVS; then
	OPENLDAP_REPO="(from CVS checkout) "
fi

echo "Configuring ${TB}${OL_STRING}${TN} ${OPENLDAP_REPO}..."

dnl Determine host platform
dnl		we try not to use this for much
AC_CANONICAL_TARGET([])

AC_SUBST(PACKAGE,$OL_PACKAGE)dnl
AC_SUBST(VERSION,$OL_VERSION)dnl
AC_DEFINE_UNQUOTED(OPENLDAP_PACKAGE,"$PACKAGE",Package)
AC_DEFINE_UNQUOTED(OPENLDAP_VERSION,"$VERSION",Version)

AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION,$OL_API_INC,Version)
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION_MAJOR,$OL_MAJOR,Major)
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION_MINOR,$OL_MINOR,Minor)
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION_PATCH,$OL_PATCH,Patch)

OPENLDAP_LIBRELEASE=$OL_API_LIB_RELEASE
AC_SUBST(OPENLDAP_LIBRELEASE)dnl

OPENLDAP_LIBVERSION=$OL_API_LIB_VERSION
AC_SUBST(OPENLDAP_LIBVERSION)dnl

OPENLDAP_RELEASE_DATE="$OL_RELEASE_DATE"
AC_SUBST(OPENLDAP_RELEASE_DATE)dnl

AC_PREREQ(2.69)dnl Required Autoconf version

AH_TOP([
/* begin of portable.h.pre */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _LDAP_PORTABLE_H
#define _LDAP_PORTABLE_H

/* define this if needed to get reentrant functions */
#ifndef REENTRANT
#undef REENTRANT
#endif
#ifndef _REENTRANT
#undef _REENTRANT
#endif

/* define this if needed to get threadsafe functions */
#ifndef THREADSAFE
#undef THREADSAFE
#endif
#ifndef _THREADSAFE
#undef _THREADSAFE
#endif
#ifndef THREAD_SAFE
#undef THREAD_SAFE
#endif
#ifndef _THREAD_SAFE
#undef _THREAD_SAFE
#endif

#ifndef _SGI_MP_SOURCE
#undef _SGI_MP_SOURCE
#endif

/* end of portable.h.pre */
])
AH_BOTTOM([
/* begin of portable.h.post */

#ifdef _WIN32
	/* don't suck in all of the win32 api */
#	define WIN32_LEAN_AND_MEAN 1
#endif

#ifndef LDAP_NEEDS_PROTOTYPES
/* force LDAP_P to always include prototypes */
#define LDAP_NEEDS_PROTOTYPES 1
#endif

#ifndef LDAP_REL_ENG
#if (LDAP_VENDOR_VERSION == 000000) && !defined(LDAP_DEVEL)
#define LDAP_DEVEL
#endif
#if defined(LDAP_DEVEL) && !defined(LDAP_TEST)
#define LDAP_TEST
#endif
#endif

#ifdef HAVE_STDDEF_H
#	include <stddef.h>
#endif

#ifdef HAVE_EBCDIC 
/* ASCII/EBCDIC converting replacements for stdio funcs
 * vsnprintf and snprintf are used too, but they are already
 * checked by the configure script
 */
#define fputs ber_pvt_fputs
#define fgets ber_pvt_fgets
#define printf ber_pvt_printf
#define fprintf ber_pvt_fprintf
#define vfprintf ber_pvt_vfprintf
#define vsprintf ber_pvt_vsprintf
#endif

#include "ac/fdset.h"

#include "ldap_cdefs.h"
#include "ldap_features.h"

#include "ac/assert.h"
#include "ac/localize.h"

#endif /* _LDAP_PORTABLE_H */
/* end of portable.h.post */
])

AC_CONFIG_HEADERS([include/portable.h:include/portable.hin])
AC_CONFIG_HEADERS([include/ldap_features.h:include/ldap_features.hin])
AC_CONFIG_HEADERS([include/lber_types.h:include/lber_types.hin])

dnl ================================================================
dnl Start Args
AC_MSG_CHECKING(configure arguments)
AC_PREFIX_DEFAULT(/usr/local)

top_builddir=`pwd`
AC_SUBST(top_builddir)dnl

dnl ----------------------------------------------------------------
dnl --with-subdir
ldap_subdir="/openldap"

AC_ARG_WITH(subdir,
[  --with-subdir=DIR       change default subdirectory used for installs],
[case "$withval" in
	no) ldap_subdir=""
		;;
	yes)
		;;
	/*|\\*)
		ldap_subdir="$withval"
		;;
	*)
		ldap_subdir="/$withval"
		;;
esac
])dnl

AC_SUBST(ldap_subdir)dnl

dnl ----------------------------------------------------------------
dnl General "enable" options
dnl set default to traditional to enable the original debug style
OL_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [enable debugging])], yes, [no yes traditional])dnl
OL_ARG_ENABLE(dynamic, [AS_HELP_STRING([--enable-dynamic], [enable linking built binaries with dynamic libs])], auto)dnl
OL_ARG_ENABLE(syslog, [AS_HELP_STRING([--enable-syslog], [enable syslog support])], auto)dnl
dnl OL_ARG_ENABLE(referrals,[AS_HELP_STRING([--enable-referrals], [enable LDAPv2+ Referrals (experimental)])], no)dnl
ol_enable_referrals=${ol_enable_referrals-no}
OL_ARG_ENABLE(ipv6, [AS_HELP_STRING([--enable-ipv6], [enable IPv6 support])], auto)dnl
OL_ARG_ENABLE(local, [AS_HELP_STRING([--enable-local], [enable AF_LOCAL (AF_UNIX) socket support])], auto)dnl

dnl ----------------------------------------------------------------
dnl General "with" options
OL_ARG_WITH(cyrus_sasl, [AS_HELP_STRING([--with-cyrus-sasl], [with Cyrus SASL support])],
	auto, [auto yes no] )
OL_ARG_WITH(systemd, [AS_HELP_STRING([--with-systemd],	[with systemd service notification support])],
	auto, [auto yes no] )
OL_ARG_WITH(fetch, [AS_HELP_STRING([--with-fetch], [with fetch(3) URL support])],
	auto, [auto yes no] )
OL_ARG_WITH(threads,
	[AS_HELP_STRING([--with-threads], [with threads library auto|nt|posix|pth|lwp|manual])],
	auto, [auto nt posix pth lwp yes no manual] )
OL_ARG_WITH(tls,
	[AS_HELP_STRING([--with-tls], [with TLS/SSL support auto|openssl|gnutls])],
	auto, [auto openssl gnutls yes no] )
OL_ARG_WITH(yielding_select,
	[AS_HELP_STRING([--with-yielding-select], [with implicitly yielding select])],
	auto, [auto yes no manual] )
OL_ARG_WITH(mp,
	[AS_HELP_STRING([--with-mp], [with multiple precision statistics auto|longlong|long|bignum|gmp])],
	auto, [auto longlong long bignum gmp yes no])
OL_ARG_WITH(odbc,
	[AS_HELP_STRING([--with-odbc], [with specific ODBC support iodbc|unixodbc|odbc32|auto])],
	auto, [auto iodbc unixodbc odbc32] )

dnl ----------------------------------------------------------------
dnl Server options
dnl ----------------------------------------------------------------

dnl ----------------------------------------------------------------
dnl SLAPD OPTIONS
SlapdOptions="dynacl \
	aci \
	cleartext \
	crypt \
	spasswd \
	modules \
	rlookups \
	slapi \
	slp \
	wrappers"

AC_ARG_ENABLE(xxslapdoptions,[
SLAPD (Standalone LDAP Daemon) Options:])

OL_ARG_ENABLE(slapd, [AS_HELP_STRING([--enable-slapd], [enable building slapd])], yes)dnl
OL_ARG_ENABLE(dynacl, [AS_HELP_STRING([--enable-dynacl], [enable run-time loadable ACL support (experimental)])], no)dnl
OL_ARG_ENABLE(aci, [AS_HELP_STRING([--enable-aci], [enable per-object ACIs (experimental)])], no, [no yes mod])dnl
OL_ARG_ENABLE(cleartext, [AS_HELP_STRING([--enable-cleartext], [enable cleartext passwords])], yes)dnl
OL_ARG_ENABLE(crypt, [AS_HELP_STRING([--enable-crypt], [enable crypt(3) passwords])], no)dnl
OL_ARG_ENABLE(spasswd, [AS_HELP_STRING([--enable-spasswd], [enable (Cyrus) SASL password verification])], no)dnl
OL_ARG_ENABLE(modules, [AS_HELP_STRING([--enable-modules], [enable dynamic module support])], no)dnl
OL_ARG_ENABLE(rlookups, [AS_HELP_STRING([--enable-rlookups], [enable reverse lookups of client hostnames])], no)dnl
OL_ARG_ENABLE(slapi, [AS_HELP_STRING([--enable-slapi], [enable SLAPI support (experimental)])], no)dnl
OL_ARG_ENABLE(slp, [AS_HELP_STRING([--enable-slp], [enable SLPv2 support])], no)dnl
OL_ARG_ENABLE(wrappers, [AS_HELP_STRING([--enable-wrappers], [enable tcp wrapper support])], no)dnl

dnl ----------------------------------------------------------------
dnl SLAPD Backend Options
Backends="dnssrv \
	ldap \
	mdb \
	meta \
	asyncmeta \
	ndb \
	null \
	passwd \
	perl \
	relay \
	sock \
	sql \
	wt"

AC_ARG_ENABLE(xxslapbackends,[
SLAPD Backend Options:])

OL_ARG_ENABLE(backends, [AS_HELP_STRING([--enable-backends], [enable all available backends])],
	--, [no yes mod])dnl
OL_ARG_ENABLE(dnssrv, [AS_HELP_STRING([--enable-dnssrv], [enable dnssrv backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(ldap, [AS_HELP_STRING([--enable-ldap], [enable ldap backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(mdb, [AS_HELP_STRING([--enable-mdb], [enable mdb database backend])],
	yes, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(meta, [AS_HELP_STRING([--enable-meta], [enable metadirectory backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(asyncmeta, [AS_HELP_STRING([--enable-asyncmeta], [enable asynchronous metadirectory backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(ndb, [AS_HELP_STRING([--enable-ndb], [enable MySQL NDB Cluster backend])],
	no, [no yes mod])dnl
OL_ARG_ENABLE(null, [AS_HELP_STRING([--enable-null], [enable null backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(passwd, [AS_HELP_STRING([--enable-passwd], [enable passwd backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(perl, [AS_HELP_STRING([--enable-perl], [enable perl backend])],
	no, [no yes mod])dnl
OL_ARG_ENABLE(relay, [AS_HELP_STRING([--enable-relay], [enable relay backend])],
	yes, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(sock, [AS_HELP_STRING([--enable-sock], [enable sock backend])],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(sql, [AS_HELP_STRING([--enable-sql], [enable sql backend])],
	no, [no yes mod])dnl
OL_ARG_ENABLE(wt, [AS_HELP_STRING([--enable-wt], [enable WiredTiger backend])],
	no, [no yes mod], ol_enable_backends)dnl

dnl ----------------------------------------------------------------
dnl SLAPD Overlay Options
Overlays="accesslog \
	auditlog \
	autoca \
	collect \
	constraint \
	dds \
	deref \
	dyngroup \
	dynlist \
	homedir \
	memberof \
	otp \
	ppolicy \
	proxycache \
	refint \
	remoteauth \
	retcode \
	rwm \
	seqmod \
	sssvlv \
	syncprov \
	translucent \
	unique \
	valsort"

Pwmods="argon2"

AC_ARG_ENABLE(xxslapoverlays,[
SLAPD Overlay Options:])

OL_ARG_ENABLE(overlays, [AS_HELP_STRING([--enable-overlays], [enable all available overlays])],
	--, [no yes mod])dnl
OL_ARG_ENABLE(accesslog, [AS_HELP_STRING([--enable-accesslog], [In-Directory Access Logging overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(auditlog, [AS_HELP_STRING([--enable-auditlog], [Audit Logging overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(autoca, [AS_HELP_STRING([--enable-autoca], [Automatic Certificate Authority overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(collect, [AS_HELP_STRING([--enable-collect], [Collect overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(constraint, [AS_HELP_STRING([--enable-constraint], [Attribute Constraint overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(dds, [AS_HELP_STRING([--enable-dds], [Dynamic Directory Services overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(deref, [AS_HELP_STRING([--enable-deref], [Dereference overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(dyngroup, [AS_HELP_STRING([--enable-dyngroup], [Dynamic Group overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(dynlist, [AS_HELP_STRING([--enable-dynlist], [Dynamic List overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(homedir, [AS_HELP_STRING([--enable-homedir], [Home Directory Management overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(memberof, [AS_HELP_STRING([--enable-memberof], [Reverse Group Membership overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(otp, [AS_HELP_STRING([--enable-otp], [OTP 2-factor authentication overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(ppolicy, [AS_HELP_STRING([--enable-ppolicy], [Password Policy overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(proxycache, [AS_HELP_STRING([--enable-proxycache], [Proxy Cache overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(refint, [AS_HELP_STRING([--enable-refint], [Referential Integrity overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(remoteauth, [AS_HELP_STRING([--enable-remoteauth], [Deferred Authentication overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(retcode, [AS_HELP_STRING([--enable-retcode], [Return Code testing overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(rwm, [AS_HELP_STRING([--enable-rwm], [Rewrite/Remap overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(seqmod, [AS_HELP_STRING([--enable-seqmod], [Sequential Modify overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(sssvlv, [AS_HELP_STRING([--enable-sssvlv], [ServerSideSort/VLV overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(syncprov, [AS_HELP_STRING([--enable-syncprov], [Syncrepl Provider overlay])],
	yes, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(translucent, [AS_HELP_STRING([--enable-translucent], [Translucent Proxy overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(unique, [AS_HELP_STRING([--enable-unique], [Attribute Uniqueness overlay])],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(valsort, [AS_HELP_STRING([--enable-valsort], [Value Sorting overlay])],
	no, [no yes mod], ol_enable_overlays)

dnl ----------------------------------------------------------------
dnl PASSWORD MODULE OPTIONS
AC_ARG_ENABLE(pwmodoptions,[
SLAPD Password Module Options:])
OL_ARG_ENABLE(argon2, [AS_HELP_STRING([--enable-argon2], [Argon2 password hashing module])],
	no, [no yes], ol_enable_pwmodules)
OL_ARG_WITH(argon2,
	[AS_HELP_STRING([--with-argon2], [with argon2 support library auto|libsodium|libargon2])],
	auto, [auto libsodium libargon2 yes no] )

dnl ----------------------------------------------------------------
dnl BALANCER OPTIONS
AC_ARG_ENABLE(balanceroptions,[
LLOADD (Load Balancer Daemon) Options:])
OL_ARG_ENABLE(balancer, [AS_HELP_STRING([--enable-balancer], [enable load balancer])],
	no, [no yes mod])

dnl ----------------------------------------------------------------
AC_ARG_ENABLE(xxliboptions,[
Library Generation & Linking Options])
AC_ENABLE_STATIC
AC_ENABLE_SHARED
OL_ARG_ENABLE(versioning, [AS_HELP_STRING([--enable-versioning], [Enable versioned symbols in shared library])],
	auto, [no yes auto])
dnl ----------------------------------------------------------------
dnl Validate options
dnl ----------------------------------------------------------------

if test $ol_enable_slapd = no ; then
	dnl SLAPD was specifically disabled
	dnl Disable all of its options

	for i in $SlapdOptions; do
		eval "ol_tmp=\$ol_enable_$i"
		if test $ol_tmp = yes ; then
			AC_MSG_WARN([slapd disabled, ignoring --enable-$i argument])
			eval "ol_enable_$i=no"
		fi
	done

	for i in $Backends $Overlays $Pwmods; do
		eval "ol_tmp=\$ol_enable_$i"
		if test $ol_tmp != no ; then
			AC_MSG_WARN([slapd disabled, ignoring --enable-$i argument])
			eval "ol_enable_$i=no"
		fi
	done

	if test $ol_enable_balancer = mod ; then
		AC_MSG_WARN([slapd disabled, ignoring --enable-balancer=mod argument])
		ol_enable_balancer=no
	fi
else
	dnl If slapd enabled and loadable module support disabled
	dnl then require at least one built-in backend

	if test $ol_enable_modules = no; then

		for i in backends overlays balancer $Backends $Overlays; do
			eval "ol_tmp=\$ol_enable_$i"
			if test -n "$ol_tmp" && test "$ol_tmp" = mod ; then
				AC_MSG_ERROR([--enable-$i=mod requires --enable-modules])
			fi
		done

		for i in $Pwmods; do
			eval "ol_tmp=\$ol_enable_$i"
			if test -n "$ol_tmp" && test "$ol_tmp" = yes ; then
				AC_MSG_ERROR([--enable-$i=yes requires --enable-modules])
			fi
		done

		ol_any_backend=no
		for i in $Backends; do
			eval "ol_tmp=\$ol_enable_$i"
			if test $ol_tmp = yes; then
				ol_any_backend=yes
			fi
		done

		if test $ol_any_backend = no; then
			AC_MSG_ERROR([slapd requires a backend])
		fi
	fi
fi

if test $ol_enable_aci = yes ; then
	if test $ol_enable_dynacl = no ; then
		AC_MSG_ERROR([--enable-aci requires --enable-dynacl])
	fi
elif test $ol_enable_aci = mod ; then
	AC_MSG_ERROR([ACI build as dynamic module not supported (yet)])
fi

if test $ol_enable_modules = yes ; then
	if test $ol_enable_dynamic = no ; then
		AC_MSG_ERROR([--enable-modules requires --enable-dynamic])
	fi
	ol_enable_dynamic=yes
fi

if test $ol_enable_balancer != no ; then
	dnl Load Balancer was specifically enabled
	if test $ol_with_threads = no ; then
		AC_MSG_ERROR([Load balancer requires threads])
	fi
fi

if test $ol_enable_spasswd = yes ; then
	if test $ol_with_cyrus_sasl = no ; then
		AC_MSG_ERROR([--enable-spasswd requires --with-cyrus-sasl])
	fi
	ol_with_cyrus_sasl=yes
fi

if test $ol_enable_meta/$ol_enable_ldap = yes/no ; then
	AC_MSG_ERROR([--enable-meta requires --enable-ldap])
fi

if test $ol_enable_asyncmeta/$ol_enable_ldap = yes/no ; then
	AC_MSG_ERROR([--enable-asyncmeta requires --enable-ldap])
fi

AC_MSG_RESULT(done)

dnl ----------------------------------------------------------------
dnl Initialize vars
LDAP_LIBS=
SLAPD_NDB_LIBS=
SLAPD_NDB_INCS=
LTHREAD_LIBS=
LEVENT_LIBS=
LUTIL_LIBS=

CLIENT_LIBS=

SLAPD_LIBS=
BALANCER_LIBS=
BALANCER_INCLUDE=

BUILD_SLAPD=no
BUILD_BALANCER=no

BUILD_THREAD=no

BUILD_SLAPI=no
SLAPD_SLAPI_DEPEND=

BUILD_DNSSRV=no
BUILD_LDAP=no
BUILD_MDB=no
BUILD_META=no
BUILD_ASYNCMETA=no
BUILD_NDB=no
BUILD_NULL=no
BUILD_PASSWD=no
BUILD_PERL=no
BUILD_RELAY=no
BUILD_SHELL=no
BUILD_SOCK=no
BUILD_SQL=no
BUILD_WT=no

BUILD_ACCESSLOG=no
BUILD_AUDITLOG=no
BUILD_AUTOCA=no
BUILD_CONSTRAINT=no
BUILD_DDS=no
BUILD_DENYOP=no
BUILD_DEREF=no
BUILD_DYNGROUP=no
BUILD_DYNLIST=no
BUILD_LASTMOD=no
BUILD_HOMEDIR=no
BUILD_MEMBEROF=no
BUILD_OTP=no
BUILD_PPOLICY=no
BUILD_PROXYCACHE=no
BUILD_REFINT=no
BUILD_REMOTEAUTH=no
BUILD_RETCODE=no
BUILD_RWM=no
BUILD_SEQMOD=no
BUILD_SSSVLV=no
BUILD_SYNCPROV=no
BUILD_TRANSLUCENT=no
BUILD_UNIQUE=no
BUILD_VALSORT=no

BUILD_PW_ARGON2=no

SLAPD_STATIC_OVERLAYS=
SLAPD_DYNAMIC_OVERLAYS=

SLAPD_DYNAMIC_PWMODS=

SLAPD_MODULES_LDFLAGS=
SLAPD_MODULES_CPPFLAGS=

SLAPD_STATIC_BACKENDS="back-ldif back-monitor"
SLAPD_DYNAMIC_BACKENDS=

SLAPD_PERL_LDFLAGS=
MOD_PERL_LDFLAGS=
PERL_CPPFLAGS=

SLAPD_SQL_LDFLAGS=
SLAPD_SQL_LIBS=
SLAPD_SQL_INCLUDES=

SASL_LIBS=
TLS_LIBS=
WITH_TLS_TYPE=no
MODULES_LIBS=
SLAPI_LIBS=
LIBSLAPI=
AUTH_LIBS=
SYSTEMD_LIBS=

SLAPD_SLP_LIBS=
SLAPD_GMP_LIBS=

dnl ================================================================
dnl Checks for programs

AC_DEFINE(HAVE_MKVERSION, 1, [define this if you have mkversion])

dnl ----------------------------------------------------------------
dnl
dnl Determine which C translator to use
dnl

dnl AIX Thread requires we use cc_r or xlc_r.
dnl But only do this IF AIX and CC is not set
dnl and threads are auto|yes|posix.
dnl
dnl If we find cc_r|xlc_r, force pthreads and assume
dnl		pthread_create is in $LIBS (ie: don't bring in
dnl		any additional thread libraries)
dnl If we do not find cc_r|xlc_r, disable threads

ol_aix_threads=no
case "$target" in
*-*-aix*) dnl all AIX is not a good idea.
	if test -z "$CC" ; then
		case "$ol_with_threads" in
		auto | yes |  posix) ol_aix_threads=yes ;;
		esac
	fi
;;
esac

if test $ol_aix_threads = yes ; then
	if test -z "${CC}" ; then
		AC_CHECK_PROGS(CC,cc_r xlc_r cc)

		if test "$CC" = cc ; then
			dnl no CC! don't allow --with-threads
			if test $ol_with_threads != auto ; then
				AC_MSG_ERROR([--with-threads requires cc_r (or other suitable compiler) on AIX])
			else
				AC_MSG_WARN([disabling threads, no cc_r on AIX])
			fi
			ol_with_threads=no
  		fi
	fi

	case ${CC} in cc_r | xlc_r)
		ol_with_threads=posix
		ol_cv_pthread_create=yes
		;;
	esac
fi

if test -z "${CC}"; then
	AC_CHECK_PROGS(CC,cc gcc,missing)

	if test "${CC}" = "missing" ; then
		AC_MSG_ERROR([Unable to locate cc(1) or suitable replacement.  Check PATH or set CC.])
	fi
fi

if test -z "${AR}"; then
	AC_CHECK_PROGS(AR,ar gar,missing)

	if test "${AR}" = "missing" ; then
		AC_MSG_ERROR([Unable to locate ar(1) or suitable replacement.  Check PATH or set AR.])
	fi
fi

if test -z "${STRIP}"; then
	AC_CHECK_PROGS(STRIP,strip,missing)

	if test "${STRIP}" = "missing" ; then
		AC_MSG_ERROR([Unable to locate strip(1) or suitable replacement.  Check PATH or set STRIP.])
	fi
fi


AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_DLOPEN
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

dnl ----------------------------------------------------------------
dnl Perl
ol_link_perl=no
if test $ol_enable_perl != no ; then
	AC_PATH_PROG(PERLBIN, perl, /usr/bin/perl)

	if test "no$PERLBIN" = "no" ; then
		if test $ol_enable_perl = yes ; then
			AC_MSG_ERROR([could not locate perl])
		fi

	else
		PERL_CPPFLAGS="`$PERLBIN -MExtUtils::Embed -e ccopts`"
		PERL_LDFLAGS="`$PERLBIN -MExtUtils::Embed -e ldopts|sed -e 's/ -lc / /' -e 's/ -lc$//'`"

		if test x"$ol_enable_perl" = "xyes" ; then
			SLAPD_PERL_LDFLAGS="$PERL_LDFLAGS"
		else
			MOD_PERL_LDFLAGS="$PERL_LDFLAGS"
		fi
		dnl should check perl version
		ol_link_perl=yes
	fi
fi

AC_PROG_CPP
OL_MSVC

dnl ----------------------------------------------------------------
dnl Checks for Windows NT
case $host_os in
  *mingw32* ) ac_cv_mingw32=yes ;;
  *cygwin* ) ac_cv_cygwin=yes ;;
  *interix* ) ac_cv_interix=yes ;;
esac

AC_CHECK_TOOL(RC, windres, )

dnl ----------------------------------------------------------------
dnl Checks for file extensions
AC_EXEEXT
AC_OBJEXT
AC_DEFINE_UNQUOTED(EXEEXT, "${EXEEXT}", [defined to be the EXE extension])

dnl ----------------------------------------------------------------
dnl BeOS requires -lbe -lroot -lnet
AC_CHECK_LIB(be, be_app, [LIBS="$LIBS -lbe -lroot -lnet"], :, [-lroot -lnet])

dnl ----------------------------------------------------------------
dnl OpenLDAP requires STDC features
AC_PROG_CC
if test "X${ac_cv_prog_cc_stdc}" = "Xno" ; then
	AC_MSG_ERROR([OpenLDAP requires compiler to support STDC constructs.])
fi

dnl ----------------------------------------------------------------
dnl Check cc depend flags
OL_MKDEPEND
if test "${ol_cv_mkdep}" = no ; then
	# this will soon become an error
	AC_MSG_WARN([do not know how to generate dependencies])
fi

dnl ----------------------------------------------------------------
dnl Check for AIX security library
AC_CHECK_LIB(s, afopen, [
	AUTH_LIBS=-ls
	AC_DEFINE(HAVE_AIX_SECURITY,1,[define if you have AIX security lib])
])

dnl ----------------------------------------------------------------
dnl Check for IBM OS/390
case "$target" in
*-ibm-openedition)
	ac_cv_func_getopt=no
	AC_DEFINE(BOTH_STRINGS_H,1,[define to use both <string.h> and <strings.h>])
	;;
esac

dnl ----------------------------------------------------------------
dnl Check for module support
ol_link_modules=no
WITH_MODULES_ENABLED=no
if test $ol_enable_modules != no ; then
	AC_CHECK_HEADERS(ltdl.h)

	if test $ac_cv_header_ltdl_h = no ; then
		AC_MSG_ERROR([could not locate libtool ltdl.h])
	fi

	AC_CHECK_LIB(ltdl, lt_dlinit, [
		MODULES_LIBS=-lltdl
		AC_DEFINE(HAVE_LIBLTDL,1,[define if you have libtool -ltdl])
	])

	if test "$ac_cv_lib_ltdl_lt_dlinit" = no ; then
		AC_MSG_ERROR([could not locate libtool -lltdl])
	fi
	ol_link_modules=yes
	WITH_MODULES_ENABLED=yes
fi

dnl ----------------------------------------------------------------
dnl Checks for header files.
OL_HEADER_STDC

if test $ol_cv_header_stdc != yes; then
	AC_MSG_WARN([could not locate Standard C compliant headers])
fi

AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ

AC_CHECK_HEADERS(	\
	arpa/inet.h		\
	arpa/nameser.h	\
	assert.h		\
	bits/types.h	\
	conio.h			\
	crypt.h			\
	direct.h		\
	errno.h			\
	fcntl.h			\
	filio.h			\
	getopt.h		\
	grp.h			\
	io.h			\
	libutil.h		\
	limits.h		\
	locale.h		\
	malloc.h		\
	memory.h		\
	psap.h			\
	pwd.h			\
	process.h		\
	sgtty.h			\
	shadow.h		\
	stddef.h		\
	string.h		\
	strings.h		\
	sysexits.h		\
	sys/file.h		\
	sys/filio.h		\
	sys/fstyp.h		\
	sys/errno.h		\
	sys/ioctl.h		\
	sys/param.h		\
	sys/privgrp.h	\
	sys/resource.h	\
	sys/select.h	\
	sys/socket.h	\
	sys/stat.h		\
	sys/syslog.h	\
	sys/time.h		\
	sys/types.h		\
	sys/uio.h		\
	sys/vmount.h	\
	syslog.h		\
	termios.h		\
	unistd.h		\
	utime.h			\
)

dnl Only check Winsock on MinGW
if test "$ac_cv_mingw32" = yes \
	-o "$ac_cv_interix" = yes \
	-o "$ol_cv_msvc" = yes
then
	AC_CHECK_HEADERS( winsock.h winsock2.h )
fi

AC_CHECK_HEADERS( resolv.h, [], [],
[$ac_includes_default
#include <netinet/in.h>
])

AC_CHECK_HEADERS( netinet/tcp.h, [], [],
[$ac_includes_default
#include <netinet/in.h>
])

AC_CHECK_HEADERS( sys/ucred.h, [], [],
[$ac_includes_default
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
])

dnl ----------------------------------------------------------------
dnl Checks for libraries

AC_CHECK_FUNCS( sigaction sigset )
AC_CHECK_FUNCS( fmemopen )

dnl HP-UX requires -lV3
dnl this is not needed on newer versions of HP-UX
if test $ac_cv_func_sigaction = no && test $ac_cv_func_sigaction = no ; then
	AC_CHECK_LIB(V3, sigset)
fi

if test $ol_cv_msvc = yes ; then
   ol_cv_winsock=yes
fi

dnl The following is INTENTIONALLY scripted out because shell does not
dnl support variable names with the '@' character, which is what
dnl autoconf would try to generate if one merely used AC_SEARCH_LIBS
if test "$ac_cv_header_winsock_h" = yes; then
	AC_CACHE_CHECK([for winsock], [ol_cv_winsock],[
	save_LIBS="$LIBS"
	for curlib in none ws2_32 wsock32; do
		if test $curlib != none ; then
	    	LIBS="$save_LIBS -l$curlib"
		fi
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <winsock.h>
			]], [[
			socket(0,0,0);
			select(0,NULL,NULL,NULL,NULL);
			closesocket(0);
			gethostname(NULL,0);
			]])],[ol_cv_winsock=$curlib],[ol_cv_winsock=no])

		test "$ol_cv_winsock" != no && break
	done
	LIBS="$save_LIBS"
	])

	if test $ol_cv_winsock != no ; then
    	AC_DEFINE(HAVE_WINSOCK, 1, [define if you have winsock])
    	ac_cv_func_socket=yes
    	ac_cv_func_select=yes
    	ac_cv_func_closesocket=yes
    	ac_cv_func_gethostname=yes

		if test $ol_cv_winsock != none -a $ol_cv_winsock != yes ; then
        	LIBS="$LIBS -l$ol_cv_winsock"
		fi

    	if test $ol_cv_winsock = ws2_32 -o $ol_cv_winsock = yes ; then
			AC_DEFINE(HAVE_WINSOCK2, 1, [define if you have winsock2])
    	fi
	fi
fi

dnl Find socket()
dnl Likely combinations:
dnl		-lsocket [ -lnsl_s | -lnsl ]
dnl		-linet

AC_CHECK_FUNC(socket, :, [	
dnl hopefully we won't include too many libraries
	AC_CHECK_LIB(socket, main)
	AC_CHECK_LIB(net, socket)
	AC_CHECK_LIB(nsl_s, main)
	AC_CHECK_LIB(nsl, main)
	AC_CHECK_LIB(inet, socket)
	AC_CHECK_LIB(gen, main)
])

dnl require select
AC_CHECK_FUNC(select, :, AC_MSG_ERROR([select() required.]))

if test "${ac_cv_header_winsock_h}" != yes; then
	dnl Select arg types
	dnl (if this detection becomes permanent, it and the select() detection
	dnl should be done before the yielding select test) 
	AC_FUNC_SELECT_ARGTYPES
fi

dnl check to see if system call automatically restart
dnl AC_SYS_RESTARTABLE_SYSCALLS

dnl ----------------------------------------------------------------
AC_CHECK_FUNCS( poll )
if test $ac_cv_func_poll = yes; then
AC_CHECK_HEADERS( poll.h sys/poll.h )
fi

dnl ----------------------------------------------------------------
AC_CHECK_HEADERS( sys/epoll.h )
if test "${ac_cv_header_sys_epoll_h}" = yes; then
	AC_MSG_CHECKING(for epoll system call)
	AC_RUN_IFELSE([AC_LANG_SOURCE([[int main(int argc, char **argv)
{
	int epfd = epoll_create(256);
	exit (epfd == -1 ? 1 : 0);
}]])],[AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_EPOLL,1, [define if your system supports epoll])],[AC_MSG_RESULT(no)],[AC_MSG_RESULT(no)])
fi

dnl ----------------------------------------------------------------
AC_CHECK_HEADERS( sys/event.h )
if test "${ac_cv_header_sys_event_h}" = yes; then
AC_MSG_CHECKING(for kqueue system call)
AC_RUN_IFELSE([AC_LANG_SOURCE([[int main(int argc, char **argv)
{
	int kqfd = kqueue();
	exit (kqfd == -1 ? 1 : 0);
}]])],[AC_MSG_RESULT(yes)
AC_DEFINE(HAVE_KQUEUE,1, [define if your system supports kqueue])],[AC_MSG_RESULT(no)],[AC_MSG_RESULT(no)])
fi

dnl ----------------------------------------------------------------
AC_CHECK_HEADERS( sys/devpoll.h )
dnl "/dev/poll" needs <sys/poll.h> as well...
if test "${ac_cv_header_sys_devpoll_h}" = yes \
		-a "${ac_cv_header_poll_h}" = yes ; \
then
	AC_MSG_CHECKING(for /dev/poll)
	AC_RUN_IFELSE([AC_LANG_SOURCE([[int main(int argc, char **argv)
{
	int devpollfd = open("/dev/poll", /* O_RDWR */ 2);
	exit (devpollfd == -1 ? 1 : 0);
}]])],[AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_DEVPOLL,1, [define if your system supports /dev/poll])],[AC_MSG_RESULT(no)],[AC_MSG_RESULT(no)])
fi

dnl ----------------------------------------------------------------
OL_STRERROR

dnl ----------------------------------------------------------------
dnl require POSIX regex
AC_CHECK_HEADERS( regex.h, [], [],
[$ac_includes_default
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

if test "$ac_cv_header_regex_h" != yes ; then
	AC_MSG_ERROR([POSIX regex.h required.])
fi
AC_SEARCH_LIBS(regfree, [regex gnuregex],
	:, AC_MSG_ERROR([POSIX regex required.]))

OL_POSIX_REGEX
if test "$ol_cv_c_posix_regex" = no ; then
	AC_MSG_ERROR([broken POSIX regex!])
fi

dnl ----------------------------------------------------------------
dnl UUID Support

have_uuid=no
AC_CHECK_HEADERS(sys/uuid.h)
dnl The HAVE_UUID_TO_STR code path also needs uuid_create
if test $ac_cv_header_sys_uuid_h = yes ; then
	save_LIBS="$LIBS"
	AC_SEARCH_LIBS([uuid_to_str], [uuid], [have_uuid=yes], :)
	AC_SEARCH_LIBS([uuid_create], [uuid], :, [have_uuid=no])
	LIBS="$save_LIBS"

	if test $have_uuid = yes ; then
		AC_DEFINE(HAVE_UUID_TO_STR,1,
			[define if you have uuid_to_str()])

		test "$ac_cv_search_uuid_to_str" = "none required" || \
			LUTIL_LIBS="$LUTIL_LIBS $ac_cv_search_uuid_to_str"
	fi
fi

dnl Look for uuid_generate
dnl The HAVE_UUID_GENERATE code path also needs uuid_unparse_lower
if test $have_uuid = no ; then
	AC_CHECK_HEADERS(uuid/uuid.h)
	if test $ac_cv_header_uuid_uuid_h = yes ; then
		save_LIBS="$LIBS"
		AC_SEARCH_LIBS([uuid_generate], [uuid], [have_uuid=yes], :)
		AC_SEARCH_LIBS([uuid_unparse_lower], [uuid], :, [have_uuid=no])
		LIBS="$save_LIBS"

		if test $have_uuid = yes ; then
			AC_DEFINE(HAVE_UUID_GENERATE,1,
				[define if you have uuid_generate()])

			test "$ac_cv_search_uuid_generate" = "none required" || \
				LUTIL_LIBS="$LUTIL_LIBS $ac_cv_search_uuid_generate"
		fi
	fi
fi

dnl For windows, check for the need of RPCRT for UUID function support
if test $have_uuid = no ; then
	AC_MSG_CHECKING(to see if -lrpcrt4 is needed for win32 UUID support)
	save_LIBS="$LIBS"
	LIBS="$LIBS -lrpcrt4"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
		int __stdcall UuidCreate(void *);
		int __stdcall UuidToStringA(void *,void **);
		]], [[
		UuidCreate(0);
		UuidToStringA(0,0);
		]])],[need_rpcrt=yes],[need_rpcrt=no])
	if test $need_rpcrt = yes; then
		SLAPD_LIBS="$SLAPD_LIBS -lrpcrt4"
		CLIENT_LIBS="$CLIENT_LIBS -lrpcrt4"
	fi
	LIBS="$save_LIBS"
	AC_MSG_RESULT($need_rpcrt)
fi

dnl ----------------------------------------------------------------
dnl Check for resolver routines
OL_RESOLVER_LINK

ol_link_dnssrv=no
if test "$ol_cv_lib_resolver" != no ; then
	AC_DEFINE(HAVE_RES_QUERY,1,
		[define if you have res_query()])

	if test "$ol_enable_dnssrv" != no ; then
		ol_link_dnssrv=yes
	fi

	if test "$ol_cv_lib_resolver" != yes ; then
		LIBS="$ol_cv_lib_resolver $LIBS"
	fi
fi

if test "$ol_enable_dnssrv" = yes || test "$ol_enable_dnssrv" = mod ; then
	if test "$ol_link_dnssrv" = no ; then
		AC_MSG_ERROR([DNSSRV requires res_query()])
	fi
else
	ol_enable_dnssrv=no
fi

AC_CHECK_FUNCS( hstrerror )

dnl ----------------------------------------------------------------
dnl PF_INET6 support requires getaddrinfo and INET6_ADDRSTRLEN
dnl PF_LOCAL may use getaddrinfo in available
AC_CHECK_FUNCS( getaddrinfo getnameinfo gai_strerror inet_ntop )

ol_link_ipv6=no
if test $ac_cv_func_getaddrinfo = no || test $ac_cv_func_inet_ntop = no ; then
	if test $ol_enable_ipv6 = yes ; then
		AC_MSG_ERROR([IPv6 support requires getaddrinfo() and inet_ntop()])
	fi
elif test $ol_enable_ipv6 != no ; then
	AC_CACHE_CHECK([INET6_ADDRSTRLEN],[ol_cv_inet6_addrstrlen],[
		AC_EGREP_CPP(__has_inet6_addrstrlen__,[
#			include <netinet/in.h>
#			ifdef INET6_ADDRSTRLEN
				__has_inet6_addrstrlen__;
#			endif
		], [ol_cv_inet6_addrstrlen=yes], [ol_cv_inet6_addrstrlen=no])])


	AC_CACHE_CHECK([struct sockaddr_storage],ol_cv_struct_sockaddr_storage,[
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
]], [[
			struct sockaddr_storage ss;
]])],[ol_cv_struct_sockaddr_storage=yes],[ol_cv_struct_sockaddr_storage=no])])

	if test $ol_cv_inet6_addrstrlen = yes &&
	   test $ol_cv_struct_sockaddr_storage = yes ; then
		ol_link_ipv6=yes
	elif test $ol_enable_ipv6 = yes &&
	     test $ol_cv_inet6_addrstrlen = no ; then
		AC_MSG_ERROR([IPv6 support requires INET6_ADDRSTRLEN])
	elif test $ol_enable_ipv6 = yes &&
	     test $ol_cv_struct_sockaddr_storage = no ; then
		AC_MSG_ERROR([IPv6 support requires struct sockaddr_storage])
	fi
fi

if test $ol_enable_local != no ; then
	AC_CHECK_HEADERS( sys/un.h )

	if test $ol_enable_local = auto ; then
		ol_enable_local=$ac_cv_header_sys_un_h
	elif test $ac_cv_header_sys_un_h = no ; then
		AC_MSG_ERROR([AF_LOCAL domain support requires sys/un.h])
	fi
fi

dnl ----------------------------------------------------------------
dnl TLS/SSL
	
if test $ol_with_tls = yes ; then
	ol_with_tls=auto
fi

ol_link_tls=no
if test $ol_with_tls = openssl || test $ol_with_tls = auto ; then
	AC_CHECK_HEADERS(openssl/ssl.h)

	if test $ac_cv_header_openssl_ssl_h = yes ; then
			AC_PREPROC_IFELSE([AC_LANG_SOURCE(
				[[#include <openssl/opensslv.h>]
[#if OPENSSL_VERSION_NUMBER < 0x1010100fL]
[#error "OpenSSL is too old"]
[#endif]])],
					, [AC_MSG_FAILURE([OpenSSL 1.1.1 or newer required])])

		AC_CHECK_LIB(ssl, SSL_export_keying_material_early,
			[have_openssl=yes], [have_openssl=no],
			[-lcrypto])

		if test $have_openssl = yes ; then
			ol_with_tls=openssl
			ol_link_tls=yes
			WITH_TLS_TYPE=openssl

			AC_DEFINE(HAVE_OPENSSL, 1, 
				[define if you have OpenSSL])

			TLS_LIBS="-lssl -lcrypto"
		fi
	fi
fi

if test $ol_link_tls = no ; then
	if test $ol_with_tls = gnutls || test $ol_with_tls = auto ; then
		AC_CHECK_HEADERS(gnutls/gnutls.h)

		if test $ac_cv_header_gnutls_gnutls_h = yes ; then
			AC_PREPROC_IFELSE([AC_LANG_SOURCE(
				[[#include <gnutls/gnutls.h>]
[#if GNUTLS_VERSION_NUMBER < 0x030306]
[#error "GnuTLS is too old"]
[#endif]])],
					, [AC_MSG_FAILURE([GnuTLS 3.3.6 or newer required])])

			AC_CHECK_LIB(gnutls, gnutls_init,
				[have_gnutls=yes], [have_gnutls=no])

			if test $have_gnutls = yes ; then
				ol_with_tls=gnutls
				ol_link_tls=yes
				WITH_TLS_TYPE=gnutls

				TLS_LIBS="-lgnutls"

				AC_DEFINE(HAVE_GNUTLS, 1, 
					[define if you have GNUtls])
			fi
		fi
	fi
fi

WITH_TLS=no
if test $ol_link_tls = yes ; then
	AC_DEFINE(HAVE_TLS, 1, [define if you have TLS])
	WITH_TLS=yes
elif test $ol_with_tls = auto ; then
	AC_MSG_WARN([Could not locate TLS/SSL package])
	AC_MSG_WARN([TLS data protection not supported!])
elif test $ol_with_tls != no ; then
	AC_MSG_ERROR([Could not locate TLS/SSL package])
else
	AC_MSG_WARN([TLS data protection not supported!])
fi


dnl ----------------------------------------------------------------
dnl Threads?
ol_link_threads=no

case $ol_with_threads in auto | yes | nt)

	OL_NT_THREADS

	if test "$ol_cv_nt_threads" = yes ; then
		ol_link_threads=nt
		ol_with_threads=found
		ol_with_yielding_select=yes

		AC_DEFINE(HAVE_NT_SERVICE_MANAGER,1,[if you have NT Service Manager])
		AC_DEFINE(HAVE_NT_EVENT_LOG,1,[if you have NT Event Log])
	fi

	if test $ol_with_threads = nt ; then
		AC_MSG_ERROR([could not locate NT Threads])
	fi
	;;
esac

case $ol_with_threads in auto | yes | posix)

	AC_CHECK_HEADERS(pthread.h)

	if test $ac_cv_header_pthread_h = yes ; then
		OL_POSIX_THREAD_VERSION

		if test $ol_cv_pthread_version != 0 ; then
			AC_DEFINE_UNQUOTED(HAVE_PTHREADS,$ol_cv_pthread_version,
				[define to pthreads API spec revision])
		else
			AC_MSG_ERROR([unknown pthread version])
		fi

		# consider threads found
		ol_with_threads=found

		OL_HEADER_LINUX_THREADS
		OL_HEADER_GNU_PTH_PTHREAD_H

		if test $ol_cv_header_gnu_pth_pthread_h = no ; then
			AC_CHECK_HEADERS(sched.h)
		fi

		dnl Now the hard part, how to link?
		dnl
		dnl currently supported checks:
		dnl
		dnl Check for no flags 
		dnl 	pthread_create() in $LIBS
		dnl
		dnl Check special pthread (final) flags
		dnl 	[skipped] pthread_create() with -mt (Solaris) [disabled]
		dnl 	pthread_create() with -kthread (FreeBSD)
		dnl 	pthread_create() with -pthread (FreeBSD/Digital Unix)
		dnl 	pthread_create() with -pthreads (?)
		dnl 	pthread_create() with -mthreads (AIX)
		dnl 	pthread_create() with -thread (?)
		dnl
		dnl Check pthread (final) libraries
		dnl 	pthread_mutex_unlock() in -lpthread -lmach -lexc -lc_r (OSF/1)
		dnl 	pthread_mutex_lock() in -lpthread -lmach -lexc (OSF/1)
		dnl 	[skipped] pthread_mutex_trylock() in -lpthread -lexc (OSF/1)
		dnl 	pthread_join() -Wl,-woff,85 -lpthread (IRIX)
		dnl 	pthread_create() in -lpthread (many)
		dnl 	pthread_create() in -lc_r (FreeBSD)
		dnl
		dnl Check pthread (draft4) flags (depreciated)
		dnl 	pthread_create() with -threads (OSF/1)
		dnl
		dnl Check pthread (draft4) libraries (depreciated)
		dnl 	pthread_mutex_unlock() in -lpthreads -lmach -lexc -lc_r (OSF/1)
		dnl 	pthread_mutex_lock() in -lpthreads -lmach -lexc (OSF/1)
		dnl 	pthread_mutex_trylock() in -lpthreads -lexc (OSF/1)
		dnl 	pthread_create() in -lpthreads (many)
		dnl

		dnl pthread_create in $LIBS
		AC_CACHE_CHECK([for pthread_create in default libraries],
			ol_cv_pthread_create,[
			AC_RUN_IFELSE([OL_PTHREAD_TEST_PROGRAM],
				[ol_cv_pthread_create=yes],
				[ol_cv_pthread_create=no],
				[AC_TRY_LINK(OL_PTHREAD_TEST_INCLUDES,OL_PTHREAD_TEST_FUNCTION,
					[ol_cv_pthread_create=yes],
					[ol_cv_pthread_create=no])])])

		if test $ol_cv_pthread_create != no ; then
			ol_link_threads=posix
			ol_link_pthreads=""
		fi
		
dnl		OL_PTHREAD_TRY([-mt],		[ol_cv_pthread_mt])
		OL_PTHREAD_TRY([-kthread],	[ol_cv_pthread_kthread])
		OL_PTHREAD_TRY([-pthread],	[ol_cv_pthread_pthread])
		OL_PTHREAD_TRY([-pthreads],	[ol_cv_pthread_pthreads])
		OL_PTHREAD_TRY([-mthreads],	[ol_cv_pthread_mthreads])
		OL_PTHREAD_TRY([-thread],	[ol_cv_pthread_thread])

		OL_PTHREAD_TRY([-lpthread -lmach -lexc -lc_r],
			[ol_cv_pthread_lpthread_lmach_lexc_lc_r])
		OL_PTHREAD_TRY([-lpthread -lmach -lexc],
			[ol_cv_pthread_lpthread_lmach_lexc])
dnl		OL_PTHREAD_TRY([-lpthread -lexc],
dnl			[ol_cv_pthread_lpthread_lexc])

		OL_PTHREAD_TRY([-lpthread -Wl,-woff,85],
			[ol_cv_pthread_lib_lpthread_woff])

		OL_PTHREAD_TRY([-lpthread],	[ol_cv_pthread_lpthread])
		OL_PTHREAD_TRY([-lc_r],		[ol_cv_pthread_lc_r])

		OL_PTHREAD_TRY([-threads],	[ol_cv_pthread_threads])

		OL_PTHREAD_TRY([-lpthreads -lmach -lexc -lc_r],
			[ol_cv_pthread_lpthreads_lmach_lexc_lc_r])
		OL_PTHREAD_TRY([-lpthreads -lmach -lexc],
			[ol_cv_pthread_lpthreads_lmach_lexc])
		OL_PTHREAD_TRY([-lpthreads -lexc],
			[ol_cv_pthread_lpthreads_lexc])

		OL_PTHREAD_TRY([-lpthreads],[ol_cv_pthread_lib_lpthreads])

		if test $ol_link_threads != no ; then
			LTHREAD_LIBS="$LTHREAD_LIBS $ol_link_pthreads"

			dnl save flags
			save_CPPFLAGS="$CPPFLAGS"
			save_LIBS="$LIBS"
			LIBS="$LTHREAD_LIBS $LIBS"

			dnl All POSIX Thread (final) implementations should have
			dnl sched_yield instead of pthread yield.
			dnl check for both, and thr_yield for Solaris
			AC_CHECK_FUNCS(sched_yield pthread_yield thr_yield)

			if test $ac_cv_func_sched_yield = no &&
			   test $ac_cv_func_pthread_yield = no &&
			   test $ac_cv_func_thr_yield = no ; then
				dnl Digital UNIX has sched_yield() in -lrt
				AC_CHECK_LIB(rt, sched_yield,
					[LTHREAD_LIBS="$LTHREAD_LIBS -lrt"
					AC_DEFINE(HAVE_SCHED_YIELD,1,
						[Define if you have the sched_yield function.])
					ac_cv_func_sched_yield=yes],
					[ac_cv_func_sched_yield=no])
			fi
			if test $ac_cv_func_sched_yield = no &&
			   test $ac_cv_func_pthread_yield = no &&
			   test "$ac_cv_func_thr_yield" = no ; then
				AC_MSG_WARN([could not locate sched_yield() or pthread_yield()])
			fi

			dnl Check functions for compatibility
			AC_CHECK_FUNCS(pthread_kill)

			dnl Check for pthread_rwlock_destroy with <pthread.h>
			dnl as pthread_rwlock_t may not be defined.
			AC_CACHE_CHECK([for pthread_rwlock_destroy with <pthread.h>],
				[ol_cv_func_pthread_rwlock_destroy], [
				dnl save the flags
				AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <pthread.h>
pthread_rwlock_t rwlock;
]], [[pthread_rwlock_destroy(&rwlock);]])],[ol_cv_func_pthread_rwlock_destroy=yes],[ol_cv_func_pthread_rwlock_destroy=no])
			])
			if test $ol_cv_func_pthread_rwlock_destroy = yes ; then
				AC_DEFINE(HAVE_PTHREAD_RWLOCK_DESTROY,1,
					[define if you have pthread_rwlock_destroy function])
			fi

			dnl Check for pthread_detach with <pthread.h> inclusion
			dnl as it's symbol may have been mangled.
			AC_CACHE_CHECK([for pthread_detach with <pthread.h>],
				[ol_cv_func_pthread_detach], [
				dnl save the flags
				AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <pthread.h>
#ifndef NULL
#define NULL (void*)0
#endif
]], [[pthread_detach(NULL);]])],[ol_cv_func_pthread_detach=yes],[ol_cv_func_pthread_detach=no])
			])

			if test $ol_cv_func_pthread_detach = no ; then
				AC_MSG_ERROR([could not locate pthread_detach()])
			fi

			AC_DEFINE(HAVE_PTHREAD_DETACH,1,
				[define if you have pthread_detach function])

			dnl Check for setconcurrency functions
			AC_CHECK_FUNCS(	\
				pthread_setconcurrency \
				pthread_getconcurrency \
				thr_setconcurrency \
				thr_getconcurrency \
			)

			OL_SYS_LINUX_THREADS
			OL_LINUX_THREADS

			if test $ol_cv_linux_threads = error; then
				AC_MSG_ERROR([LinuxThreads header/library mismatch]);
			fi

			AC_CACHE_CHECK([AC_LANG_SOURCE([if pthread_create() works])],
				ol_cv_pthread_create_works,[
			AC_RUN_IFELSE([OL_PTHREAD_TEST_PROGRAM],
				[ol_cv_pthread_create_works=yes],
				[ol_cv_pthread_create_works=no],
				[dnl assume yes
				ol_cv_pthread_create_works=yes])])

			if test $ol_cv_pthread_create_works = no ; then
				AC_MSG_ERROR([pthread_create is not usable, check environment settings])
			fi

			ol_replace_broken_yield=no
dnl			case "$target" in
dnl			*-*-linux*) 
dnl				AC_CHECK_FUNCS(nanosleep)
dnl				ol_replace_broken_yield=yes
dnl			;;
dnl			esac

			if test $ol_replace_broken_yield = yes ; then
				AC_DEFINE([REPLACE_BROKEN_YIELD],1,
					[define if sched_yield yields the entire process])
			fi

			dnl Check if select causes an yield
			if test $ol_with_yielding_select = auto ; then
				AC_CACHE_CHECK([if select yields when using pthreads],
					ol_cv_pthread_select_yields,[
				AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#ifndef NULL
#define NULL (void*) 0
#endif

static int fildes[2];

static void *task(p)
	void *p;
{
	int i;
	struct timeval tv;

	fd_set rfds;

	tv.tv_sec=10;
	tv.tv_usec=0;

	FD_ZERO(&rfds);
	FD_SET(fildes[0], &rfds);

	/* we're not interested in any fds */
	i = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);

	if(i < 0) {
		perror("select");
		exit(10);
	}

	exit(0); /* if we exit here, the select blocked the whole process */
}

int main(argc, argv)
	int argc;
	char **argv;
{
	pthread_t t;

	/* create a pipe to select */
	if(pipe(&fildes[0])) {
		perror("select");
		exit(1);
	}

#ifdef HAVE_PTHREAD_SETCONCURRENCY
	(void) pthread_setconcurrency(2);
#else
#ifdef HAVE_THR_SETCONCURRENCY
	/* Set Solaris LWP concurrency to 2 */
	thr_setconcurrency(2);
#endif
#endif

#if HAVE_PTHREADS < 6
	pthread_create(&t, pthread_attr_default, task, NULL);
#else
	pthread_create(&t, NULL, task, NULL);
#endif

	/* make sure task runs first */
#ifdef HAVE_THR_YIELD
	thr_yield();
#elif defined( HAVE_SCHED_YIELD )
	sched_yield();
#elif defined( HAVE_PTHREAD_YIELD )
	pthread_yield();
#endif

	exit(2);
}]])],[ol_cv_pthread_select_yields=no],[ol_cv_pthread_select_yields=yes],[ol_cv_pthread_select_yields=cross])])

				if test $ol_cv_pthread_select_yields = cross ; then
					AC_MSG_ERROR([crossing compiling: use --with-yielding_select=yes|no|manual])
				fi

				if test $ol_cv_pthread_select_yields = yes ; then
					ol_with_yielding_select=yes
				fi
			fi

			dnl restore flags
			CPPFLAGS="$save_CPPFLAGS"
			LIBS="$save_LIBS"
		else
			AC_MSG_ERROR([could not locate usable POSIX Threads])
		fi
	fi

	if test $ol_with_threads = posix ; then
		AC_MSG_ERROR([could not locate POSIX Threads])
	fi
	;;
esac

case $ol_with_threads in auto | yes | pth)

	AC_CHECK_HEADERS(pth.h)

	if test $ac_cv_header_pth_h = yes ; then
		AC_CHECK_LIB(pth, pth_version, [have_pth=yes], [have_pth=no])

		if test $have_pth = yes ; then
			AC_DEFINE(HAVE_GNU_PTH,1,[if you have GNU Pth])
			LTHREAD_LIBS="$LTHREAD_LIBS -lpth"
			ol_link_threads=pth
			ol_with_threads=found

			if test $ol_with_yielding_select = auto ; then
				ol_with_yielding_select=yes
			fi
		fi
	fi
	;;
esac

case $ol_with_threads in auto | yes | lwp)

	dnl check for SunOS5 LWP
	AC_CHECK_HEADERS(thread.h synch.h)
	if test $ac_cv_header_thread_h = yes &&
	   test $ac_cv_header_synch_h = yes ; then
		AC_CHECK_LIB(thread, thr_create, [have_thr=yes], [have_thr=no])

		if test $have_thr = yes ; then
			AC_DEFINE(HAVE_THR,1,
				[if you have Solaris LWP (thr) package])
			LTHREAD_LIBS="$LTHREAD_LIBS -lthread"
			ol_link_threads=thr

			if test $ol_with_yielding_select = auto ; then
				ol_with_yielding_select=yes
			fi

			dnl Check for setconcurrency functions
			AC_CHECK_FUNCS(	\
				thr_setconcurrency \
				thr_getconcurrency \
			)
		fi
	fi
	;;
esac

if test $ol_with_yielding_select = yes ; then
	AC_DEFINE(HAVE_YIELDING_SELECT,1,
		[define if select implicitly yields])
fi

if test $ol_with_threads = manual ; then
	dnl User thinks he can manually configure threads.
	ol_link_threads=yes

	AC_MSG_WARN([thread defines and link options must be set manually])

	AC_CHECK_HEADERS(pthread.h sched.h)
	AC_CHECK_FUNCS(sched_yield pthread_yield)
	OL_HEADER_LINUX_THREADS

	AC_CHECK_HEADERS(thread.h synch.h)
fi

if test $ol_link_threads != no && test $ol_link_threads != nt ; then
	dnl needed to get reentrant/threadsafe versions
	dnl
	AC_DEFINE(REENTRANT,1)
	AC_DEFINE(_REENTRANT,1)
	AC_DEFINE(THREAD_SAFE,1)
	AC_DEFINE(_THREAD_SAFE,1)
	AC_DEFINE(THREADSAFE,1)
	AC_DEFINE(_THREADSAFE,1)
	AC_DEFINE(_SGI_MP_SOURCE,1)

	dnl The errno declaration may dependent upon _REENTRANT.
	dnl If it does, we must link with thread support.
	AC_CACHE_CHECK([for thread specific errno],
		[ol_cv_errno_thread_specific], [
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <errno.h>]], [[errno = 0;]])],[ol_cv_errno_thread_specific=yes],[ol_cv_errno_thread_specific=no])
	])

	dnl The h_errno declaration may dependent upon _REENTRANT.
	dnl If it does, we must link with thread support.
	AC_CACHE_CHECK([for thread specific h_errno],
		[ol_cv_h_errno_thread_specific], [
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[h_errno = 0;]])],[ol_cv_h_errno_thread_specific=yes],[ol_cv_h_errno_thread_specific=no])
	])

	if test $ol_cv_errno_thread_specific != yes ||
	   test $ol_cv_h_errno_thread_specific != yes ; then
		LIBS="$LTHREAD_LIBS $LIBS"
		LTHREAD_LIBS=""
	fi

dnl When in thread environment, use 
dnl		#if defined( HAVE_REENTRANT_FUNCTIONS ) || defined( HAVE_FUNC_R )
dnl			func_r(...);
dnl		#else
dnl			/* lock */
dnl			func(...);
dnl			/* unlock */
dnl		#endif
dnl
dnl HAVE_REENTRANT_FUNCTIONS is derived from:
dnl		_POSIX_REENTRANT_FUNCTIONS
dnl		_POSIX_THREAD_SAFE_FUNCTIONS
dnl		_POSIX_THREADSAFE_FUNCTIONS
dnl
dnl		and is currently defined in <ldap_pvt_thread.h>
dnl
dnl libldap/*.c should only include <ldap_pvt_thread.h> iff
dnl LDAP_R_COMPILE is defined.  ie:
dnl		#ifdef LDAP_R_COMPILE
dnl		#	include <ldap_pvt_thread.h>
dnl		#endif
dnl
dnl LDAP_R_COMPILE is defined by libldap/ldap-int.h
dnl specifically for compiling the threadsafe version of
dnl the ldap library.
dnl		
dnl	dnl check for reentrant/threadsafe functions
dnl	dnl
dnl	dnl note: these should only be used when linking
dnl	dnl		with $LTHREAD_LIBS
dnl	dnl
dnl	save_CPPFLAGS="$CPPFLAGS"
dnl	save_LIBS="$LIBS"
dnl	LIBS="$LTHREAD_LIBS $LIBS"
dnl	AC_CHECK_FUNCS(	\
dnl		gmtime_r \
dnl		gethostbyaddr_r gethostbyname_r \
dnl		feof_unlocked unlocked_feof \
dnl		putc_unlocked unlocked_putc \
dnl		flockfile ftrylockfile \
dnl	)
dnl	CPPFLAGS="$save_CPPFLAGS"
dnl	LIBS="$save_LIBS"
fi  

if test $ol_link_threads = no ; then
	if test $ol_enable_slapd != no; then
		AC_MSG_ERROR([slapd requires thread support])
	fi

	if test $ol_with_threads = yes ; then
		AC_MSG_ERROR([no suitable thread support])
	fi

	if test $ol_with_threads = auto ; then
		AC_MSG_WARN([no suitable thread support, disabling threads])
		ol_with_threads=no
	fi

	AC_DEFINE(NO_THREADS,1,
		[define if you have (or want) no threads])
	LTHREAD_LIBS=""
	BUILD_THREAD=no
else
	BUILD_THREAD=yes
fi

if test $ol_link_threads != no ; then
	AC_DEFINE(LDAP_API_FEATURE_X_OPENLDAP_THREAD_SAFE,1,
		[define to 1 if library is thread safe])

	dnl This could be enabled without threads if all of the
	dnl reentrant functions are available. Needs testing.
	AC_DEFINE(LDAP_API_FEATURE_X_OPENLDAP_REENTRANT,1,
		[define to 1 if library is reentrant])
fi

dnl ----------------------------------------------------------------
dnl Tests for reentrant functions necessary for reentrant build
AC_CHECK_FUNCS(		\
	ctime_r			\
	gmtime_r localtime_r \
	gethostbyname_r	gethostbyaddr_r \
)

if test "$ac_cv_func_ctime_r" = no ; then
	ol_cv_func_ctime_r_nargs=0
else
	OL_FUNC_CTIME_R_NARGS
dnl	OL_FUNC_CTIME_R_TYPE
fi

if test "$ac_cv_func_gethostbyname_r" = yes ; then
 	OL_FUNC_GETHOSTBYNAME_R_NARGS
else
 	ol_cv_func_gethostbyname_r_nargs=0
fi
 
if test "$ac_cv_func_gethostbyaddr_r" = yes ; then
 	OL_FUNC_GETHOSTBYADDR_R_NARGS
else
 	ol_cv_func_gethostbyaddr_r_nargs=0
fi

dnl ----------------------------------------------------------------

if test $ol_enable_dynamic = yes && test $enable_shared = yes ; then
	BUILD_LIBS_DYNAMIC=shared
	AC_DEFINE(LDAP_LIBS_DYNAMIC, 1, [define if LDAP libs are dynamic])
	LTSTATIC=""
else
	BUILD_LIBS_DYNAMIC=static
	LTSTATIC="-static"
fi
AC_SUBST(LTSTATIC)dnl

dnl ----------------------------------------------------------------
if test $ol_enable_wrappers != no ; then
	AC_CHECK_HEADERS(tcpd.h,[
		AC_MSG_CHECKING([for TCP wrappers library])
		save_LIBS="$LIBS"
		LIBS="$LIBS -lwrap"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <tcpd.h>
int allow_severity = 0;
int deny_severity  = 0;

struct request_info *req;
		]], [[
hosts_access(req)
		]])],[AC_MSG_RESULT([-lwrap])
		have_wrappers=yes
		LIBS="$save_LIBS"],[
		dnl try with -lnsl
		LIBS="$LIBS -lnsl"
		AC_TRY_LINK([
#include <tcpd.h>
int allow_severity = 0;
int deny_severity  = 0;

struct request_info *req;
		],[
hosts_access(req)
		],[AC_MSG_RESULT([-lwrap -lnsl])
		have_wrappers=yes
		LIBS="$save_LIBS -lnsl"],[
		AC_MSG_RESULT(no)
		have_wrappers=no
		LIBS=$save_LIBS])])],[have_wrappers=no])

	if test $have_wrappers = yes ; then
		AC_DEFINE(HAVE_TCPD,1, [define if you have -lwrap])
		WRAP_LIBS="-lwrap"
	elif test $ol_enable_wrappers = yes ; then
		AC_MSG_ERROR([could not find TCP wrappers, select appropriate options or disable])
	else
		AC_MSG_WARN([could not find TCP wrappers, support disabled])
		WRAP_LIBS=""
	fi
fi

dnl ----------------------------------------------------------------
if test $ol_enable_syslog != no ; then
	AC_CHECK_FUNC(openlog)
	if test $ac_cv_func_openlog = no && test $ol_enable_syslog = yes; then
		AC_MSG_ERROR(could not find syslog, select appropriate options or disable)
	fi
	ol_enable_syslog=$ac_cv_func_openlog
fi

dnl ----------------------------------------------------------------
dnl SQL
ol_link_sql=no
if test $ol_enable_sql != no ; then
	AC_CHECK_HEADERS(sql.h sqlext.h,[],[
		AC_MSG_ERROR([could not locate SQL headers])
	])

	sql_LIBS="$LIBS"
	LIBS="$LTHREAD_LIBS $LIBS"

	if test $ol_with_odbc = auto ; then
		ol_with_odbc="iodbc unixodbc odbc32"
	fi

	for odbc in $ol_with_odbc ; do
		if test $ol_link_sql = no ; then
			case $odbc in
			iodbc)
				AC_CHECK_LIB(iodbc, SQLDriverConnect, [have_iodbc=yes], [have_iodbc=no])
				if test $have_iodbc = yes ; then
					ol_link_sql="-liodbc"
				fi
				;;

			unixodbc)
				AC_CHECK_LIB(odbc, SQLDriverConnect, [have_odbc=yes], [have_odbc=no])
				if test $have_odbc = yes ; then
					ol_link_sql="-lodbc"
				fi
				;;

			odbc32)
				AC_CHECK_LIB(odbc32, SQLDriverConnect, [have_odbc32=yes], [have_odbc32=no])
				
				dnl The windows API uses __stdcall which cannot be detected by AC_CHECK_LIB
				if test $have_odbc32 = no ; then
					AC_MSG_CHECKING([for SQLDriverConnect in -lodbc32 with windows.h])
					save_LIBS="$LIBS"
					LIBS="$LIBS -lodbc32"
					AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <windows.h>
					#include <sqlext.h>
						]], [[
						SQLDriverConnect(NULL,NULL,NULL,0,NULL,0,NULL,0);
						]])],[have_odbc32=yes], [have_odbc32=no])
					LIBS="$save_LIBS"
					AC_MSG_RESULT($have_odbc32)
				fi
				
				if test $have_odbc32 = yes ; then
					ol_link_sql="-lodbc32"
				fi
				;;

			*)
				AC_MSG_ERROR([unknown ODBC library])
				;;
			esac
		fi
	done

	LIBS="$sql_LIBS"

	if test $ol_link_sql != no ; then
		SLAPD_SQL_LIBS="$ol_link_sql"

	elif test $ol_enable_sql != auto ; then
		AC_MSG_ERROR([could not locate suitable ODBC library])
	fi
fi

dnl ----------------------------------------------------------------
dnl MySQL NDBapi
dnl Note: uses C++, but we don't want to add C++ test overhead to
dnl the rest of the libtool machinery.
ol_link_ndb=no
if test $ol_enable_ndb != no ; then
	AC_CHECK_PROG(MYSQL,mysql_config,yes)
	if test "$MYSQL" != yes ; then
		AC_MSG_ERROR([could not locate mysql_config])
	fi

	SQL_INC=`mysql_config --include`
	SLAPD_NDB_INCS="$SQL_INC $SQL_INC/storage/ndb $SQL_INC/storage/ndb/ndbapi"

	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$SLAPD_NDB_INCS"
	AC_MSG_CHECKING(for NdbApi.hpp)
	AC_PREPROC_IFELSE(
		[AC_LANG_SOURCE([[#include <NdbApi.hpp>]])],
			AC_MSG_RESULT(yes),
			AC_MSG_ERROR([could not locate NdbApi headers])
	)
	CPPFLAGS="$save_CPPFLAGS"

	SQL_LIB=`mysql_config --libs_r`
	SLAPD_NDB_LIBS="$SQL_LIB -lndbclient -lstdc++"

	save_LDFLAGS="$LDFLAGS"
	save_LIBS="$LIBS"
	LDFLAGS="$SQL_LIB"
	AC_CHECK_LIB(ndbclient,ndb_init,[: ok],[
		AC_MSG_ERROR([could not locate ndbclient library])
	],[-lstdc++])
	LIBS="$save_LIBS"
	LDFLAGS="$save_LDFLAGS"

	if test "$ol_enable_ndb" = yes ; then
		SLAPD_LIBS="$SLAPD_LIBS \$(SLAPD_NDB_LIBS)"
	fi
fi

dnl ----------------------------------------------------------------
dnl WiredTiger
ol_link_wt=no
if test $ol_enable_wt != no ; then
	PKG_CHECK_MODULES(WT, wiredtiger)
	if test $ol_enable_wt = yes ; then
		SLAPD_LIBS="$SLAPD_LIBS \$(WT_LIBS)"
	fi
	ol_link_wt=yes
fi

dnl ----------------------------------------------------------------
dnl
dnl Check for Cyrus SASL
dnl
WITH_SASL=no
ol_link_sasl=no
ol_link_spasswd=no
if test $ol_with_cyrus_sasl != no ; then
	AC_CHECK_HEADERS(sasl/sasl.h sasl.h)

	if test $ac_cv_header_sasl_sasl_h = yes ||
	   test $ac_cv_header_sasl_h = yes; then
		AC_CHECK_LIB(sasl2, sasl_client_init,
			[ol_link_sasl="-lsasl2"],
			[AC_CHECK_LIB(sasl, sasl_client_init,
				[ol_link_sasl="-lsasl"])])
	fi

	if test $ol_link_sasl = no ; then
		if test $ol_with_cyrus_sasl != auto ; then
			AC_MSG_ERROR([Could not locate Cyrus SASL])
		else
			AC_MSG_WARN([Could not locate Cyrus SASL])
			AC_MSG_WARN([SASL authentication not supported!])
			if test $ol_link_tls = no ; then
				AC_MSG_WARN([Strong authentication not supported!])
			fi
		fi
	else
		OL_SASL_COMPAT
		if test $ol_cv_sasl_compat = no ; then
			ol_link_sasl=no
			AC_MSG_ERROR([Cyrus SASL library located but is incompatible])
		fi

		AC_DEFINE(HAVE_CYRUS_SASL,1,[define if you have Cyrus SASL])
		SASL_LIBS="$ol_link_sasl"
		if test $ol_enable_spasswd != no ; then
			ol_link_spasswd=yes
		fi

		ac_save_LIBS="$LIBS"
		LIBS="$LIBS $ol_link_sasl"
		AC_CHECK_FUNC(sasl_version, [AC_DEFINE(HAVE_SASL_VERSION,1,
			[define if your SASL library has sasl_version()])])
		LIBS="$ac_save_LIBS"

		WITH_SASL=yes
	fi

else
	AC_MSG_WARN([SASL authentication not supported!])
	if test $ol_link_tls = no ; then
		AC_MSG_WARN([Strong authentication not supported!])
	fi
fi

dnl ----------------------------------------------------------------
dnl
dnl Check for systemd (only if we have a server)
dnl
WITH_SYSTEMD=no
systemdsystemunitdir=
ol_link_systemd=no
if test $ol_enable_slapd == no && test $ol_enable_balancer != yes ; then
	if test $ol_with_systemd != no ; then
		AC_MSG_WARN([servers disabled, ignoring --with-systemd=$ol_with_systemd argument])
		ol_with_systemd=no
	fi
fi
if test $ol_with_systemd != no ; then
	AC_CHECK_HEADERS(systemd/sd-daemon.h)

	if test $ac_cv_header_systemd_sd_daemon_h = yes; then
		AC_CHECK_LIB(systemd, sd_notify,
			[ol_link_systemd="-lsystemd"])
	fi

	if test $ol_link_systemd = no ; then
		if test $ol_with_systemd != auto ; then
			AC_MSG_ERROR([Could not locate systemd])
		else
			AC_MSG_WARN([Could not locate systemd])
			AC_MSG_WARN([systemd service notification not supported!])
		fi
	else
		AC_DEFINE(HAVE_SYSTEMD,1,[define if you have systemd])
		SYSTEMD_LIBS="$ol_link_systemd"
		WITH_SYSTEMD=yes

		PKG_CHECK_VAR(systemdsystemunitdir, systemd, systemdsystemunitdir)
		if test -z "$systemdsystemunitdir"; then
			if test -d /usr/lib/systemd/system; then
				systemdsystemunitdir=/usr/lib/systemd/system
			else
				systemdsystemunitdir=/lib/systemd/system
			fi
		fi
	fi
fi
AC_SUBST(systemdsystemunitdir)

dnl ----------------------------------------------------------------
dnl Check for entropy sources
if test $cross_compiling != yes && test "$ac_cv_mingw32" != yes ; then
	dev=no
	if test -r /dev/urandom ; then
		dev="/dev/urandom";
	elif test -r /idev/urandom ; then
		dev="/idev/urandom";
	elif test -r /dev/srandom ; then
		dev="/dev/srandom";
	elif test -r /dev/random ; then
		dev="/dev/random";
	elif test -r /idev/random ; then
		dev="/idev/random";
	fi

	if test $dev != no ; then
		AC_DEFINE_UNQUOTED(URANDOM_DEVICE,"$dev",[set to urandom device])
	fi
fi

dnl ----------------------------------------------------------------
dnl
dnl Check for fetch URL support
dnl		should be extended to support other fetch URL APIs
dnl
ol_link_fetch=no
if test $ol_with_fetch != no ; then
	OL_LIB_FETCH

	if test $ol_cv_lib_fetch != no ; then
		LIBS="$LIBS $ol_link_fetch"
		ol_link_fetch=freebsd

	elif test $ol_with_fetch != auto ; then
		AC_MSG_ERROR(no suitable API for --with-fetch=$ol_with_fetch)
	fi 
fi

dnl ----------------------------------------------------------------
dnl FreeBSD (and others) have crypt(3) in -lcrypt
if test $ol_enable_crypt != no ; then
	save_LIBS="$LIBS"
	LIBS="$TLS_LIBS $LIBS"

	AC_CHECK_FUNC(crypt, [have_crypt=yes], [
		LIBS="$save_LIBS"
		AC_CHECK_LIB(crypt, crypt, [LUTIL_LIBS="$LUTIL_LIBS -lcrypt"
			have_crypt=yes], [have_crypt=no])])

	LIBS="$TLS_LIBS $LIBS"
	AC_CHECK_LIB(crypt, crypt_r, [have_crypt_r=yes], [have_crypt_r=no])

	LIBS="$save_LIBS"

	if test $have_crypt = yes ; then
		AC_DEFINE(HAVE_CRYPT,1, [define if crypt(3) is available])
		if test $have_crypt_r = yes ; then
			AC_DEFINE(HAVE_CRYPT_R, 1, [define if crypt_r() is also available])
		fi
	else
		AC_MSG_WARN([could not find crypt])
		if test $ol_enable_crypt = yes ; then
			AC_MSG_ERROR([could not find crypt, select appropriate options or disable])
		fi

		AC_MSG_WARN([disabling crypt support])
		ol_enable_crypt=no
	fi
fi

dnl ----------------------------------------------------------------
if test $ol_enable_slp != no ; then
	AC_CHECK_HEADERS( slp.h )

	if test $ac_cv_header_slp_h = yes ; then
		AC_CHECK_LIB(slp, SLPOpen, [have_slp=yes], [have_slp=no])
		if test $have_slp = yes ; then
			AC_DEFINE(HAVE_SLP, 1, [define if you have -lslp])
			SLAPD_SLP_LIBS=-lslp
		fi

	elif test $ol_enable_slp = yes ; then
		AC_MSG_ERROR([SLP not found])
	fi
fi

dnl ----------------------------------------------------------------
dnl Libevent
if test $ol_enable_balancer != no ; then
	AC_CHECK_LIB(event_extra, evdns_base_new,
		[have_libevent=yes
		LEVENT_LIBS="$LEVENT_LIBS -levent_core -levent_extra"],
		[AC_CHECK_LIB(event, evdns_base_new,
			[have_libevent=yes
			LEVENT_LIBS="$LEVENT_LIBS -levent"],
			[have_libevent=no])])
	AC_CHECK_LIB(event, libevent_global_shutdown, [], [have_libevent=no])

	if test $have_libevent = yes ; then
		AC_DEFINE(HAVE_LIBEVENT, 1, [define if you have -levent])
	else
		AC_MSG_ERROR([You need libevent 2.1 or later with DNS support to build the load balancer])
	fi
fi

dnl ----------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.

AC_CHECK_TYPE(mode_t, int)
AC_CHECK_TYPE(off_t, long)
AC_CHECK_TYPE(pid_t, int)
AC_CHECK_TYPE(ssize_t, [signed int])
AC_CHECK_TYPE(caddr_t,	[char *])
AC_CHECK_TYPE(size_t, unsigned)

AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([ptrdiff_t])


AC_CHECK_TYPE([socklen_t],,, [$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_WINSOCK2
#include <ws2tcpip.h>
#endif])

dnl socklen_t-like type in accept(), default socklen_t or int:
dnl - The OS might define socklen_t without using it.  POSIX moved from
dnl   int to size_t to socklen_t, hoping to stay at a 32-bit type, and
dnl   HP-UX now has selectors for what to use.
dnl - On Solaris 2.8 the prototype has void *len, but the default is OK.
AC_MSG_CHECKING([the type of arg 3 to accept()])
AC_CACHE_VAL(ol_cv_type_ber_socklen_t, [
	set socklen_t int unsigned "unsigned long" long size_t
	test "$ac_cv_type_socklen_t" = yes || shift
	ol_cv_type_ber_socklen_t=$1 guessing="guessing "
	for lentype in "$@" ; do for addrtype in "struct sockaddr" void ; do
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
extern int accept(int s, $addrtype *ap, $lentype *lp);
], [
accept(0, (struct sockaddr *) 0, ($lentype *) 0);
])], [ol_cv_type_ber_socklen_t=$lentype guessing= ; break 2])
	done ; done])
AC_MSG_RESULT([$guessing$ol_cv_type_ber_socklen_t *])
AC_DEFINE_UNQUOTED(ber_socklen_t, $ol_cv_type_ber_socklen_t,
	[Define to the type of arg 3 for `accept'.])

dnl Modules should use ber_socklen_t, not socklen_t.  Define socklen_t
dnl for the time being anyway, for backwards compatibility.
if test "$ac_cv_type_socklen_t" != yes; then
	AC_DEFINE_UNQUOTED([socklen_t], [$ol_cv_type_ber_socklen_t],
		[Define like ber_socklen_t if <sys/socket.h> does not define.])
fi


AC_TYPE_SIGNAL

AC_CHECK_TYPE([sig_atomic_t],,
	[AC_DEFINE_UNQUOTED([sig_atomic_t], [int],
		[Define to `int' if <signal.h> does not define.])],
	[$ac_includes_default
#include <signal.h>
	])

AC_TYPE_UID_T

AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct passwd.pw_gecos],,,[$ac_includes_default
#include <pwd.h>])
AC_CHECK_MEMBERS([struct passwd.pw_passwd],,,[$ac_includes_default
#include <pwd.h>])

OL_C_UPPER_LOWER
AC_C_CONST
OL_C_VOLATILE

if test $cross_compiling = yes ; then
	AC_MSG_WARN([Crossing compiling... all bets are off!])
	AC_DEFINE(CROSS_COMPILING, 1, [define if cross compiling])
else
	AC_C_BIGENDIAN
fi

AC_CHECK_SIZEOF(short) 
AC_CHECK_SIZEOF(int) 
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(wchar_t)

if test "$ac_cv_sizeof_int" -lt 4 ; then
	AC_MSG_WARN([OpenLDAP requires 'int' to be 32 bits or greater.])

	AC_DEFINE(LBER_INT_T,long,[define to 32-bit or greater integer type])
else
	AC_DEFINE(LBER_INT_T,int,[define to 32-bit or greater integer type])
fi

AC_DEFINE(LBER_LEN_T,long,[define to large integer type])
AC_DEFINE(LBER_SOCKET_T,int,[define to socket descriptor type])
AC_DEFINE(LBER_TAG_T,long,[define to large integer type])

dnl ----------------------------------------------------------------
dnl Check for multiple precision support
if test $ol_with_mp = longlong || test $ol_with_mp = auto ; then
	if test $ac_cv_sizeof_long_long -gt 4 ; then
		ol_with_mp=longlong
		AC_DEFINE(USE_MP_LONG_LONG,1,[define to use 'long long' for MP])
	elif test $ol_with_mp = longlong ; then
		AC_MSG_ERROR([long long unusable for multiple precision])
	fi
fi
if test $ol_with_mp = long || test $ol_with_mp = auto ; then
	if test $ac_cv_sizeof_long -gt 4 ; then
		ol_with_mp=long
		AC_DEFINE(USE_MP_LONG,1,[define to use 'long' for MP])
	elif test $ol_with_mp = long ; then
		AC_MSG_ERROR([long unusable for multiple precision])
	fi
fi
if test $ol_with_mp = bignum || test $ol_with_mp = auto ; then
	AC_CHECK_HEADERS(openssl/bn.h)
	AC_CHECK_HEADERS(openssl/crypto.h)
	if test "$ac_cv_header_openssl_bn_h" = "yes" &&
		test "$ac_cv_header_openssl_crypto_h" = "yes" &&
		test "$ol_with_tls" = "found" ; then
		ol_with_mp=bignum
		AC_DEFINE(USE_MP_BIGNUM,1,[define to use OpenSSL BIGNUM for MP])
	elif test $ol_with_mp = bignum ; then
		AC_MSG_ERROR([bignum not available])
	fi
fi
if test $ol_with_mp = gmp || test $ol_with_mp = auto ; then
	AC_CHECK_HEADERS(gmp.h)
	AC_CHECK_LIB(gmp, __gmpz_add_ui)
	if test $ac_cv_header_gmp_h = yes && test $ac_cv_lib_gmp___gmpz_add_ui = yes ; then
		AC_DEFINE(USE_MP_GMP,1,[define to use GMP for MP])
		ol_with_mp=gmp
	elif test $ol_with_mp = gmp ; then
		AC_MSG_ERROR([gmp not available])
	fi
fi
if test $ol_with_mp = auto ; then
	ol_with_mp=no
fi

dnl ----------------------------------------------------------------
dnl Checks for library functions.
AC_FUNC_MEMCMP

if test $ac_cv_func_memcmp_working = no ; then
	AC_DEFINE(NEED_MEMCMP_REPLACEMENT,1,
		[define if memcmp is not 8-bit clean or is otherwise broken])
fi

AC_FUNC_STRFTIME

OL_FUNC_INET_ATON

dnl Check for NT specific routines
AC_CHECK_FUNC(_spawnlp, AC_DEFINE(HAVE_SPAWNLP,1,[if you have spawnlp()]))

AC_CHECK_FUNC(_snprintf, [ac_cv_func_snprintf=yes
	AC_DEFINE(snprintf, _snprintf, [define to snprintf routine])
])

AC_CHECK_FUNCS(vsnprintf _vsnprintf)

if test $ac_cv_func_vsnprintf = no -a $ac_cv_func__vsnprintf = yes ; then
	ac_cv_func_vsnprintf=yes
	AC_DEFINE(vsnprintf, _vsnprintf, [define to vsnprintf routine])
fi

AC_FUNC_VPRINTF

if test $ac_cv_func_vprintf = yes ; then
	dnl check for vsnprintf
	AC_CHECK_FUNCS(snprintf vsnprintf)
fi

AC_CHECK_FUNCS(			\
	bcopy			\
	clock_gettime	\
	closesocket		\
	chroot			\
	endgrent		\
	endpwent		\
	fcntl			\
	flock			\
	fstat			\
	getdtablesize		\
	geteuid			\
	getgrgid		\
	gethostname		\
	getpassphrase		\
	getpwuid		\
	getpwnam		\
	getspnam		\
	gettimeofday		\
	initgroups		\
	inet_ntoa_b		\
	ioctl			\
	lockf			\
	memcpy			\
	memmove			\
	memrchr			\
	mkstemp			\
	mktemp			\
	pipe			\
	read			\
	recv			\
	recvfrom		\
	setpwfile		\
	setgid			\
	setegid			\
	setsid			\
	setuid			\
	seteuid			\
	signal			\
	strdup			\
	strpbrk			\
	strrchr			\
	strsep			\
	strstr			\
	strtol			\
	strtoul			\
	strtoq			\
	strtouq			\
	strtoll			\
	strtoull		\
	strspn			\
	sysconf			\
	waitpid			\
	wait4			\
	write			\
	send			\
	sendmsg			\
	sendto			\
)

dnl We actually may need to replace more than this.
AC_REPLACE_FUNCS(getopt getpeereid)

if test "$ac_cv_func_getopt" != yes; then
	LIBSRCS="$LIBSRCS getopt.c"
fi

if test "$ac_cv_func_getpeereid" != yes; then
	AC_CHECK_FUNCS( getpeerucred )
	if test "$ac_cv_func_getpeerucred" != yes ; then
		AC_CHECK_MEMBERS([struct msghdr.msg_accrightslen],,,
			[$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])
		if test "$ac_cv_member_struct_msghdr_msg_accrightslen" != yes; then
			AC_CHECK_MEMBERS([struct msghdr.msg_control],,,
				[$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])
		fi
		AC_CHECK_MEMBERS([struct stat.st_fstype, struct stat.st_vfstype])
		if test "$ac_cv_member_struct_stat_st_fstype" = yes; then
			AC_COMPILE_IFELSE([AC_LANG_SOURCE([struct stat st; char *ptr=st.st_fstype;])],
				AC_DEFINE([HAVE_STRUCT_STAT_ST_FSTYPE_CHAR],1,[define to 1 if st_fstype is char *]),
				AC_DEFINE([HAVE_STRUCT_STAT_ST_FSTYPE_INT],1,[define to 1 if st_fstype is int]))
		fi
	fi
	LIBSRCS="$LIBSRCS getpeereid.c"
fi

if test "$ac_cv_func_snprintf" != yes ||
   test "$ac_cv_func_vsnprintf" != yes; then
	if test "$ac_cv_func_snprintf" != yes; then
		AC_DEFINE(snprintf, ber_pvt_snprintf, [define to snprintf routine])
	fi
	if test "$ac_cv_func_vsnprintf" != yes; then
		AC_DEFINE(vsnprintf, ber_pvt_vsnprintf, [define to snprintf routine])
	fi
fi

dnl ----------------------------------------------------------------
dnl Sort out defines

if test "$ol_enable_slapi" != no ; then
	dnl This check is done also if --enable-modules is used;
	dnl it is duplicated here, 'cause it'd be cached anyway
	AC_CHECK_HEADERS(ltdl.h)

	if test $ac_cv_header_ltdl_h != yes ; then
		AC_MSG_ERROR([could not locate <ltdl.h>])
	fi
	AC_CHECK_LIB(ltdl, lt_dlinit, [
		SLAPI_LIBS=-lltdl
		LIBSLAPI=slapi/libslapi.la
		AC_DEFINE(HAVE_LIBLTDL,1,[define if you have libtool -ltdl])
	],[AC_MSG_ERROR([could not locate libtool -lltdl])])

	AC_DEFINE(LDAP_SLAPI,1, [define this to add SLAPI code])
fi

if test "$ol_enable_debug" != no ; then
	if test "$ol_enable_debug" = traditional; then
		AC_DEFINE(OLD_DEBUG,1,
			[define to use the original debug style])
	fi
	AC_DEFINE(LDAP_DEBUG,1,
		[define this to add debugging code])
fi
if test "$ol_enable_syslog" != no ; then
	AC_DEFINE(LDAP_SYSLOG,1,
		[define this to add syslog code])
fi
if test "$ol_enable_referrals" != no ; then
	AC_DEFINE(LDAP_API_FEATURE_X_OPENLDAP_V2_REFERRALS,LDAP_VENDOR_VERSION,
		[define to LDAP VENDOR VERSION])
fi
if test "$ol_enable_local" != no; then
	AC_DEFINE(LDAP_PF_LOCAL,1,[define to support PF_LOCAL])
fi
if test "$ol_link_ipv6" != no; then
	AC_DEFINE(LDAP_PF_INET6,1,[define to support PF_INET6])
fi
if test "$ol_enable_cleartext" != no ; then
	AC_DEFINE(SLAPD_CLEARTEXT,1,[define to support cleartext passwords])
fi
if test "$ol_enable_crypt" != no ; then
	AC_DEFINE(SLAPD_CRYPT,1,[define to support crypt(3) passwords])
fi
if test "$ol_link_spasswd" != no ; then
	AC_DEFINE(SLAPD_SPASSWD,1,[define to support SASL passwords])
fi
if test "$ol_enable_rlookups" != no ; then
	AC_DEFINE(SLAPD_RLOOKUPS,1,[define to support reverse lookups])
fi
if test "$ol_enable_aci" != no ; then
	if test "$ol_enable_aci" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		dnl remove this after moving servers/slapd/aci.c in contrib/slapd-modules/acl
		AC_MSG_ERROR([ACI build as dynamic module not supported (yet)])
	else 
		MFLAG=SLAPD_MOD_STATIC
	fi
	WITH_ACI_ENABLED=$ol_enable_aci
	AC_DEFINE_UNQUOTED(SLAPD_ACI_ENABLED,$MFLAG,[define to support per-object ACIs])
else
	WITH_ACI_ENABLED=no
fi
if test "$ol_enable_dynacl" != no ; then
	AC_DEFINE(SLAP_DYNACL,1,[define to support run-time loadable ACL])
fi

if test "$ol_link_modules" != no ; then
	AC_DEFINE(SLAPD_MODULES,1,[define to support modules])
	BUILD_SLAPD=yes
	SLAPD_MODULES_LDFLAGS="-dlopen self"
fi

AC_DEFINE(SLAPD_MOD_STATIC,1,[statically linked module])
AC_DEFINE(SLAPD_MOD_DYNAMIC,2,[dynamically linked module])

if test "$ol_enable_dnssrv" != no ; then
	BUILD_SLAPD=yes
	BUILD_DNSSRV=$ol_enable_dnssrv
	if test "$ol_enable_dnssrv" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-dnssrv"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-dnssrv"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_DNSSRV,$MFLAG,[define to support DNS SRV backend])
fi

if test "$ol_enable_ldap" != no ; then
	BUILD_SLAPD=yes
	BUILD_LDAP=$ol_enable_ldap
	if test "$ol_enable_ldap" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-ldap"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-ldap"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_LDAP,$MFLAG,[define to support LDAP backend])
fi

if test "$ol_enable_mdb" != no ; then
	BUILD_SLAPD=yes
	BUILD_MDB=$ol_enable_mdb
	if test "$ol_enable_mdb" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-mdb"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-mdb"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_MDB,$MFLAG,[define to support MDB backend])
fi

if test "$ol_enable_meta" != no ; then
	BUILD_SLAPD=yes
	BUILD_META=$ol_enable_meta
	if test "$ol_enable_meta" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-meta"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-meta"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_META,$MFLAG,[define to support LDAP Metadirectory backend])
fi

if test "$ol_enable_asyncmeta" != no ; then
	BUILD_SLAPD=yes
	BUILD_ASYNCMETA=$ol_enable_asyncmeta
	if test "$ol_enable_asyncmeta" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-asyncmeta"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-asyncmeta"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_ASYNCMETA,$MFLAG,[define to support LDAP Async Metadirectory backend])
fi

if test "$ol_enable_ndb" != no ; then
	BUILD_SLAPD=yes
	BUILD_NDB=$ol_enable_ndb
	if test "$ol_enable_ndb" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-ndb"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-ndb"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_NDB,$MFLAG,[define to support NDB backend])
fi

if test "$ol_enable_null" != no ; then
	BUILD_SLAPD=yes
	BUILD_NULL=$ol_enable_null
	if test "$ol_enable_null" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-null"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-null"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_NULL,$MFLAG,[define to support NULL backend])
fi

if test "$ol_enable_passwd" != no ; then
	BUILD_SLAPD=yes
	BUILD_PASSWD=$ol_enable_passwd
	if test "$ol_enable_passwd" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-passwd"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-passwd"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_PASSWD,$MFLAG,[define to support PASSWD backend])
fi

if test "$ol_link_perl" != no ; then
	BUILD_SLAPD=yes
	BUILD_PERL=$ol_enable_perl
	if test "$ol_enable_perl" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-perl"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-perl"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_PERL,$MFLAG,[define to support PERL backend])
fi

if test "$ol_enable_relay" != no ; then
	BUILD_SLAPD=yes
	BUILD_RELAY=$ol_enable_relay
	if test "$ol_enable_relay" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-relay"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-relay"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_RELAY,$MFLAG,[define to support relay backend])
fi

if test "$ol_enable_sock" != no ; then
	BUILD_SLAPD=yes
	BUILD_SOCK=$ol_enable_sock
	if test "$ol_enable_sock" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-sock"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-sock"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_SOCK,$MFLAG,[define to support SOCK backend])
fi

if test "$ol_link_sql" != no ; then
	BUILD_SLAPD=yes
	BUILD_SQL=$ol_enable_sql
	if test "$ol_enable_sql" = mod; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-sql"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-sql"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_SQL,$MFLAG,[define to support SQL backend])
fi

if test "$ol_link_wt" != no ; then
	BUILD_SLAPD=yes
	BUILD_WT=$ol_enable_wt
	if test "$ol_enable_wt" = mod; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS back-wt"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS back-wt"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_WT,$MFLAG,[define to support WiredTiger backend])
fi

if test "$ol_enable_accesslog" != no ; then
	BUILD_ACCESSLOG=$ol_enable_accesslog
	if test "$ol_enable_accesslog" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS accesslog.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS accesslog.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_ACCESSLOG,$MFLAG,[define for In-Directory Access Logging overlay])
fi

if test "$ol_enable_auditlog" != no ; then
	BUILD_AUDITLOG=$ol_enable_auditlog
	if test "$ol_enable_auditlog" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS auditlog.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS auditlog.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_AUDITLOG,$MFLAG,[define for Audit Logging overlay])
fi

if test "$ol_enable_autoca" != no ; then
	if test $ol_with_tls != openssl ; then
		AC_MSG_ERROR([--enable-autoca=$ol_enable_autoca requires --with-tls=openssl])
	fi

	BUILD_AUTOCA=$ol_enable_autoca
	if test "$ol_enable_autoca" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS autoca.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS autoca.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_AUTOCA,$MFLAG,[define for Automatic Certificate Authority overlay])
fi

if test "$ol_enable_collect" != no ; then
        BUILD_COLLECT=$ol_enable_collect
        if test "$ol_enable_collect" = mod ; then
                MFLAG=SLAPD_MOD_DYNAMIC
                SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS collect.la"
        else
                MFLAG=SLAPD_MOD_STATIC
                SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS collect.o"
        fi
        AC_DEFINE_UNQUOTED(SLAPD_OVER_COLLECT,$MFLAG,[define for Collect overlay])
fi

if test "$ol_enable_constraint" != no ; then
	BUILD_CONSTRAINT=$ol_enable_constraint
	if test "$ol_enable_constraint" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS constraint.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS constraint.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_CONSTRAINT,$MFLAG,[define for Attribute Constraint overlay])
fi

if test "$ol_enable_dds" != no ; then
	BUILD_DDS=$ol_enable_dds
	if test "$ol_enable_dds" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS dds.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS dds.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DDS,$MFLAG,[define for Dynamic Directory Services overlay])
fi

if test "$ol_enable_deref" != no ; then
	BUILD_DEREF=$ol_enable_deref
	if test "$ol_enable_deref" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS deref.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS deref.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DEREF,$MFLAG,[define for Dynamic Directory Services overlay])
fi

if test "$ol_enable_dyngroup" != no ; then
	BUILD_DYNGROUP=$ol_enable_dyngroup
	if test "$ol_enable_dyngroup" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS dyngroup.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS dyngroup.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DYNGROUP,$MFLAG,[define for Dynamic Group overlay])
fi

if test "$ol_enable_dynlist" != no ; then
	BUILD_DYNLIST=$ol_enable_dynlist
	if test "$ol_enable_dynlist" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS dynlist.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS dynlist.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DYNLIST,$MFLAG,[define for Dynamic List overlay])
fi

if test "$ol_enable_homedir" != no ; then
	BUILD_HOMEDIR=$ol_enable_homedir
	if test "$ol_enable_homedir" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS homedir.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS homedir.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_HOMEDIR,$MFLAG,[define for Home Directory Management overlay])
fi

if test "$ol_enable_memberof" != no ; then
	BUILD_MEMBEROF=$ol_enable_memberof
	if test "$ol_enable_memberof" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS memberof.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS memberof.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_MEMBEROF,$MFLAG,[define for Reverse Group Membership overlay])
fi

if test "$ol_enable_otp" != no ; then
	if test $ol_with_tls = no ; then
		AC_MSG_ERROR([--enable-otp=$ol_enable_otp requires --with-tls])
	fi

	BUILD_OTP=$ol_enable_otp
	if test "$ol_enable_otp" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS otp.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS otp.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_OTP,$MFLAG,[define for OTP 2-factor Authentication overlay])
fi

if test "$ol_enable_ppolicy" != no ; then
	BUILD_PPOLICY=$ol_enable_ppolicy
	if test "$ol_enable_ppolicy" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS ppolicy.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS ppolicy.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_PPOLICY,$MFLAG,[define for Password Policy overlay])
fi

if test "$ol_enable_proxycache" != no ; then
	BUILD_PROXYCACHE=$ol_enable_proxycache
	if test "$ol_enable_proxycache" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS pcache.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS pcache.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_PROXYCACHE,$MFLAG,[define for Proxy Cache overlay])
fi

if test "$ol_enable_refint" != no ; then
	BUILD_REFINT=$ol_enable_refint
	if test "$ol_enable_refint" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS refint.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS refint.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_REFINT,$MFLAG,[define for Referential Integrity overlay])
fi

if test "$ol_enable_remoteauth" != no ; then
	BUILD_REMOTEAUTH=$ol_enable_remoteauth
	if test "$ol_enable_remoteauth" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS remoteauth.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS remoteauth.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_REMOTEAUTH,$MFLAG,[define for Deferred Authentication overlay])
fi

if test "$ol_enable_retcode" != no ; then
	BUILD_RETCODE=$ol_enable_retcode
	if test "$ol_enable_retcode" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS retcode.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS retcode.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_RETCODE,$MFLAG,[define for Return Code overlay])
fi

if test "$ol_enable_rwm" != no ; then
	BUILD_RWM=$ol_enable_rwm
	if test "$ol_enable_rwm" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS rwm.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS rwm_x.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_RWM,$MFLAG,[define for Rewrite/Remap overlay])
fi

if test "$ol_enable_seqmod" != no ; then
	BUILD_SEQMOD=$ol_enable_seqmod
	if test "$ol_enable_seqmod" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS seqmod.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS seqmod.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_SEQMOD,$MFLAG,[define for Sequential Modify overlay])
fi

if test "$ol_enable_sssvlv" != no ; then
	BUILD_SSSVLV=$ol_enable_sssvlv
	if test "$ol_enable_sssvlv" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS sssvlv.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS sssvlv.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_SSSVLV,$MFLAG,[define for ServerSideSort/VLV overlay])
fi

if test "$ol_enable_syncprov" != no ; then
	BUILD_SYNCPROV=$ol_enable_syncprov
	if test "$ol_enable_syncprov" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS syncprov.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS syncprov.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_SYNCPROV,$MFLAG,[define for Syncrepl Provider overlay])
fi

if test "$ol_enable_translucent" != no ; then
	BUILD_TRANSLUCENT=$ol_enable_translucent
	if test "$ol_enable_translucent" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS translucent.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS translucent.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_TRANSLUCENT,$MFLAG,[define for Translucent Proxy overlay])
fi

if test "$ol_enable_unique" != no ; then
	BUILD_UNIQUE=$ol_enable_unique
	if test "$ol_enable_unique" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS unique.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS unique.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_UNIQUE,$MFLAG,[define for Attribute Uniqueness overlay])
fi

if test "$ol_enable_valsort" != no ; then
	BUILD_VALSORT=$ol_enable_valsort
	if test "$ol_enable_valsort" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS valsort.la"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS valsort.o"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_VALSORT,$MFLAG,[define for Value Sorting overlay])
fi

ol_link_argon2=no
if test "$ol_enable_argon2" = "yes" ; then
	if test $ol_with_argon2 = libargon2 || test $ol_with_argon2 = auto; then
		AC_CHECK_HEADERS(argon2.h)
		if test $ac_cv_header_argon2_h = yes ; then
			AC_CHECK_LIB(argon2, argon2i_hash_encoded,
				[have_argon2=yes], [have_argon2=no],
				[-largon2])
		fi
		if test "$have_argon2" = "yes" ; then
			ol_with_argon2=libargon2
			ol_link_argon2=yes
			AC_DEFINE(HAVE_LIBARGON2, 1,
				[define if you have libargon2])
			ARGON2_LIBS="-largon2"
		fi
	fi
	if test $ol_with_argon2 = libsodium || test $ol_with_argon2 = auto; then
		AC_CHECK_HEADERS(sodium.h)
		if test $ac_cv_header_sodium_h = yes ; then
			AC_CHECK_LIB(sodium, crypto_pwhash_str_alg,
				[have_argon2=yes], [have_argon2=no],
				[-lsodium])
		fi
		if test "$have_argon2" = "yes" ; then
			ol_with_argon2=libsodium
			ol_link_argon2=yes
			AC_DEFINE(HAVE_LIBSODIUM, 1,
				[define if you have libsodium])
			ARGON2_LIBS="-lsodium"
		fi
	fi

	if test "$ol_link_argon2" = no ; then
		AC_MSG_ERROR([--enable_argon2=$ol_enable_argon2 requires --with-argon2])
	fi

	BUILD_PW_ARGON2=$ol_enable_argon2
	if test "$ol_enable_argon2" = "yes" ; then
		SLAPD_DYNAMIC_PWMODS="$SLAPD_DYNAMIC_PWDMODS argon2.la"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_PWMOD_PW_ARGON2,$SLAPD_MOD_DYNAMIC,[define for Argon2 Password hashing module])
fi

if test "$ol_enable_balancer" != no \
		-a "$ol_with_threads" != no \
		-a "$have_libevent" = yes ; then
	if test "$ol_enable_balancer" = mod; then
		BALANCER_INCLUDE=Makefile.module
		BUILD_BALANCER=mod
	else
		BALANCER_INCLUDE=Makefile.server
		BUILD_BALANCER=yes
	fi
fi

if test "$ol_enable_slapi" != no ; then
	AC_DEFINE(ENABLE_SLAPI,1,[define to enable slapi library])
	BUILD_SLAPI=yes
	SLAPD_SLAPI_DEPEND=libslapi.a
fi

OL_VERSIONED_SYMBOLS=""
if test $ol_enable_versioning != no; then
	LDVS=`$LD --help < /dev/null 2>/dev/null | grep gnu-version-script`
	if test -z "$LDVS"; then
		LDVS=`$LD --help < /dev/null 2>/dev/null | grep version-script`
		if test -z "$LDVS"; then
			if test $ol_enable_versioning = "yes" ; then
				AC_MSG_ERROR([Library symbol versioning requested but not supported])
			fi
		else
			OL_VERSIONED_SYMBOLS="-Wl,--version-script="
		fi
	else
		OL_VERSIONED_SYMBOLS="-z gnu-version-script="
	fi
fi

dnl ----------------------------------------------------------------

dnl
dnl For Windows build, we don't want to include -dlopen flags.
dnl They hurt more than they help.
dnl

if test "$ac_cv_mingw32" = yes -o $ol_cv_msvc = yes ; then
	PLAT=NT
	SLAPD_MODULES_LDFLAGS=
else
	PLAT=UNIX
fi

AC_SUBST(LIBSRCS)
AC_SUBST(PLAT)
AC_SUBST(WITH_SASL)
AC_SUBST(WITH_TLS)
AC_SUBST(WITH_MODULES_ENABLED)
AC_SUBST(WITH_ACI_ENABLED)
AC_SUBST(WITH_SYSTEMD)
AC_SUBST(BUILD_THREAD)
AC_SUBST(BUILD_LIBS_DYNAMIC)
AC_SUBST(OL_VERSIONED_SYMBOLS)

AC_SUBST(BUILD_SLAPD)
dnl slapi
  AC_SUBST(BUILD_SLAPI)
  AC_SUBST(SLAPD_SLAPI_DEPEND)
dnl backends
  AC_SUBST(BUILD_DNSSRV)
  AC_SUBST(BUILD_LDAP)
  AC_SUBST(BUILD_MDB)
  AC_SUBST(BUILD_META)
  AC_SUBST(BUILD_ASYNCMETA)
  AC_SUBST(BUILD_NDB)
  AC_SUBST(BUILD_NULL)
  AC_SUBST(BUILD_PASSWD)
  AC_SUBST(BUILD_RELAY)
  AC_SUBST(BUILD_PERL)
  AC_SUBST(BUILD_SHELL)
  AC_SUBST(BUILD_SOCK)
  AC_SUBST(BUILD_SQL)
  AC_SUBST(BUILD_WT)
dnl overlays
  AC_SUBST(BUILD_ACCESSLOG)
  AC_SUBST(BUILD_AUDITLOG)
  AC_SUBST(BUILD_AUTOCA)
  AC_SUBST(BUILD_COLLECT)
  AC_SUBST(BUILD_CONSTRAINT)
  AC_SUBST(BUILD_DDS)
  AC_SUBST(BUILD_DENYOP)
  AC_SUBST(BUILD_DEREF)
  AC_SUBST(BUILD_DYNGROUP)
  AC_SUBST(BUILD_DYNLIST)
  AC_SUBST(BUILD_LASTMOD)
  AC_SUBST(BUILD_HOMEDIR)
  AC_SUBST(BUILD_MEMBEROF)
  AC_SUBST(BUILD_OTP)
  AC_SUBST(BUILD_PPOLICY)
  AC_SUBST(BUILD_PROXYCACHE)
  AC_SUBST(BUILD_REFINT)
  AC_SUBST(BUILD_REMOTEAUTH)
  AC_SUBST(BUILD_RETCODE)
  AC_SUBST(BUILD_RWM)
  AC_SUBST(BUILD_SEQMOD)
  AC_SUBST(BUILD_SSSVLV)
  AC_SUBST(BUILD_SYNCPROV)
  AC_SUBST(BUILD_TRANSLUCENT)
  AC_SUBST(BUILD_UNIQUE)
  AC_SUBST(BUILD_VALSORT)
  AC_SUBST(BUILD_BALANCER)
dnl pwmods
  AC_SUBST(BUILD_PW_ARGON2)

AC_SUBST(LDAP_LIBS)
AC_SUBST(CLIENT_LIBS)
AC_SUBST(SLAPD_LIBS)
AC_SUBST(BALANCER_LIBS)
AC_SUBST(SLAPD_NDB_LIBS)
AC_SUBST(SLAPD_NDB_INCS)
AC_SUBST(LTHREAD_LIBS)
AC_SUBST(LUTIL_LIBS)
AC_SUBST(LEVENT_LIBS)
AC_SUBST(WRAP_LIBS)

AC_SUBST(SLAPD_MODULES_CPPFLAGS)
AC_SUBST(SLAPD_MODULES_LDFLAGS)

AC_SUBST(SLAPD_NO_STATIC)
AC_SUBST(SLAPD_STATIC_BACKENDS)
AC_SUBST(SLAPD_DYNAMIC_BACKENDS)
AC_SUBST(SLAPD_STATIC_OVERLAYS)
AC_SUBST(SLAPD_DYNAMIC_OVERLAYS)
AC_SUBST(SLAPD_DYNAMIC_PWMODS)

AC_SUBST(PERL_CPPFLAGS)
AC_SUBST(SLAPD_PERL_LDFLAGS)
AC_SUBST(MOD_PERL_LDFLAGS)

AC_SUBST(SASL_LIBS)
AC_SUBST(TLS_LIBS)
AC_SUBST(WITH_TLS_TYPE)
AC_SUBST(MODULES_LIBS)
AC_SUBST(SLAPI_LIBS)
AC_SUBST(LIBSLAPI)
AC_SUBST(AUTH_LIBS)
AC_SUBST(ARGON2_LIBS)
AC_SUBST(SYSTEMD_LIBS)

AC_SUBST(SLAPD_SLP_LIBS)
AC_SUBST(SLAPD_GMP_LIBS)

AC_SUBST(SLAPD_SQL_LDFLAGS)
AC_SUBST(SLAPD_SQL_LIBS)
AC_SUBST(SLAPD_SQL_INCLUDES)

AC_SUBST(WT_CFLAGS)
AC_SUBST(WT_LIBS)

AC_SUBST(BALANCER_INCLUDE)

dnl ----------------------------------------------------------------
dnl final help output
AC_ARG_WITH(xxinstall,[
See INSTALL file for further details.])

dnl ----------------------------------------------------------------
dnl final output
dnl

AC_CONFIG_FILES([Makefile:build/top.mk:Makefile.in:build/dir.mk]
[doc/Makefile:build/top.mk:doc/Makefile.in:build/dir.mk]
[doc/man/Makefile:build/top.mk:doc/man/Makefile.in:build/dir.mk]
[doc/man/man1/Makefile:build/top.mk:doc/man/man1/Makefile.in:build/man.mk]
[doc/man/man3/Makefile:build/top.mk:doc/man/man3/Makefile.in:build/man.mk]
[doc/man/man5/Makefile:build/top.mk:doc/man/man5/Makefile.in:build/man.mk]
[doc/man/man8/Makefile:build/top.mk:doc/man/man8/Makefile.in:build/man.mk]
[clients/Makefile:build/top.mk:clients/Makefile.in:build/dir.mk]
[clients/tools/Makefile:build/top.mk:clients/tools/Makefile.in:build/rules.mk]
[include/Makefile:build/top.mk:include/Makefile.in]
[libraries/Makefile:build/top.mk:libraries/Makefile.in:build/dir.mk]
[libraries/liblber/Makefile:build/top.mk:libraries/liblber/Makefile.in:build/lib.mk:build/lib-shared.mk]
[libraries/liblber/lber.pc]
[libraries/liblber/liblber.vers]
[libraries/libldap/Makefile:build/top.mk:libraries/libldap/Makefile.in:build/lib.mk:build/lib-shared.mk]
[libraries/libldap/ldap.pc]
[libraries/libldap/libldap.vers]
[libraries/liblunicode/Makefile:build/top.mk:libraries/liblunicode/Makefile.in:build/lib.mk:build/lib-static.mk]
[libraries/liblutil/Makefile:build/top.mk:libraries/liblutil/Makefile.in:build/lib.mk:build/lib-static.mk]
[libraries/librewrite/Makefile:build/top.mk:libraries/librewrite/Makefile.in:build/lib.mk:build/lib-static.mk]
[servers/Makefile:build/top.mk:servers/Makefile.in:build/dir.mk]
[servers/slapd/Makefile:build/top.mk:servers/slapd/Makefile.in:build/srv.mk]
[servers/slapd/back-dnssrv/Makefile:build/top.mk:servers/slapd/back-dnssrv/Makefile.in:build/mod.mk]
[servers/slapd/back-ldap/Makefile:build/top.mk:servers/slapd/back-ldap/Makefile.in:build/mod.mk]
[servers/slapd/back-ldif/Makefile:build/top.mk:servers/slapd/back-ldif/Makefile.in:build/mod.mk]
[servers/slapd/back-mdb/Makefile:build/top.mk:servers/slapd/back-mdb/Makefile.in:build/mod.mk]
[servers/slapd/back-meta/Makefile:build/top.mk:servers/slapd/back-meta/Makefile.in:build/mod.mk]
[servers/slapd/back-asyncmeta/Makefile:build/top.mk:servers/slapd/back-asyncmeta/Makefile.in:build/mod.mk]
[servers/slapd/back-monitor/Makefile:build/top.mk:servers/slapd/back-monitor/Makefile.in:build/mod.mk]
[servers/slapd/back-ndb/Makefile:build/top.mk:servers/slapd/back-ndb/Makefile.in:build/mod.mk]
[servers/slapd/back-null/Makefile:build/top.mk:servers/slapd/back-null/Makefile.in:build/mod.mk]
[servers/slapd/back-passwd/Makefile:build/top.mk:servers/slapd/back-passwd/Makefile.in:build/mod.mk]
[servers/slapd/back-perl/Makefile:build/top.mk:servers/slapd/back-perl/Makefile.in:build/mod.mk]
[servers/slapd/back-relay/Makefile:build/top.mk:servers/slapd/back-relay/Makefile.in:build/mod.mk]
[servers/slapd/back-sock/Makefile:build/top.mk:servers/slapd/back-sock/Makefile.in:build/mod.mk]
[servers/slapd/back-sql/Makefile:build/top.mk:servers/slapd/back-sql/Makefile.in:build/mod.mk]
[servers/slapd/back-wt/Makefile:build/top.mk:servers/slapd/back-wt/Makefile.in:build/mod.mk]
[servers/slapd/slapi/Makefile:build/top.mk:servers/slapd/slapi/Makefile.in:build/lib.mk:build/lib-shared.mk]
[servers/slapd/overlays/Makefile:build/top.mk:servers/slapd/overlays/Makefile.in:build/lib.mk]
[servers/slapd/pwmods/Makefile:build/top.mk:servers/slapd/pwmods/Makefile.in:build/lib.mk]
[servers/lloadd/Makefile:build/top.mk:servers/lloadd/Makefile.in]
[servers/lloadd/Makefile.server:servers/lloadd/Makefile_server.in:build/srv.mk]
[servers/lloadd/Makefile.module:servers/lloadd/Makefile_module.in:build/mod.mk]
[tests/Makefile:build/top.mk:tests/Makefile.in:build/dir.mk]
[tests/run]
[tests/progs/Makefile:build/top.mk:tests/progs/Makefile.in:build/rules.mk])

AC_CONFIG_COMMANDS([default],[[
chmod +x tests/run
date > stamp-h
BACKENDSC="servers/slapd/backends.c"
echo "Making $BACKENDSC"
rm -f $BACKENDSC
cat > $BACKENDSC << ENDX
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* This file is automatically generated by configure; please do not edit. */

#include "portable.h"
#include "slap.h"

ENDX
if test "${STATIC_BACKENDS}"; then
	for b in config ${STATIC_BACKENDS}; do
		bb=`echo "${b}" | sed -e 's/back-//'`
		cat >> $BACKENDSC << ENDX
extern BI_init ${bb}_back_initialize;
ENDX
	done

	cat >> $BACKENDSC << ENDX

BackendInfo slap_binfo[] = {
ENDX

	for b in config ${STATIC_BACKENDS}; do
		bb=`echo "${b}" | sed -e 's/back-//'`
		echo "    Add ${bb} ..."
		cat >> $BACKENDSC << ENDX
	{ "${bb}", ${bb}_back_initialize },
ENDX
	done

	cat >> $BACKENDSC << ENDX
	{ NULL, NULL },
};

/* end of generated file */
ENDX
fi
OVERLAYSC="servers/slapd/overlays/statover.c"
echo "Making $OVERLAYSC"
rm -f $OVERLAYSC
cat > $OVERLAYSC << ENDX
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* This file is automatically generated by configure; please do not edit. */

#include "portable.h"
#include "slap.h"

ENDX
if test "${STATIC_OVERLAYS}"; then
	for o in ${STATIC_OVERLAYS}; do
		oo=`echo "${o}" | sed -e 's/.o$//' -e 's/_x$//'`
		cat >> $OVERLAYSC << ENDX
extern OV_init ${oo}_initialize;
ENDX
	done
fi

cat >> $OVERLAYSC << ENDX

OverlayInit slap_oinfo[] = {
ENDX

if test "${STATIC_OVERLAYS}"; then
	for o in ${STATIC_OVERLAYS}; do
		oo=`echo "${o}" | sed -e 's/.o$//' -e 's/_x$//'`
		echo "    Add ${oo} ..."
		cat >> $OVERLAYSC << ENDX
	{ "${oo}", ${oo}_initialize },
ENDX
	done
fi

	cat >> $OVERLAYSC << ENDX
	{ NULL, NULL },
};

/* end of generated file */
ENDX

if test "${ol_cv_mkdep}" = no; then
	echo '(Do not "make depend"; we do not know how to build dependencies)'
else
	echo 'Please run "make depend" to build dependencies'
fi
]],[[
STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS"
STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS"
]])
AC_OUTPUT
