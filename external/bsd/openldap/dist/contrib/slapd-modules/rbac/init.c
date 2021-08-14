/*	$NetBSD: init.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* init.c - RBAC initialization */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 *
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
__RCSID("$NetBSD: init.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

static slap_callback nullsc = { NULL, NULL, NULL, NULL };

struct slap_rbac_internal_schema slap_rbac_schema;

extern rbac_tenant_t rbac_tenants;
extern int initialize_jts( void );

rbac_ad_t rbac_session_ads[] = {
	{ RBAC_SESSION_ID,
		BER_BVC("rbacSessid"), &slap_rbac_schema.ad_session_id },
	{ RBAC_USER_DN,
		BER_BVC("rbacUserDN"), &slap_rbac_schema.ad_session_user_dn },
	{ RBAC_ROLES,
		BER_BVC("rbacRoles"), &slap_rbac_schema.ad_session_roles },
	{ RBAC_ROLE_CONSTRAINTS,
		BER_BVC("rbacRoleConstraints"),
		&slap_rbac_schema.ad_session_role_constraints },
	{ RBAC_UID,
		BER_BVC("uid"), &slap_rbac_schema.ad_uid},
	{ RBAC_TENANT_ID,
		BER_BVC("tenantid"), &slap_rbac_schema.ad_tenant_id },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

rbac_ad_t rbac_session_permission_ads[] = {
	{ RBAC_OP_NAME,
		BER_BVC("rbacOpName"), &slap_rbac_schema.ad_permission_opname },
	{ RBAC_OBJ_NAME,
		BER_BVC("rbacObjName"), &slap_rbac_schema.ad_permission_objname },
	{ RBAC_ROLE_NAME,
		BER_BVC("rbacRoleName"), &slap_rbac_schema.ad_permission_rolename },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

rbac_ad_t audit_ads[] = {
	{ RBAC_AUDIT_OP,
		BER_BVC("rbacAuditOp"), &slap_rbac_schema.ad_audit_op },
	{ RBAC_AUDIT_ID,
		BER_BVC("rbacAuditId"), &slap_rbac_schema.ad_audit_id },
	{ RBAC_AUDIT_ROLES,
		BER_BVC("rbacAuditRoles"), &slap_rbac_schema.ad_audit_roles },
	{ RBAC_AUDIT_REQUESTED_ROLES,
		BER_BVC("rbacAuditRequestedRoles"),
		&slap_rbac_schema.ad_audit_requested_roles
	},
	{ RBAC_AUDIT_TIMESTAMP,
		BER_BVC("rbacAuditTimestamp"), &slap_rbac_schema.ad_audit_timestamp },
	{ RBAC_AUDIT_RESOURCES,
		BER_BVC("rbacAuditResources"), &slap_rbac_schema.ad_audit_resources },
	{ RBAC_AUDIT_OBJS,
		BER_BVC("rbacAuditObjects"), &slap_rbac_schema.ad_audit_objects },
	{ RBAC_AUDIT_OPS,
		BER_BVC("rbacAuditOperations"), &slap_rbac_schema.ad_audit_operations },
	{ RBAC_AUDIT_RESULT,
		BER_BVC("rbacAuditResult"), &slap_rbac_schema.ad_audit_result },
	{ RBAC_AUDIT_PROPERTIES,
		BER_BVC("rbacAuditProperties"), &slap_rbac_schema.ad_audit_properties },
	{ RBAC_AUDIT_MSGS,
		BER_BVC("rbacAuditMessages"), &slap_rbac_schema.ad_audit_messages },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

/* initialize repository attribute descriptions */

static int
initialize_sessions()
{
	int i, nattrs, rc = LDAP_SUCCESS;
	const char *text;

	for ( nattrs = 0; !BER_BVISNULL( &rbac_session_ads[nattrs].attr );
			nattrs++ )
		; /* count the number of attrs */

	slap_rbac_schema.session_attrs =
			slap_sl_calloc( sizeof(AttributeName), nattrs + 1, NULL );

	for ( i = 0; !BER_BVISNULL( &rbac_session_ads[i].attr ); i++ ) {
		rc = slap_bv2ad(
				&rbac_session_ads[i].attr, rbac_session_ads[i].ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
		slap_rbac_schema.session_attrs[i].an_name = rbac_session_ads[i].attr;
		slap_rbac_schema.session_attrs[i].an_desc = *rbac_session_ads[i].ad;
	}

	BER_BVZERO( &slap_rbac_schema.session_attrs[nattrs].an_name );

done:;
	return rc;
}

static int
initialize_audit()
{
	int i, rc = LDAP_SUCCESS;
	const char *text;

	/* for audit */
	for ( i = 0; !BER_BVISNULL( &audit_ads[i].attr ); i++ ) {
		rc = slap_bv2ad( &audit_ads[i].attr, audit_ads[i].ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
	}

done:;
	return rc;
}

static int
initialize_tenant(
		BackendDB *be,
		ConfigReply *cr,
		tenant_info_t *tenantp,
		int init_op )
{
	int rc = LDAP_SUCCESS;
	Entry *e = NULL;
	OperationBuffer opbuf;
	Operation *op2;
	SlapReply rs2 = { REP_RESULT };
	Connection conn = { 0 };
	struct berval rbac_container_oc = BER_BVC("rbacContainer");
	struct berval rbac_audit_container = BER_BVC("audit");
	struct berval rbac_session_container = BER_BVC("rbac");
	void *thrctx = ldap_pvt_thread_pool_context();

	e = entry_alloc();

	switch ( init_op ) {
		case INIT_AUDIT_CONTAINER:
			ber_dupbv( &e->e_name, &tenantp->audit_basedn );
			ber_dupbv( &e->e_nname, &tenantp->audit_basedn );

			/* container cn */
			attr_merge_one(
					e, slap_schema.si_ad_cn, &rbac_audit_container, NULL );
			break;
		case INIT_SESSION_CONTAINER:
			ber_dupbv( &e->e_name, &tenantp->sessions_basedn );
			ber_dupbv( &e->e_nname, &tenantp->sessions_basedn );

			/* rendered dynmaicObject for session */
			attr_merge_one( e, slap_schema.si_ad_objectClass,
					&slap_schema.si_oc_dynamicObject->soc_cname, NULL );

			/* container cn */
			attr_merge_one(
					e, slap_schema.si_ad_cn, &rbac_session_container, NULL );
			break;
		default:
			break;
	}

	attr_merge_one(
			e, slap_schema.si_ad_objectClass, &rbac_container_oc, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
			&rbac_container_oc, NULL );

	/* store RBAC session */
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op2 = &opbuf.ob_op;
	op2->o_callback = &nullsc;
	op2->o_tag = LDAP_REQ_ADD;
	op2->o_protocol = LDAP_VERSION3;
	op2->o_req_dn = e->e_name;
	op2->o_req_ndn = e->e_nname;
	op2->ora_e = e;
	op2->o_bd = select_backend( &op2->o_req_ndn, 0 );
	op2->o_dn = op2->o_bd->be_rootdn;
	op2->o_ndn = op2->o_bd->be_rootndn;
	rc = op2->o_bd->be_add( op2, &rs2 );

	if ( e ) entry_free( e );

	return rc;
}

int
rbac_initialize_tenants( BackendDB *be, ConfigReply *cr )
{
	int rc = LDAP_SUCCESS;
	rbac_tenant_t *tenantp = NULL;

	for ( tenantp = &rbac_tenants; tenantp; tenantp = tenantp->next ) {
		rc = initialize_tenant(
				be, cr, &tenantp->tenant_info, INIT_AUDIT_CONTAINER );
		if ( rc != LDAP_SUCCESS ) {
			if ( rc == LDAP_ALREADY_EXISTS ) {
				Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
						"audit container exists, tenant (%s)\n",
						tenantp->tenant_info.tid.bv_val ?
								tenantp->tenant_info.tid.bv_val :
								"NULL" );
				rc = LDAP_SUCCESS;
			} else {
				Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
						"failed to initialize (%s): rc (%d)\n",
						tenantp->tenant_info.tid.bv_val ?
								tenantp->tenant_info.tid.bv_val :
								"NULL",
						rc );
				goto done;
			}
		} else {
			Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
					"created audit container for tenant (%s):\n",
					tenantp->tenant_info.tid.bv_val ?
							tenantp->tenant_info.tid.bv_val :
							"NULL" );
		}
		rc = initialize_tenant(
				be, cr, &tenantp->tenant_info, INIT_SESSION_CONTAINER );
		if ( rc != LDAP_SUCCESS ) {
			if ( rc == LDAP_ALREADY_EXISTS ) {
				Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
						"session container exists, tenant (%s)\n",
						tenantp->tenant_info.tid.bv_val ?
								tenantp->tenant_info.tid.bv_val :
								"NULL" );
				rc = LDAP_SUCCESS;
			} else {
				Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
						"failed to initialize (%s): rc (%d)\n",
						tenantp->tenant_info.tid.bv_val ?
								tenantp->tenant_info.tid.bv_val :
								"NULL",
						rc );
				goto done;
			}
		} else {
			Debug( LDAP_DEBUG_ANY, "rbac_initialize: "
					"created session container for tenant (%s):\n",
					tenantp->tenant_info.tid.bv_val ?
							tenantp->tenant_info.tid.bv_val :
							"NULL" );
		}
	}

done:;

	return rc;
}

static int
initialize_rbac_session_permissions()
{
	int i, rc = LDAP_SUCCESS;
	const char *text;

	for ( i = 0; !BER_BVISNULL( &rbac_session_permission_ads[i].attr ); i++ ) {
		rc = slap_bv2ad( &rbac_session_permission_ads[i].attr,
				rbac_session_permission_ads[i].ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
	}

done:;
	return rc;
}

int
rbac_initialize_repository()
{
	int rc = LDAP_SUCCESS;

	rc = initialize_jts();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = initialize_rbac_session_permissions();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = initialize_sessions();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = initialize_audit();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	return rc;
}
