/*++
/* NAME
/*	recipient 3
/* SUMMARY
/*	deliver to one local recipient
/* SYNOPSIS
/*	#include "virtual.h"
/*
/*	int	deliver_recipient(state, usr_attr)
/*	LOCAL_STATE state;
/*	USER_ATTR *usr_attr;
/* DESCRIPTION
/*	deliver_recipient() delivers a message to a local recipient.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, sender, and more.
/* .IP usr_attr
/*	Attributes describing user rights and mailbox location.
/* DIAGNOSTICS
/*	deliver_recipient() returns non-zero when delivery should be
/*	tried again.
/* SEE ALSO
/*	mailbox(3) delivery to UNIX-style mailbox
/*	maildir(3) delivery to qmail-style maildir
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
#include <stringops.h>

/* Global library. */

#include <bounce.h>

/* Application-specific. */

#include "virtual.h"

/* deliver_recipient - deliver one local recipient */

int     deliver_recipient(LOCAL_STATE state, USER_ATTR usr_attr)
{
    const char *myname = "deliver_recipient";
    int     rcpt_stat;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * Set up the recipient-specific attributes. The recipient's lookup
     * handle is the full address.
     */
    if (state.msg_attr.delivered == 0)
	state.msg_attr.delivered = state.msg_attr.rcpt.address;
    state.msg_attr.user = mystrdup(state.msg_attr.rcpt.address);
    lowercase(state.msg_attr.user);

    /*
     * Deliver
     */
    if (msg_verbose)
	deliver_attr_dump(&state.msg_attr);

    if (deliver_mailbox(state, usr_attr, &rcpt_stat) == 0)
	rcpt_stat = deliver_unknown(state);

    /*
     * Cleanup.
     */
    myfree(state.msg_attr.user);

    return (rcpt_stat);
}
