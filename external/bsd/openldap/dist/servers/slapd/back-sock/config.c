/* config.c - sock backend configuration file routine */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-sock/config.c,v 1.5.2.1 2008/02/09 00:46:09 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2008 The OpenLDAP Foundation.
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
 * This work was initially developed by Brian Candler for inclusion
 * in OpenLDAP Software. Dynamic config support by Howard Chu.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "back-sock.h"

static ConfigDriver bs_cf_gen;

enum {
	BS_EXT = 1
};

static ConfigTable bscfg[] = {
	{ "socketpath", "pathname", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct sockinfo, si_sockpath),
		"( OLcfgDbAt:7.1 NAME 'olcDbSocketPath' "
			"DESC 'Pathname for Unix domain socket' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "extensions", "ext", 2, 0, 0, ARG_MAGIC|BS_EXT,
		bs_cf_gen, "( OLcfgDbAt:7.2 NAME 'olcDbSocketExtensions' "
			"DESC 'binddn, peername, or ssf' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL }
};

static ConfigOCs bsocs[] = {
	{ "( OLcfgDbOc:7.1 "
		"NAME 'olcDbSocketConfig' "
		"DESC 'Socket backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST olcDbSocketPath "
		"MAY olcDbSocketExtensions )",
			Cft_Database, bscfg },
	{ NULL, 0, NULL }
};

static slap_verbmasks bs_exts[] = {
	{ BER_BVC("binddn"), SOCK_EXT_BINDDN },
	{ BER_BVC("peername"), SOCK_EXT_PEERNAME },
	{ BER_BVC("ssf"), SOCK_EXT_SSF },
	{ BER_BVNULL, 0 }
};

static int
bs_cf_gen( ConfigArgs *c )
{
	struct sockinfo	*si = c->be->be_private;
	int rc;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case BS_EXT:
			return mask_to_verbs( bs_exts, si->si_extensions, &c->rvalue_vals );
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case BS_EXT:
			if ( c->valx < 0 ) {
				si->si_extensions = 0;
				rc = 0;
			} else {
				slap_mask_t dels = 0;
				rc = verbs_to_mask( c->argc, c->argv, bs_exts, &dels );
				if ( rc == 0 )
					si->si_extensions ^= dels;
			}
			return rc;
		}

	} else {
		switch( c->type ) {
		case BS_EXT:
			return verbs_to_mask( c->argc, c->argv, bs_exts, &si->si_extensions );
		}
	}
	return 1;
}

int
sock_back_init_cf( BackendInfo *bi )
{
	bi->bi_cf_ocs = bsocs;

	return config_register_schema( bscfg, bsocs );
}
