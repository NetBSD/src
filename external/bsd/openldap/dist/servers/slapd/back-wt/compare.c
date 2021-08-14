/*	$NetBSD: compare.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: compare.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "back-wt.h"
#include "slap-config.h"

int
wt_compare( Operation *op, SlapReply *rs )
{
    struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	Entry *e = NULL;
	int manageDSAit = get_manageDSAit( op );
	int rc;
	wt_ctx *wc = NULL;

	Debug( LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(wt_compare) ": %s\n",
		   op->o_req_dn.bv_val );

	wc = wt_ctx_get(op, wi);
	if( !wc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_compare)
			   ": wt_ctx_get failed\n" );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
        send_ldap_result( op, rs );
        return rs->sr_err;
	}

	rs->sr_err = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
	switch( rs->sr_err ) {
	case 0:
	case WT_NOTFOUND:
		break;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	if ( rs->sr_err == WT_NOTFOUND ) {
		if ( e != NULL ) {
			/* return referral only if "disclose" is granted on the object */
			if ( ! access_allowed( op, e, slap_schema.si_ad_entry,
								   NULL, ACL_DISCLOSE, NULL ) )
			{
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			} else {
				rs->sr_matched = ch_strdup( e->e_dn );
				if ( is_entry_referral( e )) {
					BerVarray ref = get_entry_referrals( op, e );
					rs->sr_ref = referral_rewrite( ref,
												   &e->e_name,
												   &op->o_req_dn,
												   LDAP_SCOPE_DEFAULT );
					ber_bvarray_free( ref );
				} else {
					rs->sr_ref = NULL;
				}
				rs->sr_err = LDAP_REFERRAL;
			}
			wt_entry_return( e );
			e = NULL;
		} else {
			rs->sr_ref = referral_rewrite( default_referral,
										   NULL,
										   &op->o_req_dn,
										   LDAP_SCOPE_DEFAULT );
			rs->sr_err = rs->sr_ref ? LDAP_REFERRAL : LDAP_NO_SUCH_OBJECT;
		}

		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		send_ldap_result( op, rs );
		goto done;
	}

	if (!manageDSAit && is_entry_referral( e ) ) {
		/* return referral only if "disclose" is granted on the object */
		if ( !access_allowed( op, e, slap_schema.si_ad_entry,
							  NULL, ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			/* entry is a referral, don't allow compare */
			rs->sr_ref = get_entry_referrals( op, e );
			rs->sr_err = LDAP_REFERRAL;
			rs->sr_matched = e->e_name.bv_val;
		}

		Debug( LDAP_DEBUG_TRACE, "entry is referral\n" );

		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;
		goto done;
	}

	rs->sr_err = slap_compare_entry( op, e, op->orc_ava );

return_results:
	send_ldap_result( op, rs );

	switch ( rs->sr_err ) {
	case LDAP_COMPARE_FALSE:
	case LDAP_COMPARE_TRUE:
		rs->sr_err = LDAP_SUCCESS;
		break;
	}

done:
	if ( e != NULL ) {
		wt_entry_return( e );
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
