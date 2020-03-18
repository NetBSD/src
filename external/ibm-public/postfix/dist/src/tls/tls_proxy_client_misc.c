/*	$NetBSD: tls_proxy_client_misc.c,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

/*++
/* NAME
/*	tls_proxy_client_misc 3
/* SUMMARY
/*	TLS_CLIENT_XXX structure support
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	TLS_CLIENT_PARAMS *tls_proxy_client_param_from_config(params)
/*	TLS_CLIENT_PARAMS *params;
/*
/*	char	*tls_proxy_client_param_to_string(buf, params)
/*	VSTRING *buf;
/*	TLS_CLIENT_PARAMS *params;
/*
/*	char	*tls_proxy_client_param_with_names_to_string(buf, params)
/*	VSTRING *buf;
/*	TLS_CLIENT_PARAMS *params;
/*
/*	char	*tls_proxy_client_init_to_string(buf, init_props)
/*	VSTRING *buf;
/*	TLS_CLIENT_INIT_PROPS *init_props;
/* DESCRIPTION
/*	tls_proxy_client_param_from_config() initializes a TLS_CLIENT_PARAMS
/*	structure from configuration parameters and returns its
/*	argument. Strings are not copied. The result must therefore
/*	not be passed to tls_proxy_client_param_free().
/*
/*	tls_proxy_client_param_to_string() produces a lookup key
/*	that is unique for the TLS_CLIENT_PARAMS member values.
/*
/*	tls_proxy_client_param_with_names_to_string() produces a
/*	string with "name = value\n" for each TLS_CLIENT_PARAMS
/*	member. This may be useful for reporting differences between
/*	TLS_CLIENT_PARAMS instances.
/*
/*	tls_proxy_client_init_to_string() produces a lookup key
/*	that is unique for the properties received by
/*	tls_proxy_client_init_scan().
/*
/*	tls_proxy_client_init_with_names_to_string() produces a
/*	string with "name = value\n" for each TLS_CLIENT_INIT_PROPS
/*	member. This may be useful for reporting differences between
/*	TLS_CLIENT_INIT_PROPS instances.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#ifdef USE_TLS

/* System library. */

#include <sys_defs.h>

/* Utility library */

#include <attr.h>
#include <msg.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_client_param_from_config - initialize TLS_CLIENT_PARAMS from configuration */

TLS_CLIENT_PARAMS *tls_proxy_client_param_from_config(TLS_CLIENT_PARAMS *params)
{
    TLS_PROXY_PARAMS(params,
		     tls_high_clist = var_tls_high_clist,
		     tls_medium_clist = var_tls_medium_clist,
		     tls_low_clist = var_tls_low_clist,
		     tls_export_clist = var_tls_export_clist,
		     tls_null_clist = var_tls_null_clist,
		     tls_eecdh_auto = var_tls_eecdh_auto,
		     tls_eecdh_strong = var_tls_eecdh_strong,
		     tls_eecdh_ultra = var_tls_eecdh_ultra,
		     tls_bug_tweaks = var_tls_bug_tweaks,
		     tls_ssl_options = var_tls_ssl_options,
		     tls_dane_digests = var_tls_dane_digests,
		     tls_mgr_service = var_tls_mgr_service,
		     tls_tkt_cipher = var_tls_tkt_cipher,
		     tls_daemon_rand_bytes = var_tls_daemon_rand_bytes,
		     tls_append_def_CA = var_tls_append_def_CA,
		     tls_bc_pkey_fprint = var_tls_bc_pkey_fprint,
		     tls_preempt_clist = var_tls_preempt_clist,
		     tls_multi_wildcard = var_tls_multi_wildcard);
    return (params);
}

/* tls_proxy_client_param_to_string - serialize TLS_CLIENT_PARAMS to string */

char   *tls_proxy_client_param_to_string(VSTRING *buf, TLS_CLIENT_PARAMS *params)
{
    vstring_sprintf(buf, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n"
		    "%s\n%s\n%d\n%d\n%d\n%d\n%d\n",
		    params->tls_high_clist, params->tls_medium_clist,
		    params->tls_low_clist, params->tls_export_clist,
		    params->tls_null_clist, params->tls_eecdh_auto,
		    params->tls_eecdh_strong, params->tls_eecdh_ultra,
		    params->tls_bug_tweaks, params->tls_ssl_options,
		    params->tls_dane_digests, params->tls_mgr_service,
		    params->tls_tkt_cipher, params->tls_daemon_rand_bytes,
		    params->tls_append_def_CA, params->tls_bc_pkey_fprint,
		    params->tls_preempt_clist, params->tls_multi_wildcard);
    return (vstring_str(buf));
}

/* tls_proxy_client_param_with_names_to_string - serialize TLS_CLIENT_PARAMS to string */

char   *tls_proxy_client_param_with_names_to_string(VSTRING *buf, TLS_CLIENT_PARAMS *params)
{
    vstring_sprintf(buf, "%s = %s\n%s = %s\n%s = %s\n%s = %s\n%s = %s\n"
		    "%s = %s\n%s = %s\n%s = %s\n%s = %s\n%s = %s\n%s = %s\n"
		    "%s = %s\n%s = %s\n%s = %d\n"
		    "%s = %d\n%s = %d\n%s = %d\n%s = %d\n",
		    VAR_TLS_HIGH_CLIST, params->tls_high_clist,
		    VAR_TLS_MEDIUM_CLIST, params->tls_medium_clist,
		    VAR_TLS_LOW_CLIST, params->tls_low_clist,
		    VAR_TLS_EXPORT_CLIST, params->tls_export_clist,
		    VAR_TLS_NULL_CLIST, params->tls_null_clist,
		    VAR_TLS_EECDH_AUTO, params->tls_eecdh_auto,
		    VAR_TLS_EECDH_STRONG, params->tls_eecdh_strong,
		    VAR_TLS_EECDH_ULTRA, params->tls_eecdh_ultra,
		    VAR_TLS_BUG_TWEAKS, params->tls_bug_tweaks,
		    VAR_TLS_SSL_OPTIONS, params->tls_ssl_options,
		    VAR_TLS_DANE_DIGESTS, params->tls_dane_digests,
		    VAR_TLS_MGR_SERVICE, params->tls_mgr_service,
		    VAR_TLS_TKT_CIPHER, params->tls_tkt_cipher,
		    VAR_TLS_DAEMON_RAND_BYTES, params->tls_daemon_rand_bytes,
		    VAR_TLS_APPEND_DEF_CA, params->tls_append_def_CA,
		    VAR_TLS_BC_PKEY_FPRINT, params->tls_bc_pkey_fprint,
		    VAR_TLS_PREEMPT_CLIST, params->tls_preempt_clist,
		    VAR_TLS_MULTI_WILDCARD, params->tls_multi_wildcard);
    return (vstring_str(buf));
}

/* tls_proxy_client_init_to_string - serialize to string */

char   *tls_proxy_client_init_to_string(VSTRING *buf,
					        TLS_CLIENT_INIT_PROPS *props)
{
    vstring_sprintf(buf, "%s\n%s\n%d\n%s\n%s\n%s\n%s\n%s\n%s\n"
		    "%s\n%s\n%s\n%s\n%s\n", props->log_param,
		    props->log_level, props->verifydepth,
		    props->cache_type, props->chain_files,
		    props->cert_file, props->key_file,
		    props->dcert_file, props->dkey_file,
		    props->eccert_file, props->eckey_file,
		    props->CAfile, props->CApath, props->mdalg);
    return (vstring_str(buf));
}

/* tls_proxy_client_init_with_names_to_string - serialize to string */

char   *tls_proxy_client_init_with_names_to_string(VSTRING *buf,
					        TLS_CLIENT_INIT_PROPS *props)
{
    vstring_sprintf(buf, "%s = %s\n%s = %s\n%s = %d\n%s = %s\n%s = %s\n"
		    "%s = %s\n%s = %s\n%s = %s\n%s = %s\n%s = %s\n"
		    "%s = %s\n%s = %s\n%s = %s\n%s = %s\n",
		    TLS_ATTR_LOG_PARAM, props->log_param,
		    TLS_ATTR_LOG_LEVEL, props->log_level,
		    TLS_ATTR_VERIFYDEPTH, props->verifydepth,
		    TLS_ATTR_CACHE_TYPE, props->cache_type,
		    TLS_ATTR_CHAIN_FILES, props->chain_files,
		    TLS_ATTR_CERT_FILE, props->cert_file,
		    TLS_ATTR_KEY_FILE, props->key_file,
		    TLS_ATTR_DCERT_FILE, props->dcert_file,
		    TLS_ATTR_DKEY_FILE, props->dkey_file,
		    TLS_ATTR_ECCERT_FILE, props->eccert_file,
		    TLS_ATTR_ECKEY_FILE, props->eckey_file,
		    TLS_ATTR_CAFILE, props->CAfile,
		    TLS_ATTR_CAPATH, props->CApath,
		    TLS_ATTR_MDALG, props->mdalg);
    return (vstring_str(buf));
}

#endif
