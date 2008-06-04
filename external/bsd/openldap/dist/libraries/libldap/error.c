/* $OpenLDAP: pkg/ldap/libraries/libldap/error.c,v 1.76.2.3 2008/02/11 23:26:41 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

struct ldaperror {
	int	e_code;
	char *e_reason;
};

static struct ldaperror ldap_builtin_errlist[] = {
	{LDAP_SUCCESS, 					N_("Success")},
	{LDAP_OPERATIONS_ERROR, 		N_("Operations error")},
	{LDAP_PROTOCOL_ERROR, 			N_("Protocol error")},
	{LDAP_TIMELIMIT_EXCEEDED,		N_("Time limit exceeded")},
	{LDAP_SIZELIMIT_EXCEEDED, 		N_("Size limit exceeded")},
	{LDAP_COMPARE_FALSE, 			N_("Compare False")},
	{LDAP_COMPARE_TRUE, 			N_("Compare True")},
	{LDAP_STRONG_AUTH_NOT_SUPPORTED, N_("Authentication method not supported")},
	{LDAP_STRONG_AUTH_REQUIRED, 	N_("Strong(er) authentication required")},
	{LDAP_PARTIAL_RESULTS, 			N_("Partial results and referral received")},

	{LDAP_REFERRAL,					N_("Referral")},
	{LDAP_ADMINLIMIT_EXCEEDED,		N_("Administrative limit exceeded")},
	{LDAP_UNAVAILABLE_CRITICAL_EXTENSION,
									N_("Critical extension is unavailable")},
	{LDAP_CONFIDENTIALITY_REQUIRED,	N_("Confidentiality required")},
	{LDAP_SASL_BIND_IN_PROGRESS,	N_("SASL bind in progress")},

	{LDAP_NO_SUCH_ATTRIBUTE, 		N_("No such attribute")},
	{LDAP_UNDEFINED_TYPE, 			N_("Undefined attribute type")},
	{LDAP_INAPPROPRIATE_MATCHING, 	N_("Inappropriate matching")},
	{LDAP_CONSTRAINT_VIOLATION, 	N_("Constraint violation")},
	{LDAP_TYPE_OR_VALUE_EXISTS, 	N_("Type or value exists")},
	{LDAP_INVALID_SYNTAX, 			N_("Invalid syntax")},

	{LDAP_NO_SUCH_OBJECT, 			N_("No such object")},
	{LDAP_ALIAS_PROBLEM, 			N_("Alias problem")},
	{LDAP_INVALID_DN_SYNTAX,		N_("Invalid DN syntax")},
	{LDAP_IS_LEAF, 					N_("Entry is a leaf")},
	{LDAP_ALIAS_DEREF_PROBLEM,	 	N_("Alias dereferencing problem")},

	{LDAP_INAPPROPRIATE_AUTH, 		N_("Inappropriate authentication")},
	{LDAP_INVALID_CREDENTIALS, 		N_("Invalid credentials")},
	{LDAP_INSUFFICIENT_ACCESS, 		N_("Insufficient access")},
	{LDAP_BUSY, 					N_("Server is busy")},
	{LDAP_UNAVAILABLE, 				N_("Server is unavailable")},
	{LDAP_UNWILLING_TO_PERFORM, 	N_("Server is unwilling to perform")},
	{LDAP_LOOP_DETECT, 				N_("Loop detected")},

	{LDAP_NAMING_VIOLATION, 		N_("Naming violation")},
	{LDAP_OBJECT_CLASS_VIOLATION, 	N_("Object class violation")},
	{LDAP_NOT_ALLOWED_ON_NONLEAF, 	N_("Operation not allowed on non-leaf")},
	{LDAP_NOT_ALLOWED_ON_RDN,	 	N_("Operation not allowed on RDN")},
	{LDAP_ALREADY_EXISTS, 			N_("Already exists")},
	{LDAP_NO_OBJECT_CLASS_MODS, 	N_("Cannot modify object class")},
	{LDAP_RESULTS_TOO_LARGE,		N_("Results too large")},
	{LDAP_AFFECTS_MULTIPLE_DSAS,	N_("Operation affects multiple DSAs")},

	{LDAP_OTHER, 					N_("Other (e.g., implementation specific) error")},

	{LDAP_CANCELLED,				N_("Cancelled")},
	{LDAP_NO_SUCH_OPERATION,		N_("No Operation to Cancel")},
	{LDAP_TOO_LATE,					N_("Too Late to Cancel")},
	{LDAP_CANNOT_CANCEL,			N_("Cannot Cancel")},

	{LDAP_ASSERTION_FAILED,			N_("Assertion Failed")},
	{LDAP_X_ASSERTION_FAILED,		N_("Assertion Failed (X)")},

	{LDAP_PROXIED_AUTHORIZATION_DENIED, N_("Proxied Authorization Denied")},
	{LDAP_X_PROXY_AUTHZ_FAILURE,		N_("Proxy Authorization Failure (X)")},

	{LDAP_SYNC_REFRESH_REQUIRED,	N_("Content Sync Refresh Required")},
	{LDAP_X_SYNC_REFRESH_REQUIRED,	N_("Content Sync Refresh Required (X)")},

	{LDAP_X_NO_OPERATION,			N_("No Operation (X)")},

	{LDAP_CUP_RESOURCES_EXHAUSTED,	N_("LCUP Resources Exhausted")},
	{LDAP_CUP_SECURITY_VIOLATION,	N_("LCUP Security Violation")},
	{LDAP_CUP_INVALID_DATA,			N_("LCUP Invalid Data")},
	{LDAP_CUP_UNSUPPORTED_SCHEME,	N_("LCUP Unsupported Scheme")},
	{LDAP_CUP_RELOAD_REQUIRED,		N_("LCUP Reload Required")},

#ifdef LDAP_X_TXN
	{LDAP_X_TXN_SPECIFY_OKAY,		N_("TXN specify okay")},
	{LDAP_X_TXN_ID_INVALID,			N_("TXN ID is invalid")},
#endif

	/* API ResultCodes */
	{LDAP_SERVER_DOWN,				N_("Can't contact LDAP server")},
	{LDAP_LOCAL_ERROR,				N_("Local error")},
	{LDAP_ENCODING_ERROR,			N_("Encoding error")},
	{LDAP_DECODING_ERROR,			N_("Decoding error")},
	{LDAP_TIMEOUT,					N_("Timed out")},
	{LDAP_AUTH_UNKNOWN,				N_("Unknown authentication method")},
	{LDAP_FILTER_ERROR,				N_("Bad search filter")},
	{LDAP_USER_CANCELLED,			N_("User cancelled operation")},
	{LDAP_PARAM_ERROR,				N_("Bad parameter to an ldap routine")},
	{LDAP_NO_MEMORY,				N_("Out of memory")},

	{LDAP_CONNECT_ERROR,			N_("Connect error")},
	{LDAP_NOT_SUPPORTED,			N_("Not Supported")},
	{LDAP_CONTROL_NOT_FOUND,		N_("Control not found")},
	{LDAP_NO_RESULTS_RETURNED,		N_("No results returned")},
	{LDAP_MORE_RESULTS_TO_RETURN,	N_("More results to return")},
	{LDAP_CLIENT_LOOP,				N_("Client Loop")},
	{LDAP_REFERRAL_LIMIT_EXCEEDED,	N_("Referral Limit Exceeded")},

	{0, NULL}
};

static struct ldaperror *ldap_errlist = ldap_builtin_errlist; 

void ldap_int_error_init( void ) {
}

static const struct ldaperror *
ldap_int_error( int err )
{
	int	i;

	/* XXYYZ: O(n) search instead of O(1) lookup */
	for ( i=0; ldap_errlist[i].e_reason != NULL; i++ ) {
		if ( err == ldap_errlist[i].e_code ) {
			return &ldap_errlist[i];
		}
	}

	return NULL;
}

char *
ldap_err2string( int err )
{
	const struct ldaperror *e;
	
	Debug( LDAP_DEBUG_TRACE, "ldap_err2string\n", 0, 0, 0 );

	e = ldap_int_error( err );

	if (e) {
		return e->e_reason;

	} else if ( LDAP_API_ERROR(err) ) {
		return _("Unknown API error");

	} else if ( LDAP_E_ERROR(err) ) {
		return _("Unknown (extension) error");

	} else if ( LDAP_X_ERROR(err) ) {
		return _("Unknown (private extension) error");
	}

	return _("Unknown error");
}

/* deprecated */
void
ldap_perror( LDAP *ld, LDAP_CONST char *str )
{
    int i;
	const struct ldaperror *e;
	Debug( LDAP_DEBUG_TRACE, "ldap_perror\n", 0, 0, 0 );

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( str != NULL );

	e = ldap_int_error( ld->ld_errno );

	fprintf( stderr, "%s: %s (%d)\n",
		str ? str : "ldap_perror",
		e ? _(e->e_reason) : _("unknown result code"),
		ld->ld_errno );

	if ( ld->ld_matched != NULL && ld->ld_matched[0] != '\0' ) {
		fprintf( stderr, _("\tmatched DN: %s\n"), ld->ld_matched );
	}

	if ( ld->ld_error != NULL && ld->ld_error[0] != '\0' ) {
		fprintf( stderr, _("\tadditional info: %s\n"), ld->ld_error );
	}

	if ( ld->ld_referrals != NULL && ld->ld_referrals[0] != NULL) {
		fprintf( stderr, _("\treferrals:\n") );
		for (i=0; ld->ld_referrals[i]; i++) {
			fprintf( stderr, _("\t\t%s\n"), ld->ld_referrals[i] );
		}
	}

	fflush( stderr );
}

/* deprecated */
int
ldap_result2error( LDAP *ld, LDAPMessage *r, int freeit )
{
	int rc, err;

	rc = ldap_parse_result( ld, r, &err,
		NULL, NULL, NULL, NULL, freeit );

	return err != LDAP_SUCCESS ? err : rc;
}

/*
 * Parse LDAPResult Messages:
 *
 *   LDAPResult ::= SEQUENCE {
 *     resultCode      ENUMERATED,
 *     matchedDN       LDAPDN,
 *     errorMessage    LDAPString,
 *     referral        [3] Referral OPTIONAL }
 *
 * including Bind results:
 *
 *   BindResponse ::= [APPLICATION 1] SEQUENCE {
 *     COMPONENTS OF LDAPResult,
 *     serverSaslCreds  [7] OCTET STRING OPTIONAL }
 *
 * and ExtendedOp results:
 *
 *   ExtendedResponse ::= [APPLICATION 24] SEQUENCE {
 *     COMPONENTS OF LDAPResult,
 *     responseName     [10] LDAPOID OPTIONAL,
 *     response         [11] OCTET STRING OPTIONAL }
 *
 */
int
ldap_parse_result(
	LDAP			*ld,
	LDAPMessage		*r,
	int				*errcodep,
	char			**matcheddnp,
	char			**errmsgp,
	char			***referralsp,
	LDAPControl		***serverctrls,
	int				freeit )
{
	LDAPMessage	*lm;
	ber_int_t errcode = LDAP_SUCCESS;

	ber_tag_t tag;
	BerElement	*ber;

	Debug( LDAP_DEBUG_TRACE, "ldap_parse_result\n", 0, 0, 0 );

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( r != NULL );

	if(errcodep != NULL) *errcodep = LDAP_SUCCESS;
	if(matcheddnp != NULL) *matcheddnp = NULL;
	if(errmsgp != NULL) *errmsgp = NULL;
	if(referralsp != NULL) *referralsp = NULL;
	if(serverctrls != NULL) *serverctrls = NULL;

#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &ld->ld_res_mutex );
#endif
	/* Find the result, last msg in chain... */
	lm = r->lm_chain_tail;
	/* FIXME: either this is not possible (assert?)
	 * or it should be handled */
	if ( lm != NULL ) {
		switch ( lm->lm_msgtype ) {
		case LDAP_RES_SEARCH_ENTRY:
		case LDAP_RES_SEARCH_REFERENCE:
		case LDAP_RES_INTERMEDIATE:
			lm = NULL;
			break;

		default:
			break;
		}
	}

	if( lm == NULL ) {
		ld->ld_errno = LDAP_NO_RESULTS_RETURNED;
#ifdef LDAP_R_COMPILE
		ldap_pvt_thread_mutex_unlock( &ld->ld_res_mutex );
#endif
		return ld->ld_errno;
	}

	if ( ld->ld_error ) {
		LDAP_FREE( ld->ld_error );
		ld->ld_error = NULL;
	}
	if ( ld->ld_matched ) {
		LDAP_FREE( ld->ld_matched );
		ld->ld_matched = NULL;
	}
	if ( ld->ld_referrals ) {
		LDAP_VFREE( ld->ld_referrals );
		ld->ld_referrals = NULL;
	}

	/* parse results */

	ber = ber_dup( lm->lm_ber );

	if ( ld->ld_version < LDAP_VERSION2 ) {
		tag = ber_scanf( ber, "{iA}",
			&ld->ld_errno, &ld->ld_error );

	} else {
		ber_len_t len;

		tag = ber_scanf( ber, "{iAA" /*}*/,
			&ld->ld_errno, &ld->ld_matched, &ld->ld_error );

		if( tag != LBER_ERROR ) {
			/* peek for referrals */
			if( ber_peek_tag(ber, &len) == LDAP_TAG_REFERRAL ) {
				tag = ber_scanf( ber, "v", &ld->ld_referrals );
			}
		}

		/* need to clean out misc items */
		if( tag != LBER_ERROR ) {
			if( lm->lm_msgtype == LDAP_RES_BIND ) {
				/* look for sasl result creditials */
				if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SASL_RES_CREDS ) {
					/* skip 'em */
					tag = ber_scanf( ber, "x" );
				}

			} else if( lm->lm_msgtype == LDAP_RES_EXTENDED ) {
				/* look for exop result oid or value */
				if ( ber_peek_tag( ber, &len ) == LDAP_TAG_EXOP_RES_OID ) {
					/* skip 'em */
					tag = ber_scanf( ber, "x" );
				}

				if ( tag != LBER_ERROR &&
					ber_peek_tag( ber, &len ) == LDAP_TAG_EXOP_RES_VALUE )
				{
					/* skip 'em */
					tag = ber_scanf( ber, "x" );
				}
			}
		}

		if( tag != LBER_ERROR ) {
			int rc = ldap_pvt_get_controls( ber, serverctrls );

			if( rc != LDAP_SUCCESS ) {
				tag = LBER_ERROR;
			}
		}

		if( tag != LBER_ERROR ) {
			tag = ber_scanf( ber, /*{*/"}" );
		}
	}

	if ( tag == LBER_ERROR ) {
		ld->ld_errno = errcode = LDAP_DECODING_ERROR;
	}

	if( ber != NULL ) {
		ber_free( ber, 0 );
	}

	/* return */
	if( errcodep != NULL ) {
		*errcodep = ld->ld_errno;
	}
	if ( errcode == LDAP_SUCCESS ) {
		if( matcheddnp != NULL ) {
			if ( ld->ld_matched )
			{
				*matcheddnp = LDAP_STRDUP( ld->ld_matched );
			}
		}
		if( errmsgp != NULL ) {
			if ( ld->ld_error )
			{
				*errmsgp = LDAP_STRDUP( ld->ld_error );
			}
		}

		if( referralsp != NULL) {
			*referralsp = ldap_value_dup( ld->ld_referrals );
		}
	}

	if ( freeit ) {
		ldap_msgfree( r );
	}
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &ld->ld_res_mutex );
#endif

	return( errcode );
}
