/*	$NetBSD: bounce_warn_service.c,v 1.1.1.1 2009/06/23 10:08:42 tron Exp $	*/

/*++
/* NAME
/*	bounce_warn_service 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_warn_service(flags, queue_name, queue_id, encoding,
/*					sender, envid, dsn_ret, templates)
/*	int	flags;
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*encoding;
/*	char	*sender;
/*	char	*envid;
/*	int	dsn_ret;
/*	BOUNCE_TEMPLATES *ts;
/* DESCRIPTION
/*	This module implements the server side of the bounce_warn()
/*	(send delay notice) request. The logfile
/*	is not removed, and a warning is sent instead of a bounce.
/*
/*	When a message bounces, a full copy is sent to the originator,
/*	and an optional  copy of the diagnostics with message headers is
/*	sent to the postmaster.  The result is non-zero when the operation
/*	should be tried again.
/*
/*	When a bounce is sent, the sender address is the empty
/*	address.  When a bounce bounces, an optional double bounce
/*	with the entire undeliverable mail is sent to the postmaster,
/*	with as sender address the double bounce address.
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <name_mask.h>

/* Global library. */

#include <mail_params.h>
#include <mail_queue.h>
#include <post_mail.h>
#include <mail_addr.h>
#include <mail_error.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_warn_service - send a delayed mail notice */

int     bounce_warn_service(int unused_flags, char *service, char *queue_name,
			            char *queue_id, char *encoding,
			            char *recipient, char *dsn_envid,
			            int dsn_ret, BOUNCE_TEMPLATES *ts)
{
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 1;
    int     postmaster_status = 1;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);
    VSTRING *new_id = vstring_alloc(10);
    char   *postmaster;
    int     count;

    /*
     * Initialize. Open queue file, bounce log, etc.
     * 
     * XXX DSN This service produces RFC 3464-style "delayed mail" reports from
     * information in the defer logfile. That same file is used for three
     * different types of report:
     * 
     * a) On-demand reports of all delayed deliveries by the mailq(1) command.
     * This reports all recipients that have a transient delivery error.
     * 
     * b) RFC 3464-style "delayed mail" notifications by the defer(8) service.
     * This reports to the sender all recipients that have no DSN NOTIFY
     * information (compatibility) and all recipients that have DSN
     * NOTIFY=DELAY; this reports to postmaster all recipients, subject to
     * notify_classes restrictions.
     * 
     * c) RFC 3464-style bounce reports by the bounce(8) service when mail is
     * too old. This reports to the sender all recipients that have no DSN
     * NOTIFY information (compatibility) and all recipients that have DSN
     * NOTIFY=FAILURE; this reports to postmaster all recipients, subject to
     * notify_classes restrictions.
     */
    bounce_info = bounce_mail_init(service, queue_name, queue_id,
				   encoding, dsn_envid, ts->delay);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_TRACE_FLAGS	0

    /*
     * The choice of sender address depends on the recipient address. For a
     * single bounce (a non-delivery notification to the message originator),
     * the sender address is the empty string. For a double bounce (typically
     * a failed single bounce, or a postmaster notification that was produced
     * by any of the mail processes) the sender address is defined by the
     * var_double_bounce_sender configuration variable. When a double bounce
     * cannot be delivered, the queue manager blackholes the resulting triple
     * bounce message.
     */

    /*
     * Double bounce failed. Never send a triple bounce.
     * 
     * However, this does not prevent double bounces from bouncing on other
     * systems. In order to cope with this, either the queue manager must
     * recognize the double-bounce recipient address and discard mail, or
     * every delivery agent must recognize the double-bounce sender address
     * and substitute something else so mail does not come back at us.
     */
    if (strcasecmp(recipient, mail_addr_double_bounce()) == 0) {
	msg_warn("%s: undeliverable postmaster notification discarded",
		 queue_id);
	bounce_status = 0;
    }

    /*
     * Single bounce failed. Optionally send a double bounce to postmaster,
     * subject to notify_classes restrictions.
     */
#define ANY_BOUNCE (MAIL_ERROR_2BOUNCE | MAIL_ERROR_BOUNCE)
#define SEND_POSTMASTER_DELAY_NOTICE  (notify_mask & MAIL_ERROR_DELAY)

    else if (*recipient == 0) {
	if (!SEND_POSTMASTER_DELAY_NOTICE) {
	    bounce_status = 0;
	} else {
	    postmaster = var_delay_rcpt;
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 postmaster,
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {

		/*
		 * Double bounce to Postmaster. This is the last opportunity
		 * for this message to be delivered. Send the text with
		 * reason for the bounce, and the headers of the original
		 * message. Don't bother sending the boiler-plate text.
		 */
		count = -1;
		if (!bounce_header(bounce, bounce_info, postmaster,
				   POSTMASTER_COPY)
		    && (count = bounce_diagnostic_log(bounce, bounce_info,
						   DSN_NOTIFY_OVERRIDE)) > 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_diagnostic_dsn(bounce, bounce_info,
					     DSN_NOTIFY_OVERRIDE) > 0) {
		    bounce_original(bounce, bounce_info, DSN_RET_FULL);
		    bounce_status = post_mail_fclose(bounce);
		    if (bounce_status == 0)
			msg_info("%s: postmaster delay notification: %s",
				 queue_id, STR(new_id));
		} else {
		    (void) vstream_fclose(bounce);
		    if (count == 0)
			bounce_status = 0;
		}
	    }
	}
    }

    /*
     * Non-bounce failed. Send a single bounce, subject to DSN NOTIFY
     * restrictions.
     */
    else {
	if ((bounce = post_mail_fopen_nowait(NULL_SENDER, recipient,
					     INT_FILT_MASK_BOUNCE,
					     NULL_TRACE_FLAGS,
					     new_id)) != 0) {

	    /*
	     * Send the bounce message header, some boilerplate text that
	     * pretends that we are a polite mail system, the text with
	     * reason for the bounce, and a copy of the original message.
	     */
	    count = -1;
	    if (bounce_header(bounce, bounce_info, recipient,
			      NO_POSTMASTER_COPY) == 0
		&& bounce_boilerplate(bounce, bounce_info) == 0
		&& (count = bounce_diagnostic_log(bounce, bounce_info,
						  DSN_NOTIFY_DELAY)) > 0
		&& bounce_header_dsn(bounce, bounce_info) == 0
		&& bounce_diagnostic_dsn(bounce, bounce_info,
					 DSN_NOTIFY_DELAY) > 0) {
		bounce_original(bounce, bounce_info, DSN_RET_HDRS);
		bounce_status = post_mail_fclose(bounce);
		if (bounce_status == 0)
		    msg_info("%s: sender delay notification: %s",
			     queue_id, STR(new_id));
	    } else {
		(void) vstream_fclose(bounce);
		if (count == 0)
		    bounce_status = 0;
	    }
	}

	/*
	 * Optionally send a postmaster notice, subject to notify_classes
	 * restrictions.
	 * 
	 * This postmaster notice is not critical, so if it fails don't
	 * retransmit the bounce that we just generated, just log a warning.
	 */
	if (bounce_status == 0 && SEND_POSTMASTER_DELAY_NOTICE
	    && strcasecmp(recipient, mail_addr_double_bounce()) != 0) {

	    /*
	     * Send the text with reason for the bounce, and the headers of
	     * the original message. Don't bother sending the boiler-plate
	     * text. This postmaster notice is not critical, so if it fails
	     * don't retransmit the bounce that we just generated, just log a
	     * warning.
	     */
	    postmaster = var_delay_rcpt;
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 postmaster,
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {
		count = -1;
		if (bounce_header(bounce, bounce_info, postmaster,
				  POSTMASTER_COPY) == 0
		    && (count = bounce_diagnostic_log(bounce, bounce_info,
						   DSN_NOTIFY_OVERRIDE)) > 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_diagnostic_dsn(bounce, bounce_info,
					     DSN_NOTIFY_OVERRIDE) > 0) {
		    bounce_original(bounce, bounce_info, DSN_RET_HDRS);
		    postmaster_status = post_mail_fclose(bounce);
		    if (postmaster_status == 0)
			msg_info("%s: postmaster delay notification: %s",
				 queue_id, STR(new_id));
		} else {
		    (void) vstream_fclose(bounce);
		    if (count == 0)
			postmaster_status = 0;
		}
	    }
	    if (postmaster_status)
		msg_warn("%s: postmaster notice failed while warning %s",
			 queue_id, recipient);
	}
    }

    /*
     * Cleanup.
     */
    bounce_mail_free(bounce_info);
    vstring_free(new_id);

    return (bounce_status);
}
