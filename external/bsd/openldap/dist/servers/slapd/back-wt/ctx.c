/*	$NetBSD: ctx.c,v 1.3 2025/09/05 21:16:31 christos Exp $	*/

/* OpenLDAP WiredTiger backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2024 The OpenLDAP Foundation.
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

wt_ctx *
wt_ctx_init(struct wt_info *wi)
{
	int rc;
	wt_ctx *wc;

	wc = ch_malloc( sizeof( wt_ctx ) );
	if( !wc ) {
		Debug( LDAP_DEBUG_ANY, "wt_ctx_init: cannot allocate memory\n" );
		return NULL;
	}

	memset(wc, 0, sizeof(wt_ctx));

	rc = wi->wi_conn->open_session(wi->wi_conn, NULL, NULL, &wc->session);
	if( rc ) {
		Debug( LDAP_DEBUG_ANY, "wt_ctx_init: open_session error %s(%d)\n",
			   wiredtiger_strerror(rc), rc );
		return NULL;
	}

	/* readonly mode */
	if (!wi->wi_cache) {
		return wc;
	}

	rc = wi->wi_cache->open_session(wi->wi_cache, NULL, NULL, &wc->idlcache_session);
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_ctx_init: cannot open idlcache session %s(%d)\n",
			   wiredtiger_strerror(rc), rc );
		return NULL;
	}

	return wc;
}

void
wt_ctx_free( void *key, void *data )
{
	wt_ctx *wc = data;

	if(wc->session){
		/*
		 * The session will close automatically when db closing.
		 * We can close session here, but it's require to check db
		 * status, otherwise it will cause SEGV.
		 */
		/*
		if(IS_DB_OPEN) {
		    wc->session->close(wc->session, NULL);
		}
		*/
		wc->session = NULL;
	}

	ch_free(wc);
}

wt_ctx *
wt_ctx_get(Operation *op, struct wt_info *wi){
	int rc;
	void *data;
	wt_ctx *wc = NULL;

	rc = ldap_pvt_thread_pool_getkey(op->o_threadctx,
									 wi, &data, NULL );
	if( rc ){
		wc = wt_ctx_init(wi);
		if( !wc ) {
			Debug( LDAP_DEBUG_ANY, "wt_ctx: wt_ctx_init failed\n" );
			return NULL;
		}
		rc = ldap_pvt_thread_pool_setkey( op->o_threadctx,
										  wi, wc, wt_ctx_free,
										  NULL, NULL );
		if( rc ) {
			Debug( LDAP_DEBUG_ANY, "wt_ctx: setkey error(%d)\n",
				   rc );
			return NULL;
		}
		return wc;
	}
	return (wt_ctx *)data;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
