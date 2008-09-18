/* init.c - initialize relay backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-relay/init.c,v 1.19.2.4 2008/02/12 01:03:16 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2008 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "config.h"
#include "back-relay.h"

static ConfigDriver relay_back_cf;

static ConfigTable relaycfg[] = {
	{ "relay", "relay", 2, 2, 0,
		ARG_MAGIC|ARG_DN,
		relay_back_cf, "( OLcfgDbAt:5.1 "
			"NAME 'olcRelay' "
			"DESC 'Relay DN' "
			"SYNTAX OMsDN "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ NULL }
};

static ConfigOCs relayocs[] = {
	{ "( OLcfgDbOc:5.1 "
		"NAME 'olcRelayConfig' "
		"DESC 'Relay backend configuration' "
		"SUP olcDatabaseConfig "
		"MAY ( olcRelay "
		") )",
		 	Cft_Database, relaycfg},
	{ NULL, 0, NULL }
};

static int
relay_back_cf( ConfigArgs *c )
{
	relay_back_info	*ri = ( relay_back_info * )c->be->be_private;
	int		rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		if ( ri != NULL && !BER_BVISNULL( &ri->ri_realsuffix ) ) {
			value_add_one( &c->rvalue_vals, &ri->ri_realsuffix );
			return 0;
		}
		return 1;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !BER_BVISNULL( &ri->ri_realsuffix ) ) {
			ch_free( ri->ri_realsuffix.bv_val );
			BER_BVZERO( &ri->ri_realsuffix );
			ri->ri_bd = NULL;
			return 0;
		}
		return 1;

	} else {
		BackendDB *bd;

		assert( ri != NULL );
		assert( BER_BVISNULL( &ri->ri_realsuffix ) );

		if ( c->be->be_nsuffix == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"\"relay\" directive "
				"must appear after \"suffix\"" );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			rc = 1;
			goto relay_done;
		}

		if ( !BER_BVISNULL( &c->be->be_nsuffix[ 1 ] ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"relaying of multiple suffix "
				"database not supported" );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			rc = 1;
			goto relay_done;
		}

		bd = select_backend( &c->value_ndn, 1 );
		if ( bd == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"cannot find database "
				"of relay dn \"%s\" "
				"in \"olcRelay <dn>\"\n",
				c->value_dn.bv_val );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			rc = 1;
			goto relay_done;

		} else if ( bd->be_private == c->be->be_private ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"relay dn \"%s\" would call self "
				"in \"relay <dn>\" line\n",
				c->value_dn.bv_val );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			rc = 1;
			goto relay_done;
		}

		ri->ri_realsuffix = c->value_ndn;
		BER_BVZERO( &c->value_ndn );

relay_done:;
		ch_free( c->value_dn.bv_val );
		ch_free( c->value_ndn.bv_val );
	}

	return rc;
}

int
relay_back_initialize( BackendInfo *bi )
{
	bi->bi_init = 0;
	bi->bi_open = 0;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = relay_back_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = relay_back_db_open;
#if 0
	bi->bi_db_close = relay_back_db_close;
#endif
	bi->bi_db_destroy = relay_back_db_destroy;

	bi->bi_op_bind = relay_back_op_bind;
	bi->bi_op_unbind = relay_back_op_unbind;
	bi->bi_op_search = relay_back_op_search;
	bi->bi_op_compare = relay_back_op_compare;
	bi->bi_op_modify = relay_back_op_modify;
	bi->bi_op_modrdn = relay_back_op_modrdn;
	bi->bi_op_add = relay_back_op_add;
	bi->bi_op_delete = relay_back_op_delete;
	bi->bi_op_abandon = relay_back_op_abandon;
	bi->bi_op_cancel = relay_back_op_cancel;
	bi->bi_extended = relay_back_op_extended;
	bi->bi_entry_release_rw = relay_back_entry_release_rw;
	bi->bi_entry_get_rw = relay_back_entry_get_rw;
#if 0	/* see comment in op.c */
	bi->bi_chk_referrals = relay_back_chk_referrals;
#endif
	bi->bi_operational = relay_back_operational;
	bi->bi_has_subordinates = relay_back_has_subordinates;

	bi->bi_connection_init = relay_back_connection_init;
	bi->bi_connection_destroy = relay_back_connection_destroy;

	bi->bi_cf_ocs = relayocs;

	return config_register_schema( relaycfg, relayocs );
}

int
relay_back_db_init( Backend *be, ConfigReply *cr)
{
	relay_back_info		*ri;

	be->be_private = NULL;

	ri = (relay_back_info *)ch_calloc( 1, sizeof( relay_back_info ) );
	if ( ri == NULL ) {
 		return -1;
 	}

	ri->ri_bd = NULL;
	BER_BVZERO( &ri->ri_realsuffix );
	ri->ri_massage = 0;

	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	be->be_private = (void *)ri;

	return 0;
}

int
relay_back_db_open( Backend *be, ConfigReply *cr )
{
	relay_back_info		*ri = (relay_back_info *)be->be_private;

	assert( ri != NULL );

	if ( !BER_BVISNULL( &ri->ri_realsuffix ) ) {
		ri->ri_bd = select_backend( &ri->ri_realsuffix, 1 );

		/* must be there: it was during config! */
		assert( ri->ri_bd != NULL );

		/* inherit controls */
		AC_MEMCPY( be->be_ctrls, ri->ri_bd->be_ctrls, sizeof( be->be_ctrls ) );

	} else {
		/* inherit all? */
		AC_MEMCPY( be->be_ctrls, frontendDB->be_ctrls, sizeof( be->be_ctrls ) );
	}

	return 0;
}

int
relay_back_db_close( Backend *be, ConfigReply *cr )
{
	return 0;
}

int
relay_back_db_destroy( Backend *be, ConfigReply *cr)
{
	relay_back_info		*ri = (relay_back_info *)be->be_private;

	if ( ri ) {
		if ( !BER_BVISNULL( &ri->ri_realsuffix ) ) {
			ch_free( ri->ri_realsuffix.bv_val );
		}
		ch_free( ri );
	}

	return 0;
}

#if SLAPD_RELAY == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( relay )

#endif /* SLAPD_RELAY == SLAPD_MOD_DYNAMIC */

