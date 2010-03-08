/*	$NetBSD: options.c,v 1.1.1.2 2010/03/08 02:14:16 lukem Exp $	*/

/* OpenLDAP: pkg/ldap/libraries/libldap/options.c,v 1.75.2.11 2009/08/12 23:40:56 quanah Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2009 The OpenLDAP Foundation.
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

#define LDAP_OPT_REBIND_PROC 0x4e814d
#define LDAP_OPT_REBIND_PARAMS 0x4e814e

#define LDAP_OPT_NEXTREF_PROC 0x4e815d
#define LDAP_OPT_NEXTREF_PARAMS 0x4e815e

#define LDAP_OPT_URLLIST_PROC 0x4e816d
#define LDAP_OPT_URLLIST_PARAMS 0x4e816e

static const LDAPAPIFeatureInfo features[] = {
#ifdef LDAP_API_FEATURE_X_OPENLDAP
	{	/* OpenLDAP Extensions API Feature */
		LDAP_FEATURE_INFO_VERSION,
		"X_OPENLDAP",
		LDAP_API_FEATURE_X_OPENLDAP
	},
#endif

#ifdef LDAP_API_FEATURE_THREAD_SAFE
	{	/* Basic Thread Safe */
		LDAP_FEATURE_INFO_VERSION,
		"THREAD_SAFE",
		LDAP_API_FEATURE_THREAD_SAFE
	},
#endif
#ifdef LDAP_API_FEATURE_SESSION_THREAD_SAFE
	{	/* Session Thread Safe */
		LDAP_FEATURE_INFO_VERSION,
		"SESSION_THREAD_SAFE",
		LDAP_API_FEATURE_SESSION_THREAD_SAFE
	},
#endif
#ifdef LDAP_API_FEATURE_OPERATION_THREAD_SAFE
	{	/* Operation Thread Safe */
		LDAP_FEATURE_INFO_VERSION,
		"OPERATION_THREAD_SAFE",
		LDAP_API_FEATURE_OPERATION_THREAD_SAFE
	},
#endif
#ifdef LDAP_API_FEATURE_X_OPENLDAP_REENTRANT
	{	/* OpenLDAP Reentrant */
		LDAP_FEATURE_INFO_VERSION,
		"X_OPENLDAP_REENTRANT",
		LDAP_API_FEATURE_X_OPENLDAP_REENTRANT
	},
#endif
#if defined( LDAP_API_FEATURE_X_OPENLDAP_THREAD_SAFE ) && \
	defined( LDAP_THREAD_SAFE )
	{	/* OpenLDAP Thread Safe */
		LDAP_FEATURE_INFO_VERSION,
		"X_OPENLDAP_THREAD_SAFE",
		LDAP_API_FEATURE_X_OPENLDAP_THREAD_SAFE
	},
#endif
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_REFERRALS
	{	/* V2 Referrals */
		LDAP_FEATURE_INFO_VERSION,
		"X_OPENLDAP_V2_REFERRALS",
		LDAP_API_FEATURE_X_OPENLDAP_V2_REFERRALS
	},
#endif
	{0, NULL, 0}
};

int
ldap_get_option(
	LDAP	*ld,
	int		option,
	void	*outvalue)
{
	struct ldapoptions *lo;

	/* Get pointer to global option structure */
	lo = LDAP_INT_GLOBAL_OPT();   
	if (NULL == lo)	{
		return LDAP_NO_MEMORY;
	}

	if( lo->ldo_valid != LDAP_INITIALIZED ) {
		ldap_int_initialize(lo, NULL);
	}

	if(ld != NULL) {
		assert( LDAP_VALID( ld ) );

		if( !LDAP_VALID( ld ) ) {
			return LDAP_OPT_ERROR;
		}

		lo = &ld->ld_options;
	}

	if(outvalue == NULL) {
		/* no place to get to */
		return LDAP_OPT_ERROR;
	}

	switch(option) {
	case LDAP_OPT_API_INFO: {
			struct ldapapiinfo *info = (struct ldapapiinfo *) outvalue;

			if(info == NULL) {
				/* outvalue must point to an apiinfo structure */
				return LDAP_OPT_ERROR;
			}

			if(info->ldapai_info_version != LDAP_API_INFO_VERSION) {
				/* api info version mismatch */
				info->ldapai_info_version = LDAP_API_INFO_VERSION;
				return LDAP_OPT_ERROR;
			}

			info->ldapai_api_version = LDAP_API_VERSION;
			info->ldapai_protocol_version = LDAP_VERSION_MAX;

			if(features[0].ldapaif_name == NULL) {
				info->ldapai_extensions = NULL;
			} else {
				int i;
				info->ldapai_extensions = LDAP_MALLOC(sizeof(char *) *
					sizeof(features)/sizeof(LDAPAPIFeatureInfo));

				for(i=0; features[i].ldapaif_name != NULL; i++) {
					info->ldapai_extensions[i] =
						LDAP_STRDUP(features[i].ldapaif_name);
				}

				info->ldapai_extensions[i] = NULL;
			}

			info->ldapai_vendor_name = LDAP_STRDUP(LDAP_VENDOR_NAME);
			info->ldapai_vendor_version = LDAP_VENDOR_VERSION;

			return LDAP_OPT_SUCCESS;
		} break;

	case LDAP_OPT_DESC:
		if( ld == NULL || ld->ld_sb == NULL ) {
			/* bad param */
			break;
		} 

		ber_sockbuf_ctrl( ld->ld_sb, LBER_SB_OPT_GET_FD, outvalue );
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_SOCKBUF:
		if( ld == NULL ) break;
		*(Sockbuf **)outvalue = ld->ld_sb;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_TIMEOUT:
		/* the caller has to free outvalue ! */
		if ( lo->ldo_tm_api.tv_sec < 0 ) {
			*(void **)outvalue = NULL;
		} else if ( ldap_int_timeval_dup( outvalue, &lo->ldo_tm_api ) != 0 ) {
			return LDAP_OPT_ERROR;
		}
		return LDAP_OPT_SUCCESS;
		
	case LDAP_OPT_NETWORK_TIMEOUT:
		/* the caller has to free outvalue ! */
		if ( lo->ldo_tm_net.tv_sec < 0 ) {
			*(void **)outvalue = NULL;
		} else if ( ldap_int_timeval_dup( outvalue, &lo->ldo_tm_net ) != 0 ) {
			return LDAP_OPT_ERROR;
		}
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_DEREF:
		* (int *) outvalue = lo->ldo_deref;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_SIZELIMIT:
		* (int *) outvalue = lo->ldo_sizelimit;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_TIMELIMIT:
		* (int *) outvalue = lo->ldo_timelimit;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_REFERRALS:
		* (int *) outvalue = (int) LDAP_BOOL_GET(lo, LDAP_BOOL_REFERRALS);
		return LDAP_OPT_SUCCESS;
		
	case LDAP_OPT_RESTART:
		* (int *) outvalue = (int) LDAP_BOOL_GET(lo, LDAP_BOOL_RESTART);
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_PROTOCOL_VERSION:
		* (int *) outvalue = lo->ldo_version;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_SERVER_CONTROLS:
		* (LDAPControl ***) outvalue =
			ldap_controls_dup( lo->ldo_sctrls );

		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_CLIENT_CONTROLS:
		* (LDAPControl ***) outvalue =
			ldap_controls_dup( lo->ldo_cctrls );

		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_HOST_NAME:
		* (char **) outvalue = ldap_url_list2hosts(lo->ldo_defludp);
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_URI:
		* (char **) outvalue = ldap_url_list2urls(lo->ldo_defludp);
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_DEFBASE:
		if( lo->ldo_defbase == NULL ) {
			* (char **) outvalue = NULL;
		} else {
			* (char **) outvalue = LDAP_STRDUP(lo->ldo_defbase);
		}

		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_CONNECT_ASYNC:
		* (int *) outvalue = (int) LDAP_BOOL_GET(lo, LDAP_BOOL_CONNECT_ASYNC);
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_CONNECT_CB:
		{
			/* Getting deletes the specified callback */
			ldaplist **ll = &lo->ldo_conn_cbs;
			for (;*ll;ll = &(*ll)->ll_next) {
				if ((*ll)->ll_data == outvalue) {
					ldaplist *lc = *ll;
					*ll = lc->ll_next;
					LDAP_FREE(lc);
					break;
				}
			}
		}
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_RESULT_CODE:
		if(ld == NULL) {
			/* bad param */
			break;
		} 
		* (int *) outvalue = ld->ld_errno;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_DIAGNOSTIC_MESSAGE:
		if(ld == NULL) {
			/* bad param */
			break;
		} 

		if( ld->ld_error == NULL ) {
			* (char **) outvalue = NULL;
		} else {
			* (char **) outvalue = LDAP_STRDUP(ld->ld_error);
		}

		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_MATCHED_DN:
		if(ld == NULL) {
			/* bad param */
			break;
		} 

		if( ld->ld_matched == NULL ) {
			* (char **) outvalue = NULL;
		} else {
			* (char **) outvalue = LDAP_STRDUP( ld->ld_matched );
		}

		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_REFERRAL_URLS:
		if(ld == NULL) {
			/* bad param */
			break;
		} 

		if( ld->ld_referrals == NULL ) {
			* (char ***) outvalue = NULL;
		} else {
			* (char ***) outvalue = ldap_value_dup(ld->ld_referrals);
		}

		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_API_FEATURE_INFO: {
			LDAPAPIFeatureInfo *info = (LDAPAPIFeatureInfo *) outvalue;
			int i;

			if(info == NULL) return LDAP_OPT_ERROR;

			if(info->ldapaif_info_version != LDAP_FEATURE_INFO_VERSION) {
				/* api info version mismatch */
				info->ldapaif_info_version = LDAP_FEATURE_INFO_VERSION;
				return LDAP_OPT_ERROR;
			}

			if(info->ldapaif_name == NULL) return LDAP_OPT_ERROR;

			for(i=0; features[i].ldapaif_name != NULL; i++) {
				if(!strcmp(info->ldapaif_name, features[i].ldapaif_name)) {
					info->ldapaif_version =
						features[i].ldapaif_version;
					return LDAP_OPT_SUCCESS;
				}
			}
		}
		break;

	case LDAP_OPT_DEBUG_LEVEL:
		* (int *) outvalue = lo->ldo_debug;
		return LDAP_OPT_SUCCESS;
	
	case LDAP_OPT_X_KEEPALIVE_IDLE:
		* (int *) outvalue = lo->ldo_keepalive_idle;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_X_KEEPALIVE_PROBES:
		* (int *) outvalue = lo->ldo_keepalive_probes;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_X_KEEPALIVE_INTERVAL:
		* (int *) outvalue = lo->ldo_keepalive_interval;
		return LDAP_OPT_SUCCESS;

	default:
#ifdef HAVE_TLS
		if ( ldap_pvt_tls_get_option( ld, option, outvalue ) == 0 ) {
			return LDAP_OPT_SUCCESS;
		}
#endif
#ifdef HAVE_CYRUS_SASL
		if ( ldap_int_sasl_get_option( ld, option, outvalue ) == 0 ) {
			return LDAP_OPT_SUCCESS;
		}
#endif
#ifdef HAVE_GSSAPI
		if ( ldap_int_gssapi_get_option( ld, option, outvalue ) == 0 ) {
			return LDAP_OPT_SUCCESS;
		}
#endif
		/* bad param */
		break;
	}

	return LDAP_OPT_ERROR;
}

int
ldap_set_option(
	LDAP	*ld,
	int		option,
	LDAP_CONST void	*invalue)
{
	struct ldapoptions *lo;
	int *dbglvl = NULL;

	/* Get pointer to global option structure */
	lo = LDAP_INT_GLOBAL_OPT();
	if (lo == NULL)	{
		return LDAP_NO_MEMORY;
	}

	/*
	 * The architecture to turn on debugging has a chicken and egg
	 * problem. Thus, we introduce a fix here.
	 */

	if (option == LDAP_OPT_DEBUG_LEVEL) {
		dbglvl = (int *) invalue;
	}

	if( lo->ldo_valid != LDAP_INITIALIZED ) {
		ldap_int_initialize(lo, dbglvl);
	}

	if(ld != NULL) {
		assert( LDAP_VALID( ld ) );

		if( !LDAP_VALID( ld ) ) {
			return LDAP_OPT_ERROR;
		}

		lo = &ld->ld_options;
	}

	switch(option) {
	case LDAP_OPT_REFERRALS:
		if(invalue == LDAP_OPT_OFF) {
			LDAP_BOOL_CLR(lo, LDAP_BOOL_REFERRALS);
		} else {
			LDAP_BOOL_SET(lo, LDAP_BOOL_REFERRALS);
		}
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_RESTART:
		if(invalue == LDAP_OPT_OFF) {
			LDAP_BOOL_CLR(lo, LDAP_BOOL_RESTART);
		} else {
			LDAP_BOOL_SET(lo, LDAP_BOOL_RESTART);
		}
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_CONNECT_ASYNC:
		if(invalue == LDAP_OPT_OFF) {
			LDAP_BOOL_CLR(lo, LDAP_BOOL_CONNECT_ASYNC);
		} else {
			LDAP_BOOL_SET(lo, LDAP_BOOL_CONNECT_ASYNC);
		}
		return LDAP_OPT_SUCCESS;
	}

	/* options which can withstand invalue == NULL */
	switch ( option ) {
	case LDAP_OPT_SERVER_CONTROLS: {
			LDAPControl *const *controls =
				(LDAPControl *const *) invalue;

			if( lo->ldo_sctrls )
				ldap_controls_free( lo->ldo_sctrls );

			if( controls == NULL || *controls == NULL ) {
				lo->ldo_sctrls = NULL;
				return LDAP_OPT_SUCCESS;
			}
				
			lo->ldo_sctrls = ldap_controls_dup( controls );

			if(lo->ldo_sctrls == NULL) {
				/* memory allocation error ? */
				break;
			}
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_CLIENT_CONTROLS: {
			LDAPControl *const *controls =
				(LDAPControl *const *) invalue;

			if( lo->ldo_cctrls )
				ldap_controls_free( lo->ldo_cctrls );

			if( controls == NULL || *controls == NULL ) {
				lo->ldo_cctrls = NULL;
				return LDAP_OPT_SUCCESS;
			}
				
			lo->ldo_cctrls = ldap_controls_dup( controls );

			if(lo->ldo_cctrls == NULL) {
				/* memory allocation error ? */
				break;
			}
		} return LDAP_OPT_SUCCESS;


	case LDAP_OPT_HOST_NAME: {
			const char *host = (const char *) invalue;
			LDAPURLDesc *ludlist = NULL;
			int rc = LDAP_OPT_SUCCESS;

			if(host != NULL) {
				rc = ldap_url_parsehosts( &ludlist, host,
					lo->ldo_defport ? lo->ldo_defport : LDAP_PORT );

			} else if(ld == NULL) {
				/*
				 * must want global default returned
				 * to initial condition.
				 */
				rc = ldap_url_parselist_ext(&ludlist, "ldap://localhost/", NULL,
					LDAP_PVT_URL_PARSE_NOEMPTY_HOST
					| LDAP_PVT_URL_PARSE_DEF_PORT );

			} else {
				/*
				 * must want the session default
				 *   updated to the current global default
				 */
				ludlist = ldap_url_duplist(
					ldap_int_global_options.ldo_defludp);
				if (ludlist == NULL)
					rc = LDAP_NO_MEMORY;
			}

			if (rc == LDAP_OPT_SUCCESS) {
				if (lo->ldo_defludp != NULL)
					ldap_free_urllist(lo->ldo_defludp);
				lo->ldo_defludp = ludlist;
			}
			return rc;
		}

	case LDAP_OPT_URI: {
			const char *urls = (const char *) invalue;
			LDAPURLDesc *ludlist = NULL;
			int rc = LDAP_OPT_SUCCESS;

			if(urls != NULL) {
				rc = ldap_url_parselist_ext(&ludlist, urls, NULL,
					LDAP_PVT_URL_PARSE_NOEMPTY_HOST
					| LDAP_PVT_URL_PARSE_DEF_PORT );
			} else if(ld == NULL) {
				/*
				 * must want global default returned
				 * to initial condition.
				 */
				rc = ldap_url_parselist_ext(&ludlist, "ldap://localhost/", NULL,
					LDAP_PVT_URL_PARSE_NOEMPTY_HOST
					| LDAP_PVT_URL_PARSE_DEF_PORT );

			} else {
				/*
				 * must want the session default
				 *   updated to the current global default
				 */
				ludlist = ldap_url_duplist(
					ldap_int_global_options.ldo_defludp);
				if (ludlist == NULL)
					rc = LDAP_URL_ERR_MEM;
			}

			switch (rc) {
			case LDAP_URL_SUCCESS:		/* Success */
				rc = LDAP_SUCCESS;
				break;

			case LDAP_URL_ERR_MEM:		/* can't allocate memory space */
				rc = LDAP_NO_MEMORY;
				break;

			case LDAP_URL_ERR_PARAM:	/* parameter is bad */
			case LDAP_URL_ERR_BADSCHEME:	/* URL doesn't begin with "ldap[si]://" */
			case LDAP_URL_ERR_BADENCLOSURE:	/* URL is missing trailing ">" */
			case LDAP_URL_ERR_BADURL:	/* URL is bad */
			case LDAP_URL_ERR_BADHOST:	/* host port is bad */
			case LDAP_URL_ERR_BADATTRS:	/* bad (or missing) attributes */
			case LDAP_URL_ERR_BADSCOPE:	/* scope string is invalid (or missing) */
			case LDAP_URL_ERR_BADFILTER:	/* bad or missing filter */
			case LDAP_URL_ERR_BADEXTS:	/* bad or missing extensions */
				rc = LDAP_PARAM_ERROR;
				break;
			}

			if (rc == LDAP_SUCCESS) {
				if (lo->ldo_defludp != NULL)
					ldap_free_urllist(lo->ldo_defludp);
				lo->ldo_defludp = ludlist;
			}
			return rc;
		}

	case LDAP_OPT_DEFBASE: {
			const char *newbase = (const char *) invalue;
			char *defbase = NULL;

			if ( newbase != NULL ) {
				defbase = LDAP_STRDUP( newbase );
				if ( defbase == NULL ) return LDAP_NO_MEMORY;

			} else if ( ld != NULL ) {
				defbase = LDAP_STRDUP( ldap_int_global_options.ldo_defbase );
				if ( defbase == NULL ) return LDAP_NO_MEMORY;
			}
			
			if ( lo->ldo_defbase != NULL )
				LDAP_FREE( lo->ldo_defbase );
			lo->ldo_defbase = defbase;
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_DIAGNOSTIC_MESSAGE: {
			const char *err = (const char *) invalue;

			if(ld == NULL) {
				/* need a struct ldap */
				return LDAP_OPT_ERROR;
			}

			if( ld->ld_error ) {
				LDAP_FREE(ld->ld_error);
				ld->ld_error = NULL;
			}

			if ( err ) {
				ld->ld_error = LDAP_STRDUP(err);
			}
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_MATCHED_DN: {
			const char *matched = (const char *) invalue;

			if (ld == NULL) {
				/* need a struct ldap */
				return LDAP_OPT_ERROR;
			}

			if( ld->ld_matched ) {
				LDAP_FREE(ld->ld_matched);
				ld->ld_matched = NULL;
			}

			if ( matched ) {
				ld->ld_matched = LDAP_STRDUP( matched );
			}
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_REFERRAL_URLS: {
			char *const *referrals = (char *const *) invalue;
			
			if(ld == NULL) {
				/* need a struct ldap */
				return LDAP_OPT_ERROR;
			}

			if( ld->ld_referrals ) {
				LDAP_VFREE(ld->ld_referrals);
			}

			if ( referrals ) {
				ld->ld_referrals = ldap_value_dup(referrals);
			}
		} return LDAP_OPT_SUCCESS;

	/* Only accessed from inside this function by ldap_set_rebind_proc() */
	case LDAP_OPT_REBIND_PROC: {
			lo->ldo_rebind_proc = (LDAP_REBIND_PROC *)invalue;		
		} return LDAP_OPT_SUCCESS;
	case LDAP_OPT_REBIND_PARAMS: {
			lo->ldo_rebind_params = (void *)invalue;		
		} return LDAP_OPT_SUCCESS;

	/* Only accessed from inside this function by ldap_set_nextref_proc() */
	case LDAP_OPT_NEXTREF_PROC: {
			lo->ldo_nextref_proc = (LDAP_NEXTREF_PROC *)invalue;		
		} return LDAP_OPT_SUCCESS;
	case LDAP_OPT_NEXTREF_PARAMS: {
			lo->ldo_nextref_params = (void *)invalue;		
		} return LDAP_OPT_SUCCESS;

	/* Only accessed from inside this function by ldap_set_urllist_proc() */
	case LDAP_OPT_URLLIST_PROC: {
			lo->ldo_urllist_proc = (LDAP_URLLIST_PROC *)invalue;		
		} return LDAP_OPT_SUCCESS;
	case LDAP_OPT_URLLIST_PARAMS: {
			lo->ldo_urllist_params = (void *)invalue;		
		} return LDAP_OPT_SUCCESS;

	/* read-only options */
	case LDAP_OPT_API_INFO:
	case LDAP_OPT_DESC:
	case LDAP_OPT_SOCKBUF:
	case LDAP_OPT_API_FEATURE_INFO:
		return LDAP_OPT_ERROR;

	/* options which cannot withstand invalue == NULL */
	case LDAP_OPT_DEREF:
	case LDAP_OPT_SIZELIMIT:
	case LDAP_OPT_TIMELIMIT:
	case LDAP_OPT_PROTOCOL_VERSION:
	case LDAP_OPT_RESULT_CODE:
	case LDAP_OPT_DEBUG_LEVEL:
	case LDAP_OPT_TIMEOUT:
	case LDAP_OPT_NETWORK_TIMEOUT:
	case LDAP_OPT_CONNECT_CB:
	case LDAP_OPT_X_KEEPALIVE_IDLE:
	case LDAP_OPT_X_KEEPALIVE_PROBES :
	case LDAP_OPT_X_KEEPALIVE_INTERVAL :
		if(invalue == NULL) {
			/* no place to set from */
			return LDAP_OPT_ERROR;
		}
		break;

	default:
#ifdef HAVE_TLS
		if ( ldap_pvt_tls_set_option( ld, option, (void *)invalue ) == 0 )
			return LDAP_OPT_SUCCESS;
#endif
#ifdef HAVE_CYRUS_SASL
		if ( ldap_int_sasl_set_option( ld, option, (void *)invalue ) == 0 )
			return LDAP_OPT_SUCCESS;
#endif
#ifdef HAVE_GSSAPI
		if ( ldap_int_gssapi_set_option( ld, option, (void *)invalue ) == 0 )
			return LDAP_OPT_SUCCESS;
#endif
		/* bad param */
		return LDAP_OPT_ERROR;
	}

	/* options which cannot withstand invalue == NULL */

	switch(option) {
	case LDAP_OPT_DEREF:
		/* FIXME: check value for protocol compliance? */
		lo->ldo_deref = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_SIZELIMIT:
		/* FIXME: check value for protocol compliance? */
		lo->ldo_sizelimit = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_TIMELIMIT:
		/* FIXME: check value for protocol compliance? */
		lo->ldo_timelimit = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_TIMEOUT: {
			const struct timeval *tv = 
				(const struct timeval *) invalue;

			lo->ldo_tm_api = *tv;
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_NETWORK_TIMEOUT: {
			const struct timeval *tv = 
				(const struct timeval *) invalue;

			lo->ldo_tm_net = *tv;
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_PROTOCOL_VERSION: {
			int vers = * (const int *) invalue;
			if (vers < LDAP_VERSION_MIN || vers > LDAP_VERSION_MAX) {
				/* not supported */
				break;
			}
			lo->ldo_version = vers;
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_RESULT_CODE: {
			int err = * (const int *) invalue;

			if(ld == NULL) {
				/* need a struct ldap */
				break;
			}

			ld->ld_errno = err;
		} return LDAP_OPT_SUCCESS;

	case LDAP_OPT_DEBUG_LEVEL:
		lo->ldo_debug = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;

	case LDAP_OPT_CONNECT_CB:
		{
			/* setting pushes the callback */
			ldaplist *ll;
			ll = LDAP_MALLOC( sizeof( *ll ));
			ll->ll_data = (void *)invalue;
			ll->ll_next = lo->ldo_conn_cbs;
			lo->ldo_conn_cbs = ll;
		}
		return LDAP_OPT_SUCCESS;
	case LDAP_OPT_X_KEEPALIVE_IDLE:
		lo->ldo_keepalive_idle = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;
	case LDAP_OPT_X_KEEPALIVE_PROBES :
		lo->ldo_keepalive_probes = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;
	case LDAP_OPT_X_KEEPALIVE_INTERVAL :
		lo->ldo_keepalive_interval = * (const int *) invalue;
		return LDAP_OPT_SUCCESS;
	
	}
	return LDAP_OPT_ERROR;
}

int
ldap_set_rebind_proc( LDAP *ld, LDAP_REBIND_PROC *proc, void *params )
{
	int rc;
	rc = ldap_set_option( ld, LDAP_OPT_REBIND_PROC, (void *)proc );
	if( rc != LDAP_OPT_SUCCESS ) return rc;

	rc = ldap_set_option( ld, LDAP_OPT_REBIND_PARAMS, (void *)params );
	return rc;
}

int
ldap_set_nextref_proc( LDAP *ld, LDAP_NEXTREF_PROC *proc, void *params )
{
	int rc;
	rc = ldap_set_option( ld, LDAP_OPT_NEXTREF_PROC, (void *)proc );
	if( rc != LDAP_OPT_SUCCESS ) return rc;

	rc = ldap_set_option( ld, LDAP_OPT_NEXTREF_PARAMS, (void *)params );
	return rc;
}

int
ldap_set_urllist_proc( LDAP *ld, LDAP_URLLIST_PROC *proc, void *params )
{
	int rc;
	rc = ldap_set_option( ld, LDAP_OPT_URLLIST_PROC, (void *)proc );
	if( rc != LDAP_OPT_SUCCESS ) return rc;

	rc = ldap_set_option( ld, LDAP_OPT_URLLIST_PARAMS, (void *)params );
	return rc;
}
