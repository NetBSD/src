/*++
/* NAME
/*	tok822_resolve 3
/* SUMMARY
/*	address resolving, client interface
/* SYNOPSIS
/*	#include <tok822.h>
/*
/*	void	tok822_resolve(addr, reply)
/*	TOK822	*addr;
/*	RESOLVE_REPLY *reply;
/*
/*	void	tok822_resolve_from(sender, addr, reply)
/*	const char *sender;
/*	TOK822	*addr;
/*	RESOLVE_REPLY *reply;
/* DESCRIPTION
/*	tok822_resolve() takes an address token tree and finds out the
/*	transport to deliver via, the next-hop host on that transport,
/*	and the recipient relative to that host.
/*
/*	tok822_resolve_from() allows the caller to specify sender context
/*	that will be used to look up sender-dependent relayhost information.
/* SEE ALSO
/*	resolve_clnt(3) basic resolver client interface
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

#include <vstring.h>
#include <msg.h>

/* Global library. */

#include "resolve_clnt.h"
#include "tok822.h"

/* tok822_resolve - address rewriting interface */

void    tok822_resolve_from(const char *sender, TOK822 *addr,
			            RESOLVE_REPLY *reply)
{
    VSTRING *intern_form = vstring_alloc(100);

    if (addr->type != TOK822_ADDR)
	msg_panic("tok822_resolve: non-address token type: %d", addr->type);

    /*
     * Internalize the token tree and ship it to the resolve service.
     * Shipping string forms is much simpler than shipping parse trees.
     */
    tok822_internalize(intern_form, addr->head, TOK822_STR_DEFL);
    resolve_clnt_query_from(sender, vstring_str(intern_form), reply);
    if (msg_verbose)
	msg_info("tok822_resolve: from=%s addr=%s -> chan=%s, host=%s, rcpt=%s",
		 sender,
		 vstring_str(intern_form), vstring_str(reply->transport),
		 vstring_str(reply->nexthop), vstring_str(reply->recipient));

    vstring_free(intern_form);
}
