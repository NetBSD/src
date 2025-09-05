/*	$NetBSD: dn2id.c,v 1.3 2025/09/05 21:16:31 christos Exp $	*/

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
__RCSID("$NetBSD: dn2id.c,v 1.3 2025/09/05 21:16:31 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"
#include "idl.h"

static char *
mkrevdn(struct berval src){
	char *dst, *p;
	char *rdn;
	size_t rdn_len;

	p = dst = ch_malloc(src.bv_len + 2);
	while(src.bv_len){
		rdn = ber_bvrchr( &src, ',' );
		if (rdn) {
			rdn_len = src.bv_len;
			src.bv_len = rdn - src.bv_val;
			rdn_len -= src.bv_len + 1;
			rdn++;
		}else{
			/* first rdn */
			rdn_len = src.bv_len;
			rdn = src.bv_val;
			src.bv_len = 0;
		}
		memcpy( p, rdn, rdn_len );
		p += rdn_len;
		*p++ = ',';
	}
	*p = '\0';
	return dst;
}

int
wt_dn2id_add(
	Operation *op,
	wt_ctx *wc,
	ID pid,
	Entry *e)
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	int rc;
	WT_SESSION *session = wc->session;
	WT_CURSOR *cursor = wc->dn2id_w;
	char *revdn = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id_add 0x%lx: \"%s\"\n",
		   e->e_id, e->e_ndn );
	assert( e->e_id != NOID );

	/* make reverse dn */
	revdn = mkrevdn(e->e_nname);

	if(!cursor){
		rc = session->open_cursor(session, WT_TABLE_DN2ID, NULL,
								  "overwrite=false", &cursor);
		if(rc){
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2id_add: open_cursor failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		wc->dn2id_w = cursor;
	}
	cursor->set_key(cursor, revdn);
	cursor->set_value(cursor, e->e_ndn, e->e_id, pid);
	rc = cursor->insert(cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_dn2id_add: insert failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
    }

	if (wi->wi_flags & WT_USE_IDLCACHE) {
		wt_idlcache_clear(op, wc, &e->e_nname);
	}

done:
	if(revdn){
		ch_free(revdn);
	}

#ifdef WT_CURSOR_CACHE
	if(cursor){
		cursor->reset(cursor);
	}
#else
	if(cursor){
		cursor->close(cursor);
		wc->dn2id_w = NULL;
	}
#endif

	Debug( LDAP_DEBUG_TRACE, "<= wt_dn2id_add 0x%lx: %d\n", e->e_id, rc );
	return rc;
}

int
wt_dn2id_delete(
	Operation *op,
	wt_ctx *wc,
	struct berval *ndn)
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	int rc = 0;
	WT_SESSION *session = wc->session;
	WT_CURSOR *cursor = wc->dn2id_w;
	char *revdn = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id_delete %s\n", ndn->bv_val );

	/* make reverse dn */
	revdn = mkrevdn(*ndn);

	if(!cursor){
		rc = session->open_cursor(session, WT_TABLE_DN2ID, NULL,
								  "overwrite=false", &cursor);
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2id_delete: open_cursor failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		wc->dn2id_w = cursor;
	}

	cursor->set_key(cursor, revdn);
	rc = cursor->remove(cursor);
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_dn2id_delete: remove failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	if (wi->wi_flags & WT_USE_IDLCACHE) {
		wt_idlcache_clear(op, wc, ndn);
	}

	Debug( LDAP_DEBUG_TRACE,
		   "<= wt_dn2id_delete %s: %d\n", ndn->bv_val, rc );
done:
	if(revdn){
		ch_free(revdn);
	}

#ifdef WT_CURSOR_CACHE
	if(cursor){
		cursor->reset(cursor);
	}
#else
	if(cursor){
		cursor->close(cursor);
		wc->dn2id_w = NULL;
	}
#endif
	return rc;
}

int
wt_dn2id(
	Operation *op,
	wt_ctx *wc,
    struct berval *ndn,
    ID *id)
{
	WT_SESSION *session = wc->session;
	WT_CURSOR *cursor = wc->dn2id_ndn;
	int rc = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id(\"%s\")\n", ndn->bv_val );

	if ( ndn->bv_len == 0 ) {
		*id = 0;
		goto done;
	}

	if(!cursor){
		rc = session->open_cursor(session, WT_INDEX_NDN
								  "(id)",
								  NULL, NULL, &cursor);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2id: cursor open failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		wc->dn2id_ndn = cursor;
	}

	cursor->set_key(cursor, ndn->bv_val);
	rc = cursor->search(cursor);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		goto done;
	default:
		Debug( LDAP_DEBUG_ANY,
			   "wt_dn2id: search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}
	rc = cursor->get_value(cursor, id);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   "wt_dn2id: get_value failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

done:

#ifdef WT_CURSOR_CACHE
	if(cursor){
		cursor->reset(cursor);
	}
#else
	if(cursor){
		cursor->close(cursor);
		wc->dn2id_ndn = NULL;
	}
#endif

	if( rc ) {
		Debug( LDAP_DEBUG_TRACE, "<= wt_dn2id: get failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
	} else {
		Debug( LDAP_DEBUG_TRACE, "<= wt_dn2id: got id=0x%lx\n",
			   *id );
	}

	return rc;
}

int
wt_dn2id_has_children(
	Operation *op,
	wt_ctx *wc,
	ID id )
{
	WT_SESSION *session = wc->session;
	WT_CURSOR *cursor = wc->index_pid;
	int rc;
	uint64_t key = id;

	if(!cursor){
		rc = session->open_cursor(session, WT_INDEX_PID,
								  NULL, NULL, &cursor);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2id_has_children: cursor open failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		wc->index_pid = cursor;
	}

	cursor->set_key(cursor, key);
	rc = cursor->search(cursor);

done:

#ifdef WT_CURSOR_CACHE
	if(cursor){
		cursor->reset(cursor);
	}
#else
	if(cursor){
		cursor->close(cursor);
		wc->index_pid = NULL;
	}
#endif

	return rc;
}

int
wt_dn2idl_db(
	Operation *op,
	wt_ctx *wc,
	struct berval *ndn,
	Entry *e,
	ID *ids,
	ID *stack)
{
	WT_SESSION *session = wc->session;
	WT_CURSOR *cursor = wc->dn2id;
	int rc;
	char *revdn = NULL;
	size_t revdn_len;
	char *key;
	ID id, pid;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_dn2idl(\"%s\")\n",
		   ndn->bv_val );

	revdn = mkrevdn(*ndn);
	revdn_len = strlen(revdn);

	if ( !cursor ) {
		rc = session->open_cursor(session, WT_TABLE_DN2ID"(id, pid)",
								  NULL, NULL, &cursor);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2idl: cursor open failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		wc->dn2id = cursor;
	}
	cursor->set_key(cursor, revdn);
	rc = cursor->search(cursor);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   "wt_dn2idl: search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	if( op->ors_scope == LDAP_SCOPE_CHILDREN ) {
		cursor->next(cursor);
	}

	do {
		rc = cursor->get_key(cursor, &key);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2idl: get_key failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		rc = cursor->get_value(cursor, &id, &pid);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   "wt_dn2id: get_value failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}

		if( strncmp(revdn, key, revdn_len) ){
			break;
		}

		if( op->ors_scope == LDAP_SCOPE_ONELEVEL && e->e_id != pid ){
			goto next;
		}
		wt_idl_append_one(ids, id);
	next:
		rc = cursor->next(cursor);
	}while(rc == 0);

	if (rc == WT_NOTFOUND ) {
		rc = LDAP_SUCCESS;
	}

	wt_idl_sort(ids, stack);
	Debug( LDAP_DEBUG_TRACE,
		   "<= wt_dn2idl_db: size=%ld first=%ld last=%ld\n",
		   (long) ids[0],
		   (long) WT_IDL_FIRST(ids),
		   (long) WT_IDL_LAST(ids) );

done:
	if(revdn){
		ch_free(revdn);
	}
#ifdef WT_CURSOR_CACHE
	if(cursor){
		cursor->reset(cursor);
	}
#else
	if(cursor){
		cursor->close(cursor);
		wc->dn2id = NULL;
	}
#endif
	return rc;
}

int
wt_dn2idl(
	Operation *op,
	wt_ctx *wc,
	struct berval *ndn,
	Entry *e,
	ID *ids,
	ID *stack)
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	int rc;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_dn2idl(\"%s\")\n", ndn->bv_val );

	if(op->ors_scope != LDAP_SCOPE_ONELEVEL &&
	   be_issuffix( op->o_bd, &e->e_nname )){
		WT_IDL_ALL(wi, ids);
		return 0;
	}

	if (wi->wi_flags & WT_USE_IDLCACHE) {
		rc = wt_idlcache_get(wc, ndn, op->ors_scope, ids);
		if (rc == 0) {
			/* cache hit */
			return rc;
		}
		/* cache miss */
	}

	if ( wi->wi_flags & WT_USE_IDLCACHE ) {
		wt_idlcache_begin(wc, ndn, op->ors_scope);
	}
	rc = wt_dn2idl_db(op, wc, ndn, e, ids, stack);
	if ( rc == 0 && wi->wi_flags & WT_USE_IDLCACHE ) {
		wt_idlcache_set(wc, ndn, op->ors_scope, ids);
	}

	return rc;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
