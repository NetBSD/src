/*	$NetBSD: extended.c,v 1.2 2020/08/11 13:15:42 christos Exp $	*/

/* extended.c - sock backend extended routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2020 The OpenLDAP Foundation.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: extended.c,v 1.2 2020/08/11 13:15:42 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "back-sock.h"

#include "lutil.h"

int
sock_back_extended( Operation *op, SlapReply *rs )
{
	int			rc;
	struct	sockinfo	*si = (struct sockinfo *) op->o_bd->be_private;
	FILE		*fp;
	struct berval b64;

	Debug( LDAP_DEBUG_ARGS, "==> sock_back_extended(%s)\n",
		op->ore_reqoid.bv_val, op->o_req_dn.bv_val, 0 );

	if ( (fp = opensock( si->si_sockpath )) == NULL ) {
		send_ldap_error( op, rs, LDAP_OTHER,
			"could not open socket" );
		return( -1 );
	}

	/* write out the request to the extended process */
	fprintf( fp, "EXTENDED\n" );
	fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
	sock_print_conn( fp, op->o_conn, si );
	sock_print_suffixes( fp, op->o_bd );
	fprintf( fp, "oid: %s\n", op->ore_reqoid.bv_val );

	if (op->ore_reqdata) {

		b64.bv_len = LUTIL_BASE64_ENCODE_LEN( op->ore_reqdata->bv_len ) + 1;
		b64.bv_val = op->o_tmpalloc( b64.bv_len + 1, op->o_tmpmemctx );

		rc = lutil_b64_ntop(
			(unsigned char *) op->ore_reqdata->bv_val, op->ore_reqdata->bv_len,
			b64.bv_val, b64.bv_len );

		b64.bv_len = rc;
		assert( strlen(b64.bv_val) == b64.bv_len );

		fprintf( fp, "value: %s\n", b64.bv_val );

		op->o_tmpfree( b64.bv_val, op->o_tmpmemctx );

	}

	fprintf( fp, "\n" );

	/* read in the results and send them along */
	rc = sock_read_and_send_results( op, rs, fp );
	fclose( fp );

	return( rc );
}
