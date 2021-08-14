/*	$NetBSD: init.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* init.c - initialization of a back-asyncmeta database */
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
__RCSID("$NetBSD: init.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "slap-config.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

int asyncmeta_debug;

int
asyncmeta_back_open(
	BackendInfo	*bi )
{
	/* FIXME: need to remove the pagedResults, and likely more... */
	bi->bi_controls = slap_known_controls;

	return 0;
}

int
asyncmeta_back_initialize(
	BackendInfo	*bi )
{
	int rc;
	struct berval debugbv = BER_BVC("asyncmeta");

	rc = slap_loglevel_get( &debugbv, &asyncmeta_debug );
	if ( rc ) {
		return rc;
	}

	bi->bi_flags =
#if 0
	/* this is not (yet) set essentially because back-meta does not
	 * directly support extended operations... */
#ifdef LDAP_DYNAMIC_OBJECTS
		/* this is set because all the support a proxy has to provide
		 * is the capability to forward the refresh exop, and to
		 * pass thru entries that contain the dynamicObject class
		 * and the entryTtl attribute */
		SLAP_BFLAG_DYNAMIC |
#endif /* LDAP_DYNAMIC_OBJECTS */
#endif

		/* back-meta recognizes RFC4525 increment;
		 * let the remote server complain, if needed (ITS#5912) */
		SLAP_BFLAG_INCREMENT;

	bi->bi_open = asyncmeta_back_open;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = asyncmeta_back_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = asyncmeta_back_db_open;
	bi->bi_db_close = asyncmeta_back_db_close;
	bi->bi_db_destroy = asyncmeta_back_db_destroy;

	bi->bi_op_bind = asyncmeta_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = asyncmeta_back_search;
	bi->bi_op_compare = asyncmeta_back_compare;
	bi->bi_op_modify = asyncmeta_back_modify;
	bi->bi_op_modrdn = asyncmeta_back_modrdn;
	bi->bi_op_add = asyncmeta_back_add;
	bi->bi_op_delete = asyncmeta_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy =  0 /* asyncmeta_back_conn_destroy */;

	return asyncmeta_back_init_cf( bi );
}

int
asyncmeta_back_db_init(
	Backend		*be,
	ConfigReply	*cr)
{
	a_metainfo_t	*mi;
	int		i;
	BackendInfo	*bi;

	bi = backend_info( "ldap" );
	if ( !bi || !bi->bi_extra ) {
		Debug( LDAP_DEBUG_ANY,
			"asyncmeta_back_db_init: needs back-ldap\n" );
		return 1;
	}

	mi = ch_calloc( 1, sizeof( a_metainfo_t ) );
	if ( mi == NULL ) {
		return -1;
	}

	/* set default flags */
	mi->mi_flags =
		META_BACK_F_DEFER_ROOTDN_BIND
		| META_BACK_F_PROXYAUTHZ_ALWAYS
		| META_BACK_F_PROXYAUTHZ_ANON
		| META_BACK_F_PROXYAUTHZ_NOANON;

	/*
	 * At present the default is no default target;
	 * this may change
	 */
	mi->mi_defaulttarget = META_DEFAULT_TARGET_NONE;
	mi->mi_bind_timeout.tv_sec = 0;
	mi->mi_bind_timeout.tv_usec = META_BIND_TIMEOUT;

	mi->mi_rebind_f = asyncmeta_back_default_rebind;
	mi->mi_urllist_f = asyncmeta_back_default_urllist;

	ldap_pvt_thread_mutex_init( &mi->mi_cache.mutex );

	/* safe default */
	mi->mi_nretries = META_RETRY_DEFAULT;
	mi->mi_version = LDAP_VERSION3;

	for ( i = 0; i < SLAP_OP_LAST; i++ ) {
		mi->mi_timeout[ i ] = META_BACK_CFG_DEFAULT_OPS_TIMEOUT;
	}

	for ( i = LDAP_BACK_PCONN_FIRST; i < LDAP_BACK_PCONN_LAST; i++ ) {
		mi->mi_conn_priv[ i ].mic_num = 0;
		LDAP_TAILQ_INIT( &mi->mi_conn_priv[ i ].mic_priv );
	}
	mi->mi_conn_priv_max = LDAP_BACK_CONN_PRIV_DEFAULT;

	mi->mi_ldap_extra = (ldap_extra_t *)bi->bi_extra;
	ldap_pvt_thread_mutex_init( &mi->mi_mc_mutex);

	be->be_private = mi;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return 0;
}

int
asyncmeta_target_finish(
	a_metainfo_t *mi,
	a_metatarget_t *mt,
	const char *log,
	char *msg,
	size_t msize
)
{
	slap_bindconf	sb = { BER_BVNULL };
	int rc;

	ber_str2bv( mt->mt_uri, 0, 0, &sb.sb_uri );
	sb.sb_version = mt->mt_version;
	sb.sb_method = LDAP_AUTH_SIMPLE;
	BER_BVSTR( &sb.sb_binddn, "" );

	if ( META_BACK_TGT_T_F_DISCOVER( mt ) ) {
		rc = slap_discover_feature( &sb,
				slap_schema.si_ad_supportedFeatures->ad_cname.bv_val,
				LDAP_FEATURE_ABSOLUTE_FILTERS );
		if ( rc == LDAP_COMPARE_TRUE ) {
			mt->mt_flags |= LDAP_BACK_F_T_F;
		}
	}

	if ( META_BACK_TGT_CANCEL_DISCOVER( mt ) ) {
		rc = slap_discover_feature( &sb,
				slap_schema.si_ad_supportedExtension->ad_cname.bv_val,
				LDAP_EXOP_CANCEL );
		if ( rc == LDAP_COMPARE_TRUE ) {
			mt->mt_flags |= LDAP_BACK_F_CANCEL_EXOP;
		}
	}

	if ( !( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE )
		|| mt->mt_idassert_authz != NULL )
	{
		mi->mi_flags &= ~META_BACK_F_PROXYAUTHZ_ALWAYS;
	}

	if ( ( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL )
		&& !( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) )
	{
		Debug(LDAP_DEBUG_ANY,
		      "%s: inconsistent idassert configuration " "(likely authz=\"*\" used with \"non-prescriptive\" flag) (target %s)\n",
		      log, mt->mt_uri );
		return 1;
	}

	if ( !( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) )
	{
		mi->mi_flags &= ~META_BACK_F_PROXYAUTHZ_ANON;
	}

	if ( ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) )
	{
		mi->mi_flags &= ~META_BACK_F_PROXYAUTHZ_NOANON;
	}

	return 0;
}

int
asyncmeta_back_db_open(
	Backend		*be,
	ConfigReply	*cr )
{
	a_metainfo_t	*mi = (a_metainfo_t *)be->be_private;
	char msg[SLAP_TEXT_BUFLEN];
	int		i;

	if ( mi->mi_ntargets == 0 ) {
		/* Dynamically added, nothing to check here until
		 * some targets get added
		 */
		if ( slapMode & SLAP_SERVER_RUNNING )
			return 0;

		Debug( LDAP_DEBUG_ANY,
			"asyncmeta_back_db_open: no targets defined\n" );
		return 1;
	}
	mi->mi_num_conns = 0;
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		a_metatarget_t	*mt = mi->mi_targets[ i ];
		if ( asyncmeta_target_finish( mi, mt,
			"asyncmeta_back_db_open", msg, sizeof( msg )))
			return 1;
	}
	mi->mi_num_conns = (mi->mi_max_target_conns == 0) ? META_BACK_CFG_MAX_TARGET_CONNS : mi->mi_max_target_conns;
	assert(mi->mi_num_conns > 0);
	mi->mi_conns = ch_calloc( mi->mi_num_conns, sizeof( a_metaconn_t ));
	for (i = 0; i < mi->mi_num_conns; i++) {
		a_metaconn_t *mc = &mi->mi_conns[i];
		ldap_pvt_thread_mutex_init( &mc->mc_om_mutex);
		mc->mc_authz_target = META_BOUND_NONE;
		mc->mc_conns = ch_calloc( mi->mi_ntargets, sizeof( a_metasingleconn_t ));
		mc->mc_info = mi;
		LDAP_STAILQ_INIT( &mc->mc_om_list );
	}
	mi->mi_suffix = be->be_suffix[0];
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	mi->mi_task = ldap_pvt_runqueue_insert( &slapd_rq, 1,
		asyncmeta_timeout_loop, mi, "asyncmeta_timeout_loop", mi->mi_suffix.bv_val );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return 0;
}

/*
 * asyncmeta_back_conn_free()
 *
 * actually frees a connection; the reference count must be 0,
 * and it must not (or no longer) be in the cache.
 */
void
asyncmeta_back_conn_free(
	void 		*v_mc )
{
	a_metaconn_t		*mc = v_mc;

	assert( mc != NULL );
	ldap_pvt_thread_mutex_destroy( &mc->mc_om_mutex );
	free( mc );
}

static void
asyncmeta_back_stop_miconns( a_metainfo_t *mi )
{

	/*Todo do any other mc cleanup here if necessary*/
}

static void
asyncmeta_back_clear_miconns( a_metainfo_t *mi )
{
	int i, j;
	a_metaconn_t *mc;
	for (i = 0; i < mi->mi_num_conns; i++) {
		mc = &mi->mi_conns[i];
		/* todo clear the message queue */
		for (j = 0; j < mi->mi_ntargets; j ++) {
			asyncmeta_clear_one_msc(NULL, mc, j, 1, __FUNCTION__);
		}
		free(mc->mc_conns);
		ldap_pvt_thread_mutex_destroy( &mc->mc_om_mutex );
	}
	free(mi->mi_conns);
}

static void
asyncmeta_target_free(
	a_metatarget_t	*mt )
{
	if ( mt->mt_uri ) {
		free( mt->mt_uri );
		ldap_pvt_thread_mutex_destroy( &mt->mt_uri_mutex );
	}
	if ( mt->mt_subtree ) {
		asyncmeta_subtree_destroy( mt->mt_subtree );
		mt->mt_subtree = NULL;
	}
	if ( mt->mt_filter ) {
		asyncmeta_filter_destroy( mt->mt_filter );
		mt->mt_filter = NULL;
	}
	if ( !BER_BVISNULL( &mt->mt_psuffix ) ) {
		free( mt->mt_psuffix.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_nsuffix ) ) {
		free( mt->mt_nsuffix.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_binddn ) ) {
		free( mt->mt_binddn.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_bindpw ) ) {
		free( mt->mt_bindpw.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_authcID ) ) {
		ch_free( mt->mt_idassert_authcID.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_authcDN ) ) {
		ch_free( mt->mt_idassert_authcDN.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_passwd ) ) {
		ch_free( mt->mt_idassert_passwd.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_authzID ) ) {
		ch_free( mt->mt_idassert_authzID.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_sasl_mech ) ) {
		ch_free( mt->mt_idassert_sasl_mech.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_sasl_realm ) ) {
		ch_free( mt->mt_idassert_sasl_realm.bv_val );
	}
	if ( mt->mt_idassert_authz != NULL ) {
		ber_bvarray_free( mt->mt_idassert_authz );
	}
	if ( !BER_BVISNULL( &mt->mt_lsuffixm )) {
		ch_free( mt->mt_lsuffixm.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_rsuffixm )) {
		ch_free( mt->mt_rsuffixm.bv_val );
	}
	free( mt );
}

int
asyncmeta_back_db_close(
	Backend		*be,
	ConfigReply	*cr )
{
	a_metainfo_t	*mi;

	if ( be->be_private ) {
		mi = ( a_metainfo_t * )be->be_private;
		if ( mi->mi_task != NULL ) {
			ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			if ( ldap_pvt_runqueue_isrunning( &slapd_rq, mi->mi_task )) {
				ldap_pvt_runqueue_stoptask( &slapd_rq,  mi->mi_task);
			}
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			mi->mi_task = NULL;
		}
		ldap_pvt_thread_mutex_lock( &mi->mi_mc_mutex );
		asyncmeta_back_stop_miconns( mi );
		ldap_pvt_thread_mutex_unlock( &mi->mi_mc_mutex );
	}
	return 0;
}

int
asyncmeta_back_db_destroy(
	Backend		*be,
	ConfigReply	*cr )
{
	a_metainfo_t	*mi;

	if ( be->be_private ) {
		int i;

		mi = ( a_metainfo_t * )be->be_private;
		/*
		 * Destroy the per-target stuff (assuming there's at
		 * least one ...)
		 */
		if ( mi->mi_targets != NULL ) {
			for ( i = 0; i < mi->mi_ntargets; i++ ) {
				a_metatarget_t	*mt = mi->mi_targets[ i ];

				if ( META_BACK_TGT_QUARANTINE( mt ) ) {
					if ( mt->mt_quarantine.ri_num != mi->mi_quarantine.ri_num )
					{
						mi->mi_ldap_extra->retry_info_destroy( &mt->mt_quarantine );
					}

					ldap_pvt_thread_mutex_destroy( &mt->mt_quarantine_mutex );
				}

				asyncmeta_target_free( mt );
			}

			free( mi->mi_targets );
		}

		ldap_pvt_thread_mutex_lock( &mi->mi_cache.mutex );
		if ( mi->mi_cache.tree ) {
			ldap_avl_free( mi->mi_cache.tree, asyncmeta_dncache_free );
		}

		ldap_pvt_thread_mutex_unlock( &mi->mi_cache.mutex );
		ldap_pvt_thread_mutex_destroy( &mi->mi_cache.mutex );

		if ( mi->mi_candidates != NULL ) {
			ber_memfree_x( mi->mi_candidates, NULL );
		}

		if ( META_BACK_QUARANTINE( mi ) ) {
			mi->mi_ldap_extra->retry_info_destroy( &mi->mi_quarantine );
		}

		ldap_pvt_thread_mutex_lock( &mi->mi_mc_mutex );
		asyncmeta_back_clear_miconns(mi);
		ldap_pvt_thread_mutex_unlock( &mi->mi_mc_mutex );
		ldap_pvt_thread_mutex_destroy( &mi->mi_mc_mutex );

		free( be->be_private );
	}
	return 0;
}

#if SLAPD_ASYNCMETA == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( asyncmeta )

#endif /* SLAPD_ASYNCMETA == SLAPD_MOD_DYNAMIC */
