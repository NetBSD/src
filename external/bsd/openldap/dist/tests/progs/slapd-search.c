/*	$NetBSD: slapd-search.c,v 1.3 2021/08/14 16:15:03 christos Exp $	*/

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
__RCSID("$NetBSD: slapd-search.c,v 1.3 2021/08/14 16:15:03 christos Exp $");

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
do_search( struct tester_conn_args *config,
	char *sbase, int scope, char *filter, LDAP **ldp,
	char **attrs, int noattrs, int nobind,
	int innerloop, int force );

static void
do_random( struct tester_conn_args *config,
	char *sbase, int scope, char *filter, char *attr,
	char **attrs, int noattrs, int nobind, int force );

static void
usage( char *name, char opt )
{
	if ( opt != '\0' ) {
		fprintf( stderr, "unknown/incorrect option \"%c\"\n", opt );
	}

	fprintf( stderr, "usage: %s " TESTER_COMMON_HELP
		"-b <searchbase> "
		"-s <scope> "
		"-f <searchfilter> "
		"[-a <attr>] "
		"[-A] "
		"[-F] "
		"[-N] "
		"[-S[S[S]]] "
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
	char		*sbase = NULL;
	int		scope = LDAP_SCOPE_SUBTREE;
	char		*filter  = NULL;
	char		*attr = NULL;
	char		*srchattrs[] = { "cn", "sn", NULL };
	char		**attrs = srchattrs;
	int		force = 0;
	int		noattrs = 0;
	int		nobind = 0;
	struct tester_conn_args	*config;

	config = tester_init( "slapd-search", TESTER_SEARCH );

	/* by default, tolerate referrals and no such object */
	tester_ignore_str2errlist( "REFERRAL,NO_SUCH_OBJECT" );

	while ( ( i = getopt( argc, argv, TESTER_COMMON_OPTS "Aa:b:f:FNSs:T:" ) ) != EOF )
	{
		switch ( i ) {
		case 'A':
			noattrs++;
			break;

		case 'N':
			nobind = TESTER_INIT_ONLY;
			break;

		case 'a':
			attr = optarg;
			break;

		case 'b':		/* file with search base */
			sbase = optarg;
			break;

		case 'f':		/* the search request */
			filter = optarg;
			break;

		case 'F':
			force++;
			break;

		case 'T':
			attrs = ldap_str2charray( optarg, "," );
			if ( attrs == NULL ) {
				usage( argv[0], i );
			}
			break;

		case 'S':
			swamp++;
			break;

		case 's':
			scope = ldap_pvt_str2scope( optarg );
			if ( scope == -1 ) {
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

	if (( sbase == NULL ) || ( filter == NULL ))
		usage( argv[0], 0 );

	if ( *filter == '\0' ) {

		fprintf( stderr, "%s: invalid EMPTY search filter.\n",
				argv[0] );
		exit( EXIT_FAILURE );

	}

	if ( argv[optind] != NULL ) {
		attrs = &argv[optind];
	}

	tester_config_finish( config );

	for ( i = 0; i < config->outerloops; i++ ) {
		if ( attr != NULL ) {
			do_random( config,
				sbase, scope, filter, attr,
				attrs, noattrs, nobind, force );

		} else {
			do_search( config, sbase, scope, filter,
				NULL, attrs, noattrs, nobind,
				config->loops, force );
		}
	}

	exit( EXIT_SUCCESS );
}


static void
do_random( struct tester_conn_args *config,
	char *sbase, int scope, char *filter, char *attr,
	char **srchattrs, int noattrs, int nobind, int force )
{
	LDAP	*ld = NULL;
	int  	i = 0, do_retry = config->retries;
	char	*attrs[ 2 ];
	int     rc = LDAP_SUCCESS;
	int	nvalues = 0;
	char	**values = NULL;
	LDAPMessage *res = NULL, *e = NULL;

	attrs[ 0 ] = attr;
	attrs[ 1 ] = NULL;

	tester_init_ld( &ld, config, nobind );

	rc = ldap_search_ext_s( ld, sbase, LDAP_SCOPE_SUBTREE,
		filter, attrs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &res );
	switch ( rc ) {
	case LDAP_SIZELIMIT_EXCEEDED:
	case LDAP_TIMELIMIT_EXCEEDED:
	case LDAP_SUCCESS:
		if ( ldap_count_entries( ld, res ) == 0 ) {
			if ( rc ) {
				tester_ldap_error( ld, "ldap_search_ext_s", NULL );
			}
			break;
		}

		for ( e = ldap_first_entry( ld, res ); e != NULL; e = ldap_next_entry( ld, e ) )
		{
			struct berval **v = ldap_get_values_len( ld, e, attr );

			if ( v != NULL ) {
				int n = ldap_count_values_len( v );
				int j;

				values = realloc( values, ( nvalues + n + 1 )*sizeof( char * ) );
				if ( !values ) {
					tester_error( "realloc failed" );
					exit( EXIT_FAILURE );
				}
				for ( j = 0; j < n; j++ ) {
					values[ nvalues + j ] = strdup( v[ j ]->bv_val );
				}
				values[ nvalues + j ] = NULL;
				nvalues += n;
				ldap_value_free_len( v );
			}
		}

		ldap_msgfree( res );

		if ( !values ) {
			fprintf( stderr, "  PID=%ld - Search base=\"%s\" filter=\"%s\" got %d values.\n",
				(long) pid, sbase, filter, nvalues );
			exit(EXIT_FAILURE);
		}

		if ( do_retry == config->retries ) {
			fprintf( stderr, "  PID=%ld - Search base=\"%s\" filter=\"%s\" got %d values.\n",
				(long) pid, sbase, filter, nvalues );
		}

		for ( i = 0; i < config->loops; i++ ) {
			char	buf[ BUFSIZ ];
#if 0	/* use high-order bits for better randomness (Numerical Recipes in "C") */
			int	r = rand() % nvalues;
#endif
			int	r = ((double)nvalues)*rand()/(RAND_MAX + 1.0);

			snprintf( buf, sizeof( buf ), "(%s=%s)", attr, values[ r ] );

			do_search( config,
				sbase, scope, buf, &ld,
				srchattrs, noattrs, nobind,
				1, force );
		}
		break;

	default:
		tester_ldap_error( ld, "ldap_search_ext_s", NULL );
		break;
	}

	fprintf( stderr, "  PID=%ld - Search done (%d).\n", (long) pid, rc );

	if ( values ) {
		for ( i = 0; i < nvalues; i++ ) {
			free( values[i] );
		}
		free( values );
	}

	if ( ld != NULL ) {
		ldap_unbind_ext( ld, NULL, NULL );
	}
}

static void
do_search( struct tester_conn_args *config,
	char *sbase, int scope, char *filter, LDAP **ldp,
	char **attrs, int noattrs, int nobind,
	int innerloop, int force )
{
	LDAP	*ld = ldp ? *ldp : NULL;
	int  	i = 0, do_retry = config->retries;
	int     rc = LDAP_SUCCESS;
	char	buf[ BUFSIZ ];
	int		*msgids = NULL, active = 0;

	/* make room for msgid */
	if ( swamp > 1 ) {
		msgids = (int *)calloc( sizeof(int), innerloop );
		if ( !msgids ) {
			tester_error( "calloc failed" );
			exit( EXIT_FAILURE );
		}
	}

retry:;
	if ( ld == NULL ) {
		fprintf( stderr,
			"PID=%ld - Search(%d): "
			"base=\"%s\" scope=%s filter=\"%s\" "
			"attrs=%s%s.\n",
			(long) pid, innerloop,
			sbase, ldap_pvt_scope2str( scope ), filter,
			attrs[0], attrs[1] ? " (more...)" : "" );

		tester_init_ld( &ld, config, nobind );
	}

	if ( swamp > 1 ) {
		do {
			LDAPMessage *res = NULL;
			int j, msgid;

			if ( i < innerloop ) {
				rc = ldap_search_ext( ld, sbase, scope,
						filter, NULL, noattrs, NULL, NULL,
						NULL, LDAP_NO_LIMIT, &msgids[i] );

				active++;
#if 0
				fprintf( stderr,
					">>> PID=%ld - Search maxloop=%d cnt=%d active=%d msgid=%d: "
					"base=\"%s\" scope=%s filter=\"%s\"\n",
					(long) pid, innerloop, i, active, msgids[i],
					sbase, ldap_pvt_scope2str( scope ), filter );
#endif
				i++;

				if ( rc ) {
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
					snprintf( buf, sizeof( buf ),
						"base=\"%s\" filter=\"%s\"\n",
						sbase, filter );
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
							"<<< PID=%ld - SearchDone maxloop=%d cnt=%d active=%d msgid=%d: "
							"base=\"%s\" scope=%s filter=\"%s\"\n",
							(long) pid, innerloop, j, active, msgid,
							sbase, ldap_pvt_scope2str( scope ), filter );
#endif
						break;
					}
				}
				break;

			default:
				/* other messages unexpected */
				fprintf( stderr,
					"### PID=%ld - Search(%d): "
					"base=\"%s\" scope=%s filter=\"%s\" "
					"attrs=%s%s. unexpected response tag=%d\n",
					(long) pid, innerloop,
					sbase, ldap_pvt_scope2str( scope ), filter,
					attrs[0], attrs[1] ? " (more...)" : "", rc );
				break;
			}

			if ( res != NULL ) {
				ldap_msgfree( res );
			}
		} while ( i < innerloop || active > 0 );

	} else {
		for ( ; i < innerloop; i++ ) {
			LDAPMessage *res = NULL;

			if (swamp) {
				int msgid;
				rc = ldap_search_ext( ld, sbase, scope,
						filter, NULL, noattrs, NULL, NULL,
						NULL, LDAP_NO_LIMIT, &msgid );
				if ( rc == LDAP_SUCCESS ) continue;
				else break;
			}
	
			rc = ldap_search_ext_s( ld, sbase, scope,
					filter, attrs, noattrs, NULL, NULL,
					NULL, LDAP_NO_LIMIT, &res );
			if ( res != NULL ) {
				ldap_msgfree( res );
			}
	
			if ( rc ) {
				int first = tester_ignore_err( rc );
				/* if ignore.. */
				if ( first ) {
					/* only log if first occurrence */
					if ( ( force < 2 && first > 0 ) || abs(first) == 1 ) {
						tester_ldap_error( ld, "ldap_search_ext_s", NULL );
					}
					continue;
				}
	
				/* busy needs special handling */
				snprintf( buf, sizeof( buf ),
					"base=\"%s\" filter=\"%s\"\n",
					sbase, filter );
				tester_ldap_error( ld, "ldap_search_ext_s", buf );
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
		fprintf( stderr, "  PID=%ld - Search done (%d).\n", (long) pid, rc );

		if ( ld != NULL ) {
			ldap_unbind_ext( ld, NULL, NULL );
		}
	}
}
