/*	$NetBSD: jts.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* jts.c - RBAC JTS initialization */
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
__RCSID("$NetBSD: jts.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

struct slap_rbac_tenant_schema slap_rbac_jts_schema;

/* to replace all JTS schema initialization */
rbac_ad_t ft_ads[] = {
	{ RBAC_ROLE_ASSIGNMENT,
		BER_BVC("ftRA"), &slap_rbac_jts_schema.ad_role },
	{ RBAC_ROLE_CONSTRAINTS,
		BER_BVC("ftRC"), &slap_rbac_jts_schema.ad_role_constraint },
	{ RBAC_USER_CONSTRAINTS,
		BER_BVC("ftCstr"), &slap_rbac_jts_schema.ad_user_constraint },
	{ RBAC_UID,
		BER_BVC("uid"), &slap_rbac_jts_schema.ad_uid },
	{ RBAC_USERS,
		BER_BVC("ftUsers"), &slap_rbac_jts_schema.ad_permission_users },
	{ RBAC_ROLES,
		BER_BVC("ftRoles"), &slap_rbac_jts_schema.ad_permission_roles },
	{ RBAC_OBJ_NAME,
		BER_BVC("ftObjNm"), &slap_rbac_jts_schema.ad_permission_objname },
	{ RBAC_OP_NAME,
		BER_BVC("ftOpNm"), &slap_rbac_jts_schema.ad_permission_opname },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

rbac_ad_t ft_user_ads[] = {
	{ RBAC_ROLE_ASSIGNMENT,
		BER_BVC("ftRA"), &slap_rbac_jts_schema.ad_role },
	{ RBAC_ROLE_CONSTRAINTS,
		BER_BVC("ftRC"), &slap_rbac_jts_schema.ad_role_constraint },
	{ RBAC_USER_CONSTRAINTS,
		BER_BVC("ftCstr"), &slap_rbac_jts_schema.ad_user_constraint },
	{ RBAC_UID,
		BER_BVC("uid"), &slap_rbac_jts_schema.ad_uid },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

rbac_ad_t ft_perm_ads[] = {
	{ RBAC_USERS,
		BER_BVC("ftUsers"), &slap_rbac_jts_schema.ad_permission_users },
	{ RBAC_ROLES,
		BER_BVC("ftRoles"), &slap_rbac_jts_schema.ad_permission_roles },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

rbac_ad_t ft_session_perm_ads[] = {
	{ RBAC_USERS,
		BER_BVC("ftUsers"), &slap_rbac_jts_schema.ad_permission_users },
	{ RBAC_ROLES,
		BER_BVC("ftRoles"), &slap_rbac_jts_schema.ad_permission_roles },
	{ RBAC_OBJ_NAME,
		BER_BVC("ftObjNm"), &slap_rbac_jts_schema.ad_permission_objname },
	{ RBAC_OP_NAME,
		BER_BVC("ftOpNm"), &slap_rbac_jts_schema.ad_permission_opname },

	{ RBAC_NONE, BER_BVNULL, NULL }
};

static int
initialize_jts_session_permission_ads()
{
	int i, nattrs, rc = LDAP_SUCCESS;

	for ( nattrs = 0; !BER_BVISNULL( &ft_session_perm_ads[nattrs].attr );
			nattrs++ )
		; /* count the number of attrs */

	slap_rbac_jts_schema.session_perm_attrs =
			slap_sl_calloc( sizeof(AttributeName), nattrs + 1, NULL );

	for ( i = 0; !BER_BVISNULL( &ft_session_perm_ads[i].attr ); i++ ) {
		slap_rbac_jts_schema.session_perm_attrs[i].an_name =
				ft_session_perm_ads[i].attr;
		slap_rbac_jts_schema.session_perm_attrs[i].an_desc =
				*ft_session_perm_ads[i].ad;
	}

	BER_BVZERO( &slap_rbac_jts_schema.session_perm_attrs[nattrs].an_name );

	slap_rbac_jts_schema.session_permissions_ads = ft_session_perm_ads;

	return rc;
}

static int
initialize_jts_permission_ads()
{
	int i, nattrs, rc = LDAP_SUCCESS;

	/* jts permissions configuration */

	for ( nattrs = 0; !BER_BVISNULL( &ft_perm_ads[nattrs].attr ); nattrs++ )
		; /* count the number of attrs */

	slap_rbac_jts_schema.perm_attrs =
			slap_sl_calloc( sizeof(AttributeName), nattrs + 1, NULL );

	for ( i = 0; !BER_BVISNULL( &ft_perm_ads[i].attr ); i++ ) {
		slap_rbac_jts_schema.perm_attrs[i].an_name = ft_perm_ads[i].attr;
		slap_rbac_jts_schema.perm_attrs[i].an_desc = *ft_perm_ads[i].ad;
	}

	BER_BVZERO( &slap_rbac_jts_schema.perm_attrs[nattrs].an_name );

	slap_rbac_jts_schema.permission_ads = ft_perm_ads;

	return rc;
}

static int
initialize_jts_user_ads()
{
	int i, nattrs, rc = LDAP_SUCCESS;

	/* jts user attribute descriptions */

	/* jts user attributes */
	for ( nattrs = 0; !BER_BVISNULL( &ft_user_ads[nattrs].attr ); nattrs++ )
		; /* count the number of attrs */

	slap_rbac_jts_schema.user_attrs =
			slap_sl_calloc( sizeof(AttributeName), nattrs + 1, NULL );

	for ( i = 0; !BER_BVISNULL( &ft_user_ads[i].attr ); i++ ) {
		slap_rbac_jts_schema.user_attrs[i].an_name = ft_user_ads[i].attr;
		slap_rbac_jts_schema.user_attrs[i].an_desc = *ft_user_ads[i].ad;
	}

	BER_BVZERO( &slap_rbac_jts_schema.user_attrs[nattrs].an_name );

	slap_rbac_jts_schema.user_ads = ft_user_ads;

	return rc;
}

int
initialize_jts()
{
	int i, rc;
	const char *text;

	/* jts attributes */
	for ( i = 0; !BER_BVISNULL( &ft_ads[i].attr ); i++ ) {
		rc = slap_bv2ad( &ft_ads[i].attr, ft_ads[i].ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
	}

	rc = initialize_jts_user_ads();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = initialize_jts_permission_ads();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = initialize_jts_session_permission_ads();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

done:;
	return rc;
}
