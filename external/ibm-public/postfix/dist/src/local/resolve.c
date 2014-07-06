/*	$NetBSD: resolve.c,v 1.1.1.2 2014/07/06 19:27:52 tron Exp $	*/

/*++
/* NAME
/*	resolve 3
/* SUMMARY
/*	resolve recipient and deliver locally or remotely
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	deliver_resolve_tree(state, usr_attr, addr)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	TOK822	*addr;
/*
/*	int	deliver_resolve_addr(state, usr_attr, addr)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	char	*addr;
/* DESCRIPTION
/*	deliver_resolve_XXX() resolves a recipient that is the result from
/*	e.g., alias expansion, and delivers locally or via forwarding.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, sender and more.
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* .IP addr
/*	An address from, e.g., alias expansion.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. The result is non-zero when the
/*	operation should be tried again. Warnings: malformed address.
/* SEE ALSO
/*	recipient(3) local delivery
/*	indirect(3) deliver via forwarding
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
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <htable.h>

/* Global library. */

#include <mail_proto.h>
#include <resolve_clnt.h>
#include <rewrite_clnt.h>
#include <tok822.h>
#include <mail_params.h>
#include <defer.h>

/* Application-specific. */

#include "local.h"

/* deliver_resolve_addr - resolve and deliver */

int     deliver_resolve_addr(LOCAL_STATE state, USER_ATTR usr_attr, char *addr)
{
    TOK822 *tree;
    int     result;

    tree = tok822_scan_addr(addr);
    result = deliver_resolve_tree(state, usr_attr, tree);
    tok822_free_tree(tree);
    return (result);
}

/* deliver_resolve_tree - resolve and deliver */

int     deliver_resolve_tree(LOCAL_STATE state, USER_ATTR usr_attr, TOK822 *addr)
{
    const char *myname = "deliver_resolve_tree";
    RESOLVE_REPLY reply;
    int     status;
    ssize_t ext_len;
    char   *ratsign;
    int     rcpt_delim;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * Initialize.
     */
    resolve_clnt_init(&reply);

    /*
     * Rewrite the address to canonical form, just like the cleanup service
     * does. Then, resolve the address to (transport, nexhop, recipient),
     * just like the queue manager does. The only part missing here is the
     * virtual address substitution. Message forwarding fixes most of that.
     */
    tok822_rewrite(addr, REWRITE_CANON);
    tok822_resolve(addr, &reply);

    /*
     * First, a healthy portion of error handling.
     */
    if (reply.flags & RESOLVE_FLAG_FAIL) {
	dsb_simple(state.msg_attr.why, "4.3.0", "address resolver failure");
	status = defer_append(BOUNCE_FLAGS(state.request),
			      BOUNCE_ATTR(state.msg_attr));
    } else if (reply.flags & RESOLVE_FLAG_ERROR) {
	dsb_simple(state.msg_attr.why, "5.1.3",
		   "bad recipient address syntax: %s", STR(reply.recipient));
	status = bounce_append(BOUNCE_FLAGS(state.request),
			       BOUNCE_ATTR(state.msg_attr));
    } else {

	/*
	 * Splice in the optional unmatched address extension.
	 */
	if (state.msg_attr.unmatched) {
	    rcpt_delim = state.msg_attr.local[strlen(state.msg_attr.user)];
	    if ((ratsign = strrchr(STR(reply.recipient), '@')) == 0) {
		VSTRING_ADDCH(reply.recipient, rcpt_delim);
		vstring_strcat(reply.recipient, state.msg_attr.unmatched);
	    } else {
		ext_len = strlen(state.msg_attr.unmatched);
		VSTRING_SPACE(reply.recipient, ext_len + 2);
		if ((ratsign = strrchr(STR(reply.recipient), '@')) == 0)
		    msg_panic("%s: recipient @ botch", myname);
		memmove(ratsign + ext_len + 1, ratsign, strlen(ratsign) + 1);
		*ratsign = rcpt_delim;
		memcpy(ratsign + 1, state.msg_attr.unmatched, ext_len);
		VSTRING_SKIP(reply.recipient);
	    }
	}
	state.msg_attr.rcpt.address = STR(reply.recipient);

	/*
	 * Delivery to a local or non-local address. For a while there was
	 * some ugly code to force local recursive alias expansions on a host
	 * with no authority over the local domain, but that code was just
	 * too unclean.
	 */
	if (strcmp(state.msg_attr.relay, STR(reply.transport)) == 0) {
	    status = deliver_recipient(state, usr_attr);
	} else {
	    status = deliver_indirect(state);
	}
    }

    /*
     * Cleanup.
     */
    resolve_clnt_free(&reply);

    return (status);
}
