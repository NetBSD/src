/*	$NetBSD: version.h,v 1.4 2000/07/15 17:02:17 thorpej Exp $	*/

#ifdef __KRB5_VERSION
#define	heimdal_long_version	__heimdal_long_version
#define	heimdal_version		__heimdal_version
#define	__NO_KRB4_VERSION
#endif

#ifdef __KRB4_VERSION
#define	krb4_long_version	__krb4_long_version
#define	krb4_version		__krb4_version
#define	__NO_KRB5_VERSION
#endif

#ifndef __NO_KRB5_VERSION
const char *heimdal_long_version = "@(#)$Version: heimdal-0.2t (NetBSD) $";
const char *heimdal_version = "heimdal-0.2t";
#endif

#ifndef __NO_KRB4_VERSION
const char *krb4_long_version = "@(#)$Version: krb4-1.0.1 (NetBSD) $";
const char *krb4_version = "krb4-1.0.1";
#endif
