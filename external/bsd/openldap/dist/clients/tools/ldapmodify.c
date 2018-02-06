/*	$NetBSD: ldapmodify.c,v 1.1.1.6 2018/02/06 01:53:07 christos Exp $	*/

/* ldapmodify.c - generic program to modify or add entries using LDAP */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2017 The OpenLDAP Foundation.
 * Portions Copyright 2006 Howard Chu.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 1998-2001 Net Boolean Incorporated.
 * Portions Copyright 2001-2003 IBM Corporation.
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
/* Portions Copyright (c) 1992-1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.  This
 * software is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).  Additional significant contributors
 * include:
 *   Kurt D. Zeilenga
 *   Norbert Klasen
 *   Howard Chu
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ldapmodify.c,v 1.1.1.6 2018/02/06 01:53:07 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/socket.h>
#include <ac/time.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <ldap.h>

#include "lutil.h"
#include "lutil_ldap.h"
#include "ldif.h"
#include "ldap_defaults.h"
#include "ldap_pvt.h"
#include "lber_pvt.h"

#include "common.h"

static int	ldapadd;
static char *rejfile = NULL;
static LDAP	*ld = NULL;

#define	M_SEP	0x7f

/* strings found in LDIF entries */
static struct berval BV_VERSION = BER_BVC("version");
static struct berval BV_DN = BER_BVC("dn");
static struct berval BV_CONTROL = BER_BVC("control");
static struct berval BV_CHANGETYPE = BER_BVC("changetype");
static struct berval BV_ADDCT = BER_BVC("add");
static struct berval BV_MODIFYCT = BER_BVC("modify");
static struct berval BV_DELETECT = BER_BVC("delete");
static struct berval BV_MODRDNCT = BER_BVC("modrdn");
static struct berval BV_MODDNCT = BER_BVC("moddn");
static struct berval BV_RENAMECT = BER_BVC("rename");
static struct berval BV_MODOPADD = BER_BVC("add");
static struct berval BV_MODOPREPLACE = BER_BVC("replace");
static struct berval BV_MODOPDELETE = BER_BVC("delete");
static struct berval BV_MODOPINCREMENT = BER_BVC("increment");
static struct berval BV_NEWRDN = BER_BVC("newrdn");
static struct berval BV_DELETEOLDRDN = BER_BVC("deleteoldrdn");
static struct berval BV_NEWSUP = BER_BVC("newsuperior");

#define	BV_CASEMATCH(a, b) \
	((a)->bv_len == (b)->bv_len && 0 == strcasecmp((a)->bv_val, (b)->bv_val))

static int process_ldif_rec LDAP_P(( char *rbuf, unsigned long lineno ));
static int parse_ldif_control LDAP_P(( struct berval *val, LDAPControl ***pctrls ));
static int domodify LDAP_P((
	const char *dn,
	LDAPMod **pmods,
	LDAPControl **pctrls,
	int newentry ));
static int dodelete LDAP_P((
	const char *dn,
	LDAPControl **pctrls ));
static int dorename LDAP_P((
	const char *dn,
	const char *newrdn,
	const char *newsup,
	int deleteoldrdn,
	LDAPControl **pctrls ));
static int process_response(
	LDAP *ld,
	int msgid,
	int res,
	const char *dn );

#ifdef LDAP_X_TXN
static int txn = 0;
static int txnabort = 0;
struct berval *txn_id = NULL;
#endif

void
usage( void )
{
	fprintf( stderr, _("Add or modify entries from an LDAP server\n\n"));
	fprintf( stderr, _("usage: %s [options]\n"), prog);
	fprintf( stderr, _("	The list of desired operations are read from stdin"
		" or from the file\n"));
	fprintf( stderr, _("	specified by \"-f file\".\n"));
	fprintf( stderr, _("Add or modify options:\n"));
	fprintf( stderr, _("  -a         add values (%s)\n"),
		(ldapadd ? _("default") : _("default is to replace")));
	fprintf( stderr, _("  -c         continuous operation mode (do not stop on errors)\n"));
	fprintf( stderr, _("  -E [!]ext=extparam	modify extensions"
		" (! indicate s criticality)\n"));
	fprintf( stderr, _("  -f file    read operations from `file'\n"));
	fprintf( stderr, _("  -M         enable Manage DSA IT control (-MM to make critical)\n"));
	fprintf( stderr, _("  -P version protocol version (default: 3)\n"));
#ifdef LDAP_X_TXN
 	fprintf( stderr,
		_("             [!]txn=<commit|abort>         (transaction)\n"));
#endif
	fprintf( stderr, _("  -S file    write skipped modifications to `file'\n"));

	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "aE:rS:"
	"cd:D:e:f:h:H:IMnNO:o:p:P:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	char	*control, *cvalue;
	int		crit;

	switch ( i ) {
	case 'E': /* modify extensions */
		if( protocol == LDAP_VERSION2 ) {
			fprintf( stderr, _("%s: -E incompatible with LDAPv%d\n"),
				prog, protocol );
			exit( EXIT_FAILURE );
		}

		/* should be extended to support comma separated list of
		 *	[!]key[=value] parameters, e.g.  -E !foo,bar=567
		 */

		crit = 0;
		cvalue = NULL;
		if( optarg[0] == '!' ) {
			crit = 1;
			optarg++;
		}

		control = ber_strdup( optarg );
		if ( (cvalue = strchr( control, '=' )) != NULL ) {
			*cvalue++ = '\0';
		}

#ifdef LDAP_X_TXN
		if( strcasecmp( control, "txn" ) == 0 ) {
			/* Transaction */
			if( txn ) {
				fprintf( stderr,
					_("txn control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if( cvalue != NULL ) {
				if( strcasecmp( cvalue, "abort" ) == 0 ) {
					txnabort=1;
				} else if( strcasecmp( cvalue, "commit" ) != 0 ) {
					fprintf( stderr, _("Invalid value for txn control, %s\n"),
						cvalue );
					exit( EXIT_FAILURE );
				}
			}

			txn = 1 + crit;
		} else
#endif
		{
			fprintf( stderr, _("Invalid modify extension name: %s\n"),
				control );
			usage();
		}
		break;

	case 'a':	/* add */
		ldapadd = 1;
		break;

	case 'r':	/* replace (obsolete) */
		break;

	case 'S':	/* skipped modifications to file */
		if( rejfile != NULL ) {
			fprintf( stderr, _("%s: -S previously specified\n"), prog );
			exit( EXIT_FAILURE );
		}
		rejfile = ber_strdup( optarg );
		break;

	default:
		return 0;
	}
	return 1;
}


int
main( int argc, char **argv )
{
	char		*rbuf = NULL, *rejbuf = NULL;
	FILE		*rejfp;
	struct LDIFFP *ldiffp = NULL, ldifdummy = {0};
	char		*matched_msg, *error_msg;
	int		rc, retval, ldifrc;
	int		len;
	int		i = 0, lmax = 0;
	unsigned long	lineno, nextline = 0;
	LDAPControl	c[1];

	prog = lutil_progname( "ldapmodify", argc, argv );

	/* strncmp instead of strcmp since NT binaries carry .exe extension */
	ldapadd = ( strncasecmp( prog, "ldapadd", sizeof("ldapadd")-1 ) == 0 );

	tool_init( ldapadd ? TOOL_ADD : TOOL_MODIFY );

	tool_args( argc, argv );

	if ( argc != optind ) usage();

	if ( rejfile != NULL ) {
		if (( rejfp = fopen( rejfile, "w" )) == NULL ) {
			perror( rejfile );
			retval = EXIT_FAILURE;
			goto fail;
		}
	} else {
		rejfp = NULL;
	}

	if ( infile != NULL ) {
		if (( ldiffp = ldif_open( infile, "r" )) == NULL ) {
			perror( infile );
			retval = EXIT_FAILURE;
			goto fail;
		}
	} else {
		ldifdummy.fp = stdin;
		ldiffp = &ldifdummy;
	}

	if ( debug ) ldif_debug = debug;

	ld = tool_conn_setup( dont, 0 );

	if ( !dont ) {
		tool_bind( ld );
	}

#ifdef LDAP_X_TXN
	if( txn ) {
		/* start transaction */
		rc = ldap_txn_start_s( ld, NULL, NULL, &txn_id );
		if( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_txn_start_s", rc, NULL, NULL, NULL, NULL );
			if( txn > 1 ) {
				retval = EXIT_FAILURE;
				goto fail;
			}
			txn = 0;
		}
	}
#endif

	if ( 0
#ifdef LDAP_X_TXN
		|| txn
#endif
		)
	{
#ifdef LDAP_X_TXN
		if( txn ) {
			c[i].ldctl_oid = LDAP_CONTROL_X_TXN_SPEC;
			c[i].ldctl_value = *txn_id;
			c[i].ldctl_iscritical = 1;
			i++;
		}
#endif
	}

	tool_server_controls( ld, c, i );

	rc = 0;
	retval = 0;
	lineno = 1;
	while (( rc == 0 || contoper ) && ( ldifrc = ldif_read_record( ldiffp, &nextline,
		&rbuf, &lmax )) > 0 )
	{
		if ( rejfp ) {
			len = strlen( rbuf );
			if (( rejbuf = (char *)ber_memalloc( len+1 )) == NULL ) {
				perror( "malloc" );
				retval = EXIT_FAILURE;
				goto fail;
			}
			memcpy( rejbuf, rbuf, len+1 );
		}

		rc = process_ldif_rec( rbuf, lineno );
		lineno = nextline+1;

		if ( rc ) retval = rc;
		if ( rc && rejfp ) {
			fprintf(rejfp, _("# Error: %s (%d)"), ldap_err2string(rc), rc);

			matched_msg = NULL;
			ldap_get_option(ld, LDAP_OPT_MATCHED_DN, &matched_msg);
			if ( matched_msg != NULL ) {
				if ( *matched_msg != '\0' ) {
					fprintf( rejfp, _(", matched DN: %s"), matched_msg );
				}
				ldap_memfree( matched_msg );
			}

			error_msg = NULL;
			ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, &error_msg);
			if ( error_msg != NULL ) {
				if ( *error_msg != '\0' ) {
					fprintf( rejfp, _(", additional info: %s"), error_msg );
				}
				ldap_memfree( error_msg );
			}
			fprintf( rejfp, "\n%s\n", rejbuf );
		}

		if (rejfp) ber_memfree( rejbuf );
	}
	ber_memfree( rbuf );

	if ( ldifrc < 0 )
		retval = LDAP_OTHER;

#ifdef LDAP_X_TXN
	if( retval == 0 && txn ) {
		rc = ldap_set_option( ld, LDAP_OPT_SERVER_CONTROLS, NULL );
		if ( rc != LDAP_OPT_SUCCESS ) {
			fprintf( stderr, "Could not unset controls for ldap_txn_end\n");
		}

		/* create transaction */
		rc = ldap_txn_end_s( ld, !txnabort, txn_id, NULL, NULL, NULL );
		if( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_txn_end_s", rc, NULL, NULL, NULL, NULL );
			retval = rc;
		}
	}
#endif

fail:;
	if ( rejfp != NULL ) {
		fclose( rejfp );
	}

	if ( ldiffp != NULL && ldiffp != &ldifdummy ) {
		ldif_close( ldiffp );
	}

	tool_exit( ld, retval );
}


static int
process_ldif_rec( char *rbuf, unsigned long linenum )
{
	char	*line, *dn, *newrdn, *newsup;
	int		rc, modop;
	int		expect_modop, expect_sep;
	int		deleteoldrdn;
	int		new_entry, delete_entry, got_all;
	LDAPMod	**pmods, *lm = NULL;
	int version;
	LDAPControl **pctrls;
	int i, j, k, lines, idn, nmods;
	struct berval *btype, *vals, **bvl, bv;
	char *freeval;
	unsigned char *mops = NULL;

	new_entry = ldapadd;

	rc = got_all = delete_entry = modop = expect_modop = 0;
	expect_sep = 0;
	version = 0;
	deleteoldrdn = 1;
	pmods = NULL;
	pctrls = NULL;
	dn = newrdn = newsup = NULL;

	lines = ldif_countlines( rbuf );
	btype = ber_memcalloc( 1, (lines+1)*2*sizeof(struct berval)+lines );
	if ( !btype )
		return LDAP_NO_MEMORY;

	vals = btype+lines+1;
	freeval = (char *)(vals+lines+1);
	i = -1;

	while ( rc == 0 && ( line = ldif_getline( &rbuf )) != NULL ) {
		int freev;

		if ( *line == '\n' || *line == '\0' ) {
			break;
		}

		++i;

		if ( line[0] == '-' && !line[1] ) {
			BER_BVZERO( btype+i );
			freeval[i] = 0;
			continue;
		}
	
		if ( ( rc = ldif_parse_line2( line, btype+i, vals+i, &freev ) ) < 0 ) {
			fprintf( stderr, _("%s: invalid format (line %lu) entry: \"%s\"\n"),
				prog, linenum+i, dn == NULL ? "" : dn );
			rc = LDAP_PARAM_ERROR;
			goto leave;
		}
		freeval[i] = freev;

		if ( dn == NULL ) {
			if ( linenum+i == 1 && BV_CASEMATCH( btype+i, &BV_VERSION )) {
				int	v;
				if( vals[i].bv_len == 0 || lutil_atoi( &v, vals[i].bv_val) != 0 || v != 1 ) {
					fprintf( stderr,
						_("%s: invalid version %s, line %lu (ignored)\n"),
						prog, vals[i].bv_val, linenum );
				}
				version++;

			} else if ( BV_CASEMATCH( btype+i, &BV_DN )) {
				dn = vals[i].bv_val;
				idn = i;
			}
			/* skip all lines until we see "dn:" */
		}
	}

	/* check to make sure there was a dn: line */
	if ( !dn ) {
		rc = 0;
		goto leave;
	}

	lines = i+1;

	if( lines == 0 ) {
		rc = 0;
		goto leave;
	}

	if( version && lines == 1 ) {
		rc = 0;
		goto leave;
	}

	i = idn+1;
	/* Check for "control" tag after dn and before changetype. */
	if ( BV_CASEMATCH( btype+i, &BV_CONTROL )) {
		/* Parse and add it to the list of controls */
		rc = parse_ldif_control( vals+i, &pctrls );
		if (rc != 0) {
			fprintf( stderr,
				_("%s: Error processing %s line, line %lu: %s\n"),
				prog, BV_CONTROL.bv_val, linenum+i, ldap_err2string(rc) );
		}
		i++;
		if ( i>= lines ) {
short_input:
			fprintf( stderr,
				_("%s: Expecting more input after %s line, line %lu\n"),
				prog, btype[i-1].bv_val, linenum+i );
			
			rc = LDAP_PARAM_ERROR;
			goto leave;
		}
	}

	/* Check for changetype */
	if ( BV_CASEMATCH( btype+i, &BV_CHANGETYPE )) {
#ifdef LIBERAL_CHANGETYPE_MODOP
		/* trim trailing spaces (and log warning ...) */
		int icnt;
		for ( icnt = vals[i].bv_len; --icnt > 0; ) {
			if ( !isspace( (unsigned char) vals[i].bv_val[icnt] ) ) {
				break;
			}
		}

		if ( ++icnt != vals[i].bv_len ) {
			fprintf( stderr, _("%s: illegal trailing space after"
				" \"%s: %s\" trimmed (line %lu, entry \"%s\")\n"),
				prog, BV_CHANGETYPE.bv_val, vals[i].bv_val, linenum+i, dn );
			vals[i].bv_val[icnt] = '\0';
		}
#endif /* LIBERAL_CHANGETYPE_MODOP */

		if ( BV_CASEMATCH( vals+i, &BV_MODIFYCT )) {
			new_entry = 0;
			expect_modop = 1;
		} else if ( BV_CASEMATCH( vals+i, &BV_ADDCT )) {
			new_entry = 1;
			modop = LDAP_MOD_ADD;
		} else if ( BV_CASEMATCH( vals+i, &BV_MODRDNCT )
			|| BV_CASEMATCH( vals+i, &BV_MODDNCT )
			|| BV_CASEMATCH( vals+i, &BV_RENAMECT ))
		{
			i++;
			if ( i >= lines )
				goto short_input;
			if ( !BV_CASEMATCH( btype+i, &BV_NEWRDN )) {
				fprintf( stderr, _("%s: expecting \"%s:\" but saw"
					" \"%s:\" (line %lu, entry \"%s\")\n"),
					prog, BV_NEWRDN.bv_val, btype[i].bv_val, linenum+i, dn );
				rc = LDAP_PARAM_ERROR;
				goto leave;
			}
			newrdn = vals[i].bv_val;
			i++;
			if ( i >= lines )
				goto short_input;
			if ( !BV_CASEMATCH( btype+i, &BV_DELETEOLDRDN )) {
				fprintf( stderr, _("%s: expecting \"%s:\" but saw"
					" \"%s:\" (line %lu, entry \"%s\")\n"),
					prog, BV_DELETEOLDRDN.bv_val, btype[i].bv_val, linenum+i, dn );
				rc = LDAP_PARAM_ERROR;
				goto leave;
			}
			deleteoldrdn = ( vals[i].bv_val[0] == '0' ) ? 0 : 1;
			i++;
			if ( i < lines ) {
				if ( !BV_CASEMATCH( btype+i, &BV_NEWSUP )) {
					fprintf( stderr, _("%s: expecting \"%s:\" but saw"
						" \"%s:\" (line %lu, entry \"%s\")\n"),
						prog, BV_NEWSUP.bv_val, btype[i].bv_val, linenum+i, dn );
					rc = LDAP_PARAM_ERROR;
					goto leave;
				}
				newsup = vals[i].bv_val;
				i++;
			}
			got_all = 1;
		} else if ( BV_CASEMATCH( vals+i, &BV_DELETECT )) {
			got_all = delete_entry = 1;
		} else {
			fprintf( stderr,
				_("%s:  unknown %s \"%s\" (line %lu, entry \"%s\")\n"),
				prog, BV_CHANGETYPE.bv_val, vals[i].bv_val, linenum+i, dn );
			rc = LDAP_PARAM_ERROR;
			goto leave;
		}
		i++;
	} else if ( ldapadd ) {		/*  missing changetype => add */
		new_entry = 1;
		modop = LDAP_MOD_ADD;
	} else {
		expect_modop = 1;	/* missing changetype => modify */
	}

	if ( got_all ) {
		if ( i < lines ) {
			fprintf( stderr,
				_("%s: extra lines at end (line %lu, entry \"%s\")\n"),
				prog, linenum+i, dn );
			rc = LDAP_PARAM_ERROR;
			goto leave;
		}
		goto doit;
	}

	nmods = lines - i;
	idn = i;

	if ( new_entry ) {
		int fv;

		/* Make sure all attributes with multiple values are contiguous */
		for (; i<lines; i++) {
			for (j=i+1; j<lines; j++) {
				if ( !btype[j].bv_val ) {
					fprintf( stderr,
						_("%s: missing attributeDescription (line %lu, entry \"%s\")\n"),
						prog, linenum+j, dn );
					rc = LDAP_PARAM_ERROR;
					goto leave;
				}
				if ( BV_CASEMATCH( btype+i, btype+j )) {
					nmods--;
					/* out of order, move intervening attributes down */
					if ( j != i+1 ) {
						bv = vals[j];
						fv = freeval[j];
						for (k=j; k>i; k--) {
							btype[k] = btype[k-1];
							vals[k] = vals[k-1];
							freeval[k] = freeval[k-1];
						}
						k++;
						btype[k] = btype[i];
						vals[k] = bv;
						freeval[k] = fv;
					}
					i++;
				}
			}
		}
		/* Allocate space for array of mods, array of pointers to mods,
		 * and array of pointers to values, allowing for NULL terminators
		 * for the pointer arrays...
		 */
		lm = ber_memalloc( nmods * sizeof(LDAPMod) +
			(nmods+1) * sizeof(LDAPMod*) +
			(lines + nmods - idn) * sizeof(struct berval *));
		pmods = (LDAPMod **)(lm+nmods);
		bvl = (struct berval **)(pmods+nmods+1);

		j = 0;
		k = -1;
		BER_BVZERO(&bv);
		for (i=idn; i<lines; i++) {
			if ( BV_CASEMATCH( btype+i, &BV_DN )) {
				fprintf( stderr, _("%s: attributeDescription \"%s\":"
					" (possible missing newline"
						" after line %lu, entry \"%s\"?)\n"),
					prog, btype[i].bv_val, linenum+i - 1, dn );
			}
			if ( !BV_CASEMATCH( btype+i, &bv )) {
				bvl[k++] = NULL;
				bv = btype[i];
				lm[j].mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
				lm[j].mod_type = bv.bv_val;
				lm[j].mod_bvalues = bvl+k;
				pmods[j] = lm+j;
				j++;
			}
			bvl[k++] = vals+i;
		}
		bvl[k] = NULL;
		pmods[j] = NULL;
		goto doit;
	}

	mops = ber_memalloc( lines+1 );
	mops[lines] = M_SEP;
	mops[i-1] = M_SEP;

	for ( ; i<lines; i++ ) {
		if ( expect_modop ) {
#ifdef LIBERAL_CHANGETYPE_MODOP
			/* trim trailing spaces (and log warning ...) */
		    int icnt;
		    for ( icnt = vals[i].bv_len; --icnt > 0; ) {
				if ( !isspace( (unsigned char) vals[i].bv_val[icnt] ) ) break;
			}
    
			if ( ++icnt != vals[i].bv_len ) {
				fprintf( stderr, _("%s: illegal trailing space after"
					" \"%s: %s\" trimmed (line %lu, entry \"%s\")\n"),
					prog, type, vals[i].bv_val, linenum+i, dn );
				vals[i].bv_val[icnt] = '\0';
			}
#endif /* LIBERAL_CHANGETYPE_MODOP */

			expect_modop = 0;
			expect_sep = 1;
			if ( BV_CASEMATCH( btype+i, &BV_MODOPADD )) {
				modop = LDAP_MOD_ADD;
				mops[i] = M_SEP;
				nmods--;
			} else if ( BV_CASEMATCH( btype+i, &BV_MODOPREPLACE )) {
			/* defer handling these since they might have no values.
			 * Use the BVALUES flag to signal that these were
			 * deferred. If values are provided later, this
			 * flag will be switched off.
			 */
				modop = LDAP_MOD_REPLACE;
				mops[i] = modop | LDAP_MOD_BVALUES;
				btype[i] = vals[i];
			} else if ( BV_CASEMATCH( btype+i, &BV_MODOPDELETE )) {
				modop = LDAP_MOD_DELETE;
				mops[i] = modop | LDAP_MOD_BVALUES;
				btype[i] = vals[i];
			} else if ( BV_CASEMATCH( btype+i, &BV_MODOPINCREMENT )) {
				modop = LDAP_MOD_INCREMENT;
				mops[i] = M_SEP;
				nmods--;
			} else {	/* no modify op: invalid LDIF */
				fprintf( stderr, _("%s: modify operation type is missing at"
					" line %lu, entry \"%s\"\n"),
					prog, linenum+i, dn );
				rc = LDAP_PARAM_ERROR;
				goto leave;
			}
			bv = vals[i];
		} else if ( expect_sep && BER_BVISEMPTY( btype+i )) {
			mops[i] = M_SEP;
			expect_sep = 0;
			expect_modop = 1;
			nmods--;
		} else {
			if ( !BV_CASEMATCH( btype+i, &bv )) {
				fprintf( stderr, _("%s: wrong attributeType at"
					" line %lu, entry \"%s\"\n"),
					prog, linenum+i, dn );
				rc = LDAP_PARAM_ERROR;
				goto leave;
			}
			mops[i] = modop;
			/* If prev op was deferred and matches this type,
			 * clear the flag
			 */
			if ( (mops[i-1] & LDAP_MOD_BVALUES)
				&& BV_CASEMATCH( btype+i, btype+i-1 ))
			{
				mops[i-1] = M_SEP;
				nmods--;
			}
		}
	}

#if 0	/* we should faithfully encode the LDIF, not combine */
	/* Make sure all modops with multiple values are contiguous */
	for (i=idn; i<lines; i++) {
		if ( mops[i] == M_SEP )
			continue;
		for (j=i+1; j<lines; j++) {
			if ( mops[j] == M_SEP || mops[i] != mops[j] )
				continue;
			if ( BV_CASEMATCH( btype+i, btype+j )) {
				nmods--;
				/* out of order, move intervening attributes down */
				if ( j != i+1 ) {
					int c;
					struct berval bv;
					char fv;

					c = mops[j];
					bv = vals[j];
					fv = freeval[j];
					for (k=j; k>i; k--) {
						btype[k] = btype[k-1];
						vals[k] = vals[k-1];
						freeval[k] = freeval[k-1];
						mops[k] = mops[k-1];
					}
					k++;
					btype[k] = btype[i];
					vals[k] = bv;
					freeval[k] = fv;
					mops[k] = c;
				}
				i++;
			}
		}
	}
#endif

	/* Allocate space for array of mods, array of pointers to mods,
	 * and array of pointers to values, allowing for NULL terminators
	 * for the pointer arrays...
	 */
	lm = ber_memalloc( nmods * sizeof(LDAPMod) +
		(nmods+1) * sizeof(LDAPMod*) +
		(lines + nmods - idn) * sizeof(struct berval *));
	pmods = (LDAPMod **)(lm+nmods);
	bvl = (struct berval **)(pmods+nmods+1);

	j = 0;
	k = -1;
	BER_BVZERO(&bv);
	mops[idn-1] = M_SEP;
	for (i=idn; i<lines; i++) {
		if ( mops[i] == M_SEP )
			continue;
		if ( mops[i] != mops[i-1] || !BV_CASEMATCH( btype+i, &bv )) {
			bvl[k++] = NULL;
			bv = btype[i];
			lm[j].mod_op = mops[i] | LDAP_MOD_BVALUES;
			lm[j].mod_type = bv.bv_val;
			if ( mops[i] & LDAP_MOD_BVALUES ) {
				lm[j].mod_bvalues = NULL;
			} else {
				lm[j].mod_bvalues = bvl+k;
			}
			pmods[j] = lm+j;
			j++;
		}
		bvl[k++] = vals+i;
	}
	bvl[k] = NULL;
	pmods[j] = NULL;

doit:
	/* If default controls are set (as with -M option) and controls are
	   specified in the LDIF file, we must add the default controls to
	   the list of controls sent with the ldap operation.
	*/
	if ( rc == 0 ) {
		if (pctrls) {
			LDAPControl **defctrls = NULL;   /* Default server controls */
			LDAPControl **newctrls = NULL;
			ldap_get_option(ld, LDAP_OPT_SERVER_CONTROLS, &defctrls);
			if (defctrls) {
				int npc=0;                       /* Num of LDIF controls */
				int ndefc=0;                     /* Num of default controls */
				while (pctrls[npc]) npc++;       /* Count LDIF controls */
				while (defctrls[ndefc]) ndefc++; /* Count default controls */
				newctrls = ber_memrealloc(pctrls,
					(npc+ndefc+1)*sizeof(LDAPControl*));

				if (newctrls == NULL) {
					rc = LDAP_NO_MEMORY;
				} else {
					int i;
					pctrls = newctrls;
					for (i=npc; i<npc+ndefc; i++) {
						pctrls[i] = ldap_control_dup(defctrls[i-npc]);
						if (pctrls[i] == NULL) {
							rc = LDAP_NO_MEMORY;
							break;
						}
					}
					pctrls[npc+ndefc] = NULL;
				}
				ldap_controls_free(defctrls);  /* Must be freed by library */
			}
		}
	}

	if ( rc == 0 ) {
		if ( delete_entry ) {
			rc = dodelete( dn, pctrls );
		} else if ( newrdn != NULL ) {
			rc = dorename( dn, newrdn, newsup, deleteoldrdn, pctrls );
		} else {
			rc = domodify( dn, pmods, pctrls, new_entry );
		}

		if ( rc == LDAP_SUCCESS ) {
			rc = 0;
		}
	}

leave:
    if (pctrls != NULL) {
    	ldap_controls_free( pctrls );
	}
	if ( lm != NULL ) {
		ber_memfree( lm );
	}
	if ( mops != NULL ) {
		ber_memfree( mops );
	}
	for (i=lines-1; i>=0; i--)
		if ( freeval[i] ) ber_memfree( vals[i].bv_val );
	ber_memfree( btype );

	return( rc );
}

/* Parse an LDIF control line of the form
      control:  oid  [true/false]  [: value]              or
      control:  oid  [true/false]  [:: base64-value]      or
      control:  oid  [true/false]  [:< url]
   The control is added to the list of controls in *ppctrls.
*/      
static int
parse_ldif_control(
	struct berval *bval,
	LDAPControl ***ppctrls )
{
	char *oid = NULL;
	int criticality = 0;   /* Default is false if not present */
	int i, rc=0;
	char *s, *oidStart;
	LDAPControl *newctrl = NULL;
	LDAPControl **pctrls = NULL;
	struct berval type, bv = BER_BVNULL;
	int freeval = 0;

	if (ppctrls) pctrls = *ppctrls;
	/* OID should come first. Validate and extract it. */
	s = bval->bv_val;
	if (*s == 0) return ( LDAP_PARAM_ERROR );
	oidStart = s;
	while (isdigit((unsigned char)*s) || *s == '.') {
		s++;                           /* OID should be digits or . */
	}
	if (s == oidStart) { 
		return ( LDAP_PARAM_ERROR );   /* OID was not present */
	}
	if (*s) {                          /* End of OID should be space or NULL */
		if (!isspace((unsigned char)*s)) {
			return ( LDAP_PARAM_ERROR ); /* else OID contained invalid chars */
		}
		*s++ = 0;                    /* Replace space with null to terminate */
	}

	oid = ber_strdup(oidStart);
	if (oid == NULL) return ( LDAP_NO_MEMORY );

	/* Optional Criticality field is next. */
	while (*s && isspace((unsigned char)*s)) {
		s++;                         /* Skip white space before criticality */
	}
	if (strncasecmp(s, "true", 4) == 0) {
		criticality = 1;
		s += 4;
	} 
	else if (strncasecmp(s, "false", 5) == 0) {
		criticality = 0;
		s += 5;
	}

	/* Optional value field is next */
	while (*s && isspace((unsigned char)*s)) {
		s++;                         /* Skip white space before value */
	}
	if (*s) {
		if (*s != ':') {           /* If value is present, must start with : */
			rc = LDAP_PARAM_ERROR;
			goto cleanup;
		}

		/* Back up so value is in the form
		     a: value
		     a:: base64-value
		     a:< url
		   Then we can use ldif_parse_line2 to extract and decode the value
		*/
		s--;
		*s = 'a';

		rc = ldif_parse_line2(s, &type, &bv, &freeval);
		if (rc < 0) {
			rc = LDAP_PARAM_ERROR;
			goto cleanup;
		}
    }

	/* Create a new LDAPControl structure. */
	newctrl = (LDAPControl *)ber_memalloc(sizeof(LDAPControl));
	if ( newctrl == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto cleanup;
	}
	newctrl->ldctl_oid = oid;
	oid = NULL;
	newctrl->ldctl_iscritical = criticality;
	if ( freeval )
		newctrl->ldctl_value = bv;
	else
		ber_dupbv( &newctrl->ldctl_value, &bv );

	/* Add the new control to the passed-in list of controls. */
	i = 0;
	if (pctrls) {
		while ( pctrls[i] ) {    /* Count the # of controls passed in */
			i++;
		}
	}
	/* Allocate 1 more slot for the new control and 1 for the NULL. */
	pctrls = (LDAPControl **) ber_memrealloc(pctrls,
		(i+2)*(sizeof(LDAPControl *)));
	if (pctrls == NULL) {
		rc = LDAP_NO_MEMORY;
		goto cleanup;
	}
	pctrls[i] = newctrl;
	newctrl = NULL;
	pctrls[i+1] = NULL;
	*ppctrls = pctrls;

cleanup:
	if (newctrl) {
		if (newctrl->ldctl_oid) ber_memfree(newctrl->ldctl_oid);
		if (newctrl->ldctl_value.bv_val) {
			ber_memfree(newctrl->ldctl_value.bv_val);
		}
		ber_memfree(newctrl);
	}
	if (oid) ber_memfree(oid);

	return( rc );
}


static int
domodify(
	const char *dn,
	LDAPMod **pmods,
	LDAPControl **pctrls,
	int newentry )
{
	int			rc, i, j, k, notascii, op;
	struct berval	*bvp;

	if ( dn == NULL ) {
		fprintf( stderr, _("%s: no DN specified\n"), prog );
		return( LDAP_PARAM_ERROR );
	}

	if ( pmods == NULL ) {
		/* implement "touch" (empty sequence)
		 * modify operation (note that there
		 * is no symmetry with the UNIX command,
		 * since \"touch\" on a non-existent entry
		 * will fail)*/
		printf( "warning: no attributes to %sadd (entry=\"%s\")\n",
			newentry ? "" : "change or ", dn );

	} else {
		for ( i = 0; pmods[ i ] != NULL; ++i ) {
			op = pmods[ i ]->mod_op & ~LDAP_MOD_BVALUES;
			if( op == LDAP_MOD_ADD && ( pmods[i]->mod_bvalues == NULL )) {
				fprintf( stderr,
					_("%s: attribute \"%s\" has no values (entry=\"%s\")\n"),
					prog, pmods[i]->mod_type, dn );
				return LDAP_PARAM_ERROR;
			}
		}

		if ( verbose ) {
			for ( i = 0; pmods[ i ] != NULL; ++i ) {
				op = pmods[ i ]->mod_op & ~LDAP_MOD_BVALUES;
				printf( "%s %s:\n",
					op == LDAP_MOD_REPLACE ? _("replace") :
						op == LDAP_MOD_ADD ?  _("add") :
							op == LDAP_MOD_INCREMENT ?  _("increment") :
								op == LDAP_MOD_DELETE ?  _("delete") :
									_("unknown"),
					pmods[ i ]->mod_type );
	
				if ( pmods[ i ]->mod_bvalues != NULL ) {
					for ( j = 0; pmods[ i ]->mod_bvalues[ j ] != NULL; ++j ) {
						bvp = pmods[ i ]->mod_bvalues[ j ];
						notascii = 0;
						for ( k = 0; (unsigned long) k < bvp->bv_len; ++k ) {
							if ( !isascii( bvp->bv_val[ k ] )) {
								notascii = 1;
								break;
							}
						}
						if ( notascii ) {
							printf( _("\tNOT ASCII (%ld bytes)\n"), bvp->bv_len );
						} else {
							printf( "\t%s\n", bvp->bv_val );
						}
					}
				}
			}
		}
	}

	if ( newentry ) {
		printf( "%sadding new entry \"%s\"\n", dont ? "!" : "", dn );
	} else {
		printf( "%smodifying entry \"%s\"\n", dont ? "!" : "", dn );
	}

	if ( !dont ) {
		int	msgid;
		if ( newentry ) {
			rc = ldap_add_ext( ld, dn, pmods, pctrls, NULL, &msgid );
		} else {
			rc = ldap_modify_ext( ld, dn, pmods, pctrls, NULL, &msgid );
		}

		if ( rc != LDAP_SUCCESS ) {
			/* print error message about failed update including DN */
			fprintf( stderr, _("%s: update failed: %s\n"), prog, dn );
			tool_perror( newentry ? "ldap_add" : "ldap_modify",
				rc, NULL, NULL, NULL, NULL );
			goto done;
		}
		rc = process_response( ld, msgid,
			newentry ? LDAP_RES_ADD : LDAP_RES_MODIFY, dn );

		if ( verbose && rc == LDAP_SUCCESS ) {
			printf( _("modify complete\n") );
		}

	} else {
		rc = LDAP_SUCCESS;
	}

done:
	putchar( '\n' );
	return rc;
}


static int
dodelete(
	const char *dn,
	LDAPControl **pctrls )
{
	int	rc;
	int msgid;

	printf( _("%sdeleting entry \"%s\"\n"), dont ? "!" : "", dn );
	if ( !dont ) {
		rc = ldap_delete_ext( ld, dn, pctrls, NULL, &msgid );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, _("%s: delete failed: %s\n"), prog, dn );
			tool_perror( "ldap_delete", rc, NULL, NULL, NULL, NULL );
			goto done;
		}
		rc = process_response( ld, msgid, LDAP_RES_DELETE, dn );

		if ( verbose && rc == LDAP_SUCCESS ) {
			printf( _("delete complete\n") );
		}
	} else {
		rc = LDAP_SUCCESS;
	}

done:
	putchar( '\n' );
	return( rc );
}


static int
dorename(
	const char *dn,
	const char *newrdn,
	const char* newsup,
	int deleteoldrdn,
	LDAPControl **pctrls )
{
	int	rc;
	int msgid;

	printf( _("%smodifying rdn of entry \"%s\"\n"), dont ? "!" : "", dn );
	if ( verbose ) {
		printf( _("\tnew RDN: \"%s\" (%skeep existing values)\n"),
			newrdn, deleteoldrdn ? _("do not ") : "" );
	}
	if ( !dont ) {
		rc = ldap_rename( ld, dn, newrdn, newsup, deleteoldrdn,
			pctrls, NULL, &msgid );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, _("%s: rename failed: %s\n"), prog, dn );
			tool_perror( "ldap_rename", rc, NULL, NULL, NULL, NULL );
			goto done;
		}
		rc = process_response( ld, msgid, LDAP_RES_RENAME, dn );

		if ( verbose && rc == LDAP_SUCCESS ) {
			printf( _("rename complete\n") );
		}
	} else {
		rc = LDAP_SUCCESS;
	}

done:
	putchar( '\n' );
	return( rc );
}

static const char *
res2str( int res ) {
	switch ( res ) {
	case LDAP_RES_ADD:
		return "ldap_add";
	case LDAP_RES_DELETE:
		return "ldap_delete";
	case LDAP_RES_MODIFY:
		return "ldap_modify";
	case LDAP_RES_MODRDN:
		return "ldap_rename";
	default:
		assert( 0 );
	}

	return "ldap_unknown";
}

static int process_response(
	LDAP *ld,
	int msgid,
	int op,
	const char *dn )
{
	LDAPMessage	*res;
	int		rc = LDAP_OTHER, msgtype;
	struct timeval	tv = { 0, 0 };
	int		err;
	char		*text = NULL, *matched = NULL, **refs = NULL;
	LDAPControl	**ctrls = NULL;

	for ( ; ; ) {
		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		rc = ldap_result( ld, msgid, LDAP_MSG_ALL, &tv, &res );
		if ( tool_check_abandon( ld, msgid ) ) {
			return LDAP_CANCELLED;
		}

		if ( rc == -1 ) {
			ldap_get_option( ld, LDAP_OPT_RESULT_CODE, &rc );
			tool_perror( "ldap_result", rc, NULL, NULL, NULL, NULL );
			return rc;
		}

		if ( rc != 0 ) {
			break;
		}
	}

	msgtype = ldap_msgtype( res );

	rc = ldap_parse_result( ld, res, &err, &matched, &text, &refs, &ctrls, 1 );
	if ( rc == LDAP_SUCCESS ) rc = err;

#ifdef LDAP_X_TXN
	if ( rc == LDAP_X_TXN_SPECIFY_OKAY ) {
		rc = LDAP_SUCCESS;
	} else
#endif
	if ( rc != LDAP_SUCCESS ) {
		tool_perror( res2str( op ), rc, NULL, matched, text, refs );
	} else if ( msgtype != op ) {
		fprintf( stderr, "%s: msgtype: expected %d got %d\n",
			res2str( op ), op, msgtype );
		rc = LDAP_OTHER;
	}

	if ( text ) ldap_memfree( text );
	if ( matched ) ldap_memfree( matched );
	if ( refs ) ber_memvfree( (void **)refs );

	if ( ctrls ) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}

	return rc;
}
