/*	$NetBSD: version.h,v 1.2.2.5 2003/03/31 23:45:35 itojun Exp $	*/

#if defined(__KRB5_VERSION) && !defined(__NO_KRB4_VERSION)
#define	heimdal_long_version	__heimdal_long_version
#define	heimdal_version		__heimdal_version
#define	__NO_KRB4_VERSION
#endif

#if defined(__KRB4_VERSION) && !defined(__NO_KRB5_VERSION)
#define	krb4_long_version	__krb4_long_version
#define	krb4_version		__krb4_version
#define	__NO_KRB5_VERSION
#endif

#ifndef __NO_KRB5_VERSION
const char *heimdal_long_version = "@(#)$Version: heimdal-0.3e-nb2 (NetBSD) $";
const char *heimdal_version = "heimdal-0.3e-nb2";
#endif

#ifndef __NO_KRB4_VERSION
const char *krb4_long_version = "@(#)$Version: krb4-1.0.1 (NetBSD) $";
const char *krb4_version = "krb4-1.0.1";
#endif
