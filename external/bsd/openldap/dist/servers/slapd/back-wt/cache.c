/*	$NetBSD: cache.c,v 1.2 2025/09/05 21:16:31 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: cache.c,v 1.2 2025/09/05 21:16:31 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "slap-config.h"
#include "idl.h"

int wt_idlcache_get(wt_ctx *wc, struct berval *ndn, int scope, ID *ids)
{
	int rc = 0;
	WT_ITEM item;
	WT_SESSION *session = wc->idlcache_session;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_get(\"%s\", %d)\n",
		   ndn->bv_val, scope );

	rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
							  NULL, &cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_get: open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}
	cursor->set_key(cursor, ndn->bv_val, (int8_t)scope);
	rc = cursor->search(cursor);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		Debug(LDAP_DEBUG_TRACE, "<= wt_idlcache_get: miss\n" );
		goto done;
	default:
		Debug( LDAP_DEBUG_ANY, "<= wt_idlcache_get: search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		rc = 0;
		goto done;
	}
	rc = cursor->get_value(cursor, &item);
	if (rc) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_get: get_value failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}
	if (item.size == 0) {
		Debug(LDAP_DEBUG_TRACE, "<= wt_idlcache_get: updating\n");
		rc = WT_NOTFOUND;
		goto done;
	}
	memcpy(ids, item.data, item.size);

	Debug(LDAP_DEBUG_TRACE,
		  "<= wt_idlcache_get: hit id=%ld first=%ld last=%ld\n",
		  (long)ids[0],
		  (long)WT_IDL_FIRST(ids),
		  (long)WT_IDL_LAST(ids));
done:
	if(cursor) {
		cursor->close(cursor);
	}
	return rc;
}

int wt_idlcache_set(wt_ctx *wc, struct berval *ndn, int scope, ID *ids)
{
	int rc = 0;
	WT_ITEM item;
	WT_SESSION *session = wc->idlcache_session;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_set(\"%s\", %d)\n",
		   ndn->bv_val, scope );

	item.size = WT_IDL_SIZEOF(ids);
	item.data = ids;

	rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
							  "overwrite=false", &cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_set: open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}
	cursor->set_key(cursor, ndn->bv_val, (int8_t)scope);
	cursor->set_value(cursor, &item);
	rc = cursor->update(cursor);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		// updating cache by another thread
		goto done;
	default:
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_set: update failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	Debug(LDAP_DEBUG_TRACE,
		  "<= wt_idlcache_set: set idl size=%ld\n",
		  (long)ids[0]);
done:
	if(cursor) {
		cursor->close(cursor);
	}
	return rc;
}

int wt_idlcache_begin(wt_ctx *wc, struct berval *ndn, int scope)
{
	int rc = 0;
	WT_ITEM item;
	WT_SESSION *session = wc->idlcache_session;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_begin(\"%s\", %d)\n",
		   ndn->bv_val, scope );

	item.size = 0;
	item.data = "";

	rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
							  "overwrite=true", &cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_begin: open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}
	cursor->set_key(cursor, ndn->bv_val, (int8_t)scope);
	cursor->set_value(cursor, &item);
	rc = cursor->update(cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_begin: update failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	Debug(LDAP_DEBUG_TRACE,
		  "<= wt_idlcache_begin: set updating\n" );

done:
	if(cursor) {
		cursor->close(cursor);
	}
	return rc;
}

int wt_idlcache_clear(Operation *op, wt_ctx *wc, struct berval *ndn)
{
	BackendDB *be = op->o_bd;
	int rc = 0;
	struct berval pdn = *ndn;
	WT_SESSION *session = wc->idlcache_session;
	WT_CURSOR *cursor = NULL;
	int level = 0;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_clear(\"%s\")\n",
		   ndn->bv_val );

	if (be_issuffix( be, ndn )) {
		return 0;
	}

	rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
							  NULL, &cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_clear: open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}

	do {
		dnParent( &pdn, &pdn );
		if (level == 0) {
			/* clear only parent level cache */
			cursor->set_key(cursor, pdn.bv_val, (int8_t)LDAP_SCOPE_ONE);
			cursor->remove(cursor);
		}
		cursor->set_key(cursor, pdn.bv_val, (int8_t)LDAP_SCOPE_SUB);
		cursor->remove(cursor);
		cursor->set_key(cursor, pdn.bv_val, (int8_t)LDAP_SCOPE_CHILDREN);
		cursor->remove(cursor);
		level++;
	}while(!be_issuffix( be, &pdn ));

	if(cursor) {
		cursor->close(cursor);
	}
	return 0;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
