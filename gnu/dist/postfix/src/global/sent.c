/*++
/* NAME
/*	sent 3
/* SUMMARY
/*	log that a message was sent
/* SYNOPSIS
/*	#include <sent.h>
/*
/*	int	sent(flags, queue_id, orig_rcpt, recipient, offset, relay,
/*			entry, format, ...)
/*	int	flags;
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	long	offset;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*
/*	int	vsent(flags, queue_id, orig_rcpt, recipient, offset, relay,
/*			entry, format, ap)
/*	int	flags;
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	long	offset;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*	va_list ap;
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
/* .IP DEL_REQ_FLAG_VERIFY
/*	The message is an address verification probe. Update the
/*	address verification database.
/* .IP DEL_REQ_FLAG_EXPAND
/*	The message is an address expansion probe. Update the
/*	message delivery record.
/* .IP DEL_REQ_FLAG_RECORD
/*	This is a normal message with logged delivery. Update the
/*	the message delivery record.
/* .RE
/* .IP queue_id
/*	The message queue id.
/* .IP orig_rcpt
/*	The original envelope recipient address. If unavailable,
/*	specify a null string or a null pointer.
/* .IP recipient
/*	The recipient address.
/* .IP offset
/*	Queue file offset of the recipient record.
/* .IP relay
/*	Name of the host we're talking to.
/* .IP entry
/*	Message arrival time.
/* .IP format
/*	Optional additional information.
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
#include <stdio.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

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

/* Application-specific. */

/* sent - log that a message was sent */

int     sent(int flags, const char *id, const char *orig_rcpt,
	             const char *recipient, long offset, const char *relay,
	             time_t entry, const char *fmt,...)
{
    va_list ap;
    int     status;

    va_start(ap, fmt);
    status = vsent(flags, id, orig_rcpt, recipient,
		   offset, relay, entry, fmt, ap);
    va_end(ap);
    return (status);
}

/* vsent - log that a message was sent */

int     vsent(int flags, const char *id, const char *orig_rcpt,
	              const char *recipient, long offset, const char *relay,
	              time_t entry, const char *fmt, va_list ap)
{
    int     status;

    /*
     * MTA-requested address verification information is stored in the verify
     * service database.
     */
    if (flags & DEL_REQ_FLAG_VERIFY) {
	status = vverify_append(id, orig_rcpt, recipient, relay, entry,
				"deliverable", DEL_RCPT_STAT_OK, fmt, ap);
	return (status);
    }

    /*
     * User-requested address verification information is logged and mailed
     * to the requesting user.
     */
    if (flags & DEL_REQ_FLAG_EXPAND) {
	status = vtrace_append(flags, id, orig_rcpt, recipient, relay,
			       entry, "2.0.0", "deliverable", fmt, ap);
	return (status);
    }

    /*
     * Normal mail delivery. May also send a delivery record to the user.
     */
    else {
	if ((flags & DEL_REQ_FLAG_RECORD) == 0
	    || vtrace_append(flags, id, orig_rcpt, recipient, relay,
			     entry, "2.0.0", "delivered", fmt, ap) == 0) {
	    vlog_adhoc(id, orig_rcpt, recipient, relay,
		       entry, "sent", fmt, ap);
	    status = 0;
	} else {
	    status = defer_append(flags, id, orig_rcpt, recipient, offset,
				  relay, entry, "%s: %s service failed",
				  id, var_trace_service);
	}
	return (status);
    }
}
