/*	$NetBSD: time.h,v 1.1.1.2 2010/03/08 02:14:15 lukem Exp $	*/

/* Generic time.h */
/* OpenLDAP: pkg/ldap/include/ac/time.h,v 1.18.2.4 2009/01/22 00:00:52 kurt Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2009 The OpenLDAP Foundation.
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

#ifndef _AC_TIME_H
#define _AC_TIME_H

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#elif defined(HAVE_SYS_TIME_H)
# include <sys/time.h>
# ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h>
# endif
#else
# include <time.h>
#endif

#endif /* _AC_TIME_H */
