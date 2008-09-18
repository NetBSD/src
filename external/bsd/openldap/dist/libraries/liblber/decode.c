/* decode.c - ber input decoding routines */
/* $OpenLDAP: pkg/ldap/libraries/liblber/decode.c,v 1.105.2.4 2008/02/11 23:26:41 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1990 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/stdarg.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "lber-int.h"

static ber_len_t ber_getnint LDAP_P((
	BerElement *ber,
	ber_int_t *num,
	ber_len_t len ));

/* out->bv_len should be the buffer size on input */
int
ber_decode_oid( BerValue *in, BerValue *out )
{
	const unsigned char *der;
	unsigned long val;
	unsigned val1;
	ber_len_t i;
	char *ptr;

	assert( in != NULL );
	assert( out != NULL );

	/* need 4 chars/inbyte + \0 for input={7f 7f 7f...} */
	if ( !out->bv_val || (out->bv_len+3)/4 <= in->bv_len )
		return -1;

	ptr = NULL;
	der = (unsigned char *) in->bv_val;
	val = 0;
	for ( i=0; i < in->bv_len; i++ ) {
		val |= der[i] & 0x7f;
		if ( !( der[i] & 0x80 )) {
			if ( ptr == NULL ) {
				/* Initial "x.y": val=x*40+y, x<=2, y<40 if x=2 */
				ptr = out->bv_val;
				val1 = (val < 80 ? val/40 : 2);
				val -= val1*40;
				ptr += sprintf( ptr, "%u", val1 );
			}
			ptr += sprintf( ptr, ".%lu", val );
			val = 0;
		} else if ( val - 1UL < LBER_OID_COMPONENT_MAX >> 7 ) {
			val <<= 7;
		} else {
			/* val would overflow, or is 0 from invalid initial 0x80 octet */
			return -1;
		}
	}
	if ( ptr == NULL || val != 0 )
		return -1;

	out->bv_len = ptr - out->bv_val;
	return 0;
}

/* return the tag - LBER_DEFAULT returned means trouble */
ber_tag_t
ber_get_tag( BerElement *ber )
{
	unsigned char	xbyte;
	ber_tag_t	tag;
	unsigned int	i;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( ber_pvt_ber_remaining( ber ) < 1 ) {
		return LBER_DEFAULT;
	}

	if ( ber->ber_ptr == ber->ber_buf ) {
		tag = *(unsigned char *)ber->ber_ptr;
	} else {
		tag = ber->ber_tag;
	}
	ber->ber_ptr++;

	if ( (tag & LBER_BIG_TAG_MASK) != LBER_BIG_TAG_MASK ) {
		return tag;
	}

	for ( i = 1; i < sizeof(ber_tag_t); i++ ) {
		if ( ber_read( ber, (char *) &xbyte, 1 ) != 1 ) {
			return LBER_DEFAULT;
		}

		tag <<= 8;
		tag |= 0x00ffUL & (ber_tag_t) xbyte;

		if ( ! (xbyte & LBER_MORE_TAG_MASK) ) {
			break;
		}
	}

	/* tag too big! */
	if ( i == sizeof(ber_tag_t) ) {
		return LBER_DEFAULT;
	}

	return tag;
}

ber_tag_t
ber_skip_tag( BerElement *ber, ber_len_t *len )
{
	ber_tag_t	tag;
	unsigned char	lc;
	ber_len_t	i, noctets;
	unsigned char netlen[sizeof(ber_len_t)];

	assert( ber != NULL );
	assert( len != NULL );
	assert( LBER_VALID( ber ) );

	/*
	 * Any ber element looks like this: tag length contents.
	 * Assuming everything's ok, we return the tag byte (we
	 * can assume a single byte), and return the length in len.
	 *
	 * Assumptions:
	 *	1) definite lengths
	 *	2) primitive encodings used whenever possible
	 */

	*len = 0;

	/*
	 * First, we read the tag.
	 */

	if ( (tag = ber_get_tag( ber )) == LBER_DEFAULT ) {
		return LBER_DEFAULT;
	}

	/*
	 * Next, read the length.  The first byte contains the length of
	 * the length.	If bit 8 is set, the length is the long form,
	 * otherwise it's the short form.  We don't allow a length that's
	 * greater than what we can hold in a ber_len_t.
	 */

	if ( ber_read( ber, (char *) &lc, 1 ) != 1 ) {
		return LBER_DEFAULT;
	}

	if ( lc & 0x80U ) {
		noctets = (lc & 0x7fU);

		if ( noctets > sizeof(ber_len_t) ) {
			return LBER_DEFAULT;
		}

		if( (unsigned) ber_read( ber, (char *) netlen, noctets ) != noctets ) {
			return LBER_DEFAULT;
		}

		for( i = 0; i < noctets; i++ ) {
			*len <<= 8;
			*len |= netlen[i];
		}

	} else {
		*len = lc;
	}

	/* BER element should have enough data left */
	if( *len > (ber_len_t) ber_pvt_ber_remaining( ber ) ) {
		return LBER_DEFAULT;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;

	return tag;
}

ber_tag_t
ber_peek_tag(
	BerElement *ber,
	ber_len_t *len )
{
	/*
	 * This implementation assumes ber_skip_tag() only
	 * modifies ber_ptr field of the BerElement.
	 */

	char *save;
	ber_tag_t	tag, old;

	old = ber->ber_tag;
	save = ber->ber_ptr;
	tag = ber_skip_tag( ber, len );
	ber->ber_ptr = save;
	ber->ber_tag = old;

	return tag;
}

static ber_len_t
ber_getnint(
	BerElement *ber,
	ber_int_t *num,
	ber_len_t len )
{
	unsigned char buf[sizeof(ber_int_t)];

	assert( ber != NULL );
	assert( num != NULL );
	assert( LBER_VALID( ber ) );

	/*
	 * The tag and length have already been stripped off.  We should
	 * be sitting right before len bytes of 2's complement integer,
	 * ready to be read straight into an int.  We may have to sign
	 * extend after we read it in.
	 */

	if ( len > sizeof(ber_int_t) ) {
		return -1;
	}

	/* read into the low-order bytes of our buffer */
	if ( (ber_len_t) ber_read( ber, (char *) buf, len ) != len ) {
		return -1;
	}

	if( len ) {
		/* sign extend if necessary */
		ber_len_t i;
		ber_int_t netnum = 0x80 & buf[0] ? -1 : 0;

		/* shift in the bytes */
		for( i=0 ; i<len; i++ ) {
			netnum = (netnum << 8 ) | buf[i];
		}

		*num = netnum;

	} else {
		*num = 0;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;

	return len;
}

ber_tag_t
ber_get_int(
	BerElement *ber,
	ber_int_t *num )
{
	ber_tag_t	tag;
	ber_len_t	len;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( (tag = ber_skip_tag( ber, &len )) == LBER_DEFAULT ) {
		return LBER_DEFAULT;
	}

	if ( ber_getnint( ber, num, len ) != len ) {
		return LBER_DEFAULT;
	}
	
	return tag;
}

ber_tag_t
ber_get_enum(
	BerElement *ber,
	ber_int_t *num )
{
	return ber_get_int( ber, num );
}

ber_tag_t
ber_get_stringb(
	BerElement *ber,
	char *buf,
	ber_len_t *len )
{
	ber_len_t	datalen;
	ber_tag_t	tag;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( (tag = ber_skip_tag( ber, &datalen )) == LBER_DEFAULT ) {
		return LBER_DEFAULT;
	}

	/* must fit within allocated space with termination */
	if ( datalen >= *len ) {
		return LBER_DEFAULT;
	}

	if ( (ber_len_t) ber_read( ber, buf, datalen ) != datalen ) {
		return LBER_DEFAULT;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;

	buf[datalen] = '\0';

	*len = datalen;
	return tag;
}

/* Definitions for get_string vector
 *
 * ChArray, BvArray, and BvVec are self-explanatory.
 * BvOff is a struct berval embedded in an array of larger structures
 * of siz bytes at off bytes from the beginning of the struct.
 */
enum bgbvc { ChArray, BvArray, BvVec, BvOff };

/* Use this single cookie for state, to keep actual
 * stack use to the absolute minimum.
 */
typedef struct bgbvr {
	enum bgbvc choice;
	BerElement *ber;
	int alloc;
	ber_len_t siz;
	ber_len_t off;
	union {
		char ***c;
		BerVarray *ba;
		struct berval ***bv;
	} res;
} bgbvr;

static ber_tag_t
ber_get_stringbvl( bgbvr *b, ber_len_t *rlen )
{
	int i = 0, n;
	ber_tag_t tag;
	ber_len_t len;
	char *last, *orig;
	struct berval bv, *bvp = NULL;

	/* For rewinding, just like ber_peek_tag() */
	orig = b->ber->ber_ptr;
	tag = b->ber->ber_tag;

	if ( ber_first_element( b->ber, &len, &last ) != LBER_DEFAULT ) {
		for ( ; b->ber->ber_ptr < last; i++ ) {
			if (ber_skip_tag( b->ber, &len ) == LBER_DEFAULT) break;
			b->ber->ber_ptr += len;
			b->ber->ber_tag = *(unsigned char *)b->ber->ber_ptr;
		}
	}

	if ( rlen ) *rlen = i;

	if ( i == 0 ) {
		*b->res.c = NULL;
		return 0;
	}

	n = i;

	/* Allocate the result vector */
	switch (b->choice) {
	case ChArray:
		*b->res.c = ber_memalloc_x( (n+1)*sizeof( char * ),
			b->ber->ber_memctx);
		if ( *b->res.c == NULL ) return LBER_DEFAULT;
		(*b->res.c)[n] = NULL;
		break;
	case BvArray:
		*b->res.ba = ber_memalloc_x( (n+1)*sizeof( struct berval ),
			b->ber->ber_memctx);
		if ( *b->res.ba == NULL ) return LBER_DEFAULT;
		(*b->res.ba)[n].bv_val = NULL;
		break;
	case BvVec:
		*b->res.bv = ber_memalloc_x( (n+1)*sizeof( struct berval *),
			b->ber->ber_memctx);
		if ( *b->res.bv == NULL ) return LBER_DEFAULT;
		(*b->res.bv)[n] = NULL;
		break;
	case BvOff:
		*b->res.ba = ber_memalloc_x( (n+1) * b->siz, b->ber->ber_memctx );
		if ( *b->res.ba == NULL ) return LBER_DEFAULT;
		((struct berval *)((char *)(*b->res.ba) + n*b->siz +
			b->off))->bv_val = NULL;
		break;
	}
	b->ber->ber_ptr = orig;
	b->ber->ber_tag = tag;
	ber_skip_tag( b->ber, &len );
	
	for (n=0; n<i; n++)
	{
		tag = ber_next_element( b->ber, &len, last );
		if ( ber_get_stringbv( b->ber, &bv, b->alloc ) == LBER_DEFAULT ) {
			goto nomem;
		}

		/* store my result */
		switch (b->choice) {
		case ChArray:
			(*b->res.c)[n] = bv.bv_val;
			break;
		case BvArray:
			(*b->res.ba)[n] = bv;
			break;
		case BvVec:
			bvp = ber_memalloc_x( sizeof( struct berval ), b->ber->ber_memctx);
			if ( !bvp ) {
				LBER_FREE(bv.bv_val);
				goto nomem;
			}
			(*b->res.bv)[n] = bvp;
			*bvp = bv;
			break;
		case BvOff:
			*(BerVarray)((char *)(*b->res.ba)+n*b->siz+b->off) = bv;
			break;
		}
	}
	return tag;

nomem:
	if (b->alloc || b->choice == BvVec) {
		for (--n; n>=0; n--) {
			switch(b->choice) {
			case ChArray:
				LBER_FREE((*b->res.c)[n]);
				break;
			case BvArray:
				LBER_FREE((*b->res.ba)[n].bv_val);
				break;
			case BvVec:
				LBER_FREE((*b->res.bv)[n]->bv_val);
				LBER_FREE((*b->res.bv)[n]);
				break;
			default:
				break;
			}
		}
	}
	LBER_FREE(*b->res.c);
	*b->res.c = NULL;
	return LBER_DEFAULT;
}

ber_tag_t
ber_get_stringbv( BerElement *ber, struct berval *bv, int option )
{
	ber_tag_t	tag;

	assert( ber != NULL );
	assert( bv != NULL );

	assert( LBER_VALID( ber ) );

	if ( (tag = ber_skip_tag( ber, &bv->bv_len )) == LBER_DEFAULT ) {
		bv->bv_val = NULL;
		return LBER_DEFAULT;
	}

	if ( (ber_len_t) ber_pvt_ber_remaining( ber ) < bv->bv_len ) {
		return LBER_DEFAULT;
	}

	if ( option & LBER_BV_ALLOC ) {
		bv->bv_val = (char *) ber_memalloc_x( bv->bv_len + 1,
			ber->ber_memctx );
		if ( bv->bv_val == NULL ) {
			return LBER_DEFAULT;
		}

		if ( bv->bv_len > 0 && (ber_len_t) ber_read( ber, bv->bv_val,
			bv->bv_len ) != bv->bv_len )
		{
			LBER_FREE( bv->bv_val );
			bv->bv_val = NULL;
			return LBER_DEFAULT;
		}
	} else {
		bv->bv_val = ber->ber_ptr;
		ber->ber_ptr += bv->bv_len;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;
	if ( !( option & LBER_BV_NOTERM ))
		bv->bv_val[bv->bv_len] = '\0';

	return tag;
}

ber_tag_t
ber_get_stringbv_null( BerElement *ber, struct berval *bv, int option )
{
	ber_tag_t	tag;

	assert( ber != NULL );
	assert( bv != NULL );

	assert( LBER_VALID( ber ) );

	if ( (tag = ber_skip_tag( ber, &bv->bv_len )) == LBER_DEFAULT ) {
		bv->bv_val = NULL;
		return LBER_DEFAULT;
	}

	if ( (ber_len_t) ber_pvt_ber_remaining( ber ) < bv->bv_len ) {
		return LBER_DEFAULT;
	}

	if ( bv->bv_len == 0 ) {
		bv->bv_val = NULL;
		ber->ber_tag = *(unsigned char *)ber->ber_ptr;
		return tag;
	}

	if ( option & LBER_BV_ALLOC ) {
		bv->bv_val = (char *) ber_memalloc_x( bv->bv_len + 1,
			ber->ber_memctx );
		if ( bv->bv_val == NULL ) {
			return LBER_DEFAULT;
		}

		if ( bv->bv_len > 0 && (ber_len_t) ber_read( ber, bv->bv_val,
			bv->bv_len ) != bv->bv_len )
		{
			LBER_FREE( bv->bv_val );
			bv->bv_val = NULL;
			return LBER_DEFAULT;
		}
	} else {
		bv->bv_val = ber->ber_ptr;
		ber->ber_ptr += bv->bv_len;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;
	if ( !( option & LBER_BV_NOTERM ))
		bv->bv_val[bv->bv_len] = '\0';

	return tag;
}

ber_tag_t
ber_get_stringa( BerElement *ber, char **buf )
{
	BerValue	bv;
	ber_tag_t	tag;

	assert( buf != NULL );

	tag = ber_get_stringbv( ber, &bv, LBER_BV_ALLOC );
	*buf = bv.bv_val;

	return tag;
}

ber_tag_t
ber_get_stringa_null( BerElement *ber, char **buf )
{
	BerValue	bv;
	ber_tag_t	tag;

	assert( buf != NULL );

	tag = ber_get_stringbv_null( ber, &bv, LBER_BV_ALLOC );
	*buf = bv.bv_val;

	return tag;
}

ber_tag_t
ber_get_stringal( BerElement *ber, struct berval **bv )
{
	ber_tag_t	tag;

	assert( ber != NULL );
	assert( bv != NULL );

	*bv = (struct berval *) ber_memalloc_x( sizeof(struct berval),
		ber->ber_memctx );
	if ( *bv == NULL ) {
		return LBER_DEFAULT;
	}

	tag = ber_get_stringbv( ber, *bv, LBER_BV_ALLOC );
	if ( tag == LBER_DEFAULT ) {
		LBER_FREE( *bv );
		*bv = NULL;
	}
	return tag;
}

ber_tag_t
ber_get_bitstringa(
	BerElement *ber,
	char **buf,
	ber_len_t *blen )
{
	ber_len_t	datalen;
	ber_tag_t	tag;
	unsigned char	unusedbits;

	assert( ber != NULL );
	assert( buf != NULL );
	assert( blen != NULL );

	assert( LBER_VALID( ber ) );

	if ( (tag = ber_skip_tag( ber, &datalen )) == LBER_DEFAULT ) {
		*buf = NULL;
		return LBER_DEFAULT;
	}
	--datalen;

	*buf = (char *) ber_memalloc_x( datalen, ber->ber_memctx );
	if ( *buf == NULL ) {
		return LBER_DEFAULT;
	}

	if ( ber_read( ber, (char *)&unusedbits, 1 ) != 1 ) {
		LBER_FREE( buf );
		*buf = NULL;
		return LBER_DEFAULT;
	}

	if ( (ber_len_t) ber_read( ber, *buf, datalen ) != datalen ) {
		LBER_FREE( buf );
		*buf = NULL;
		return LBER_DEFAULT;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;

	*blen = datalen * 8 - unusedbits;
	return tag;
}

ber_tag_t
ber_get_null( BerElement *ber )
{
	ber_len_t	len;
	ber_tag_t	tag;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( (tag = ber_skip_tag( ber, &len )) == LBER_DEFAULT ) {
		return LBER_DEFAULT;
	}

	if ( len != 0 ) {
		return LBER_DEFAULT;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;

	return( tag );
}

ber_tag_t
ber_get_boolean(
	BerElement *ber,
	ber_int_t *boolval )
{
	ber_int_t	longbool;
	ber_tag_t	rc;

	assert( ber != NULL );
	assert( boolval != NULL );

	assert( LBER_VALID( ber ) );

	rc = ber_get_int( ber, &longbool );
	*boolval = longbool;

	return rc;
}

ber_tag_t
ber_first_element(
	BerElement *ber,
	ber_len_t *len,
	char **last )
{
	assert( ber != NULL );
	assert( len != NULL );
	assert( last != NULL );

	/* skip the sequence header, use the len to mark where to stop */
	if ( ber_skip_tag( ber, len ) == LBER_DEFAULT ) {
		*last = NULL;
		return LBER_DEFAULT;
	}
	ber->ber_tag = *(unsigned char *)ber->ber_ptr;

	*last = ber->ber_ptr + *len;

	if ( *last == ber->ber_ptr ) {
		return LBER_DEFAULT;
	}

	return ber_peek_tag( ber, len );
}

ber_tag_t
ber_next_element(
	BerElement *ber,
	ber_len_t *len,
	LDAP_CONST char *last )
{
	assert( ber != NULL );
	assert( len != NULL );
	assert( last != NULL );

	assert( LBER_VALID( ber ) );

	if ( ber->ber_ptr >= last ) {
		return LBER_DEFAULT;
	}

	return ber_peek_tag( ber, len );
}

/* VARARGS */
ber_tag_t
ber_scanf ( BerElement *ber,
	LDAP_CONST char *fmt,
	... )
{
	va_list		ap;
	LDAP_CONST char		*fmt_reset;
	char		*s, **ss;
	struct berval	**bvp, *bval;
	ber_int_t	*i;
	ber_len_t	*l;
	ber_tag_t	*t;
	ber_tag_t	rc;
	ber_len_t	len;

	va_start( ap, fmt );

	assert( ber != NULL );
	assert( fmt != NULL );

	assert( LBER_VALID( ber ) );

	fmt_reset = fmt;

	if ( ber->ber_debug & (LDAP_DEBUG_TRACE|LDAP_DEBUG_BER)) {
		ber_log_printf( LDAP_DEBUG_TRACE, ber->ber_debug,
			"ber_scanf fmt (%s) ber:\n", fmt );
		ber_log_dump( LDAP_DEBUG_BER, ber->ber_debug, ber, 1 );
	}

	for ( rc = 0; *fmt && rc != LBER_DEFAULT; fmt++ ) {
		/* When this is modified, remember to update
		 * the error-cleanup code below accordingly. */
		switch ( *fmt ) {
		case '!': { /* Hook */
				BERDecodeCallback *f;
				void *p;

				f = va_arg( ap, BERDecodeCallback * );
				p = va_arg( ap, void * );

				rc = (*f)( ber, p, 0 );
			} break;

		case 'a':	/* octet string - allocate storage as needed */
			ss = va_arg( ap, char ** );
			rc = ber_get_stringa( ber, ss );
			break;

		case 'A':	/* octet string - allocate storage as needed,
				 * but return NULL if len == 0 */
			ss = va_arg( ap, char ** );
			rc = ber_get_stringa_null( ber, ss );
			break;

		case 'b':	/* boolean */
			i = va_arg( ap, ber_int_t * );
			rc = ber_get_boolean( ber, i );
			break;

		case 'B':	/* bit string - allocate storage as needed */
			ss = va_arg( ap, char ** );
			l = va_arg( ap, ber_len_t * ); /* for length, in bits */
			rc = ber_get_bitstringa( ber, ss, l );
			break;

		case 'e':	/* enumerated */
		case 'i':	/* int */
			i = va_arg( ap, ber_int_t * );
			rc = ber_get_int( ber, i );
			break;

		case 'l':	/* length of next item */
			l = va_arg( ap, ber_len_t * );
			rc = ber_peek_tag( ber, l );
			break;

		case 'm':	/* octet string in berval, in-place */
			bval = va_arg( ap, struct berval * );
			rc = ber_get_stringbv( ber, bval, 0 );
			break;

		case 'M':	/* bvoffarray - must include address of
				 * a record len, and record offset.
				 * number of records will be returned thru
				 * len ptr on finish. parsed in-place.
				 */
		{
			bgbvr cookie = { BvOff };
			cookie.ber = ber;
			cookie.res.ba = va_arg( ap, struct berval ** );
			cookie.alloc = 0;
			l = va_arg( ap, ber_len_t * );
			cookie.siz = *l;
			cookie.off = va_arg( ap, ber_len_t );
			rc = ber_get_stringbvl( &cookie, l );
			break;
		}

		case 'n':	/* null */
			rc = ber_get_null( ber );
			break;

		case 'o':	/* octet string in a supplied berval */
			bval = va_arg( ap, struct berval * );
			rc = ber_get_stringbv( ber, bval, LBER_BV_ALLOC );
			break;

		case 'O':	/* octet string - allocate & include length */
			bvp = va_arg( ap, struct berval ** );
			rc = ber_get_stringal( ber, bvp );
			break;

		case 's':	/* octet string - in a buffer */
			s = va_arg( ap, char * );
			l = va_arg( ap, ber_len_t * );
			rc = ber_get_stringb( ber, s, l );
			break;

		case 't':	/* tag of next item */
			t = va_arg( ap, ber_tag_t * );
			*t = rc = ber_peek_tag( ber, &len );
			break;

		case 'T':	/* skip tag of next item */
			t = va_arg( ap, ber_tag_t * );
			*t = rc = ber_skip_tag( ber, &len );
			break;

		case 'v':	/* sequence of strings */
		{
			bgbvr cookie = { ChArray };
			cookie.ber = ber;
			cookie.res.c = va_arg( ap, char *** );
			cookie.alloc = LBER_BV_ALLOC;
			rc = ber_get_stringbvl( &cookie, NULL );
			break;
		}

		case 'V':	/* sequence of strings + lengths */
		{
			bgbvr cookie = { BvVec };
			cookie.ber = ber;
			cookie.res.bv = va_arg( ap, struct berval *** );
			cookie.alloc = LBER_BV_ALLOC;
			rc = ber_get_stringbvl( &cookie, NULL );
			break;
		}

		case 'W':	/* bvarray */
		{
			bgbvr cookie = { BvArray };
			cookie.ber = ber;
			cookie.res.ba = va_arg( ap, struct berval ** );
			cookie.alloc = LBER_BV_ALLOC;
			rc = ber_get_stringbvl( &cookie, NULL );
			break;
		}

		case 'x':	/* skip the next element - whatever it is */
			if ( (rc = ber_skip_tag( ber, &len )) == LBER_DEFAULT )
				break;
			ber->ber_ptr += len;
			ber->ber_tag = *(unsigned char *)ber->ber_ptr;
			break;

		case '{':	/* begin sequence */
		case '[':	/* begin set */
			if ( *(fmt + 1) != 'v' && *(fmt + 1) != 'V'
				&& *(fmt + 1) != 'W' && *(fmt + 1) != 'M' )
				rc = ber_skip_tag( ber, &len );
			break;

		case '}':	/* end sequence */
		case ']':	/* end set */
			break;

		default:
			if( ber->ber_debug ) {
				ber_log_printf( LDAP_DEBUG_ANY, ber->ber_debug,
					"ber_scanf: unknown fmt %c\n", *fmt );
			}
			rc = LBER_DEFAULT;
			break;
		}
	}

	va_end( ap );
	if ( rc == LBER_DEFAULT ) {
		/*
		 * Error.  Reclaim malloced memory that was given to the caller.
		 * Set allocated pointers to NULL, "data length" outvalues to 0.
		 */
		va_start( ap, fmt );

		for ( ; fmt_reset < fmt; fmt_reset++ ) {
		switch ( *fmt_reset ) {
		case '!': { /* Hook */
				BERDecodeCallback *f;
				void *p;

				f = va_arg( ap, BERDecodeCallback * );
				p = va_arg( ap, void * );

				(void) (*f)( ber, p, 1 );
			} break;

		case 'a':	/* octet string - allocate storage as needed */
		case 'A':
			ss = va_arg( ap, char ** );
			if ( *ss ) {
				LBER_FREE( *ss );
				*ss = NULL;
			}
			break;

		case 'b':	/* boolean */
		case 'e':	/* enumerated */
		case 'i':	/* int */
			(void) va_arg( ap, int * );
			break;

		case 'l':	/* length of next item */
			(void) va_arg( ap, ber_len_t * );
			break;

		case 'o':	/* octet string in a supplied berval */
			bval = va_arg( ap, struct berval * );
			if ( bval->bv_val != NULL ) {
				LBER_FREE( bval->bv_val );
				bval->bv_val = NULL;
			}
			bval->bv_len = 0;
			break;

		case 'O':	/* octet string - allocate & include length */
			bvp = va_arg( ap, struct berval ** );
			if ( *bvp ) {
				ber_bvfree( *bvp );
				*bvp = NULL;
			}
			break;

		case 's':	/* octet string - in a buffer */
			(void) va_arg( ap, char * );
			(void) va_arg( ap, ber_len_t * );
			break;

		case 't':	/* tag of next item */
		case 'T':	/* skip tag of next item */
			(void) va_arg( ap, ber_tag_t * );
			break;

		case 'B':	/* bit string - allocate storage as needed */
			ss = va_arg( ap, char ** );
			if ( *ss ) {
				LBER_FREE( *ss );
				*ss = NULL;
			}
			*(va_arg( ap, ber_len_t * )) = 0; /* for length, in bits */
			break;

		case 'm':	/* berval in-place */
		case 'M':	/* BVoff array in-place */
		case 'n':	/* null */
		case 'v':	/* sequence of strings */
		case 'V':	/* sequence of strings + lengths */
		case 'W':	/* BerVarray */
		case 'x':	/* skip the next element - whatever it is */
		case '{':	/* begin sequence */
		case '[':	/* begin set */
		case '}':	/* end sequence */
		case ']':	/* end set */
			break;

		default:
			/* format should be good */
			assert( 0 );
		}
		}

		va_end( ap );
	}

	return rc;
}
