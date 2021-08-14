/*	$NetBSD: slapd-read.c,v 1.3 2021/08/14 16:15:03 christos Exp $	*/

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
 * This work was initially developed by Kurt Spanier for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: slapd-read.c,v 1.3 2021/08/14 16:15:03 christos Exp $");

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

#include "ldap_pvt.h"

#include "slapd-common.h"

#define LOOPS	100
#define RETRIES	0

static void
do_read( struct tester_conn_args *config, char *entry, LDAP **ld,
	char **attrs, int noattrs, int nobind, int maxloop, int force );

static void
do_random( struct tester_conn_args *config, char *sbase,
	char *filter, char **attrs, int noattrs, int nobind, int force );

static void
usage( char *name, int opt )
{
	if ( opt ) {
		fprintf( stderr, "%s: unable to handle option \'%c\'\n\n",
			name, opt );
	}

	fprintf( stderr, "usage: %s " TESTER_COMMON_HELP
		"-e <entry> "
		"[-A] "
		"[-F] "
		"[-N] "
		"[-S[S[S]]] "
		"[-f filter] "
		"[-T <attrs>] "
		"[<attrs>] "
		"\n",
		name );
	exit( EXIT_FAILURE );
}

/* -S: just send requests without reading responses
 * -SS: send all requests asynchronous and immediately start reading responses
 * -SSS: send all requests asynchronous; then read responses
 */
static int swamp;

int
main( int argc, char **argv )
{
	int		i;
	char		*entry = NULL;
	char		*filter  = NULL;
	int		force = 0;
	char		*srchattrs[] = { "1.1", NULL };
	char		**attrs = srchattrs;
	int		noattrs = 0;
	int		nobind = 0;
	struct tester_conn_args	*config;

	config = tester_init( "slapd-read", TESTER_READ );

	/* by default, tolerate referrals and no such object */
	tester_ignore_str2errlist( "REFERRAL,NO_SUCH_OBJECT" );

	while ( (i = getopt( argc, argv, TESTER_COMMON_OPTS "Ae:Ff:NST:" )) != EOF ) {
		switch ( i ) {
		case 'A':
			noattrs++;
			break;

		case 'N':
			nobind = TESTER_INIT_ONLY;
			break;

		case 'e':		/* DN to search for */
			entry = optarg;
			break;

		case 'f':		/* the search request */
			filter = optarg;
			break;

		case 'F':
			force++;
			break;

		case 'S':
			swamp++;
			break;

		case 'T':
			attrs = ldap_str2charray( optarg, "," );
			if ( attrs == NULL ) {
				usage( argv[0], i );
			}
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

	if ( argv[optind] != NULL ) {
		attrs = &argv[optind];
	}

	tester_config_finish( config );

	for ( i = 0; i < config->outerloops; i++ ) {
		if ( filter != NULL ) {
			do_random( config, entry, filter, attrs,
				noattrs, nobind, force );

		} else {
			do_read( config, entry, NULL, attrs,
				noattrs, nobind, config->loops, force );
		}
	}

	exit( EXIT_SUCCESS );
}

static void
do_random( struct tester_conn_args *config, char *sbase, char *filter,
	char **srchattrs, int noattrs, int nobind, int force )
{
	LDAP	*ld = NULL;
	int  	i = 0, do_retry = config->retries;
	char	*attrs[ 2 ];
	int     rc = LDAP_SUCCESS;
	int	nvalues = 0;
	char	**values = NULL;
	LDAPMessage *res = NULL, *e = NULL;

	attrs[ 0 ] = LDAP_NO_ATTRS;
	attrs[ 1 ] = NULL;

	tester_init_ld( &ld, config, nobind );

	if ( do_retry == config->retries ) {
		fprintf( stderr, "PID=%ld - Read(%d): base=\"%s\", filter=\"%s\".\n",
				(long) pid, config->loops, sbase, filter );
	}

	rc = ldap_search_ext_s( ld, sbase, LDAP_SCOPE_SUBTREE,
		filter, attrs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &res );
	switch ( rc ) {
	case LDAP_SIZELIMIT_EXCEEDED:
	case LDAP_TIMELIMIT_EXCEEDED:
	case LDAP_SUCCESS:
		nvalues = ldap_count_entries( ld, res );
		if ( nvalues == 0 ) {
			if ( rc ) {
				tester_ldap_error( ld, "ldap_search_ext_s", NULL );
			}
			break;
		}

		values = malloc( ( nvalues + 1 ) * sizeof( char * ) );
		if ( !values ) {
			tester_error( "malloc failed" );
			exit( EXIT_FAILURE );
		}
		for ( i = 0, e = ldap_first_entry( ld, res ); e != NULL; i++, e = ldap_next_entry( ld, e ) )
		{
			values[ i ] = ldap_get_dn( ld, e );
		}
		values[ i ] = NULL;

		ldap_msgfree( res );

		if ( do_retry == config->retries ) {
			fprintf( stderr, "  PID=%ld - Read base=\"%s\" filter=\"%s\" got %d values.\n",
				(long) pid, sbase, filter, nvalues );
		}

		for ( i = 0; i < config->loops; i++ ) {
#if 0	/* use high-order bits for better randomness (Numerical Recipes in "C") */
			int	r = rand() % nvalues;
#endif
			int	r = ((double)nvalues)*rand()/(RAND_MAX + 1.0);

			do_read( config, values[ r ], &ld,
				srchattrs, noattrs, nobind, 1, force );
		}
		free( values );
		break;

	default:
		tester_ldap_error( ld, "ldap_search_ext_s", NULL );
		break;
	}

	fprintf( stderr, "  PID=%ld - Read done (%d).\n", (long) pid, rc );

	if ( ld != NULL ) {
		ldap_unbind_ext( ld, NULL, NULL );
	}
}

static void
do_read( struct tester_conn_args *config, char *entry, LDAP **ldp,
	char **attrs, int noattrs, int nobind, int maxloop, int force )
{
	LDAP	*ld = ldp ? *ldp : NULL;
	int  	i = 0, do_retry = config->retries;
	int     rc = LDAP_SUCCESS;
	int		*msgids = NULL, active = 0;

	/* make room for msgid */
	if ( swamp > 1 ) {
		msgids = (int *)calloc( sizeof(int), maxloop );
		if ( !msgids ) {
			tester_error( "calloc failed" );
			exit( EXIT_FAILURE );
		}
	}

retry:;
	if ( ld == NULL ) {
		tester_init_ld( &ld, config, nobind );
	}

	if ( do_retry == config->retries ) {
		fprintf( stderr, "PID=%ld - Read(%d): entry=\"%s\".\n",
			(long) pid, maxloop, entry );
	}

	if ( swamp > 1 ) {
		do {
			LDAPMessage *res = NULL;
			int j, msgid;

			if ( i < maxloop ) {
				rc = ldap_search_ext( ld, entry, LDAP_SCOPE_BASE,
						NULL, attrs, noattrs, NULL, NULL,
						NULL, LDAP_NO_LIMIT, &msgids[i] );

				active++;
#if 0
				fprintf( stderr,
					">>> PID=%ld - Read maxloop=%d cnt=%d active=%d msgid=%d: "
					"entry=\"%s\"\n",
					(long) pid, maxloop, i, active, msgids[i],
					entry );
#endif
				i++;

				if ( rc ) {
					char buf[BUFSIZ];
					int first = tester_ignore_err( rc );
					/* if ignore.. */
					if ( first ) {
						/* only log if first occurrence */
						if ( ( force < 2 && first > 0 ) || abs(first) == 1 ) {
							tester_ldap_error( ld, "ldap_search_ext", NULL );
						}
						continue;
					}
		
					/* busy needs special handling */
					snprintf( buf, sizeof( buf ), "entry=\"%s\"\n", entry );
					tester_ldap_error( ld, "ldap_search_ext", buf );
					if ( rc == LDAP_BUSY && do_retry > 0 ) {
						ldap_unbind_ext( ld, NULL, NULL );
						ld = NULL;
						do_retry--;
						goto retry;
					}
					break;
				}

				if ( swamp > 2 ) {
					continue;
				}
			}

			rc = ldap_result( ld, LDAP_RES_ANY, 0, NULL, &res );
			switch ( rc ) {
			case -1:
				/* gone really bad */
#if 0
				fprintf( stderr,
					">>> PID=%ld - Read maxloop=%d cnt=%d active=%d: "
					"entry=\"%s\" ldap_result()=%d\n",
					(long) pid, maxloop, i, active, entry, rc );
#endif
				goto cleanup;
	
			case 0:
				/* timeout (impossible) */
				break;
	
			case LDAP_RES_SEARCH_ENTRY:
			case LDAP_RES_SEARCH_REFERENCE:
				/* ignore */
				break;
	
			case LDAP_RES_SEARCH_RESULT:
				/* just remove, no error checking (TODO?) */
				msgid = ldap_msgid( res );
				ldap_parse_result( ld, res, &rc, NULL, NULL, NULL, NULL, 1 );
				res = NULL;

				/* linear search, bah */
				for ( j = 0; j < i; j++ ) {
					if ( msgids[ j ] == msgid ) {
						msgids[ j ] = -1;
						active--;
#if 0
						fprintf( stderr,
							"<<< PID=%ld - ReadDone maxloop=%d cnt=%d active=%d msgid=%d: "
							"entry=\"%s\"\n",
							(long) pid, maxloop, j, active, msgid, entry );
#endif
						break;
					}
				}
				break;

			default:
				/* other messages unexpected */
				fprintf( stderr,
					"### PID=%ld - Read(%d): "
					"entry=\"%s\" attrs=%s%s. unexpected response tag=%d\n",
					(long) pid, maxloop,
					entry, attrs[0], attrs[1] ? " (more...)" : "", rc );
				break;
			}

			if ( res != NULL ) {
				ldap_msgfree( res );
			}
		} while ( i < maxloop || active > 0 );

	} else {
		for ( ; i < maxloop; i++ ) {
			LDAPMessage *res = NULL;

			if (swamp) {
				int msgid;
				rc = ldap_search_ext( ld, entry, LDAP_SCOPE_BASE,
						NULL, attrs, noattrs, NULL, NULL,
						NULL, LDAP_NO_LIMIT, &msgid );
				if ( rc == LDAP_SUCCESS ) continue;
				else break;
			}
	
			rc = ldap_search_ext_s( ld, entry, LDAP_SCOPE_BASE,
					NULL, attrs, noattrs, NULL, NULL, NULL,
					LDAP_NO_LIMIT, &res );
			if ( res != NULL ) {
				ldap_msgfree( res );
			}

			if ( rc ) {
				int		first = tester_ignore_err( rc );
				char		buf[ BUFSIZ ];
	
				snprintf( buf, sizeof( buf ), "ldap_search_ext_s(%s)", entry );
	
				/* if ignore.. */
				if ( first ) {
					/* only log if first occurrence */
					if ( ( force < 2 && first > 0 ) || abs(first) == 1 ) {
						tester_ldap_error( ld, buf, NULL );
					}
					continue;
				}
	
				/* busy needs special handling */
				tester_ldap_error( ld, buf, NULL );
				if ( rc == LDAP_BUSY && do_retry > 0 ) {
					ldap_unbind_ext( ld, NULL, NULL );
					ld = NULL;
					do_retry--;
					goto retry;
				}
				break;
			}
		}
	}

cleanup:;
	if ( msgids != NULL ) {
		free( msgids );
	}

	if ( ldp != NULL ) {
		*ldp = ld;

	} else {
		fprintf( stderr, "  PID=%ld - Read done (%d).\n", (long) pid, rc );

		if ( ld != NULL ) {
			ldap_unbind_ext( ld, NULL, NULL );
		}
	}
}

