/* cancel.c - LDAP cancel extended operation */
/* $OpenLDAP: pkg/ldap/servers/slapd/cancel.c,v 1.23.2.4 2008/02/11 23:26:43 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include "slap.h"

#include <lber_pvt.h>
#include <lutil.h>

const struct berval slap_EXOP_CANCEL = BER_BVC(LDAP_EXOP_CANCEL);

int cancel_extop( Operation *op, SlapReply *rs )
{
	Operation *o;
	int rc;
	int opid;
	BerElement *ber;

	assert( ber_bvcmp( &slap_EXOP_CANCEL, &op->ore_reqoid ) == 0 );

	if ( op->ore_reqdata == NULL ) {
		rs->sr_text = "no message ID supplied";
		return LDAP_PROTOCOL_ERROR;
	}

	ber = ber_init( op->ore_reqdata );
	if ( ber == NULL ) {
		rs->sr_text = "internal error";
		return LDAP_OTHER;
	}

	if ( ber_scanf( ber, "{i}", &opid ) == LBER_ERROR ) {
		rs->sr_text = "message ID parse failed";
		return LDAP_PROTOCOL_ERROR;
	}

	(void) ber_free( ber, 1 );

	Statslog( LDAP_DEBUG_STATS, "%s CANCEL msg=%d\n",
		op->o_log_prefix, opid, 0, 0, 0 );

	if ( opid < 0 ) {
		rs->sr_text = "message ID invalid";
		return LDAP_PROTOCOL_ERROR;
	}

	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
	LDAP_STAILQ_FOREACH( o, &op->o_conn->c_pending_ops, o_next ) {
		if ( o->o_msgid == opid ) {
			LDAP_STAILQ_REMOVE( &op->o_conn->c_pending_ops, o, Operation, o_next );
			LDAP_STAILQ_NEXT(o, o_next) = NULL;
			op->o_conn->c_n_ops_pending--;
			slap_op_free( o, NULL );
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
			return LDAP_SUCCESS;
		}
	}

	LDAP_STAILQ_FOREACH( o, &op->o_conn->c_ops, o_next ) {
		if ( o->o_msgid == opid ) {
			o->o_abandon = 1;
			break;
		}
	}

	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

	if ( o ) {
		if ( o->o_cancel != SLAP_CANCEL_NONE ) {
			rs->sr_text = "message ID already being cancelled";
			return LDAP_PROTOCOL_ERROR;
		}

		o->o_cancel = SLAP_CANCEL_REQ;

		LDAP_STAILQ_FOREACH( op->o_bd, &backendDB, be_next ) {
			if( !op->o_bd->be_cancel ) continue;

			op->oq_cancel.rs_msgid = opid;
			if ( op->o_bd->be_cancel( op, rs ) == LDAP_SUCCESS ) {
				return LDAP_SUCCESS;
			}
		}

		while ( o->o_cancel == SLAP_CANCEL_REQ ) {
			ldap_pvt_thread_yield();
		}

		if ( o->o_cancel == SLAP_CANCEL_ACK ) {
			rc = LDAP_SUCCESS;
		} else {
			rc = o->o_cancel;
		}

		o->o_cancel = SLAP_CANCEL_DONE;
	} else {
		rs->sr_text = "message ID not found";
	 	rc = LDAP_NO_SUCH_OPERATION;
	}

	return rc;
}
