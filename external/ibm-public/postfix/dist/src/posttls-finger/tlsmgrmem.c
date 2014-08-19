/*	$NetBSD: tlsmgrmem.c,v 1.1.1.1.6.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	tlsmgrmem 3
/* SUMMARY
/*	Memory-based TLS manager interface for tlsfinger(1).
/* SYNOPSIS
/*	#ifdef	USE_TLS
/*	#include <tlsmgrmem.h>
/*
/*	void	tlsmgrmem_disable()
/*
/*	void	tlsmgrmem_status(enable, count, hits)
/*	int	*enable;
/*	int	*count;
/*	int	*hits;
/*
/*	void	tlsmgrmem_flush()
/*	#endif
/* DESCRIPTION
/*	tlsmgrmem_disable() disables the in-memory TLS session cache.
/*
/*	tlsmgrmem_status() reports whether the cache is enabled, the
/*	number of entries in the cache, and the number of cache hits.
/*	If any of the return pointers are null, that item is not reported.
/*
/*	tlsmgrmem_flush() flushes any cached data and frees the cache.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Viktor Dukhovni
/*--*/

#include <sys_defs.h>

#ifdef USE_TLS
#include <htable.h>
#include <vstring.h>
#include <tls_mgr.h>

#include "tlsmgrmem.h"

static HTABLE *tls_cache;
static int cache_enabled = 1;
static int cache_count;
static int cache_hits;
typedef void (*free_func) (char *);
static free_func free_value = (free_func) vstring_free;

void    tlsmgrmem_disable(void)
{
    cache_enabled = 0;
}

void    tlsmgrmem_flush(void)
{
    if (!tls_cache)
	return;
    htable_free(tls_cache, free_value);
}

void    tlsmgrmem_status(int *enabled, int *count, int *hits)
{
    if (enabled)
	*enabled = cache_enabled;
    if (count)
	*count = cache_count;
    if (hits)
	*hits = cache_hits;
}

/* tls_mgr_* - Local cache and stubs that do not talk to the TLS manager */

int     tls_mgr_seed(VSTRING *buf, int len)
{
    return (TLS_MGR_STAT_OK);
}

int     tls_mgr_policy(const char *unused_type, int *cachable, int *timeout)
{
    if (cache_enabled && tls_cache == 0)
	tls_cache = htable_create(1);
    *cachable = cache_enabled;
    *timeout = TLS_SESSION_LIFEMIN;
    return (TLS_MGR_STAT_OK);
}

int     tls_mgr_lookup(const char *unused_type, const char *key, VSTRING *buf)
{
    VSTRING *s;

    if (tls_cache == 0)
	return TLS_MGR_STAT_ERR;

    if ((s = (VSTRING *) htable_find(tls_cache, key)) == 0)
	return TLS_MGR_STAT_ERR;

    vstring_memcpy(buf, vstring_str(s), VSTRING_LEN(s));

    ++cache_hits;
    return (TLS_MGR_STAT_OK);
}

int     tls_mgr_update(const char *unused_type, const char *key,
		               const char *buf, ssize_t len)
{
    HTABLE_INFO *ent;
    VSTRING *s;

    if (tls_cache == 0)
	return TLS_MGR_STAT_ERR;

    if ((ent = htable_locate(tls_cache, key)) == 0) {
	s = vstring_alloc(len);
	ent = htable_enter(tls_cache, key, (char *) s);
    } else {
	s = (VSTRING *) ent->value;
    }
    vstring_memcpy(s, buf, len);

    ++cache_count;
    return (TLS_MGR_STAT_OK);
}

int     tls_mgr_delete(const char *unused_type, const char *key)
{
    if (tls_cache == 0)
	return TLS_MGR_STAT_ERR;

    if (htable_locate(tls_cache, key)) {
	htable_delete(tls_cache, key, free_value);
	--cache_count;
    }
    return (TLS_MGR_STAT_OK);
}

#endif
