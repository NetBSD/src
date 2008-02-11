/* csn.c - Change Sequence Number routines */
/* $OpenLDAP: pkg/ldap/libraries/liblutil/csn.c,v 1.14.2.3 2008/02/11 23:26:42 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2008 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
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
/* Portions Copyright 2000, John E. Schimmel, All rights reserved.
 * This software is not subject to any license of Mirapoint, Inc.
 *
 * This is free software; you can redistribute and use it
 * under the same terms as OpenLDAP itself.
 */
/* This work was developed by John E. Schimmel and adapted for
 * inclusion in OpenLDAP Software by Kurt D. Zeilenga.
 */

/* This file contains routines to generate a change sequence number.
 * Every add, delete, and modification is given a unique identifier
 * for use in resolving conflicts during replication operations.
 *
 * These routines are (loosly) based upon draft-ietf-ldup-model-03.txt,
 * A WORK IN PROGRESS.  The format will likely change.
 *
 * The format of a CSN string is: yyyymmddhhmmssz#s#r#c
 * where s is a counter of operations within a timeslice, r is
 * the replica id (normally zero), and c is a counter of
 * modifications within this operation.  s, r, and c are
 * represented in hex and zero padded to lengths of 6, 3, and
 * 6, respectively. (In previous implementations r was only 2 digits.)
 *
 * Calls to this routine MUST be serialized with other calls
 * to gmtime().
 */
#include "portable.h"

#include <stdio.h>
#include <ac/time.h>

#include <lutil.h>

/* Must be mutex-protected, because lutil_gettime needs mutex protection */
size_t
lutil_csnstr(char *buf, size_t len, unsigned int replica, unsigned int mod)
{
	struct lutil_tm tm;
	int n;

	lutil_gettime( &tm );

	n = snprintf( buf, len,
		"%4d%02d%02d%02d%02d%02d.%06dZ#%06x#%03x#%06x",
	    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
	    tm.tm_min, tm.tm_sec, tm.tm_usec, tm.tm_usub, replica, mod );

	if( n < 0 ) return 0;
	return ( (size_t) n < len ) ? n : 0;
}

#ifdef TEST
int
main(int argc, char **argv)
{
	char buf[ LDAP_LUTIL_CSNSTR_BUFSIZE ];

	if ( ! lutil_csnstr( buf, (size_t) 10, 0, 0 ) ) {
		fprintf(stderr, "failed lutil_csnstr\n");
	}
	if ( ! lutil_csnstr( buf, sizeof(buf), 0, 0 ) ) {
		fprintf(stderr, "failed lutil_csnstr\n");
	}
}
#endif
