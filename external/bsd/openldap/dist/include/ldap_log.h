/*	$NetBSD: ldap_log.h,v 1.1.1.3.24.1 2014/08/10 07:09:46 tls Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 * 
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1990 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef LDAP_LOG_H
#define LDAP_LOG_H

#include <stdio.h>
#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

/*
 * debug reporting levels.
 *
 * They start with the syslog levels, and
 * go down in importance.  The normal
 * debugging levels begin with LDAP_LEVEL_ENTRY
 *
 */

/*
 * The "OLD_DEBUG" means that all logging occurs at LOG_DEBUG
 */

#ifdef OLD_DEBUG
/* original behavior: all logging occurs at the same severity level */
#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
#define LDAP_LEVEL_EMERG	ldap_syslog_level
#define LDAP_LEVEL_ALERT	ldap_syslog_level
#define LDAP_LEVEL_CRIT		ldap_syslog_level
#define LDAP_LEVEL_ERR		ldap_syslog_level
#define LDAP_LEVEL_WARNING	ldap_syslog_level
#define LDAP_LEVEL_NOTICE	ldap_syslog_level
#define LDAP_LEVEL_INFO		ldap_syslog_level
#define LDAP_LEVEL_DEBUG	ldap_syslog_level
#else /* !LDAP_DEBUG || !LDAP_SYSLOG */
#define LDAP_LEVEL_EMERG	(7)
#define LDAP_LEVEL_ALERT	(7)
#define LDAP_LEVEL_CRIT		(7)
#define LDAP_LEVEL_ERR		(7)
#define LDAP_LEVEL_WARNING	(7)
#define LDAP_LEVEL_NOTICE	(7)
#define LDAP_LEVEL_INFO		(7)
#define LDAP_LEVEL_DEBUG	(7)
#endif /* !LDAP_DEBUG || !LDAP_SYSLOG */

#else /* ! OLD_DEBUG */
/* map syslog onto LDAP severity levels */
#ifdef LOG_DEBUG
#define LDAP_LEVEL_EMERG	LOG_EMERG
#define LDAP_LEVEL_ALERT	LOG_ALERT
#define LDAP_LEVEL_CRIT		LOG_CRIT
#define LDAP_LEVEL_ERR		LOG_ERR
#define LDAP_LEVEL_WARNING	LOG_WARNING
#define LDAP_LEVEL_NOTICE	LOG_NOTICE
#define LDAP_LEVEL_INFO		LOG_INFO
#define LDAP_LEVEL_DEBUG	LOG_DEBUG
#else /* ! LOG_DEBUG */
#define LDAP_LEVEL_EMERG	(0)
#define LDAP_LEVEL_ALERT	(1)
#define LDAP_LEVEL_CRIT		(2)
#define LDAP_LEVEL_ERR		(3)
#define LDAP_LEVEL_WARNING	(4)
#define LDAP_LEVEL_NOTICE	(5)
#define LDAP_LEVEL_INFO		(6)
#define LDAP_LEVEL_DEBUG	(7)
#endif /* ! LOG_DEBUG */
#endif /* ! OLD_DEBUG */
#if 0
/* in case we need to reuse the unused bits of severity */
#define	LDAP_LEVEL_MASK(s)	((s) & 0x7)
#else
#define	LDAP_LEVEL_MASK(s)	(s)
#endif

/* (yet) unused */
#define LDAP_LEVEL_ENTRY	(0x08)	/* log function entry points */
#define LDAP_LEVEL_ARGS		(0x10)	/* log function call parameters */
#define LDAP_LEVEL_RESULTS	(0x20)	/* Log function results */
#define LDAP_LEVEL_DETAIL1	(0x40)	/* log level 1 function operational details */
#define LDAP_LEVEL_DETAIL2	(0x80)	/* Log level 2 function operational details */
/* end of (yet) unused */

/* original subsystem selection mechanism */
#define LDAP_DEBUG_TRACE	0x0001
#define LDAP_DEBUG_PACKETS	0x0002
#define LDAP_DEBUG_ARGS		0x0004
#define LDAP_DEBUG_CONNS	0x0008
#define LDAP_DEBUG_BER		0x0010
#define LDAP_DEBUG_FILTER	0x0020
#define LDAP_DEBUG_CONFIG	0x0040
#define LDAP_DEBUG_ACL		0x0080
#define LDAP_DEBUG_STATS	0x0100
#define LDAP_DEBUG_STATS2	0x0200
#define LDAP_DEBUG_SHELL	0x0400
#define LDAP_DEBUG_PARSE	0x0800
#if 0 /* no longer used (nor supported) */
#define LDAP_DEBUG_CACHE	0x1000
#define LDAP_DEBUG_INDEX	0x2000
#endif
#define LDAP_DEBUG_SYNC		0x4000

#define LDAP_DEBUG_NONE		0x8000
#define LDAP_DEBUG_ANY		(-1)

/* debugging stuff */
#ifdef LDAP_DEBUG
    /*
     * This is a bogus extern declaration for the compiler. No need to ensure
     * a 'proper' dllimport.
     */
#ifndef ldap_debug
extern int	ldap_debug;
#endif /* !ldap_debug */

#ifdef LDAP_SYSLOG
extern int	ldap_syslog;
extern int	ldap_syslog_level;

#ifdef HAVE_EBCDIC
#define syslog	eb_syslog
extern void eb_syslog(int pri, const char *fmt, ...);
#endif /* HAVE_EBCDIC */

#endif /* LDAP_SYSLOG */

/* this doesn't below as part of ldap.h */
#ifdef LDAP_SYSLOG
#define Log0( level, severity, fmt )	\
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt) ); \
		if ( ldap_syslog & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), (fmt) ); \
	} while ( 0 )
#define Log1( level, severity, fmt, arg1 )	\
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt), (arg1) ); \
		if ( ldap_syslog & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), (fmt), (arg1) ); \
	} while ( 0 )
#define Log2( level, severity, fmt, arg1, arg2 ) \
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2) ); \
		if ( ldap_syslog & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), (fmt), (arg1), (arg2) ); \
	} while ( 0 )
#define Log3( level, severity, fmt, arg1, arg2, arg3 ) \
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2), (arg3) ); \
		if ( ldap_syslog & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), (fmt), (arg1), (arg2), (arg3) ); \
	} while ( 0 )
#define Log4( level, severity, fmt, arg1, arg2, arg3, arg4 ) \
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2), (arg3), (arg4) ); \
		if ( ldap_syslog & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), (fmt), (arg1), (arg2), (arg3), (arg4) ); \
	} while ( 0 )
#define Log5( level, severity, fmt, arg1, arg2, arg3, arg4, arg5 ) \
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2), (arg3), (arg4), (arg5) ); \
		if ( ldap_syslog & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), (fmt), (arg1), (arg2), (arg3), (arg4), (arg5) ); \
	} while ( 0 )
#define Debug( level, fmt, arg1, arg2, arg3 )	\
	Log3( (level), ldap_syslog_level, (fmt), (arg1), (arg2), (arg3) )
#define LogTest(level) ( ( ldap_debug | ldap_syslog ) & (level) )

#else /* ! LDAP_SYSLOG */
#define Log0( level, severity, fmt ) \
	do { \
		if ( ldap_debug & (level) ) \
	    		lutil_debug( ldap_debug, (level), (fmt) ); \
	} while ( 0 )
#define Log1( level, severity, fmt, arg1 ) \
	do { \
		if ( ldap_debug & (level) ) \
	    		lutil_debug( ldap_debug, (level), (fmt), (arg1) ); \
	} while ( 0 )
#define Log2( level, severity, fmt, arg1, arg2 ) \
	do { \
		if ( ldap_debug & (level) ) \
	    		lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2) ); \
	} while ( 0 )
#define Log3( level, severity, fmt, arg1, arg2, arg3 ) \
	do { \
		if ( ldap_debug & (level) ) \
	    		lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2), (arg3) ); \
	} while ( 0 )
#define Log4( level, severity, fmt, arg1, arg2, arg3, arg4 ) \
	do { \
		if ( ldap_debug & (level) ) \
	    		lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2), (arg3), (arg4) ); \
	} while ( 0 )
#define Log5( level, severity, fmt, arg1, arg2, arg3, arg4, arg5 ) \
	do { \
		if ( ldap_debug & (level) ) \
	    		lutil_debug( ldap_debug, (level), (fmt), (arg1), (arg2), (arg3), (arg4), (arg5) ); \
	} while ( 0 )
#define Debug( level, fmt, arg1, arg2, arg3 ) \
		Log3( (level), 0, (fmt), (arg1), (arg2), (arg3) )
#define LogTest(level) ( ldap_debug & (level) )
#endif /* ! LDAP_SYSLOG */
#else /* ! LDAP_DEBUG */
/* TODO: in case LDAP_DEBUG is undefined, make sure logs with appropriate
 * severity gets thru anyway */
#define Log0( level, severity, fmt ) ((void)0)
#define Log1( level, severity, fmt, arg1 ) ((void)0)
#define Log2( level, severity, fmt, arg1, arg2 ) ((void)0)
#define Log3( level, severity, fmt, arg1, arg2, arg3 ) ((void)0)
#define Log4( level, severity, fmt, arg1, arg2, arg3, arg4 ) ((void)0)
#define Log5( level, severity, fmt, arg1, arg2, arg3, arg4, arg5 ) ((void)0)
#define Debug( level, fmt, arg1, arg2, arg3 ) ((void)0)
#define LogTest(level) ( 0 )
#endif /* ! LDAP_DEBUG */

/* Actually now in liblber/debug.c */
LDAP_LUTIL_F(int) lutil_debug_file LDAP_P(( FILE *file ));

LDAP_LUTIL_F(void) lutil_debug LDAP_P((
	int debug, int level,
	const char* fmt, ... )) LDAP_GCCATTR((format(printf, 3, 4)));

#ifdef LDAP_DEFINE_LDAP_DEBUG
/* This struct matches the head of ldapoptions in <ldap-int.h> */
struct ldapoptions_prefix {
	short	ldo_valid;
	int		ldo_debug;
};
#define ldap_debug \
	(*(int *) ((char *)&ldap_int_global_options \
		 + offsetof(struct ldapoptions_prefix, ldo_debug)))

struct ldapoptions;
LDAP_V ( struct ldapoptions ) ldap_int_global_options;
#endif /* LDAP_DEFINE_LDAP_DEBUG */

LDAP_END_DECL

#endif /* LDAP_LOG_H */
