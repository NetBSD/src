/*	$NetBSD: bounce_workaround.c,v 1.1.1.2.8.2 2014/08/19 23:59:43 tls Exp $	*/

/*++
/* NAME
/*	bounce_workaround 3
/* SUMMARY
/*	Send non-delivery notification with sender override
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	bounce_workaround(state)
/*	LOCAL_STATE state;
/* DESCRIPTION
/*	This module works around a limitation in the bounce daemon
/*	protocol, namely, the assumption that the envelope sender
/*	address in a queue file is the delivery status notification
/*	address for all recipients in that queue file. The assumption
/*	is not valid when the local(8) delivery agent overrides the
/*	envelope sender address by an owner- alias, for one or more
/*	recipients in the queue file.
/*
/*	Sender address override is a problem only when delivering
/*	to command or file, or when breaking a Delivered-To loop.
/*	The local(8) delivery agent saves normal recipients to a
/*	new queue file, together with the replacement envelope
/*	sender address; delivery then proceeds from that new queue
/*	file, and no workaround is needed.
/*
/*	The workaround sends one non-delivery notification for each
/*	failed delivery that has a replacement sender address.  The
/*	notifications are not aggregated, unlike notifications to
/*	non-replaced sender addresses. In practice, a local alias
/*	rarely has more than one file or command destination (if
/*	only because soft error handling is problematic).
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, recipient and more.
/*	Attributes describing alias, include or forward expansion.
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. The result is non-zero when
/*	the operation should be tried again. Warnings: malformed
/*	address.
/* BUGS
/*	The proper fix is to record in the bounce logfile an error
/*	return address for each individual recipient. This would
/*	eliminate the need for VERP-specific bounce protocol code,
/*	and would move complexity from the bounce client side to
/*	the bounce server side where it more likely belongs.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <strings.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <split_at.h>

/* Global library. */

#include <mail_params.h>
#include <strip_addr.h>
#include <stringops.h>
#include <bounce.h>
#include <defer.h>
#include <split_addr.h>
#include <canon_addr.h>

/* Application-specific. */

#include "local.h"

int     bounce_workaround(LOCAL_STATE state)
{
    const char *myname = "bounce_workaround";
    VSTRING *canon_owner = 0;
    int     rcpt_stat;

    /*
     * Look up the substitute sender address.
     */
    if (var_ownreq_special) {
	char   *stripped_recipient;
	char   *owner_alias;
	const char *owner_expansion;

#define FIND_OWNER(lhs, rhs, addr) { \
	lhs = concatenate("owner-", addr, (char *) 0); \
	(void) split_at_right(lhs, '@'); \
	rhs = maps_find(alias_maps, lhs, DICT_FLAG_NONE); \
    }

	FIND_OWNER(owner_alias, owner_expansion, state.msg_attr.rcpt.address);
	if (alias_maps->error == 0 && owner_expansion == 0
	    && (stripped_recipient = strip_addr(state.msg_attr.rcpt.address,
						(char **) 0,
						var_rcpt_delim)) != 0) {
	    myfree(owner_alias);
	    FIND_OWNER(owner_alias, owner_expansion, stripped_recipient);
	    myfree(stripped_recipient);
	}
	if (alias_maps->error == 0 && owner_expansion != 0) {
	    canon_owner = canon_addr_internal(vstring_alloc(10),
					      var_exp_own_alias ?
					      owner_expansion : owner_alias);
	    SET_OWNER_ATTR(state.msg_attr, STR(canon_owner), state.level);
	}
	myfree(owner_alias);
	if (alias_maps->error != 0)
	    /* At this point, canon_owner == 0. */
	    return (defer_append(BOUNCE_FLAGS(state.request),
				 BOUNCE_ATTR(state.msg_attr)));
    }

    /*
     * Send a delivery status notification with a single recipient to the
     * substitute sender address, before completion of the delivery request.
     */
    if (canon_owner) {
	rcpt_stat = bounce_one(BOUNCE_FLAGS(state.request),
			       BOUNCE_ONE_ATTR(state.msg_attr));
	vstring_free(canon_owner);
    }

    /*
     * Send a regular delivery status notification, after completion of the
     * delivery request.
     */
    else {
	rcpt_stat = bounce_append(BOUNCE_FLAGS(state.request),
				  BOUNCE_ATTR(state.msg_attr));
    }
    return (rcpt_stat);
}
