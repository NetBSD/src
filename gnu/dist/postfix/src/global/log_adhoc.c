/*++
/* NAME
/*	log_adhoc 3
/* SUMMARY
/*	ad-hoc delivery event logging
/* SYNOPSIS
/*	#include <log_adhoc.h>
/*
/*	void	log_adhoc(id, orig_rcpt, recipient, relay,
/*				entry, status, format, ...)
/*	const char *id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *status;
/*	const char *format;
/*
/*	void	vlog_adhoc(id, orig_rcpt, recipient, relay,
/*				entry, status, format, ap)
/*	const char *id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t	entry;
/*	const char *status;
/*	const char *format;
/*	va_list	ap;
/* DESCRIPTION
/*	This module logs delivery events in an ad-hoc manner.
/*
/*	log_adhoc() appends a record to the mail logfile
/*
/*	vlog_adhoc() implements an alternative client interface.
/*
/*	Arguments:
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
/* .IP sender
/*	The sender envelope address.
/* .IP relay
/*	Host we could (not) talk to.
/* .IP status
/*	bounced, deferred, sent, and so on.
/* .IP entry
/*	Message arrival time.
/* .IP format
/*	Descriptive text.
/* .IP ap
/*	Variable-length argument list.
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

#include <log_adhoc.h>

/* log_adhoc - defer message delivery */

void    log_adhoc(const char *id, const char *orig_rcpt,
		          const char *recipient, const char *relay,
		          time_t entry, const char *status,
		          const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog_adhoc(id, orig_rcpt, recipient, relay, entry, status, fmt, ap);
    va_end(ap);
}

/* vlog_adhoc - defer delivery of queue file */

void    vlog_adhoc(const char *id, const char *orig_rcpt,
		           const char *recipient, const char *relay,
		           time_t entry, const char *status,
		           const char *fmt, va_list ap)
{
    VSTRING *why = vstring_alloc(100);
    int     delay = time((time_t *) 0) - entry;

    vstring_vsprintf(why, fmt, ap);
    if (orig_rcpt && *orig_rcpt && strcasecmp(recipient, orig_rcpt) != 0)
	msg_info("%s: to=<%s>, orig_to=<%s>, relay=%s, delay=%d, status=%s (%s)",
	  id, recipient, orig_rcpt, relay, delay, status, vstring_str(why));
    else
	msg_info("%s: to=<%s>, relay=%s, delay=%d, status=%s (%s)",
		 id, recipient, relay, delay, status, vstring_str(why));
    vstring_free(why);
}
