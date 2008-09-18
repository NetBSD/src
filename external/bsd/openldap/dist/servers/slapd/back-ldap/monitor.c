/* monitor.c - monitor ldap backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldap/monitor.c,v 1.2.2.4 2008/02/11 23:26:46 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "lutil.h"
#include "back-ldap.h"

#include "config.h"

static ObjectClass		*oc_olmLDAPDatabase;

static AttributeDescription	*ad_olmDbURIList;

/*
 * NOTE: there's some confusion in monitor OID arc;
 * by now, let's consider:
 * 
 * Subsystems monitor attributes	1.3.6.1.4.1.4203.666.1.55.0
 * Databases monitor attributes		1.3.6.1.4.1.4203.666.1.55.0.1
 * LDAP database monitor attributes	1.3.6.1.4.1.4203.666.1.55.0.1.2
 *
 * Subsystems monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0
 * Databases monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0.1
 * LDAP database monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0.1.2
 */

static struct {
	char			*name;
	char			*oid;
}		s_oid[] = {
	{ "olmLDAPAttributes",			"olmDatabaseAttributes:2" },
	{ "olmLDAPObjectClasses",		"olmDatabaseObjectClasses:2" },

	{ NULL }
};

static struct {
	char			*desc;
	AttributeDescription	**ad;
}		s_at[] = {
	{ "( olmLDAPAttributes:1 "
		"NAME ( 'olmDbURIList' ) "
		"DESC 'List of URIs a proxy is serving; can be modified run-time' "
		"SUP managedInfo )",
		&ad_olmDbURIList },

	{ NULL }
};

static struct {
	char		*desc;
	ObjectClass	**oc;
}		s_oc[] = {
	/* augments an existing object, so it must be AUXILIARY
	 * FIXME: derive from some ABSTRACT "monitoredEntity"? */
	{ "( olmLDAPObjectClasses:1 "
		"NAME ( 'olmLDAPDatabase' ) "
		"SUP top AUXILIARY "
		"MAY ( "
			"olmDbURIList "
			") )",
		&oc_olmLDAPDatabase },

	{ NULL }
};

static int
ldap_back_monitor_info_destroy( ldapinfo_t * li )
{
	if ( !BER_BVISNULL( &li->li_monitor_info.lmi_rdn ) )
		ch_free( li->li_monitor_info.lmi_rdn.bv_val );
	if ( !BER_BVISNULL( &li->li_monitor_info.lmi_nrdn ) )
		ch_free( li->li_monitor_info.lmi_nrdn.bv_val );
	if ( !BER_BVISNULL( &li->li_monitor_info.lmi_filter ) )
		ch_free( li->li_monitor_info.lmi_filter.bv_val );
	if ( !BER_BVISNULL( &li->li_monitor_info.lmi_more_filter ) )
		ch_free( li->li_monitor_info.lmi_more_filter.bv_val );

	memset( &li->li_monitor_info, 0, sizeof( li->li_monitor_info ) );

	return 0;
}

static int
ldap_back_monitor_update(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	ldapinfo_t		*li = (ldapinfo_t *)priv;

	Attribute		*a;

	/* update olmDbURIList */
	a = attr_find( e->e_attrs, ad_olmDbURIList );
	if ( a != NULL ) {
		struct berval	bv;

		assert( a->a_vals != NULL );
		assert( !BER_BVISNULL( &a->a_vals[ 0 ] ) );
		assert( BER_BVISNULL( &a->a_vals[ 1 ] ) );

		ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
		if ( li->li_uri ) {
			ber_str2bv( li->li_uri, 0, 0, &bv );
			if ( !bvmatch( &a->a_vals[ 0 ], &bv ) ) {
				ber_bvreplace( &a->a_vals[ 0 ], &bv );
			}
		}
		ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );
	}

	return SLAP_CB_CONTINUE;
}

static int
ldap_back_monitor_modify(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	ldapinfo_t		*li = (ldapinfo_t *) priv;
	
	Attribute		*save_attrs = NULL;
	Modifications		*ml,
				*ml_olmDbURIList = NULL;
	struct berval		ul = BER_BVNULL;
	int			got = 0;

	for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		if ( ml->sml_desc == ad_olmDbURIList ) {
			if ( ml_olmDbURIList != NULL ) {
				rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
				rs->sr_text = "conflicting modifications";
				goto done;
			}

			if ( ml->sml_op != LDAP_MOD_REPLACE ) {
				rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
				rs->sr_text = "modification not allowed";
				goto done;
			}

			ml_olmDbURIList = ml;
			got++;
			continue;
		}
	}

	if ( got == 0 ) {
		return SLAP_CB_CONTINUE;
	}

	save_attrs = attrs_dup( e->e_attrs );

	if ( ml_olmDbURIList != NULL ) {
		Attribute	*a = NULL;
		LDAPURLDesc	*ludlist = NULL;
		int		rc;

		ml = ml_olmDbURIList;
		assert( ml->sml_nvalues != NULL );

		if ( BER_BVISNULL( &ml->sml_nvalues[ 0 ] ) ) {
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "no value provided";
			goto done;
		}

		if ( !BER_BVISNULL( &ml->sml_nvalues[ 1 ] ) ) {
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "multiple values provided";
			goto done;
		}

		rc = ldap_url_parselist_ext( &ludlist,
			ml->sml_nvalues[ 0 ].bv_val, NULL,
			LDAP_PVT_URL_PARSE_NOEMPTY_HOST
				| LDAP_PVT_URL_PARSE_DEF_PORT );
		if ( rc != LDAP_URL_SUCCESS ) {
			rs->sr_err = LDAP_INVALID_SYNTAX;
			rs->sr_text = "unable to parse URI list";
			goto done;
		}

		ul.bv_val = ldap_url_list2urls( ludlist );
		ldap_free_urllist( ludlist );
		if ( ul.bv_val == NULL ) {
			rs->sr_err = LDAP_OTHER;
			goto done;
		}
		ul.bv_len = strlen( ul.bv_val );
		
		a = attr_find( e->e_attrs, ad_olmDbURIList );
		if ( a != NULL ) {
			if ( a->a_nvals == a->a_vals ) {
				a->a_nvals = ch_calloc( sizeof( struct berval ), 2 );
			}

			ber_bvreplace( &a->a_vals[ 0 ], &ul );
			ber_bvreplace( &a->a_nvals[ 0 ], &ul );

		} else {
			attr_merge_normalize_one( e, ad_olmDbURIList, &ul, NULL );
		}
	}

	/* apply changes */
	if ( !BER_BVISNULL( &ul ) ) {
		ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
		if ( li->li_uri ) {
			ch_free( li->li_uri );
		}
		li->li_uri = ul.bv_val;
		ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );

		BER_BVZERO( &ul );
	}

done:;
	if ( !BER_BVISNULL( &ul ) ) {
		ldap_memfree( ul.bv_val );
	}

	if ( rs->sr_err == LDAP_SUCCESS ) {
		attrs_free( save_attrs );
		return SLAP_CB_CONTINUE;
	}

	attrs_free( e->e_attrs );
	e->e_attrs = save_attrs;

	return rs->sr_err;
}

static int
ldap_back_monitor_free(
	Entry		*e,
	void		**priv )
{
	ldapinfo_t		*li = (ldapinfo_t *)(*priv);

	*priv = NULL;

	if ( !slapd_shutdown && !BER_BVISNULL( &li->li_monitor_info.lmi_rdn ) ) {
		ldap_back_monitor_info_destroy( li );
	}

	return SLAP_CB_CONTINUE;
}

static int
ldap_back_monitor_conn_create(
	Operation	*op,
	SlapReply	*rs,
	struct berval	*ndn,
	Entry 		*e_parent,
	Entry		**ep )
{
	monitor_entry_t		*mp_parent;
	ldap_monitor_info_t	*lmi;
	ldapinfo_t		*li;

	assert( e_parent->e_private != NULL );

	mp_parent = e_parent->e_private;
	lmi = (ldap_monitor_info_t *)mp_parent->mp_info;
	li = lmi->lmi_li;

	/* do the hard work! */

	return 1;
}

/*
 * call from within ldap_back_initialize()
 */
static int
ldap_back_monitor_initialize( void )
{
	int		i, code;
	ConfigArgs c;
	char	*argv[ 3 ];

	static int	ldap_back_monitor_initialized = 0;

	/* register schema here; if compiled as dynamic object,
	 * must be loaded __after__ back_monitor.la */

	if ( ldap_back_monitor_initialized++ ) {
		return 0;
	}

	if ( backend_info( "monitor" ) == NULL ) {
		return -1;
	}

	argv[ 0 ] = "back-ldap monitor";
	c.argv = argv;
	c.argc = 3;
	c.fname = argv[0];
	for ( i = 0; s_oid[ i ].name; i++ ) {
	
		argv[ 1 ] = s_oid[ i ].name;
		argv[ 2 ] = s_oid[ i ].oid;

		if ( parse_oidm( &c, 0, NULL ) != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: unable to add "
				"objectIdentifier \"%s=%s\"\n",
				s_oid[ i ].name, s_oid[ i ].oid, 0 );
			return 1;
		}
	}

	for ( i = 0; s_at[ i ].desc != NULL; i++ ) {
		code = register_at( s_at[ i ].desc, s_at[ i ].ad, 1 );
		if ( code != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: register_at failed\n",
				0, 0, 0 );
		}
	}

	for ( i = 0; s_oc[ i ].desc != NULL; i++ ) {
		code = register_oc( s_oc[ i ].desc, s_oc[ i ].oc, 1 );
		if ( code != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: register_oc failed\n",
				0, 0, 0 );
		}
	}

	return 0;
}

/*
 * call from within ldap_back_db_init()
 */
int
ldap_back_monitor_db_init( BackendDB *be )
{
	int	rc;

	rc = ldap_back_monitor_initialize();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

#if 0	/* uncomment to turn monitoring on by default */
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_MONITORING;
#endif

	return 0;
}

/*
 * call from within ldap_back_db_open()
 */
int
ldap_back_monitor_db_open( BackendDB *be )
{
	ldapinfo_t		*li = (ldapinfo_t *) be->be_private;
	char			buf[ BACKMONITOR_BUFSIZE ];
	Entry			*e = NULL;
	monitor_callback_t	*cb = NULL;
	struct berval		suffix, *filter, *base;
	char			*ptr;
	time_t			now;
	char			timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	struct berval 		timestamp;
	int			rc = 0;
	BackendInfo		*mi;
	monitor_extra_t		*mbe;

	if ( !SLAP_DBMONITORING( be ) ) {
		return 0;
	}

	/* check if monitor is configured and usable */
	mi = backend_info( "monitor" );
	if ( !mi || !mi->bi_extra ) {
		SLAP_DBFLAGS( be ) ^= SLAP_DBFLAG_MONITORING;
		return 0;
 	}
 	mbe = mi->bi_extra;

	/* don't bother if monitor is not configured */
	if ( !mbe->is_configured() ) {
		static int warning = 0;

		if ( warning++ == 0 ) {
			Debug( LDAP_DEBUG_ANY, "ldap_back_monitor_db_open: "
				"monitoring disabled; "
				"configure monitor database to enable\n",
				0, 0, 0 );
		}

		return 0;
	}

	/* set up the fake subsystem that is used to create
	 * the volatile connection entries */
	li->li_monitor_info.lmi_mss.mss_name = "back-ldap";
	li->li_monitor_info.lmi_mss.mss_flags = MONITOR_F_VOLATILE_CH;
	li->li_monitor_info.lmi_mss.mss_create = ldap_back_monitor_conn_create;

	li->li_monitor_info.lmi_li = li;
	li->li_monitor_info.lmi_scope = LDAP_SCOPE_SUBORDINATE;
	base = &li->li_monitor_info.lmi_base;
	BER_BVSTR( base, "cn=databases,cn=monitor" );
	filter = &li->li_monitor_info.lmi_filter;
	BER_BVZERO( filter );

	suffix.bv_len = ldap_bv2escaped_filter_value_len( &be->be_nsuffix[ 0 ] );
	if ( suffix.bv_len == be->be_nsuffix[ 0 ].bv_len ) {
		suffix = be->be_nsuffix[ 0 ];

	} else {
		ldap_bv2escaped_filter_value( &be->be_nsuffix[ 0 ], &suffix );
	}
	
	filter->bv_len = STRLENOF( "(&" )
		+ li->li_monitor_info.lmi_more_filter.bv_len
		+ STRLENOF( "(monitoredInfo=" )
		+ strlen( be->bd_info->bi_type )
		+ STRLENOF( ")(!(monitorOverlay=" )
		+ strlen( be->bd_info->bi_type )
		+ STRLENOF( "))(namingContexts:distinguishedNameMatch:=" )
		+ suffix.bv_len + STRLENOF( "))" );
	ptr = filter->bv_val = ch_malloc( filter->bv_len + 1 );
	ptr = lutil_strcopy( ptr, "(&" );
	ptr = lutil_strncopy( ptr, li->li_monitor_info.lmi_more_filter.bv_val,
		li->li_monitor_info.lmi_more_filter.bv_len );
	ptr = lutil_strcopy( ptr, "(monitoredInfo=" );
	ptr = lutil_strcopy( ptr, be->bd_info->bi_type );
	ptr = lutil_strcopy( ptr, ")(!(monitorOverlay=" );
	ptr = lutil_strcopy( ptr, be->bd_info->bi_type );
	ptr = lutil_strcopy( ptr, "))(namingContexts:distinguishedNameMatch:=" );
	ptr = lutil_strncopy( ptr, suffix.bv_val, suffix.bv_len );
	ptr = lutil_strcopy( ptr, "))" );
	ptr[ 0 ] = '\0';
	assert( filter->bv_len == ptr - filter->bv_val );

	if ( suffix.bv_val != be->be_nsuffix[ 0 ].bv_val ) {
		ch_free( suffix.bv_val );
	}

	now = slap_get_time();
	timestamp.bv_val = timebuf;
	timestamp.bv_len = sizeof( timebuf );
	slap_timestamp( &now, &timestamp );

	/* caller (e.g. an overlay based on back-ldap) may want to use
	 * a different RDN... */
	if ( BER_BVISNULL( &li->li_monitor_info.lmi_rdn ) ) {
		ber_str2bv( "cn=Connections", 0, 1, &li->li_monitor_info.lmi_rdn );
	}

	ptr = ber_bvchr( &li->li_monitor_info.lmi_rdn, '=' );
	assert( ptr != NULL );
	ptr[ 0 ] = '\0';
	ptr++;

	snprintf( buf, sizeof( buf ),
		"dn: %s=%s\n"
		"objectClass: monitorContainer\n"
		"%s: %s\n"
		"creatorsName: %s\n"
		"createTimestamp: %s\n"
		"modifiersName: %s\n"
		"modifyTimestamp: %s\n",
		li->li_monitor_info.lmi_rdn.bv_val,
			ptr,
		li->li_monitor_info.lmi_rdn.bv_val,
			ptr,
		BER_BVISNULL( &be->be_rootdn ) ? SLAPD_ANONYMOUS : be->be_rootdn.bv_val,
		timestamp.bv_val,
		BER_BVISNULL( &be->be_rootdn ) ? SLAPD_ANONYMOUS : be->be_rootdn.bv_val,
		timestamp.bv_val );
	e = str2entry( buf );
	if ( e == NULL ) {
		rc = -1;
		goto cleanup;
	}

	ptr[ -1 ] = '=';

	/* add labeledURI and special, modifiable URI value */
	if ( li->li_uri != NULL ) {
		struct berval	bv;
		LDAPURLDesc	*ludlist = NULL;
		int		rc;

		rc = ldap_url_parselist_ext( &ludlist,
			li->li_uri, NULL,
			LDAP_PVT_URL_PARSE_NOEMPTY_HOST
				| LDAP_PVT_URL_PARSE_DEF_PORT );
		if ( rc != LDAP_URL_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_db_open: "
				"unable to parse URI list (ignored)\n",
				0, 0, 0 );
		} else {
			for ( ; ludlist != NULL; ) {
				LDAPURLDesc	*next = ludlist->lud_next;

				bv.bv_val = ldap_url_desc2str( ludlist );
				assert( bv.bv_val != NULL );
				ldap_free_urldesc( ludlist );
				bv.bv_len = strlen( bv.bv_val );
				attr_merge_normalize_one( e, slap_schema.si_ad_labeledURI,
					&bv, NULL );
				ch_free( bv.bv_val );

				ludlist = next;
			}
		}
		
		ber_str2bv( li->li_uri, 0, 0, &bv );
		attr_merge_normalize_one( e, ad_olmDbURIList,
			&bv, NULL );
	}

	ber_dupbv( &li->li_monitor_info.lmi_nrdn, &e->e_nname );

	cb = ch_calloc( sizeof( monitor_callback_t ), 1 );
	cb->mc_update = ldap_back_monitor_update;
	cb->mc_modify = ldap_back_monitor_modify;
	cb->mc_free = ldap_back_monitor_free;
	cb->mc_private = (void *)li;

	rc = mbe->register_entry_parent( e, cb,
		(monitor_subsys_t *)&li->li_monitor_info,
		MONITOR_F_VOLATILE_CH,
		base, LDAP_SCOPE_SUBORDINATE, filter );

cleanup:;
	if ( rc != 0 ) {
		if ( cb != NULL ) {
			ch_free( cb );
			cb = NULL;
		}

		if ( e != NULL ) {
			entry_free( e );
			e = NULL;
		}

		if ( !BER_BVISNULL( filter ) ) {
			ch_free( filter->bv_val );
			BER_BVZERO( filter );
		}
	}

	/* store for cleanup */
	li->li_monitor_info.lmi_cb = (void *)cb;

	if ( e != NULL ) {
		entry_free( e );
	}

	return rc;
}

/*
 * call from within ldap_back_db_close()
 */
int
ldap_back_monitor_db_close( BackendDB *be )
{
	ldapinfo_t		*li = (ldapinfo_t *) be->be_private;

	if ( li && !BER_BVISNULL( &li->li_monitor_info.lmi_filter ) ) {
		BackendInfo		*mi;
		monitor_extra_t		*mbe;

		/* check if monitor is configured and usable */
		mi = backend_info( "monitor" );
		if ( mi && mi->bi_extra ) {
 			mbe = mi->bi_extra;

			mbe->unregister_entry_parent(
				&li->li_monitor_info.lmi_nrdn,
				(monitor_callback_t *)li->li_monitor_info.lmi_cb,
				&li->li_monitor_info.lmi_base,
				li->li_monitor_info.lmi_scope,
				&li->li_monitor_info.lmi_filter );
		}
	}

	return 0;
}

/*
 * call from within ldap_back_db_destroy()
 */
int
ldap_back_monitor_db_destroy( BackendDB *be )
{
	ldapinfo_t		*li = (ldapinfo_t *) be->be_private;

	if ( li ) {
		(void)ldap_back_monitor_info_destroy( li );
	}

	return 0;
}

