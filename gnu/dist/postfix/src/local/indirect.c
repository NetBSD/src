/*++
/* NAME
/*	indirect 3
/* SUMMARY
/*	indirect delivery
/* SYNOPSIS
/*	#include "local.h"
/*
/*	void	deliver_indirect(state)
/*	LOCAL_STATE state;
/*	char	*recipient;
/* DESCRIPTION
/*	deliver_indirect() delivers a message via the message
/*	forwarding service, with duplicate filtering up to a
/*	configurable number of recipients.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, sender and more.
/*	A table with the results from expanding aliases or lists.
/* CONFIGURATION VARIABLES
/*	duplicate_filter_limit, duplicate filter size limit
/* DIAGNOSTICS
/*	The result is non-zero when the operation should be tried again.
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

/* Utility library. */

#include <msg.h>
#include <htable.h>

/* Global library. */

#include <mail_params.h>
#include <bounce.h>
#include <defer.h>
#include <been_here.h>
#include <sent.h>

/* Application-specific. */

#include "local.h"

/* deliver_indirect - deliver mail via forwarding service */

int     deliver_indirect(LOCAL_STATE state)
{

    /*
     * Suppress duplicate expansion results. Add some sugar to the name to
     * avoid collisions with other duplicate filters. Allow the user to
     * specify an upper bound on the size of the duplicate filter, so that we
     * can handle huge mailing lists with millions of recipients.
     */
    if (msg_verbose)
	msg_info("deliver_indirect: %s", state.msg_attr.recipient);
    if (been_here(state.dup_filter, "indirect %s", state.msg_attr.recipient))
	return (0);

    /*
     * Don't forward a trace-only request.
     */
    if (DEL_REQ_TRACE_ONLY(state.request->flags))
	return (sent(BOUNCE_FLAGS(state.request),
		     SENT_ATTR(state.msg_attr),
		     "forwards to %s", state.msg_attr.recipient));

    /*
     * Send the address to the forwarding service. Inherit the delivered
     * attribute from the alias or from the .forward file owner.
     */
    if (forward_append(state.msg_attr))
	return (defer_append(BOUNCE_FLAGS(state.request),
			     BOUNCE_ATTR(state.msg_attr),
			     "unable to forward message"));
    return (0);
}
