/*	$NetBSD: slapmodify.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: slapmodify.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include "ac/stdlib.h"

#include "ac/ctype.h"
#include "ac/string.h"
#include "ac/socket.h"
#include "ac/unistd.h"

#include "lber.h"
#include "ldif.h"
#include "lutil.h"
#include "lutil_meter.h"
#include <sys/stat.h>

#include "slapcommon.h"

extern int slap_DN_strict;	/* dn.c */

static char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];

int
slapmodify( int argc, char **argv )
{
	char *buf = NULL;
	const char *text;
	char textbuf[SLAP_TEXT_BUFLEN] = { '\0' };
	size_t textlen = sizeof textbuf;
	const char *progname = "slapmodify";

	struct berval csn;
	unsigned long sid;
	struct berval bvtext;
	ID id;
	OperationBuffer opbuf;
	Operation *op;

	int checkvals, ldifrc;
	unsigned long lineno, nextline;
	int lmax;
	int rc = EXIT_SUCCESS;

	int enable_meter = 0;
	lutil_meter_t meter;
	struct stat stat_buf;

	/* default "000" */
	csnsid = 0;

	if ( isatty (2) ) enable_meter = 1;
	slap_tool_init( progname, SLAPMODIFY, argc, argv );

	memset( &opbuf, 0, sizeof(opbuf) );
	op = &opbuf.ob_op;
	op->o_hdr = &opbuf.ob_hdr;
	op->o_bd = be;

	if ( !be->be_entry_open ||
		!be->be_entry_close ||
		!be->be_entry_put ||
		!be->be_dn2id_get ||
		!be->be_entry_get ||
		!be->be_entry_modify )
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

	/* enforce schema checking unless not disabled and allow unknown
	 * attributes otherwise */
	if ( (slapMode & SLAP_TOOL_NO_SCHEMA_CHECK) == 0) {
		SLAP_DBFLAGS(be) &= ~(SLAP_DBFLAG_NO_SCHEMA_CHECK);
	} else {
		slap_DN_strict = 0;
	}

	if( !dryrun && be->be_entry_open( be, 1 ) != 0 ) {
		fprintf( stderr, "%s: could not open database.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	(void)slap_tool_update_ctxcsn_init();

	if ( enable_meter 
#ifdef LDAP_DEBUG
		/* tools default to "none" */
		&& slap_debug == LDAP_DEBUG_NONE
#endif
		&& !fstat ( fileno ( ldiffp->fp ), &stat_buf )
		&& S_ISREG(stat_buf.st_mode) ) {
		enable_meter = !lutil_meter_open(
			&meter,
			&lutil_meter_text_display,
			&lutil_meter_linear_estimator,
			stat_buf.st_size);
	} else {
		enable_meter = 0;
	}

	/* nextline is the line number of the end of the current entry */
	for( lineno=1; ( ldifrc = ldif_read_record( ldiffp, &nextline, &buf, &lmax )) > 0;
		lineno=nextline+1 )
	{
		BackendDB *bd;
		Entry *e_orig = NULL, *e = NULL;
		struct berval rbuf;
		LDIFRecord lr;
		struct berval ndn = BER_BVNULL;
		int n;
		int is_oc = 0;
		int local_rc;
		int mod_err = 0;
		char *request = "(unknown)";

		ber_str2bv( buf, 0, 0, &rbuf );

		if ( lineno < jumpline )
			continue;

		if ( enable_meter )
			lutil_meter_update( &meter,
					 ftell( ldiffp->fp ),
					 0);

		/*
		 * Initialize text buffer
		 */
		bvtext.bv_len = textlen;
		bvtext.bv_val = textbuf;
		bvtext.bv_val[0] = '\0';

		local_rc = ldap_parse_ldif_record( &rbuf, lineno, &lr,
			"slapmodify", LDIF_NO_CONTROLS );

		if ( local_rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: could not parse entry (line=%lu)\n",
				progname, lineno );
			rc = EXIT_FAILURE;
			if( continuemode ) continue;
			break;
		}

		switch ( lr.lr_op ) {
		case LDAP_REQ_ADD:
			request = "add";
			break;

		case LDAP_REQ_MODIFY:
			request = "modify";
			break;

		case LDAP_REQ_DELETE:
			if ( be->be_entry_delete )
			{
				request = "delete";
				break;
			}
			/* backend does not support delete, fallthru */

		case LDAP_REQ_MODRDN:
			fprintf( stderr, "%s: request 0x%lx not supported (line=%lu)\n",
				progname, (unsigned long)lr.lr_op, lineno );
			rc = EXIT_FAILURE;
			goto cleanup;

		default:
			/* record skipped e.g. version: or comment or something we don't handle yet */
			goto cleanup;
		}

		local_rc = dnNormalize( 0, NULL, NULL, &lr.lr_dn, &ndn, NULL );
		if ( local_rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: DN=\"%s\" normalization failed (line=%lu)\n",
				progname, lr.lr_dn.bv_val, lineno );
			rc = EXIT_FAILURE;
			goto cleanup;
		}

		/* make sure the DN is not empty */
		if( BER_BVISEMPTY( &ndn ) &&
			!BER_BVISEMPTY( be->be_nsuffix ))
		{
			fprintf( stderr, "%s: line %lu: "
				"%s entry with empty dn=\"\"",
				progname, lineno, request );
			bd = select_backend( &ndn, nosubordinates );
			if ( bd ) {
				BackendDB *bdtmp;
				int dbidx = 0;
				LDAP_STAILQ_FOREACH( bdtmp, &backendDB, be_next ) {
					if ( bdtmp == bd ) break;
					dbidx++;
				}

				assert( bdtmp != NULL );
				
				fprintf( stderr, "; did you mean to use database #%d (%s)?",
					dbidx,
					bd->be_suffix[0].bv_val );

			}
			fprintf( stderr, "\n" );
			rc = EXIT_FAILURE;
			goto cleanup;
		}

		/* check backend */
		bd = select_backend( &ndn, nosubordinates );
		if ( bd != be ) {
			fprintf( stderr, "%s: line %lu: "
				"database #%d (%s) not configured to hold \"%s\"",
				progname, lineno,
				dbnum,
				be->be_suffix[0].bv_val,
				lr.lr_dn.bv_val );
			if ( bd ) {
				BackendDB *bdtmp;
				int dbidx = 0;
				LDAP_STAILQ_FOREACH( bdtmp, &backendDB, be_next ) {
					if ( bdtmp == bd ) break;
					dbidx++;
				}

				assert( bdtmp != NULL );
				
				fprintf( stderr, "; did you mean to use database #%d (%s)?",
					dbidx,
					bd->be_suffix[0].bv_val );

			} else {
				fprintf( stderr, "; no database configured for that naming context" );
			}
			fprintf( stderr, "\n" );
			rc = EXIT_FAILURE;
			goto cleanup;
		}

		/* get id and/or entry */
		switch ( lr.lr_op ) {
			case LDAP_REQ_ADD:
				e = entry_alloc();
				ber_dupbv( &e->e_name, &lr.lr_dn );
				ber_dupbv( &e->e_nname, &ndn );
				break;

			//case LDAP_REQ_MODRDN:
			case LDAP_REQ_DELETE:
			case LDAP_REQ_MODIFY:
				id = be->be_dn2id_get( be, &ndn );
				rc = (id == NOID);
				if ( rc == LDAP_SUCCESS && lr.lr_op != LDAP_REQ_DELETE ) {
					e_orig = be->be_entry_get( be, id );
					if ( e_orig )
						e = entry_dup( e_orig );
					rc = (e == NULL);
				}
				break;
		}

		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: no such entry \"%s\" in database (lineno=%lu)\n",
				progname, ndn.bv_val, lineno );
			rc = EXIT_FAILURE;
			goto cleanup;
		}

		if ( lr.lrop_mods ) {
			for ( n = 0; lr.lrop_mods && lr.lrop_mods[ n ] != NULL; n++ ) {
				LDAPMod *mod = lr.lrop_mods[ n ];
				Modification mods = { 0 };
				unsigned i = 0;
				int bin = (mod->mod_op & LDAP_MOD_BVALUES);
				int pretty = 0;
				int normalize = 0;

				local_rc = slap_str2ad( mod->mod_type, &mods.sm_desc, &text );
				/*
				 * Usually this would be a bad idea (way too dangerous, risks
				 * corrupting the DB), but ITS#7786 documents this as a last
				 * resort to fix cn=config and missing attributes are one of
				 * the possible issues we might encounter.
				 */
				if ( local_rc == LDAP_UNDEFINED_TYPE &&
						(slapMode & SLAP_TOOL_NO_SCHEMA_CHECK) ) {
					local_rc = slap_str2undef_ad( mod->mod_type, &mods.sm_desc, &text, 0 );
				}
				if ( local_rc != LDAP_SUCCESS ) {
					fprintf( stderr, "%s: slap_str2ad(\"%s\") failed for entry \"%s\" (%d: %s, lineno=%lu)\n",
						progname, mod->mod_type, lr.lr_dn.bv_val, local_rc, text, lineno );
					rc = EXIT_FAILURE;
					goto cleanup;
				}

				mods.sm_type = mods.sm_desc->ad_cname;

				if ( mods.sm_desc->ad_type->sat_syntax->ssyn_pretty ) {
					pretty = 1;

				} else {
					assert( mods.sm_desc->ad_type->sat_syntax->ssyn_validate != NULL );
				}

				if ( mods.sm_desc->ad_type->sat_equality &&
					mods.sm_desc->ad_type->sat_equality->smr_normalize )
				{
					normalize = 1;
				}

				if ( bin && mod->mod_bvalues ) {
					for ( i = 0; mod->mod_bvalues[ i ] != NULL; i++ )
						;

				} else if ( !bin && mod->mod_values ) {
					for ( i = 0; mod->mod_values[ i ] != NULL; i++ )
						;
				}

				if ( i != 0 )
				{
					mods.sm_values = ch_calloc( sizeof( struct berval ), i + 1 );
					if ( normalize ) {
						mods.sm_nvalues = ch_calloc( sizeof( struct berval ), i + 1 );
					} else {
						mods.sm_nvalues = NULL;
					}
				}
				mods.sm_numvals = i;

				for ( i = 0; i < mods.sm_numvals; i++ ) {
					struct berval bv;

					if ( bin ) {
						bv = *mod->mod_bvalues[ i ];
					} else {
						ber_str2bv( mod->mod_values[ i ], 0, 0, &bv );
					}

					if ( pretty ) {
						local_rc = ordered_value_pretty( mods.sm_desc,
						&bv, &mods.sm_values[i], NULL );

					} else {
						local_rc = ordered_value_validate( mods.sm_desc,
							&bv, 0 );
					}

					if ( local_rc != LDAP_SUCCESS ) {
						fprintf( stderr, "%s: DN=\"%s\": unable to %s attr=%s value #%d\n",
							progname, e->e_dn, pretty ? "prettify" : "validate",
							mods.sm_desc->ad_cname.bv_val, i );
						/* handle error */
						rc = EXIT_FAILURE;
						ber_bvarray_free( mods.sm_values );
						ber_bvarray_free( mods.sm_nvalues );
						goto cleanup;
					}

					if ( !pretty ) {
						ber_dupbv( &mods.sm_values[i], &bv );
					}

					if ( normalize ) {
						local_rc = ordered_value_normalize(
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							mods.sm_desc,
							mods.sm_desc->ad_type->sat_equality,
							&mods.sm_values[i], &mods.sm_nvalues[i],
							NULL );
						if ( local_rc != LDAP_SUCCESS ) {
							fprintf( stderr, "%s: DN=\"%s\": unable to normalize attr=%s value #%d\n",
								progname, e->e_dn, mods.sm_desc->ad_cname.bv_val, i );
							/* handle error */
							rc = EXIT_FAILURE;
							ber_bvarray_free( mods.sm_values );
							ber_bvarray_free( mods.sm_nvalues );
							goto cleanup;
						}
					}
				}

				mods.sm_op = (mod->mod_op & ~LDAP_MOD_BVALUES);
				mods.sm_flags = 0;

				if ( mods.sm_desc == slap_schema.si_ad_objectClass ) {
					is_oc = 1;
				}

				switch ( mods.sm_op ) {
				case LDAP_MOD_ADD:
					local_rc = modify_add_values( e, &mods,
						0, &text, textbuf, textlen );
					break;

				case LDAP_MOD_DELETE:
					local_rc = modify_delete_values( e, &mods,
						0, &text, textbuf, textlen );
					break;

				case LDAP_MOD_REPLACE:
					local_rc = modify_replace_values( e, &mods,
						0, &text, textbuf, textlen );
					break;

				case LDAP_MOD_INCREMENT:
					local_rc = modify_increment_values( e, &mods,
						0, &text, textbuf, textlen );
					break;
				}

				ber_bvarray_free( mods.sm_values );
				ber_bvarray_free( mods.sm_nvalues );

				if ( local_rc != LDAP_SUCCESS ) {
					fprintf( stderr, "%s: DN=\"%s\": unable to modify attr=%s\n",
						progname, e->e_dn, mods.sm_desc->ad_cname.bv_val );
					rc = EXIT_FAILURE;
					goto cleanup;
				}
			}

			rc = slap_tool_entry_check( progname, op, e, lineno, &text, textbuf, textlen );
			if ( rc != LDAP_SUCCESS ) {
				rc = EXIT_FAILURE;
				goto cleanup;
			}
		}

		if ( SLAP_LASTMOD(be) && e != NULL ) {
			time_t now = slap_get_time();
			char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
			struct berval vals[ 2 ];

			struct berval name, timestamp;

			struct berval nvals[ 2 ];
			struct berval nname;
			char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];

			Attribute *a;

			vals[1].bv_len = 0;
			vals[1].bv_val = NULL;

			nvals[1].bv_len = 0;
			nvals[1].bv_val = NULL;

			csn.bv_len = ldap_pvt_csnstr( csnbuf, sizeof( csnbuf ), csnsid, 0 );
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

			a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
			if ( a != NULL ) {
				if ( a->a_vals != a->a_nvals ) {
					SLAP_FREE( a->a_nvals[0].bv_val );
					SLAP_FREE( a->a_nvals );
				}
				SLAP_FREE( a->a_vals[0].bv_val );
				SLAP_FREE( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}
			vals[0].bv_len = lutil_uuidstr( uuidbuf, sizeof( uuidbuf ) );
			vals[0].bv_val = uuidbuf;
			attr_merge_normalize_one( e, slap_schema.si_ad_entryUUID, vals, NULL );

			a = attr_find( e->e_attrs, slap_schema.si_ad_creatorsName );
			if ( a == NULL ) {
				vals[0] = name;
				nvals[0] = nname;
				attr_merge( e, slap_schema.si_ad_creatorsName, vals, nvals );

			} else {
				ber_bvreplace( &a->a_vals[0], &name );
				ber_bvreplace( &a->a_nvals[0], &nname );
			}

			a = attr_find( e->e_attrs, slap_schema.si_ad_createTimestamp );
			if ( a == NULL ) {
				vals[0] = timestamp;
				attr_merge( e, slap_schema.si_ad_createTimestamp, vals, NULL );

			} else {
				ber_bvreplace( &a->a_vals[0], &timestamp );
			}

			a = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
			if ( a == NULL ) {
				vals[0] = csn;
				attr_merge( e, slap_schema.si_ad_entryCSN, vals, NULL );

			} else {
				ber_bvreplace( &a->a_vals[0], &csn );
			}

			a = attr_find( e->e_attrs, slap_schema.si_ad_modifiersName );
			if ( a == NULL ) {
				vals[0] = name;
				nvals[0] = nname;
				attr_merge( e, slap_schema.si_ad_modifiersName, vals, nvals );

			} else {
				ber_bvreplace( &a->a_vals[0], &name );
				ber_bvreplace( &a->a_nvals[0], &nname );
			}

			a = attr_find( e->e_attrs, slap_schema.si_ad_modifyTimestamp );
			if ( a == NULL ) {
				vals[0] = timestamp;
				attr_merge( e, slap_schema.si_ad_modifyTimestamp, vals, NULL );

			} else {
				ber_bvreplace( &a->a_vals[0], &timestamp );
			}
		}

		/* check schema, objectClass etc */

		if ( !dryrun ) {
			switch ( lr.lr_op ) {
			case LDAP_REQ_ADD:
				id = be->be_entry_put( be, e, &bvtext );
				rc = (id == NOID);
				break;

			case LDAP_REQ_MODIFY:
				id = be->be_entry_modify( be, e, &bvtext );
				rc = (id == NOID);
				break;

			case LDAP_REQ_DELETE:
				rc = be->be_entry_delete( be, &ndn, &bvtext );
				break;

			}

			if( rc != LDAP_SUCCESS ) {
				fprintf( stderr, "%s: could not %s entry dn=\"%s\" "
					"(line=%lu): %s\n", progname, request, ndn.bv_val,
					lineno, bvtext.bv_val );
				rc = EXIT_FAILURE;
				goto cleanup;
			}

			sid = slap_tool_update_ctxcsn_check( progname, e );

			if ( verbose )
				fprintf( stderr, "%s: \"%s\" (%08lx)\n",
					request, ndn.bv_val, (long) id );
		} else {
			if ( verbose )
				fprintf( stderr, "%s: \"%s\"\n",
					request, ndn.bv_val );
		}

cleanup:;
		ldap_ldif_record_done( &lr );
		SLAP_FREE( ndn.bv_val );
		if ( e ) entry_free( e );
		if ( e_orig ) be_entry_release_w( op, e_orig );
		if ( rc != LDAP_SUCCESS && !continuemode ) break;
	}

	if ( ldifrc < 0 )
		rc = EXIT_FAILURE;

	bvtext.bv_len = textlen;
	bvtext.bv_val = textbuf;
	bvtext.bv_val[0] = '\0';

	if ( enable_meter ) {
		lutil_meter_update( &meter, ftell( ldiffp->fp ), 1);
		lutil_meter_close( &meter );
	}

	if ( rc == EXIT_SUCCESS ) {
		rc = slap_tool_update_ctxcsn( progname, sid, &bvtext );
	}

	ch_free( buf );

	if ( !dryrun ) {
		if ( enable_meter ) {
			fprintf( stderr, "Closing DB..." );
		}
		if( be->be_entry_close( be ) ) {
			rc = EXIT_FAILURE;
		}

		if( be->be_sync ) {
			be->be_sync( be );
		}
		if ( enable_meter ) {
			fprintf( stderr, "\n" );
		}
	}

	if ( slap_tool_destroy())
		rc = EXIT_FAILURE;

	return rc;
}

