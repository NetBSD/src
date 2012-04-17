/*	$NetBSD: bounce_trace_service.c,v 1.1.1.1.10.1 2012/04/17 00:04:34 yamt Exp $	*/

/*++
/* NAME
/*	bounce_trace_service 3
/* SUMMARY
/*	send status report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_trace_service(flags, queue_name, queue_id, encoding,
/*					sender, char *envid, int ret, templates)
/*	int	flags;
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*encoding;
/*	char	*sender;
/*	char	*envid;
/*	int	ret;
/*	BOUNCE_TEMPLATES *templates;
/* DESCRIPTION
/*	This module implements the server side of the trace_flush()
/*	(send delivery notice) request. The logfile
/*	is removed after the notice is posted.
/*
/*	A status report includes a prelude with human-readable text,
/*	a DSN-style report, and the email message that was subject of
/*	the status report.
/*
/*	When a status report is sent, the sender address is the empty
/*	address.
/* DIAGNOSTICS
/*	Fatal error: error opening existing file.
/* BUGS
/* SEE ALSO
/*	bounce(3) basic bounce service client interface
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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <mail_params.h>
#include <mail_queue.h>
#include <post_mail.h>
#include <mail_addr.h>
#include <mail_error.h>
#include <dsn_mask.h>
#include <deliver_request.h>		/* USR_VRFY and RECORD flags */

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_trace_service - send a delivery status notice */

int     bounce_trace_service(int flags, char *service, char *queue_name,
			             char *queue_id, char *encoding,
			             char *recipient, char *dsn_envid,
			             int unused_dsn_ret,
			             BOUNCE_TEMPLATES *ts)
{
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 1;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);
    VSTRING *new_id;
    int     count;
    const char *sender;

    /*
     * For consistency with fail/delay notifications, send notification for a
     * non-bounce message as a single-bounce message, send notification for a
     * single-bounce message as a double-bounce message, and drop requests to
     * send notification for a double-bounce message.
     */
#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */

    if (strcasecmp(recipient, mail_addr_double_bounce()) == 0) {
	msg_info("%s: not sending trace/success notification for "
		 "double-bounce message", queue_id);
	return (0);
    } else if (*recipient == 0) {
	if ((notify_mask & MAIL_ERROR_2BOUNCE) != 0) {
	    recipient = var_2bounce_rcpt;
	    sender = mail_addr_double_bounce();
	} else {
	    msg_info("%s: not sending trace/success notification "
		     "for single-bounce message", queue_id);
	    if (mail_queue_remove(service, queue_id) && errno != ENOENT)
		msg_fatal("remove %s %s: %m", service, queue_id);
	    return (0);
	}
    } else {
	/* Always send notification for non-bounce message. */
	sender = NULL_SENDER;
    }

    /*
     * Initialize. Open queue file, bounce log, etc.
     * 
     * XXX DSN The trace service produces information from the trace logfile
     * which is used for three types of reports:
     * 
     * a) "what-if" reports that show what would happen without actually
     * delivering mail (sendmail -bv).
     * 
     * b) A report of actual deliveries (sendmail -v).
     * 
     * c) DSN NOTIFY=SUCCESS reports of successful delivery ("delivered",
     * "expanded" or "relayed").
     */
#define NON_DSN_FLAGS (DEL_REQ_FLAG_USR_VRFY | DEL_REQ_FLAG_RECORD)

    bounce_info = bounce_mail_init(service, queue_name, queue_id,
				   encoding, dsn_envid,
				   flags & NON_DSN_FLAGS ?
				   ts->verify : ts->success);

    /*
     * XXX With multi-recipient mail some queue file recipients may have
     * NOTIFY=SUCCESS and others not. Depending on what subset of recipients
     * are delivered, a trace file may or may not be created. Even when the
     * last partial delivery attempt had no NOTIFY=SUCCESS recipients, a
     * trace file may still exist from a previous partial delivery attempt.
     * So as long as any recipient in the original queue file had
     * NOTIFY=SUCCESS we have to always look for the trace file and be
     * prepared for the file not to exist.
     * 
     * See also comments in qmgr/qmgr_active.c.
     */
    if (bounce_info->log_handle == 0) {
	if (msg_verbose)
	    msg_info("%s: no trace file -- not sending a notification",
		     queue_id);
	bounce_mail_free(bounce_info);
	return (0);
    }
#define NULL_TRACE_FLAGS	0

    /*
     * Send a single bounce with a template message header, some boilerplate
     * text that pretends that we are a polite mail system, the text with
     * per-recipient status, and a copy of the original message.
     * 
     * XXX DSN We use the same trace file for "what-if", "verbose delivery" and
     * "success" delivery reports. This saves file system overhead because
     * there are fewer potential left-over files to remove up when we create
     * a new queue file.
     */
    new_id = vstring_alloc(10);
    if ((bounce = post_mail_fopen_nowait(sender, recipient,
					 INT_FILT_MASK_BOUNCE,
					 NULL_TRACE_FLAGS,
					 new_id)) != 0) {
	count = -1;
	if (bounce_header(bounce, bounce_info, recipient,
			  NO_POSTMASTER_COPY) == 0
	    && bounce_boilerplate(bounce, bounce_info) == 0
	    && (count = bounce_diagnostic_log(bounce, bounce_info,
					      DSN_NOTIFY_OVERRIDE)) > 0
	    && bounce_header_dsn(bounce, bounce_info) == 0
	    && bounce_diagnostic_dsn(bounce, bounce_info,
				     DSN_NOTIFY_OVERRIDE) > 0) {
	    bounce_original(bounce, bounce_info, DSN_RET_HDRS);
	    bounce_status = post_mail_fclose(bounce);
	    if (bounce_status == 0)
		msg_info("%s: sender delivery status notification: %s",
			 queue_id, STR(new_id));
	} else {
	    (void) vstream_fclose(bounce);
	    if (count == 0)
		bounce_status = 0;
	}
    }

    /*
     * Examine the completion status. Delete the trace log file only when the
     * status notice was posted successfully.
     */
    if (bounce_status == 0 && mail_queue_remove(service, queue_id)
	&& errno != ENOENT)
	msg_fatal("remove %s %s: %m", service, queue_id);

    /*
     * Cleanup.
     */
    bounce_mail_free(bounce_info);
    vstring_free(new_id);

    return (bounce_status);
}
