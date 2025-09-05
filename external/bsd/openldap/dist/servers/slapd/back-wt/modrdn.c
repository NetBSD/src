/*	$NetBSD: modrdn.c,v 1.1.1.1 2025/09/05 21:09:50 christos Exp $	*/

/* OpenLDAP WiredTiger backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2024 The OpenLDAP Foundation.
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
__RCSID("$NetBSD: modrdn.c,v 1.1.1.1 2025/09/05 21:09:50 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"

int
wt_modrdn( Operation *op, SlapReply *rs )
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	wt_ctx *wc = NULL;
	Entry *e = NULL;
	Entry *p = NULL;
	Entry *ne = NULL;
	Entry dummy = {0};

	struct berval p_dn, p_ndn;
	struct berval new_dn = {0, NULL}, new_ndn = {0, NULL};

	Entry *np = NULL;             /* newSuperior Entry */
	struct berval *np_dn = NULL;  /* newSuperior dn */
	struct berval *np_ndn = NULL; /* newSuperior ndn */
	struct berval *new_parent_dn = NULL; /* np_dn, p_dn, or NULL */

	int manageDSAit = get_manageDSAit( op );
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int	rc;

	int parent_is_glue = 0;
	int parent_is_leaf = 0;

	Debug( LDAP_DEBUG_TRACE, "==> wt_modrdn(%s -> newrdn=%s - newsup=%s)\n",
		   op->o_req_dn.bv_val,
		   op->oq_modrdn.rs_newrdn.bv_val,
		   op->oq_modrdn.rs_newSup?op->oq_modrdn.rs_newSup->bv_val:"NULL" );

	ctrls[num_ctrls] = NULL;

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	wc = wt_ctx_get(op, wi);
	if( !wc ){
		Debug( LDAP_DEBUG_ANY, "wt_modrdn: wt_ctx_get failed\n");
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		send_ldap_result( op, rs );
		return rs->sr_err;
	}

	/* get parent entry */
	if ( be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
		rs->sr_err = LDAP_NAMING_VIOLATION;
		rs->sr_text = "cannot rename suffix entry";
		goto return_results;
	} else {
		dnParent( &op->o_req_ndn, &p_ndn );
	}

	rc = wt_dn2entry(op->o_bd, wc, &p_ndn, &p);
	switch( rc ) {
	case 0:
		break;
	case WT_NOTFOUND:
		Debug( LDAP_DEBUG_ARGS,
			   "<== wt_modrdn: parent does not exist %s\n", p_ndn.bv_val);
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto return_results;
	default:
		Debug( LDAP_DEBUG_ANY,
			   "<== wt_modrdn: wt_dn2entry failed (%d)\n", rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* check parent for "children" acl */
	rc = access_allowed( op, p, children, NULL,
						 op->oq_modrdn.rs_newSup == NULL ?
						 ACL_WRITE : ACL_WDEL, NULL );

	if ( !rc ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		Debug( LDAP_DEBUG_TRACE,
			   "wt_modrdn: no access to parent\n");
		rs->sr_text = "no write access to old parent's children";
		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		   "wt_modrdn: wr to children of entry %s OK\n", p_ndn.bv_val );

	if ( p_ndn.bv_val == slap_empty_bv.bv_val ) {
		p_dn = slap_empty_bv;
	} else {
		dnParent( &op->o_req_dn, &p_dn );
	}

	Debug( LDAP_DEBUG_TRACE,
		   "wt_modrdn: parent dn=%s\n", p_dn.bv_val );

	/* get entry */
	rc = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
	switch( rc ) {
	case 0:
		break;
	case WT_NOTFOUND:
		break;
	default:
		Debug( LDAP_DEBUG_ANY,
			   "<== wt_modrdn: wt_dn2entry failed (%d)\n", rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	if ( rc == WT_NOTFOUND ||
		 ( !manageDSAit && e && is_entry_glue( e ) )) {

		if ( !e ) {
			Debug( LDAP_DEBUG_ARGS,
				   "<== wt_modrdn: no such object %s\n", op->o_req_dn.bv_val);
			rc = wt_dn2aentry(op->o_bd, wc, &op->o_req_ndn, &e);
			switch( rc ) {
			case 0:
				break;
			case WT_NOTFOUND:
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				goto return_results;
			default:
				Debug( LDAP_DEBUG_ANY, "wt_modrdn: wt_dn2aentry failed (%d)\n", rc );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error";
				goto return_results;
			}
		}

		rs->sr_matched = ch_strdup( e->e_dn );

		if ( is_entry_referral( e ) ) {
			BerVarray ref = get_entry_referrals( op, e );
			rs->sr_ref = referral_rewrite( ref, &e->e_name,
										   &op->o_req_dn, LDAP_SCOPE_DEFAULT );
			ber_bvarray_free( ref );
		} else {
			rs->sr_ref = NULL;
		}
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		rs->sr_err = LDAP_REFERRAL;
		send_ldap_result( op, rs );
		goto done;
	}

	if ( get_assert( op ) &&
		 ( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	/* check write on old entry */
	rc = access_allowed( op, e, entry, NULL, ACL_WRITE, NULL );
	if ( !rc ) {
		Debug( LDAP_DEBUG_TRACE, "wt_modrdn: no access to entry\n");
		rs->sr_text = "no write access to old entry";
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto return_results;
	}

	/* Can't do it if we have kids */
	rc = wt_dn2id_has_children( op, wc, e->e_id );
	if( rc != WT_NOTFOUND ) {
		switch( rc ) {
		case 0:
			Debug(LDAP_DEBUG_ARGS, "<== wt_modrdn: non-leaf %s\n", op->o_req_dn.bv_val);
			rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
			rs->sr_text = "subtree rename not supported";
			break;
		default:
			Debug(LDAP_DEBUG_ARGS,
				  "<== wt_modrdn: has_children failed: %s (%d)\n",
				  wiredtiger_strerror(rc), rc );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
		}
		goto return_results;
	}

	if (!manageDSAit && is_entry_referral( e ) ) {
		/* parent is a referral, don't allow add */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE,
			   "wt_modrdn: entry %s is referral\n", e->e_dn );

		rs->sr_err = LDAP_REFERRAL,
		rs->sr_matched = e->e_name.bv_val;
		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;
		goto done;
	}
	
	new_parent_dn = &p_dn;	/* New Parent unless newSuperior given */
	if ( op->oq_modrdn.rs_newSup != NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			   "wt_modrdn: new parent \"%s\" requested...\n",
			   op->oq_modrdn.rs_newSup->bv_val );

		/* newSuperior == oldParent? */
		if( dn_match( &p_ndn, op->oq_modrdn.rs_nnewSup ) ) {
			Debug( LDAP_DEBUG_TRACE,
				   "wt_modrdn: new parent \"%s\" same as the old parent \"%s\"\n",
				   op->oq_modrdn.rs_newSup->bv_val, p_dn.bv_val );
			op->oq_modrdn.rs_newSup = NULL; /* ignore newSuperior */
		}
	}

	if ( op->oq_modrdn.rs_newSup != NULL ) {
		if ( op->oq_modrdn.rs_newSup->bv_len ) {
			np_dn = op->oq_modrdn.rs_newSup;
			np_ndn = op->oq_modrdn.rs_nnewSup;

			/* newSuperior == oldParent? - checked above */
			/* newSuperior == entry being moved?, if so ==> ERROR */
			if ( dnIsSuffix( np_ndn, &e->e_nname )) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = "new superior not found";
				goto return_results;
			}
			/* Get Entry with dn=newSuperior. Does newSuperior exist? */
			rc = wt_dn2entry(op->o_bd, wc, np_ndn, &np);
			switch( rc ) {
			case 0:
				break;
			case WT_NOTFOUND:
				Debug( LDAP_DEBUG_ANY,
					   "<== wt_modrdn: new superior not found: %s\n",
					   np_ndn->bv_val );
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = "new superior not found";
				goto return_results;
			default:
				Debug( LDAP_DEBUG_ANY,
					   "<== wt_modrdn: wt_dn2entry failed %s (%d)\n",
					   wiredtiger_strerror(rc), rc );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error";
				goto return_results;
			}
			Debug( LDAP_DEBUG_TRACE,
				   "wt_modrdn: wr to new parent OK np=%p, id=%ld\n",
				   (void *) np, (long) np->e_id );
			rs->sr_err = access_allowed( op, np, children,
										 NULL, ACL_WADD, NULL );
			if( ! rs->sr_err ) {
				Debug( LDAP_DEBUG_TRACE,
					   "wt_modrdn: no wr to newSup children\n" );
				rs->sr_text = "no write access to new superior's children";
				rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				goto return_results;
			}
			if ( is_entry_alias( np ) ) {
				Debug( LDAP_DEBUG_TRACE,
					   "wt_modrdn: entry is alias\n" );
				rs->sr_text = "new superior is an alias";
				rs->sr_err = LDAP_ALIAS_PROBLEM;
				goto return_results;
			}
			if ( is_entry_referral( np ) ) {
				/* parent is a referral, don't allow add */
				Debug( LDAP_DEBUG_TRACE,
					   "wt_modrdn: entry is referral\n" );
				rs->sr_text = "new superior is a referral";
				rs->sr_err = LDAP_OTHER;
				goto return_results;
			}
		} else {
			/* no parent, modrdn entry directly under root */
			/* TODO: */
			Debug( LDAP_DEBUG_TRACE,
				   "wt_modrdn: no parent, not implement yet\n" );
			rs->sr_text = "not implement yet";
			rs->sr_err = LDAP_OTHER;
			goto return_results;
		}

		Debug( LDAP_DEBUG_TRACE,
			   "wt_modrdn: wr to new parent's children OK\n" );
		new_parent_dn = np_dn;
	}

	/* Build target dn and make sure target entry doesn't exist already. */
	if (!new_dn.bv_val) {
		build_new_dn( &new_dn, new_parent_dn, &op->oq_modrdn.rs_newrdn, NULL );
	}

	if (!new_ndn.bv_val) {
		struct berval bv = {0, NULL};
		dnNormalize( 0, NULL, NULL, &new_dn, &bv, op->o_tmpmemctx );
		ber_dupbv( &new_ndn, &bv );
		/* FIXME: why not call dnNormalize() w/o ctx? */
		op->o_tmpfree( bv.bv_val, op->o_tmpmemctx );
	}

	Debug( LDAP_DEBUG_TRACE,
		   "wt_modrdn: new ndn=%s\n", new_ndn.bv_val );

	/* check new entry */
	rc = wt_dn2entry(op->o_bd, wc, &new_ndn, &ne);
    switch( rc ) {
	case 0:
		/* Allow rename to same DN */
		if(e->e_id == ne->e_id){
			break;
		}
		rs->sr_err = LDAP_ALREADY_EXISTS;
		goto return_results;
		break;
	case WT_NOTFOUND:
		break;
	default:
		Debug( LDAP_DEBUG_ANY,
			   "<== wt_modrdn: wt_dn2entry failed %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	assert( op->orr_modlist != NULL );

	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, e,
								&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				   "<== wt_modrdn: pre-read failed!\n" );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* begin transaction */
	rc = wc->session->begin_transaction(wc->session, NULL);
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   "wt_modrdn: begin_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "begin_transaction failed";
		goto return_results;
	}
	wc->is_begin_transaction = 1;
	Debug( LDAP_DEBUG_TRACE,
		   "wt_modrdn: session id: %p\n", wc->session );

	/* delete old DN */
	rc = wt_dn2id_delete( op, wc, &e->e_nname);
	if ( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			  "<== wt_modrdn: delete failed: %s (%d)\n",
			  wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "dn2id delete failed";
		goto return_results;
	}

	/* copy the entry, then override some fields */
	dummy = *e;
	dummy.e_name = new_dn;
	dummy.e_nname = new_ndn;
	dummy.e_attrs = NULL;

	/* add new DN */
	rc = wt_dn2id_add( op, wc, np?np->e_id:p->e_id, &dummy );
	if ( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			  "<== wt_modrdn: add failed: %s (%d)\n",
			  wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN add failed";
		goto return_results;
	}
	dummy.e_attrs = e->e_attrs;

	rc = wt_modify_internal( op, wc, op->orm_modlist,
							 &dummy, &rs->sr_text, textbuf, textlen );
	if( rc != LDAP_SUCCESS ) {
		Debug(LDAP_DEBUG_TRACE,
			  "<== wt_modrdn: modify failed: %s (%d)\n",
			  wiredtiger_strerror(rc), rc );
		if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
		goto return_results;
	}

	/* update entry */
	rc = wt_id2entry_update( op, wc, &dummy );
	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			   "wt_modrdn: id2entry update failed(%d)\n", rc );
		if ( rc == LDAP_ADMINLIMIT_EXCEEDED ) {
			rs->sr_text = "entry too big";
		} else {
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "entry update failed";
		}
		goto return_results;
	}

	if ( p_ndn.bv_len != 0 ) {
		parent_is_glue = is_entry_glue(p);
		/* TODO: glue entry handling */
	}

	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &dummy,
								&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				   "<== wt_modrdn: post-read failed!\n" );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	if( op->o_noop ) {
		rs->sr_err = LDAP_X_NO_OPERATION;
		goto return_results;
	}

	rc = wc->session->commit_transaction(wc->session, NULL);
	wc->is_begin_transaction = 0;
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<== wt_modrdn: commit failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit failed";
		goto return_results;
	}

	Debug(LDAP_DEBUG_TRACE,
		  "wt_modrdn: rdn modified%s id=%08lx dn=\"%s\"\n",
		  op->o_noop ? " (no-op)" : "",
		  dummy.e_id, op->o_req_dn.bv_val );

	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if ( dummy.e_attrs ) {
		attrs_free( dummy.e_attrs );
	}
	send_ldap_result( op, rs );

	if ( rs->sr_err == LDAP_SUCCESS && parent_is_glue && parent_is_leaf ) {
		op->o_delete_glue_parent = 1;
	}

done:
	if( wc && wc->is_begin_transaction ){
		Debug( LDAP_DEBUG_TRACE, "wt_modrdn: rollback transaction\n" );
		wc->session->rollback_transaction(wc->session, NULL);
		wc->is_begin_transaction = 0;
	}

	slap_graduate_commit_csn( op );

	if( new_dn.bv_val != NULL ) free( new_dn.bv_val );
	if( new_ndn.bv_val != NULL ) free( new_ndn.bv_val );

	/* free entry */
	if( e != NULL ) {
		wt_entry_return( e );
	}
	/* free parent entry */
	if( p != NULL ) {
		wt_entry_return( p );
	}
	/* free new entry */
	if( ne != NULL ) {
		wt_entry_return( ne );
	}
	/* free new parent entry */
	if( np != NULL ) {
		wt_entry_return( np );
	}

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}
	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}
	return rs->sr_err;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
