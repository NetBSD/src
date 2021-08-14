/*	$NetBSD: slapd-bind.c,v 1.3 2021/08/14 16:15:03 christos Exp $	*/

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
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: slapd-bind.c,v 1.3 2021/08/14 16:15:03 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include "ac/stdlib.h"
#include "ac/time.h"

#include "ac/ctype.h"
#include "ac/param.h"
#include "ac/socket.h"
#include "ac/string.h"
#include "ac/unistd.h"
#include "ac/wait.h"
#include "ac/time.h"

#include "ldap.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "lber_pvt.h"
#include "ldap_pvt.h"

#include "slapd-common.h"

static int
do_bind( struct tester_conn_args *config, char *dn, int maxloop, int force,
	int noinit, LDAP **ldp, struct berval *pass, int action_type, void *action );

static int
do_base( struct tester_conn_args *config, char *dn, char *base, char *filter, char *pwattr,
	int force, int noinit, int action_type, void *action );

/* This program can be invoked two ways: if -D is used to specify a Bind DN,
 * that DN will be used repeatedly for all of the Binds. If instead -b is used
 * to specify a base DN, a search will be done for all "person" objects under
 * that base DN. Then DNs from this list will be randomly selected for each
 * Bind request. All of the users must have identical passwords. Also it is
 * assumed that the users are all onelevel children of the base.
 */
static void
usage( char *name, char opt )
{
	if ( opt ) {
		fprintf( stderr, "%s: unable to handle option \'%c\'\n\n",
			name, opt );
	}

	fprintf( stderr, "usage: %s " TESTER_COMMON_HELP
		"[-b <baseDN> [-f <searchfilter>] [-a pwattr]] "
		"[-B <extra>[,...]] "
		"[-F] "
		"[-I]\n",
		name );
	exit( EXIT_FAILURE );
}

int
main( int argc, char **argv )
{
	int		i;
	char		*base = NULL;
	char		*filter = "(objectClass=person)";
	char		*pwattr = NULL;
	int		force = 0;
	int		noinit = 1;
	struct tester_conn_args	*config;

	/* extra action to do after bind... */
	struct berval	type[] = {
		BER_BVC( "tester=" ),
		BER_BVC( "add=" ),
		BER_BVC( "bind=" ),
		BER_BVC( "modify=" ),
		BER_BVC( "modrdn=" ),
		BER_BVC( "read=" ),
		BER_BVC( "search=" ),
		BER_BVNULL
	};

	LDAPURLDesc	*extra_ludp = NULL;

	config = tester_init( "slapd-bind", TESTER_BIND );

	/* by default, tolerate invalid credentials */
	tester_ignore_str2errlist( "*INVALID_CREDENTIALS" );

	while ( ( i = getopt( argc, argv, TESTER_COMMON_OPTS "a:B:b:Ff:I" ) ) != EOF )
	{
		switch ( i ) {
		case 'a':
			pwattr = optarg;
			break;

		case 'b':		/* base DN of a tree of user DNs */
			base = optarg;
			break;

		case 'B':
			{
			int	c;

			for ( c = 0; type[c].bv_val; c++ ) {
				if ( strncasecmp( optarg, type[c].bv_val, type[c].bv_len ) == 0 )
				{
					break;
				}
			}

			if ( type[c].bv_val == NULL ) {
				usage( argv[0], 'B' );
			}

			switch ( c ) {
			case TESTER_TESTER:
			case TESTER_BIND:
				/* invalid */
				usage( argv[0], 'B' );

			case TESTER_SEARCH:
				{
				if ( ldap_url_parse( &optarg[type[c].bv_len], &extra_ludp ) != LDAP_URL_SUCCESS )
				{
					usage( argv[0], 'B' );
				}
				} break;

			case TESTER_ADDEL:
			case TESTER_MODIFY:
			case TESTER_MODRDN:
			case TESTER_READ:
				/* nothing to do */
				break;

			default:
				assert( 0 );
			}

			} break;

		case 'f':
			filter = optarg;
			break;

		case 'F':
			force++;
			break;

		case 'I':
			/* reuse connection */
			noinit = 0;
			break;

		default:
			if ( tester_config_opt( config, i, optarg ) == LDAP_SUCCESS ) {
				break;
			}
			usage( argv[0], i );
			break;
		}
	}

	tester_config_finish( config );

	for ( i = 0; i < config->outerloops; i++ ) {
		int rc;

		if ( base != NULL ) {
			rc = do_base( config, config->binddn, base,
				filter, pwattr, force, noinit, -1, NULL );
		} else {
			rc = do_bind( config, config->binddn,
				config->loops, force, noinit, NULL, &config->pass, -1, NULL );
		}
		if ( rc == LDAP_SERVER_DOWN )
			break;
	}

	exit( EXIT_SUCCESS );
}


static int
do_bind( struct tester_conn_args *config, char *dn, int maxloop, int force,
	int noinit, LDAP **ldp, struct berval *pass, int action_type, void *action )
{
	LDAP	*ld = ldp ? *ldp : NULL;
	char	*bindfunc = "ldap_sasl_bind_s";
	int  	i, rc = -1;

	/* for internal search */
	int	timelimit = 0;
	int	sizelimit = 0;

	switch ( action_type ) {
	case -1:
		break;

	case TESTER_SEARCH:
		{
		LDAPURLDesc	*ludp = (LDAPURLDesc *)action;

		assert( action != NULL );

		if ( ludp->lud_exts != NULL ) {
			for ( i = 0; ludp->lud_exts[ i ] != NULL; i++ ) {
				char	*ext = ludp->lud_exts[ i ];
				int	crit = 0;

				if (ext[0] == '!') {
					crit++;
					ext++;
				}

				if ( strncasecmp( ext, "x-timelimit=", STRLENOF( "x-timelimit=" ) ) == 0 ) {
					if ( lutil_atoi( &timelimit, &ext[ STRLENOF( "x-timelimit=" ) ] ) && crit ) {
						tester_error( "unable to parse critical extension x-timelimit" );
					}

				} else if ( strncasecmp( ext, "x-sizelimit=", STRLENOF( "x-sizelimit=" ) ) == 0 ) {
					if ( lutil_atoi( &sizelimit, &ext[ STRLENOF( "x-sizelimit=" ) ] ) && crit ) {
						tester_error( "unable to parse critical extension x-sizelimit" );
					}

				} else if ( crit ) {
					tester_error( "unknown critical extension" );
				}
			}
		}
		} break;

	default:
		/* nothing to do yet */
		break;
	}
			
	if ( maxloop > 1 ) {
		fprintf( stderr, "PID=%ld - Bind(%d): dn=\"%s\".\n",
			 (long) pid, maxloop, dn );
	}

	for ( i = 0; i < maxloop; i++ ) {
		if ( !noinit || ld == NULL ) {
			tester_init_ld( &ld, config, TESTER_INIT_ONLY );

#ifdef HAVE_CYRUS_SASL
			if ( config->secprops != NULL ) {
				rc = ldap_set_option( ld,
						LDAP_OPT_X_SASL_SECPROPS, config->secprops );

				if( rc != LDAP_OPT_SUCCESS ) {
					tester_ldap_error( ld, "ldap_set_option(SECPROPS)", NULL );
					exit( EXIT_FAILURE );
				}
			}
#endif
		}

		if ( config->authmethod == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
			bindfunc = "ldap_sasl_interactive_bind_s";
			rc = ldap_sasl_interactive_bind_s( ld,
					dn,
					config->mech,
					NULL, NULL,
					LDAP_SASL_QUIET,
					lutil_sasl_interact,
					config->defaults );
#else /* HAVE_CYRUS_SASL */
			/* caller shouldn't have allowed this */
			assert(0);
#endif
		} else if ( config->authmethod == LDAP_AUTH_SIMPLE ) {
			bindfunc = "ldap_sasl_bind_s";
			rc = ldap_sasl_bind_s( ld,
					dn, LDAP_SASL_SIMPLE,
					pass, NULL, NULL, NULL );
		}

		if ( rc ) {
			int first = tester_ignore_err( rc );

			/* if ignore.. */
			if ( first ) {
				/* only log if first occurrence */
				if ( ( force < 2 && first > 0 ) || abs(first) == 1 ) {
					tester_ldap_error( ld, bindfunc, NULL );
				}
				rc = LDAP_SUCCESS;

			} else {
				tester_ldap_error( ld, bindfunc, NULL );
			}
		}

		switch ( action_type ) {
		case -1:
			break;

		case TESTER_SEARCH:
			{
			LDAPURLDesc	*ludp = (LDAPURLDesc *)action;
			LDAPMessage	*res = NULL;
			struct timeval	tv = { 0 }, *tvp = NULL;

			if ( timelimit ) {
				tv.tv_sec = timelimit;
				tvp = &tv;
			}

			assert( action != NULL );

			rc = ldap_search_ext_s( ld,
				ludp->lud_dn, ludp->lud_scope,
				ludp->lud_filter, ludp->lud_attrs, 0,
				NULL, NULL, tvp, sizelimit, &res );
			ldap_msgfree( res );
			} break;

		default:
			/* nothing to do yet */
			break;
		}
			
		if ( !noinit ) {
			ldap_unbind_ext( ld, NULL, NULL );
			ld = NULL;
		}

		if ( rc != LDAP_SUCCESS ) {
			break;
		}
	}

	if ( maxloop > 1 ) {
		fprintf( stderr, "  PID=%ld - Bind done (%d).\n", (long) pid, rc );
	}

	if ( ldp && noinit ) {
		*ldp = ld;

	} else if ( ld != NULL ) {
		ldap_unbind_ext( ld, NULL, NULL );
	}

	return rc;
}


static int
do_base( struct tester_conn_args *config, char *dn, char *base, char *filter, char *pwattr,
	int force, int noinit, int action_type, void *action )
{
	LDAP	*ld = NULL;
	int  	i = 0;
	int     rc = LDAP_SUCCESS;
	ber_int_t msgid;
	LDAPMessage *res, *msg;
	char **dns = NULL;
	struct berval *creds = NULL;
	char *attrs[] = { LDAP_NO_ATTRS, NULL };
	int ndns = 0;
#ifdef _WIN32
	DWORD beg, end;
#else
	struct timeval beg, end;
#endif
	char *nullstr = "";

	tester_init_ld( &ld, config, 0 );

	fprintf( stderr, "PID=%ld - Bind(%d): base=\"%s\", filter=\"%s\" attr=\"%s\".\n",
			(long) pid, config->loops, base, filter, pwattr );

	if ( pwattr != NULL ) {
		attrs[ 0 ] = pwattr;
	}
	rc = ldap_search_ext( ld, base, LDAP_SCOPE_SUBTREE,
			filter, attrs, 0, NULL, NULL, 0, 0, &msgid );
	if ( rc != LDAP_SUCCESS ) {
		tester_ldap_error( ld, "ldap_search_ext", NULL );
		exit( EXIT_FAILURE );
	}

	while ( ( rc = ldap_result( ld, LDAP_RES_ANY, LDAP_MSG_ONE, NULL, &res ) ) > 0 )
	{
		BerElement *ber;
		struct berval bv;
		int done = 0;

		for ( msg = ldap_first_message( ld, res ); msg;
			msg = ldap_next_message( ld, msg ) )
		{
			switch ( ldap_msgtype( msg ) ) {
			case LDAP_RES_SEARCH_ENTRY:
				rc = ldap_get_dn_ber( ld, msg, &ber, &bv );
				dns = realloc( dns, (ndns + 1)*sizeof(char *) );
				if ( !dns ) {
					tester_error( "realloc failed" );
					exit( EXIT_FAILURE );
				}
				dns[ndns] = ber_strdup( bv.bv_val );
				if ( pwattr != NULL ) {
					struct berval	**values = ldap_get_values_len( ld, msg, pwattr );

					creds = realloc( creds, (ndns + 1)*sizeof(struct berval) );
					if ( !creds ) {
						tester_error( "realloc failed" );
						exit( EXIT_FAILURE );
					}
					if ( values == NULL ) {
novals:;
						creds[ndns].bv_len = 0;
						creds[ndns].bv_val = nullstr;

					} else {
						static struct berval	cleartext = BER_BVC( "{CLEARTEXT} " );
						struct berval		value = *values[ 0 ];

						if ( value.bv_val[ 0 ] == '{' ) {
							char *end = ber_bvchr( &value, '}' );

							if ( end ) {
								if ( ber_bvcmp( &value, &cleartext ) == 0 ) {
									value.bv_val += cleartext.bv_len;
									value.bv_len -= cleartext.bv_len;

								} else {
									ldap_value_free_len( values );
									goto novals;
								}
							}

						}

						ber_dupbv( &creds[ndns], &value );
						ldap_value_free_len( values );
					}
				}
				ndns++;
				ber_free( ber, 0 );
				break;

			case LDAP_RES_SEARCH_RESULT:
				done = 1;
				break;
			}
			if ( done )
				break;
		}
		ldap_msgfree( res );
		if ( done ) break;
	}

#ifdef _WIN32
	beg = GetTickCount();
#else
	gettimeofday( &beg, NULL );
#endif

	if ( ndns == 0 ) {
		tester_error( "No DNs" );
		if ( ld != NULL ) {
			ldap_unbind_ext( ld, NULL, NULL );
		}
		return 1;
	}

	fprintf( stderr, "  PID=%ld - Bind base=\"%s\" filter=\"%s\" got %d values.\n",
		(long) pid, base, filter, ndns );

	/* Ok, got list of DNs, now start binding to each */
	for ( i = 0; i < config->loops; i++ ) {
		struct berval *pass = &config->pass;
		int		j;

#if 0	/* use high-order bits for better randomness (Numerical Recipes in "C") */
		j = rand() % ndns;
#endif
		j = ((double)ndns)*rand()/(RAND_MAX + 1.0);

		if ( creds && !BER_BVISEMPTY( &creds[j] ) ) {
			pass = &creds[j];
		}

		if ( do_bind( config, dns[j], 1, force, noinit, &ld, pass,
			action_type, action ) && !force )
		{
			break;
		}
	}

	if ( ld != NULL ) {
		ldap_unbind_ext( ld, NULL, NULL );
		ld = NULL;
	}

#ifdef _WIN32
	end = GetTickCount();
	end -= beg;

	fprintf( stderr, "  PID=%ld - Bind done %d in %d.%03d seconds.\n",
		(long) pid, i, end / 1000, end % 1000 );
#else
	gettimeofday( &end, NULL );
	end.tv_usec -= beg.tv_usec;
	if (end.tv_usec < 0 ) {
		end.tv_usec += 1000000;
		end.tv_sec -= 1;
	}
	end.tv_sec -= beg.tv_sec;

	fprintf( stderr, "  PID=%ld - Bind done %d in %ld.%06ld seconds.\n",
		(long) pid, i, (long) end.tv_sec, (long) end.tv_usec );
#endif

	if ( dns ) {
		for ( i = 0; i < ndns; i++ ) {
			ber_memfree( dns[i] );
		}
		free( dns );
	}

	if ( creds ) {
		for ( i = 0; i < ndns; i++ ) {
			if ( creds[i].bv_val != nullstr ) {
				ber_memfree( creds[i].bv_val );
			}
		}
		free( creds );
	}

	return 0;
}
