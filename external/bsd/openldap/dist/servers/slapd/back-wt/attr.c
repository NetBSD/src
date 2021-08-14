/*	$NetBSD: attr.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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

#include "back-wt.h"
#include "slap-config.h"

/* Find the ad, return -1 if not found,
 * set point for insertion if ins is non-NULL
 */
int
wt_attr_slot( struct wt_info *wi, AttributeDescription *ad, int *ins )
{
	unsigned base = 0, cursor = 0;
	unsigned n = wi->wi_nattrs;
	int val = 0;

	while ( 0 < n ) {
		unsigned pivot = n >> 1;
		cursor = base + pivot;

		val = SLAP_PTRCMP( ad, wi->wi_attrs[cursor]->ai_desc );
		if ( val < 0 ) {
			n = pivot;
		} else if ( val > 0 ) {
			base = cursor + 1;
			n -= pivot + 1;
		} else {
			return cursor;
		}
	}
	if ( ins ) {
		if ( val > 0 )
			++cursor;
		*ins = cursor;
	}
	return -1;
}

static int
ainfo_insert( struct wt_info *wi, AttrInfo *a )
{
	int x;
	int i = wt_attr_slot( wi, a->ai_desc, &x );

	/* Is it a dup? */
	if ( i >= 0 )
		return -1;

	wi->wi_attrs = ch_realloc( wi->wi_attrs, ( wi->wi_nattrs+1 ) *
							   sizeof( AttrInfo * ));
	if ( x < wi->wi_nattrs )
		AC_MEMCPY( &wi->wi_attrs[x+1], &wi->wi_attrs[x],
				   ( wi->wi_nattrs - x ) * sizeof( AttrInfo *));
	wi->wi_attrs[x] = a;
	wi->wi_nattrs++;
	return 0;
}

AttrInfo *
wt_attr_mask(
	struct wt_info	*wi,
	AttributeDescription *desc )
{
	int i = wt_attr_slot( wi, desc, NULL );
	return i < 0 ? NULL : wi->wi_attrs[i];
}

int
wt_attr_index_config(
	struct wt_info	*wi,
	const char		*fname,
	int			lineno,
	int			argc,
	char		**argv,
	struct		config_reply_s *c_reply)
{
	int rc = 0;
	int	i;
	slap_mask_t mask;
	char **attrs;
	char **indexes = NULL;

	attrs = ldap_str2charray( argv[0], "," );

	if( attrs == NULL ) {
		fprintf( stderr, "%s: line %d: "
			"no attributes specified: %s\n",
			fname, lineno, argv[0] );
		return LDAP_PARAM_ERROR;
	}

	if ( argc > 1 ) {
		indexes = ldap_str2charray( argv[1], "," );

		if( indexes == NULL ) {
			fprintf( stderr, "%s: line %d: "
				"no indexes specified: %s\n",
				fname, lineno, argv[1] );
			rc = LDAP_PARAM_ERROR;
			goto done;
		}
	}

	if( indexes == NULL ) {
		mask = wi->wi_defaultmask;

	} else {
		mask = 0;

		for ( i = 0; indexes[i] != NULL; i++ ) {
			slap_mask_t index;

			rc = slap_str2index( indexes[i], &index );

			if( rc != LDAP_SUCCESS ) {
				if ( c_reply )
				{
					snprintf(c_reply->msg, sizeof(c_reply->msg),
						"index type \"%s\" undefined", indexes[i] );

					fprintf( stderr, "%s: line %d: %s\n",
						fname, lineno, c_reply->msg );
				}
				rc = LDAP_PARAM_ERROR;
				goto done;
			}

			mask |= index;
		}
	}

	if( !mask ) {
		if ( c_reply )
		{
			snprintf(c_reply->msg, sizeof(c_reply->msg),
				"no indexes selected" );
			fprintf( stderr, "%s: line %d: %s\n",
				fname, lineno, c_reply->msg );
		}
		rc = LDAP_PARAM_ERROR;
		goto done;
	}

	for ( i = 0; attrs[i] != NULL; i++ ) {
		AttrInfo	*a;
		AttributeDescription *ad;
		const char *text;
#ifdef LDAP_COMP_MATCH
		ComponentReference* cr = NULL;
		AttrInfo *a_cr = NULL;
#endif

		if( strcasecmp( attrs[i], "default" ) == 0 ) {
			wi->wi_defaultmask |= mask;
			continue;
		}

#ifdef LDAP_COMP_MATCH
		if ( is_component_reference( attrs[i] ) ) {
			rc = extract_component_reference( attrs[i], &cr );
			if ( rc != LDAP_SUCCESS ) {
				if ( c_reply )
				{
					snprintf(c_reply->msg, sizeof(c_reply->msg),
						"index component reference\"%s\" undefined",
						attrs[i] );
					fprintf( stderr, "%s: line %d: %s\n",
						fname, lineno, c_reply->msg );
				}
				goto done;
			}
			cr->cr_indexmask = mask;
			/*
			 * After extracting a component reference
			 * only the name of a attribute will be remaining
			 */
		} else {
			cr = NULL;
		}
#endif
		ad = NULL;
		rc = slap_str2ad( attrs[i], &ad, &text );

		if( rc != LDAP_SUCCESS ) {
			if ( c_reply )
			{
				snprintf(c_reply->msg, sizeof(c_reply->msg),
					"index attribute \"%s\" undefined",
					attrs[i] );

				fprintf( stderr, "%s: line %d: %s\n",
					fname, lineno, c_reply->msg );
			}
fail:
#ifdef LDAP_COMP_MATCH
			ch_free( cr );
#endif
			goto done;
		}

		if( ad == slap_schema.si_ad_entryDN || slap_ad_is_binary( ad ) ) {
			if (c_reply) {
				snprintf(c_reply->msg, sizeof(c_reply->msg),
					"index of attribute \"%s\" disallowed", attrs[i] );
				fprintf( stderr, "%s: line %d: %s\n",
					fname, lineno, c_reply->msg );
			}
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto fail;
		}

		if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) && !(
			ad->ad_type->sat_approx
				&& ad->ad_type->sat_approx->smr_indexer
				&& ad->ad_type->sat_approx->smr_filter ) )
		{
			if (c_reply) {
				snprintf(c_reply->msg, sizeof(c_reply->msg),
					"approx index of attribute \"%s\" disallowed", attrs[i] );
				fprintf( stderr, "%s: line %d: %s\n",
					fname, lineno, c_reply->msg );
			}
			rc = LDAP_INAPPROPRIATE_MATCHING;
			goto fail;
		}

		if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) && !(
			ad->ad_type->sat_equality
				&& ad->ad_type->sat_equality->smr_indexer
				&& ad->ad_type->sat_equality->smr_filter ) )
		{
			if (c_reply) {
				snprintf(c_reply->msg, sizeof(c_reply->msg),
					"equality index of attribute \"%s\" disallowed", attrs[i] );
				fprintf( stderr, "%s: line %d: %s\n",
					fname, lineno, c_reply->msg );
			}
			rc = LDAP_INAPPROPRIATE_MATCHING;
			goto fail;
		}

		if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) && !(
			ad->ad_type->sat_substr
				&& ad->ad_type->sat_substr->smr_indexer
				&& ad->ad_type->sat_substr->smr_filter ) )
		{
			if (c_reply) {
				snprintf(c_reply->msg, sizeof(c_reply->msg),
					"substr index of attribute \"%s\" disallowed", attrs[i] );
				fprintf( stderr, "%s: line %d: %s\n",
					fname, lineno, c_reply->msg );
			}
			rc = LDAP_INAPPROPRIATE_MATCHING;
			goto fail;
		}

		Debug( LDAP_DEBUG_CONFIG, "index %s 0x%04lx\n",
			ad->ad_cname.bv_val, mask );

		a = (AttrInfo *) ch_malloc( sizeof(AttrInfo) );

#ifdef LDAP_COMP_MATCH
		a->ai_cr = NULL;
#endif
		a->ai_desc = ad;

		if ( wi->wi_flags & WT_IS_OPEN ) {
			a->ai_indexmask = 0;
			a->ai_newmask = mask;
		} else {
			a->ai_indexmask = mask;
			a->ai_newmask = 0;
		}

#ifdef LDAP_COMP_MATCH
		if ( cr ) {
			a_cr = wt_attr_mask( wi, ad );
			if ( a_cr ) {
				/*
				 * AttrInfo is already in AVL
				 * just add the extracted component reference
				 * in the AttrInfo
				 */
				ch_free( a );
				rc = insert_component_reference( cr, &a_cr->ai_cr );
				if ( rc != LDAP_SUCCESS) {
					fprintf( stderr, " error during inserting component reference in %s ", attrs[i]);
					rc = LDAP_PARAM_ERROR;
					goto fail;
				}
				continue;
			} else {
				rc = insert_component_reference( cr, &a->ai_cr );
				if ( rc != LDAP_SUCCESS) {
					fprintf( stderr, " error during inserting component reference in %s ", attrs[i]);
					rc = LDAP_PARAM_ERROR;
					ch_free( a );
					goto fail;
				}
			}
		}
#endif
		rc = ainfo_insert( wi, a );
		if( rc ) {
			if ( wi->wi_flags & WT_IS_OPEN ) {
				AttrInfo *b = wt_attr_mask( wi, ad );
				/* If there is already an index defined for this attribute
				 * it must be replaced. Otherwise we end up with multiple
				 * olcIndex values for the same attribute */
				if ( b->ai_indexmask & WT_INDEX_DELETING ) {
					/* If we were editing this attr, reset it */
					b->ai_indexmask &= ~WT_INDEX_DELETING;
					/* If this is leftover from a previous add, commit it */
					if ( b->ai_newmask )
						b->ai_indexmask = b->ai_newmask;
					b->ai_newmask = a->ai_newmask;
					ch_free( a );
					rc = 0;
					continue;
				}
			}
			if (c_reply) {
				snprintf(c_reply->msg, sizeof(c_reply->msg),
					"duplicate index definition for attr \"%s\"",
					attrs[i] );
				fprintf( stderr, "%s: line %d: %s\n",
					fname, lineno, c_reply->msg );
			}

			rc = LDAP_PARAM_ERROR;
			goto done;
		}
	}

done:
	ldap_charray_free( attrs );
	if ( indexes != NULL ) ldap_charray_free( indexes );

	return rc;
}

void
wt_attr_info_free( AttrInfo *ai )
{
#ifdef LDAP_COMP_MATCH
	free( ai->ai_cr );
#endif
	free( ai );
}

void
wt_attr_index_destroy( struct wt_info *wi )
{
	int i;

	for ( i=0; i<wi->wi_nattrs; i++ )
		wt_attr_info_free( wi->wi_attrs[i] );

	free( wi->wi_attrs );
}



/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
