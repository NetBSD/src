/* $OpenLDAP: pkg/ldap/servers/slapd/slapadd.c,v 1.36.2.7 2008/04/14 21:15:47 quanah Exp $ */
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
 *    Pierangelo Masarati
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lber.h>
#include <ldif.h>
#include <lutil.h>

#include "slapcommon.h"

static char csnbuf[ LDAP_LUTIL_CSNSTR_BUFSIZE ];
static char maxcsnbuf[ LDAP_LUTIL_CSNSTR_BUFSIZE * ( SLAP_SYNC_SID_MAX + 1 ) ];

int
slapadd( int argc, char **argv )
{
	char *buf = NULL;
	const char *text;
	char textbuf[SLAP_TEXT_BUFLEN] = { '\0' };
	size_t textlen = sizeof textbuf;
	const char *progname = "slapadd";

	struct berval csn;
	struct berval maxcsn[ SLAP_SYNC_SID_MAX + 1 ];
	unsigned long sid;
	struct berval bvtext;
	Attribute *attr;
	Entry *ctxcsn_e;
	ID	ctxcsn_id, id;
	OperationBuffer opbuf;
	Operation *op;

	int match;
	int checkvals;
	int lineno, nextline;
	int lmax;
	int rc = EXIT_SUCCESS;
	int manage = 0;	

	/* default "000" */
	csnsid = 0;

	slap_tool_init( progname, SLAPADD, argc, argv );

	memset( &opbuf, 0, sizeof(opbuf) );
	op = &opbuf.ob_op;
	op->o_hdr = &opbuf.ob_hdr;

	if( !be->be_entry_open ||
		!be->be_entry_close ||
		!be->be_entry_put ||
		(update_ctxcsn &&
		 (!be->be_dn2id_get ||
		  !be->be_entry_get ||
		  !be->be_entry_modify)) )
	{
		fprintf( stderr, "%s: database doesn't support necessary operations.\n",
			progname );
		if ( dryrun ) {
			fprintf( stderr, "\t(dry) continuing...\n" );

		} else {
			exit( EXIT_FAILURE );
		}
	}

	checkvals = (slapMode & SLAP_TOOL_QUICK) ? 0 : 1;

	lmax = 0;
	nextline = 0;

	/* enforce schema checking unless not disabled */
	if ( (slapMode & SLAP_TOOL_NO_SCHEMA_CHECK) == 0) {
		SLAP_DBFLAGS(be) &= ~(SLAP_DBFLAG_NO_SCHEMA_CHECK);
	}

	if( !dryrun && be->be_entry_open( be, 1 ) != 0 ) {
		fprintf( stderr, "%s: could not open database.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	if ( update_ctxcsn ) {
		maxcsn[ 0 ].bv_val = maxcsnbuf;
		for ( sid = 1; sid <= SLAP_SYNC_SID_MAX; sid++ ) {
			maxcsn[ sid ].bv_val = maxcsn[ sid - 1 ].bv_val + LDAP_LUTIL_CSNSTR_BUFSIZE;
			maxcsn[ sid ].bv_len = 0;
		}
	}

	/* nextline is the line number of the end of the current entry */
	for( lineno=1; ldif_read_record( ldiffp, &nextline, &buf, &lmax );
		lineno=nextline+1 ) {
		Entry *e;

		if ( lineno < jumpline )
			continue;

		e = str2entry2( buf, checkvals );

		/*
		 * Initialize text buffer
		 */
		bvtext.bv_len = textlen;
		bvtext.bv_val = textbuf;
		bvtext.bv_val[0] = '\0';

		if( e == NULL ) {
			fprintf( stderr, "%s: could not parse entry (line=%d)\n",
				progname, lineno );
			rc = EXIT_FAILURE;
			if( continuemode ) continue;
			break;
		}

		/* make sure the DN is not empty */
		if( BER_BVISEMPTY( &e->e_nname ) &&
			!BER_BVISEMPTY( be->be_nsuffix )) {
			fprintf( stderr, "%s: empty dn=\"%s\" (line=%d)\n",
				progname, e->e_dn, lineno );
			rc = EXIT_FAILURE;
			entry_free( e );
			if( continuemode ) continue;
			break;
		}

		/* check backend */
		if( select_backend( &e->e_nname, nosubordinates )
			!= be )
		{
			fprintf( stderr, "%s: line %d: "
				"database (%s) not configured to hold \"%s\"\n",
				progname, lineno,
				be ? be->be_suffix[0].bv_val : "<none>",
				e->e_dn );
			fprintf( stderr, "%s: line %d: "
				"database (%s) not configured to hold \"%s\"\n",
				progname, lineno,
				be ? be->be_nsuffix[0].bv_val : "<none>",
				e->e_ndn );
			rc = EXIT_FAILURE;
			entry_free( e );
			if( continuemode ) continue;
			break;
		}

		{
			Attribute *oc = attr_find( e->e_attrs,
				slap_schema.si_ad_objectClass );

			if( oc == NULL ) {
				fprintf( stderr, "%s: dn=\"%s\" (line=%d): %s\n",
					progname, e->e_dn, lineno,
					"no objectClass attribute");
				rc = EXIT_FAILURE;
				entry_free( e );
				if( continuemode ) continue;
				break;
			}

			/* check schema */
			op->o_bd = be;

			if ( (slapMode & SLAP_TOOL_NO_SCHEMA_CHECK) == 0) {
				rc = entry_schema_check( op, e, NULL, manage, 1,
					&text, textbuf, textlen );

				if( rc != LDAP_SUCCESS ) {
					fprintf( stderr, "%s: dn=\"%s\" (line=%d): (%d) %s\n",
						progname, e->e_dn, lineno, rc, text );
					rc = EXIT_FAILURE;
					entry_free( e );
					if( continuemode ) continue;
					break;
				}
				textbuf[ 0 ] = '\0';
			}
		}

		if ( SLAP_LASTMOD(be) ) {
			time_t now = slap_get_time();
			char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
			struct berval vals[ 2 ];

			struct berval name, timestamp;

			struct berval nvals[ 2 ];
			struct berval nname;
			char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];

			vals[1].bv_len = 0;
			vals[1].bv_val = NULL;

			nvals[1].bv_len = 0;
			nvals[1].bv_val = NULL;

			csn.bv_len = lutil_csnstr( csnbuf, sizeof( csnbuf ), csnsid, 0 );
			csn.bv_val = csnbuf;

			timestamp.bv_val = timebuf;
			timestamp.bv_len = sizeof(timebuf);

			slap_timestamp( &now, &timestamp );

			if ( BER_BVISEMPTY( &be->be_rootndn ) ) {
				BER_BVSTR( &name, SLAPD_ANONYMOUS );
				nname = name;
			} else {
				name = be->be_rootdn;
				nname = be->be_rootndn;
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_entryUUID )
				== NULL )
			{
				vals[0].bv_len = lutil_uuidstr( uuidbuf, sizeof( uuidbuf ) );
				vals[0].bv_val = uuidbuf;
				attr_merge_normalize_one( e, slap_schema.si_ad_entryUUID, vals, NULL );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_creatorsName )
				== NULL )
			{
				vals[0] = name;
				nvals[0] = nname;
				attr_merge( e, slap_schema.si_ad_creatorsName, vals, nvals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_createTimestamp )
				== NULL )
			{
				vals[0] = timestamp;
				attr_merge( e, slap_schema.si_ad_createTimestamp, vals, NULL );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_entryCSN )
				== NULL )
			{
				vals[0] = csn;
				attr_merge( e, slap_schema.si_ad_entryCSN, vals, NULL );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_modifiersName )
				== NULL )
			{
				vals[0] = name;
				nvals[0] = nname;
				attr_merge( e, slap_schema.si_ad_modifiersName, vals, nvals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_modifyTimestamp )
				== NULL )
			{
				vals[0] = timestamp;
				attr_merge( e, slap_schema.si_ad_modifyTimestamp, vals, NULL );
			}

			if ( update_ctxcsn ) {
				int rc_sid;

				attr = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
				assert( attr != NULL );

				rc_sid = slap_parse_csn_sid( &attr->a_nvals[ 0 ] );
				if ( rc_sid < 0 ) {
					Debug( LDAP_DEBUG_ANY, "%s: could not "
						"extract SID from entryCSN=%s\n",
						progname, attr->a_nvals[ 0 ].bv_val, 0 );

				} else {
					assert( rc_sid <= SLAP_SYNC_SID_MAX );

					sid = (unsigned)rc_sid;
					if ( maxcsn[ sid ].bv_len != 0 ) {
						match = 0;
						value_match( &match, slap_schema.si_ad_entryCSN,
							slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							&maxcsn[ sid ], &attr->a_nvals[0], &text );
					} else {
						match = -1;
					}
					if ( match < 0 ) {
						strcpy( maxcsn[ sid ].bv_val, attr->a_nvals[0].bv_val );
						maxcsn[ sid ].bv_len = attr->a_nvals[0].bv_len;
					}
				}
			}
		}

		if ( !dryrun ) {
			id = be->be_entry_put( be, e, &bvtext );
			if( id == NOID ) {
				fprintf( stderr, "%s: could not add entry dn=\"%s\" "
								 "(line=%d): %s\n", progname, e->e_dn,
								 lineno, bvtext.bv_val );
				rc = EXIT_FAILURE;
				entry_free( e );
				if( continuemode ) continue;
				break;
			}
			if ( verbose )
				fprintf( stderr, "added: \"%s\" (%08lx)\n",
					e->e_dn, (long) id );
		} else {
			if ( verbose )
				fprintf( stderr, "added: \"%s\"\n",
					e->e_dn );
		}

		entry_free( e );
	}

	bvtext.bv_len = textlen;
	bvtext.bv_val = textbuf;
	bvtext.bv_val[0] = '\0';

	if ( rc == EXIT_SUCCESS && update_ctxcsn && !dryrun && sid != SLAP_SYNC_SID_MAX + 1 ) {
		ctxcsn_id = be->be_dn2id_get( be, be->be_nsuffix );
		if ( ctxcsn_id == NOID ) {
			fprintf( stderr, "%s: context entry is missing\n", progname );
			rc = EXIT_FAILURE;
		} else {
			ctxcsn_e = be->be_entry_get( be, ctxcsn_id );
			if ( ctxcsn_e != NULL ) {
				Entry *e = entry_dup( ctxcsn_e );
				int change;
				attr = attr_find( e->e_attrs, slap_schema.si_ad_contextCSN );
				if ( attr ) {
					int		i;

					change = 0;

					for ( i = 0; !BER_BVISNULL( &attr->a_nvals[ i ] ); i++ ) {
						int rc_sid;

						rc_sid = slap_parse_csn_sid( &attr->a_nvals[ i ] );
						if ( rc_sid < 0 ) {
							Debug( LDAP_DEBUG_ANY,
								"%s: unable to extract SID "
								"from #%d contextCSN=%s\n",
								progname, i,
								attr->a_nvals[ i ].bv_val );
							continue;
						}

						assert( rc_sid <= SLAP_SYNC_SID_MAX );

						sid = (unsigned)rc_sid;

						if ( maxcsn[ sid ].bv_len == 0 ) {
							match = -1;

						} else {
							value_match( &match, slap_schema.si_ad_entryCSN,
								slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
								SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
								&maxcsn[ sid ], &attr->a_nvals[i], &text );
						}

						if ( match > 0 ) {
							change = 1;
						} else {
							AC_MEMCPY( maxcsn[ sid ].bv_val,
								attr->a_nvals[ i ].bv_val,
								attr->a_nvals[ i ].bv_len );
							maxcsn[ sid ].bv_val[ attr->a_nvals[ i ].bv_len ] = '\0';
							maxcsn[ sid ].bv_len = attr->a_nvals[ i ].bv_len;
						}
					}

					if ( change ) {
						if ( attr->a_nvals != attr->a_vals ) {
							ber_bvarray_free( attr->a_nvals );
						}
						attr->a_nvals = NULL;
						ber_bvarray_free( attr->a_vals );
						attr->a_vals = NULL;
						attr->a_numvals = 0;
					}
				} else {
					change = 1;
				}

				if ( change ) {
					for ( sid = 0; sid <= SLAP_SYNC_SID_MAX; sid++ ) {
						if ( maxcsn[ sid ].bv_len ) {
							attr_merge_one( e, slap_schema.si_ad_contextCSN,
								&maxcsn[ sid], NULL );
						}
					}

					ctxcsn_id = be->be_entry_modify( be, e, &bvtext );
					if( ctxcsn_id == NOID ) {
						fprintf( stderr, "%s: could not modify ctxcsn\n",
							progname);
						rc = EXIT_FAILURE;
					} else if ( verbose ) {
						fprintf( stderr, "modified: \"%s\" (%08lx)\n",
							e->e_dn, (long) ctxcsn_id );
					}
				}
				entry_free( e );
			}
		} 
	}

	ch_free( buf );

	if ( !dryrun ) {
		if( be->be_entry_close( be ) ) {
			rc = EXIT_FAILURE;
		}

		if( be->be_sync ) {
			be->be_sync( be );
		}
	}

	slap_tool_destroy();

	return rc;
}

