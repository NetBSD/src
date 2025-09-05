/*	$NetBSD: now.c,v 1.1.1.1 2025/09/05 21:09:44 christos Exp $	*/

/* $OpenLDAP$ */
/*
 * Copyright 1998-2024 The OpenLDAP Foundation.
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
 * This work was initially developed by Pierangelo Masarati
 * for inclusion in OpenLDAP Software.
 */
/*
 * This dynacl module compares the value of a given attribute type
 * with the current time.  The syntax is
 *
 *	dynacl/now=<=attr
 *
 * where attr is an attribute whose syntax is generalizedTime
 * with generalizedTimeOrderingMatch as ORDERING rule.
 */ 

#include <portable.h>

#include <ac/string.h>
#include <slap.h>
#include <lutil.h>

/* Need dynacl... */

#ifdef SLAP_DYNACL

typedef enum {
	NOW_GE,
	NOW_LE
} now_style_t;

typedef struct now_t {
	AttributeDescription	*now_ad;
	now_style_t		now_style;
} now_t;

static int now_dynacl_destroy( void *priv );

static int
now_dynacl_parse(
	const char	*fname,
	int 		lineno,
	const char	*opts,
	slap_style_t	style,
	const char	*pattern,
	void		**privp )
{
	now_t			*now;
	now_style_t		sty = NOW_GE;
	AttributeDescription	*ad = NULL;
	int			rc;
	const char		*text = NULL;
	Syntax			*syn;
	MatchingRule		*mr;

	syn = syn_find( "1.3.6.1.4.1.1466.115.121.1.24" );
	if ( syn == NULL ) {
		fprintf( stderr,
			"%s line %d: unable to find syntax 1.3.6.1.4.1.1466.115.121.1.24 (generalizedTime)\n",
			fname, lineno );
		return 1;
	}

	mr = mr_find( "generalizedTimeOrderingMatch" );
	if ( mr == NULL ) {
		fprintf( stderr,
			"%s line %d: unable to find generalizedTimeOrderingMatch rule\n",
			fname, lineno );
		return 1;
	}

	if ( strncmp( pattern, ">=", STRLENOF( ">=" ) ) == 0 ) {
		sty = NOW_GE;
		pattern += 2;

	} else if ( strncmp( pattern, "<=", STRLENOF( "<=" ) ) == 0 ) {
		sty = NOW_LE;
		pattern += 2;
	}

	rc = slap_str2ad( pattern, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		fprintf( stderr, "%s line %d: now ACL: "
			"unable to lookup \"%s\" "
			"attributeDescription (%d: %s).\n",
			fname, lineno, pattern, rc, text );
		return 1;
	}

	if ( ad->ad_type->sat_syntax != syn ) {
		fprintf( stderr,
			"%s line %d: syntax of attribute \"%s\" is not 1.3.6.1.4.1.1466.115.121.1.24 (generalizedTime)\n",
			fname, lineno, ad->ad_cname.bv_val );
		return 1;
	}

	if ( ad->ad_type->sat_ordering != mr ) {
		fprintf( stderr,
			"%s line %d: ordering matching rule of attribute \"%s\" is not generalizedTimeOrderingMatch\n",
			fname, lineno, ad->ad_cname.bv_val );
		return 1;
	}

	now = ch_calloc( 1, sizeof( now_t ) );
	now->now_ad = ad;
	now->now_style = sty;

	*privp = (void *)now;

	return 0;
}

static int
now_dynacl_unparse(
	void		*priv,
	struct berval	*bv )
{
	now_t		*now = (now_t *)priv;
	char		*ptr;

	bv->bv_len = STRLENOF( " dynacl/now=" ) + 2 + now->now_ad->ad_cname.bv_len;
	bv->bv_val = ch_malloc( bv->bv_len + 1 );

	ptr = lutil_strcopy( bv->bv_val, " dynacl/now=" );
	ptr[ 0 ] = now->now_style == NOW_GE ? '>' : '<';
	ptr[ 1 ] = '=';
	ptr += 2;
	ptr = lutil_strncopy( ptr, now->now_ad->ad_cname.bv_val, now->now_ad->ad_cname.bv_len );
	ptr[ 0 ] = '\0';

	bv->bv_len = ptr - bv->bv_val;

	return 0;
}

static int
now_dynacl_mask(
	void			*priv,
	Operation		*op,
	Entry			*target,
	AttributeDescription	*desc,
	struct berval		*val,
	int			nmatch,
	regmatch_t		*matches,
	slap_access_t		*grant,
	slap_access_t		*deny )
{
	now_t		*now = (now_t *)priv;
	int		rc;
	Attribute	*a;

	ACL_INVALIDATE( *deny );

	assert( target != NULL );

	a = attr_find( target->e_attrs, now->now_ad );
	if ( !a ) {
		rc = LDAP_NO_SUCH_ATTRIBUTE;

	} else {
		char		timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		struct berval	timestamp;
		time_t		t = slap_get_time();
		int		match;
		MatchingRule	*mr = now->now_ad->ad_type->sat_ordering;
		const char	*text = NULL;

		timestamp.bv_val = timebuf;
		timestamp.bv_len = sizeof( timebuf );

		slap_timestamp( &t, &timestamp );

		rc = value_match( &match, now->now_ad, mr, SLAP_MR_ORDERING,
			&timestamp, &a->a_vals[ 0 ], &text );
		if ( rc == LDAP_SUCCESS ) {
			if ( now->now_style == NOW_LE ) {
				match = -match;
			}

			if ( match >= 0 ) {
				rc = LDAP_COMPARE_TRUE;

			} else {
				rc = LDAP_COMPARE_FALSE;
			}
		}
	}

	if ( rc == LDAP_COMPARE_TRUE ) {
		ACL_LVL_ASSIGN_WRITE( *grant );
	}

	return 0;
}

static int
now_dynacl_destroy(
	void		*priv )
{
	now_t		*now = (now_t *)priv;

	if ( now != NULL ) {
		ch_free( now );
	}

	return 0;
}

static struct slap_dynacl_t now_dynacl = {
	"now",
	now_dynacl_parse,
	now_dynacl_unparse,
	now_dynacl_mask,
	now_dynacl_destroy
};

int
init_module( int argc, char *argv[] )
{
	return slap_dynacl_register( &now_dynacl );
}

#endif /* SLAP_DYNACL */
