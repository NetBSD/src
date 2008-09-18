/* collect.c - Demonstration of overlay code */
/* $OpenLDAP: pkg/ldap/servers/slapd/overlays/collect.c,v 1.5.2.4 2008/02/11 23:26:48 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * Portions Copyright 2003 Howard Chu.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_COLLECT

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"

/* This is a cheap hack to implement a collective attribute.
 *
 * This demonstration overlay looks for a specified attribute in an
 * ancestor of a given entry and adds that attribute to the given
 * entry when it is returned in a search response. It takes no effect
 * for any other operations. If the ancestor does not exist, there
 * is no effect. If no attribute was configured, there is no effect.
 */

typedef struct collect_info {
	struct collect_info *ci_next;
	struct berval ci_dn;
	AttributeDescription *ci_ad;
} collect_info;

static int
collect_cf( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	int rc = 1;

	switch( c->op ) {
	case SLAP_CONFIG_EMIT:
		{
		collect_info *ci;
		for ( ci = on->on_bi.bi_private; ci; ci = ci->ci_next ) {
			struct berval bv;
			int len;

			bv.bv_len = ci->ci_dn.bv_len + STRLENOF("\"\" ") +
				ci->ci_ad->ad_cname.bv_len;
			bv.bv_val = ch_malloc( bv.bv_len + 1 );
			len = snprintf( bv.bv_val, bv.bv_len + 1, "\"%s\" %s",
				ci->ci_dn.bv_val, ci->ci_ad->ad_cname.bv_val );
			assert( len == bv.bv_len );
			ber_bvarray_add( &c->rvalue_vals, &bv );
			rc = 0;
		}
		}
		break;
	case LDAP_MOD_DELETE:
		if ( c->valx == -1 ) {
		/* Delete entire attribute */
			collect_info *ci;
			while (( ci = on->on_bi.bi_private )) {
				on->on_bi.bi_private = ci->ci_next;
				ch_free( ci->ci_dn.bv_val );
				ch_free( ci );
			}
		} else {
		/* Delete just one value */
			collect_info **cip, *ci;
			int i;
			cip = (collect_info **)&on->on_bi.bi_private;
			for ( i=0; i <= c->valx; i++, cip = &ci->ci_next ) ci = *cip;
			*cip = ci->ci_next;
			ch_free( ci->ci_dn.bv_val );
			ch_free( ci );
		}
		rc = 0;
		break;
	case SLAP_CONFIG_ADD:
	case LDAP_MOD_ADD:
		{
		collect_info *ci;
		struct berval bv, dn;
		const char *text;
		AttributeDescription *ad = NULL;

		ber_str2bv( c->argv[1], 0, 0, &bv );
		if ( dnNormalize( 0, NULL, NULL, &bv, &dn, NULL ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid DN: \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		if ( slap_str2ad( c->argv[2], &ad, &text ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s attribute description unknown: \"%s\"",
				c->argv[0], c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		/* The on->on_bi.bi_private pointer can be used for
		 * anything this instance of the overlay needs.
		 */
		ci = ch_malloc( sizeof( collect_info ));
		ci->ci_ad = ad;
		ci->ci_dn = dn;
		ci->ci_next = on->on_bi.bi_private;
		on->on_bi.bi_private = ci;
		rc = 0;
		}
	}
	return rc;
}

static ConfigTable collectcfg[] = {
	{ "collectinfo", "dn> <attribute", 3, 3, 0,
	  ARG_MAGIC, collect_cf,
	  "( OLcfgOvAt:19.1 NAME 'olcCollectInfo' "
	  "DESC 'DN of entry and attribute to distribute' "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs collectocs[] = {
	{ "( OLcfgOvOc:19.1 "
	  "NAME 'olcCollectConfig' "
	  "DESC 'Collective Attribute configuration' "
	  "SUP olcOverlayConfig "
	  "MAY olcCollectInfo )",
	  Cft_Overlay, collectcfg },
	{ NULL, 0, NULL }
};

static int
collect_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	collect_info *ci;

	while (( ci = on->on_bi.bi_private )) {
		on->on_bi.bi_private = ci->ci_next;
		ch_free( ci->ci_dn.bv_val );
		ch_free( ci );
	}
	return 0;
}

static int
collect_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	collect_info *ci = on->on_bi.bi_private;

	/* If we've been configured and the current response is
	 * a search entry
	 */
	if ( ci && rs->sr_type == REP_SEARCH ) {
		int rc;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;

		for (; ci; ci=ci->ci_next ) {
			BerVarray vals = NULL;

			/* Is our configured entry an ancestor of this one? */
			if ( !dnIsSuffix( &rs->sr_entry->e_nname, &ci->ci_dn ))
				continue;

			/* Extract the values of the desired attribute from
			 * the ancestor entry
			 */
			rc = backend_attribute( op, NULL, &ci->ci_dn, ci->ci_ad, &vals, ACL_READ );

			/* If there are any values, merge them into the
			 * current entry
			 */
			if ( vals ) {
				/* The current entry may live in a cache, so
				 * don't modify it directly. Make a copy and
				 * work with that instead.
				 */
				if ( !( rs->sr_flags & REP_ENTRY_MODIFIABLE )) {
					rs->sr_entry = entry_dup( rs->sr_entry );
					rs->sr_flags |= REP_ENTRY_MODIFIABLE |
						REP_ENTRY_MUSTBEFREED;
				}
				attr_merge( rs->sr_entry, ci->ci_ad, vals, NULL );
				ber_bvarray_free_x( vals, op->o_tmpmemctx );
			}
		}
	}
	/* Default is to just fall through to the normal processing */
	return SLAP_CB_CONTINUE;
}

static slap_overinst collect;

int collect_initialize() {
	int code;

	collect.on_bi.bi_type = "collect";
	collect.on_bi.bi_db_destroy = collect_destroy;
	collect.on_response = collect_response;

	collect.on_bi.bi_cf_ocs = collectocs;
	code = config_register_schema( collectcfg, collectocs );
	if ( code ) return code;

	return overlay_register( &collect );
}

#if SLAPD_OVER_COLLECT == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return collect_initialize();
}
#endif

#endif /* SLAPD_OVER_COLLECT */
