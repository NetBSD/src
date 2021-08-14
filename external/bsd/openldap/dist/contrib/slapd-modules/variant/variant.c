/*	$NetBSD: variant.c,v 1.2 2021/08/14 16:14:54 christos Exp $	*/

/* variant.c - variant overlay */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2021 Symas Corporation.
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
 * This work was developed in 2016-2017 by Ondřej Kuzník for Symas Corp.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: variant.c,v 1.2 2021/08/14 16:14:54 christos Exp $");

#include "portable.h"

#ifdef SLAPD_OVER_VARIANT

#include "slap.h"
#include "slap-config.h"
#include "ldap_queue.h"

typedef enum variant_type_t {
	VARIANT_INFO_PLAIN = 1 << 0,
	VARIANT_INFO_REGEX = 1 << 1,

	VARIANT_INFO_ALL = ~0
} variant_type_t;

typedef struct variant_info_t {
	int passReplication;
	LDAP_STAILQ_HEAD(variant_list, variantEntry_info) variants, regex_variants;
} variant_info_t;

typedef struct variantEntry_info {
	variant_info_t *ov;
	struct berval dn;
	variant_type_t type;
	regex_t *regex;
	LDAP_SLIST_HEAD(attribute_list, variantAttr_info) attributes;
	LDAP_STAILQ_ENTRY(variantEntry_info) next;
} variantEntry_info;

typedef struct variantAttr_info {
	variantEntry_info *variant;
	struct berval dn;
	AttributeDescription *attr, *alternative;
	LDAP_SLIST_ENTRY(variantAttr_info) next;
} variantAttr_info;

static int
variant_build_dn(
		Operation *op,
		variantAttr_info *vai,
		int nmatch,
		regmatch_t *pmatch,
		struct berval *out )
{
	struct berval dn, *ndn = &op->o_req_ndn;
	char *dest, *p, *prev, *end = vai->dn.bv_val + vai->dn.bv_len;
	size_t len = vai->dn.bv_len;
	int rc;

	p = vai->dn.bv_val;
	while ( (p = memchr( p, '$', end - p )) != NULL ) {
		len -= 1;
		p += 1;

		if ( ( *p >= '0' ) && ( *p <= '9' ) ) {
			int i = *p - '0';

			len += ( pmatch[i].rm_eo - pmatch[i].rm_so );
		} else if ( *p != '$' ) {
			/* Should have been checked at configuration time */
			assert(0);
		}
		len -= 1;
		p += 1;
	}

	dest = dn.bv_val = ch_realloc( out->bv_val, len + 1 );
	dn.bv_len = len;

	prev = vai->dn.bv_val;
	while ( (p = memchr( prev, '$', end - prev )) != NULL ) {
		len = p - prev;
		AC_MEMCPY( dest, prev, len );
		dest += len;
		p += 1;

		if ( ( *p >= '0' ) && ( *p <= '9' ) ) {
			int i = *p - '0';
			len = pmatch[i].rm_eo - pmatch[i].rm_so;

			AC_MEMCPY( dest, ndn->bv_val + pmatch[i].rm_so, len );
			dest += len;
		} else if ( *p == '$' ) {
			*dest++ = *p;
		}
		prev = p + 1;
	}
	len = end - prev;
	AC_MEMCPY( dest, prev, len );
	dest += len;
	*dest = '\0';

	rc = dnNormalize( 0, NULL, NULL, &dn, out, NULL );
	ch_free( dn.bv_val );

	return rc;
}

static int
variant_build_entry(
		Operation *op,
		variantEntry_info *vei,
		struct berval *dn,
		Entry **ep,
		int nmatch,
		regmatch_t *pmatch )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	BackendDB *be_orig = op->o_bd, *db;
	struct berval ndn = BER_BVNULL;
	variantAttr_info *vai;
	Attribute *a;
	BerVarray nvals;
	Entry *e;
	unsigned int i;
	int rc;

	assert( ep );
	assert( !*ep );

	rc = overlay_entry_get_ov( op, dn, NULL, NULL, 0, &e, on );
	if ( rc == LDAP_SUCCESS && is_entry_referral( e ) ) {
		overlay_entry_release_ov( op, e, 0, on );
		rc = LDAP_REFERRAL;
	}

	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	*ep = entry_dup( e );
	overlay_entry_release_ov( op, e, 0, on );

	LDAP_SLIST_FOREACH( vai, &vei->attributes, next ) {
		if ( vei->type == VARIANT_INFO_REGEX ) {
			rc = variant_build_dn( op, vai, nmatch, pmatch, &ndn );
			if ( rc != LDAP_SUCCESS ) {
				goto done;
			}
		} else {
			ndn = vai->dn;
		}

		(void)attr_delete( &(*ep)->e_attrs, vai->attr );
		op->o_bd = be_orig;

		/* only select backend if not served by ours, would retrace all
		 * overlays again */
		db = select_backend( &ndn, 0 );
		if ( db && db != be_orig->bd_self ) {
			op->o_bd = db;
			rc = be_entry_get_rw( op, &ndn, NULL, vai->alternative, 0, &e );
		} else {
			rc = overlay_entry_get_ov(
					op, &ndn, NULL, vai->alternative, 0, &e, on );
		}

		switch ( rc ) {
			case LDAP_SUCCESS:
				break;
			case LDAP_INSUFFICIENT_ACCESS:
			case LDAP_NO_SUCH_ATTRIBUTE:
			case LDAP_NO_SUCH_OBJECT:
				rc = LDAP_SUCCESS;
				continue;
				break;
			default:
				goto done;
				break;
		}

		a = attr_find( e->e_attrs, vai->alternative );

		/* back-ldif doesn't check the attribute exists in the entry before
		 * returning it */
		if ( a ) {
			if ( a->a_nvals ) {
				nvals = a->a_nvals;
			} else {
				nvals = a->a_vals;
			}

			for ( i = 0; i < a->a_numvals; i++ ) {
				if ( backend_access( op, e, &ndn, vai->alternative, &nvals[i],
							ACL_READ, NULL ) != LDAP_SUCCESS ) {
					continue;
				}

				rc = attr_merge_one( *ep, vai->attr, &a->a_vals[i], &nvals[i] );
				if ( rc != LDAP_SUCCESS ) {
					break;
				}
			}
		}

		if ( db && db != be_orig->bd_self ) {
			be_entry_release_rw( op, e, 0 );
		} else {
			overlay_entry_release_ov( op, e, 0, on );
		}
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
	}

done:
	op->o_bd = be_orig;
	if ( rc != LDAP_SUCCESS && *ep ) {
		entry_free( *ep );
		*ep = NULL;
	}
	if ( vei->type == VARIANT_INFO_REGEX ) {
		ch_free( ndn.bv_val );
	}

	return rc;
}

static int
variant_find_config(
		Operation *op,
		variant_info_t *ov,
		struct berval *ndn,
		int which,
		variantEntry_info **veip,
		size_t nmatch,
		regmatch_t *pmatch )
{
	variantEntry_info *vei;

	assert( veip );

	if ( which & VARIANT_INFO_PLAIN ) {
		int diff;

		LDAP_STAILQ_FOREACH( vei, &ov->variants, next ) {
			dnMatch( &diff, 0, NULL, NULL, ndn, &vei->dn );
			if ( diff ) continue;

			*veip = vei;
			return LDAP_SUCCESS;
		}
	}

	if ( which & VARIANT_INFO_REGEX ) {
		LDAP_STAILQ_FOREACH( vei, &ov->regex_variants, next ) {
			if ( regexec( vei->regex, ndn->bv_val, nmatch, pmatch, 0 ) ) {
				continue;
			}

			*veip = vei;
			return LDAP_SUCCESS;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
variant_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei;
	int rc;

	/* Replication always uses the rootdn */
	if ( ov->passReplication && SLAPD_SYNC_IS_SYNCCONN(op->o_connid) &&
			be_isroot( op ) ) {
		return SLAP_CB_CONTINUE;
	}

	Debug( LDAP_DEBUG_TRACE, "variant_op_add: "
			"dn=%s\n", op->o_req_ndn.bv_val );

	rc = variant_find_config(
			op, ov, &op->o_req_ndn, VARIANT_INFO_ALL, &vei, 0, NULL );
	if ( rc == LDAP_SUCCESS ) {
		variantAttr_info *vai;

		LDAP_SLIST_FOREACH( vai, &vei->attributes, next ) {
			Attribute *a;
			for ( a = op->ora_e->e_attrs; a; a = a->a_next ) {
				if ( a->a_desc == vai->attr ) {
					rc = LDAP_CONSTRAINT_VIOLATION;
					send_ldap_error( op, rs, rc,
							"variant: trying to add variant attributes" );
					goto done;
				}
			}
		}
	}
	rc = SLAP_CB_CONTINUE;

done:
	Debug( LDAP_DEBUG_TRACE, "variant_op_add: "
			"finished with %d\n",
			rc );
	return rc;
}

static int
variant_op_compare( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei;
	regmatch_t pmatch[10];
	int rc, nmatch = sizeof(pmatch) / sizeof(regmatch_t);

	Debug( LDAP_DEBUG_TRACE, "variant_op_compare: "
			"dn=%s\n", op->o_req_ndn.bv_val );

	rc = variant_find_config(
			op, ov, &op->o_req_ndn, VARIANT_INFO_ALL, &vei, nmatch, pmatch );
	if ( rc == LDAP_SUCCESS ) {
		Entry *e = NULL;

		rc = variant_build_entry( op, vei, &op->o_req_ndn, &e, nmatch, pmatch );
		/* in case of error, just let the backend deal with the mod and the
		 * client should get a meaningful error back */
		if ( rc != LDAP_SUCCESS ) {
			rc = SLAP_CB_CONTINUE;
		} else {
			rc = slap_compare_entry( op, e, op->orc_ava );

			entry_free( e );
			e = NULL;
		}
	}

	if ( rc != SLAP_CB_CONTINUE ) {
		rs->sr_err = rc;
		send_ldap_result( op, rs );
	}

	Debug( LDAP_DEBUG_TRACE, "variant_op_compare: "
			"finished with %d\n", rc );
	return rc;
}

static int
variant_cmp_op( const void *l, const void *r )
{
	const Operation *left = l, *right = r;
	int diff;

	dnMatch( &diff, 0, NULL, NULL, (struct berval *)&left->o_req_ndn,
			(void *)&right->o_req_ndn );

	return diff;
}

static int
variant_run_mod( void *nop, void *arg )
{
	SlapReply nrs = { REP_RESULT };
	slap_callback cb = { 0 };
	Operation *op = nop;
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	int *rc = arg;

	cb.sc_response = slap_null_cb;
	op->o_callback = &cb;

	Debug( LDAP_DEBUG_TRACE, "variant_run_mod: "
			"running mod on dn=%s\n",
			op->o_req_ndn.bv_val );
	*rc = on->on_info->oi_orig->bi_op_modify( op, &nrs );
	Debug( LDAP_DEBUG_TRACE, "variant_run_mod: "
			"finished with %d\n", *rc );

	return ( *rc != LDAP_SUCCESS );
}

/** Move the Modifications back to the original Op so that they can be disposed
 * of by the original creator
 */
static int
variant_reassign_mods( void *nop, void *arg )
{
	Operation *op = nop, *orig_op = arg;
	Modifications *mod;

	assert( op->orm_modlist );

	for ( mod = op->orm_modlist; mod->sml_next; mod = mod->sml_next )
		/* get the tail mod */;

	mod->sml_next = orig_op->orm_modlist;
	orig_op->orm_modlist = op->orm_modlist;

	return LDAP_SUCCESS;
}

void
variant_free_op( void *op )
{
	ch_free( ((Operation *)op)->o_req_ndn.bv_val );
	ch_free( op );
}

static int
variant_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei;
	variantAttr_info *vai;
	Avlnode *ops = NULL;
	Entry *e = NULL;
	Modifications *mod, *nextmod;
	regmatch_t pmatch[10];
	int rc, nmatch = sizeof(pmatch) / sizeof(regmatch_t);

	/* Replication always uses the rootdn */
	if ( ov->passReplication && SLAPD_SYNC_IS_SYNCCONN(op->o_connid) &&
			be_isroot( op ) ) {
		return SLAP_CB_CONTINUE;
	}

	Debug( LDAP_DEBUG_TRACE, "variant_op_mod: "
			"dn=%s\n", op->o_req_ndn.bv_val );

	rc = variant_find_config(
			op, ov, &op->o_req_ndn, VARIANT_INFO_ALL, &vei, nmatch, pmatch );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "variant_op_mod: "
				"not a variant\n" );
		rc = SLAP_CB_CONTINUE;
		goto done;
	}

	rc = variant_build_entry( op, vei, &op->o_req_ndn, &e, nmatch, pmatch );
	/* in case of error, just let the backend deal with the mod and the client
	 * should get a meaningful error back */
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "variant_op_mod: "
				"failed to retrieve entry\n" );
		rc = SLAP_CB_CONTINUE;
		goto done;
	}

	rc = acl_check_modlist( op, e, op->orm_modlist );
	entry_free( e );

	if ( !rc ) {
		rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		send_ldap_error( op, rs, rc, "" );
		return rc;
	}

	for ( mod = op->orm_modlist; mod; mod = nextmod ) {
		Operation needle = { .o_req_ndn = BER_BVNULL }, *nop;

		nextmod = mod->sml_next;

		LDAP_SLIST_FOREACH( vai, &vei->attributes, next ) {
			if ( vai->attr == mod->sml_desc ) {
				break;
			}
		}

		if ( vai ) {
			if ( vei->type == VARIANT_INFO_REGEX ) {
				rc = variant_build_dn(
						op, vai, nmatch, pmatch, &needle.o_req_ndn );
				if ( rc != LDAP_SUCCESS ) {
					continue;
				}
			} else {
				needle.o_req_ndn = vai->dn;
			}

			nop = ldap_avl_find( ops, &needle, variant_cmp_op );
			if ( nop == NULL ) {
				nop = ch_calloc( 1, sizeof(Operation) );
				*nop = *op;

				ber_dupbv( &nop->o_req_ndn, &needle.o_req_ndn );
				nop->o_req_dn = nop->o_req_ndn;
				nop->orm_modlist = NULL;

				rc = ldap_avl_insert( &ops, nop, variant_cmp_op, ldap_avl_dup_error );
				assert( rc == 0 );
			}
			mod->sml_desc = vai->alternative;

			op->orm_modlist = nextmod;
			mod->sml_next = nop->orm_modlist;
			nop->orm_modlist = mod;

			if ( vei->type == VARIANT_INFO_REGEX ) {
				ch_free( needle.o_req_ndn.bv_val );
			}
		}
	}

	if ( !ops ) {
		Debug( LDAP_DEBUG_TRACE, "variant_op_mod: "
				"no variant attributes in mod\n" );
		return SLAP_CB_CONTINUE;
	}

	/*
	 * First run original Operation
	 * This will take care of making sure the entry exists as well.
	 *
	 * FIXME?
	 * Since we cannot make the subsequent Ops atomic wrt. this one, we just
	 * let it send the response as well. After all, the changes on the main DN
	 * have finished by then
	 */
	rc = on->on_info->oi_orig->bi_op_modify( op, rs );
	if ( rc == LDAP_SUCCESS ) {
		/* FIXME: if a mod fails, should we attempt to apply the rest? */
		ldap_avl_apply( ops, variant_run_mod, &rc, -1, AVL_INORDER );
	}

	ldap_avl_apply( ops, variant_reassign_mods, op, -1, AVL_INORDER );
	ldap_avl_free( ops, variant_free_op );

done:
	Debug( LDAP_DEBUG_TRACE, "variant_op_mod: "
			"finished with %d\n", rc );
	return rc;
}

static int
variant_search_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = op->o_callback->sc_private;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei;
	int rc;

	if ( rs->sr_type == REP_RESULT ) {
		ch_free( op->o_callback );
		op->o_callback = NULL;
	}

	if ( rs->sr_type != REP_SEARCH ) {
		return SLAP_CB_CONTINUE;
	}

	rc = variant_find_config(
			op, ov, &rs->sr_entry->e_nname, VARIANT_INFO_PLAIN, &vei, 0, NULL );
	if ( rc == LDAP_SUCCESS ) {
		rs->sr_nentries--;
		return rc;
	}

	return SLAP_CB_CONTINUE;
}

static int
variant_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei;
	slap_callback *cb;
	Entry *e = NULL;
	regmatch_t pmatch[10];
	int variantInScope = 0, rc = SLAP_CB_CONTINUE,
		nmatch = sizeof(pmatch) / sizeof(regmatch_t);

	if ( ov->passReplication && ( op->o_sync > SLAP_CONTROL_IGNORED ) ) {
		return SLAP_CB_CONTINUE;
	}

	Debug( LDAP_DEBUG_TRACE, "variant_op_search: "
			"dn=%s, scope=%d\n",
			op->o_req_ndn.bv_val, op->ors_scope );

	LDAP_STAILQ_FOREACH( vei, &ov->variants, next ) {
		if ( !dnIsSuffixScope( &vei->dn, &op->o_req_ndn, op->ors_scope ) )
			continue;

		variantInScope = 1;

		rc = variant_build_entry( op, vei, &vei->dn, &e, 0, NULL );
		if ( rc == LDAP_NO_SUCH_OBJECT || rc == LDAP_REFERRAL ) {
			rc = SLAP_CB_CONTINUE;
			continue;
		} else if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "variant_op_search: "
					"failed to retrieve entry: dn=%s\n",
					vei->dn.bv_val );
			goto done;
		}

		if ( test_filter( op, e, op->ors_filter ) == LDAP_COMPARE_TRUE ) {
			Debug( LDAP_DEBUG_TRACE, "variant_op_search: "
					"entry matched: dn=%s\n",
					vei->dn.bv_val );
			rs->sr_entry = e;
			rs->sr_attrs = op->ors_attrs;
			rc = send_search_entry( op, rs );
		}
		entry_free( e );
		e = NULL;
	}

	/* Three options:
	 * - the entry has been handled above, in that case vei->type is VARIANT_INFO_PLAIN
	 * - the entry matches a regex, use the first one and we're finished
	 * - no configuration matches entry - do nothing
	 */
	if ( op->ors_scope == LDAP_SCOPE_BASE &&
			variant_find_config( op, ov, &op->o_req_ndn, VARIANT_INFO_ALL, &vei,
					nmatch, pmatch ) == LDAP_SUCCESS &&
			vei->type == VARIANT_INFO_REGEX ) {
		rc = variant_build_entry( op, vei, &op->o_req_ndn, &e, nmatch, pmatch );
		if ( rc == LDAP_NO_SUCH_OBJECT || rc == LDAP_REFERRAL ) {
			rc = SLAP_CB_CONTINUE;
		} else if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "variant_op_search: "
					"failed to retrieve entry: dn=%s\n",
					vei->dn.bv_val );
			goto done;
		} else {
			if ( test_filter( op, e, op->ors_filter ) == LDAP_COMPARE_TRUE ) {
				Debug( LDAP_DEBUG_TRACE, "variant_op_search: "
						"entry matched: dn=%s\n",
						vei->dn.bv_val );
				rs->sr_entry = e;
				rs->sr_attrs = op->ors_attrs;
				rc = send_search_entry( op, rs );
			}
			entry_free( e );
			e = NULL;
			goto done;
		}
	}
	rc = SLAP_CB_CONTINUE;

	if ( variantInScope ) {
		cb = ch_calloc( 1, sizeof(slap_callback) );
		cb->sc_private = on;
		cb->sc_response = variant_search_response;
		cb->sc_next = op->o_callback;

		op->o_callback = cb;
	}

done:
	if ( rc != SLAP_CB_CONTINUE ) {
		rs->sr_err = (rc == LDAP_SUCCESS) ? rc : LDAP_OTHER;
		send_ldap_result( op, rs );
	}
	Debug( LDAP_DEBUG_TRACE, "variant_op_search: "
			"finished with %d\n", rc );
	return rc;
}

/* Configuration */

static ConfigLDAPadd variant_ldadd;
static ConfigLDAPadd variant_regex_ldadd;
static ConfigLDAPadd variant_attr_ldadd;

static ConfigDriver variant_set_dn;
static ConfigDriver variant_set_regex;
static ConfigDriver variant_set_alt_dn;
static ConfigDriver variant_set_alt_pattern;
static ConfigDriver variant_set_attribute;
static ConfigDriver variant_add_alt_attr;
static ConfigDriver variant_add_alt_attr_regex;

static ConfigCfAdd variant_cfadd;

enum
{
	VARIANT_ATTR = 1,
	VARIANT_ATTR_ALT,

	VARIANT_LAST,
};

static ConfigTable variant_cfg[] = {
	{ "passReplication", "on|off", 2, 2, 0,
		ARG_ON_OFF|ARG_OFFSET,
		(void *)offsetof( variant_info_t, passReplication ),
		"( OLcfgOvAt:FIXME.1 NAME 'olcVariantPassReplication' "
			"DESC 'Whether to let searches with replication control "
				"pass unmodified' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	{ "variantDN", "dn", 2, 2, 0,
		ARG_DN|ARG_QUOTE|ARG_MAGIC,
		variant_set_dn,
		"( OLcfgOvAt:FIXME.2 NAME 'olcVariantEntry' "
			"DESC 'DN of the variant entry' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX OMsDN "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	{ "variantRegex", "regex", 2, 2, 0,
		ARG_BERVAL|ARG_QUOTE|ARG_MAGIC,
		variant_set_regex,
		"( OLcfgOvAt:FIXME.6 NAME 'olcVariantEntryRegex' "
			"DESC 'Pattern for the variant entry' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	/* These have no equivalent in slapd.conf */
	{ "", NULL, 2, 2, 0,
		ARG_STRING|ARG_MAGIC|VARIANT_ATTR,
		variant_set_attribute,
		"( OLcfgOvAt:FIXME.3 NAME 'olcVariantVariantAttribute' "
			"DESC 'Attribute to fill in the entry' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	{ "", NULL, 2, 2, 0,
		ARG_STRING|ARG_MAGIC|VARIANT_ATTR_ALT,
		variant_set_attribute,
		"( OLcfgOvAt:FIXME.4 NAME 'olcVariantAlternativeAttribute' "
			"DESC 'Attribute to take from the alternative entry' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	{ "", NULL, 2, 2, 0,
		ARG_DN|ARG_QUOTE|ARG_MAGIC,
		variant_set_alt_dn,
		"( OLcfgOvAt:FIXME.5 NAME 'olcVariantAlternativeEntry' "
			"DESC 'DN of the alternative entry' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX OMsDN "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	{ "", NULL, 2, 2, 0,
		ARG_BERVAL|ARG_QUOTE|ARG_MAGIC,
		variant_set_alt_pattern,
		"( OLcfgOvAt:FIXME.7 NAME 'olcVariantAlternativeEntryPattern' "
			"DESC 'Replacement pattern to locate the alternative entry' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
			NULL, NULL
	},
	/* slapd.conf alternatives for the four above */
	{ "variantSpec", "attr attr2 dn", 4, 4, 0,
		ARG_QUOTE|ARG_MAGIC,
		variant_add_alt_attr,
		NULL, NULL, NULL
	},
	{ "variantRegexSpec", "attr attr2 pattern", 4, 4, 0,
		ARG_QUOTE|ARG_MAGIC,
		variant_add_alt_attr_regex,
		NULL, NULL, NULL
	},

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs variant_ocs[] = {
	{ "( OLcfgOvOc:FIXME.1 "
		"NAME 'olcVariantConfig' "
		"DESC 'Variant overlay configuration' "
		"SUP olcOverlayConfig "
		"MAY ( olcVariantPassReplication ) )",
		Cft_Overlay, variant_cfg, NULL, variant_cfadd },
	{ "( OLcfgOvOc:FIXME.2 "
		"NAME 'olcVariantVariant' "
		"DESC 'Variant configuration' "
		"MUST ( olcVariantEntry ) "
		"MAY ( name ) "
		"SUP top "
		"STRUCTURAL )",
		Cft_Misc, variant_cfg, variant_ldadd },
	{ "( OLcfgOvOc:FIXME.3 "
		"NAME 'olcVariantAttribute' "
		"DESC 'Variant attribute description' "
		"MUST ( olcVariantVariantAttribute $ "
			"olcVariantAlternativeAttribute $ "
			"olcVariantAlternativeEntry "
		") "
		"MAY name "
		"SUP top "
		"STRUCTURAL )",
		Cft_Misc, variant_cfg, variant_attr_ldadd },
	{ "( OLcfgOvOc:FIXME.4 "
		"NAME 'olcVariantRegex' "
		"DESC 'Variant configuration' "
		"MUST ( olcVariantEntryRegex ) "
		"MAY ( name ) "
		"SUP top "
		"STRUCTURAL )",
		Cft_Misc, variant_cfg, variant_regex_ldadd },
	{ "( OLcfgOvOc:FIXME.5 "
		"NAME 'olcVariantAttributePattern' "
		"DESC 'Variant attribute description' "
		"MUST ( olcVariantVariantAttribute $ "
			"olcVariantAlternativeAttribute $ "
			"olcVariantAlternativeEntryPattern "
		") "
		"MAY name "
		"SUP top "
		"STRUCTURAL )",
		Cft_Misc, variant_cfg, variant_attr_ldadd },

	{ NULL, 0, NULL }
};

static int
variant_set_dn( ConfigArgs *ca )
{
	variantEntry_info *vei2, *vei = ca->ca_private;
	slap_overinst *on = (slap_overinst *)ca->bi;
	variant_info_t *ov = on->on_bi.bi_private;
	int diff;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		value_add_one( &ca->rvalue_vals, &vei->dn );
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		ber_memfree( vei->dn.bv_val );
		BER_BVZERO( &vei->dn );
		return LDAP_SUCCESS;
	}

	if ( !vei ) {
		vei = ch_calloc( 1, sizeof(variantEntry_info) );
		vei->ov = ov;
		vei->type = VARIANT_INFO_PLAIN;
		LDAP_SLIST_INIT(&vei->attributes);
		LDAP_STAILQ_ENTRY_INIT(vei, next);
		LDAP_STAILQ_INSERT_TAIL(&ov->variants, vei, next);

		ca->ca_private = vei;
	}
	vei->dn = ca->value_ndn;
	ber_memfree( ca->value_dn.bv_val );

	/* Each DN should only be listed once */
	LDAP_STAILQ_FOREACH( vei2, &vei->ov->variants, next ) {
		if ( vei == vei2 ) continue;

		dnMatch( &diff, 0, NULL, NULL, &vei->dn, &vei2->dn );
		if ( !diff ) {
			ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
			return ca->reply.err;
		}
	}

	return LDAP_SUCCESS;
}

static int
variant_set_regex( ConfigArgs *ca )
{
	variantEntry_info *vei2, *vei = ca->ca_private;
	slap_overinst *on = (slap_overinst *)ca->bi;
	variant_info_t *ov = on->on_bi.bi_private;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_bv = vei->dn;
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		ber_memfree( vei->dn.bv_val );
		BER_BVZERO( &vei->dn );
		regfree( vei->regex );
		return LDAP_SUCCESS;
	}

	if ( !vei ) {
		vei = ch_calloc( 1, sizeof(variantEntry_info) );
		vei->ov = ov;
		vei->type = VARIANT_INFO_REGEX;
		LDAP_SLIST_INIT(&vei->attributes);
		LDAP_STAILQ_ENTRY_INIT(vei, next);
		LDAP_STAILQ_INSERT_TAIL(&ov->regex_variants, vei, next);

		ca->ca_private = vei;
	}
	vei->dn = ca->value_bv;

	/* Each regex should only be listed once */
	LDAP_STAILQ_FOREACH( vei2, &vei->ov->regex_variants, next ) {
		if ( vei == vei2 ) continue;

		if ( !ber_bvcmp( &ca->value_bv, &vei2->dn ) ) {
			ch_free( vei );
			ca->ca_private = NULL;
			ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
			return ca->reply.err;
		}
	}

	vei->regex = ch_calloc( 1, sizeof(regex_t) );
	if ( regcomp( vei->regex, vei->dn.bv_val, REG_EXTENDED ) ) {
		ch_free( vei->regex );
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	}

	return LDAP_SUCCESS;
}

static int
variant_set_alt_dn( ConfigArgs *ca )
{
	variantAttr_info *vai = ca->ca_private;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		value_add_one( &ca->rvalue_vals, &vai->dn );
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		ber_memfree( vai->dn.bv_val );
		BER_BVZERO( &vai->dn );
		return LDAP_SUCCESS;
	}

	vai->dn = ca->value_ndn;
	ber_memfree( ca->value_dn.bv_val );

	return LDAP_SUCCESS;
}

static int
variant_set_alt_pattern( ConfigArgs *ca )
{
	variantAttr_info *vai = ca->ca_private;
	char *p = ca->value_bv.bv_val,
		 *end = ca->value_bv.bv_val + ca->value_bv.bv_len;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_bv = vai->dn;
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		ber_memfree( vai->dn.bv_val );
		BER_BVZERO( &vai->dn );
		return LDAP_SUCCESS;
	}

	while ( (p = memchr( p, '$', end - p )) != NULL ) {
		p += 1;

		if ( ( ( *p >= '0' ) && ( *p <= '9' ) ) || ( *p == '$' ) ) {
			p += 1;
		} else {
			Debug( LDAP_DEBUG_ANY, "variant_set_alt_pattern: "
					"invalid replacement pattern supplied '%s'\n",
					ca->value_bv.bv_val );
			ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
			return ca->reply.err;
		}
	}

	vai->dn = ca->value_bv;

	return LDAP_SUCCESS;
}

static int
variant_set_attribute( ConfigArgs *ca )
{
	variantAttr_info *vai2, *vai = ca->ca_private;
	char *s = ca->value_string;
	const char *text;
	AttributeDescription **ad;
	int rc;

	if ( ca->type == VARIANT_ATTR ) {
		ad = &vai->attr;
	} else {
		ad = &vai->alternative;
	}

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_string = ch_strdup( (*ad)->ad_cname.bv_val );
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		*ad = NULL;
		return LDAP_SUCCESS;
	}

	if ( *s == '{' ) {
		s = strchr( s, '}' );
		if ( !s ) {
			ca->reply.err = LDAP_UNDEFINED_TYPE;
			return ca->reply.err;
		}
		s += 1;
	}

	rc = slap_str2ad( s, ad, &text );
	ber_memfree( ca->value_string );
	if ( rc ) {
		return rc;
	}

	/* Both attributes have to share the same syntax */
	if ( vai->attr && vai->alternative &&
			vai->attr->ad_type->sat_syntax !=
					vai->alternative->ad_type->sat_syntax ) {
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	}

	if ( ca->type == VARIANT_ATTR ) {
		/* Each attribute should only be listed once */
		LDAP_SLIST_FOREACH( vai2, &vai->variant->attributes, next ) {
			if ( vai == vai2 ) continue;
			if ( vai->attr == vai2->attr ) {
				ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
				return ca->reply.err;
			}
		}
	}

	return LDAP_SUCCESS;
}

static int
variant_add_alt_attr( ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei =
			LDAP_STAILQ_LAST( &ov->variants, variantEntry_info, next );
	variantAttr_info *vai;
	struct berval dn, ndn;
	int rc;

	vai = ch_calloc( 1, sizeof(variantAttr_info) );
	vai->variant = vei;
	LDAP_SLIST_ENTRY_INIT( vai, next );
	ca->ca_private = vai;

	ca->value_string = ch_strdup( ca->argv[1] );
	ca->type = VARIANT_ATTR;
	rc = variant_set_attribute( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	ca->value_string = ch_strdup( ca->argv[2] );
	ca->type = VARIANT_ATTR_ALT;
	rc = variant_set_attribute( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	dn.bv_val = ca->argv[3];
	dn.bv_len = strlen( dn.bv_val );
	rc = dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	ca->type = 0;
	BER_BVZERO( &ca->value_dn );
	ca->value_ndn = ndn;
	rc = variant_set_alt_dn( ca );
	if ( rc != LDAP_SUCCESS ) {
		ch_free( ndn.bv_val );
		goto done;
	}

done:
	if ( rc == LDAP_SUCCESS ) {
		LDAP_SLIST_INSERT_HEAD( &vei->attributes, vai, next );
	} else {
		ca->reply.err = rc;
	}

	return rc;
}

static int
variant_add_alt_attr_regex( ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei =
			LDAP_STAILQ_LAST( &ov->regex_variants, variantEntry_info, next );
	variantAttr_info *vai;
	int rc;

	vai = ch_calloc( 1, sizeof(variantAttr_info) );
	vai->variant = vei;
	LDAP_SLIST_ENTRY_INIT( vai, next );
	ca->ca_private = vai;

	ca->value_string = ch_strdup( ca->argv[1] );
	ca->type = VARIANT_ATTR;
	rc = variant_set_attribute( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	ca->value_string = ch_strdup( ca->argv[2] );
	ca->type = VARIANT_ATTR_ALT;
	rc = variant_set_attribute( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	ca->type = 0;
	ber_str2bv( ca->argv[3], 0, 1, &ca->value_bv );
	rc = variant_set_alt_pattern( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

done:
	if ( rc == LDAP_SUCCESS ) {
		LDAP_SLIST_INSERT_HEAD( &vei->attributes, vai, next );
	} else {
		ca->reply.err = rc;
	}

	return rc;
}

static int
variant_ldadd_cleanup( ConfigArgs *ca )
{
	variantEntry_info *vei = ca->ca_private;
	slap_overinst *on = (slap_overinst *)ca->bi;
	variant_info_t *ov = on->on_bi.bi_private;

	if ( ca->reply.err != LDAP_SUCCESS ) {
		assert( LDAP_SLIST_EMPTY(&vei->attributes) );
		ch_free( vei );
		return LDAP_SUCCESS;
	}

	if ( vei->type == VARIANT_INFO_PLAIN ) {
		LDAP_STAILQ_INSERT_TAIL(&ov->variants, vei, next);
	} else {
		LDAP_STAILQ_INSERT_TAIL(&ov->regex_variants, vei, next);
	}

	return LDAP_SUCCESS;
}

static int
variant_ldadd( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	slap_overinst *on;
	variant_info_t *ov;
	variantEntry_info *vei;

	if ( cei->ce_type != Cft_Overlay || !cei->ce_bi ||
			cei->ce_bi->bi_cf_ocs != variant_ocs )
		return LDAP_CONSTRAINT_VIOLATION;

	on = (slap_overinst *)cei->ce_bi;
	ov = on->on_bi.bi_private;

	vei = ch_calloc( 1, sizeof(variantEntry_info) );
	vei->ov = ov;
	vei->type = VARIANT_INFO_PLAIN;
	LDAP_SLIST_INIT(&vei->attributes);
	LDAP_STAILQ_ENTRY_INIT(vei, next);

	ca->bi = cei->ce_bi;
	ca->ca_private = vei;
	config_push_cleanup( ca, variant_ldadd_cleanup );
	/* config_push_cleanup is only run in the case of online config but we use it to
	 * save the new config when done with the entry */
	ca->lineno = 0;

	return LDAP_SUCCESS;
}

static int
variant_regex_ldadd( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	slap_overinst *on;
	variant_info_t *ov;
	variantEntry_info *vei;

	if ( cei->ce_type != Cft_Overlay || !cei->ce_bi ||
			cei->ce_bi->bi_cf_ocs != variant_ocs )
		return LDAP_CONSTRAINT_VIOLATION;

	on = (slap_overinst *)cei->ce_bi;
	ov = on->on_bi.bi_private;

	vei = ch_calloc( 1, sizeof(variantEntry_info) );
	vei->ov = ov;
	vei->type = VARIANT_INFO_REGEX;
	LDAP_SLIST_INIT(&vei->attributes);
	LDAP_STAILQ_ENTRY_INIT(vei, next);

	ca->bi = cei->ce_bi;
	ca->ca_private = vei;
	config_push_cleanup( ca, variant_ldadd_cleanup );
	/* config_push_cleanup is only run in the case of online config but we use it to
	 * save the new config when done with the entry */
	ca->lineno = 0;

	return LDAP_SUCCESS;
}

static int
variant_attr_ldadd_cleanup( ConfigArgs *ca )
{
	variantAttr_info *vai = ca->ca_private;
	variantEntry_info *vei = vai->variant;

	if ( ca->reply.err != LDAP_SUCCESS ) {
		ch_free( vai );
		return LDAP_SUCCESS;
	}

	LDAP_SLIST_INSERT_HEAD(&vei->attributes, vai, next);

	return LDAP_SUCCESS;
}

static int
variant_attr_ldadd( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	variantEntry_info *vei;
	variantAttr_info *vai;
	CfEntryInfo *parent = cei->ce_parent;

	if ( cei->ce_type != Cft_Misc || !parent || !parent->ce_bi ||
			parent->ce_bi->bi_cf_ocs != variant_ocs )
		return LDAP_CONSTRAINT_VIOLATION;

	vei = (variantEntry_info *)cei->ce_private;

	vai = ch_calloc( 1, sizeof(variantAttr_info) );
	vai->variant = vei;
	LDAP_SLIST_ENTRY_INIT(vai, next);

	ca->ca_private = vai;
	config_push_cleanup( ca, variant_attr_ldadd_cleanup );
	/* config_push_cleanup is only run in the case of online config but we use it to
	 * save the new config when done with the entry */
	ca->lineno = 0;

	return LDAP_SUCCESS;
}

static int
variant_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	variant_info_t *ov = on->on_bi.bi_private;
	variantEntry_info *vei;
	variantAttr_info *vai;
	Entry *e;
	struct berval rdn;
	int i = 0;

	LDAP_STAILQ_FOREACH( vei, &ov->variants, next ) {
		int j = 0;
		rdn.bv_len = snprintf(
				ca->cr_msg, sizeof(ca->cr_msg), "name={%d}variant", i++ );
		rdn.bv_val = ca->cr_msg;

		ca->ca_private = vei;
		e = config_build_entry(
				op, rs, p->e_private, ca, &rdn, &variant_ocs[1], NULL );
		assert( e );

		LDAP_SLIST_FOREACH( vai, &vei->attributes, next ) {
			rdn.bv_len = snprintf( ca->cr_msg, sizeof(ca->cr_msg),
					"olcVariantVariantAttribute={%d}%s", j++,
					vai->attr->ad_cname.bv_val );
			rdn.bv_val = ca->cr_msg;

			ca->ca_private = vai;
			config_build_entry(
					op, rs, e->e_private, ca, &rdn, &variant_ocs[2], NULL );
		}
	}

	LDAP_STAILQ_FOREACH( vei, &ov->regex_variants, next ) {
		int j = 0;
		rdn.bv_len = snprintf(
				ca->cr_msg, sizeof(ca->cr_msg), "name={%d}regex", i++ );
		rdn.bv_val = ca->cr_msg;

		ca->ca_private = vei;
		e = config_build_entry(
				op, rs, p->e_private, ca, &rdn, &variant_ocs[3], NULL );
		assert( e );

		LDAP_SLIST_FOREACH( vai, &vei->attributes, next ) {
			rdn.bv_len = snprintf( ca->cr_msg, sizeof(ca->cr_msg),
					"olcVariantVariantAttribute={%d}%s", j++,
					vai->attr->ad_cname.bv_val );
			rdn.bv_val = ca->cr_msg;

			ca->ca_private = vai;
			config_build_entry(
					op, rs, e->e_private, ca, &rdn, &variant_ocs[4], NULL );
		}
	}
	return LDAP_SUCCESS;
}

static slap_overinst variant;

static int
variant_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	variant_info_t *ov;

	if ( SLAP_ISGLOBALOVERLAY(be) ) {
		Debug( LDAP_DEBUG_ANY, "variant overlay must be instantiated within "
				"a database.\n" );
		return 1;
	}

	ov = ch_calloc( 1, sizeof(variant_info_t) );
	LDAP_STAILQ_INIT(&ov->variants);
	LDAP_STAILQ_INIT(&ov->regex_variants);

	on->on_bi.bi_private = ov;

	return LDAP_SUCCESS;
}

static int
variant_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	variant_info_t *ov = on->on_bi.bi_private;

	if ( ov ) {
		while ( !LDAP_STAILQ_EMPTY( &ov->variants ) ) {
			variantEntry_info *vei = LDAP_STAILQ_FIRST( &ov->variants );
			LDAP_STAILQ_REMOVE_HEAD( &ov->variants, next );

			while ( !LDAP_SLIST_EMPTY( &vei->attributes ) ) {
				variantAttr_info *vai = LDAP_SLIST_FIRST( &vei->attributes );
				LDAP_SLIST_REMOVE_HEAD( &vei->attributes, next );

				ber_memfree( vai->dn.bv_val );
				ch_free( vai );
			}
			ber_memfree( vei->dn.bv_val );
			ch_free( vei );
		}
		while ( !LDAP_STAILQ_EMPTY( &ov->regex_variants ) ) {
			variantEntry_info *vei = LDAP_STAILQ_FIRST( &ov->regex_variants );
			LDAP_STAILQ_REMOVE_HEAD( &ov->regex_variants, next );

			while ( !LDAP_SLIST_EMPTY( &vei->attributes ) ) {
				variantAttr_info *vai = LDAP_SLIST_FIRST( &vei->attributes );
				LDAP_SLIST_REMOVE_HEAD( &vei->attributes, next );

				ber_memfree( vai->dn.bv_val );
				ch_free( vai );
			}
			ber_memfree( vei->dn.bv_val );
			ch_free( vei );
		}
		ch_free( ov );
	}

	return LDAP_SUCCESS;
}

int
variant_initialize()
{
	int rc;

	variant.on_bi.bi_type = "variant";
	variant.on_bi.bi_db_init = variant_db_init;
	variant.on_bi.bi_db_destroy = variant_db_destroy;

	variant.on_bi.bi_op_add = variant_op_add;
	variant.on_bi.bi_op_compare = variant_op_compare;
	variant.on_bi.bi_op_modify = variant_op_mod;
	variant.on_bi.bi_op_search = variant_op_search;

	variant.on_bi.bi_cf_ocs = variant_ocs;

	rc = config_register_schema( variant_cfg, variant_ocs );
	if ( rc ) return rc;

	return overlay_register( &variant );
}

#if SLAPD_OVER_VARIANT == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return variant_initialize();
}
#endif

#endif /* SLAPD_OVER_VARIANT */
