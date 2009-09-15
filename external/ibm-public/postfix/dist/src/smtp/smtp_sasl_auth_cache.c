/*	$NetBSD: smtp_sasl_auth_cache.c,v 1.1.1.1.2.2 2009/09/15 06:03:33 snj Exp $	*/

/*++
/* NAME
/*	smtp_sasl_auth_cache 3
/* SUMMARY
/*	Postfix SASL authentication reply cache
/* SYNOPSIS
/*	#include "smtp.h"
/*	#include "smtp_sasl_auth_cache.h"
/*
/*	SMTP_SASL_AUTH_CACHE *smtp_sasl_auth_cache_init(map, ttl)
/*	const char *map
/*	int	ttl;
/*
/*	void	smtp_sasl_auth_cache_store(auth_cache, session, resp)
/*	SMTP_SASL_AUTH_CACHE *auth_cache;
/*	const SMTP_SESSION *session;
/*	const SMTP_RESP *resp;
/*
/*	int	smtp_sasl_auth_cache_find(auth_cache, session)
/*	SMTP_SASL_AUTH_CACHE *auth_cache;
/*	const SMTP_SESSION *session;
/*
/*	char	*smtp_sasl_auth_cache_dsn(auth_cache)
/*	SMTP_SASL_AUTH_CACHE *auth_cache;
/*
/*	char	*smtp_sasl_auth_cache_text(auth_cache)
/*	SMTP_SASL_AUTH_CACHE *auth_cache;
/* DESCRIPTION
/*	This module maintains a cache of SASL authentication server replies.
/*	This can be used to avoid repeated login failure errors.
/*
/*	smtp_sasl_auth_cache_init() opens or creates the named cache.
/*
/*	smtp_sasl_auth_cache_store() stores information about a
/*	SASL login attempt together with the server status and
/*	complete response.
/*
/*	smtp_sasl_auth_cache_find() returns non-zero when a cache
/*	entry exists for the given host, username and password.
/*
/*	smtp_sasl_auth_cache_dsn() and smtp_sasl_auth_cache_text()
/*	return the status and complete server response as found
/*	with smtp_sasl_auth_cache_find().
/*
/*	Arguments:
/* .IP map
/*	Lookup table name. The name must be singular and must start
/*	with "proxy:".
/* .IP ttl
/*	The time after which a cache entry is considered expired.
/* .IP session
/*	Session context.
/* .IP resp
/*	Remote SMTP server response, to be stored into the cache.
/* DIAGNOSTICS
/*	All errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Original author:
/*	Keean Schupke
/*	Fry-IT Ltd.
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library
  */
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <base64_code.h>
#include <dict.h>

 /*
  * Global library
  */
#include <dsn_util.h>
#include <dict_proxy.h>

 /*
  * Application-specific
  */
#include "smtp.h"
#include "smtp_sasl_auth_cache.h"

 /*
  * XXX This feature stores passwords, so we must mask them with a strong
  * cryptographic hash. This requires OpenSSL support.
  * 
  * XXX It would be even better if the stored hash were salted.
  */
#ifdef HAVE_SASL_AUTH_CACHE

/* smtp_sasl_auth_cache_init - per-process initialization (pre jail) */

SMTP_SASL_AUTH_CACHE *smtp_sasl_auth_cache_init(const char *map, int ttl)
{
    const char *myname = "smtp_sasl_auth_cache_init";
    SMTP_SASL_AUTH_CACHE *auth_cache;

    /*
     * Sanity checks.
     */
#define HAS_MULTIPLE_VALUES(s) ((s)[strcspn((s),  ", \t\r\n")] != 0)

    if (*map == 0)
	msg_panic("%s: empty SASL authentication cache name", myname);
    if (ttl < 0)
	msg_panic("%s: bad SASL authentication cache ttl: %d", myname, ttl);
    if (HAS_MULTIPLE_VALUES(map))
	msg_fatal("SASL authentication cache name \"%s\" "
		  "contains multiple values", map);

    /*
     * XXX To avoid multiple writers the map needs to be maintained by the
     * proxywrite service. We would like to have a DICT_FLAG_REQ_PROXY flag
     * so that the library can enforce this, but that requires moving the
     * dict_proxy module one level down in the build dependency hierachy.
     */
#define CACHE_DICT_OPEN_FLAGS \
	(DICT_FLAG_DUP_REPLACE | DICT_FLAG_SYNC_UPDATE)
#define PROXY_COLON	DICT_TYPE_PROXY ":"
#define PROXY_COLON_LEN	(sizeof(PROXY_COLON) - 1)

    if (strncmp(map, PROXY_COLON, PROXY_COLON_LEN) != 0)
	msg_fatal("SASL authentication cache name \"%s\" must start with \""
		  PROXY_COLON, map);

    auth_cache = (SMTP_SASL_AUTH_CACHE *) mymalloc(sizeof(*auth_cache));
    auth_cache->dict = dict_open(map, O_CREAT | O_RDWR, CACHE_DICT_OPEN_FLAGS);
    auth_cache->ttl = ttl;
    auth_cache->dsn = mystrdup("");
    auth_cache->text = mystrdup("");
    return (auth_cache);
}

 /*
  * Each cache lookup key contains a server host name and user name. Each
  * cache value contains a time stamp, a hashed password, and the server
  * response. With this organization, we don't have to worry about cache
  * pollution, because we can detect if a cache entry has expired, or if the
  * password has changed.
  */

/* smtp_sasl_auth_cache_make_key - format auth failure cache lookup key */

static char *smtp_sasl_auth_cache_make_key(const char *host, const char *user)
{
    VSTRING *buf = vstring_alloc(100);

    vstring_sprintf(buf, "%s;%s", host, user);
    return (vstring_export(buf));
}

/* smtp_sasl_auth_cache_make_pass - hash the auth failure cache password */

static char *smtp_sasl_auth_cache_make_pass(const char *password)
{
    VSTRING *buf = vstring_alloc(2 * SHA_DIGEST_LENGTH);

    base64_encode(buf, (const char *) SHA1((const unsigned char *) password,
					   strlen(password), 0),
		  SHA_DIGEST_LENGTH);
    return (vstring_export(buf));
}

/* smtp_sasl_auth_cache_make_value - format auth failure cache value */

static char *smtp_sasl_auth_cache_make_value(const char *password,
					             const char *dsn,
					             const char *rep_str)
{
    VSTRING *val_buf = vstring_alloc(100);
    char   *pwd_hash;
    unsigned long now = (unsigned long) time((time_t *) 0);

    pwd_hash = smtp_sasl_auth_cache_make_pass(password);
    vstring_sprintf(val_buf, "%lu;%s;%s;%s", now, pwd_hash, dsn, rep_str);
    myfree(pwd_hash);
    return (vstring_export(val_buf));
}

/* smtp_sasl_auth_cache_valid_value - validate auth failure cache value */

static int smtp_sasl_auth_cache_valid_value(SMTP_SASL_AUTH_CACHE *auth_cache,
				              const char *entry,
				              const char *password)
{
    ssize_t len = strlen(entry);
    char   *cache_hash = mymalloc(len);
    char   *curr_hash;
    unsigned long now = (unsigned long) time((time_t *) 0);
    unsigned long time_stamp;
    int     valid;

    auth_cache->dsn = myrealloc(auth_cache->dsn, len);
    auth_cache->text = myrealloc(auth_cache->text, len);

    if (sscanf(entry, "%lu;%[^;];%[^;];%[^\n]", &time_stamp, cache_hash,
	       auth_cache->dsn, auth_cache->text) != 4
	|| !dsn_valid(auth_cache->dsn)) {
	msg_warn("bad smtp_sasl_auth_cache entry: %.100s", entry);
	valid = 0;
    } else if (time_stamp + auth_cache->ttl < now) {
	valid = 0;
    } else {
	curr_hash = smtp_sasl_auth_cache_make_pass(password);
	valid = (strcmp(cache_hash, curr_hash) == 0);
	myfree(curr_hash);
    }
    myfree(cache_hash);
    return (valid);
}

/* smtp_sasl_auth_cache_find - search auth failure cache */

int     smtp_sasl_auth_cache_find(SMTP_SASL_AUTH_CACHE *auth_cache,
				          const SMTP_SESSION *session)
{
    char   *key;
    const char *entry;
    int     valid = 0;

    key = smtp_sasl_auth_cache_make_key(session->host, session->sasl_username);
    if ((entry = dict_get(auth_cache->dict, key)) != 0)
	if ((valid = smtp_sasl_auth_cache_valid_value(auth_cache, entry,
						session->sasl_passwd)) == 0)
	    /* Remove expired, password changed, or malformed cache entry. */
	    if (dict_del(auth_cache->dict, key) == 0)
		msg_warn("SASL auth failure map %s: entry not deleted: %s",
			 auth_cache->dict->name, key);
    myfree(key);
    return (valid);
}

/* smtp_sasl_auth_cache_store - update auth failure cache */

void    smtp_sasl_auth_cache_store(SMTP_SASL_AUTH_CACHE *auth_cache,
				           const SMTP_SESSION *session,
				           const SMTP_RESP *resp)
{
    char   *key;
    char   *value;

    key = smtp_sasl_auth_cache_make_key(session->host, session->sasl_username);
    value = smtp_sasl_auth_cache_make_value(session->sasl_passwd,
					    resp->dsn, resp->str);
    dict_put(auth_cache->dict, key, value);

    myfree(value);
    myfree(key);
}

#endif
