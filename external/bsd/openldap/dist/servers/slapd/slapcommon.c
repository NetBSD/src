/* slapcommon.c - common routine for the slap tools */
/* $OpenLDAP: pkg/ldap/servers/slapd/slapcommon.c,v 1.73.2.7 2008/02/11 23:26:44 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 2003 IBM Corporation.
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
 * This work was initially developed by Kurt Zeilenga for inclusion
 * in OpenLDAP Software.  Additional signficant contributors include
 *    Jong Hyuk Choi
 *    Hallvard B. Furuseth
 *    Howard Chu
 *    Pierangelo Masarati
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "slapcommon.h"
#include "lutil.h"
#include "ldif.h"

tool_vars tool_globals;

#ifdef CSRIMALLOC
static char *leakfilename;
static FILE *leakfile;
#endif

static LDIFFP dummy;

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
int start_syslog;
static char **syslog_unknowns;
#ifdef LOG_LOCAL4
static int syslogUser = SLAP_DEFAULT_SYSLOG_USER;
#endif /* LOG_LOCAL4 */
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

static void
usage( int tool, const char *progname )
{
	char *options = NULL;
	fprintf( stderr,
		"usage: %s [-v] [-d debuglevel] [-f configfile] [-F configdir] [-o <name>[=<value>]]",
		progname );

	switch( tool ) {
	case SLAPACL:
		options = "\n\t[-U authcID | -D authcDN] [-X authzID | -o authzDN=<DN>]"
			"\n\t-b DN [-u] [attr[/access][:value]] [...]\n";
		break;

	case SLAPADD:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix]\n"
			"\t[-l ldiffile] [-j linenumber] [-q] [-u] [-s] [-w]\n";
		break;

	case SLAPAUTH:
		options = "\n\t[-U authcID] [-X authzID] [-R realm] [-M mech] ID [...]\n";
		break;

	case SLAPCAT:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix]"
			" [-l ldiffile] [-a filter]\n";
		break;

	case SLAPDN:
		options = "\n\t[-N | -P] DN [...]\n";
		break;

	case SLAPINDEX:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix] [attr ...] [-q] [-t]\n";
		break;

	case SLAPTEST:
		options = " [-u]\n";
		break;
	}

	if ( options != NULL ) {
		fputs( options, stderr );
	}
	exit( EXIT_FAILURE );
}

static int
parse_slapopt( void )
{
	size_t	len = 0;
	char	*p;

	p = strchr( optarg, '=' );
	if ( p != NULL ) {
		len = p - optarg;
		p++;
	}

	if ( strncasecmp( optarg, "sockurl", len ) == 0 ) {
		if ( !BER_BVISNULL( &listener_url ) ) {
			ber_memfree( listener_url.bv_val );
		}
		ber_str2bv( p, 0, 1, &listener_url );

	} else if ( strncasecmp( optarg, "domain", len ) == 0 ) {
		if ( !BER_BVISNULL( &peer_domain ) ) {
			ber_memfree( peer_domain.bv_val );
		}
		ber_str2bv( p, 0, 1, &peer_domain );

	} else if ( strncasecmp( optarg, "peername", len ) == 0 ) {
		if ( !BER_BVISNULL( &peer_name ) ) {
			ber_memfree( peer_name.bv_val );
		}
		ber_str2bv( p, 0, 1, &peer_name );

	} else if ( strncasecmp( optarg, "sockname", len ) == 0 ) {
		if ( !BER_BVISNULL( &sock_name ) ) {
			ber_memfree( sock_name.bv_val );
		}
		ber_str2bv( p, 0, 1, &sock_name );

	} else if ( strncasecmp( optarg, "ssf", len ) == 0 ) {
		if ( lutil_atou( &ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "transport_ssf", len ) == 0 ) {
		if ( lutil_atou( &transport_ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse transport_ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "tls_ssf", len ) == 0 ) {
		if ( lutil_atou( &tls_ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse tls_ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "sasl_ssf", len ) == 0 ) {
		if ( lutil_atou( &sasl_ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse sasl_ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "authzDN", len ) == 0 ) {
		ber_str2bv( p, 0, 1, &authzDN );

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
	} else if ( strncasecmp( optarg, "syslog", len ) == 0 ) {
		if ( parse_debug_level( p, &ldap_syslog, &syslog_unknowns ) ) {
			return -1;
		}
		start_syslog = 1;

	} else if ( strncasecmp( optarg, "syslog-level", len ) == 0 ) {
		if ( parse_syslog_level( p, &ldap_syslog_level ) ) {
			return -1;
		}
		start_syslog = 1;

#ifdef LOG_LOCAL4
	} else if ( strncasecmp( optarg, "syslog-user", len ) == 0 ) {
		if ( parse_syslog_user( p, &syslogUser ) ) {
			return -1;
		}
		start_syslog = 1;
#endif /* LOG_LOCAL4 */
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

	} else {
		return -1;
	}

	return 0;
}

/*
 * slap_tool_init - initialize slap utility, handle program options.
 * arguments:
 *	name		program name
 *	tool		tool code
 *	argc, argv	command line arguments
 */

static int need_shutdown;

void
slap_tool_init(
	const char* progname,
	int tool,
	int argc, char **argv )
{
	char *options;
	char *conffile = NULL;
	char *confdir = NULL;
	struct berval base = BER_BVNULL;
	char *filterstr = NULL;
	char *subtree = NULL;
	char *ldiffile	= NULL;
	char **debug_unknowns = NULL;
	int rc, i, dbnum;
	int mode = SLAP_TOOL_MODE;
	int truncatemode = 0;
	int use_glue = 1;

#ifdef LDAP_DEBUG
	/* tools default to "none", so that at least LDAP_DEBUG_ANY 
	 * messages show up; use -d 0 to reset */
	slap_debug = LDAP_DEBUG_NONE;
#endif
	ldap_syslog = 0;

#ifdef CSRIMALLOC
	leakfilename = malloc( strlen( progname ) + STRLENOF( ".leak" ) + 1 );
	sprintf( leakfilename, "%s.leak", progname );
	if( ( leakfile = fopen( leakfilename, "w" )) == NULL ) {
		leakfile = stderr;
	}
	free( leakfilename );
#endif

	switch( tool ) {
	case SLAPADD:
		options = "b:cd:f:F:gj:l:n:o:qsS:uvw";
		break;

	case SLAPCAT:
		options = "a:b:cd:f:F:gl:n:o:s:v";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPDN:
		options = "d:f:F:No:Pv";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPTEST:
		options = "d:f:F:o:Quv";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPAUTH:
		options = "d:f:F:M:o:R:U:vX:";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPINDEX:
		options = "b:cd:f:F:gn:o:qtv";
		mode |= SLAP_TOOL_READMAIN;
		break;

	case SLAPACL:
		options = "b:D:d:f:F:o:uU:vX:";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	default:
		fprintf( stderr, "%s: unknown tool mode (%d)\n", progname, tool );
		exit( EXIT_FAILURE );
	}

	dbnum = -1;
	while ( (i = getopt( argc, argv, options )) != EOF ) {
		switch ( i ) {
		case 'a':
			filterstr = ch_strdup( optarg );
			break;

		case 'b':
			ber_str2bv( optarg, 0, 1, &base );
			break;

		case 'c':	/* enable continue mode */
			continuemode++;
			break;

		case 'd': {	/* turn on debugging */
			int	level = 0;

			if ( parse_debug_level( optarg, &level, &debug_unknowns ) ) {
				usage( tool, progname );
			}
#ifdef LDAP_DEBUG
			if ( level == 0 ) {
				/* allow to reset log level */
				slap_debug = 0;

			} else {
				slap_debug |= level;
			}
#else
			if ( level != 0 )
				fputs( "must compile with LDAP_DEBUG for debugging\n",
				       stderr );
#endif
			} break;

		case 'D':
			ber_str2bv( optarg, 0, 1, &authcDN );
			break;

		case 'f':	/* specify a conf file */
			conffile = ch_strdup( optarg );
			break;

		case 'F':	/* specify a conf dir */
			confdir = ch_strdup( optarg );
			break;

		case 'g':	/* disable subordinate glue */
			use_glue = 0;
			break;

		case 'j':	/* jump to linenumber */
			if ( lutil_atoi( &jumpline, optarg ) ) {
				usage( tool, progname );
			}
			break;

		case 'l':	/* LDIF file */
			ldiffile = ch_strdup( optarg );
			break;

		case 'M':
			ber_str2bv( optarg, 0, 0, &mech );
			break;

		case 'N':
			if ( dn_mode && dn_mode != SLAP_TOOL_LDAPDN_NORMAL ) {
				usage( tool, progname );
			}
			dn_mode = SLAP_TOOL_LDAPDN_NORMAL;
			break;

		case 'n':	/* which config file db to index */
			if ( lutil_atoi( &dbnum, optarg ) ) {
				usage( tool, progname );
			}
			break;

		case 'o':
			if ( parse_slapopt() ) {
				usage( tool, progname );
			}
			break;

		case 'P':
			if ( dn_mode && dn_mode != SLAP_TOOL_LDAPDN_PRETTY ) {
				usage( tool, progname );
			}
			dn_mode = SLAP_TOOL_LDAPDN_PRETTY;
			break;

		case 'Q':
			quiet++;
			slap_debug = 0;
			break;

		case 'q':	/* turn on quick */
			mode |= SLAP_TOOL_QUICK;
			break;

		case 'R':
			realm = optarg;
			break;

		case 'S':
			if ( lutil_atou( &csnsid, optarg )
				|| csnsid > SLAP_SYNC_SID_MAX )
			{
				usage( tool, progname );
			}
			break;

		case 's':	/* dump subtree */
			if ( tool == SLAPADD )
				mode |= SLAP_TOOL_NO_SCHEMA_CHECK;
			else if ( tool == SLAPCAT )
				subtree = ch_strdup( optarg );
			break;

		case 't':	/* turn on truncate */
			truncatemode++;
			mode |= SLAP_TRUNCATE_MODE;
			break;

		case 'U':
			ber_str2bv( optarg, 0, 0, &authcID );
			break;

		case 'u':	/* dry run */
			dryrun++;
			break;

		case 'v':	/* turn on verbose */
			verbose++;
			break;

		case 'w':	/* write context csn at the end */
			update_ctxcsn++;
			break;

		case 'X':
			ber_str2bv( optarg, 0, 0, &authzID );
			break;

		default:
			usage( tool, progname );
			break;
		}
	}

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
	if ( start_syslog ) {
		char *logName;
#ifdef HAVE_EBCDIC
		logName = ch_strdup( progname );
		__atoe( logName );
#else
		logName = (char *)progname;
#endif

#ifdef LOG_LOCAL4
		openlog( logName, OPENLOG_OPTIONS, syslogUser );
#elif defined LOG_DEBUG
		openlog( logName, OPENLOG_OPTIONS );
#endif
#ifdef HAVE_EBCDIC
		free( logName );
#endif
	}
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

	switch ( tool ) {
	case SLAPADD:
	case SLAPCAT:
		if ( ( argc != optind ) || (dbnum >= 0 && base.bv_val != NULL ) ) {
			usage( tool, progname );
		}

		break;

	case SLAPINDEX:
		if ( dbnum >= 0 && base.bv_val != NULL ) {
			usage( tool, progname );
		}

		break;

	case SLAPDN:
		if ( argc == optind ) {
			usage( tool, progname );
		}
		break;

	case SLAPAUTH:
		if ( argc == optind && BER_BVISNULL( &authcID ) ) {
			usage( tool, progname );
		}
		break;

	case SLAPTEST:
		if ( argc != optind ) {
			usage( tool, progname );
		}
		break;

	case SLAPACL:
		if ( !BER_BVISNULL( &authcDN ) && !BER_BVISNULL( &authcID ) ) {
			usage( tool, progname );
		}
		if ( BER_BVISNULL( &base ) ) {
			usage( tool, progname );
		}
		ber_dupbv( &baseDN, &base );
		break;

	default:
		break;
	}

	if ( ldiffile == NULL ) {
		dummy.fp = tool == SLAPCAT ? stdout : stdin;
		ldiffp = &dummy;

	} else if ((ldiffp = ldif_open( ldiffile, tool == SLAPCAT ? "w" : "r" ))
		== NULL )
	{
		perror( ldiffile );
		exit( EXIT_FAILURE );
	}

	/*
	 * initialize stuff and figure out which backend we're dealing with
	 */

	rc = slap_init( mode, progname );
	if ( rc != 0 ) {
		fprintf( stderr, "%s: slap_init failed!\n", progname );
		exit( EXIT_FAILURE );
	}

	rc = read_config( conffile, confdir );

	if ( rc != 0 ) {
		fprintf( stderr, "%s: bad configuration %s!\n",
			progname, confdir ? "directory" : "file" );
		exit( EXIT_FAILURE );
	}

	if ( debug_unknowns ) {
		rc = parse_debug_unknowns( debug_unknowns, &slap_debug );
		ldap_charray_free( debug_unknowns );
		debug_unknowns = NULL;
		if ( rc )
			exit( EXIT_FAILURE );
	}

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
	if ( syslog_unknowns ) {
		rc = parse_debug_unknowns( syslog_unknowns, &ldap_syslog );
		ldap_charray_free( syslog_unknowns );
		syslog_unknowns = NULL;
		if ( rc )
			exit( EXIT_FAILURE );
	}
#endif

	at_oc_cache = 1;

	switch ( tool ) {
	case SLAPADD:
	case SLAPCAT:
	case SLAPINDEX:
		if ( !nbackends ) {
			fprintf( stderr, "No databases found "
					"in config file\n" );
			exit( EXIT_FAILURE );
		}
		break;

	default:
		break;
	}

	if ( use_glue ) {
		rc = glue_sub_attach();

		if ( rc != 0 ) {
			fprintf( stderr,
				"%s: subordinate configuration error\n", progname );
			exit( EXIT_FAILURE );
		}
	}

	rc = slap_schema_check();

	if ( rc != 0 ) {
		fprintf( stderr, "%s: slap_schema_prep failed!\n", progname );
		exit( EXIT_FAILURE );
	}

	switch ( tool ) {
	case SLAPDN:
	case SLAPTEST:
	case SLAPAUTH:
		be = NULL;
		goto startup;

	default:
		break;
	}

	if( filterstr ) {
		filter = str2filter( filterstr );

		if( filter == NULL ) {
			fprintf( stderr, "Invalid filter '%s'\n", filterstr );
			exit( EXIT_FAILURE );
		}
	}

	if( subtree ) {
		struct berval val;
		ber_str2bv( subtree, 0, 0, &val );
		rc = dnNormalize( 0, NULL, NULL, &val, &sub_ndn, NULL );
		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "Invalid subtree DN '%s'\n", subtree );
			exit( EXIT_FAILURE );
		}

		if ( BER_BVISNULL( &base ) && dbnum == -1 ) {
			base = val;
		} else {
			free( subtree );
		}
	}

	if( base.bv_val != NULL ) {
		struct berval nbase;

		rc = dnNormalize( 0, NULL, NULL, &base, &nbase, NULL );
		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: slap_init invalid suffix (\"%s\")\n",
				progname, base.bv_val );
			exit( EXIT_FAILURE );
		}

		be = select_backend( &nbase, 0 );
		ber_memfree( nbase.bv_val );

		switch ( tool ) {
		case SLAPACL:
			goto startup;

		default:
			break;
		}

		if( be == NULL ) {
			fprintf( stderr, "%s: slap_init no backend for \"%s\"\n",
				progname, base.bv_val );
			exit( EXIT_FAILURE );
		}
		/* If the named base is a glue master, operate on the
		 * entire context
		 */
		if ( SLAP_GLUE_INSTANCE( be ) ) {
			nosubordinates = 1;
		}

	} else if ( dbnum == -1 ) {
		/* no suffix and no dbnum specified, just default to
		 * the first available database
		 */
		if ( nbackends <= 0 ) {
			fprintf( stderr, "No available databases\n" );
			exit( EXIT_FAILURE );
		}
		LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
			dbnum++;

			/* db #0 is cn=config, don't select it as a default */
			if ( dbnum < 1 ) continue;
		
		 	if ( SLAP_MONITOR(be))
				continue;

		/* If just doing the first by default and it is a
		 * glue subordinate, find the master.
		 */
			if ( SLAP_GLUE_SUBORDINATE(be) ) {
				nosubordinates = 1;
				continue;
			}
			break;
		}

		if ( !be ) {
			fprintf( stderr, "Available database(s) "
					"do not allow %s\n", progname );
			exit( EXIT_FAILURE );
		}
		
		if ( nosubordinates == 0 && dbnum > 1 ) {
			Debug( LDAP_DEBUG_ANY,
				"The first database does not allow %s;"
				" using the first available one (%d)\n",
				progname, dbnum, 0 );
		}

	} else if ( dbnum < 0 || dbnum > (nbackends-1) ) {
		fprintf( stderr,
			"Database number selected via -n is out of range\n"
			"Must be in the range 0 to %d"
			" (number of configured databases)\n",
			nbackends-1 );
		exit( EXIT_FAILURE );

	} else {
		LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
			if ( dbnum == 0 ) break;
			dbnum--;
		}
	}

startup:;

#ifdef CSRIMALLOC
	mal_leaktrace(1);
#endif

	if ( conffile != NULL ) {
		ch_free( conffile );
	}

	if ( ldiffile != NULL ) {
		ch_free( ldiffile );
	}

	/* slapdn doesn't specify a backend to startup */
	if ( !dryrun && tool != SLAPDN ) {
		need_shutdown = 1;

		if ( slap_startup( be ) ) {
			switch ( tool ) {
			case SLAPTEST:
				fprintf( stderr, "slap_startup failed "
						"(test would succeed using "
						"the -u switch)\n" );
				break;

			default:
				fprintf( stderr, "slap_startup failed\n" );
				break;
			}

			exit( EXIT_FAILURE );
		}
	}
}

void slap_tool_destroy( void )
{
	if ( !dryrun ) {
		if ( need_shutdown ) {
			slap_shutdown( be );
		}
		slap_destroy();
	}
#ifdef SLAPD_MODULES
	if ( slapMode == SLAP_SERVER_MODE ) {
	/* always false. just pulls in necessary symbol references. */
		lutil_uuidstr(NULL, 0);
	}
	module_kill();
#endif
	schema_destroy();
#ifdef HAVE_TLS
	ldap_pvt_tls_destroy();
#endif
	config_destroy();

#ifdef CSRIMALLOC
	mal_dumpleaktrace( leakfile );
#endif

	if ( !BER_BVISNULL( &authcDN ) ) {
		ch_free( authcDN.bv_val );
	}

	if ( ldiffp && ldiffp != &dummy ) {
		ldif_close( ldiffp );
	}
}
