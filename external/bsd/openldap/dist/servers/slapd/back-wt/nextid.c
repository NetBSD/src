/*	$NetBSD: nextid.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: nextid.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "slap-config.h"

int wt_next_id(BackendDB *be, ID *out){
    struct wt_info *wi = (struct wt_info *) be->be_private;
	*out = __sync_add_and_fetch(&wi->wi_lastid, 1);
	return 0;
}

int wt_last_id( BackendDB *be, WT_SESSION *session, ID *out )
{
    WT_CURSOR *cursor;
    int rc;
    uint64_t id;

    rc = session->open_cursor(session, WT_TABLE_ID2ENTRY, NULL, NULL, &cursor);
    if(rc){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_last_id)
			   ": open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
    }

    rc = cursor->prev(cursor);
	switch(rc) {
	case 0:
		rc = cursor->get_key(cursor, &id);
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   LDAP_XSTRING(wt_last_id)
				   ": get_key failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			return rc;
		}
		*out = id;
		break;
	case WT_NOTFOUND:
        /* no entry */
        *out = 0;
		break;
	default:
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_last_id)
			   ": prev failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
    }

    rc = cursor->close(cursor);
    if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_last_id)
			   ": close failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
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
