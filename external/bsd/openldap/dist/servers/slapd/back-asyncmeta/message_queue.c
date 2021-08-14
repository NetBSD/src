/*	$NetBSD: message_queue.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* message_queue.c - routines to maintain the per-connection lists
 * of pending operations */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2021 The OpenLDAP Foundation.
 * Portions Copyright 2016 Symas Corporation.
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
 * This work was developed by Symas Corporation
 * based on back-meta module for inclusion in OpenLDAP Software.
 * This work was sponsored by Ericsson. */

#include <sys/cdefs.h>
__RCSID("$NetBSD: message_queue.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "lutil.h"
#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"
#include "../../../libraries/liblber/lber-int.h"
#include "lutil.h"


typedef struct listptr {
	void *reserved;
	struct listptr *next;
} listptr;

typedef struct listhead {
	struct listptr *list;
	int cnt;
} listhead;

#ifndef LH_MAX
#define LH_MAX	16
#endif

static void asyncmeta_memctx_put(void *threadctx, void *memctx)
{
	slap_sl_mem_setctx(threadctx, NULL);
	slap_sl_mem_destroy((void *)1, memctx);
}

int asyncmeta_new_bm_context(Operation *op,
			     SlapReply *rs,
			     bm_context_t **new_bc,
			     int ntargets,
			     a_metainfo_t	*mi)
{
	int i;
	*new_bc = op->o_tmpcalloc( 1, sizeof( bm_context_t ), op->o_tmpmemctx );

	(*new_bc)->op = op;
	(*new_bc)->copy_op = *op;
	(*new_bc)->candidates = op->o_tmpcalloc(ntargets, sizeof(SlapReply),op->o_tmpmemctx);
	(*new_bc)->msgids = op->o_tmpcalloc(ntargets, sizeof(int),op->o_tmpmemctx);
	(*new_bc)->nretries = op->o_tmpcalloc(ntargets, sizeof(int),op->o_tmpmemctx);
	(*new_bc)->c_peer_name = op->o_conn->c_peer_name;
	(*new_bc)->is_root = be_isroot( op );

	switch(op->o_tag) {
	case LDAP_REQ_COMPARE:
		{
		AttributeAssertion *ava = op->o_tmpcalloc( 1, sizeof(AttributeAssertion), op->o_tmpmemctx );
		*ava = *op->orc_ava;
		op->orc_ava = ava;
		}
		break;
	case LDAP_REQ_MODRDN:
		if (op->orr_newSup != NULL) {
			struct berval *bv = op->o_tmpalloc( sizeof( struct berval ), op->o_tmpmemctx );
			*bv = *op->orr_newSup;
			op->orr_newSup = bv;
		}

		if (op->orr_nnewSup != NULL) {
			struct berval *bv = op->o_tmpalloc( sizeof( struct berval ), op->o_tmpmemctx );
			*bv = *op->orr_nnewSup;
			op->orr_nnewSup = bv;
		}
		break;
	default:
		break;
	}
	for (i = 0; i < ntargets; i++) {
		(*new_bc)->msgids[i] = META_MSGID_UNDEFINED;
	}
	for (i = 0; i < ntargets; i++) {
		(*new_bc)->nretries[i] = mi->mi_targets[i]->mt_nretries;
	}
	return LDAP_SUCCESS;
}

void asyncmeta_free_op(Operation *op)
{
	assert (op != NULL);
	switch (op->o_tag) {
	case LDAP_REQ_SEARCH:
		break;
	case LDAP_REQ_ADD:
		if ( op->ora_modlist != NULL ) {
			slap_mods_free(op->ora_modlist, 0 );
		}

		if ( op->ora_e != NULL ) {
			entry_free( op->ora_e );
		}

		break;
	case LDAP_REQ_MODIFY:
		if ( op->orm_modlist != NULL ) {
			slap_mods_free(op->orm_modlist, 1 );
		}
		break;
	case LDAP_REQ_MODRDN:
		if ( op->orr_modlist != NULL ) {
			slap_mods_free(op->orr_modlist, 1 );
		}
		break;
	case LDAP_REQ_COMPARE:
		break;
	case LDAP_REQ_DELETE:
		break;
	default:
		Debug( LDAP_DEBUG_TRACE, "==> asyncmeta_free_op : other message type" );
	}

	connection_op_finish( op );
	slap_op_free( op, op->o_threadctx );
}




void asyncmeta_clear_bm_context(bm_context_t *bc)
{

	Operation *op = bc->op;
	void *thrctx, *memctx;
	int i;

	if ( bc->bc_mc && bc->bc_mc->mc_info ) {
		for (i = 0; i < bc->bc_mc->mc_info->mi_ntargets; i++) {
			if (bc->candidates[ i ].sr_text != NULL) {
				ch_free( (char *)bc->candidates[ i ].sr_text );
				bc->candidates[ i ].sr_text = NULL;
			}
		}
	}

	if (op->o_conn->c_conn_idx == -1)
		return;
	memctx = op->o_tmpmemctx;
	thrctx = op->o_threadctx;
	while (op->o_bd == bc->copy_op.o_bd)
		ldap_pvt_thread_yield();
	asyncmeta_free_op(op);
	asyncmeta_memctx_put(thrctx, memctx);
}

int asyncmeta_add_message_queue(a_metaconn_t *mc, bm_context_t *bc)
{
	a_metainfo_t *mi = mc->mc_info;
	int max_pending_ops = (mi->mi_max_pending_ops == 0) ? META_BACK_CFG_MAX_PENDING_OPS : mi->mi_max_pending_ops;

	Debug( LDAP_DEBUG_TRACE, "add_message_queue: mc %p, pending_ops %d, max_pending %d\n",
		mc, mc->pending_ops, max_pending_ops );

	assert(bc->bc_mc == NULL);
	if (mc->pending_ops >= max_pending_ops) {
		return LDAP_BUSY;
	}
	bc->bc_mc = mc;

	slap_sl_mem_setctx(bc->op->o_threadctx, NULL);
	LDAP_STAILQ_INSERT_TAIL( &mc->mc_om_list, bc, bc_next);
	mc->pending_ops++;
	return LDAP_SUCCESS;
}


void
asyncmeta_drop_bc(a_metaconn_t *mc, bm_context_t *bc)
{
	bm_context_t *om;
	LDAP_STAILQ_FOREACH( om, &mc->mc_om_list, bc_next ) {
		if (om == bc) {
			LDAP_STAILQ_REMOVE(&mc->mc_om_list, om, bm_context_t, bc_next);
			mc->pending_ops--;
			break;
		}
	}
	assert(om == bc);
	assert(bc->bc_mc == mc);
}


bm_context_t *
asyncmeta_find_message(ber_int_t msgid, a_metaconn_t *mc, int candidate)
{
	bm_context_t *om;
	LDAP_STAILQ_FOREACH( om, &mc->mc_om_list, bc_next ) {
		if (om->candidates[candidate].sr_msgid == msgid && !om->bc_invalid) {
			break;
		}
	}
	return om;
}

bm_context_t *
asyncmeta_bc_in_queue(a_metaconn_t *mc, bm_context_t *bc)
{
	bm_context_t *om;
	LDAP_STAILQ_FOREACH( om, &mc->mc_om_list, bc_next ) {
		if (om == bc) {
			return bc;
		}
	}
	return NULL;
}
