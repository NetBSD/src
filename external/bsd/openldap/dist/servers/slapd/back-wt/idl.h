/*	$NetBSD: idl.h,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

/* OpenLDAP WiredTiger backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2021 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS:
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#ifndef _WI_IDL_H_
#define _WT_IDL_H_

/* IDL sizes - likely should be even bigger
 *   limiting factors: sizeof(ID), thread stack size
 */
#define	WT_IDL_LOGN	16	/* DB_SIZE is 2^16, UM_SIZE is 2^17 */
#define WT_IDL_DB_SIZE		(1<<WT_IDL_LOGN)
#define WT_IDL_UM_SIZE		(1<<(WT_IDL_LOGN+1))
#define WT_IDL_UM_SIZEOF	(WT_IDL_UM_SIZE * sizeof(ID))

#define WT_IDL_DB_MAX		(WT_IDL_DB_SIZE-1)

#define WT_IDL_UM_MAX		(WT_IDL_UM_SIZE-1)

#define WT_IDL_IS_RANGE(ids)	((ids)[0] == NOID)
#define WT_IDL_RANGE_SIZE		(3)
#define WT_IDL_RANGE_SIZEOF	(WT_IDL_RANGE_SIZE * sizeof(ID))
#define WT_IDL_SIZEOF(ids)		((WT_IDL_IS_RANGE(ids) \
	? WT_IDL_RANGE_SIZE : ((ids)[0]+1)) * sizeof(ID))

#define WT_IDL_RANGE_FIRST(ids)	((ids)[1])
#define WT_IDL_RANGE_LAST(ids)		((ids)[2])

#define WT_IDL_RANGE( ids, f, l ) \
	do { \
		(ids)[0] = NOID; \
		(ids)[1] = (f);  \
		(ids)[2] = (l);  \
	} while(0)

#define WT_IDL_ZERO(ids) \
	do { \
		(ids)[0] = 0; \
		(ids)[1] = 0; \
		(ids)[2] = 0; \
	} while(0)

#define WT_IDL_IS_ZERO(ids) ( (ids)[0] == 0 )
#define WT_IDL_IS_ALL( range, ids ) ( (ids)[0] == NOID \
	&& (ids)[1] <= (range)[1] && (range)[2] <= (ids)[2] )

#define WT_IDL_CPY( dst, src ) (AC_MEMCPY( dst, src, WT_IDL_SIZEOF( src ) ))

#define WT_IDL_ID( wi, ids, id ) WT_IDL_RANGE( ids, id, ((wi)->wi_lastid) )
#define WT_IDL_ALL( wi, ids ) WT_IDL_RANGE( ids, 1, ((wi)->wi_lastid) )

#define WT_IDL_FIRST( ids )	( (ids)[1] )
#define WT_IDL_LLAST( ids )	( (ids)[(ids)[0]] )
#define WT_IDL_LAST( ids )		( WT_IDL_IS_RANGE(ids) \
	? (ids)[2] : (ids)[(ids)[0]] )

#define WT_IDL_N( ids )		( WT_IDL_IS_RANGE(ids) \
	? ((ids)[2]-(ids)[1])+1 : (ids)[0] )

LDAP_BEGIN_DECL
LDAP_END_DECL

#endif
