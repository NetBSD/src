/*	$NetBSD: rbacaudit.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbacaudit.c - RBAC Audit */
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
__RCSID("$NetBSD: rbacaudit.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

static struct rbac_audit_op {
	audit_op_t op;
	struct berval op_bv;
} rbac_audit_ops[] = {
	{ CreateSession, BER_BVC("CreateSession") },
	{ CheckAccess, BER_BVC("CheckAccess") },
	{ AddActiveRole, BER_BVC("AddActiveRole") },
	{ DropActiveRole, BER_BVC("DropActiveRole") },
	{ SessionPermissions, BER_BVC("SessionPermissions") },
	{ DeleteSession, BER_BVC("DeleteSession") },
	{ SessionRoles, BER_BVC("SessionRoles") },

	{ -1, BER_BVNULL }
};

static int
rbac_audit_fake_cb( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_ANY, "rbac_audit_fake_cb\n" );

	return 0;
}

void
rbac_audit(
		Operation *op,
		audit_op_t rbac_op,
		rbac_session_t *sessp,
		rbac_req_t *reqp,
		int result,
		char *msg )
{
	int op_idx, rc = LDAP_SUCCESS;
	int found = 0;
	struct berval timestamp;
	tenant_info_t *tenantp = rbac_tid2tenant( &sessp->tenantid );
	slap_callback cb = { 0 };
	SlapReply rs2 = { REP_RESULT };
	Entry *e = NULL;
	struct berval auditObjectClass = BER_BVC("rbacAudit");
	struct berval auditResultSuccess = BER_BVC("success");
	struct berval auditResultFailed = BER_BVC("failed");
	struct berval bv, rdn, nrdn;
	char rdnbuf[RBAC_BUFLEN];
	time_t now;
	char nowstr[LDAP_LUTIL_GENTIME_BUFSIZE];

	for ( op_idx = 0; rbac_audit_ops[op_idx].op != -1; op_idx++ ) {
		if ( rbac_op == rbac_audit_ops[op_idx].op ) {
			/* legit audit op */
			found = 1;
			break;
		}
	}

	if ( !found ) goto done;

	e = entry_alloc();

	/* audit timestamp */
	now = slap_get_time(); /* stored for later consideration */
	timestamp.bv_val = nowstr;
	timestamp.bv_len = sizeof(nowstr);
	slap_timestamp( &now, &timestamp );

	/* construct audit record DN; FIXME: random() call  */
	sprintf( rdnbuf, "%s%d", RBAC_AUDIT_RDN_EQ, (int)op->o_tid );
	strcat( rdnbuf, "-" );
	strncat( rdnbuf, timestamp.bv_val, timestamp.bv_len );
	bv.bv_val = &rdnbuf[0];
	bv.bv_len = strlen( &rdnbuf[0] );

	rc = dnPrettyNormal( NULL, &bv, &rdn, &nrdn, NULL );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_audit: "
				"unable to normalize audit rDN (rc=%d)\n", rc );
		goto done;
	}

	/* FIXME: audit_basedn should have been normalized */
	build_new_dn( &e->e_name, &tenantp->audit_basedn, &rdn, NULL );
	build_new_dn( &e->e_nname, &tenantp->audit_basedn, &nrdn, NULL );

	ch_free( rdn.bv_val );
	ch_free( nrdn.bv_val );

	/* add timestamp */
	attr_merge_one( e, slap_rbac_schema.ad_audit_timestamp, &timestamp, NULL );

	/* add rbac audit objectClass */

	attr_merge_one( e, slap_schema.si_ad_objectClass, &auditObjectClass, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
			&auditObjectClass, NULL );

	/* audit op */
	attr_merge_one( e, slap_rbac_schema.ad_audit_op,
			&rbac_audit_ops[op_idx].op_bv, NULL );

	/* userid */
	if ( sessp && !BER_BVISNULL( &sessp->uid ) ) {
		attr_merge_one( e, slap_schema.si_ad_uid, &sessp->uid, NULL );
	}

	/* session id */

	if ( sessp && !BER_BVISNULL( &sessp->sessid ) ) {
		AttributeDescription *ad = NULL;
		const char *text = NULL;
		struct berval sessid = BER_BVC("rbacSessid");

		rc = slap_bv2ad( &sessid, &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
		attr_merge_one( e, ad, &sessp->sessid, NULL );
	}

	/* audit result */
	attr_merge_one( e, slap_rbac_schema.ad_audit_result,
			result == LDAP_SUCCESS ? &auditResultSuccess : &auditResultFailed,
			NULL );

	switch ( rbac_op ) {
		case CreateSession:
			/* audit roles */
			if ( sessp && sessp->roles ) {
				attr_merge( e, slap_rbac_schema.ad_audit_roles, sessp->roles,
						NULL );
			}
			if ( reqp && reqp->roles ) {
				attr_merge( e, slap_rbac_schema.ad_audit_requested_roles,
						reqp->roles, NULL );
			}
			break;

		case CheckAccess:
			if ( sessp && sessp->roles ) {
				attr_merge( e, slap_rbac_schema.ad_audit_roles, sessp->roles,
						NULL );
			}
			if ( reqp && !BER_BVISEMPTY( &reqp->opname ) ) {
				attr_merge_one( e, slap_rbac_schema.ad_audit_operations,
						&reqp->opname, NULL );
			}
			if ( reqp && !BER_BVISEMPTY( &reqp->objname ) ) {
				attr_merge_one( e, slap_rbac_schema.ad_audit_objects,
						&reqp->objname, NULL );
			}
			break;

		case AddActiveRole:
			if ( reqp && reqp->roles ) {
				attr_merge( e, slap_rbac_schema.ad_audit_requested_roles,
						reqp->roles, NULL );
			}
			break;

		case DropActiveRole:
			/* audit roles */
			if ( reqp && reqp->roles ) {
				attr_merge( e, slap_rbac_schema.ad_audit_requested_roles,
						reqp->roles, NULL );
			}
			break;

		case SessionPermissions:
			if ( sessp && sessp->roles ) {
				attr_merge( e, slap_rbac_schema.ad_audit_roles, sessp->roles,
						NULL );
			}
			break;

		case DeleteSession:
		case SessionRoles:
		default:
			break;
	}

	/* record the audit record */
	Operation op2 = *op;
	rbac_callback_info_t rbac_cb;
	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_audit_fake_cb;
	op2.o_callback = &cb;

	op2.o_tag = LDAP_REQ_ADD;
	op2.o_protocol = LDAP_VERSION3;
	op2.o_req_dn = e->e_name;
	op2.o_req_ndn = e->e_nname;
	op2.ora_e = e;
	op2.o_bd = select_backend( &op2.o_req_ndn, 0 );
	op2.o_dn = op2.o_bd->be_rootdn;
	op2.o_ndn = op2.o_bd->be_rootndn;

	op2.ors_limit = NULL;
	rc = op2.o_bd->be_add( &op2, &rs2 );

done:
	if ( e ) entry_free( e );

	return;
}
