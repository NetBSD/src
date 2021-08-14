/*	$NetBSD: bind.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* bind.c - bind request handler functions for binding
 * to remote targets for back-asyncmeta */
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
__RCSID("$NetBSD: bind.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include "slap.h"
#include "../../../libraries/libldap/ldap-int.h"

#define AVL_INTERNAL
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"
#include "lutil_ldap.h"

#define LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ	"2.16.840.1.113730.3.4.12"

static int
asyncmeta_proxy_authz_bind(
	a_metaconn_t		*mc,
	int			candidate,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	int			dolock );

static int
asyncmeta_single_bind(
	Operation		*op,
	SlapReply		*rs,
	a_metaconn_t		*mc,
	int			candidate );

int
asyncmeta_back_bind( Operation *op, SlapReply *rs )
{
	a_metainfo_t	*mi = ( a_metainfo_t * )op->o_bd->be_private;
	a_metaconn_t	*mc = NULL;

	int		rc = LDAP_OTHER,
			i,
			gotit = 0,
			isroot = 0;

	SlapReply	*candidates;

	candidates = op->o_tmpcalloc(mi->mi_ntargets, sizeof(SlapReply),op->o_tmpmemctx);
	rs->sr_err = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_ARGS, "%s asyncmeta_back_bind: dn=\"%s\".\n",
		op->o_log_prefix, op->o_req_dn.bv_val );

	/* the test on the bind method should be superfluous */
	switch ( be_rootdn_bind( op, rs ) ) {
	case LDAP_SUCCESS:
		if ( META_BACK_DEFER_ROOTDN_BIND( mi ) ) {
			/* frontend will return success */
			return rs->sr_err;
		}

		isroot = 1;
		/* fallthru */

	case SLAP_CB_CONTINUE:
		break;

	default:
		/* be_rootdn_bind() sent result */
		return rs->sr_err;
	}

	/* we need asyncmeta_getconn() not send result even on error,
	 * because we want to intercept the error and make it
	 * invalidCredentials */
	mc = asyncmeta_getconn( op, rs, candidates, NULL, LDAP_BACK_BIND_DONTSEND, 1 );
	if ( !mc ) {
		Debug(LDAP_DEBUG_ANY,
		      "%s asyncmeta_back_bind: no target " "for dn \"%s\" (%d%s%s).\n",
		      op->o_log_prefix, op->o_req_dn.bv_val,
		      rs->sr_err, rs->sr_text ? ". " : "",
		      rs->sr_text ? rs->sr_text : "" );

		/* FIXME: there might be cases where we don't want
		 * to map the error onto invalidCredentials */
		switch ( rs->sr_err ) {
		case LDAP_NO_SUCH_OBJECT:
		case LDAP_UNWILLING_TO_PERFORM:
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			rs->sr_text = NULL;
			break;
		}
		send_ldap_result( op, rs );
		return rs->sr_err;
	}

	/*
	 * Each target is scanned ...
	 */
	mc->mc_authz_target = META_BOUND_NONE;
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		a_metatarget_t	*mt = mi->mi_targets[ i ];
		int		lerr;

		/*
		 * Skip non-candidates
		 */
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}

		if ( gotit == 0 ) {
			/* set rc to LDAP_SUCCESS only if at least
			 * one candidate has been tried */
			rc = LDAP_SUCCESS;
			gotit = 1;

		} else if ( !isroot ) {
			/*
			 * A bind operation is expected to have
			 * ONE CANDIDATE ONLY!
			 */
			Debug( LDAP_DEBUG_ANY,
				"### %s asyncmeta_back_bind: more than one"
				" candidate selected...\n",
				op->o_log_prefix );
		}

		if ( isroot ) {
			if ( mt->mt_idassert_authmethod == LDAP_AUTH_NONE
				|| BER_BVISNULL( &mt->mt_idassert_authcDN ) )
			{
				a_metasingleconn_t	*msc = &mc->mc_conns[ i ];

				if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
					ch_free( msc->msc_bound_ndn.bv_val );
					BER_BVZERO( &msc->msc_bound_ndn );
				}

				if ( !BER_BVISNULL( &msc->msc_cred ) ) {
					/* destroy sensitive data */
					memset( msc->msc_cred.bv_val, 0,
						msc->msc_cred.bv_len );
					ch_free( msc->msc_cred.bv_val );
					BER_BVZERO( &msc->msc_cred );
				}

				continue;
			}


			(void)asyncmeta_proxy_authz_bind( mc, i, op, rs, LDAP_BACK_DONTSEND, 1 );
			lerr = rs->sr_err;

		} else {
			lerr = asyncmeta_single_bind( op, rs, mc, i );
		}

		if ( lerr != LDAP_SUCCESS ) {
			rc = rs->sr_err = lerr;

			/* FIXME: in some cases (e.g. unavailable)
			 * do not assume it's not candidate; rather
			 * mark this as an error to be eventually
			 * reported to client */
			META_CANDIDATE_CLEAR( &candidates[ i ] );
			break;
		}
	}

	if ( mc != NULL ) {
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			a_metasingleconn_t	*msc = &mc->mc_conns[ i ];
			if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
				ch_free( msc->msc_bound_ndn.bv_val );
			}

			if ( !BER_BVISNULL( &msc->msc_cred ) ) {
				/* destroy sensitive data */
				memset( msc->msc_cred.bv_val, 0,
					msc->msc_cred.bv_len );
				ch_free( msc->msc_cred.bv_val );
			}
		}
		asyncmeta_back_conn_free( mc );
	}

	/*
	 * rc is LDAP_SUCCESS if at least one bind succeeded,
	 * err is the last error that occurred during a bind;
	 * if at least (and at most?) one bind succeeds, fine.
	 */
	if ( rc != LDAP_SUCCESS ) {

		/*
		 * deal with bind failure ...
		 */

		/*
		 * no target was found within the naming context,
		 * so bind must fail with invalid credentials
		 */
		if ( rs->sr_err == LDAP_SUCCESS && gotit == 0 ) {
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
		} else {
			rs->sr_err = slap_map_api2result( rs );
		}
		send_ldap_result( op, rs );
		return rs->sr_err;

	}
	return LDAP_SUCCESS;
}

static int
asyncmeta_bind_op_result(
	Operation		*op,
	SlapReply		*rs,
	a_metaconn_t		*mc,
	int			candidate,
	int			msgid,
	ldap_back_send_t	sendok,
	int			dolock )
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	LDAPMessage		*res;
	struct timeval		tv;
	int			rc;
	int			nretries = mt->mt_nretries;

	Debug( LDAP_DEBUG_TRACE,
		">>> %s asyncmeta_bind_op_result[%d]\n",
		op->o_log_prefix, candidate );

	/* make sure this is clean */
	assert( rs->sr_ctrls == NULL );

	if ( rs->sr_err == LDAP_SUCCESS ) {
		time_t		stoptime = (time_t)(-1),
				timeout;
		int		timeout_err = op->o_protocol >= LDAP_VERSION3 ?
				LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
		const char	*timeout_text = "Operation timed out";
		slap_op_t	opidx = slap_req2op( op->o_tag );

		/* since timeout is not specified, compute and use
		 * the one specific to the ongoing operation */
		if ( opidx == LDAP_REQ_SEARCH ) {
			if ( op->ors_tlimit <= 0 ) {
				timeout = 0;

			} else {
				timeout = op->ors_tlimit;
				timeout_err = LDAP_TIMELIMIT_EXCEEDED;
				timeout_text = NULL;
			}

		} else {
			timeout = mt->mt_timeout[ opidx ];
		}

		/* better than nothing :) */
		if ( timeout == 0 ) {
			if ( mi->mi_idle_timeout ) {
				timeout = mi->mi_idle_timeout;

			}
		}

		if ( timeout ) {
			stoptime = op->o_time + timeout;
		}

		LDAP_BACK_TV_SET( &tv );

		/*
		 * handle response!!!
		 */
retry:;
		rc = ldap_result( msc->msc_ld, msgid, LDAP_MSG_ALL, &tv, &res );
		switch ( rc ) {
		case 0:
			if ( nretries != META_RETRY_NEVER
				|| ( timeout && slap_get_time() <= stoptime ) )
			{
				ldap_pvt_thread_yield();
				if ( nretries > 0 ) {
					nretries--;
				}
				tv = mt->mt_bind_timeout;
				goto retry;
			}

			/* don't let anyone else use this handler,
			 * because there's a pending bind that will not
			 * be acknowledged */
			assert( LDAP_BACK_CONN_BINDING( msc ) );

#ifdef DEBUG_205
			Debug( LDAP_DEBUG_ANY, "### %s asyncmeta_bind_op_result ldap_unbind_ext[%d] ld=%p\n",
				op->o_log_prefix, candidate, (void *)msc->msc_ld );
#endif /* DEBUG_205 */

			rs->sr_err = timeout_err;
			rs->sr_text = timeout_text;
			break;

		case -1:
			ldap_get_option( msc->msc_ld, LDAP_OPT_ERROR_NUMBER,
				&rs->sr_err );

			Debug( LDAP_DEBUG_ANY,
			      "### %s asyncmeta_bind_op_result[%d]: err=%d (%s) nretries=%d.\n",
			      op->o_log_prefix, candidate, rs->sr_err,
			      ldap_err2string(rs->sr_err), nretries );
			break;

		default:
			/* only touch when activity actually took place... */
			if ( mi->mi_idle_timeout != 0 && msc->msc_time < op->o_time ) {
				msc->msc_time = op->o_time;
			}

			/* FIXME: matched? referrals? response controls? */
			rc = ldap_parse_result( msc->msc_ld, res, &rs->sr_err,
					NULL, NULL, NULL, NULL, 1 );
			if ( rc != LDAP_SUCCESS ) {
				rs->sr_err = rc;
			}
			rs->sr_err = slap_map_api2result( rs );
			break;
		}
	}

	rs->sr_err = slap_map_api2result( rs );
	Debug( LDAP_DEBUG_TRACE,
		"<<< %s asyncmeta_bind_op_result[%d] err=%d\n",
		op->o_log_prefix, candidate, rs->sr_err );

	return rs->sr_err;
}

/*
 * asyncmeta_single_bind
 *
 * attempts to perform a bind with creds
 */
static int
asyncmeta_single_bind(
	Operation		*op,
	SlapReply		*rs,
	a_metaconn_t		*mc,
	int			candidate )
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	struct berval		mdn = BER_BVNULL;
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	int			msgid;
	a_dncookie		dc;
	struct berval		save_o_dn;
	int			save_o_do_not_cache;
	LDAPControl		**ctrls = NULL;

	if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
		ch_free( msc->msc_bound_ndn.bv_val );
		BER_BVZERO( &msc->msc_bound_ndn );
	}

	if ( !BER_BVISNULL( &msc->msc_cred ) ) {
		/* destroy sensitive data */
		memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
		ch_free( msc->msc_cred.bv_val );
		BER_BVZERO( &msc->msc_cred );
	}

	/*
	 * Rewrite the bind dn if needed
	 */
	dc.op = op;
	dc.target = mt;
	dc.memctx = op->o_tmpmemctx;
	dc.to_from = MASSAGE_REQ;

	asyncmeta_dn_massage( &dc, &op->o_req_dn, &mdn );

	/* don't add proxyAuthz; set the bindDN */
	save_o_dn = op->o_dn;
	save_o_do_not_cache = op->o_do_not_cache;
	op->o_do_not_cache = 1;
	op->o_dn = op->o_req_dn;

	ctrls = op->o_ctrls;
	rs->sr_err = asyncmeta_controls_add( op, rs, mc, candidate, be_isroot(op), &ctrls );
	op->o_dn = save_o_dn;
	op->o_do_not_cache = save_o_do_not_cache;
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto return_results;
	}

	/* FIXME: this fixes the bind problem right now; we need
	 * to use the asynchronous version to get the "matched"
	 * and more in case of failure ... */
	/* FIXME: should we check if at least some of the op->o_ctrls
	 * can/should be passed? */
	for (;;) {
		rs->sr_err = ldap_sasl_bind( msc->msc_ld, mdn.bv_val,
			LDAP_SASL_SIMPLE, &op->orb_cred,
			ctrls, NULL, &msgid );
		if ( rs->sr_err != LDAP_X_CONNECTING ) {
			break;
		}
		ldap_pvt_thread_yield();
	}

	mi->mi_ldap_extra->controls_free( op, rs, &ctrls );

	asyncmeta_bind_op_result( op, rs, mc, candidate, msgid, LDAP_BACK_DONTSEND, 1 );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto return_results;
	}

	/* If defined, proxyAuthz will be used also when
	 * back-ldap is the authorizing backend; for this
	 * purpose, a successful bind is followed by a
	 * bind with the configured identity assertion */
	/* NOTE: use with care */
	if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) {
		asyncmeta_proxy_authz_bind( mc, candidate, op, rs, LDAP_BACK_SENDERR, 1 );
		if ( !LDAP_BACK_CONN_ISBOUND( msc ) ) {
			goto return_results;
		}
		goto cache_refresh;
	}

	ber_bvreplace( &msc->msc_bound_ndn, &op->o_req_ndn );
	LDAP_BACK_CONN_ISBOUND_SET( msc );
	mc->mc_authz_target = candidate;

	if ( META_BACK_TGT_SAVECRED( mt ) ) {
		if ( !BER_BVISNULL( &msc->msc_cred ) ) {
			memset( msc->msc_cred.bv_val, 0,
				msc->msc_cred.bv_len );
		}
		ber_bvreplace( &msc->msc_cred, &op->orb_cred );
		ldap_set_rebind_proc( msc->msc_ld, mt->mt_rebind_f, msc );
	}

cache_refresh:;
	if ( mi->mi_cache.ttl != META_DNCACHE_DISABLED
			&& !BER_BVISEMPTY( &op->o_req_ndn ) )
	{
		( void )asyncmeta_dncache_update_entry( &mi->mi_cache,
				&op->o_req_ndn, candidate );
	}

return_results:;
	if ( mdn.bv_val != op->o_req_dn.bv_val ) {
		op->o_tmpfree( mdn.bv_val, op->o_tmpmemctx );
	}

	if ( META_BACK_TGT_QUARANTINE( mt ) ) {
		asyncmeta_quarantine( op, mi, rs, candidate );
	}
	ldap_unbind_ext( msc->msc_ld, NULL, NULL );
	msc->msc_ld = NULL;
	ldap_ld_free( msc->msc_ldr, 0, NULL, NULL );
	msc->msc_ldr = NULL;
	return rs->sr_err;
}


/*
 * asyncmeta_back_default_rebind
 *
 * This is a callback used for chasing referrals using the same
 * credentials as the original user on this session.
 */
int
asyncmeta_back_default_rebind(
	LDAP			*ld,
	LDAP_CONST char		*url,
	ber_tag_t		request,
	ber_int_t		msgid,
	void			*params )
{
	a_metasingleconn_t	*msc = ( a_metasingleconn_t * )params;

	return ldap_sasl_bind_s( ld, msc->msc_bound_ndn.bv_val,
			LDAP_SASL_SIMPLE, &msc->msc_cred,
			NULL, NULL, NULL );
}

/*
 * meta_back_default_urllist
 *
 * This is a callback used for mucking with the urllist
 */
int
asyncmeta_back_default_urllist(
	LDAP		*ld,
	LDAPURLDesc	**urllist,
	LDAPURLDesc	**url,
	void		*params )
{
	a_metatarget_t	*mt = (a_metatarget_t *)params;
	LDAPURLDesc	**urltail;

	if ( urllist == url ) {
		return LDAP_SUCCESS;
	}

	for ( urltail = &(*url)->lud_next; *urltail; urltail = &(*urltail)->lud_next )
		/* count */ ;

	*urltail = *urllist;
	*urllist = *url;
	*url = NULL;

	ldap_pvt_thread_mutex_lock( &mt->mt_uri_mutex );
	if ( mt->mt_uri ) {
		ch_free( mt->mt_uri );
	}

	ldap_get_option( ld, LDAP_OPT_URI, (void *)&mt->mt_uri );
	ldap_pvt_thread_mutex_unlock( &mt->mt_uri_mutex );

	return LDAP_SUCCESS;
}

int
asyncmeta_back_cancel(
	a_metaconn_t		*mc,
	Operation		*op,
	ber_int_t		msgid,
	int			candidate )
{

	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	int			rc = LDAP_OTHER;
	struct timeval tv = { 0, 0 };
	ber_socket_t s;

	Debug( LDAP_DEBUG_TRACE, ">>> %s asyncmeta_back_cancel[%d] msgid=%d\n",
		op->o_log_prefix, candidate, msgid );

	if (!( LDAP_BACK_CONN_ISBOUND( msc )
	       || LDAP_BACK_CONN_ISANON( msc )) || msc->msc_ld == NULL ) {
		Debug( LDAP_DEBUG_TRACE, ">>> %s asyncmeta_back_cancel[%d] msgid=%d\n already reset",
		op->o_log_prefix, candidate, msgid );
		return LDAP_SUCCESS;
	}

	ldap_get_option( msc->msc_ld, LDAP_OPT_DESC, &s );
	if (s < 0) {
		return rc;
	}
	rc = ldap_int_poll( msc->msc_ld, s, &tv, 1);
	if (rc < 0) {
		rc = LDAP_SERVER_DOWN;
		return rc;
	}
	/* default behavior */
	if ( META_BACK_TGT_ABANDON( mt ) ) {
		rc = ldap_abandon_ext( msc->msc_ld, msgid, NULL, NULL );

	} else if ( META_BACK_TGT_IGNORE( mt ) ) {
		rc = ldap_pvt_discard( msc->msc_ld, msgid );

	} else if ( META_BACK_TGT_CANCEL( mt ) ) {
		rc = ldap_cancel_s( msc->msc_ld, msgid, NULL, NULL );

	} else {
		assert( 0 );
	}

	Debug( LDAP_DEBUG_TRACE, "<<< %s asyncmeta_back_cancel[%d] err=%d\n",
		op->o_log_prefix, candidate, rc );

	return rc;
}



/*
 * asyncmeta_back_proxy_authz_cred()
 *
 * prepares credentials & method for meta_back_proxy_authz_bind();
 * or, if method is SASL, performs the SASL bind directly.
 */
int
asyncmeta_back_proxy_authz_cred(
	a_metaconn_t		*mc,
	int			candidate,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	struct berval		*binddn,
	struct berval		*bindcred,
	int			*method )
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	struct berval		ndn;
	int			dobind = 0;
	struct timeval old_tv = {0, 0};
	struct timeval bind_tv = { mt->mt_timeout[ SLAP_OP_BIND ], 0};
	/* don't proxyAuthz if protocol is not LDAPv3 */
	switch ( mt->mt_version ) {
	case LDAP_VERSION3:
		break;

	case 0:
		if ( op->o_protocol == 0 || op->o_protocol == LDAP_VERSION3 ) {
			break;
		}
		/* fall thru */

	default:
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
		LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
		goto done;
	}

	if ( op->o_tag == LDAP_REQ_BIND ) {
		ndn = op->o_req_ndn;

	} else if ( !BER_BVISNULL( &op->o_conn->c_ndn ) ) {
		ndn = op->o_conn->c_ndn;

	} else {
		ndn = op->o_ndn;
	}
	rs->sr_err = LDAP_SUCCESS;

	/*
	 * FIXME: we need to let clients use proxyAuthz
	 * otherwise we cannot do symmetric pools of servers;
	 * we have to live with the fact that a user can
	 * authorize itself as any ID that is allowed
	 * by the authzTo directive of the "proxyauthzdn".
	 */
	/*
	 * NOTE: current Proxy Authorization specification
	 * and implementation do not allow proxy authorization
	 * control to be provided with Bind requests
	 */
	/*
	 * if no bind took place yet, but the connection is bound
	 * and the "proxyauthzdn" is set, then bind as
	 * "proxyauthzdn" and explicitly add the proxyAuthz
	 * control to every operation with the dn bound
	 * to the connection as control value.
	 */

	/* bind as proxyauthzdn only if no idassert mode
	 * is requested, or if the client's identity
	 * is authorized */
	switch ( mt->mt_idassert_mode ) {
	case LDAP_BACK_IDASSERT_LEGACY:
		if ( !BER_BVISNULL( &ndn ) && !BER_BVISEMPTY( &ndn ) ) {
			if ( !BER_BVISNULL( &mt->mt_idassert_authcDN ) && !BER_BVISEMPTY( &mt->mt_idassert_authcDN ) )
			{
				*binddn = mt->mt_idassert_authcDN;
				*bindcred = mt->mt_idassert_passwd;
				dobind = 1;
			}
		}
		break;

	default:
		/* NOTE: rootdn can always idassert */
		if ( BER_BVISNULL( &ndn )
			&& mt->mt_idassert_authz == NULL
			&& !( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) )
		{
			if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
				rs->sr_err = LDAP_INAPPROPRIATE_AUTH;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
				goto done;

			}

			rs->sr_err = LDAP_SUCCESS;
			*binddn = slap_empty_bv;
			*bindcred = slap_empty_bv;
			break;

		} else if ( mt->mt_idassert_authz && !be_isroot( op ) ) {
			struct berval authcDN;

			if ( BER_BVISNULL( &ndn ) ) {
				authcDN = slap_empty_bv;

			} else {
				authcDN = ndn;
			}
			rs->sr_err = slap_sasl_matches( op, mt->mt_idassert_authz,
					&authcDN, &authcDN );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
					if ( sendok & LDAP_BACK_SENDERR ) {
						send_ldap_result( op, rs );
					}
					LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
					goto done;
				}

				rs->sr_err = LDAP_SUCCESS;
				*binddn = slap_empty_bv;
				*bindcred = slap_empty_bv;
				break;
			}
		}

		*binddn = mt->mt_idassert_authcDN;
		*bindcred = mt->mt_idassert_passwd;
		dobind = 1;
		break;
	}

	if ( dobind && mt->mt_idassert_authmethod == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void		*defaults = NULL;
		struct berval	authzID = BER_BVNULL;
		int		freeauthz = 0;

		/* if SASL supports native authz, prepare for it */
		if ( ( !op->o_do_not_cache || !op->o_is_auth_check ) &&
				( mt->mt_idassert_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) )
		{
			switch ( mt->mt_idassert_mode ) {
			case LDAP_BACK_IDASSERT_OTHERID:
			case LDAP_BACK_IDASSERT_OTHERDN:
				authzID = mt->mt_idassert_authzID;
				break;

			case LDAP_BACK_IDASSERT_ANONYMOUS:
				BER_BVSTR( &authzID, "dn:" );
				break;

			case LDAP_BACK_IDASSERT_SELF:
				if ( BER_BVISNULL( &ndn ) ) {
					/* connection is not authc'd, so don't idassert */
					BER_BVSTR( &authzID, "dn:" );
					break;
				}
				authzID.bv_len = STRLENOF( "dn:" ) + ndn.bv_len;
				authzID.bv_val = slap_sl_malloc( authzID.bv_len + 1, op->o_tmpmemctx );
				AC_MEMCPY( authzID.bv_val, "dn:", STRLENOF( "dn:" ) );
				AC_MEMCPY( authzID.bv_val + STRLENOF( "dn:" ),
						ndn.bv_val, ndn.bv_len + 1 );
				freeauthz = 1;
				break;

			default:
				break;
			}
		}

		if ( mt->mt_idassert_secprops != NULL ) {
			rs->sr_err = ldap_set_option( msc->msc_ld,
				LDAP_OPT_X_SASL_SECPROPS,
				(void *)mt->mt_idassert_secprops );

			if ( rs->sr_err != LDAP_OPT_SUCCESS ) {
				rs->sr_err = LDAP_OTHER;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
				goto done;
			}
		}

		ldap_get_option( msc->msc_ld, LDAP_OPT_TIMEOUT, (void *)&old_tv);

		if (mt->mt_timeout[ SLAP_OP_BIND ] > 0 ) {
			rs->sr_err = ldap_set_option( msc->msc_ld,
				LDAP_OPT_TIMEOUT,
				(void *)&bind_tv );

			if ( rs->sr_err != LDAP_OPT_SUCCESS ) {
				rs->sr_err = LDAP_OTHER;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
				goto done;
			}
		}
		defaults = lutil_sasl_defaults( msc->msc_ld,
				mt->mt_idassert_sasl_mech.bv_val,
				mt->mt_idassert_sasl_realm.bv_val,
				mt->mt_idassert_authcID.bv_val,
				mt->mt_idassert_passwd.bv_val,
				authzID.bv_val );
		if ( defaults == NULL ) {
			rs->sr_err = LDAP_OTHER;
			LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			goto done;
		}

		rs->sr_err = ldap_sasl_interactive_bind_s( msc->msc_ld, binddn->bv_val,
				mt->mt_idassert_sasl_mech.bv_val, NULL, NULL,
				LDAP_SASL_QUIET, lutil_sasl_interact,
				defaults );

		/* restore the old timeout just in case */
		ldap_set_option( msc->msc_ld, LDAP_OPT_TIMEOUT, (void *)&old_tv );

		rs->sr_err = slap_map_api2result( rs );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			if ( LogTest( asyncmeta_debug ) ) {
				char	time_buf[ SLAP_TEXT_BUFLEN ];
				asyncmeta_get_timestamp(time_buf);
				Debug( asyncmeta_debug, "[%s] asyncmeta_back_proxy_authz_cred failed bind msc: %p\n",
				      time_buf, msc );
			}
			LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}

		} else {
			LDAP_BACK_CONN_ISBOUND_SET( msc );
		}

		lutil_sasl_freedefs( defaults );
		if ( freeauthz ) {
			slap_sl_free( authzID.bv_val, op->o_tmpmemctx );
		}

		goto done;
#endif /* HAVE_CYRUS_SASL */
	}

	*method = mt->mt_idassert_authmethod;
	switch ( mt->mt_idassert_authmethod ) {
	case LDAP_AUTH_NONE:
		BER_BVSTR( binddn, "" );
		BER_BVSTR( bindcred, "" );
		/* fallthru */

	case LDAP_AUTH_SIMPLE:
		break;

	default:
		/* unsupported! */
		LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
		rs->sr_err = LDAP_AUTH_METHOD_NOT_SUPPORTED;
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
		break;
	}

done:;

	if ( !BER_BVISEMPTY( binddn ) ) {
		LDAP_BACK_CONN_ISIDASSERT_SET( msc );
	}

	return rs->sr_err;
}

static int
asyncmeta_proxy_authz_bind(
	a_metaconn_t *mc,
	int candidate,
	Operation *op,
	SlapReply *rs,
	ldap_back_send_t sendok,
	int dolock )
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	struct berval		binddn = BER_BVC( "" ),
				cred = BER_BVC( "" );
	int			method = LDAP_AUTH_NONE,
				rc;

	rc = asyncmeta_back_proxy_authz_cred( mc, candidate, op, rs, sendok, &binddn, &cred, &method );
	if ( rc == LDAP_SUCCESS && !LDAP_BACK_CONN_ISBOUND( msc ) ) {
		int	msgid;

		switch ( method ) {
		case LDAP_AUTH_NONE:
		case LDAP_AUTH_SIMPLE:
			for (;;) {
				rs->sr_err = ldap_sasl_bind( msc->msc_ld,
					binddn.bv_val, LDAP_SASL_SIMPLE,
					&cred, NULL, NULL, &msgid );
				if ( rs->sr_err != LDAP_X_CONNECTING ) {
					break;
				}
				ldap_pvt_thread_yield();
			}

			rc = asyncmeta_bind_op_result( op, rs, mc, candidate, msgid, sendok, dolock );
			if ( rc == LDAP_SUCCESS ) {
				/* set rebind stuff in case of successful proxyAuthz bind,
				 * so that referral chasing is attempted using the right
				 * identity */
				LDAP_BACK_CONN_ISBOUND_SET( msc );
				ber_bvreplace( &msc->msc_bound_ndn, &binddn );

				if ( META_BACK_TGT_SAVECRED( mt ) ) {
					if ( !BER_BVISNULL( &msc->msc_cred ) ) {
						memset( msc->msc_cred.bv_val, 0,
							msc->msc_cred.bv_len );
					}
					ber_bvreplace( &msc->msc_cred, &cred );
					ldap_set_rebind_proc( msc->msc_ld, mt->mt_rebind_f, msc );
				}
			}
			break;

		default:
			assert( 0 );
			break;
		}
	}

	return LDAP_BACK_CONN_ISBOUND( msc );
}


static int
asyncmeta_back_proxy_authz_ctrl(Operation *op,
				SlapReply *rs,
				struct berval	*bound_ndn,
				int		version,
				int             isroot,
				slap_idassert_t	*si,
				LDAPControl	*ctrl )
{
	slap_idassert_mode_t	mode;
	struct berval		assertedID,
				ndn;

	rs->sr_err = SLAP_CB_CONTINUE;

	/* FIXME: SASL/EXTERNAL over ldapi:// doesn't honor the authcID,
	 * but if it is not set this test fails.  We need a different
	 * means to detect if idassert is enabled */
	if ( ( BER_BVISNULL( &si->si_bc.sb_authcId ) || BER_BVISEMPTY( &si->si_bc.sb_authcId ) )
		&& ( BER_BVISNULL( &si->si_bc.sb_binddn ) || BER_BVISEMPTY( &si->si_bc.sb_binddn ) )
		&& BER_BVISNULL( &si->si_bc.sb_saslmech ) )
	{
		goto done;
	}

	if ( !op->o_conn || op->o_do_not_cache || ( isroot ) ) {
		goto done;
	}

	if ( op->o_tag == LDAP_REQ_BIND ) {
		ndn = op->o_req_ndn;

#if 0
	} else if ( !BER_BVISNULL( &op->o_conn->c_ndn ) ) {
		ndn = op->o_conn->c_ndn;
#endif
	} else {
		ndn = op->o_ndn;
	}

	if ( si->si_mode == LDAP_BACK_IDASSERT_LEGACY ) {
		if ( op->o_proxy_authz ) {
			/*
			 * FIXME: we do not want to perform proxyAuthz
			 * on behalf of the client, because this would
			 * be performed with "proxyauthzdn" privileges.
			 *
			 * This might actually be too strict, since
			 * the "proxyauthzdn" authzTo, and each entry's
			 * authzFrom attributes may be crafted
			 * to avoid unwanted proxyAuthz to take place.
			 */
#if 0
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "proxyAuthz not allowed within namingContext";
#endif
			goto done;
		}

		if ( !BER_BVISNULL( bound_ndn ) ) {
			goto done;
		}

		if ( BER_BVISNULL( &ndn ) ) {
			goto done;
		}

		if ( BER_BVISNULL( &si->si_bc.sb_binddn ) ) {
			goto done;
		}

	} else if ( si->si_bc.sb_method == LDAP_AUTH_SASL ) {
		if ( ( si->si_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) )
		{
			/* already asserted in SASL via native authz */
			goto done;
		}

	} else if ( si->si_authz && !isroot ) {
		int		rc;
		struct berval authcDN;

		if ( BER_BVISNULL( &ndn ) ) {
			authcDN = slap_empty_bv;
		} else {
			authcDN = ndn;
		}
		rc = slap_sasl_matches( op, si->si_authz,
				&authcDN, &authcDN );
		if ( rc != LDAP_SUCCESS ) {
			if ( si->si_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
				/* ndn is not authorized
				 * to use idassert */
				rs->sr_err = rc;
			}
			goto done;
		}
	}

	if ( op->o_proxy_authz ) {
		/*
		 * FIXME: we can:
		 * 1) ignore the already set proxyAuthz control
		 * 2) leave it in place, and don't set ours
		 * 3) add both
		 * 4) reject the operation
		 *
		 * option (4) is very drastic
		 * option (3) will make the remote server reject
		 * the operation, thus being equivalent to (4)
		 * option (2) will likely break the idassert
		 * assumptions, so we cannot accept it;
		 * option (1) means that we are contradicting
		 * the client's request.
		 *
		 * I think (4) is the only correct choice.
		 */
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "proxyAuthz not allowed within namingContext";
	}

	if ( op->o_is_auth_check ) {
		mode = LDAP_BACK_IDASSERT_NOASSERT;

	} else {
		mode = si->si_mode;
	}

	switch ( mode ) {
	case LDAP_BACK_IDASSERT_LEGACY:
		/* original behavior:
		 * assert the client's identity */
	case LDAP_BACK_IDASSERT_SELF:
		assertedID = ndn;
		break;

	case LDAP_BACK_IDASSERT_ANONYMOUS:
		/* assert "anonymous" */
		assertedID = slap_empty_bv;
		break;

	case LDAP_BACK_IDASSERT_NOASSERT:
		/* don't assert; bind as proxyauthzdn */
		goto done;

	case LDAP_BACK_IDASSERT_OTHERID:
	case LDAP_BACK_IDASSERT_OTHERDN:
		/* assert idassert DN */
		assertedID = si->si_bc.sb_authzId;
		break;

	default:
		assert( 0 );
	}

	/* if we got here, "" is allowed to proxyAuthz */
	if ( BER_BVISNULL( &assertedID ) ) {
		assertedID = slap_empty_bv;
	}

	/* don't idassert the bound DN (ITS#4497) */
	if ( dn_match( &assertedID, bound_ndn ) ) {
		goto done;
	}

	ctrl->ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
	ctrl->ldctl_iscritical = ( ( si->si_flags & LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL ) == LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL );

	switch ( si->si_mode ) {
	/* already in u:ID or dn:DN form */
	case LDAP_BACK_IDASSERT_OTHERID:
	case LDAP_BACK_IDASSERT_OTHERDN:
		ber_dupbv_x( &ctrl->ldctl_value, &assertedID, op->o_tmpmemctx );
		rs->sr_err = LDAP_SUCCESS;
		break;

	/* needs the dn: prefix */
	default:
		ctrl->ldctl_value.bv_len = assertedID.bv_len + STRLENOF( "dn:" );
		ctrl->ldctl_value.bv_val = op->o_tmpalloc( ctrl->ldctl_value.bv_len + 1,
				op->o_tmpmemctx );
		AC_MEMCPY( ctrl->ldctl_value.bv_val, "dn:", STRLENOF( "dn:" ) );
		AC_MEMCPY( &ctrl->ldctl_value.bv_val[ STRLENOF( "dn:" ) ],
				assertedID.bv_val, assertedID.bv_len + 1 );
		rs->sr_err = LDAP_SUCCESS;
		break;
	}

	/* Older versions of <draft-weltman-ldapv3-proxy> required
	 * to encode the value of the authzID (and called it proxyDN);
	 * this hack provides compatibility with those DSAs that
	 * implement it this way */
	if ( si->si_flags & LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND ) {
		struct berval		authzID = ctrl->ldctl_value;
		BerElementBuffer	berbuf;
		BerElement		*ber = (BerElement *)&berbuf;
		ber_tag_t		tag;

		ber_init2( ber, 0, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

		tag = ber_printf( ber, "O", &authzID );
		if ( tag == LBER_ERROR ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber;
		}

		if ( ber_flatten2( ber, &ctrl->ldctl_value, 1 ) == -1 ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber;
		}

		rs->sr_err = LDAP_SUCCESS;

free_ber:;
		op->o_tmpfree( authzID.bv_val, op->o_tmpmemctx );
		ber_free_buf( ber );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto done;
		}

	} else if ( si->si_flags & LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ ) {
		struct berval		authzID = ctrl->ldctl_value,
					tmp;
		BerElementBuffer	berbuf;
		BerElement		*ber = (BerElement *)&berbuf;
		ber_tag_t		tag;

		if ( strncasecmp( authzID.bv_val, "dn:", STRLENOF( "dn:" ) ) != 0 ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		tmp = authzID;
		tmp.bv_val += STRLENOF( "dn:" );
		tmp.bv_len -= STRLENOF( "dn:" );

		ber_init2( ber, 0, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

		/* apparently, Mozilla API encodes this
		 * as "SEQUENCE { LDAPDN }" */
		tag = ber_printf( ber, "{O}", &tmp );
		if ( tag == LBER_ERROR ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber2;
		}

		if ( ber_flatten2( ber, &ctrl->ldctl_value, 1 ) == -1 ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber2;
		}

		ctrl->ldctl_oid = LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ;
		rs->sr_err = LDAP_SUCCESS;

free_ber2:;
		op->o_tmpfree( authzID.bv_val, op->o_tmpmemctx );
		ber_free_buf( ber );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto done;
		}
	}

done:;

	return rs->sr_err;
}

/*
 * Add controls;
 *
 * if any needs to be added, it is prepended to existing ones,
 * in a newly allocated array.  The companion function
 * mi->mi_ldap_extra->controls_free() must be used to restore the original
 * status of op->o_ctrls.
 */
int
asyncmeta_controls_add( Operation *op,
			SlapReply *rs,
			a_metaconn_t	*mc,
			int		candidate,
			int             isroot,
			LDAPControl	***pctrls )
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	LDAPControl		**ctrls = NULL;
	/* set to the maximum number of controls this backend can add */
	LDAPControl		c[ 2 ] = {{ 0 }};
	int			n = 0, i, j1 = 0, j2 = 0, skipped = 0;

	*pctrls = NULL;

	rs->sr_err = LDAP_SUCCESS;

	/* don't add controls if protocol is not LDAPv3 */
	switch ( mt->mt_version ) {
	case LDAP_VERSION3:
		break;

	case 0:
		if ( op->o_protocol == 0 || op->o_protocol == LDAP_VERSION3 ) {
			break;
		}
		/* fall thru */

	default:
		goto done;
	}

	/* put controls that go __before__ existing ones here */

	/* proxyAuthz for identity assertion */
	switch ( asyncmeta_back_proxy_authz_ctrl( op, rs, &msc->msc_bound_ndn,
					     mt->mt_version, isroot, &mt->mt_idassert, &c[ j1 ] ) )
	{
	case SLAP_CB_CONTINUE:
		break;

	case LDAP_SUCCESS:
		j1++;
		break;

	default:
		goto done;
	}

	/* put controls that go __after__ existing ones here */

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	/* session tracking */
	if ( META_BACK_TGT_ST_REQUEST( mt ) ) {
		switch ( slap_ctrl_session_tracking_request_add( op, rs, &c[ j1 + j2 ] ) ) {
		case SLAP_CB_CONTINUE:
			break;

		case LDAP_SUCCESS:
			j2++;
			break;

		default:
			goto done;
		}
	}
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

	if ( rs->sr_err == SLAP_CB_CONTINUE ) {
		rs->sr_err = LDAP_SUCCESS;
	}

	/* if nothing to do, just bail out */
	if ( j1 == 0 && j2 == 0 ) {
		goto done;
	}

	assert( j1 + j2 <= (int) (sizeof( c )/sizeof( c[0] )) );

	if ( op->o_ctrls ) {
		for ( n = 0; op->o_ctrls[ n ]; n++ )
			/* just count ctrls */ ;
	}

	ctrls = op->o_tmpalloc( (n + j1 + j2 + 1) * sizeof( LDAPControl * ) + ( j1 + j2 ) * sizeof( LDAPControl ),
			op->o_tmpmemctx );
	if ( j1 ) {
		ctrls[ 0 ] = (LDAPControl *)&ctrls[ n + j1 + j2 + 1 ];
		*ctrls[ 0 ] = c[ 0 ];
		for ( i = 1; i < j1; i++ ) {
			ctrls[ i ] = &ctrls[ 0 ][ i ];
			*ctrls[ i ] = c[ i ];
		}
	}

	i = 0;
	if ( op->o_ctrls ) {
		LDAPControl *proxyauthz = ldap_control_find(
				LDAP_CONTROL_PROXY_AUTHZ, op->o_ctrls, NULL );

		for ( i = 0; op->o_ctrls[ i ]; i++ ) {
			/* Only replace it if we generated one */
			if ( j1 && proxyauthz && proxyauthz == op->o_ctrls[ i ] ) {
				/* Frontend has already checked only one is present */
				assert( skipped == 0 );
				skipped++;
				continue;
			}
			ctrls[ i + j1 - skipped ] = op->o_ctrls[ i ];
		}
	}

	n += j1 - skipped;
	if ( j2 ) {
		ctrls[ n ] = (LDAPControl *)&ctrls[ n + j2 + 1 ] + j1;
		*ctrls[ n ] = c[ j1 ];
		for ( i = 1; i < j2; i++ ) {
			ctrls[ n + i ] = &ctrls[ n ][ i ];
			*ctrls[ n + i ] = c[ i ];
		}
	}

	ctrls[ n + j2 ] = NULL;

done:;
	if ( ctrls == NULL ) {
		ctrls = op->o_ctrls;
	}

	*pctrls = ctrls;

	return rs->sr_err;
}


/*
 * asyncmeta_dobind_init()
 *
 * initiates bind for a candidate target
 */
meta_search_candidate_t
asyncmeta_dobind_init(Operation *op, SlapReply *rs, bm_context_t *bc, a_metaconn_t *mc, int candidate)
{
	SlapReply		*candidates = bc->candidates;
	a_metainfo_t		*mi = ( a_metainfo_t * )mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	struct berval		binddn = msc->msc_bound_ndn,
				cred = msc->msc_cred;
	int			method;

	int			rc;
	ber_int_t	msgid;

	meta_search_candidate_t	retcode;

	Debug( LDAP_DEBUG_TRACE, "%s >>> asyncmeta_dobind_init[%d] msc %p\n",
		op->o_log_prefix, candidate, msc );

	if ( mc->mc_authz_target == META_BOUND_ALL ) {
		return META_SEARCH_CANDIDATE;
	}

	if ( slapd_shutdown ) {
		rs->sr_err = LDAP_UNAVAILABLE;
		return META_SEARCH_ERR;
	}

	retcode = META_SEARCH_BINDING;
	if ( LDAP_BACK_CONN_ISBOUND( msc ) || LDAP_BACK_CONN_ISANON( msc ) ) {
		/* already bound (or anonymous) */

#ifdef DEBUG_205
		char	buf[ SLAP_TEXT_BUFLEN ] = { '\0' };
		int	bound = 0;

		if ( LDAP_BACK_CONN_ISBOUND( msc ) ) {
			bound = 1;
		}

		Debug( LDAP_DEBUG_ANY,
		       "### %s asyncmeta_dobind_init[%d] mc=%p ld=%p%s DN=\"%s\"\n",
		       op->o_log_prefix, candidate, (void *)mc,
		       (void *)msc->msc_ld, bound ? " bound" : " anonymous",
		       bound == 0 ? "" : msc->msc_bound_ndn.bv_val );
#endif /* DEBUG_205 */

		retcode = META_SEARCH_CANDIDATE;

	} else if ( META_BACK_CONN_CREATING( msc ) || LDAP_BACK_CONN_BINDING( msc ) ) {
		/* another thread is binding the target for this conn; wait */

#ifdef DEBUG_205

		Debug( LDAP_DEBUG_ANY,
		       "### %s asyncmeta_dobind_init[%d] mc=%p ld=%p needbind\n",
		       op->o_log_prefix, candidate, (void *)mc,
		       (void *)msc->msc_ld );
#endif /* DEBUG_205 */

		candidates[ candidate ].sr_msgid = META_MSGID_NEED_BIND;
		retcode = META_SEARCH_NEED_BIND;
	} else {
		/* we'll need to bind the target for this conn */

#ifdef DEBUG_205

		Debug( LDAP_DEBUG_ANY,
		       "### %s asyncmeta_dobind_init[%d] mc=%p ld=%p binding\n",
		       op->o_log_prefix, candidate, (void *)mc,
		       (void *)msc->msc_ld );
#endif /* DEBUG_205 */

		if ( msc->msc_ld == NULL ) {
			/* for some reason (e.g. because formerly in "binding"
			 * state, with eventual connection expiration or invalidation)
			 * it was not initialized as expected */

			Debug( LDAP_DEBUG_ANY, "%s asyncmeta_dobind_init[%d] mc=%p ld=NULL\n",
				op->o_log_prefix, candidate, (void *)mc );

			rc = asyncmeta_init_one_conn( op, rs, mc, candidate,
						      LDAP_BACK_CONN_ISPRIV( mc ), LDAP_BACK_DONTSEND, 0 );

			switch ( rc ) {
			case LDAP_SUCCESS:
				assert( msc->msc_ld != NULL );
				break;

			case LDAP_SERVER_DOWN:
			case LDAP_UNAVAILABLE:
				goto down;

			default:
				goto other;
			}
		}

		LDAP_BACK_CONN_BINDING_SET( msc );
	}

	if ( retcode != META_SEARCH_BINDING ) {
		return retcode;
	}

	if ( op->o_conn != NULL &&
		!op->o_do_not_cache &&
		( BER_BVISNULL( &msc->msc_bound_ndn ) ||
			BER_BVISEMPTY( &msc->msc_bound_ndn ) ||
			( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) ) )
	{
		rc = asyncmeta_back_proxy_authz_cred( mc, candidate, op, rs, LDAP_BACK_DONTSEND, &binddn, &cred, &method );
		switch ( rc ) {
		case LDAP_SUCCESS:
			break;
		case LDAP_UNAVAILABLE:
			goto down;
		default:
			goto other;
		}

		/* NOTE: we copy things here, even if bind didn't succeed yet,
		 * because the connection is not shared until bind is over */
		if ( !BER_BVISNULL( &binddn ) ) {
			ber_bvreplace( &msc->msc_bound_ndn, &binddn );
			if ( META_BACK_TGT_SAVECRED( mt ) && !BER_BVISNULL( &cred ) ) {
				if ( !BER_BVISNULL( &msc->msc_cred ) ) {
					memset( msc->msc_cred.bv_val, 0,
						msc->msc_cred.bv_len );
				}
				ber_bvreplace( &msc->msc_cred, &cred );
			}
		}
		if ( LDAP_BACK_CONN_ISBOUND( msc ) ) {
			/* apparently, idassert was configured with SASL bind,
			 * so bind occurred inside meta_back_proxy_authz_cred() */
			LDAP_BACK_CONN_BINDING_CLEAR( msc );
			return META_SEARCH_CANDIDATE;
		}

		/* paranoid */
		switch ( method ) {
		case LDAP_AUTH_NONE:
		case LDAP_AUTH_SIMPLE:
			/* do a simple bind with binddn, cred */
			break;

		default:
			assert( 0 );
			break;
		}
	}

	assert( msc->msc_ld != NULL );

	if ( !BER_BVISEMPTY( &binddn ) && BER_BVISEMPTY( &cred ) ) {
		/* bind anonymously? */
		Debug( LDAP_DEBUG_ANY, "%s asyncmeta_dobind_init[%d] mc=%p: "
			"non-empty dn with empty cred; binding anonymously\n",
			op->o_log_prefix, candidate, (void *)mc );
		cred = slap_empty_bv;

	} else if ( BER_BVISEMPTY( &binddn ) && !BER_BVISEMPTY( &cred ) ) {
		/* error */
		Debug( LDAP_DEBUG_ANY, "%s asyncmeta_dobind_init[%d] mc=%p: "
			"empty dn with non-empty cred: error\n",
			op->o_log_prefix, candidate, (void *)mc );
		rc = LDAP_OTHER;
		goto other;
	}
retry_bind:
	if ( LogTest( asyncmeta_debug ) ) {
		char	time_buf[ SLAP_TEXT_BUFLEN ];
		asyncmeta_get_timestamp(time_buf);
		Debug( asyncmeta_debug, "[%s] asyncmeta_dobind_init sending bind msc: %p\n",
		      time_buf, msc );
	}
	rc = ldap_sasl_bind( msc->msc_ld, binddn.bv_val, LDAP_SASL_SIMPLE, &cred,
			NULL, NULL, &msgid );
	ldap_get_option( msc->msc_ld, LDAP_OPT_RESULT_CODE, &rc );
	if ( LogTest( asyncmeta_debug ) ) {
		char	time_buf[ SLAP_TEXT_BUFLEN ];
		asyncmeta_get_timestamp(time_buf);
		Debug( asyncmeta_debug, "[%s] asyncmeta_dobind_init rc=%d msc: %p\n",
		      time_buf, rc, msc );
	}
	if ( LogTest( LDAP_DEBUG_TRACE )) {
		ber_socket_t s;
		char sockname[LDAP_IPADDRLEN];
		struct berval sockbv = BER_BVC( sockname );
		Sockaddr addr;
		socklen_t len = sizeof( addr );

		ldap_get_option( msc->msc_ld, LDAP_OPT_DESC, &s );
		getsockname( s, &addr.sa_addr, &len );
		ldap_pvt_sockaddrstr( &addr, &sockbv );
		Debug( LDAP_DEBUG_TRACE, "%s asyncmeta_dobind_init msc %p ld %p ldr %p fd %d addr %s\n",
			op->o_log_prefix, msc, msc->msc_ld, msc->msc_ldr, s, sockname );
	}

	if (rc == LDAP_SERVER_DOWN ) {
		goto down;
	} else if (rc == LDAP_BUSY) {
		if (rs->sr_text == NULL) {
			rs->sr_text = "Unable to establish LDAP connection to target within the specified network timeout.";
		}
		LDAP_BACK_CONN_BINDING_CLEAR( msc );
		goto other;
	}
	/* mark as need bind so it gets send when the bind response is received */
	candidates[ candidate ].sr_msgid = META_MSGID_NEED_BIND;
	asyncmeta_set_msc_time(msc);
#ifdef DEBUG_205
	Debug( LDAP_DEBUG_ANY,
	       "### %s asyncmeta_dobind_init[%d] mc=%p ld=%p rc=%d\n",
	       op->o_log_prefix, candidate, (void *)mc,
	       (void *)mc->mc_conns[candidate].msc_ld, rc );
#endif /* DEBUG_205 */

	switch ( rc ) {
	case LDAP_SUCCESS:
		assert( msgid >= 0 );
		if ( LogTest( asyncmeta_debug ) ) {
			char	time_buf[ SLAP_TEXT_BUFLEN ];
			asyncmeta_get_timestamp(time_buf);
			Debug( asyncmeta_debug, "[%s] asyncmeta_dobind_init sending bind success msc: %p\n",
			      time_buf, msc );
		}
		META_BINDING_SET( &candidates[ candidate ] );
		rs->sr_err = LDAP_SUCCESS;
		msc->msc_binding_time = slap_get_time();
		return META_SEARCH_BINDING;

	case LDAP_X_CONNECTING:
		/* must retry, same conn */
		candidates[ candidate ].sr_msgid = META_MSGID_CONNECTING;
		LDAP_BACK_CONN_BINDING_CLEAR( msc );
		goto retry_bind;

	case LDAP_SERVER_DOWN:
down:;
		retcode = META_SEARCH_ERR;
		rs->sr_err = LDAP_UNAVAILABLE;
		if (rs->sr_text == NULL) {
			rs->sr_text = "Unable to bind to remote target - target down or unavailable";
		}
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
                LDAP_BACK_CONN_BINDING_CLEAR( msc );
		break;

		/* fall thru */

	default:
other:;
		rs->sr_err = rc;
		rc = slap_map_api2result( rs );
		candidates[ candidate ].sr_err = rc;
		if ( META_BACK_ONERR_STOP( mi ) ) {
			retcode = META_SEARCH_ERR;

		} else {
			retcode = META_SEARCH_NOT_CANDIDATE;
		}
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
                LDAP_BACK_CONN_BINDING_CLEAR( msc );
		break;
	}

	return retcode;
}




meta_search_candidate_t
asyncmeta_dobind_init_with_retry(Operation *op, SlapReply *rs, bm_context_t *bc, a_metaconn_t *mc, int candidate)
{

	int rc;
	a_metasingleconn_t *msc = &mc->mc_conns[candidate];
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];

	if (META_BACK_CONN_INVALID(msc) || (LDAP_BACK_CONN_BINDING( msc ) && msc->msc_binding_time > 0
					    && (msc->msc_binding_time + mt->mt_timeout[ SLAP_OP_BIND ]) < slap_get_time())) {
		char	buf[ SLAP_TEXT_BUFLEN ];
		snprintf( buf, sizeof( buf ), "called from %s:%d", __FILE__, __LINE__ );
		ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
		asyncmeta_reset_msc(NULL, mc, candidate, 0, buf);

		rc = asyncmeta_init_one_conn( op, rs, mc, candidate,
					      LDAP_BACK_CONN_ISPRIV( mc ), LDAP_BACK_DONTSEND, 0 );
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	}

	if ( LDAP_BACK_CONN_ISBOUND( msc ) || LDAP_BACK_CONN_ISANON( msc ) ) {
		if ( mc->pending_ops > 1 ) {
			asyncmeta_send_all_pending_ops( mc, candidate, op->o_threadctx, 1 );
		}
		return META_SEARCH_CANDIDATE;
	}

retry_dobind:
	ldap_pvt_thread_mutex_lock( &mc->mc_om_mutex );
	rc = asyncmeta_dobind_init(op, rs, bc, mc, candidate);
	if (rs->sr_err != LDAP_UNAVAILABLE && rs->sr_err != LDAP_BUSY) {
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
		return rc;
	} else if (bc->nretries[candidate] == 0) {
		char	buf[ SLAP_TEXT_BUFLEN ];
		snprintf( buf, sizeof( buf ), "called from %s:%d", __FILE__, __LINE__ );
		asyncmeta_reset_msc(NULL, mc, candidate, 0, buf);
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
		return rc;
	}
	/* need to retry */
	bc->nretries[candidate]--;
	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		/* this lock is required; however,
		 * it's invoked only when logging is on */
		ldap_pvt_thread_mutex_lock( &mt->mt_uri_mutex );
		Debug( LDAP_DEBUG_ANY,
		       "%s asyncmeta_dobind_init_with_retry[%d]: retrying URI=\"%s\" DN=\"%s\".\n",
		       op->o_log_prefix, candidate, mt->mt_uri,
		       BER_BVISNULL(&msc->msc_bound_ndn) ? "" : msc->msc_bound_ndn.bv_val );
		ldap_pvt_thread_mutex_unlock( &mt->mt_uri_mutex );
	}

	asyncmeta_reset_msc(NULL, mc, candidate, 0,  __FUNCTION__);
	rc = asyncmeta_init_one_conn( op, rs, mc, candidate,
				      LDAP_BACK_CONN_ISPRIV( mc ), LDAP_BACK_DONTSEND, 0 );

	if (rs->sr_err != LDAP_SUCCESS) {
		asyncmeta_reset_msc(NULL, mc, candidate, 0,  __FUNCTION__);
		ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
		return META_SEARCH_ERR;
	}
	ldap_pvt_thread_mutex_unlock( &mc->mc_om_mutex );
	goto retry_dobind;
	return rc;
}
