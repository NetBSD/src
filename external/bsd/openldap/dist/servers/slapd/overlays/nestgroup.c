/*	$NetBSD: nestgroup.c,v 1.2 2025/09/05 21:16:32 christos Exp $	*/

/* nestgroup.c - nested group overlay */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2024 The OpenLDAP Foundation.
 * Copyright 2024 by Howard Chu.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: nestgroup.c,v 1.2 2025/09/05 21:16:32 christos Exp $");

#include "portable.h"

#ifdef SLAPD_OVER_NESTGROUP

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "slap-config.h"

/* This overlay dynamically constructs member and memberOf attributes
 * for nested groups.
 */

#define SLAPD_MEMBEROF_ATTR	"memberOf"

#define NG_MBR_VALUES	0x01
#define NG_MBR_FILTER	0x02
#define NG_MOF_VALUES	0x04
#define NG_MOF_FILTER	0x08
#define NG_NEGATED		0x10

static AttributeDescription *ad_member;
static AttributeDescription *ad_memberOf;

static slap_verbmasks nestgroup_flags[] = {
	{ BER_BVC("member-values"),	NG_MBR_VALUES },
	{ BER_BVC("member-filter"),	NG_MBR_FILTER },
	{ BER_BVC("memberof-values"),	NG_MOF_VALUES },
	{ BER_BVC("memberof-filter"),	NG_MOF_FILTER },
	{ BER_BVNULL,	0 }
};

enum {
	NG_MEMBER = 1,
	NG_MEMBEROF,
	NG_GROUPBASE,
	NG_FLAGS
};

typedef struct nestgroup_info_t {
	AttributeDescription *ngi_member;
	AttributeDescription *ngi_memberOf;
	BerVarray ngi_groupBase;
	BerVarray ngi_ngroupBase;
	int ngi_flags;
} nestgroup_info_t;

static int ngroup_cf( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	nestgroup_info_t *ngi = (nestgroup_info_t *)on->on_bi.bi_private;
	int rc = 1;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case NG_MEMBER:
			if ( ngi->ngi_member ) {
				value_add_one( &c->rvalue_vals, &ngi->ngi_member->ad_cname );
				rc = 0;
			}
			break;
		case NG_MEMBEROF:
			if ( ngi->ngi_memberOf ) {
				value_add_one( &c->rvalue_vals, &ngi->ngi_memberOf->ad_cname );
				rc = 0;
			}
			break;
		case NG_GROUPBASE:
			if ( ngi->ngi_groupBase ) {
				value_add( &c->rvalue_vals, ngi->ngi_groupBase );
				value_add( &c->rvalue_nvals, ngi->ngi_ngroupBase );
				rc = 0;
			}
			break;
		case NG_FLAGS:
			return mask_to_verbs( nestgroup_flags, ngi->ngi_flags, &c->rvalue_vals );
		default:
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case NG_MEMBER:
			ngi->ngi_member = ad_member;
			rc = 0;
			break;
		case NG_MEMBEROF:
			ngi->ngi_memberOf = ad_memberOf;
			rc = 0;
			break;
		case NG_GROUPBASE:
			if ( c->valx < 0 ) {
				ber_bvarray_free( ngi->ngi_groupBase );
				ber_bvarray_free( ngi->ngi_ngroupBase );
				ngi->ngi_groupBase = NULL;
				ngi->ngi_ngroupBase = NULL;
			} else {
				int i = c->valx;
				ch_free( ngi->ngi_groupBase[i].bv_val );
				ch_free( ngi->ngi_ngroupBase[i].bv_val );
				do {
					ngi->ngi_groupBase[i] = ngi->ngi_groupBase[i+1];
					ngi->ngi_ngroupBase[i] = ngi->ngi_ngroupBase[i+1];
					i++;
				} while ( !BER_BVISNULL( &ngi->ngi_groupBase[i] ));
			}
			rc = 0;
			break;
		case NG_FLAGS:
			if ( !c->line ) {
				ngi->ngi_flags = 0;
			} else {
				int i = verb_to_mask( c->line, nestgroup_flags );
				ngi->ngi_flags &= ~nestgroup_flags[i].mask;
			}
			rc = 0;
			break;
		default:
			break;
		}
		return rc;
	}

	switch( c->type ) {
	case NG_MEMBER:
		if ( !is_at_syntax( c->value_ad->ad_type, SLAPD_DN_SYNTAX ) &&
			!is_at_syntax( c->value_ad->ad_type, SLAPD_NAMEUID_SYNTAX )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"member attribute=\"%s\" must use DN (%s) or NAMEUID (%s) syntax",
				c->argv[1], SLAPD_DN_SYNTAX, SLAPD_NAMEUID_SYNTAX );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		ngi->ngi_member = c->value_ad;
		rc = 0;
		break;
	case NG_MEMBEROF:
		if ( !is_at_syntax( c->value_ad->ad_type, SLAPD_DN_SYNTAX ) &&
			!is_at_syntax( c->value_ad->ad_type, SLAPD_NAMEUID_SYNTAX )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"memberOf attribute=\"%s\" must use DN (%s) or NAMEUID (%s) syntax",
				c->argv[1], SLAPD_DN_SYNTAX, SLAPD_NAMEUID_SYNTAX );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		ngi->ngi_memberOf = c->value_ad;
		rc = 0;
		break;
	case NG_GROUPBASE:
		ber_bvarray_add( &ngi->ngi_groupBase, &c->value_dn );
		ber_bvarray_add( &ngi->ngi_ngroupBase, &c->value_ndn );
		rc = 0;
		break;
	case NG_FLAGS: {
		slap_mask_t flags = 0;
		int i;
		if ( c->op != SLAP_CONFIG_ADD && c->argc > 2 ) {
			/* We wouldn't know how to delete these values later */
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"Please insert multiple names as separate %s values",
				c->argv[0] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			rc = LDAP_INVALID_SYNTAX;
			break;
		}
		i = verbs_to_mask( c->argc, c->argv, nestgroup_flags, &flags );
		if ( i ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown option", c->argv[0] );
			Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
				c->log, c->cr_msg, c->argv[i]);
			return(1);
		}
		ngi->ngi_flags |= flags;
		rc = 0;
		break; }
	default:
		break;
	}

	return rc;
}

static ConfigTable ngroupcfg[] = {
	{ "nestgroup-member", "member-ad", 2, 2, 0,
	  ARG_MAGIC|ARG_ATDESC|NG_MEMBER, ngroup_cf,
	  "( OLcfgOvAt:25.1 NAME 'olcNestGroupMember' "
	  "EQUALITY caseIgnoreMatch "
	  "DESC 'Member attribute' "
	  "SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "nestgroup-memberof", "memberOf-ad", 2, 2, 0,
	  ARG_MAGIC|ARG_ATDESC|NG_MEMBEROF, ngroup_cf,
	  "( OLcfgOvAt:25.2 NAME 'olcNestGroupMemberOf' "
	  "EQUALITY caseIgnoreMatch "
	  "DESC 'MemberOf attribute' "
	  "SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "nestgroup-base", "dn", 2, 2, 0,
	  ARG_DN|ARG_QUOTE|ARG_MAGIC|NG_GROUPBASE, ngroup_cf,
	  "( OLcfgOvAt:25.3 NAME 'olcNestGroupBase' "
	  "EQUALITY distinguishedNameMatch "
	  "DESC 'Base[s] of group subtree[s]' "
	  "SYNTAX OMsDN )", NULL, NULL },
	{ "nestgroup-flags", "options", 2, 0, 0,
	  ARG_MAGIC|NG_FLAGS, ngroup_cf,
	  "( OLcfgOvAt:25.4 NAME 'olcNestGroupFlags' "
	  "EQUALITY caseIgnoreMatch "
	  "DESC 'Features to use' "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs ngroupocs[] = {
	{ "( OLcfgOvOc:25.1 "
	  "NAME 'olcNestGroupConfig' "
	  "DESC 'Nested Group configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcNestGroupMember $ olcNestGroupMemberOf $ "
	  " olcNestGroupBase $ olcNestGroupFlags ) ) ",
	  Cft_Overlay, ngroupcfg },
	{ NULL, 0, NULL }
};

typedef struct nestgroup_filterinst_t {
	Filter *nf_f;
	Filter *nf_new;
	Entry *nf_e;
} nestgroup_filterinst_t;

/* Record occurrences of ad in filter. Ignore in negated filters. */
static void
nestgroup_filter_instances( Operation *op, AttributeDescription *ad, Filter *f, int not,
	int *nfn, nestgroup_filterinst_t **nfp, int *negated )
{
	if ( !f )
		return;

	switch( f->f_choice & SLAPD_FILTER_MASK ) {
	case LDAP_FILTER_EQUALITY:
		if ( f->f_av_desc == ad ) {
			if ( not ) {
				*negated = 1;
			} else {
				nestgroup_filterinst_t *nf = *nfp;
				int n = *nfn;
				nf = op->o_tmprealloc( nf, (n + 1) * sizeof(nestgroup_filterinst_t), op->o_tmpmemctx );
				nf[n].nf_f = f;
				nf[n].nf_new = NULL;
				nf[n++].nf_e = NULL;
				*nfp = nf;
				*nfn = n;
			}
		}
		break;
	case SLAPD_FILTER_COMPUTED:
	case LDAP_FILTER_PRESENT:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_APPROX:
	case LDAP_FILTER_SUBSTRINGS:
	case LDAP_FILTER_EXT:
		break;
	case LDAP_FILTER_NOT:	not ^= 1;
		/* FALLTHRU */
	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
		for ( f = f->f_list; f; f = f->f_next )
			nestgroup_filter_instances( op, ad, f, not, nfn, nfp, negated );
	}
}

static int
nestgroup_check_needed( Operation *op, int attrflags, AttributeDescription *ad )
{
	if ( is_at_operational( ad->ad_type )) {
		if ( SLAP_OPATTRS( attrflags ))
			return 1;
	} else {
		if ( SLAP_USERATTRS( attrflags ))
			return 1;
	}
	return ( ad_inlist( ad, op->ors_attrs ));
}

typedef struct DNpair {
	struct berval dp_ndn;
	struct berval dp_dn;
	struct DNpair *dp_next;
	int dp_flag;
} DNpair;

typedef struct gdn_info {
	TAvlnode *gi_DNs;
	DNpair *gi_DNlist;
	nestgroup_info_t *gi_ngi;
	int gi_numDNs;
	int gi_saveDN;
	Attribute *gi_merge;
} gdn_info;

static int
nestgroup_dncmp( const void *v1, const void *v2 )
{
	return ber_bvcmp((const struct berval *)v1, (const struct berval *)v2);
}

static int
nestgroup_gotDNresp( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		gdn_info *gi = (gdn_info *)(op->o_callback+1);
		DNpair *dp = op->o_tmpalloc( sizeof(DNpair), op->o_tmpmemctx );
		dp->dp_ndn = rs->sr_entry->e_nname;
		if ( ldap_tavl_insert( &gi->gi_DNs, dp, nestgroup_dncmp, ldap_avl_dup_error )) {
			op->o_tmpfree( dp, op->o_tmpmemctx );
		} else {
			ber_dupbv_x( &dp->dp_ndn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
			if ( gi->gi_saveDN )
				ber_dupbv_x( &dp->dp_dn, &rs->sr_entry->e_name, op->o_tmpmemctx );
			gi->gi_numDNs++;
			dp->dp_next = gi->gi_DNlist;
			dp->dp_flag = 0;
			gi->gi_DNlist = dp;
		}
	}
	return 0;
}

static void
nestgroup_get_parentDNs( Operation *op, struct berval *ndn )
{
	SlapReply r = { REP_SEARCH };
	gdn_info *gi = (gdn_info *)(op->o_callback+1);
	nestgroup_info_t *ngi = gi->gi_ngi;
	int i;

	op->ors_filter->f_av_value = *ndn;
	for ( i=0; !BER_BVISEMPTY( &ngi->ngi_ngroupBase[i] ); i++ ) {
		op->o_req_dn = ngi->ngi_groupBase[i];
		op->o_req_ndn = ngi->ngi_ngroupBase[i];
		op->o_bd->be_search( op, &r );
	}
	gi->gi_numDNs = 0; /* ignore first count, that's just the original member= result set */

	while ( gi->gi_DNlist ) {
		int prevnum;
		DNpair *dp = gi->gi_DNlist;
		gi->gi_DNlist = NULL;
		for ( ; dp; dp=dp->dp_next ) {
			op->ors_filter->f_av_value = dp->dp_ndn;
			prevnum = gi->gi_numDNs;
			for ( i=0; !BER_BVISEMPTY( &ngi->ngi_ngroupBase[i] ); i++ ) {
				op->o_req_dn = ngi->ngi_groupBase[i];
				op->o_req_ndn = ngi->ngi_ngroupBase[i];
				op->o_bd->be_search( op, &r );
			}
			if ( gi->gi_numDNs > prevnum )
				dp->dp_flag = 1;	/* this group had a parent */
		}
	}
}

static void
nestgroup_memberFilter( Operation *op, int mbr_nf, nestgroup_filterinst_t *mbr_f )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	nestgroup_info_t *ngi = on->on_bi.bi_private;
	AttributeDescription *ad = mbr_f[0].nf_f->f_av_desc;
	slap_callback *sc;
	gdn_info *gi;
	Filter mf;
	AttributeAssertion mava;
	Operation o = *op;
	int i;

	o.o_managedsait = SLAP_CONTROL_CRITICAL;
	sc = op->o_tmpcalloc( 1, sizeof(slap_callback) + sizeof(gdn_info), op->o_tmpmemctx);
	gi = (gdn_info *)(sc+1);
	gi->gi_ngi = ngi;
	o.o_callback = sc;
	sc->sc_response = nestgroup_gotDNresp;
	o.ors_attrs = slap_anlist_no_attrs;

	mf.f_choice = LDAP_FILTER_EQUALITY;
	mf.f_ava = &mava;
	mf.f_av_desc = ad;
	mf.f_next = NULL;

	o.ors_scope = LDAP_SCOPE_SUBTREE;
	o.ors_deref = LDAP_DEREF_NEVER;
	o.ors_limit = NULL;
	o.ors_tlimit = SLAP_NO_LIMIT;
	o.ors_slimit = SLAP_NO_LIMIT;
	o.ors_filter = &mf;
	o.o_bd->bd_info = (BackendInfo *)on->on_info;

	for ( i=0; i<mbr_nf; i++ ) {
		gi->gi_DNs = NULL;
		gi->gi_numDNs = 0;
		nestgroup_get_parentDNs( &o, &mbr_f[i].nf_f->f_av_value );
		if ( gi->gi_numDNs ) {
			int j;
			Filter *f, *nf;
			TAvlnode *t;
			DNpair *dp;

			f = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
			f->f_next = NULL;
			t = ldap_tavl_end( gi->gi_DNs, TAVL_DIR_RIGHT );
			do {
				dp = t->avl_data;
				if ( dp->dp_flag ) {
					nf = f;
					nf->f_ava = op->o_tmpcalloc( 1, sizeof( AttributeAssertion ), op->o_tmpmemctx );
					nf->f_choice = LDAP_FILTER_EQUALITY;
					nf->f_av_desc = ad;
					nf->f_av_value = dp->dp_ndn;
					f = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
					f->f_next = nf;
				}
				t = ldap_tavl_next( t, TAVL_DIR_LEFT );
				op->o_tmpfree( dp, op->o_tmpmemctx );
			} while ( t );
			f->f_choice = LDAP_FILTER_EQUALITY;
			f->f_ava = mbr_f[i].nf_f->f_ava;
			mbr_f[i].nf_new = f;
		}
		ldap_tavl_free( gi->gi_DNs, NULL );
	}
	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	op->o_tmpfree( sc, op->o_tmpmemctx );
}

static void
nestgroup_addUnique( Operation *op, Attribute *old, Attribute *new )
{
	/* strip out any duplicates from new before adding */
	struct berval *bv, *nbv;
	int i, j, flags;

	bv = op->o_tmpalloc( (new->a_numvals + 1) * 2 * sizeof(struct berval), op->o_tmpmemctx );
	nbv = bv + new->a_numvals+1;

	flags = SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH|SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH;
	for (i=0,j=0; i<new->a_numvals; i++) {
		int rc = attr_valfind( old, flags, &new->a_nvals[i], NULL, NULL );
		if ( rc ) {
			bv[j] = new->a_vals[i];
			nbv[j++] = new->a_nvals[i];
		}
	}
	BER_BVZERO( &bv[j] );
	BER_BVZERO( &nbv[j] );
	attr_valadd( old, bv, nbv, j );
	op->o_tmpfree( bv, op->o_tmpmemctx );
}

static void
nestgroup_get_childDNs( Operation *op, slap_overinst *on, gdn_info *gi, struct berval *ndn )
{
	nestgroup_info_t *ngi = on->on_bi.bi_private;
	Entry *e;
	Attribute *a;

	if ( overlay_entry_get_ov( op, ndn, NULL, NULL, 0, &e, on ) != LDAP_SUCCESS || e == NULL )
		return;

	a = attr_find( e->e_attrs, ngi->ngi_member );
	if ( a ) {
		int i, j;
		for (i = 0; i<a->a_numvals; i++ ) {
			/* record all group entries */
			for (j = 0; !BER_BVISEMPTY( &ngi->ngi_groupBase[j] ); j++) {
				if ( dnIsSuffix( &a->a_nvals[i], &ngi->ngi_ngroupBase[j] )) {
					DNpair *dp = op->o_tmpalloc( sizeof(DNpair), op->o_tmpmemctx );
					dp->dp_ndn = a->a_nvals[i];
					if ( ldap_tavl_insert( &gi->gi_DNs, dp, nestgroup_dncmp, ldap_avl_dup_error )) {
						op->o_tmpfree( dp, op->o_tmpmemctx );
					} else {
						ber_dupbv_x( &dp->dp_ndn, &a->a_nvals[i], op->o_tmpmemctx );
						gi->gi_numDNs++;
						dp->dp_next = gi->gi_DNlist;
						gi->gi_DNlist = dp;
					}
					break;
				}
			}
		}
		if ( gi->gi_merge ) {
			nestgroup_addUnique( op, gi->gi_merge, a );
		}
	}
	overlay_entry_release_ov( op, e, 0, on );
}

static void
nestgroup_memberOfFilter( Operation *op, int mof_nf, nestgroup_filterinst_t *mof_f )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	AttributeDescription *ad = mof_f[0].nf_f->f_av_desc;
	gdn_info gi = {0};
	int i;

	for ( i=0; i<mof_nf; i++ ) {
		gi.gi_DNs = NULL;
		gi.gi_numDNs = 0;
		nestgroup_get_childDNs( op, on, &gi, &mof_f[i].nf_f->f_av_value );

		while ( gi.gi_DNlist ) {
			DNpair *dp = gi.gi_DNlist;
			gi.gi_DNlist = NULL;
			for ( ; dp; dp=dp->dp_next ) {
				nestgroup_get_childDNs( op, on, &gi, &dp->dp_ndn );
			}
		}

		if ( gi.gi_numDNs ) {
			int j;
			Filter *f, *nf;
			TAvlnode *t;
			DNpair *dp;

			f = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
			f->f_next = NULL;
			t = ldap_tavl_end( gi.gi_DNs, TAVL_DIR_RIGHT );
			do {
				dp = t->avl_data;
				nf = f;
				nf->f_ava = op->o_tmpcalloc( 1, sizeof( AttributeAssertion ), op->o_tmpmemctx );
				nf->f_choice = LDAP_FILTER_EQUALITY;
				nf->f_av_desc = ad;
				nf->f_av_value = dp->dp_ndn;
				f = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
				f->f_next = nf;
				t = ldap_tavl_next( t, TAVL_DIR_LEFT );
				op->o_tmpfree( dp, op->o_tmpmemctx );
			} while ( t );
			ldap_tavl_free( gi.gi_DNs, NULL );
			f->f_choice = LDAP_FILTER_EQUALITY;
			f->f_ava = mof_f[i].nf_f->f_ava;
			mof_f[i].nf_new = f;
		}
	}
}

static void
nestgroup_memberOfVals( Operation *op, slap_overinst *on, Attribute *a )
{
	nestgroup_info_t *ngi = on->on_bi.bi_private;
	Operation o = *op;
	slap_callback *sc;
	gdn_info *gi;
	Filter mf;
	AttributeAssertion mava;
	int i;

	o.o_managedsait = SLAP_CONTROL_CRITICAL;
	sc = op->o_tmpcalloc( 1, sizeof(slap_callback) + sizeof(gdn_info), op->o_tmpmemctx);
	gi = (gdn_info *)(sc+1);
	gi->gi_ngi = ngi;
	o.o_callback = sc;
	sc->sc_response = nestgroup_gotDNresp;
	o.ors_attrs = slap_anlist_no_attrs;

	mf.f_choice = LDAP_FILTER_EQUALITY;
	mf.f_ava = &mava;
	mf.f_av_desc = ngi->ngi_member;
	mf.f_next = NULL;

	o.ors_filter = &mf;
	o.ors_scope = LDAP_SCOPE_SUBTREE;
	o.ors_deref = LDAP_DEREF_NEVER;
	o.ors_limit = NULL;
	o.ors_tlimit = SLAP_NO_LIMIT;
	o.ors_slimit = SLAP_NO_LIMIT;
	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	gi->gi_saveDN = 1;

	for ( i=0; i<a->a_numvals; i++ ) {
		nestgroup_get_parentDNs( &o, &a->a_nvals[i] );

		while ( gi->gi_DNlist ) {
			DNpair *dp = gi->gi_DNlist;
			gi->gi_DNlist = NULL;
			for ( ; dp; dp=dp->dp_next ) {
				nestgroup_get_parentDNs( &o, &dp->dp_ndn );
			}
		}
	}
	if ( gi->gi_DNs ) {
		TAvlnode *p = ldap_tavl_end( gi->gi_DNs, TAVL_DIR_LEFT );
		int flags = SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH|SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH;
		do {
			DNpair *dp = p->avl_data;
			int rc = attr_valfind( a, flags, &dp->dp_ndn, NULL, NULL );
			if ( rc )
				attr_valadd( a, &dp->dp_dn, &dp->dp_ndn, 1 );
			op->o_tmpfree( dp->dp_dn.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( dp->dp_ndn.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( dp, op->o_tmpmemctx );
			p = ldap_tavl_next( p, TAVL_DIR_RIGHT );
		} while ( p );
		ldap_tavl_free( gi->gi_DNs, NULL );
	}
	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	op->o_tmpfree( sc, op->o_tmpmemctx );
}

typedef struct nestgroup_cbinfo {
	slap_overinst *nc_on;
	int nc_needed;
} nestgroup_cbinfo;

static int
nestgroup_searchresp( Operation *op, SlapReply *rs )
{
	if (rs->sr_type == REP_SEARCH ) {
		nestgroup_cbinfo *nc = op->o_callback->sc_private;
		slap_overinst *on = nc->nc_on;
		nestgroup_info_t *ngi = on->on_bi.bi_private;
		Attribute *a;

		if ( nc->nc_needed & NG_MBR_VALUES ) {
			a = attr_find( rs->sr_entry->e_attrs, ngi->ngi_member );
			if ( a ) {
				gdn_info gi = {0};
				int i, j;
				if ( !( rs->sr_flags & REP_ENTRY_MODIFIABLE )) {
					Entry *e = entry_dup( rs->sr_entry );
					rs_replace_entry( op, rs, on, e );
					rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
					a = attr_find( e->e_attrs, ngi->ngi_member );
				}
				gi.gi_merge = a;

				for ( i=0; i<a->a_numvals; i++ ) {
					for ( j=0; !BER_BVISEMPTY( &ngi->ngi_ngroupBase[j] ); j++ ) {
						if ( dnIsSuffix( &a->a_nvals[i], &ngi->ngi_ngroupBase[j] )) {
							nestgroup_get_childDNs( op, on, &gi, &a->a_nvals[i] );

							while ( gi.gi_DNlist ) {
								DNpair *dp = gi.gi_DNlist;
								gi.gi_DNlist = NULL;
								for ( ; dp; dp=dp->dp_next ) {
									nestgroup_get_childDNs( op, on, &gi, &dp->dp_ndn );
								}
							}
							break;
						}
					}
				}
				if ( gi.gi_numDNs ) {
					TAvlnode *p = ldap_tavl_end( gi.gi_DNs, TAVL_DIR_LEFT );
					do {
						DNpair *dp = p->avl_data;
						op->o_tmpfree( dp->dp_ndn.bv_val, op->o_tmpmemctx );
						op->o_tmpfree( dp, op->o_tmpmemctx );
						p = ldap_tavl_next( p, TAVL_DIR_RIGHT );
					} while ( p );
					ldap_tavl_free( gi.gi_DNs, NULL );
				}
			}
		}

		if ( nc->nc_needed & NG_MOF_VALUES ) {
			a = attr_find( rs->sr_entry->e_attrs, ngi->ngi_memberOf );
			if ( a ) {
				if ( !( rs->sr_flags & REP_ENTRY_MODIFIABLE )) {
					Entry *e = entry_dup( rs->sr_entry );
					rs_replace_entry( op, rs, on, e );
					rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
					a = attr_find( e->e_attrs, ngi->ngi_memberOf );
				}
				nestgroup_memberOfVals( op, on, a );
			}
		}
		if (( nc->nc_needed & NG_NEGATED ) &&
			test_filter( op, rs->sr_entry, op->ors_filter ) != LDAP_COMPARE_TRUE )
			return 0;
	}
	return SLAP_CB_CONTINUE;
}

static int
nestgroup_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	nestgroup_info_t *ngi = on->on_bi.bi_private;
	int mbr_nf = 0, mof_nf = 0, negated = 0;
	nestgroup_filterinst_t *mbr_f = NULL, *mof_f = NULL;

	if ( get_manageDSAit( op ))
		return SLAP_CB_CONTINUE;

	/* groupBase must be explicitly configured */
	if ( !ngi->ngi_ngroupBase )
		return SLAP_CB_CONTINUE;

	/* handle attrs in filter */
	if ( ngi->ngi_flags & NG_MBR_FILTER ) {
		nestgroup_filter_instances( op, ngi->ngi_member, op->ors_filter, 0, &mbr_nf, &mbr_f, &negated );
		if ( mbr_nf ) {
			/* find member=(parent groups) */
			nestgroup_memberFilter( op, mbr_nf, mbr_f );
		}
	}
	if ( ngi->ngi_flags & NG_MOF_FILTER ) {
		nestgroup_filter_instances( op, ngi->ngi_memberOf, op->ors_filter, 0, &mof_nf, &mof_f, &negated );
		if ( mof_nf ) {
			/* find memberOf=(child groups) */
			nestgroup_memberOfFilter( op, mof_nf, mof_f );
		}
	}
	if ( mbr_nf ) {
		int i;
		for ( i=0; i<mbr_nf; i++ ) {
			if ( mbr_f[i].nf_new ) {
				mbr_f[i].nf_f->f_choice = LDAP_FILTER_OR;
				mbr_f[i].nf_f->f_list = mbr_f[i].nf_new;
			}
		}
		op->o_tmpfree( mbr_f, op->o_tmpmemctx );
	}
	if ( mof_nf ) {
		int i;
		for ( i=0; i<mof_nf; i++ ) {
			if ( mof_f[i].nf_new ) {
				mof_f[i].nf_f->f_choice = LDAP_FILTER_OR;
				mof_f[i].nf_f->f_list = mof_f[i].nf_new;
			}
		}
		op->o_tmpfree( mof_f, op->o_tmpmemctx );
	}

	if ( ngi->ngi_flags & ( NG_MBR_VALUES|NG_MOF_VALUES )) {
		/* check for attrs in attrlist */
		int attrflags = slap_attr_flags( op->ors_attrs );
		int needed = 0;
		if (( ngi->ngi_flags & NG_MBR_VALUES ) &&
			nestgroup_check_needed( op, attrflags, ngi->ngi_member )) {
			/* collect all members from child groups */
			needed |= NG_MBR_VALUES;
		}
		if (( ngi->ngi_flags & NG_MOF_VALUES ) &&
			nestgroup_check_needed( op, attrflags, ngi->ngi_memberOf )) {
			/* collect DNs of all parent groups */
			needed |= NG_MOF_VALUES;
		}
		if ( needed ) {
			nestgroup_cbinfo *nc;
			slap_callback *sc = op->o_tmpcalloc( 1, sizeof(slap_callback)+sizeof(nestgroup_cbinfo), op->o_tmpmemctx );
			nc = (nestgroup_cbinfo *)(sc+1);
			sc->sc_private = nc;
			nc->nc_needed = needed;
			nc->nc_on = on;
			sc->sc_response = nestgroup_searchresp;
			sc->sc_next = op->o_callback;
			op->o_callback = sc;
			if ( negated ) nc->nc_needed |= NG_NEGATED;
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
nestgroup_db_init(
	BackendDB *be,
	ConfigReply *cr)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	nestgroup_info_t *ngi;
	int rc;
	const char *text = NULL;

	ngi = (nestgroup_info_t *)ch_calloc( 1, sizeof( *ngi ));
	on->on_bi.bi_private = ngi;

	if ( !ad_memberOf ) {
		rc = slap_str2ad( SLAPD_MEMBEROF_ATTR, &ad_memberOf, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "nestgroup_db_init: "
					"unable to find attribute=\"%s\": %s (%d)\n",
					SLAPD_MEMBEROF_ATTR, text, rc );
			return rc;
		}
	}

	if ( !ad_member ) {
		rc = slap_str2ad( SLAPD_GROUP_ATTR, &ad_member, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "nestgroup_db_init: "
					"unable to find attribute=\"%s\": %s (%d)\n",
					SLAPD_GROUP_ATTR, text, rc );
			return rc;
		}
	}

	return 0;
}

static int
nestgroup_db_open(
	BackendDB *be,
	ConfigReply *cr)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	nestgroup_info_t *ngi = on->on_bi.bi_private;

	if ( !ngi->ngi_member )
		ngi->ngi_member = ad_member;

	if ( !ngi->ngi_memberOf )
		ngi->ngi_memberOf = ad_memberOf;

	return 0;
}

static int
nestgroup_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	nestgroup_info_t *ngi = on->on_bi.bi_private;

	ber_bvarray_free( ngi->ngi_groupBase );
	ber_bvarray_free( ngi->ngi_ngroupBase );
	ch_free( ngi );

	return 0;
}

static slap_overinst nestgroup;

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

int nestgroup_initialize() {
	int code;

	code = register_at(
	"( 1.2.840.113556.1.2.102 "
	"NAME 'memberOf' "
	"DESC 'Group that the entry belongs to' "
	"SYNTAX '1.3.6.1.4.1.1466.115.121.1.12' "
	"EQUALITY distinguishedNameMatch "	/* added */
	"USAGE dSAOperation "			/* added; questioned */
	"NO-USER-MODIFICATION " 		/* added */
	"X-ORIGIN 'iPlanet Delegated Administrator' )",
	&ad_memberOf, 1 );
	if ( code && code != SLAP_SCHERR_ATTR_DUP ) {
		Debug( LDAP_DEBUG_ANY,
			"nestgroup_initialize: register_at (memberOf) failed\n" );
		return code;
	}

	nestgroup.on_bi.bi_type = "nestgroup";
	nestgroup.on_bi.bi_db_init = nestgroup_db_init;
	nestgroup.on_bi.bi_db_open = nestgroup_db_open;
	nestgroup.on_bi.bi_db_destroy = nestgroup_db_destroy;

	nestgroup.on_bi.bi_op_search = nestgroup_op_search;
/*	nestgroup.on_bi.bi_op_compare = nestgroup_op_compare; */

	nestgroup.on_bi.bi_cf_ocs = ngroupocs;
	code = config_register_schema( ngroupcfg, ngroupocs );
	if ( code ) return code;

	return overlay_register( &nestgroup );
}

#if SLAPD_OVER_NESTGROUP == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return nestgroup_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_NESTGROUP) */
