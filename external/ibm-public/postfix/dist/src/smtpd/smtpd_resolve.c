/*	$NetBSD: smtpd_resolve.c,v 1.1.1.1.2.2 2009/09/15 06:03:34 snj Exp $	*/

/*++
/* NAME
/*	smtpd_resolve 3
/* SUMMARY
/*	caching resolve client
/* SYNOPSIS
/*	#include <smtpd_resolve.h>
/*
/*	void	smtpd_resolve_init(cache_size)
/*	int	cache_size;
/*
/*	const RESOLVE_REPLY *smtpd_resolve_addr(addr)
/*	const char *addr;
/* DESCRIPTION
/*	This module maintains a resolve client cache that persists
/*	across SMTP sessions (not process life times). Addresses
/*	are always resolved in local rewriting context.
/*
/*	smtpd_resolve_init() initializes the cache and must
/*	called once before the cache can be used.
/*
/*	smtpd_resolve_addr() resolves one address or returns
/*	a known result from cache.
/*
/*	Arguments:
/* .IP cache_size
/*	The requested cache size.
/* .IP addr
/*	The address to resolve.
/* DIAGNOSTICS
/*	All errors are fatal.
/* BUGS
/*	The recipient address is always case folded to lowercase.
/*	Changing this requires great care, since the address is used
/*	for policy lookups.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <ctable.h>
#include <stringops.h>

/* Global library. */

#include <rewrite_clnt.h>
#include <resolve_clnt.h>
#include <mail_proto.h>

/* Application-specific. */

#include <smtpd_resolve.h>

static CTABLE *smtpd_resolve_cache;

#define STR(x) vstring_str(x)

/* resolve_pagein - page in an address resolver result */

static void *resolve_pagein(const char *addr, void *unused_context)
{
    static VSTRING *query;
    RESOLVE_REPLY *reply;

    /*
     * Initialize on the fly.
     */
    if (query == 0)
	query = vstring_alloc(10);

    /*
     * Initialize.
     */
    reply = (RESOLVE_REPLY *) mymalloc(sizeof(*reply));
    resolve_clnt_init(reply);

    /*
     * Resolve the address.
     */
    rewrite_clnt_internal(MAIL_ATTR_RWR_LOCAL, addr, query);
    resolve_clnt_query(STR(query), reply);
    lowercase(STR(reply->recipient));		/* XXX */

    /*
     * Save the result.
     */
    return ((void *) reply);
}

/* resolve_pageout - page out an address resolver result */

static void resolve_pageout(void *data, void *unused_context)
{
    RESOLVE_REPLY *reply = (RESOLVE_REPLY *) data;

    resolve_clnt_free(reply);
    myfree((void *) reply);
}

/* smtpd_resolve_init - set up global cache */

void    smtpd_resolve_init(int cache_size)
{

    /*
     * Sanity check.
     */
    if (smtpd_resolve_cache)
	msg_panic("smtpd_resolve_init: multiple initialization");

    /*
     * Initialize the resolved address cache. Note: the cache persists across
     * SMTP sessions so we cannot make it dependent on session state.
     */
    smtpd_resolve_cache = ctable_create(cache_size, resolve_pagein,
					resolve_pageout, (void *) 0);
}

/* smtpd_resolve_addr - resolve cached addres */

const RESOLVE_REPLY *smtpd_resolve_addr(const char *addr)
{

    /*
     * Sanity check.
     */
    if (smtpd_resolve_cache == 0)
	msg_panic("smtpd_resolve_addr: missing initialization");

    /*
     * Reply from the read-through cache.
     */
    return (const RESOLVE_REPLY *) ctable_locate(smtpd_resolve_cache, addr);
}
