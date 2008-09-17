/*++
/* NAME
/*	sent 3
/* SUMMARY
/*	log that a message was or could be sent
/* SYNOPSIS
/*	#include <sent.h>
/*
/*	int	sent(flags, queue_id, stats, recipient, relay, dsn)
/*	int	flags;
/*	const char *queue_id;
/*	MSG_STATS *stats;
/*	RECIPIENT *recipient;
/*	const char *relay;
/*	DSN *dsn;
/* DESCRIPTION
/*	sent() logs that a message was successfully delivered,
/*	updates the address verification service, or updates a
/*	message delivery record on request by the sender. The
/*	flags argument determines the action.
/*
/*	vsent() implements an alternative interface.
/*
/*	Arguments:
/* .IP flags
/*	Zero or more of the following:
/* .RS
/* .IP SENT_FLAG_NONE
/*	The message is a normal delivery request.
/* .IP DEL_REQ_FLAG_MTA_VRFY
/*	The message is an MTA-requested address verification probe.
/*	Update the address verification database.
/* .IP DEL_REQ_FLAG_USR_VRFY
/*	The message is a user-requested address expansion probe.
/*	Update the message delivery record.
/* .IP DEL_REQ_FLAG_RECORD
/*	This is a normal message with logged delivery. Update the
/*	the message delivery record.
/* .RE .IP queue_id
/*	The message queue id.
/* .IP stats
/*	Time stamps from different message delivery stages
/*	and session reuse count.
/* .IP recipient
/*	Recipient information. See recipient_list(3).
/* .IP relay
/*	Name of the host we're talking to.
/* .IP dsn
/*	Delivery status. See dsn(3). The action is ignored in case
/*	of a probe message. Otherwise, "delivered" is assumed when
/*	no action is specified.
/* DIAGNOSTICS
/*	A non-zero result means the operation failed.
/*
/*	Fatal: out of memory.
/* BUGS
/*	Should be replaced by routines with an attribute-value based
/*	interface instead of an interface that uses a rigid argument list.
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <verify.h>
#include <log_adhoc.h>
#include <trace.h>
#include <defer.h>
#include <sent.h>
#include <dsn_util.h>
#include <dsn_mask.h>

/* Application-specific. */

/* sent - log that a message was or could be sent */

int     sent(int flags, const char *id, MSG_STATS *stats,
	             RECIPIENT *recipient, const char *relay,
	             DSN *dsn)
{
    DSN     my_dsn = *dsn;
    int     status;

    /*
     * Sanity check.
     */
    if (my_dsn.status[0] != '2' || !dsn_valid(my_dsn.status)) {
	msg_warn("sent: ignoring dsn code \"%s\"", my_dsn.status);
	my_dsn.status = "2.0.0";
    }

    /*
     * MTA-requested address verification information is stored in the verify
     * service database.
     */
    if (flags & DEL_REQ_FLAG_MTA_VRFY) {
	my_dsn.action = "deliverable";
	status = verify_append(id, stats, recipient, relay, &my_dsn,
			       DEL_RCPT_STAT_OK);
	return (status);
    }

    /*
     * User-requested address verification information is logged and mailed
     * to the requesting user.
     */
    if (flags & DEL_REQ_FLAG_USR_VRFY) {
	my_dsn.action = "deliverable";
	status = trace_append(flags, id, stats, recipient, relay, &my_dsn);
	return (status);
    }

    /*
     * Normal mail delivery. May also send a delivery record to the user.
     */
    else {
	if (my_dsn.action == 0 || my_dsn.action[0] == 0)
	    my_dsn.action = "delivered";

	if (((flags & DEL_REQ_FLAG_RECORD) == 0
	  || trace_append(flags, id, stats, recipient, relay, &my_dsn) == 0)
	    && ((recipient->dsn_notify & DSN_NOTIFY_SUCCESS) == 0
	|| trace_append(flags, id, stats, recipient, relay, &my_dsn) == 0)) {
	    log_adhoc(id, stats, recipient, relay, &my_dsn, "sent");
	    status = 0;
	} else {
	    VSTRING *junk = vstring_alloc(100);

	    vstring_sprintf(junk, "%s: %s service failed",
			    id, var_trace_service);
	    my_dsn.reason = vstring_str(junk);
	    my_dsn.status ="4.3.0";
	    status = defer_append(flags, id, stats, recipient, relay, &my_dsn);
	    vstring_free(junk);
	}
	return (status);
    }
}
