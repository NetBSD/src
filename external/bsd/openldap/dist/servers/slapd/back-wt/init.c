/*	$NetBSD: init.c,v 1.3 2025/09/05 21:16:32 christos Exp $	*/

/* OpenLDAP WiredTiger backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2024 The OpenLDAP Foundation.
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
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: init.c,v 1.3 2025/09/05 21:16:32 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "slap-config.h"

static int
wt_db_init( BackendDB *be, ConfigReply *cr )
{
	struct wt_info *wi;

	Debug( LDAP_DEBUG_TRACE, "wt_db_init: Initializing wt backend\n" );

	/* allocate backend-database-specific stuff */
	wi = ch_calloc( 1, sizeof(struct wt_info) );
	wi->wi_home = ch_strdup( SLAPD_DEFAULT_DB_DIR );
	wi->wi_config = ch_calloc( 1, WT_CONFIG_MAX + 1);
	if ( slapMode & SLAP_TOOL_READONLY ) {
		strcpy(wi->wi_config, "readonly");
	} else {
		strcpy(wi->wi_config, "create");
	}
	wi->wi_lastid = 0;
	wi->wi_search_stack_depth = DEFAULT_SEARCH_STACK_DEPTH;
	wi->wi_search_stack = NULL;
	wi->wi_flags = WT_USE_IDLCACHE;

	be->be_private = wi;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return LDAP_SUCCESS;
}

static int
wt_db_open( BackendDB *be, ConfigReply *cr )
{
	struct wt_info *wi = (struct wt_info *) be->be_private;
	int rc;
	struct stat st;
	WT_SESSION *session = NULL;
	WT_SESSION *cache_session = NULL;

	if ( be->be_suffix == NULL ) {
		Debug( LDAP_DEBUG_ANY, "wt_db_open: need suffix.\n" );
		return -1;
	}

	Debug( LDAP_DEBUG_ARGS,
		   "wt_db_open: \"%s\", home=%s, config=%s\n",
		   be->be_suffix[0].bv_val, wi->wi_home, wi->wi_config );

	/* Check existence of home. Any error means trouble */
	rc = stat( wi->wi_home, &st );
	if( rc ) {
		int saved_errno = errno;
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot access database directory \"%s\" (%d).\n",
			   be->be_suffix[0].bv_val, wi->wi_home, saved_errno );
		return -1;
	}

	/* back-wt is always clean */
	be->be_flags |= SLAP_DBFLAG_CLEAN;
	
	/* Open and create database */
	rc = wiredtiger_open(wi->wi_home, NULL,
						 wi->wi_config, &wi->wi_conn);
	if( rc ) {
		int saved_errno = errno;
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot open database \"%s\" (%d).\n",
			   be->be_suffix[0].bv_val, wi->wi_home, saved_errno );
		return -1;
	}

	rc = wi->wi_conn->open_session(wi->wi_conn, NULL, NULL, &session);
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot open session: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	if ( slapMode & SLAP_TOOL_READONLY ) {
		goto readonly;
	}

	/* checking for obsolete table */
	rc = session->verify(session, WT_INDEX_REVDN, NULL);
	if ( !rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "incompatible wiredtiger table, please restore from LDIF.\n",
			   be->be_suffix[0].bv_val );
		return -1;
	}

	/* create tables and indexes */
	rc = session->create(session,
						 WT_TABLE_ID2ENTRY,
						 "key_format=Q,"
						 "value_format=Su,"
						 "columns=(id,dn,entry)");
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot create entry table: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	rc = session->create(session,
						 WT_TABLE_DN2ID,
						 "key_format=S,"
						 "value_format=SQQ,"
						 "columns=(revdn,ndn,id,pid)");
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot create entry table: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	/* not using dn2id index for id2entry table */
	rc = session->create(session, WT_INDEX_DN, "columns=(dn)");
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot create dn index: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	rc = session->create(session, WT_INDEX_PID, "columns=(pid)");
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot create pid index: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	rc = session->create(session, WT_INDEX_NDN, "columns=(ndn)");
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot create ndn index: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	/* open in-memory database for idlcache */
	rc = wiredtiger_open(be->be_suffix[0].bv_val, NULL,
						 "in_memory=true", &wi->wi_cache);
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot open database for cache (%s).\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	rc = wi->wi_cache->open_session(wi->wi_cache, NULL, NULL, &cache_session);
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot open session for cache: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

	rc = cache_session->create(cache_session,
							   WT_TABLE_IDLCACHE,
							   "key_format=Sb,"
							   "value_format=u,"
							   "columns=(ndn,scope,idl)");
	if( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_db_open: database \"%s\": "
			   "cannot create idlcache table: \"%s\"\n",
			   be->be_suffix[0].bv_val, wiredtiger_strerror(rc) );
		return -1;
	}

readonly:
	rc = wt_last_id( be, session, &wi->wi_lastid);
	if (rc) {
		snprintf( cr->msg, sizeof(cr->msg), "database \"%s\": "
				  "last_id() failed: %s(%d).",
				  be->be_suffix[0].bv_val, wiredtiger_strerror(rc), rc );
        Debug( LDAP_DEBUG_ANY, "wt_db_open: %s\n", cr->msg );
		return rc;
	}

	if (session) {
		session->close(session, NULL);
	}
	if (cache_session) {
		cache_session->close(cache_session, NULL);
	}

	wi->wi_flags |= WT_IS_OPEN;
    return LDAP_SUCCESS;
}

static int
wt_db_close( BackendDB *be, ConfigReply *cr )
{
	struct wt_info *wi = (struct wt_info *) be->be_private;
	int rc;

	if ( wi->wi_cache ) {
		rc = wi->wi_cache->close(wi->wi_cache, NULL);
		if( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   "wt_db_close: cannot close cache database (%d).\n", rc );
			return -1;
		}
	}

	if ( wi->wi_conn ) {
		rc = wi->wi_conn->close(wi->wi_conn, NULL);
		if( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   "wt_db_close: cannot close database (%d).\n", rc );
			return -1;
		}
		wi->wi_flags &= ~WT_IS_OPEN;
	}

    return LDAP_SUCCESS;
}

static int
wt_db_destroy( Backend *be, ConfigReply *cr )
{
	struct wt_info *wi = (struct wt_info *) be->be_private;

	if( wi->wi_home ) {
		ch_free( wi->wi_home );
		wi->wi_home = NULL;
	}

	if( wi->wi_config ) {
		ch_free( wi->wi_config );
		wi->wi_config = NULL;
	}

	wt_attr_index_destroy( wi );
	ch_free( wi );
	be->be_private = NULL;

	return LDAP_SUCCESS;
}

int
wt_back_initialize( BackendInfo *bi )
{
	static const char *controls[] = {
		LDAP_CONTROL_ASSERT,
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
		LDAP_CONTROL_PAGEDRESULTS,
		LDAP_CONTROL_PRE_READ,
		LDAP_CONTROL_POST_READ,
		LDAP_CONTROL_SUBENTRIES,
		LDAP_CONTROL_X_PERMISSIVE_MODIFY,
#ifdef LDAP_X_TXN
		LDAP_CONTROL_X_TXN_SPEC,
#endif
		NULL
	};

	/* initialize the database system */
	Debug( LDAP_DEBUG_TRACE,
		   "wt_back_initialize: initialize WiredTiger backend\n" );

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_SUBENTRIES |
		SLAP_BFLAG_ALIASES |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = (char **)controls;

	/* version check */
	Debug( LDAP_DEBUG_TRACE,
		   "wt_back_initialize: %s\n",
		   wiredtiger_version(NULL, NULL, NULL) );

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = wt_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = wt_db_open;
	bi->bi_db_close = wt_db_close;
	bi->bi_db_destroy = wt_db_destroy;

	bi->bi_op_add = wt_add;
	bi->bi_op_bind = wt_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = wt_search;
	bi->bi_op_compare = wt_compare;
	bi->bi_op_modify = wt_modify;
	bi->bi_op_modrdn = wt_modrdn;
	bi->bi_op_delete = wt_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = wt_extended;
#ifdef LDAP_X_TXN
	bi->bi_op_txn = 0;
#endif

	bi->bi_chk_referrals = 0;
	bi->bi_operational = wt_operational;

	bi->bi_has_subordinates = wt_hasSubordinates;
	bi->bi_entry_release_rw = wt_entry_release;
	bi->bi_entry_get_rw = wt_entry_get;

	bi->bi_tool_entry_open = wt_tool_entry_open;
	bi->bi_tool_entry_close = wt_tool_entry_close;
	bi->bi_tool_entry_first = backend_tool_entry_first;
	bi->bi_tool_entry_first_x = wt_tool_entry_first_x;
	bi->bi_tool_entry_next = wt_tool_entry_next;
	bi->bi_tool_entry_get = wt_tool_entry_get;
	bi->bi_tool_entry_put = wt_tool_entry_put;
	bi->bi_tool_entry_reindex = wt_tool_entry_reindex;
	bi->bi_tool_sync = 0;
	bi->bi_tool_dn2id_get = wt_tool_dn2id_get;
	bi->bi_tool_entry_modify = wt_tool_entry_modify;

#if LDAP_VENDOR_VERSION_MINOR == X || LDAP_VENDOR_VERSION_MINOR >= 5
	bi->bi_tool_entry_delete = wt_tool_entry_delete;
#endif

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return wt_back_init_cf( bi );
}

#if SLAPD_WT == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( wt )

#endif /* SLAPD_WT == SLAPD_MOD_DYNAMIC */

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
