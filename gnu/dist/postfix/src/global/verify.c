/*	$NetBSD: verify.c,v 1.1.1.3 2004/07/28 22:49:19 heas Exp $	*/

/*++
/* NAME
/*	verify 3
/* SUMMARY
/*	update verify database
/* SYNOPSIS
/*	#include <verify.h>
/*
/*	int	verify_append(queue_id, orig_rcpt, recipient,
/*				relay, entry, status,
/*				recipient_status, format, ...)
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t  entry;
/*	const char *status;
/*	int	recipient_status;
/*	const char *format;
/*
/*	int	vverify_append(queue_id, orig_rcpt, recipient,
/*				relay, entry, status,
/*				recipient_status, format, ap)
/*	int	recipient_status;
/*	const char *queue_id;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	const char *relay;
/*	time_t  entry;
/*	const char *status;
/*	int	recipient_status;
/*	const char *format;
/*	va_list ap;
/* DESCRIPTION
/*	This module implements an impedance adaptor between the
/*	verify_clnt interface and the interface expected by the
/*	bounce/defer/sent modules.
/*
/*	verify_append() updates the address verification database
/*	and logs the action to the mailer logfile.
/*
/*	vverify_append() implements an alternative interface.
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
/* .IP status
/*	"deliverable", "undeliverable", and so on.
/* .IP recipient_status
/*	One of the following recipient verification status codes:
/* .RS
/* .IP DEL_REQ_RCPT_STAT_OK
/*	Successful delivery.
/* .IP DEL_REQ_RCPT_STAT_DEFER
/*	Temporary delivery error.
/* .IP DEL_REQ_RCPT_STAT_BOUNCE
/*	Hard delivery error.
/* .RE
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
#include <verify.h>

/* verify_append - update address verification database */

int     verify_append(const char *queue_id, const char *orig_rcpt,
		             const char *recipient, const char *relay,
		             time_t entry, const char *status,
		             int rcpt_stat, const char *fmt,...)
{
    va_list ap;
    int     req_stat;

    va_start(ap, fmt);
    req_stat = vverify_append(queue_id, orig_rcpt, recipient, relay,
			     entry, status, rcpt_stat, fmt, ap);
    va_end(ap);
    return (req_stat);
}

/* vverify_append - update address verification database */

int     vverify_append(const char *queue_id, const char *orig_rcpt,
		              const char *recipient, const char *relay,
		              time_t entry, const char *status,
		              int rcpt_stat, const char *fmt, va_list ap)
{
    VSTRING *text = vstring_alloc(10);
    int     req_stat;

    /*
     * Impedance adaptor between bounce/defer/sent and verify_clnt.
     */
    vstring_vsprintf(text, fmt, ap);
    if (var_verify_neg_cache || rcpt_stat == DEL_RCPT_STAT_OK) {
	req_stat = verify_clnt_update(orig_rcpt, rcpt_stat,
				      "%s", vstring_str(text));
	if (req_stat == VRFY_STAT_OK && strcasecmp(recipient, orig_rcpt) != 0)
	    req_stat = verify_clnt_update(recipient, rcpt_stat,
					  "%s", vstring_str(text));
    } else {
	status = "undeliverable-but-not-cached";
	req_stat = VRFY_STAT_OK;
    }
    if (req_stat == VRFY_STAT_OK) {
	log_adhoc(queue_id, orig_rcpt, recipient, relay,
		  entry, status, "%s", vstring_str(text));
	req_stat = 0;
    } else {
	msg_warn("%s: %s service failure", queue_id, var_verify_service);
	req_stat = -1;
    }
    vstring_free(text);
    return (req_stat);
}
