/*++
/* NAME
/*	defer 3
/* SUMMARY
/*	defer service client interface
/* SYNOPSIS
/*	#include <defer.h>
/*
/*	int	defer_append(flags, id, orig_rcpt, recipient, offset, relay,
/*				entry, format, ...)
/*	int	flags;
/*	const char *id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	long	offset;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*
/*	int	vdefer_append(flags, id, orig_rcpt, recipient, offset, relay,
/*				entry, format, ap)
/*	int	flags;
/*	const char *id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	long	offset;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*	va_list	ap;
/*
/*	int	defer_flush(flags, queue, id, encoding, sender)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*
/*	int	defer_warn(flags, queue, id, sender)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *sender;
/* DESCRIPTION
/*	This module implements a client interface to the defer service,
/*	which maintains a per-message logfile with status records for
/*	each recipient whose delivery is deferred, and the reason why.
/*
/*	defer_append() appends a record to the per-message defer log,
/*	with the reason for delayed delivery to the named recipient,
/*	updates the address verification service, or updates a message
/*	delivery record on request by the sender. The flags argument
/*	determines the action.
/*	The result is a convenient non-zero value.
/*	When the fast flush cache is enabled, the fast flush server is
/*	notified of deferred mail.
/*
/*	vdefer_append() implements an alternative client interface.
/*
/*	defer_flush() bounces the specified message to the specified
/*	sender, including the defer log that was built with defer_append().
/*	defer_flush() requests that the deferred recipients are deleted
/*	from the original queue file.
/*	The result is zero in case of success, non-zero otherwise.
/*
/*	defer_warn() sends a warning message that the mail in question has
/*	been deferred.  It does not flush the log.
/*
/*	Arguments:
/* .IP flags
/*	The bit-wise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to explicitly request not special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the defer log in case of an error (as in: pretend
/*	that we never even tried to defer this message).
/* .IP DEL_REQ_FLAG_VERIFY
/*	The message is an address verification probe. Update the
/*	address verification database instead of deferring mail.
/* .IP DEL_REQ_FLAG_EXPAND
/*	The message is an address expansion probe. Update the
/*	the message delivery record instead of deferring mail.
/* .IP DEL_REQ_FLAG_RECORD
/*	This is a normal message with logged delivery. Update the
/*	message delivery record and defer mail delivery.
/* .RE
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The queue id of the original message file.
/* .IP orig_rcpt
/*	The original envelope recipient address. If unavailable,
/*	specify a null string or null pointer.
/* .IP recipient
/*	A recipient address that is being deferred. The domain part
/*	of the address is marked dead (for a limited amount of time).
/* .IP offset
/*	Queue file offset of recipient record.
/* .IP encoding
/*	The body content encoding: MAIL_ATTR_ENC_{7BIT,8BIT,NONE}.
/* .IP sender
/*	The sender envelope address.
/* .IP relay
/*	Host we could not talk to.
/* .IP entry
/*	Message arrival time.
/* .IP format
/*	The reason for non-delivery.
/* .IP ap
/*	Variable-length argument list.
/* .PP
/*	For convenience, these functions always return a non-zero result.
/* DIAGNOSTICS
/*	Warnings: problems connecting to the defer service.
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
#include <mail_queue.h>
#include <mail_proto.h>
#include <flush_clnt.h>
#include <verify.h>
#include <log_adhoc.h>
#include <trace.h>
#include <bounce.h>
#include <defer.h>

#define STR(x)	vstring_str(x)

/* defer_append - defer message delivery */

int     defer_append(int flags, const char *id, const char *orig_rcpt,
	              const char *recipient, long offset, const char *relay,
		             time_t entry, const char *fmt,...)
{
    va_list ap;
    int     status;

    va_start(ap, fmt);
    status = vdefer_append(flags, id, orig_rcpt, recipient,
			   offset, relay, entry, fmt, ap);
    va_end(ap);
    return (status);
}

/* vdefer_append - defer delivery of queue file */

int     vdefer_append(int flags, const char *id, const char *orig_rcpt,
	              const char *recipient, long offset, const char *relay,
		              time_t entry, const char *fmt, va_list ap)
{
    const char *rcpt_domain;
    int     status;

    /*
     * MTA-requested address verification information is stored in the verify
     * service database.
     */
    if (flags & DEL_REQ_FLAG_VERIFY) {
	status = vverify_append(id, orig_rcpt, recipient, relay, entry,
			     "undeliverable", DEL_RCPT_STAT_DEFER, fmt, ap);
	return (status);
    }

    /*
     * User-requested address verification information is logged and mailed
     * to the requesting user.
     */
    if (flags & DEL_REQ_FLAG_EXPAND) {
	status = vtrace_append(flags, id, orig_rcpt, recipient, relay,
			       entry, "4.0.0", "undeliverable", fmt, ap);
	return (status);
    }

    /*
     * Normal mail delivery. May also send a delivery record to the user.
     */
    else {
	VSTRING *why = vstring_alloc(100);

	vstring_vsprintf(why, fmt, ap);

	if (orig_rcpt == 0)
	    orig_rcpt = "";
	if (mail_command_client(MAIL_CLASS_PRIVATE, var_defer_service,
			   ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_APPEND,
				ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
				ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
				ATTR_TYPE_STR, MAIL_ATTR_ORCPT, orig_rcpt,
				ATTR_TYPE_STR, MAIL_ATTR_RECIP, recipient,
				ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, offset,
				ATTR_TYPE_STR, MAIL_ATTR_STATUS, "4.0.0",
				ATTR_TYPE_STR, MAIL_ATTR_ACTION, "delayed",
			     ATTR_TYPE_STR, MAIL_ATTR_WHY, vstring_str(why),
				ATTR_TYPE_END) != 0)
	    msg_warn("%s: %s service failure", id, var_defer_service);
	vlog_adhoc(id, orig_rcpt, recipient, relay, entry, "deferred", fmt, ap);

	/*
	 * Traced delivery.
	 */
	if (flags & DEL_REQ_FLAG_RECORD)
	    if (vtrace_append(flags, id, orig_rcpt, recipient, relay,
			      entry, "4.0.0", "deferred", fmt, ap) != 0)
		msg_warn("%s: %s service failure", id, var_trace_service);

	/*
	 * Notify the fast flush service. XXX Should not this belong in the
	 * bounce/defer daemon? Well, doing it here is more robust.
	 */
	if ((rcpt_domain = strrchr(recipient, '@')) != 0 && *++rcpt_domain != 0)
	    switch (flush_add(rcpt_domain, id)) {
	    case FLUSH_STAT_OK:
	    case FLUSH_STAT_DENY:
		break;
	    default:
		msg_warn("%s: %s service failure", id, var_flush_service);
		break;
	    }
	vstring_free(why);
	return (-1);
    }
}

/* defer_flush - flush the defer log and deliver to the sender */

int     defer_flush(int flags, const char *queue, const char *id,
		            const char *encoding, const char *sender)
{
    flags |= BOUNCE_FLAG_DELRCPT;

    if (mail_command_client(MAIL_CLASS_PRIVATE, var_defer_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_FLUSH,
			    ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else {
	return (-1);
    }
}

/* defer_warn - send a copy of the defer log to the sender as a warning bounce
 * do not flush the log */

int     defer_warn(int flags, const char *queue, const char *id,
		           const char *sender)
{
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_defer_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_WARN,
			    ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else {
	return (-1);
    }
}
