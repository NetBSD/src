/*	$NetBSD: reject_deliver_request.c,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	reject_deliver_request 3
/* SUMMARY
/*	reject an entire delivery request
/* SYNOPSIS
/*	#include <reject_deliver_request.h>
/*
/*	int     reject_deliver_request(
/*	const char *service,
/*	DELIVER_REQUEST *request,
/*	const char *code,
/*	const char *format, ...);
/* DESCRIPTION
/*	reject_deliver_request() rejects an entire delivery request
/*	and bounces or defers all its recipients. The result value
/*	is the request's delivery status.
/*
/*	Arguments:
/* .IP service
/*	The service name from master.cf.
/* .IP request
/*	The delivery request that is being rejected.
/* .IP code
/*	Enhanced status code, must be in 4.X.X or 5.X.X. form.
/*	All recipients in the request are bounced or deferred
/*	depending on the status code value.
/* .IP "format, ..."
/*	Format string and optional arguments.
/* DIAGNOSTICS
/*	Panic: interface violation. Fatal: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>

 /*
  * Global library.
  */
#include <bounce.h>
#include <defer.h>
#include <deliver_completed.h>
#include <deliver_request.h>
#include <recipient_list.h>

/* reject_deliver_request - reject an entire delivery request */

int     reject_deliver_request(const char *service, DELIVER_REQUEST *request,
			               const char *code,
			               const char *format,...)
{
    const char myname[] = "reject_deliver_request";
    va_list ap;
    RECIPIENT *rcpt;
    DSN_BUF *why;
    int     status;
    int     result = 0;
    int     n;

    /*
     * Format something that we can pass to bounce_append() or
     * defer_append().
     */
    va_start(ap, format);
    why = vdsb_simple(dsb_create(), code, format, ap);
    va_end(ap);
    (void) DSN_FROM_DSN_BUF(why);
    if (strchr("45", vstring_str(why->status)[0]) == 0)
	msg_panic("%s: bad enhanced status code %s", myname, code);

    /*
     * Blindly bounce or defer all recipients.
     */
    for (n = 0; n < request->rcpt_list.len; n++) {
	rcpt = request->rcpt_list.info + n;
	status = (vstring_str(why->status)[0] != '4' ?
		  bounce_append : defer_append)
	    (DEL_REQ_TRACE_FLAGS(request->flags),
	     request->queue_id,
	     &request->msg_stats, rcpt,
	     service, &why->dsn);
	if (status == 0)
	    deliver_completed(request->fp, rcpt->offset);
	result |= status;
    }
    dsb_free(why);
    return (result);
}
