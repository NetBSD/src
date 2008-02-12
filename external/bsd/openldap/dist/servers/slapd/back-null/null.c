/* null.c - the null backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-null/null.c,v 1.18.2.5 2008/02/12 00:58:15 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2008 The OpenLDAP Foundation.
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
 * This work was originally developed by Hallvard Furuseth for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "config.h"

struct null_info {
	int	ni_bind_allowed;
	ID	ni_nextid;
};


/* LDAP operations */

static int
null_back_bind( Operation *op, SlapReply *rs )
{
	struct null_info *ni = (struct null_info *) op->o_bd->be_private;

	if ( ni->ni_bind_allowed || be_isroot_pw( op ) ) {
		/* front end will send result on success (0) */
		return LDAP_SUCCESS;
	}

	rs->sr_err = LDAP_INVALID_CREDENTIALS;
	send_ldap_result( op, rs );

	return rs->sr_err;
}

/* add, delete, modify, modrdn, search */
static int
null_back_success( Operation *op, SlapReply *rs )
{
	rs->sr_err = LDAP_SUCCESS;
	send_ldap_result( op, rs );
	return 0;
}

/* compare */
static int
null_back_false( Operation *op, SlapReply *rs )
{
	rs->sr_err = LDAP_COMPARE_FALSE;
	send_ldap_result( op, rs );
	return 0;
}


/* for overlays */
static int
null_back_entry_get(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **ent )
{
	assert( *ent == NULL );

	/* don't admit the object isn't there */
	return oc || at ? LDAP_NO_SUCH_ATTRIBUTE : LDAP_BUSY;
}


/* Slap tools */

static int
null_tool_entry_open( BackendDB *be, int mode )
{
	return 0;
}

static int
null_tool_entry_close( BackendDB *be )
{
	assert( be != NULL );
	return 0;
}

static ID
null_tool_entry_next( BackendDB *be )
{
	return NOID;
}

static Entry *
null_tool_entry_get( BackendDB *be, ID id )
{
	assert( slapMode & SLAP_TOOL_MODE );
	return NULL;
}

static ID
null_tool_entry_put( BackendDB *be, Entry *e, struct berval *text )
{
	assert( slapMode & SLAP_TOOL_MODE );
	assert( text != NULL );
	assert( text->bv_val != NULL );
	assert( text->bv_val[0] == '\0' );	/* overconservative? */

	e->e_id = ((struct null_info *) be->be_private)->ni_nextid++;
	return e->e_id;
}


/* Setup */

static int
null_back_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	struct null_info *ni = (struct null_info *) be->be_private;

	if ( ni == NULL ) {
		fprintf( stderr, "%s: line %d: null database info is null!\n",
			fname, lineno );
		return 1;
	}

	/* bind requests allowed */
	if ( strcasecmp( argv[0], "bind" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing <on/off> in \"bind <on/off>\" line\n",
			         fname, lineno );
			return 1;
		}
		ni->ni_bind_allowed = strcasecmp( argv[1], "off" );

	/* anything else */
	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return 0;
}

static int
null_back_db_init( BackendDB *be, ConfigReply *cr )
{
	struct null_info *ni = ch_calloc( 1, sizeof(struct null_info) );
	ni->ni_bind_allowed = 0;
	ni->ni_nextid = 1;
	be->be_private = ni;
	return 0;
}

static int
null_back_db_destroy( Backend *be, ConfigReply *cr )
{
	free( be->be_private );
	return 0;
}


int
null_back_initialize( BackendInfo *bi )
{
	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = null_back_db_init;
	bi->bi_db_config = null_back_db_config;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = null_back_db_destroy;

	bi->bi_op_bind = null_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = null_back_success;
	bi->bi_op_compare = null_back_false;
	bi->bi_op_modify = null_back_success;
	bi->bi_op_modrdn = null_back_success;
	bi->bi_op_add = null_back_success;
	bi->bi_op_delete = null_back_success;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_entry_get_rw = null_back_entry_get;

	bi->bi_tool_entry_open = null_tool_entry_open;
	bi->bi_tool_entry_close = null_tool_entry_close;
	bi->bi_tool_entry_first = null_tool_entry_next;
	bi->bi_tool_entry_next = null_tool_entry_next;
	bi->bi_tool_entry_get = null_tool_entry_get;
	bi->bi_tool_entry_put = null_tool_entry_put;

	return 0;
}

#if SLAPD_NULL == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( null )

#endif /* SLAPD_NULL == SLAPD_MOD_DYNAMIC */
