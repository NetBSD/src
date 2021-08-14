/*	$NetBSD: txn.c,v 1.3 2021/08/14 16:14:58 christos Exp $	*/

/* txn.c - LDAP Transactions */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
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
__RCSID("$NetBSD: txn.c,v 1.3 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include "slap.h"

#include <lber_pvt.h>
#include <lutil.h>

const struct berval slap_EXOP_TXN_START = BER_BVC(LDAP_EXOP_TXN_START);
const struct berval slap_EXOP_TXN_END = BER_BVC(LDAP_EXOP_TXN_END);

int txn_start_extop(
	Operation *op, SlapReply *rs )
{
	int rc;
	struct berval *bv;

	Debug( LDAP_DEBUG_STATS, "%s TXN START\n",
		op->o_log_prefix );

	if( op->ore_reqdata != NULL ) {
		rs->sr_text = "no request data expected";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_bd = op->o_conn->c_authz_backend;
	if( backend_check_restrictions( op, rs,
		(struct berval *)&slap_EXOP_TXN_START ) != LDAP_SUCCESS )
	{
		return rs->sr_err;
	}

	/* acquire connection lock */
	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

	if( op->o_conn->c_txn != CONN_TXN_INACTIVE ) {
		rs->sr_text = "Too many transactions";
		rc = LDAP_BUSY;
		goto done;
	}

	assert( op->o_conn->c_txn_backend == NULL );
	op->o_conn->c_txn = CONN_TXN_SPECIFY;

	bv = (struct berval *) ch_malloc( sizeof (struct berval) );
	bv->bv_len = 0;
	bv->bv_val = NULL;

	rs->sr_rspdata = bv;
	rc = LDAP_SUCCESS;

done:
	/* release connection lock */
	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
	return rc;
}

int txn_spec_ctrl(
	Operation *op, SlapReply *rs, LDAPControl *ctrl )
{
	if ( !ctrl->ldctl_iscritical ) {
		rs->sr_text = "txnSpec control must be marked critical";
		return LDAP_PROTOCOL_ERROR;
	}
	if( op->o_txnSpec ) {
		rs->sr_text = "txnSpec control provided multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( ctrl->ldctl_value.bv_val == NULL ) {
		rs->sr_text = "no transaction identifier provided";
		return LDAP_PROTOCOL_ERROR;
	}
	if ( ctrl->ldctl_value.bv_len != 0 ) {
		rs->sr_text = "invalid transaction identifier";
		return LDAP_TXN_ID_INVALID;
	}

	if ( op->o_preread ) { /* temporary limitation */
		rs->sr_text = "cannot perform pre-read in transaction";
		return LDAP_UNWILLING_TO_PERFORM;
	} 
	if ( op->o_postread ) { /* temporary limitation */
		rs->sr_text = "cannot perform post-read in transaction";
		return LDAP_UNWILLING_TO_PERFORM;
	}

	op->o_txnSpec = SLAP_CONTROL_CRITICAL;
	return LDAP_SUCCESS;
}

typedef struct txn_rctrls {
	struct txn_rctrls *tr_next;
	ber_int_t	tr_msgid;
	LDAPControl ** tr_ctrls;
} txn_rctrls;

static int txn_result( Operation *op, SlapReply *rs )
{
	if ( rs->sr_ctrls ) {
		txn_rctrls **t0, *tr;
		for ( t0 = (txn_rctrls **) &op->o_callback->sc_private; *t0;
			t0 = &(*t0)->tr_next )
			;
		tr = op->o_tmpalloc( sizeof( txn_rctrls ), op->o_tmpmemctx );
		tr->tr_next = NULL;
		*t0 = tr;
		tr->tr_msgid = op->o_msgid;
		tr->tr_ctrls = ldap_controls_dup( rs->sr_ctrls );
	}
	return rs->sr_err;
}

static int txn_put_ctrls( Operation *op, BerElement *ber, txn_rctrls *tr )
{
	txn_rctrls *next;
	int i;
	ber_printf( ber, "{" );
	for ( ; tr; tr  = next ) {
		next = tr->tr_next;
		ber_printf( ber, "{it{", tr->tr_msgid, LDAP_TAG_CONTROLS );
		for ( i = 0; tr->tr_ctrls[i]; i++ )
			ldap_pvt_put_control( tr->tr_ctrls[i], ber );
		ber_printf( ber, "}}" );
		ldap_controls_free( tr->tr_ctrls );
		op->o_tmpfree( tr, op->o_tmpmemctx );
	}
	ber_printf( ber, "}" );
	return 0;
}

int txn_end_extop(
	Operation *op, SlapReply *rs )
{
	int rc;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t commit=1;
	struct berval txnid;
	Operation *o, *p;
	Connection *c = op->o_conn;

	Debug( LDAP_DEBUG_STATS, "%s TXN END\n",
		op->o_log_prefix );

	if( op->ore_reqdata == NULL ) {
		rs->sr_text = "request data expected";
		return LDAP_PROTOCOL_ERROR;
	}
	if( op->ore_reqdata->bv_len == 0 ) {
		rs->sr_text = "empty request data";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_bd = c->c_authz_backend;
	if( backend_check_restrictions( op, rs,
		(struct berval *)&slap_EXOP_TXN_END ) != LDAP_SUCCESS )
	{
		return rs->sr_err;
	}

	ber_init2( ber, op->ore_reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );
	if( tag == LBER_ERROR ) {
		rs->sr_text = "request data decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	tag = ber_peek_tag( ber, &len );
	if( tag == LBER_BOOLEAN ) {
		tag = ber_scanf( ber, "b", &commit );
		if( tag == LBER_ERROR ) {
			rs->sr_text = "request data decoding error";
			return LDAP_PROTOCOL_ERROR;
		}
	}

	tag = ber_scanf( ber, /*{*/ "m}", &txnid );
	if( tag == LBER_ERROR ) {
		rs->sr_text = "request data decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	if( txnid.bv_len ) {
		rs->sr_text = "invalid transaction identifier";
		return LDAP_TXN_ID_INVALID;
	}

	/* acquire connection lock */
	ldap_pvt_thread_mutex_lock( &c->c_mutex );

	if( c->c_txn != CONN_TXN_SPECIFY ) {
		rs->sr_text = "invalid transaction identifier";
		rc = LDAP_TXN_ID_INVALID;
		goto done;
	}
	c->c_txn = CONN_TXN_SETTLE;

	if( commit ) {
		slap_callback cb = {0};
		OpExtra *txn = NULL;
		if ( op->o_abandon ) {
			goto drain;
		}

		if( LDAP_STAILQ_EMPTY(&c->c_txn_ops) ) {
			/* no updates to commit */
			rs->sr_text = "no updates to commit";
			rc = LDAP_OPERATIONS_ERROR;
			goto settled;
		}

		cb.sc_response = txn_result;
		LDAP_STAILQ_FOREACH( o, &c->c_txn_ops, o_next ) {
			o->o_bd = c->c_txn_backend;
			p = o;
			if ( !txn ) {
				rc = o->o_bd->bd_info->bi_op_txn(o, SLAP_TXN_BEGIN, &txn );
				if ( rc ) {
					rs->sr_text = "couldn't start DB transaction";
					rc = LDAP_OTHER;
					goto drain;
				}
			} else {
				LDAP_SLIST_INSERT_HEAD( &o->o_extra, txn, oe_next );
			}
			cb.sc_next = o->o_callback;
			o->o_callback = &cb;
			{
				SlapReply rs = {REP_RESULT};
				int opidx = slap_req2op( o->o_tag );
				assert( opidx != SLAP_OP_LAST );
				o->o_threadctx = op->o_threadctx;
				o->o_tid = op->o_tid;
				ldap_pvt_thread_mutex_unlock( &c->c_mutex );
				rc = (&o->o_bd->bd_info->bi_op_bind)[opidx]( o, &rs );
				ldap_pvt_thread_mutex_lock( &c->c_mutex );
			}
			if ( rc ) {
				struct berval *bv = NULL;
				BerElementBuffer berbuf;
				BerElement *ber = (BerElement *)&berbuf;

				ber_init_w_nullc( ber, LBER_USE_DER );
				ber_printf( ber, "{i", o->o_msgid );
				if ( cb.sc_private )
					txn_put_ctrls( op, ber, cb.sc_private );
				ber_printf( ber, "}" );
				ber_flatten( ber, &bv );
				ber_free_buf( ber );
				rs->sr_rspdata = bv;
				o->o_bd->bd_info->bi_op_txn(o, SLAP_TXN_ABORT, &txn );
				goto drain;
			}
		}
		if ( cb.sc_private ) {
			struct berval *bv = NULL;
			BerElementBuffer berbuf;
			BerElement *ber = (BerElement *)&berbuf;

			ber_init_w_nullc( ber, LBER_USE_DER );
			ber_printf( ber, "{" );
			txn_put_ctrls( op, ber, cb.sc_private );
			ber_printf( ber, "}" );
			ber_flatten( ber, &bv );
			ber_free_buf( ber );
			rs->sr_rspdata = bv;
		}
		o = p;
		rc = o->o_bd->bd_info->bi_op_txn(o, SLAP_TXN_COMMIT, &txn );
		if ( rc ) {
			rs->sr_text = "transaction commit failed";
			rc = LDAP_OTHER;
		}
	} else {
		rs->sr_text = "transaction aborted";
		rc = LDAP_SUCCESS;
	}

drain:
	/* drain txn ops list */
	while (( o = LDAP_STAILQ_FIRST( &c->c_txn_ops )) != NULL ) {
		LDAP_STAILQ_REMOVE_HEAD( &c->c_txn_ops, o_next );
		LDAP_STAILQ_NEXT( o, o_next ) = NULL;
		slap_op_free( o, NULL );
	}

settled:
	assert( LDAP_STAILQ_EMPTY(&c->c_txn_ops) );
	assert( c->c_txn == CONN_TXN_SETTLE );
	c->c_txn = CONN_TXN_INACTIVE;
	c->c_txn_backend = NULL;

done:
	/* release connection lock */
	ldap_pvt_thread_mutex_unlock( &c->c_mutex );

	return rc;
}

int txn_preop( Operation *op, SlapReply *rs )
{
	/* acquire connection lock */
	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
	if( op->o_conn->c_txn == CONN_TXN_INACTIVE ) {
		rs->sr_text = "invalid transaction identifier";
		rs->sr_err = LDAP_TXN_ID_INVALID;
		goto txnReturn;
	}

	if( op->o_conn->c_txn_backend == NULL ) {
		op->o_conn->c_txn_backend = op->o_bd;

	} else if( op->o_conn->c_txn_backend != op->o_bd ) {
		rs->sr_text = "transaction cannot span multiple database contexts";
		rs->sr_err = LDAP_AFFECTS_MULTIPLE_DSAS;
		goto txnReturn;
	}

	if ( !SLAP_TXNS( op->o_bd )) {
		rs->sr_text = "backend doesn't support transactions";
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto txnReturn;
	}

	/* insert operation into transaction */
	LDAP_STAILQ_REMOVE( &op->o_conn->c_ops, op, Operation, o_next );
	LDAP_STAILQ_INSERT_TAIL( &op->o_conn->c_txn_ops, op, o_next );

txnReturn:
	/* release connection lock */
	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

	if ( op->o_tag != LDAP_REQ_EXTENDED )
		send_ldap_result( op, rs );
	if ( !rs->sr_err )
		rs->sr_err = LDAP_TXN_SPECIFY_OKAY;
	return rs->sr_err;
}
