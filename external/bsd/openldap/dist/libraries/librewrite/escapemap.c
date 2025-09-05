/*	$NetBSD: escapemap.c,v 1.1.1.1 2025/09/05 21:09:34 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2024 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Ondřej Kuzník for inclusion in OpenLDAP
 * Software.
 */

#include <portable.h>

#define LDAP_DEPRECATED 1
#include "rewrite-int.h"
#include "rewrite-map.h"

#include <ldap_pvt.h>

typedef int (escape_fn)( struct berval *input, struct berval *output );

/*
 * Map configuration, a NULL-terminated list of escape_fn pointers
 */
struct escape_map_data {
	escape_fn **fn;
};

/*
 * (un)escape functions
 */

static int
map_escape_to_filter( struct berval *input, struct berval *output )
{
	return ldap_bv2escaped_filter_value( input, output );
}

static int
map_unescape_filter( struct berval *input, struct berval *output )
{
	ber_slen_t len;

	if ( ber_dupbv( output, input ) == NULL ) {
		return REWRITE_ERR;
	}

	len = ldap_pvt_filter_value_unescape( output->bv_val );
	if ( len < 0 ) {
		ber_memfree( output->bv_val );
		return REWRITE_ERR;
	}
	output->bv_len = len;

	return LDAP_SUCCESS;
}

static int
map_escape_to_dn( struct berval *input, struct berval *output )
{
	LDAPAVA ava = { .la_attr = BER_BVC("uid"),
					.la_value = *input,
					.la_flags = LDAP_AVA_STRING },
			*ava_[] = { &ava, NULL };
	LDAPRDN rdn[] = { ava_, NULL };
	LDAPDN dn = rdn;
	struct berval dnstr;
	char *p;
	int rc;

	rc = ldap_dn2bv( dn, &dnstr, LDAP_DN_FORMAT_LDAPV3 );
	if ( rc != LDAP_SUCCESS ) {
		return REWRITE_ERR;
	}

	p = strchr( dnstr.bv_val, '=' );
	p++;

	output->bv_len = dnstr.bv_len - ( p - dnstr.bv_val );
	output->bv_val = malloc( output->bv_len + 1 );
	if ( output->bv_val == NULL ) {
		free( dnstr.bv_val );
		return REWRITE_ERR;
	}
	memcpy( output->bv_val, p, output->bv_len );
	output->bv_val[output->bv_len] = '\0';

	free( dnstr.bv_val );
	return REWRITE_SUCCESS;
}

static int
map_unescape_dn( struct berval *input, struct berval *output )
{
	LDAPDN dn;
	struct berval fake_dn;
	char *p;
	int rc = REWRITE_SUCCESS;

	fake_dn.bv_len = STRLENOF("uid=") + input->bv_len;
	fake_dn.bv_val = p = malloc( fake_dn.bv_len + 1 );
	if ( p == NULL ) {
		return REWRITE_ERR;
	}

	memcpy( p, "uid=", STRLENOF("uid=") );
	p += STRLENOF("uid=");
	memcpy( p, input->bv_val, input->bv_len );
	fake_dn.bv_val[fake_dn.bv_len] = '\0';

	if ( ldap_bv2dn( &fake_dn, &dn, LDAP_DN_FORMAT_LDAPV3 ) != LDAP_SUCCESS ) {
		free( fake_dn.bv_val );
		return REWRITE_ERR;
	}
	if ( ber_dupbv( output, &dn[0][0]->la_value ) == NULL ) {
		rc = REWRITE_ERR;
	}
	ldap_dnfree( dn );
	free( fake_dn.bv_val );
	return rc;
}

/* Registered callbacks */

static void *
map_escape_parse(
		const char *fname,
		int lineno,
		int argc,
		char **argv
)
{
	escape_fn **fns;
	int i;

	assert( fname != NULL );
	assert( argv != NULL );

	if ( argc < 1 ) {
		Debug( LDAP_DEBUG_ANY,
				"[%s:%d] escape map needs at least one operation\n",
				fname, lineno );
		return NULL;
	}

	fns = calloc( sizeof(escape_fn *), argc + 1 );
	if ( fns == NULL ) {
		return NULL;
	}

	for ( i = 0; i < argc; i++ ) {
		if ( strcasecmp( argv[i], "escape2dn" ) == 0 ) {
			fns[i] = map_escape_to_dn;
		} else if ( strcasecmp( argv[i], "escape2filter" ) == 0 ) {
			fns[i] = map_escape_to_filter;
		} else if ( strcasecmp( argv[i], "unescapedn" ) == 0 ) {
			fns[i] = map_unescape_dn;
		} else if ( strcasecmp( argv[i], "unescapefilter" ) == 0 ) {
			fns[i] = map_unescape_filter;
		} else {
			Debug( LDAP_DEBUG_ANY,
				"[%s:%d] unknown option %s (ignored)\n",
				fname, lineno, argv[i] );
			free( fns );
			return NULL;
		}
	}

	return (void *)fns;
}

static int
map_escape_apply(
		void *private,
		const char *input,
		struct berval *output )
{
	escape_fn **fns = private;
	struct berval tmpin, tmpout = BER_BVNULL;
	int i;

	assert( private != NULL );
	assert( input != NULL );
	assert( output != NULL );

	ber_str2bv( input, 0, 1, &tmpin );

	for ( i=0; fns[i]; i++ ) {
		int rc = fns[i]( &tmpin, &tmpout );
		free( tmpin.bv_val );
		if ( rc != REWRITE_SUCCESS ) {
			return rc;
		}
		tmpin = tmpout;
		BER_BVZERO( &tmpout );
	}
	*output = tmpin;

	return REWRITE_SUCCESS;
}

static int
map_escape_destroy(
		void *private
)
{
	struct ldap_map_data *data = private;

	assert( private != NULL );
	free( data );

	return 0;
}

const rewrite_mapper rewrite_escape_mapper = {
	"escape",
	map_escape_parse,
	map_escape_apply,
	map_escape_destroy
};
