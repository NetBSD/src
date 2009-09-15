/*	$NetBSD: trace.c,v 1.1.1.1.2.2 2009/09/15 06:02:55 snj Exp $	*/

/*++
/* NAME
/*	trace 3
/* SUMMARY
/*	user requested delivery tracing
/* SYNOPSIS
/*	#include <trace.h>
/*
/*	int	trace_append(flags, id, stats, rcpt, relay, dsn)
/*	int	flags;
/*	const char *id;
/*	MSG_STATS *stats;
/*	RECIPIENT *rcpt;
/*	const char *relay;
/*	DSN	*dsn;
/*
/*	int     trace_flush(flags, queue, id, encoding, sender,
/*				dsn_envid, dsn_ret)
/*	int     flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/* DESCRIPTION
/*	trace_append() updates the message delivery record that is
/*	mailed back to the originator. In case of a trace-only
/*	message, the recipient status is also written to the
/*	mailer logfile.
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
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The message queue id.
/* .IP encoding
/*	The body content encoding: MAIL_ATTR_ENC_{7BIT,8BIT,NONE}.
/* .IP sender
/*	The sender envelope address.
/* .IP dsn_envid
/*	Optional DSN envelope ID.
/* .IP dsn_ret
/*	Optional DSN return full/headers option.
/* .IP stats
/*	Time stamps from different message delivery stages
/*	and session reuse count.
/* .IP rcpt
/*	Recipient information. See recipient_list(3).
/* .IP relay
/*	The host we sent the mail to.
/* .IP dsn
/*	Delivery status information. See dsn(3).
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <log_adhoc.h>
#include <rcpt_print.h>
#include <dsn_print.h>
#include <trace.h>

/* trace_append - append to message delivery record */

int     trace_append(int flags, const char *id, MSG_STATS *stats,
		             RECIPIENT *rcpt, const char *relay,
		             DSN *dsn)
{
    VSTRING *why = vstring_alloc(100);
    DSN     my_dsn = *dsn;
    int     req_stat;

    /*
     * User-requested address verification, verbose delivery, or DSN SUCCESS
     * notification.
     */
    if (strcmp(relay, NO_RELAY_AGENT) != 0)
	vstring_sprintf(why, "delivery via %s: ", relay);
    vstring_strcat(why, my_dsn.reason);
    my_dsn.reason = vstring_str(why);

    if (mail_command_client(MAIL_CLASS_PRIVATE, var_trace_service,
			    ATTR_TYPE_INT, MAIL_ATTR_NREQ, BOUNCE_CMD_APPEND,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_FUNC, rcpt_print, (void *) rcpt,
			    ATTR_TYPE_FUNC, dsn_print, (void *) &my_dsn,
			    ATTR_TYPE_END) != 0) {
	msg_warn("%s: %s service failure", id, var_trace_service);
	req_stat = -1;
    } else {
	if (flags & DEL_REQ_FLAG_USR_VRFY)
	    log_adhoc(id, stats, rcpt, relay, dsn, my_dsn.action);
	req_stat = 0;
    }
    vstring_free(why);
    return (req_stat);
}

/* trace_flush - deliver delivery record to the sender */

int     trace_flush(int flags, const char *queue, const char *id,
		            const char *encoding, const char *sender,
		            const char *dsn_envid, int dsn_ret)
{
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_trace_service,
			    ATTR_TYPE_INT, MAIL_ATTR_NREQ, BOUNCE_CMD_TRACE,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
			    ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, dsn_ret,
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else {
	return (-1);
    }
}
