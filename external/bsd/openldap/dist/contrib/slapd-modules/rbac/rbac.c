/*	$NetBSD: rbac.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbac.c - RBAC main file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2013-2021 The OpenLDAP Foundation.
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: rbac.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

#define RBAC_REQ 1

static slap_overinst rbac;

static struct berval slap_EXOP_CREATE_SESSION =
		BER_BVC(LDAP_RBAC_EXOP_CREATE_SESSION);
static struct berval slap_EXOP_CHECK_ACCESS =
		BER_BVC(LDAP_RBAC_EXOP_CHECK_ACCESS);
static struct berval slap_EXOP_ADD_ACTIVE_ROLE =
		BER_BVC(LDAP_RBAC_EXOP_ADD_ACTIVE_ROLE);
static struct berval slap_EXOP_DROP_ACTIVE_ROLE =
		BER_BVC(LDAP_RBAC_EXOP_DROP_ACTIVE_ROLE);
static struct berval slap_EXOP_DELETE_SESSION =
		BER_BVC(LDAP_RBAC_EXOP_DELETE_SESSION);
static struct berval slap_EXOP_SESSION_ROLES =
		BER_BVC(LDAP_RBAC_EXOP_SESSION_ROLES);

rbac_tenant_t rbac_tenants = {
	{
		.schema = &slap_rbac_jts_schema,
	},
	NULL
};

static ConfigDriver rbac_cf_gen;

static ConfigTable rbaccfg[] = {
	{ "rbac-default-users-base-dn", "usersDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_DEFAULT_USERS_BASE_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.1 NAME 'olcRBACDefaultUsersBaseDn' "
			"DESC 'default Base DN for RBAC users ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-default-roles-base-dn", "rolesDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_DEFAULT_ROLES_BASE_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.2 NAME 'olcRBACDefaultRolesBaseDn' "
			"DESC 'default base DN for RBAC roles ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-default-permissions-base-dn", "permissionsDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_DEFAULT_PERMISSIONS_BASE_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.3 NAME 'olcRBACDefaultPermissionsBaseDn' "
			"DESC 'default base DN for RBAC permissions ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-default-sessions-base-dn", "sessionsDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_DEFAULT_SESSIONS_BASE_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.4 NAME 'olcRBACDefaultSessionsBaseDn' "
			"DESC 'default base DN for RBAC permissions ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-admin", "adminDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_ADMIN_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.5 NAME 'olcRBACAdminDn' "
			"DESC 'default admin DN for RBAC repository ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-pwd", "adminPwd", 2, 2, 0,
		ARG_MAGIC|RBAC_ADMIN_PWD,
		rbac_cf_gen,
		"(OLcfgCtAt:7.6 NAME 'olcRBACAdminPwd' "
			"DESC 'default admin pwd for RBAC repository ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-session-admin", "sessionAdminDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_SESSION_ADMIN_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.7 NAME 'olcRBACSessionAdminDn' "
			"DESC 'admin DN for RBAC session repository ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-session-admin-pwd", "sessionAdminPwd", 2, 2, 0,
		ARG_MAGIC|RBAC_SESSION_ADMIN_PWD,
		rbac_cf_gen,
		"(OLcfgCtAt:7.8 NAME 'olcRBACSessionAdminPwd' "
			"DESC 'admin pwd for RBAC session repository ' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL
	},
	{ "tenant", "tenant", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_TENANT,
		rbac_cf_gen, "(OLcfgCtAt:7.9 NAME 'olcRBACTenant' "
			"DESC 'RBAC tenant ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-default-audit-base-dn", "auditDN", 2, 2, 0,
		ARG_MAGIC|ARG_DN|RBAC_DEFAULT_AUDIT_BASE_DN,
		rbac_cf_gen,
		"(OLcfgCtAt:7.10 NAME 'olcRBACDefaultAuditBaseDn' "
			"DESC 'default base DN for RBAC audit records ' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "rbac-default-tenant-id", "tenantId", 2, 2, 0,
		ARG_MAGIC|RBAC_DEFAULT_TENANT_ID,
		rbac_cf_gen,
		"(OLcfgCtAt:7.11 NAME 'olcRBACDefaultTenantId' "
			"DESC 'default tenant id' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs rbac_ocs[] = {
	{ "( OLcfgCtOc:7.1 "
			"NAME 'olcRBACConfig' "
			"DESC 'RBAC configuration' "
			"SUP olcOverlayConfig "
			"MAY ( olcRBACDefaultUsersBaseDn $ olcRBACDefaultRolesBaseDn $ "
			"olcRBACDefaultPermissionsBaseDn $ olcRBACDefaultSessionsBaseDn $ "
			"olcRBACAdminDn $  olcRBACAdminPwd $ olcRBACSessionAdminDn $ "
			"olcRBACSessionAdminPwd) )",
		Cft_Overlay, rbaccfg },

	{ NULL, 0, NULL }
};

static slap_verbmasks rbac_keys[] = {
	{ BER_BVC("default_users_base_dn"), RBAC_DEFAULT_USERS_BASE_DN },
	{ BER_BVC("default_roles_base_dn"), RBAC_DEFAULT_ROLES_BASE_DN },
	{ BER_BVC("default_permissions_base_dn"),
		RBAC_DEFAULT_PERMISSIONS_BASE_DN },
	{ BER_BVC("tenant"), RBAC_TENANT },

	{ BER_BVNULL, 0 }
};

static slap_verbmasks rbac_tenant_keys[] = {
	{ BER_BVC("id"), RBAC_TENANT_ID },
	{ BER_BVC("users_base_dn"), RBAC_USERS_BASE_DN },
	{ BER_BVC("roles_base_dn"), RBAC_ROLES_BASE_DN },
	{ BER_BVC("permissions_base_dn"), RBAC_PERMISSIONS_BASE_DN },

	{ BER_BVNULL, 0 }
};

static void
rbac_tenant_parse( char *tenent_info, tenant_info_t *tenants )
{
	return;
}

static int
rbac_cf_gen( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	rbac_tenant_t *ri = &rbac_tenants;
	int rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch ( c->type ) {
			case RBAC_DEFAULT_USERS_BASE_DN:
				value_add_one( &c->rvalue_vals, &ri->tenant_info.users_basedn );
				break;
			case RBAC_DEFAULT_ROLES_BASE_DN:
				value_add_one( &c->rvalue_vals, &ri->tenant_info.roles_basedn );
				break;
			case RBAC_DEFAULT_PERMISSIONS_BASE_DN:
				value_add_one(
						&c->rvalue_vals, &ri->tenant_info.permissions_basedn );
				break;
			case RBAC_DEFAULT_SESSIONS_BASE_DN:
				value_add_one(
						&c->rvalue_vals, &ri->tenant_info.sessions_basedn );
				break;
			case RBAC_DEFAULT_AUDIT_BASE_DN:
				value_add_one( &c->rvalue_vals, &ri->tenant_info.audit_basedn );
				break;
			case RBAC_ADMIN_DN:
				value_add_one( &c->rvalue_vals, &ri->tenant_info.admin );
				break;
			case RBAC_ADMIN_PWD:
				value_add_one( &c->rvalue_vals, &ri->tenant_info.pwd );
				break;
			case RBAC_SESSION_ADMIN_DN:
				value_add_one(
						&c->rvalue_vals, &ri->tenant_info.session_admin );
				break;
			case RBAC_DEFAULT_TENANT_ID:
				value_add_one( &c->rvalue_vals, &ri->tenant_info.tid );
				break;
			default:
				break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		/* FIXME */
		return 1;
	}
	switch ( c->type ) {
		case RBAC_DEFAULT_USERS_BASE_DN: {
			struct berval dn = BER_BVNULL;
			ber_str2bv( c->argv[1], 0, 0, &dn );
			rc = dnNormalize(
					0, NULL, NULL, &dn, &ri->tenant_info.users_basedn, NULL );
			break;
		}
		case RBAC_DEFAULT_ROLES_BASE_DN: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.roles_basedn );
			break;
		}
		case RBAC_DEFAULT_PERMISSIONS_BASE_DN: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.permissions_basedn );
			break;
		}
		case RBAC_DEFAULT_SESSIONS_BASE_DN: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.sessions_basedn );
			break;
		}
		case RBAC_DEFAULT_AUDIT_BASE_DN: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.audit_basedn );
			break;
		}
		case RBAC_ADMIN_DN: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.admin );
			break;
		}
		case RBAC_ADMIN_PWD: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.pwd );
			break;
		}
		case RBAC_SESSION_ADMIN_DN: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.session_admin );
			break;
		}
		case RBAC_SESSION_ADMIN_PWD: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.session_admin_pwd );
			break;
		}
		case RBAC_DEFAULT_TENANT_ID: {
			ber_str2bv( c->argv[1], 0, 1, &ri->tenant_info.tid );
			break;
		}
		case RBAC_TENANT: {
			rbac_tenant_parse( c->argv[1], &ri->tenant_info );
			break;
		}
		default:
			break;
	}

	return rc;
}

/*
 * rbac configuration
 */

tenant_info_t *
rbac_tid2tenant( struct berval *tid )
{
	/* return the only tenant for now */
	return &rbac_tenants.tenant_info;
}

//{ BER_BVC(LDAP_RBAC_EXOP_SESSION_ROLES), rbac_session_roles },

static int
slap_parse_rbac_session_roles(
		struct berval *in,
		rbac_req_t **reqpp,
		const char **text,
		void *ctx )
{
	int rc = LDAP_SUCCESS;
	struct berval reqdata = BER_BVNULL;
	rbac_req_t *reqp = NULL;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len = -1;

	*text = NULL;

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in rbac_session_roles exop";
		return LDAP_PROTOCOL_ERROR;
	}

	reqp = rbac_alloc_req( RBAC_REQ_SESSION_ROLES );

	if ( !reqp ) {
		*text = "unable to allocate memory for rbac_session_roles exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_session_roles: "
				"decoding error.\n" );
		goto decoding_error;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_RBAC_USER_ID_SESS ) {
		struct berval uid;
		tag = ber_scanf( ber, "m", &uid );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_session_roles: "
					"user id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->uid, &uid, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	//tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_RBAC_SESSION_ID_SESS ) {
		struct berval sessid;
		tag = ber_scanf( ber, "m", &sessid );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_session_roles: "
					"session id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->sessid, &sessid, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;

		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_session_roles: "
				"decoding error, len=%ld\n",
				(long)len );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "data decoding error";
	}

	if ( rc == LDAP_SUCCESS ) {
		*reqpp = reqp;
	} else {
		rbac_free_req( reqp );
		*reqpp = NULL;
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

static int
rbac_session_roles( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	const struct berval rbac_op = BER_BVC("SessionRoles");
	rbac_req_t *reqp = NULL;
	rbac_session_t *sessp;
	int rc;

	rs->sr_err = slap_parse_rbac_session_roles(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_roles: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* checking whether the session is owned by the user */
	if ( !rbac_is_session_owner( sessp, reqp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_roles: "
				"session not owned by user\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	rc = rbac_int_delete_session( op, sessp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_roles: "
				"unable to delete session\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/*
	 * If we wanted to...
	 * load these roles into a response with a sequence nested within a
	 * sequence: (No, we're not actually doing this here.)
	 *		0x30 LL		ber_printf( ber,  "{" );
	 *			0x04 L1
	 *				0x04 L2 a b c d
	 *				0x04 L3 e f g h
	 *				0x04 L4 i j k l
	 * add all three ber_bvarray_add_x( &roles, &tmpbv, NULL );
	 * close it      ber_printf( ber, "t{W}", LDAP_TAG_EXOP_RBAC_ROLES, roles );
	 */

	/*
	 * Instead we are...
	 * loading these roles into the response within a sequence:  (Yes, we are doing this here.)
	 *		0x30 LL		ber_printf( ber,  "{" );
	 *			0x04 L1 a b c d
	 *			0x04 L2 e f g h
	 *			0x04 L3 i j k l
	 *	add all three ber_bvarray_add_x( &roles, &tmpbv, NULL );
	 *	close it      ber_printf( ber, "tW", LDAP_TAG_EXOP_RBAC_ROLES, roles );
	 */
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_init_w_nullc( ber, LBER_USE_DER );
	BerVarray roles = NULL;
	if ( sessp->roles ) {
		struct berval tmpbv;
		// open the sequence:
		ber_printf( ber, "{" /*}*/ );
		//char *role;
		int i = 0;
		//BerVarray roles = NULL;
		for ( i = 0; !BER_BVISNULL( &sessp->roles[i] ); i++ ) {
			tmpbv.bv_val = sessp->roles[i].bv_val;
			tmpbv.bv_len = sessp->roles[i].bv_len;
			// add role name:
			ber_bvarray_add_x( &roles, &tmpbv, NULL );

			//LBER_F( int )
			//ber_bvecadd_x LDAP_P(( struct berval ***bvec,
			//	struct berval *bv, void *ctx ));

			// first attempt at sequence within a sequence...
			// open another sequence:
			/*
			ber_printf( ber, "{" } );
			// add role name (again):
			ber_bvarray_add_x(&roles, &tmpbv, NULL);
			// close the nested sequence:
			ber_printf( ber, { "}" );
*/
			// end 2nd sequence
		}
		/*
		 * This is how we add several octet strings at one time. An array of struct berval's is supplied.
		 * The array is terminated by a struct berval with a NULL bv_val.
		 * Note that a construct like '{W}' is required to get an actual SEQUENCE OF octet strings.
		 * But here we are using 'tW' which allows passing a collection of octets w/out a nesting within a sequence.
         */
		ber_printf( ber, "tW",
				LDAP_TAG_EXOP_RBAC_ROLES, roles);

		// TODO: determine why free on roles array causes a seg fault:
		//ber_bvarray_free_x(roles, NULL);

		// close the sequence:
		ber_printf( ber, /*{*/ "N}" );
	}

	if ( rc < 0 ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
	} else {
		(void)ber_flatten( ber, &rs->sr_rspdata );
		rs->sr_err = LDAP_SUCCESS;
	}
	ber_free_buf( ber );
	// END LOAD ROLES INTO RESPONSE

done:;
	rs->sr_err = rc;

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_SESSION_ROLES.bv_val );

	/* generate audit log */
	rbac_audit(
			op, SessionRoles, sessp, reqp, rs->sr_err, (char *)rs->sr_text );
	rbac_free_session( sessp );
	rbac_free_req( reqp );
	return rs->sr_err;
}

static int
rbac_session_rolesx( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	const struct berval rbac_op = BER_BVC("SessionRoles");
	rbac_session_t *sessp = NULL;
	rbac_req_t *reqp = NULL;
	int rc;

	rs->sr_err = slap_parse_rbac_session_roles(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_roles: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* checking whether the session is owned by the user */
	if ( !rbac_is_session_owner( sessp, reqp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_roles: "
				"session not owned by user\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	rc = rbac_int_delete_session( op, sessp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_roles: "
				"unable to delete session\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/*
	 * If we wanted to...
	 * load these roles into a response with a sequence nested within a
	 * sequence: (No, we're not actually doing this here.)
	 *		0x30 LL		ber_printf( ber,  "{" );
	 *			0x04 L1
	 *				0x04 L2 a b c d
	 *				0x04 L3 e f g h
	 *				0x04 L4 i j k l
	 * add all three ber_bvarray_add_x( &roles, &tmpbv, NULL );
	 * close it      ber_printf( ber, "t{W}", LDAP_TAG_EXOP_RBAC_ROLES, roles );
	 */

	/*
	 * Instead we are...
	 * loading these roles into the response within a sequence:  (Yes, we are doing this here.)
	 *		0x30 LL		ber_printf( ber,  "{" );
	 *			0x04 L1 a b c d
	 *			0x04 L2 e f g h
	 *			0x04 L3 i j k l
	 *	add all three ber_bvarray_add_x( &roles, &tmpbv, NULL );
	 *	close it      ber_printf( ber, "tW", LDAP_TAG_EXOP_RBAC_ROLES, roles );
	 */
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_init_w_nullc( ber, LBER_USE_DER );
	BerVarray roles = NULL;
	if ( sessp->roles ) {
		struct berval tmpbv;
		// open the sequence:
		ber_printf( ber, "{" /*}*/ );
		//char *role;
		int i = 0;
		//BerVarray roles = NULL;
		for ( i = 0; !BER_BVISNULL( &sessp->roles[i] ); i++ ) {
			tmpbv.bv_val = sessp->roles[i].bv_val;
			tmpbv.bv_len = sessp->roles[i].bv_len;
			// add role name:
			ber_bvarray_add_x( &roles, &tmpbv, NULL );

			// first attempt at sequence within a sequence...
			// open another sequence:
			/*
			ber_printf( ber, "{" } );
			// add role name (again):
			ber_bvarray_add_x(&roles, &tmpbv, NULL);
			// close the nested sequence:
			ber_printf( ber, { "}" );
*/
			// end 2nd sequence
		}
		/*
		 * This is how we add several octet strings at one time. An array of struct berval's is supplied.
		 * The array is terminated by a struct berval with a NULL bv_val.
		 * Note that a construct like '{W}' is required to get an actual SEQUENCE OF octet strings.
		 * But here we are using 'tW' which allows passing a collection of octets w/out a nesting within a sequence.
         */
		ber_printf( ber, "tW",
				LDAP_TAG_EXOP_RBAC_ROLES, roles);

		// TODO: determine why free on roles array causes a seg fault:
		//ber_bvarray_free_x(roles, NULL);

		// close the sequence:
		ber_printf( ber, /*{*/ "N}" );
	}

	if ( rc < 0 ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
	} else {
		(void)ber_flatten( ber, &rs->sr_rspdata );
		rs->sr_err = LDAP_SUCCESS;
	}
	ber_free_buf( ber );
	// END LOAD ROLES INTO RESPONSE

done:;
	rs->sr_err = rc;

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_SESSION_ROLES.bv_val );

	/* generate audit log */
	rbac_audit(
			op, SessionRoles, sessp, reqp, rs->sr_err, (char *)rs->sr_text );
	rbac_free_session( sessp );
	rbac_free_req( reqp );
	return rs->sr_err;
}

/*
 * slap_parse_rbac_create_session
 */
static int
slap_parse_rbac_create_session(
		struct berval *in,
		rbac_req_t **reqpp,
		const char **text,
		void *ctx )
{
	int rc = LDAP_SUCCESS;
	struct berval reqdata = BER_BVNULL;
	rbac_req_t *reqp = NULL;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len = -1;

	*text = NULL;

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in rbac_create_session exop";
		return LDAP_PROTOCOL_ERROR;
	}

	reqp = rbac_alloc_req( RBAC_REQ_CREATE_SESSION );

	if ( !reqp ) {
		*text = "unable to allocate memory for bac_create_session exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
				"decoding error.\n" );
		goto decoding_error;
	}

	// Order: 1. sessionId, 2. tenantId, 3. userId, 4. password and 5. roles
	/* must-have */
	tag = ber_peek_tag( ber, &len );

	// 1. SESSIONID
	if ( tag == LDAP_TAG_EXOP_RBAC_SESSION_ID ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
					"session id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->sessid, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// 2. TENANT ID
	if ( tag == LDAP_TAG_EXOP_RBAC_TENANT_ID ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
					"tenant id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->tenantid, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// 3. USERID
	if ( tag == LDAP_TAG_EXOP_RBAC_USER_ID ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
					"user id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->uid, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// 4. PASSWORD
	if ( tag == LDAP_TAG_EXOP_RBAC_AUTHTOK ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv);
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
					"authtok parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->authtok, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// 5. ROLES
	if ( tag == LDAP_TAG_EXOP_RBAC_ACTIVE_ROLE ) {
		tag = ber_scanf( ber, "W", &reqp->roles);
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
					"role parse failed.\n" );
			goto decoding_error;
		}
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;

		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
				"decoding error, len=%ld\n",
				(long)len );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "data decoding error";
	}

	if ( rc == LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "slap_parse_rbac_create_session: "
				"SUCCESS\n" );

		*reqpp = reqp;
	} else {
		Debug( LDAP_DEBUG_ANY, "slap_parse_rbac_create_session: "
				"NO SUCCESS RC=%d\n", rc );

		rbac_free_req( reqp );
		*reqpp = NULL;
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

/*
 * rbac_create_session:
 *     1. authenticate user
 *     2. evaluate pwd policy
 *     3. create session
 */
static int
rbac_create_session( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	struct berval rbac_op = BER_BVC("CreateSession");
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	int rc = LDAP_SUCCESS;
	rbac_session_t *sessp = NULL;
	rbac_user_t *userp = NULL;
	rbac_req_t *reqp = NULL;

	rs->sr_err = slap_parse_rbac_create_session(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	/* read user entry */
	userp = rbac_read_user( op, reqp );
	if ( !userp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_create_session: "
				"unable to read user entry\n" );
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_text = "rbac_create_session: unable to read user entry";
		goto done;
	}

	if ( !BER_BVISNULL( &userp->password ) ) {
		/* if request is with pwd, authenticate the user */
		rc = rbac_authenticate_user( op, userp );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "rbac_create_session: "
					"rbac_authenticate_user failed!\n" );
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			rs->sr_text = "rbac_create_session: invalid credential";
			goto done;
		}
		/*
		rbac_user_t *ui = op->o_callback->sc_private;
		int pVal = ui->authz;
		printf("password reset val=%d", pVal );
*/

	} else {
		/* no pwd is provided, check whether the requesting session */
		/* id has the access privilege to create a session on behalf */
		/* of the user */
		rc = rbac_create_session_acl_check( &reqp->sessid, userp );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "rbac_create_session: "
					"rbac_authenticate_user failed!\n" );
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "rbac_create_session: session permission denied";
			goto done;
		}
	}

	/* check user temporal constraint */
	rc = rbac_user_temporal_constraint( userp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_create_session: "
				"rbac_user_temporal_constraint() failed!\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "rbac_create_session: temporal constraint violation";
		goto done;
	}

	sessp = rbac_alloc_session();
	if ( !sessp ) {
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "rbac_create_session: unable to allocate session";
		goto done;
	}

	rc = activate_session_roles( sessp, reqp, userp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_create_session: "
				"failed to activate roles to session!\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text =
				"rbac_create_session: failed to activate roles into session";
		goto done;
	}

	/* store uid and tenant id in session */
	ber_dupbv( &sessp->userdn, &userp->dn );
	ber_dupbv( &sessp->uid, &reqp->uid );
	ber_dupbv( &sessp->tenantid, &reqp->tenantid );

	/* register RBAC session */
	rc = rbac_register_session( op, rs, sessp );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	ber_init_w_nullc( ber, LBER_USE_DER );
	rc = ber_printf( ber, "{tO}", LDAP_TAG_EXOP_RBAC_SESSION_ID,
			&sessp->sessid );
	if ( rc < 0 ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
	} else {
		(void)ber_flatten( ber, &rs->sr_rspdata );
		rs->sr_rspoid = ch_strdup( slap_EXOP_CREATE_SESSION.bv_val );
		rs->sr_err = LDAP_SUCCESS;
	}

	ber_free_buf( ber );

done:;

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CREATE_SESSION.bv_val );
	/* generate audit log */
	rbac_audit(
			op, CreateSession, sessp, reqp, rs->sr_err, (char *)rs->sr_text );

	rbac_free_req( reqp );
	rbac_free_session( sessp );

	//if (rs->sr_err != LDAP_SUCCESS) {
	//send_ldap_result( op, rs );
	//}

	return rs->sr_err;
}

/*
 * slap_parse_rbac_check_access
 */
static int
slap_parse_rbac_check_access(
		struct berval *in,
		rbac_req_t **reqpp,
		const char **text,
		void *ctx )
{
	int rc = LDAP_SUCCESS;
	struct berval reqdata = BER_BVNULL;
	rbac_req_t *reqp = NULL;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;

	*text = NULL;
	reqp = rbac_alloc_req( RBAC_REQ_CHECK_ACCESS );

	if ( !reqp ) {
		*text = "unable to allocate memory for slap_parse_rbac_check_access";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in rbac_check_access exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
				"decoding error.\n" );
		goto decoding_error;
	}

	// sessionId is required:
	tag = ber_peek_tag( ber, &len );
	if ( tag != LDAP_TAG_EXOP_RBAC_SESSION_ID ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
				"decoding error.\n" );
		goto decoding_error;
	} else {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
					"session id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->sessid, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// operationName is required:
	if ( tag != LDAP_TAG_EXOP_RBAC_OPNAME ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
				"decoding error.\n" );
		goto decoding_error;
	} else {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
					"opname parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->opname, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// objectName is required:
	if ( tag != LDAP_TAG_EXOP_RBAC_OBJNAME ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
				"decoding error.\n" );
		goto decoding_error;
	} else {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
					"objname parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->objname, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	// objectId is optional:
	if ( tag == LDAP_TAG_EXOP_RBAC_OBJID ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
					"objid parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->objid, &bv, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;

		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_check_access: "
				"decoding error, len=%ld\n",
				(long)len );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "data decoding error";
	}

	if ( rc == LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "slap_parse_rbac_check_access: "
				"SUCCESS\n" );
		*reqpp = reqp;
	} else {
		Debug( LDAP_DEBUG_ANY, "slap_parse_rbac_check_access: "
				"FAIL\n" );
		rbac_free_req( reqp );
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

// checkAcess F  (ALL)
static int
rbac_check_access( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_session_t *sessp = NULL;
	rbac_permission_t *permp = NULL;
	rbac_constraint_t *cp = NULL;
	rbac_req_t *reqp = NULL;
	const struct berval rbac_op = BER_BVC("CheckAccess");
	int rc = LDAP_SUCCESS;
	int found = 0;

	rs->sr_err = slap_parse_rbac_check_access(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	BER_BVZERO( &op->o_req_dn );
	BER_BVZERO( &op->o_req_ndn );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* read the permission using objectName and OpName */
	permp = rbac_read_permission( op, reqp );
	if ( !permp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"permission not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	// Convert the user-role constraint data from BerVarray to rbac_constraint_t format
	cp = rbac_user_role_constraints( sessp->role_constraints );

	// Now do the actual rbac checkAccess:
	rc = rbac_check_session_permission( sessp, permp, cp );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_user_permission: "
				"failed\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

done:

	rs->sr_err = rc;
	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CHECK_ACCESS.bv_val );

	/* generate audit log */
	rbac_audit( op, CheckAccess, sessp, reqp, rs->sr_err, (char *)rs->sr_text );

	rbac_free_permission( permp );
	rbac_free_req( reqp );
	rbac_free_session( sessp );
	rbac_free_constraints( cp );

	return rs->sr_err;
}

// checkAcess A loop back
static int
rbac_check_accessA( Operation *op, SlapReply *rs )
{
	int rc = LDAP_SUCCESS;

	//rs->sr_err = slap_parse_rbac_check_access(op->ore_reqdata,
	//	&reqp, &rs->sr_text, NULL);

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CHECK_ACCESS.bv_val );
	rs->sr_err = rc;

	return rc;
}

// checkAcess B parse
static int
rbac_check_accessB( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_req_t *reqp = NULL;
	const struct berval rbac_op = BER_BVC("CheckAccess");
	int rc = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_ANY, "rbac_check_access\n" );

	rs->sr_err = slap_parse_rbac_check_access(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	BER_BVZERO( &op->o_req_dn );
	BER_BVZERO( &op->o_req_ndn );

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CHECK_ACCESS.bv_val );
	rs->sr_err = rc;

	rbac_free_req( reqp );

	return rc;
}

// checkAcess C - parse request & read session record
static int
rbac_check_accessC( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_session_t *sessp = NULL;
	rbac_req_t *reqp = NULL;
	const struct berval rbac_op = BER_BVC("CheckAccess");
	int rc = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_ANY, "rbac_check_access\n" );

	rs->sr_err = slap_parse_rbac_check_access(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	BER_BVZERO( &op->o_req_dn );
	BER_BVZERO( &op->o_req_ndn );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

done:

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CHECK_ACCESS.bv_val );
	rs->sr_err = rc;

	rbac_free_req( reqp );
	rbac_free_session( sessp );
	return rc;
}

// checkAcess D, parse, read perm
static int
rbac_check_accessD( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_permission_t *permp = NULL;
	rbac_req_t *reqp = NULL;
	const struct berval rbac_op = BER_BVC("CheckAccess");
	int rc = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_ANY, "rbac_check_access\n" );

	rs->sr_err = slap_parse_rbac_check_access(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	BER_BVZERO( &op->o_req_dn );
	BER_BVZERO( &op->o_req_ndn );

	/* get the session using the session id */
	/*
	sessp = rbac_session_byid(op, reqp);
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}
*/

	/* read the permission using objectName and OpName */
	permp = rbac_read_permission( op, reqp );
	if ( !permp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"permission not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

done:

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CHECK_ACCESS.bv_val );
	rs->sr_err = rc;

	rbac_free_permission( permp );
	rbac_free_req( reqp );

	return rc;
}

// checkAcess E everything but the audit insert
static int
rbac_check_accessE( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_session_t *sessp = NULL;
	rbac_permission_t *permp = NULL;
	rbac_constraint_t *cp = NULL;
	rbac_req_t *reqp = NULL;
	const struct berval rbac_op = BER_BVC("CheckAccess");
	int rc = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_ANY, "rbac_check_access\n" );

	rs->sr_err = slap_parse_rbac_check_access(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	BER_BVZERO( &op->o_req_dn );
	BER_BVZERO( &op->o_req_ndn );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* read the permission using objectName and OpName */
	permp = rbac_read_permission( op, reqp );
	if ( !permp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_access: "
				"permission not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	// Convert the user-role constraint data from BerVarray to rbac_constraint_t format
	cp = rbac_user_role_constraints( sessp->role_constraints );

	// Now do the actual rbac checkAccess:
	rc = rbac_check_session_permission( sessp, permp, cp );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_check_user_permission: "
				"failed\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

done:

	rs->sr_err = rc;
	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_CHECK_ACCESS.bv_val );

	/* generate audit log */
	//rbac_audit(op, CheckAccess, sessp, reqp, rs->sr_err,
	//	(char *) rs->sr_text);

	rbac_free_permission( permp );
	rbac_free_req( reqp );
	rbac_free_session( sessp );
	rbac_free_constraints( cp );

	return rs->sr_err;
}

/* check whether role exists and role assigned to the user */
static int
rbac_check_user_role(
		rbac_req_t *reqp,
		rbac_session_t *sessp,
		rbac_user_t *userp )
{
	int rc = 0;
	int i;

	//assert(!BER_BVISEMPTY(&reqp->roles[0]));
	assert( !BER_BVISEMPTY( &reqp->role ) );

	for ( i = 0; !BER_BVISNULL( &userp->roles[i] ); i++ ) {
		//if (!ber_bvstrcasecmp(&userp->roles[i], &reqp->roles[0])) {
		if ( !ber_bvstrcasecmp( &userp->roles[i], &reqp->role ) ) {
			rc = 1; /* found the match */
			goto done;
		}
	}

done:;

	return rc;
}

/* check whether role exists and role assigned to the session */
static int
rbac_check_session_role( rbac_req_t *reqp, rbac_session_t *sessp )
{
	int rc = 0;
	int i;

	for ( i = 0; !BER_BVISNULL( &sessp->roles[i] ); i++ ) {
		//if (!ber_bvstrcasecmp(&sessp->roles[i], &reqp->roles[0])) {
		if ( !ber_bvstrcasecmp( &sessp->roles[i], &reqp->role ) ) {
			rc = 1; /* found the match */
			goto done;
		}
	}

done:;

	return rc;
}

/* make sure user is the owner of the session */
static int
rbac_check_user_session( rbac_session_t *sessp, rbac_req_t *reqp )
{
	int rc = 0;

	if ( BER_BVISNULL( &sessp->uid ) || BER_BVISNULL( &reqp->uid ) ||
			sessp->uid.bv_len != reqp->uid.bv_len ) {
		goto done;
	}

	if ( !strncasecmp(
				sessp->uid.bv_val, reqp->uid.bv_val, reqp->uid.bv_len ) ) {
		rc = 1;
		goto done;
	}

done:;

	return rc;
}

/*
 * slap_parse_rbac_active_role
 */
static int
slap_parse_rbac_active_role(
		struct berval *in,
		int add_or_drop_role,
		rbac_req_t **reqpp,
		const char **text,
		void *ctx )
{
	int rc = LDAP_SUCCESS;
	struct berval reqdata = BER_BVNULL;
	rbac_req_t *reqp = NULL;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len = -1;

	*text = NULL;

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in rbac_create_session exop";
		return LDAP_PROTOCOL_ERROR;
	}

	reqp = rbac_alloc_req( add_or_drop_role );

	if ( !reqp ) {
		*text = "unable to allocate memory for rbac_add_drop_active_role exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_active_role: "
				"decoding error.\n" );
		goto decoding_error;
	}

	tag = ber_peek_tag( ber, &len );
	//if ( tag == LDAP_TAG_EXOP_RBAC_USER_ID ) {
	if ( tag == LDAP_TAG_EXOP_RBAC_USER_ID_SESS ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_active_role: "
					"user id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv( &reqp->uid, &bv );
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LDAP_TAG_EXOP_RBAC_SESSION_ID_SESS ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_active_role: "
					"session id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv( &reqp->sessid, &bv );
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LDAP_TAG_EXOP_RBAC_ROLE_NM_SESS ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		//tag = ber_scanf( ber, "W", &reqp->roles);
		//tag = ber_scanf( ber, "m", &reqp->roles);
		//tag = ber_scanf( ber, "m", &reqp->roles[0]);
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
					"role parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv( &reqp->role, &bv );
		//ber_dupbv(&reqp->roles[0], &bv);
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_create_session: "
				"decoding error, len=%ld\n",
				(long)len );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "data decoding error";
	}

	if ( rc == LDAP_SUCCESS ) {
		*reqpp = reqp;
	} else {
		rbac_free_req( reqp );
		*reqpp = NULL;
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

static int
rbac_add_active_role( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	struct berval rbac_op = BER_BVC("AddActiveRole");
	rbac_req_t *reqp = NULL;
	rbac_user_t *userp = NULL;
	rbac_session_t *sessp;
	int rc = LDAP_SUCCESS;

	rs->sr_err = slap_parse_rbac_active_role( op->ore_reqdata,
			RBAC_REQ_ADD_ACTIVE_ROLE, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
				"session not found\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "rbac_add_active_role: session not found";
		goto done;
	}

	/* read user entry */
	userp = rbac_read_user( op, reqp );
	if ( !userp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
				"unable to read user entry\n" );
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_text = "rbac_add_active_role: unable to read user entry";
		goto done;
	}

	/* make sure role exists and role assigned to the user */
	if ( !rbac_check_user_role( reqp, sessp, userp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
				"role not assigned to the user\n" );
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_text = "rbac_add_active_role: role not assigned to the user";
		goto done;
	}

	/* make sure user is the owner of the session */
	if ( !rbac_check_user_session( sessp, reqp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
				"user not owner of session\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "rbac_add_active_role: user not owner of the session";
		goto done;
	}

	/* add the role to the session */
	rc = rbac_session_add_role( op, sessp, reqp );
	if ( rc != LDAP_SUCCESS ) {
		rs->sr_err = rc;
		if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
			rs->sr_text =
					"rbac_add_active_role: role already activated in session";
			Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
					"role already activated in session\n" );
		} else {
			rs->sr_text = "rbac_add_active_role: unable to add role to session";
			Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
					"unable to add role to session\n" );
		}
		goto done;
	}

done:

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_ADD_ACTIVE_ROLE.bv_val );

	/* generate audit log */
	rbac_audit(
			op, AddActiveRole, sessp, reqp, rs->sr_err, (char *)rs->sr_text );

	rbac_free_session( sessp );
	rbac_free_user( userp );
	rbac_free_req( reqp );

	return rs->sr_err;
}

static int
rbac_drop_active_role( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	const struct berval rbac_op = BER_BVC("DropActiveRole");
	rbac_session_t *sessp;
	rbac_req_t *reqp = NULL;
	int rc = LDAP_SUCCESS;

	rs->sr_err = slap_parse_rbac_active_role( op->ore_reqdata,
			RBAC_REQ_DROP_ACTIVE_ROLE, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_drop_active_role: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	if ( BER_BVISNULL( &reqp->role ) || !sessp->roles ||
			BER_BVISNULL( &sessp->roles[0] ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_drop_active_role: "
				"unavailable role\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* make sure role exists and role assigned to the user */
	if ( !rbac_check_session_role( reqp, sessp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_drop_active_role: "
				"role not assigned to session\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* make sure user is the owner of the session */
	if ( !rbac_check_user_session( sessp, reqp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_drop_active_role: "
				"user not owner of session\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "rbac_drop_active_role: user not owner of the session";
		goto done;
	}

	/* drop the role to the session */
	rc = rbac_session_drop_role( op, sessp, reqp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_drop_active_role: "
				"unable to drop active role from session\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "rbac_drop_active_role: unable to drop role from session";
		goto done;
	}

done:
	rs->sr_err = rc;

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_DROP_ACTIVE_ROLE.bv_val );

	/* generate audit log */
	rbac_audit(
			op, DropActiveRole, sessp, reqp, rs->sr_err, (char *)rs->sr_text );

	rbac_free_session( sessp );
	rbac_free_req( reqp );

	return rs->sr_err;
}

/*
 * slap_parse_rbac_delete_session
 */
static int
slap_parse_rbac_delete_session(
		struct berval *in,
		rbac_req_t **reqpp,
		const char **text,
		void *ctx )
{
	int rc = LDAP_SUCCESS;
	struct berval reqdata = BER_BVNULL;
	rbac_req_t *reqp = NULL;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len = -1;

	*text = NULL;

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in rbac_delete_session exop";
		return LDAP_PROTOCOL_ERROR;
	}

	reqp = rbac_alloc_req( RBAC_REQ_DELETE_SESSION );

	if ( !reqp ) {
		*text = "unable to allocate memory for rbac_delete_session exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_delete_session: "
				"decoding error.\n" );
		goto decoding_error;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_RBAC_USER_ID_SESS ) {
		struct berval uid;
		tag = ber_scanf( ber, "m", &uid );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_delete_session: "
					"user id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->uid, &uid, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	//tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_RBAC_SESSION_ID_SESS ) {
		struct berval sessid;
		tag = ber_scanf( ber, "m", &sessid );
		if ( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_delete_session: "
					"session id parse failed.\n" );
			goto decoding_error;
		}
		ber_dupbv_x( &reqp->sessid, &sessid, ctx );
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;

		Debug( LDAP_DEBUG_TRACE, "slap_parse_rbac_delete_session: "
				"decoding error, len=%ld\n",
				(long)len );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "data decoding error";
	}

	if ( rc == LDAP_SUCCESS ) {
		*reqpp = reqp;
	} else {
		rbac_free_req( reqp );
		*reqpp = NULL;
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

static int
rbac_delete_session( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	const struct berval rbac_op = BER_BVC("DeleteSession");
	rbac_session_t *sessp = NULL;
	rbac_req_t *reqp = NULL;
	int rc;

	rs->sr_err = slap_parse_rbac_delete_session(
			op->ore_reqdata, &reqp, &rs->sr_text, NULL );

	assert( rs->sr_err == LDAP_SUCCESS );

	/* get the session using the session id */
	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_delete_session: "
				"session not found\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* checking whether the session is owned by the user */
	if ( !rbac_is_session_owner( sessp, reqp ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_delete_session: "
				"session not owned by user\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	rc = rbac_int_delete_session( op, sessp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_int_delete_session: "
				"unable to delete session\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

done:;

	rs->sr_err = rc;

	// always put the OID in the response:
	rs->sr_rspoid = ch_strdup( slap_EXOP_DELETE_SESSION.bv_val );

	/* generate audit log */
	rbac_audit(
			op, DeleteSession, sessp, reqp, rs->sr_err, (char *)rs->sr_text );

	rbac_free_session( sessp );
	rbac_free_req( reqp );

	return rs->sr_err;
}

/* returns the permissions associated with a session */
static int
rbac_session_permissions( Operation *op, SlapReply *rs, rbac_req_t *reqp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	const struct berval rbac_op = BER_BVC("SessionPermissions");
	rbac_session_t *sessp;

	sessp = rbac_session_byid( op, reqp );
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_permissions: "
				"session id not found\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	rs->sr_err = rbac_int_session_permissions( op, rs, reqp, sessp );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_permissions: "
				"permissions not found\n" );
		goto done;
	}

done:;
	return rs->sr_err;
}

/* extract session permission info from op */
int
rbac_search_parse_session_permissions_req(
		Operation *op,
		rbac_req_t **reqpp,
		const char **text,
		void *ctx )
{
	int rc = LDAP_SUCCESS;
	struct berval *sessid = NULL;
	rbac_req_t *reqp = NULL;
	*text = NULL;
	struct berval rbac_session_id = BER_BVC("sessionID");
	struct berval rbac_session_permissions_attr =
			BER_BVC("sessionPermissions");
	AttributeDescription *ad = NULL;
	Filter *f;

	/* check simple assertion (sessionID=<session id>) */
	f = op->ors_filter;
	ad = f->f_ava->aa_desc;
	if ( !ad || ber_bvstrcasecmp( &rbac_session_id, &ad->ad_cname ) ) {
		goto done;
	}
	sessid = &f->f_ava->aa_value;

	if ( !rbac_is_valid_session_id( sessid ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_search_parse_session_permissions_req: "
				"invalid session id\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* check requested attr */

	if ( !op->oq_search.rs_attrs ||
			BER_BVISNULL( &op->oq_search.rs_attrs[0].an_name ) ||
			ber_bvstrcasecmp( &op->oq_search.rs_attrs[0].an_name,
					&rbac_session_permissions_attr ) ||
			!BER_BVISNULL( &op->oq_search.rs_attrs[1].an_name ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_search_parse_session_permissions_req: "
				"only sessionPermissions allowed\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	reqp = rbac_alloc_req( RBAC_REQ_SESSION_PERMISSIONS );
	if ( !reqp ) {
		*text = "unable to allocate memory for rbac_session_permissions req";
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* retrieve session id from search filter */
	ber_dupbv_x( &reqp->sessid, sessid, ctx );

done:;

	if ( rc == LDAP_SUCCESS ) {
		*reqpp = reqp;
	} else {
		rbac_free_req( reqp );
		*reqpp = NULL;
	}

	return rc;
}

static int
rbac_search( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_ANY, "rbac_search entry\n" );

	return SLAP_CB_CONTINUE;
}

/*
static int
rbac_search( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_req_t *reqp = NULL;
	int rc = SLAP_CB_CONTINUE;

	 only session_permissions is implemented for now
	rc = rbac_search_parse_session_permissions_req(
			op, &reqp, &rs->sr_text, NULL );
	if ( !reqp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_search: "
				"invalid search for session permissions\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	rc = rbac_session_permissions( op, rs, reqp );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_search: "
				"session permissions failed\n" );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	rs->sr_err = LDAP_SUCCESS;

done:;
	send_ldap_result( op, rs );

	return rc;
}
*/

static struct exop {
	struct berval oid;
	BI_op_extended *extended;
} rbac_exop_table[] = {
	{ BER_BVC(LDAP_RBAC_EXOP_CREATE_SESSION), rbac_create_session },
	{ BER_BVC(LDAP_RBAC_EXOP_CHECK_ACCESS), rbac_check_access },
	{ BER_BVC(LDAP_RBAC_EXOP_ADD_ACTIVE_ROLE), rbac_add_active_role },
	{ BER_BVC(LDAP_RBAC_EXOP_DROP_ACTIVE_ROLE), rbac_drop_active_role },
	{ BER_BVC(LDAP_RBAC_EXOP_DELETE_SESSION), rbac_delete_session },
	{ BER_BVC(LDAP_RBAC_EXOP_SESSION_ROLES), rbac_session_roles },

	{ BER_BVNULL, NULL }
};

static int
rbac_add( Operation *op, SlapReply *rs )
{
	return SLAP_CB_CONTINUE;
}

static int
rbac_bind( Operation *op, SlapReply *rs )
{
	return SLAP_CB_CONTINUE;
}

static int
rbac_compare( Operation *op, SlapReply *rs )
{
	return SLAP_CB_CONTINUE;
}

static int
rbac_delete( Operation *op, SlapReply *rs )
{
	return SLAP_CB_CONTINUE;
}

static int
rbac_modify( Operation *op, SlapReply *rs )
{
	return SLAP_CB_CONTINUE;
}

static int
rbac_extended( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	int rc = SLAP_CB_CONTINUE;
	int i;

	for ( i = 0; rbac_exop_table[i].extended != NULL; i++ ) {
		if ( bvmatch( &rbac_exop_table[i].oid, &op->oq_extended.rs_reqoid ) ) {
			rc = rbac_exop_table[i].extended( op, rs );
			switch ( rc ) {
				case LDAP_SUCCESS:
					break;
				case SLAP_CB_CONTINUE:
				case SLAPD_ABANDON:
					return rc;
				default:
					send_ldap_result( op, rs );
					return rc;
			}
			break;
		}
	}

	return rc;
}

static int
rbac_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;

	return 0;
}

static int
rbac_db_open( BackendDB *be, ConfigReply *cr )
{
	int rc = LDAP_SUCCESS;

	rc = rbac_initialize_tenants( be, cr );

	return rc;
}

static int
rbac_db_close( BackendDB *be, ConfigReply *cr )
{
	return 0;
}

int
rbac_initialize()
{
	int rc;

	rc = load_extop2( (struct berval *)&slap_EXOP_CREATE_SESSION,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, rbac_create_session, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
				"unable to register rbac_create_session exop: %d\n",
				rc );
		return rc;
	}

	rc = load_extop2( (struct berval *)&slap_EXOP_CHECK_ACCESS,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, rbac_check_access, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
				"unable to register rbac_check_access exop: %d\n",
				rc );
		return rc;
	}

	rc = load_extop2( (struct berval *)&slap_EXOP_ADD_ACTIVE_ROLE,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, rbac_add_active_role, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
				"unable to register rbac_add_active_role exop: %d\n",
				rc );
		return rc;
	}

	rc = load_extop2( (struct berval *)&slap_EXOP_DROP_ACTIVE_ROLE,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, rbac_drop_active_role, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
				"unable to register rbac_drop_active_role exop: %d\n",
				rc );
		return rc;
	}

	rc = load_extop2( (struct berval *)&slap_EXOP_DELETE_SESSION,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, rbac_delete_session, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
				"unable to register rbac_delete_session exop: %d\n",
				rc );
		return rc;
	}

	rc = load_extop2( (struct berval *)&slap_EXOP_SESSION_ROLES,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, rbac_session_roles, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
				"unable to register rbac_session_roles exop: %d\n",
				rc );
		return rc;
	}

	rbac.on_bi.bi_type = "rbac";
	rbac.on_bi.bi_db_init = rbac_db_init;
	rbac.on_bi.bi_db_open = rbac_db_open;
	rbac.on_bi.bi_db_close = rbac_db_close;

	rbac.on_bi.bi_op_add = rbac_add;
	rbac.on_bi.bi_op_bind = rbac_bind;
	rbac.on_bi.bi_op_compare = rbac_compare;
	rbac.on_bi.bi_op_delete = rbac_delete;
	rbac.on_bi.bi_op_modify = rbac_modify;
	rbac.on_bi.bi_op_search = rbac_search;
	rbac.on_bi.bi_extended = rbac_extended;
	rbac.on_bi.bi_cf_ocs = rbac_ocs;

	/*	rbac.on_bi.bi_connection_destroy = rbac_connection_destroy; */

	rc = config_register_schema( rbaccfg, rbac_ocs );
	if ( rc ) return rc;

	rc = rbac_initialize_repository();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	return overlay_register( &rbac );
}

int
init_module( int argc, char *argv[] )
{
	return rbac_initialize();
}
