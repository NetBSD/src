/* $OpenLDAP: pkg/ldap/servers/slapd/back-sql/config.c,v 1.32.2.5 2008/02/11 23:26:48 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2008 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
 * Portions Copyright 2004 Mark Adamson.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Dmitry Kovalev for inclusion
 * by OpenLDAP Software.  Additional significant contributors include
 * Pierangelo Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include "ac/string.h"
#include <sys/types.h>

#include "slap.h"
#include "ldif.h"
#include "proto-sql.h"

static int
create_baseObject(
	BackendDB	*be,
	const char	*fname,
	int		lineno );

static int
read_baseObject(
	BackendDB	*be,
	const char	*fname );

int
backsql_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	backsql_info 	*bi = (backsql_info *)be->be_private;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_db_config()\n", 0, 0, 0 );
	assert( bi != NULL );
  
	if ( !strcasecmp( argv[ 0 ], "dbhost" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing hostname in \"dbhost\" directive\n",
				fname, lineno, 0 );
			return 1;
	    	}
		bi->sql_dbhost = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE,
			"<==backsql_db_config(): hostname=%s\n",
			bi->sql_dbhost, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "dbuser" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing username in \"dbuser\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_dbuser = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): dbuser=%s\n",
			bi->sql_dbuser, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "dbpasswd" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing password in \"dbpasswd\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_dbpasswd = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"dbpasswd=%s\n", /* bi->sql_dbpasswd */ "xxxx", 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "dbname" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing database name in \"dbname\" "
				"directive\n", fname, lineno, 0 );
			return 1;
		}
		bi->sql_dbname = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): dbname=%s\n",
			bi->sql_dbname, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "concat_pattern" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing pattern"
				"in \"concat_pattern\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		if ( backsql_split_pattern( argv[ 1 ], &bi->sql_concat_func, 2 ) ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"unable to parse pattern \"%s\"\n"
				"in \"concat_pattern\" directive\n",
				fname, lineno, argv[ 1 ] );
			return 1;
		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"concat_pattern=\"%s\"\n", argv[ 1 ], 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "subtree_cond" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing SQL condition "
				"in \"subtree_cond\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		ber_str2bv( argv[ 1 ], 0, 1, &bi->sql_subtree_cond );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"subtree_cond=%s\n", bi->sql_subtree_cond.bv_val, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "children_cond" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing SQL condition "
				"in \"children_cond\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		ber_str2bv( argv[ 1 ], 0, 1, &bi->sql_children_cond );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"children_cond=%s\n", bi->sql_children_cond.bv_val, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "dn_match_cond" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing SQL condition "
				"in \"dn_match_cond\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		ber_str2bv( argv[ 1 ], 0, 1, &bi->sql_dn_match_cond );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"children_cond=%s\n", bi->sql_dn_match_cond.bv_val, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "oc_query" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing SQL statement "
				"in \"oc_query\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_oc_query = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"oc_query=%s\n", bi->sql_oc_query, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "at_query" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing SQL statement "
				"in \"at_query\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_at_query = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"at_query=%s\n", bi->sql_at_query, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "insentry_stmt" ) ||
			!strcasecmp( argv[ 0 ], "insentry_query" ) )
	{
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing SQL statement "
				"in \"insentry_stmt\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_insentry_stmt = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"insentry_stmt=%s\n", bi->sql_insentry_stmt, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "create_needs_select" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"create_needs_select\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_CREATE_NEEDS_SELECT;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_CREATE_NEEDS_SELECT;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"create_needs_select\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"create_needs_select =%s\n", 
			BACKSQL_CREATE_NEEDS_SELECT( bi ) ? "yes" : "no",
			0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "upper_func" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing function name "
				"in \"upper_func\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		ber_str2bv( argv[ 1 ], 0, 1, &bi->sql_upper_func );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"upper_func=%s\n", bi->sql_upper_func.bv_val, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "upper_needs_cast" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"upper_needs_cast\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_UPPER_NEEDS_CAST;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_UPPER_NEEDS_CAST;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"upper_needs_cast\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"upper_needs_cast =%s\n", 
			BACKSQL_UPPER_NEEDS_CAST( bi ) ? "yes" : "no", 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "strcast_func" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing function name "
				"in \"strcast_func\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		ber_str2bv( argv[ 1 ], 0, 1, &bi->sql_strcast_func );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"strcast_func=%s\n", bi->sql_strcast_func.bv_val, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "delentry_stmt" ) ||
			!strcasecmp( argv[ 0 ], "delentry_query" ) )
	{
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing SQL statement "
				"in \"delentry_stmt\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_delentry_stmt = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"delentry_stmt=%s\n", bi->sql_delentry_stmt, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "renentry_stmt" ) ||
			!strcasecmp( argv[ 0 ], "renentry_query" ) )
	{
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing SQL statement "
				"in \"renentry_stmt\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_renentry_stmt = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"renentry_stmt=%s\n", bi->sql_renentry_stmt, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "delobjclasses_stmt" ) ||
			!strcasecmp( argv[ 0 ], "delobjclasses_query" ) )
	{
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing SQL statement "
				"in \"delobjclasses_stmt\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_delobjclasses_stmt = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"delobjclasses_stmt=%s\n", bi->sql_delobjclasses_stmt, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "has_ldapinfo_dn_ru" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"has_ldapinfo_dn_ru\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_HAS_LDAPINFO_DN_RU;
			bi->sql_flags |= BSQLF_DONTCHECK_LDAPINFO_DN_RU;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_HAS_LDAPINFO_DN_RU;
			bi->sql_flags |= BSQLF_DONTCHECK_LDAPINFO_DN_RU;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"has_ldapinfo_dn_ru\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"has_ldapinfo_dn_ru=%s\n", 
			BACKSQL_HAS_LDAPINFO_DN_RU( bi ) ? "yes" : "no", 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "fail_if_no_mapping" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"fail_if_no_mapping\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_FAIL_IF_NO_MAPPING;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_FAIL_IF_NO_MAPPING;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"fail_if_no_mapping\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"fail_if_no_mapping=%s\n", 
			BACKSQL_FAIL_IF_NO_MAPPING( bi ) ? "yes" : "no", 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "allow_orphans" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"allow_orphans\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_ALLOW_ORPHANS;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_ALLOW_ORPHANS;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"allow_orphans\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"allow_orphans=%s\n", 
			BACKSQL_ALLOW_ORPHANS( bi ) ? "yes" : "no", 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "baseobject" ) ) {
		if ( be->be_suffix == NULL ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): : "
				"must be defined after \"suffix\"\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( bi->sql_baseObject ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): : "
				"\"baseObject\" already provided (will be overwritten)\n",
				fname, lineno, 0 );
			entry_free( bi->sql_baseObject );
		}
	
		switch ( argc ) {
		case 1:
			return create_baseObject( be, fname, lineno );

		case 2:
			return read_baseObject( be, argv[ 1 ] );

		default:
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"trailing values "
				"in \"baseObject\" directive?\n",
				fname, lineno, 0 );
			return 1;
		}

	} else if ( !strcasecmp( argv[ 0 ], "sqllayer" ) ) {
		if ( backsql_api_config( bi, argv[ 1 ], argc - 2, &argv[ 2 ] ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"unable to load sqllayer \"%s\"\n",
				fname, lineno, argv[ 1 ] );
			return 1;
		}

	} else if ( !strcasecmp( argv[ 0 ], "id_query" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE, 
				"<==backsql_db_config (%s line %d): "
				"missing SQL condition "
				"in \"id_query\" directive\n",
				fname, lineno, 0 );
			return 1;
		}
		bi->sql_id_query = ch_strdup( argv[ 1 ] );
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"id_query=%s\n", bi->sql_id_query, 0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "use_subtree_shortcut" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"use_subtree_shortcut\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_USE_SUBTREE_SHORTCUT;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_USE_SUBTREE_SHORTCUT;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"use_subtree_shortcut\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"use_subtree_shortcut=%s\n", 
			BACKSQL_USE_SUBTREE_SHORTCUT( bi ) ? "yes" : "no",
			0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "fetch_all_attrs" ) ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"fetch_all_attrs\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_FETCH_ALL_ATTRS;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_FETCH_ALL_ATTRS;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"fetch_all_attrs\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"fetch_all_attrs=%s\n", 
			BACKSQL_FETCH_ALL_ATTRS( bi ) ? "yes" : "no",
			0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "fetch_attrs" ) ) {
		char		*str, *s, *next;
		const char	*delimstr = ",";

		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing <attrlist>"
				"in \"fetch_all_attrs <attrlist>\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		str = ch_strdup( argv[ 1 ] );
		for ( s = ldap_pvt_strtok( str, delimstr, &next );
				s != NULL;
				s = ldap_pvt_strtok( NULL, delimstr, &next ) )
		{
			if ( strlen( s ) == 1 ) {
				if ( *s == '*' ) {
					bi->sql_flags |= BSQLF_FETCH_ALL_USERATTRS;
					argv[ 1 ][ s - str ] = ',';

				} else if ( *s == '+' ) {
					bi->sql_flags |= BSQLF_FETCH_ALL_OPATTRS;
					argv[ 1 ][ s - str ] = ',';
				}
			}
		}
		ch_free( str );
		bi->sql_anlist = str2anlist( bi->sql_anlist, argv[ 1 ], delimstr );
		if ( bi->sql_anlist == NULL ) {
			return -1;
		}

	} else if ( !strcasecmp( argv[ 0 ], "check_schema" ) ) {
		if ( argc != 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing { yes | no }"
				"in \"check_schema\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			bi->sql_flags |= BSQLF_CHECK_SCHEMA;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			bi->sql_flags &= ~BSQLF_CHECK_SCHEMA;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"\"check_schema\" directive arg "
				"must be \"yes\" or \"no\"\n",
				fname, lineno, 0 );
			return 1;

		}
		Debug( LDAP_DEBUG_TRACE, "<==backsql_db_config(): "
			"check_schema=%s\n", 
			BACKSQL_CHECK_SCHEMA( bi ) ? "yes" : "no",
			0, 0 );

	} else if ( !strcasecmp( argv[ 0 ], "aliasing_keyword" ) ) {
		if ( argc != 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing arg "
				"in \"aliasing_keyword <string>\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( ! BER_BVISNULL( &bi->sql_aliasing ) ) {
			ch_free( bi->sql_aliasing.bv_val );
		}

		ber_str2bv( argv[ 1 ], strlen( argv[ 1 ] ) + 1, 1,
			&bi->sql_aliasing );
		/* add a trailing space... */
		bi->sql_aliasing.bv_val[ bi->sql_aliasing.bv_len - 1] = ' ';

	} else if ( !strcasecmp( argv[ 0 ], "aliasing_quote" ) ) {
		if ( argc != 2 ) {
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): "
				"missing arg "
				"in \"aliasing_quote <string>\" directive\n",
				fname, lineno, 0 );
			return 1;
		}

		if ( ! BER_BVISNULL( &bi->sql_aliasing_quote ) ) {
			ch_free( bi->sql_aliasing_quote.bv_val );
		}

		ber_str2bv( argv[ 1 ], 0, 1, &bi->sql_aliasing_quote );

	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return 0;
}

/*
 * Read the entries specified in fname and merge the attributes
 * to the user defined baseObject entry. Note that if we find any errors
 * what so ever, we will discard the entire entries, print an
 * error message and return.
 */
static int
read_baseObject( 
	BackendDB	*be,
	const char	*fname )
{
	backsql_info 	*bi = (backsql_info *)be->be_private;
	LDIFFP		*fp;
	int		rc = 0, lineno = 0, lmax = 0;
	char		*buf = NULL;

	assert( fname != NULL );

	fp = ldif_open( fname, "r" );
	if ( fp == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"could not open back-sql baseObject "
			"attr file \"%s\" - absolute path?\n",
			fname, 0, 0 );
		perror( fname );
		return LDAP_OTHER;
	}

	bi->sql_baseObject = entry_alloc();
	if ( bi->sql_baseObject == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"read_baseObject_file: entry_alloc failed", 0, 0, 0 );
		ldif_close( fp );
		return LDAP_NO_MEMORY;
	}
	bi->sql_baseObject->e_name = be->be_suffix[0];
	bi->sql_baseObject->e_nname = be->be_nsuffix[0];
	bi->sql_baseObject->e_attrs = NULL;

	while ( ldif_read_record( fp, &lineno, &buf, &lmax ) ) {
		Entry		*e = str2entry( buf );
		Attribute	*a;

		if( e == NULL ) {
			fprintf( stderr, "back-sql baseObject: "
					"could not parse entry (line=%d)\n",
					lineno );
			rc = LDAP_OTHER;
			break;
		}

		/* make sure the DN is the database's suffix */
		if ( !be_issuffix( be, &e->e_nname ) ) {
			fprintf( stderr,
				"back-sql: invalid baseObject - "
				"dn=\"%s\" (line=%d)\n",
				e->e_name.bv_val, lineno );
			entry_free( e );
			rc = EXIT_FAILURE;
			break;
		}

		/*
		 * we found a valid entry, so walk thru all the attributes in the
		 * entry, and add each attribute type and description to baseObject
		 */
		for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
			if ( attr_merge( bi->sql_baseObject, a->a_desc,
						a->a_vals,
						( a->a_nvals == a->a_vals ) ?
						NULL : a->a_nvals ) )
			{
				rc = LDAP_OTHER;
				break;
			}
		}

		entry_free( e );
		if ( rc ) {
			break;
		}
	}

	if ( rc ) {
		entry_free( bi->sql_baseObject );
		bi->sql_baseObject = NULL;
	}

	ch_free( buf );

	ldif_close( fp );

	Debug( LDAP_DEBUG_CONFIG, "back-sql baseObject file \"%s\" read.\n",
			fname, 0, 0 );

	return rc;
}

static int
create_baseObject(
	BackendDB	*be,
	const char	*fname,
	int		lineno )
{
	backsql_info 	*bi = (backsql_info *)be->be_private;
	LDAPRDN		rdn;
	char		*p;
	int		rc, iAVA;
	char		buf[1024];

	snprintf( buf, sizeof(buf),
			"dn: %s\n"
			"objectClass: extensibleObject\n"
			"description: builtin baseObject for back-sql\n"
			"description: all entries mapped "
				"in table \"ldap_entries\" "
				"must have "
				"\"" BACKSQL_BASEOBJECT_IDSTR "\" "
				"in the \"parent\" column",
			be->be_suffix[0].bv_val );

	bi->sql_baseObject = str2entry( buf );
	if ( bi->sql_baseObject == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<==backsql_db_config (%s line %d): "
			"unable to parse baseObject entry\n",
			fname, lineno, 0 );
		return 1;
	}

	if ( BER_BVISEMPTY( &be->be_suffix[ 0 ] ) ) {
		return 0;
	}

	rc = ldap_bv2rdn( &be->be_suffix[ 0 ], &rdn, (char **)&p,
			LDAP_DN_FORMAT_LDAP );
	if ( rc != LDAP_SUCCESS ) {
		snprintf( buf, sizeof(buf),
			"unable to extract RDN "
			"from baseObject DN \"%s\" (%d: %s)",
			be->be_suffix[ 0 ].bv_val,
			rc, ldap_err2string( rc ) );
		Debug( LDAP_DEBUG_TRACE,
			"<==backsql_db_config (%s line %d): %s\n",
			fname, lineno, buf );
		return 1;
	}

	for ( iAVA = 0; rdn[ iAVA ]; iAVA++ ) {
		LDAPAVA				*ava = rdn[ iAVA ];
		AttributeDescription		*ad = NULL;
		slap_syntax_transform_func	*transf = NULL;
		struct berval			bv = BER_BVNULL;
		const char			*text = NULL;

		assert( ava != NULL );

		rc = slap_bv2ad( &ava->la_attr, &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( buf, sizeof(buf),
				"AttributeDescription of naming "
				"attribute #%d from baseObject "
				"DN \"%s\": %d: %s",
				iAVA, be->be_suffix[ 0 ].bv_val,
				rc, ldap_err2string( rc ) );
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): %s\n",
				fname, lineno, buf );
			return 1;
		}
		
		transf = ad->ad_type->sat_syntax->ssyn_pretty;
		if ( transf ) {
			/*
	 		 * transform value by pretty function
			 *	if value is empty, use empty_bv
			 */
			rc = ( *transf )( ad->ad_type->sat_syntax,
				ava->la_value.bv_len
					? &ava->la_value
					: (struct berval *) &slap_empty_bv,
				&bv, NULL );
	
			if ( rc != LDAP_SUCCESS ) {
				snprintf( buf, sizeof(buf),
					"prettying of attribute #%d "
					"from baseObject "
					"DN \"%s\" failed: %d: %s",
					iAVA, be->be_suffix[ 0 ].bv_val,
					rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_TRACE,
					"<==backsql_db_config (%s line %d): "
					"%s\n",
					fname, lineno, buf );
				return 1;
			}
		}

		if ( !BER_BVISNULL( &bv ) ) {
			if ( ava->la_flags & LDAP_AVA_FREE_VALUE ) {
				ber_memfree( ava->la_value.bv_val );
			}
			ava->la_value = bv;
			ava->la_flags |= LDAP_AVA_FREE_VALUE;
		}

		attr_merge_normalize_one( bi->sql_baseObject,
				ad, &ava->la_value, NULL );
	}

	ldap_rdnfree( rdn );

	return 0;
}

