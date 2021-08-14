/*	$NetBSD: delete.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: delete.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "back-wt.h"
#include "slap-config.h"

int
wt_delete( Operation *op, SlapReply *rs )
{
    struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	struct berval   pdn = {0, NULL};
	Entry *e = NULL;
	Entry *p = NULL;
	int manageDSAit = get_manageDSAit( op );
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	wt_ctx *wc;
	int rc;
	WT_CURSOR *cursor = NULL;

	int parent_is_glue = 0;
	int parent_is_leaf = 0;

	Debug( LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(wt_delete) ": %s\n",
		   op->o_req_dn.bv_val );

	if( op->o_txnSpec && txn_preop( op, rs ))
		return rs->sr_err;

	ctrls[num_ctrls] = 0;
	rs->sr_text = NULL;

	wc = wt_ctx_get(op, wi);
	if( !wc ){
        Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_delete)
			   ": wt_ctx_get failed\n" );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

/* allocate CSN */
	if ( BER_BVISNULL( &op->o_csn ) ) {
		struct berval csn;
		char csnbuf[LDAP_PVT_CSNSTR_BUFSIZE];

		csn.bv_val = csnbuf;
		csn.bv_len = sizeof(csnbuf);
		slap_get_csn( op, &csn, 1 );
	}

	if ( !be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
		dnParent( &op->o_req_ndn, &pdn );
	}

	/* get parent */
	rc = wt_dn2entry(op->o_bd, wc, &pdn, &p);
	switch( rc ) {
	case 0:
	case WT_NOTFOUND:
		break;
	default:
		/* TODO: error handling */
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_delete)
			   ": error at wt_dn2entry() rc=%d\n",
			   rc );
		goto return_results;
	}

	if ( rc == WT_NOTFOUND && pdn.bv_len != 0 ) {
		Debug( LDAP_DEBUG_ARGS,
			   "<== " LDAP_XSTRING(wt_delete) ": no such object %s\n",
			   op->o_req_dn.bv_val );

		if ( p && !BER_BVISEMPTY( &p->e_name )) {
			rs->sr_matched = ch_strdup( p->e_name.bv_val );
			if ( is_entry_referral( p )) {
				BerVarray ref = get_entry_referrals( op, p );
				rs->sr_ref = referral_rewrite( ref, &p->e_name,
											   &op->o_req_dn, LDAP_SCOPE_DEFAULT );
				ber_bvarray_free( ref );
			} else {
				rs->sr_ref = NULL;
			}
		} else {
			rs->sr_ref = referral_rewrite( default_referral, NULL,
										   &op->o_req_dn, LDAP_SCOPE_DEFAULT );
		}

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	/* get entry */
	rc = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
	switch( rc ) {
	case 0:
		break;
	case WT_NOTFOUND:
		Debug( LDAP_DEBUG_ARGS,
			   "<== " LDAP_XSTRING(wt_delete)
			   ": no such object %s\n",
			   op->o_req_dn.bv_val );
		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	default:
		/* TODO: error handling */
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_delete)
			   ": error at wt_dn2entry() rc=%d\n",
			   rc );
		goto return_results;
	}

	/* FIXME : dn2entry() should return non-glue entry */
	if ( !manageDSAit && is_entry_glue( e ) ) {
		Debug( LDAP_DEBUG_ARGS,
			   "<== " LDAP_XSTRING(wt_delete)
			   ": glue entry %s\n",
			   op->o_req_dn.bv_val );

		rs->sr_matched = ch_strdup( e->e_dn );
		if ( is_entry_referral( e )) {
			BerVarray ref = get_entry_referrals( op, e );
			rs->sr_ref = referral_rewrite( ref, &e->e_name,
										   &op->o_req_dn, LDAP_SCOPE_DEFAULT );
			ber_bvarray_free( ref );
		} else {
			rs->sr_ref = NULL;
		}

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	if ( pdn.bv_len != 0 ) {
		/* check parent for "children" acl */
		rs->sr_err = access_allowed( op, p,
									 children, NULL, ACL_WDEL, NULL );

		if ( !rs->sr_err  ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<== " LDAP_XSTRING(wt_delete) ": no write "
				   "access to parent\n" );
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			rs->sr_text = "no write access to parent";
			goto return_results;
		}

	} else {
		/* no parent, must be root to delete */
		if( ! be_isroot( op ) ) {
			if ( be_issuffix( op->o_bd, (struct berval *)&slap_empty_bv )
				 || be_shadow_update( op ) ) {
				p = (Entry *)&slap_entry_root;

				/* check parent for "children" acl */
				rs->sr_err = access_allowed( op, p,
											 children, NULL, ACL_WDEL, NULL );

				p = NULL;

				if ( !rs->sr_err  ) {
					Debug( LDAP_DEBUG_TRACE,
						   "<== " LDAP_XSTRING(wt_delete)
						   ": no access to parent\n" );
					rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
					rs->sr_text = "no write access to parent";
					goto return_results;
				}

			} else {
				Debug( LDAP_DEBUG_TRACE,
					   "<== " LDAP_XSTRING(wt_delete)
					   ": no parent and not root\n" );
				rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				goto return_results;
			}
		}
	}

	if ( get_assert( op ) &&
		 ( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	rs->sr_err = access_allowed( op, e,
								 entry, NULL, ACL_WDEL, NULL );
	if ( !rs->sr_err  ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<== " LDAP_XSTRING(wt_delete) ": no write access "
			   "to entry\n" );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;
	}

	if ( !manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow delete */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(tw_delete) ": entry is referral\n" );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_matched = ch_strdup( e->e_name.bv_val );
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	/* pre-read */
	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, e,
								&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				   "<== " LDAP_XSTRING(wt_delete) ": pre-read "
				   "failed!\n" );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
                 * operation if control fails? */
				goto return_results;
			}
		}
	}

    /* Can't do it if we have kids */
	rc = wt_dn2id_has_children( op, wc->session, e->e_id );
	if( rc != WT_NOTFOUND ) {
		switch( rc ) {
		case 0:
			Debug(LDAP_DEBUG_ARGS,
				  "<== " LDAP_XSTRING(wt_delete)
				  ": non-leaf %s\n",
				  op->o_req_dn.bv_val );
			rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
			rs->sr_text = "subordinate objects must be deleted first";
			break;
		default:
			Debug(LDAP_DEBUG_ARGS,
				  "<== " LDAP_XSTRING(wt_delete)
				  ": has_children failed: %s (%d)\n",
				  wiredtiger_strerror(rc), rc );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
		}
		goto return_results;
	}

	/* begin transaction */
	rc = wc->session->begin_transaction(wc->session, NULL);
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add) ": begin_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "begin_transaction failed";
		goto return_results;
	}

	/* delete from dn2id */
	rc = wt_dn2id_delete( op, wc->session, &e->e_nname);
	if ( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			  "<== " LDAP_XSTRING(wt_delete)
			  ": dn2id failed: %s (%d)\n",
			  wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "dn2id delete failed";
		wc->session->rollback_transaction(wc->session, NULL);
		goto return_results;
	}

	/* delete indices for old attributes */
	rc = wt_index_entry_del( op, wc, e );
	if ( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			  "<== " LDAP_XSTRING(wt_delete)
			  ": index delete failed: %s (%d)\n",
			  wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "index delete failed";
		wc->session->rollback_transaction(wc->session, NULL);
		goto return_results;
	}

	/* fixup delete CSN */
	if ( !SLAP_SHADOW( op->o_bd )) {
		struct berval vals[2];

		assert( !BER_BVISNULL( &op->o_csn ) );
		vals[0] = op->o_csn;
		BER_BVZERO( &vals[1] );
		rs->sr_err = wt_index_values( op, wc->session, slap_schema.si_ad_entryCSN,
									  vals, 0, SLAP_INDEX_ADD_OP );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_text = "entryCSN index update failed";
			rs->sr_err = LDAP_OTHER;
			wc->session->rollback_transaction(wc->session, NULL);
			goto return_results;
		}
	}

	/* delete from id2entry */
	rc = wt_id2entry_delete( op, wc->session, e );
	if ( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<== " LDAP_XSTRING(wt_delete)
			   ": id2entry failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "entry delete failed";
		wc->session->rollback_transaction(wc->session, NULL);
		goto return_results;
	}

	if ( pdn.bv_len != 0 ) {
		// TODO: glue entry
	}

	rc = wc->session->commit_transaction(wc->session, NULL);
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<== " LDAP_XSTRING(wt_delete)
			   ": commit_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit_transaction failed";
		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		   LDAP_XSTRING(wt_delete)
		   ": deleted%s id=%08lx dn=\"%s\"\n",
		   op->o_noop ? " (no-op)" : "", e->e_id, op->o_req_dn.bv_val );

	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) {
		rs->sr_ctrls = ctrls;
	}

return_results:
	if ( rs->sr_err == LDAP_SUCCESS && parent_is_glue && parent_is_leaf ) {
		op->o_delete_glue_parent = 1;
	}

	if ( p != NULL ) {
		wt_entry_return( p );
	}

	/* free entry */
	if( e != NULL ) {
		wt_entry_return( e );
	}

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}

	/* TODO: checkpoint */

	return rs->sr_err;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
