/*++
/* NAME
/*	bounce 3
/* SUMMARY
/*	bounce service client
/* SYNOPSIS
/*	#include <bounce.h>
/*
/*	int	bounce_append(flags, id, orig_rcpt, recipient, relay,
/*				entry, format, ...)
/*	int	flags;
/*	const char *id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*
/*	int	vbounce_append(flags, id, orig_rcpt, recipient, relay,
/*				entry, format, ap)
/*	int	flags;
/*	const char *id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*	va_list ap;
/*
/*	int	bounce_flush(flags, queue, id, encoding, sender)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*
/*	int	bounce_one(flags, queue, id, encoding, sender, orig_rcpt,
/*				recipient, relay, entry, format, ...)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*
/*	int	vbounce_one(flags, queue, id, encoding, sender, orig_rcpt,
/*				recipient, relay, entry, format, ap)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*	va_list ap;
/* DESCRIPTION
/*	This module implements the client interface to the message
/*	bounce service, which maintains a per-message log of status
/*	records with recipients that were bounced, and the reason why.
/*
/*	bounce_append() appends a reason for non-delivery to the
/*	bounce log for the named recipient.
/*
/*	vbounce_append() implements an alternative interface.
/*
/*	bounce_flush() actually bounces the specified message to
/*	the specified sender, including the bounce log that was
/*	built with bounce_append().
/*
/*	bounce_one() bounces one recipient and immediately sends a
/*	notification to the sender. This procedure does not append
/*	the recipient and reason to the per-message bounce log, and
/*	should be used when a delivery agent changes the error
/*	return address in a manner that depends on the recipient
/*	address.
/*
/*	vbounce_one() implements an alternative interface.
/*
/*	Arguments:
/* .IP flags
/*	The bitwise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to request no special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the bounce log in case of an error (as in: pretend
/*	that we never even tried to bounce this message).
/* .RE
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The message queue id if the original message file. The bounce log
/*	file has the same name as the original message file.
/* .IP encoding
/*	The body content encoding: MAIL_ATTR_ENC_{7BIT,8BIT,NONE}.
/* .IP sender
/*	The sender envelope address.
/* .IP relay
/*	Name of the host that the message could not be delivered to.
/*	This information is used for syslogging only.
/* .IP entry
/*	Message arrival time.
/* .IP orig_rcpt
/*	The original envelope recipient address. If unavailable,
/*	specify a null string or null pointer.
/* .IP recipient
/*	Recipient address that the message could not be delivered to.
/*	This information is used for syslogging only.
/* .IP format
/*	The reason for non-delivery.
/* .IP ap
/*	Variable-length argument list.
/* DIAGNOSTICS
/*	In case of success, these functions log the action, and return a
/*	zero value. Otherwise, the functions return a non-zero result,
/*	and when BOUNCE_FLAG_CLEAN is disabled, log that message
/*	delivery is deferred.
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
#include <time.h>
#include <stdarg.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include "mail_params.h"
#include "mail_proto.h"
#include "defer.h"
#include "bounce.h"

/* bounce_append - append reason to per-message bounce log */

int     bounce_append(int flags, const char *id, const char *orig_rcpt,
		              const char *recipient, const char *relay,
		              time_t entry, const char *fmt,...)
{
    va_list ap;
    int     status;

    va_start(ap, fmt);
    status = vbounce_append(flags, id, orig_rcpt, recipient,
			    relay, entry, fmt, ap);
    va_end(ap);
    return (status);
}

/* vbounce_append - append bounce reason to per-message log */

int     vbounce_append(int flags, const char *id, const char *orig_rcpt,
		               const char *recipient, const char *relay,
		               time_t entry, const char *fmt, va_list ap)
{
    VSTRING *why;
    int     status;
    int     delay;

    /*
     * When we're pretending that we can't bounce, don't create a defer log
     * file when we wouldn't keep the bounce log file. That's a lot of
     * negatives in one sentence.
     */
    if (var_soft_bounce && (flags & BOUNCE_FLAG_CLEAN))
	return (-1);

    why = vstring_alloc(100);
    delay = time((time_t *) 0) - entry;
    vstring_vsprintf(why, fmt, ap);
    if (orig_rcpt == 0)
	orig_rcpt = "";
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_soft_bounce ?
			    var_defer_service : var_bounce_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_APPEND,
			    ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ORCPT, orig_rcpt,
			    ATTR_TYPE_STR, MAIL_ATTR_RECIP, recipient,
			    ATTR_TYPE_STR, MAIL_ATTR_WHY, vstring_str(why),
			    ATTR_TYPE_END) == 0) {
	if (*orig_rcpt && strcasecmp(recipient, orig_rcpt) != 0)
	    msg_info("%s: to=<%s>, orig_to=<%s>, relay=%s, delay=%d, status=%s (%s%s)",
		     id, recipient, orig_rcpt, relay, delay,
		     var_soft_bounce ? "deferred" : "bounced",
		     var_soft_bounce ? "SOFT BOUNCE - " : "",
		     vstring_str(why));
	else
	    msg_info("%s: to=<%s>, relay=%s, delay=%d, status=%s (%s%s)",
		     id, recipient, relay, delay,
		     var_soft_bounce ? "deferred" : "bounced",
		     var_soft_bounce ? "SOFT BOUNCE - " : "",
		     vstring_str(why));
	status = (var_soft_bounce ? -1 : 0);
    } else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	status = defer_append(flags, id, orig_rcpt, recipient, "bounce", entry,
			      "bounce failed");
    } else {
	status = -1;
    }
    vstring_free(why);
    return (status);
}

/* bounce_flush - flush the bounce log and deliver to the sender */

int     bounce_flush(int flags, const char *queue, const char *id,
		             const char *encoding, const char *sender)
{

    /*
     * When we're pretending that we can't bounce, don't send a bounce
     * message.
     */
    if (var_soft_bounce)
	return (-1);
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_bounce_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_FLUSH,
			    ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	msg_info("%s: status=deferred (bounce failed)", id);
	return (-1);
    } else {
	return (-1);
    }
}

/* bounce_one - send notice for one recipient */

int     bounce_one(int flags, const char *queue, const char *id,
		           const char *encoding, const char *sender,
		           const char *orig_rcpt, const char *recipient,
		           const char *relay, time_t entry,
		           const char *fmt,...)
{
    va_list ap;
    int     status;

    va_start(ap, fmt);
    status = vbounce_one(flags, queue, id, encoding, sender, orig_rcpt,
			 recipient, relay, entry, fmt, ap);
    va_end(ap);
    return (status);
}

/* vbounce_one - send notice for one recipient */

int     vbounce_one(int flags, const char *queue, const char *id,
		            const char *encoding, const char *sender,
		            const char *orig_rcpt, const char *recipient,
		            const char *relay, time_t entry,
		            const char *fmt, va_list ap)
{
    VSTRING *why;
    int     status;
    int     delay;

    /*
     * When we're not bouncing, then use the standard logfile based
     * procedure.
     */
    if (var_soft_bounce)
	return (vbounce_append(flags, id, orig_rcpt, recipient,
			       relay, entry, fmt, ap));

    why = vstring_alloc(100);
    delay = time((time_t *) 0) - entry;
    vstring_vsprintf(why, fmt, ap);
    if (orig_rcpt == 0)
	orig_rcpt = "";
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_bounce_service,
			    ATTR_TYPE_NUM, MAIL_ATTR_NREQ, BOUNCE_CMD_ONE,
			    ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_ORCPT, orig_rcpt,
			    ATTR_TYPE_STR, MAIL_ATTR_RECIP, recipient,
			    ATTR_TYPE_STR, MAIL_ATTR_WHY, vstring_str(why),
			    ATTR_TYPE_END) == 0) {
	if (*orig_rcpt && strcasecmp(recipient, orig_rcpt) != 0)
	    msg_info("%s: to=<%s>, orig_to=<%s>, relay=%s, delay=%d, status=bounced (%s)",
		  id, recipient, orig_rcpt, relay, delay, vstring_str(why));
	else
	    msg_info("%s: to=<%s>, relay=%s, delay=%d, status=bounced (%s)",
		     id, recipient, relay, delay, vstring_str(why));
	status = 0;
    } else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	status = defer_append(flags, id, orig_rcpt, recipient, "bounce", entry,
			      "bounce failed");
    } else {
	status = -1;
    }
    vstring_free(why);
    return (status);
}
