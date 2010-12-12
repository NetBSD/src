/*	$NetBSD: syslog.h,v 1.1.1.3 2010/12/12 15:21:27 adam Exp $	*/

/* Generic syslog.h */
/* OpenLDAP: pkg/ldap/include/ac/syslog.h,v 1.17.2.5 2010/04/13 20:22:52 kurt Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2010 The OpenLDAP Foundation.
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

#ifndef _AC_SYSLOG_H_
#define _AC_SYSLOG_H_

#if defined( HAVE_SYSLOG_H )
#include <syslog.h>
#elif defined ( HAVE_SYS_SYSLOG_H )
#include <sys/syslog.h>
#endif

#if defined( LOG_NDELAY ) && defined( LOG_NOWAIT )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NDELAY | LOG_NOWAIT )
#elif defined( LOG_NDELAY )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NDELAY )
#elif defined( LOG_NOWAIT )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NOWAIT )
#elif defined( LOG_PID )
#	define OPENLOG_OPTIONS ( LOG_PID )
#else
#   define OPENLOG_OPTIONS ( 0 )
#endif

#endif /* _AC_SYSLOG_H_ */
