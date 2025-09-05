/*	$NetBSD: time.h,v 1.4 2025/09/05 21:16:14 christos Exp $	*/

/* Generic time.h */
/* $OpenLDAP$ */
/*
 * Copyright 1998-2024 The OpenLDAP Foundation, Redwood City, California, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted only
 * as authorized by the OpenLDAP Public License.  A copy of this
 * license is available at http://www.OpenLDAP.org/license.html or
 * in file LICENSE in the top-level directory of the distribution.
 */

#ifndef _AC_TIME_H
#define _AC_TIME_H

#if defined HAVE_SYS_TIME_H
# include <sys/time.h>
# ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h>
# endif
#endif
# include <time.h>

#endif /* _AC_TIME_H */
