/*	$NetBSD: tls_mgr.c,v 1.1.1.1 2005/08/18 21:11:06 rpaulo Exp $	*/

/*++
/* NAME
/*	tls_mgr 3
/* SUMMARY
/*	tlsmgr client interface
/* SYNOPSIS
/*	#include <tls_mgr.h>
/*
/*	int	tls_mgr_seed(buf, len)
/*	VSTRING	*buf;
/*	int	len;
/*
/*	int	tls_mgr_policy(cache_types)
/*	int	*cache_types;
/*
/*	int	tls_mgr_update(cache_type, cache_id,
/*				openssl_version, flags, buf, len)
/*	int	cache_type;
/*	const char *cache_id;
/*	long	openssl_version;
/*	int	flags;
/*	const char *buf;
/*	int	len;
/*
/*	int	tls_mgr_lookup(cache_type, cache_id,
/*				openssl_version, flags, buf)
/*	int	cache_type;
/*	const char *cache_id;
/*	long	openssl_version;
/*	int	flags;
/*	VSTRING	*buf;
/*
/*	int	tls_mgr_delete(cache_type, cache_id)
/*	int	cache_type;
/*	const char *cache_id;
/* DESCRIPTION
/*	These routines communicate with the tlsmgr(8) server for
/*	entropy and session cache management. Since these are
/*	non-critical services, requests are allowed to fail without
/*	disrupting Postfix.
/*
/*	tls_mgr_seed() requests entropy from the tlsmgr(8)
/*	Pseudo Random Number Generator (PRNG) pool.
/*
/*	tls_mgr_policy() requests the session caching policy.
/*
/*	tls_mgr_lookup() loads the specified session from
/*	the specified session cache.
/*
/*	tls_mgr_update() saves the specified session to
/*	the specified session cache.
/*
/*	tls_mgr_delete() removes specified session from
/*	the specified session cache.
/*
/*	Arguments
/* .IP cache_types
/*	The bit-wise OR of zero or more of the following:
/* .RS
/* .IP TLS_MGR_SCACHE_CLIENT
/*	Session caching is enabled for SMTP client sessions.
/* .IP TLS_MGR_SCACHE_SERVER
/*	Session caching is enabled for SMTP server sessions.
/* .RE
/* .IP cache_type
/*	One of TLS_MGR_SCACHE_CLIENT or TLS_MGR_SCACHE_SERVER (see above).
/* .IP cache_id
/*	The session cache lookup key.
/* .IP openssl_version
/*	The OpenSSL version. Sessions saved by the wrong OpenSSL version are
/*	deleted, to avoid compatibility problems.
/* .IP flags
/*	Flags that must be set in the retrieved cache entry; it not,
/*	the cache entry is deleted.
/* .IP buf
/*	The result or input buffer.
/* .IP len
/*	The length of the input buffer, or the amount of data requested.
/* DIAGNOSTICS
/*	All client functions return one of the following status codes:
/* .IP TLS_MGR_STAT_OK
/*      The request completed, and the requested operation was
/*	successful (for example, the requested session was found,
/*	or the specified session was saved or removed).
/* .IP TLS_MGR_STAT_ERR
/*      The request completed, but the requested operation failed
/*	(for example, the requested object was not found or the
/*	specified session was not saved or removed).
/* .IP TLS_MGR_STAT_FAIL
/*      The request could not complete (the client could not
/*	communicate with the tlsmgr(8) server).
/* SEE ALSO
/*	tlsmgr(8) TLS session and PRNG management
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <attr.h>
#include <attr_clnt.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <tls_mgr.h>

/* Application-specific. */

static ATTR_CLNT *tls_mgr;

/* tls_mgr_open - create client handle */

static void tls_mgr_open(void)
{

    /*
     * Sanity check.
     */
    if (tls_mgr != 0)
	msg_panic("tls_mgr_open: multiple initialization");

    /*
     * Use whatever IPC is preferred for internal use: UNIX-domain sockets or
     * Solaris streams.
     */
#ifndef VAR_TLS_MGR_SERVICE
    tls_mgr = attr_clnt_create("local:" TLS_MGR_CLASS "/" TLS_MGR_SERVICE,
			       var_ipc_timeout, var_ipc_idle_limit,
			       var_ipc_ttl_limit);
#else
    tls_mgr = attr_clnt_create(var_tlsmgr_service, var_ipc_timeout,
			       var_ipc_idle_limit, var_ipc_ttl_limit);
#endif
    attr_clnt_control(tls_mgr,
		      ATTR_CLNT_CTL_PROTO, attr_vprint, attr_vscan,
		      ATTR_CLNT_CTL_END);
}

/* tls_mgr_seed - request PRNG seed */

int     tls_mgr_seed(VSTRING *buf, int len)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0)
	tls_mgr_open();

    /*
     * Request seed.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request attributes */
			  ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_SEED,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_SIZE, len,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes */
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SEED, buf,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_policy - request caching policy */

int     tls_mgr_policy(int *policy)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0)
	tls_mgr_open();

    /*
     * Request policy.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request attributes */
			ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_POLICY,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes */
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_POLICY, policy,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_lookup - request cached session */

int     tls_mgr_lookup(int cache_type, const char *cache_id,
		               long openssl_vsn, int flags, VSTRING *buf)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0)
	tls_mgr_open();

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_LOOKUP,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_LONG, TLS_MGR_ATTR_VERSION, openssl_vsn,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_FLAGS, flags,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, buf,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_update - save session to cache */

int     tls_mgr_update(int cache_type, const char *cache_id,
		               long openssl_vsn, int flags,
		               const char *buf, int len)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0)
	tls_mgr_open();

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_UPDATE,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_LONG, TLS_MGR_ATTR_VERSION, openssl_vsn,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_FLAGS, flags,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, len, buf,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_delete - remove cached session */

int     tls_mgr_delete(int cache_type, const char *cache_id)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0)
	tls_mgr_open();

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_DELETE,
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_NUM, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

#ifdef TEST

/* System library. */

#include <stdlib.h>

/* Utility library. */

#include <argv.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <hex_code.h>

/* Global library. */

#include <config.h>

/* Application-specific. */

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

int     main(int unused_ac, char **av)
{
    VSTRING *inbuf = vstring_alloc(10);
    int     status;
    ARGV   *argv = 0;

    msg_vstream_init(av[0], VSTREAM_ERR);

    msg_verbose = 3;

    mail_conf_read();
    msg_info("using config files in %s", var_config_dir);

    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    while (vstring_fgets_nonl(inbuf, VSTREAM_IN)) {
	argv = argv_split(STR(inbuf), " \t\r\n");
	if (argv->argc == 0)
	    continue;

#define COMMAND(argv, str, len) \
    (strcasecmp(argv->argv[0], str) == 0 && argv->argc == len)

	if (COMMAND(argv, "policy", 1)) {
	    int     cache_types;

	    status = tls_mgr_policy(&cache_types);
	    vstream_printf("status=%d policy=0x%x\n", status, cache_types);
	} else if (COMMAND(argv, "seed", 2)) {
	    VSTRING *buf = vstring_alloc(10);
	    VSTRING *hex = vstring_alloc(10);
	    int     len = atoi(argv->argv[1]);

	    status = tls_mgr_seed(buf, len);
	    hex_encode(hex, STR(buf), LEN(buf));
	    vstream_printf("status=%d seed=%s\n", status, STR(hex));
	    vstring_free(hex);
	    vstring_free(buf);
	} else if (COMMAND(argv, "lookup", 5)) {
	    VSTRING *buf = vstring_alloc(10);
	    int     cache_type = atoi(argv->argv[1]);
	    long    openssl_vsn = atol(argv->argv[3]);
	    int     flags = atoi(argv->argv[4]);

	    status = tls_mgr_lookup(cache_type, argv->argv[2],
				    openssl_vsn, flags, buf);
	    vstream_printf("status=%d session=%.*s\n",
			   status, LEN(buf), STR(buf));
	} else if (COMMAND(argv, "update", 6)) {
	    int     cache_type = atoi(argv->argv[1]);
	    long    openssl_vsn = atol(argv->argv[3]);
	    int     flags = atoi(argv->argv[4]);

	    status = tls_mgr_update(cache_type, argv->argv[2],
				    openssl_vsn, flags,
				    argv->argv[5], strlen(argv->argv[5]));
	    vstream_printf("status=%d\n", status);
	} else if (COMMAND(argv, "delete", 3)) {
	    int     cache_type = atoi(argv->argv[1]);

	    status = tls_mgr_delete(cache_type, argv->argv[2]);
	    vstream_printf("status=%d\n", status);
	} else {
	    vstream_printf("usage:\n"
			   "seed byte_count\n"
			   "policy\n"
			"lookup cache_type cache_id openssl_version flags\n"
		"update cache_type cache_id openssl_version flags session\n"
			   "delete cache_type cache_id\n");
	}
	vstream_fflush(VSTREAM_OUT);
    }
    if (argv)
	argv_free(argv);

    vstring_free(inbuf);
}

#endif					/* TEST */

#endif					/* USE_TLS */
