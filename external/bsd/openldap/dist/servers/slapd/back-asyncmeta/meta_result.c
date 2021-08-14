/*	$NetBSD: meta_result.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* meta_result.c - target responses processing */
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
__RCSID("$NetBSD: meta_result.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"
#include "ldap_rq.h"
#include "../../../libraries/liblber/lber-int.h"

static void
asyncmeta_send_ldap_result(bm_context_t *bc, Operation *op, SlapReply *rs)
{
	if (bc->c_peer_name.bv_val == op->o_conn->c_peer_name.bv_val && !bc->op->o_abandon ) {
		send_ldap_result(&bc->copy_op, rs);
		bc->op->o_callback = bc->copy_op.o_callback;
		bc->op->o_extra = bc->copy_op.o_extra;
		bc->op->o_ctrls = bc->copy_op.o_ctrls;
	}
}

static int
asyncmeta_is_last_result(a_metaconn_t *mc, bm_context_t *bc, int candidate)
{
	a_metainfo_t	*mi = mc->mc_info;
	int i;
	SlapReply *candidates = bc->candidates;
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}
		if (candidates[ i ].sr_msgid != META_MSGID_IGNORE ||
		    candidates[ i ].sr_type != REP_RESULT) {
			return 1;
		}
	}
	return 0;
}

meta_search_candidate_t
asyncmeta_dobind_result(
	a_metaconn_t		*mc,
	int			candidate,
	SlapReply		*bind_result,
	LDAPMessage		*res )
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	meta_search_candidate_t	retcode = META_SEARCH_NOT_CANDIDATE;
	int			rc;

	assert( msc->msc_ldr != NULL );

	if ( mi->mi_idle_timeout != 0 ) {
		asyncmeta_set_msc_time(msc);
	}

	if ( LogTest( asyncmeta_debug ) ) {
		char	time_buf[ SLAP_TEXT_BUFLEN ];
		asyncmeta_get_timestamp(time_buf);
		Debug( asyncmeta_debug, "[%x] [%s] asyncmeta_dobind_result msc: %p, "
		       "msc->msc_binding_time: %x, msc->msc_flags:%x\n ",
		       (unsigned int)slap_get_time(), time_buf, msc,
		       (unsigned int)msc->msc_binding_time, msc->msc_mscflags );
	}
	/* FIXME: matched? referrals? response controls? */
	rc = ldap_parse_result( msc->msc_ldr, res,
				&(bind_result->sr_err),
				(char **)&(bind_result->sr_matched),
				(char **)&(bind_result->sr_text),
				NULL, NULL, 0 );

	if ( LogTest( asyncmeta_debug ) ) {
		char	time_buf[ SLAP_TEXT_BUFLEN ];
		asyncmeta_get_timestamp(time_buf);
		Debug( asyncmeta_debug,
		       "[%s] asyncmeta_dobind_result error=%d msc: %p\n",
		       time_buf,bind_result->sr_err, msc );
	}

	if ( rc != LDAP_SUCCESS ) {
		bind_result->sr_err = rc;
	}
	rc = slap_map_api2result( bind_result );

	LDAP_BACK_CONN_BINDING_CLEAR( msc );
	if ( rc != LDAP_SUCCESS ) {
		bind_result->sr_err = rc;
	} else {
		/* FIXME: check if bound as idassert authcDN! */
		if ( BER_BVISNULL( &msc->msc_bound_ndn )
			|| BER_BVISEMPTY( &msc->msc_bound_ndn ) )
		{
			LDAP_BACK_CONN_ISANON_SET( msc );
			if ( LogTest( asyncmeta_debug ) ) {
				char	time_buf[ SLAP_TEXT_BUFLEN ];
				asyncmeta_get_timestamp(time_buf);
				Debug( asyncmeta_debug, "[%s] asyncmeta_dobind_result anonymous msc: %p\n",
				      time_buf, msc );
			}

		} else {
			if ( META_BACK_TGT_SAVECRED( mt ) &&
				!BER_BVISNULL( &msc->msc_cred ) &&
				!BER_BVISEMPTY( &msc->msc_cred ) )
			{
				ldap_set_rebind_proc( msc->msc_ldr, mt->mt_rebind_f, msc );
			}
			if ( LogTest( asyncmeta_debug ) ) {
				char	time_buf[ SLAP_TEXT_BUFLEN ];
				asyncmeta_get_timestamp(time_buf);
				Debug( asyncmeta_debug, "[%s] asyncmeta_dobind_result success msc: %p\n",
				      time_buf, msc );
			}
			LDAP_BACK_CONN_ISBOUND_SET( msc );
		}
		retcode = META_SEARCH_CANDIDATE;
	}
	return retcode;
}

static int
asyncmeta_send_entry(
	Operation 	*op,
	SlapReply	*rs,
	a_metaconn_t	*mc,
	int 		target,
	LDAPMessage 	*e )
{
	a_metainfo_t 		*mi = mc->mc_info;
	struct berval		a, mapped = BER_BVNULL;
	int			check_sorted_attrs = 0;
	Entry 			ent = {0};
	BerElement 		ber = *ldap_get_message_ber( e );
	Attribute 		*attr, **attrp;
	struct berval 		bdn,
				dn = BER_BVNULL;
	const char 		*text;
	a_dncookie		dc;
	ber_len_t		len;
	int			rc;
	void	*mem_mark;

	mem_mark = slap_sl_mark( op->o_tmpmemctx );
	ber_set_option( &ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	if ( ber_scanf( &ber, "l{", &len ) == LBER_ERROR ) {
		return LDAP_DECODING_ERROR;
	}

	if ( ber_set_option( &ber, LBER_OPT_REMAINING_BYTES, &len ) != LBER_OPT_SUCCESS ) {
		return LDAP_OTHER;
	}

	if ( ber_scanf( &ber, "m{", &bdn ) == LBER_ERROR ) {
		return LDAP_DECODING_ERROR;
	}

	/*
	 * Rewrite the dn of the result, if needed
	 */
	dc.op = op;
	dc.target = mi->mi_targets[ target ];
	dc.memctx = op->o_tmpmemctx;
	dc.to_from = MASSAGE_REP;
	asyncmeta_dn_massage( &dc, &bdn, &dn );

	/*
	 * Note: this may fail if the target host(s) schema differs
	 * from the one known to the meta, and a DN with unknown
	 * attributes is returned.
	 *
	 * FIXME: should we log anything, or delegate to dnNormalize?
	 */
	rc = dnPrettyNormal( NULL, &dn, &ent.e_name, &ent.e_nname,
		op->o_tmpmemctx );
	if ( dn.bv_val != bdn.bv_val ) {
			op->o_tmpfree( dn.bv_val, op->o_tmpmemctx );
	}
	BER_BVZERO( &dn );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"%s asyncmeta_send_entry(\"%s\"): "
			"invalid DN syntax\n",
			op->o_log_prefix, ent.e_name.bv_val );
		rc = LDAP_INVALID_DN_SYNTAX;
		goto done;
	}

	/*
	 * cache dn
	 */
	if ( mi->mi_cache.ttl != META_DNCACHE_DISABLED ) {
		( void )asyncmeta_dncache_update_entry( &mi->mi_cache,
				&ent.e_nname, target );
	}

	attrp = &ent.e_attrs;

	while ( ber_scanf( &ber, "{m", &a ) != LBER_ERROR ) {
		int				last = 0;
		slap_syntax_validate_func	*validate;
		slap_syntax_transform_func	*pretty;

		if ( ber_pvt_ber_remaining( &ber ) < 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"%s asyncmeta_send_entry(\"%s\"): "
				"unable to parse attr \"%s\".\n",
				op->o_log_prefix, ent.e_name.bv_val, a.bv_val );

			rc = LDAP_OTHER;
			goto done;
		}

		if ( ber_pvt_ber_remaining( &ber ) == 0 ) {
			break;
		}

		attr = op->o_tmpcalloc( 1, sizeof(Attribute), op->o_tmpmemctx );
		if ( slap_bv2ad( &a, &attr->a_desc, &text )
				!= LDAP_SUCCESS) {
			if ( slap_bv2undef_ad( &a, &attr->a_desc, &text,
				SLAP_AD_PROXIED ) != LDAP_SUCCESS )
			{
				Debug(LDAP_DEBUG_ANY,
				      "%s meta_send_entry(\"%s\"): " "slap_bv2undef_ad(%s): %s\n",
				      op->o_log_prefix, ent.e_name.bv_val,
				      mapped.bv_val, text );
				( void )ber_scanf( &ber, "x" /* [W] */ );
				op->o_tmpfree( attr, op->o_tmpmemctx );
				continue;
			}
		}

		if ( attr->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL )
			check_sorted_attrs = 1;

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
			op->o_tmpfree( attr, op->o_tmpmemctx );
			continue;
		}

		if ( ber_scanf( &ber, "[W]", &attr->a_vals ) == LBER_ERROR
				|| attr->a_vals == NULL )
		{
			attr->a_vals = (struct berval *)&slap_dummy_bv;

		} else {
			for ( last = 0; !BER_BVISNULL( &attr->a_vals[ last ] ); ++last )
				;
		}
		attr->a_numvals = last;

		validate = attr->a_desc->ad_type->sat_syntax->ssyn_validate;
		pretty = attr->a_desc->ad_type->sat_syntax->ssyn_pretty;

		if ( !validate && !pretty ) {
			ber_bvarray_free_x( attr->a_vals, op->o_tmpmemctx );
			op->o_tmpfree( attr, op->o_tmpmemctx );
			goto next_attr;
		}

		/*
		 * It is necessary to try to rewrite attributes with
		 * dn syntax because they might be used in ACLs as
		 * members of groups; since ACLs are applied to the
		 * rewritten stuff, no dn-based subecj clause could
		 * be used at the ldap backend side (see
		 * http://www.OpenLDAP.org/faq/data/cache/452.html)
		 * The problem can be overcome by moving the dn-based
		 * ACLs to the target directory server, and letting
		 * everything pass thru the ldap backend.
		 */
		{
			int	i;

			if ( attr->a_desc->ad_type->sat_syntax ==
				slap_schema.si_syn_distinguishedName )
			{
				asyncmeta_dnattr_result_rewrite( &dc, attr->a_vals );

			} else if ( attr->a_desc == slap_schema.si_ad_ref ) {
				asyncmeta_referral_result_rewrite( &dc, attr->a_vals );

			}

			for ( i = 0; i < last; i++ ) {
				struct berval	pval;
				int		rc;

				if ( pretty ) {
					rc = ordered_value_pretty( attr->a_desc,
						&attr->a_vals[i], &pval, op->o_tmpmemctx );

				} else {
					rc = ordered_value_validate( attr->a_desc,
						&attr->a_vals[i], 0 );
				}

				if ( rc ) {
					ber_memfree_x( attr->a_vals[i].bv_val, op->o_tmpmemctx );
					if ( --last == i ) {
						BER_BVZERO( &attr->a_vals[ i ] );
						break;
					}
					attr->a_vals[i] = attr->a_vals[last];
					BER_BVZERO( &attr->a_vals[last] );
					i--;
					continue;
				}

				if ( pretty ) {
					ber_memfree_x( attr->a_vals[i].bv_val, op->o_tmpmemctx );
					attr->a_vals[i] = pval;
				}
			}

			if ( last == 0 && attr->a_vals != &slap_dummy_bv ) {
				ber_bvarray_free_x( attr->a_vals, op->o_tmpmemctx );
				op->o_tmpfree( attr, op->o_tmpmemctx );
				goto next_attr;
			}
		}

		if ( last && attr->a_desc->ad_type->sat_equality &&
			attr->a_desc->ad_type->sat_equality->smr_normalize )
		{
			int i;

			attr->a_nvals = op->o_tmpalloc( ( last + 1 ) * sizeof( struct berval ), op->o_tmpmemctx );
			for ( i = 0; i<last; i++ ) {
				/* if normalizer fails, drop this value */
				if ( ordered_value_normalize(
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					attr->a_desc,
					attr->a_desc->ad_type->sat_equality,
					&attr->a_vals[i], &attr->a_nvals[i],
					op->o_tmpmemctx )) {
					ber_memfree_x( attr->a_vals[i].bv_val, op->o_tmpmemctx );
					if ( --last == i ) {
						BER_BVZERO( &attr->a_vals[ i ] );
						break;
					}
					attr->a_vals[i] = attr->a_vals[last];
					BER_BVZERO( &attr->a_vals[last] );
					i--;
				}
			}
			BER_BVZERO( &attr->a_nvals[i] );
			if ( last == 0 ) {
				ber_bvarray_free_x( attr->a_vals, op->o_tmpmemctx );
				ber_bvarray_free_x( attr->a_nvals, op->o_tmpmemctx );
				op->o_tmpfree( attr, op->o_tmpmemctx );
				goto next_attr;
			}

		} else {
			attr->a_nvals = attr->a_vals;
		}

		attr->a_numvals = last;
		*attrp = attr;
		attrp = &attr->a_next;
next_attr:;
	}

	/* Check for sorted attributes */
	if ( check_sorted_attrs ) {
		for ( attr = ent.e_attrs; attr; attr = attr->a_next ) {
			if ( attr->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL ) {
				while ( attr->a_numvals > 1 ) {
					int i;
					int rc = slap_sort_vals( (Modifications *)attr, &text, &i, op->o_tmpmemctx );
					if ( rc != LDAP_TYPE_OR_VALUE_EXISTS )
						break;

					/* Strip duplicate values */
					if ( attr->a_nvals != attr->a_vals )
						ber_memfree_x( attr->a_nvals[i].bv_val, op->o_tmpmemctx );
					ber_memfree_x( attr->a_vals[i].bv_val, op->o_tmpmemctx );
					attr->a_numvals--;
					if ( (unsigned)i < attr->a_numvals ) {
						attr->a_vals[i] = attr->a_vals[attr->a_numvals];
						if ( attr->a_nvals != attr->a_vals )
							attr->a_nvals[i] = attr->a_nvals[attr->a_numvals];
					}
					BER_BVZERO(&attr->a_vals[attr->a_numvals]);
					if ( attr->a_nvals != attr->a_vals )
						BER_BVZERO(&attr->a_nvals[attr->a_numvals]);
				}
				attr->a_flags |= SLAP_ATTR_SORTED_VALS;
			}
		}
	}
	Debug( LDAP_DEBUG_TRACE,
	       "%s asyncmeta_send_entry(\"%s\"): "
	       ".\n",
	       op->o_log_prefix, ent.e_name.bv_val );
	ldap_get_entry_controls( mc->mc_conns[target].msc_ldr,
		e, &rs->sr_ctrls );
	rs->sr_entry = &ent;
	rs->sr_attrs = op->ors_attrs;
	rs->sr_operational_attrs = NULL;
	rs->sr_flags = mi->mi_targets[ target ]->mt_rep_flags;
	rs->sr_err = LDAP_SUCCESS;
	rc = send_search_entry( op, rs );
	switch ( rc ) {
	case LDAP_UNAVAILABLE:
		rc = LDAP_OTHER;
		break;
	}

done:;
	if ( rs->sr_ctrls != NULL ) {
		ldap_controls_free( rs->sr_ctrls );
		rs->sr_ctrls = NULL;
	}
#if 0
	while ( ent.e_attrs ) {
		attr = ent.e_attrs;
		ent.e_attrs = attr->a_next;
		if ( attr->a_nvals != attr->a_vals )
			ber_bvarray_free_x( attr->a_nvals, op->o_tmpmemctx );
		ber_bvarray_free_x( attr->a_vals, op->o_tmpmemctx );
		op->o_tmpfree( attr, op->o_tmpmemctx );
	}
	if (ent.e_name.bv_val != NULL) {
		op->o_tmpfree( ent.e_name.bv_val, op->o_tmpmemctx );
	}

	if (ent.e_nname.bv_val != NULL) {
		op->o_tmpfree( ent.e_nname.bv_val, op->o_tmpmemctx );
	}
	if (rs->sr_entry && rs->sr_entry != &ent) {
		entry_free( rs->sr_entry );
	}
#endif
	slap_sl_release( mem_mark, op->o_tmpmemctx );
	rs->sr_entry = NULL;
	rs->sr_attrs = NULL;
	return rc;
}

static void
asyncmeta_search_last_result(a_metaconn_t *mc, bm_context_t *bc, int candidate, int sres)
{
	a_metainfo_t	*mi = mc->mc_info;
	Operation *op = bc->op;
	SlapReply *rs = &bc->rs;
	int	   i;
	SlapReply *candidates = bc->candidates;
	char *matched = NULL;

	if ( bc->candidate_match > 0 ) {
		struct berval	pmatched = BER_BVNULL;

		/* we use the first one */
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			if ( META_IS_CANDIDATE( &candidates[ i ] )
					&& candidates[ i ].sr_matched != NULL )
			{
				struct berval	bv, pbv;
				int		rc;

				/* if we got success, and this target
				 * returned noSuchObject, and its suffix
				 * is a superior of the searchBase,
				 * ignore the matchedDN */
				if ( sres == LDAP_SUCCESS
					&& candidates[ i ].sr_err == LDAP_NO_SUCH_OBJECT
					&& op->o_req_ndn.bv_len > mi->mi_targets[ i ]->mt_nsuffix.bv_len )
				{
					free( (char *)candidates[ i ].sr_matched );
					candidates[ i ].sr_matched = NULL;
					continue;
				}

				ber_str2bv( candidates[ i ].sr_matched, 0, 0, &bv );
				rc = dnPretty( NULL, &bv, &pbv, op->o_tmpmemctx );

				if ( rc == LDAP_SUCCESS ) {

					/* NOTE: if they all are superiors
					 * of the baseDN, the shorter is also
					 * superior of the longer... */
					if ( pbv.bv_len > pmatched.bv_len ) {
						if ( !BER_BVISNULL( &pmatched ) ) {
							op->o_tmpfree( pmatched.bv_val, op->o_tmpmemctx );
						}
						pmatched = pbv;

					} else {
						op->o_tmpfree( pbv.bv_val, op->o_tmpmemctx );
					}
				}

				if ( candidates[ i ].sr_matched != NULL ) {
					free( (char *)candidates[ i ].sr_matched );
					candidates[ i ].sr_matched = NULL;
				}
			}
		}

		if ( !BER_BVISNULL( &pmatched ) ) {
			matched = pmatched.bv_val;
		}

	} else if ( sres == LDAP_NO_SUCH_OBJECT ) {
		matched = mi->mi_suffix.bv_val;
	}

	/*
	 * In case we returned at least one entry, we return LDAP_SUCCESS
	 * otherwise, the latter error code we got
	 */

	if ( sres == LDAP_SUCCESS ) {
		if ( rs->sr_v2ref ) {
			sres = LDAP_REFERRAL;
		}

		if ( META_BACK_ONERR_REPORT( mi ) ) {
			/*
			 * Report errors, if any
			 *
			 * FIXME: we should handle error codes and return the more
			 * important/reasonable
			 */
			for ( i = 0; i < mi->mi_ntargets; i++ ) {
				if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
					continue;
				}

				if ( candidates[ i ].sr_err != LDAP_SUCCESS
					&& candidates[ i ].sr_err != LDAP_NO_SUCH_OBJECT )
				{
					sres = candidates[ i ].sr_err;
					break;
				}
			}
		}
	}
	Debug( LDAP_DEBUG_TRACE,
	       "%s asyncmeta_search_last_result(\"%d\"): "
	       ".\n",
	       op->o_log_prefix, candidate );
	rs->sr_err = sres;
	rs->sr_matched = ( sres == LDAP_SUCCESS ? NULL : matched );
	rs->sr_text =  ( sres == LDAP_SUCCESS ? NULL : candidates[candidate].sr_text );
	rs->sr_ref = ( sres == LDAP_REFERRAL ? rs->sr_v2ref : NULL );
	asyncmeta_send_ldap_result(bc, op, rs);
	rs->sr_text = NULL;
	rs->sr_matched = NULL;
	rs->sr_ref = NULL;
}

static meta_search_candidate_t
asyncmeta_send_pending_op(bm_context_t *bc, int candidate)
{
	meta_search_candidate_t	retcode;
	switch (bc->op->o_tag) {
		case LDAP_REQ_SEARCH:
			retcode = asyncmeta_back_search_start( &bc->copy_op, &bc->rs, bc->bc_mc, bc, candidate,  NULL, 0 , 0);
			break;
		case LDAP_REQ_ADD:
			retcode = asyncmeta_back_add_start( &bc->copy_op, &bc->rs, bc->bc_mc, bc, candidate, 0);
			break;
		case LDAP_REQ_MODIFY:
			retcode = asyncmeta_back_modify_start( &bc->copy_op, &bc->rs, bc->bc_mc, bc, candidate, 0);
			break;
		case LDAP_REQ_MODRDN:
			retcode = asyncmeta_back_modrdn_start( &bc->copy_op, &bc->rs, bc->bc_mc, bc, candidate, 0);
			break;
		case LDAP_REQ_COMPARE:
			retcode = asyncmeta_back_compare_start( &bc->copy_op, &bc->rs, bc->bc_mc, bc, candidate, 0);
			break;
		case LDAP_REQ_DELETE:
			retcode = asyncmeta_back_delete_start( &bc->copy_op, &bc->rs, bc->bc_mc, bc, candidate, 0);
			break;
		default:
			retcode = META_SEARCH_NOT_CANDIDATE;
		}
	return retcode;
}


meta_search_candidate_t
asyncmeta_send_all_pending_ops(a_metaconn_t *mc, int candidate, void *ctx, int dolock)
{
	a_metainfo_t	*mi = mc->mc_info;
	bm_context_t *bc, *onext;
	a_metasingleconn_t *msc = &mc->mc_conns[candidate];

	if ( dolock )
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );

	msc->msc_active++;
	for (bc = LDAP_STAILQ_FIRST(&mc->mc_om_list); bc; bc = onext) {
		meta_search_candidate_t ret;
		onext = LDAP_STAILQ_NEXT(bc, bc_next);
		if (bc->candidates[candidate].sr_msgid == META_MSGID_NEED_BIND)
			bc->candidates[candidate].sr_msgid = META_MSGID_GOT_BIND;
		if (bc->candidates[candidate].sr_msgid != META_MSGID_GOT_BIND || bc->bc_active > 0 || bc->op->o_abandon > 0) {
			continue;
		}
		bc->op->o_threadctx = ctx;
		bc->op->o_tid = ldap_pvt_thread_pool_tid( ctx );
		slap_sl_mem_setctx(ctx, bc->op->o_tmpmemctx);
		bc->bc_active++;
		ret = asyncmeta_send_pending_op(bc, candidate);
		if (ret != META_SEARCH_CANDIDATE) {
			bc->candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
			bc->candidates[ candidate ].sr_type = REP_RESULT;
			bc->candidates[ candidate ].sr_err = bc->rs.sr_err;
			if (bc->op->o_tag != LDAP_REQ_SEARCH || (META_BACK_ONERR_STOP( mi )) ||
			    (asyncmeta_is_last_result(mc, bc, candidate) == 0)) {
				LDAP_STAILQ_REMOVE(&mc->mc_om_list, bc, bm_context_t, bc_next);
				mc->pending_ops--;
				asyncmeta_send_ldap_result(bc, bc->op, &bc->rs);
				asyncmeta_clear_bm_context(bc);
			}
		} else {
			bc->bc_active--;
		}
	}
	msc->msc_active--;

	if ( dolock )
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );

	return META_SEARCH_CANDIDATE;
}

meta_search_candidate_t
asyncmeta_return_bind_errors(a_metaconn_t *mc, int candidate, SlapReply *bind_result, void *ctx, int dolock)
{
	a_metainfo_t	*mi = mc->mc_info;
	bm_context_t *bc, *onext;

	if ( dolock )
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );

	for (bc = LDAP_STAILQ_FIRST(&mc->mc_om_list); bc; bc = onext) {
		onext = LDAP_STAILQ_NEXT(bc, bc_next);
		if (bc->candidates[candidate].sr_msgid != META_MSGID_NEED_BIND
		    || bc->bc_active > 0 || bc->op->o_abandon > 0) {
			continue;
		}
		bc->candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		bc->candidates[ candidate ].sr_type = REP_RESULT;
		bc->candidates[ candidate ].sr_err = bind_result->sr_err;
		if (bc->op->o_tag != LDAP_REQ_SEARCH || (META_BACK_ONERR_STOP( mi )) ||
		    (asyncmeta_is_last_result(mc, bc, candidate) == 0)) {
			LDAP_STAILQ_REMOVE(&mc->mc_om_list, bc, bm_context_t, bc_next);
			bc->op->o_threadctx = ctx;
			bc->op->o_tid = ldap_pvt_thread_pool_tid( ctx );
			slap_sl_mem_setctx(ctx, bc->op->o_tmpmemctx);
			bc->rs.sr_err = bind_result->sr_err;
			bc->rs.sr_text = bind_result->sr_text;
			mc->pending_ops--;
			asyncmeta_send_ldap_result(bc, bc->op, &bc->rs);
			asyncmeta_clear_bm_context(bc);
		}
	}

	if ( dolock )
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );

	return META_SEARCH_CANDIDATE;
}

static meta_search_candidate_t
asyncmeta_handle_bind_result(LDAPMessage *msg, a_metaconn_t *mc, int candidate, void *ctx)
{
	meta_search_candidate_t	retcode;
	SlapReply bind_result = {0};
	/* could modify the msc, safer to lock it */
	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	retcode = asyncmeta_dobind_result( mc, candidate, &bind_result, msg );
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	if ( retcode == META_SEARCH_CANDIDATE ) {
		/* send the remaining pending ops */
		asyncmeta_send_all_pending_ops(mc, candidate, ctx, 1);
	} else {
		asyncmeta_return_bind_errors(mc, candidate, &bind_result, ctx, 1);
	}
	return retcode;
}

int
asyncmeta_handle_search_msg(LDAPMessage *res, a_metaconn_t *mc, bm_context_t *bc, int candidate)
{
	a_metainfo_t	*mi;
	a_metatarget_t	*mt;
	a_metasingleconn_t *msc;
	Operation *op = bc->op;
	SlapReply *rs;
	int	   i, rc = LDAP_SUCCESS, sres;
	SlapReply *candidates;
	char		**references = NULL;
	LDAPControl	**ctrls = NULL;
	a_dncookie dc;
	LDAPMessage *msg;
	ber_int_t id;

	rs = &bc->rs;
	mi = mc->mc_info;
	mt = mi->mi_targets[ candidate ];
	msc = &mc->mc_conns[ candidate ];
	dc.op = op;
	dc.target = mt;
	dc.to_from = MASSAGE_REP;
	id = ldap_msgid(res);


	candidates = bc->candidates;
	i = candidate;

	while (res && !META_BACK_CONN_INVALID(msc)) {
	for (msg = ldap_first_message(msc->msc_ldr, res); msg; msg = ldap_next_message(msc->msc_ldr, msg)) {
		switch(ldap_msgtype(msg)) {
		case LDAP_RES_SEARCH_ENTRY:
			Debug( LDAP_DEBUG_TRACE,
				"%s asyncmeta_handle_search_msg: msc %p entry\n",
				op->o_log_prefix, msc );
			if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
				/* don't retry any more... */
				candidates[ i ].sr_type = REP_RESULT;
			}
			/* count entries returned by target */
			candidates[ i ].sr_nentries++;
			if (bc->c_peer_name.bv_val == op->o_conn->c_peer_name.bv_val && !op->o_abandon) {
				rs->sr_err = asyncmeta_send_entry( &bc->copy_op, rs, mc, i, msg );
			} else {
				goto err_cleanup;
			}
			switch ( rs->sr_err ) {
			case LDAP_SIZELIMIT_EXCEEDED:
				asyncmeta_send_ldap_result(bc, op, rs);
				rs->sr_err = LDAP_SUCCESS;
				goto err_cleanup;
			case LDAP_UNAVAILABLE:
				rs->sr_err = LDAP_OTHER;
				break;
			default:
				break;
			}
			bc->is_ok++;
			break;

		case LDAP_RES_SEARCH_REFERENCE:
			if ( META_BACK_TGT_NOREFS( mt ) ) {
				rs->sr_err = LDAP_OTHER;
				asyncmeta_send_ldap_result(bc, op, rs);
				goto err_cleanup;
			}
			if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
				/* don't retry any more... */
				candidates[ i ].sr_type = REP_RESULT;
			}
			bc->is_ok++;
			rc = ldap_parse_reference( msc->msc_ldr, msg,
				   &references, &rs->sr_ctrls, 0 );

			if ( rc != LDAP_SUCCESS || references == NULL ) {
				rs->sr_err = LDAP_OTHER;
				asyncmeta_send_ldap_result(bc, op, rs);
				goto err_cleanup;
			}

			/* FIXME: merge all and return at the end */

			{
				int cnt;
				for ( cnt = 0; references[ cnt ]; cnt++ )
					;

				rs->sr_ref = op->o_tmpalloc( sizeof( struct berval ) * ( cnt + 1 ),
								 op->o_tmpmemctx );

				for ( cnt = 0; references[ cnt ]; cnt++ ) {
					ber_str2bv_x( references[ cnt ], 0, 1, &rs->sr_ref[ cnt ],
							  op->o_tmpmemctx );
				}
				BER_BVZERO( &rs->sr_ref[ cnt ] );
			}

			{
				dc.memctx = op->o_tmpmemctx;
				( void )asyncmeta_referral_result_rewrite( &dc, rs->sr_ref );
			}

			if ( rs->sr_ref != NULL ) {
				if (!BER_BVISNULL( &rs->sr_ref[ 0 ] ) ) {
					/* ignore return value by now */
					( void )send_search_reference( op, rs );
				}

				ber_bvarray_free_x( rs->sr_ref, op->o_tmpmemctx );
				rs->sr_ref = NULL;
			}

			/* cleanup */
			if ( references ) {
				ber_memvfree( (void **)references );
			}

			if ( rs->sr_ctrls ) {
				ldap_controls_free( rs->sr_ctrls );
				rs->sr_ctrls = NULL;
			}
			break;

		case LDAP_RES_INTERMEDIATE:
			if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
				/* don't retry any more... */
				candidates[ i ].sr_type = REP_RESULT;
			}
			bc->is_ok++;

			/* FIXME: response controls
			 * are passed without checks */
			rs->sr_err = ldap_parse_intermediate( msc->msc_ldr,
								  msg,
								  (char **)&rs->sr_rspoid,
								  &rs->sr_rspdata,
								  &rs->sr_ctrls,
								  0 );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				candidates[ i ].sr_type = REP_RESULT;
				rs->sr_err = LDAP_OTHER;
				asyncmeta_send_ldap_result(bc, op, rs);
				goto err_cleanup;
			}

			slap_send_ldap_intermediate( op, rs );

			if ( rs->sr_rspoid != NULL ) {
				ber_memfree( (char *)rs->sr_rspoid );
				rs->sr_rspoid = NULL;
			}

			if ( rs->sr_rspdata != NULL ) {
				ber_bvfree( rs->sr_rspdata );
				rs->sr_rspdata = NULL;
			}

			if ( rs->sr_ctrls != NULL ) {
				ldap_controls_free( rs->sr_ctrls );
				rs->sr_ctrls = NULL;
			}
			break;

		case LDAP_RES_SEARCH_RESULT:
			if ( mi->mi_idle_timeout != 0 ) {
				asyncmeta_set_msc_time(msc);
			}
			Debug( LDAP_DEBUG_TRACE,
			       "%s asyncmeta_handle_search_msg: msc %p result\n",
			       op->o_log_prefix, msc );
			candidates[ i ].sr_type = REP_RESULT;
			candidates[ i ].sr_msgid = META_MSGID_IGNORE;
			/* NOTE: ignores response controls
			 * (and intermediate response controls
			 * as well, except for those with search
			 * references); this may not be correct,
			 * but if they're not ignored then
			 * back-meta would need to merge them
			 * consistently (think of pagedResults...)
			 */
			/* FIXME: response controls? */
			rs->sr_err = ldap_parse_result( msc->msc_ldr,
							msg,
							&candidates[ i ].sr_err,
								(char **)&candidates[ i ].sr_matched,
							(char **)&candidates[ i ].sr_text,
							&references,
							&ctrls /* &candidates[ i ].sr_ctrls (unused) */ ,
							0 );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				candidates[ i ].sr_err = rs->sr_err;
				sres = slap_map_api2result( &candidates[ i ] );
				candidates[ i ].sr_type = REP_RESULT;
				goto finish;
			}

			rs->sr_err = candidates[ i ].sr_err;

			/* massage matchedDN if need be */
			if ( candidates[ i ].sr_matched != NULL ) {
				struct berval	match, mmatch;

				ber_str2bv( candidates[ i ].sr_matched,
						0, 0, &match );
				candidates[ i ].sr_matched = NULL;

				dc.memctx = NULL;
				asyncmeta_dn_massage( &dc, &match, &mmatch );
				if ( mmatch.bv_val == match.bv_val ) {
					candidates[ i ].sr_matched
						= ch_strdup( mmatch.bv_val );

				} else {
					candidates[ i ].sr_matched = mmatch.bv_val;
				}

				bc->candidate_match++;
				ldap_memfree( match.bv_val );
			}

			/* add references to array */
			/* RFC 4511: referrals can only appear
			 * if result code is LDAP_REFERRAL */
			if ( references != NULL
				 && references[ 0 ] != NULL
				 && references[ 0 ][ 0 ] != '\0' )
			{
				if ( rs->sr_err != LDAP_REFERRAL ) {
					Debug( LDAP_DEBUG_ANY,
						   "%s asncmeta_search_result[%d]: "
						   "got referrals with err=%d\n",
						   op->o_log_prefix,
						   i, rs->sr_err );

				} else {
					BerVarray	sr_ref;
					int		cnt;

					for ( cnt = 0; references[ cnt ]; cnt++ )
						;

					sr_ref = op->o_tmpalloc( sizeof( struct berval ) * ( cnt + 1 ),
								 op->o_tmpmemctx );

					for ( cnt = 0; references[ cnt ]; cnt++ ) {
						ber_str2bv_x( references[ cnt ], 0, 1, &sr_ref[ cnt ],
								  op->o_tmpmemctx );
					}
					BER_BVZERO( &sr_ref[ cnt ] );

					dc.memctx = op->o_tmpmemctx;
					( void )asyncmeta_referral_result_rewrite( &dc, sr_ref );

					if ( rs->sr_v2ref == NULL ) {
						rs->sr_v2ref = sr_ref;

					} else {
						for ( cnt = 0; !BER_BVISNULL( &sr_ref[ cnt ] ); cnt++ ) {
							ber_bvarray_add_x( &rs->sr_v2ref, &sr_ref[ cnt ],
									   op->o_tmpmemctx );
						}
						ber_memfree_x( sr_ref, op->o_tmpmemctx );
					}
				}

			} else if ( rs->sr_err == LDAP_REFERRAL ) {
				Debug( LDAP_DEBUG_TRACE,
					   "%s asyncmeta_search_result[%d]: "
					   "got err=%d with null "
					   "or empty referrals\n",
					   op->o_log_prefix,
					   i, rs->sr_err );

				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			}

			/* cleanup */
			ber_memvfree( (void **)references );

			sres = slap_map_api2result( rs );

			if ( candidates[ i ].sr_err == LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_search_result[%d] "
				       "match=\"%s\" err=%ld\n",
				       op->o_log_prefix, i,
				       candidates[ i ].sr_matched ? candidates[ i ].sr_matched : "",
				       (long) candidates[ i ].sr_err );
			} else {
					Debug( LDAP_DEBUG_ANY,  "%s asyncmeta_search_result[%d] "
				       "match=\"%s\" err=%ld (%s)\n",
				       op->o_log_prefix, i,
				       candidates[ i ].sr_matched ? candidates[ i ].sr_matched : "",
					       (long) candidates[ i ].sr_err, ldap_err2string( candidates[ i ].sr_err ) );
			}

			switch ( sres ) {
			case LDAP_NO_SUCH_OBJECT:
				/* is_ok is touched any time a valid
				 * (even intermediate) result is
				 * returned; as a consequence, if
				 * a candidate returns noSuchObject
				 * it is ignored and the candidate
				 * is simply demoted. */
				if ( bc->is_ok ) {
					sres = LDAP_SUCCESS;
				}
				break;

			case LDAP_SUCCESS:
				if ( ctrls != NULL && ctrls[0] != NULL ) {
#ifdef SLAPD_META_CLIENT_PR
					LDAPControl *pr_c;

					pr_c = ldap_control_find( LDAP_CONTROL_PAGEDRESULTS, ctrls, NULL );
					if ( pr_c != NULL ) {
						BerElementBuffer berbuf;
						BerElement *ber = (BerElement *)&berbuf;
						ber_tag_t tag;
						ber_int_t prsize;
						struct berval prcookie;

						/* unsolicited, do not accept */
						if ( mt->mt_ps == 0 ) {
							rs->sr_err = LDAP_OTHER;
							goto err_pr;
						}

						ber_init2( ber, &pr_c->ldctl_value, LBER_USE_DER );

						tag = ber_scanf( ber, "{im}", &prsize, &prcookie );
						if ( tag == LBER_ERROR ) {
							rs->sr_err = LDAP_OTHER;
							goto err_pr;
						}

						/* more pages? new search request */
						if ( !BER_BVISNULL( &prcookie ) && !BER_BVISEMPTY( &prcookie ) ) {
							if ( mt->mt_ps > 0 ) {
								/* ignore size if specified */
								prsize = 0;

							} else if ( prsize == 0 ) {
								/* guess the page size from the entries returned so far */
								prsize = candidates[ i ].sr_nentries;
							}

							candidates[ i ].sr_nentries = 0;
							candidates[ i ].sr_msgid = META_MSGID_IGNORE;
							candidates[ i ].sr_type = REP_INTERMEDIATE;

							assert( candidates[ i ].sr_matched == NULL );
							assert( candidates[ i ].sr_text == NULL );
							assert( candidates[ i ].sr_ref == NULL );

							switch ( asyncmeta_back_search_start( &bc->copy_op, rs, mc, bc, i, &prcookie, prsize, 1 ) )
							{
							case META_SEARCH_CANDIDATE:
								assert( candidates[ i ].sr_msgid >= 0 );
								ldap_controls_free( ctrls );
								//	goto free_message;

							case META_SEARCH_ERR:
							case META_SEARCH_NEED_BIND:
err_pr:;
								candidates[ i ].sr_err = rs->sr_err;
								candidates[ i ].sr_type = REP_RESULT;
								if ( META_BACK_ONERR_STOP( mi ) ) {
									asyncmeta_send_ldap_result(bc, op, rs);
									ldap_controls_free( ctrls );
									goto err_cleanup;
								}
								/* fallthru */

							case META_SEARCH_NOT_CANDIDATE:
								/* means that asyncmeta_back_search_start()
								 * failed but onerr == continue */
								candidates[ i ].sr_msgid = META_MSGID_IGNORE;
								candidates[ i ].sr_type = REP_RESULT;
								break;

							default:
								/* impossible */
								assert( 0 );
								break;
							}
							break;
						}
					}
#endif /* SLAPD_META_CLIENT_PR */

					ldap_controls_free( ctrls );
				}
				/* fallthru */

			case LDAP_REFERRAL:
				bc->is_ok++;
				break;

			case LDAP_SIZELIMIT_EXCEEDED:
				/* if a target returned sizelimitExceeded
				 * and the entry count is equal to the
				 * proxy's limit, the target would have
				 * returned more, and the error must be
				 * propagated to the client; otherwise,
				 * the target enforced a limit lower
				 * than what requested by the proxy;
				 * ignore it */
				candidates[ i ].sr_err = rs->sr_err;
				if ( rs->sr_nentries == op->ors_slimit
					 || META_BACK_ONERR_STOP( mi ) )
				{
					const char *save_text;
got_err:
					save_text = rs->sr_text;
					rs->sr_text = candidates[ i ].sr_text;
					asyncmeta_send_ldap_result(bc, op, rs);
					if (candidates[ i ].sr_text != NULL) {
						ch_free( (char *)candidates[ i ].sr_text );
						candidates[ i ].sr_text = NULL;
					}
					rs->sr_text = save_text;
					ldap_controls_free( ctrls );
					goto err_cleanup;
				}
				break;

			default:
				candidates[ i ].sr_err = rs->sr_err;
				if ( META_BACK_ONERR_STOP( mi ) ) {
					goto got_err;
				}
				break;
			}
			/* if this is the last result we will ever receive, send it back  */
			rc = rs->sr_err;
			if (asyncmeta_is_last_result(mc, bc, i) == 0) {
				Debug( LDAP_DEBUG_TRACE,
					"%s asyncmeta_handle_search_msg: msc %p last result\n",
					op->o_log_prefix, msc );
				asyncmeta_search_last_result(mc, bc, i, sres);
err_cleanup:
				rc = rs->sr_err;
				ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
				asyncmeta_drop_bc( mc, bc);
				asyncmeta_clear_bm_context(bc);
				ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
				ldap_msgfree(res);
				return rc;
			}
finish:
			break;

		default:
			continue;
		}
	}
		ldap_msgfree(res);
		res = NULL;
		if (candidates[ i ].sr_type != REP_RESULT) {
			struct timeval	tv = {0};
			rc = ldap_result( msc->msc_ldr, id, LDAP_MSG_RECEIVED, &tv, &res );
			if (res != NULL) {
				msc->msc_result_time = slap_get_time();
			}
		}
	}
	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	bc->bc_active--;
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );

	return rc;
}

/* handles the received result for add, modify, modrdn, compare and delete ops */

int asyncmeta_handle_common_result(LDAPMessage *msg, a_metaconn_t *mc, bm_context_t *bc, int candidate)
{
	a_metainfo_t	*mi;
	a_metatarget_t	*mt;
	a_metasingleconn_t *msc;
	const char	*save_text = NULL,
		*save_matched = NULL;
	BerVarray	save_ref = NULL;
	LDAPControl	**save_ctrls = NULL;
	void		*matched_ctx = NULL;

	char		*matched = NULL;
	char		*text = NULL;
	char		**refs = NULL;
	LDAPControl	**ctrls = NULL;
	Operation *op;
	SlapReply *rs;
	int		rc;

	mi = mc->mc_info;
	mt = mi->mi_targets[ candidate ];
	msc = &mc->mc_conns[ candidate ];

	op = bc->op;
	rs = &bc->rs;
	save_text = rs->sr_text,
	save_matched = rs->sr_matched;
	save_ref = rs->sr_ref;
	save_ctrls = rs->sr_ctrls;
	rs->sr_text = NULL;
	rs->sr_matched = NULL;
	rs->sr_ref = NULL;
	rs->sr_ctrls = NULL;

	/* only touch when activity actually took place... */
	if ( mi->mi_idle_timeout != 0 ) {
		asyncmeta_set_msc_time(msc);
	}

	rc = ldap_parse_result( msc->msc_ldr, msg, &rs->sr_err,
				&matched, &text, &refs, &ctrls, 0 );

	if ( rc == LDAP_SUCCESS ) {
		rs->sr_text = text;
	} else {
		rs->sr_err = rc;
	}
	rs->sr_err = slap_map_api2result( rs );

	/* RFC 4511: referrals can only appear
	 * if result code is LDAP_REFERRAL */
	if ( refs != NULL
	     && refs[ 0 ] != NULL
	     && refs[ 0 ][ 0 ] != '\0' )
	{
		if ( rs->sr_err != LDAP_REFERRAL ) {
			Debug( LDAP_DEBUG_ANY,
			       "%s asyncmeta_handle_common_result[%d]: "
			       "got referrals with err=%d\n",
			       op->o_log_prefix,
			       candidate, rs->sr_err );

		} else {
			int	i;

			for ( i = 0; refs[ i ] != NULL; i++ )
				/* count */ ;
			rs->sr_ref = op->o_tmpalloc( sizeof( struct berval ) * ( i + 1 ),
						     op->o_tmpmemctx );
			for ( i = 0; refs[ i ] != NULL; i++ ) {
				ber_str2bv( refs[ i ], 0, 0, &rs->sr_ref[ i ] );
			}
			BER_BVZERO( &rs->sr_ref[ i ] );
		}

	} else if ( rs->sr_err == LDAP_REFERRAL ) {
		Debug( LDAP_DEBUG_ANY,
		       "%s asyncmeta_handle_common_result[%d]: "
		       "got err=%d with null "
		       "or empty referrals\n",
		       op->o_log_prefix,
		       candidate, rs->sr_err );

		rs->sr_err = LDAP_NO_SUCH_OBJECT;
	}

	if ( ctrls != NULL ) {
		rs->sr_ctrls = ctrls;
	}

	/* if the error in the reply structure is not
	 * LDAP_SUCCESS, try to map it from client
	 * to server error */
	if ( !LDAP_ERR_OK( rs->sr_err ) ) {
		rs->sr_err = slap_map_api2result( rs );

		/* internal ops ( op->o_conn == NULL )
		 * must not reply to client */
		if ( op->o_conn && !op->o_do_not_cache && matched ) {

			/* record the (massaged) matched
			 * DN into the reply structure */
			rs->sr_matched = matched;
		}
	}

	if ( META_BACK_TGT_QUARANTINE( mt ) ) {
		asyncmeta_quarantine( op, mi, rs, candidate );
	}

	if ( matched != NULL ) {
		struct berval	dn, pdn;

		ber_str2bv( matched, 0, 0, &dn );
		if ( dnPretty( NULL, &dn, &pdn, op->o_tmpmemctx ) == LDAP_SUCCESS ) {
			ldap_memfree( matched );
			matched_ctx = op->o_tmpmemctx;
			matched = pdn.bv_val;
		}
		rs->sr_matched = matched;
	}

	if ( rs->sr_err == LDAP_UNAVAILABLE || rs->sr_err == LDAP_SERVER_DOWN ) {
		if ( rs->sr_text == NULL ) {
				rs->sr_text = "Target is unavailable";
			}
	}

	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	asyncmeta_drop_bc( mc, bc);
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );

	if ( op->o_conn ) {
		asyncmeta_send_ldap_result(bc, op, rs);
	}

	if ( matched ) {
		op->o_tmpfree( (char *)rs->sr_matched, matched_ctx );
	}
	if ( text ) {
		ldap_memfree( text );
	}
	if ( rs->sr_ref ) {
		op->o_tmpfree( rs->sr_ref, op->o_tmpmemctx );
		rs->sr_ref = NULL;
	}
	if ( refs ) {
		ber_memvfree( (void **)refs );
	}
	if ( ctrls ) {
		assert( rs->sr_ctrls != NULL );
		ldap_controls_free( ctrls );
	}

	rs->sr_text = save_text;
	rs->sr_matched = save_matched;
	rs->sr_ref = save_ref;
	rs->sr_ctrls = save_ctrls;
	rc = (LDAP_ERR_OK( rs->sr_err ) ? LDAP_SUCCESS : rs->sr_err);
	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	asyncmeta_clear_bm_context(bc);
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	return rc;
}

/* This takes care to clean out the outbound queue in case we have a read error
 * sending back responses to the client */
int
asyncmeta_op_read_error(a_metaconn_t *mc, int candidate, int error, void* ctx)
{
	bm_context_t *bc, *onext;
	int cleanup;
	Operation *op;
	SlapReply *rs;
	SlapReply *candidates;

	/* no outstanding ops, nothing to do but log */
	Debug( LDAP_DEBUG_TRACE,
	       "asyncmeta_op_read_error: ldr=%p, err=%d\n",
	       mc->mc_conns[candidate].msc_ldr, error );

	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	/*someone may be trying to write */
	if (mc->mc_conns[candidate].msc_active <= 1) {
		asyncmeta_clear_one_msc(NULL, mc, candidate, 0, __FUNCTION__);
	} else {
		META_BACK_CONN_INVALID_SET(&mc->mc_conns[candidate]);
	}

	if (mc->pending_ops <= 0) {
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
		return LDAP_SUCCESS;
	}

	for (bc = LDAP_STAILQ_FIRST(&mc->mc_om_list); bc; bc = onext) {
		onext = LDAP_STAILQ_NEXT(bc, bc_next);
		cleanup = 0;
		candidates = bc->candidates;
		/* was this op affected? */
		if ( !META_IS_CANDIDATE( &candidates[ candidate ] ) )
			continue;

		if (bc->op->o_abandon) {
			bc->bc_invalid = 1;
			continue;
		}

		if (bc->bc_active > 0) {
			bc->bc_invalid = 1;
			continue;
		}

		bc->op->o_threadctx = ctx;
		bc->op->o_tid = ldap_pvt_thread_pool_tid( ctx );
		slap_sl_mem_setctx(ctx, bc->op->o_tmpmemctx);

		op = bc->op;
		rs = &bc->rs;
		switch (op->o_tag) {
		case LDAP_REQ_ADD:
		case LDAP_REQ_MODIFY:
		case LDAP_REQ_MODRDN:
		case LDAP_REQ_COMPARE:
		case LDAP_REQ_DELETE:
			rs->sr_err = LDAP_UNAVAILABLE;
			rs->sr_text = "Read error on connection to target";
			asyncmeta_send_ldap_result( bc, op, rs );
			cleanup = 1;
			break;
		case LDAP_REQ_SEARCH:
		{
			a_metainfo_t *mi = mc->mc_info;
			rs->sr_err = LDAP_UNAVAILABLE;
			rs->sr_text = "Read error on connection to target";
			candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
			candidates[ candidate ].sr_type = REP_RESULT;
			if ( (META_BACK_ONERR_STOP( mi ) ||
			      asyncmeta_is_last_result(mc, bc, candidate)) && op->o_conn) {
				asyncmeta_send_ldap_result( bc, op, rs );
				cleanup = 1;
			}
		}
			break;
		default:
			break;
		}

		if (cleanup) {
			int j;
			a_metainfo_t *mi = mc->mc_info;
			for (j=0; j<mi->mi_ntargets; j++) {
				if (j != candidate && bc->candidates[j].sr_msgid >= 0
				    && mc->mc_conns[j].msc_ld != NULL) {
					asyncmeta_back_cancel( mc, op,
							       bc->candidates[ j ].sr_msgid, j );
				}
			}
			LDAP_STAILQ_REMOVE(&mc->mc_om_list, bc, bm_context_t, bc_next);
			mc->pending_ops--;
			asyncmeta_clear_bm_context(bc);
		}
	}
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	return LDAP_SUCCESS;
}

void *
asyncmeta_op_handle_result(void *ctx, void *arg)
{
	a_metaconn_t *mc = arg;
	int		i, j, rc, ntargets;
	struct timeval	tv = {0};
	LDAPMessage     *msg;
	a_metasingleconn_t *msc;
	bm_context_t *bc;
	void *oldctx;

	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	rc = ++mc->mc_active;
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	if (rc > 1)
		return NULL;

	ntargets = mc->mc_info->mi_ntargets;
	i = ntargets;
	oldctx = slap_sl_mem_create(SLAP_SLAB_SIZE, SLAP_SLAB_STACK, ctx, 0);	/* get existing memctx */

again:
	for (j=0; j<ntargets; j++) {
		i++;
		if (i >= ntargets) i = 0;
		msc = &mc->mc_conns[i];
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
		if (!mc->mc_conns[i].msc_ldr ||
		    META_BACK_CONN_CREATING( &mc->mc_conns[i] ) ||
		    META_BACK_CONN_INVALID(&mc->mc_conns[i])) {
			ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
			continue;
		}

		msc->msc_active++;
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );

		rc = ldap_result( mc->mc_conns[i].msc_ldr, LDAP_RES_ANY, LDAP_MSG_RECEIVED, &tv, &msg );
		if (rc < 1) {
			if (rc < 0) {
				ldap_get_option( mc->mc_conns[i].msc_ldr, LDAP_OPT_ERROR_NUMBER, &rc);
				META_BACK_CONN_INVALID_SET(&mc->mc_conns[i]);
				asyncmeta_op_read_error(mc, i, rc, ctx);
			}
			ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
			msc->msc_active--;
			ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
			continue;
		}
		rc = ldap_msgtype( msg );
		if (rc == LDAP_RES_BIND) {
			if ( LogTest( asyncmeta_debug ) ) {
				char	time_buf[ SLAP_TEXT_BUFLEN ];
				asyncmeta_get_timestamp(time_buf);
				Debug( asyncmeta_debug, "[%s] asyncmeta_op_handle_result received bind msgid=%d msc: %p\n",
				      time_buf, ldap_msgid(msg), msc );
			}
			asyncmeta_handle_bind_result(msg, mc, i, ctx);
			mc->mc_info->mi_targets[i]->mt_timeout_ops = 0;
			ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
			msc->msc_result_time = slap_get_time();
			msc->msc_active--;
			ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
			if (msg)
				ldap_msgfree(msg);

			continue;
		}
retry_bc:
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
		bc = asyncmeta_find_message(ldap_msgid(msg), mc, i);
/* The sender might not be yet done with the context. On error it might also remove it
 * so it's best to try and find it again after a wait */
		if (bc && bc->bc_active > 0) {
			ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
			ldap_pvt_thread_yield();
			goto retry_bc;
		}
		if (bc) {
			bc->bc_active++;
		}

		msc->msc_result_time = slap_get_time();
		msc->msc_active--;
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
		if (!bc) {
			Debug( asyncmeta_debug,
				"asyncmeta_op_handle_result: Unable to find bc for msguid %d, msc: %p\n", ldap_msgid(msg), msc );
			ldap_msgfree(msg);
			continue;
		}

		/* set our memctx */
		bc->op->o_threadctx = ctx;
		bc->op->o_tid = ldap_pvt_thread_pool_tid( ctx );
		slap_sl_mem_setctx(ctx, bc->op->o_tmpmemctx);
		if (bc->op->o_abandon) {
			ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
			asyncmeta_drop_bc( mc, bc);
			if ( bc->op->o_tag == LDAP_REQ_SEARCH ) {
				int j;
				for (j=0; j<ntargets; j++) {
					if (bc->candidates[j].sr_msgid >= 0) {
						a_metasingleconn_t *tmp_msc = &mc->mc_conns[j];
						tmp_msc->msc_active++;
						asyncmeta_back_cancel( mc, bc->op,
								       bc->candidates[ j ].sr_msgid, j );
						tmp_msc->msc_active--;
					}
				}
			}
			asyncmeta_clear_bm_context(bc);
			ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
			if (msg)
				ldap_msgfree(msg);
			continue;
		}

		switch (rc) {
		case LDAP_RES_SEARCH_ENTRY:
		case LDAP_RES_SEARCH_REFERENCE:
		case LDAP_RES_SEARCH_RESULT:
		case LDAP_RES_INTERMEDIATE:
			asyncmeta_handle_search_msg(msg, mc, bc, i);
			mc->mc_info->mi_targets[i]->mt_timeout_ops = 0;
			msg = NULL;
			break;
		case LDAP_RES_ADD:
		case LDAP_RES_DELETE:
		case LDAP_RES_MODDN:
		case LDAP_RES_COMPARE:
		case LDAP_RES_MODIFY:
			rc = asyncmeta_handle_common_result(msg, mc, bc, i);
			mc->mc_info->mi_targets[i]->mt_timeout_ops = 0;
			break;
		default:
			{
			Debug( asyncmeta_debug,
				   "asyncmeta_op_handle_result: "
				   "unrecognized response message tag=%d\n",
				   rc );

			}
		}
		if (msg)
			ldap_msgfree(msg);
	}

	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	rc = --mc->mc_active;
	if (rc) {
		i++;
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
		goto again;
	}
	slap_sl_mem_setctx(ctx, oldctx);
	if (mc->mc_conns) {
		for (i=0; i<ntargets; i++) {
			if (!slapd_shutdown && !META_BACK_CONN_INVALID(msc)
			    && mc->mc_conns[i].msc_ldr && mc->mc_conns[i].conn) {
				connection_client_enable(mc->mc_conns[i].conn);
			}
		}
	}
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	return NULL;
}

void asyncmeta_set_msc_time(a_metasingleconn_t *msc)
{
	msc->msc_time = slap_get_time();
}

void* asyncmeta_timeout_loop(void *ctx, void *arg)
{
	struct re_s* rtask = arg;
	a_metainfo_t *mi = rtask->arg;
	bm_context_t *bc, *onext;
	time_t current_time = slap_get_time();
	int i, j;
	LDAP_STAILQ_HEAD(BCList, bm_context_t) timeout_list;
	LDAP_STAILQ_INIT( &timeout_list );

	Debug( asyncmeta_debug, "asyncmeta_timeout_loop[%p] start at [%ld] \n", rtask, current_time );
	void *oldctx = slap_sl_mem_create(SLAP_SLAB_SIZE, SLAP_SLAB_STACK, ctx, 0);
	for (i=0; i<mi->mi_num_conns; i++) {
		a_metaconn_t * mc= &mi->mi_conns[i];
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
		for (bc = LDAP_STAILQ_FIRST(&mc->mc_om_list); bc; bc = onext) {
			onext = LDAP_STAILQ_NEXT(bc, bc_next);
			if (bc->bc_active > 0) {
				continue;
			}

			if (bc->op->o_abandon ) {
					/* set our memctx */
				bc->op->o_threadctx = ctx;
				bc->op->o_tid = ldap_pvt_thread_pool_tid( ctx );
				slap_sl_mem_setctx(ctx, bc->op->o_tmpmemctx);
				Operation *op = bc->op;

				LDAP_STAILQ_REMOVE(&mc->mc_om_list, bc, bm_context_t, bc_next);
				mc->pending_ops--;
				for (j=0; j<mi->mi_ntargets; j++) {
					if (bc->candidates[j].sr_msgid >= 0) {
						a_metasingleconn_t *msc = &mc->mc_conns[j];
						if ( op->o_tag == LDAP_REQ_SEARCH ) {
							msc->msc_active++;
							asyncmeta_back_cancel( mc, op,
									       bc->candidates[ j ].sr_msgid, j );
							msc->msc_active--;
						}
					}
				}
				asyncmeta_clear_bm_context(bc);
				continue;
			}
			if (bc->bc_invalid) {
				LDAP_STAILQ_REMOVE(&mc->mc_om_list, bc, bm_context_t, bc_next);
				mc->pending_ops--;
				LDAP_STAILQ_INSERT_TAIL( &timeout_list, bc, bc_next);
				continue;
			}

			if (bc->timeout && bc->stoptime < current_time) {
				Operation *op = bc->op;
				LDAP_STAILQ_REMOVE(&mc->mc_om_list, bc, bm_context_t, bc_next);
				mc->pending_ops--;
				LDAP_STAILQ_INSERT_TAIL( &timeout_list, bc, bc_next);
				for (j=0; j<mi->mi_ntargets; j++) {
					if (bc->candidates[j].sr_msgid >= 0) {
						a_metasingleconn_t *msc = &mc->mc_conns[j];
						asyncmeta_set_msc_time(msc);
						if ( op->o_tag == LDAP_REQ_SEARCH ) {
							msc->msc_active++;
							asyncmeta_back_cancel( mc, op,
									       bc->candidates[ j ].sr_msgid, j );
							msc->msc_active--;
						}
					}
				}
			}
		}
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );

		for (bc = LDAP_STAILQ_FIRST(&timeout_list); bc; bc = onext) {
			Operation *op = bc->op;
			SlapReply *rs = &bc->rs;
			int		timeout_err;
			const char *timeout_text;

			onext = LDAP_STAILQ_NEXT(bc, bc_next);
			LDAP_STAILQ_REMOVE(&timeout_list, bc, bm_context_t, bc_next);
			/* set our memctx */
			bc->op->o_threadctx = ctx;
			bc->op->o_tid = ldap_pvt_thread_pool_tid( ctx );
			slap_sl_mem_setctx(ctx, bc->op->o_tmpmemctx);

			if (bc->searchtime) {
				timeout_err = LDAP_TIMELIMIT_EXCEEDED;
			} else {
				timeout_err = op->o_protocol >= LDAP_VERSION3 ?
					LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
			}

			if ( bc->bc_invalid ) {
				timeout_text = "Operation is invalid - target connection has been reset";
			} else {
				a_metasingleconn_t *log_msc =  &mc->mc_conns[0];
				Debug( asyncmeta_debug,
				       "asyncmeta_timeout_loop:Timeout op %s loop[%p], "
				       "current_time:%ld, op->o_time:%ld msc: %p, "
				       "msc->msc_binding_time: %x, msc->msc_flags:%x \n",
				       bc->op->o_log_prefix, rtask, current_time, bc->op->o_time,
				       log_msc, (unsigned int)log_msc->msc_binding_time, log_msc->msc_mscflags );

				if (bc->searchtime) {
					timeout_text = NULL;
				} else {
					timeout_text = "Operation timed out";
				}

				for (j=0; j<mi->mi_ntargets; j++) {
					if (bc->candidates[j].sr_msgid >= 0) {
						a_metatarget_t     *mt = mi->mi_targets[j];
						if (!META_BACK_TGT_QUARANTINE( mt ) ||
						    bc->candidates[j].sr_type == REP_RESULT) {
							continue;
						}

						if (mt->mt_isquarantined > LDAP_BACK_FQ_NO) {
							timeout_err = LDAP_UNAVAILABLE;
						} else {
							mt->mt_timeout_ops++;
							if ((mi->mi_max_timeout_ops > 0) &&
							    (mt->mt_timeout_ops > mi->mi_max_timeout_ops)) {
								timeout_err = LDAP_UNAVAILABLE;
								rs->sr_err = timeout_err;
								if (mt->mt_isquarantined == LDAP_BACK_FQ_NO)
									asyncmeta_quarantine(op, mi, rs, j);
							}
						}
					}
				}
			}
			rs->sr_err = timeout_err;
			rs->sr_text = timeout_text;
			if (!bc->op->o_abandon ) {
				asyncmeta_send_ldap_result( bc, bc->op, &bc->rs );
			}
			asyncmeta_clear_bm_context(bc);
		}

		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
		if (mi->mi_idle_timeout) {
			for (j=0; j<mi->mi_ntargets; j++) {
				a_metasingleconn_t *msc = &mc->mc_conns[j];
				if ( msc->msc_active > 0 ) {
					continue;
				}
				if (mc->pending_ops > 0) {
					continue;
				}
				current_time = slap_get_time();
				if (msc->msc_ld && msc->msc_time > 0 && msc->msc_time + mi->mi_idle_timeout < current_time) {
					asyncmeta_clear_one_msc(NULL, mc, j, 1, __FUNCTION__);
				}
			}
		}
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	}

	slap_sl_mem_setctx(ctx, oldctx);
	current_time = slap_get_time();
	Debug( asyncmeta_debug, "asyncmeta_timeout_loop[%p] stop at [%ld] \n", rtask, current_time );
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return NULL;
}

