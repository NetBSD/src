/*	$NetBSD: tls_proxy_client_misc.c,v 1.3.2.1 2023/12/25 12:43:36 martin Exp $	*/

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
/*	char	*tls_proxy_client_param_serialize(print_fn, buf, params)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTRING *buf;
/*	const TLS_CLIENT_PARAMS *params;
/*
/*	char	*tls_proxy_client_init_serialize(print_fn, buf, init_props)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTRING *buf;
/*	const TLS_CLIENT_INIT_PROPS *init_props;
/* DESCRIPTION
/*	tls_proxy_client_param_from_config() initializes a TLS_CLIENT_PARAMS
/*	structure from configuration parameters and returns its
/*	argument. Strings are not copied. The result must therefore
/*	not be passed to tls_proxy_client_param_free().
/*
/*	tls_proxy_client_param_serialize() and
/*	tls_proxy_client_init_serialize() serialize the specified
/*	object to a memory buffer, using the specified print function
/*	(typically, attr_print_plain). The result can be used
/*	determine whether there are any differences between instances
/*	of the same object type.
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
		     tls_cnf_file = var_tls_cnf_file,
		     tls_cnf_name = var_tls_cnf_name,
		     tls_high_clist = var_tls_high_clist,
		     tls_medium_clist = var_tls_medium_clist,
		     tls_null_clist = var_tls_null_clist,
		     tls_eecdh_auto = var_tls_eecdh_auto,
		     tls_eecdh_strong = var_tls_eecdh_strong,
		     tls_eecdh_ultra = var_tls_eecdh_ultra,
		     tls_ffdhe_auto = var_tls_ffdhe_auto,
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

/* tls_proxy_client_param_serialize - serialize TLS_CLIENT_PARAMS to string */

char   *tls_proxy_client_param_serialize(ATTR_PRINT_COMMON_FN print_fn,
					         VSTRING *buf,
				            const TLS_CLIENT_PARAMS *params)
{
    const char myname[] = "tls_proxy_client_param_serialize";
    VSTREAM *mp;

    if ((mp = vstream_memopen(buf, O_WRONLY)) == 0
	|| print_fn(mp, ATTR_FLAG_NONE,
		    SEND_ATTR_FUNC(tls_proxy_client_param_print,
				   (const void *) params),
		    ATTR_TYPE_END) != 0
	|| vstream_fclose(mp) != 0)
	msg_fatal("%s: can't serialize properties: %m", myname);
    return (vstring_str(buf));
}

/* tls_proxy_client_init_serialize - serialize to string */

char   *tls_proxy_client_init_serialize(ATTR_PRINT_COMMON_FN print_fn,
					        VSTRING *buf,
				         const TLS_CLIENT_INIT_PROPS *props)
{
    const char myname[] = "tls_proxy_client_init_serialize";
    VSTREAM *mp;

    if ((mp = vstream_memopen(buf, O_WRONLY)) == 0
	|| print_fn(mp, ATTR_FLAG_NONE,
		    SEND_ATTR_FUNC(tls_proxy_client_init_print,
				   (const void *) props),
		    ATTR_TYPE_END) != 0
	|| vstream_fclose(mp) != 0)
	msg_fatal("%s: can't serialize properties: %m", myname);
    return (vstring_str(buf));
}

#endif
