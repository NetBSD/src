/* ldapsync.c -- LDAP Content Sync Routines */
/* $OpenLDAP: pkg/ldap/servers/slapd/ldapsync.c,v 1.32.2.7 2008/02/11 23:26:44 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * Portions Copyright 2003 IBM Corporation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "../../libraries/liblber/lber-int.h" /* get ber_strndup() */
#include "lutil_ldap.h"

struct slap_sync_cookie_s slap_sync_cookie =
	LDAP_STAILQ_HEAD_INITIALIZER( slap_sync_cookie );

void
slap_compose_sync_cookie(
	Operation *op,
	struct berval *cookie,
	BerVarray csn,
	int rid,
	int sid )
{
	int len, numcsn = 0;

	if ( csn ) {
		for (; !BER_BVISNULL( &csn[numcsn] ); numcsn++);
	}

	if ( numcsn == 0 || rid == -1 ) {
		char cookiestr[ LDAP_LUTIL_CSNSTR_BUFSIZE + 20 ];
		if ( rid == -1 ) {
			cookiestr[0] = '\0';
			len = 0;
		} else {
			len = snprintf( cookiestr, sizeof( cookiestr ),
					"rid=%03d", rid );
			if ( sid >= 0 ) {
				len += sprintf( cookiestr+len, ",sid=%03x", sid );
			}
		}
		ber_str2bv_x( cookiestr, len, 1, cookie, 
			op ? op->o_tmpmemctx : NULL );
	} else {
		char *ptr;
		int i;

		len = 0;
		for ( i=0; i<numcsn; i++)
			len += csn[i].bv_len + 1;

		len += STRLENOF("rid=123,csn=");
		if ( sid >= 0 )
			len += STRLENOF("sid=xxx,");

		cookie->bv_val = slap_sl_malloc( len, op ? op->o_tmpmemctx : NULL );

		len = sprintf( cookie->bv_val, "rid=%03d,", rid );
		ptr = cookie->bv_val + len;
		if ( sid >= 0 ) {
			ptr += sprintf( ptr, "sid=%03x,", sid );
		}
		ptr = lutil_strcopy( ptr, "csn=" );
		for ( i=0; i<numcsn; i++) {
			ptr = lutil_strncopy( ptr, csn[i].bv_val, csn[i].bv_len );
			*ptr++ = ';';
		}
		ptr--;
		*ptr = '\0';
		cookie->bv_len = ptr - cookie->bv_val;
	}
}

void
slap_sync_cookie_free(
	struct sync_cookie *cookie,
	int free_cookie
)
{
	if ( cookie == NULL )
		return;

	if ( cookie->sids ) {
		ch_free( cookie->sids );
		cookie->sids = NULL;
	}

	if ( cookie->ctxcsn ) {
		ber_bvarray_free( cookie->ctxcsn );
		cookie->ctxcsn = NULL;
	}
	cookie->numcsns = 0;
	if ( !BER_BVISNULL( &cookie->octet_str )) {
		ch_free( cookie->octet_str.bv_val );
		BER_BVZERO( &cookie->octet_str );
	}

	if ( free_cookie ) {
		ch_free( cookie );
	}

	return;
}

int
slap_parse_csn_sid( struct berval *csnp )
{
	char *p, *q;
	struct berval csn = *csnp;
	int i;

	p = ber_bvchr( &csn, '#' );
	if ( !p )
		return -1;
	p++;
	csn.bv_len -= p - csn.bv_val;
	csn.bv_val = p;

	p = ber_bvchr( &csn, '#' );
	if ( !p )
		return -1;
	p++;
	csn.bv_len -= p - csn.bv_val;
	csn.bv_val = p;

	q = ber_bvchr( &csn, '#' );
	if ( !q )
		return -1;

	csn.bv_len = q - p;

	i = strtol( p, &q, 16 );
	if ( p == q || q != p + csn.bv_len || i < 0 || i > SLAP_SYNC_SID_MAX ) {
		i = -1;
	}

	return i;
}

int *
slap_parse_csn_sids( BerVarray csns, int numcsns, void *memctx )
{
	int i, *ret;

	ret = slap_sl_malloc( numcsns * sizeof(int), memctx );
	for ( i=0; i<numcsns; i++ ) {
		ret[i] = slap_parse_csn_sid( &csns[i] );
	}
	return ret;
}

int
slap_parse_sync_cookie(
	struct sync_cookie *cookie,
	void *memctx
)
{
	char *csn_ptr;
	char *csn_str;
	char *cval;
	char *next, *end;
	AttributeDescription *ad = slap_schema.si_ad_modifyTimestamp;

	if ( cookie == NULL )
		return -1;

	if ( cookie->octet_str.bv_len <= STRLENOF( "rid=" ) )
		return -1;

	cookie->rid = -1;
	cookie->sid = -1;
	cookie->ctxcsn = NULL;
	cookie->sids = NULL;
	cookie->numcsns = 0;

	end = cookie->octet_str.bv_val + cookie->octet_str.bv_len;

	for ( next=cookie->octet_str.bv_val; next < end; ) {
		if ( !strncmp( next, "rid=", STRLENOF("rid=") )) {
			char *rid_ptr = next;
			cookie->rid = strtol( &rid_ptr[ STRLENOF( "rid=" ) ], &next, 10 );
			if ( next == rid_ptr ||
				next > end ||
				( *next && *next != ',' ) ||
				cookie->rid < 0 ||
				cookie->rid > SLAP_SYNC_RID_MAX )
			{
				return -1;
			}
			if ( *next == ',' ) {
				next++;
			}
			if ( !ad ) {
				break;
			}
			continue;
		}
		if ( !strncmp( next, "sid=", STRLENOF("sid=") )) {
			char *sid_ptr = next;
			sid_ptr = next;
			cookie->sid = strtol( &sid_ptr[ STRLENOF( "sid=" ) ], &next, 16 );
			if ( next == sid_ptr ||
				next > end ||
				( *next && *next != ',' ) ||
				cookie->sid < 0 ||
				cookie->sid > SLAP_SYNC_SID_MAX )
			{
				return -1;
			}
			if ( *next == ',' ) {
				next++;
			}
			continue;
		}
		if ( !strncmp( next, "csn=", STRLENOF("csn=") )) {
			slap_syntax_validate_func *validate;
			struct berval stamp;

			next += STRLENOF("csn=");
			while ( next < end ) {
				csn_str = next;
				/* FIXME use csnValidate when it gets implemented */
				csn_ptr = strchr( csn_str, '#' );
				if ( !csn_ptr || csn_ptr > end )
					break;
				/* ad will be NULL when called from main. we just
				 * want to parse the rid then. But we still iterate
				 * through the string to find the end.
				 */
				if ( ad ) {
					stamp.bv_val = csn_str;
					stamp.bv_len = csn_ptr - csn_str;
					validate = ad->ad_type->sat_syntax->ssyn_validate;
					if ( validate( ad->ad_type->sat_syntax, &stamp )
						!= LDAP_SUCCESS )
						break;
				}
				cval = strchr( csn_ptr, ';' );
				if ( !cval )
					cval = strchr(csn_ptr, ',' );
				if ( cval )
					stamp.bv_len = cval - csn_str;
				else
					stamp.bv_len = end - csn_str;
				if ( ad ) {
					struct berval bv;
					ber_dupbv_x( &bv, &stamp, memctx );
					ber_bvarray_add_x( &cookie->ctxcsn, &bv, memctx );
					cookie->numcsns++;
				}
				if ( cval ) {
					next = cval + 1;
					if ( *cval != ';' )
						break;
				} else {
					next = end;
					break;
				}
			}
			continue;
		}
		next++;
	}
	if ( cookie->numcsns ) {
		cookie->sids = slap_parse_csn_sids( cookie->ctxcsn, cookie->numcsns,
			memctx );
	}
	return 0;
}

int
slap_init_sync_cookie_ctxcsn(
	struct sync_cookie *cookie
)
{
	char csnbuf[ LDAP_LUTIL_CSNSTR_BUFSIZE + 4 ];
	struct berval octet_str = BER_BVNULL;
	struct berval ctxcsn = BER_BVNULL;

	if ( cookie == NULL )
		return -1;

	octet_str.bv_len = snprintf( csnbuf, LDAP_LUTIL_CSNSTR_BUFSIZE + 4,
					"csn=%4d%02d%02d%02d%02d%02dZ#%06x#%02x#%06x",
					1900, 1, 1, 0, 0, 0, 0, 0, 0 );
	octet_str.bv_val = csnbuf;
	ch_free( cookie->octet_str.bv_val );
	ber_dupbv( &cookie->octet_str, &octet_str );

	ctxcsn.bv_val = octet_str.bv_val + 4;
	ctxcsn.bv_len = octet_str.bv_len - 4;
	cookie->ctxcsn = NULL;
	value_add_one( &cookie->ctxcsn, &ctxcsn );
	cookie->numcsns = 1;
	cookie->sid = -1;

	return 0;
}

struct sync_cookie *
slap_dup_sync_cookie(
	struct sync_cookie *dst,
	struct sync_cookie *src
)
{
	struct sync_cookie *new;
	int i;

	if ( src == NULL )
		return NULL;

	if ( dst ) {
		ber_bvarray_free( dst->ctxcsn );
		dst->ctxcsn = NULL;
		dst->sids = NULL;
		ch_free( dst->octet_str.bv_val );
		BER_BVZERO( &dst->octet_str );
		new = dst;
	} else {
		new = ( struct sync_cookie * )
				ch_calloc( 1, sizeof( struct sync_cookie ));
	}

	new->rid = src->rid;
	new->sid = src->sid;
	new->numcsns = src->numcsns;

	if ( src->numcsns ) {
		if ( ber_bvarray_dup_x( &new->ctxcsn, src->ctxcsn, NULL )) {
			if ( !dst ) {
				ch_free( new );
			}
			return NULL;
		}
		new->sids = ch_malloc( src->numcsns * sizeof(int) );
		for (i=0; i<src->numcsns; i++)
			new->sids[i] = src->sids[i];
	}

	if ( !BER_BVISNULL( &src->octet_str )) {
		ber_dupbv( &new->octet_str, &src->octet_str );
	}

	return new;
}

