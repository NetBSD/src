/*++
/* NAME
/*	unknown 3
/* SUMMARY
/*	delivery of unknown recipients
/* SYNOPSIS
/*	#include "virtual.h"
/*
/*	int	deliver_unknown(state)
/*	LOCAL_STATE state;
/* DESCRIPTION
/*	deliver_unknown() delivers a message for unknown recipients.
/* .PP
/*	Arguments:
/* .IP state
/*	Message delivery attributes (sender, recipient etc.).
/* .IP usr_attr
/*	Attributes describing user rights and mailbox location.
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

/* Utility library. */

#include <msg.h>

/* Global library. */

#include <bounce.h>

/* Application-specific. */

#include "virtual.h"

/* deliver_unknown - delivery for unknown recipients */

int     deliver_unknown(LOCAL_STATE state)
{
    char   *myname = "deliver_unknown";

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    return (bounce_append(BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
			  "unknown user: \"%s\"", state.msg_attr.user));

}
