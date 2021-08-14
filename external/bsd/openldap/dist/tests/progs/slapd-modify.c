/*	$NetBSD: slapd-modify.c,v 1.3 2021/08/14 16:15:03 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2021 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: slapd-modify.c,v 1.3 2021/08/14 16:15:03 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include "ac/stdlib.h"

#include "ac/ctype.h"
#include "ac/param.h"
#include "ac/socket.h"
#include "ac/string.h"
#include "ac/unistd.h"
#include "ac/wait.h"

#include "ldap.h"
#include "lutil.h"

#include "slapd-common.h"

#define LOOPS	100

static void
do_modify( struct tester_conn_args *config, char *entry,
		char *attr, char *value, int friendly );


static void
usage( char *name, int opt )
{
	if ( opt ) {
		fprintf( stderr, "%s: unable to handle option \'%c\'\n\n",
			name, opt );
	}

	fprintf( stderr, "usage: %s " TESTER_COMMON_HELP
		"-a <attr:val> "
		"-e <entry> "
		"[-F]\n",
		name );
	exit( EXIT_FAILURE );
}

int
main( int argc, char **argv )
{
	int		i;
	char		*entry = NULL;
	char		*ava = NULL;
	char		*value = NULL;
	int		friendly = 0;
	struct tester_conn_args	*config;

	config = tester_init( "slapd-modify", TESTER_MODIFY );

	while ( ( i = getopt( argc, argv, TESTER_COMMON_OPTS "a:e:F" ) ) != EOF )
	{
		switch ( i ) {
		case 'F':
			friendly++;
			break;

		case 'i':
			/* ignored (!) by now */
			break;

		case 'e':		/* entry to modify */
			entry = optarg;
			break;

		case 'a':
			ava = optarg;
			break;

		default:
			if ( tester_config_opt( config, i, optarg ) == LDAP_SUCCESS ) {
				break;
			}
			usage( argv[0], i );
			break;
		}
	}

	if (( entry == NULL ) || ( ava == NULL ))
		usage( argv[0], 0 );

	if ( *entry == '\0' ) {

		fprintf( stderr, "%s: invalid EMPTY entry DN.\n",
				argv[0] );
		exit( EXIT_FAILURE );

	}
	if ( *ava  == '\0' ) {
		fprintf( stderr, "%s: invalid EMPTY AVA.\n",
				argv[0] );
		exit( EXIT_FAILURE );
	}
	
	if ( !( value = strchr( ava, ':' ))) {
		fprintf( stderr, "%s: invalid AVA.\n",
				argv[0] );
		exit( EXIT_FAILURE );
	}
	*value++ = '\0'; 
	while ( *value && isspace( (unsigned char) *value ))
		value++;

	tester_config_finish( config );

	for ( i = 0; i < config->outerloops; i++ ) {
		do_modify( config, entry, ava, value, friendly );
	}

	exit( EXIT_SUCCESS );
}


static void
do_modify( struct tester_conn_args *config,
	char *entry, char* attr, char* value, int friendly )
{
	LDAP	*ld = NULL;
	int  	i = 0, do_retry = config->retries;
	int     rc = LDAP_SUCCESS;

	struct ldapmod mod;
	struct ldapmod *mods[2];
	char *values[2];

	values[0] = value;
	values[1] = NULL;
	mod.mod_op = LDAP_MOD_ADD;
	mod.mod_type = attr;
	mod.mod_values = values;
	mods[0] = &mod;
	mods[1] = NULL;

retry:;
	if ( ld == NULL ) {
		tester_init_ld( &ld, config, 0 );
	}

	if ( do_retry == config->retries ) {
		fprintf( stderr, "PID=%ld - Modify(%d): entry=\"%s\".\n",
			(long) pid, config->loops, entry );
	}

	for ( ; i < config->loops; i++ ) {
		mod.mod_op = LDAP_MOD_ADD;
		rc = ldap_modify_ext_s( ld, entry, mods, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_modify_ext_s", NULL );
			switch ( rc ) {
			case LDAP_TYPE_OR_VALUE_EXISTS:
				/* NOTE: this likely means
				 * the second modify failed
				 * during the previous round... */
				if ( !friendly ) {
					goto done;
				}
				break;

			case LDAP_BUSY:
			case LDAP_UNAVAILABLE:
				if ( do_retry > 0 ) {
					do_retry--;
					goto retry;
				}
				/* fall thru */

			default:
				goto done;
			}
		}
		
		mod.mod_op = LDAP_MOD_DELETE;
		rc = ldap_modify_ext_s( ld, entry, mods, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_modify_ext_s", NULL );
			switch ( rc ) {
			case LDAP_NO_SUCH_ATTRIBUTE:
				/* NOTE: this likely means
				 * the first modify failed
				 * during the previous round... */
				if ( !friendly ) {
					goto done;
				}
				break;

			case LDAP_BUSY:
			case LDAP_UNAVAILABLE:
				if ( do_retry > 0 ) {
					do_retry--;
					goto retry;
				}
				/* fall thru */

			default:
				goto done;
			}
		}

	}

done:;
	fprintf( stderr, "  PID=%ld - Modify done (%d).\n", (long) pid, rc );

	ldap_unbind_ext( ld, NULL, NULL );
}


