/*	$NetBSD: tls_proxy_server_scan.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	tls_proxy_server_scan 3
/* SUMMARY
/*	read TLS_SERVER_XXX structures from stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int	tls_proxy_server_init_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/*
/*	tls_proxy_server_init_free(init_props)
/*	TLS_SERVER_INIT_PROPS *init_props;
/*
/*	int	tls_proxy_server_start_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/*
/*	void	tls_proxy_server_start_free(start_props)
/*	TLS_SERVER_START_PROPS *start_props;
/* DESCRIPTION
/*	tls_proxy_server_init_scan() reads a TLS_SERVER_INIT_PROPS
/*	structure from the named stream using the specified attribute
/*	scan routine. tls_proxy_server_init_scan() is meant to be passed
/*	as a call-back function to attr_scan(), as shown below.
/*
/*	tls_proxy_server_init_free() destroys a TLS_SERVER_INIT_PROPS
/*	structure that was created by tls_proxy_server_init_scan().
/*
/*	TLS_SERVER_INIT_PROPS *init_props = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_server_init_scan, (void *) &init_props)
/*	...
/*	if (init_props)
/*	    tls_proxy_client_init_free(init_props);
/*
/*	tls_proxy_server_start_scan() reads a TLS_SERVER_START_PROPS
/*	structure from the named stream using the specified attribute
/*	scan routine. tls_proxy_server_start_scan() is meant to be passed
/*	as a call-back function to attr_scan(), as shown below.
/*
/*	tls_proxy_server_start_free() destroys a TLS_SERVER_START_PROPS
/*	structure that was created by tls_proxy_server_start_scan().
/*
/*	TLS_SERVER_START_PROPS *start_props = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_server_start_scan, (void *) &start_props)
/*	...
/*	if (start_props)
/*	    tls_proxy_server_start_free(start_props);
/* DIAGNOSTICS
/*	Fatal: out of memory.
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

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_server_init_scan - receive TLS_SERVER_INIT_PROPS from stream */

int     tls_proxy_server_init_scan(ATTR_SCAN_COMMON_FN scan_fn, VSTREAM *fp,
				           int flags, void *ptr)
{
    TLS_SERVER_INIT_PROPS *props
    = (TLS_SERVER_INIT_PROPS *) mymalloc(sizeof(*props));
    int     ret;
    VSTRING *log_param = vstring_alloc(25);
    VSTRING *log_level = vstring_alloc(25);
    VSTRING *cache_type = vstring_alloc(25);
    VSTRING *chain_files = vstring_alloc(25);
    VSTRING *cert_file = vstring_alloc(25);
    VSTRING *key_file = vstring_alloc(25);
    VSTRING *dcert_file = vstring_alloc(25);
    VSTRING *dkey_file = vstring_alloc(25);
    VSTRING *eccert_file = vstring_alloc(25);
    VSTRING *eckey_file = vstring_alloc(25);
    VSTRING *CAfile = vstring_alloc(25);
    VSTRING *CApath = vstring_alloc(25);
    VSTRING *protocols = vstring_alloc(25);
    VSTRING *eecdh_grade = vstring_alloc(25);
    VSTRING *dh1024_param_file = vstring_alloc(25);
    VSTRING *dh512_param_file = vstring_alloc(25);
    VSTRING *mdalg = vstring_alloc(25);

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(props, 0, sizeof(*props));
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_STR(TLS_ATTR_LOG_PARAM, log_param),
		  RECV_ATTR_STR(TLS_ATTR_LOG_LEVEL, log_level),
		  RECV_ATTR_INT(TLS_ATTR_VERIFYDEPTH, &props->verifydepth),
		  RECV_ATTR_STR(TLS_ATTR_CACHE_TYPE, cache_type),
		  RECV_ATTR_INT(TLS_ATTR_SET_SESSID, &props->set_sessid),
		  RECV_ATTR_STR(TLS_ATTR_CHAIN_FILES, chain_files),
		  RECV_ATTR_STR(TLS_ATTR_CERT_FILE, cert_file),
		  RECV_ATTR_STR(TLS_ATTR_KEY_FILE, key_file),
		  RECV_ATTR_STR(TLS_ATTR_DCERT_FILE, dcert_file),
		  RECV_ATTR_STR(TLS_ATTR_DKEY_FILE, dkey_file),
		  RECV_ATTR_STR(TLS_ATTR_ECCERT_FILE, eccert_file),
		  RECV_ATTR_STR(TLS_ATTR_ECKEY_FILE, eckey_file),
		  RECV_ATTR_STR(TLS_ATTR_CAFILE, CAfile),
		  RECV_ATTR_STR(TLS_ATTR_CAPATH, CApath),
		  RECV_ATTR_STR(TLS_ATTR_PROTOCOLS, protocols),
		  RECV_ATTR_STR(TLS_ATTR_EECDH_GRADE, eecdh_grade),
		  RECV_ATTR_STR(TLS_ATTR_DH1K_PARAM_FILE, dh1024_param_file),
		  RECV_ATTR_STR(TLS_ATTR_DH512_PARAM_FILE, dh512_param_file),
		  RECV_ATTR_INT(TLS_ATTR_ASK_CCERT, &props->ask_ccert),
		  RECV_ATTR_STR(TLS_ATTR_MDALG, mdalg),
		  ATTR_TYPE_END);
    /* Always construct a well-formed structure. */
    props->log_param = vstring_export(log_param);
    props->log_level = vstring_export(log_level);
    props->cache_type = vstring_export(cache_type);
    props->chain_files = vstring_export(chain_files);
    props->cert_file = vstring_export(cert_file);
    props->key_file = vstring_export(key_file);
    props->dcert_file = vstring_export(dcert_file);
    props->dkey_file = vstring_export(dkey_file);
    props->eccert_file = vstring_export(eccert_file);
    props->eckey_file = vstring_export(eckey_file);
    props->CAfile = vstring_export(CAfile);
    props->CApath = vstring_export(CApath);
    props->protocols = vstring_export(protocols);
    props->eecdh_grade = vstring_export(eecdh_grade);
    props->dh1024_param_file = vstring_export(dh1024_param_file);
    props->dh512_param_file = vstring_export(dh512_param_file);
    props->mdalg = vstring_export(mdalg);
    ret = (ret == 20 ? 1 : -1);
    if (ret != 1) {
	tls_proxy_server_init_free(props);
	props = 0;
    }
    *(TLS_SERVER_INIT_PROPS **) ptr = props;
    return (ret);
}

/* tls_proxy_server_init_free - destroy TLS_SERVER_INIT_PROPS structure */

void    tls_proxy_server_init_free(TLS_SERVER_INIT_PROPS *props)
{
    myfree((void *) props->log_param);
    myfree((void *) props->log_level);
    myfree((void *) props->cache_type);
    myfree((void *) props->chain_files);
    myfree((void *) props->cert_file);
    myfree((void *) props->key_file);
    myfree((void *) props->dcert_file);
    myfree((void *) props->dkey_file);
    myfree((void *) props->eccert_file);
    myfree((void *) props->eckey_file);
    myfree((void *) props->CAfile);
    myfree((void *) props->CApath);
    myfree((void *) props->protocols);
    myfree((void *) props->eecdh_grade);
    myfree((void *) props->dh1024_param_file);
    myfree((void *) props->dh512_param_file);
    myfree((void *) props->mdalg);
    myfree((void *) props);
}

/* tls_proxy_server_start_scan - receive TLS_SERVER_START_PROPS from stream */

int     tls_proxy_server_start_scan(ATTR_SCAN_COMMON_FN scan_fn, VSTREAM *fp,
				            int flags, void *ptr)
{
    TLS_SERVER_START_PROPS *props
    = (TLS_SERVER_START_PROPS *) mymalloc(sizeof(*props));
    int     ret;
    VSTRING *serverid = vstring_alloc(25);
    VSTRING *namaddr = vstring_alloc(25);
    VSTRING *cipher_grade = vstring_alloc(25);
    VSTRING *cipher_exclusions = vstring_alloc(25);
    VSTRING *mdalg = vstring_alloc(25);

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(props, 0, sizeof(*props));
    props->ctx = 0;
    props->stream = 0;
    /* XXX Caller sets fd. */
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_INT(TLS_ATTR_TIMEOUT, &props->timeout),
		  RECV_ATTR_INT(TLS_ATTR_REQUIRECERT, &props->requirecert),
		  RECV_ATTR_STR(TLS_ATTR_SERVERID, serverid),
		  RECV_ATTR_STR(TLS_ATTR_NAMADDR, namaddr),
		  RECV_ATTR_STR(TLS_ATTR_CIPHER_GRADE, cipher_grade),
		  RECV_ATTR_STR(TLS_ATTR_CIPHER_EXCLUSIONS,
				cipher_exclusions),
		  RECV_ATTR_STR(TLS_ATTR_MDALG, mdalg),
		  ATTR_TYPE_END);
    props->serverid = vstring_export(serverid);
    props->namaddr = vstring_export(namaddr);
    props->cipher_grade = vstring_export(cipher_grade);
    props->cipher_exclusions = vstring_export(cipher_exclusions);
    props->mdalg = vstring_export(mdalg);
    ret = (ret == 7 ? 1 : -1);
    if (ret != 1) {
	tls_proxy_server_start_free(props);
	props = 0;
    }
    *(TLS_SERVER_START_PROPS **) ptr = props;
    return (ret);
}

/* tls_proxy_server_start_free - destroy TLS_SERVER_START_PROPS structure */

void    tls_proxy_server_start_free(TLS_SERVER_START_PROPS *props)
{
    /* XXX Caller closes fd. */
    myfree((void *) props->serverid);
    myfree((void *) props->namaddr);
    myfree((void *) props->cipher_grade);
    myfree((void *) props->cipher_exclusions);
    myfree((void *) props->mdalg);
    myfree((void *) props);
}

#endif
