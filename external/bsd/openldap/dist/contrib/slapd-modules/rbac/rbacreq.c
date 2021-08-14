/*	$NetBSD: rbacreq.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbacreq.c - RBAC requests */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 *
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: rbacreq.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

rbac_req_t *
rbac_alloc_req( int type )
{
	rbac_req_t *reqp = NULL;

	reqp = ch_calloc( 1, sizeof(rbac_req_t) );

	reqp->req_type = type;
	BER_BVZERO( &reqp->sessid );
	BER_BVZERO( &reqp->tenantid );
	/* session creation */
	BER_BVZERO( &reqp->uid );
	BER_BVZERO( &reqp->authtok );
	reqp->roles = NULL;
	/* check access  */
	BER_BVZERO( &reqp->opname );
	BER_BVZERO( &reqp->objname );
	BER_BVZERO( &reqp->objid );
	/* add/drop role */
	BER_BVZERO( &reqp->role );

	return reqp;
}

void
rbac_free_req( rbac_req_t *reqp )
{
	if ( !reqp ) return;

	if ( !BER_BVISNULL( &reqp->sessid ) )
		ber_memfree( reqp->sessid.bv_val );

	if ( !BER_BVISNULL( &reqp->tenantid ) )
		ber_memfree( reqp->tenantid.bv_val );

	/* session creation */
	if ( !BER_BVISNULL( &reqp->uid ) )
		ber_memfree( reqp->uid.bv_val );

	if ( !BER_BVISNULL( &reqp->authtok ) )
		ber_memfree( reqp->authtok.bv_val );

	if ( reqp->roles )
		ber_bvarray_free( reqp->roles );

	/* check access  */
	if ( !BER_BVISNULL( &reqp->opname ) )
		ber_memfree( reqp->opname.bv_val );

	if ( !BER_BVISNULL( &reqp->objname ) )
		ber_memfree( reqp->objname.bv_val );

	if ( !BER_BVISNULL( &reqp->objid ) )
		ber_memfree( reqp->objid.bv_val );

	ch_free( reqp );

	return;
}
