/*	$NetBSD: key.c,v 1.3 2025/09/05 21:16:32 christos Exp $	*/

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
__RCSID("$NetBSD: key.c,v 1.3 2025/09/05 21:16:32 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "slap-config.h"
#include "idl.h"

/* read a key */
int
wt_key_read(
	Backend *be,
	WT_CURSOR *cursor,
	struct berval *bkey,
	ID *ids,
	WT_CURSOR **saved_cursor,
	int get_flag
	)
{
	int rc;
	WT_ITEM key;
	int exact;
	WT_ITEM key2;
	ID id;
	int comp;
	long scanned = 0;

	Debug( LDAP_DEBUG_TRACE, "=> key_read\n" );

	WT_IDL_ZERO(ids);
	bv2ITEM(bkey, &key);
	cursor->set_key(cursor, &key, 0);
	rc = cursor->search_near(cursor, &exact);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		rc = LDAP_SUCCESS;
		goto done;
	default:
		Debug( LDAP_DEBUG_ANY,
			   "wt_key_read: search_near failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc);
		goto done;
	}
	do {
		scanned++;
		rc = cursor->get_key(cursor, &key2, &id);
		if( rc ){
			Debug( LDAP_DEBUG_ANY,
				   "wt_key_read: get_key failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			break;
		}
		comp = 0;
		if (key.size != key2.size ||
			(comp = memcmp(key2.data, key.data, key.size))) {
			if(comp > 0){
				break;
			}
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
		wt_idl_append_one(ids, id);
		rc = cursor->next(cursor);
	} while(rc == 0);

	if ( rc == WT_NOTFOUND && exact == 0 ) {
		rc = LDAP_SUCCESS;
	}

done:
	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "<= wt_key_read: failed (%d) %ld scanned\n",
			   rc, scanned );
	} else {
		Debug( LDAP_DEBUG_TRACE, "<= wt_key_read %ld candidates %ld scanned\n",
			   (long) WT_IDL_N(ids), scanned );
	}

	return rc;
}

/* Add or remove stuff from index files */
int
wt_key_change(
	Backend *be,
	WT_CURSOR *cursor,
	struct berval *k,
	ID id,
	int op
)
{
	int	rc;
	WT_ITEM item;

	Debug( LDAP_DEBUG_TRACE, "=> key_change(%s,%lx)\n",
		   op == SLAP_INDEX_ADD_OP ? "ADD":"DELETE", (long) id );

	bv2ITEM(k, &item);
	cursor->set_key(cursor, &item, id);
	cursor->set_value(cursor, NULL);

	if (op == SLAP_INDEX_ADD_OP) {
		/* Add values */
		rc = cursor->insert(cursor);
		if ( rc == WT_DUPLICATE_KEY ) rc = 0;
	} else {
		/* Delete values */
		rc = cursor->remove(cursor);
		if ( rc == WT_NOTFOUND ) rc = 0;
	}
	if( rc ) {
		if ( rc != WT_ROLLBACK ) {
			Debug( LDAP_DEBUG_ANY,
				   "wt_key_change: error: %s (%d)\n",
				   wiredtiger_strerror(rc), rc);
		}
		return rc;
	}

	Debug( LDAP_DEBUG_TRACE, "<= key_change %d\n", rc );

	return rc;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
