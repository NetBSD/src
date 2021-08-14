/*	$NetBSD: dn2id.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: dn2id.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"
#include "idl.h"

char *
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
		AC_MEMCPY( p, rdn, rdn_len );
		p += rdn_len;
		*p++ = ',';
	}
	*p = '\0';
	return dst;
}

int
wt_dn2id_add(
	Operation *op,
	WT_SESSION *session,
	ID pid,
	Entry *e)
{
	int rc;
	WT_CURSOR *cursor = NULL;
	char *revdn = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id_add 0x%lx: \"%s\"\n",
		   e->e_id, e->e_ndn );
	assert( e->e_id != NOID );

	/* make reverse dn */
	revdn = mkrevdn(e->e_nname);

	rc = session->open_cursor(session, WT_TABLE_DN2ID, NULL,
							  NULL, &cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_add)
			   ": open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
    }
	cursor->set_key(cursor, e->e_ndn);
	cursor->set_value(cursor, e->e_id, pid, revdn);
	rc = cursor->insert(cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_add)
			   ": insert failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
    }

done:
	if(revdn){
		ch_free(revdn);
	}
	if(cursor){
		cursor->close(cursor);
	}
	Debug( LDAP_DEBUG_TRACE, "<= wt_dn2id_add 0x%lx: %d\n", e->e_id, rc );
	return rc;
}

int
wt_dn2id_delete(
	Operation *op,
	WT_SESSION *session,
	struct berval *ndn)
{
	int rc = 0;
	WT_CURSOR *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id_delete %s\n", ndn->bv_val );

	rc = session->open_cursor(session, WT_TABLE_DN2ID, NULL,
							  NULL, &cursor);
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_delete)
			   ": open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	cursor->set_key(cursor, ndn->bv_val);
	rc = cursor->remove(cursor);
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_delete)
			   ": remove failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	Debug( LDAP_DEBUG_TRACE,
		   "<= wt_dn2id_delete %s: %d\n",
		   ndn->bv_val, rc );
done:
	if(cursor){
		cursor->close(cursor);
	}
	return rc;
}

int
wt_dn2id(
	Operation *op,
	WT_SESSION *session,
    struct berval *ndn,
    ID *id)
{
	WT_CURSOR *cursor = NULL;
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	int rc;
	ID nid;

	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id(\"%s\")\n",
		   ndn->bv_val );

	if ( ndn->bv_len == 0 ) {
		*id = 0;
		goto done;
	}

	rc = session->open_cursor(session, WT_TABLE_DN2ID
							  "(id)",
                              NULL, NULL, &cursor);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id)
			   ": cursor open failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
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
			   LDAP_XSTRING(wt_dn2id)
			   ": search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}
	rc = cursor->get_value(cursor, id);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id)
			   ": get_value failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

done:
	if(cursor){
		cursor->close(cursor);
	}

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
	WT_SESSION *session,
	ID id )
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	WT_CURSOR *cursor = NULL;
	int rc;
	uint64_t key = id;

	rc = session->open_cursor(session, WT_INDEX_PID,
                              NULL, NULL, &cursor);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_has_children)
			   ": cursor open failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	cursor->set_key(cursor, key);
	rc = cursor->search(cursor);

done:
	if(cursor){
		cursor->close(cursor);
	}

	return rc;
}

int
wt_dn2idl(
	Operation *op,
	WT_SESSION *session,
	struct berval *ndn,
	Entry *e,
	ID *ids,
	ID *stack)
{
	struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	WT_CURSOR *cursor = NULL;
	int exact = 0;
	int rc;
	char *revdn = NULL;
	size_t revdn_len;
	char *key;
	ID id, pid;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_dn2idl(\"%s\")\n",
		   ndn->bv_val );

	if(op->ors_scope != LDAP_SCOPE_ONELEVEL &&
	   be_issuffix( op->o_bd, &e->e_nname )){
		WT_IDL_ALL(wi, ids);
		return 0;
	}

	revdn = mkrevdn(*ndn);
	revdn_len = strlen(revdn);
	rc = session->open_cursor(session, WT_INDEX_REVDN"(id, pid)",
                              NULL, NULL, &cursor);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2idl)
			   ": cursor open failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}
	cursor->set_key(cursor, revdn);
	rc = cursor->search_near(cursor, &exact);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2idl)
			   ": search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	do {
		rc = cursor->get_key(cursor, &key);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   LDAP_XSTRING(wt_dn2idl)
				   ": get_key failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}

		if( strncmp(revdn, key, revdn_len) ){
			if(exact < 0){
				rc = cursor->next(cursor);
				if (rc) {
					break;
				}else{
					continue;
				}
			}
			break;
		}
		exact = 0;
		rc = cursor->get_value(cursor, &id, &pid);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   LDAP_XSTRING(wt_dn2id)
				   ": get_value failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			goto done;
		}
		if( op->ors_scope == LDAP_SCOPE_ONELEVEL &&
			e->e_id != pid){
			rc = cursor->next(cursor);
			if ( rc ) {
				break;
			}
			continue;
		}else{
			wt_idl_append_one(ids, id);
		}
		rc = cursor->next(cursor);
	}while(rc == 0);

	if (rc == WT_NOTFOUND ) {
		rc = LDAP_SUCCESS;
	}

done:
	if(revdn){
		ch_free(revdn);
	}
	if(cursor){
		cursor->close(cursor);
	}
	return rc;
}

#if 0
int
wt_dn2id(
	Operation *op,
	WT_SESSION *session,
    struct berval *dn,
    ID *id)
{
	struct wt_info *wi = (struct wy_info *) op->o_bd->be_private;
	WT_CURSOR *cursor = NULL;
	int rc;
	Debug( LDAP_DEBUG_TRACE, "=> wt_dn2id(\"%s\")\n", dn->bv_val );

	rc = session->open_cursor(session, WT_INDEX_DN"(id)",
                              NULL, NULL, &cursor);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id)
			   ": cursor open failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}
	cursor->set_key(cursor, dn->bv_val);
	rc = cursor->search(cursor);
	if( !rc ){
		cursor->get_key(cursor, &id);
	}
	cursor->close(cursor);
	return rc;
}
#endif

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
