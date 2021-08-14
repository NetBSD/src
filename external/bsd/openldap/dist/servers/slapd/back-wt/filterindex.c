/*	$NetBSD: filterindex.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: filterindex.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "idl.h"

static int
presence_candidates(
	Operation *op,
	wt_ctx *wc,
	AttributeDescription *desc,
	ID *ids )
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	int rc;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_presence_candidates (%s)\n",
		   desc->ad_cname.bv_val );

	WT_IDL_ALL( wi, ids );

	if( desc == slap_schema.si_ad_objectClass ) {
		return 0;
	}

	rc = wt_index_param( op->o_bd, desc, LDAP_FILTER_PRESENT,
						 &mask, &prefix );

	if( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		/* not indexed */
		Debug( LDAP_DEBUG_FILTER,
			   "<= wt_presence_candidates: (%s) not indexed\n",
			   desc->ad_cname.bv_val );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_presence_candidates: (%s) index_param "
			   "returned=%d\n",
			   desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( prefix.bv_val == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_presence_candidates: (%s) no prefix\n",
			   desc->ad_cname.bv_val );
		return -1;
	}

	/* open index cursor */
	cursor = wt_ctx_index_cursor(wc, &desc->ad_type->sat_cname, 0);
	if( !cursor ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_presence_candidates: open index cursor failed: %s\n",
			   desc->ad_type->sat_cname.bv_val );
		return 0;
	}

	rc = wt_key_read( op->o_bd, cursor, &prefix, ids, NULL, 0 );

	if(cursor){
		cursor->close(cursor);
	}
	Debug(LDAP_DEBUG_TRACE,
		  "<= wt_presence_candidates: id=%ld first=%ld last=%ld\n",
		  (long) ids[0],
		  (long) WT_IDL_FIRST(ids),
		  (long) WT_IDL_LAST(ids) );

	return 0;
}

static int
equality_candidates(
	Operation *op,
	wt_ctx *wc,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp)
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	int i;
	int rc;
	MatchingRule *mr;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_equality_candidates (%s)\n",
		   ava->aa_desc->ad_cname.bv_val );

	if ( ava->aa_desc == slap_schema.si_ad_entryDN ) {
		ID id = NOID;
		rc = wt_dn2id(op, wc->session, &ava->aa_value, &id);
		if( rc == 0 ){
			wt_idl_append_one(ids, id);
		}else if ( rc == WT_NOTFOUND ) {
			WT_IDL_ZERO( ids );
			rc = 0;
		}
		return rc;
	}

	WT_IDL_ALL( wi, ids );

	rc = wt_index_param( op->o_bd, ava->aa_desc, LDAP_FILTER_EQUALITY,
						 &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_FILTER,
			   "<= wt_equality_candidates: (%s) not indexed\n",
			   ava->aa_desc->ad_cname.bv_val );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_equality_candidates: (%s) index_param failed (%d)\n",
			   ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	mr = ava->aa_desc->ad_type->sat_equality;
	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_EQUALITY,
		mask,
		ava->aa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		&ava->aa_value,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_equality_candidates: (%s, %s) "
			   "MR filter failed (%d)\n",
			   prefix.bv_val, ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_equality_candidates: (%s) no keys\n",
			   ava->aa_desc->ad_cname.bv_val );
		return 0;
	}

	/* open index cursor */
	cursor = wt_ctx_index_cursor(wc, &ava->aa_desc->ad_type->sat_cname, 0);
	if( !cursor ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_equality_candidates: open index cursor failed: %s\n",
			   ava->aa_desc->ad_type->sat_cname.bv_val );
		return 0;
	}

	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = wt_key_read( op->o_bd, cursor, &keys[i], tmp, NULL, 0 );
		if( rc == WT_NOTFOUND ) {
			WT_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<= wt_equality_candidates: (%s) "
				   "key read failed (%d)\n",
				   ava->aa_desc->ad_cname.bv_val, rc );
			break;
		}
		if ( i == 0 ) {
			WT_IDL_CPY( ids, tmp );
		} else {
			wt_idl_intersection( ids, tmp );
		}

		if( WT_IDL_IS_ZERO( ids ) )
			break;
	}

	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	if(cursor){
		cursor->close(cursor);
	}

	Debug( LDAP_DEBUG_TRACE,
		   "<= wt_equality_candidates: id=%ld, first=%ld, last=%ld\n",
		   (long) ids[0],
		   (long) WT_IDL_FIRST(ids),
		   (long) WT_IDL_LAST(ids) );

	return rc;
}

static int
approx_candidates(
	Operation *op,
	wt_ctx *wc,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp )
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	int i;
	int rc;
    slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_approx_candidates (%s)\n",
		   ava->aa_desc->ad_cname.bv_val );

	WT_IDL_ALL( wi, ids );

	rc = wt_index_param( op->o_bd, ava->aa_desc, LDAP_FILTER_APPROX,
						 &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_FILTER,
			   "<= wt_approx_candidates: (%s) not indexed\n",
			   ava->aa_desc->ad_cname.bv_val );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_approx_candidates: (%s) index_param failed (%d)\n",
			   ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	mr = ava->aa_desc->ad_type->sat_approx;
	if( !mr ) {
		/* no approx matching rule, try equality matching rule */
		mr = ava->aa_desc->ad_type->sat_equality;
	}

	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_APPROX,
		mask,
		ava->aa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		&ava->aa_value,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_approx_candidates: (%s, %s) MR filter failed (%d)\n",
			   prefix.bv_val, ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_approx_candidates: (%s) no keys (%s)\n",
			   prefix.bv_val, ava->aa_desc->ad_cname.bv_val );
		return 0;
	}

	/* open index cursor */
	cursor = wt_ctx_index_cursor(wc, &ava->aa_desc->ad_type->sat_cname, 0);
	if( !cursor ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_approx_candidates: open index cursor failed: %s\n",
			   ava->aa_desc->ad_type->sat_cname.bv_val );
		return 0;
	}

	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = wt_key_read( op->o_bd, cursor, &keys[i], tmp, NULL, 0 );
		if( rc == WT_NOTFOUND ) {
			WT_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<= wt_approx_candidates: (%s) key read failed (%d)\n",
				   ava->aa_desc->ad_cname.bv_val, rc );
			break;
		}

		if( WT_IDL_IS_ZERO( tmp ) ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<= wt_approx_candidates: (%s) NULL\n",
				   ava->aa_desc->ad_cname.bv_val );
			WT_IDL_ZERO( ids );
			break;
		}

		if ( i == 0 ) {
			WT_IDL_CPY( ids, tmp );
		} else {
			wt_idl_intersection( ids, tmp );
		}

		if( WT_IDL_IS_ZERO( ids ) )
			break;
	}

	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	if(cursor){
		cursor->close(cursor);
	}

	Debug( LDAP_DEBUG_TRACE,
		   "<= wt_approx_candidates %ld, first=%ld, last=%ld\n",
		   (long) ids[0],
		   (long) WT_IDL_FIRST(ids),
		   (long) WT_IDL_LAST(ids) );

	return rc;
}

static int
substring_candidates(
	Operation *op,
	wt_ctx *wc,
	SubstringsAssertion *sub,
	ID *ids,
	ID *tmp )
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	int i;
	int rc;
    slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_substring_candidates (%s)\n",
		   sub->sa_desc->ad_cname.bv_val );

	WT_IDL_ALL( wi, ids );

	rc = wt_index_param( op->o_bd, sub->sa_desc, LDAP_FILTER_SUBSTRINGS,
						 &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_FILTER,
			   "<= wt_substring_candidates: (%s) not indexed\n",
			   sub->sa_desc->ad_cname.bv_val );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_substring_candidates: (%s) "
			   "index_param failed (%d)\n",
			   sub->sa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	mr = sub->sa_desc->ad_type->sat_substr;

	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_SUBSTRINGS,
		mask,
		sub->sa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		sub,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_substring_candidates: (%s) MR filter failed (%d)\n",
			   sub->sa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			   "<= wt_substring_candidates: (0x%04lx) no keys (%s)\n",
			   mask, sub->sa_desc->ad_cname.bv_val );
		return 0;
	}

	/* open index cursor */
	cursor = wt_ctx_index_cursor(wc, &sub->sa_desc->ad_cname, 0);
	if( !cursor ) {
		Debug( LDAP_DEBUG_ANY,
			   "<= wt_substring_candidates: open index cursor failed: %s\n",
			   sub->sa_desc->ad_cname.bv_val );
		return 0;
	}

	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = wt_key_read( op->o_bd, cursor, &keys[i], tmp, NULL, 0 );

		if( rc == WT_NOTFOUND ) {
			WT_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<= wt_substring_candidates: (%s) key read failed (%d)\n",
				   sub->sa_desc->ad_cname.bv_val, rc );
			break;
		}

		if( WT_IDL_IS_ZERO( tmp ) ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<= wt_substring_candidates: (%s) NULL\n",
				   sub->sa_desc->ad_cname.bv_val );
			WT_IDL_ZERO( ids );
			break;
		}

		if ( i == 0 ) {
			WT_IDL_CPY( ids, tmp );
		} else {
			wt_idl_intersection( ids, tmp );
		}

		if( WT_IDL_IS_ZERO( ids ) )
			break;
	}

	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	if(cursor){
		cursor->close(cursor);
	}

	Debug( LDAP_DEBUG_TRACE,
		   "<= wt_substring_candidates: %ld, first=%ld, last=%ld\n",
		   (long) ids[0],
		   (long) WT_IDL_FIRST(ids),
		   (long) WT_IDL_LAST(ids));
	return rc;
}


static int
list_candidates(
	Operation *op,
	wt_ctx *wc,
	Filter *flist,
	int ftype,
	ID *ids,
	ID *tmp,
	ID *save )
{
	int rc = 0;
	Filter  *f;

	Debug( LDAP_DEBUG_FILTER, "=> wt_list_candidates 0x%x\n", ftype );
	for ( f = flist; f != NULL; f = f->f_next ) {
		/* ignore precomputed scopes */
		if ( f->f_choice == SLAPD_FILTER_COMPUTED &&
			 f->f_result == LDAP_SUCCESS ) {
			continue;
		}
		WT_IDL_ZERO( save );
		rc = wt_filter_candidates( op, wc, f, save, tmp,
								   save+WT_IDL_UM_SIZE );

		if ( rc != 0 ) {
			/* TODO: error handling */
			/*
			if ( rc == DB_LOCK_DEADLOCK )
				return rc;
			*/
			if ( ftype == LDAP_FILTER_AND ) {
				rc = 0;
				continue;
			}
			break;
		}


		if ( ftype == LDAP_FILTER_AND ) {
			if ( f == flist ) {
				WT_IDL_CPY( ids, save );
			} else {
				wt_idl_intersection( ids, save );
			}
			if( WT_IDL_IS_ZERO( ids ) )
				break;
		} else {
			if ( f == flist ) {
				WT_IDL_CPY( ids, save );
			} else {
				wt_idl_union( ids, save );
			}
		}
	}

	if( rc == LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_FILTER,
			   "<= wt_list_candidates: id=%ld first=%ld last=%ld\n",
			   (long) ids[0],
			   (long) WT_IDL_FIRST(ids),
			   (long) WT_IDL_LAST(ids) );

	} else {
		Debug( LDAP_DEBUG_FILTER,
			   "<= wt_list_candidates: undefined rc=%d\n",
			   rc );
	}

	return 0;
}

int
wt_filter_candidates(
	Operation *op,
	wt_ctx *wc,
	Filter *f,
	ID *ids,
	ID *tmp,
	ID *stack )
{
	struct wt_info *wi = (struct wt_info *)op->o_bd->be_private;
	int rc = 0;
	Debug( LDAP_DEBUG_FILTER, "=> wt_filter_candidates\n" );

	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		WT_IDL_ZERO( ids );
		goto done;
	}

	switch ( f->f_choice ) {
	case SLAPD_FILTER_COMPUTED:
		switch( f->f_result ) {
		case SLAPD_COMPARE_UNDEFINED:
			/* This technically is not the same as FALSE, but it
			 * certainly will produce no matches.
			 */
			/* FALL THRU */
		case LDAP_COMPARE_FALSE:
			WT_IDL_ZERO( ids );
			break;
		case LDAP_COMPARE_TRUE: {

			WT_IDL_ALL( wi, ids );
		} break;
		case LDAP_SUCCESS:
			/* this is a pre-computed scope, leave it alone */
			break;
		}
		break;
	case LDAP_FILTER_PRESENT:
		Debug( LDAP_DEBUG_FILTER, "\tPRESENT\n" );
		rc = presence_candidates( op, wc, f->f_desc, ids );
		break;

	case LDAP_FILTER_EQUALITY:
		Debug( LDAP_DEBUG_FILTER, "\tEQUALITY\n" );
		rc = equality_candidates( op, wc, f->f_ava, ids, tmp );
		break;

	case LDAP_FILTER_APPROX:
		Debug( LDAP_DEBUG_FILTER, "\tAPPROX\n" );
		rc = approx_candidates( op, wc, f->f_ava, ids, tmp );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		Debug( LDAP_DEBUG_FILTER, "\tSUBSTRINGS\n" );
		rc = substring_candidates( op, wc, f->f_sub, ids, tmp );
		break;

	case LDAP_FILTER_GE:
		/* if no GE index, use pres */
		/* TODO: not implement yet */
		rc = presence_candidates( op, wc, f->f_ava->aa_desc, ids );
		break;

    case LDAP_FILTER_LE:
		/* if no LE index, use pres */
		/* TODO: not implement yet */
		Debug( LDAP_DEBUG_FILTER, "\tLE\n" );
		rc = presence_candidates( op, wc, f->f_ava->aa_desc, ids );
		break;

	case LDAP_FILTER_NOT:
		/* no indexing to support NOT filters */
		Debug( LDAP_DEBUG_FILTER, "\tNOT\n" );
		WT_IDL_ALL( wi, ids );
		break;

	case LDAP_FILTER_AND:
		Debug( LDAP_DEBUG_FILTER, "\tAND\n" );
		rc = list_candidates( op, wc,
							  f->f_and, LDAP_FILTER_AND, ids, tmp, stack );
		break;

	case LDAP_FILTER_OR:
		Debug( LDAP_DEBUG_FILTER, "\tOR\n" );
		rc = list_candidates( op, wc,
							  f->f_or, LDAP_FILTER_OR, ids, tmp, stack );
		break;

	case LDAP_FILTER_EXT:
		/* TODO: not implement yet */
		Debug( LDAP_DEBUG_FILTER, "\tEXT\n" );
		rc = presence_candidates( op, wc, f->f_ava->aa_desc, ids );
		break;

	default:
		Debug( LDAP_DEBUG_FILTER, "\tUNKNOWN %lu\n",
			   (unsigned long) f->f_choice );
		/* Must not return NULL, otherwise extended filters break */
		WT_IDL_ALL( wi, ids );
	}

done:
	Debug( LDAP_DEBUG_FILTER,
		   "<= wt_filter_candidates: id=%ld first=%ld last=%ld\n",
		   (long) ids[0],
		   (long) WT_IDL_FIRST( ids ),
		   (long) WT_IDL_LAST( ids ) );
	return 0;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
