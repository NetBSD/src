/*	$NetBSD: slapd-modrdn.c,v 1.3 2021/08/14 16:15:03 christos Exp $	*/

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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Howard Chu, based in part
 * on other OpenLDAP test tools, for inclusion in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: slapd-modrdn.c,v 1.3 2021/08/14 16:15:03 christos Exp $");

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
#define RETRIES	0

static void
do_modrdn( struct tester_conn_args *config,
		char *entry, int friendly );

static void
usage( char *name, char opt )
{
	if ( opt ) {
		fprintf( stderr, "%s: unable to handle option \'%c\'\n\n",
			name, opt );
	}

	fprintf( stderr, "usage: %s " TESTER_COMMON_HELP
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
	int		friendly = 0;
	struct tester_conn_args	*config;

	config = tester_init( "slapd-modrdn", TESTER_MODRDN );

	while ( ( i = getopt( argc, argv, TESTER_COMMON_OPTS "e:F" ) ) != EOF )
	{
		switch ( i ) {
		case 'F':
			friendly++;
			break;

		case 'i':
			/* ignored (!) by now */
			break;

		case 'e':		/* entry to rename */
			entry = optarg;
			break;

		default:
			if ( tester_config_opt( config, i, optarg ) == LDAP_SUCCESS ) {
				break;
			}
			usage( argv[0], i );
			break;
		}
	}

	if ( entry == NULL )
		usage( argv[0], 0 );

	if ( *entry == '\0' ) {

		fprintf( stderr, "%s: invalid EMPTY entry DN.\n",
				argv[0] );
		exit( EXIT_FAILURE );

	}

	tester_config_finish( config );

	for ( i = 0; i < config->outerloops; i++ ) {
		do_modrdn( config, entry, friendly );
	}

	exit( EXIT_SUCCESS );
}


static void
do_modrdn( struct tester_conn_args *config,
	char *entry, int friendly )
{
	LDAP	*ld = NULL;
	int  	i, do_retry = config->retries;
	char	*DNs[2];
	char	*rdns[2];
	int	rc = LDAP_SUCCESS;
	char	*p1, *p2;

	DNs[0] = entry;
	DNs[1] = strdup( entry );
	if ( DNs[1] == NULL ) {
		tester_error( "strdup failed" );
		exit( EXIT_FAILURE );
	}

	/* reverse the RDN, make new DN */
	p1 = strchr( entry, '=' ) + 1;
	p2 = strchr( p1, ',' );

	*p2 = '\0';
	rdns[1] = strdup( entry );
	if ( rdns[1] == NULL ) {
		tester_error( "strdup failed" );
		exit( EXIT_FAILURE );
	}
	*p2-- = ',';

	for (i = p1 - entry;p2 >= p1;)
		DNs[1][i++] = *p2--;
	
	DNs[1][i] = '\0';
	rdns[0] = strdup( DNs[1] );
	if ( rdns[0] == NULL ) {
		tester_error( "strdup failed" );
		exit( EXIT_FAILURE );
	}
	DNs[1][i] = ',';

	i = 0;

retry:;
	if ( ld == NULL ) {
		tester_init_ld( &ld, config, 0 );
	}

	if ( do_retry == config->retries ) {
		fprintf( stderr, "PID=%ld - Modrdn(%d): entry=\"%s\".\n",
			(long) pid, config->loops, entry );
	}

	for ( ; i < config->loops; i++ ) {
		rc = ldap_rename_s( ld, DNs[0], rdns[0], NULL, 0, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_rename_s", NULL );
			switch ( rc ) {
			case LDAP_NO_SUCH_OBJECT:
				/* NOTE: this likely means
				 * the second modrdn failed
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
		rc = ldap_rename_s( ld, DNs[1], rdns[1], NULL, 1, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_rename_s", NULL );
			switch ( rc ) {
			case LDAP_NO_SUCH_OBJECT:
				/* NOTE: this likely means
				 * the first modrdn failed
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
	fprintf( stderr, "  PID=%ld - Modrdn done (%d).\n", (long) pid, rc );

	ldap_unbind_ext( ld, NULL, NULL );

	free( DNs[1] );
	free( rdns[0] );
	free( rdns[1] );
}
