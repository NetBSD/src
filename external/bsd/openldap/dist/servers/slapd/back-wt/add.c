/*	$NetBSD: add.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: add.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"

int
wt_add( Operation *op, SlapReply *rs )
{
    struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	struct berval   pdn;
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	ID eid;
	int num_retries = 0;
	int success;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;
	wt_ctx *wc;
	Entry *e = NULL;
	Entry *p = NULL;
	ID pid;
	int rc;

    Debug( LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(wt_add) ": %s\n",
		   op->ora_e->e_name.bv_val );

	ctrls[num_ctrls] = 0;

	/* check entry's schema */
	rs->sr_err = entry_schema_check(
		op, op->ora_e, NULL,
		get_relax(op), 1, NULL, &rs->sr_text, textbuf, textlen );
    if ( rs->sr_err != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add)
			   ": entry failed schema check: %s (%d)\n",
			   rs->sr_text, rs->sr_err );
        goto return_results;
    }

    /* add opattrs to shadow as well, only missing attrs will actually
     * be added; helps compatibility with older OL versions */
    rs->sr_err = slap_add_opattrs( op, &rs->sr_text, textbuf, textlen, 1 );
    if ( rs->sr_err != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add)
			   ": entry failed op attrs add: %s (%d)\n",
			   rs->sr_text, rs->sr_err );
        goto return_results;
    }

    if ( get_assert( op ) &&
		 ( test_filter( op, op->ora_e, get_assertion( op ))
		   != LDAP_COMPARE_TRUE ))
    {
        rs->sr_err = LDAP_ASSERTION_FAILED;
        goto return_results;
    }

	/* Not used
	 * subentry = is_entry_subentry( op->ora_e );
	 */

    /*
     * Get the parent dn and see if the corresponding entry exists.
     */
    if ( be_issuffix( op->o_bd, &op->ora_e->e_nname ) ) {
        pdn = slap_empty_bv;
    } else {
        dnParent( &op->ora_e->e_nname, &pdn );
    }

	wc = wt_ctx_get(op, wi);
	if( !wc ){
        Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_add)
			   ": wt_ctx_get failed\n" );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
        send_ldap_result( op, rs );
        return rs->sr_err;
	}

	rc = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
	switch( rc ) {
	case 0:
		rs->sr_err = LDAP_ALREADY_EXISTS;
		goto return_results;
		break;
	case WT_NOTFOUND:
		break;
	default:
		/* TODO: retry handling */
        Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_add)
			   ": error at wt_dn2entry() rc=%d\n",
			   rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

    /* get parent entry */
	rc = wt_dn2pentry(op->o_bd, wc, &op->o_req_ndn, &p);
	switch( rc ){
	case 0:
	case WT_NOTFOUND:
		break;
	default:
        Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_add)
			   ": error at wt_dn2pentry() rc=%d\n",
			   rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	if ( !p )
		p = (Entry *)&slap_entry_root;

	if ( !bvmatch( &pdn, &p->e_nname ) ) {
		rs->sr_matched = ber_strdup_x( p->e_name.bv_val,
									   op->o_tmpmemctx );
		if ( p != (Entry *)&slap_entry_root ) {
			rs->sr_ref = is_entry_referral( p )
				? get_entry_referrals( op, p )
				: NULL;
			wt_entry_return( p );
		} else {
			rs->sr_ref = NULL;
		}
		p = NULL;
        Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add) ": parent "
			   "does not exist\n" );
        rs->sr_err = LDAP_REFERRAL;
        rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
        goto return_results;
	}

	rs->sr_err = access_allowed( op, p,
								 children, NULL, ACL_WADD, NULL );
	if ( ! rs->sr_err ) {
		/*
		if ( p != (Entry *)&slap_entry_root )
			wt_entry_return( op, p );
		*/
		p = NULL;

		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add) ": no write access to parent\n" );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to parent";
		goto return_results;;
	}

	if ( p != (Entry *)&slap_entry_root ) {
		if ( is_entry_subentry( p ) ) {
			wt_entry_return( p );
			p = NULL;
			/* parent is a subentry, don't allow add */
			Debug( LDAP_DEBUG_TRACE,
				   LDAP_XSTRING(wt_add) ": parent is subentry\n" );
			rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			rs->sr_text = "parent is a subentry";
			goto return_results;;
		}

		if ( is_entry_alias( p ) ) {
			wt_entry_return( p );
			p = NULL;
			/* parent is an alias, don't allow add */
			Debug( LDAP_DEBUG_TRACE,
				   LDAP_XSTRING(wt_add) ": parent is alias\n" );
			rs->sr_err = LDAP_ALIAS_PROBLEM;
			rs->sr_text = "parent is an alias";
			goto return_results;;
		}

		if ( is_entry_referral( p ) ) {
			BerVarray ref = get_entry_referrals( op, p );
			/* parent is a referral, don't allow add */
			rs->sr_matched = ber_strdup_x( p->e_name.bv_val,
										   op->o_tmpmemctx );
			rs->sr_ref = referral_rewrite( ref, &p->e_name,
										   &op->o_req_dn, LDAP_SCOPE_DEFAULT );
			ber_bvarray_free( ref );
			wt_entry_return( p );
			p = NULL;
			Debug( LDAP_DEBUG_TRACE,
				   LDAP_XSTRING(wt_add) ": parent is referral\n" );

			rs->sr_err = LDAP_REFERRAL;
			rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
			goto return_results;
		}
	}

#if 0
	if ( subentry ) {
		/* FIXME: */
		/* parent must be an administrative point of the required kind */
	}
#endif

	/* free parent */
	if ( p != (Entry *)&slap_entry_root ) {
		pid = p->e_id;
		if ( p->e_nname.bv_len ) {
			struct berval ppdn;

			/* ITS#5326: use parent's DN if differs from provided one */
			dnParent( &op->ora_e->e_name, &ppdn );
			if ( !dn_match( &p->e_name, &ppdn ) ) {
				struct berval rdn;
				struct berval newdn;

				dnRdn( &op->ora_e->e_name, &rdn );

				build_new_dn( &newdn, &p->e_name, &rdn, NULL );
				if ( op->ora_e->e_name.bv_val != op->o_req_dn.bv_val )
					ber_memfree( op->ora_e->e_name.bv_val );
				op->ora_e->e_name = newdn;

				/* FIXME: should check whether
                 * dnNormalize(newdn) == e->e_nname ... */
			}
		}

		wt_entry_return( p );
	}
	p = NULL;

	rs->sr_err = access_allowed( op, op->ora_e,
								 entry, NULL, ACL_WADD, NULL );

	if ( ! rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add) ": no write access to entry\n" );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;
	}

	/*
	 * Check ACL for attribute write access
	 */
	if (!acl_check_modlist(op, op->ora_e, op->ora_modlist)) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add) ": no write access to attribute\n" );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to attribute";
		goto return_results;
	}

	rc = wc->session->begin_transaction(wc->session, NULL);
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add) ": begin_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "begin_transaction failed";
		goto return_results;
	}
	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(wt_add) ": session id: %p\n",
		   wc->session );

	wt_next_id( op->o_bd, &eid );
	op->ora_e->e_id = eid;

	rc = wt_dn2id_add( op, wc->session, pid, op->ora_e );
	if( rc ){
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add)
			   ": dn2id_add failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		switch( rc ) {
		case WT_DUPLICATE_KEY:
			rs->sr_err = LDAP_ALREADY_EXISTS;
			break;
		default:
			rs->sr_err = LDAP_OTHER;
		}
		wc->session->rollback_transaction(wc->session, NULL);
		goto return_results;
	}

	rc = wt_id2entry_add( op, wc->session, op->ora_e );
	if ( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_add)
			   ": id2entry_add failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		if ( rc == LDAP_ADMINLIMIT_EXCEEDED ) {
			rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
			rs->sr_text = "entry is too big";
		} else {
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "entry store failed";
		}
		wc->session->rollback_transaction(wc->session, NULL);
		goto return_results;
	}

	/* add indices */
	rc = wt_index_entry_add( op, wc, op->ora_e );
	if ( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			  "<== " LDAP_XSTRING(wt_add)
			  ": index add failed: %s (%d)\n",
			  wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "index add failed";
		wc->session->rollback_transaction(wc->session, NULL);
		goto return_results;
	}

	rc = wc->session->commit_transaction(wc->session, NULL);
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<== " LDAP_XSTRING(wt_add)
			   ": commit_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit_transaction failed";
		goto return_results;
	}

	rs->sr_err = LDAP_SUCCESS;

	/* post-read */
	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if ( slap_read_controls( op, rs, op->ora_e,
								 &slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				   "<=- " LDAP_XSTRING(wt_add) ": post-read "
				   "failed!\n" );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
                 * operation if control fails? */
				goto return_results;
			}
		}
	}

	Debug(LDAP_DEBUG_TRACE,
		  LDAP_XSTRING(wt_add) ": added%s id=%08lx dn=\"%s\"\n",
		  op->o_noop ? " (no-op)" : "",
		  op->ora_e->e_id, op->ora_e->e_dn );

return_results:
	success = rs->sr_err;
	send_ldap_result( op, rs );

	slap_graduate_commit_csn( op );

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
