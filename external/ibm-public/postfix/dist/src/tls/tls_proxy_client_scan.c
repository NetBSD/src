/*	$NetBSD: tls_proxy_client_scan.c,v 1.3.2.1 2023/12/25 12:43:36 martin Exp $	*/

/*++
/* NAME
/*	tls_proxy_client_scan 3
/* SUMMARY
/*	read TLS_CLIENT_XXX structures from stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int	tls_proxy_client_param_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM	*stream;
/*	int	flags;
/*	void	*ptr;
/*
/*	void	tls_proxy_client_param_free(params)
/*	TLS_CLIENT_PARAMS *params;
/*
/*	int	tls_proxy_client_init_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM	*stream;
/*	int	flags;
/*	void	*ptr;
/*
/*	void	tls_proxy_client_init_free(init_props)
/*	TLS_CLIENT_INIT_PROPS *init_props;
/*
/*	int	tls_proxy_client_start_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM	*stream;
/*	int	flags;
/*	void	*ptr;
/*
/*	void	tls_proxy_client_start_free(start_props)
/*	TLS_CLIENT_START_PROPS *start_props;
/* DESCRIPTION
/*	tls_proxy_client_param_scan() reads a TLS_CLIENT_PARAMS structure from
/*	the named stream using the specified attribute scan routine.
/*	tls_proxy_client_param_scan() is meant to be passed as a call-back
/*	function to attr_scan(), as shown below.
/*
/*	tls_proxy_client_param_free() destroys a TLS_CLIENT_PARAMS structure
/*	that was created by tls_proxy_client_param_scan().
/*
/*	TLS_CLIENT_PARAMS *param = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_client_param_scan, (void *) &param)
/*	...
/*	if (param != 0)
/*	    tls_proxy_client_param_free(param);
/*
/*	tls_proxy_client_init_scan() reads a full TLS_CLIENT_INIT_PROPS
/*	structure from the named stream using the specified attribute
/*	scan routine. tls_proxy_client_init_scan() is meant to be passed
/*	as a call-back function to attr_scan(), as shown below.
/*
/*	tls_proxy_client_init_free() destroys a TLS_CLIENT_INIT_PROPS
/*	structure that was created by tls_proxy_client_init_scan().
/*
/*	TLS_CLIENT_INIT_PROPS *init_props = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_client_init_scan, (void *) &init_props)
/*	...
/*	if (init_props != 0)
/*	    tls_proxy_client_init_free(init_props);
/*
/*	tls_proxy_client_start_scan() reads a TLS_CLIENT_START_PROPS
/*	structure, without the stream of file descriptor members,
/*	from the named stream using the specified attribute scan
/*	routine. tls_proxy_client_start_scan() is meant to be passed
/*	as a call-back function to attr_scan(), as shown below.
/*
/*	tls_proxy_client_start_free() destroys a TLS_CLIENT_START_PROPS
/*	structure that was created by tls_proxy_client_start_scan().
/*
/*	TLS_CLIENT_START_PROPS *start_props = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_client_start_scan, (void *) &start_props)
/*	...
/*	if (start_props != 0)
/*	    tls_proxy_client_start_free(start_props);
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

#include <argv_attr.h>
#include <attr.h>
#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>
#include <tls_proxy.h>

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* tls_proxy_client_param_free - destroy TLS_CLIENT_PARAMS structure */

void    tls_proxy_client_param_free(TLS_CLIENT_PARAMS *params)
{
    myfree(params->tls_cnf_file);
    myfree(params->tls_cnf_name);
    myfree(params->tls_high_clist);
    myfree(params->tls_medium_clist);
    myfree(params->tls_null_clist);
    myfree(params->tls_eecdh_auto);
    myfree(params->tls_eecdh_strong);
    myfree(params->tls_eecdh_ultra);
    myfree(params->tls_ffdhe_auto);
    myfree(params->tls_bug_tweaks);
    myfree(params->tls_ssl_options);
    myfree(params->tls_dane_digests);
    myfree(params->tls_mgr_service);
    myfree(params->tls_tkt_cipher);
    myfree((void *) params);
}

/* tls_proxy_client_param_scan - receive TLS_CLIENT_PARAMS from stream */

int     tls_proxy_client_param_scan(ATTR_SCAN_COMMON_FN scan_fn, VSTREAM *fp,
				            int flags, void *ptr)
{
    TLS_CLIENT_PARAMS *params
    = (TLS_CLIENT_PARAMS *) mymalloc(sizeof(*params));
    int     ret;
    VSTRING *cnf_file = vstring_alloc(25);
    VSTRING *cnf_name = vstring_alloc(25);
    VSTRING *tls_high_clist = vstring_alloc(25);
    VSTRING *tls_medium_clist = vstring_alloc(25);
    VSTRING *tls_null_clist = vstring_alloc(25);
    VSTRING *tls_eecdh_auto = vstring_alloc(25);
    VSTRING *tls_eecdh_strong = vstring_alloc(25);
    VSTRING *tls_eecdh_ultra = vstring_alloc(25);
    VSTRING *tls_ffdhe_auto = vstring_alloc(25);
    VSTRING *tls_bug_tweaks = vstring_alloc(25);
    VSTRING *tls_ssl_options = vstring_alloc(25);
    VSTRING *tls_dane_digests = vstring_alloc(25);
    VSTRING *tls_mgr_service = vstring_alloc(25);
    VSTRING *tls_tkt_cipher = vstring_alloc(25);

    if (msg_verbose)
	msg_info("begin tls_proxy_client_param_scan");

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(params, 0, sizeof(*params));
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_STR(TLS_ATTR_CNF_FILE, cnf_file),
		  RECV_ATTR_STR(TLS_ATTR_CNF_NAME, cnf_name),
		  RECV_ATTR_STR(VAR_TLS_HIGH_CLIST, tls_high_clist),
		  RECV_ATTR_STR(VAR_TLS_MEDIUM_CLIST, tls_medium_clist),
		  RECV_ATTR_STR(VAR_TLS_NULL_CLIST, tls_null_clist),
		  RECV_ATTR_STR(VAR_TLS_EECDH_AUTO, tls_eecdh_auto),
		  RECV_ATTR_STR(VAR_TLS_EECDH_STRONG, tls_eecdh_strong),
		  RECV_ATTR_STR(VAR_TLS_EECDH_ULTRA, tls_eecdh_ultra),
		  RECV_ATTR_STR(VAR_TLS_FFDHE_AUTO, tls_ffdhe_auto),
		  RECV_ATTR_STR(VAR_TLS_BUG_TWEAKS, tls_bug_tweaks),
		  RECV_ATTR_STR(VAR_TLS_SSL_OPTIONS, tls_ssl_options),
		  RECV_ATTR_STR(VAR_TLS_DANE_DIGESTS, tls_dane_digests),
		  RECV_ATTR_STR(VAR_TLS_MGR_SERVICE, tls_mgr_service),
		  RECV_ATTR_STR(VAR_TLS_TKT_CIPHER, tls_tkt_cipher),
		  RECV_ATTR_INT(VAR_TLS_DAEMON_RAND_BYTES,
				&params->tls_daemon_rand_bytes),
		  RECV_ATTR_INT(VAR_TLS_APPEND_DEF_CA,
				&params->tls_append_def_CA),
		  RECV_ATTR_INT(VAR_TLS_BC_PKEY_FPRINT,
				&params->tls_bc_pkey_fprint),
		  RECV_ATTR_INT(VAR_TLS_PREEMPT_CLIST,
				&params->tls_preempt_clist),
		  RECV_ATTR_INT(VAR_TLS_MULTI_WILDCARD,
				&params->tls_multi_wildcard),
		  ATTR_TYPE_END);
    /* Always construct a well-formed structure. */
    params->tls_cnf_file = vstring_export(cnf_file);
    params->tls_cnf_name = vstring_export(cnf_name);
    params->tls_high_clist = vstring_export(tls_high_clist);
    params->tls_medium_clist = vstring_export(tls_medium_clist);
    params->tls_null_clist = vstring_export(tls_null_clist);
    params->tls_eecdh_auto = vstring_export(tls_eecdh_auto);
    params->tls_eecdh_strong = vstring_export(tls_eecdh_strong);
    params->tls_eecdh_ultra = vstring_export(tls_eecdh_ultra);
    params->tls_ffdhe_auto = vstring_export(tls_ffdhe_auto);
    params->tls_bug_tweaks = vstring_export(tls_bug_tweaks);
    params->tls_ssl_options = vstring_export(tls_ssl_options);
    params->tls_dane_digests = vstring_export(tls_dane_digests);
    params->tls_mgr_service = vstring_export(tls_mgr_service);
    params->tls_tkt_cipher = vstring_export(tls_tkt_cipher);

    ret = (ret == 19 ? 1 : -1);
    if (ret != 1) {
	tls_proxy_client_param_free(params);
	params = 0;
    }
    *(TLS_CLIENT_PARAMS **) ptr = params;
    if (msg_verbose)
	msg_info("tls_proxy_client_param_scan ret=%d", ret);
    return (ret);
}

/* tls_proxy_client_init_free - destroy TLS_CLIENT_INIT_PROPS structure */

void    tls_proxy_client_init_free(TLS_CLIENT_INIT_PROPS *props)
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
    myfree((void *) props->mdalg);
    myfree((void *) props);
}

/* tls_proxy_client_init_scan - receive TLS_CLIENT_INIT_PROPS from stream */

int     tls_proxy_client_init_scan(ATTR_SCAN_COMMON_FN scan_fn, VSTREAM *fp,
				           int flags, void *ptr)
{
    TLS_CLIENT_INIT_PROPS *props
    = (TLS_CLIENT_INIT_PROPS *) mymalloc(sizeof(*props));
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
    VSTRING *mdalg = vstring_alloc(25);

    if (msg_verbose)
	msg_info("begin tls_proxy_client_init_scan");

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(props, 0, sizeof(*props));
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_STR(TLS_ATTR_LOG_PARAM, log_param),
		  RECV_ATTR_STR(TLS_ATTR_LOG_LEVEL, log_level),
		  RECV_ATTR_INT(TLS_ATTR_VERIFYDEPTH, &props->verifydepth),
		  RECV_ATTR_STR(TLS_ATTR_CACHE_TYPE, cache_type),
		  RECV_ATTR_STR(TLS_ATTR_CHAIN_FILES, chain_files),
		  RECV_ATTR_STR(TLS_ATTR_CERT_FILE, cert_file),
		  RECV_ATTR_STR(TLS_ATTR_KEY_FILE, key_file),
		  RECV_ATTR_STR(TLS_ATTR_DCERT_FILE, dcert_file),
		  RECV_ATTR_STR(TLS_ATTR_DKEY_FILE, dkey_file),
		  RECV_ATTR_STR(TLS_ATTR_ECCERT_FILE, eccert_file),
		  RECV_ATTR_STR(TLS_ATTR_ECKEY_FILE, eckey_file),
		  RECV_ATTR_STR(TLS_ATTR_CAFILE, CAfile),
		  RECV_ATTR_STR(TLS_ATTR_CAPATH, CApath),
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
    props->mdalg = vstring_export(mdalg);
    ret = (ret == 14 ? 1 : -1);
    if (ret != 1) {
	tls_proxy_client_init_free(props);
	props = 0;
    }
    *(TLS_CLIENT_INIT_PROPS **) ptr = props;
    if (msg_verbose)
	msg_info("tls_proxy_client_init_scan ret=%d", ret);
    return (ret);
}

/* tls_proxy_client_start_free - destroy TLS_CLIENT_START_PROPS structure */

void    tls_proxy_client_start_free(TLS_CLIENT_START_PROPS *props)
{
    myfree((void *) props->nexthop);
    myfree((void *) props->host);
    myfree((void *) props->namaddr);
    myfree((void *) props->sni);
    myfree((void *) props->serverid);
    myfree((void *) props->helo);
    myfree((void *) props->protocols);
    myfree((void *) props->cipher_grade);
    myfree((void *) props->cipher_exclusions);
    if (props->matchargv)
	argv_free((ARGV *) props->matchargv);
    myfree((void *) props->mdalg);
    if (props->dane)
	tls_dane_free((TLS_DANE *) props->dane);
    myfree((void *) props);
}

/* tls_proxy_client_tlsa_scan - receive TLS_TLSA from stream */

static int tls_proxy_client_tlsa_scan(ATTR_SCAN_COMMON_FN scan_fn,
				          VSTREAM *fp, int flags, void *ptr)
{
    static VSTRING *data;
    TLS_TLSA *head;
    int     count;
    int     ret;

    if (data == 0)
	data = vstring_alloc(64);

    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_INT(TLS_ATTR_COUNT, &count),
		  ATTR_TYPE_END);
    if (ret == 1 && msg_verbose)
	msg_info("tls_proxy_client_tlsa_scan count=%d", count);

    for (head = 0; ret == 1 && count > 0; --count) {
	int     u, s, m;

	ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		      RECV_ATTR_INT(TLS_ATTR_USAGE, &u),
		      RECV_ATTR_INT(TLS_ATTR_SELECTOR, &s),
		      RECV_ATTR_INT(TLS_ATTR_MTYPE, &m),
		      RECV_ATTR_DATA(TLS_ATTR_DATA, data),
		      ATTR_TYPE_END);
	if (ret == 4) {
	    ret = 1;
	    /* This makes a copy of the static vstring content */
	    head = tlsa_prepend(head, u, s, m, (unsigned char *) STR(data),
				LEN(data));
	} else
	    ret = -1;
    }

    if (ret != 1) {
	tls_tlsa_free(head);
	head = 0;
    }
    *(TLS_TLSA **) ptr = head;
    if (msg_verbose)
	msg_info("tls_proxy_client_tlsa_scan ret=%d", ret);
    return (ret);
}

/* tls_proxy_client_dane_scan - receive TLS_DANE from stream */

static int tls_proxy_client_dane_scan(ATTR_SCAN_COMMON_FN scan_fn,
				          VSTREAM *fp, int flags, void *ptr)
{
    TLS_DANE *dane = 0;
    int     ret;
    int     have_dane = 0;

    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_INT(TLS_ATTR_DANE, &have_dane),
		  ATTR_TYPE_END);
    if (msg_verbose)
	msg_info("tls_proxy_client_dane_scan have_dane=%d", have_dane);

    if (ret == 1 && have_dane) {
	VSTRING *base_domain = vstring_alloc(25);

	dane = tls_dane_alloc();
	/* We only need the base domain and TLSA RRs */
	ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		      RECV_ATTR_STR(TLS_ATTR_DOMAIN, base_domain),
		      RECV_ATTR_FUNC(tls_proxy_client_tlsa_scan,
				     &dane->tlsa),
		      ATTR_TYPE_END);

	/* Always construct a well-formed structure. */
	dane->base_domain = vstring_export(base_domain);
	ret = (ret == 2 ? 1 : -1);
	if (ret != 1) {
	    tls_dane_free(dane);
	    dane = 0;
	}
    }
    *(TLS_DANE **) ptr = dane;
    if (msg_verbose)
	msg_info("tls_proxy_client_dane_scan ret=%d", ret);
    return (ret);
}

/* tls_proxy_client_start_scan - receive TLS_CLIENT_START_PROPS from stream */

int     tls_proxy_client_start_scan(ATTR_SCAN_COMMON_FN scan_fn, VSTREAM *fp,
				            int flags, void *ptr)
{
    TLS_CLIENT_START_PROPS *props
    = (TLS_CLIENT_START_PROPS *) mymalloc(sizeof(*props));
    int     ret;
    VSTRING *nexthop = vstring_alloc(25);
    VSTRING *host = vstring_alloc(25);
    VSTRING *namaddr = vstring_alloc(25);
    VSTRING *sni = vstring_alloc(25);
    VSTRING *serverid = vstring_alloc(25);
    VSTRING *helo = vstring_alloc(25);
    VSTRING *protocols = vstring_alloc(25);
    VSTRING *cipher_grade = vstring_alloc(25);
    VSTRING *cipher_exclusions = vstring_alloc(25);
    VSTRING *mdalg = vstring_alloc(25);

    if (msg_verbose)
	msg_info("begin tls_proxy_client_start_scan");

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(props, 0, sizeof(*props));
    props->ctx = 0;
    props->stream = 0;
    props->fd = -1;
    props->dane = 0;				/* scan_fn may return early */
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_INT(TLS_ATTR_TIMEOUT, &props->timeout),
		  RECV_ATTR_INT(TLS_ATTR_TLS_LEVEL, &props->tls_level),
		  RECV_ATTR_STR(TLS_ATTR_NEXTHOP, nexthop),
		  RECV_ATTR_STR(TLS_ATTR_HOST, host),
		  RECV_ATTR_STR(TLS_ATTR_NAMADDR, namaddr),
		  RECV_ATTR_STR(TLS_ATTR_SNI, sni),
		  RECV_ATTR_STR(TLS_ATTR_SERVERID, serverid),
		  RECV_ATTR_STR(TLS_ATTR_HELO, helo),
		  RECV_ATTR_STR(TLS_ATTR_PROTOCOLS, protocols),
		  RECV_ATTR_STR(TLS_ATTR_CIPHER_GRADE, cipher_grade),
		  RECV_ATTR_STR(TLS_ATTR_CIPHER_EXCLUSIONS,
				cipher_exclusions),
		  RECV_ATTR_FUNC(argv_attr_scan, &props->matchargv),
		  RECV_ATTR_STR(TLS_ATTR_MDALG, mdalg),
		  RECV_ATTR_FUNC(tls_proxy_client_dane_scan,
				 &props->dane),
		  ATTR_TYPE_END);
    /* Always construct a well-formed structure. */
    props->nexthop = vstring_export(nexthop);
    props->host = vstring_export(host);
    props->namaddr = vstring_export(namaddr);
    props->sni = vstring_export(sni);
    props->serverid = vstring_export(serverid);
    props->helo = vstring_export(helo);
    props->protocols = vstring_export(protocols);
    props->cipher_grade = vstring_export(cipher_grade);
    props->cipher_exclusions = vstring_export(cipher_exclusions);
    props->mdalg = vstring_export(mdalg);
    ret = (ret == 14 ? 1 : -1);
    if (ret != 1) {
	tls_proxy_client_start_free(props);
	props = 0;
    }
    *(TLS_CLIENT_START_PROPS **) ptr = props;
    if (msg_verbose)
	msg_info("tls_proxy_client_start_scan ret=%d", ret);
    return (ret);
}

#endif
