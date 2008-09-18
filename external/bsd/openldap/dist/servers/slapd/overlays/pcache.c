/* $OpenLDAP: pkg/ldap/servers/slapd/overlays/pcache.c,v 1.88.2.17 2008/07/08 21:09:37 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * Portions Copyright 2003 IBM Corporation.
 * Portions Copyright 2003 Symas Corporation.
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
 * This work was initially developed by Apurva Kumar for inclusion
 * in OpenLDAP Software and subsequently rewritten by Howard Chu.
 */

#include "portable.h"

#ifdef SLAPD_OVER_PROXYCACHE

#include <stdio.h>

#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lutil.h"
#include "ldap_rq.h"
#include "avl.h"

#include "config.h"

#ifdef LDAP_DEVEL
/*
 * Control that allows to access the private DB
 * instead of the public one
 */
#define	PCACHE_CONTROL_PRIVDB		"1.3.6.1.4.1.4203.666.11.9.5.1"

/*
 * Extended Operation that allows to remove a query from the cache
 */
#define PCACHE_EXOP_QUERY_DELETE	"1.3.6.1.4.1.4203.666.11.9.6.1"
#endif

/* query cache structs */
/* query */

typedef struct Query_s {
	Filter* 	filter; 	/* Search Filter */
	struct berval 	base; 		/* Search Base */
	int 		scope;		/* Search scope */
} Query;

struct query_template_s;

typedef struct Qbase_s {
	Avlnode *scopes[4];		/* threaded AVL trees of cached queries */
	struct berval base;
	int queries;
} Qbase;

/* struct representing a cached query */
typedef struct cached_query_s {
	Filter					*filter;
	Filter					*first;
	Qbase					*qbase;
	int						scope;
	struct berval			q_uuid;		/* query identifier */
	int						q_sizelimit;
	struct query_template_s		*qtemp;	/* template of the query */
	time_t						expiry_time;	/* time till the query is considered valid */
	struct cached_query_s  		*next;  	/* next query in the template */
	struct cached_query_s  		*prev;  	/* previous query in the template */
	struct cached_query_s		*lru_up;	/* previous query in the LRU list */
	struct cached_query_s		*lru_down;	/* next query in the LRU list */
	ldap_pvt_thread_rdwr_t		rwlock;
} CachedQuery;

/*
 * URL representation:
 *
 * ldap:///<base>??<scope>?<filter>?x-uuid=<uid>,x-template=<template>,x-attrset=<attrset>,x-expiry=<expiry>
 *
 * <base> ::= CachedQuery.qbase->base
 * <scope> ::= CachedQuery.scope
 * <filter> ::= filter2bv(CachedQuery.filter)
 * <uuid> ::= CachedQuery.q_uuid
 * <attrset> ::= CachedQuery.qtemp->attr_set_index
 * <expiry> ::= CachedQuery.expiry_time
 *
 * quick hack: parse URI, call add_query() and then fix
 * CachedQuery.expiry_time and CachedQuery.q_uuid
 */

/*
 * Represents a set of projected attributes.
 */

struct attr_set {
	struct query_template_s *templates;
	AttributeName*	attrs; 		/* specifies the set */
	unsigned	flags;
#define	PC_CONFIGURED	(0x1)
#define	PC_REFERENCED	(0x2)
#define	PC_GOT_OC		(0x4)
	int 		count;		/* number of attributes */
};

/* struct representing a query template
 * e.g. template string = &(cn=)(mail=)
 */
typedef struct query_template_s {
	struct query_template_s *qtnext;
	struct query_template_s *qmnext;

	Avlnode*		qbase;
	CachedQuery* 	query;	        /* most recent query cached for the template */
	CachedQuery* 	query_last;     /* oldest query cached for the template */
	ldap_pvt_thread_rdwr_t t_rwlock; /* Rd/wr lock for accessing queries in the template */
	struct berval	querystr;	/* Filter string corresponding to the QT */

	int 		attr_set_index; /* determines the projected attributes */
	int 		no_of_queries;  /* Total number of queries in the template */
	time_t		ttl;		/* TTL for the queries of this template */
	time_t		negttl;		/* TTL for negative results */
	time_t		limitttl;	/* TTL for sizelimit exceeding results */
	struct attr_set t_attrs;	/* filter attrs + attr_set */
} QueryTemplate;

typedef enum {
	PC_IGNORE = 0,
	PC_POSITIVE,
	PC_NEGATIVE,
	PC_SIZELIMIT
} pc_caching_reason_t;

static const char *pc_caching_reason_str[] = {
	"IGNORE",
	"POSITIVE",
	"NEGATIVE",
	"SIZELIMIT",

	NULL
};

struct query_manager_s;

/* prototypes for functions for 1) query containment
 * 2) query addition, 3) cache replacement
 */
typedef CachedQuery *(QCfunc)(Operation *op, struct query_manager_s*,
	Query*, QueryTemplate*);
typedef CachedQuery *(AddQueryfunc)(Operation *op, struct query_manager_s*,
	Query*, QueryTemplate*, pc_caching_reason_t, int wlock);
typedef void (CRfunc)(struct query_manager_s*, struct berval*);

/* LDAP query cache */
typedef struct query_manager_s {
	struct attr_set* 	attr_sets;		/* possible sets of projected attributes */
	QueryTemplate*	  	templates;		/* cacheable templates */

	CachedQuery*		lru_top;		/* top and bottom of LRU list */
	CachedQuery*		lru_bottom;

	ldap_pvt_thread_mutex_t		lru_mutex;	/* mutex for accessing LRU list */

	/* Query cache methods */
	QCfunc			*qcfunc;			/* Query containment*/
	CRfunc 			*crfunc;			/* cache replacement */
	AddQueryfunc	*addfunc;			/* add query */
} query_manager;

/* LDAP query cache manager */
typedef struct cache_manager_s {
	BackendDB	db;	/* underlying database */
	unsigned long	num_cached_queries; 		/* total number of cached queries */
	unsigned long   max_queries;			/* upper bound on # of cached queries */
	int		save_queries;			/* save cached queries across restarts */
	int 	numattrsets;			/* number of attribute sets */
	int 	cur_entries;			/* current number of entries cached */
	int 	max_entries;			/* max number of entries cached */
	int 	num_entries_limit;		/* max # of entries in a cacheable query */

	char	response_cb;			/* install the response callback
						 * at the tail of the callback list */
#define PCACHE_RESPONSE_CB_HEAD	0
#define PCACHE_RESPONSE_CB_TAIL	1
	char	defer_db_open;			/* defer open for online add */

	time_t	cc_period;		/* interval between successive consistency checks (sec) */
	int 	cc_paused;
	void	*cc_arg;

	ldap_pvt_thread_mutex_t		cache_mutex;

	query_manager*   qm;	/* query cache managed by the cache manager */
} cache_manager;

static int pcache_debug;

#ifdef PCACHE_CONTROL_PRIVDB
static int privDB_cid;
#endif /* PCACHE_CONTROL_PRIVDB */

static AttributeDescription *ad_queryId, *ad_cachedQueryURL;
static struct {
	char	*desc;
	AttributeDescription **adp;
} as[] = {
	{ "( 1.3.6.1.4.1.4203.666.11.9.1.1 "
		"NAME 'queryId' "
		"DESC 'ID of query the entry belongs to, formatted as a UUID' "
		"EQUALITY octetStringMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{64} "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )",
		&ad_queryId },
	{ "( 1.3.6.1.4.1.4203.666.11.9.1.2 "
		"NAME 'cachedQueryURL' "
		"DESC 'URI describing a cached query' "
		"EQUALITY caseExactMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )",
		&ad_cachedQueryURL },
	{ NULL }
};

static int
filter2template(
	Operation		*op,
	Filter			*f,
	struct			berval *fstr,
	AttributeName**		filter_attrs,
	int*			filter_cnt,
	int*			filter_got_oc );

static CachedQuery *
add_query(
	Operation *op,
	query_manager* qm,
	Query* query,
	QueryTemplate *templ,
	pc_caching_reason_t why,
	int wlock);

static int
remove_query_data(
	Operation	*op,
	SlapReply	*rs,
	struct berval	*query_uuid );

/*
 * Turn a cached query into its URL representation
 */
static int
query2url( Operation *op, CachedQuery *q, struct berval *urlbv )
{
	struct berval	bv_scope,
			bv_filter;
	char		attrset_buf[ 32 ],
			expiry_buf[ 32 ],
			*ptr;
	ber_len_t	attrset_len,
			expiry_len;

	ldap_pvt_scope2bv( q->scope, &bv_scope );
	filter2bv_x( op, q->filter, &bv_filter );
	attrset_len = snprintf( attrset_buf, sizeof( attrset_buf ),
		"%lu", (unsigned long)q->qtemp->attr_set_index );
	expiry_len = snprintf( expiry_buf, sizeof( expiry_buf ),
		"%lu", (unsigned long)q->expiry_time );

	urlbv->bv_len = STRLENOF( "ldap:///" )
		+ q->qbase->base.bv_len
		+ STRLENOF( "??" )
		+ bv_scope.bv_len
		+ STRLENOF( "?" )
		+ bv_filter.bv_len
		+ STRLENOF( "?x-uuid=" )
		+ q->q_uuid.bv_len
		+ STRLENOF( ",x-attrset=" )
		+ attrset_len
		+ STRLENOF( ",x-expiry=" )
		+ expiry_len;
	ptr = urlbv->bv_val = ber_memalloc_x( urlbv->bv_len + 1, op->o_tmpmemctx );
	ptr = lutil_strcopy( ptr, "ldap:///" );
	ptr = lutil_strcopy( ptr, q->qbase->base.bv_val );
	ptr = lutil_strcopy( ptr, "??" );
	ptr = lutil_strcopy( ptr, bv_scope.bv_val );
	ptr = lutil_strcopy( ptr, "?" );
	ptr = lutil_strcopy( ptr, bv_filter.bv_val );
	ptr = lutil_strcopy( ptr, "?x-uuid=" );
	ptr = lutil_strcopy( ptr, q->q_uuid.bv_val );
	ptr = lutil_strcopy( ptr, ",x-attrset=" );
	ptr = lutil_strcopy( ptr, attrset_buf );
	ptr = lutil_strcopy( ptr, ",x-expiry=" );
	ptr = lutil_strcopy( ptr, expiry_buf );

	ber_memfree_x( bv_filter.bv_val, op->o_tmpmemctx );

	return 0;
}

/*
 * Turn an URL representing a formerly cached query into a cached query,
 * and try to cache it
 */
static int
url2query(
	char		*url,
	Operation	*op,
	query_manager	*qm )
{
	Query		query = { 0 };
	QueryTemplate	*qt;
	CachedQuery	*cq;
	LDAPURLDesc	*lud = NULL;
	struct berval	base,
			tempstr = BER_BVNULL,
			uuid;
	int		attrset;
	time_t		expiry_time;
	int		i,
			got_uuid = 0,
			got_attrset = 0,
			got_expiry = 0,
			rc = 0;

	rc = ldap_url_parse( url, &lud );
	if ( rc != LDAP_URL_SUCCESS ) {
		return -1;
	}

	/* non-allowed fields */
	if ( lud->lud_host != NULL ) {
		rc = 1;
		goto error;
	}

	if ( lud->lud_attrs != NULL ) {
		rc = 1;
		goto error;
	}

	/* be pedantic */
	if ( strcmp( lud->lud_scheme, "ldap" ) != 0 ) {
		rc = 1;
		goto error;
	}

	/* required fields */
	if ( lud->lud_dn == NULL || lud->lud_dn[ 0 ] == '\0' ) {
		rc = 1;
		goto error;
	}

	switch ( lud->lud_scope ) {
	case LDAP_SCOPE_BASE:
	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBTREE:
	case LDAP_SCOPE_SUBORDINATE:
		break;

	default:
		rc = 1;
		goto error;
	}

	if ( lud->lud_filter == NULL || lud->lud_filter[ 0 ] == '\0' ) {
		rc = 1;
		goto error;
	}

	if ( lud->lud_exts == NULL ) {
		rc = 1;
		goto error;
	}

	for ( i = 0; lud->lud_exts[ i ] != NULL; i++ ) {
		if ( strncmp( lud->lud_exts[ i ], "x-uuid=", STRLENOF( "x-uuid=" ) ) == 0 ) {
			struct berval	tmpUUID;
			Syntax		*syn_UUID = slap_schema.si_ad_entryUUID->ad_type->sat_syntax;

			ber_str2bv( &lud->lud_exts[ i ][ STRLENOF( "x-uuid=" ) ], 0, 0, &tmpUUID );
			rc = syn_UUID->ssyn_pretty( syn_UUID, &tmpUUID, &uuid, NULL );
			if ( rc != LDAP_SUCCESS ) {
				goto error;
			}
			got_uuid = 1;

		} else if ( strncmp( lud->lud_exts[ i ], "x-attrset=", STRLENOF( "x-attrset=" ) ) == 0 ) {
			rc = lutil_atoi( &attrset, &lud->lud_exts[ i ][ STRLENOF( "x-attrset=" ) ] );
			if ( rc ) {
				goto error;
			}
			got_attrset = 1;

		} else if ( strncmp( lud->lud_exts[ i ], "x-expiry=", STRLENOF( "x-expiry=" ) ) == 0 ) {
			unsigned long l;

			rc = lutil_atoul( &l, &lud->lud_exts[ i ][ STRLENOF( "x-expiry=" ) ] );
			if ( rc ) {
				goto error;
			}
			expiry_time = (time_t)l;
			got_expiry = 1;

		} else {
			rc = -1;
			goto error;
		}
	}

	if ( !got_uuid ) {
		rc = 1;
		goto error;
	}

	if ( !got_attrset ) {
		rc = 1;
		goto error;
	}

	if ( !got_expiry ) {
		rc = 1;
		goto error;
	}

	/* ignore expired queries */
	if ( expiry_time <= slap_get_time()) {
		Operation	op2 = *op;
		SlapReply	rs2 = { 0 };

		memset( &op2.oq_search, 0, sizeof( op2.oq_search ) );

		(void)remove_query_data( &op2, &rs2, &uuid );

		rc = 0;

	} else {
		ber_str2bv( lud->lud_dn, 0, 0, &base );
		rc = dnNormalize( 0, NULL, NULL, &base, &query.base, NULL );
		if ( rc != LDAP_SUCCESS ) {
			goto error;
		}
		query.scope = lud->lud_scope;
		query.filter = str2filter( lud->lud_filter );

		tempstr.bv_val = ch_malloc( strlen( lud->lud_filter ) + 1 );
		tempstr.bv_len = 0;
		if ( filter2template( op, query.filter, &tempstr, NULL, NULL, NULL ) ) {
			ch_free( tempstr.bv_val );
			rc = -1;
			goto error;
		}

		/* check for query containment */
		qt = qm->attr_sets[attrset].templates;
		for ( ; qt; qt = qt->qtnext ) {
			/* find if template i can potentially answer tempstr */
			if ( bvmatch( &qt->querystr, &tempstr ) ) {
				break;
			}
		}

		if ( qt == NULL ) {
			rc = 1;
			goto error;
		}

		cq = add_query( op, qm, &query, qt, PC_POSITIVE, 0 );
		if ( cq != NULL ) {
			cq->expiry_time = expiry_time;
			cq->q_uuid = uuid;

			/* it's now into cq->filter */
			BER_BVZERO( &uuid );
			query.filter = NULL;

		} else {
			rc = 1;
		}
	}

error:;
	if ( query.filter != NULL ) filter_free( query.filter );
	if ( !BER_BVISNULL( &tempstr ) ) ch_free( tempstr.bv_val );
	if ( !BER_BVISNULL( &query.base ) ) ch_free( query.base.bv_val );
	if ( !BER_BVISNULL( &uuid ) ) ch_free( uuid.bv_val );
	if ( lud != NULL ) ldap_free_urldesc( lud );

	return rc;
}

/* Return 1 for an added entry, else 0 */
static int
merge_entry(
	Operation		*op,
	Entry			*e,
	struct berval*		query_uuid )
{
	int		rc;
	Modifications* modlist = NULL;
	const char* 	text = NULL;
	Attribute		*attr;
	char			textbuf[SLAP_TEXT_BUFLEN];
	size_t			textlen = sizeof(textbuf);

	SlapReply sreply = {REP_RESULT};

	slap_callback cb = { NULL, slap_null_cb, NULL, NULL };

	attr = e->e_attrs;
	e->e_attrs = NULL;

	/* add queryId attribute */
	attr_merge_one( e, ad_queryId, query_uuid, NULL );

	/* append the attribute list from the fetched entry */
	e->e_attrs->a_next = attr;

	op->o_tag = LDAP_REQ_ADD;
	op->o_protocol = LDAP_VERSION3;
	op->o_callback = &cb;
	op->o_time = slap_get_time();
	op->o_do_not_cache = 1;

	op->ora_e = e;
	op->o_req_dn = e->e_name;
	op->o_req_ndn = e->e_nname;
	rc = op->o_bd->be_add( op, &sreply );

	if ( rc != LDAP_SUCCESS ) {
		if ( rc == LDAP_ALREADY_EXISTS ) {
			slap_entry2mods( e, &modlist, &text, textbuf, textlen );
			modlist->sml_op = LDAP_MOD_ADD;
			op->o_tag = LDAP_REQ_MODIFY;
			op->orm_modlist = modlist;
			op->o_bd->be_modify( op, &sreply );
			slap_mods_free( modlist, 1 );
		} else if ( rc == LDAP_REFERRAL ||
					rc == LDAP_NO_SUCH_OBJECT ) {
			syncrepl_add_glue( op, e );
			e = NULL;
			rc = 1;
		}
		if ( e ) {
			entry_free( e );
			rc = 0;
		}
	} else {
		if ( op->ora_e == e )
			be_entry_release_w( op, e );
		rc = 1;
	}

	return rc;
}

/* Length-ordered sort on normalized DNs */
static int pcache_dn_cmp( const void *v1, const void *v2 )
{
	const Qbase *q1 = v1, *q2 = v2;

	int rc = q1->base.bv_len - q2->base.bv_len;
	if ( rc == 0 )
		rc = strncmp( q1->base.bv_val, q2->base.bv_val, q1->base.bv_len );
	return rc;
}

static int lex_bvcmp( struct berval *bv1, struct berval *bv2 )
{
	int len, dif;
	dif = bv1->bv_len - bv2->bv_len;
	len = bv1->bv_len;
	if ( dif > 0 ) len -= dif;
	len = memcmp( bv1->bv_val, bv2->bv_val, len );
	if ( !len )
		len = dif;
	return len;
}

/* compare the first value in each filter */
static int pcache_filter_cmp( const void *v1, const void *v2 )
{
	const CachedQuery *q1 = v1, *q2 =v2;
	int rc, weight1, weight2;

	switch( q1->first->f_choice ) {
	case LDAP_FILTER_PRESENT:
		weight1 = 0;
		break;
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
		weight1 = 1;
		break;
	default:
		weight1 = 2;
	}
	switch( q2->first->f_choice ) {
	case LDAP_FILTER_PRESENT:
		weight2 = 0;
		break;
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
		weight2 = 1;
		break;
	default:
		weight2 = 2;
	}
	rc = weight1 - weight2;
	if ( !rc ) {
		switch( weight1 ) {
		case 0:	return 0;
		case 1:
			rc = lex_bvcmp( &q1->first->f_av_value, &q2->first->f_av_value );
			break;
		case 2:
			if ( q1->first->f_choice == LDAP_FILTER_SUBSTRINGS ) {
				rc = 0;
				if ( !BER_BVISNULL( &q1->first->f_sub_initial )) {
					if ( !BER_BVISNULL( &q2->first->f_sub_initial )) {
						rc = lex_bvcmp( &q1->first->f_sub_initial,
							&q2->first->f_sub_initial );
					} else {
						rc = 1;
					}
				} else if ( !BER_BVISNULL( &q2->first->f_sub_initial )) {
					rc = -1;
				}
				if ( rc ) break;
				if ( q1->first->f_sub_any ) {
					if ( q2->first->f_sub_any ) {
						rc = lex_bvcmp( q1->first->f_sub_any,
							q2->first->f_sub_any );
					} else {
						rc = 1;
					}
				} else if ( q2->first->f_sub_any ) {
					rc = -1;
				}
				if ( rc ) break;
				if ( !BER_BVISNULL( &q1->first->f_sub_final )) {
					if ( !BER_BVISNULL( &q2->first->f_sub_final )) {
						rc = lex_bvcmp( &q1->first->f_sub_final,
							&q2->first->f_sub_final );
					} else {
						rc = 1;
					}
				} else if ( !BER_BVISNULL( &q2->first->f_sub_final )) {
					rc = -1;
				}
			} else {
				rc = lex_bvcmp( &q1->first->f_mr_value,
					&q2->first->f_mr_value );
			}
			break;
		}
	}

	return rc;
}

/* add query on top of LRU list */
static void
add_query_on_top (query_manager* qm, CachedQuery* qc)
{
	CachedQuery* top = qm->lru_top;

	qm->lru_top = qc;

	if (top)
		top->lru_up = qc;
	else
		qm->lru_bottom = qc;

	qc->lru_down = top;
	qc->lru_up = NULL;
	Debug( pcache_debug, "Base of added query = %s\n",
			qc->qbase->base.bv_val, 0, 0 );
}

/* remove_query from LRU list */

static void
remove_query (query_manager* qm, CachedQuery* qc)
{
	CachedQuery* up;
	CachedQuery* down;

	if (!qc)
		return;

	up = qc->lru_up;
	down = qc->lru_down;

	if (!up)
		qm->lru_top = down;

	if (!down)
		qm->lru_bottom = up;

	if (down)
		down->lru_up = up;

	if (up)
		up->lru_down = down;

	qc->lru_up = qc->lru_down = NULL;
}

/* find and remove string2 from string1
 * from start if position = 1,
 * from end if position = 3,
 * from anywhere if position = 2
 * string1 is overwritten if position = 2.
 */

static int
find_and_remove(struct berval* ber1, struct berval* ber2, int position)
{
	int ret=0;

	if ( !ber2->bv_val )
		return 1;
	if ( !ber1->bv_val )
		return 0;

	switch( position ) {
	case 1:
		if ( ber1->bv_len >= ber2->bv_len && !memcmp( ber1->bv_val,
			ber2->bv_val, ber2->bv_len )) {
			ret = 1;
			ber1->bv_val += ber2->bv_len;
			ber1->bv_len -= ber2->bv_len;
		}
		break;
	case 2: {
		char *temp;
		ber1->bv_val[ber1->bv_len] = '\0';
		temp = strstr( ber1->bv_val, ber2->bv_val );
		if ( temp ) {
			strcpy( temp, temp+ber2->bv_len );
			ber1->bv_len -= ber2->bv_len;
			ret = 1;
		}
		break;
		}
	case 3:
		if ( ber1->bv_len >= ber2->bv_len &&
			!memcmp( ber1->bv_val+ber1->bv_len-ber2->bv_len, ber2->bv_val,
				ber2->bv_len )) {
			ret = 1;
			ber1->bv_len -= ber2->bv_len;
		}
		break;
	}
	return ret;
}


static struct berval*
merge_init_final(Operation *op, struct berval* init, struct berval* any,
	struct berval* final)
{
	struct berval* merged, *temp;
	int i, any_count, count;

	for (any_count=0; any && any[any_count].bv_val; any_count++)
		;

	count = any_count;

	if (init->bv_val)
		count++;
	if (final->bv_val)
		count++;

	merged = (struct berval*)op->o_tmpalloc( (count+1)*sizeof(struct berval),
		op->o_tmpmemctx );
	temp = merged;

	if (init->bv_val) {
		ber_dupbv_x( temp, init, op->o_tmpmemctx );
		temp++;
	}

	for (i=0; i<any_count; i++) {
		ber_dupbv_x( temp, any, op->o_tmpmemctx );
		temp++; any++;
	}

	if (final->bv_val){
		ber_dupbv_x( temp, final, op->o_tmpmemctx );
		temp++;
	}
	BER_BVZERO( temp );
	return merged;
}

/* Each element in stored must be found in incoming. Incoming is overwritten.
 */
static int
strings_containment(struct berval* stored, struct berval* incoming)
{
	struct berval* element;
	int k=0;
	int j, rc = 0;

	for ( element=stored; element->bv_val != NULL; element++ ) {
		for (j = k; incoming[j].bv_val != NULL; j++) {
			if (find_and_remove(&(incoming[j]), element, 2)) {
				k = j;
				rc = 1;
				break;
			}
			rc = 0;
		}
		if ( rc ) {
			continue;
		} else {
			return 0;
		}
	}
	return 1;
}

static int
substr_containment_substr(Operation *op, Filter* stored, Filter* incoming)
{
	int rc = 0;

	struct berval init_incoming;
	struct berval final_incoming;
	struct berval *remaining_incoming = NULL;

	if ((!(incoming->f_sub_initial.bv_val) && (stored->f_sub_initial.bv_val))
	   || (!(incoming->f_sub_final.bv_val) && (stored->f_sub_final.bv_val)))
		return 0;

	init_incoming = incoming->f_sub_initial;
	final_incoming =  incoming->f_sub_final;

	if (find_and_remove(&init_incoming,
			&(stored->f_sub_initial), 1) && find_and_remove(&final_incoming,
			&(stored->f_sub_final), 3))
	{
		if (stored->f_sub_any == NULL) {
			rc = 1;
			goto final;
		}
		remaining_incoming = merge_init_final(op, &init_incoming,
						incoming->f_sub_any, &final_incoming);
		rc = strings_containment(stored->f_sub_any, remaining_incoming);
		ber_bvarray_free_x( remaining_incoming, op->o_tmpmemctx );
	}
final:
	return rc;
}

static int
substr_containment_equality(Operation *op, Filter* stored, Filter* incoming)
{
	struct berval incoming_val[2];
	int rc = 0;

	incoming_val[1] = incoming->f_av_value;

	if (find_and_remove(incoming_val+1,
			&(stored->f_sub_initial), 1) && find_and_remove(incoming_val+1,
			&(stored->f_sub_final), 3)) {
		if (stored->f_sub_any == NULL){
			rc = 1;
			goto final;
		}
		ber_dupbv_x( incoming_val, incoming_val+1, op->o_tmpmemctx );
		BER_BVZERO( incoming_val+1 );
		rc = strings_containment(stored->f_sub_any, incoming_val);
		op->o_tmpfree( incoming_val[0].bv_val, op->o_tmpmemctx );
	}
final:
	return rc;
}

static Filter *
filter_first( Filter *f )
{
	while ( f->f_choice == LDAP_FILTER_OR || f->f_choice == LDAP_FILTER_AND )
		f = f->f_and;
	return f;
}


static CachedQuery *
find_filter( Operation *op, Avlnode *root, Filter *inputf, Filter *first )
{
	Filter* fs;
	Filter* fi;
	MatchingRule* mrule = NULL;
	int res=0, eqpass= 0;
	int ret, rc, dir;
	Avlnode *ptr;
	CachedQuery cq, *qc;

	cq.filter = inputf;
	cq.first = first;

	/* substring matches sort to the end, and we just have to
	 * walk the entire list.
	 */
	if ( first->f_choice == LDAP_FILTER_SUBSTRINGS ) {
		ptr = tavl_end( root, 1 );
		dir = TAVL_DIR_LEFT;
	} else {
		ptr = tavl_find3( root, &cq, pcache_filter_cmp, &ret );
		dir = (first->f_choice == LDAP_FILTER_GE) ? TAVL_DIR_LEFT :
			TAVL_DIR_RIGHT;
	}

	while (ptr) {
		qc = ptr->avl_data;
		fi = inputf;
		fs = qc->filter;

		/* an incoming substr query can only be satisfied by a cached
		 * substr query.
		 */
		if ( first->f_choice == LDAP_FILTER_SUBSTRINGS &&
			qc->first->f_choice != LDAP_FILTER_SUBSTRINGS )
			break;

		/* an incoming eq query can be satisfied by a cached eq or substr
		 * query
		 */
		if ( first->f_choice == LDAP_FILTER_EQUALITY ) {
			if ( eqpass == 0 ) {
				if ( qc->first->f_choice != LDAP_FILTER_EQUALITY ) {
nextpass:			eqpass = 1;
					ptr = tavl_end( root, 1 );
					dir = TAVL_DIR_LEFT;
					continue;
				}
			} else {
				if ( qc->first->f_choice != LDAP_FILTER_SUBSTRINGS )
					break;
			}
		}
		do {
			res=0;
			switch (fs->f_choice) {
			case LDAP_FILTER_EQUALITY:
				if (fi->f_choice == LDAP_FILTER_EQUALITY)
					mrule = fs->f_ava->aa_desc->ad_type->sat_equality;
				else
					ret = 1;
				break;
			case LDAP_FILTER_GE:
			case LDAP_FILTER_LE:
				mrule = fs->f_ava->aa_desc->ad_type->sat_ordering;
				break;
			default:
				mrule = NULL; 
			}
			if (mrule) {
				const char *text;
				rc = value_match(&ret, fs->f_ava->aa_desc, mrule,
					SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
					&(fi->f_ava->aa_value),
					&(fs->f_ava->aa_value), &text);
				if (rc != LDAP_SUCCESS) {
					return NULL;
				}
				if ( fi==first && fi->f_choice==LDAP_FILTER_EQUALITY && ret )
					goto nextpass;
			}
			switch (fs->f_choice) {
			case LDAP_FILTER_OR:
			case LDAP_FILTER_AND:
				fs = fs->f_and;
				fi = fi->f_and;
				res=1;
				break;
			case LDAP_FILTER_SUBSTRINGS:
				/* check if the equality query can be
				* answered with cached substring query */
				if ((fi->f_choice == LDAP_FILTER_EQUALITY)
					&& substr_containment_equality( op,
					fs, fi))
					res=1;
				/* check if the substring query can be
				* answered with cached substring query */
				if ((fi->f_choice ==LDAP_FILTER_SUBSTRINGS
					) && substr_containment_substr( op,
					fs, fi))
					res= 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_PRESENT:
				res=1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_EQUALITY:
				if (ret == 0)
					res = 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_GE:
				if (mrule && ret >= 0)
					res = 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_LE:
				if (mrule && ret <= 0)
					res = 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_NOT:
				res=0;
				break;
			default:
				break;
			}
		} while((res) && (fi != NULL) && (fs != NULL));

		if ( res )
			return qc;
		ptr = tavl_next( ptr, dir );
	}
	return NULL;
}

/* check whether query is contained in any of
 * the cached queries in template
 */
static CachedQuery *
query_containment(Operation *op, query_manager *qm,
		  Query *query,
		  QueryTemplate *templa)
{
	CachedQuery* qc;
	int depth = 0, tscope;
	Qbase qbase, *qbptr = NULL;
	struct berval pdn;

	if (query->filter != NULL) {
		Filter *first;

		Debug( pcache_debug, "Lock QC index = %p\n",
				(void *) templa, 0, 0 );
		qbase.base = query->base;

		first = filter_first( query->filter );

		ldap_pvt_thread_rdwr_rlock(&templa->t_rwlock);
		for( ;; ) {
			/* Find the base */
			qbptr = avl_find( templa->qbase, &qbase, pcache_dn_cmp );
			if ( qbptr ) {
				tscope = query->scope;
				/* Find a matching scope:
				 * match at depth 0 OK
				 * scope is BASE,
				 *	one at depth 1 OK
				 *  subord at depth > 0 OK
				 *	subtree at any depth OK
				 * scope is ONE,
				 *  subtree or subord at any depth OK
				 * scope is SUBORD,
				 *  subtree or subord at any depth OK
				 * scope is SUBTREE,
				 *  subord at depth > 0 OK
				 *  subtree at any depth OK
				 */
				for ( tscope = 0 ; tscope <= LDAP_SCOPE_CHILDREN; tscope++ ) {
					switch ( query->scope ) {
					case LDAP_SCOPE_BASE:
						if ( tscope == LDAP_SCOPE_BASE && depth ) continue;
						if ( tscope == LDAP_SCOPE_ONE && depth != 1) continue;
						if ( tscope == LDAP_SCOPE_CHILDREN && !depth ) continue;
						break;
					case LDAP_SCOPE_ONE:
						if ( tscope == LDAP_SCOPE_BASE )
							tscope = LDAP_SCOPE_ONE;
						if ( tscope == LDAP_SCOPE_ONE && depth ) continue;
						if ( !depth ) break;
						if ( tscope < LDAP_SCOPE_SUBTREE )
							tscope = LDAP_SCOPE_SUBTREE;
						break;
					case LDAP_SCOPE_SUBTREE:
						if ( tscope < LDAP_SCOPE_SUBTREE )
							tscope = LDAP_SCOPE_SUBTREE;
						if ( tscope == LDAP_SCOPE_CHILDREN && !depth ) continue;
						break;
					case LDAP_SCOPE_CHILDREN:
						if ( tscope < LDAP_SCOPE_SUBTREE )
							tscope = LDAP_SCOPE_SUBTREE;
						break;
					}
					if ( !qbptr->scopes[tscope] ) continue;

					/* Find filter */
					qc = find_filter( op, qbptr->scopes[tscope],
							query->filter, first );
					if ( qc ) {
						if ( qc->q_sizelimit ) {
							ldap_pvt_thread_rdwr_runlock(&templa->t_rwlock);
							return NULL;
						}
						ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
						if (qm->lru_top != qc) {
							remove_query(qm, qc);
							add_query_on_top(qm, qc);
						}
						ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
						return qc;
					}
				}
			}
			if ( be_issuffix( op->o_bd, &qbase.base ))
				break;
			/* Up a level */
			dnParent( &qbase.base, &pdn );
			qbase.base = pdn;
			depth++;
		}

		Debug( pcache_debug,
			"Not answerable: Unlock QC index=%p\n",
			(void *) templa, 0, 0 );
		ldap_pvt_thread_rdwr_runlock(&templa->t_rwlock);
	}
	return NULL;
}

static void
free_query (CachedQuery* qc)
{
	free(qc->q_uuid.bv_val);
	filter_free(qc->filter);
	free(qc);
}


/* Add query to query cache, the returned Query is locked for writing */
static CachedQuery *
add_query(
	Operation *op,
	query_manager* qm,
	Query* query,
	QueryTemplate *templ,
	pc_caching_reason_t why,
	int wlock)
{
	CachedQuery* new_cached_query = (CachedQuery*) ch_malloc(sizeof(CachedQuery));
	Qbase *qbase, qb;
	Filter *first;
	int rc;
	time_t ttl = 0;;

	new_cached_query->qtemp = templ;
	BER_BVZERO( &new_cached_query->q_uuid );
	new_cached_query->q_sizelimit = 0;

	switch ( why ) {
	case PC_POSITIVE:
		ttl = templ->ttl;
		break;

	case PC_NEGATIVE:
		ttl = templ->negttl;
		break;

	case PC_SIZELIMIT:
		ttl = templ->limitttl;
		break;

	default:
		assert( 0 );
		break;
	}
	new_cached_query->expiry_time = slap_get_time() + ttl;
	new_cached_query->lru_up = NULL;
	new_cached_query->lru_down = NULL;
	Debug( pcache_debug, "Added query expires at %ld (%s)\n",
			(long) new_cached_query->expiry_time,
			pc_caching_reason_str[ why ], 0 );

	new_cached_query->scope = query->scope;
	new_cached_query->filter = query->filter;
	new_cached_query->first = first = filter_first( query->filter );
	
	ldap_pvt_thread_rdwr_init(&new_cached_query->rwlock);
	if (wlock)
		ldap_pvt_thread_rdwr_wlock(&new_cached_query->rwlock);

	qb.base = query->base;

	/* Adding a query    */
	Debug( pcache_debug, "Lock AQ index = %p\n",
			(void *) templ, 0, 0 );
	ldap_pvt_thread_rdwr_wlock(&templ->t_rwlock);
	qbase = avl_find( templ->qbase, &qb, pcache_dn_cmp );
	if ( !qbase ) {
		qbase = ch_calloc( 1, sizeof(Qbase) + qb.base.bv_len + 1 );
		qbase->base.bv_len = qb.base.bv_len;
		qbase->base.bv_val = (char *)(qbase+1);
		memcpy( qbase->base.bv_val, qb.base.bv_val, qb.base.bv_len );
		qbase->base.bv_val[qbase->base.bv_len] = '\0';
		avl_insert( &templ->qbase, qbase, pcache_dn_cmp, avl_dup_error );
	}
	new_cached_query->next = templ->query;
	new_cached_query->prev = NULL;
	new_cached_query->qbase = qbase;
	rc = tavl_insert( &qbase->scopes[query->scope], new_cached_query,
		pcache_filter_cmp, avl_dup_error );
	if ( rc == 0 ) {
		qbase->queries++;
		if (templ->query == NULL)
			templ->query_last = new_cached_query;
		else
			templ->query->prev = new_cached_query;
		templ->query = new_cached_query;
		templ->no_of_queries++;
	} else {
		ch_free( new_cached_query );
		new_cached_query = find_filter( op, qbase->scopes[query->scope],
							query->filter, first );
		filter_free( query->filter );
	}
	Debug( pcache_debug, "TEMPLATE %p QUERIES++ %d\n",
			(void *) templ, templ->no_of_queries, 0 );

	Debug( pcache_debug, "Unlock AQ index = %p \n",
			(void *) templ, 0, 0 );
	ldap_pvt_thread_rdwr_wunlock(&templ->t_rwlock);

	/* Adding on top of LRU list  */
	if ( rc == 0 ) {
		ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
		add_query_on_top(qm, new_cached_query);
		ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
	}
	return rc == 0 ? new_cached_query : NULL;
}

static void
remove_from_template (CachedQuery* qc, QueryTemplate* template)
{
	if (!qc->prev && !qc->next) {
		template->query_last = template->query = NULL;
	} else if (qc->prev == NULL) {
		qc->next->prev = NULL;
		template->query = qc->next;
	} else if (qc->next == NULL) {
		qc->prev->next = NULL;
		template->query_last = qc->prev;
	} else {
		qc->next->prev = qc->prev;
		qc->prev->next = qc->next;
	}
	tavl_delete( &qc->qbase->scopes[qc->scope], qc, pcache_filter_cmp );
	qc->qbase->queries--;
	if ( qc->qbase->queries == 0 ) {
		avl_delete( &template->qbase, qc->qbase, pcache_dn_cmp );
		ch_free( qc->qbase );
		qc->qbase = NULL;
	}

	template->no_of_queries--;
}

/* remove bottom query of LRU list from the query cache */
/*
 * NOTE: slight change in functionality.
 *
 * - if result->bv_val is NULL, the query at the bottom of the LRU
 *   is removed
 * - otherwise, the query whose UUID is *result is removed
 *	- if not found, result->bv_val is zeroed
 */
static void
cache_replacement(query_manager* qm, struct berval *result)
{
	CachedQuery* bottom;
	QueryTemplate *temp;

	ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
	if ( BER_BVISNULL( result ) ) {
		bottom = qm->lru_bottom;

		if (!bottom) {
			Debug ( pcache_debug,
				"Cache replacement invoked without "
				"any query in LRU list\n", 0, 0, 0 );
			ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
			return;
		}

	} else {
		for ( bottom = qm->lru_bottom;
			bottom != NULL;
			bottom = bottom->lru_up )
		{
			if ( bvmatch( result, &bottom->q_uuid ) ) {
				break;
			}
		}

		if ( !bottom ) {
			Debug ( pcache_debug,
				"Could not find query with uuid=\"%s\""
				"in LRU list\n", result->bv_val, 0, 0 );
			ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
			BER_BVZERO( result );
			return;
		}
	}

	temp = bottom->qtemp;
	remove_query(qm, bottom);
	ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);

	*result = bottom->q_uuid;
	BER_BVZERO( &bottom->q_uuid );

	Debug( pcache_debug, "Lock CR index = %p\n", (void *) temp, 0, 0 );
	ldap_pvt_thread_rdwr_wlock(&temp->t_rwlock);
	remove_from_template(bottom, temp);
	Debug( pcache_debug, "TEMPLATE %p QUERIES-- %d\n",
		(void *) temp, temp->no_of_queries, 0 );
	Debug( pcache_debug, "Unlock CR index = %p\n", (void *) temp, 0, 0 );
	ldap_pvt_thread_rdwr_wunlock(&temp->t_rwlock);
	free_query(bottom);
}

struct query_info {
	struct query_info *next;
	struct berval xdn;
	int del;
};

static int
remove_func (
	Operation	*op,
	SlapReply	*rs
)
{
	Attribute *attr;
	struct query_info *qi;
	int count = 0;

	if ( rs->sr_type != REP_SEARCH ) return 0;

	attr = attr_find( rs->sr_entry->e_attrs,  ad_queryId );
	if ( attr == NULL ) return 0;

	count = attr->a_numvals;
	assert( count > 0 );
	qi = op->o_tmpalloc( sizeof( struct query_info ), op->o_tmpmemctx );
	qi->next = op->o_callback->sc_private;
	op->o_callback->sc_private = qi;
	ber_dupbv_x( &qi->xdn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
	qi->del = ( count == 1 );

	return 0;
}

static int
remove_query_data(
	Operation	*op,
	SlapReply	*rs,
	struct berval	*query_uuid )
{
	struct query_info	*qi, *qnext;
	char			filter_str[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(queryId=)" ) ];
	AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
	Filter			filter = {LDAP_FILTER_EQUALITY};
	SlapReply 		sreply = {REP_RESULT};
	slap_callback cb = { NULL, remove_func, NULL, NULL };
	int deleted = 0;

	sreply.sr_entry = NULL;
	sreply.sr_nentries = 0;
	op->ors_filterstr.bv_len = snprintf(filter_str, sizeof(filter_str),
		"(%s=%s)", ad_queryId->ad_cname.bv_val, query_uuid->bv_val);
	filter.f_ava = &ava;
	filter.f_av_desc = ad_queryId;
	filter.f_av_value = *query_uuid;

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_callback = &cb;
	op->o_time = slap_get_time();
	op->o_do_not_cache = 1;

	op->o_req_dn = op->o_bd->be_suffix[0];
	op->o_req_ndn = op->o_bd->be_nsuffix[0];
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_filter = &filter;
	op->ors_filterstr.bv_val = filter_str;
	op->ors_filterstr.bv_len = strlen(filter_str);
	op->ors_attrs = NULL;
	op->ors_attrsonly = 0;

	op->o_bd->be_search( op, &sreply );

	for ( qi=cb.sc_private; qi; qi=qnext ) {
		qnext = qi->next;

		op->o_req_dn = qi->xdn;
		op->o_req_ndn = qi->xdn;

		if ( qi->del ) {
			Debug( pcache_debug, "DELETING ENTRY TEMPLATE=%s\n",
				query_uuid->bv_val, 0, 0 );

			op->o_tag = LDAP_REQ_DELETE;

			if (op->o_bd->be_delete(op, &sreply) == LDAP_SUCCESS) {
				deleted++;
			}

		} else {
			Modifications mod;
			struct berval vals[2];

			vals[0] = *query_uuid;
			vals[1].bv_val = NULL;
			vals[1].bv_len = 0;
			mod.sml_op = LDAP_MOD_DELETE;
			mod.sml_flags = 0;
			mod.sml_desc = ad_queryId;
			mod.sml_type = ad_queryId->ad_cname;
			mod.sml_values = vals;
			mod.sml_nvalues = NULL;
                        mod.sml_numvals = 1;
			mod.sml_next = NULL;
			Debug( pcache_debug,
				"REMOVING TEMP ATTR : TEMPLATE=%s\n",
				query_uuid->bv_val, 0, 0 );

			op->orm_modlist = &mod;

			op->o_bd->be_modify( op, &sreply );
		}
		op->o_tmpfree( qi->xdn.bv_val, op->o_tmpmemctx );
		op->o_tmpfree( qi, op->o_tmpmemctx );
	}
	return deleted;
}

static int
get_attr_set(
	AttributeName* attrs,
	query_manager* qm,
	int num
);

static int
filter2template(
	Operation		*op,
	Filter			*f,
	struct			berval *fstr,
	AttributeName**		filter_attrs,
	int*			filter_cnt,
	int*			filter_got_oc )
{
	AttributeDescription *ad;
	int len, ret;

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		ad = f->f_av_desc;
		len = STRLENOF( "(=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s=)", ad->ad_cname.bv_val );
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_GE:
		ad = f->f_av_desc;
		len = STRLENOF( "(>=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s>=)", ad->ad_cname.bv_val);
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_LE:
		ad = f->f_av_desc;
		len = STRLENOF( "(<=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s<=)", ad->ad_cname.bv_val);
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_APPROX:
		ad = f->f_av_desc;
		len = STRLENOF( "(~=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s~=)", ad->ad_cname.bv_val);
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_SUBSTRINGS:
		ad = f->f_sub_desc;
		len = STRLENOF( "(=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s=)", ad->ad_cname.bv_val );
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_PRESENT:
		ad = f->f_desc;
		len = STRLENOF( "(=*)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s=*)", ad->ad_cname.bv_val );
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT: {
		int rc = 0;
		fstr->bv_val[fstr->bv_len++] = '(';
		switch ( f->f_choice ) {
		case LDAP_FILTER_AND:
			fstr->bv_val[fstr->bv_len] = '&';
			break;
		case LDAP_FILTER_OR:
			fstr->bv_val[fstr->bv_len] = '|';
			break;
		case LDAP_FILTER_NOT:
			fstr->bv_val[fstr->bv_len] = '!';
			break;
		}
		fstr->bv_len++;

		for ( f = f->f_list; f != NULL; f = f->f_next ) {
			rc = filter2template( op, f, fstr, filter_attrs, filter_cnt,
				filter_got_oc );
			if ( rc ) break;
		}
		fstr->bv_val[fstr->bv_len++] = ')';
		fstr->bv_val[fstr->bv_len] = '\0';

		return rc;
		}

	default:
		/* a filter should at least have room for "()",
		 * an "=" and for a 1-char attr */
		strcpy( fstr->bv_val, "(?=)" );
		fstr->bv_len += STRLENOF("(?=)");
		return -1;
	}

	if ( filter_attrs != NULL ) {
		*filter_attrs = (AttributeName *)op->o_tmprealloc(*filter_attrs,
				(*filter_cnt + 2)*sizeof(AttributeName), op->o_tmpmemctx);

		(*filter_attrs)[*filter_cnt].an_desc = ad;
		(*filter_attrs)[*filter_cnt].an_name = ad->ad_cname;
		(*filter_attrs)[*filter_cnt].an_oc = NULL;
		(*filter_attrs)[*filter_cnt].an_oc_exclude = 0;
		BER_BVZERO( &(*filter_attrs)[*filter_cnt+1].an_name );
		(*filter_cnt)++;
		if ( ad == slap_schema.si_ad_objectClass )
			*filter_got_oc = 1;
	}

	return 0;
}

struct search_info {
	slap_overinst *on;
	Query query;
	QueryTemplate *qtemp;
	AttributeName*  save_attrs;	/* original attributes, saved for response */
	int max;
	int over;
	int count;
	int slimit;
	int slimit_exceeded;
	pc_caching_reason_t caching_reason;
	Entry *head, *tail;
};

static void
remove_query_and_data(
	Operation	*op,
	SlapReply	*rs,
	cache_manager	*cm,
	struct berval	*uuid )
{
	query_manager*		qm = cm->qm;

	qm->crfunc( qm, uuid );
	if ( !BER_BVISNULL( uuid ) ) {
		int	return_val;

		Debug( pcache_debug,
			"Removing query UUID %s\n",
			uuid->bv_val, 0, 0 );
		return_val = remove_query_data( op, rs, uuid );
		Debug( pcache_debug,
			"QUERY REMOVED, SIZE=%d\n",
			return_val, 0, 0);
		ldap_pvt_thread_mutex_lock( &cm->cache_mutex );
		cm->cur_entries -= return_val;
		cm->num_cached_queries--;
		Debug( pcache_debug,
			"STORED QUERIES = %lu\n",
			cm->num_cached_queries, 0, 0 );
		ldap_pvt_thread_mutex_unlock( &cm->cache_mutex );
		Debug( pcache_debug,
			"QUERY REMOVED, CACHE ="
			"%d entries\n",
			cm->cur_entries, 0, 0 );
	}
}

/*
 * Callback used to fetch queryId values based on entryUUID;
 * used by pcache_remove_entries_from_cache()
 */
static int
fetch_queryId_cb( Operation *op, SlapReply *rs )
{
	int		rc = 0;

	/* only care about searchEntry responses */
	if ( rs->sr_type != REP_SEARCH ) {
		return 0;
	}

	/* allow only one response per entryUUID */
	if ( op->o_callback->sc_private != NULL ) {
		rc = 1;

	} else {
		Attribute	*a;

		/* copy all queryId values into callback's private data */
		a = attr_find( rs->sr_entry->e_attrs, ad_queryId );
		if ( a != NULL ) {
			BerVarray	vals = NULL;

			ber_bvarray_dup_x( &vals, a->a_nvals, op->o_tmpmemctx );
			op->o_callback->sc_private = (void *)vals;
		}
	}

	/* clear entry if required */
	if ( rs->sr_flags & REP_ENTRY_MUSTBEFREED ) {
		entry_free( rs->sr_entry );
		rs->sr_entry = NULL;
		rs->sr_flags ^= REP_ENTRY_MUSTBEFREED;
	}

	return rc;
}

/*
 * Call that allows to remove a set of entries from the cache,
 * by forcing the removal of all the related queries.
 */
int
pcache_remove_entries_from_cache(
	Operation	*op,
	cache_manager	*cm,
	BerVarray	entryUUIDs )
{
	Connection	conn = { 0 };
	OperationBuffer opbuf;
	Operation	op2;
	slap_callback	sc = { 0 };
	SlapReply	rs = { REP_RESULT };
	Filter		f = { 0 };
	char		filtbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(entryUUID=)" ) ];
	AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;
	AttributeName	attrs[ 2 ] = { 0 };
	int		s, rc;

	if ( op == NULL ) {
		void	*thrctx = ldap_pvt_thread_pool_context();

		connection_fake_init( &conn, &opbuf, thrctx );
		op = &opbuf.ob_op;

	} else {
		op2 = *op;
		op = &op2;
	}

	memset( &op->oq_search, 0, sizeof( op->oq_search ) );
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;
	f.f_choice = LDAP_FILTER_EQUALITY;
	f.f_ava = &ava;
	ava.aa_desc = slap_schema.si_ad_entryUUID;
	op->ors_filter = &f;
	op->ors_slimit = 1;
	op->ors_tlimit = SLAP_NO_LIMIT;
	attrs[ 0 ].an_desc = ad_queryId;
	attrs[ 0 ].an_name = ad_queryId->ad_cname;
	op->ors_attrs = attrs;
	op->ors_attrsonly = 0;

	op->o_req_dn = cm->db.be_suffix[ 0 ];
	op->o_req_ndn = cm->db.be_nsuffix[ 0 ];

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_managedsait = SLAP_CONTROL_CRITICAL;
	op->o_bd = &cm->db;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	sc.sc_response = fetch_queryId_cb;
	op->o_callback = &sc;

	for ( s = 0; !BER_BVISNULL( &entryUUIDs[ s ] ); s++ ) {
		BerVarray	vals = NULL;

		op->ors_filterstr.bv_len = snprintf( filtbuf, sizeof( filtbuf ),
			"(entryUUID=%s)", entryUUIDs[ s ].bv_val );
		op->ors_filterstr.bv_val = filtbuf;
		ava.aa_value = entryUUIDs[ s ];

		rc = op->o_bd->be_search( op, &rs );
		if ( rc != LDAP_SUCCESS ) {
			continue;
		}

		vals = (BerVarray)op->o_callback->sc_private;
		if ( vals != NULL ) {
			int		i;

			for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
				struct berval	val = vals[ i ];

				remove_query_and_data( op, &rs, cm, &val );

				if ( !BER_BVISNULL( &val ) && val.bv_val != vals[ i ].bv_val ) {
					ch_free( val.bv_val );
				}
			}

			ber_bvarray_free_x( vals, op->o_tmpmemctx );
			op->o_callback->sc_private = NULL;
		}
	}

	return 0;
}

/*
 * Call that allows to remove a query from the cache.
 */
int
pcache_remove_query_from_cache(
	Operation	*op,
	cache_manager	*cm,
	struct berval	*queryid )
{
	Operation	op2 = *op;
	SlapReply	rs2 = { 0 };

	op2.o_bd = &cm->db;

	/* remove the selected query */
	remove_query_and_data( &op2, &rs2, cm, queryid );

	return LDAP_SUCCESS;
}

/*
 * Call that allows to remove a set of queries related to an entry 
 * from the cache; if queryid is not null, the entry must belong to
 * the query indicated by queryid.
 */
int
pcache_remove_entry_queries_from_cache(
	Operation	*op,
	cache_manager	*cm,
	struct berval	*ndn,
	struct berval	*queryid )
{
	Connection		conn = { 0 };
	OperationBuffer 	opbuf;
	Operation		op2;
	slap_callback		sc = { 0 };
	SlapReply		rs = { REP_RESULT };
	Filter			f = { 0 };
	char			filter_str[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(queryId=)" ) ];
	AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
	AttributeName		attrs[ 2 ] = { 0 };
	int			rc;

	BerVarray		vals = NULL;

	if ( op == NULL ) {
		void	*thrctx = ldap_pvt_thread_pool_context();

		connection_fake_init( &conn, &opbuf, thrctx );
		op = &opbuf.ob_op;

	} else {
		op2 = *op;
		op = &op2;
	}

	memset( &op->oq_search, 0, sizeof( op->oq_search ) );
	op->ors_scope = LDAP_SCOPE_BASE;
	op->ors_deref = LDAP_DEREF_NEVER;
	if ( queryid == NULL || BER_BVISNULL( queryid ) ) {
		BER_BVSTR( &op->ors_filterstr, "(objectClass=*)" );
		f.f_choice = LDAP_FILTER_PRESENT;
		f.f_desc = slap_schema.si_ad_objectClass;

	} else {
		op->ors_filterstr.bv_len = snprintf( filter_str,
			sizeof( filter_str ), "(%s=%s)",
			ad_queryId->ad_cname.bv_val, queryid->bv_val );
		f.f_choice = LDAP_FILTER_EQUALITY;
		f.f_ava = &ava;
		f.f_av_desc = ad_queryId;
		f.f_av_value = *queryid;
	}
	op->ors_filter = &f;
	op->ors_slimit = 1;
	op->ors_tlimit = SLAP_NO_LIMIT;
	attrs[ 0 ].an_desc = ad_queryId;
	attrs[ 0 ].an_name = ad_queryId->ad_cname;
	op->ors_attrs = attrs;
	op->ors_attrsonly = 0;

	op->o_req_dn = *ndn;
	op->o_req_ndn = *ndn;

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_managedsait = SLAP_CONTROL_CRITICAL;
	op->o_bd = &cm->db;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	sc.sc_response = fetch_queryId_cb;
	op->o_callback = &sc;

	rc = op->o_bd->be_search( op, &rs );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	vals = (BerVarray)op->o_callback->sc_private;
	if ( vals != NULL ) {
		int		i;

		for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
			struct berval	val = vals[ i ];

			remove_query_and_data( op, &rs, cm, &val );

			if ( !BER_BVISNULL( &val ) && val.bv_val != vals[ i ].bv_val ) {
				ch_free( val.bv_val );
			}
		}

		ber_bvarray_free_x( vals, op->o_tmpmemctx );
	}

	return LDAP_SUCCESS;
}

static int
cache_entries(
	Operation	*op,
	SlapReply	*rs,
	struct berval *query_uuid )
{
	struct search_info *si = op->o_callback->sc_private;
	slap_overinst *on = si->on;
	cache_manager *cm = on->on_bi.bi_private;
	int		return_val = 0;
	Entry		*e;
	struct berval	crp_uuid;
	char		uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
	Operation	*op_tmp;
	Connection	conn = {0};
	OperationBuffer opbuf;
	void		*thrctx = ldap_pvt_thread_pool_context();

	query_uuid->bv_len = lutil_uuidstr(uuidbuf, sizeof(uuidbuf));
	ber_str2bv(uuidbuf, query_uuid->bv_len, 1, query_uuid);

	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op_tmp = &opbuf.ob_op;
	op_tmp->o_bd = &cm->db;
	op_tmp->o_dn = cm->db.be_rootdn;
	op_tmp->o_ndn = cm->db.be_rootndn;

	Debug( pcache_debug, "UUID for query being added = %s\n",
			uuidbuf, 0, 0 );

	for ( e=si->head; e; e=si->head ) {
		si->head = e->e_private;
		e->e_private = NULL;
		while ( cm->cur_entries > (cm->max_entries) ) {
			BER_BVZERO( &crp_uuid );
			remove_query_and_data( op_tmp, rs, cm, &crp_uuid );
		}

		return_val = merge_entry(op_tmp, e, query_uuid);
		ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
		cm->cur_entries += return_val;
		Debug( pcache_debug,
			"ENTRY ADDED/MERGED, CACHED ENTRIES=%d\n",
			cm->cur_entries, 0, 0 );
		return_val = 0;
		ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);
	}

	return return_val;
}

static int
pcache_op_cleanup( Operation *op, SlapReply *rs ) {
	slap_callback	*cb = op->o_callback;
	struct search_info *si = cb->sc_private;
	slap_overinst *on = si->on;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager*		qm = cm->qm;

	if ( rs->sr_type == REP_SEARCH ) {
		Entry *e;

		/* don't return more entries than requested by the client */
		if ( si->slimit && rs->sr_nentries >= si->slimit ) {
			si->slimit_exceeded = 1;
		}

		/* If we haven't exceeded the limit for this query,
		 * build a chain of answers to store. If we hit the
		 * limit, empty the chain and ignore the rest.
		 */
		if ( !si->over ) {
			if ( si->count < si->max ) {
				si->count++;
				e = entry_dup( rs->sr_entry );
				if ( !si->head ) si->head = e;
				if ( si->tail ) si->tail->e_private = e;
				si->tail = e;

			} else {
				si->over = 1;
				si->count = 0;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
				si->tail = NULL;
			}
		}

	}

	if ( rs->sr_type == REP_RESULT || 
		op->o_abandon || rs->sr_err == SLAPD_ABANDON )
	{
		if ( si->save_attrs != NULL ) {
			rs->sr_attrs = si->save_attrs;
			op->ors_attrs = si->save_attrs;
		}
		if ( (op->o_abandon || rs->sr_err == SLAPD_ABANDON) && 
				si->caching_reason == PC_IGNORE ) {
			filter_free( si->query.filter );
			if ( si->count ) {
				/* duplicate query, free it */
				Entry *e;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
			}
			op->o_callback = op->o_callback->sc_next;
			op->o_tmpfree( cb, op->o_tmpmemctx );
		} else if ( si->caching_reason != PC_IGNORE ) {
			CachedQuery *qc = qm->addfunc(op, qm, &si->query,
				si->qtemp, si->caching_reason, 1 );

			if ( qc != NULL ) {
				switch ( si->caching_reason ) {
				case PC_POSITIVE:
					cache_entries( op, rs, &qc->q_uuid );
					break;

				case PC_SIZELIMIT:
					qc->q_sizelimit = rs->sr_nentries;
					break;

				case PC_NEGATIVE:
					break;

				default:
					assert( 0 );
					break;
				}
				ldap_pvt_thread_rdwr_wunlock(&qc->rwlock);
				ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
				cm->num_cached_queries++;
				Debug( pcache_debug, "STORED QUERIES = %lu\n",
						cm->num_cached_queries, 0, 0 );
				ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);

				/* If the consistency checker suspended itself,
				 * wake it back up
				 */
				if ( cm->cc_paused ) {
					ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
					if ( cm->cc_paused ) {
						cm->cc_paused = 0;
						ldap_pvt_runqueue_resched( &slapd_rq, cm->cc_arg, 0 );
					}
					ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
				}

			} else if ( si->count ) {
				/* duplicate query, free it */
				Entry *e;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
			}

		} else {
			filter_free( si->query.filter );
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
pcache_response(
	Operation	*op,
	SlapReply	*rs )
{
	struct search_info *si = op->o_callback->sc_private;

	if ( si->save_attrs != NULL ) {
		rs->sr_attrs = si->save_attrs;
		op->ors_attrs = si->save_attrs;
	}

	if ( rs->sr_type == REP_SEARCH ) {
		/* don't return more entries than requested by the client */
		if ( si->slimit_exceeded ) {
			return 0;
		}

	} else if ( rs->sr_type == REP_RESULT ) {

		if ( si->count ) {
			if ( rs->sr_err == LDAP_SUCCESS ) {
				si->caching_reason = PC_POSITIVE;

			} else if ( rs->sr_err == LDAP_SIZELIMIT_EXCEEDED
				&& si->qtemp->limitttl )
			{
				si->caching_reason = PC_SIZELIMIT;
			}

		} else if ( si->qtemp->negttl && !si->count && !si->over &&
				rs->sr_err == LDAP_SUCCESS )
		{
			si->caching_reason = PC_NEGATIVE;
		}


		if ( si->slimit_exceeded ) {
			rs->sr_err = LDAP_SIZELIMIT_EXCEEDED;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
add_filter_attrs(
	Operation *op,
	AttributeName** new_attrs,
	struct attr_set *attrs,
	AttributeName* filter_attrs,
	int fattr_cnt,
	int fattr_got_oc)
{
	int alluser = 0;
	int allop = 0;
	int i, j;
	int count;
	int addoc = 0;

	/* duplicate attrs */
	count = attrs->count + fattr_cnt;
	if ( !fattr_got_oc && !(attrs->flags & PC_GOT_OC)) {
		addoc = 1;
		count++;
	}

	*new_attrs = (AttributeName*)ch_calloc( count + 1,
		sizeof(AttributeName) );
	for (i=0; i<attrs->count; i++) {
		(*new_attrs)[i].an_name = attrs->attrs[i].an_name;
		(*new_attrs)[i].an_desc = attrs->attrs[i].an_desc;
	}
	BER_BVZERO( &(*new_attrs)[i].an_name );
	alluser = an_find(*new_attrs, &AllUser);
	allop = an_find(*new_attrs, &AllOper);

	j = i;
	for ( i=0; i<fattr_cnt; i++ ) {
		if ( an_find(*new_attrs, &filter_attrs[i].an_name ) ) {
			continue;
		}
		if ( is_at_operational(filter_attrs[i].an_desc->ad_type) ) {
			if ( allop ) {
				continue;
			}
		} else if ( alluser ) {
			continue;
		}
		(*new_attrs)[j].an_name = filter_attrs[i].an_name;
		(*new_attrs)[j].an_desc = filter_attrs[i].an_desc;
		(*new_attrs)[j].an_oc = NULL;
		(*new_attrs)[j].an_oc_exclude = 0;
		j++;
	}
	if ( addoc ) {
		(*new_attrs)[j].an_name = slap_schema.si_ad_objectClass->ad_cname;
		(*new_attrs)[j].an_desc = slap_schema.si_ad_objectClass;
		(*new_attrs)[j].an_oc = NULL;
		(*new_attrs)[j].an_oc_exclude = 0;
		j++;
	}
	BER_BVZERO( &(*new_attrs)[j].an_name );

	return count;
}

/* NOTE: this is a quick workaround to let pcache minimally interact
 * with pagedResults.  A more articulated solutions would be to
 * perform the remote query without control and cache all results,
 * performing the pagedResults search only within the client
 * and the proxy.  This requires pcache to understand pagedResults. */
static int
pcache_chk_controls(
	Operation	*op,
	SlapReply	*rs )
{
	const char	*non = "";
	const char	*stripped = "";

	switch( op->o_pagedresults ) {
	case SLAP_CONTROL_NONCRITICAL:
		non = "non-";
		stripped = "; stripped";
		/* fallthru */

	case SLAP_CONTROL_CRITICAL:
		Debug( pcache_debug, "%s: "
			"%scritical pagedResults control "
			"disabled with proxy cache%s.\n",
			op->o_log_prefix, non, stripped );
		
		slap_remove_control( op, rs, slap_cids.sc_pagedResults, NULL );
		break;

	default:
		rs->sr_err = SLAP_CB_CONTINUE;
		break;
	}

	return rs->sr_err;
}

#ifdef PCACHE_CONTROL_PRIVDB
static int
pcache_op_privdb(
	Operation		*op,
	SlapReply		*rs )
{
	slap_overinst 	*on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager 	*cm = on->on_bi.bi_private;
	slap_callback	*save_cb;
	slap_op_t	type;

	/* skip if control is unset */
	if ( op->o_ctrlflag[ privDB_cid ] != SLAP_CONTROL_CRITICAL ) {
		return SLAP_CB_CONTINUE;
	}

	/* The cache DB isn't open yet */
	if ( cm->defer_db_open ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE,
			"pcachePrivDB: cacheDB not available" );
		return rs->sr_err;
	}

	/* FIXME: might be a little bit exaggerated... */
	if ( !be_isroot( op ) ) {
		save_cb = op->o_callback;
		op->o_callback = NULL;
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"pcachePrivDB: operation not allowed" );
		op->o_callback = save_cb;

		return rs->sr_err;
	}

	/* map tag to operation */
	type = slap_req2op( op->o_tag );
	if ( type != SLAP_OP_LAST ) {
		BI_op_func	**func;
		int		rc;

		/* execute, if possible */
		func = &cm->db.be_bind;
		if ( func[ type ] != NULL ) {
			Operation	op2 = *op;
	
			op2.o_bd = &cm->db;

			rc = func[ type ]( &op2, rs );
			if ( type == SLAP_OP_BIND && rc == LDAP_SUCCESS ) {
				op->o_conn->c_authz_cookie = cm->db.be_private;
			}
		}
	}

	/* otherwise fall back to error */
	save_cb = op->o_callback;
	op->o_callback = NULL;
	send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		"operation not supported with pcachePrivDB control" );
	op->o_callback = save_cb;

	return rs->sr_err;
}
#endif /* PCACHE_CONTROL_PRIVDB */

static int
pcache_op_search(
	Operation	*op,
	SlapReply	*rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager*		qm = cm->qm;

	int i = -1;

	AttributeName	*filter_attrs = NULL;

	Query		query;
	QueryTemplate	*qtemp = NULL;

	int 		attr_set = -1;
	CachedQuery 	*answerable = NULL;
	int 		cacheable = 0;
	int		fattr_cnt=0;
	int		fattr_got_oc = 0;

	struct berval	tempstr;

#ifdef PCACHE_CONTROL_PRIVDB
	if ( op->o_ctrlflag[ privDB_cid ] == SLAP_CONTROL_CRITICAL ) {
		return pcache_op_privdb( op, rs );
	}
#endif /* PCACHE_CONTROL_PRIVDB */

	/* The cache DB isn't open yet */
	if ( cm->defer_db_open ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE,
			"pcachePrivDB: cacheDB not available" );
		return rs->sr_err;
	}

	tempstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len+1, op->o_tmpmemctx );
	tempstr.bv_len = 0;
	if ( filter2template( op, op->ors_filter, &tempstr, &filter_attrs,
		&fattr_cnt, &fattr_got_oc )) {
		op->o_tmpfree( tempstr.bv_val, op->o_tmpmemctx );
		return SLAP_CB_CONTINUE;
	}

	Debug( pcache_debug, "query template of incoming query = %s\n",
					tempstr.bv_val, 0, 0 );

	/* FIXME: cannot cache/answer requests with pagedResults control */

	/* find attr set */
	attr_set = get_attr_set(op->ors_attrs, qm, cm->numattrsets);

	query.filter = op->ors_filter;
	query.base = op->o_req_ndn;
	query.scope = op->ors_scope;

	/* check for query containment */
	if (attr_set > -1) {
		QueryTemplate *qt = qm->attr_sets[attr_set].templates;
		for (; qt; qt = qt->qtnext ) {
			/* find if template i can potentially answer tempstr */
			if (qt->querystr.bv_len != tempstr.bv_len ||
				strcasecmp( qt->querystr.bv_val, tempstr.bv_val ))
				continue;
			cacheable = 1;
			qtemp = qt;
			Debug( pcache_debug, "Entering QC, querystr = %s\n",
			 		op->ors_filterstr.bv_val, 0, 0 );
			answerable = (*(qm->qcfunc))(op, qm, &query, qt);

			if (answerable)
				break;
		}
	}
	op->o_tmpfree( tempstr.bv_val, op->o_tmpmemctx );

	if (answerable) {
		/* Need to clear the callbacks of the original operation,
		 * in case there are other overlays */
		BackendDB	*save_bd = op->o_bd;
		slap_callback	*save_cb = op->o_callback;

		Debug( pcache_debug, "QUERY ANSWERABLE\n", 0, 0, 0 );
		op->o_tmpfree( filter_attrs, op->o_tmpmemctx );
		ldap_pvt_thread_rdwr_rlock(&answerable->rwlock);
		if ( BER_BVISNULL( &answerable->q_uuid )) {
			/* No entries cached, just an empty result set */
			i = rs->sr_err = 0;
			send_ldap_result( op, rs );
		} else {
			op->o_bd = &cm->db;
			op->o_callback = NULL;
			i = cm->db.bd_info->bi_op_search( op, rs );
		}
		ldap_pvt_thread_rdwr_runlock(&answerable->rwlock);
		ldap_pvt_thread_rdwr_runlock(&qtemp->t_rwlock);
		op->o_bd = save_bd;
		op->o_callback = save_cb;
		return i;
	}

	Debug( pcache_debug, "QUERY NOT ANSWERABLE\n", 0, 0, 0 );

	ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
	if (cm->num_cached_queries >= cm->max_queries) {
		cacheable = 0;
	}
	ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);

	if (op->ors_attrsonly)
		cacheable = 0;

	if (cacheable) {
		slap_callback		*cb;
		struct search_info	*si;

		Debug( pcache_debug, "QUERY CACHEABLE\n", 0, 0, 0 );
		query.filter = filter_dup(op->ors_filter, NULL);
		ldap_pvt_thread_rdwr_wlock(&qtemp->t_rwlock);
		if ( !qtemp->t_attrs.count ) {
			qtemp->t_attrs.count = add_filter_attrs(op,
				&qtemp->t_attrs.attrs,
				&qm->attr_sets[attr_set],
				filter_attrs, fattr_cnt, fattr_got_oc);
		}
		ldap_pvt_thread_rdwr_wunlock(&qtemp->t_rwlock);

		cb = op->o_tmpalloc( sizeof(*cb) + sizeof(*si), op->o_tmpmemctx );
		cb->sc_response = pcache_response;
		cb->sc_cleanup = pcache_op_cleanup;
		cb->sc_private = (cb+1);
		si = cb->sc_private;
		si->on = on;
		si->query = query;
		si->qtemp = qtemp;
		si->max = cm->num_entries_limit ;
		si->over = 0;
		si->count = 0;
		si->slimit = 0;
		si->slimit_exceeded = 0;
		si->caching_reason = PC_IGNORE;
		if ( op->ors_slimit && op->ors_slimit < cm->num_entries_limit ) {
			si->slimit = op->ors_slimit;
			op->ors_slimit = cm->num_entries_limit;
		}
		si->head = NULL;
		si->tail = NULL;
		si->save_attrs = op->ors_attrs;

		op->ors_attrs = qtemp->t_attrs.attrs;

		if ( cm->response_cb == PCACHE_RESPONSE_CB_HEAD ) {
			cb->sc_next = op->o_callback;
			op->o_callback = cb;

		} else {
			slap_callback		**pcb;

			/* need to move the callback at the end, in case other
			 * overlays are present, so that the final entry is
			 * actually cached */
			cb->sc_next = NULL;
			for ( pcb = &op->o_callback; *pcb; pcb = &(*pcb)->sc_next );
			*pcb = cb;
		}

	} else {
		Debug( pcache_debug, "QUERY NOT CACHEABLE\n",
					0, 0, 0);
	}

	op->o_tmpfree( filter_attrs, op->o_tmpmemctx );

	return SLAP_CB_CONTINUE;
}

static int
get_attr_set(
	AttributeName* attrs,
	query_manager* qm,
	int num )
{
	int i;
	int count = 0;

	if ( attrs ) {
		for ( ; attrs[count].an_name.bv_val; count++ );
	}

	/* recognize a single "*" or a "1.1" */
	if ( count == 0 ) {
		count = 1;
		attrs = slap_anlist_all_user_attributes;

	} else if ( count == 1 && strcmp( attrs[0].an_name.bv_val, LDAP_NO_ATTRS ) == 0 ) {
		count = 0;
		attrs = NULL;
	}

	for ( i = 0; i < num; i++ ) {
		AttributeName *a2;
		int found = 1;

		if ( count > qm->attr_sets[i].count ) {
			continue;
		}

		if ( !count ) {
			if ( !qm->attr_sets[i].count ) {
				break;
			}
			continue;
		}

		for ( a2 = attrs; a2->an_name.bv_val; a2++ ) {
			if ( !an_find( qm->attr_sets[i].attrs, &a2->an_name ) ) {
				found = 0;
				break;
			}
		}

		if ( found ) {
			break;
		}
	}

	if ( i == num ) {
		i = -1;
	}

	return i;
}

static void*
consistency_check(
	void *ctx,
	void *arg )
{
	struct re_s *rtask = arg;
	slap_overinst *on = rtask->arg;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager *qm = cm->qm;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;

	SlapReply rs = {REP_RESULT};
	CachedQuery* query;
	int return_val, pause = 1;
	QueryTemplate* templ;

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	op->o_bd = &cm->db;
	op->o_dn = cm->db.be_rootdn;
	op->o_ndn = cm->db.be_rootndn;

	cm->cc_arg = arg;

	for (templ = qm->templates; templ; templ=templ->qmnext) {
		query = templ->query_last;
		if ( query ) pause = 0;
		op->o_time = slap_get_time();
		while (query && (query->expiry_time < op->o_time)) {
			int rem = 0;
			Debug( pcache_debug, "Lock CR index = %p\n",
					(void *) templ, 0, 0 );
			ldap_pvt_thread_rdwr_wlock(&templ->t_rwlock);
			if ( query == templ->query_last ) {
				rem = 1;
				remove_from_template(query, templ);
				Debug( pcache_debug, "TEMPLATE %p QUERIES-- %d\n",
						(void *) templ, templ->no_of_queries, 0 );
				Debug( pcache_debug, "Unlock CR index = %p\n",
						(void *) templ, 0, 0 );
			}
			ldap_pvt_thread_rdwr_wunlock(&templ->t_rwlock);
			if ( !rem ) {
				query = templ->query_last;
				continue;
			}
			ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
			remove_query(qm, query);
			ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
			if ( BER_BVISNULL( &query->q_uuid ))
				return_val = 0;
			else
				return_val = remove_query_data(op, &rs, &query->q_uuid);
			Debug( pcache_debug, "STALE QUERY REMOVED, SIZE=%d\n",
						return_val, 0, 0 );
			ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
			cm->cur_entries -= return_val;
			cm->num_cached_queries--;
			Debug( pcache_debug, "STORED QUERIES = %lu\n",
					cm->num_cached_queries, 0, 0 );
			ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);
			Debug( pcache_debug,
				"STALE QUERY REMOVED, CACHE ="
				"%d entries\n",
				cm->cur_entries, 0, 0 );
			free_query(query);
			query = templ->query_last;
		}
	}
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}
	/* If there were no queries, defer processing for a while */
	cm->cc_paused = pause;
	ldap_pvt_runqueue_resched( &slapd_rq, rtask, pause );

	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return NULL;
}


#define MAX_ATTR_SETS 500

enum {
	PC_MAIN = 1,
	PC_ATTR,
	PC_TEMP,
	PC_RESP,
	PC_QUERIES
};

static ConfigDriver pc_cf_gen;
static ConfigLDAPadd pc_ldadd;
static ConfigCfAdd pc_cfadd;

static ConfigTable pccfg[] = {
	{ "proxycache", "backend> <max_entries> <numattrsets> <entry limit> "
				"<cycle_time",
		6, 6, 0, ARG_MAGIC|ARG_NO_DELETE|PC_MAIN, pc_cf_gen,
		"( OLcfgOvAt:2.1 NAME 'olcProxyCache' "
			"DESC 'ProxyCache basic parameters' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "proxyattrset", "index> <attributes...",
		2, 0, 0, ARG_MAGIC|PC_ATTR, pc_cf_gen,
		"( OLcfgOvAt:2.2 NAME 'olcProxyAttrset' "
			"DESC 'A set of attributes to cache' "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "proxytemplate", "filter> <attrset-index> <TTL> <negTTL",
		4, 6, 0, ARG_MAGIC|PC_TEMP, pc_cf_gen,
		"( OLcfgOvAt:2.3 NAME 'olcProxyTemplate' "
			"DESC 'Filter template, attrset, cache TTL, "
				"optional negative TTL, optional sizelimit TTL' "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "response-callback", "head|tail(default)",
		2, 2, 0, ARG_MAGIC|PC_RESP, pc_cf_gen,
		"( OLcfgOvAt:2.4 NAME 'olcProxyResponseCB' "
			"DESC 'Response callback position in overlay stack' "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "proxyCacheQueries", "queries",
		2, 2, 0, ARG_INT|ARG_MAGIC|PC_QUERIES, pc_cf_gen,
		"( OLcfgOvAt:2.5 NAME 'olcProxyCacheQueries' "
			"DESC 'Maximum number of queries to cache' "
			"SYNTAX OMsInteger )", NULL, NULL },
	{ "proxySaveQueries", "TRUE|FALSE",
		2, 2, 0, ARG_ON_OFF|ARG_OFFSET, (void *)offsetof(cache_manager, save_queries),
		"( OLcfgOvAt:2.6 NAME 'olcProxySaveQueries' "
			"DESC 'Save cached queries for hot restart' "
			"SYNTAX OMsBoolean )", NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

/* Need to no-op this keyword for dynamic config */
static ConfigTable pcdummy[] = {
	{ "", "", 0, 0, 0, ARG_IGNORED,
		NULL, "( OLcfgGlAt:13 NAME 'olcDatabase' "
			"DESC 'The backend type for a database instance' "
			"SUP olcBackend SINGLE-VALUE X-ORDERED 'SIBLINGS' )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs pcocs[] = {
	{ "( OLcfgOvOc:2.1 "
		"NAME 'olcPcacheConfig' "
		"DESC 'ProxyCache configuration' "
		"SUP olcOverlayConfig "
		"MUST ( olcProxyCache $ olcProxyAttrset $ olcProxyTemplate ) "
		"MAY ( olcProxyResponseCB $ olcProxyCacheQueries $ olcProxySaveQueries ) )",
		Cft_Overlay, pccfg, NULL, pc_cfadd },
	{ "( OLcfgOvOc:2.2 "
		"NAME 'olcPcacheDatabase' "
		"DESC 'Cache database configuration' "
		"AUXILIARY )", Cft_Misc, pcdummy, pc_ldadd },
	{ NULL, 0, NULL }
};

static int pcache_db_open2( slap_overinst *on, ConfigReply *cr );

static int
pc_ldadd_cleanup( ConfigArgs *c )
{
	slap_overinst *on = c->ca_private;
	return pcache_db_open2( on, &c->reply );
}

static int
pc_ldadd( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
	slap_overinst *on;
	cache_manager *cm;

	if ( p->ce_type != Cft_Overlay || !p->ce_bi ||
		p->ce_bi->bi_cf_ocs != pcocs )
		return LDAP_CONSTRAINT_VIOLATION;

	on = (slap_overinst *)p->ce_bi;
	cm = on->on_bi.bi_private;
	ca->be = &cm->db;
	/* Defer open if this is an LDAPadd */
	if ( CONFIG_ONLINE_ADD( ca ))
		ca->cleanup = pc_ldadd_cleanup;
	else
		cm->defer_db_open = 0;
	ca->ca_private = on;
	return LDAP_SUCCESS;
}

static int
pc_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *ca )
{
	CfEntryInfo *pe = p->e_private;
	slap_overinst *on = (slap_overinst *)pe->ce_bi;
	cache_manager *cm = on->on_bi.bi_private;
	struct berval bv;

	/* FIXME: should not hardcode "olcDatabase" here */
	bv.bv_len = snprintf( ca->cr_msg, sizeof( ca->cr_msg ),
		"olcDatabase=%s", cm->db.bd_info->bi_type );
	if ( bv.bv_len < 0 || bv.bv_len >= sizeof( ca->cr_msg ) ) {
		return -1;
	}
	bv.bv_val = ca->cr_msg;
	ca->be = &cm->db;
	cm->defer_db_open = 0;

	/* We can only create this entry if the database is table-driven
	 */
	if ( cm->db.bd_info->bi_cf_ocs )
		config_build_entry( op, rs, pe, ca, &bv, cm->db.bd_info->bi_cf_ocs,
			&pcocs[1] );

	return 0;
}

static int
pc_cf_gen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	cache_manager* 	cm = on->on_bi.bi_private;
	query_manager*  qm = cm->qm;
	QueryTemplate* 	temp;
	AttributeName*  attr_name;
	AttributeName* 	attrarray;
	const char* 	text=NULL;
	int		i, num, rc = 0;
	char		*ptr;
	unsigned long	t;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval bv;
		switch( c->type ) {
		case PC_MAIN:
			bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s %d %d %d %ld",
				cm->db.bd_info->bi_type, cm->max_entries, cm->numattrsets,
				cm->num_entries_limit, cm->cc_period );
			bv.bv_val = c->cr_msg;
			value_add_one( &c->rvalue_vals, &bv );
			break;
		case PC_ATTR:
			for (i=0; i<cm->numattrsets; i++) {
				if ( !qm->attr_sets[i].count ) continue;

				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ), "%d", i );

				/* count the attr length */
				for ( attr_name = qm->attr_sets[i].attrs;
					attr_name->an_name.bv_val; attr_name++ )
					bv.bv_len += attr_name->an_name.bv_len + 1;

				bv.bv_val = ch_malloc( bv.bv_len+1 );
				ptr = lutil_strcopy( bv.bv_val, c->cr_msg );
				for ( attr_name = qm->attr_sets[i].attrs;
					attr_name->an_name.bv_val; attr_name++ ) {
					*ptr++ = ' ';
					ptr = lutil_strcopy( ptr, attr_name->an_name.bv_val );
				}
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			if ( !c->rvalue_vals )
				rc = 1;
			break;
		case PC_TEMP:
			for (temp=qm->templates; temp; temp=temp->qmnext) {
				/* HEADS-UP: always print all;
				 * if optional == 0, ignore */
				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ),
					" %d %ld %ld %ld",
					temp->attr_set_index,
					temp->ttl,
					temp->negttl,
					temp->limitttl );
				bv.bv_len += temp->querystr.bv_len + 2;
				bv.bv_val = ch_malloc( bv.bv_len+1 );
				ptr = bv.bv_val;
				*ptr++ = '"';
				ptr = lutil_strcopy( ptr, temp->querystr.bv_val );
				*ptr++ = '"';
				strcpy( ptr, c->cr_msg );
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			if ( !c->rvalue_vals )
				rc = 1;
			break;
		case PC_RESP:
			if ( cm->response_cb == PCACHE_RESPONSE_CB_HEAD ) {
				BER_BVSTR( &bv, "head" );
			} else {
				BER_BVSTR( &bv, "tail" );
			}
			value_add_one( &c->rvalue_vals, &bv );
			break;
		case PC_QUERIES:
			c->value_int = cm->max_queries;
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		return 1;	/* FIXME */
#if 0
		switch( c->type ) {
		case PC_ATTR:
		case PC_TEMP:
		}
		return rc;
#endif
	}

	switch( c->type ) {
	case PC_MAIN:
		if ( cm->numattrsets > 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"proxycache\" directive already provided" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_atoi( &cm->numattrsets, c->argv[3] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse num attrsets=\"%s\" (arg #3)",
				c->argv[3] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->numattrsets <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "numattrsets (arg #3) must be positive" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->numattrsets > MAX_ATTR_SETS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "numattrsets (arg #3) must be <= %d", MAX_ATTR_SETS );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( !backend_db_init( c->argv[1], &cm->db, -1, NULL )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unknown backend type (arg #1)" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_atoi( &cm->max_entries, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse max entries=\"%s\" (arg #2)",
				c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->max_entries <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "max entries (arg #2) must be positive.\n" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_atoi( &cm->num_entries_limit, c->argv[4] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse entry limit=\"%s\" (arg #4)",
				c->argv[4] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->num_entries_limit <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "entry limit (arg #4) must be positive" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->num_entries_limit > cm->max_entries ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "entry limit (arg #4) must be less than max entries %d (arg #2)", cm->max_entries );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_parse_time( c->argv[5], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse period=\"%s\" (arg #5)",
				c->argv[5] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		cm->cc_period = (time_t)t;
		Debug( pcache_debug,
				"Total # of attribute sets to be cached = %d.\n",
				cm->numattrsets, 0, 0 );
		qm->attr_sets = ( struct attr_set * )ch_calloc( cm->numattrsets,
			    			sizeof( struct attr_set ) );
		break;
	case PC_ATTR:
		if ( cm->numattrsets == 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"proxycache\" directive not provided yet" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( lutil_atoi( &num, c->argv[1] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse attrset #=\"%s\"",
				c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( num < 0 || num >= cm->numattrsets ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "attrset index %d out of bounds (must be %s%d)",
				num, cm->numattrsets > 1 ? "0->" : "", cm->numattrsets - 1 );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		qm->attr_sets[num].flags |= PC_CONFIGURED;
		if ( c->argc == 2 ) {
			/* assume "1.1" */
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"need an explicit attr in attrlist; use \"*\" to indicate all attrs" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;

		} else if ( c->argc == 3 ) {
			if ( strcmp( c->argv[2], LDAP_ALL_USER_ATTRIBUTES ) == 0 ) {
				qm->attr_sets[num].count = 1;
				qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( 2,
					sizeof( AttributeName ) );
				BER_BVSTR( &qm->attr_sets[num].attrs[0].an_name, LDAP_ALL_USER_ATTRIBUTES );
				break;

			} else if ( strcmp( c->argv[2], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 ) {
				qm->attr_sets[num].count = 1;
				qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( 2,
					sizeof( AttributeName ) );
				BER_BVSTR( &qm->attr_sets[num].attrs[0].an_name, LDAP_ALL_OPERATIONAL_ATTRIBUTES );
				break;

			} else if ( strcmp( c->argv[2], LDAP_NO_ATTRS ) == 0 ) {
				break;
			}
			/* else: fallthru */

		} else if ( c->argc == 4 ) {
			if ( ( strcmp( c->argv[2], LDAP_ALL_USER_ATTRIBUTES ) == 0 && strcmp( c->argv[3], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 )
				|| ( strcmp( c->argv[2], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 && strcmp( c->argv[3], LDAP_ALL_USER_ATTRIBUTES ) == 0 ) )
			{
				qm->attr_sets[num].count = 2;
				qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( 3,
					sizeof( AttributeName ) );
				BER_BVSTR( &qm->attr_sets[num].attrs[0].an_name, LDAP_ALL_USER_ATTRIBUTES );
				BER_BVSTR( &qm->attr_sets[num].attrs[1].an_name, LDAP_ALL_OPERATIONAL_ATTRIBUTES );
				break;
			}
			/* else: fallthru */
		}

		if ( c->argc > 2 ) {
			int all_user = 0, all_op = 0;

			qm->attr_sets[num].count = c->argc - 2;
			qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( c->argc - 1,
				sizeof( AttributeName ) );
			attr_name = qm->attr_sets[num].attrs;
			for ( i = 2; i < c->argc; i++ ) {
				attr_name->an_desc = NULL;
				if ( strcmp( c->argv[i], LDAP_NO_ATTRS ) == 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"invalid attr #%d \"%s\" in attrlist",
						i - 2, c->argv[i] );
					Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
					ch_free( qm->attr_sets[num].attrs );
					qm->attr_sets[num].attrs = NULL;
					qm->attr_sets[num].count = 0;
					return 1;
				}
				if ( strcmp( c->argv[i], LDAP_ALL_USER_ATTRIBUTES ) == 0 ) {
					all_user = 1;
					BER_BVSTR( &attr_name->an_name, LDAP_ALL_USER_ATTRIBUTES );
				} else if ( strcmp( c->argv[i], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 ) {
					all_op = 1;
					BER_BVSTR( &attr_name->an_name, LDAP_ALL_OPERATIONAL_ATTRIBUTES );
				} else {
					if ( slap_str2ad( c->argv[i], &attr_name->an_desc, &text ) ) {
						strcpy( c->cr_msg, text );
						Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
						ch_free( qm->attr_sets[num].attrs );
						qm->attr_sets[num].attrs = NULL;
						qm->attr_sets[num].count = 0;
						return 1;
					}
					attr_name->an_name = attr_name->an_desc->ad_cname;
				}
				attr_name->an_oc = NULL;
				attr_name->an_oc_exclude = 0;
				if ( attr_name->an_desc == slap_schema.si_ad_objectClass )
					qm->attr_sets[num].flags |= PC_GOT_OC;
				attr_name++;
				BER_BVZERO( &attr_name->an_name );
			}

			/* warn if list contains both "*" and "+" */
			if ( i > 4 && all_user && all_op ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"warning: attribute list contains \"*\" and \"+\"" );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			}
		}
		break;
	case PC_TEMP:
		if ( cm->numattrsets == 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"proxycache\" directive not provided yet" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( lutil_atoi( &i, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse template #=\"%s\"",
				c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( i < 0 || i >= cm->numattrsets || 
			!(qm->attr_sets[i].flags & PC_CONFIGURED )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "template index %d invalid (%s%d)",
				i, cm->numattrsets > 1 ? "0->" : "", cm->numattrsets - 1 );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		temp = ch_calloc( 1, sizeof( QueryTemplate ));
		temp->qmnext = qm->templates;
		qm->templates = temp;
		ldap_pvt_thread_rdwr_init( &temp->t_rwlock );
		temp->query = temp->query_last = NULL;
		if ( lutil_parse_time( c->argv[3], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse template ttl=\"%s\"",
				c->argv[3] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		temp->ttl = (time_t)t;
		temp->negttl = (time_t)0;
		temp->limitttl = (time_t)0;
		switch ( c->argc ) {
		case 6:
			if ( lutil_parse_time( c->argv[5], &t ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse template sizelimit ttl=\"%s\"",
					c->argv[5] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return( 1 );
			}
			temp->limitttl = (time_t)t;
			/* fallthru */

		case 5:
			if ( lutil_parse_time( c->argv[4], &t ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse template negative ttl=\"%s\"",
					c->argv[4] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return( 1 );
			}
			temp->negttl = (time_t)t;
			break;
		}

		temp->no_of_queries = 0;

		ber_str2bv( c->argv[1], 0, 1, &temp->querystr );
		Debug( pcache_debug, "Template:\n", 0, 0, 0 );
		Debug( pcache_debug, "  query template: %s\n",
				temp->querystr.bv_val, 0, 0 );
		temp->attr_set_index = i;
		qm->attr_sets[i].flags |= PC_REFERENCED;
		temp->qtnext = qm->attr_sets[i].templates;
		qm->attr_sets[i].templates = temp;
		Debug( pcache_debug, "  attributes: \n", 0, 0, 0 );
		if ( ( attrarray = qm->attr_sets[i].attrs ) != NULL ) {
			for ( i=0; attrarray[i].an_name.bv_val; i++ )
				Debug( pcache_debug, "\t%s\n",
					attrarray[i].an_name.bv_val, 0, 0 );
		}
		break;
	case PC_RESP:
		if ( strcasecmp( c->argv[1], "head" ) == 0 ) {
			cm->response_cb = PCACHE_RESPONSE_CB_HEAD;

		} else if ( strcasecmp( c->argv[1], "tail" ) == 0 ) {
			cm->response_cb = PCACHE_RESPONSE_CB_TAIL;

		} else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unknown specifier" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		break;
	case PC_QUERIES:
		if ( c->value_int <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "max queries must be positive" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		cm->max_queries = c->value_int;
		break;
	}
	return rc;
}

static int
pcache_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	cache_manager* 	cm = on->on_bi.bi_private;

	/* Something for the cache database? */
	if ( cm->db.bd_info && cm->db.bd_info->bi_db_config )
		return cm->db.bd_info->bi_db_config( &cm->db, fname, lineno,
			argc, argv );
	return SLAP_CONF_UNKNOWN;
}

static int
pcache_db_init(
	BackendDB *be,
	ConfigReply *cr)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm;
	query_manager *qm;

	cm = (cache_manager *)ch_malloc(sizeof(cache_manager));
	on->on_bi.bi_private = cm;

	qm = (query_manager*)ch_malloc(sizeof(query_manager));

	cm->db = *be;
	SLAP_DBFLAGS(&cm->db) |= SLAP_DBFLAG_NO_SCHEMA_CHECK;
	cm->db.be_private = NULL;
	cm->db.be_pcl_mutexp = &cm->db.be_pcl_mutex;
	cm->qm = qm;
	cm->numattrsets = 0;
	cm->num_entries_limit = 5;
	cm->num_cached_queries = 0;
	cm->max_entries = 0;
	cm->cur_entries = 0;
	cm->max_queries = 10000;
	cm->save_queries = 0;
	cm->response_cb = PCACHE_RESPONSE_CB_TAIL;
	cm->defer_db_open = 1;
	cm->cc_period = 1000;
	cm->cc_paused = 0;
	cm->cc_arg = NULL;

	qm->attr_sets = NULL;
	qm->templates = NULL;
	qm->lru_top = NULL;
	qm->lru_bottom = NULL;

	qm->qcfunc = query_containment;
	qm->crfunc = cache_replacement;
	qm->addfunc = add_query;
	ldap_pvt_thread_mutex_init(&qm->lru_mutex);

	ldap_pvt_thread_mutex_init(&cm->cache_mutex);
	return 0;
}

static int
pcache_cachedquery_open_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		Attribute	*a;

		a = attr_find( rs->sr_entry->e_attrs, ad_cachedQueryURL );
		if ( a != NULL ) {
			BerVarray	*valsp;

			assert( a->a_nvals != NULL );

			valsp = op->o_callback->sc_private;
			assert( *valsp == NULL );

			ber_bvarray_dup_x( valsp, a->a_nvals, op->o_tmpmemctx );
		}
	}

	return 0;
}

static int
pcache_cachedquery_count_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		int	*countp = (int *)op->o_callback->sc_private;

		(*countp)++;
	}

	return 0;
}

static int
pcache_db_open2(
	slap_overinst *on,
	ConfigReply *cr )
{
	cache_manager	*cm = on->on_bi.bi_private;
	query_manager*  qm = cm->qm;
	int rc;

	rc = backend_startup_one( &cm->db, cr );
	if ( rc == 0 ) {
		cm->defer_db_open = 0;
	}

	/* There is no runqueue in TOOL mode */
	if (( slapMode & SLAP_SERVER_MODE ) && rc == 0 ) {
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		ldap_pvt_runqueue_insert( &slapd_rq, cm->cc_period,
			consistency_check, on,
			"pcache_consistency", cm->db.be_suffix[0].bv_val );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

		/* Cached database must have the rootdn */
		if ( BER_BVISNULL( &cm->db.be_rootndn )
				|| BER_BVISEMPTY( &cm->db.be_rootndn ) )
		{
			Debug( LDAP_DEBUG_ANY, "pcache_db_open(): "
				"underlying database of type \"%s\"\n"
				"    serving naming context \"%s\"\n"
				"    has no \"rootdn\", required by \"proxycache\".\n",
				on->on_info->oi_orig->bi_type,
				cm->db.be_suffix[0].bv_val, 0 );
			return 1;
		}

		if ( cm->save_queries ) {
			void		*thrctx = ldap_pvt_thread_pool_context();
			Connection	conn = { 0 };
			OperationBuffer	opbuf;
			Operation	*op;
			slap_callback	cb = { 0 };
			SlapReply	rs = { 0 };
			BerVarray	vals = NULL;
			Filter		f = { 0 }, f2 = { 0 };
			AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
			AttributeName	attrs[ 2 ] = { 0 };

			connection_fake_init( &conn, &opbuf, thrctx );
			op = &opbuf.ob_op;

			op->o_bd = &cm->db;

			op->o_tag = LDAP_REQ_SEARCH;
			op->o_protocol = LDAP_VERSION3;
			cb.sc_response = pcache_cachedquery_open_cb;
			cb.sc_private = &vals;
			op->o_callback = &cb;
			op->o_time = slap_get_time();
			op->o_do_not_cache = 1;
			op->o_managedsait = SLAP_CONTROL_CRITICAL;

			op->o_dn = cm->db.be_rootdn;
			op->o_ndn = cm->db.be_rootndn;
			op->o_req_dn = cm->db.be_suffix[ 0 ];
			op->o_req_ndn = cm->db.be_nsuffix[ 0 ];

			op->ors_scope = LDAP_SCOPE_BASE;
			op->ors_deref = LDAP_DEREF_NEVER;
			op->ors_slimit = 1;
			op->ors_tlimit = SLAP_NO_LIMIT;
			ber_str2bv( "(cachedQueryURL=*)", 0, 0, &op->ors_filterstr );
			f.f_choice = LDAP_FILTER_PRESENT;
			f.f_desc = ad_cachedQueryURL;
			op->ors_filter = &f;
			attrs[ 0 ].an_desc = ad_cachedQueryURL;
			attrs[ 0 ].an_name = ad_cachedQueryURL->ad_cname;
			op->ors_attrs = attrs;
			op->ors_attrsonly = 0;

			rc = op->o_bd->be_search( op, &rs );
			if ( rc == LDAP_SUCCESS && vals != NULL ) {
				int	i;

				for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
					if ( url2query( vals[ i ].bv_val, op, qm ) == 0 ) {
						cm->num_cached_queries++;
					}
				}

				ber_bvarray_free_x( vals, op->o_tmpmemctx );
			}

			/* count cached entries */
			f.f_choice = LDAP_FILTER_NOT;
			f.f_not = &f2;
			f2.f_choice = LDAP_FILTER_EQUALITY;
			f2.f_ava = &ava;
			f2.f_av_desc = slap_schema.si_ad_objectClass;
			BER_BVSTR( &f2.f_av_value, "glue" );
			ber_str2bv( "(!(objectClass=glue))", 0, 0, &op->ors_filterstr );

			op->ors_slimit = SLAP_NO_LIMIT;
			op->ors_scope = LDAP_SCOPE_SUBTREE;
			op->ors_attrs = slap_anlist_no_attrs;

			op->o_callback->sc_response = pcache_cachedquery_count_cb;
			rs.sr_nentries = 0;
			op->o_callback->sc_private = &rs.sr_nentries;

			rc = op->o_bd->be_search( op, &rs );

			cm->cur_entries = rs.sr_nentries;

			/* ignore errors */
			rc = 0;
		}
	}
	return rc;
}

static int
pcache_db_open(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	cache_manager	*cm = on->on_bi.bi_private;
	query_manager*  qm = cm->qm;
	int		i, ncf = 0, rf = 0, nrf = 0, rc = 0;

	/* check attr sets */
	for ( i = 0; i < cm->numattrsets; i++) {
		if ( !( qm->attr_sets[i].flags & PC_CONFIGURED ) ) {
			if ( qm->attr_sets[i].flags & PC_REFERENCED ) {
				Debug( LDAP_DEBUG_CONFIG, "pcache: attr set #%d not configured but referenced.\n", i, 0, 0 );
				rf++;

			} else {
				Debug( LDAP_DEBUG_CONFIG, "pcache: warning, attr set #%d not configured.\n", i, 0, 0 );
			}
			ncf++;

		} else if ( !( qm->attr_sets[i].flags & PC_REFERENCED ) ) {
			Debug( LDAP_DEBUG_CONFIG, "pcache: attr set #%d configured but not referenced.\n", i, 0, 0 );
			nrf++;
		}
	}

	if ( ncf || rf || nrf ) {
		Debug( LDAP_DEBUG_CONFIG, "pcache: warning, %d attr sets configured but not referenced.\n", nrf, 0, 0 );
		Debug( LDAP_DEBUG_CONFIG, "pcache: warning, %d attr sets not configured.\n", ncf, 0, 0 );
		Debug( LDAP_DEBUG_CONFIG, "pcache: %d attr sets not configured but referenced.\n", rf, 0, 0 );

		if ( rf > 0 ) {
			return 1;
		}
	}

	/* need to inherit something from the original database... */
	cm->db.be_def_limit = be->be_def_limit;
	cm->db.be_limits = be->be_limits;
	cm->db.be_acl = be->be_acl;
	cm->db.be_dfltaccess = be->be_dfltaccess;

	if ( SLAP_DBMONITORING( be ) ) {
		SLAP_DBFLAGS( &cm->db ) |= SLAP_DBFLAG_MONITORING;

	} else {
		SLAP_DBFLAGS( &cm->db ) &= ~SLAP_DBFLAG_MONITORING;
	}

	if ( !cm->defer_db_open )
		rc = pcache_db_open2( on, cr );

	return rc;
}

static void
pcache_free_qbase( void *v )
{
	Qbase *qb = v;
	int i;

	for (i=0; i<3; i++)
		tavl_free( qb->scopes[i], NULL );
	ch_free( qb );
}

static int
pcache_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager *qm = cm->qm;
	QueryTemplate *tm;
	int i, rc = 0;

	if ( cm->save_queries ) {
		CachedQuery	*qc;
		BerVarray	vals = NULL;

		void		*thrctx;
		Connection	conn = { 0 };
		OperationBuffer	opbuf;
		Operation	*op;
		slap_callback	cb = { 0 };

		SlapReply	rs = { REP_RESULT };
		Modifications	mod = { 0 };

		thrctx = ldap_pvt_thread_pool_context();

		connection_fake_init( &conn, &opbuf, thrctx );
		op = &opbuf.ob_op;

		if ( qm->templates != NULL ) {
			for ( tm = qm->templates; tm != NULL; tm = tm->qmnext ) {
				for ( qc = tm->query; qc; qc = qc->next ) {
					struct berval	bv;

					if ( query2url( op, qc, &bv ) == 0 ) {
						ber_bvarray_add_x( &vals, &bv, op->o_tmpmemctx );
					}
				}
			}
		}

		op->o_bd = &cm->db;
		op->o_dn = cm->db.be_rootdn;
		op->o_ndn = cm->db.be_rootndn;

		op->o_tag = LDAP_REQ_MODIFY;
		op->o_protocol = LDAP_VERSION3;
		cb.sc_response = slap_null_cb;
		op->o_callback = &cb;
		op->o_time = slap_get_time();
		op->o_do_not_cache = 1;
		op->o_managedsait = SLAP_CONTROL_CRITICAL;

		op->o_req_dn = op->o_bd->be_suffix[0];
		op->o_req_ndn = op->o_bd->be_nsuffix[0];

		mod.sml_op = LDAP_MOD_REPLACE;
		mod.sml_flags = 0;
		mod.sml_desc = ad_cachedQueryURL;
		mod.sml_type = ad_cachedQueryURL->ad_cname;
		mod.sml_values = vals;
		mod.sml_nvalues = NULL;
                mod.sml_numvals = 1;
		mod.sml_next = NULL;
		Debug( pcache_debug,
			"%sSETTING CACHED QUERY URLS\n",
			vals == NULL ? "RE" : "", 0, 0 );

		op->orm_modlist = &mod;

		op->o_bd->be_modify( op, &rs );

		ber_bvarray_free_x( vals, op->o_tmpmemctx );
	}

	/* cleanup stuff inherited from the original database... */
	cm->db.be_limits = NULL;
	cm->db.be_acl = NULL;

	/* stop the thread ... */
	if ( cm->cc_arg ) {
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		if ( ldap_pvt_runqueue_isrunning( &slapd_rq, cm->cc_arg ) ) {
			ldap_pvt_runqueue_stoptask( &slapd_rq, cm->cc_arg );
		}
		ldap_pvt_runqueue_remove( &slapd_rq, cm->cc_arg );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	if ( cm->db.bd_info->bi_db_close ) {
		rc = cm->db.bd_info->bi_db_close( &cm->db, NULL );
	}
	while ( (tm = qm->templates) != NULL ) {
		CachedQuery *qc, *qn;
		qm->templates = tm->qmnext;
		for ( qc = tm->query; qc; qc = qn ) {
			qn = qc->next;
			free_query( qc );
		}
		avl_free( tm->qbase, pcache_free_qbase );
		free( tm->querystr.bv_val );
		ldap_pvt_thread_rdwr_destroy( &tm->t_rwlock );
		free( tm->t_attrs.attrs );
		free( tm );
	}

	for ( i=0; i<cm->numattrsets; i++ ) {
		free( qm->attr_sets[i].attrs );
	}
	free( qm->attr_sets );
	qm->attr_sets = NULL;

	return rc;
}

static int
pcache_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager *qm = cm->qm;

	if ( cm->db.be_private != NULL ) {
		backend_stopdown_one( &cm->db );
	}

	ldap_pvt_thread_mutex_destroy( &qm->lru_mutex );
	ldap_pvt_thread_mutex_destroy( &cm->cache_mutex );
	free( qm );
	free( cm );

	return 0;
}

#ifdef PCACHE_CONTROL_PRIVDB
/*
        Control ::= SEQUENCE {
             controlType             LDAPOID,
             criticality             BOOLEAN DEFAULT FALSE,
             controlValue            OCTET STRING OPTIONAL }

        controlType ::= 1.3.6.1.4.1.4203.666.11.9.5.1

 * criticality must be TRUE; controlValue must be absent.
 */
static int
parse_privdb_ctrl(
	Operation	*op,
	SlapReply	*rs,
	LDAPControl	*ctrl )
{
	if ( op->o_ctrlflag[ privDB_cid ] != SLAP_CONTROL_NONE ) {
		rs->sr_text = "privateDB control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "privateDB control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !ctrl->ldctl_iscritical ) {
		rs->sr_text = "privateDB control criticality required";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_ctrlflag[ privDB_cid ] = SLAP_CONTROL_CRITICAL;

	return LDAP_SUCCESS;
}

static char *extops[] = {
	LDAP_EXOP_MODIFY_PASSWD,
	NULL
};
#endif /* PCACHE_CONTROL_PRIVDB */

#ifdef PCACHE_EXOP_QUERY_DELETE
static struct berval pcache_exop_QUERY_DELETE = BER_BVC( PCACHE_EXOP_QUERY_DELETE );

#define	LDAP_TAG_EXOP_QUERY_DELETE_BASE	((LBER_CLASS_CONTEXT|LBER_CONSTRUCTED) + 0)
#define	LDAP_TAG_EXOP_QUERY_DELETE_DN	((LBER_CLASS_CONTEXT|LBER_CONSTRUCTED) + 1)
#define	LDAP_TAG_EXOP_QUERY_DELETE_UUID	((LBER_CLASS_CONTEXT|LBER_CONSTRUCTED) + 2)

/*
        ExtendedRequest ::= [APPLICATION 23] SEQUENCE {
             requestName      [0] LDAPOID,
             requestValue     [1] OCTET STRING OPTIONAL }

        requestName ::= 1.3.6.1.4.1.4203.666.11.9.6.1

        requestValue ::= SEQUENCE { CHOICE {
                  baseDN           [0] LDAPDN
                  entryDN          [1] LDAPDN },
             queryID          [2] OCTET STRING (SIZE(16))
                  -- constrained to UUID }

 * Either baseDN or entryDN must be present, to allow database selection.
 *
 * 1. if baseDN and queryID are present, then the query corresponding
 *    to queryID is deleted;
 * 2. if baseDN is present and queryID is absent, then all queries
 *    are deleted;
 * 3. if entryDN is present and queryID is absent, then all queries
 *    corresponding to the queryID values present in entryDN are deleted;
 * 4. if entryDN and queryID are present, then all queries
 *    corresponding to the queryID values present in entryDN are deleted,
 *    but only if the value of queryID is contained in the entry;
 *
 * Currently, only 1, 3 and 4 are implemented.  2 can be obtained by either
 * recursively deleting the database (ldapdelete -r) with PRIVDB control,
 * or by removing the database files.

        ExtendedResponse ::= [APPLICATION 24] SEQUENCE {
             COMPONENTS OF LDAPResult,
             responseName     [10] LDAPOID OPTIONAL,
             responseValue    [11] OCTET STRING OPTIONAL }

 * responseName and responseValue must be absent.
 */

/*
 * - on success, *tagp is either LDAP_TAG_EXOP_QUERY_DELETE_BASE
 *   or LDAP_TAG_EXOP_QUERY_DELETE_DN.
 * - if ndn != NULL, it is set to the normalized DN in the request
 *   corresponding to either the baseDN or the entryDN, according
 *   to *tagp; memory is malloc'ed on the Operation's slab, and must
 *   be freed by the caller.
 * - if uuid != NULL, it is set to point to the normalized UUID;
 *   memory is malloc'ed on the Operation's slab, and must
 *   be freed by the caller.
 */
static int
pcache_parse_query_delete(
	struct berval	*in,
	ber_tag_t	*tagp,
	struct berval	*ndn,
	struct berval	*uuid,
	const char	**text,
	void		*ctx )
{
	int			rc = LDAP_SUCCESS;
	ber_tag_t		tag;
	ber_len_t		len = -1;
	BerElementBuffer	berbuf;
	BerElement		*ber = (BerElement *)&berbuf;
	struct berval		reqdata = BER_BVNULL;

	*text = NULL;

	if ( ndn ) {
		BER_BVZERO( ndn );
	}

	if ( uuid ) {
		BER_BVZERO( uuid );
	}

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in queryDelete exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE,
			"pcache_parse_query_delete: decoding error.\n",
			0, 0, 0 );
		goto decoding_error;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_BASE
		|| tag == LDAP_TAG_EXOP_QUERY_DELETE_DN )
	{
		*tagp = tag;

		if ( ndn != NULL ) {
			struct berval	dn;

			tag = ber_scanf( ber, "m", &dn );
			if ( tag == LBER_ERROR ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: DN parse failed.\n",
					0, 0, 0 );
				goto decoding_error;
			}

			rc = dnNormalize( 0, NULL, NULL, &dn, ndn, ctx );
			if ( rc != LDAP_SUCCESS ) {
				*text = "invalid DN in queryDelete exop request data";
				goto done;
			}

		} else {
			tag = ber_scanf( ber, "x" /* "m" */ );
			if ( tag == LBER_DEFAULT ) {
				goto decoding_error;
			}
		}

		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_UUID ) {
		if ( uuid != NULL ) {
			struct berval	bv;
			char		uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];

			tag = ber_scanf( ber, "m", &bv );
			if ( tag == LBER_ERROR ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: UUID parse failed.\n",
					0, 0, 0 );
				goto decoding_error;
			}

			if ( bv.bv_len != 16 ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: invalid UUID length %lu.\n",
					(unsigned long)bv.bv_len, 0, 0 );
				goto decoding_error;
			}

			rc = lutil_uuidstr_from_normalized(
				bv.bv_val, bv.bv_len,
				uuidbuf, sizeof( uuidbuf ) );
			if ( rc == -1 ) {
				goto decoding_error;
			}
			ber_str2bv( uuidbuf, rc, 1, uuid );
			rc = LDAP_SUCCESS;

		} else {
			tag = ber_skip_tag( ber, &len );
			if ( tag == LBER_DEFAULT ) {
				goto decoding_error;
			}

			if ( len != 16 ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: invalid UUID length %lu.\n",
					(unsigned long)len, 0, 0 );
				goto decoding_error;
			}
		}

		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;
		Debug( LDAP_DEBUG_TRACE,
			"pcache_parse_query_delete: decoding error\n",
			0, 0, 0 );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "queryDelete data decoding error";

done:;
		if ( ndn && !BER_BVISNULL( ndn ) ) {
			slap_sl_free( ndn->bv_val, ctx );
			BER_BVZERO( ndn );
		}

		if ( uuid && !BER_BVISNULL( uuid ) ) {
			slap_sl_free( uuid->bv_val, ctx );
			BER_BVZERO( uuid );
		}
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

static int
pcache_exop_query_delete(
	Operation	*op,
	SlapReply	*rs )
{
	BackendDB	*bd = op->o_bd;

	struct berval	uuid = BER_BVNULL,
			*uuidp = NULL;
	char		buf[ SLAP_TEXT_BUFLEN ] = { '\0' };
	int		len = 0;
	ber_tag_t	tag = LBER_DEFAULT;

	if ( LogTest( LDAP_DEBUG_STATS ) ) {
		uuidp = &uuid;
	}

	rs->sr_err = pcache_parse_query_delete( op->ore_reqdata,
		&tag, &op->o_req_ndn, uuidp,
		&rs->sr_text, op->o_tmpmemctx );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		return rs->sr_err;
	}

	if ( LogTest( LDAP_DEBUG_STATS ) ) {
		assert( !BER_BVISNULL( &op->o_req_ndn ) );
		len = snprintf( buf, sizeof( buf ), " dn=\"%s\"", op->o_req_ndn.bv_val );

		if ( !BER_BVISNULL( &uuid ) ) {
			snprintf( &buf[ len ], sizeof( buf ) - len, " queryId=\"%s\"", uuid.bv_val );
		}

		Debug( LDAP_DEBUG_STATS, "%s QUERY DELETE%s\n",
			op->o_log_prefix, buf, 0 );
	}
	op->o_req_dn = op->o_req_ndn;

	op->o_bd = select_backend( &op->o_req_ndn, 0 );
	rs->sr_err = backend_check_restrictions( op, rs,
		(struct berval *)&pcache_exop_QUERY_DELETE );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto done;
	}

	if ( op->o_bd->be_extended == NULL ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE_CRITICAL_EXTENSION,
			"backend does not support extended operations" );
		goto done;
	}

	op->o_bd->be_extended( op, rs );

done:;
	if ( !BER_BVISNULL( &op->o_req_ndn ) ) {
		op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &op->o_req_ndn );
		BER_BVZERO( &op->o_req_dn );
	}

	if ( !BER_BVISNULL( &uuid ) ) {
		op->o_tmpfree( uuid.bv_val, op->o_tmpmemctx );
	}

	op->o_bd = bd;

        return rs->sr_err;
}

static int
pcache_op_extended( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager	*cm = on->on_bi.bi_private;

#ifdef PCACHE_CONTROL_PRIVDB
	if ( op->o_ctrlflag[ privDB_cid ] == SLAP_CONTROL_CRITICAL ) {
		return pcache_op_privdb( op, rs );
	}
#endif /* PCACHE_CONTROL_PRIVDB */

	if ( bvmatch( &op->ore_reqoid, &pcache_exop_QUERY_DELETE ) ) {
		struct berval	uuid = BER_BVNULL;
		ber_tag_t	tag = LBER_DEFAULT;

		rs->sr_err = pcache_parse_query_delete( op->ore_reqdata,
			&tag, NULL, &uuid, &rs->sr_text, op->o_tmpmemctx );
		assert( rs->sr_err == LDAP_SUCCESS );

		if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_DN ) {
			/* remove all queries related to the selected entry */
			rs->sr_err = pcache_remove_entry_queries_from_cache( op,
				cm, &op->o_req_ndn, &uuid );

		} else if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_BASE ) {
			if ( !BER_BVISNULL( &uuid ) ) {
				/* remove the selected query */
				rs->sr_err = pcache_remove_query_from_cache( op,
					cm, &uuid );

			} else {
				/* TODO: remove all queries */
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "deletion of all queries not implemented";
			}
		}

		op->o_tmpfree( uuid.bv_val, op->o_tmpmemctx );
	}

	return rs->sr_err;
}
#endif /* PCACHE_EXOP_QUERY_DELETE */

static slap_overinst pcache;

static char *obsolete_names[] = {
	"proxycache",
	NULL
};

#if SLAPD_OVER_PROXYCACHE == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_PROXYCACHE == SLAPD_MOD_DYNAMIC */
int
pcache_initialize()
{
	int i, code;
	struct berval debugbv = BER_BVC("pcache");

	code = slap_loglevel_get( &debugbv, &pcache_debug );
	if ( code ) {
		return code;
	}

#ifdef PCACHE_CONTROL_PRIVDB
	code = register_supported_control( PCACHE_CONTROL_PRIVDB,
		SLAP_CTRL_BIND|SLAP_CTRL_ACCESS|SLAP_CTRL_HIDE, extops,
		parse_privdb_ctrl, &privDB_cid );
	if ( code != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"pcache_initialize: failed to register control %s (%d)\n",
			PCACHE_CONTROL_PRIVDB, code, 0 );
		return code;
	}
#endif /* PCACHE_CONTROL_PRIVDB */

#ifdef PCACHE_EXOP_QUERY_DELETE
	code = load_extop2( (struct berval *)&pcache_exop_QUERY_DELETE,
		SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, pcache_exop_query_delete,
		0 );
	if ( code != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"pcache_initialize: unable to register queryDelete exop: %d.\n",
			code, 0, 0 );
		return code;
	}
#endif /* PCACHE_EXOP_QUERY_DELETE */

	for ( i = 0; as[i].desc != NULL; i++ ) {
		code = register_at( as[i].desc, as[i].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"pcache_initialize: register_at #%d failed\n", i, 0, 0 );
			return code;
		}
		(*as[i].adp)->ad_type->sat_flags |= SLAP_AT_HIDE;
	}

	pcache.on_bi.bi_type = "pcache";
	pcache.on_bi.bi_obsolete_names = obsolete_names;
	pcache.on_bi.bi_db_init = pcache_db_init;
	pcache.on_bi.bi_db_config = pcache_db_config;
	pcache.on_bi.bi_db_open = pcache_db_open;
	pcache.on_bi.bi_db_close = pcache_db_close;
	pcache.on_bi.bi_db_destroy = pcache_db_destroy;

	pcache.on_bi.bi_op_search = pcache_op_search;
#ifdef PCACHE_CONTROL_PRIVDB
	pcache.on_bi.bi_op_bind = pcache_op_privdb;
	pcache.on_bi.bi_op_compare = pcache_op_privdb;
	pcache.on_bi.bi_op_modrdn = pcache_op_privdb;
	pcache.on_bi.bi_op_modify = pcache_op_privdb;
	pcache.on_bi.bi_op_add = pcache_op_privdb;
	pcache.on_bi.bi_op_delete = pcache_op_privdb;
#endif /* PCACHE_CONTROL_PRIVDB */
#ifdef PCACHE_EXOP_QUERY_DELETE
	pcache.on_bi.bi_extended = pcache_op_extended;
#elif defined( PCACHE_CONTROL_PRIVDB )
	pcache.on_bi.bi_extended = pcache_op_privdb;
#endif

	pcache.on_bi.bi_chk_controls = pcache_chk_controls;

	pcache.on_bi.bi_cf_ocs = pcocs;

	code = config_register_schema( pccfg, pcocs );
	if ( code ) return code;

	{
		const char *text;
		code = slap_str2ad( "olcDatabase", &pcdummy[0].ad, &text );
		if ( code ) return code;
	}
	return overlay_register( &pcache );
}

#if SLAPD_OVER_PROXYCACHE == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return pcache_initialize();
}
#endif

#endif	/* defined(SLAPD_OVER_PROXYCACHE) */
