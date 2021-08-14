/*	$NetBSD: conn.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* conn.c - handles connections to remote targets */
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
+ * This work was developed by Symas Corporation
+ * based on back-meta module for inclusion in OpenLDAP Software.
+ * This work was sponsored by Ericsson. */

#include <sys/cdefs.h>
__RCSID("$NetBSD: conn.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include "slap.h"
#include "../../../libraries/libldap/ldap-int.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

/*
 * asyncmeta_conn_alloc
 *
 * Allocates a connection structure, making room for all the referenced targets
 */
static a_metaconn_t *
asyncmeta_conn_alloc(
	a_metainfo_t	*mi)
{
	a_metaconn_t	*mc;
	int		ntargets = mi->mi_ntargets;

	assert( ntargets > 0 );

	/* malloc all in one */
	mc = ( a_metaconn_t * )ch_calloc( 1, sizeof( a_metaconn_t ) + ntargets * sizeof( a_metasingleconn_t ));
	if ( mc == NULL ) {
		return NULL;
	}

	mc->mc_info = mi;
	ldap_pvt_thread_mutex_init( &mc->mc_om_mutex);
	mc->mc_authz_target = META_BOUND_NONE;
	mc->mc_conns = (a_metasingleconn_t *)(mc+1);
	return mc;
}

/*
 * asyncmeta_init_one_conn
 *
 * Initializes one connection
 */
int
asyncmeta_init_one_conn(
	Operation		*op,
	SlapReply		*rs,
	a_metaconn_t		*mc,
	int			candidate,
	int			ispriv,
	ldap_back_send_t	sendok,
	int			dolock)
{
	a_metainfo_t		*mi = mc->mc_info;
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];
	a_metasingleconn_t	*msc = NULL;
	int			version;
	a_dncookie		dc;
	int			isauthz = ( candidate == mc->mc_authz_target );
	int			do_return = 0;
#ifdef HAVE_TLS
	int			is_ldaps = 0;
	int			do_start_tls = 0;
#endif /* HAVE_TLS */

	/* if the server is quarantined, and
	 * - the current interval did not expire yet, or
	 * - no more retries should occur,
	 * don't return the connection */
	if ( mt->mt_isquarantined ) {
		slap_retry_info_t	*ri = &mt->mt_quarantine;
		int			dont_retry = 0;

		if ( mt->mt_quarantine.ri_interval ) {
			ldap_pvt_thread_mutex_lock( &mt->mt_quarantine_mutex );
			dont_retry = ( mt->mt_isquarantined > LDAP_BACK_FQ_NO );
			if ( dont_retry ) {
				dont_retry = ( ri->ri_num[ ri->ri_idx ] == SLAP_RETRYNUM_TAIL
					|| slap_get_time() < ri->ri_last + ri->ri_interval[ ri->ri_idx ] );
				if ( !dont_retry ) {
					Debug(LDAP_DEBUG_ANY,
					      "%s asyncmeta_init_one_conn[%d]: quarantine " "retry block #%d try #%d.\n",
					      op->o_log_prefix,
					      candidate, ri->ri_idx,
					      ri->ri_count );

					mt->mt_isquarantined = LDAP_BACK_FQ_RETRYING;
				}

			}
			ldap_pvt_thread_mutex_unlock( &mt->mt_quarantine_mutex );
		}

		if ( dont_retry ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			rs->sr_text = "Target is quarantined";
			Debug( LDAP_DEBUG_ANY, "%s asyncmeta_init_one_conn: Target is quarantined\n",
			       op->o_log_prefix );
			if ( op->o_conn && ( sendok & LDAP_BACK_SENDERR ) ) {
					send_ldap_result( op, rs );
			}
			return rs->sr_err;
		}
	}
		msc = &mc->mc_conns[candidate];
	/*
	 * Already init'ed
	 */
	if ( LDAP_BACK_CONN_ISBOUND( msc )
		|| LDAP_BACK_CONN_ISANON( msc ) )
	{
		assert( msc->msc_ld != NULL );
		rs->sr_err = LDAP_SUCCESS;
		do_return = 1;

	} else if ( META_BACK_CONN_CREATING( msc )
		|| LDAP_BACK_CONN_BINDING( msc ) )
	{
		rs->sr_err = LDAP_SUCCESS;
		do_return = 1;

	} else if ( META_BACK_CONN_INITED( msc ) ) {
		assert( msc->msc_ld != NULL );
		rs->sr_err = LDAP_SUCCESS;
		do_return = 1;

	} else {
		/*
		 * creating...
		 */
		META_BACK_CONN_CREATING_SET( msc );
	}

	if ( do_return ) {
		if ( rs->sr_err != LDAP_SUCCESS
			&& op->o_conn
			&& ( sendok & LDAP_BACK_SENDERR ) )
		{
			send_ldap_result( op, rs );
		}

		return rs->sr_err;
	}

	assert( msc->msc_ld == NULL );

	/*
	 * Attempts to initialize the connection to the target ds
	 */
	ldap_pvt_thread_mutex_lock( &mt->mt_uri_mutex );

	rs->sr_err = ldap_initialize( &msc->msc_ld, mt->mt_uri );
#ifdef HAVE_TLS
	is_ldaps = ldap_is_ldaps_url( mt->mt_uri );
#endif /* HAVE_TLS */
	ldap_pvt_thread_mutex_unlock( &mt->mt_uri_mutex );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s asyncmeta_init_one_conn: ldap_initialize failed err=%d\n",
		       op->o_log_prefix, rs->sr_err );
		goto error_return;
	}

	ldap_set_option( msc->msc_ld, LDAP_OPT_KEEPCONN, LDAP_OPT_ON);

	msc->msc_ldr = ldap_dup(msc->msc_ld);
	if (!msc->msc_ldr) {
		ldap_ld_free(msc->msc_ld, 0, NULL, NULL);
		rs->sr_err = LDAP_NO_MEMORY;
		goto error_return;
	}

	/*
	 * Set LDAP version. This will always succeed: If the client
	 * bound with a particular version, then so can we.
	 */
	if ( mt->mt_version != 0 ) {
		version = mt->mt_version;

	} else if ( op->o_conn->c_protocol != 0 ) {
		version = op->o_conn->c_protocol;

	} else {
		version = LDAP_VERSION3;
	}
	ldap_set_option( msc->msc_ld, LDAP_OPT_PROTOCOL_VERSION, &version );
	ldap_set_urllist_proc( msc->msc_ld, mt->mt_urllist_f, mt->mt_urllist_p );

	/* automatically chase referrals ("chase-referrals [{yes|no}]" statement) */
	ldap_set_option( msc->msc_ld, LDAP_OPT_REFERRALS,
		META_BACK_TGT_CHASE_REFERRALS( mt ) ? LDAP_OPT_ON : LDAP_OPT_OFF );

	slap_client_keepalive(msc->msc_ld, &mt->mt_tls.sb_keepalive);

	if ( mt->mt_tls.sb_tcp_user_timeout > 0 ) {
		ldap_set_option( msc->msc_ld, LDAP_OPT_TCP_USER_TIMEOUT,
				 &mt->mt_tls.sb_tcp_user_timeout );
	}

#ifdef HAVE_TLS
	{
		slap_bindconf *sb = NULL;

		if ( ispriv ) {
			sb = &mt->mt_idassert.si_bc;
		} else {
			sb = &mt->mt_tls;
		}

		bindconf_tls_set( sb, msc->msc_ld );

		if ( !is_ldaps ) {
			if ( META_BACK_TGT_USE_TLS( mt )
				|| ( op->o_conn->c_is_tls && META_BACK_TGT_PROPAGATE_TLS( mt ) ) )
			{
				do_start_tls = 1;
			}
		}
	}

	/* start TLS ("tls [try-]{start|propagate}" statement) */
	if ( do_start_tls ) {
#ifdef SLAP_STARTTLS_ASYNCHRONOUS
		/*
		 * use asynchronous StartTLS; in case, chase referral
		 * FIXME: OpenLDAP does not return referral on StartTLS yet
		 */
		int		msgid;

		rs->sr_err = ldap_start_tls( msc->msc_ld, NULL, NULL, &msgid );
		if ( rs->sr_err == LDAP_SUCCESS ) {
			LDAPMessage	*res = NULL;
			int		rc, nretries = mt->mt_nretries;
			struct timeval	tv;

			LDAP_BACK_TV_SET( &tv );

retry:;
			rc = ldap_result( msc->msc_ld, msgid, LDAP_MSG_ALL, &tv, &res );
			switch ( rc ) {
			case -1:
				rs->sr_err = LDAP_OTHER;
				break;

			case 0:
				if ( nretries != 0 ) {
					if ( nretries > 0 ) {
						nretries--;
					}
					LDAP_BACK_TV_SET( &tv );
					goto retry;
				}
				rs->sr_err = LDAP_OTHER;
				break;

			default:
				/* only touch when activity actually took place... */
				if ( mi->mi_idle_timeout != 0 ) {
					asyncmeta_set_msc_time(msc);
				}
				break;
			}

			if ( rc == LDAP_RES_EXTENDED ) {
				struct berval	*data = NULL;

				/* NOTE: right now, data is unused, so don't get it */
				rs->sr_err = ldap_parse_extended_result( msc->msc_ld,
					res, NULL, NULL /* &data */ , 0 );
				if ( rs->sr_err == LDAP_SUCCESS ) {
					int		err;

					/* FIXME: matched? referrals? response controls? */
					rs->sr_err = ldap_parse_result( msc->msc_ld,
						res, &err, NULL, NULL, NULL, NULL, 1 );
					res = NULL;

					if ( rs->sr_err == LDAP_SUCCESS ) {

						rs->sr_err = err;
					}
					rs->sr_err = slap_map_api2result( rs );

					/* FIXME: in case a referral
					 * is returned, should we try
					 * using it instead of the
					 * configured URI? */
					if ( rs->sr_err == LDAP_SUCCESS ) {
						rs->sr_err = ldap_install_tls( msc->msc_ld );

					} else if ( rs->sr_err == LDAP_REFERRAL ) {
						/* FIXME: LDAP_OPERATIONS_ERROR? */
						rs->sr_err = LDAP_OTHER;
						rs->sr_text = "Unwilling to chase referral "
							"returned by Start TLS exop";
					}

					if ( data ) {
						ber_bvfree( data );
					}
				}

			} else {
				rs->sr_err = LDAP_OTHER;
			}

			if ( res != NULL ) {
				ldap_msgfree( res );
			}
		}
#else /* ! SLAP_STARTTLS_ASYNCHRONOUS */
		/*
		 * use synchronous StartTLS
		 */
		rs->sr_err = ldap_start_tls_s( msc->msc_ld, NULL, NULL );
#endif /* ! SLAP_STARTTLS_ASYNCHRONOUS */
		if (rs->sr_err != LDAP_SUCCESS) {
			Debug( LDAP_DEBUG_ANY, "%s asyncmeta_init_one_conn: ldap_start_tls_s failed err=%d\n",
			       op->o_log_prefix, rs->sr_err );
		}
		/* if StartTLS is requested, only attempt it if the URL
		 * is not "ldaps://"; this may occur not only in case
		 * of misconfiguration, but also when used in the chain
		 * overlay, where the "uri" can be parsed out of a referral */
		if ( rs->sr_err == LDAP_SERVER_DOWN
			|| ( rs->sr_err != LDAP_SUCCESS
				&& META_BACK_TGT_TLS_CRITICAL( mt ) ) )
		{

#ifdef DEBUG_205
			Debug( LDAP_DEBUG_ANY,
				"### %s asyncmeta_init_one_conn(TLS) "
				"ldap_unbind_ext[%d] ld=%p\n",
				op->o_log_prefix, candidate,
				(void *)msc->msc_ld );
#endif /* DEBUG_205 */

			/* need to trash a failed Start TLS */
			asyncmeta_clear_one_msc( op, mc, candidate, 1, __FUNCTION__ );
			goto error_return;
		}
	}
#endif /* HAVE_TLS */
	/*
	 * Set the network timeout if set
	 */
	if ( mt->mt_network_timeout != 0 ) {
		struct timeval  network_timeout;
		network_timeout.tv_sec = 0;
		network_timeout.tv_usec = mt->mt_network_timeout*1000;

		ldap_set_option( msc->msc_ld, LDAP_OPT_NETWORK_TIMEOUT,
				 (void *)&network_timeout );
	}

	/*
	 * If the connection DN is not null, an attempt to rewrite it is made
	 */

	if ( ispriv ) {
		if ( !BER_BVISNULL( &mt->mt_idassert_authcDN ) ) {
			ber_bvreplace( &msc->msc_bound_ndn, &mt->mt_idassert_authcDN );
			if ( !BER_BVISNULL( &mt->mt_idassert_passwd ) ) {
				if ( !BER_BVISNULL( &msc->msc_cred ) ) {
					memset( msc->msc_cred.bv_val, 0,
						msc->msc_cred.bv_len );
				}
				ber_bvreplace( &msc->msc_cred, &mt->mt_idassert_passwd );
			}
			LDAP_BACK_CONN_ISIDASSERT_SET( msc );

		} else {
			ber_bvreplace( &msc->msc_bound_ndn, &slap_empty_bv );
		}

	} else {
		if ( !BER_BVISNULL( &msc->msc_cred ) ) {
			memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
			ber_memfree_x( msc->msc_cred.bv_val, NULL );
			BER_BVZERO( &msc->msc_cred );
		}
		if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
			ber_memfree_x( msc->msc_bound_ndn.bv_val, NULL );
			BER_BVZERO( &msc->msc_bound_ndn );
		}
		if ( !BER_BVISEMPTY( &op->o_ndn )
			&& isauthz )
		{
			dc.op = op;
			dc.target = mt;
			dc.memctx = NULL;
			dc.to_from = MASSAGE_REQ;

			/*
			 * Rewrite the bind dn if needed
			 */
			asyncmeta_dn_massage( &dc, &op->o_conn->c_dn, &msc->msc_bound_ndn );

			/* copy the DN if needed */
			if ( msc->msc_bound_ndn.bv_val == op->o_conn->c_dn.bv_val ) {
				ber_dupbv( &msc->msc_bound_ndn, &op->o_conn->c_dn );
			}
		} else {
			ber_dupbv( &msc->msc_bound_ndn, (struct berval *)&slap_empty_bv );
		}
	}
	assert( !BER_BVISNULL( &msc->msc_bound_ndn ) );

error_return:;

	if (msc != NULL) {
		META_BACK_CONN_CREATING_CLEAR( msc );
	}
	if ( rs->sr_err == LDAP_SUCCESS && msc != NULL) {
		META_BACK_CONN_INITED_SET( msc );
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		rs->sr_err = slap_map_api2result( rs );
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
	}
	return rs->sr_err;
}


static int
asyncmeta_get_candidate(
	Operation	*op,
	SlapReply	*rs,
	struct berval	*ndn )
{
	a_metainfo_t	*mi = ( a_metainfo_t * )op->o_bd->be_private;
	long		candidate;

	/*
	 * tries to get a unique candidate
	 * (takes care of default target)
	 */
	candidate = asyncmeta_select_unique_candidate( mi, ndn );

	/*
	 * if any is found, inits the connection
	 */
	if ( candidate == META_TARGET_NONE ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_text = "No suitable candidate target found";

	} else {
		rs->sr_err = LDAP_SUCCESS;
	}

	return candidate;
}


/*
 * asyncmeta_getconn
 *
 * Prepares the connection structure
 *
 * RATIONALE:
 *
 * - determine what DN is being requested:
 *
 *	op	requires candidate	checks
 *
 *	add	unique			parent of o_req_ndn
 *	bind	unique^*[/all]		o_req_ndn [no check]
 *	compare	unique^+		o_req_ndn
 *	delete	unique			o_req_ndn
 *	modify	unique			o_req_ndn
 *	search	any			o_req_ndn
 *	modrdn	unique[, unique]	o_req_ndn[, orr_nnewSup]
 *
 * - for ops that require the candidate to be unique, in case of multiple
 *   occurrences an internal search with sizeLimit=1 is performed
 *   if a unique candidate can actually be determined.  If none is found,
 *   the operation aborts; if multiple are found, the default target
 *   is used if defined and candidate; otherwise the operation aborts.
 *
 * *^note: actually, the bind operation is handled much like a search;
 *   i.e. the bind is broadcast to all candidate targets.
 *
 * +^note: actually, the compare operation is handled much like a search;
 *   i.e. the compare is broadcast to all candidate targets, while checking
 *   that exactly none (noSuchObject) or one (TRUE/FALSE/UNDEFINED) is
 *   returned.
 */
a_metaconn_t *
asyncmeta_getconn(
	Operation 		*op,
	SlapReply		*rs,
	SlapReply               *candidates,
	int 			*candidate,
	ldap_back_send_t	sendok,
	int                     alloc_new)
{
	a_metainfo_t	*mi = ( a_metainfo_t * )op->o_bd->be_private;
	a_metaconn_t	*mc = NULL,
			mc_curr = {{ 0 }};
	int		cached = META_TARGET_NONE,
			i = META_TARGET_NONE,
			err = LDAP_SUCCESS,
			new_conn = 0,
			ncandidates = 0;


	meta_op_type	op_type = META_OP_REQUIRE_SINGLE;
	enum		{
		META_DNTYPE_ENTRY,
		META_DNTYPE_PARENT,
		META_DNTYPE_NEWPARENT
	}		dn_type = META_DNTYPE_ENTRY;
	struct berval	ndn = op->o_req_ndn,
			pndn;

	if (alloc_new > 0) {
		mc = asyncmeta_conn_alloc(mi);
		new_conn = 0;
	} else {
		mc = asyncmeta_get_next_mc(mi);
	}

	ldap_pvt_thread_mutex_lock(&mc->mc_om_mutex);
	/* Internal searches are privileged and shared. So is root. */
	if ( ( !BER_BVISEMPTY( &op->o_ndn ) && META_BACK_PROXYAUTHZ_ALWAYS( mi ) )
		|| ( BER_BVISEMPTY( &op->o_ndn ) && META_BACK_PROXYAUTHZ_ANON( mi ) )
		|| op->o_do_not_cache || be_isroot( op ) )
	{
		LDAP_BACK_CONN_ISPRIV_SET( &mc_curr );
		LDAP_BACK_PCONN_ROOTDN_SET( &mc_curr, op );

	} else if ( BER_BVISEMPTY( &op->o_ndn ) && META_BACK_PROXYAUTHZ_NOANON( mi ) )
	{
		LDAP_BACK_CONN_ISANON_SET( &mc_curr );
		LDAP_BACK_PCONN_ANON_SET( &mc_curr, op );

	} else {
		/* Explicit binds must not be shared */
		if ( !BER_BVISEMPTY( &op->o_ndn )
			|| op->o_tag == LDAP_REQ_BIND
			|| SLAP_IS_AUTHZ_BACKEND( op ) )
		{
			//mc_curr.mc_conn = op->o_conn;

		} else {
			LDAP_BACK_CONN_ISANON_SET( &mc_curr );
			LDAP_BACK_PCONN_ANON_SET( &mc_curr, op );
		}
	}

	switch ( op->o_tag ) {
	case LDAP_REQ_ADD:
		/* if we go to selection, the entry must not exist,
		 * and we must be able to resolve the parent */
		dn_type = META_DNTYPE_PARENT;
		dnParent( &ndn, &pndn );
		break;

	case LDAP_REQ_MODRDN:
		/* if nnewSuperior is not NULL, it must resolve
		 * to the same candidate as the req_ndn */
		if ( op->orr_nnewSup ) {
			dn_type = META_DNTYPE_NEWPARENT;
		}
		break;

	case LDAP_REQ_BIND:
		/* if bound as rootdn, the backend must bind to all targets
		 * with the administrative identity
		 * (unless pseoudoroot-bind-defer is TRUE) */
		if ( op->orb_method == LDAP_AUTH_SIMPLE && be_isroot_pw( op ) ) {
			op_type = META_OP_REQUIRE_ALL;
		}
		break;

	case LDAP_REQ_COMPARE:
	case LDAP_REQ_DELETE:
	case LDAP_REQ_MODIFY:
		/* just a unique candidate */
		break;

	case LDAP_REQ_SEARCH:
		/* allow multiple candidates for the searchBase */
		op_type = META_OP_ALLOW_MULTIPLE;
		break;

	default:
		/* right now, just break (exop?) */
		break;
	}

	/*
	 * require all connections ...
	 */
	if ( op_type == META_OP_REQUIRE_ALL ) {
		if ( LDAP_BACK_CONN_ISPRIV( &mc_curr ) ) {
			LDAP_BACK_CONN_ISPRIV_SET( mc );

		} else if ( LDAP_BACK_CONN_ISANON( &mc_curr ) ) {
			LDAP_BACK_CONN_ISANON_SET( mc );
		}

		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			/*
			 * The target is activated; if needed, it is
			 * also init'd
			 */
			candidates[ i ].sr_err = asyncmeta_init_one_conn( op,
				rs, mc, i, LDAP_BACK_CONN_ISPRIV( &mc_curr ),
				LDAP_BACK_DONTSEND, !new_conn );
			if ( candidates[ i ].sr_err == LDAP_SUCCESS ) {
				if ( new_conn && ( sendok & LDAP_BACK_BINDING ) ) {
					LDAP_BACK_CONN_BINDING_SET( &mc->mc_conns[ i ] );
				}
				META_CANDIDATE_SET( &candidates[ i ] );
				ncandidates++;

			} else {

				/*
				 * FIXME: in case one target cannot
				 * be init'd, should the other ones
				 * be tried?
				 */
				META_CANDIDATE_RESET( &candidates[ i ] );
				err = candidates[ i ].sr_err;
				continue;
			}
		}

		if ( ncandidates == 0 ) {
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			rs->sr_text = "Unable to select valid candidates";

			if ( sendok & LDAP_BACK_SENDERR ) {
				if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
					rs->sr_matched = mi->mi_suffix.bv_val;
				}
				send_ldap_result( op, rs );
				rs->sr_matched = NULL;
			}
			ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
			if ( alloc_new > 0) {
				asyncmeta_back_conn_free( mc );
			}
			return NULL;
		}

		goto done;
	}

	/*
	 * looks in cache, if any
	 */
	if ( mi->mi_cache.ttl != META_DNCACHE_DISABLED ) {
		cached = i = asyncmeta_dncache_get_target( &mi->mi_cache, &op->o_req_ndn );
	}

	if ( op_type == META_OP_REQUIRE_SINGLE ) {
		int			j;

		for ( j = 0; j < mi->mi_ntargets; j++ ) {
			META_CANDIDATE_RESET( &candidates[ j ] );
		}

		/*
		 * tries to get a unique candidate
		 * (takes care of default target)
		 */
		if ( i == META_TARGET_NONE ) {
			i = asyncmeta_get_candidate( op, rs, &ndn );

			if ( rs->sr_err == LDAP_NO_SUCH_OBJECT && dn_type == META_DNTYPE_PARENT ) {
				i = asyncmeta_get_candidate( op, rs, &pndn );
			}

			if ( i < 0 || rs->sr_err != LDAP_SUCCESS ) {
				if ( sendok & LDAP_BACK_SENDERR ) {
					if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
						rs->sr_matched = mi->mi_suffix.bv_val;
					}
					send_ldap_result( op, rs );
					rs->sr_matched = NULL;
				}
				ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
				if ( mc != NULL && alloc_new ) {
					asyncmeta_back_conn_free( mc );
				}
				return NULL;
			}
		}

		if ( dn_type == META_DNTYPE_NEWPARENT && asyncmeta_get_candidate( op, rs, op->orr_nnewSup ) != i )
		{
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "Cross-target rename not supported";
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
			if ( mc != NULL && alloc_new > 0 ) {
				asyncmeta_back_conn_free( mc );
			}
			return NULL;
		}

		Debug( LDAP_DEBUG_TRACE,
		       "==>asyncmeta__getconn: got target=%d for ndn=\"%s\" from cache\n",
		       i, op->o_req_ndn.bv_val );
			if ( LDAP_BACK_CONN_ISPRIV( &mc_curr ) ) {
				LDAP_BACK_CONN_ISPRIV_SET( mc );

			} else if ( LDAP_BACK_CONN_ISANON( &mc_curr ) ) {
				LDAP_BACK_CONN_ISANON_SET( mc );
			}

		/*
		 * Clear all other candidates
		 */
			( void )asyncmeta_clear_unused_candidates( op, i , mc, candidates);

		/*
		 * The target is activated; if needed, it is
		 * also init'd. In case of error, asyncmeta_init_one_conn
		 * sends the appropriate result.
		 */
		err = asyncmeta_init_one_conn( op, rs, mc, i,
			LDAP_BACK_CONN_ISPRIV( &mc_curr ), sendok, !new_conn );
		if ( err != LDAP_SUCCESS ) {
			/*
			 * FIXME: in case one target cannot
			 * be init'd, should the other ones
			 * be tried?
			 */
			META_CANDIDATE_RESET( &candidates[ i ] );
			ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
			if ( mc != NULL && alloc_new > 0 ) {
				asyncmeta_back_conn_free( mc );
			}
			return NULL;
		}

		candidates[ i ].sr_err = LDAP_SUCCESS;
		META_CANDIDATE_SET( &candidates[ i ] );
		ncandidates++;

		if ( candidate ) {
			*candidate = i;
		}

	/*
	 * if no unique candidate ...
	 */
	} else {
	if ( LDAP_BACK_CONN_ISPRIV( &mc_curr ) ) {
		LDAP_BACK_CONN_ISPRIV_SET( mc );

	} else if ( LDAP_BACK_CONN_ISANON( &mc_curr ) ) {
		LDAP_BACK_CONN_ISANON_SET( mc );
	}

		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			a_metatarget_t		*mt = mi->mi_targets[ i ];

			META_CANDIDATE_RESET( &candidates[ i ] );

			if ( i == cached
				|| asyncmeta_is_candidate( mt, &op->o_req_ndn,
					op->o_tag == LDAP_REQ_SEARCH ? op->ors_scope : LDAP_SCOPE_SUBTREE ) )
			{

				/*
				 * The target is activated; if needed, it is
				 * also init'd
				 */
				int lerr = asyncmeta_init_one_conn( op, rs, mc, i,
					LDAP_BACK_CONN_ISPRIV( &mc_curr ),
					LDAP_BACK_DONTSEND, !new_conn );
				candidates[ i ].sr_err = lerr;
				if ( lerr == LDAP_SUCCESS ) {
					META_CANDIDATE_SET( &candidates[ i ] );
					ncandidates++;

					Debug( LDAP_DEBUG_TRACE, "%s: asyncmeta_getconn[%d]\n",
						op->o_log_prefix, i );

				} else if ( lerr == LDAP_UNAVAILABLE && !META_BACK_ONERR_STOP( mi ) ) {
					META_CANDIDATE_SET( &candidates[ i ] );

					Debug( LDAP_DEBUG_TRACE, "%s: asyncmeta_getconn[%d] %s\n",
						op->o_log_prefix, i,
						mt->mt_isquarantined != LDAP_BACK_FQ_NO ? "quarantined" : "unavailable" );

				} else {

					/*
					 * FIXME: in case one target cannot
					 * be init'd, should the other ones
					 * be tried?
					 */
					/* leave the target candidate, but record the error for later use */
					err = lerr;

					if ( lerr == LDAP_UNAVAILABLE && mt->mt_isquarantined != LDAP_BACK_FQ_NO ) {
						Debug( LDAP_DEBUG_TRACE, "%s: asyncmeta_getconn[%d] quarantined err=%d\n",
							op->o_log_prefix, i, lerr );

					} else {
						Debug( LDAP_DEBUG_ANY, "%s: asyncmeta_getconn[%d] failed err=%d\n",
							op->o_log_prefix, i, lerr );
					}

					if ( META_BACK_ONERR_STOP( mi ) ) {
						if ( sendok & LDAP_BACK_SENDERR ) {
							send_ldap_result( op, rs );
						}
						ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
						if ( alloc_new > 0 ) {
							asyncmeta_back_conn_free( mc );

						}
						return NULL;
					}

					continue;
				}

			}
		}

		if ( ncandidates == 0 ) {
			if ( rs->sr_err == LDAP_SUCCESS ) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = "Unable to select valid candidates";
			}

			if ( sendok & LDAP_BACK_SENDERR ) {
				if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
					rs->sr_matched = mi->mi_suffix.bv_val;
				}
				send_ldap_result( op, rs );
				rs->sr_matched = NULL;
			}
			if ( alloc_new > 0 ) {
				asyncmeta_back_conn_free( mc );

			}
			ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
			return NULL;
		}
	}

done:;
	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;

	if ( new_conn ) {
		if ( !LDAP_BACK_PCONN_ISPRIV( mc ) ) {
			/*
			 * Err could be -1 in case a duplicate metaconn is inserted
			 */
			switch ( err ) {
			case 0:
				break;
			default:
				LDAP_BACK_CONN_CACHED_CLEAR( mc );
				if ( LogTest( LDAP_DEBUG_ANY ) ) {
					char buf[STRLENOF("4294967295U") + 1] = { 0 };
					mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

					Debug( LDAP_DEBUG_ANY,
						"%s asyncmeta_getconn: candidates=%d conn=%s insert failed\n",
						op->o_log_prefix, ncandidates, buf );
				}

				asyncmeta_back_conn_free( mc );

				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "Proxy bind collision";
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				return NULL;
			}
		}

		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			char buf[STRLENOF("4294967295U") + 1] = { 0 };
			mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

			Debug( LDAP_DEBUG_TRACE,
				"%s asyncmeta_getconn: candidates=%d conn=%s inserted\n",
				op->o_log_prefix, ncandidates, buf );
		}

	} else {
		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			char buf[STRLENOF("4294967295U") + 1] = { 0 };
			mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

			Debug( LDAP_DEBUG_TRACE,
				"%s asyncmeta_getconn: candidates=%d conn=%s fetched\n",
				op->o_log_prefix, ncandidates, buf );
		}
	}
	ldap_pvt_thread_mutex_unlock(&mc->mc_om_mutex);
	return mc;
}

void
asyncmeta_quarantine(
	Operation	*op,
	a_metainfo_t    *mi,
	SlapReply	*rs,
	int		candidate )
{
	a_metatarget_t		*mt = mi->mi_targets[ candidate ];

	slap_retry_info_t	*ri = &mt->mt_quarantine;

	ldap_pvt_thread_mutex_lock( &mt->mt_quarantine_mutex );

	if ( rs->sr_err == LDAP_UNAVAILABLE ) {
		time_t	new_last = slap_get_time();

		switch ( mt->mt_isquarantined ) {
		case LDAP_BACK_FQ_NO:
			if ( ri->ri_last == new_last ) {
				goto done;
			}

			Debug( LDAP_DEBUG_ANY,
				"%s asyncmeta_quarantine[%d]: enter.\n",
				op->o_log_prefix, candidate );

			ri->ri_idx = 0;
			ri->ri_count = 0;
			break;

		case LDAP_BACK_FQ_RETRYING:
			Debug(LDAP_DEBUG_ANY,
			      "%s asyncmeta_quarantine[%d]: block #%d try #%d failed.\n",
			      op->o_log_prefix, candidate, ri->ri_idx,
			      ri->ri_count );

			++ri->ri_count;
			if ( ri->ri_num[ ri->ri_idx ] != SLAP_RETRYNUM_FOREVER
				&& ri->ri_count == ri->ri_num[ ri->ri_idx ] )
			{
				ri->ri_count = 0;
				++ri->ri_idx;
			}
			break;

		default:
			goto done;
		}

		mt->mt_isquarantined = LDAP_BACK_FQ_YES;
		ri->ri_last = new_last;

	} else if ( mt->mt_isquarantined == LDAP_BACK_FQ_RETRYING ) {
		Debug( LDAP_DEBUG_ANY,
			"%s asyncmeta_quarantine[%d]: exit.\n",
			op->o_log_prefix, candidate );

		if ( mi->mi_quarantine_f ) {
			(void)mi->mi_quarantine_f( mi, candidate,
				mi->mi_quarantine_p );
		}

		ri->ri_count = 0;
		ri->ri_idx = 0;
		mt->mt_isquarantined = LDAP_BACK_FQ_NO;
		mt->mt_timeout_ops = 0;
	}

done:;
	ldap_pvt_thread_mutex_unlock( &mt->mt_quarantine_mutex );
}

a_metaconn_t *
asyncmeta_get_next_mc( a_metainfo_t *mi )
{
	a_metaconn_t *mc = NULL;

	ldap_pvt_thread_mutex_lock( &mi->mi_mc_mutex );
	if (mi->mi_next_conn >= mi->mi_num_conns-1) {
		mi->mi_next_conn = 0;
	} else {
		mi->mi_next_conn++;
	}

	mc = &mi->mi_conns[mi->mi_next_conn];
	ldap_pvt_thread_mutex_unlock( &mi->mi_mc_mutex );
	return mc;
}

int asyncmeta_start_listeners(a_metaconn_t *mc, SlapReply *candidates, bm_context_t *bc)
{
	int i;
	for (i = 0; i < mc->mc_info->mi_ntargets; i++) {
		asyncmeta_start_one_listener(mc, candidates, bc, i);
	}
	return LDAP_SUCCESS;
}

int asyncmeta_start_one_listener(a_metaconn_t *mc,
				 SlapReply *candidates,
				 bm_context_t *bc,
				 int candidate)
{
	a_metasingleconn_t *msc;
	ber_socket_t s;

	msc = &mc->mc_conns[candidate];
	if ( slapd_shutdown || !META_BACK_CONN_INITED( msc ) || msc->msc_ld == NULL
	    || META_BACK_CONN_INVALID(msc) || !META_IS_CANDIDATE( &candidates[ candidate ] )) {
		return LDAP_SUCCESS;
	}
	bc->msgids[candidate] = candidates[candidate].sr_msgid;
	if ( msc->conn == NULL) {
		ldap_get_option( msc->msc_ld, LDAP_OPT_DESC, &s );
		if (s < 0) {
			/* Todo a meaningful log pls */
			return LDAP_OTHER;
		}
		msc->conn = connection_client_setup( s, asyncmeta_op_handle_result, mc );
	}
	connection_client_enable( msc->conn );
	return LDAP_SUCCESS;
}

int
asyncmeta_clear_one_msc(
	Operation	*op,
	a_metaconn_t	*mc,
	int		candidate,
	int             unbind,
	const char *caller)
{
	a_metasingleconn_t *msc;
	if (mc == NULL) {
		return 0;
	}
	msc = &mc->mc_conns[candidate];
	if ( LogTest( asyncmeta_debug ) ) {
		char	time_buf[ SLAP_TEXT_BUFLEN ];
		asyncmeta_get_timestamp(time_buf);
		Debug( asyncmeta_debug, "[%s] Resetting msc: %p, msc_ld: %p, "
		       "msc_bound_ndn: %s, msc->conn: %p, %s \n",
		       time_buf, msc, msc->msc_ld, msc->msc_bound_ndn.bv_val,
		       msc->conn, caller ? caller : "" );
	}
	msc->msc_mscflags = 0;
	if (msc->conn) {
		connection_client_stop( msc->conn );
		msc->conn = NULL;
	}

	if ( msc->msc_ld != NULL ) {

#ifdef DEBUG_205
		Debug( LDAP_DEBUG_ANY, "### %s asyncmeta_clear_one_msc ldap_unbind_ext[%d] ld=%p\n",
		       op ? op->o_log_prefix : "", candidate, (void *)msc->msc_ld );
#endif /* DEBUG_205 */

		ldap_unbind_ext( msc->msc_ld, NULL, NULL );
		msc->msc_ld = NULL;
		ldap_ld_free( msc->msc_ldr, 0, NULL, NULL );
		msc->msc_ldr = NULL;
	}

	if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
		ber_memfree_x( msc->msc_bound_ndn.bv_val, NULL );
		BER_BVZERO( &msc->msc_bound_ndn );
	}

	if ( !BER_BVISNULL( &msc->msc_cred ) ) {
		memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
		ber_memfree_x( msc->msc_cred.bv_val, NULL );
		BER_BVZERO( &msc->msc_cred );
	}
	msc->msc_time = 0;
	msc->msc_binding_time = 0;
	msc->msc_result_time = 0;
	return 0;
}

void asyncmeta_get_timestamp(char *buf)
{
	struct timespec tp;
	struct tm *ttm;
	clock_gettime(CLOCK_REALTIME, &tp);
	ttm = gmtime(&tp.tv_sec);
	sprintf(buf, "%d:%d:%d.%ld", ttm->tm_hour, ttm->tm_min, ttm->tm_sec, tp.tv_nsec/1000);
}

int
asyncmeta_reset_msc(
	Operation	*op,
	a_metaconn_t	*mc,
	int		candidate,
	int             unbind,
	const char *caller)
{
	a_metasingleconn_t *msc = &mc->mc_conns[candidate];
	if ( LogTest( asyncmeta_debug ) ) {
		char	time_buf[ SLAP_TEXT_BUFLEN ];
		asyncmeta_get_timestamp(time_buf);
		Debug(asyncmeta_debug, "[%x] Will attempt to reset [%s] msc: %p, "
		      "msc->msc_binding_time: %x, msc->msc_flags:%x %s\n",
		      (unsigned int)slap_get_time(), time_buf, msc,
		      (unsigned int)msc->msc_binding_time, msc->msc_mscflags, caller );
	}
	if (msc->msc_active <= 1 && mc->mc_active < 1) {
		bm_context_t *om;
		asyncmeta_clear_one_msc(NULL, mc, candidate, 0, caller);
		/* set whatever's in the queue to invalid, so the timeout loop cleans it up,
		 * but do not invalidate the current op*/
		LDAP_STAILQ_FOREACH( om, &mc->mc_om_list, bc_next ) {
			if (om->candidates[candidate].sr_msgid >= 0 && (om->op != op)) {
				om->bc_invalid = 1;
			}
		}
		return LDAP_SUCCESS;
	} else {
		META_BACK_CONN_INVALID_SET(msc);
		Debug( asyncmeta_debug, "[%x] Failed to reset msc %p, msc_active=%d, mc_active=%d, %s\n",
		       (unsigned int)slap_get_time(), msc, msc->msc_active, mc->mc_active, caller );
	}
	return LDAP_OTHER;
}


void asyncmeta_log_msc(a_metasingleconn_t *msc)
{
	ber_socket_t s = 0;
	if (msc->msc_ld) {
		ldap_get_option( msc->msc_ld, LDAP_OPT_DESC, &s );
	}
	Debug( asyncmeta_debug, "msc: %p, msc_ld: %p, msc_ld socket: %d, "
	       "msc_bound_ndn: %s, msc->conn: %p\n", msc, msc->msc_ld,
	       (int)s, msc->msc_bound_ndn.bv_val, msc->conn );
}

void asyncmeta_log_conns(a_metainfo_t *mi)
{
	a_metaconn_t *mc;
	int i, j;
	for (i = 0; i < mi->mi_num_conns; i++) {
		mc = &mi->mi_conns[i];
		Debug(asyncmeta_debug, "mc: %p, mc->pending_ops: %d\n", mc, mc->pending_ops);
		for (j = 0; j < mi->mi_ntargets; j++ ) {
			asyncmeta_log_msc(&mc->mc_conns[j]);
		}

	}
}
