/*	$NetBSD: rbac.h,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbac.h -  */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2021 The OpenLDAP Foundation.
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
 *
 */

#ifndef RBAC_H
#define RBAC_H

LDAP_BEGIN_DECL

#include "ldap_rbac.h"

#define USE_NEW_THREAD_CONTEXT 1
#define RBAC_BUFLEN 1024

/* tenant initialization op */
#define INIT_AUDIT_CONTAINER 0x01
#define INIT_SESSION_CONTAINER 0x02

typedef struct rbac_ad {
	int type;
	struct berval attr;
	AttributeDescription **ad;
} rbac_ad_t;

/* RBAC AttributeDescriptions */
struct slap_rbac_internal_schema {
	/* slapd schema */
	AttributeDescription *ad_uid;

	/* RBAC tenant */
	AttributeDescription *ad_tenant_id;

	/* RBAC sessions */
	AttributeDescription *ad_session_id;
	AttributeDescription *ad_session_user_dn;
	AttributeDescription *ad_session_roles;
	AttributeDescription *ad_session_role_constraints;

	/* RBAC session permissions */
	AttributeDescription *ad_permission_opname;
	AttributeDescription *ad_permission_objname;
	AttributeDescription *ad_permission_rolename;

	/* RBAC audit */
	AttributeDescription *ad_audit_op; /* rbac op: create_session */
	AttributeDescription *ad_audit_id;
	AttributeDescription *ad_audit_roles;
	AttributeDescription *ad_audit_requested_roles;
	AttributeDescription *ad_audit_timestamp;
	AttributeDescription *ad_audit_resources;
	AttributeDescription *ad_audit_objects;
	AttributeDescription *ad_audit_operations; /* resource ops */
	AttributeDescription *ad_audit_result;
	AttributeDescription *ad_audit_properties;
	AttributeDescription *ad_audit_messages;

	/* RBAC session attributes */
	AttributeName *session_attrs;
};

extern struct slap_rbac_internal_schema slap_rbac_schema;

/* attributes in tenant repository */
struct slap_rbac_tenant_schema {
	/* user role assignments, role constraints, and user constraint */
	AttributeDescription *ad_role;
	AttributeDescription *ad_role_constraint;
	AttributeDescription *ad_user_constraint;
	AttributeDescription *ad_uid;

	/* session permission */
	AttributeDescription *ad_permission_users;
	AttributeDescription *ad_permission_roles;
	AttributeDescription *ad_permission_objname;
	AttributeDescription *ad_permission_opname;

	/* the list of attributes when doing searches in the jts repo */
	AttributeName *user_attrs;
	AttributeName *perm_attrs; /* attrs to retrieve for check access */
	AttributeName *session_perm_attrs; /* attrs for session permissions */

	/* the corresponding list of attribute description mapping */
	rbac_ad_t *user_ads;
	rbac_ad_t *permission_ads;
	rbac_ad_t *session_permissions_ads;
};

extern struct slap_rbac_tenant_schema slap_rbac_jts_schema;

/* types of RBAC requests */
typedef struct rbac_request {
	int req_type;
	struct berval sessid;
	struct berval tenantid;

	/* session creation */
	struct berval uid;
	struct berval authtok;
	BerVarray roles;
	struct berval role;

	/* check access */
	struct berval opname;
	struct berval objname;
	struct berval objid;
} rbac_req_t;

typedef struct rbac_constraint {
	struct berval name; /* user name or role name */
	int allowed_inactivity; /* secs */
	int begin_time; /* secs */
	int end_time; /* secs */
	lutil_timet begin_date;
	lutil_timet end_date;
	lutil_timet begin_lock_date;
	lutil_timet end_lock_date;
	int day_mask;
	struct rbac_constraint *next;
} rbac_constraint_t;

/* holds RBAC info */
typedef struct tenant_info {
	struct berval tid; /* tenant id */
	struct berval admin;
	struct berval pwd;
	struct berval users_basedn;
	struct berval roles_basedn;
	struct berval audit_basedn;
	struct berval permissions_basedn;
	struct berval sessions_basedn;
	struct berval session_admin;
	struct berval session_admin_pwd;
	struct slap_rbac_tenant_schema *schema;
} tenant_info_t;

typedef struct rbac_tenant {
	tenant_info_t tenant_info;
	struct rbac_tenant *next;
} rbac_tenant_t;

/* for RBAC callback */
typedef struct rbac_callback_info {
	tenant_info_t *tenantp;
	void *private;
} rbac_callback_info_t;

/* RBAC user */
typedef struct rbac_user {
	struct berval tenantid;
	struct berval uid;
	struct berval dn;
	struct berval constraints;
	struct berval password;
	struct berval msg;
	int authz; /* flag for bind (pwd policy) info */
	BerVarray roles;
	BerVarray role_constraints;
#if 0 /* additional parameters from Fortress */
	private String userId;
	@XmlElement(nillable = true)
		private char[] password;
	@XmlElement(nillable = true)
		private char[] newPassword;
	private String internalId;
	@XmlElement(nillable = true)
		private List<UserRole> roles;
	@XmlElement(nillable = true)
		private List<UserAdminRole> adminRoles;
	private String pwPolicy;
	private String cn;
	private String sn;
	private String dn;
	private String ou;
	private String description;
	private String beginTime;
	private String endTime;
	private String beginDate;
	private String endDate;
	private String beginLockDate;
	private String endLockDate;
	private String dayMask;
	private String name;
	private int timeout;
	private boolean reset;
	private boolean locked;
	private Boolean system;
	@XmlElement(nillable = true)
		private Props props = new Props();
	@XmlElement(nillable = true)
		private Address address;
	@XmlElement(nillable = true)
		private List<String> phones;
	@XmlElement(nillable = true)
		private List<String> mobiles;
	@XmlElement(nillable = true)
		private List<String> emails;
#endif /* 0 */
} rbac_user_t;

enum {
	RBAC_NONE = 0,
	RBAC_TENANT,
	RBAC_TENANT_ID,
	RBAC_USERS_BASE_DN,
	RBAC_ROLES_BASE_DN,
	RBAC_PERMISSIONS_BASE_DN,
	RBAC_ADMIN_DN,
	RBAC_ADMIN_PWD,
	RBAC_SESSIONS_BASE_DN,
	RBAC_SESSION_ADMIN_DN,
	RBAC_SESSION_ADMIN_PWD,
	RBAC_ROLE_ASSIGNMENT,
	RBAC_ROLE_CONSTRAINTS,
	RBAC_USER_CONSTRAINTS,
	RBAC_UID,
	RBAC_USERS,
	RBAC_ROLES,
	RBAC_OBJ_NAME,
	RBAC_OP_NAME,
	RBAC_ROLE_NAME,
	RBAC_SESSION_ID,
	RBAC_USER_DN,
	RBAC_AUDIT_ROLES,
	RBAC_AUDIT_RESOURCES,
	RBAC_AUDIT_RESULT,
	RBAC_AUDIT_TIMESTAMP,
	RBAC_AUDIT_PROPERTIES,
	RBAC_AUDIT_OP,
	RBAC_AUDIT_ID,
	RBAC_AUDIT_REQUESTED_ROLES,
	RBAC_AUDIT_OBJS,
	RBAC_AUDIT_OPS,
	RBAC_AUDIT_MSGS,
	RBAC_LAST
};

enum {
	RBAC_DEFAULT_TENANT_ID = RBAC_LAST,
	RBAC_DEFAULT_USERS_BASE_DN,
	RBAC_DEFAULT_PERMISSIONS_BASE_DN,
	RBAC_DEFAULT_ROLES_BASE_DN,
	RBAC_DEFAULT_SESSIONS_BASE_DN,
	RBAC_DEFAULT_AUDIT_BASE_DN
};

typedef struct rbac_user_idlist {
	char *user_id;
	struct rbac_user_idlist *next;
} rbac_user_idlist_t;

/* RBAC sessions */
#define RBAC_SESSION_RDN_EQ "rbacSessid="
#define RBAC_AUDIT_RDN_EQ "rbacAuditId="

typedef struct rbac_session {
	rbac_user_t *user;
	struct berval tenantid;
	struct berval sessid;
	struct berval uid;
	struct berval userdn;
	char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
	struct berval sessdn;
	long last_access;
	int timeout;
	int warning_id;
	int error_id;
	int grace_logins;
	int expiration_secs;
	int is_authenticated; /* boolean */
	struct berval message;
	BerVarray roles;
	BerVarray role_constraints;
} rbac_session_t;

/* RBAC roles */
typedef struct rbac_role {
	char *name;
	char *description;
	struct rbac_role *parent;
	struct rbac_role *next;
} rbac_role_t;

typedef struct rbac_role_list {
	char *name;
	struct rbac_role_list *next;
} rbac_role_list_t;

/* RBAC permissions */
typedef struct rbac_permission {
	struct berval dn;
	int admin; /* boolean */
	struct berval internalId;
	BerVarray opName;
	BerVarray objName;
	struct berval objectId;
	struct berval abstractName;
	struct berval type;
	BerVarray roles;
	BerVarray uids;
	struct rbac_permission *next;
} rbac_permission_t;

/* RBAC Audit */
typedef enum {
	CreateSession = 0,
	CheckAccess,
	AddActiveRole,
	DropActiveRole,
	SessionPermissions,
	DeleteSession,
	SessionRoles
} audit_op_t;

/* function prototypes */

int rbac_initialize_repository( void );
int rbac_initialize_tenants( BackendDB *be, ConfigReply *cr );

/* RBAC tenant information */
tenant_info_t *rbac_tid2tenant( struct berval *tid );

rbac_req_t *rbac_alloc_req( int type );
void rbac_free_req( rbac_req_t *reqp );

rbac_user_t *rbac_read_user( Operation *op, rbac_req_t *rabc_reqp );
int rbac_authenticate_user( Operation *op, rbac_user_t *user );
int rbac_user_temporal_constraint( rbac_user_t *userp );
void rbac_free_user( rbac_user_t *user );

rbac_session_t *rbac_alloc_session( void );
int rbac_is_valid_session_id( struct berval *sessid );
rbac_session_t *rbac_session_byid( Operation *op, rbac_req_t *reqp );
int rbac_is_session_owner( rbac_session_t *sessp, rbac_req_t *reqp );
int rbac_register_session( Operation *op, SlapReply *rs, rbac_session_t *sess );
int rbac_int_delete_session( Operation *op, rbac_session_t *sessp );
int rbac_session_add_role(
	Operation *op,
	rbac_session_t *sessp,
	rbac_req_t *reqp );
int rbac_session_drop_role(
	Operation *op,
	rbac_session_t *sessp,
	rbac_req_t *reqp );
int rbac_int_session_permissions(
	Operation *op,
	SlapReply *rs,
	rbac_req_t *reqp,
	rbac_session_t *sessp );
int activate_session_roles(
	rbac_session_t *sessp,
	rbac_req_t *reqp,
	rbac_user_t *userp );
void rbac_free_session( rbac_session_t *sessp );

rbac_constraint_t *rbac_user_role_constraints( BerVarray values );
rbac_constraint_t *rbac_role2constraint(
	struct berval *role,
	rbac_constraint_t *role_constraints );
rbac_constraint_t *rbac_bv2constraint( struct berval *bv );
int rbac_check_time_constraint( rbac_constraint_t *cp );
void rbac_free_constraint( rbac_constraint_t *cp );
void rbac_free_constraints( rbac_constraint_t *constraints );

rbac_permission_t *rbac_read_permission( Operation *op, rbac_req_t *rbac_reqp );
int rbac_check_session_permission(
	rbac_session_t *sessp,
	rbac_permission_t *permp,
	rbac_constraint_t *role_constraints );
void rbac_free_permission( rbac_permission_t *permp );

/* audit functions */
void rbac_audit(
	Operation *op,
	audit_op_t rbac_op,
	rbac_session_t *sessp,
	rbac_req_t *reqp,
	int result,
	char *msg );

/* acl functions */
int rbac_create_session_acl_check( struct berval *sessid, rbac_user_t *userp );

void rbac_to_lower( struct berval *bv );

LDAP_END_DECL

#endif /* RBAC_H */
