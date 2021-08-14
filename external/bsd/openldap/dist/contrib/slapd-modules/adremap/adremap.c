/*	$NetBSD: adremap.c,v 1.2 2021/08/14 16:14:50 christos Exp $	*/

/* adremap.c - Case-folding and DN-value remapping for AD proxies */
/* $OpenLDAP$ */
/*
 * Copyright 2015 Howard Chu <hyc@symas.com>.
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
__RCSID("$NetBSD: adremap.c,v 1.2 2021/08/14 16:14:50 christos Exp $");

#include "portable.h"

/*
 * This file implements an overlay that performs two remapping functions
 * to allow older POSIX clients to use Microsoft AD:
 * 1: downcase the values of a configurable list of attributes
 * 2: dereference some DN-valued attributes and convert to their simple names
 *	   e.g. generate memberUid based on member
 */

#ifdef SLAPD_OVER_ADREMAP

#include <ldap.h>
#include "lutil.h"
#include "slap.h"
#include <ac/errno.h>
#include <ac/time.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include "slap-config.h"

typedef struct adremap_dnv {
	struct adremap_dnv *ad_next;
	AttributeDescription *ad_dnattr;	/* DN-valued attr to deref */
	AttributeDescription *ad_deref;		/* target attr's value to retrieve */
	AttributeDescription *ad_newattr;	/* New attr to collect new values */
	ObjectClass *ad_group;		/* group objectclass on target */
	ObjectClass *ad_mapgrp;		/* group objectclass to map */
	ObjectClass *ad_refgrp;		/* objectclass of target DN */
	struct berval ad_refbase;	/* base DN of target entries */
} adremap_dnv;
/* example: member uid memberUid */

typedef struct adremap_case {
	struct adremap_case *ac_next;
	AttributeDescription *ac_attr;
} adremap_case;

/* Per-instance configuration information */
typedef struct adremap_info {
	adremap_case *ai_case;	/* attrs to downcase */
	adremap_dnv *ai_dnv;	/* DN attrs to remap */
} adremap_info;

enum {
	ADREMAP_CASE = 1,
	ADREMAP_DNV
};

static ConfigDriver adremap_cf_case;
static ConfigDriver adremap_cf_dnv;

/* configuration attribute and objectclass */
static ConfigTable adremapcfg[] = {
	{ "adremap-downcase", "attrs", 2, 0, 0,
	  ARG_MAGIC|ADREMAP_CASE, adremap_cf_case,
	  "( OLcfgCtAt:6.1 "
	  "NAME 'olcADremapDowncase' "
	  "DESC 'List of attributes to casefold to lower case' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "adremap-dnmap", "dnattr targetattr newattr remoteOC localOC targetOC baseDN", 8, 8, 0,
	  ARG_MAGIC|ADREMAP_DNV, adremap_cf_dnv,
	  "( OLcfgCtAt:6.2 "
	  "NAME 'olcADremapDNmap' "
	  "DESC 'DN attr to map, attr from target to use, attr to generate, objectclass of remote"
	   " group, objectclass mapped group, objectclass of target entry, base DN of target entry' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs adremapocs[] = {
	{ "( OLcfgCtOc:6.1 "
	  "NAME 'olcADremapConfig' "
	  "DESC 'AD remap configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcADremapDowncase $ olcADremapDNmap ) )",
	  Cft_Overlay, adremapcfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static int
adremap_cf_case(ConfigArgs *c)
{
	BackendDB *be = (BackendDB *)c->be;
	slap_overinst *on = (slap_overinst *)c->bi;
	adremap_info *ai = on->on_bi.bi_private;
	adremap_case *ac, **a2;
	int rc = ARG_BAD_CONF;

	switch(c->op) {
	case SLAP_CONFIG_EMIT:
		for (ac = ai->ai_case; ac; ac=ac->ac_next) {
			rc = value_add_one(&c->rvalue_vals, &ac->ac_attr->ad_cname);
			if (rc) break;
		}
		break;
	case LDAP_MOD_DELETE:
		if (c->valx < 0) {
			for (ac = ai->ai_case; ac; ac=ai->ai_case) {
				ai->ai_case = ac->ac_next;
				ch_free(ac);
			}
		} else {
			int i;
			for (i=0, a2 = &ai->ai_case; i<c->valx; i++, a2 = &(*a2)->ac_next);
			ac = *a2;
			*a2 = ac->ac_next;
			ch_free(ac);
		}
		rc = 0;
		break;
	default: {
		const char *text;
		adremap_case ad;
		ad.ac_attr = NULL;
		rc = slap_str2ad(c->argv[1], &ad.ac_attr, &text);
		if (rc) break;
		for (a2 = &ai->ai_case; *a2; a2 = &(*a2)->ac_next);
		ac = ch_malloc(sizeof(adremap_case));
		ac->ac_next = NULL;
		ac->ac_attr = ad.ac_attr;
		*a2 = ac;
		break;
		}
	}
	return rc;
}

static int
adremap_cf_dnv(ConfigArgs *c)
{
	BackendDB *be = (BackendDB *)c->be;
	slap_overinst *on = (slap_overinst *)c->bi;
	adremap_info *ai = on->on_bi.bi_private;
	adremap_dnv *ad, **a2;
	int rc = ARG_BAD_CONF;

	switch(c->op) {
	case SLAP_CONFIG_EMIT:
		for (ad = ai->ai_dnv; ad; ad=ad->ad_next) {
			char *ptr;
			struct berval bv;
			bv.bv_len = ad->ad_dnattr->ad_cname.bv_len + ad->ad_deref->ad_cname.bv_len + ad->ad_newattr->ad_cname.bv_len + 2;
			bv.bv_len += ad->ad_group->soc_cname.bv_len + ad->ad_mapgrp->soc_cname.bv_len + ad->ad_refgrp->soc_cname.bv_len + 3;
			bv.bv_len += ad->ad_refbase.bv_len + 3;
			bv.bv_val = ch_malloc(bv.bv_len + 1);
			ptr = lutil_strcopy(bv.bv_val, ad->ad_dnattr->ad_cname.bv_val);
			*ptr++ = ' ';
			ptr = lutil_strcopy(ptr, ad->ad_deref->ad_cname.bv_val);
			*ptr++ = ' ';
			ptr = lutil_strcopy(ptr, ad->ad_newattr->ad_cname.bv_val);
			*ptr++ = ' ';
			ptr = lutil_strcopy(ptr, ad->ad_group->soc_cname.bv_val);
			*ptr++ = ' ';
			ptr = lutil_strcopy(ptr, ad->ad_mapgrp->soc_cname.bv_val);
			*ptr++ = ' ';
			ptr = lutil_strcopy(ptr, ad->ad_refgrp->soc_cname.bv_val);
			*ptr++ = ' ';
			*ptr++ = '"';
			ptr = lutil_strcopy(ptr, ad->ad_refbase.bv_val);
			*ptr++ = '"';
			*ptr = '\0';
			ber_bvarray_add(&c->rvalue_vals, &bv);
		}
		if (ai->ai_dnv) rc = 0;
		break;
	case LDAP_MOD_DELETE:
		if (c->valx < 0) {
			for (ad = ai->ai_dnv; ad; ad=ai->ai_dnv) {
				ai->ai_dnv = ad->ad_next;
				ch_free(ad);
			}
		} else {
			int i;
			for (i=0, a2 = &ai->ai_dnv; i<c->valx; i++, a2 = &(*a2)->ad_next);
			ad = *a2;
			*a2 = ad->ad_next;
			ch_free(ad);
		}
		rc = 0;
		break;
	default: {
		const char *text;
		adremap_dnv av = {0};
		struct berval dn;
		rc = slap_str2ad(c->argv[1], &av.ad_dnattr, &text);
		if (rc) break;
		if (av.ad_dnattr->ad_type->sat_syntax != slap_schema.si_syn_distinguishedName) {
			rc = 1;
			snprintf(c->cr_msg, sizeof(c->cr_msg), "<%s> not a DN-valued attribute",
				c->argv[0]);
			Debug(LDAP_DEBUG_ANY, "%s: %s(%s)\n", c->log, c->cr_msg, c->argv[1]);
			break;
		}
		rc = slap_str2ad(c->argv[2], &av.ad_deref, &text);
		if (rc) break;
		rc = slap_str2ad(c->argv[3], &av.ad_newattr, &text);
		if (rc) break;
		av.ad_group = oc_find(c->argv[4]);
		if (!av.ad_group) {
			rc = 1;
			break;
		}
		av.ad_mapgrp = oc_find(c->argv[5]);
		if (!av.ad_mapgrp) {
			rc = 1;
			break;
		}
		av.ad_refgrp = oc_find(c->argv[6]);
		if (!av.ad_refgrp) {
			rc = 1;
			break;
		}
		ber_str2bv(c->argv[7], 0, 0, &dn);
		rc = dnNormalize(0, NULL, NULL, &dn, &av.ad_refbase, NULL);
		if (rc) break;

		for (a2 = &ai->ai_dnv; *a2; a2 = &(*a2)->ad_next);
		ad = ch_malloc(sizeof(adremap_dnv));
		ad->ad_next = NULL;
		ad->ad_dnattr = av.ad_dnattr;
		ad->ad_deref = av.ad_deref;
		ad->ad_newattr = av.ad_newattr;
		ad->ad_group = av.ad_group;
		ad->ad_mapgrp = av.ad_mapgrp;
		ad->ad_refgrp = av.ad_refgrp;
		ad->ad_refbase = av.ad_refbase;
		*a2 = ad;
		break;
		}
	}
	return rc;
}

typedef struct adremap_ctx {
	slap_overinst *on;
	AttributeName an;
	AttributeDescription *ad;
	int an_swap;
} adremap_ctx;

static int
adremap_search_resp(
	Operation *op,
	SlapReply *rs
)
{
	adremap_ctx *ctx = op->o_callback->sc_private;
	slap_overinst *on = ctx->on;
	adremap_info *ai = on->on_bi.bi_private;
	adremap_case *ac;
	adremap_dnv *ad;
	Attribute *a;
	Entry *e;

	if (rs->sr_type != REP_SEARCH)
		return SLAP_CB_CONTINUE;

	/* we munged the attr list, restore it to original */
	if (ctx->an_swap) {
		int i;
		ctx->an_swap = 0;
		for (i=0; rs->sr_attrs[i].an_name.bv_val; i++) {
			if (rs->sr_attrs[i].an_desc == ctx->ad) {
				rs->sr_attrs[i] = ctx->an;
				break;
			}
		}
		/* Usually rs->sr_attrs is just op->ors_attrs, but
		 * overlays like rwm may make a new copy. Fix both
		 * if needed.
		 */
		if (op->ors_attrs != rs->sr_attrs) {
			for (i=0; op->ors_attrs[i].an_name.bv_val; i++) {
				if (op->ors_attrs[i].an_desc == ctx->ad) {
					op->ors_attrs[i] = ctx->an;
					break;
				}
			}
		}
	}
	e = rs->sr_entry;
	for (ac = ai->ai_case; ac; ac = ac->ac_next) {
		a = attr_find(e->e_attrs, ac->ac_attr);
		if (a) {
			int i, j;
			if (!(rs->sr_flags & REP_ENTRY_MODIFIABLE)) {
				e = entry_dup(e);
				rs_replace_entry(op, rs, on, e);
				rs->sr_flags |= REP_ENTRY_MODIFIABLE|REP_ENTRY_MUSTBEFREED;
				a = attr_find(e->e_attrs, ac->ac_attr);
			}
			for (i=0; i<a->a_numvals; i++) {
				unsigned char *c = a->a_vals[i].bv_val;
				for (j=0; j<a->a_vals[i].bv_len; j++)
					if (isupper(c[j]))
						c[j] = tolower(c[j]);
			}
		}
	}
	for (ad = ai->ai_dnv; ad; ad = ad->ad_next) {
		a = attr_find(e->e_attrs, ad->ad_dnattr);
		if (a) {
			Entry *n;
			Attribute *dr;
			int i, rc;
			if (!(rs->sr_flags & REP_ENTRY_MODIFIABLE)) {
				e = entry_dup(e);
				rs_replace_entry(op, rs, on, e);
				rs->sr_flags |= REP_ENTRY_MODIFIABLE|REP_ENTRY_MUSTBEFREED;
				a = attr_find(e->e_attrs, ad->ad_dnattr);
			}
			for (i=0; i<a->a_numvals; i++) {
				struct berval dv;
				dv = ad->ad_deref->ad_cname;
					/* If the RDN uses the deref attr, just use it directly */
				if (a->a_nvals[i].bv_val[dv.bv_len] == '=' &&
					!memcmp(a->a_nvals[i].bv_val, dv.bv_val, dv.bv_len)) {
					struct berval bv, nv;
					char *ptr;
					bv = a->a_vals[i];
					nv = a->a_nvals[i];
					bv.bv_val += dv.bv_len + 1;
					ptr = strchr(bv.bv_val, ',');
					if (ptr)
						bv.bv_len = ptr - bv.bv_val;
					else
						bv.bv_len -= dv.bv_len+1;
					nv.bv_val += dv.bv_len + 1;
					ptr = strchr(nv.bv_val, ',');
					if (ptr)
						nv.bv_len = ptr - nv.bv_val;
					else
						nv.bv_len -= dv.bv_len+1;
					attr_merge_one(e, ad->ad_newattr, &bv, &nv);
				} else {
					/* otherwise look up the deref attr */
					n = NULL;
					rc = be_entry_get_rw(op, &a->a_nvals[i], NULL, ad->ad_deref, 0, &n);
					if (!rc && n) {
						dr = attr_find(n->e_attrs, ad->ad_deref);
						if (dr)
							attr_merge_one(e, ad->ad_newattr, dr->a_vals, dr->a_nvals);
						be_entry_release_r(op, n);
					}
				}
			}
		}
	}
	return SLAP_CB_CONTINUE;
}

static int adremap_refsearch(
	Operation *op,
	SlapReply *rs
)
{
	if (rs->sr_type == REP_SEARCH) {
		slap_callback *sc = op->o_callback;
		struct berval *dn = sc->sc_private;
		ber_dupbv_x(dn, &rs->sr_entry->e_nname, op->o_tmpmemctx);
		return LDAP_SUCCESS;
	}
	return rs->sr_err;
}

static adremap_dnv *adremap_filter(
	Operation *op,
	adremap_info *ai
)
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	Filter *f = op->ors_filter, *fn = NULL;
	adremap_dnv *ad = NULL;
	struct berval bv;
	int fextra = 0;

	/* Do we need to munge the filter? First see if it's of
	 * the form (objectClass=<mapgrp>)
	 * or form (&(objectClass=<mapgrp>)...)
	 * or form (&(&(objectClass=<mapgrp>)...)...)
	 */
	if (f->f_choice == LDAP_FILTER_AND && f->f_and) {
		fextra = 1;
		f = f->f_and;
		fn = f->f_next;
	}
	if (f->f_choice == LDAP_FILTER_AND && f->f_and) {
		fextra = 2;
		f = f->f_and;
	}
	if (f->f_choice == LDAP_FILTER_EQUALITY &&
		f->f_av_desc == slap_schema.si_ad_objectClass) {
		struct berval bv = f->f_av_value;

		for (ad = ai->ai_dnv; ad; ad = ad->ad_next) {
			if (!ber_bvstrcasecmp( &bv, &ad->ad_mapgrp->soc_cname )) {
			/* Now check to see if next element is (<newattr>=foo) */
				Filter *fnew;
				if (fn && fn->f_choice == LDAP_FILTER_EQUALITY &&
					fn->f_av_desc == ad->ad_newattr) {
					Filter fr[3];
					AttributeAssertion aa[2] = {0};
					Operation op2;
					slap_callback cb = {0};
					SlapReply rs = {REP_RESULT};
					struct berval dn = BER_BVNULL;

					/* It's a match, setup a search with filter
					 * (&(objectclass=<refgrp>)(<deref>=foo))
					 */
					fr[0].f_choice = LDAP_FILTER_AND;
					fr[0].f_and = &fr[1];
					fr[0].f_next = NULL;

					fr[1].f_choice = LDAP_FILTER_EQUALITY;
					fr[1].f_ava = &aa[0];
					fr[1].f_av_desc = slap_schema.si_ad_objectClass;
					fr[1].f_av_value = ad->ad_refgrp->soc_cname;
					fr[1].f_next = &fr[2];

					fr[2].f_choice = LDAP_FILTER_EQUALITY;
					fr[2].f_ava = &aa[1];
					fr[2].f_av_desc = ad->ad_deref;
					fr[2].f_av_value = fn->f_av_value;
					fr[2].f_next = NULL;

					/* Search with this filter to retrieve target DN */
					op2 = *op;
					op2.o_callback = &cb;
					cb.sc_response = adremap_refsearch;
					cb.sc_private = &dn;
					op2.o_req_dn = ad->ad_refbase;
					op2.o_req_ndn = ad->ad_refbase;
					op2.ors_filter = fr;
					filter2bv_x(op, fr, &op2.ors_filterstr);
					op2.ors_deref = LDAP_DEREF_NEVER;
					op2.ors_slimit = 1;
					op2.ors_tlimit = SLAP_NO_LIMIT;
					op2.ors_attrs = slap_anlist_no_attrs;
					op2.ors_attrsonly = 1;
					op2.o_no_schema_check = 1;
					op2.o_bd->bd_info = (BackendInfo *)on->on_info;
					op2.o_bd->be_search(&op2, &rs);
					op2.o_bd->bd_info = (BackendInfo *)on;
					op->o_tmpfree(op2.ors_filterstr.bv_val, op->o_tmpmemctx);

					if (!dn.bv_len) {	/* no match was found */
						ad = NULL;
						break;
					}

					if (rs.sr_err) {	/* sizelimit exceeded, etc.: invalid name */
						op->o_tmpfree(dn.bv_val, op->o_tmpmemctx);
						ad = NULL;
						break;
					}

					/* Build a new filter of form
					 * (&(objectclass=<group>)(<dnattr>=foo-DN)...)
					 */
					f = op->o_tmpalloc(sizeof(Filter), op->o_tmpmemctx);
					f->f_choice = LDAP_FILTER_AND;
					fnew = f;
					f->f_next = NULL;

					f->f_and = op->o_tmpalloc(sizeof(Filter), op->o_tmpmemctx);
					f = f->f_and;
					f->f_choice = LDAP_FILTER_EQUALITY;
					f->f_ava = op->o_tmpcalloc(1, sizeof(AttributeAssertion), op->o_tmpmemctx);
					f->f_av_desc = slap_schema.si_ad_objectClass;
					ber_dupbv_x(&f->f_av_value, &ad->ad_group->soc_cname, op->o_tmpmemctx);

					f->f_next = op->o_tmpalloc(sizeof(Filter), op->o_tmpmemctx);
					f = f->f_next;
					f->f_choice = LDAP_FILTER_EQUALITY;
					f->f_ava = op->o_tmpcalloc(1, sizeof(AttributeAssertion), op->o_tmpmemctx);
					f->f_av_desc = ad->ad_dnattr;
					f->f_av_value = dn;

					f->f_next = fn->f_next;
					fn->f_next = NULL;
				} else {
					/* Build a new filter of form
					 * (objectclass=<group>)
					 */
					f->f_next = NULL;	/* disconnect old chain */

					f = op->o_tmpalloc(sizeof(Filter), op->o_tmpmemctx);
					f->f_choice = LDAP_FILTER_EQUALITY;
					f->f_ava = op->o_tmpcalloc(1, sizeof(AttributeAssertion), op->o_tmpmemctx);
					f->f_av_desc = slap_schema.si_ad_objectClass;
					ber_dupbv_x(&f->f_av_value, &ad->ad_group->soc_cname, op->o_tmpmemctx);

					/* If there was a wrapping (&), attach it. */
					if (fextra) {
						fnew = op->o_tmpalloc(sizeof(Filter), op->o_tmpmemctx);
						fnew->f_choice = LDAP_FILTER_AND;
						fnew->f_and = f;
						fnew->f_next = NULL;
						f->f_next = fn;
					} else {
						fnew = f;
						f->f_next = NULL;
					}
				}
				if (fextra > 1) {
					f = op->o_tmpalloc(sizeof(Filter), op->o_tmpmemctx);
					f->f_choice = LDAP_FILTER_AND;
					f->f_and = fnew->f_and;
					f->f_next = f->f_and->f_next;
					f->f_and->f_next = op->ors_filter->f_and->f_and->f_next;
					op->ors_filter->f_and->f_and->f_next = NULL;
					fnew->f_and = f;
				}
				filter_free_x(op, op->ors_filter, 1);
				op->o_tmpfree(op->ors_filterstr.bv_val, op->o_tmpmemctx);
				op->ors_filter = fnew;
				filter2bv_x(op, op->ors_filter, &op->ors_filterstr);
				break;
			}
		}
	}
	return ad;
}

static int
adremap_search(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	adremap_info *ai = (adremap_info *) on->on_bi.bi_private;
	adremap_ctx *ctx;
	adremap_dnv *ad = NULL;
	slap_callback *cb;

	/* Is this our own internal search? Ignore it */
	if (op->o_no_schema_check)
		return SLAP_CB_CONTINUE;

	if (ai->ai_dnv)
		/* check for filter match, fallthru if none */
		ad = adremap_filter(op, ai);

	cb = op->o_tmpcalloc(1, sizeof(slap_callback)+sizeof(adremap_ctx), op->o_tmpmemctx);
	cb->sc_response = adremap_search_resp;
	cb->sc_private = cb+1;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;
	ctx = cb->sc_private;
	ctx->on = on;
	if (ad && op->ors_attrs) {	/* see if we need to remap a search attr */
		int i;
		for (i=0; op->ors_attrs[i].an_name.bv_val; i++) {
			if (op->ors_attrs[i].an_desc == ad->ad_newattr) {
				ctx->an_swap = 1;
				ctx->ad = ad->ad_dnattr;
				ctx->an = op->ors_attrs[i];
				op->ors_attrs[i].an_desc = ad->ad_dnattr;
				op->ors_attrs[i].an_name = ad->ad_dnattr->ad_cname;
				break;
			}
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
adremap_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;

	/* initialize private structure to store configuration */
	on->on_bi.bi_private = ch_calloc( 1, sizeof(adremap_info) );

	return 0;
}

static int
adremap_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	adremap_info *ai = (adremap_info *) on->on_bi.bi_private;
	adremap_case *ac;
	adremap_dnv *ad;

	/* free config */
	for (ac = ai->ai_case; ac; ac = ai->ai_case) {
		ai->ai_case = ac->ac_next;
		ch_free(ac);
	}
	for (ad = ai->ai_dnv; ad; ad = ai->ai_dnv) {
		ai->ai_dnv = ad->ad_next;
		ch_free(ad);
	}
	free( ai );

	return 0;
}

static slap_overinst adremap;

int adremap_initialize()
{
	int i, code;

	adremap.on_bi.bi_type = "adremap";
	adremap.on_bi.bi_flags = SLAPO_BFLAG_SINGLE;
	adremap.on_bi.bi_db_init = adremap_db_init;
	adremap.on_bi.bi_db_destroy = adremap_db_destroy;
	adremap.on_bi.bi_op_search = adremap_search;

	/* register configuration directives */
	adremap.on_bi.bi_cf_ocs = adremapocs;
	code = config_register_schema( adremapcfg, adremapocs );
	if ( code ) return code;

	return overlay_register( &adremap );
}

#if SLAPD_OVER_ADREMAP == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return adremap_initialize();
}
#endif

#endif	/* defined(SLAPD_OVER_ADREMAP) */
