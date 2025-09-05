/*	$NetBSD: verbs.c,v 1.1.1.1 2025/09/05 21:09:46 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: verbs.c,v 1.1.1.1 2025/09/05 21:09:46 christos Exp $");

#include "portable.h"

#include "slap.h"
#include "slap-config.h"

int
bverb_to_mask( struct berval *bword, slap_verbmasks *v ) {
	int i;
	for ( i = 0; !BER_BVISNULL(&v[i].word); i++ ) {
		if ( !ber_bvstrcasecmp( bword, &v[i].word) ) break;
	}
	return i;
}

int
verb_to_mask( const char *word, slap_verbmasks *v ) {
	struct berval	bword;
	ber_str2bv( word, 0, 0, &bword );
	return bverb_to_mask( &bword, v );
}

int
verbs_to_mask( int argc, char *argv[], slap_verbmasks *v, slap_mask_t *m ) {
	int i, j;
	for (i = 1; i < argc; i++ ) {
		j = verb_to_mask( argv[i], v );
		if ( BER_BVISNULL(&v[j].word) ) return i;
		while ( !v[j].mask ) j--;
		*m |= v[j].mask;
	}
	return 0;
}

/*
 * Mask keywords that represent multiple bits should occur before single
 * bit keywords in the verbmasks array.
 */
int
mask_to_verbs( slap_verbmasks *v, slap_mask_t m, BerVarray *bva ) {
	int i, rc = 1;

	if ( m ) {
		for ( i=0; !BER_BVISNULL(&v[i].word); i++ ) {
			if (!v[i].mask) continue;
			if ( ( m & v[i].mask ) == v[i].mask ) {
				value_add_one( bva, &v[i].word );
				rc = 0;
				m ^= v[i].mask;
				if ( !m ) break;
			}
		}
	}
	return rc;
}

/* Return the verbs as a single string, separated by delim */
int
mask_to_verbstring( slap_verbmasks *v, slap_mask_t m0, char delim, struct berval *bv )
{
	int i, rc = 1;

	BER_BVZERO( bv );
	if ( m0 ) {
		slap_mask_t m = m0;
		char *ptr;
		for ( i=0; !BER_BVISNULL(&v[i].word); i++ ) {
			if ( !v[i].mask ) continue;
			if ( ( m & v[i].mask ) == v[i].mask ) {
				bv->bv_len += v[i].word.bv_len + 1;
				rc = 0;
				m ^= v[i].mask;
				if ( !m ) break;
			}
		}
		bv->bv_val = ch_malloc(bv->bv_len);
		bv->bv_len--;
		ptr = bv->bv_val;
		m = m0;
		for ( i=0; !BER_BVISNULL(&v[i].word); i++ ) {
			if ( !v[i].mask ) continue;
			if ( ( m & v[i].mask ) == v[i].mask ) {
				ptr = lutil_strcopy( ptr, v[i].word.bv_val );
				*ptr++ = delim;
				m ^= v[i].mask;
				if ( !m ) break;
			}
		}
		ptr[-1] = '\0';
	}
	return rc;
}

/* Parse a verbstring */
int
verbstring_to_mask( slap_verbmasks *v, char *str, char delim, slap_mask_t *m ) {
	int j;
	char *d;
	struct berval bv;

	do {
		bv.bv_val = str;
		d = strchr( str, delim );
		if ( d )
			bv.bv_len = d - str;
		else
			bv.bv_len = strlen( str );
		j = bverb_to_mask( &bv, v );
		if ( BER_BVISNULL(&v[j].word) ) return 1;
		while ( !v[j].mask ) j--;
		*m |= v[j].mask;
		str += bv.bv_len + 1;
	} while ( d );
	return 0;
}

int
slap_verbmasks_init( slap_verbmasks **vp, slap_verbmasks *v )
{
	int		i;

	assert( *vp == NULL );

	for ( i = 0; !BER_BVISNULL( &v[ i ].word ); i++ ) /* EMPTY */;

	*vp = ch_calloc( i + 1, sizeof( slap_verbmasks ) );

	for ( i = 0; !BER_BVISNULL( &v[ i ].word ); i++ ) {
		ber_dupbv( &(*vp)[ i ].word, &v[ i ].word );
		*((slap_mask_t *)&(*vp)[ i ].mask) = v[ i ].mask;
	}

	BER_BVZERO( &(*vp)[ i ].word );

	return 0;
}

int
slap_verbmasks_destroy( slap_verbmasks *v )
{
	int		i;

	assert( v != NULL );

	for ( i = 0; !BER_BVISNULL( &v[ i ].word ); i++ ) {
		ch_free( v[ i ].word.bv_val );
	}

	ch_free( v );

	return 0;
}

int
slap_verbmasks_append(
	slap_verbmasks	**vp,
	slap_mask_t	m,
	struct berval	*v,
	slap_mask_t	*ignore )
{
	int	i;

	if ( !m ) {
		return LDAP_OPERATIONS_ERROR;
	}

	for ( i = 0; !BER_BVISNULL( &(*vp)[ i ].word ); i++ ) {
		if ( !(*vp)[ i ].mask ) continue;

		if ( ignore != NULL ) {
			int	j;

			for ( j = 0; ignore[ j ] != 0; j++ ) {
				if ( (*vp)[ i ].mask == ignore[ j ] ) {
					goto check_next;
				}
			}
		}

		if ( ( m & (*vp)[ i ].mask ) == (*vp)[ i ].mask ) {
			if ( ber_bvstrcasecmp( v, &(*vp)[ i ].word ) == 0 ) {
				/* already set; ignore */
				return LDAP_SUCCESS;
			}
			/* conflicts */
			return LDAP_TYPE_OR_VALUE_EXISTS;
		}

		if ( m & (*vp)[ i ].mask ) {
			/* conflicts */
			return LDAP_CONSTRAINT_VIOLATION;
		}
check_next:;
	}

	*vp = ch_realloc( *vp, sizeof( slap_verbmasks ) * ( i + 2 ) );
	ber_dupbv( &(*vp)[ i ].word, v );
	*((slap_mask_t *)&(*vp)[ i ].mask) = m;
	BER_BVZERO( &(*vp)[ i + 1 ].word );

	return LDAP_SUCCESS;
}

int
enum_to_verb(slap_verbmasks *v, slap_mask_t m, struct berval *bv) {
	int i;

	for (i=0; !BER_BVISNULL(&v[i].word); i++) {
		if ( m == v[i].mask ) {
			if ( bv != NULL ) {
				*bv = v[i].word;
			}
			return i;
		}
	}
	return -1;
}

/* register a new verbmask */
int
slap_verbmask_register(
	slap_verbmasks *vm_,
	slap_verbmasks **vmp,
	struct berval *bv,
	int mask )
{
	slap_verbmasks	*vm = *vmp;
	int		i;

	/* check for duplicate word */
	/* NOTE: we accept duplicate codes; the first occurrence will be used
	 * when mapping from mask to verb */
	i = verb_to_mask( bv->bv_val, vm );
	if ( !BER_BVISNULL( &vm[ i ].word ) ) {
		return -1;
	}

	for ( i = 0; !BER_BVISNULL( &vm[ i ].word ); i++ )
		;

	if ( vm == vm_ ) {
		/* first time: duplicate array */
		vm = ch_calloc( i + 2, sizeof( slap_verbmasks ) );
		for ( i = 0; !BER_BVISNULL( &vm_[ i ].word ); i++ )
		{
			ber_dupbv( &vm[ i ].word, &vm_[ i ].word );
			*((slap_mask_t*)&vm[ i ].mask) = vm_[ i ].mask;
		}

	} else {
		vm = ch_realloc( vm, (i + 2) * sizeof( slap_verbmasks ) );
	}

	ber_dupbv( &vm[ i ].word, bv );
	*((slap_mask_t*)&vm[ i ].mask) = mask;

	BER_BVZERO( &vm[ i+1 ].word );

	*vmp = vm;

	return i;
}
