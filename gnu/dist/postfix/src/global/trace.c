/*++
/* NAME
/*	trace 3
/* SUMMARY
/*	user requested delivery tracing
/* SYNOPSIS
/*	#include <trace.h>
/*
/*	int	trace_append(flags, queue_id, orig_rcpt, recipient, relay,
/*				entry, dsn_code, dsn_action, format, ...)
/*	int	flags;
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *dsn_code;
/*	const char *dsn_action;
/*	const char *format;
/*
/*	int	vtrace_append(flags, queue_id, orig_rcpt, recipient, relay,
/*				entry, dsn_code, dsn_action, format, ap)
/*	int	flags;
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *dsn_code;
/*	const char *dsn_action;
/*	const char *format;
/*	va_list ap;
/*
/*	int     trace_flush(flags, queue, id, encoding, sender)
/*	int     flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/* DESCRIPTION
/*	trace_append() updates the message delivery record that is
/*	mailed back to the originator. In case of a trace-only
/*	message, the recipient status is also written to the
/*	mailer logfile.
/*
/*	vtrace_append() implements an alternative interface.
/*
/*	trace_flush() returns the specified message to the specified
/*	sender, including the message delivery record log that was built
/*	with vtrace_append().
/*
/*	Arguments:
/* .IP flags
/*	The bitwise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to request no special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the logfile in case of an error (as in: pretend
/*	that we never even tried to deliver this message).
/* .RE
/* .IP queue_id
/*	The message queue id.
/* .IP orig_rcpt
/*	The original envelope recipient address. If unavailable,
/*	specify a null string or a null pointer.
/* .IP recipient
/*	The recipient address.
/* .IP relay
/*	The host we sent the mail to.
/* .IP entry
/*	Message arrival time.
/* .IP dsn_code
/*	three-digit dot-separated code.
/* .IP dsn_action
/*	"deliverable", "undeliverable", and so on.
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
#include <mail_proto.h>
#include <verify_clnt.h>
#include <log_adhoc.h>
#include <bounce.h>
#include <trace.h>

/* trace_append - append to message delivery record */

int     trace_append(int flags, const char *queue_id,
		             const char *orig_rcpt, const char *recipient,
		             const char *relay, time_t entry,
		             const char *dsn_code, const char *dsn_action,
		             const char *fmt,...)
{
    va_list ap;
    int     req_stat;

    va_start(ap, fmt);
    req_stat = vtrace_append(flags, queue_id, orig_rcpt, recipient,
			     relay, entry, dsn_code, dsn_action, fmt, ap);
    va_end(ap);
    return (req_stat);
}

/* vtrace_append - append to message delivery record */

int     vtrace_append(int flags, const char *queue_id,
		              const char *orig_rcpt, const char *recipient,
		              const char *relay, time_t entry,
		              const char *dsn_code, const char *dsn_action,
		              const char *fmt, va_list ap)
{
    VSTRING *why = vstring_alloc(100);
    int     req_stat;

    /*
     * User-requested address verification or verbose delivery. Mail the
     * report to the requesting user.
     */
    vstring_sprintf(why, "delivery via %s: ", relay);
    vstring_vsprintf_append(why, fmt, ap);

    if (orig_rcpt == 0)
	orig_rcpt = "";
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_trace_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_APPEND,
			    ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			    ATTR_TYPE_STR, MAIL_ATTR_ORCPT, orig_rcpt,
			    ATTR_TYPE_STR, MAIL_ATTR_RECIP, recipient,
			    ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, (long) 0,
			    ATTR_TYPE_STR, MAIL_ATTR_STATUS, dsn_code,
			    ATTR_TYPE_STR, MAIL_ATTR_ACTION, dsn_action,
			    ATTR_TYPE_STR, MAIL_ATTR_WHY, vstring_str(why),
			    ATTR_TYPE_END) != 0) {
	msg_warn("%s: %s service failure", queue_id, var_trace_service);
	req_stat = -1;
    } else {
	if (flags & DEL_REQ_FLAG_EXPAND)
	    vlog_adhoc(queue_id, orig_rcpt, recipient, relay,
		       entry, dsn_action, fmt, ap);
	req_stat = 0;
    }
    vstring_free(why);
    return (req_stat);
}

/* trace_flush - deliver delivery record to the sender */

int     trace_flush(int flags, const char *queue, const char *id,
		            const char *encoding, const char *sender)
{
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_trace_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_TRACE,
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
