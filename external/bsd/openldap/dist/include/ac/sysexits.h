/*	$NetBSD: sysexits.h,v 1.1.1.6 2018/02/06 01:53:05 christos Exp $	*/

/* Generic sysexits */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2017 The OpenLDAP Foundation.
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

#ifndef _AC_SYSEXITS_H_
#define _AC_SYSEXITS_H_

#ifdef HAVE_SYSEXITS_H
#	include <sysexits.h>
#else
#	include <sysexits-compat.h>
#endif

#endif /* _AC_SYSEXITS_H_ */
