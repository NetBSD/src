/* op.c - relay backend operations */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-relay/op.c,v 1.15.2.6 2008/02/12 01:03:16 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2008 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include "slap.h"
#include "back-relay.h"

#define	RB_ERR_MASK		(0x0000FFFFU)
#define RB_ERR			(0x10000000U)
#define RB_UNSUPPORTED_FLAG	(0x20000000U)
#define RB_REFERRAL		(0x40000000U)
#define RB_SEND			(0x80000000U)
#define RB_UNSUPPORTED		(LDAP_UNWILLING_TO_PERFORM|RB_ERR|RB_UNSUPPORTED_FLAG)
#define	RB_UNSUPPORTED_SEND	(RB_UNSUPPORTED|RB_SEND)
#define	RB_REFERRAL_SEND	(RB_REFERRAL|RB_SEND)
#define	RB_ERR_SEND		(RB_ERR|RB_SEND)
#define	RB_ERR_REFERRAL_SEND	(RB_ERR|RB_REFERRAL|RB_SEND)

static int
relay_back_swap_bd( Operation *op, SlapReply *rs )
{
	slap_callback	*cb = op->o_callback;
	BackendDB	*be = op->o_bd;

	op->o_bd = cb->sc_private;
	cb->sc_private = be;

	return SLAP_CB_CONTINUE;
}

#define relay_back_add_cb( cb, op ) \
	{						\
		(cb)->sc_next = (op)->o_callback;	\
		(cb)->sc_response = relay_back_swap_bd;	\
		(cb)->sc_cleanup = relay_back_swap_bd;	\
		(cb)->sc_private = (op)->o_bd;		\
		(op)->o_callback = (cb);		\
	}

/*
 * selects the backend if not enforced at config;
 * in case of failure, behaves based on err:
 *	-1			don't send result
 *	LDAP_SUCCESS		don't send result; may send referral if dosend
 *	any valid error 	send as error result if dosend
 */
static BackendDB *
relay_back_select_backend( Operation *op, SlapReply *rs, slap_mask_t fail_mode )
{
	relay_back_info		*ri = (relay_back_info *)op->o_bd->be_private;
	BackendDB		*bd = ri->ri_bd;
	int			rc = ( fail_mode & RB_ERR_MASK );

	if ( bd == NULL && !BER_BVISNULL( &op->o_req_ndn ) ) {
		bd = select_backend( &op->o_req_ndn, 1 );
		if ( bd == op->o_bd ) {
			Debug( LDAP_DEBUG_ANY,
				"%s: back-relay for DN=\"%s\" would call self.\n",
				op->o_log_prefix, op->o_req_dn.bv_val, 0 );
			if ( fail_mode & RB_ERR ) {
				rs->sr_err = rc;
				if ( fail_mode & RB_SEND ) {
					send_ldap_result( op, rs );
				}
			}

			return NULL;
		}
	}

	if ( bd == NULL ) {
		if ( ( fail_mode & RB_REFERRAL )
			&& ( fail_mode & RB_SEND )
			&& !BER_BVISNULL( &op->o_req_ndn )
			&& default_referral )
		{
			rs->sr_err = LDAP_REFERRAL;

			/* if we set sr_err to LDAP_REFERRAL,
			 * we must provide one */
			rs->sr_ref = referral_rewrite(
				default_referral,
				NULL, &op->o_req_dn,
				LDAP_SCOPE_DEFAULT );
			if ( !rs->sr_ref ) {
				rs->sr_ref = default_referral;
			}

			send_ldap_result( op, rs );

			if ( rs->sr_ref != default_referral ) {
				ber_bvarray_free( rs->sr_ref );
			}

			return NULL;
		}

		/* NOTE: err is LDAP_INVALID_CREDENTIALS for bind,
		 * LDAP_NO_SUCH_OBJECT for other operations.
		 * noSuchObject cannot be returned by bind */
		rs->sr_err = rc;
		if ( fail_mode & RB_SEND ) {
			send_ldap_result( op, rs );
		}
	}

	return bd;
}

static int
relay_back_op(
	Operation	*op,
	SlapReply	*rs,
	BackendDB	*bd,
	BI_op_func	*func,
	slap_mask_t	fail_mode )
{
	int			rc = ( fail_mode & RB_ERR_MASK );

	if ( func ) {
		BackendDB	*be = op->o_bd;
		slap_callback	cb;

		relay_back_add_cb( &cb, op );

		op->o_bd = bd;
		rc = func( op, rs );
		op->o_bd = be;

		if ( op->o_callback == &cb ) {
			op->o_callback = op->o_callback->sc_next;
		}

	} else if ( fail_mode & RB_ERR ) {
		rs->sr_err = rc;
		if ( fail_mode & RB_UNSUPPORTED_FLAG ) {
			rs->sr_text = "operation not supported within naming context";
		}

		if ( fail_mode & RB_SEND ) {
			send_ldap_result( op, rs );
		}
	}

	return rc;
}

int
relay_back_op_bind( Operation *op, SlapReply *rs )
{
	BackendDB	*bd;

	/* allow rootdn as a means to auth without the need to actually
 	 * contact the proxied DSA */
	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		return rs->sr_err;
	}

	bd = relay_back_select_backend( op, rs,
		( LDAP_INVALID_CREDENTIALS | RB_ERR_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_bind,
		( LDAP_INVALID_CREDENTIALS | RB_ERR_SEND ) );
}

int
relay_back_op_unbind( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs, 0 );
	if ( bd != NULL ) {
		(void)relay_back_op( op, rs, bd, bd->be_unbind, 0 );
	}

	return 0;
}

int
relay_back_op_search( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR_REFERRAL_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_search,
		RB_UNSUPPORTED_SEND );
}

int
relay_back_op_compare( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR_REFERRAL_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_compare,
		( SLAP_CB_CONTINUE | RB_ERR ) );
}

int
relay_back_op_modify( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR_REFERRAL_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_modify,
		RB_UNSUPPORTED_SEND );
}

int
relay_back_op_modrdn( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR_REFERRAL_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_modrdn,
		RB_UNSUPPORTED_SEND );
}

int
relay_back_op_add( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR_REFERRAL_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_add,
		RB_UNSUPPORTED_SEND );
}

int
relay_back_op_delete( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR_REFERRAL_SEND ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_delete,
		RB_UNSUPPORTED_SEND );
}

int
relay_back_op_abandon( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs, 0 );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_abandon, 0 );
}

int
relay_back_op_cancel( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;
	int			rc;

	bd = relay_back_select_backend( op, rs,
		( LDAP_CANNOT_CANCEL | RB_ERR ) );
	if ( bd == NULL ) {
		if ( op->o_cancel == SLAP_CANCEL_REQ ) {
			op->o_cancel = LDAP_CANNOT_CANCEL;
		}
		return rs->sr_err;
	}

	rc = relay_back_op( op, rs, bd, bd->be_cancel,
		( LDAP_CANNOT_CANCEL | RB_ERR ) );
	if ( rc == LDAP_CANNOT_CANCEL && op->o_cancel == SLAP_CANCEL_REQ )
	{
		op->o_cancel = LDAP_CANNOT_CANCEL;
	}

	return rc;
}

int
relay_back_op_extended( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_NO_SUCH_OBJECT | RB_ERR | RB_REFERRAL ) );
	if ( bd == NULL ) {
		return rs->sr_err;
	}

	return relay_back_op( op, rs, bd, bd->be_extended,
		RB_UNSUPPORTED );
}

int
relay_back_entry_release_rw( Operation *op, Entry *e, int rw )
{
	relay_back_info		*ri = (relay_back_info *)op->o_bd->be_private;
	BackendDB		*bd;
	int			rc = 1;

	bd = ri->ri_bd;
	if ( bd == NULL) {
		bd = select_backend( &op->o_req_ndn, 1 );
		if ( bd == NULL ) {
			return 1;
		}
	}

	if ( bd->be_release ) {
		BackendDB	*be = op->o_bd;

		op->o_bd = bd;
		rc = bd->be_release( op, e, rw );
		op->o_bd = be;
	}

	return rc;

}

int
relay_back_entry_get_rw( Operation *op, struct berval *ndn,
	ObjectClass *oc, AttributeDescription *at, int rw, Entry **e )
{
	relay_back_info		*ri = (relay_back_info *)op->o_bd->be_private;
	BackendDB		*bd;
	int			rc = 1;

	bd = ri->ri_bd;
	if ( bd == NULL) {
		bd = select_backend( &op->o_req_ndn, 1 );
		if ( bd == NULL ) {
			return 1;
		}
	}

	if ( bd->be_fetch ) {
		BackendDB	*be = op->o_bd;

		op->o_bd = bd;
		rc = bd->be_fetch( op, ndn, oc, at, rw, e );
		op->o_bd = be;
	}

	return rc;

}

/*
 * NOTE: even the existence of this function is questionable: we cannot
 * pass the bi_chk_referrals() call thru the rwm overlay because there
 * is no way to rewrite the req_dn back; but then relay_back_chk_referrals()
 * is passing the target database a DN that likely does not belong to its
 * naming context... mmmh.
 */
int
relay_back_chk_referrals( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_SUCCESS | RB_ERR_REFERRAL_SEND ) );
	/* FIXME: this test only works if there are no overlays, so
	 * it is nearly useless; if made stricter, no nested back-relays
	 * can be instantiated... too bad. */
	if ( bd == NULL || bd == op->o_bd ) {
		return 0;
	}

	/* no nested back-relays... */
	if ( overlay_is_over( bd ) ) {
		slap_overinfo	*oi = (slap_overinfo *)bd->bd_info->bi_private;

		if ( oi->oi_orig == op->o_bd->bd_info ) {
			return 0;
		}
	}

	return relay_back_op( op, rs, bd, bd->be_chk_referrals, LDAP_SUCCESS );
}

int
relay_back_operational( Operation *op, SlapReply *rs )
{
	BackendDB		*bd;

	bd = relay_back_select_backend( op, rs,
		( LDAP_SUCCESS | RB_ERR ) );
	/* FIXME: this test only works if there are no overlays, so
	 * it is nearly useless; if made stricter, no nested back-relays
	 * can be instantiated... too bad. */
	if ( bd == NULL || bd == op->o_bd ) {
		return 0;
	}

	return relay_back_op( op, rs, bd, bd->be_operational, 0 );
}

int
relay_back_has_subordinates( Operation *op, Entry *e, int *hasSubs )
{
	SlapReply		rs = { 0 };
	BackendDB		*bd;
	int			rc = 1;

	bd = relay_back_select_backend( op, &rs,
		( LDAP_SUCCESS | RB_ERR ) );
	/* FIXME: this test only works if there are no overlays, so
	 * it is nearly useless; if made stricter, no nested back-relays
	 * can be instantiated... too bad. */
	if ( bd == NULL || bd == op->o_bd ) {
		return 0;
	}

	if ( bd->be_has_subordinates ) {
		BackendDB	*be = op->o_bd;

		op->o_bd = bd;
		rc = bd->be_has_subordinates( op, e, hasSubs );
		op->o_bd = be;
	}

	return rc;

}

int
relay_back_connection_init( BackendDB *bd, Connection *c )
{
	relay_back_info		*ri = (relay_back_info *)bd->be_private;

	bd = ri->ri_bd;
	if ( bd == NULL ) {
		return 0;
	}

	if ( bd->be_connection_init ) {
		return bd->be_connection_init( bd, c );
	}

	return 0;
}

int
relay_back_connection_destroy( BackendDB *bd, Connection *c )
{
	relay_back_info		*ri = (relay_back_info *)bd->be_private;

	bd = ri->ri_bd;
	if ( bd == NULL ) {
		return 0;
	}

	if ( bd->be_connection_destroy ) {
		return bd->be_connection_destroy( bd, c );
	}

	return 0;

}

/*
 * FIXME: must implement tools as well
 */
