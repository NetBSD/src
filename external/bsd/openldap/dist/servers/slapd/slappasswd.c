/* $OpenLDAP: pkg/ldap/servers/slapd/slappasswd.c,v 1.5.2.5 2008/02/11 23:26:44 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
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
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <ldap.h>
#include <lber_pvt.h>
#include <lutil.h>
#include <lutil_sha1.h>

#include "ldap_defaults.h"
#include "slap.h"

static int	verbose = 0;

static void
usage(const char *s)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -c format\tcrypt(3) salt format\n"
		"  -g\t\tgenerate random password\n"
		"  -h hash\tpassword scheme\n"
		"  -n\t\tomit trailing newline\n"
		"  -s secret\tnew password\n"
		"  -u\t\tgenerate RFC2307 values (default)\n"
		"  -v\t\tincrease verbosity\n"
		"  -T file\tread file for new password\n"
		, s );

	exit( EXIT_FAILURE );
}

int
slappasswd( int argc, char *argv[] )
{
#ifdef LUTIL_SHA1_BYTES
	char	*default_scheme = "{SSHA}";
#else
	char	*default_scheme = "{SMD5}";
#endif
	char	*scheme = default_scheme;

	char	*newpw = NULL;
	char	*pwfile = NULL;
	const char *text;
	const char *progname = "slappasswd";

	int		i;
	char		*newline = "\n";
	struct berval passwd = BER_BVNULL;
	struct berval hash;

	while( (i = getopt( argc, argv,
		"c:d:gh:ns:T:vu" )) != EOF )
	{
		switch (i) {
		case 'c':	/* crypt salt format */
			scheme = "{CRYPT}";
			lutil_salt_format( optarg );
			break;

		case 'g':	/* new password (generate) */
			if ( pwfile != NULL ) {
				fprintf( stderr, "Option -g incompatible with -T\n" );
				return EXIT_FAILURE;

			} else if ( newpw != NULL ) {
				fprintf( stderr, "New password already provided\n" );
				return EXIT_FAILURE;

			} else if ( lutil_passwd_generate( &passwd, 8 )) {
				fprintf( stderr, "Password generation failed\n" );
				return EXIT_FAILURE;
			}
			break;

		case 'h':	/* scheme */
			if ( scheme != default_scheme ) {
				fprintf( stderr, "Scheme already provided\n" );
				return EXIT_FAILURE;

			} else {
				scheme = ch_strdup( optarg );
			}
			break;

		case 'n':
			newline = "";
			break;

		case 's':	/* new password (secret) */
			if ( pwfile != NULL ) {
				fprintf( stderr, "Option -s incompatible with -T\n" );
				return EXIT_FAILURE;

			} else if ( newpw != NULL ) {
				fprintf( stderr, "New password already provided\n" );
				return EXIT_FAILURE;

			} else {
				char* p;
				newpw = ch_strdup( optarg );

				for( p = optarg; *p != '\0'; p++ ) {
					*p = '\0';
				}
			}
			break;

		case 'T':	/* password file */
			if ( pwfile != NULL ) {
				fprintf( stderr, "Password file already provided\n" );
				return EXIT_FAILURE;

			} else if ( newpw != NULL ) {
				fprintf( stderr, "Option -T incompatible with -s/-g\n" );
				return EXIT_FAILURE;

			}
			pwfile = optarg;
			break;

		case 'u':	/* RFC2307 userPassword */
			break;

		case 'v':	/* verbose */
			verbose++;
			break;

		default:
			usage ( progname );
		}
	}

	if( argc - optind != 0 ) {
		usage( progname );
	} 

	if( pwfile != NULL ) {
		if( lutil_get_filed_password( pwfile, &passwd )) {
			return EXIT_FAILURE;
		}
	} else if ( BER_BVISEMPTY( &passwd )) {
		if( newpw == NULL ) {
			/* prompt for new password */
			char *cknewpw;
			newpw = ch_strdup(getpassphrase("New password: "));
			cknewpw = getpassphrase("Re-enter new password: ");
	
			if( strcmp( newpw, cknewpw )) {
				fprintf( stderr, "Password values do not match\n" );
				return EXIT_FAILURE;
			}
		}

		passwd.bv_val = newpw;
		passwd.bv_len = strlen(passwd.bv_val);
	} else {
		hash = passwd;
		goto print_pw;
	}

	lutil_passwd_hash( &passwd, scheme, &hash, &text );
	if( hash.bv_val == NULL ) {
		fprintf( stderr,
			"Password generation failed for scheme %s: %s\n",
			scheme, text ? text : "" );
		return EXIT_FAILURE;
	}

	if( lutil_passwd( &hash, &passwd, NULL, &text ) ) {
		fprintf( stderr, "Password verification failed. %s\n",
			text ? text : "" );
		return EXIT_FAILURE;
	}

print_pw:;
	printf( "%s%s" , hash.bv_val, newline );
	return EXIT_SUCCESS;
}
