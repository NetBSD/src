/* encode.c - ber output encoding routines */
/* $OpenLDAP: pkg/ldap/libraries/liblber/encode.c,v 1.64.2.3 2008/02/11 23:26:41 kurt Exp $ */
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

#include <ctype.h>
#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/stdarg.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "lber-int.h"

static int ber_put_len LDAP_P((
	BerElement *ber,
	ber_len_t len,
	int nosos ));

static int ber_start_seqorset LDAP_P((
	BerElement *ber,
	ber_tag_t tag ));

static int ber_put_seqorset LDAP_P(( BerElement *ber ));

static int ber_put_int_or_enum LDAP_P((
	BerElement *ber,
	ber_int_t num,
	ber_tag_t tag ));

#define	BER_TOP_BYTE(type)	(sizeof(type)-1)
#define	BER_TOP_MASK(type)	((type)0xffU << (BER_TOP_BYTE(type)*8))

static int
ber_calc_taglen( ber_tag_t tag )
{
	int	i = BER_TOP_BYTE(ber_tag_t);
	ber_tag_t	mask = BER_TOP_MASK(ber_tag_t);

	/* find the first non-all-zero byte in the tag */
	for ( ; i > 0; i-- ) {
		/* not all zero */
		if ( tag & mask ) break;
		mask >>= 8;
	}

	return i + 1;
}

static int
ber_put_tag(
	BerElement	*ber,
	ber_tag_t tag,
	int nosos )
{
	int rc;
	int taglen;
	int	i;
	unsigned char nettag[sizeof(ber_tag_t)];

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	taglen = ber_calc_taglen( tag );

	for( i=taglen-1; i>=0; i-- ) {
		nettag[i] = (unsigned char)(tag & 0xffU);
		tag >>= 8;
	}

	rc = ber_write( ber, (char *) nettag, taglen, nosos );

	return rc;
}

static ber_len_t
ber_calc_lenlen( ber_len_t len )
{
	/*
	 * short len if it's less than 128 - one byte giving the len,
	 * with bit 8 0.
	 */

	if ( len <= (ber_len_t) 0x7FU ) return 1;

	/*
	 * long len otherwise - one byte with bit 8 set, giving the
	 * length of the length, followed by the length itself.
	 */

	if ( len <= (ber_len_t) 0xffU ) return 2;
	if ( len <= (ber_len_t) 0xffffU ) return 3;
	if ( len <= (ber_len_t) 0xffffffU ) return 4;

	return 5;
}

static int
ber_put_len( BerElement *ber, ber_len_t len, int nosos )
{
	int rc;
	int		i,j;
	char		lenlen;
	ber_len_t	mask;
	unsigned char netlen[sizeof(ber_len_t)];

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	/*
	 * short len if it's less than 128 - one byte giving the len,
	 * with bit 8 0.
	 */

	if ( len <= 127 ) {
		char length_byte = (char) len;
		return ber_write( ber, &length_byte, 1, nosos );
	}

	/*
	 * long len otherwise - one byte with bit 8 set, giving the
	 * length of the length, followed by the length itself.
	 */

	/* find the first non-all-zero byte */
	i = BER_TOP_BYTE(ber_len_t);
	mask = BER_TOP_MASK(ber_len_t);
	for ( ; i > 0; i-- ) {
		/* not all zero */
		if ( len & mask ) break;
		mask >>= 8;
	}
	lenlen = (unsigned char) ++i;
	if ( lenlen > 4 ) return -1;

	lenlen |= 0x80UL;

	/* write the length of the length */
	if ( ber_write( ber, &lenlen, 1, nosos ) != 1 ) return -1;

	for( j=i-1; j>=0; j-- ) {
		netlen[j] = (unsigned char)(len & 0xffU);
		len >>= 8;
	}

	/* write the length itself */
	rc = ber_write( ber, (char *) netlen, i, nosos );

	return rc == i ?  i+1 : -1;
}

/* out->bv_len should be the buffer size on input */
int
ber_encode_oid( BerValue *in, BerValue *out )
{
	unsigned char *der;
	unsigned long val1, val;
	int i, j, len;
	char *ptr, *end, *inend;

	assert( in != NULL );
	assert( out != NULL );

	if ( !out->bv_val || out->bv_len < in->bv_len/2 )
		return -1;

	der = (unsigned char *) out->bv_val;
	ptr = in->bv_val;
	inend = ptr + in->bv_len;

	/* OIDs start with <0-1>.<0-39> or 2.<any>, DER-encoded 40*val1+val2 */
	if ( !isdigit( (unsigned char) *ptr )) return -1;
	val1 = strtoul( ptr, &end, 10 );
	if ( end == ptr || val1 > 2 ) return -1;
	if ( *end++ != '.' || !isdigit( (unsigned char) *end )) return -1;
	val = strtoul( end, &ptr, 10 );
	if ( ptr == end ) return -1;
	if ( val > (val1 < 2 ? 39 : LBER_OID_COMPONENT_MAX - 80) ) return -1;
	val += val1 * 40;

	for (;;) {
		if ( ptr > inend ) return -1;

		len = 0;
		do {
			der[len++] = (val & 0xff) | 0x80;
		} while ( (val >>= 7) != 0 );
		der[0] &= 0x7f;
		for ( i = 0, j = len; i < --j; i++ ) {
			unsigned char tmp = der[i];
			der[i] = der[j];
			der[j] = tmp;
		}
		der += len;
		if ( ptr == inend )
			break;

		if ( *ptr++ != '.' ) return -1;
		if ( !isdigit( (unsigned char) *ptr )) return -1;
		val = strtoul( ptr, &end, 10 );
		if ( end == ptr || val > LBER_OID_COMPONENT_MAX ) return -1;
		ptr = end;
	}

	out->bv_len = (char *)der - out->bv_val;
	return 0;
}

static int
ber_put_int_or_enum(
	BerElement *ber,
	ber_int_t num,
	ber_tag_t tag )
{
	int rc;
	int	i, j, sign, taglen, lenlen;
	ber_len_t	len;
	ber_uint_t	unum, mask;
	unsigned char netnum[sizeof(ber_uint_t)];

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	sign = (num < 0);
	unum = num;	/* Bit fiddling should be done with unsigned values */

	/*
	 * high bit is set - look for first non-all-one byte
	 * high bit is clear - look for first non-all-zero byte
	 */
	i = BER_TOP_BYTE(ber_int_t);
	mask = BER_TOP_MASK(ber_uint_t);
	for ( ; i > 0; i-- ) {
		if ( sign ) {
			/* not all ones */
			if ( (unum & mask) != mask ) break;
		} else {
			/* not all zero */
			if ( unum & mask ) break;
		}
		mask >>= 8;
	}

	/*
	 * we now have the "leading byte".  if the high bit on this
	 * byte matches the sign bit, we need to "back up" a byte.
	 */
	mask = (unum & ((ber_uint_t)0x80U << (i * 8)));
	if ( (mask && !sign) || (sign && !mask) ) {
		i++;
	}

	len = i + 1;

	if ( (taglen = ber_put_tag( ber, tag, 0 )) == -1 ) {
		return -1;
	}

	if ( (lenlen = ber_put_len( ber, len, 0 )) == -1 ) {
		return -1;
	}
	i++;

	for( j=i-1; j>=0; j-- ) {
		netnum[j] = (unsigned char)(unum & 0xffU);
		unum >>= 8;
	}

	rc = ber_write( ber, (char *) netnum, i, 0 );

	/* length of tag + length + contents */
	return rc == i ? taglen + lenlen + i : -1;
}

int
ber_put_enum(
	BerElement *ber,
	ber_int_t num,
	ber_tag_t tag )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_ENUMERATED;
	}

	return ber_put_int_or_enum( ber, num, tag );
}

int
ber_put_int(
	BerElement *ber,
	ber_int_t num,
	ber_tag_t tag )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_INTEGER;
	}

	return ber_put_int_or_enum( ber, num, tag );
}

int
ber_put_ostring(
	BerElement *ber,
	LDAP_CONST char *str,
	ber_len_t len,
	ber_tag_t tag )
{
	int taglen, lenlen, rc;

	assert( ber != NULL );
	assert( str != NULL );

	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_OCTETSTRING;
	}

	if ( (taglen = ber_put_tag( ber, tag, 0 )) == -1 )
		return -1;

	if ( (lenlen = ber_put_len( ber, len, 0 )) == -1 ||
		(ber_len_t) ber_write( ber, str, len, 0 ) != len )
	{
		rc = -1;
	} else {
		/* return length of tag + length + contents */
		rc = taglen + lenlen + len;
	}

	return rc;
}

int
ber_put_berval(
	BerElement *ber,
	struct berval *bv,
	ber_tag_t tag )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if( bv == NULL || bv->bv_len == 0 ) {
		return ber_put_ostring( ber, "", (ber_len_t) 0, tag );
	}

	return ber_put_ostring( ber, bv->bv_val, bv->bv_len, tag );
}

int
ber_put_string(
	BerElement *ber,
	LDAP_CONST char *str,
	ber_tag_t tag )
{
	assert( ber != NULL );
	assert( str != NULL );

	assert( LBER_VALID( ber ) );

	return ber_put_ostring( ber, str, strlen( str ), tag );
}

int
ber_put_bitstring(
	BerElement *ber,
	LDAP_CONST char *str,
	ber_len_t blen /* in bits */,
	ber_tag_t tag )
{
	int				taglen, lenlen;
	ber_len_t		len;
	unsigned char	unusedbits;

	assert( ber != NULL );
	assert( str != NULL );

	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_BITSTRING;
	}

	if ( (taglen = ber_put_tag( ber, tag, 0 )) == -1 ) {
		return -1;
	}

	len = ( blen + 7 ) / 8;
	unusedbits = (unsigned char) ((len * 8) - blen);
	if ( (lenlen = ber_put_len( ber, len + 1, 0 )) == -1 ) {
		return -1;
	}

	if ( ber_write( ber, (char *)&unusedbits, 1, 0 ) != 1 ) {
		return -1;
	}

	if ( (ber_len_t) ber_write( ber, str, len, 0 ) != len ) {
		return -1;
	}

	/* return length of tag + length + unused bit count + contents */
	return taglen + 1 + lenlen + len;
}

int
ber_put_null( BerElement *ber, ber_tag_t tag )
{
	int	taglen;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_NULL;
	}

	if ( (taglen = ber_put_tag( ber, tag, 0 )) == -1 ) {
		return -1;
	}

	if ( ber_put_len( ber, 0, 0 ) != 1 ) {
		return -1;
	}

	return taglen + 1;
}

int
ber_put_boolean(
	BerElement *ber,
	ber_int_t boolval,
	ber_tag_t tag )
{
	int				taglen;
	unsigned char	c;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT )
		tag = LBER_BOOLEAN;

	if ( (taglen = ber_put_tag( ber, tag, 0 )) == -1 ) {
		return -1;
	}

	if ( ber_put_len( ber, 1, 0 ) != 1 ) {
		return -1;
	}

	c = boolval ? (unsigned char) ~0U : (unsigned char) 0U;

	if ( ber_write( ber, (char *) &c, 1, 0 ) != 1 ) {
		return -1;
	}

	return taglen + 2;
}

#define FOUR_BYTE_LEN	5

static int
ber_start_seqorset(
	BerElement *ber,
	ber_tag_t tag )
{
	Seqorset	*new;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	new = (Seqorset *) ber_memcalloc_x( 1, sizeof(Seqorset), ber->ber_memctx );

	if ( new == NULL ) {
		return -1;
	}

	new->sos_ber = ber;
	if ( ber->ber_sos == NULL ) {
		new->sos_first = ber->ber_ptr;
	} else {
		new->sos_first = ber->ber_sos->sos_ptr;
	}

	/* Set aside room for a 4 byte length field */
	new->sos_ptr = new->sos_first + ber_calc_taglen( tag ) + FOUR_BYTE_LEN;
	new->sos_tag = tag;

	new->sos_next = ber->ber_sos;
	ber->ber_sos = new;

	return 0;
}

int
ber_start_seq( BerElement *ber, ber_tag_t tag )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_SEQUENCE;
	}

	return ber_start_seqorset( ber, tag );
}

int
ber_start_set( BerElement *ber, ber_tag_t tag )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if ( tag == LBER_DEFAULT ) {
		tag = LBER_SET;
	}

	return ber_start_seqorset( ber, tag );
}

static int
ber_put_seqorset( BerElement *ber )
{
	int rc;
	ber_len_t	len;
	unsigned char netlen[sizeof(ber_len_t)];
	int			taglen;
	ber_len_t	lenlen;
	unsigned char	ltag = 0x80U + FOUR_BYTE_LEN - 1;
	Seqorset	*next;
	Seqorset	**sos = &ber->ber_sos;

	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	if( *sos == NULL ) return -1;

	/*
	 * If this is the toplevel sequence or set, we need to actually
	 * write the stuff out.	 Otherwise, it's already been put in
	 * the appropriate buffer and will be written when the toplevel
	 * one is written.  In this case all we need to do is update the
	 * length and tag.
	 */

	len = (*sos)->sos_clen;

	if ( sizeof(ber_len_t) > 4 && len > 0xffffffffUL ) {
		return -1;
	}

	if ( ber->ber_options & LBER_USE_DER ) {
		lenlen = ber_calc_lenlen( len );

	} else {
		lenlen = FOUR_BYTE_LEN;
	}

	if( lenlen > 1 ) {
		int i;
		ber_len_t j = len;
		for( i=lenlen-2; i >= 0; i-- ) {
			netlen[i] = j & 0xffU;
			j >>= 8;
		}
	} else {
		netlen[0] = (unsigned char)(len & 0x7fU);
	}

	if ( (next = (*sos)->sos_next) == NULL ) {
		/* write the tag */
		if ( (taglen = ber_put_tag( ber, (*sos)->sos_tag, 1 )) == -1 ) {
			return( -1 );
		}

		if ( ber->ber_options & LBER_USE_DER ) {
			/* Write the length in the minimum # of octets */
			if ( ber_put_len( ber, len, 1 ) == -1 ) {
				return -1;
			}

			if (lenlen != FOUR_BYTE_LEN) {
				/*
				 * We set aside FOUR_BYTE_LEN bytes for
				 * the length field.  Move the data if
				 * we don't actually need that much
				 */
				AC_MEMCPY( (*sos)->sos_first + taglen +
				    lenlen, (*sos)->sos_first + taglen +
				    FOUR_BYTE_LEN, len );
			}
		} else {
			/* Fill FOUR_BYTE_LEN bytes for length field */
			/* one byte of length length */
			if ( ber_write( ber, (char *)&ltag, 1, 1 ) != 1 ) {
				return -1;
			}

			/* the length itself */
			rc  = ber_write( ber, (char *) netlen, FOUR_BYTE_LEN-1, 1 );

			if( rc != FOUR_BYTE_LEN - 1 ) {
				return -1;
			}
		}
		/* The ber_ptr is at the set/seq start - move it to the end */
		(*sos)->sos_ber->ber_ptr += len;

	} else {
		int i;
		unsigned char nettag[sizeof(ber_tag_t)];
		ber_tag_t tmptag = (*sos)->sos_tag;

		if( ber->ber_sos->sos_ptr > ber->ber_end ) {
			/* The sos_ptr exceeds the end of the BerElement
			 * this can happen, for example, when the sos_ptr
			 * is near the end and no data was written for the
			 * 'V'.	 We must realloc the BerElement to ensure
			 * we don't overwrite the buffer when writing
			 * the tag and length fields.
			 */
			ber_len_t ext = ber->ber_sos->sos_ptr - ber->ber_end;

			if( ber_realloc( ber,  ext ) != 0 ) {
				return -1;
			}
		}

		/* the tag */
		taglen = ber_calc_taglen( tmptag );

		for( i = taglen-1; i >= 0; i-- ) {
			nettag[i] = (unsigned char)(tmptag & 0xffU);
			tmptag >>= 8;
		}

		AC_FMEMCPY( (*sos)->sos_first, nettag, taglen );

		if ( ber->ber_options & LBER_USE_DER ) {
			ltag = (lenlen == 1)
				? (unsigned char) len
				: (unsigned char) (0x80U + (lenlen - 1));
		}

		/* one byte of length length */
		(*sos)->sos_first[1] = ltag;

		if ( ber->ber_options & LBER_USE_DER ) {
			if (lenlen > 1) {
				/* Write the length itself */
				AC_FMEMCPY( (*sos)->sos_first + 2, netlen, lenlen - 1 );
			}
			if (lenlen != FOUR_BYTE_LEN) {
				/*
				 * We set aside FOUR_BYTE_LEN bytes for
				 * the length field.  Move the data if
				 * we don't actually need that much
				 */
				AC_FMEMCPY( (*sos)->sos_first + taglen +
				    lenlen, (*sos)->sos_first + taglen +
				    FOUR_BYTE_LEN, len );
			}
		} else {
			/* the length itself */
			AC_FMEMCPY( (*sos)->sos_first + taglen + 1,
			    netlen, FOUR_BYTE_LEN - 1 );
		}

		next->sos_clen += (taglen + lenlen + len);
		next->sos_ptr += (taglen + lenlen + len);
	}

	/* we're done with this seqorset, so free it up */
	ber_memfree_x( (char *) (*sos), ber->ber_memctx );
	*sos = next;

	return taglen + lenlen + len;
}

int
ber_put_seq( BerElement *ber )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	return ber_put_seqorset( ber );
}

int
ber_put_set( BerElement *ber )
{
	assert( ber != NULL );
	assert( LBER_VALID( ber ) );

	return ber_put_seqorset( ber );
}

/* N tag */
static ber_tag_t lber_int_null = 0;

/* VARARGS */
int
ber_printf( BerElement *ber, LDAP_CONST char *fmt, ... )
{
	va_list		ap;
	char		*s, **ss;
	struct berval	*bv, **bvp;
	int		rc;
	ber_int_t	i;
	ber_len_t	len;

	assert( ber != NULL );
	assert( fmt != NULL );

	assert( LBER_VALID( ber ) );

	va_start( ap, fmt );

	for ( rc = 0; *fmt && rc != -1; fmt++ ) {
		switch ( *fmt ) {
		case '!': { /* hook */
				BEREncodeCallback *f;
				void *p;

				f = va_arg( ap, BEREncodeCallback * );
				p = va_arg( ap, void * );

				rc = (*f)( ber, p );
			} break;

		case 'b':	/* boolean */
			i = va_arg( ap, ber_int_t );
			rc = ber_put_boolean( ber, i, ber->ber_tag );
			break;

		case 'i':	/* int */
			i = va_arg( ap, ber_int_t );
			rc = ber_put_int( ber, i, ber->ber_tag );
			break;

		case 'e':	/* enumeration */
			i = va_arg( ap, ber_int_t );
			rc = ber_put_enum( ber, i, ber->ber_tag );
			break;

		case 'n':	/* null */
			rc = ber_put_null( ber, ber->ber_tag );
			break;

		case 'N':	/* Debug NULL */
			if( lber_int_null != 0 ) {
				/* Insert NULL to ensure peer ignores unknown tags */
				rc = ber_put_null( ber, lber_int_null );
			} else {
				rc = 0;
			}
			break;

		case 'o':	/* octet string (non-null terminated) */
			s = va_arg( ap, char * );
			len = va_arg( ap, ber_len_t );
			rc = ber_put_ostring( ber, s, len, ber->ber_tag );
			break;

		case 'O':	/* berval octet string */
			bv = va_arg( ap, struct berval * );
			if( bv == NULL ) break;
			rc = ber_put_berval( ber, bv, ber->ber_tag );
			break;

		case 's':	/* string */
			s = va_arg( ap, char * );
			rc = ber_put_string( ber, s, ber->ber_tag );
			break;

		case 'B':	/* bit string */
		case 'X':	/* bit string (deprecated) */
			s = va_arg( ap, char * );
			len = va_arg( ap, int );	/* in bits */
			rc = ber_put_bitstring( ber, s, len, ber->ber_tag );
			break;

		case 't':	/* tag for the next element */
			ber->ber_tag = va_arg( ap, ber_tag_t );
			ber->ber_usertag = 1;
			break;

		case 'v':	/* vector of strings */
			if ( (ss = va_arg( ap, char ** )) == NULL )
				break;
			for ( i = 0; ss[i] != NULL; i++ ) {
				if ( (rc = ber_put_string( ber, ss[i],
				    ber->ber_tag )) == -1 )
					break;
			}
			break;

		case 'V':	/* sequences of strings + lengths */
			if ( (bvp = va_arg( ap, struct berval ** )) == NULL )
				break;
			for ( i = 0; bvp[i] != NULL; i++ ) {
				if ( (rc = ber_put_berval( ber, bvp[i],
				    ber->ber_tag )) == -1 )
					break;
			}
			break;

		case 'W':	/* BerVarray */
			if ( (bv = va_arg( ap, BerVarray )) == NULL )
				break;
			for ( i = 0; bv[i].bv_val != NULL; i++ ) {
				if ( (rc = ber_put_berval( ber, &bv[i],
				    ber->ber_tag )) == -1 )
					break;
			}
			break;

		case '{':	/* begin sequence */
			rc = ber_start_seq( ber, ber->ber_tag );
			break;

		case '}':	/* end sequence */
			rc = ber_put_seqorset( ber );
			break;

		case '[':	/* begin set */
			rc = ber_start_set( ber, ber->ber_tag );
			break;

		case ']':	/* end set */
			rc = ber_put_seqorset( ber );
			break;

		default:
			if( ber->ber_debug ) {
				ber_log_printf( LDAP_DEBUG_ANY, ber->ber_debug,
					"ber_printf: unknown fmt %c\n", *fmt );
			}
			rc = -1;
			break;
		}

		if ( ber->ber_usertag == 0 ) {
			ber->ber_tag = LBER_DEFAULT;
		} else {
			ber->ber_usertag = 0;
		}
	}

	va_end( ap );

	return rc;
}
