/*++
/* NAME
/*	sent 3
/* SUMMARY
/*	log that a message was sent
/* SYNOPSIS
/*	#include <sent.h>
/*
/*	int	sent(queue_id, orig_rcpt, recipient, relay, entry, format, ...)
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*
/*	int	vsent(queue_id, orig_rcpt, recipient, relay, entry, format, ap)
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *format;
/*	va_list ap;
/* DESCRIPTION
/*	sent() logs that a message was successfully delivered.
/*
/*	vsent() implements an alternative interface.
/*
/*	Arguments:
/* .IP queue_id
/*	The message queue id.
/* .IP orig_rcpt
/*	The original envelope recipient address. If unavailable,
/*	specify a null string or a null pointer.
/* .IP recipient
/*	The recipient address.
/* .IP relay
/*	Name of the host we're talking to.
/* .IP entry
/*	Message arrival time.
/* .IP format
/*	Optional additional information.
/* .PP
/*	For convenience, sent() always returns a zero result.
/* DIAGNOSTICS
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

#include "sent.h"

/* sent - log that a message was sent */

int     sent(const char *queue_id, const char *orig_rcpt,
	             const char *recipient, const char *relay,
	             time_t entry, const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vsent(queue_id, orig_rcpt, recipient, relay, entry, fmt, ap);
    va_end(ap);
    return (0);
}

/* vsent - log that a message was sent */

int     vsent(const char *queue_id, const char *orig_rcpt,
	              const char *recipient, const char *relay,
	              time_t entry, const char *fmt, va_list ap)
{
#define TEXT (vstring_str(text))
    VSTRING *text = vstring_alloc(100);
    int     delay = time((time_t *) 0) - entry;

    vstring_vsprintf(text, fmt, ap);
    if (orig_rcpt && *orig_rcpt && strcasecmp(recipient, orig_rcpt) != 0)
	msg_info("%s: to=<%s>, orig_to=<%s>, relay=%s, delay=%d, status=sent%s%s%s",
		 queue_id, recipient, orig_rcpt, relay, delay,
		 *TEXT ? " (" : "", TEXT, *TEXT ? ")" : "");
    else
	msg_info("%s: to=<%s>, relay=%s, delay=%d, status=sent%s%s%s",
		 queue_id, recipient, relay, delay,
		 *TEXT ? " (" : "", TEXT, *TEXT ? ")" : "");
    vstring_free(text);
    return (0);
}
