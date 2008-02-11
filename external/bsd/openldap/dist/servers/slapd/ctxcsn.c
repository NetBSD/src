/* ctxcsn.c -- Context CSN Management Routines */
/* $OpenLDAP: pkg/ldap/servers/slapd/ctxcsn.c,v 1.40.2.6 2008/02/12 00:44:15 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * Portions Copyright 2003 IBM Corporation.
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

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "lutil_ldap.h"

const struct berval slap_ldapsync_bv = BER_BVC("ldapsync");
const struct berval slap_ldapsync_cn_bv = BER_BVC("cn=ldapsync");
int slap_serverID;

void
slap_get_commit_csn(
	Operation *op,
	struct berval *maxcsn
)
{
	struct slap_csn_entry *csne, *committed_csne = NULL;

	if ( maxcsn ) {
		BER_BVZERO( maxcsn );
	}

	ldap_pvt_thread_mutex_lock( op->o_bd->be_pcl_mutexp );

	LDAP_TAILQ_FOREACH( csne, op->o_bd->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			csne->ce_state = SLAP_CSN_COMMIT;
			break;
		}
	}

	LDAP_TAILQ_FOREACH( csne, op->o_bd->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_state == SLAP_CSN_COMMIT ) committed_csne = csne;
		if ( csne->ce_state == SLAP_CSN_PENDING ) break;
	}

	if ( committed_csne && maxcsn ) *maxcsn = committed_csne->ce_csn;
	ldap_pvt_thread_mutex_unlock( op->o_bd->be_pcl_mutexp );
}

void
slap_rewind_commit_csn( Operation *op )
{
	struct slap_csn_entry *csne;

	ldap_pvt_thread_mutex_lock( op->o_bd->be_pcl_mutexp );

	LDAP_TAILQ_FOREACH( csne, op->o_bd->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			csne->ce_state = SLAP_CSN_PENDING;
			break;
		}
	}

	ldap_pvt_thread_mutex_unlock( op->o_bd->be_pcl_mutexp );
}

void
slap_graduate_commit_csn( Operation *op )
{
	struct slap_csn_entry *csne;

	if ( op == NULL ) return;
	if ( op->o_bd == NULL ) return;

#if 0
	/* it is NULL when we get here from the frontendDB;
	 * alternate fix: initialize frontendDB like all other backends */
	assert( op->o_bd->be_pcl_mutexp != NULL );
#endif
	
	if ( op->o_bd->be_pcl_mutexp == NULL ) return;

	ldap_pvt_thread_mutex_lock( op->o_bd->be_pcl_mutexp );

	LDAP_TAILQ_FOREACH( csne, op->o_bd->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			LDAP_TAILQ_REMOVE( op->o_bd->be_pending_csn_list,
				csne, ce_csn_link );
			Debug( LDAP_DEBUG_SYNC, "slap_graduate_commit_csn: removing %p %s\n",
				csne->ce_csn.bv_val, csne->ce_csn.bv_val, 0 );
			if ( op->o_csn.bv_val == csne->ce_csn.bv_val ) {
				BER_BVZERO( &op->o_csn );
			}
			ch_free( csne->ce_csn.bv_val );
			ch_free( csne );
			break;
		}
	}

	ldap_pvt_thread_mutex_unlock( op->o_bd->be_pcl_mutexp );

	return;
}

static struct berval ocbva[] = {
	BER_BVC("top"),
	BER_BVC("subentry"),
	BER_BVC("syncProviderSubentry"),
	BER_BVNULL
};

Entry *
slap_create_context_csn_entry(
	Backend *be,
	struct berval *context_csn )
{
	Entry* e;

	struct berval bv;

	e = entry_alloc();

	attr_merge( e, slap_schema.si_ad_objectClass,
		ocbva, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
		&ocbva[1], NULL );
	attr_merge_one( e, slap_schema.si_ad_cn,
		(struct berval *)&slap_ldapsync_bv, NULL );

	if ( context_csn ) {
		attr_merge_one( e, slap_schema.si_ad_contextCSN,
			context_csn, NULL );
	}

	BER_BVSTR( &bv, "{}" );
	attr_merge_one( e, slap_schema.si_ad_subtreeSpecification, &bv, NULL );

	build_new_dn( &e->e_name, &be->be_nsuffix[0],
		(struct berval *)&slap_ldapsync_cn_bv, NULL );
	ber_dupbv( &e->e_nname, &e->e_name );

	return e;
}

void
slap_queue_csn(
	Operation *op,
	struct berval *csn )
{
	struct slap_csn_entry *pending;

	pending = (struct slap_csn_entry *) ch_calloc( 1,
			sizeof( struct slap_csn_entry ));

	Debug( LDAP_DEBUG_SYNC, "slap_queue_csn: queing %p %s\n", csn->bv_val, csn->bv_val, 0 );

	ldap_pvt_thread_mutex_lock( op->o_bd->be_pcl_mutexp );

	ber_dupbv( &pending->ce_csn, csn );
	ber_bvreplace_x( &op->o_csn, &pending->ce_csn, op->o_tmpmemctx );
	pending->ce_connid = op->o_connid;
	pending->ce_opid = op->o_opid;
	pending->ce_state = SLAP_CSN_PENDING;
	LDAP_TAILQ_INSERT_TAIL( op->o_bd->be_pending_csn_list,
		pending, ce_csn_link );
	ldap_pvt_thread_mutex_unlock( op->o_bd->be_pcl_mutexp );
}

int
slap_get_csn(
	Operation *op,
	struct berval *csn,
	int manage_ctxcsn )
{
	if ( csn == NULL ) return LDAP_OTHER;

	/* gmtime doesn't always need a mutex, but lutil_csnstr does */
	ldap_pvt_thread_mutex_lock( &gmtime_mutex );
	csn->bv_len = lutil_csnstr( csn->bv_val, csn->bv_len, slap_serverID, 0 );
	ldap_pvt_thread_mutex_unlock( &gmtime_mutex );

	if ( manage_ctxcsn )
		slap_queue_csn( op, csn );

	return LDAP_SUCCESS;
}
