/*	$NetBSD: tls_mgr.c,v 1.1.1.1.26.1 2014/08/10 07:12:50 tls Exp $	*/

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
/*	int	tls_mgr_policy(cache_type, cachable, timeout)
/*	const char *cache_type;
/*	int	*cachable;
/*	int	*timeout;
/*
/*	int	tls_mgr_update(cache_type, cache_id, buf, len)
/*	const char *cache_type;
/*	const char *cache_id;
/*	const char *buf;
/*	ssize_t	len;
/*
/*	int	tls_mgr_lookup(cache_type, cache_id, buf)
/*	const char *cache_type;
/*	const char *cache_id;
/*	VSTRING	*buf;
/*
/*	int	tls_mgr_delete(cache_type, cache_id)
/*	const char *cache_type;
/*	const char *cache_id;
/*
/*	TLS_TICKET_KEY *tls_mgr_key(keyname, timeout)
/*	unsigned char *keyname;
/*	int	timeout;
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
/*	tls_mgr_key() is used to retrieve the current TLS session ticket
/*	encryption or decryption keys.
/*
/*	Arguments:
/* .IP cache_type
/*	One of TLS_MGR_SCACHE_SMTPD, TLS_MGR_SCACHE_SMTP or
/*	TLS_MGR_SCACHE_LMTP.
/* .IP cachable
/*	Pointer to int, set non-zero if the requested cache_type
/*	is enabled.
/* .IP timeout
/*	Pointer to int, returns the cache entry timeout.
/* .IP cache_id
/*	The session cache lookup key.
/* .IP buf
/*	The result or input buffer.
/* .IP len
/*	The length of the input buffer, or the amount of data requested.
/* .IP keyname
/*	Is null when requesting the current encryption keys.  Otherwise,
/*	keyname is a pointer to an array of TLS_TICKET_NAMELEN unsigned
/*	chars (not NUL terminated) that is an identifier for a key
/*	previously used to encrypt a session ticket.  When encrypting
/*	a null result indicates that session tickets are not supported, when
/*	decrypting it indicates that no matching keys were found.
/* .IP timeout
/*	The encryption key timeout.  Once a key has been active for this many
/*	seconds it is retired and used only for decrypting previously issued
/*	session tickets for another timeout seconds, and is then destroyed.
/*	The timeout must not be longer than half the SSL session lifetime.
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <attr.h>
#include <attr_clnt.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>

/* TLS library. */
#include <tls_mgr.h>

/* Application-specific. */

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

static ATTR_CLNT *tls_mgr;

/* tls_mgr_open - create client handle */

static void tls_mgr_open(void)
{
    char   *service;

    /*
     * Sanity check.
     */
    if (tls_mgr != 0)
	msg_panic("tls_mgr_open: multiple initialization");

    /*
     * Use whatever IPC is preferred for internal use: UNIX-domain sockets or
     * Solaris streams.
     */
    service = concatenate("local:" TLS_MGR_CLASS "/", var_tls_mgr_service,
			  (char *) 0);
    tls_mgr = attr_clnt_create(service, var_ipc_timeout,
			       var_ipc_idle_limit, var_ipc_ttl_limit);
    myfree(service);

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
			  ATTR_TYPE_INT, TLS_MGR_ATTR_SIZE, len,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SEED, buf,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_policy - request caching policy */

int     tls_mgr_policy(const char *cache_type, int *cachable, int *timeout)
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
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, TLS_MGR_ATTR_CACHABLE, cachable,
			  ATTR_TYPE_INT, TLS_MGR_ATTR_SESSTOUT, timeout,
			  ATTR_TYPE_END) != 3)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_lookup - request cached session */

int     tls_mgr_lookup(const char *cache_type, const char *cache_id,
		               VSTRING *buf)
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
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, buf,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_update - save session to cache */

int     tls_mgr_update(const char *cache_type, const char *cache_id,
		               const char *buf, ssize_t len)
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
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, len, buf,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_delete - remove cached session */

int     tls_mgr_delete(const char *cache_type, const char *cache_id)
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
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* request_scache_key - ask tlsmgr(8) for matching key */

static TLS_TICKET_KEY *request_scache_key(unsigned char *keyname)
{
    TLS_TICKET_KEY tmp;
    static VSTRING *keybuf;
    char   *name;
    size_t  len;
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0)
	tls_mgr_open();

    if (keybuf == 0)
	keybuf = vstring_alloc(sizeof(tmp));

    /* In tlsmgr requests we encode null key names as empty strings. */
    name = keyname ? (char *) keyname : "";
    len = keyname ? TLS_TICKET_NAMELEN : 0;

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_TKTKEY,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_KEYNAME, len, name,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_KEYBUF, keybuf,
			  ATTR_TYPE_END) != 2
	|| status != TLS_MGR_STAT_OK
	|| LEN(keybuf) != sizeof(tmp))
	return (0);

    memcpy((char *) &tmp, STR(keybuf), sizeof(tmp));
    return (tls_scache_key_rotate(&tmp));
}

/* tls_mgr_key - session ticket key lookup, local cache, then tlsmgr(8) */

TLS_TICKET_KEY *tls_mgr_key(unsigned char *keyname, int timeout)
{
    TLS_TICKET_KEY *key = 0;
    time_t  now = time((time_t *) 0);

    /* A zero timeout disables session tickets. */
    if (timeout <= 0)
	return (0);

    if ((key = tls_scache_key(keyname, now, timeout)) == 0)
	key = request_scache_key(keyname);
    return (key);
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
	if (argv->argc == 0) {
	    argv_free(argv);
	    continue;
	}
#define COMMAND(argv, str, len) \
    (strcasecmp(argv->argv[0], str) == 0 && argv->argc == len)

	if (COMMAND(argv, "policy", 2)) {
	    int     cachable;
	    int     timeout;

	    status = tls_mgr_policy(argv->argv[1], &cachable, &timeout);
	    vstream_printf("status=%d cachable=%d timeout=%d\n",
			   status, cachable, timeout);
	} else if (COMMAND(argv, "seed", 2)) {
	    VSTRING *buf = vstring_alloc(10);
	    VSTRING *hex = vstring_alloc(10);
	    int     len = atoi(argv->argv[1]);

	    status = tls_mgr_seed(buf, len);
	    hex_encode(hex, STR(buf), LEN(buf));
	    vstream_printf("status=%d seed=%s\n", status, STR(hex));
	    vstring_free(hex);
	    vstring_free(buf);
	} else if (COMMAND(argv, "lookup", 3)) {
	    VSTRING *buf = vstring_alloc(10);

	    status = tls_mgr_lookup(argv->argv[1], argv->argv[2], buf);
	    vstream_printf("status=%d session=%.*s\n",
			   status, LEN(buf), STR(buf));
	    vstring_free(buf);
	} else if (COMMAND(argv, "update", 4)) {
	    status = tls_mgr_update(argv->argv[1], argv->argv[2],
				    argv->argv[3], strlen(argv->argv[3]));
	    vstream_printf("status=%d\n", status);
	} else if (COMMAND(argv, "delete", 3)) {
	    status = tls_mgr_delete(argv->argv[1], argv->argv[2]);
	    vstream_printf("status=%d\n", status);
	} else {
	    vstream_printf("usage:\n"
			   "seed byte_count\n"
			   "policy smtpd|smtp|lmtp\n"
			   "lookup smtpd|smtp|lmtp cache_id\n"
			   "update smtpd|smtp|lmtp cache_id session\n"
			   "delete smtpd|smtp|lmtp cache_id\n");
	}
	vstream_fflush(VSTREAM_OUT);
	argv_free(argv);
    }

    vstring_free(inbuf);
    return (0);
}

#endif					/* TEST */

#endif					/* USE_TLS */
