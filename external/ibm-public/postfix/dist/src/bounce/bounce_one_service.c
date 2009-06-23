/*	$NetBSD: bounce_one_service.c,v 1.1.1.1 2009/06/23 10:08:42 tron Exp $	*/

/*++
/* NAME
/*	bounce_one_service 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_one_service(flags, queue_name, queue_id, encoding,
/*					orig_sender, envid, ret,
/*					rcpt_buf, dsn_buf, templates)
/*	int	flags;
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*encoding;
/*	char	*orig_sender;
/*	char	*envid;
/*	int	ret;
/*	RCPT_BUF *rcpt_buf;
/*	DSN_BUF	*dsn_buf;
/*	BOUNCE_TEMPLATES *templates;
/* DESCRIPTION
/*	This module implements the server side of the bounce_one()
/*	(send bounce message for one recipient) request.
/*
/*	When a message bounces, a full copy is sent to the originator,
/*	and an optional copy of the diagnostics with message headers is
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
#include <post_mail.h>
#include <mail_addr.h>
#include <mail_error.h>
#include <bounce.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_one_service - send a bounce for one recipient */

int     bounce_one_service(int flags, char *queue_name, char *queue_id,
			           char *encoding, char *orig_sender,
			           char *dsn_envid, int dsn_ret,
			           RCPT_BUF *rcpt_buf, DSN_BUF *dsn_buf,
			           BOUNCE_TEMPLATES *ts)
{
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 1;
    int     postmaster_status = 1;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);
    VSTRING *new_id = vstring_alloc(10);

    /*
     * Initialize. Open queue file, bounce log, etc.
     */
    bounce_info = bounce_mail_one_init(queue_name, queue_id, encoding,
				       dsn_envid, rcpt_buf, dsn_buf,
				       ts->failure);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_TRACE_FLAGS	0

    /*
     * The choice of bounce sender address depends on the original sender
     * address. For a single bounce (a non-delivery notification to the
     * message originator), the sender address is the empty string. For a
     * double bounce (typically a failed single bounce, or a postmaster
     * notification that was produced by any of the mail processes) the
     * sender address is defined by the var_double_bounce_sender
     * configuration variable. When a double bounce cannot be delivered, the
     * queue manager blackholes the resulting triple bounce message.
     */

    /*
     * Double bounce failed. Never send a triple bounce.
     * 
     * However, this does not prevent double bounces from bouncing on other
     * systems. In order to cope with this, either the queue manager must
     * recognize the double-bounce original sender address and discard mail,
     * or every delivery agent must recognize the double-bounce sender
     * address and substitute something else so mail does not come back at
     * us.
     */
    if (strcasecmp(orig_sender, mail_addr_double_bounce()) == 0) {
	msg_warn("%s: undeliverable postmaster notification discarded",
		 queue_id);
	bounce_status = 0;
    }

    /*
     * Single bounce failed. Optionally send a double bounce to postmaster,
     * subject to notify_classes restrictions.
     */
#define ANY_BOUNCE (MAIL_ERROR_2BOUNCE | MAIL_ERROR_BOUNCE)
#define SEND_POSTMASTER_ANY_BOUNCE_NOTICE (notify_mask & ANY_BOUNCE)

    else if (*orig_sender == 0) {
	if (!SEND_POSTMASTER_ANY_BOUNCE_NOTICE) {
	    bounce_status = 0;
	} else {
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 var_2bounce_rcpt,
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {

		/*
		 * Double bounce to Postmaster. This is the last opportunity
		 * for this message to be delivered. Send the text with
		 * reason for the bounce, and the headers of the original
		 * message. Don't bother sending the boiler-plate text.
		 */
		if (!bounce_header(bounce, bounce_info, var_2bounce_rcpt,
				   POSTMASTER_COPY)
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, DSN_RET_FULL);
		bounce_status = post_mail_fclose(bounce);
		if (bounce_status == 0)
		    msg_info("%s: postmaster non-delivery notification: %s",
			     queue_id, STR(new_id));
	    }
	}
    }

    /*
     * Non-bounce failed. Send a single bounce, subject to DSN NOTIFY
     * restrictions.
     */
    else {
	RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;

	if (rcpt->dsn_notify != 0		/* compat */
	    && (rcpt->dsn_notify & DSN_NOTIFY_FAILURE) == 0) {
	    bounce_status = 0;
	} else {
	    if ((bounce = post_mail_fopen_nowait(NULL_SENDER, orig_sender,
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {

		/*
		 * Send the bounce message header, some boilerplate text that
		 * pretends that we are a polite mail system, the text with
		 * reason for the bounce, and a copy of the original message.
		 */
		if (bounce_header(bounce, bounce_info, orig_sender,
				  NO_POSTMASTER_COPY) == 0
		    && bounce_boilerplate(bounce, bounce_info) == 0
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, dsn_ret ?
				    dsn_ret : DSN_RET_FULL);
		bounce_status = post_mail_fclose(bounce);
		if (bounce_status == 0)
		    msg_info("%s: sender non-delivery notification: %s",
			     queue_id, STR(new_id));
	    }
	}

	/*
	 * Optionally send a postmaster notice, subject to notify_classes
	 * restrictions.
	 * 
	 * This postmaster notice is not critical, so if it fails don't
	 * retransmit the bounce that we just generated, just log a warning.
	 */
#define SEND_POSTMASTER_SINGLE_BOUNCE_NOTICE (notify_mask & MAIL_ERROR_BOUNCE)

	if (bounce_status == 0 && SEND_POSTMASTER_SINGLE_BOUNCE_NOTICE
	    && strcasecmp(orig_sender, mail_addr_double_bounce()) != 0) {

	    /*
	     * Send the text with reason for the bounce, and the headers of
	     * the original message. Don't bother sending the boiler-plate
	     * text. This postmaster notice is not critical, so if it fails
	     * don't retransmit the bounce that we just generated, just log a
	     * warning.
	     */
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 var_bounce_rcpt,
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {
		if (bounce_header(bounce, bounce_info, var_bounce_rcpt,
				  POSTMASTER_COPY) == 0
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, DSN_RET_HDRS);
		postmaster_status = post_mail_fclose(bounce);
		if (postmaster_status == 0)
		    msg_info("%s: postmaster non-delivery notification: %s",
			     queue_id, STR(new_id));
	    }
	    if (postmaster_status)
		msg_warn("%s: postmaster notice failed while bouncing to %s",
			 queue_id, orig_sender);
	}
    }

    /*
     * Optionally, delete the recipient from the queue file.
     */
    if (bounce_status == 0 && (flags & BOUNCE_FLAG_DELRCPT))
	bounce_delrcpt_one(bounce_info);

    /*
     * Cleanup.
     */
    bounce_mail_free(bounce_info);
    vstring_free(new_id);

    return (bounce_status);
}
