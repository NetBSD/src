/*	$NetBSD: map.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* map.c - ldap backend mapping routines */
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

/* This is an altered version */
/*
 * Copyright 1999, Howard Chu, All rights reserved. <hyc@highlandsun.com>
 *
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 *
 * 4. This notice may not be removed or altered.
 *
 *
 *
 * Copyright 2016, Symas Corporation
 *
 * This is based on the back-meta/map.c version by Pierangelo Masarati.
 * The previously reported conditions apply to the modified code as well.
 * Changes in the original code are highlighted where required.
 * Credits for the original code go to the author, Howard Chu.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: map.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "lutil.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

void
asyncmeta_referral_result_rewrite(
	a_dncookie		*dc,
	BerVarray		a_vals
)
{
	int		i, last;

	assert( dc != NULL );
	assert( a_vals != NULL );

	for ( last = 0; !BER_BVISNULL( &a_vals[ last ] ); last++ )
		;
	last--;

	for ( i = 0; !BER_BVISNULL( &a_vals[ i ] ); i++ ) {
		struct berval	dn,
				olddn = BER_BVNULL;
		int		rc;
		LDAPURLDesc	*ludp;

		rc = ldap_url_parse( a_vals[ i ].bv_val, &ludp );
		if ( rc != LDAP_URL_SUCCESS ) {
			/* leave attr untouched if massage failed */
			continue;
		}

		/* FIXME: URLs like "ldap:///dc=suffix" if passed
		 * thru ldap_url_parse() and ldap_url_desc2str()
		 * get rewritten as "ldap:///dc=suffix??base";
		 * we don't want this to occur... */
		if ( ludp->lud_scope == LDAP_SCOPE_BASE ) {
			ludp->lud_scope = LDAP_SCOPE_DEFAULT;
		}

		ber_str2bv( ludp->lud_dn, 0, 0, &olddn );

		asyncmeta_dn_massage( dc, &olddn, &dn );
		/* leave attr untouched if massage did nothing */
		if ( olddn.bv_val != dn.bv_val )
		{
			char	*newurl;

			ludp->lud_dn = dn.bv_val;
			newurl = ldap_url_desc2str( ludp );
			dc->op->o_tmpfree( dn.bv_val, dc->memctx );
			if ( newurl )
			{
				/* FIXME: leave attr untouched
				 * even if ldap_url_desc2str failed...
				 */

				ber_memfree_x( a_vals[ i ].bv_val, dc->op->o_tmpmemctx );
				ber_str2bv_x( newurl, 0, 1, &a_vals[ i ], dc->memctx );
				ber_memfree( newurl );
				ludp->lud_dn = olddn.bv_val;
			}
		}
		ldap_free_urldesc( ludp );
	}
}

void
asyncmeta_dnattr_result_rewrite(
	a_dncookie		*dc,
	BerVarray		a_vals
)
{
	struct berval	bv;
	int		i;

	assert( a_vals != NULL );

	for ( i = 0; !BER_BVISNULL( &a_vals[i] ); i++ ) {
		asyncmeta_dn_massage( dc, &a_vals[i], &bv );
		if ( bv.bv_val != a_vals[i].bv_val ) {
			ber_memfree_x( a_vals[i].bv_val, dc->memctx );
			a_vals[i] = bv;
		}
	}
}

/*
 * asyncmeta_dn_massage
 *
 * Aliases the suffix.
 */
void
asyncmeta_dn_massage(
	a_dncookie *dc,
	struct berval *odn,
	struct berval *res
)
{
	struct berval pretty = {0,NULL}, *dn = odn;
	struct berval *osuff, *nsuff;
	int diff;

	assert( res );

	BER_BVZERO(res);
	if ( dn == NULL )
		return;

	/* no suffix massage configured */
	if ( !dc->target->mt_lsuffixm.bv_val ) {
		*res = *dn;
		return;
	}

	if ( dc->to_from == MASSAGE_REQ ) {
		osuff = &dc->target->mt_lsuffixm;
		nsuff = &dc->target->mt_rsuffixm;
	} else {
		osuff = &dc->target->mt_rsuffixm;
		nsuff = &dc->target->mt_lsuffixm;
		/* DN from remote server may be in arbitrary form.
		 * Pretty it so we can parse reliably.
		 */
		dnPretty( NULL, dn, &pretty, dc->op->o_tmpmemctx );
		if (pretty.bv_val) dn = &pretty;
	}

	diff = dn->bv_len - osuff->bv_len;
	/* DN is shorter than suffix - ignore */
	if ( diff < 0 ) {
ignore:
		*res = *odn;
		if (pretty.bv_val)
			dc->op->o_tmpfree( pretty.bv_val, dc->op->o_tmpmemctx );
		return;
	}

	/* DN longer than our suffix and doesn't match */
	if ( diff > 0 && !DN_SEPARATOR(dn->bv_val[diff-1]))
		goto ignore;

	/* suffix is same length as ours, but doesn't match */
	if ( strcasecmp( osuff->bv_val, &dn->bv_val[diff] ))
		goto ignore;

	res->bv_len = diff + nsuff->bv_len;
	res->bv_val = dc->op->o_tmpalloc( res->bv_len + 1, dc->memctx );
	strncpy( res->bv_val, dn->bv_val, diff );
	strcpy( &res->bv_val[diff], nsuff->bv_val );

	if (pretty.bv_val)
		dc->op->o_tmpfree( pretty.bv_val, dc->op->o_tmpmemctx );
}
