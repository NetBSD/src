/*	$NetBSD: rbacsess.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbacsess.c - RBAC session */
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
__RCSID("$NetBSD: rbacsess.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

static slap_callback nullsc = { NULL, NULL, NULL, NULL };

extern rbac_ad_t rbac_session_permission_ads[];
extern rbac_ad_t rbac_session_ads[];

struct berval slapo_session_oc = BER_BVC("rbacSession");

typedef struct session_perm_req {
	Operation *op;
	SlapReply *rs;
	struct berval *sessid;
	struct berval permdn;
	tenant_info_t *tenantp;
} session_perm_req_t;

static int
rbac_sess_fake_cb( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_ANY, "rbac_sess_fake_cb\n" );

	return 0;
}

static int
rbac_send_session_permission(
		session_perm_req_t *sess_perm_reqp,
		rbac_permission_t *perm )
{
	int i, rc = LDAP_SUCCESS;
	Operation *op = sess_perm_reqp->op;
	SlapReply *rs = sess_perm_reqp->rs;
	struct berval *sessidp = sess_perm_reqp->sessid;
	struct berval *permdnp = &sess_perm_reqp->permdn;

	Entry *e = entry_alloc();
	e->e_attrs = NULL;
	ber_dupbv( &e->e_name, permdnp );
	ber_dupbv( &e->e_nname, permdnp );
	e->e_private = NULL;
	attr_merge_one( e, slap_rbac_schema.ad_session_id, sessidp, NULL );

	for ( i = 0; !BER_BVISNULL( &rbac_session_permission_ads[i].attr ); i++ ) {
		switch ( rbac_session_permission_ads[i].type ) {
			case RBAC_OP_NAME:
				attr_merge_one( e, *rbac_session_permission_ads[i].ad,
						&perm->opName[0], NULL );
				break;
			case RBAC_OBJ_NAME:
				attr_merge_one( e, *rbac_session_permission_ads[i].ad,
						&perm->objName[0], NULL );
				break;
			case RBAC_ROLE_NAME:
				attr_merge( e, *rbac_session_permission_ads[i].ad, perm->roles,
						NULL );
				break;
			default:
				break;
		}
	}

	rs->sr_entry = e;
	rs->sr_flags = REP_ENTRY_MUSTRELEASE;
	rc = send_search_entry( op, rs );

	return rc;
}

static int
rbac_session_permissions_cb( Operation *op, SlapReply *rs )
{
	session_perm_req_t *sess_perm_reqp = op->o_callback->sc_private;
	tenant_info_t *tenantp = NULL;
	rbac_permission_t *permp = NULL;
	rbac_ad_t *session_permissions_ads;
	int i;

	if ( rs->sr_type != REP_SEARCH ) return 0;

	assert( sess_perm_reqp );

	tenantp = sess_perm_reqp->tenantp;
	session_permissions_ads = tenantp->schema->session_permissions_ads;

	permp = ch_calloc( 1, sizeof(rbac_permission_t) );

	for ( i = 0; !BER_BVISNULL( &session_permissions_ads[i].attr ); i++ ) {
		Attribute *attr = NULL;

		attr = attr_find(
				rs->sr_entry->e_attrs, *session_permissions_ads[i].ad );
		if ( attr != NULL ) {
			switch ( session_permissions_ads[i].type ) {
				case RBAC_USERS:
					ber_bvarray_dup_x( &permp->uids, attr->a_nvals, NULL );
					break;
				case RBAC_ROLES:
					ber_bvarray_dup_x( &permp->roles, attr->a_nvals, NULL );
					break;
				case RBAC_OBJ_NAME:
					ber_bvarray_dup_x( &permp->objName, attr->a_nvals, NULL );
					break;
				case RBAC_OP_NAME:
					ber_bvarray_dup_x( &permp->opName, attr->a_nvals, NULL );
					break;
			}
		}
	}

	rbac_send_session_permission( sess_perm_reqp, permp );
	rbac_free_permission( permp );
	permp = NULL;

	return SLAP_CB_CONTINUE;
}

static int
rbac_read_session_cb( Operation *op, SlapReply *rs )
{
	rbac_session_t *sessp = op->o_callback->sc_private;
	int i;

	if ( rs->sr_type != REP_SEARCH ) return 0;

	ber_dupbv( &sessp->sessdn, &rs->sr_entry->e_name );

	for ( i = 0; !BER_BVISNULL( &rbac_session_ads[i].attr ); i++ ) {
		Attribute *attr = NULL;
		attr = attr_find( rs->sr_entry->e_attrs, *rbac_session_ads[i].ad );
		if ( attr != NULL ) {
			switch ( rbac_session_ads[i].type ) {
				case RBAC_SESSION_ID:
					ber_dupbv( &sessp->sessid, &attr->a_vals[0] );
					break;
				case RBAC_USER_DN:
					ber_dupbv( &sessp->userdn, &attr->a_vals[0] );
					break;
				case RBAC_ROLES:
					ber_bvarray_dup_x( &sessp->roles, attr->a_nvals, NULL );
					break;
				case RBAC_ROLE_CONSTRAINTS:
					ber_bvarray_dup_x(
							&sessp->role_constraints, attr->a_nvals, NULL );
					break;
				case RBAC_UID:
					ber_dupbv( &sessp->uid, &attr->a_vals[0] );
					break;
				case RBAC_TENANT_ID:
					ber_dupbv( &sessp->tenantid, &attr->a_vals[0] );
					break;
				default:
					break;
			}
		}
	}

	//return SLAP_CB_CONTINUE;
	return 0;
}

/* check whether the session is owned by the user */
int
rbac_is_session_owner( rbac_session_t *sessp, rbac_req_t *reqp )
{
	int rc = 0;

	if ( BER_BVISEMPTY( &sessp->uid ) || BER_BVISEMPTY( &reqp->uid ) ) {
		Debug( LDAP_DEBUG_ANY, "session not owned by user\n" );
		rc = 0;
		goto done;
	}

	if ( !ber_bvstrcasecmp( &sessp->uid, &reqp->uid ) ) {
		rc = 1;
		goto done;
	}

done:;
	return rc;
}

int
rbac_session_add_role( Operation *op, rbac_session_t *sessp, rbac_req_t *reqp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	slap_callback cb = { 0 };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	rbac_callback_info_t rbac_cb;
	tenant_info_t *tenantp = NULL;
	struct berval vals[2];
	Modifications mod;
	int rc = LDAP_SUCCESS;

	tenantp = rbac_tid2tenant( &reqp->tenantid );
	if ( !tenantp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_add_role: "
				"no tenant info with the req\n" );
		goto done;
	}

	// convert the role name to lower case:
	rbac_to_lower( &reqp->role );

	//ber_dupbv( &vals[0], &reqp->roles[0]);
	ber_dupbv( &vals[0], &reqp->role );
	BER_BVZERO( &vals[1] );

	/* create mod list */
	mod.sml_op = LDAP_MOD_ADD;
	mod.sml_flags = 0;
	mod.sml_type = slap_rbac_schema.ad_session_roles->ad_cname;
	mod.sml_desc = slap_rbac_schema.ad_session_roles;
	mod.sml_numvals = 1;
	mod.sml_values = vals;
	mod.sml_nvalues = NULL;
	mod.sml_next = NULL;

	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_sess_fake_cb;
	op2.o_callback = &cb;

	op2.o_tag = LDAP_REQ_MODIFY;
	op2.orm_modlist = &mod;
	op2.o_req_dn = sessp->sessdn;
	op2.o_req_ndn = sessp->sessdn;
	op2.o_bd = select_backend( &op2.o_req_ndn, 0 );
	op2.o_dn = op2.o_bd->be_rootdn;
	op2.o_ndn = op2.o_bd->be_rootdn;
	op2.ors_limit = NULL;
	rc = op2.o_bd->be_modify( &op2, &rs2 );
	ch_free( vals[0].bv_val );

done:;
	if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_add_active_role: "
				"role already activated in session\n" );
	}
	return rc;
}

int
rbac_session_drop_role( Operation *op, rbac_session_t *sessp, rbac_req_t *reqp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	slap_callback cb = { 0 };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	rbac_callback_info_t rbac_cb;
	tenant_info_t *tenantp = NULL;
	Modifications *m = NULL;
	int rc = LDAP_SUCCESS;

	tenantp = rbac_tid2tenant( &reqp->tenantid );
	if ( !tenantp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_drop_role: "
				"no tenant info with the req\n" );
		goto done;
	}

	/* create mod list */
	m = ch_calloc( sizeof(Modifications), 1 );
	m->sml_op = LDAP_MOD_DELETE;
	m->sml_flags = 0;
	m->sml_type = slap_rbac_schema.ad_session_roles->ad_cname;
	m->sml_desc = slap_rbac_schema.ad_session_roles;
	m->sml_numvals = 1;
	m->sml_values = ch_calloc( sizeof(struct berval), 2 );
	m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );
	//ber_dupbv( &m->sml_values[0], &reqp->roles[0]);

	// convert the role name to lower case:
	rbac_to_lower( &reqp->role );

	ber_dupbv( &m->sml_values[0], &reqp->role );

	// todo: determine if this needs to be done:
	//BER_BVZERO(&m->sml_values[1]);

	ber_dupbv( &m->sml_nvalues[0], &reqp->role );
	BER_BVZERO( &m->sml_nvalues[1] );

	//ber_dupbv( &m->sml_nvalues[0], &reqp->roles[0]);
	//ber_dupbv( &m->sml_nvalues[0], &reqp->role);
	//BER_BVZERO(&m->sml_nvalues[1]);

	m->sml_next = NULL;

	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_sess_fake_cb;
	op2.o_callback = &cb;

	op2.o_dn = tenantp->session_admin;
	op2.o_ndn = tenantp->session_admin;
	op2.o_tag = LDAP_REQ_MODIFY;
	op2.orm_modlist = m;
	op2.o_req_dn = sessp->sessdn;
	op2.o_req_ndn = sessp->sessdn;
	op2.o_bd = select_backend( &op2.o_req_ndn, 0 );

	op2.ors_limit = NULL;
	rc = op2.o_bd->be_modify( &op2, &rs2 );

done:;
	if ( m ) {
		slap_mods_free( m, 1 );
	}

	return rc;
}

/* delete the session */
int
rbac_int_delete_session( Operation *op, rbac_session_t *sessp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	slap_callback cb = { 0 };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	rbac_callback_info_t rbac_cb;
	tenant_info_t *tenantp = NULL;
	int rc = LDAP_SUCCESS;

	tenantp = rbac_tid2tenant( &sessp->tenantid );
	if ( !tenantp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_drop_role: "
				"no tenant info with the req\n" );
		goto done;
	}

	/* delete RBAC session */
	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_sess_fake_cb;
	op2.o_callback = &cb;

	op2.o_dn = tenantp->session_admin;
	op2.o_ndn = tenantp->session_admin;
	op2.o_tag = LDAP_REQ_DELETE;
	op2.o_req_dn = sessp->sessdn;
	op2.o_req_ndn = sessp->sessdn;
	op2.o_bd = select_backend( &op2.o_req_ndn, 0 );
	rc = op2.o_bd->be_delete( &op2, &rs2 );

done:;
	return rc;
}

rbac_session_t *
rbac_alloc_session()
{
	rbac_session_t *sessp = NULL;

	sessp = ch_malloc( sizeof(rbac_session_t) );
	sessp->sessid.bv_len =
			lutil_uuidstr( sessp->uuidbuf, sizeof(sessp->uuidbuf) );
	sessp->sessid.bv_val = sessp->uuidbuf;

	sessp->user = NULL;
	BER_BVZERO( &sessp->tenantid );
	BER_BVZERO( &sessp->uid );
	BER_BVZERO( &sessp->userdn );
	BER_BVZERO( &sessp->sessdn );
	BER_BVZERO( &sessp->message );

	sessp->last_access = 0;
	sessp->timeout = 0;
	sessp->warning_id = 0;
	sessp->error_id = 0;
	sessp->grace_logins = 0;
	sessp->expiration_secs = 0;
	sessp->is_authenticated = 0;

	sessp->roles = NULL;
	sessp->role_constraints = NULL;

	return sessp;
}

int
rbac_register_session( Operation *op, SlapReply *rs, rbac_session_t *sessp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	struct berval rdn, nrdn;
	SlapReply rs2 = { REP_RESULT };
	OperationBuffer opbuf;
	Operation *op2;
	Connection conn = { 0 };
	Entry *e = NULL;
	int rc = LDAP_SUCCESS;
	char rdnbuf[
		STRLENOF(RBAC_SESSION_RDN_EQ) + LDAP_LUTIL_UUIDSTR_BUFSIZE + 1];
	tenant_info_t *tenantp = rbac_tid2tenant( &sessp->tenantid );
#ifdef USE_NEW_THREAD_CONTEXT
	void *thrctx = ldap_pvt_thread_pool_context();
#else
	void *thrctx = op->o_tmpmemctx;
#endif

	if ( !sessp ) {
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* dynamic objects */
	e = entry_alloc();

	strcpy( rdnbuf, RBAC_SESSION_RDN_EQ );
	strncat( rdnbuf, sessp->sessid.bv_val, sessp->sessid.bv_len );
	rdn.bv_val = rdnbuf;
	rdn.bv_len = STRLENOF(RBAC_SESSION_RDN_EQ) + sessp->sessid.bv_len;
	nrdn.bv_val = rdnbuf;
	nrdn.bv_len = STRLENOF(RBAC_SESSION_RDN_EQ) + sessp->sessid.bv_len;

	build_new_dn( &e->e_name, &tenantp->sessions_basedn, &rdn, NULL );
	build_new_dn( &e->e_nname, &tenantp->sessions_basedn, &nrdn, NULL );

	attr_merge_one( e, slap_schema.si_ad_objectClass, &slapo_session_oc, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
			&slapo_session_oc, NULL );
	attr_merge_one( e, slap_rbac_schema.ad_session_id, &sessp->sessid, NULL );

	if ( !BER_BVISNULL( &sessp->uid ) ) {
		attr_merge_one( e, slap_schema.si_ad_uid, &sessp->uid, NULL );
	}

	/* add tenant id */
	if ( !BER_BVISNULL( &sessp->tenantid ) ) {
		attr_merge_one(
				e, slap_rbac_schema.ad_tenant_id, &sessp->tenantid, NULL );
	}

	/* add the userdn */
	if ( !BER_BVISNULL( &sessp->userdn ) ) {
		attr_merge_one(
				e, slap_rbac_schema.ad_session_user_dn, &sessp->userdn, NULL );
	}

	if ( sessp->roles ) {
		attr_merge( e, slap_rbac_schema.ad_session_roles, sessp->roles, NULL );
	}

	// TODO: ensure this is correct way to store constraints in session:
	if ( sessp->role_constraints ) {
		attr_merge( e, slap_rbac_schema.ad_session_role_constraints,
				sessp->role_constraints, NULL );
	}
	/* rendered dynmaicObject */
	attr_merge_one( e, slap_schema.si_ad_objectClass,
			&slap_schema.si_oc_dynamicObject->soc_cname, NULL );

	/* store RBAC session */
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op2 = &opbuf.ob_op;
	//Operation op2 = *op;
	//op2.o_callback = &nullsc;
	//rbac_callback_info_t rbac_cb;
	//cb.sc_private      = &rbac_cb;
	//cb.sc_response     = rbac_sess_fake_cb;
	//op2.o_callback    = &cb;
	//op2.ors_limit     = NULL;
	op->o_callback = &nullsc;
	op2->o_dn = tenantp->session_admin;
	op2->o_ndn = tenantp->session_admin;
	op2->o_tag = LDAP_REQ_ADD;
	op2->o_protocol = LDAP_VERSION3;
	op2->o_req_dn = e->e_name;
	op2->o_req_ndn = e->e_nname;
	op2->ora_e = e;
	op2->o_bd = frontendDB;

	rc = op2->o_bd->be_add( op2, &rs2 );

done:;
	if ( e ) entry_free( e );
	return rc;
}

int
rbac_register_session2( Operation *op, SlapReply *rs, rbac_session_t *sessp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	struct berval rdn, nrdn;
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	rbac_callback_info_t rbac_cb;
	//OperationBuffer opbuf;
	//Connection conn = {0};
	Entry *e = NULL;
	int rc = LDAP_SUCCESS;
	char rdnbuf[STRLENOF(RBAC_SESSION_RDN_EQ) + LDAP_LUTIL_UUIDSTR_BUFSIZE +
			1];
	tenant_info_t *tenantp = rbac_tid2tenant( &sessp->tenantid );
	slap_callback cb = { 0 };
	//#ifdef USE_NEW_THREAD_CONTEXT
	//	void *thrctx = ldap_pvt_thread_pool_context();
	//#else
	//	void *thrctx = op->o_tmpmemctx;
	//#endif

	if ( !sessp ) {
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* dynamic objects */
	e = entry_alloc();

	strcpy( rdnbuf, RBAC_SESSION_RDN_EQ );
	strncat( rdnbuf, sessp->sessid.bv_val, sessp->sessid.bv_len );
	rdn.bv_val = rdnbuf;
	rdn.bv_len = STRLENOF(RBAC_SESSION_RDN_EQ) + sessp->sessid.bv_len;
	nrdn.bv_val = rdnbuf;
	nrdn.bv_len = STRLENOF(RBAC_SESSION_RDN_EQ) + sessp->sessid.bv_len;

	build_new_dn( &e->e_name, &tenantp->sessions_basedn, &rdn, NULL );
	build_new_dn( &e->e_nname, &tenantp->sessions_basedn, &nrdn, NULL );

	attr_merge_one( e, slap_schema.si_ad_objectClass, &slapo_session_oc, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
			&slapo_session_oc, NULL );
	attr_merge_one( e, slap_rbac_schema.ad_session_id, &sessp->sessid, NULL );

	if ( !BER_BVISNULL( &sessp->uid ) ) {
		attr_merge_one( e, slap_schema.si_ad_uid, &sessp->uid, NULL );
	}

	/* add tenant id */
	if ( !BER_BVISNULL( &sessp->tenantid ) ) {
		attr_merge_one(
				e, slap_rbac_schema.ad_tenant_id, &sessp->tenantid, NULL );
	}

	/* add the userdn */
	if ( !BER_BVISNULL( &sessp->userdn ) ) {
		attr_merge_one(
				e, slap_rbac_schema.ad_session_user_dn, &sessp->userdn, NULL );
	}

	if ( sessp->roles ) {
		attr_merge( e, slap_rbac_schema.ad_session_roles, sessp->roles, NULL );
	}

	// TODO: ensure this is correct way to store constraints in session:
	if ( sessp->role_constraints ) {
		attr_merge( e, slap_rbac_schema.ad_session_role_constraints,
				sessp->role_constraints, NULL );
	}
	/* rendered dynmaicObject */
	attr_merge_one( e, slap_schema.si_ad_objectClass,
			&slap_schema.si_oc_dynamicObject->soc_cname, NULL );

	/* store RBAC session */
	//connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	//op2 = &opbuf.ob_op;
	//op2.o_ctrlflag = op->o_ctrlflag;
	// todo this ain't right"
	//op2.o_ctrlflag = 0;
	//OperationBuffer *opbuf;
	//memset( opbuf, 0, sizeof(OperationBuffer));
	//op2.o_hdr = &opbuf->ob_hdr;
	//op2.o_controls = opbuf->ob_controls;

	// fails on modify.c:353 with segfault

	//op2.o_callback = &nullsc;
	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_sess_fake_cb;
	op2.o_callback = &cb;
	op2.o_dn = tenantp->session_admin;
	op2.o_ndn = tenantp->session_admin;
	op2.o_tag = LDAP_REQ_ADD;
	op2.o_protocol = LDAP_VERSION3;
	op2.o_req_dn = e->e_name;
	op2.o_req_ndn = e->e_nname;
	op2.ora_e = e;
	op2.o_bd = frontendDB;
	//op2.ors_limit     = NULL;

	rc = op2.o_bd->be_add( &op2, &rs2 );

done:;
	if ( e ) entry_free( e );

	return rc;
}

int
rbac_is_valid_session_id( struct berval *sessid )
{
	/* TODO: simple test */
	if ( !sessid || sessid->bv_len != 36 ) {
		if ( !sessid ) {
			Debug( LDAP_DEBUG_ANY, "rbac_is_valid_session_id: "
					"null sessid\n" );
		} else {
			Debug( LDAP_DEBUG_ANY, "rbac_is_valid_session_id: "
					"len (%lu)\n",
					sessid->bv_len );
		}
		return 0;
	}

	else {
		return 1;
	}
}

/* create an rbac request with the session ID */
rbac_req_t *
rbac_is_search_session_permissions( Operation *op )
{
	rbac_req_t *reqp = NULL;

	/* check whether the search for sessionPermissions and *
	 * with a valid sessionID */

	return reqp;
}

rbac_session_t *
rbac_session_byid_fake( Operation *op, rbac_req_t *reqp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_session_t *sessp = NULL;
	int rc = LDAP_SUCCESS;
	char fbuf[RBAC_BUFLEN];
	struct berval filter = { sizeof(fbuf), fbuf };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	rbac_callback_info_t rbac_cb;
	slap_callback cb = { 0 };
	tenant_info_t *tenantp = NULL;

	if ( !rbac_is_valid_session_id( &reqp->sessid ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_byid: "
				"invalid session id (%s)\n",
				reqp->sessid.bv_val );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	sessp = rbac_alloc_session();
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_byid: "
				"unable to allocate session memory\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	tenantp = rbac_tid2tenant( &reqp->tenantid );

	/* session id filter */
	memset( fbuf, 0, sizeof(fbuf) );
	strcpy( fbuf, RBAC_SESSION_RDN_EQ );
	strncpy( &fbuf[0] + sizeof(RBAC_SESSION_RDN_EQ) - 1, reqp->sessid.bv_val,
			reqp->sessid.bv_len );
	filter.bv_val = fbuf;
	filter.bv_len = strlen( fbuf );

	//cb.sc_private     = sessp;
	//cb.sc_response    = rbac_read_session_cb;
	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_sess_fake_cb;
	op2.o_callback = &cb;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_dn = tenantp->session_admin;
	op2.o_ndn = tenantp->session_admin;
	op2.o_req_dn = tenantp->sessions_basedn;
	op2.o_req_ndn = tenantp->sessions_basedn;
	op2.ors_filterstr = filter;
	op2.ors_filter = str2filter_x( &op2, filter.bv_val );
	op2.ors_scope = LDAP_SCOPE_SUBTREE;
	op2.ors_attrs = slap_rbac_schema.session_attrs;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_slimit = SLAP_NO_LIMIT;
	op2.o_bd = frontendDB;
	// hyc change to fix seg fault:
	op2.ors_limit = NULL;

	rc = op2.o_bd->be_search( &op2, &rs2 );
	filter_free_x( &op2, op2.ors_filter, 1 );

done:
	// TODO: find equivalent way of check nentries (broke with fake connection fix)
	//if ( rc != LDAP_SUCCESS || rs2.sr_nentries <= 0 ) {
	if ( rc != LDAP_SUCCESS ) {
		rbac_free_session( sessp );
		sessp = NULL;
	}

	return sessp;
}

rbac_session_t *
rbac_session_byid( Operation *op, rbac_req_t *reqp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_session_t *sessp = NULL;
	int rc = LDAP_SUCCESS;
	char fbuf[RBAC_BUFLEN];
	struct berval filter = { sizeof(fbuf), fbuf };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	slap_callback cb = { 0 };
	tenant_info_t *tenantp = NULL;

	if ( !rbac_is_valid_session_id( &reqp->sessid ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_byid: "
				"invalid session id (%s)\n",
				reqp->sessid.bv_val );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	sessp = rbac_alloc_session();
	if ( !sessp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_session_byid: "
				"unable to allocate session memory\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	tenantp = rbac_tid2tenant( &reqp->tenantid );

	/* session id filter */
	memset( fbuf, 0, sizeof(fbuf) );
	strcpy( fbuf, RBAC_SESSION_RDN_EQ );
	strncpy( &fbuf[0] + sizeof(RBAC_SESSION_RDN_EQ) - 1, reqp->sessid.bv_val,
			reqp->sessid.bv_len );
	filter.bv_val = fbuf;
	filter.bv_len = strlen( fbuf );

	cb.sc_private = sessp;
	cb.sc_response = rbac_read_session_cb;
	op2.o_callback = &cb;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_dn = tenantp->session_admin;
	op2.o_ndn = tenantp->session_admin;
	op2.o_req_dn = tenantp->sessions_basedn;
	op2.o_req_ndn = tenantp->sessions_basedn;
	op2.ors_filterstr = filter;
	op2.ors_filter = str2filter_x( &op2, filter.bv_val );
	op2.ors_scope = LDAP_SCOPE_SUBTREE;
	op2.ors_attrs = slap_rbac_schema.session_attrs;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_slimit = SLAP_NO_LIMIT;
	op2.o_bd = frontendDB;
	// hyc change to fix seg fault:
	op2.ors_limit = NULL;

	rc = op2.o_bd->be_search( &op2, &rs2 );
	filter_free_x( &op2, op2.ors_filter, 1 );

done:
	// TODO: find equivalent way of check nentries (broke with fake connection fix)
	//if ( rc != LDAP_SUCCESS || rs2.sr_nentries <= 0 ) {
	if ( rc != LDAP_SUCCESS ) {
		rbac_free_session( sessp );
		sessp = NULL;
	}

	return sessp;
}

static char *
rbac_int_session_permissions_filterstr( Operation *op, rbac_session_t *sessp )
{
	char filterbuf[RBAC_BUFLEN];
	int i;

	memset( filterbuf, 0, sizeof(filterbuf) );

	strcat( filterbuf, "(&(objectClass=ftOperation)(|" );
	strcat( filterbuf, "(ftUsers=" );
	strcat( filterbuf, sessp->uid.bv_val );
	strcat( filterbuf, ")" );

	/* add ftRoles filters */
	for ( i = 0; !BER_BVISEMPTY( &sessp->roles[i] ); i++ ) {
		strcat( filterbuf, "(ftRoles=" );
		strncat( filterbuf, sessp->roles[i].bv_val, sessp->roles[i].bv_len );
		strcat( filterbuf, ")" );
	}
	strcat( filterbuf, "))" );
	return strdup( filterbuf );
}

int
rbac_int_session_permissions(
		Operation *op,
		SlapReply *rs,
		rbac_req_t *reqp,
		rbac_session_t *sessp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	tenant_info_t *tenantp = NULL;
	int rc;
	struct berval filter;
	char *filterstr;
	struct berval permndn = BER_BVNULL;
	OperationBuffer opbuf;
	Connection conn = { 0 };
	SlapReply rs2 = { REP_RESULT };
	Operation *op2;
	slap_callback cb = { 0 };
	char permbuf[1024];
	session_perm_req_t sess_perm_req;
#ifdef USE_NEW_THREAD_CONTEXT
	void *thrctx = ldap_pvt_thread_pool_context();
#else
	void *thrctx = op->o_tmpmemctx;
#endif

	tenantp = rbac_tid2tenant( &reqp->tenantid );

	/* construct session permissions dn */
	memset( permbuf, 0, sizeof(permbuf) );
	strcat( permbuf, "rbacSessid=" );
	strncat( permbuf, sessp->sessid.bv_val, sessp->sessid.bv_len );
	strcat( permbuf, ",dc=rbac" );
	sess_perm_req.op = op;
	sess_perm_req.rs = rs;
	sess_perm_req.permdn.bv_val = permbuf;
	sess_perm_req.permdn.bv_len = strlen( permbuf );
	sess_perm_req.sessid = &reqp->sessid;
	sess_perm_req.tenantp = tenantp;

	filterstr = rbac_int_session_permissions_filterstr( op, sessp );
	if ( !filterstr ) {
		Debug( LDAP_DEBUG_ANY, "unable to construct filter for session permissions\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}
	filter.bv_val = filterstr;
	filter.bv_len = strlen( filterstr );

	rc = dnNormalize(
			0, NULL, NULL, &tenantp->permissions_basedn, &permndn, NULL );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_permission: "
				"unable to normalize permission DN\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op2 = &opbuf.ob_op;
	//Operation op2 = *op;
	cb.sc_private = &sess_perm_req;
	cb.sc_response = rbac_session_permissions_cb;
	op2->o_callback = &cb;
	op2->o_tag = LDAP_REQ_SEARCH;
	op2->o_dn = tenantp->admin;
	op2->o_ndn = tenantp->admin;
	op2->o_req_dn = tenantp->permissions_basedn;
	op2->o_req_ndn = permndn;
	op2->ors_filterstr = filter;
	op2->ors_filter = str2filter_x( op, filter.bv_val );
	op2->ors_scope = LDAP_SCOPE_SUB;
	op2->ors_attrs = tenantp->schema->session_perm_attrs;
	op2->ors_tlimit = SLAP_NO_LIMIT;
	op2->ors_slimit = SLAP_NO_LIMIT;
	op2->ors_attrsonly = 0;
	op2->o_bd = frontendDB;
	//op2.ors_limit     = NULL;
	rc = op2->o_bd->be_search( op2, &rs2 );
	filter_free_x( op, op2->ors_filter, 1 );

done:;
	/* generate audit log */
	rbac_audit( op, SessionPermissions, sessp, reqp, rc, (char *)rs->sr_text );

	rs->sr_err = rc;
	return rs->sr_err;
}

void
rbac_free_session( rbac_session_t *sessp )
{
	if ( !sessp ) return;

	if ( sessp->user ) {
		rbac_free_user( sessp->user );
	}

	if ( !BER_BVISNULL( &sessp->uid ) ) {
		ber_memfree( sessp->uid.bv_val );
	}

	if ( !BER_BVISNULL( &sessp->tenantid ) ) {
		ber_memfree( sessp->tenantid.bv_val );
	}

	if ( !BER_BVISNULL( &sessp->userdn ) ) {
		ber_memfree( sessp->userdn.bv_val );
	}

	if ( !BER_BVISNULL( &sessp->sessdn ) ) {
		ber_memfree( sessp->sessdn.bv_val );
	}

	if ( !BER_BVISNULL( &sessp->message ) ) {
		ber_memfree( sessp->message.bv_val );
	}

	if ( sessp->roles ) {
		ber_bvarray_free( sessp->roles );
	}

	if ( sessp->role_constraints ) {
		ber_bvarray_free( sessp->role_constraints );
	}

	ch_free( sessp );

	return;
}

/* roles included from request are activated into a session only when
 * they exist and have been assigned to the user. If no roles included in request, all
 * roles assigned to the user are activated into the rbac session.
 */
int
activate_session_roles(
		rbac_session_t *sessp,
		rbac_req_t *reqp,
		rbac_user_t *userp )
{
	int i, j, rc = LDAP_UNWILLING_TO_PERFORM;
	if ( !sessp || !reqp || !userp ) {
		goto done;
	}

	/* no role requested, assign all roles from the user to the session. */
	if ( reqp->roles == NULL || BER_BVISNULL( &reqp->roles[0] ) ) {
		//if (!reqp->roles || BER_BVISNULL(&reqp->roles[0])) {
		/* no roles assigned to the user */
		if ( !userp->roles || BER_BVISNULL( &userp->roles[0] ) ) goto done;
		for ( i = 0; !BER_BVISNULL( &userp->roles[i] ); i++ ) {
			struct berval role;
			ber_dupbv_x( &role, &userp->roles[i], NULL );
			ber_bvarray_add( &sessp->roles, &role );
			rc = LDAP_SUCCESS;
		}

		// TODO: smm 20141218 - make sure this is correct way to add constraints to user session.
		for ( i = 0; !BER_BVISNULL( &userp->role_constraints[i] ); i++ ) {
			struct berval roleconstraint;
			ber_dupbv_x( &roleconstraint, &userp->role_constraints[i], NULL );
			ber_bvarray_add( &sessp->role_constraints, &roleconstraint );
			rc = LDAP_SUCCESS;
		}

	} else {
		for ( i = 0; !BER_BVISNULL( &reqp->roles[i] ); i++ ) {
			for ( j = 0; !BER_BVISNULL( &userp->roles[j] ); j++ ) {
				if ( !ber_bvstrcasecmp( &reqp->roles[i], &userp->roles[j] ) ) {
					/* requested role is assigned to the user */
					struct berval role;
					ber_dupbv_x( &role, &userp->roles[i], NULL );
					ber_bvarray_add( &sessp->roles, &role );
					rc = LDAP_SUCCESS;
				}
			}
		}
	}

done:;
	return rc;
}
