/* search.c - ldap backend search function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldap/search.c,v 1.201.2.11 2008/07/10 00:28:39 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2008 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "back-ldap.h"
#undef ldap_debug	/* silence a warning in ldap-int.h */
#include "../../../libraries/libldap/ldap-int.h"

#include "lutil.h"

static int
ldap_build_entry( Operation *op, LDAPMessage *e, Entry *ent,
	 struct berval *bdn );

/*
 * Quick'n'dirty rewrite of filter in case of error, to deal with
 * <draft-zeilenga-ldap-t-f>.
 */
static int
ldap_back_munge_filter(
	Operation	*op,
	struct berval	*filter )
{
	ldapinfo_t	*li = (ldapinfo_t *) op->o_bd->be_private;

	char		*ptr;
	int		gotit = 0;

	Debug( LDAP_DEBUG_ARGS, "=> ldap_back_munge_filter \"%s\"\n",
			filter->bv_val, 0, 0 );

	for ( ptr = strstr( filter->bv_val, "(?=" ); 
			ptr;
			ptr = strstr( ptr, "(?=" ) )
	{
		static struct berval
			bv_true = BER_BVC( "(?=true)" ),
			bv_false = BER_BVC( "(?=false)" ),
			bv_undefined = BER_BVC( "(?=undefined)" ),
			bv_t = BER_BVC( "(&)" ),
			bv_f = BER_BVC( "(|)" ),
			bv_T = BER_BVC( "(objectClass=*)" ),
			bv_F = BER_BVC( "(!(objectClass=*))" );
		struct berval	*oldbv = NULL,
				*newbv = NULL,
				oldfilter = BER_BVNULL;

		if ( strncmp( ptr, bv_true.bv_val, bv_true.bv_len ) == 0 ) {
			oldbv = &bv_true;
			if ( LDAP_BACK_T_F( li ) ) {
				newbv = &bv_t;

			} else {
				newbv = &bv_T;
			}

		} else if ( strncmp( ptr, bv_false.bv_val, bv_false.bv_len ) == 0 )
		{
			oldbv = &bv_false;
			if ( LDAP_BACK_T_F( li ) ) {
				newbv = &bv_f;

			} else {
				newbv = &bv_F;
			}

		} else if ( strncmp( ptr, bv_undefined.bv_val, bv_undefined.bv_len ) == 0 )
		{
			oldbv = &bv_undefined;
			newbv = &bv_F;

		} else {
			gotit = 0;
			goto done;
		}

		oldfilter = *filter;
		if ( newbv->bv_len > oldbv->bv_len ) {
			filter->bv_len += newbv->bv_len - oldbv->bv_len;
			if ( filter->bv_val == op->ors_filterstr.bv_val ) {
				filter->bv_val = op->o_tmpalloc( filter->bv_len + 1,
						op->o_tmpmemctx );

				AC_MEMCPY( filter->bv_val, op->ors_filterstr.bv_val,
						op->ors_filterstr.bv_len + 1 );

			} else {
				filter->bv_val = op->o_tmprealloc( filter->bv_val,
						filter->bv_len + 1, op->o_tmpmemctx );
			}

			ptr = filter->bv_val + ( ptr - oldfilter.bv_val );
		}

		AC_MEMCPY( &ptr[ newbv->bv_len ],
				&ptr[ oldbv->bv_len ], 
				oldfilter.bv_len - ( ptr - filter->bv_val ) - oldbv->bv_len + 1 );
		AC_MEMCPY( ptr, newbv->bv_val, newbv->bv_len );

		ptr += newbv->bv_len;
		gotit = 1;
	}

done:;
	Debug( LDAP_DEBUG_ARGS, "<= ldap_back_munge_filter \"%s\" (%d)\n",
			filter->bv_val, gotit, 0 );

	return gotit;
}

int
ldap_back_search(
		Operation	*op,
		SlapReply	*rs )
{
	ldapinfo_t	*li = (ldapinfo_t *) op->o_bd->be_private;

	ldapconn_t	*lc = NULL;
	struct timeval	tv;
	time_t		stoptime = (time_t)(-1);
	LDAPMessage	*res,
			*e;
	int		rc = 0,
			msgid; 
	struct berval	match = BER_BVNULL,
			filter = BER_BVNULL;
	int		free_filter = 0;
	int		i;
	char		**attrs = NULL;
	int		freetext = 0;
	int		do_retry = 1, dont_retry = 0;
	LDAPControl	**ctrls = NULL;
	char		**references = NULL;

	/* FIXME: shouldn't this be null? */
	const char	*save_matched = rs->sr_matched;

	if ( !ldap_back_dobind( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
		return rs->sr_err;
	}

	/*
	 * FIXME: in case of values return filter, we might want
	 * to map attrs and maybe rewrite value
	 */

	/* should we check return values? */
	if ( op->ors_deref != -1 ) {
		ldap_set_option( lc->lc_ld, LDAP_OPT_DEREF,
				(void *)&op->ors_deref );
	}

	if ( op->ors_tlimit != SLAP_NO_LIMIT ) {
		tv.tv_sec = op->ors_tlimit;
		tv.tv_usec = 0;
		stoptime = op->o_time + op->ors_tlimit;

	} else {
		LDAP_BACK_TV_SET( &tv );
	}

	if ( op->ors_attrs ) {
		for ( i = 0; !BER_BVISNULL( &op->ors_attrs[i].an_name ); i++ )
			/* just count attrs */ ;

		attrs = ch_malloc( ( i + 1 )*sizeof( char * ) );
		if ( attrs == NULL ) {
			rs->sr_err = LDAP_NO_MEMORY;
			rc = -1;
			goto finish;
		}
	
		for ( i = 0; !BER_BVISNULL( &op->ors_attrs[i].an_name ); i++ ) {
			attrs[ i ] = op->ors_attrs[i].an_name.bv_val;
		}
		attrs[ i ] = NULL;
	}

	ctrls = op->o_ctrls;
	rc = ldap_back_controls_add( op, rs, lc, &ctrls );
	if ( rc != LDAP_SUCCESS ) {
		goto finish;
	}

	/* deal with <draft-zeilenga-ldap-t-f> filters */
	filter = op->ors_filterstr;
retry:
	rs->sr_err = ldap_search_ext( lc->lc_ld, op->o_req_dn.bv_val,
			op->ors_scope, filter.bv_val,
			attrs, op->ors_attrsonly, ctrls, NULL,
			tv.tv_sec ? &tv : NULL,
			op->ors_slimit, &msgid );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		switch ( rs->sr_err ) {
		case LDAP_SERVER_DOWN:
			if ( do_retry ) {
				do_retry = 0;
				if ( ldap_back_retry( &lc, op, rs, LDAP_BACK_DONTSEND ) ) {
					goto retry;
				}
			}

			if ( lc == NULL ) {
				/* reset by ldap_back_retry ... */
				rs->sr_err = slap_map_api2result( rs );

			} else {
				rc = ldap_back_op_result( lc, op, rs, msgid, 0, LDAP_BACK_DONTSEND );
			}
				
			goto finish;

		case LDAP_FILTER_ERROR:
			if ( ldap_back_munge_filter( op, &filter ) ) {
				free_filter = 1;
				goto retry;
			}

			/* invalid filters return success with no data */
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_text = NULL;
			goto finish;
		
		default:
			rs->sr_err = slap_map_api2result( rs );
			rs->sr_text = NULL;
			goto finish;
		}
	}

	/* if needed, initialize timeout */
	if ( li->li_timeout[ SLAP_OP_SEARCH ] ) {
		if ( tv.tv_sec == 0 || tv.tv_sec > li->li_timeout[ SLAP_OP_SEARCH ] ) {
			tv.tv_sec = li->li_timeout[ SLAP_OP_SEARCH ];
			tv.tv_usec = 0;
		}
	}

	/* We pull apart the ber result, stuff it into a slapd entry, and
	 * let send_search_entry stuff it back into ber format. Slow & ugly,
	 * but this is necessary for version matching, and for ACL processing.
	 */

	for ( rc = -2; rc != -1; rc = ldap_result( lc->lc_ld, msgid, LDAP_MSG_ONE, &tv, &res ) )
	{
		/* check for abandon */
		if ( op->o_abandon || LDAP_BACK_CONN_ABANDON( lc ) ) {
			if ( rc > 0 ) {
				ldap_msgfree( res );
			}
			(void)ldap_back_cancel( lc, op, rs, msgid, LDAP_BACK_DONTSEND );
			rc = SLAPD_ABANDON;
			goto finish;
		}

		if ( rc == 0 || rc == -2 ) {
			ldap_pvt_thread_yield();

			/* check timeout */
			if ( li->li_timeout[ SLAP_OP_SEARCH ] ) {
				if ( rc == 0 ) {
					(void)ldap_back_cancel( lc, op, rs, msgid, LDAP_BACK_DONTSEND );
					rs->sr_text = "Operation timed out";
					rc = rs->sr_err = op->o_protocol >= LDAP_VERSION3 ?
						LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
					goto finish;
				}

			} else {
				LDAP_BACK_TV_SET( &tv );
			}

			/* check time limit */
			if ( op->ors_tlimit != SLAP_NO_LIMIT
					&& slap_get_time() > stoptime )
			{
				(void)ldap_back_cancel( lc, op, rs, msgid, LDAP_BACK_DONTSEND );
				rc = rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
				goto finish;
			}
			continue;

		} else {
			/* only touch when activity actually took place... */
			if ( li->li_idle_timeout && lc ) {
				lc->lc_time = op->o_time;
			}

			/* don't retry any more */
			dont_retry = 1;
		}


		if ( rc == LDAP_RES_SEARCH_ENTRY ) {
			Entry		ent = { 0 };
			struct berval	bdn = BER_BVNULL;

			do_retry = 0;

			e = ldap_first_entry( lc->lc_ld, res );
			rc = ldap_build_entry( op, e, &ent, &bdn );
			if ( rc == LDAP_SUCCESS ) {
				ldap_get_entry_controls( lc->lc_ld, res, &rs->sr_ctrls );
				rs->sr_entry = &ent;
				rs->sr_attrs = op->ors_attrs;
				rs->sr_operational_attrs = NULL;
				rs->sr_flags = 0;
				rs->sr_err = LDAP_SUCCESS;
				rc = rs->sr_err = send_search_entry( op, rs );
				if ( rs->sr_ctrls ) {
					ldap_controls_free( rs->sr_ctrls );
					rs->sr_ctrls = NULL;
				}
				rs->sr_entry = NULL;
				if ( !BER_BVISNULL( &ent.e_name ) ) {
					assert( ent.e_name.bv_val != bdn.bv_val );
					op->o_tmpfree( ent.e_name.bv_val, op->o_tmpmemctx );
					BER_BVZERO( &ent.e_name );
				}
				if ( !BER_BVISNULL( &ent.e_nname ) ) {
					op->o_tmpfree( ent.e_nname.bv_val, op->o_tmpmemctx );
					BER_BVZERO( &ent.e_nname );
				}
				entry_clean( &ent );
			}
			ldap_msgfree( res );
			if ( rc != LDAP_SUCCESS ) {
				if ( rc == LDAP_UNAVAILABLE ) {
					rc = rs->sr_err = LDAP_OTHER;
				} else {
					(void)ldap_back_cancel( lc, op, rs, msgid, LDAP_BACK_DONTSEND );
				}
				goto finish;
			}

		} else if ( rc == LDAP_RES_SEARCH_REFERENCE ) {
			if ( LDAP_BACK_NOREFS( li ) ) {
				ldap_msgfree( res );
				continue;
			}

			do_retry = 0;
			rc = ldap_parse_reference( lc->lc_ld, res,
					&references, &rs->sr_ctrls, 1 );

			if ( rc != LDAP_SUCCESS ) {
				continue;
			}

			/* FIXME: there MUST be at least one */
			if ( references && references[ 0 ] && references[ 0 ][ 0 ] ) {
				int		cnt;

				for ( cnt = 0; references[ cnt ]; cnt++ )
					/* NO OP */ ;

				/* FIXME: there MUST be at least one */
				rs->sr_ref = op->o_tmpalloc( ( cnt + 1 ) * sizeof( struct berval ),
					op->o_tmpmemctx );

				for ( cnt = 0; references[ cnt ]; cnt++ ) {
					ber_str2bv( references[ cnt ], 0, 0, &rs->sr_ref[ cnt ] );
				}
				BER_BVZERO( &rs->sr_ref[ cnt ] );

				/* ignore return value by now */
				rs->sr_entry = NULL;
				( void )send_search_reference( op, rs );

			} else {
				Debug( LDAP_DEBUG_ANY,
					"%s ldap_back_search: "
					"got SEARCH_REFERENCE "
					"with no referrals\n",
					op->o_log_prefix, 0, 0 );
			}

			/* cleanup */
			if ( references ) {
				ber_memvfree( (void **)references );
				op->o_tmpfree( rs->sr_ref, op->o_tmpmemctx );
				rs->sr_ref = NULL;
				references = NULL;
			}

			if ( rs->sr_ctrls ) {
				ldap_controls_free( rs->sr_ctrls );
				rs->sr_ctrls = NULL;
			}

		} else {
			char		*err = NULL;

			rc = ldap_parse_result( lc->lc_ld, res, &rs->sr_err,
					&match.bv_val, &err,
					&references, &rs->sr_ctrls, 1 );
			if ( rc != LDAP_SUCCESS ) {
				rs->sr_err = rc;
			}
			rs->sr_err = slap_map_api2result( rs );
			if ( err ) {
				rs->sr_text = err;
				freetext = 1;
			}

			/* RFC 4511: referrals can only appear
			 * if result code is LDAP_REFERRAL */
			if ( references 
				&& references[ 0 ]
				&& references[ 0 ][ 0 ] )
			{
				if ( rs->sr_err != LDAP_REFERRAL ) {
					Debug( LDAP_DEBUG_ANY,
						"%s ldap_back_search: "
						"got referrals with err=%d\n",
						op->o_log_prefix,
						rs->sr_err, 0 );

				} else {
					int	cnt;

					for ( cnt = 0; references[ cnt ]; cnt++ )
						/* NO OP */ ;
				
					rs->sr_ref = op->o_tmpalloc( ( cnt + 1 ) * sizeof( struct berval ),
						op->o_tmpmemctx );

					for ( cnt = 0; references[ cnt ]; cnt++ ) {
						/* duplicating ...*/
						ber_str2bv( references[ cnt ], 0, 0, &rs->sr_ref[ cnt ] );
					}
					BER_BVZERO( &rs->sr_ref[ cnt ] );
				}

			} else if ( rs->sr_err == LDAP_REFERRAL ) {
				Debug( LDAP_DEBUG_ANY,
					"%s ldap_back_search: "
					"got err=%d with null "
					"or empty referrals\n",
					op->o_log_prefix,
					rs->sr_err, 0 );

				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			}

			if ( match.bv_val != NULL ) {
				match.bv_len = strlen( match.bv_val );
			}

			rc = 0;
			break;
		}

		/* if needed, restore timeout */
		if ( li->li_timeout[ SLAP_OP_SEARCH ] ) {
			if ( tv.tv_sec == 0 || tv.tv_sec > li->li_timeout[ SLAP_OP_SEARCH ] ) {
				tv.tv_sec = li->li_timeout[ SLAP_OP_SEARCH ];
				tv.tv_usec = 0;
			}
		}
	}

 	if ( rc == -1 && dont_retry == 0 ) {
		if ( do_retry ) {
			do_retry = 0;
			if ( ldap_back_retry( &lc, op, rs, LDAP_BACK_DONTSEND ) ) {
				goto retry;
			}
		}
		rs->sr_err = LDAP_SERVER_DOWN;
		rs->sr_err = slap_map_api2result( rs );
		goto finish;
	}

	/*
	 * Rewrite the matched portion of the search base, if required
	 */
	if ( !BER_BVISNULL( &match ) && !BER_BVISEMPTY( &match ) ) {
		struct berval	pmatch;

		if ( dnPretty( NULL, &match, &pmatch, op->o_tmpmemctx ) == LDAP_SUCCESS ) {
			rs->sr_matched = pmatch.bv_val;
			LDAP_FREE( match.bv_val );

		} else {
			rs->sr_matched = match.bv_val;
		}
	}

	if ( rs->sr_v2ref ) {
		rs->sr_err = LDAP_REFERRAL;
	}

finish:;
	if ( LDAP_BACK_QUARANTINE( li ) ) {
		ldap_back_quarantine( op, rs );
	}

#if 0
	/* let send_ldap_result play cleanup handlers (ITS#4645) */
	if ( rc != SLAPD_ABANDON )
#endif
	{
		send_ldap_result( op, rs );
	}

	(void)ldap_back_controls_free( op, rs, &ctrls );

	if ( rs->sr_ctrls ) {
		ldap_controls_free( rs->sr_ctrls );
		rs->sr_ctrls = NULL;
	}

	if ( rs->sr_matched != NULL && rs->sr_matched != save_matched ) {
		if ( rs->sr_matched != match.bv_val ) {
			ber_memfree_x( (char *)rs->sr_matched, op->o_tmpmemctx );

		} else {
			LDAP_FREE( match.bv_val );
		}
		rs->sr_matched = save_matched;
	}

	if ( free_filter ) {
		op->o_tmpfree( filter.bv_val, op->o_tmpmemctx );
	}

	if ( rs->sr_text ) {
		if ( freetext ) {
			LDAP_FREE( (char *)rs->sr_text );
		}
		rs->sr_text = NULL;
	}

	if ( rs->sr_ref ) {
		op->o_tmpfree( rs->sr_ref, op->o_tmpmemctx );
		rs->sr_ref = NULL;
	}

	if ( references ) {
		ber_memvfree( (void **)references );
	}

	if ( attrs ) {
		ch_free( attrs );
	}

	if ( lc != NULL ) {
		ldap_back_release_conn( li, lc );
	}

	return rs->sr_err;
}

static int
ldap_build_entry(
		Operation	*op,
		LDAPMessage	*e,
		Entry		*ent,
		struct berval	*bdn )
{
	struct berval	a;
	BerElement	ber = *e->lm_ber;
	Attribute	*attr, **attrp;
	const char	*text;
	int		last;
	char *lastb;
	ber_len_t len;

	/* safe assumptions ... */
	assert( ent != NULL );
	BER_BVZERO( &ent->e_bv );

	if ( ber_scanf( &ber, "{m", bdn ) == LBER_ERROR ) {
		return LDAP_DECODING_ERROR;
	}

	/*
	 * Note: this may fail if the target host(s) schema differs
	 * from the one known to the meta, and a DN with unknown
	 * attributes is returned.
	 * 
	 * FIXME: should we log anything, or delegate to dnNormalize?
	 */
	/* Note: if the distinguished values or the naming attributes
	 * change, should we massage them as well?
	 */
	if ( dnPrettyNormal( NULL, bdn, &ent->e_name, &ent->e_nname,
		op->o_tmpmemctx ) != LDAP_SUCCESS )
	{
		return LDAP_INVALID_DN_SYNTAX;
	}

	ent->e_attrs = NULL;
	if ( ber_first_element( &ber, &len, &lastb ) != LBER_SEQUENCE ) {
		return LDAP_SUCCESS;
	}

	attrp = &ent->e_attrs;
	while ( ber_next_element( &ber, &len, lastb ) == LBER_SEQUENCE &&
		ber_scanf( &ber, "{m", &a ) != LBER_ERROR ) {
		int				i;
		slap_syntax_validate_func	*validate;
		slap_syntax_transform_func	*pretty;

		attr = attr_alloc( NULL );
		if ( attr == NULL ) {
			continue;
		}
		if ( slap_bv2ad( &a, &attr->a_desc, &text ) 
				!= LDAP_SUCCESS )
		{
			if ( slap_bv2undef_ad( &a, &attr->a_desc, &text,
				SLAP_AD_PROXIED ) != LDAP_SUCCESS )
			{
				Debug( LDAP_DEBUG_ANY, 
					"%s ldap_build_entry: "
					"slap_bv2undef_ad(%s): %s\n",
					op->o_log_prefix, a.bv_val, text );
				attr_free( attr );
				continue;
			}
		}

		/* no subschemaSubentry */
		if ( attr->a_desc == slap_schema.si_ad_subschemaSubentry
			|| attr->a_desc == slap_schema.si_ad_entryDN )
		{

			/* 
			 * We eat target's subschemaSubentry because
			 * a search for this value is likely not
			 * to resolve to the appropriate backend;
			 * later, the local subschemaSubentry is
			 * added.
			 *
			 * We also eat entryDN because the frontend
			 * will reattach it without checking if already
			 * present...
			 */
			( void )ber_scanf( &ber, "x" /* [W] */ );

			attr_free( attr );
			continue;
		}
		
		if ( ber_scanf( &ber, "[W]", &attr->a_vals ) == LBER_ERROR
				|| attr->a_vals == NULL )
		{
			/*
			 * Note: attr->a_vals can be null when using
			 * values result filter
			 */
			attr->a_vals = (struct berval *)&slap_dummy_bv;
		}

		validate = attr->a_desc->ad_type->sat_syntax->ssyn_validate;
		pretty = attr->a_desc->ad_type->sat_syntax->ssyn_pretty;

		if ( !validate && !pretty ) {
			attr->a_nvals = NULL;
			attr_free( attr );
			goto next_attr;
		}

		for ( i = 0; !BER_BVISNULL( &attr->a_vals[i] ); i++ ) {
			struct berval	pval;
			int		rc;

			if ( pretty ) {
				rc = pretty( attr->a_desc->ad_type->sat_syntax,
					&attr->a_vals[i], &pval, NULL );

			} else {
				rc = validate( attr->a_desc->ad_type->sat_syntax,
					&attr->a_vals[i] );
			}

			if ( rc != LDAP_SUCCESS ) {
				/* check if, by chance, it's an undefined objectClass */
				if ( attr->a_desc == slap_schema.si_ad_objectClass &&
						oc_bvfind_undef( &attr->a_vals[i] ) != NULL )
				{
					ber_dupbv( &pval, &attr->a_vals[i] );

				} else {
					attr->a_nvals = NULL;
					attr_free( attr );
					goto next_attr;
				}
			}

			if ( pretty ) {
				LBER_FREE( attr->a_vals[i].bv_val );
				attr->a_vals[i] = pval;
			}
		}
		attr->a_numvals = last = i;

		if ( last && attr->a_desc->ad_type->sat_equality &&
				attr->a_desc->ad_type->sat_equality->smr_normalize )
		{
			attr->a_nvals = ch_malloc( ( last + 1 )*sizeof( struct berval ) );
			for ( i = 0; i < last; i++ ) {
				int		rc;

				/*
				 * check that each value is valid per syntax
				 * and pretty if appropriate
				 */
				rc = attr->a_desc->ad_type->sat_equality->smr_normalize(
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					attr->a_desc->ad_type->sat_syntax,
					attr->a_desc->ad_type->sat_equality,
					&attr->a_vals[i], &attr->a_nvals[i],
					NULL );

				if ( rc != LDAP_SUCCESS ) {
					BER_BVZERO( &attr->a_nvals[i] );
					attr_free( attr );
					goto next_attr;
				}
			}
			BER_BVZERO( &attr->a_nvals[i] );

		} else {
			attr->a_nvals = attr->a_vals;
		}
		*attrp = attr;
		attrp = &attr->a_next;

next_attr:;
	}

	return LDAP_SUCCESS;
}

/* return 0 IFF we can retrieve the entry with ndn
 */
int
ldap_back_entry_get(
		Operation		*op,
		struct berval		*ndn,
		ObjectClass		*oc,
		AttributeDescription	*at,
		int			rw,
		Entry			**ent )
{
	ldapinfo_t	*li = (ldapinfo_t *) op->o_bd->be_private;

	ldapconn_t	*lc = NULL;
	int		rc,
			do_not_cache;
	ber_tag_t	tag;
	struct berval	bdn;
	LDAPMessage	*result = NULL,
			*e = NULL;
	char		*attr[3], **attrp = NULL;
	char		*filter = NULL;
	SlapReply	rs;
	int		do_retry = 1;
	LDAPControl	**ctrls = NULL;

	*ent = NULL;

	/* Tell getconn this is a privileged op */
	do_not_cache = op->o_do_not_cache;
	tag = op->o_tag;
	/* do not cache */
	op->o_do_not_cache = 1;
	/* ldap_back_entry_get() is an entry lookup, so it does not need
	 * to know what the entry is being looked up for */
	op->o_tag = LDAP_REQ_SEARCH;
	rc = ldap_back_dobind( &lc, op, &rs, LDAP_BACK_DONTSEND );
	op->o_do_not_cache = do_not_cache;
	op->o_tag = tag;
	if ( !rc ) {
		return rs.sr_err;
	}

	if ( at ) {
		attrp = attr;
		if ( oc && at != slap_schema.si_ad_objectClass ) {
			attr[0] = slap_schema.si_ad_objectClass->ad_cname.bv_val;
			attr[1] = at->ad_cname.bv_val;
			attr[2] = NULL;

		} else {
			attr[0] = at->ad_cname.bv_val;
			attr[1] = NULL;
		}
	}

	if ( oc ) {
		char	*ptr;

		filter = op->o_tmpalloc( STRLENOF( "(objectClass=" ")" ) 
				+ oc->soc_cname.bv_len + 1, op->o_tmpmemctx );
		ptr = lutil_strcopy( filter, "(objectClass=" );
		ptr = lutil_strcopy( ptr, oc->soc_cname.bv_val );
		*ptr++ = ')';
		*ptr++ = '\0';
	}

retry:
	ctrls = op->o_ctrls;
	rc = ldap_back_controls_add( op, &rs, lc, &ctrls );
	if ( rc != LDAP_SUCCESS ) {
		goto cleanup;
	}

	/* TODO: timeout? */
	rc = ldap_search_ext_s( lc->lc_ld, ndn->bv_val, LDAP_SCOPE_BASE, filter,
				attrp, 0, ctrls, NULL,
				NULL, LDAP_NO_LIMIT, &result );
	if ( rc != LDAP_SUCCESS ) {
		if ( rc == LDAP_SERVER_DOWN && do_retry ) {
			do_retry = 0;
			if ( ldap_back_retry( &lc, op, &rs, LDAP_BACK_DONTSEND ) ) {
				/* if the identity changed, there might be need to re-authz */
				(void)ldap_back_controls_free( op, &rs, &ctrls );
				goto retry;
			}
		}
		goto cleanup;
	}

	e = ldap_first_entry( lc->lc_ld, result );
	if ( e == NULL ) {
		/* the entry exists, but it doesn't match the filter? */
		goto cleanup;
	}

	*ent = entry_alloc();
	if ( *ent == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto cleanup;
	}

	rc = ldap_build_entry( op, e, *ent, &bdn );

	if ( rc != LDAP_SUCCESS ) {
		entry_free( *ent );
		*ent = NULL;
	}

cleanup:
	(void)ldap_back_controls_free( op, &rs, &ctrls );

	if ( result ) {
		ldap_msgfree( result );
	}

	if ( filter ) {
		op->o_tmpfree( filter, op->o_tmpmemctx );
	}

	if ( lc != NULL ) {
		ldap_back_release_conn( li, lc );
	}

	return rc;
}

