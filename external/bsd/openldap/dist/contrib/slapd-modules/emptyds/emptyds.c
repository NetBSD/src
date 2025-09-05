/*	$NetBSD: emptyds.c,v 1.2 2025/09/05 21:16:16 christos Exp $	*/

/* emptyds.c */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2014-2024 The OpenLDAP Foundation.
 * Portions Copyright (C) 2014 DAASI International GmbH, Tamim Ziai.
 * Portions Copyright (C) 2022 Ondřej Kuzník, Symas Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * http://www.OpenLDAP.org/license.html.
 */
/* ACKNOLEDGEDMENTS:
 * This work was initially developed by Tamim Ziai of DAASI International GmbH
 * for inclusion in OpenLDAP Software.
 */
/* slapo-emptyds
 *
 * This is an OpenLDAP overlay that accepts empty strings as attribute values
 * without syntax violation but never actually stores them. This allows
 * applications that used to work with LDAP implementations allowing empty
 * strings (such as Novel eDirectory) to continue to work with OpenLDAP without
 * any modifications. Add and modify change types will be proceeded as follows,
 * other operations will be forwarded without modifications:
 *
 * changeType: add                  changeType: add
 * sn: <empty>              -->     sn: blah
 * sn: blah
 *
 * changeType: modify               changeType: modify
 * add: sn                  -->     add: sn
 * sn: <empty>                      sn: blah
 * sn: blah
 *
 * changeType: modify               changeType: modify
 * delete: sn               -->     delete: sn
 * sn: <empty>                      sn: blah
 * sn: blah
 *
 * changeType: modify               changeType: modify
 * replace: sn              -->     replace: sn
 * sn: <empty>
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: emptyds.c,v 1.2 2025/09/05 21:16:16 christos Exp $");

#include "portable.h"
#include "slap.h"

static slap_overinst emptyds;

static const char ds_oid[] = "1.3.6.1.4.1.1466.115.121.1.15";

static slap_syntax_validate_func *ssyn_validate_original = NULL;
static slap_syntax_transform_func *ssyn_pretty_original = NULL;
static int emptyds_instances = 0;

static unsigned int
remove_empty_values( Modification *m, Attribute *a )
{
	BerVarray vals = m ? m->sm_values : a->a_vals,
			  nvals = m ? m->sm_nvalues : a->a_nvals;
	unsigned int i, j, numvals = m ? m->sm_numvals : a->a_numvals;

	for ( i = 0; i < numvals && !BER_BVISEMPTY( &vals[i] ); i++ )
		/* Find first empty */;

	if ( i == numvals ) return i;

	/*
	 * We have an empty value at index i, move all of them to the end of the
	 * list, preserving the order of non-empty values.
	 */
	j = i + 1;
	for ( j = i + 1; j < numvals; j++ ) {
		struct berval tmp;

		if ( BER_BVISEMPTY( &vals[j] ) ) continue;

		tmp = vals[i];
		vals[i] = vals[j];
		vals[j] = tmp;

		if ( nvals && vals != nvals ) {
			tmp = nvals[i];
			nvals[i] = nvals[j];
			nvals[j] = tmp;
		}

		if ( m && a && m->sm_values != a->a_vals ) {
			tmp = a->a_vals[i];
			a->a_vals[i] = a->a_vals[j];
			a->a_vals[j] = tmp;

			if ( a->a_nvals && a->a_vals != a->a_nvals ) {
				tmp = a->a_nvals[i];
				a->a_nvals[i] = a->a_nvals[j];
				a->a_nvals[j] = tmp;
			}
		}
		i++;
	}

	/* Free empty vals */
	for ( ; j && i < j--; ) {
		ber_memfree( vals[j].bv_val );
		if ( nvals && vals != nvals ) {
			ber_memfree( nvals[j].bv_val );
			BER_BVZERO( &nvals[j] );
		}

		if ( m && a && m->sm_values != a->a_vals ) {
			if ( m->sm_values[j].bv_val != a->a_vals[j].bv_val ) {
				ber_memfree( a->a_vals[j].bv_val );
				BER_BVZERO( &a->a_vals[j] );

				if ( a->a_nvals && a->a_vals != a->a_nvals ) {
					ber_memfree( a->a_nvals[j].bv_val );
					BER_BVZERO( &a->a_nvals[j] );
				}
			}
		}
		BER_BVZERO( &vals[j] );
	}

	return i;
}

/**
 *  Remove all operations with empty strings.
 */
static int
emptyds_op_add( Operation *op, SlapReply *rs )
{
	Attribute **ap, **nexta, *a;
	Modifications **mlp, **nextp = NULL, *ml;
	Entry *e = op->ora_e;

	/*
	 * op->ora_modlist can be NULL, at least accesslog doesn't always populate
	 * it on an add.
	 */
	for ( ap = &e->e_attrs, a = e->e_attrs, mlp = &op->ora_modlist,
		  ml = op->ora_modlist;
			a != NULL;
			ap = nexta, a = *ap, mlp = nextp, ml = ml ? *mlp : NULL ) {
		AttributeType *at = a->a_desc->ad_type;
		unsigned int remaining;

		nexta = &a->a_next;
		if ( ml ) {
			nextp = &ml->sml_next;
		}

		if ( at->sat_syntax != slap_schema.si_syn_directoryString ||
				at->sat_atype.at_usage != LDAP_SCHEMA_USER_APPLICATIONS )
			continue;

		remaining = remove_empty_values( &ml->sml_mod, a );
		if ( remaining == a->a_numvals ) continue;
		/* Empty values found */

		if ( !remaining ) {
			/* All values are empty */
			*ap = a->a_next;
			a->a_next = NULL;
			nexta = ap;

			if ( ml ) {
				*mlp = ml->sml_next;
				ml->sml_next = NULL;
				nextp = mlp;
				/* Values are generally shared with attribute */
				slap_mods_free( ml, ml->sml_values != a->a_vals );
			}
			attr_free( a );
		} else {
			a->a_numvals = remaining;
			if ( ml ) {
				ml->sml_mod.sm_numvals = remaining;
			}
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
emptyds_op_modify( Operation *op, SlapReply *rs )
{
	Modifications **mlp, **nextp, *ml;

	for ( mlp = &op->orm_modlist, ml = op->orm_modlist; ml != NULL;
			mlp = nextp, ml = *mlp ) {
		AttributeType *at = ml->sml_desc->ad_type;
		unsigned int remaining;

		nextp = &ml->sml_next;

		if ( at->sat_syntax != slap_schema.si_syn_directoryString ||
				at->sat_atype.at_usage != LDAP_SCHEMA_USER_APPLICATIONS )
			continue;

		remaining = remove_empty_values( &ml->sml_mod, NULL );
		if ( remaining == ml->sml_numvals ) continue;

		if ( !remaining ) {
			/* All values are empty */
			if ( ml->sml_op == LDAP_MOD_REPLACE ) {
				/* Replace is kept */
				if ( ml->sml_nvalues && ml->sml_nvalues != ml->sml_values ) {
					ber_bvarray_free( ml->sml_nvalues );
				}
				if ( ml->sml_values ) {
					ber_bvarray_free( ml->sml_values );
				}

				ml->sml_numvals = 0;
				ml->sml_values = NULL;
				ml->sml_nvalues = NULL;
			} else {
				/* Remove modification */
				*mlp = ml->sml_next;
				ml->sml_next = NULL;
				nextp = mlp;
				slap_mods_free( ml, 1 );
			}
		} else {
			ml->sml_numvals = remaining;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
emptyds_ssyn_validate( Syntax *syntax, struct berval *in )
{
	if ( BER_BVISEMPTY( in ) && syntax == slap_schema.si_syn_directoryString ) {
		return LDAP_SUCCESS;
	}
	return ssyn_validate_original( syntax, in );
}

static int
emptyds_ssyn_pretty( Syntax *syntax,
		struct berval *in,
		struct berval *out,
		void *memctx )
{
	if ( BER_BVISEMPTY( in ) && syntax == slap_schema.si_syn_directoryString ) {
		return LDAP_SUCCESS;
	}
	return ssyn_pretty_original( syntax, in, out, memctx );
}

static int
emptyds_db_init( BackendDB *be, ConfigReply *cr )
{
	Syntax *syntax = syn_find( ds_oid );

	if ( syntax == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "emptyds_db_init: "
				"Syntax %s not found\n",
				ds_oid );
	} else {
		Debug( LDAP_DEBUG_TRACE, "emptyds_db_init: "
				"Found syntax: %s\n",
				syntax->ssyn_bvoid.bv_val );
		if ( ssyn_validate_original == NULL && syntax->ssyn_validate != NULL ) {
			ssyn_validate_original = syntax->ssyn_validate;
			syntax->ssyn_validate = emptyds_ssyn_validate;
		}
		if ( ssyn_pretty_original == NULL && syntax->ssyn_pretty != NULL ) {
			ssyn_pretty_original = syntax->ssyn_pretty;
			syntax->ssyn_pretty = &emptyds_ssyn_pretty;
		}
	}

	emptyds_instances++;
	return LDAP_SUCCESS;
}

static int
emptyds_db_destroy( BackendDB *be, ConfigReply *cr )
{
	Syntax *syntax = syn_find( ds_oid );

	if ( --emptyds_instances == 0 && syntax != NULL ) {
		if ( syntax->ssyn_validate == emptyds_ssyn_validate ) {
			syntax->ssyn_validate = ssyn_validate_original;
		}
		ssyn_validate_original = NULL;

		if ( syntax->ssyn_pretty == emptyds_ssyn_pretty ) {
			syntax->ssyn_pretty = ssyn_pretty_original;
		}
		ssyn_pretty_original = NULL;
	}

	assert( emptyds_instances >= 0 );
	return LDAP_SUCCESS;
}

int
emptyds_init()
{
	emptyds.on_bi.bi_type = "emptyds";
	emptyds.on_bi.bi_op_add = emptyds_op_add;
	emptyds.on_bi.bi_op_modify = emptyds_op_modify;
	emptyds.on_bi.bi_db_init = emptyds_db_init;
	emptyds.on_bi.bi_db_destroy = emptyds_db_destroy;

	return overlay_register( &emptyds );
}

int
init_module( int argc, char *argv[] )
{
	return emptyds_init();
}
