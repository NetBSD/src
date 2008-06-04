/* $OpenLDAP: pkg/ldap/servers/slapd/back-perl/config.c,v 1.22.2.3 2008/02/11 23:26:47 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2008 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
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

#include "perl_back.h"


/**********************************************************
 *
 * Config
 *
 **********************************************************/
int
perl_back_db_config(
	 BackendDB *be,
	 const char *fname,
	 int lineno,
	 int argc,
	 char **argv
)
{
	SV* loc_sv;
	PerlBackend *perl_back = (PerlBackend *) be->be_private;
	char eval_str[EVAL_BUF_SIZE];
	int count ;
	int args;
	int return_code;
	

	if ( strcasecmp( argv[0], "perlModule" ) == 0 ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_ANY,
				 "%s.pm: line %d: missing module in \"perlModule <module>\" line\n",
				fname, lineno, 0 );
			return( 1 );
		}

#ifdef PERL_IS_5_6
		snprintf( eval_str, EVAL_BUF_SIZE, "use %s;", argv[1] );
		eval_pv( eval_str, 0 );

		if (SvTRUE(ERRSV)) {
			STRLEN n_a;

			fprintf(stderr , "Error %s\n", SvPV(ERRSV, n_a)) ;
		}
#else
		snprintf( eval_str, EVAL_BUF_SIZE, "%s.pm", argv[1] );
		perl_require_pv( eval_str );

		if (SvTRUE(GvSV(errgv))) {
			fprintf(stderr , "Error %s\n", SvPV(GvSV(errgv), na)) ;
		}
#endif /* PERL_IS_5_6 */
		else {
			dSP; ENTER; SAVETMPS;
			PUSHMARK(sp);
			XPUSHs(sv_2mortal(newSVpv(argv[1], 0)));
			PUTBACK;

#ifdef PERL_IS_5_6
			count = call_method("new", G_SCALAR);
#else
			count = perl_call_method("new", G_SCALAR);
#endif

			SPAGAIN;

			if (count != 1) {
				croak("Big trouble in config\n") ;
			}

			perl_back->pb_obj_ref = newSVsv(POPs);

			PUTBACK; FREETMPS; LEAVE ;
		}

	} else if ( strcasecmp( argv[0], "perlModulePath" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
				"%s: line %d: missing module in \"PerlModulePath <module>\" line\n",
				fname, lineno );
			return( 1 );
		}

		snprintf( eval_str, EVAL_BUF_SIZE, "push @INC, '%s';", argv[1] );
#ifdef PERL_IS_5_6
		loc_sv = eval_pv( eval_str, 0 );
#else
		loc_sv = perl_eval_pv( eval_str, 0 );
#endif

		/* XXX loc_sv return value is ignored. */

	} else if ( strcasecmp( argv[0], "filterSearchResults" ) == 0 ) {
		perl_back->pb_filter_search_results = 1;
	} else {
		return_code = SLAP_CONF_UNKNOWN;
		/*
		 * Pass it to Perl module if defined
		 */

		{
			dSP ;  ENTER ; SAVETMPS;

			PUSHMARK(sp) ;
			XPUSHs( perl_back->pb_obj_ref );

			/* Put all arguments on the perl stack */
			for( args = 0; args < argc; args++ ) {
				XPUSHs(sv_2mortal(newSVpv(argv[args], 0)));
			}

			PUTBACK ;

#ifdef PERL_IS_5_6
			count = call_method("config", G_SCALAR);
#else
			count = perl_call_method("config", G_SCALAR);
#endif

			SPAGAIN ;

			if (count != 1) {
				croak("Big trouble in config\n") ;
			}

			return_code = POPi;

			PUTBACK ; FREETMPS ;  LEAVE ;

		}

		return return_code;
	}

	return 0;
}
