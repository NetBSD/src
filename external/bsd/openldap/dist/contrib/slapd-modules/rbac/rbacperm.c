/*	$NetBSD: rbacperm.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbacperm.c - RBAC permission */
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
__RCSID("$NetBSD: rbacperm.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

static int
rbac_read_permission_cb( Operation *op, SlapReply *rs )
{
	rbac_callback_info_t *cbp = op->o_callback->sc_private;
	rbac_ad_t *permission_ads;
	rbac_permission_t *permp;
	int i;

	if ( rs->sr_type != REP_SEARCH ) return 0;

	assert( cbp );

	permp = ch_calloc( 1, sizeof(rbac_permission_t) );
	permission_ads = cbp->tenantp->schema->permission_ads;

	ber_dupbv( &permp->dn, &rs->sr_entry->e_name );
	for ( i = 0; !BER_BVISNULL( &permission_ads[i].attr ); i++ ) {
		Attribute *attr = NULL;
		attr = attr_find( rs->sr_entry->e_attrs, *permission_ads[i].ad );
		if ( attr != NULL ) {
			switch ( permission_ads[i].type ) {
				case RBAC_USERS:
					ber_bvarray_dup_x( &permp->uids, attr->a_nvals, NULL );
					break;
				case RBAC_ROLES:
					ber_bvarray_dup_x( &permp->roles, attr->a_nvals, NULL );
					break;
				default:
					break;
			}
		}
	}

	cbp->private = (void *)permp;

	return 0;
}

/*
 * check whether roles assigned to a user allows access to roles in
 * a permission, subject to role constraints
 */
int
rbac_check_session_permission(
		rbac_session_t *sessp,
		rbac_permission_t *permp,
		rbac_constraint_t *role_constraints )
{
	int rc = LDAP_INSUFFICIENT_ACCESS;
	rbac_constraint_t *cp = NULL;
	int i, j;

	if ( !sessp->roles || !permp->roles ) goto done;

	for ( i = 0; !BER_BVISNULL( &sessp->roles[i] ); i++ ) {
		for ( j = 0; !BER_BVISNULL( &permp->roles[j] ); j++ ) {
			if ( ber_bvstrcasecmp( &sessp->roles[i], &permp->roles[j] ) == 0 ) {
				/* role temporal constraint */
				cp = rbac_role2constraint( &permp->roles[j], role_constraints );
				if ( !cp || rbac_check_time_constraint( cp ) == LDAP_SUCCESS ) {
					rc = LDAP_SUCCESS;
					goto done;
				}
			}
		}
	}
done:;
	return rc;
}

rbac_permission_t *
rbac_read_permission( Operation *op, rbac_req_t *reqp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	rbac_callback_info_t rbac_cb;
	int rc = LDAP_SUCCESS;
	char fbuf[1024];
	struct berval filter = { sizeof(fbuf), fbuf };
	char permbuf[1024];
	struct berval permdn = { sizeof(permbuf), permbuf };
	struct berval permndn = BER_BVNULL;
	char pcls[] = "(objectClass=ftOperation)";
	SlapReply rs2 = { REP_RESULT };
	slap_callback cb = { 0 };
	tenant_info_t *tenantp = rbac_tid2tenant( &reqp->tenantid );

#if 0 /* check valid object name and op name */
	if ( !is_valid_opname( &reqp->opname ) ||
			!is_valid_objname( &reqp->objname ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_permission: "
				"invalid opname (%s) or objname (%s)\n",
				reqp->opname.bv_val, reqp->objname.bv_val );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}
#endif

	if ( !tenantp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_permission: "
				"missing tenant information\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	if ( reqp->objid.bv_val != NULL ) {
		permdn.bv_len = snprintf( permdn.bv_val, permdn.bv_len,
				"ftObjId=%s+ftOpNm=%s,ftObjNm=%s,%s", reqp->objid.bv_val,
				reqp->opname.bv_val, reqp->objname.bv_val,
				tenantp->permissions_basedn.bv_val );
	} else {
		permdn.bv_len = snprintf( permdn.bv_val, permdn.bv_len,
				"ftOpNm=%s,ftObjNm=%s,%s", reqp->opname.bv_val,
				reqp->objname.bv_val, tenantp->permissions_basedn.bv_val );
	}

	rc = dnNormalize( 0, NULL, NULL, &permdn, &permndn, NULL );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_permission: "
				"unable to normalize permission DN\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	filter.bv_val = pcls;
	filter.bv_len = strlen( pcls );
	rbac_cb.tenantp = tenantp;
	rbac_cb.private = NULL;

	Operation op2 = *op;
	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_read_permission_cb;
	op2.o_callback = &cb;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_dn = tenantp->admin;
	op2.o_ndn = tenantp->admin;
	op2.o_req_dn = permdn;
	op2.o_req_ndn = permndn;
	op2.ors_filterstr = filter;
	op2.ors_filter = str2filter_x( &op2, filter.bv_val );
	op2.ors_scope = LDAP_SCOPE_BASE;
	op2.ors_attrs = tenantp->schema->perm_attrs;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_slimit = SLAP_NO_LIMIT;
	op2.ors_attrsonly = 0;
	op2.ors_limit = NULL;
	op2.o_bd = frontendDB;
	rc = op2.o_bd->be_search( &op2, &rs2 );
	filter_free_x( &op2, op2.ors_filter, 1 );

done:;
	ch_free( permndn.bv_val );

	if ( rc != LDAP_SUCCESS ) {
		rbac_free_permission((rbac_permission_t *)rbac_cb.private);
	}

	return (rbac_permission_t *)rbac_cb.private;
}

void
rbac_free_permission( rbac_permission_t *permp )
{
	if ( !permp ) return;

	if ( !BER_BVISNULL( &permp->dn ) ) {
		ber_memfree( permp->dn.bv_val );
	}

	if ( !BER_BVISNULL( &permp->internalId ) ) {
		ber_memfree( permp->internalId.bv_val );
	}

	if ( permp->opName ) {
		ber_bvarray_free( permp->opName );
	}

	if ( permp->objName ) {
		ber_bvarray_free( permp->objName );
	}

	if ( !BER_BVISNULL( &permp->objectId ) ) {
		ber_memfree( permp->objectId.bv_val );
	}

	if ( !BER_BVISNULL( &permp->abstractName ) ) {
		ber_memfree( permp->abstractName.bv_val );
	}

	if ( !BER_BVISNULL( &permp->type ) ) {
		ber_memfree( permp->type.bv_val );
	}

	if ( permp->roles ) {
		ber_bvarray_free( permp->roles );
	}

	if ( permp->uids ) {
		ber_bvarray_free( permp->uids );
	}
	ch_free( permp );

	return;
}
