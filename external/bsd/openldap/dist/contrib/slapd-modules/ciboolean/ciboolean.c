/*	$NetBSD: ciboolean.c,v 1.2 2025/09/05 21:16:15 christos Exp $	*/

/* ciboolean.c - enable case-insensitive boolean values */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2024 The OpenLDAP Foundation.
 * Copyright 2022 Symas Corp. All Rights Reserved.
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
 * This work was developed in 2022 by Nadezhda Ivanova for Symas Corp.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ciboolean.c,v 1.2 2025/09/05 21:16:15 christos Exp $");

#include "portable.h"

#ifdef SLAPD_MOD_CIBOOLEAN

#include "slap.h"
#include "ac/ctype.h"

static int
cibooleanValidate(
	Syntax *syntax,
	struct berval *in )
{
	/* Allow for case insensitive comparison with TRUE and FALSE */

	struct berval bv;
	int i;

	if( in->bv_len == slap_true_bv.bv_len ) {
		bv = slap_true_bv;
	} else if( in->bv_len == slap_false_bv.bv_len ) {
		bv = slap_false_bv;
	} else {
		return LDAP_INVALID_SYNTAX;
	}

	if ( ber_bvstrcasecmp( in, &bv ) != 0 ) {
			return LDAP_INVALID_SYNTAX;
	}
	return LDAP_SUCCESS;
}

static int
cibooleanMatchNormalize(
	slap_mask_t use,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval nvalue;
	ber_len_t i;

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( use ) != 0 );

	if ( BER_BVISNULL( val ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	nvalue.bv_len = val->bv_len;
	nvalue.bv_val = slap_sl_malloc( nvalue.bv_len + 1, ctx );
	nvalue.bv_val[nvalue.bv_len] = '\0';
	for ( i = 0; i < nvalue.bv_len; i++ ) {
		nvalue.bv_val[i] = TOUPPER( val->bv_val[i] );
	}

	*normalized = nvalue;
	return LDAP_SUCCESS;
}


int ciboolean_initialize()
{

	MatchingRule *bm = mr_find( "2.5.13.13" );
	Syntax *syn = syn_find( "1.3.6.1.4.1.1466.115.121.1.7" );
	if ( bm == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			   "ciboolean_initialize: unable to find booleanMatch matching rule\n");
		return -1;
	}

	if ( syn == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			   "ciboolean_initialize: unable to find Boolean syntax\n");
		return -1;
	}

	bm->smr_normalize = cibooleanMatchNormalize;
	syn->ssyn_validate = cibooleanValidate;
	return 0;
}

#if SLAPD_MOD_CIBOOLEAN == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[])
{
	return ciboolean_initialize();
}
#endif

#endif /* SLAPD_MOD_CIBOOLEAN */
