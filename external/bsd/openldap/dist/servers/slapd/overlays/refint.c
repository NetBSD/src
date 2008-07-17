/* refint.c - referential integrity module */
/* $OpenLDAP: pkg/ldap/servers/slapd/overlays/refint.c,v 1.19.2.9 2008/05/27 20:18:19 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2008 The OpenLDAP Foundation.
 * Portions Copyright 2004 Symas Corporation.
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
 * This work was initially developed by Symas Corp. for inclusion in
 * OpenLDAP Software.  This work was sponsored by Hewlett-Packard.
 */

#include "portable.h"

/* This module maintains referential integrity for a set of
 * DN-valued attributes by searching for all references to a given
 * DN whenever the DN is changed or its entry is deleted, and making
 * the appropriate update.
 *
 * Updates are performed using the database rootdn in a separate task
 * to allow the original operation to complete immediately.
 */

#ifdef SLAPD_OVER_REFINT

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "ldap_rq.h"

static slap_overinst refint;

/* The DN to use in the ModifiersName for all refint updates */
static BerValue refint_dn = BER_BVC("cn=Referential Integrity Overlay");
static BerValue refint_ndn = BER_BVC("cn=referential integrity overlay");

typedef struct refint_attrs_s {
	struct refint_attrs_s	*next;
	AttributeDescription	*attr;
	BerVarray		old_vals;
	BerVarray		old_nvals;
	BerVarray		new_vals;
	BerVarray		new_nvals;
	int				ra_numvals;
} refint_attrs;

typedef struct dependents_s {
	struct dependents_s *next;
	BerValue dn;				/* target dn */
	BerValue ndn;
	refint_attrs *attrs;
} dependent_data;

typedef struct refint_q {
	struct refint_q *next;
	struct refint_data_s *rdata;
	dependent_data *attrs;		/* entries and attrs returned from callback */
	BackendDB *db;
	BerValue olddn;
	BerValue oldndn;
	BerValue newdn;
	BerValue newndn;
} refint_q;

typedef struct refint_data_s {
	const char *message;			/* breadcrumbs */
	struct refint_attrs_s *attrs;	/* list of known attrs */
	BerValue dn;				/* basedn in parent, */
	BerValue nothing;			/* the nothing value, if needed */
	BerValue nnothing;			/* normalized nothingness */
	BerValue refint_dn;			/* modifier's name */
	BerValue refint_ndn;			/* normalized modifier's name */
	struct re_s *qtask;
	refint_q *qhead;
	refint_q *qtail;
	ldap_pvt_thread_mutex_t qmutex;
} refint_data;

#define	RUNQ_INTERVAL	36000	/* a long time */

static MatchingRule	*mr_dnSubtreeMatch;

enum {
	REFINT_ATTRS = 1,
	REFINT_NOTHING,
	REFINT_MODIFIERSNAME
};

static ConfigDriver refint_cf_gen;

static ConfigTable refintcfg[] = {
	{ "refint_attributes", "attribute...", 2, 0, 0,
	  ARG_MAGIC|REFINT_ATTRS, refint_cf_gen,
	  "( OLcfgOvAt:11.1 NAME 'olcRefintAttribute' "
	  "DESC 'Attributes for referential integrity' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "refint_nothing", "string", 2, 2, 0,
	  ARG_DN|ARG_MAGIC|REFINT_NOTHING, refint_cf_gen,
	  "( OLcfgOvAt:11.2 NAME 'olcRefintNothing' "
	  "DESC 'Replacement DN to supply when needed' "
	  "SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "refint_modifiersName", "DN", 2, 2, 0,
	  ARG_DN|ARG_MAGIC|REFINT_MODIFIERSNAME, refint_cf_gen,
	  "( OLcfgOvAt:11.3 NAME 'olcRefintModifiersName' "
	  "DESC 'The DN to use as modifiersName' "
	  "SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs refintocs[] = {
	{ "( OLcfgOvOc:11.1 "
	  "NAME 'olcRefintConfig' "
	  "DESC 'Referential integrity configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcRefintAttribute "
		"$ olcRefintNothing "
		"$ olcRefintModifiersName "
	  ") )",
	  Cft_Overlay, refintcfg },
	{ NULL, 0, NULL }
};

static int
refint_cf_gen(ConfigArgs *c)
{
	slap_overinst *on = (slap_overinst *)c->bi;
	refint_data *dd = (refint_data *)on->on_bi.bi_private;
	refint_attrs *ip, *pip, **pipp = NULL;
	AttributeDescription *ad;
	const char *text;
	int rc = ARG_BAD_CONF;
	int i;

	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		switch ( c->type ) {
		case REFINT_ATTRS:
			ip = dd->attrs;
			while ( ip ) {
				value_add_one( &c->rvalue_vals,
					       &ip->attr->ad_cname );
				ip = ip->next;
			}
			rc = 0;
			break;
		case REFINT_NOTHING:
			if ( !BER_BVISEMPTY( &dd->nothing )) {
				rc = value_add_one( &c->rvalue_vals,
						    &dd->nothing );
				if ( rc ) return rc;
				rc = value_add_one( &c->rvalue_nvals,
						    &dd->nnothing );
				return rc;
			}
			rc = 0;
			break;
		case REFINT_MODIFIERSNAME:
			if ( !BER_BVISEMPTY( &dd->refint_dn )) {
				rc = value_add_one( &c->rvalue_vals,
						    &dd->refint_dn );
				if ( rc ) return rc;
				rc = value_add_one( &c->rvalue_nvals,
						    &dd->refint_ndn );
				return rc;
			}
			rc = 0;
			break;
		default:
			abort ();
		}
		break;
	case LDAP_MOD_DELETE:
		switch ( c->type ) {
		case REFINT_ATTRS:
			pipp = &dd->attrs;
			if ( c->valx < 0 ) {
				ip = *pipp;
				*pipp = NULL;
				while ( ip ) {
					pip = ip;
					ip = ip->next;
					ch_free ( pip );
				}
			} else {
				/* delete from linked list */
				for ( i=0; i < c->valx; ++i ) {
					pipp = &(*pipp)->next;
				}
				ip = *pipp;
				*pipp = (*pipp)->next;

				/* AttributeDescriptions are global so
				 * shouldn't be freed here... */
				ch_free ( ip );
			}
			rc = 0;
			break;
		case REFINT_NOTHING:
			if ( dd->nothing.bv_val )
				ber_memfree ( dd->nothing.bv_val );
			if ( dd->nnothing.bv_val )
				ber_memfree ( dd->nnothing.bv_val );
			dd->nothing.bv_len = 0;
			dd->nnothing.bv_len = 0;
			rc = 0;
			break;
		case REFINT_MODIFIERSNAME:
			if ( dd->refint_dn.bv_val )
				ber_memfree ( dd->refint_dn.bv_val );
			if ( dd->refint_ndn.bv_val )
				ber_memfree ( dd->refint_ndn.bv_val );
			dd->refint_dn.bv_len = 0;
			dd->refint_ndn.bv_len = 0;
			rc = 0;
			break;
		default:
			abort ();
		}
		break;
	case SLAP_CONFIG_ADD:
		/* fallthrough to LDAP_MOD_ADD */
	case LDAP_MOD_ADD:
		switch ( c->type ) {
		case REFINT_ATTRS:
			rc = 0;
			for ( i=1; i < c->argc; ++i ) {
				ad = NULL;
				if ( slap_str2ad ( c->argv[i], &ad, &text )
				     == LDAP_SUCCESS) {
					ip = ch_malloc (
						sizeof ( refint_attrs ) );
					ip->attr = ad;
					ip->next = dd->attrs;
					dd->attrs = ip;
				} else {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s <%s>: %s", c->argv[0], c->argv[i], text );
					Debug ( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
						"%s: %s\n", c->log, c->cr_msg, 0 );
					rc = ARG_BAD_CONF;
				}
			}
			break;
		case REFINT_NOTHING:
			if ( dd->nothing.bv_val )
				ber_memfree ( dd->nothing.bv_val );
			if ( dd->nnothing.bv_val )
				ber_memfree ( dd->nnothing.bv_val );
			dd->nothing = c->value_dn;
			dd->nnothing = c->value_ndn;
			rc = 0;
			break;
		case REFINT_MODIFIERSNAME:
			if ( dd->refint_dn.bv_val )
				ber_memfree ( dd->refint_dn.bv_val );
			if ( dd->refint_ndn.bv_val )
				ber_memfree ( dd->refint_ndn.bv_val );
			dd->refint_dn = c->value_dn;
			dd->refint_ndn = c->value_ndn;
			rc = 0;
			break;
		default:
			abort ();
		}
		break;
	default:
		abort ();
	}

	return rc;
}

/*
** allocate new refint_data;
** store in on_bi.bi_private;
**
*/

static int
refint_db_init(
	BackendDB	*be,
	ConfigReply	*cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	refint_data *id = ch_calloc(1,sizeof(refint_data));

	id->message = "_init";
	on->on_bi.bi_private = id;
	ldap_pvt_thread_mutex_init( &id->qmutex );
	return(0);
}

static int
refint_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;

	if ( on->on_bi.bi_private ) {
		refint_data *id = on->on_bi.bi_private;
		on->on_bi.bi_private = NULL;
		ldap_pvt_thread_mutex_destroy( &id->qmutex );
		ch_free( id );
	}
	return(0);
}

/*
** initialize, copy basedn if not already set
**
*/

static int
refint_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on	= (slap_overinst *)be->bd_info;
	refint_data *id	= on->on_bi.bi_private;
	id->message		= "_open";

	if ( BER_BVISNULL( &id->dn )) {
		if ( BER_BVISNULL( &be->be_nsuffix[0] ))
			return -1;
		ber_dupbv( &id->dn, &be->be_nsuffix[0] );
	}
	if ( BER_BVISNULL( &id->refint_dn ) ) {
		ber_dupbv( &id->refint_dn, &refint_dn );
		ber_dupbv( &id->refint_ndn, &refint_ndn );
	}
	return(0);
}


/*
** foreach configured attribute:
**	free it;
** free our basedn;
** (do not) free id->message;
** reset on_bi.bi_private;
** free our config data;
**
*/

static int
refint_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on	= (slap_overinst *) be->bd_info;
	refint_data *id	= on->on_bi.bi_private;
	refint_attrs *ii, *ij;
	id->message		= "_close";

	for(ii = id->attrs; ii; ii = ij) {
		ij = ii->next;
		ch_free(ii);
	}
	id->attrs = NULL;

	ch_free( id->dn.bv_val );
	BER_BVZERO( &id->dn );
	ch_free( id->nothing.bv_val );
	BER_BVZERO( &id->nothing );
	ch_free( id->nnothing.bv_val );
	BER_BVZERO( &id->nnothing );
	ch_free( id->refint_dn.bv_val );
	BER_BVZERO( &id->refint_dn );
	ch_free( id->refint_ndn.bv_val );
	BER_BVZERO( &id->refint_ndn );

	return(0);
}

/*
** search callback
** generates a list of Attributes from search results
*/

static int
refint_search_cb(
	Operation *op,
	SlapReply *rs
)
{
	Attribute *a;
	BerVarray b = NULL;
	refint_q *rq = op->o_callback->sc_private;
	refint_data *dd = rq->rdata;
	refint_attrs *ia, *da = dd->attrs, *na;
	dependent_data *ip;
	int i;

	Debug(LDAP_DEBUG_TRACE, "refint_search_cb <%s>\n",
		rs->sr_entry ? rs->sr_entry->e_name.bv_val : "NOTHING", 0, 0);

	if (rs->sr_type != REP_SEARCH || !rs->sr_entry) return(0);

	/*
	** foreach configured attribute type:
	**	if this attr exists in the search result,
	**	and it has a value matching the target:
	**		allocate an attr;
	**		if this is a delete and there's only one value:
	**			allocate the same attr again;
	**
	*/

	ip = op->o_tmpalloc(sizeof(dependent_data), op->o_tmpmemctx );
	ber_dupbv_x( &ip->dn, &rs->sr_entry->e_name, op->o_tmpmemctx );
	ber_dupbv_x( &ip->ndn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
	ip->next = rq->attrs;
	rq->attrs = ip;
	ip->attrs = NULL;
	for(ia = da; ia; ia = ia->next) {
		if ( (a = attr_find(rs->sr_entry->e_attrs, ia->attr) ) ) {
			int		first = -1, count = 0, deleted = 0;

			na = NULL;

			for(i = 0, b = a->a_nvals; b[i].bv_val; i++) {
				count++;

				if(dnIsSuffix(&b[i], &rq->oldndn)) {
					/* first match? create structure */
					if ( na == NULL ) {
						na = op->o_tmpcalloc( 1,
							sizeof( refint_attrs ),
							op->o_tmpmemctx );
						na->next = ip->attrs;
						ip->attrs = na;
						na->attr = ia->attr;

						/* delete, or exact match? note it's first match */
						if ( BER_BVISEMPTY( &rq->newdn ) &&
							b[i].bv_len == rq->oldndn.bv_len )
						{
							first = i;
						}
					}

					/* if it's a rename, or a subordinate match,
					 * save old and build new dn */
					if ( !BER_BVISEMPTY( &rq->newdn ) &&
						b[i].bv_len != rq->oldndn.bv_len )
					{
						struct berval	newsub, newdn, olddn, oldndn;

						/* if not first, save first as well */
						if ( first != -1 ) {

							ber_dupbv_x( &olddn, &a->a_vals[first], op->o_tmpmemctx );
							ber_bvarray_add_x( &na->old_vals, &olddn, op->o_tmpmemctx );
							ber_dupbv_x( &oldndn, &a->a_nvals[first], op->o_tmpmemctx );
							ber_bvarray_add_x( &na->old_nvals, &oldndn, op->o_tmpmemctx );
							na->ra_numvals++;

							newsub = a->a_vals[first];
							newsub.bv_len -= rq->olddn.bv_len + 1;

							build_new_dn( &newdn, &rq->newdn, &newsub, op->o_tmpmemctx );

							ber_bvarray_add_x( &na->new_vals, &newdn, op->o_tmpmemctx );

							newsub = a->a_nvals[first];
							newsub.bv_len -= rq->oldndn.bv_len + 1;

							build_new_dn( &newdn, &rq->newndn, &newsub, op->o_tmpmemctx );

							ber_bvarray_add_x( &na->new_nvals, &newdn, op->o_tmpmemctx );
							
							first = -1;
						}

						ber_dupbv_x( &olddn, &a->a_vals[i], op->o_tmpmemctx );
						ber_bvarray_add_x( &na->old_vals, &olddn, op->o_tmpmemctx );
						ber_dupbv_x( &oldndn, &a->a_nvals[i], op->o_tmpmemctx );
						ber_bvarray_add_x( &na->old_nvals, &oldndn, op->o_tmpmemctx );
						na->ra_numvals++;

						newsub = a->a_vals[i];
						newsub.bv_len -= rq->olddn.bv_len + 1;

						build_new_dn( &newdn, &rq->newdn, &newsub, op->o_tmpmemctx );

						ber_bvarray_add_x( &na->new_vals, &newdn, op->o_tmpmemctx );

						newsub = a->a_nvals[i];
						newsub.bv_len -= rq->oldndn.bv_len + 1;

						build_new_dn( &newdn, &rq->newndn, &newsub, op->o_tmpmemctx );

						ber_bvarray_add_x( &na->new_nvals, &newdn, op->o_tmpmemctx );
					}

					/* count deletes */
					if ( BER_BVISEMPTY( &rq->newdn ) ) {
						deleted++;
					}
				}

				/* If this is a delete and no value would be left, and
				 * we have a nothing DN configured, allocate the attr again.
				 */
				if ( count == deleted && !BER_BVISNULL(&dd->nothing) )
				{
					na = op->o_tmpcalloc( 1,
						sizeof( refint_attrs ),
						op->o_tmpmemctx );
					na->next = ip->attrs;
					ip->attrs = na;
					na->attr = ia->attr;
				}

				Debug( LDAP_DEBUG_TRACE, "refint_search_cb: %s: %s (#%d)\n",
					a->a_desc->ad_cname.bv_val, rq->olddn.bv_val, count );
			}
		}
	}

	return(0);
}

static int
refint_repair(
	Operation	*op,
	SlapReply	*rs,
	refint_data	*id,
	refint_q	*rq )
{
	dependent_data	*dp, *dp_next;
	int		rc;

	op->o_callback->sc_response = refint_search_cb;
	op->o_req_dn = op->o_bd->be_suffix[ 0 ];
	op->o_req_ndn = op->o_bd->be_nsuffix[ 0 ];
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	/* search */
	rc = op->o_bd->be_search( op, rs );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"refint_repair: search failed: %d\n",
			rc, 0, 0 );
		return 0;
	}

	/* safety? paranoid just in case */
	if ( op->o_callback->sc_private == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"refint_repair: callback wiped out sc_private?!\n",
			0, 0, 0 );
		return 0;
	}

	/* Set up the Modify requests */
	op->o_callback->sc_response = &slap_null_cb;

	/*
	 * [our search callback builds a list of attrs]
	 * foreach attr:
	 *	make sure its dn has a backend;
	 *	build Modification* chain;
	 *	call the backend modify function;
	 *
	 */

	for ( dp = rq->attrs; dp; dp = dp_next ) {
		Operation	op2 = *op;
		SlapReply	rs2 = { 0 };
		refint_attrs	*ra;
		Modifications	*m, *first = NULL;

		dp_next = dp->next;

		op2.o_tag = LDAP_REQ_MODIFY;
		op2.orm_modlist = NULL;
		op2.o_req_dn	= dp->dn;
		op2.o_req_ndn	= dp->ndn;
		op2.o_bd = select_backend( &dp->ndn, 1 );
		if ( !op2.o_bd ) {
			Debug( LDAP_DEBUG_TRACE,
				"refint_repair: no backend for DN %s!\n",
				dp->dn.bv_val, 0, 0 );
			return 0;
		}

		rs2.sr_type = REP_RESULT;
		for ( ra = dp->attrs; ra; ra = dp->attrs ) {
			size_t	len;

			dp->attrs = ra->next;
			/* Set our ModifiersName */
			if ( SLAP_LASTMOD( op->o_bd ) ) {
				m = op2.o_tmpalloc( sizeof(Modifications) +
					4*sizeof(BerValue), op2.o_tmpmemctx );
				m->sml_next = op2.orm_modlist;
				if ( !first )
					first = m;
				op2.orm_modlist = m;
				m->sml_op = LDAP_MOD_REPLACE;
				m->sml_flags = SLAP_MOD_INTERNAL;
				m->sml_desc = slap_schema.si_ad_modifiersName;
				m->sml_type = m->sml_desc->ad_cname;
				m->sml_numvals = 1;
				m->sml_values = (BerVarray)(m+1);
				m->sml_nvalues = m->sml_values+2;
				BER_BVZERO( &m->sml_values[1] );
				BER_BVZERO( &m->sml_nvalues[1] );
				m->sml_values[0] = id->refint_dn;
				m->sml_nvalues[0] = id->refint_ndn;
			}
			if ( !BER_BVISEMPTY( &rq->newdn ) || ( ra->next &&
				ra->attr == ra->next->attr ) )
			{
				len = sizeof(Modifications);

				if ( ra->new_vals == NULL ) {
					len += 4*sizeof(BerValue);
				}

				m = op2.o_tmpalloc( len, op2.o_tmpmemctx );
				m->sml_next = op2.orm_modlist;
				if ( !first )
					first = m;
				op2.orm_modlist = m;
				m->sml_op = LDAP_MOD_ADD;
				m->sml_flags = 0;
				m->sml_desc = ra->attr;
				m->sml_type = ra->attr->ad_cname;
				if ( ra->new_vals == NULL ) {
					m->sml_values = (BerVarray)(m+1);
					m->sml_nvalues = m->sml_values+2;
					BER_BVZERO( &m->sml_values[1] );
					BER_BVZERO( &m->sml_nvalues[1] );
					m->sml_numvals = 1;
					if ( BER_BVISEMPTY( &rq->newdn ) ) {
						op2.o_tmpfree( ra, op2.o_tmpmemctx );
						ra = dp->attrs;
						dp->attrs = ra->next;
						m->sml_values[0] = id->nothing;
						m->sml_nvalues[0] = id->nnothing;
					} else {
						m->sml_values[0] = rq->newdn;
						m->sml_nvalues[0] = rq->newndn;
					}
				} else {
					m->sml_values = ra->new_vals;
					m->sml_nvalues = ra->new_nvals;
					m->sml_numvals = ra->ra_numvals;
				}
			}

			len = sizeof(Modifications);
			if ( ra->old_vals == NULL ) {
				len += 4*sizeof(BerValue);
			}

			m = op2.o_tmpalloc( len, op2.o_tmpmemctx );
			m->sml_next = op2.orm_modlist;
			op2.orm_modlist = m;
			if ( !first )
				first = m;
			m->sml_op = LDAP_MOD_DELETE;
			m->sml_flags = 0;
			m->sml_desc = ra->attr;
			m->sml_type = ra->attr->ad_cname;
			if ( ra->old_vals == NULL ) {
				m->sml_numvals = 1;
				m->sml_values = (BerVarray)(m+1);
				m->sml_nvalues = m->sml_values+2;
				m->sml_values[0] = rq->olddn;
				m->sml_nvalues[0] = rq->oldndn;
				BER_BVZERO( &m->sml_values[1] );
				BER_BVZERO( &m->sml_nvalues[1] );
			} else {
				m->sml_values = ra->old_vals;
				m->sml_nvalues = ra->old_nvals;
				m->sml_numvals = ra->ra_numvals;
			}
			op2.o_tmpfree( ra, op2.o_tmpmemctx );
		}

		op2.o_dn = op2.o_bd->be_rootdn;
		op2.o_ndn = op2.o_bd->be_rootndn;
		slap_op_time( &op2.o_time, &op2.o_tincr );
		if ( ( rc = op2.o_bd->be_modify( &op2, &rs2 ) ) != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"refint_repair: dependent modify failed: %d\n",
				rs2.sr_err, 0, 0 );
		}

		while ( ( m = op2.orm_modlist ) ) {
			op2.orm_modlist = m->sml_next;
			if ( m->sml_values && m->sml_values != (BerVarray)(m+1) ) {
				ber_bvarray_free_x( m->sml_values, op2.o_tmpmemctx );
				ber_bvarray_free_x( m->sml_nvalues, op2.o_tmpmemctx );
			}
			op2.o_tmpfree( m, op2.o_tmpmemctx );
			if ( m == first ) break;
		}
		slap_mods_free( op2.orm_modlist, 1 );
		op2.o_tmpfree( dp->ndn.bv_val, op2.o_tmpmemctx );
		op2.o_tmpfree( dp->dn.bv_val, op2.o_tmpmemctx );
		op2.o_tmpfree( dp, op2.o_tmpmemctx );
	}

	return 0;
}

static void *
refint_qtask( void *ctx, void *arg )
{
	struct re_s *rtask = arg;
	refint_data *id = rtask->arg;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	SlapReply rs = {REP_RESULT};
	slap_callback cb = { NULL, NULL, NULL, NULL };
	Filter ftop, *fptr;
	refint_q *rq;
	refint_attrs *ip;

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	/*
	** build a search filter for all configured attributes;
	** populate our Operation;
	** pass our data (attr list, dn) to backend via sc_private;
	** call the backend search function;
	** nb: (|(one=thing)) is valid, but do smart formatting anyway;
	** nb: 16 is arbitrarily a dozen or so extra bytes;
	**
	*/

	ftop.f_choice = LDAP_FILTER_OR;
	ftop.f_next = NULL;
	ftop.f_or = NULL;
	op->ors_filter = &ftop;
	for(ip = id->attrs; ip; ip = ip->next) {
		fptr = op->o_tmpcalloc( sizeof(Filter) + sizeof(MatchingRuleAssertion),
			1, op->o_tmpmemctx );
		/* Use (attr:dnSubtreeMatch:=value) to catch subtree rename
		 * and subtree delete where supported */
		fptr->f_choice = LDAP_FILTER_EXT;
		fptr->f_mra = (MatchingRuleAssertion *)(fptr+1);
		fptr->f_mr_rule = mr_dnSubtreeMatch;
		fptr->f_mr_rule_text = mr_dnSubtreeMatch->smr_bvoid;
		fptr->f_mr_desc = ip->attr;
		fptr->f_mr_dnattrs = 0;
		fptr->f_next = ftop.f_or;
		ftop.f_or = fptr;
	}

	for (;;) {
		/* Dequeue an op */
		ldap_pvt_thread_mutex_lock( &id->qmutex );
		rq = id->qhead;
		if ( rq ) {
			id->qhead = rq->next;
			if ( !id->qhead )
				id->qtail = NULL;
		}
		ldap_pvt_thread_mutex_unlock( &id->qmutex );
		if ( !rq )
			break;

		for (fptr = ftop.f_or; fptr; fptr = fptr->f_next )
			fptr->f_mr_value = rq->oldndn;

		filter2bv_x( op, op->ors_filter, &op->ors_filterstr );

		/* callback gets the searched dn instead */
		cb.sc_private	= rq;
		cb.sc_response	= refint_search_cb;
		op->o_callback	= &cb;
		op->o_tag	= LDAP_REQ_SEARCH;
		op->ors_scope	= LDAP_SCOPE_SUBTREE;
		op->ors_deref	= LDAP_DEREF_NEVER;
		op->ors_limit   = NULL;
		op->ors_slimit	= SLAP_NO_LIMIT;
		op->ors_tlimit	= SLAP_NO_LIMIT;

		/* no attrs! */
		op->ors_attrs = slap_anlist_no_attrs;

		slap_op_time( &op->o_time, &op->o_tincr );

		if ( rq->db != NULL ) {
			op->o_bd = rq->db;
			refint_repair( op, &rs, id, rq );

		} else {
			BackendDB	*be;

			LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
				/* we may want to skip cn=config */
				if ( be == LDAP_STAILQ_FIRST(&backendDB) ) {
					continue;
				}

				if ( be->be_search && be->be_modify ) {
					op->o_bd = be;
					refint_repair( op, &rs, id, rq );
				}
			}
		}

		if ( !BER_BVISNULL( &rq->newndn )) {
			ch_free( rq->newndn.bv_val );
			ch_free( rq->newdn.bv_val );
		}
		ch_free( rq->oldndn.bv_val );
		ch_free( rq->olddn.bv_val );
		ch_free( rq );
	}

	/* free filter */
	for ( fptr = ftop.f_or; fptr; ) {
		Filter *f_next = fptr->f_next;
		op->o_tmpfree( fptr, op->o_tmpmemctx );
		fptr = f_next;
	}

	/* wait until we get explicitly scheduled again */
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, id->qtask );
	ldap_pvt_runqueue_resched( &slapd_rq,id->qtask, 1 );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return NULL;
}

/*
** refint_response
** search for matching records and modify them
*/

static int
refint_response(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	refint_data *id = on->on_bi.bi_private;
	BerValue pdn;
	int ac;
	refint_q *rq;
	BackendDB *db = NULL;
	refint_attrs *ip;

	id->message = "_refint_response";

	/* If the main op failed or is not a Delete or ModRdn, ignore it */
	if (( op->o_tag != LDAP_REQ_DELETE && op->o_tag != LDAP_REQ_MODRDN ) ||
		rs->sr_err != LDAP_SUCCESS )
		return SLAP_CB_CONTINUE;

	/*
	** validate (and count) the list of attrs;
	**
	*/

	for(ip = id->attrs, ac = 0; ip; ip = ip->next, ac++);
	if(!ac) {
		Debug( LDAP_DEBUG_TRACE,
			"refint_response called without any attributes\n", 0, 0, 0 );
		return SLAP_CB_CONTINUE;
	}

	/*
	** find the backend that matches our configured basedn;
	** make sure it exists and has search and modify methods;
	**
	*/

	if ( on->on_info->oi_origdb != frontendDB ) {
		db = select_backend(&id->dn, 1);

		if ( db ) {
			if ( !db->be_search || !db->be_modify ) {
				Debug( LDAP_DEBUG_TRACE,
					"refint_response: backend missing search and/or modify\n",
					0, 0, 0 );
				return SLAP_CB_CONTINUE;
			}
		} else {
			Debug( LDAP_DEBUG_TRACE,
				"refint_response: no backend for our baseDN %s??\n",
				id->dn.bv_val, 0, 0 );
			return SLAP_CB_CONTINUE;
		}
	}

	rq = ch_calloc( 1, sizeof( refint_q ));
	ber_dupbv( &rq->olddn, &op->o_req_dn );
	ber_dupbv( &rq->oldndn, &op->o_req_ndn );
	rq->db = db;
	rq->rdata = id;

	if ( op->o_tag == LDAP_REQ_MODRDN ) {
		if ( op->oq_modrdn.rs_newSup ) {
			pdn = *op->oq_modrdn.rs_newSup;
		} else {
			dnParent( &op->o_req_dn, &pdn );
		}
		build_new_dn( &rq->newdn, &pdn, &op->orr_newrdn, NULL );
		if ( op->oq_modrdn.rs_nnewSup ) {
			pdn = *op->oq_modrdn.rs_nnewSup;
		} else {
			dnParent( &op->o_req_ndn, &pdn );
		}
		build_new_dn( &rq->newndn, &pdn, &op->orr_nnewrdn, NULL );
	}

	ldap_pvt_thread_mutex_lock( &id->qmutex );
	if ( id->qtail ) {
		id->qtail->next = rq;
	} else {
		id->qhead = rq;
	}
	id->qtail = rq;
	ldap_pvt_thread_mutex_unlock( &id->qmutex );

	ac = 0;
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	if ( !id->qtask ) {
		id->qtask = ldap_pvt_runqueue_insert( &slapd_rq, RUNQ_INTERVAL,
			refint_qtask, id, "refint_qtask",
			op->o_bd->be_suffix[0].bv_val );
		ac = 1;
	} else {
		if ( !ldap_pvt_runqueue_isrunning( &slapd_rq, id->qtask ) &&
			!id->qtask->next_sched.tv_sec ) {
			id->qtask->interval.tv_sec = 0;
			ldap_pvt_runqueue_resched( &slapd_rq, id->qtask, 0 );
			id->qtask->interval.tv_sec = RUNQ_INTERVAL;
			ac = 1;
		}
	}
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	if ( ac )
		slap_wake_listener();

	return SLAP_CB_CONTINUE;
}

/*
** init_module is last so the symbols resolve "for free" --
** it expects to be called automagically during dynamic module initialization
*/

int refint_initialize() {
	int rc;

	mr_dnSubtreeMatch = mr_find( "dnSubtreeMatch" );
	if ( mr_dnSubtreeMatch == NULL ) {
		Debug( LDAP_DEBUG_ANY, "refint_initialize: "
			"unable to find MatchingRule 'dnSubtreeMatch'.\n",
			0, 0, 0 );
		return 1;
	}

	/* statically declared just after the #includes at top */
	refint.on_bi.bi_type = "refint";
	refint.on_bi.bi_db_init = refint_db_init;
	refint.on_bi.bi_db_destroy = refint_db_destroy;
	refint.on_bi.bi_db_open = refint_open;
	refint.on_bi.bi_db_close = refint_close;
	refint.on_response = refint_response;

	refint.on_bi.bi_cf_ocs = refintocs;
	rc = config_register_schema ( refintcfg, refintocs );
	if ( rc ) return rc;

	return(overlay_register(&refint));
}

#if SLAPD_OVER_REFINT == SLAPD_MOD_DYNAMIC && defined(PIC)
int init_module(int argc, char *argv[]) {
	return refint_initialize();
}
#endif

#endif /* SLAPD_OVER_REFINT */
