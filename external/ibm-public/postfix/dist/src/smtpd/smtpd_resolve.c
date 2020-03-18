/*	$NetBSD: smtpd_resolve.c,v 1.3 2020/03/18 19:05:20 christos Exp $	*/

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
/*	const RESOLVE_REPLY *smtpd_resolve_addr(sender, addr)
/*	const char *sender;
/*	const char *addr;
/* DESCRIPTION
/*	This module maintains a resolve client cache that persists
/*	across SMTP sessions (not process life times). Addresses
/*	are always resolved in local rewriting context.
/*
/*	smtpd_resolve_init() initializes the cache and must be
/*	called before the cache can be used. This function may also
/*	be called to flush the cache after an address class update.
/*
/*	smtpd_resolve_addr() resolves one address or returns
/*	a known result from cache.
/*
/*	Arguments:
/* .IP cache_size
/*	The requested cache size.
/* .IP sender
/*	The message sender, or null pointer.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <ctable.h>
#include <stringops.h>
#include <split_at.h>

/* Global library. */

#include <rewrite_clnt.h>
#include <resolve_clnt.h>
#include <mail_proto.h>

/* Application-specific. */

#include <smtpd_resolve.h>

static CTABLE *smtpd_resolve_cache;

#define STR(x) vstring_str(x)
#define SENDER_ADDR_JOIN_CHAR '\n'

/* resolve_pagein - page in an address resolver result */

static void *resolve_pagein(const char *sender_plus_addr, void *unused_context)
{
    const char myname[] = "resolve_pagein";
    static VSTRING *query;
    static VSTRING *junk;
    static VSTRING *sender_buf;
    RESOLVE_REPLY *reply;
    const char *sender;
    const char *addr;

    /*
     * Initialize on the fly.
     */
    if (query == 0) {
	query = vstring_alloc(10);
	junk = vstring_alloc(10);
	sender_buf = vstring_alloc(10);
    }

    /*
     * Initialize.
     */
    reply = (RESOLVE_REPLY *) mymalloc(sizeof(*reply));
    resolve_clnt_init(reply);

    /*
     * Split the sender and address.
     */
    vstring_strcpy(junk, sender_plus_addr);
    sender = STR(junk);
    if ((addr = split_at(STR(junk), SENDER_ADDR_JOIN_CHAR)) == 0)
	msg_panic("%s: bad search key: \"%s\"", myname, sender_plus_addr);

    /*
     * Resolve the address.
     */
    rewrite_clnt_internal(MAIL_ATTR_RWR_LOCAL, sender, sender_buf);
    rewrite_clnt_internal(MAIL_ATTR_RWR_LOCAL, addr, query);
    resolve_clnt_query_from(STR(sender_buf), STR(query), reply);
    vstring_strcpy(junk, STR(reply->recipient));
    casefold(reply->recipient, STR(junk));	/* XXX */

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
     * Flush a pre-existing cache. The smtpd_check test program requires this
     * after an address class change.
     */
    if (smtpd_resolve_cache)
	ctable_free(smtpd_resolve_cache);

    /*
     * Initialize the resolved address cache. Note: the cache persists across
     * SMTP sessions so we cannot make it dependent on session state.
     */
    smtpd_resolve_cache = ctable_create(cache_size, resolve_pagein,
					resolve_pageout, (void *) 0);
}

/* smtpd_resolve_addr - resolve cached address */

const RESOLVE_REPLY *smtpd_resolve_addr(const char *sender, const char *addr)
{
    static VSTRING *sender_plus_addr_buf;

    /*
     * Initialize on the fly.
     */
    if (sender_plus_addr_buf == 0)
	sender_plus_addr_buf = vstring_alloc(10);

    /*
     * Sanity check.
     */
    if (smtpd_resolve_cache == 0)
	msg_panic("smtpd_resolve_addr: missing initialization");

    /*
     * Reply from the read-through cache.
     */
    vstring_sprintf(sender_plus_addr_buf, "%s%c%s",
		    sender ? sender : RESOLVE_NULL_FROM,
		    SENDER_ADDR_JOIN_CHAR, addr);
    return (const RESOLVE_REPLY *)
	ctable_locate(smtpd_resolve_cache, STR(sender_plus_addr_buf));
}
