/*	$NetBSD: unknown.c,v 1.1.1.6.12.2 2007/08/02 08:05:17 heas Exp $	*/

/*++
/* NAME
/*	unknown 3
/* SUMMARY
/*	delivery of unknown recipients
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	deliver_unknown(state, usr_attr)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/* DESCRIPTION
/*	deliver_unknown() delivers a message for unknown recipients.
/* .IP \(bu
/*	If an alternative message transport is specified via the
/*	fallback_transport parameter, delivery is delegated to the
/*	named transport.
/* .IP \(bu
/*	If an alternative address is specified via the luser_relay
/*	configuration parameter, mail is forwarded to that address.
/* .IP \(bu
/*	Otherwise the recipient is bounced.
/* .PP
/*	The luser_relay parameter is subjected to $name expansion of
/*	the standard message attributes: $user, $home, $shell, $domain,
/*	$recipient, $mailbox, $extension, $recipient_delimiter, not
/*	all of which actually make sense.
/*
/*	Arguments:
/* .IP state
/*	Message delivery attributes (sender, recipient etc.).
/*	Attributes describing alias, include or forward expansion.
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* .IP usr_attr
/*	Attributes describing user rights and environment.
/* DIAGNOSTICS
/*	The result status is non-zero when delivery should be tried again.
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
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <stringops.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <been_here.h>
#include <mail_params.h>
#include <mail_proto.h>
#include <bounce.h>
#include <mail_addr.h>
#include <sent.h>
#include <deliver_pass.h>

/* Application-specific. */

#include "local.h"

/* deliver_unknown - delivery for unknown recipients */

int     deliver_unknown(LOCAL_STATE state, USER_ATTR usr_attr)
{
    const char *myname = "deliver_unknown";
    int     status;
    VSTRING *expand_luser;
    static MAPS *transp_maps;
    const char *map_transport;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * DUPLICATE/LOOP ELIMINATION
     * 
     * Don't deliver the same user twice.
     */
    if (been_here(state.dup_filter, "%s %s", myname, state.msg_attr.local))
	return (0);

    /*
     * The fall-back transport specifies a delivery machanism that handles
     * users not found in the aliases or UNIX passwd databases.
     */
    if (*var_fbck_transp_maps && transp_maps == 0)
	transp_maps = maps_create(VAR_FBCK_TRANSP_MAPS, var_fbck_transp_maps,
				  DICT_FLAG_LOCK | DICT_FLAG_NO_REGSUB);
    /* The -1 is a hint for the down-stream deliver_completed() function. */
    if (*var_fbck_transp_maps
	&& (map_transport = maps_find(transp_maps, state.msg_attr.user,
				      DICT_FLAG_NONE)) != 0) {
	state.msg_attr.rcpt.offset = -1L;
	return (deliver_pass(MAIL_CLASS_PRIVATE, map_transport,
			     state.request, &state.msg_attr.rcpt));
    }
    if (*var_fallback_transport) {
	state.msg_attr.rcpt.offset = -1L;
	return (deliver_pass(MAIL_CLASS_PRIVATE, var_fallback_transport,
			     state.request, &state.msg_attr.rcpt));
    }

    /*
     * Subject the luser_relay address to $name expansion, disable
     * propagation of unmatched address extension, and re-inject the address
     * into the delivery machinery. Do not give special treatment to "|stuff"
     * or /stuff.
     */
    if (*var_luser_relay) {
	state.msg_attr.unmatched = 0;
	expand_luser = vstring_alloc(100);
	local_expand(expand_luser, var_luser_relay, &state, &usr_attr, (char *) 0);
	status = deliver_resolve_addr(state, usr_attr, STR(expand_luser));
	vstring_free(expand_luser);
	return (status);
    }

    /*
     * If no alias was found for a required reserved name, toss the message
     * into the bit bucket, and issue a warning instead.
     */
#define STREQ(x,y) (strcasecmp(x,y) == 0)

    if (STREQ(state.msg_attr.local, MAIL_ADDR_MAIL_DAEMON)
	|| STREQ(state.msg_attr.local, MAIL_ADDR_POSTMASTER)) {
	msg_warn("required alias not found: %s", state.msg_attr.local);
	dsb_simple(state.msg_attr.why, "2.0.0", "discarded");
	return (sent(BOUNCE_FLAGS(state.request), SENT_ATTR(state.msg_attr)));
    }

    /*
     * Bounce the message when no luser relay is specified.
     */
    dsb_simple(state.msg_attr.why, "5.1.1",
	       "unknown user: \"%s\"", state.msg_attr.local);
    return (bounce_append(BOUNCE_FLAGS(state.request),
			  BOUNCE_ATTR(state.msg_attr)));
}
