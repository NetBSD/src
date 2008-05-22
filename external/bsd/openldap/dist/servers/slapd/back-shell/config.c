/* config.c - shell backend configuration file routine */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-shell/config.c,v 1.18.2.3 2008/02/11 23:26:47 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "shell.h"

int
shell_back_db_config(
    BackendDB	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	struct shellinfo	*si = (struct shellinfo *) be->be_private;

	if ( si == NULL ) {
		fprintf( stderr, "%s: line %d: shell backend info is null!\n",
		    fname, lineno );
		return( 1 );
	}

	/* command + args to exec for binds */
	if ( strcasecmp( argv[0], "bind" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"bind <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_bind = ldap_charray_dup( &argv[1] );

	/* command + args to exec for unbinds */
	} else if ( strcasecmp( argv[0], "unbind" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"unbind <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_unbind = ldap_charray_dup( &argv[1] );

	/* command + args to exec for searches */
	} else if ( strcasecmp( argv[0], "search" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"search <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_search = ldap_charray_dup( &argv[1] );

	/* command + args to exec for compares */
	} else if ( strcasecmp( argv[0], "compare" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"compare <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_compare = ldap_charray_dup( &argv[1] );

	/* command + args to exec for modifies */
	} else if ( strcasecmp( argv[0], "modify" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"modify <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_modify = ldap_charray_dup( &argv[1] );

	/* command + args to exec for modrdn */
	} else if ( strcasecmp( argv[0], "modrdn" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"modrdn <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_modrdn = ldap_charray_dup( &argv[1] );

	/* command + args to exec for add */
	} else if ( strcasecmp( argv[0], "add" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"add <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_add = ldap_charray_dup( &argv[1] );

	/* command + args to exec for delete */
	} else if ( strcasecmp( argv[0], "delete" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
	"%s: line %d: missing executable in \"delete <executable>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		si->si_delete = ldap_charray_dup( &argv[1] );

	/* anything else */
	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return 0;
}
