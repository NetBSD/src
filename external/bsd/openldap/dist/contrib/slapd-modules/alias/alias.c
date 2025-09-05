/*	$NetBSD: alias.c,v 1.2 2025/09/05 21:16:15 christos Exp $	*/

/* alias.c - expose an attribute under a different name */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2023 The OpenLDAP Foundation.
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
 * This work was developed in 2023 by Ondřej Kuzník for Symas Corp.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: alias.c,v 1.2 2025/09/05 21:16:15 christos Exp $");

#include "portable.h"

#ifdef SLAPD_OVER_ALIAS

#include <inttypes.h>
#include <ac/stdlib.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"
#include "ldap_queue.h"

typedef struct alias_mapping_t {
	AttributeDescription *source;
	AttributeDescription *alias;
} alias_mapping;

typedef struct alias_info_t {
	alias_mapping *mappings;
} alias_info;

typedef struct alias_sc_private_t {
	slap_overinst *on;
	AttributeName *attrs_orig, *attrs_new;
} alias_sc_private;

static alias_mapping *
attribute_mapped( alias_info *ov, AttributeDescription *ad )
{
	alias_mapping *m;

	for ( m = ov->mappings; m && m->source; m++ ) {
		if ( ad == m->alias ) return m;
	}

	return NULL;
}

static int
alias_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	alias_info *ov = on->on_bi.bi_private;
	Entry *e = op->ora_e;
	Attribute *a;
	int rc = LDAP_SUCCESS;

	if ( !BER_BVISEMPTY( &e->e_nname ) ) {
		LDAPRDN rDN;
		const char *p;
		int i;

		rc = ldap_bv2rdn_x( &e->e_nname, &rDN, (char **)&p, LDAP_DN_FORMAT_LDAP,
				op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "alias_op_add: "
					"can't parse rdn: dn=%s\n",
					op->o_req_ndn.bv_val );
			return SLAP_CB_CONTINUE;
		}

		for ( i = 0; rDN[i]; i++ ) {
			AttributeDescription *ad = NULL;

			/* If we can't resolve the attribute, ignore it */
			if ( slap_bv2ad( &rDN[i]->la_attr, &ad, &p ) ) {
				continue;
			}

			if ( attribute_mapped( ov, ad ) ) {
				rc = LDAP_CONSTRAINT_VIOLATION;
				break;
			}
		}

		ldap_rdnfree_x( rDN, op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			send_ldap_error( op, rs, rc,
					"trying to add a virtual attribute in RDN" );
			return rc;
		}
	}

	for ( a = e->e_attrs; a; a = a->a_next ) {
		if ( attribute_mapped( ov, a->a_desc ) ) {
			rc = LDAP_CONSTRAINT_VIOLATION;
			send_ldap_error( op, rs, rc,
					"trying to add a virtual attribute" );
			return LDAP_CONSTRAINT_VIOLATION;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
alias_op_compare( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	alias_info *ov = on->on_bi.bi_private;
	alias_mapping *alias = attribute_mapped( ov, op->orc_ava->aa_desc );

	if ( alias )
		op->orc_ava->aa_desc = alias->source;

	return SLAP_CB_CONTINUE;
}

static int
alias_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	alias_info *ov = on->on_bi.bi_private;
	Modifications *mod;
	int rc = LDAP_CONSTRAINT_VIOLATION;

	for ( mod = op->orm_modlist; mod; mod = mod->sml_next ) {
		if ( attribute_mapped( ov, mod->sml_desc ) ) {
			send_ldap_error( op, rs, rc,
					"trying to modify a virtual attribute" );
			return LDAP_CONSTRAINT_VIOLATION;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
alias_op_modrdn( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	alias_info *ov = on->on_bi.bi_private;
	LDAPRDN rDN;
	const char *p;
	int i, rc = SLAP_CB_CONTINUE;

	rc = ldap_bv2rdn_x( &op->orr_nnewrdn, &rDN, (char **)&p,
			LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "alias_op_modrdn: "
				"can't parse rdn for dn=%s\n",
				op->o_req_ndn.bv_val );
		return SLAP_CB_CONTINUE;
	}

	for ( i = 0; rDN[i]; i++ ) {
		AttributeDescription *ad = NULL;

		/* If we can't resolve the attribute, ignore it */
		if ( slap_bv2ad( &rDN[i]->la_attr, &ad, &p ) ) {
			continue;
		}

		if ( attribute_mapped( ov, ad ) ) {
			rc = LDAP_CONSTRAINT_VIOLATION;
			break;
		}
	}

	ldap_rdnfree_x( rDN, op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_error( op, rs, rc,
				"trying to add a virtual attribute in RDN" );
		return rc;
	}

	return SLAP_CB_CONTINUE;
}

static int
alias_response_cleanup( Operation *op, SlapReply *rs )
{
	alias_sc_private *data = op->o_callback->sc_private;

	if ( rs->sr_type == REP_RESULT || op->o_abandon ||
			rs->sr_err == SLAPD_ABANDON )
	{
		if ( op->ors_attrs == data->attrs_new )
			op->ors_attrs = data->attrs_orig;

		ch_free( data->attrs_new );
		ch_free( op->o_callback );
		op->o_callback = NULL;
	}

	return SLAP_CB_CONTINUE;
}

static int
alias_response( Operation *op, SlapReply *rs )
{
	alias_sc_private *data = op->o_callback->sc_private;
	slap_overinst *on = data->on;
	alias_info *ov = on->on_bi.bi_private;
	Entry *e = NULL, *e_orig = rs->sr_entry;
	alias_mapping *mapping;
	int rc = SLAP_CB_CONTINUE;

	if ( rs->sr_type != REP_SEARCH || !e_orig ) {
		return rc;
	}

	for ( mapping = ov->mappings; mapping && mapping->source; mapping++ ) {
		Attribute *source, *a;
		int operational = is_at_operational( mapping->source->ad_type ),
			keep_source = 0;
		slap_mask_t requested = operational ?
			SLAP_OPATTRS_YES : SLAP_USERATTRS_YES;

		if ( !(requested & rs->sr_attr_flags) &&
				!ad_inlist( mapping->alias, rs->sr_attrs ) )
			continue;

		/* TODO: deal with multiple aliases from the same source */
		if ( (requested & rs->sr_attr_flags) ||
				ad_inlist( mapping->source, data->attrs_orig ) ) {
			keep_source = 1;
		}

		if ( operational ) {
			source = attr_find( rs->sr_operational_attrs, mapping->source );
		}
		if ( !source ) {
			operational = 0;
			source = attr_find( e_orig->e_attrs, mapping->source );
		}
		if ( !source )
			continue;

		if ( operational ) {
			if ( !keep_source ) {
				source->a_desc = mapping->alias;
			} else {
				Attribute **ap;

				a = attr_dup( source );
				a->a_desc = mapping->alias;

				for ( ap = &rs->sr_operational_attrs; *ap; ap=&(*ap)->a_next );
				*ap = a;
			}
			continue;
		}

		if ( !e ) {
			if ( rs->sr_flags & REP_ENTRY_MODIFIABLE ) {
				e = e_orig;
			} else {
				e = entry_dup( e_orig );
			}
		}

		a = attr_find( e->e_attrs, mapping->source );
		if ( !keep_source ) {
			a->a_desc = mapping->alias;
		} else {
			attr_merge( e, mapping->alias, a->a_vals, a->a_nvals );
		}
	}

	if ( e && e != e_orig ) {
		rs_replace_entry( op, rs, on, e );
		rs->sr_flags &= ~REP_ENTRY_MASK;
		rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
	}

	return rc;
}

static int
alias_filter( alias_info *ov, Filter *f )
{
	int changed = 0;

	switch ( f->f_choice ) {
		case LDAP_FILTER_AND:
		case LDAP_FILTER_OR: {
			for ( f = f->f_and; f; f = f->f_next ) {
				int result = alias_filter( ov, f );
				if ( result < 0 ) {
					return result;
				}
				changed += result;
			}
		} break;

		case LDAP_FILTER_NOT:
			return alias_filter( ov, f->f_not );

		case LDAP_FILTER_PRESENT: {
			alias_mapping *alias = attribute_mapped( ov, f->f_desc );
			if ( alias ) {
				f->f_desc = alias->source;
				changed = 1;
			}
		} break;

		case LDAP_FILTER_APPROX:
		case LDAP_FILTER_EQUALITY:
		case LDAP_FILTER_GE:
		case LDAP_FILTER_LE: {
			alias_mapping *alias = attribute_mapped( ov, f->f_av_desc );
			if ( alias ) {
				f->f_av_desc = alias->source;
				changed = 1;
			}
		} break;

		case LDAP_FILTER_SUBSTRINGS: {
			alias_mapping *alias = attribute_mapped( ov, f->f_sub_desc );
			if ( alias ) {
				f->f_sub_desc = alias->source;
				changed = 1;
			}
		} break;

		case LDAP_FILTER_EXT: {
			alias_mapping *alias = attribute_mapped( ov, f->f_mr_desc );
			if ( alias ) {
				f->f_mr_desc = alias->source;
				changed = 1;
			}
		} break;

		default:
			return -1;
	}
	return changed;
}

static int
alias_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	alias_info *ov = on->on_bi.bi_private;
	alias_mapping *mapping;
	AttributeName *an_orig = NULL, *an_new = NULL;
	int mapped, an_length = 0;

	if ( get_manageDSAit( op ) )
		return SLAP_CB_CONTINUE;

	/*
	 * 1. check filter: traverse, map aliased attributes
	 * 2. unparse filter
	 * 3. check all requested attributes -> register callback if one matches
	 */
	if ( (mapped = alias_filter( ov, op->ors_filter )) < 0 ) {
		send_ldap_error( op, rs, LDAP_OTHER,
				"alias_op_search: failed to process filter" );
		return LDAP_OTHER;
	}

	if ( mapped ) {
		op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
	}

	mapped = 0;
	for ( mapping = ov->mappings; mapping && mapping->source; mapping++ ) {
		int operational = is_at_operational( mapping->source->ad_type );
		slap_mask_t requested = operational ?
			SLAP_OPATTRS_YES : SLAP_USERATTRS_YES;

		if ( requested & slap_attr_flags( op->ors_attrs ) ) {
			mapped = 1;
		} else if ( ad_inlist( mapping->alias, op->ors_attrs ) ) {
			mapped = 1;
			if ( !an_length ) {
				for ( ; !BER_BVISNULL( &op->ors_attrs[an_length].an_name ); an_length++ )
					/* Count */;
			}

			an_new = ch_realloc( an_new, (an_length+2)*sizeof(AttributeName) );
			if ( !an_orig ) {
				int i;
				an_orig = op->ors_attrs;
				for ( i=0; i < an_length; i++ ) {
					an_new[i] = an_orig[i];
				}
			}

			an_new[an_length].an_name = mapping->source->ad_cname;
			an_new[an_length].an_desc = mapping->source;
			an_length++;

			BER_BVZERO( &an_new[an_length].an_name );
		}
	}

	if ( mapped ) {
		/* We have something to map back */
		slap_callback *cb = op->o_tmpcalloc( 1,
				sizeof(slap_callback)+sizeof(alias_sc_private),
				op->o_tmpmemctx );
		alias_sc_private *data = (alias_sc_private *)(cb+1);

		data->on = on;

		cb->sc_response = alias_response;
		cb->sc_private = data;
		cb->sc_next = op->o_callback;
		cb->sc_cleanup = alias_response_cleanup;

		if ( an_new ) {
			data->attrs_orig = an_orig;
			data->attrs_new = an_new;
			op->ors_attrs = an_new;
		}

		op->o_callback = cb;
	}

	return SLAP_CB_CONTINUE;
}

/* Configuration */

static ConfigDriver alias_config_mapping;

static ConfigTable alias_cfg[] = {
	{ "alias_attribute", "attr> <attr", 3, 3, 0,
		ARG_MAGIC,
		alias_config_mapping,
		"( OLcfgCtAt:10.1 NAME 'olcAliasMapping' "
			"DESC 'Alias definition' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )",
		NULL, NULL
	},

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

/*
 * FIXME: There is no reason to keep olcAliasMapping MAY (making this overlay
 * a noop) except we can't enforce a MUST with slaptest+slapd.conf.
 */
static ConfigOCs alias_ocs[] = {
	{ "( OLcfgCtOc:10.1 "
		"NAME 'olcAliasConfig' "
		"DESC 'Alias overlay configuration' "
		"MAY ( olcAliasMapping ) "
		"SUP olcOverlayConfig )",
		Cft_Overlay, alias_cfg },

	{ NULL, 0, NULL }
};

static int
alias_config_mapping( ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	alias_info *ov = on->on_bi.bi_private;
	AttributeDescription *source = NULL, *alias = NULL;
	AttributeType *sat, *aat;
	const char *text;
	int i, rc = LDAP_CONSTRAINT_VIOLATION;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		alias_mapping *mapping;

		for ( mapping = ov->mappings; mapping && mapping->source; mapping++ ) {
			char buf[SLAP_TEXT_BUFLEN];
			struct berval bv = { .bv_val = buf, .bv_len = SLAP_TEXT_BUFLEN };
			bv.bv_len = snprintf( buf, bv.bv_len, "%s %s",
					mapping->source->ad_cname.bv_val,
					mapping->alias->ad_cname.bv_val );
			value_add_one( &ca->rvalue_vals, &bv );
		}
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		if ( ca->valx < 0 ) {
			ch_free( ov->mappings );
			ov->mappings = NULL;
		} else {
			i = ca->valx;
			do {
				ov->mappings[i] = ov->mappings[i+1];
				i++;
			} while ( ov->mappings[i].source );
		}
		return LDAP_SUCCESS;
	}

	rc = slap_str2ad( ca->argv[1], &source, &text );
	if ( rc ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"cannot resolve attribute '%s': \"%s\"",
				ca->argv[1], text );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		goto done;
	}

	rc = slap_str2ad( ca->argv[2], &alias, &text );
	if ( rc ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"cannot resolve attribute '%s': \"%s\"",
				ca->argv[2], text );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		goto done;
	}

	sat = source->ad_type;
	aat = alias->ad_type;
	if ( sat == aat ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"cannot map attribute %s to itself",
				source->ad_cname.bv_val );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;
	}

	/* The types have to match */
	if ( is_at_operational( sat ) != is_at_operational( aat ) ||
			is_at_single_value( sat ) != is_at_single_value( aat ) ||
			sat->sat_syntax != aat->sat_syntax ||
			sat->sat_equality != aat->sat_equality ||
			sat->sat_approx != aat->sat_approx ||
			sat->sat_ordering != aat->sat_ordering ||
			sat->sat_substr != aat->sat_substr ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"attributes %s and %s syntax and/or "
				"default matching rules don't match",
				source->ad_cname.bv_val,
				alias->ad_cname.bv_val );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;
	}

	if ( !ov->mappings ) {
		ov->mappings = ch_calloc( 2, sizeof(alias_mapping) );
		ov->mappings[0].source = source;
		ov->mappings[0].alias = alias;
	} else {
		int i;

		for ( i = 0; ov->mappings[i].source; i++ ) {
			if ( alias == ov->mappings[i].alias ) {
				snprintf( ca->cr_msg, sizeof(ca->cr_msg),
						"attribute %s already mapped from %s",
						alias->ad_cname.bv_val,
						ov->mappings[i].source->ad_cname.bv_val );
				Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
				rc = LDAP_CONSTRAINT_VIOLATION;
				goto done;
			}
			if ( alias == ov->mappings[i].source ) {
				snprintf( ca->cr_msg, sizeof(ca->cr_msg),
						"cannot use %s as alias source, already mapped from %s",
						source->ad_cname.bv_val,
						ov->mappings[i].source->ad_cname.bv_val );
				Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
				rc = LDAP_CONSTRAINT_VIOLATION;
				goto done;
			}
			if ( source == ov->mappings[i].alias ) {
				snprintf( ca->cr_msg, sizeof(ca->cr_msg),
						"cannot use %s as alias, it is aliased to %s",
						alias->ad_cname.bv_val,
						ov->mappings[i].alias->ad_cname.bv_val );
				Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
				rc = LDAP_CONSTRAINT_VIOLATION;
				goto done;
			}
		}

		if ( ca->valx < 0 || ca->valx > i )
			ca->valx = i;

		i++;
		ov->mappings = ch_realloc( ov->mappings, (i + 1) * sizeof(alias_mapping) );
		do {
			ov->mappings[i] = ov->mappings[i-1];
		} while ( --i > ca->valx );
		ov->mappings[i].source = source;
		ov->mappings[i].alias = alias;
	}

	rc = LDAP_SUCCESS;
done:
	ca->reply.err = rc;
	return rc;
}

static slap_overinst alias;

static int
alias_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	alias_info *ov;

	/* TODO: can this be global? */
	if ( SLAP_ISGLOBALOVERLAY(be) ) {
		Debug( LDAP_DEBUG_ANY, "alias overlay must be instantiated "
				"within a database.\n" );
		return 1;
	}

	ov = ch_calloc( 1, sizeof(alias_info) );
	on->on_bi.bi_private = ov;

	return LDAP_SUCCESS;
}

static int
alias_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	alias_info *ov = on->on_bi.bi_private;

	if ( ov && ov->mappings ) {
		ch_free( ov->mappings );
	}
	ch_free( ov );

	return LDAP_SUCCESS;
}

int
alias_initialize()
{
	int rc;

	alias.on_bi.bi_type = "alias";
	alias.on_bi.bi_db_init = alias_db_init;
	alias.on_bi.bi_db_destroy = alias_db_destroy;

	alias.on_bi.bi_op_add = alias_op_add;
	alias.on_bi.bi_op_compare = alias_op_compare;
	alias.on_bi.bi_op_modify = alias_op_mod;
	alias.on_bi.bi_op_modrdn = alias_op_modrdn;
	alias.on_bi.bi_op_search = alias_op_search;

	alias.on_bi.bi_cf_ocs = alias_ocs;

	rc = config_register_schema( alias_cfg, alias_ocs );
	if ( rc ) return rc;

	return overlay_register( &alias );
}

#if SLAPD_OVER_ALIAS == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return alias_initialize();
}
#endif

#endif /* SLAPD_OVER_ALIAS */
