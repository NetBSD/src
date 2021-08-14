/*	$NetBSD: ctx.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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

wt_ctx *
wt_ctx_init(struct wt_info *wi)
{
	int rc;
	wt_ctx *wc;

	wc = ch_malloc( sizeof( wt_ctx ) );
	if( !wc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_ctx_init)
			   ": cannot allocate memory\n" );
		return NULL;
	}

	memset(wc, 0, sizeof(wt_ctx));

	if(!wc->session){
		rc = wi->wi_conn->open_session(wi->wi_conn, NULL, NULL, &wc->session);
		if( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   LDAP_XSTRING(wt_ctx_session)
				   ": open_session error %s(%d)\n",
				   wiredtiger_strerror(rc), rc );
			return NULL;
		}
	}
	return wc;
}

void
wt_ctx_free( void *key, void *data )
{
	wt_ctx *wc = data;

	if(wc->session){
		wc->session->close(wc->session, NULL);
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
									 wt_ctx_get, &data, NULL );
	if( rc ){
		wc = wt_ctx_init(wi);
		if( !wc ) {
			Debug( LDAP_DEBUG_ANY,
				   LDAP_XSTRING(wt_ctx)
				   ": wt_ctx_init failed\n" );
			return NULL;
		}
		rc = ldap_pvt_thread_pool_setkey( op->o_threadctx,
										  wt_ctx_get, wc, wt_ctx_free,
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

WT_CURSOR *
wt_ctx_index_cursor(wt_ctx *wc, struct berval *name, int create)
{
	WT_CURSOR *cursor = NULL;
	WT_SESSION *session = wc->session;
	char tablename[1024];
	int rc;

	snprintf(tablename, sizeof(tablename), "table:%s", name->bv_val);

	rc = session->open_cursor(session, tablename, NULL,
							  "overwrite=false", &cursor);
	if (rc == ENOENT && create) {
		rc = session->create(session,
							 tablename,
							 "key_format=uQ,"
							 "value_format=x,"
							 "columns=(key, id, none)");
		if( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   LDAP_XSTRING(indexer) ": table \"%s\": "
				   "cannot create idnex table: %s (%d)\n",
				   tablename, wiredtiger_strerror(rc), rc);
			return NULL;
		}
		rc = session->open_cursor(session, tablename, NULL,
								  "overwrite=false", &cursor);
	}
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_id2entry_put)
			   ": open cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return NULL;
	}

	return cursor;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
