/*	$NetBSD: bind.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: bind.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "back-wt.h"
#include "slap-config.h"

int
wt_bind( Operation *op, SlapReply *rs )
{
    struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	WT_SESSION *session;
	wt_ctx *wc;
	int rc;
	Entry *e = NULL;
	Attribute *a;
	AttributeDescription *password = slap_schema.si_ad_userPassword;

    Debug( LDAP_DEBUG_ARGS,
		   "==> " LDAP_XSTRING(wt_bind) ": dn: %s\n",
		   op->o_req_dn.bv_val );

	/* allow noauth binds */
	switch ( be_rootdn_bind( op, NULL ) ) {
	case LDAP_SUCCESS:
        /* frontend will send result */
        return rs->sr_err = LDAP_SUCCESS;

    default:
        /* give the database a chance */
        /* NOTE: this behavior departs from that of other backends,
         * since the others, in case of password checking failure
         * do not give the database a chance.  If an entry with
         * rootdn's name does not exist in the database the result
         * will be the same.  See ITS#4962 for discussion. */
        break;
	}

	wc = wt_ctx_get(op, wi);
	if( !wc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_bind)
			   ": wt_ctx_get failed\n" );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
        send_ldap_result( op, rs );
        return rs->sr_err;
	}

	/* get entry */
	rc = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
	switch( rc ) {
	case 0:
		break;
	case WT_NOTFOUND:
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		send_ldap_result( op, rs );
		return rs->sr_err;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
        send_ldap_result( op, rs );
        return rs->sr_err;
	}

	ber_dupbv( &op->oq_bind.rb_edn, &e->e_name );

    /* check for deleted */
	if ( is_entry_subentry( e ) ) {
        /* entry is an subentry, don't allow bind */
		Debug( LDAP_DEBUG_TRACE, "entry is subentry\n" );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	}

	if ( is_entry_alias( e ) ) {
        /* entry is an alias, don't allow bind */
		Debug( LDAP_DEBUG_TRACE, "entry is alias\n" );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	}

	if ( is_entry_referral( e ) ) {
		Debug( LDAP_DEBUG_TRACE, "entry is referral\n" );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	}

	switch ( op->oq_bind.rb_method ) {
	case LDAP_AUTH_SIMPLE:
		a = attr_find( e->e_attrs, password );
		if ( a == NULL ) {
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			goto done;
		}

		if ( slap_passwd_check( op, e, a, &op->oq_bind.rb_cred,
								&rs->sr_text ) != 0 )
		{
            /* failure; stop front end from sending result */
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			goto done;
		}
		rs->sr_err = 0;
		break;

    default:
		rs->sr_err = LDAP_STRONG_AUTH_NOT_SUPPORTED;
		rs->sr_text = "authentication method not supported";
	}

done:
	/* free entry */
	if (e) {
		wt_entry_return(e);
	}
	if (rs->sr_err) {
		send_ldap_result( op, rs );
        if ( rs->sr_ref ) {
            ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;
		}
	}
	return rs->sr_err;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
