/*	$NetBSD: rbacuser.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* rbacuser.c - RBAC users */
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
__RCSID("$NetBSD: rbacuser.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

static int ppolicy_cid = -1;

static rbac_user_t *
rbac_alloc_user()
{
	rbac_user_t *userp = ch_calloc( 1, sizeof(rbac_user_t) );

	BER_BVZERO( &userp->tenantid );
	BER_BVZERO( &userp->uid );
	BER_BVZERO( &userp->dn );
	BER_BVZERO( &userp->password );
	BER_BVZERO( &userp->constraints );
	BER_BVZERO( &userp->msg );
	userp->roles = NULL;
	userp->role_constraints = NULL;

	return userp;
}

static int
rbac_read_user_cb( Operation *op, SlapReply *rs )
{
	rbac_callback_info_t *cbp = op->o_callback->sc_private;
	rbac_ad_t *user_ads;
	rbac_user_t *userp = NULL;
	int rc = 0, i;

	Debug( LDAP_DEBUG_ANY, "rbac_read_user_cb\n" );

	if ( rs->sr_type != REP_SEARCH ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_user_cb: "
				"sr_type != REP_SEARCH\n" );
		return 0;
	}

	assert( cbp );

	user_ads = cbp->tenantp->schema->user_ads;

	userp = rbac_alloc_user();
	if ( !userp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_user_cb: "
				"rbac_alloc_user failed\n" );

		goto done;
	}

	ber_dupbv( &userp->dn, &rs->sr_entry->e_name );

	Debug( LDAP_DEBUG_ANY, "DEBUG rbac_read_user_cb (%s): "
			"rc (%d)\n",
			userp->dn.bv_val, rc );

	for ( i = 0; !BER_BVISNULL( &user_ads[i].attr ); i++ ) {
		Attribute *attr = NULL;

		attr = attr_find( rs->sr_entry->e_attrs, *user_ads[i].ad );
		if ( attr != NULL ) {
			switch ( user_ads[i].type ) {
				case RBAC_ROLE_ASSIGNMENT:
					ber_bvarray_dup_x( &userp->roles, attr->a_nvals, NULL );
					break;
				case RBAC_ROLE_CONSTRAINTS:
					ber_bvarray_dup_x(
							&userp->role_constraints, attr->a_nvals, NULL );
					break;
				case RBAC_USER_CONSTRAINTS:
					ber_dupbv_x( &userp->constraints, &attr->a_nvals[0], NULL );
					break;
				case RBAC_UID:
					ber_dupbv_x( &userp->uid, &attr->a_nvals[0], NULL );
					break;
				default:
					break;
			}
		}
	}

done:;
	cbp->private = userp;

	return 0;
}

static int
rbac_bind_cb( Operation *op, SlapReply *rs )
{
	rbac_user_t *ui = op->o_callback->sc_private;

	LDAPControl *ctrl = ldap_control_find(
			LDAP_CONTROL_PASSWORDPOLICYRESPONSE, rs->sr_ctrls, NULL );
	if ( ctrl ) {
		LDAP *ld;
		ber_int_t expire, grace;
		LDAPPasswordPolicyError error;

		ldap_create( &ld );
		if ( ld ) {
			int rc = ldap_parse_passwordpolicy_control(
					ld, ctrl, &expire, &grace, &error );
			if ( rc == LDAP_SUCCESS ) {
				ui->authz = RBAC_PASSWORD_GOOD;
				if ( grace > 0 ) {
					//ui->msg.bv_len = sprintf(ui->msg.bv_val,
					//		"Password expired; %d grace logins remaining",
					//		grace);
					ui->authz = RBAC_BIND_NEW_AUTHTOK_REQD;
				} else if ( error != PP_noError ) {
					ber_str2bv( ldap_passwordpolicy_err2txt( error ), 0, 0,
							&ui->msg );

					switch ( error ) {
						case PP_passwordExpired:
							ui->authz = RBAC_PASSWORD_EXPIRATION_WARNING;

							if ( expire >= 0 ) {
								char *unit = "seconds";
								if ( expire > 60 ) {
									expire /= 60;
									unit = "minutes";
								}
								if ( expire > 60 ) {
									expire /= 60;
									unit = "hours";
								}
								if ( expire > 24 ) {
									expire /= 24;
									unit = "days";
								}
#if 0 /* Who warns about expiration so far in advance? */
								if (expire > 7) {
									expire /= 7;
									unit = "weeks";
								}
								if (expire > 4) {
									expire /= 4;
									unit = "months";
								}
								if (expire > 12) {
									expire /= 12;
									unit = "years";
								}
#endif
							}

							//rs->sr_err = ;
							break;
						case PP_accountLocked:
							ui->authz = RBAC_ACCOUNT_LOCKED;
							//rs->sr_err = ;
							break;
						case PP_changeAfterReset:
							ui->authz = RBAC_CHANGE_AFTER_RESET;
							rs->sr_err = LDAP_SUCCESS;
							break;
						case PP_passwordModNotAllowed:
							ui->authz = RBAC_NO_MODIFICATIONS;
							//rs->sr_err = ;
							break;
						case PP_mustSupplyOldPassword:
							ui->authz = RBAC_MUST_SUPPLY_OLD;
							//rs->sr_err = ;
							break;
						case PP_insufficientPasswordQuality:
							ui->authz = RBAC_INSUFFICIENT_QUALITY;
							//rs->sr_err = ;
							break;
						case PP_passwordTooShort:
							ui->authz = RBAC_PASSWORD_TOO_SHORT;
							//rs->sr_err = ;
							break;
						case PP_passwordTooYoung:
							ui->authz = RBAC_PASSWORD_TOO_YOUNG;
							//rs->sr_err = ;
							break;
						case PP_passwordInHistory:
							ui->authz = RBAC_HISTORY_VIOLATION;
							//rs->sr_err = ;
							break;
						case PP_noError:
						default:
							// do nothing
							//ui->authz = RBAC_PASSWORD_GOOD;
							rs->sr_err = LDAP_SUCCESS;
							break;
					}

//					switch (error) {
//					case PP_passwordExpired:
						/* report this during authz */
//						rs->sr_err = LDAP_SUCCESS;
						/* fallthru */
//					case PP_changeAfterReset:
//						ui->authz = RBAC_BIND_NEW_AUTHTOK_REQD;
//					}
				}
			}
			ldap_unbind_ext( ld, NULL, NULL );
		}
	}

	return 0;
}

/* exported user functions */
int
rbac_authenticate_user( Operation *op, rbac_user_t *userp )
{
	int rc = LDAP_SUCCESS;
	slap_callback cb = { 0 };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	LDAPControl *sctrls[4];
	LDAPControl sctrl[3];
	int nsctrls = 0;
	LDAPControl c;
	struct berval ber_bvnull = BER_BVNULL;
	struct berval dn, ndn;

	rc = dnPrettyNormal( 0, &userp->dn, &dn, &ndn, NULL );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	cb.sc_response = rbac_bind_cb;
	cb.sc_private = userp;
	op2.o_callback = &cb;
	op2.o_dn = ber_bvnull;
	op2.o_ndn = ber_bvnull;
	op2.o_tag = LDAP_REQ_BIND;
	op2.o_protocol = LDAP_VERSION3;
	op2.orb_method = LDAP_AUTH_SIMPLE;
	op2.orb_cred = userp->password;
	op2.o_req_dn = dn;
	op2.o_req_ndn = ndn;

	// loading the ldap pw policy controls loaded into here, added by smm:
	c.ldctl_oid = LDAP_CONTROL_PASSWORDPOLICYREQUEST;
	c.ldctl_value.bv_val = NULL;
	c.ldctl_value.bv_len = 0;
	c.ldctl_iscritical = 0;
	sctrl[nsctrls] = c;
	sctrls[nsctrls] = &sctrl[nsctrls];
	sctrls[++nsctrls] = NULL;
	op2.o_ctrls = sctrls;

	if ( ppolicy_cid < 0 ) {
		rc = slap_find_control_id( LDAP_CONTROL_PASSWORDPOLICYREQUEST,
				&ppolicy_cid );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
	}
	// smm - need to set the control flag too:
	op2.o_ctrlflag[ppolicy_cid] = SLAP_CONTROL_CRITICAL;

	slap_op_time( &op2.o_time, &op2.o_tincr );
	op2.o_bd = frontendDB;
	rc = op2.o_bd->be_bind( &op2, &rs2 );
	if ( userp->authz > 0 ) {
		Debug( LDAP_DEBUG_ANY, "rbac_authenticate_user (%s): "
				"password policy violation (%d)\n",
				userp->dn.bv_val ? userp->dn.bv_val : "NULL", userp->authz );
	}

done:;
	ch_free( dn.bv_val );
	ch_free( ndn.bv_val );

	Debug( LDAP_DEBUG_ANY, "rbac_authenticate_user (%s): "
			"rc (%d)\n",
			userp->dn.bv_val ? userp->dn.bv_val : "NULL", rc );
	return rc;
}

/*
	isvalidusername(): from OpenLDAP ~/contrib/slapd-modules/nssov/passwd.c
	Checks to see if the specified name is a valid user name.

    This test is based on the definition from POSIX (IEEE Std 1003.1, 2004, 3.426 User Name
	 and 3.276 Portable Filename Character Set):
	 http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_426
	 http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_276

	 The standard defines user names valid if they contain characters from
	 the set [A-Za-z0-9._-] where the hyphen should not be used as first
	 character. As an extension this test allows the dolar '$' sign as the last
	 character to support Samba special accounts.
*/
static int
isvalidusername( struct berval *bv )
{
	int i;
	char *name = bv->bv_val;
	if ( (name == NULL) || ( name[0] == '\0' ) ) return 0;
	/* check first character */
	if ( !( ( name[0] >= 'A' && name[0] <= 'Z' ) ||
				 ( name[0] >= 'a' && name[0] <= 'z' ) ||
				 ( name[0] >= '0' && name[0] <= '9' ) || name[0] == '.' ||
				 name[0] == '_' ) )
		return 0;
	/* check other characters */
	for ( i = 1; i < bv->bv_len; i++ ) {
		if ( name[i] == '$' ) {
			/* if the char is $ we require it to be the last char */
			if ( name[i + 1] != '\0' ) return 0;
		} else if ( !( ( name[i] >= 'A' && name[i] <= 'Z' ) ||
							( name[i] >= 'a' && name[i] <= 'z' ) ||
							( name[i] >= '0' && name[i] <= '9' ) ||
							name[i] == '.' || name[i] == '_' ||
							name[i] == '-' ) )
			return 0;
	}
	/* no test failed so it must be good */
	return -1;
}

rbac_user_t *
rbac_read_user( Operation *op, rbac_req_t *reqp )
{
	int rc = LDAP_SUCCESS;
	tenant_info_t *tenantp = rbac_tid2tenant( &reqp->tenantid );
	rbac_user_t *userp = NULL;
	char fbuf[RBAC_BUFLEN];
	struct berval filter = { sizeof(fbuf), fbuf };
	SlapReply rs2 = { REP_RESULT };
	Operation op2 = *op;
	slap_callback cb = { 0 };
	rbac_callback_info_t rbac_cb;

	if ( !tenantp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_user: "
				"missing tenant information\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* uid is a pre-requisite for reading the user information */
	if ( BER_BVISNULL( &reqp->uid ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_user: "
				"missing uid, unable to read user entry\n" );
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	if ( !isvalidusername( &reqp->uid ) ) {
		Debug( LDAP_DEBUG_ANY, "rbac_read_user: "
				"invalid user id\n" );
		rc = LDAP_NO_SUCH_OBJECT;
		goto done;
	}

	rbac_cb.tenantp = tenantp;
	rbac_cb.private = NULL;

	memset( fbuf, 0, sizeof(fbuf) );
	strcpy( fbuf, "uid=" );
	strncat( fbuf, reqp->uid.bv_val, reqp->uid.bv_len );
	filter.bv_val = fbuf;
	filter.bv_len = strlen( fbuf );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "rbac_create_session: "
				"invalid DN syntax\n" );
		goto done;
	}

	cb.sc_private = &rbac_cb;
	cb.sc_response = rbac_read_user_cb;
	op2.o_callback = &cb;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_dn = tenantp->admin;
	op2.o_ndn = tenantp->admin;
	op2.o_req_dn = tenantp->users_basedn;
	op2.o_req_ndn = tenantp->users_basedn;
	op2.ors_filterstr = filter;
	op2.ors_filter = str2filter_x( &op2, filter.bv_val );
	op2.ors_scope = LDAP_SCOPE_SUBTREE;
	op2.ors_attrs = tenantp->schema->user_attrs;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_slimit = SLAP_NO_LIMIT;
	op2.ors_attrsonly = 0;
	op2.o_bd = frontendDB;
	op2.ors_limit = NULL;
	rc = op2.o_bd->be_search( &op2, &rs2 );
	filter_free_x( &op2, op2.ors_filter, 1 );

done:;
	if ( rc == LDAP_SUCCESS && rbac_cb.private ) {
		userp = (rbac_user_t *)rbac_cb.private;
		if ( !BER_BVISNULL( &reqp->authtok ) )
			ber_dupbv( &userp->password, &reqp->authtok );
		rbac_cb.private = NULL;
		return userp;
	} else {
		userp = (rbac_user_t *)rbac_cb.private;
		rbac_free_user( userp );
		return NULL;
	}
}

/* evaluate temporal constraints for the user */
int
rbac_user_temporal_constraint( rbac_user_t *userp )
{
	int rc = LDAP_SUCCESS;
	rbac_constraint_t *cp = NULL;

	if ( BER_BVISNULL( &userp->constraints ) ) {
		/* no temporal constraint */
		goto done;
	}

	cp = rbac_bv2constraint( &userp->constraints );
	if ( !cp ) {
		Debug( LDAP_DEBUG_ANY, "rbac_user_temporal_constraint: "
				"invalid user constraint \n" );
		rc = LDAP_OTHER;
		goto done;
	}

	rc = rbac_check_time_constraint( cp );

done:;
	rbac_free_constraint( cp );

	return rc;
}

/*
rbac_constraint_t *
rbac_user_role_constraintsx(rbac_user_t *userp)
{
	rbac_constraint_t *tmp, *cp = NULL;
	int i = 0;

	if (!userp || !userp->role_constraints)
		goto done;

	while (!BER_BVISNULL(&userp->role_constraints[i])) {
		tmp = rbac_bv2constraint(&userp->role_constraints[i++]);
		if (tmp) {
			if (!cp) {
				cp = tmp;
			} else {
				tmp->next = cp;
				cp = tmp;
			}
		}
	}

done:;
	return cp;
}
*/

rbac_constraint_t *
rbac_user_role_constraints( BerVarray values )
{
	rbac_constraint_t *curr, *head = NULL;
	int i = 0;

	if ( values ) {
		while ( !BER_BVISNULL( &values[i] ) ) {
			curr = rbac_bv2constraint( &values[i++] );
			if ( curr ) {
				curr->next = head;
				head = curr;
			}
		}
	}

	return head;
}

/*

void main() {
   item * curr, * head;
   int i;

   head = NULL;

   for(i=1;i<=10;i++) {
      curr = (item *)malloc(sizeof(item));
      curr->val = i;
      curr->next  = head;
      head = curr;
   }

   curr = head;

   while(curr) {
      printf("%d\n", curr->val);
      curr = curr->next ;
   }
}

 */

/*
 *
rbac_user_role_constraints2(BerVarray values)
{
	rbac_constraint_t *tmp, *cp = NULL;
	int i = 0;

	if (!values)
		goto done;

	while (!BER_BVISNULL(&values[i])) {
		tmp = rbac_bv2constraint(&values[i++]);
		if (tmp) {
			if (!cp) {
				cp = tmp;
			} else {
				tmp->next = cp;
				cp = tmp;
				//cp->next = tmp;
				//cp = tmp->next;

			}
		}
	}

done:;
	return cp;
}


rbac_user_role_constraints3(rbac_constraint_t *values)
{
	rbac_constraint_t *tmp, *cp = NULL;
	int i = 0;

	if (!values)
		goto done;

	while (!BER_BVISNULL(values[i])) {
		tmp = rbac_bv2constraint(&values[i++]);
		if (tmp) {
			if (!cp) {
				cp = tmp;
			} else {
				tmp->next = cp;
				cp = tmp;
			}
		}
	}

done:;
	return cp;
}
*/

void
rbac_free_user( rbac_user_t *userp )
{
	if ( !userp ) return;

	if ( !BER_BVISNULL( &userp->tenantid ) ) {
		ber_memfree( userp->tenantid.bv_val );
	}

	if ( !BER_BVISNULL( &userp->uid ) ) {
		ber_memfree( userp->uid.bv_val );
	}

	if ( !BER_BVISNULL( &userp->dn ) ) {
		ber_memfree( userp->dn.bv_val );
	}

	if ( !BER_BVISNULL( &userp->constraints ) ) {
		ber_memfree( userp->constraints.bv_val );
	}

	if ( !BER_BVISNULL( &userp->password ) ) {
		ber_memfree( userp->password.bv_val );
	}

	if ( !BER_BVISNULL( &userp->msg ) ) {
		ber_memfree( userp->msg.bv_val );
	}

	if ( userp->roles ) ber_bvarray_free( userp->roles );

	if ( userp->role_constraints ) ber_bvarray_free( userp->role_constraints );

	ch_free( userp );
}
