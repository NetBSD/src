/*	$NetBSD: index.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: index.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"

static char presence_keyval[] = {0,0};
static struct berval presence_key = BER_BVC(presence_keyval);

AttrInfo *wt_index_mask(
	Backend *be,
	AttributeDescription *desc,
	struct berval *atname )
{
	AttributeType *at;
	AttrInfo *ai = wt_attr_mask( be->be_private, desc );

	if( ai ) {
		*atname = desc->ad_cname;
		return ai;
	}

	/* If there is a tagging option, did we ever index the base
	 * type? If so, check for mask, otherwise it's not there.
	 */
	if( slap_ad_is_tagged( desc ) && desc != desc->ad_type->sat_ad ) {
		/* has tagging option */
		ai = wt_attr_mask( be->be_private, desc->ad_type->sat_ad );

		if ( ai && !( ai->ai_indexmask & SLAP_INDEX_NOTAGS ) ) {
			*atname = desc->ad_type->sat_cname;
			return ai;
		}
	}

	/* see if supertype defined mask for its subtypes */
	for( at = desc->ad_type; at != NULL ; at = at->sat_sup ) {
		/* If no AD, we've never indexed this type */
		if ( !at->sat_ad ) continue;

		ai = wt_attr_mask( be->be_private, at->sat_ad );

		if ( ai && !( ai->ai_indexmask & SLAP_INDEX_NOSUBTYPES ) ) {
			*atname = at->sat_cname;
			return ai;
		}
	}

	return 0;
}

/* This function is only called when evaluating search filters.
 */
int wt_index_param(
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	slap_mask_t *maskp,
	struct berval *prefixp )
{
	AttrInfo *ai;
	int rc;
	slap_mask_t mask, type = 0;

	ai = wt_index_mask( be, desc, prefixp );

	if ( !ai ) {
		/* TODO: add monitor */
		return LDAP_INAPPROPRIATE_MATCHING;
	}
	mask = ai->ai_indexmask;

	switch( ftype ) {
	case LDAP_FILTER_PRESENT:
		type = SLAP_INDEX_PRESENT;
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_PRESENT ) ) {
			*prefixp = presence_key;
			*maskp = mask;
			return LDAP_SUCCESS;
		}
		break;

	case LDAP_FILTER_APPROX:
		type = SLAP_INDEX_APPROX;
		if ( desc->ad_type->sat_approx ) {
			if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) ) {
				*maskp = mask;
				return LDAP_SUCCESS;
			}
			break;
		}

		/* Use EQUALITY rule and index for approximate match */
		/* fall thru */

	case LDAP_FILTER_EQUALITY:
		type = SLAP_INDEX_EQUALITY;
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) ) {
			*maskp = mask;
			return LDAP_SUCCESS;
		}
		break;

	case LDAP_FILTER_SUBSTRINGS:
		type = SLAP_INDEX_SUBSTR;
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) ) {
			*maskp = mask;
			return LDAP_SUCCESS;
		}
		break;

	default:
		return LDAP_OTHER;
	}

	/* TODO: add monitor index */
	return LDAP_INAPPROPRIATE_MATCHING;
}

static int indexer(
	Operation *op,
	wt_ctx *wc,
	AttributeDescription *ad,
	struct berval *atname,
	BerVarray vals,
	ID id,
	int opid,
	slap_mask_t mask )
{
	int rc, i;
	struct berval *keys;
	WT_CURSOR *cursor = NULL;
	WT_SESSION *session = wc->session;
	assert( mask != 0 );

	cursor = wt_ctx_index_cursor(wc, atname, 1);
	if( !cursor ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(indexer)
			   ": open index cursor failed: %s\n",
			   atname->bv_val );
		goto done;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_PRESENT ) ) {
		rc = wt_key_change( op->o_bd, cursor, &presence_key, id, opid );
		if( rc ) {
			goto done;
		}
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) ) {
		rc = ad->ad_type->sat_equality->smr_indexer(
			LDAP_FILTER_EQUALITY,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_equality,
			atname, vals, &keys, op->o_tmpmemctx );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = wt_key_change( op->o_bd, cursor, &keys[i], id, opid );
				if( rc ) {
					ber_bvarray_free_x( keys, op->o_tmpmemctx );
					goto done;
				}
			}
			ber_bvarray_free_x( keys, op->o_tmpmemctx );
		}
		rc = LDAP_SUCCESS;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) ) {
		rc = ad->ad_type->sat_approx->smr_indexer(
			LDAP_FILTER_APPROX,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_approx,
			atname, vals, &keys, op->o_tmpmemctx );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = wt_key_change( op->o_bd, cursor, &keys[i], id, opid );
				if( rc ) {
					ber_bvarray_free_x( keys, op->o_tmpmemctx );
					goto done;
				}
			}
			ber_bvarray_free_x( keys, op->o_tmpmemctx );
		}

		rc = LDAP_SUCCESS;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) ) {
		rc = ad->ad_type->sat_substr->smr_indexer(
			LDAP_FILTER_SUBSTRINGS,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_substr,
			atname, vals, &keys, op->o_tmpmemctx );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = wt_key_change( op->o_bd, cursor, &keys[i], id, opid );
				if( rc ) {
					ber_bvarray_free_x( keys, op->o_tmpmemctx );
					goto done;
				}
			}
			ber_bvarray_free_x( keys, op->o_tmpmemctx );
		}

		rc = LDAP_SUCCESS;
	}

done:
	if(cursor){
		cursor->close(cursor);
	}
	return rc;
}

static int index_at_values(
	Operation *op,
	wt_ctx *wc,
	AttributeDescription *ad,
	AttributeType *type,
	struct berval *tags,
	BerVarray vals,
	ID id,
	int opid )
{
	int rc;
	slap_mask_t mask = 0;
	int ixop = opid;
	AttrInfo *ai = NULL;

	if ( opid == WT_INDEX_UPDATE_OP )
		ixop = SLAP_INDEX_ADD_OP;

	if( type->sat_sup ) {
		/* recurse */
		rc = index_at_values( op, wc, NULL,
							  type->sat_sup, tags,
							  vals, id, opid );

		if( rc ) return rc;
	}

	/* If this type has no AD, we've never used it before */
	if( type->sat_ad ) {
		ai = wt_attr_mask( op->o_bd->be_private, type->sat_ad );
		if ( ai ) {
			#ifdef LDAP_COMP_MATCH
			/* component indexing */
			if ( ai->ai_cr ) {
				ComponentReference *cr;
				for( cr = ai->ai_cr ; cr ; cr = cr->cr_next ) {
					rc = indexer( op, wc, cr->cr_ad, &type->sat_cname,
								  cr->cr_nvals, id, ixop,
								  cr->cr_indexmask );
				}
			}
			#endif
			ad = type->sat_ad;
			/* If we're updating the index, just set the new bits that aren't
             * already in the old mask.
             */
			if ( opid == WT_INDEX_UPDATE_OP )
				mask = ai->ai_newmask & ~ai->ai_indexmask;
			else
				/* For regular updates, if there is a newmask use it. Otherwise
				 * just use the old mask.
				 */
				mask = ai->ai_newmask ? ai->ai_newmask : ai->ai_indexmask;
			if( mask ) {
				rc = indexer( op, wc, ad, &type->sat_cname,
							  vals, id, ixop, mask );
				if( rc ) return rc;
			}
		}
	}

	if( tags->bv_len ) {
		AttributeDescription *desc;

		desc = ad_find_tags( type, tags );
		if( desc ) {
			ai = wt_attr_mask( op->o_bd->be_private, desc );

			if( ai ) {
				if ( opid == WT_INDEX_UPDATE_OP )
					mask = ai->ai_newmask & ~ai->ai_indexmask;
				else
					mask = ai->ai_newmask ? ai->ai_newmask : ai->ai_indexmask;
				if ( mask ) {
					rc = indexer( op, wc, desc, &desc->ad_cname,
								  vals, id, ixop, mask );

					if( rc ) {
						return rc;
					}
				}
			}
		}
	}

	return LDAP_SUCCESS;
}

int wt_index_values(
	Operation *op,
	wt_ctx *wc,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int opid )
{
	int rc;

	/* Never index ID 0 */
	if ( id == 0 )
		return 0;

	rc = index_at_values( op, wc, desc,
						  desc->ad_type, &desc->ad_tags,
						  vals, id, opid );

	return rc;
}

int
wt_index_entry( Operation *op, wt_ctx *wc, int opid, Entry *e )
{
	int rc;
	Attribute *ap = e->e_attrs;

	if ( e->e_id == 0 )
		return 0;

	Debug( LDAP_DEBUG_TRACE, "=> index_entry_%s( %ld, \"%s\" )\n",
		   opid == SLAP_INDEX_DELETE_OP ? "del" : "add",
		   (long) e->e_id, e->e_dn ? e->e_dn : "" );

	for ( ; ap != NULL; ap = ap->a_next ) {
		rc = wt_index_values( op, wc, ap->a_desc,
							  ap->a_nvals, e->e_id, opid );
		if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				   "<= index_entry_%s( %ld, \"%s\" ) failure\n",
				   opid == SLAP_INDEX_ADD_OP ? "add" : "del",
				   (long) e->e_id, e->e_dn );
			return rc;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<= index_entry_%s( %ld, \"%s\" ) success\n",
		   opid == SLAP_INDEX_DELETE_OP ? "del" : "add",
		   (long) e->e_id, e->e_dn ? e->e_dn : "" );
	return 0;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
