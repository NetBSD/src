/*	$NetBSD: search.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* search.c - search request handler for back-asyncmeta */
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
__RCSID("$NetBSD: search.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include "slap.h"
#include "../../../libraries/liblber/lber-int.h"
#include "../../../libraries/libldap/ldap-int.h"
#include "lutil.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

static void
asyncmeta_handle_onerr_stop(Operation *op,
			    SlapReply *rs,
			    a_metaconn_t *mc,
			    bm_context_t *bc,
			    int candidate)
{
	a_metainfo_t *mi = mc->mc_info;
	int j;
	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex);
	if (asyncmeta_bc_in_queue(mc,bc) == NULL || bc->bc_active > 1) {
		bc->bc_active--;
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);
		return;
	}
	asyncmeta_drop_bc(mc, bc);
	for (j=0; j<mi->mi_ntargets; j++) {
		if (j != candidate && bc->candidates[j].sr_msgid >= 0
		    && mc->mc_conns[j].msc_ld != NULL && !META_BACK_CONN_CREATING( &mc->mc_conns[j] )) {
			asyncmeta_back_cancel( mc, op,
						bc->candidates[ j ].sr_msgid, j );
		}
	}
	slap_sl_mem_setctx(op->o_threadctx, op->o_tmpmemctx);
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);
	send_ldap_result(op, rs);
}

static int
asyncmeta_int_filter2bv( a_dncookie		*dc,
			 Filter			*f,
			 struct berval	*fstr )
{
	int		i;
	Filter		*p;
	struct berval	atmp,
			vtmp,
			ntmp,
			*tmp;
	static struct berval
			/* better than nothing... */
			ber_bvfalse = BER_BVC( "(!(objectClass=*))" ),
			ber_bvtf_false = BER_BVC( "(|)" ),
			/* better than nothing... */
			ber_bvtrue = BER_BVC( "(objectClass=*)" ),
			ber_bvtf_true = BER_BVC( "(&)" ),
			ber_bverror = BER_BVC( "(?=error)" ),
			ber_bvunknown = BER_BVC( "(?=unknown)" ),
			ber_bvnone = BER_BVC( "(?=none)" );
	ber_len_t	len;
	void *memctx = dc->memctx;

	assert( fstr != NULL );
	BER_BVZERO( fstr );

	if ( f == NULL ) {
		ber_dupbv_x( fstr, &ber_bvnone, memctx );
		return LDAP_OTHER;
	}

	switch ( ( f->f_choice & SLAPD_FILTER_MASK ) ) {
	case LDAP_FILTER_EQUALITY:
		if ( f->f_av_desc->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) {
			asyncmeta_dn_massage( dc, &f->f_av_value, &vtmp );
		} else {
			vtmp = f->f_av_value;
		}

		filter_escape_value_x( &vtmp, &ntmp, memctx );
		fstr->bv_len = f->f_av_desc->ad_cname.bv_len + ntmp.bv_len
			+ ( sizeof("(=)") - 1 );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 1, memctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s=%s)",
			f->f_av_desc->ad_cname.bv_val, ntmp.bv_len ? ntmp.bv_val : "" );

		ber_memfree_x( ntmp.bv_val, memctx );
		break;

	case LDAP_FILTER_GE:
		filter_escape_value_x( &f->f_av_value, &ntmp, memctx );
		fstr->bv_len = f->f_av_desc->ad_cname.bv_len + ntmp.bv_len
			+ ( sizeof("(>=)") - 1 );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 1, memctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s>=%s)",
			f->f_av_desc->ad_cname.bv_val, ntmp.bv_len ? ntmp.bv_val : "" );

		ber_memfree_x( ntmp.bv_val, memctx );
		break;

	case LDAP_FILTER_LE:
		filter_escape_value_x( &f->f_av_value, &ntmp, memctx );
		fstr->bv_len = f->f_av_desc->ad_cname.bv_len + ntmp.bv_len
			+ ( sizeof("(<=)") - 1 );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 1, memctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s<=%s)",
			f->f_av_desc->ad_cname.bv_val,  ntmp.bv_len ? ntmp.bv_val : "" );

		ber_memfree_x( ntmp.bv_val, memctx );
		break;

	case LDAP_FILTER_APPROX:
		filter_escape_value_x( &f->f_av_value, &ntmp, memctx );
		fstr->bv_len = f->f_av_desc->ad_cname.bv_len + ntmp.bv_len
			+ ( sizeof("(~=)") - 1 );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 1, memctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s~=%s)",
			f->f_av_desc->ad_cname.bv_val,  ntmp.bv_len ? ntmp.bv_val : "" );

		ber_memfree_x( ntmp.bv_val, memctx );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		fstr->bv_len = f->f_sub_desc->ad_cname.bv_len + ( STRLENOF( "(=*)" ) );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 128, memctx ); /* FIXME: why 128 ? */

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s=*)",
			f->f_sub_desc->ad_cname.bv_val );

		if ( !BER_BVISNULL( &f->f_sub_initial ) ) {
			len = fstr->bv_len;

			filter_escape_value_x( &f->f_sub_initial, &ntmp, memctx );

			fstr->bv_len += ntmp.bv_len;
			fstr->bv_val = dc->op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1, memctx );

			snprintf( &fstr->bv_val[len - 2], ntmp.bv_len + 3,
				/* "(attr=" */ "%s*)",
				ntmp.bv_len ? ntmp.bv_val : "" );

			ber_memfree_x( ntmp.bv_val, memctx );
		}

		if ( f->f_sub_any != NULL ) {
			for ( i = 0; !BER_BVISNULL( &f->f_sub_any[i] ); i++ ) {
				len = fstr->bv_len;
				filter_escape_value_x( &f->f_sub_any[i], &ntmp, memctx );

				fstr->bv_len += ntmp.bv_len + 1;
				fstr->bv_val = dc->op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1, memctx );

				snprintf( &fstr->bv_val[len - 1], ntmp.bv_len + 3,
					/* "(attr=[init]*[any*]" */ "%s*)",
					ntmp.bv_len ? ntmp.bv_val : "" );
				ber_memfree_x( ntmp.bv_val, memctx );
			}
		}

		if ( !BER_BVISNULL( &f->f_sub_final ) ) {
			len = fstr->bv_len;

			filter_escape_value_x( &f->f_sub_final, &ntmp, memctx );

			fstr->bv_len += ntmp.bv_len;
			fstr->bv_val = dc->op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1, memctx );

			snprintf( &fstr->bv_val[len - 1], ntmp.bv_len + 3,
				/* "(attr=[init*][any*]" */ "%s)",
				ntmp.bv_len ? ntmp.bv_val : "" );

			ber_memfree_x( ntmp.bv_val, memctx );
		}

		break;

	case LDAP_FILTER_PRESENT:
		fstr->bv_len = f->f_desc->ad_cname.bv_len + ( STRLENOF( "(=*)" ) );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 1, memctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s=*)",
			f->f_desc->ad_cname.bv_val );
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT:
		fstr->bv_len = STRLENOF( "(%)" );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 128, memctx );	/* FIXME: why 128? */

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%c)",
			f->f_choice == LDAP_FILTER_AND ? '&' :
			f->f_choice == LDAP_FILTER_OR ? '|' : '!' );

		for ( p = f->f_list; p != NULL; p = p->f_next ) {
			int	rc;

			len = fstr->bv_len;

			rc = asyncmeta_int_filter2bv( dc, p, &vtmp );
			if ( rc != LDAP_SUCCESS ) {
				return rc;
			}

			fstr->bv_len += vtmp.bv_len;
			fstr->bv_val = dc->op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1, memctx );

			snprintf( &fstr->bv_val[len-1], vtmp.bv_len + 2,
				/*"("*/ "%s)", vtmp.bv_len ? vtmp.bv_val : "" );

			ber_memfree_x( vtmp.bv_val, memctx );
		}

		break;

	case LDAP_FILTER_EXT:
		if ( f->f_mr_desc ) {
			atmp = f->f_mr_desc->ad_cname;

		} else {
			BER_BVSTR( &atmp, "" );
		}
		filter_escape_value_x( &f->f_mr_value, &ntmp, memctx );

		/* FIXME: cleanup (less ?: operators...) */
		fstr->bv_len = atmp.bv_len +
			( f->f_mr_dnattrs ? STRLENOF( ":dn" ) : 0 ) +
			( !BER_BVISEMPTY( &f->f_mr_rule_text ) ? f->f_mr_rule_text.bv_len + 1 : 0 ) +
			ntmp.bv_len + ( STRLENOF( "(:=)" ) );
		fstr->bv_val = dc->op->o_tmpalloc( fstr->bv_len + 1, memctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s%s%s%s:=%s)",
			atmp.bv_val,
			f->f_mr_dnattrs ? ":dn" : "",
			!BER_BVISEMPTY( &f->f_mr_rule_text ) ? ":" : "",
			!BER_BVISEMPTY( &f->f_mr_rule_text ) ? f->f_mr_rule_text.bv_val : "",
			ntmp.bv_len ? ntmp.bv_val : "" );
		ber_memfree_x( ntmp.bv_val, memctx );
		break;

	case SLAPD_FILTER_COMPUTED:
		switch ( f->f_result ) {
		/* FIXME: treat UNDEFINED as FALSE */
		case SLAPD_COMPARE_UNDEFINED:
			if ( META_BACK_TGT_NOUNDEFFILTER( dc->target ) ) {
				return LDAP_COMPARE_FALSE;
			}
			/* fallthru */

		case LDAP_COMPARE_FALSE:
			if ( META_BACK_TGT_T_F( dc->target ) ) {
				tmp = &ber_bvtf_false;
				break;
			}
			tmp = &ber_bvfalse;
			break;

		case LDAP_COMPARE_TRUE:
			if ( META_BACK_TGT_T_F( dc->target ) ) {
				tmp = &ber_bvtf_true;
				break;
			}

			tmp = &ber_bvtrue;
			break;

		default:
			tmp = &ber_bverror;
			break;
		}

		ber_dupbv_x( fstr, tmp, memctx );
		break;

	default:
		ber_dupbv_x( fstr, &ber_bvunknown, memctx );
		break;
	}

	return 0;
}
meta_search_candidate_t
asyncmeta_back_search_start(
				Operation *op,
				SlapReply *rs,
				a_metaconn_t *mc,
				bm_context_t *bc,
				int candidate,
				struct berval		*prcookie,
				ber_int_t		prsize,
				int do_lock)
{
	SlapReply		*candidates = bc->candidates;
	a_metainfo_t		*mi = ( a_metainfo_t * )mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	a_dncookie		dc;
	struct berval		realbase = op->o_req_dn;
	char		**attrs;
	int			realscope = op->ors_scope;
	struct berval		mbase = BER_BVNULL;
	int			rc;
	struct berval	filterbv = BER_BVNULL;
	meta_search_candidate_t	retcode;
	int timelimit;
	LDAPControl		**ctrls = NULL;
	BerElement *ber = NULL;
	ber_int_t	msgid;
	ber_socket_t s = -1;
#ifdef SLAPD_META_CLIENT_PR
	LDAPControl		**save_ctrls = NULL;
#endif /* SLAPD_META_CLIENT_PR */

	/* this should not happen; just in case... */
	if ( msc->msc_ld == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"%s: asyncmeta_back_search_start candidate=%d ld=NULL%s.\n",
			op->o_log_prefix, candidate,
			META_BACK_ONERR_STOP( mi ) ? "" : " (ignored)" );
		candidates[ candidate ].sr_err = LDAP_OTHER;
		if ( META_BACK_ONERR_STOP( mi ) ) {
			return META_SEARCH_ERR;
		}
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		return META_SEARCH_NOT_CANDIDATE;
	}

	Debug( LDAP_DEBUG_TRACE, "%s >>> asyncmeta_back_search_start: dn=%s filter=%s\n",
	       op->o_log_prefix, op->o_req_dn.bv_val, op->ors_filterstr.bv_val );
	/*
	 * modifies the base according to the scope, if required
	 */
	if ( mt->mt_nsuffix.bv_len > op->o_req_ndn.bv_len ) {
		switch ( op->ors_scope ) {
		case LDAP_SCOPE_SUBTREE:
			/*
			 * make the target suffix the new base
			 * FIXME: this is very forgiving, because
			 * "illegal" searchBases may be turned
			 * into the suffix of the target; however,
			 * the requested searchBase already passed
			 * thru the candidate analyzer...
			 */
			if ( dnIsSuffix( &mt->mt_nsuffix, &op->o_req_ndn ) ) {
				realbase = mt->mt_nsuffix;
				if ( mt->mt_scope == LDAP_SCOPE_SUBORDINATE ) {
					realscope = LDAP_SCOPE_SUBORDINATE;
				}

			} else {
				/*
				 * this target is no longer candidate
				 */
				retcode = META_SEARCH_NOT_CANDIDATE;
				goto doreturn;
			}
			break;

		case LDAP_SCOPE_SUBORDINATE:
		case LDAP_SCOPE_ONELEVEL:
		{
			struct berval	rdn = mt->mt_nsuffix;
			rdn.bv_len -= op->o_req_ndn.bv_len + STRLENOF( "," );
			if ( dnIsOneLevelRDN( &rdn )
					&& dnIsSuffix( &mt->mt_nsuffix, &op->o_req_ndn ) )
			{
				/*
				 * if there is exactly one level,
				 * make the target suffix the new
				 * base, and make scope "base"
				 */
				realbase = mt->mt_nsuffix;
				if ( op->ors_scope == LDAP_SCOPE_SUBORDINATE ) {
					if ( mt->mt_scope == LDAP_SCOPE_SUBORDINATE ) {
						realscope = LDAP_SCOPE_SUBORDINATE;
					} else {
						realscope = LDAP_SCOPE_SUBTREE;
					}
				} else {
					realscope = LDAP_SCOPE_BASE;
				}
				break;
			} /* else continue with the next case */
		}

		case LDAP_SCOPE_BASE:
			/*
			 * this target is no longer candidate
			 */
			retcode = META_SEARCH_NOT_CANDIDATE;
			goto doreturn;
		}
	}

	/* check filter expression */
	if ( mt->mt_filter ) {
		metafilter_t *mf;
		for ( mf = mt->mt_filter; mf; mf = mf->mf_next ) {
			if ( regexec( &mf->mf_regex, op->ors_filterstr.bv_val, 0, NULL, 0 ) == 0 )
				break;
		}
		/* nothing matched, this target is no longer a candidate */
		if ( !mf ) {
			retcode = META_SEARCH_NOT_CANDIDATE;
			goto doreturn;
		}
	}

	/*
	 * Rewrite the search base, if required
	 */
	dc.op = op;
	dc.target = mt;
	dc.memctx = op->o_tmpmemctx;
	dc.to_from = MASSAGE_REQ;
	asyncmeta_dn_massage( &dc, &realbase, &mbase );

	attrs = anlist2charray_x( op->ors_attrs, 0, op->o_tmpmemctx );

	if ( op->ors_tlimit != SLAP_NO_LIMIT ) {
		timelimit = op->ors_tlimit > 0 ? op->ors_tlimit : 1;
	} else {
		timelimit = -1;	/* no limit */
	}

#ifdef SLAPD_META_CLIENT_PR
	save_ctrls = op->o_ctrls;
	{
		LDAPControl *pr_c = NULL;
		int i = 0, nc = 0;

		if ( save_ctrls ) {
			for ( ; save_ctrls[i] != NULL; i++ );
			nc = i;
			pr_c = ldap_control_find( LDAP_CONTROL_PAGEDRESULTS, save_ctrls, NULL );
		}

		if ( pr_c != NULL ) nc--;
		if ( mt->mt_ps > 0 || prcookie != NULL ) nc++;

		if ( mt->mt_ps > 0 || prcookie != NULL || pr_c != NULL ) {
			int src = 0, dst = 0;
			BerElementBuffer berbuf;
			BerElement *ber = (BerElement *)&berbuf;
			struct berval val = BER_BVNULL;
			ber_len_t len;

			len = sizeof( LDAPControl * )*( nc + 1 ) + sizeof( LDAPControl );

			if ( mt->mt_ps > 0 || prcookie != NULL ) {
				struct berval nullcookie = BER_BVNULL;
				ber_tag_t tag;

				if ( prsize == 0 && mt->mt_ps > 0 ) prsize = mt->mt_ps;
				if ( prcookie == NULL ) prcookie = &nullcookie;

				ber_init2( ber, NULL, LBER_USE_DER );
				tag = ber_printf( ber, "{iO}", prsize, prcookie );
				if ( tag == LBER_ERROR ) {
					/* error */
					(void) ber_free_buf( ber );
					goto done_pr;
				}

				tag = ber_flatten2( ber, &val, 0 );
				if ( tag == LBER_ERROR ) {
					/* error */
					(void) ber_free_buf( ber );
					goto done_pr;
				}

				len += val.bv_len + 1;
			}

			op->o_ctrls = op->o_tmpalloc( len, op->o_tmpmemctx );
			if ( save_ctrls ) {
				for ( ; save_ctrls[ src ] != NULL; src++ ) {
					if ( save_ctrls[ src ] != pr_c ) {
						op->o_ctrls[ dst ] = save_ctrls[ src ];
						dst++;
					}
				}
			}

			if ( mt->mt_ps > 0 || prcookie != NULL ) {
				op->o_ctrls[ dst ] = (LDAPControl *)&op->o_ctrls[ nc + 1 ];

				op->o_ctrls[ dst ]->ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
				op->o_ctrls[ dst ]->ldctl_iscritical = 1;

				op->o_ctrls[ dst ]->ldctl_value.bv_val = (char *)&op->o_ctrls[ dst ][ 1 ];
				AC_MEMCPY( op->o_ctrls[ dst ]->ldctl_value.bv_val, val.bv_val, val.bv_len + 1 );
				op->o_ctrls[ dst ]->ldctl_value.bv_len = val.bv_len;
				dst++;

				(void)ber_free_buf( ber );
			}

			op->o_ctrls[ dst ] = NULL;
		}
done_pr:;
	}
#endif /* SLAPD_META_CLIENT_PR */

	asyncmeta_set_msc_time(msc);
	ctrls = op->o_ctrls;

	if ( asyncmeta_controls_add( op, rs, mc, candidate, bc->is_root, &ctrls )
		!= LDAP_SUCCESS )
	{
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		retcode = META_SEARCH_NOT_CANDIDATE;
		goto done;
	}

	/*
	 * Starts the search
	 */
		/* someone reset the connection */
	if (!( LDAP_BACK_CONN_ISBOUND( msc )
	       || LDAP_BACK_CONN_ISANON( msc )) || msc->msc_ld == NULL ) {
		Debug( asyncmeta_debug, "msc %p not initialized at %s:%d\n", msc, __FILE__, __LINE__ );
		goto error_unavailable;
	}
	rc = asyncmeta_int_filter2bv( &dc, op->ors_filter, &filterbv );
	if ( rc ) {
		retcode = META_SEARCH_ERR;
		goto done;
	}

	ber = ldap_build_search_req( msc->msc_ld,
			mbase.bv_val, realscope, filterbv.bv_val,
			attrs, op->ors_attrsonly,
			ctrls, NULL, timelimit, op->ors_slimit, op->ors_deref,
			&msgid );
	if (!ber) {
		Debug( asyncmeta_debug, "%s asyncmeta_back_search_start: Operation encoding failed with errno %d\n",
		       op->o_log_prefix, msc->msc_ld->ld_errno );
		rs->sr_err = LDAP_OPERATIONS_ERROR;
		rs->sr_text = "Failed to encode proxied request";
		retcode = META_SEARCH_ERR;
		goto done;
	}

	if (ber) {
		struct timeval tv = {0, mt->mt_network_timeout*1000};

		if (!( LDAP_BACK_CONN_ISBOUND( msc )
		       || LDAP_BACK_CONN_ISANON( msc )) || msc->msc_ld == NULL ) {
			Debug( asyncmeta_debug, "msc %p not initialized at %s:%d\n", msc, __FILE__, __LINE__ );
			goto error_unavailable;
		}

		ldap_get_option( msc->msc_ld, LDAP_OPT_DESC, &s );
		if (s < 0) {
			Debug( asyncmeta_debug, "msc %p not initialized at %s:%d\n", msc, __FILE__, __LINE__ );
			goto error_unavailable;
		}

		rc = ldap_int_poll( msc->msc_ld, s, &tv, 1);
		if (rc < 0) {
			Debug( asyncmeta_debug, "msc %p not writable within network timeout %s:%d\n", msc, __FILE__, __LINE__ );
			if ((msc->msc_result_time + META_BACK_RESULT_INTERVAL) < slap_get_time()) {
				rc = LDAP_SERVER_DOWN;
			} else {
				goto error_unavailable;
			}
		} else {
			candidates[ candidate ].sr_msgid = msgid;
			rc = ldap_send_initial_request( msc->msc_ld, LDAP_REQ_SEARCH,
							mbase.bv_val, ber, msgid );
			if (rc == msgid)
				rc = LDAP_SUCCESS;
			else
				rc = LDAP_SERVER_DOWN;
			ber = NULL;
		}

		switch ( rc ) {
		case LDAP_SUCCESS:
			retcode = META_SEARCH_CANDIDATE;
			asyncmeta_set_msc_time(msc);
			goto done;

		case LDAP_SERVER_DOWN:
			/* do not lock if called from asyncmeta_handle_bind_result. Also do not reset the connection */
			if (do_lock > 0) {
				ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex);
				asyncmeta_reset_msc(NULL, mc, candidate, 0, __FUNCTION__);
				ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);
			}
			Debug( asyncmeta_debug, "msc %p ldap_send_initial_request failed. %s:%d\n", msc, __FILE__, __LINE__ );
			goto error_unavailable;

		default:
			candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
			retcode = META_SEARCH_NOT_CANDIDATE;
			goto done;
		}
	}

error_unavailable:
	if (ber)
		ber_free(ber, 1);
	switch (bc->nretries[candidate]) {
	case -1: /* nretries = forever */
		retcode = META_SEARCH_NEED_BIND;
		ldap_pvt_thread_yield();
		break;
	case 0: /* no retries left */
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		rs->sr_err = LDAP_UNAVAILABLE;
		rs->sr_text = "Unable to send search request to target";
		retcode = META_SEARCH_ERR;
		break;
	default: /* more retries left - try to rebind and go again */
		retcode = META_SEARCH_NEED_BIND;
		bc->nretries[candidate]--;
		ldap_pvt_thread_yield();
		break;
	}
done:;
#if 0
	(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );
#endif
#ifdef SLAPD_META_CLIENT_PR
	if ( save_ctrls != op->o_ctrls ) {
		op->o_tmpfree( op->o_ctrls, op->o_tmpmemctx );
		op->o_ctrls = save_ctrls;
	}
#endif /* SLAPD_META_CLIENT_PR */

	if ( mbase.bv_val != realbase.bv_val ) {
		op->o_tmpfree( mbase.bv_val, op->o_tmpmemctx );
	}

doreturn:;
	Debug( LDAP_DEBUG_TRACE, "%s <<< asyncmeta_back_search_start[%p] (fd %d)=%d\n", op->o_log_prefix, msc, s, candidates[candidate].sr_msgid );
	return retcode;
}

int
asyncmeta_back_search( Operation *op, SlapReply *rs )
{
	a_metainfo_t	*mi = ( a_metainfo_t * )op->o_bd->be_private;
	time_t          timeout = 0;
	int		rc = 0;
	int		ncandidates = 0, initial_candidates = 0;
	long		i;
	SlapReply	*candidates = NULL;
	void *thrctx = op->o_threadctx;
	bm_context_t *bc;
	a_metaconn_t *mc;
	int msc_decr = 0;
	int max_pending_ops = (mi->mi_max_pending_ops == 0) ? META_BACK_CFG_MAX_PENDING_OPS : mi->mi_max_pending_ops;
	int check_bind = 0;

	rs_assert_ready( rs );
	rs->sr_flags &= ~REP_ENTRY_MASK; /* paranoia, we can set rs = non-entry */

	/*
	 * controls are set in ldap_back_dobind()
	 *
	 * FIXME: in case of values return filter, we might want
	 * to map attrs and maybe rewrite value
	 */

	asyncmeta_new_bm_context(op, rs, &bc, mi->mi_ntargets, mi );
	if (bc == NULL) {
		rs->sr_err = LDAP_OTHER;
		send_ldap_result(op, rs);
		return rs->sr_err;
	}

	candidates = bc->candidates;
	mc = asyncmeta_getconn( op, rs, candidates, NULL, LDAP_BACK_DONTSEND, 0);
	if ( !mc || rs->sr_err != LDAP_SUCCESS) {
		send_ldap_result(op, rs);
		return rs->sr_err;
	}

	/*
	 * Inits searches
	 */

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		/* reset sr_msgid; it is used in most loops
		 * to check if that target is still to be considered */
		candidates[i].sr_msgid = META_MSGID_UNDEFINED;
		/* a target is marked as candidate by asyncmeta_getconn();
		 * if for any reason (an error, it's over or so) it is
		 * no longer active, sr_msgid is set to META_MSGID_IGNORE
		 * but it remains candidate, which means it has been active
		 * at some point during the operation.  This allows to
		 * use its response code and more to compute the final
		 * response */
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}

		candidates[ i ].sr_matched = NULL;
		candidates[ i ].sr_text = NULL;
		candidates[ i ].sr_ref = NULL;
		candidates[ i ].sr_ctrls = NULL;
		candidates[ i ].sr_nentries = 0;
		candidates[ i ].sr_type = -1;

		/* get largest timeout among candidates */
		if ( mi->mi_targets[ i ]->mt_timeout[ SLAP_OP_SEARCH ]
			&& mi->mi_targets[ i ]->mt_timeout[ SLAP_OP_SEARCH ] > timeout )
		{
			timeout = mi->mi_targets[ i ]->mt_timeout[ SLAP_OP_SEARCH ];
		}
	}

	if ( op->ors_tlimit != SLAP_NO_LIMIT && (timeout == 0 || op->ors_tlimit < timeout)) {
		bc->searchtime = 1;
		bc->timeout = op->ors_tlimit;
	} else {
		bc->timeout = timeout;
	}

	bc->stoptime = op->o_time + bc->timeout;
	bc->bc_active = 1;

	if (mc->pending_ops >= max_pending_ops) {
		rs->sr_err = LDAP_BUSY;
		rs->sr_text = "Maximum pending ops limit exceeded";
		send_ldap_result(op, rs);
		return rs->sr_err;
	}

	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex);
	rc = asyncmeta_add_message_queue(mc, bc);
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		mc->mc_conns[i].msc_active++;
	}
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);

	if (rc != LDAP_SUCCESS) {
		rs->sr_err = LDAP_BUSY;
		rs->sr_text = "Maximum pending ops limit exceeded";
		send_ldap_result(op, rs);
		goto finish;
	}

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		if ( !META_IS_CANDIDATE( &candidates[ i ] )
			|| candidates[ i ].sr_err != LDAP_SUCCESS )
		{
			continue;
		}
retry:
		if (bc->timeout && bc->stoptime < slap_get_time() && META_BACK_ONERR_STOP( mi )) {
			int		timeout_err;
			const char *timeout_text;
			if (bc->searchtime) {
				timeout_err = LDAP_TIMELIMIT_EXCEEDED;
				timeout_text = NULL;
			} else {
				timeout_err = op->o_protocol >= LDAP_VERSION3 ?
					LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
				timeout_text = "Operation timed out before it was sent to target";
			}
			rs->sr_err = timeout_err;
			rs->sr_text = timeout_text;
			asyncmeta_handle_onerr_stop(op,rs,mc,bc,i);
			goto finish;

		}

		if (op->o_abandon) {
			rs->sr_err = SLAPD_ABANDON;
			asyncmeta_handle_onerr_stop(op,rs,mc,bc,i);
			goto finish;
		}

		rc = asyncmeta_dobind_init_with_retry(op, rs, bc, mc, i);
		switch (rc)
		{
		case META_SEARCH_CANDIDATE:
			/* target is already bound, just send the search request */
			ncandidates++;
			Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_back_search: IS_CANDIDATE "
			       "cnd=\"%ld\"\n", op->o_log_prefix, i );

			rc = asyncmeta_back_search_start( op, rs, mc, bc, i,  NULL, 0 , 1);
			if (rc == META_SEARCH_ERR) {
				META_CANDIDATE_CLEAR(&candidates[i]);
				candidates[ i ].sr_msgid = META_MSGID_IGNORE;
				if ( META_BACK_ONERR_STOP( mi ) ) {
					asyncmeta_handle_onerr_stop(op,rs,mc,bc,i);
					goto finish;
				}
				else {
					continue;
				}
			} else if (rc == META_SEARCH_NEED_BIND) {
				goto retry;
			}
			break;
		case META_SEARCH_NOT_CANDIDATE:
			Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_back_search: NOT_CANDIDATE "
			       "cnd=\"%ld\"\n", op->o_log_prefix, i );
			candidates[ i ].sr_msgid = META_MSGID_IGNORE;
			break;

		case META_SEARCH_NEED_BIND:
		case META_SEARCH_BINDING:
			Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_back_search: BINDING "
			       "cnd=\"%ld\" mc %p msc %p\n", op->o_log_prefix, i , mc, &mc->mc_conns[i]);
			check_bind++;
			ncandidates++;
			/* Todo add the context to the message queue but do not send the request
			 the receiver must send this when we are done binding */
			/* question - how would do receiver know to which targets??? */
			break;

		case META_SEARCH_ERR:
			Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_back_search: SEARCH_ERR "
			       "cnd=\"%ldd\"\n", op->o_log_prefix, i );
			candidates[ i ].sr_msgid = META_MSGID_IGNORE;
			candidates[ i ].sr_type = REP_RESULT;

			if ( META_BACK_ONERR_STOP( mi ) ) {
				asyncmeta_handle_onerr_stop(op,rs,mc,bc,i);
				goto finish;
			}
			else {
				continue;
			}
			break;

		default:
			assert( 0 );
			break;
		}
	}

	initial_candidates = ncandidates;

	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		char	cnd[ SLAP_TEXT_BUFLEN ];
		int	c;

		for ( c = 0; c < mi->mi_ntargets; c++ ) {
			if ( META_IS_CANDIDATE( &candidates[ c ] ) ) {
				cnd[ c ] = '*';
			} else {
				cnd[ c ] = ' ';
			}
		}
		cnd[ c ] = '\0';

		Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_back_search: ncandidates=%d "
			"cnd=\"%s\"\n", op->o_log_prefix, ncandidates, cnd );
	}

	if ( initial_candidates == 0 ) {
		/* NOTE: here we are not sending any matchedDN;
		 * this is intended, because if the back-meta
		 * is serving this search request, but no valid
		 * candidate could be looked up, it means that
		 * there is a hole in the mapping of the targets
		 * and thus no knowledge of any remote superior
		 * is available */
		Debug( LDAP_DEBUG_ANY, "%s asyncmeta_back_search: "
			"base=\"%s\" scope=%d: "
			"no candidate could be selected\n",
			op->o_log_prefix, op->o_req_dn.bv_val,
			op->ors_scope );

		/* FIXME: we're sending the first error we encounter;
		 * maybe we should pick the worst... */
		rc = LDAP_NO_SUCH_OBJECT;
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			if ( META_IS_CANDIDATE( &candidates[ i ] )
				&& candidates[ i ].sr_err != LDAP_SUCCESS )
			{
				rc = candidates[ i ].sr_err;
				break;
			}
		}
		rs->sr_err = rc;
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex);
		asyncmeta_drop_bc(mc, bc);
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);
		send_ldap_result(op, rs);
		goto finish;
	}

	/* If we were processing many targets the result from a pending Bind
	 * on an earlier target may have arrived while we were sending to a
	 * later target. See if we can now send our pending request.
	 */
	if ( check_bind ) {
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			if ( candidates[ i ].sr_msgid == META_MSGID_GOT_BIND ) {
				rc = asyncmeta_back_search_start( op, rs, mc, bc, i, NULL, 0, 1 );
				if ( rc == META_SEARCH_ERR ) {
					META_CANDIDATE_CLEAR( &candidates[i] );
					candidates[ i ].sr_msgid = META_MSGID_IGNORE;
					if ( META_BACK_ONERR_STOP( mi ) ) {
						asyncmeta_handle_onerr_stop(op,rs,mc,bc,i);
						goto finish;
					}
				}
			}
		}
	}

	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex);
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		mc->mc_conns[i].msc_active--;
	}
	msc_decr = 1;

	asyncmeta_start_listeners(mc, candidates, bc);
	bc->bc_active--;
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);
	rs->sr_err = SLAPD_ASYNCOP;

finish:
	/* we ended up straight here due to error and need to reset the msc_active*/
	if (msc_decr == 0) {
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex);
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			mc->mc_conns[i].msc_active--;
		}
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex);
	}
	return rs->sr_err;
}
