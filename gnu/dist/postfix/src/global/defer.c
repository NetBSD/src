/*++
/* NAME
/*	defer 3
/* SUMMARY
/*	defer service client interface
/* SYNOPSIS
/*	#include <defer.h>
/*
/*	int	defer_append(flags, id, recipient, relay, entry, format, ...)
/*	int	flags;
/*	const char *id;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*
/*	int	vdefer_append(flags, id, recipient, relay, entry, format, ap)
/*	int	flags;
/*	const char *id;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*	va_list	ap;
/*
/*	int	defer_flush(flags, queue, id, sender)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
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
/*	with the reason for delayed delivery to the named recipient.
/*	The result is a convenient non-zero value.
/*	When the fast flush cache is enabled, the fast flush server is
/*	notified of deferred mail.
/*
/*	vdefer_append() implements an alternative client interface.
/*
/*	defer_flush() bounces the specified message to the specified
/*	sender, including the defer log that was built with defer_append().
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
/* .RE
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The queue id of the original message file.
/* .IP recipient
/*	A recipient address that is being deferred. The domain part
/*	of the address is marked dead (for a limited amount of time).
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include "mail_queue.h"
#include "mail_proto.h"
#include "flush_clnt.h"
#include "bounce.h"
#include "defer.h"

#define STR(x)	vstring_str(x)

/* defer_append - defer message delivery */

int     defer_append(int flags, const char *id, const char *recipient,
	               const char *relay, time_t entry, const char *fmt,...)
{
    va_list ap;
    int     status;

    va_start(ap, fmt);
    status = vdefer_append(flags, id, recipient, relay, entry, fmt, ap);
    va_end(ap);
    return (status);
}

/* vdefer_append - defer delivery of queue file */

int     vdefer_append(int flags, const char *id, const char *recipient,
               const char *relay, time_t entry, const char *fmt, va_list ap)
{
    VSTRING *why = vstring_alloc(100);
    int     delay = time((time_t *) 0) - entry;
    const char *rcpt_domain;

    vstring_vsprintf(why, fmt, ap);
    if (mail_command_write(MAIL_CLASS_PRIVATE, MAIL_SERVICE_DEFER,
			   "%d %d %s %s %s", BOUNCE_CMD_APPEND,
			   flags, id, recipient, vstring_str(why)) != 0)
	msg_warn("%s: defer service failure", id);
    msg_info("%s: to=<%s>, relay=%s, delay=%d, status=deferred (%s)",
	     id, recipient, relay, delay, vstring_str(why));
    vstring_free(why);

    /*
     * Notify the fast flush service. XXX Should not this belong in the
     * bounce/defer daemon? Well, doing it here is more robust.
     */
    if ((rcpt_domain = strrchr(recipient, '@')) != 0 && *++rcpt_domain != 0)
	if (flush_add(rcpt_domain, id) != FLUSH_STAT_OK)
	    msg_warn("unable to talk to fast flush service");

    return (-1);
}

/* defer_flush - flush the defer log and deliver to the sender */

int     defer_flush(int flags, const char *queue, const char *id,
		            const char *sender)
{
    if (mail_command_write(MAIL_CLASS_PRIVATE, MAIL_SERVICE_DEFER,
			   "%d %d %s %s %s", BOUNCE_CMD_FLUSH,
			   flags, queue, id, sender) == 0) {
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
    if (mail_command_write(MAIL_CLASS_PRIVATE, MAIL_SERVICE_DEFER,
			   "%d %d %s %s %s", BOUNCE_CMD_WARN,
			   flags, queue, id, sender) == 0) {
	return (0);
    } else {
	return (-1);
    }
}
