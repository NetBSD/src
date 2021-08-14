/*	$NetBSD: modify.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: modify.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"

static struct berval scbva[] = {
	BER_BVC("glue"),
	BER_BVNULL
};

static void
wt_modify_idxflags(
	Operation *op,
	AttributeDescription *desc,
	int got_delete,
	Attribute *newattrs,
	Attribute *oldattrs )
{
	struct berval	ix_at;
	AttrInfo	*ai;

	/* check if modified attribute was indexed
	 * but not in case of NOOP... */
	ai = wt_index_mask( op->o_bd, desc, &ix_at );
	if ( ai ) {
		if ( got_delete ) {
			Attribute	*ap;
			struct berval	ix2;

			ap = attr_find( oldattrs, desc );
			if ( ap ) ap->a_flags |= SLAP_ATTR_IXDEL;

			/* Find all other attrs that index to same slot */
			for ( ap = newattrs; ap; ap = ap->a_next ) {
				ai = wt_index_mask( op->o_bd, ap->a_desc, &ix2 );
				if ( ai && ix2.bv_val == ix_at.bv_val )
					ap->a_flags |= SLAP_ATTR_IXADD;
			}

		} else {
			Attribute	*ap;

			ap = attr_find( newattrs, desc );
			if ( ap ) ap->a_flags |= SLAP_ATTR_IXADD;
		}
	}
}

int wt_modify_internal(
	Operation *op,
	wt_ctx *wc,
	Modifications *modlist,
	Entry *e,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	int rc, err;
	Modification	*mod;
	Modifications	*ml;
	Attribute	*save_attrs;
	Attribute	*ap;
	int			glue_attr_delete = 0;
	int			got_delete;

	Debug( LDAP_DEBUG_TRACE, "wt_modify_internal: 0x%08lx: %s\n",
		   e->e_id, e->e_dn );

	if ( !acl_check_modlist( op, e, modlist )) {
		return LDAP_INSUFFICIENT_ACCESS;
	}

	/* save_attrs will be disposed of by caller */
	save_attrs = e->e_attrs;
	e->e_attrs = attrs_dup( e->e_attrs );

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		int match;
		mod = &ml->sml_mod;
		switch( mod->sm_op ) {
		case LDAP_MOD_ADD:
		case LDAP_MOD_REPLACE:
			if ( mod->sm_desc == slap_schema.si_ad_structuralObjectClass ) {
				value_match( &match, slap_schema.si_ad_structuralObjectClass,
					slap_schema.si_ad_structuralObjectClass->
						ad_type->sat_equality,
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					&mod->sm_values[0], &scbva[0], text );
				if ( !match ) glue_attr_delete = 1;
			}
		}
		if ( glue_attr_delete )
			break;
	}

	if ( glue_attr_delete ) {
		Attribute	**app = &e->e_attrs;
		while ( *app != NULL ) {
			if ( !is_at_operational( (*app)->a_desc->ad_type )) {
				Attribute *save = *app;
				*app = (*app)->a_next;
				attr_free( save );
				continue;
			}
			app = &(*app)->a_next;
		}
	}

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		mod = &ml->sml_mod;
		got_delete = 0;

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: add %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			err = modify_add_values( e, mod, get_permissiveModify(op),
									 text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "wt_modify_internal: %d %s\n",
					  err, *text );
			}
			break;

		case LDAP_MOD_DELETE:
			if ( glue_attr_delete ) {
				err = LDAP_SUCCESS;
				break;
			}

			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: delete %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			err = modify_delete_values( e, mod, get_permissiveModify(op),
										text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS,
					  "wt_modify_internal: %d %s\n", err, *text );
			} else {
				got_delete = 1;
			}
			break;

		case LDAP_MOD_REPLACE:
			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: replace %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			err = modify_replace_values( e, mod, get_permissiveModify(op),
										 text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS,
					  "wt_modify_internal: %d %s\n", err, *text );
			} else {
				got_delete = 1;
			}
			break;

		case LDAP_MOD_INCREMENT:
			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: increment %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			err = modify_increment_values( e, mod, get_permissiveModify(op),
										   text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS,
					  "wt_modify_internal: %d %s\n",
					  err, *text );
			} else {
				got_delete = 1;
			}
			break;

		case SLAP_MOD_SOFTADD:
			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: softadd %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			/* Avoid problems in index_add_mods()
			 * We need to add index if necessary.
			 */
			mod->sm_op = LDAP_MOD_ADD;

			err = modify_add_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

			mod->sm_op = SLAP_MOD_SOFTADD;

			if ( err == LDAP_TYPE_OR_VALUE_EXISTS ) {
				err = LDAP_SUCCESS;
			}

			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "wt_modify_internal: %d %s\n",
					err, *text );
			}
			break;

		case SLAP_MOD_SOFTDEL:
			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: softdel %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			/* Avoid problems in index_delete_mods()
			 * We need to add index if necessary.
			 */
			mod->sm_op = LDAP_MOD_DELETE;

			err = modify_delete_values( e, mod, get_permissiveModify(op),
										text, textbuf, textlen );

			mod->sm_op = SLAP_MOD_SOFTDEL;

			if ( err == LDAP_SUCCESS ) {
				got_delete = 1;
			} else if ( err == LDAP_NO_SUCH_ATTRIBUTE ) {
				err = LDAP_SUCCESS;
			}

			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "wt_modify_internal: %d %s\n",
					  err, *text );
			}
			break;

		case SLAP_MOD_ADD_IF_NOT_PRESENT:
			if ( attr_find( e->e_attrs, mod->sm_desc ) != NULL ) {
				/* skip */
				err = LDAP_SUCCESS;
				break;
			}

			Debug(LDAP_DEBUG_ARGS,
				  "wt_modify_internal: add_if_not_present %s\n",
				  mod->sm_desc->ad_cname.bv_val );
			/* Avoid problems in index_add_mods()
			 * We need to add index if necessary.
			 */
			mod->sm_op = LDAP_MOD_ADD;

			err = modify_add_values( e, mod, get_permissiveModify(op),
									 text, textbuf, textlen );

			mod->sm_op = SLAP_MOD_ADD_IF_NOT_PRESENT;

			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "wt_modify_internal: %d %s\n",
					  err, *text );
			}
			break;

		default:
			Debug(LDAP_DEBUG_ANY, "wt_modify_internal: invalid op %d\n",
				  mod->sm_op );
			*text = "Invalid modify operation";
			err = LDAP_OTHER;
			Debug(LDAP_DEBUG_ARGS, "wt_modify_internal: %d %s\n",
				  err, *text );
		}

		if ( err != LDAP_SUCCESS ) {
			attrs_free( e->e_attrs );
			e->e_attrs = save_attrs;
			/* unlock entry, delete from cache */
			return err;
		}

		/* If objectClass was modified, reset the flags */
		if ( mod->sm_desc == slap_schema.si_ad_objectClass ) {
			e->e_ocflags = 0;
		}

		if ( glue_attr_delete ) e->e_ocflags = 0;


		/* check if modified attribute was indexed
		 * but not in case of NOOP... */
		if ( !op->o_noop ) {
			wt_modify_idxflags( op, mod->sm_desc, got_delete, e->e_attrs, save_attrs );
		}

	}

	/* check that the entry still obeys the schema */
	ap = NULL;
	rc = entry_schema_check( op, e, save_attrs, get_relax(op), 0, &ap,
		text, textbuf, textlen );
	if ( rc != LDAP_SUCCESS || op->o_noop ) {
		attrs_free( e->e_attrs );
		/* clear the indexing flags */
		for ( ap = save_attrs; ap != NULL; ap = ap->a_next ) {
			ap->a_flags &= ~(SLAP_ATTR_IXADD|SLAP_ATTR_IXDEL);
		}
		e->e_attrs = save_attrs;

		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"entry failed schema check: %s\n",
				*text );
		}

		/* if NOOP then silently revert to saved attrs */
		return rc;
	}

	/* structuralObjectClass modified! */
	if ( ap ) {
		assert( ap->a_desc == slap_schema.si_ad_structuralObjectClass );
		if ( !op->o_noop ) {
			wt_modify_idxflags( op, slap_schema.si_ad_structuralObjectClass,
								1, e->e_attrs, save_attrs );
		}
	}

	/* update the indices of the modified attributes */

	/* start with deleting the old index entries */
	for ( ap = save_attrs; ap != NULL; ap = ap->a_next ) {
		if ( ap->a_flags & SLAP_ATTR_IXDEL ) {
			struct berval *vals;
			Attribute *a2;
			ap->a_flags &= ~SLAP_ATTR_IXDEL;
			a2 = attr_find( e->e_attrs, ap->a_desc );
			if ( a2 ) {
				/* need to detect which values were deleted */
				int i, j;
				/* let add know there were deletes */
				if ( a2->a_flags & SLAP_ATTR_IXADD )
					a2->a_flags |= SLAP_ATTR_IXDEL;
				vals = op->o_tmpalloc( (ap->a_numvals + 1) *
					sizeof(struct berval), op->o_tmpmemctx );
				j = 0;
				for ( i=0; i < ap->a_numvals; i++ ) {
					rc = attr_valfind( a2, SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
						&ap->a_nvals[i], NULL, op->o_tmpmemctx );
					/* Save deleted values */
					if ( rc == LDAP_NO_SUCH_ATTRIBUTE )
						vals[j++] = ap->a_nvals[i];
				}
				BER_BVZERO(vals+j);
			} else {
				/* attribute was completely deleted */
				vals = ap->a_nvals;
			}
			rc = 0;
			if ( !BER_BVISNULL( vals )) {
				rc = wt_index_values( op, wc, ap->a_desc,
									  vals, e->e_id, SLAP_INDEX_DELETE_OP );
				if ( rc != LDAP_SUCCESS ) {
					Debug( LDAP_DEBUG_ANY,
						   "%s: attribute \"%s\" index delete failure\n",
						   op->o_log_prefix, ap->a_desc->ad_cname.bv_val );
					attrs_free( e->e_attrs );
					e->e_attrs = save_attrs;
				}
			}
			if ( vals != ap->a_nvals )
				op->o_tmpfree( vals, op->o_tmpmemctx );
			if ( rc ) return rc;
		}
	}

	/* add the new index entries */
	for ( ap = e->e_attrs; ap != NULL; ap = ap->a_next ) {
		if (ap->a_flags & SLAP_ATTR_IXADD) {
			ap->a_flags &= ~SLAP_ATTR_IXADD;
			if ( ap->a_flags & SLAP_ATTR_IXDEL ) {
				/* if any values were deleted, we must readd index
				 * for all remaining values.
				 */
				ap->a_flags &= ~SLAP_ATTR_IXDEL;
				rc = wt_index_values( op, wc, ap->a_desc, ap->a_nvals,
									  e->e_id, SLAP_INDEX_ADD_OP );
			} else {
				int found = 0;
				/* if this was only an add, we only need to index
				 * the added values.
				 */
				for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
					struct berval *vals;
					if ( ml->sml_desc != ap->a_desc || !ml->sml_numvals )
						continue;
					found = 1;
					switch( ml->sml_op ) {
					case LDAP_MOD_ADD:
					case LDAP_MOD_REPLACE:
					case LDAP_MOD_INCREMENT:
					case SLAP_MOD_SOFTADD:
					case SLAP_MOD_ADD_IF_NOT_PRESENT:
						if ( ml->sml_op == LDAP_MOD_INCREMENT )
							vals = ap->a_nvals;
						else if ( ml->sml_nvalues )
							vals = ml->sml_nvalues;
						else
							vals = ml->sml_values;
						rc = wt_index_values( op, wc, ap->a_desc,
											  vals, e->e_id, SLAP_INDEX_ADD_OP );
						break;
					}
					if ( rc )
						break;
				}
				/* This attr was affected by a modify of a subtype, so
				 * there was no direct match in the modlist. Just readd
				 * all of its values.
				 */
				if ( !found ) {
					rc = wt_index_values( op, wc, ap->a_desc, ap->a_nvals,
										  e->e_id, SLAP_INDEX_ADD_OP );
				}
			}
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
				       "%s: attribute \"%s\" index add failure\n",
					   op->o_log_prefix, ap->a_desc->ad_cname.bv_val );
				attrs_free( e->e_attrs );
				e->e_attrs = save_attrs;
				return rc;
			}
		}
	}

	return rc;
}

int
wt_modify( Operation *op, SlapReply *rs )
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	wt_ctx *wc = NULL;
	Entry		*e = NULL;
	int		manageDSAit = get_manageDSAit( op );
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	Entry		dummy = {0};

	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int rc;

	Debug( LDAP_DEBUG_ARGS, LDAP_XSTRING(wt_modify) ": %s\n",
		   op->o_req_dn.bv_val );

	if( op->o_txnSpec && txn_preop( op, rs ))
		return rs->sr_err;

	ctrls[num_ctrls] = NULL;

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

	/* Don't touch the opattrs, if this is a contextCSN update
	 * initiated from updatedn */
	if ( !be_isupdate(op) || !op->orm_modlist || op->orm_modlist->sml_next ||
		 op->orm_modlist->sml_desc != slap_schema.si_ad_contextCSN ) {

		slap_mods_opattrs( op, &op->orm_modlist, 1 );
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
		/* TODO: lookup referrals */
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto return_results;
	default:
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_modify)
			   ": wt_dn2entry failed (%d)\n",
			   rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	if ( !manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow modify */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_modify) ": entry is referral\n" );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_matched = e->e_name.bv_val;
		rs->sr_flags = REP_REF_MUSTBEFREED;
		send_ldap_result( op, rs );
		rs->sr_matched = NULL;
		goto done;
	}

	if ( get_assert( op ) &&
		 ( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if ( slap_read_controls( op, rs, e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(wt_modify) ": pre-read "
				"failed!\n" );
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
			   LDAP_XSTRING(wt_add) ": begin_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "begin_transaction failed";
		goto return_results;
	}
	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(wt_modify) ": session id: %p\n",
		   wc->session );

	/* Modify the entry */
	dummy = *e;
	rs->sr_err = wt_modify_internal( op, wc, op->orm_modlist,
									 &dummy, &rs->sr_text, textbuf, textlen );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_modify) ": modify failed (%d)\n",
			   rs->sr_err );
		/* Only free attrs if they were dup'd.  */
		if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
		goto return_results;
	}

	/* change the entry itself */
	rs->sr_err = wt_id2entry_update( op, wc->session, &dummy );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			   LDAP_XSTRING(wt_modify) ": id2entry update failed " "(%d)\n",
			   rs->sr_err );
		if ( rs->sr_err == LDAP_ADMINLIMIT_EXCEEDED ) {
			rs->sr_text = "entry too big";
		} else {
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "entry update failed";
		}
		goto return_results;
	}

	if( op->o_noop ) {
		wc->session->rollback_transaction(wc->session, NULL);
		rs->sr_err = LDAP_X_NO_OPERATION;
		goto return_results;
	}

	/* Only free attrs if they were dup'd.  */
	if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;

	rc = wc->session->commit_transaction(wc->session, NULL);
	if( rc ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<== " LDAP_XSTRING(wt_modify)
			   ": commit failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit failed";
		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		   LDAP_XSTRING(wt_modify) ": updated%s id=%08lx dn=\"%s\"\n",
		   op->o_noop ? " (no-op)" : "",
		   dummy.e_id, op->o_req_dn.bv_val );

	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &dummy,
								&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				   "<=- " LDAP_XSTRING(wt_modify)
				   ": post-read failed!\n" );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}
	if( num_ctrls ) rs->sr_ctrls = ctrls;

	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;

return_results:
	if( dummy.e_attrs ) {
		attrs_free( dummy.e_attrs );
	}
	send_ldap_result( op, rs );

done:
	slap_graduate_commit_csn( op );

	if( e != NULL ) {
		wt_entry_return( e );
	}

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}
	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}

	rs->sr_text = NULL;

	return rs->sr_err;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
