/*	$NetBSD: pathnames.h,v 1.1.1.1.14.1 2012/10/30 18:55:30 yamt Exp $	*/

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Id: pathnames.h,v 1.3 2005/04/27 04:56:34 sra Exp 
 */

#ifndef _PATH_IRS_CONF
#define	_PATH_IRS_CONF "/etc/irs.conf"
#endif

#ifndef _PATH_NETWORKS 
#define _PATH_NETWORKS  "/etc/networks"
#endif

#ifndef _PATH_GROUP
#define _PATH_GROUP "/etc/group"
#endif

#ifndef _PATH_NETGROUP
#define _PATH_NETGROUP "/etc/netgroup"
#endif

#ifndef _PATH_SERVICES 
#define _PATH_SERVICES "/etc/services"
#endif

#ifdef IRS_LCL_SV_DB
#ifndef _PATH_SERVICES_DB
#define	_PATH_SERVICES_DB _PATH_SERVICES ".db"
#endif
#endif

#ifndef _PATH_HESIOD_CONF
#define _PATH_HESIOD_CONF "/etc/hesiod.conf"
#endif

/*! \file */
