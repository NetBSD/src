/*	$NetBSD: bounce_notify_verp.c,v 1.1.1.1.2.2 2009/09/15 06:02:29 snj Exp $	*/

/*++
/* NAME
/*	bounce_notify_verp 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_notify_verp(flags, service, queue_name, queue_id, sender,
/*					dsn_envid, dsn_ret, verp_delims,
/*					templates)
/*	int	flags;
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*sender;
/*	char	*dsn_envid;
/*	int	dsn_ret;
/*	char	*verp_delims;
/*	BOUNCE_TEMPLATES *templates;
/* DESCRIPTION
/*	This module implements the server side of the bounce_notify()
/*	(send bounce message) request. The logfile
/*	is removed after and a warning is posted.
/*	The bounce recipient address is encoded in VERP format.
/*	This routine must be used for single bounces only.
/*
/*	When a message bounces, a full copy is sent to the originator,
/*	and an optional  copy of the diagnostics with message headers is
/*	sent to the postmaster.  The result is non-zero when the operation
/*	should be tried again.
/*
/*	When a bounce is sent, the sender address is the empty
/*	address.
/* DIAGNOSTICS
/*	Fatal error: error opening existing file.
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
#include <verp_sender.h>
#include <bounce.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_notify_verp - send a bounce, VERP style */

int     bounce_notify_verp(int flags, char *service, char *queue_name,
			           char *queue_id, char *encoding,
			           char *recipient, char *dsn_envid,
			           int dsn_ret, char *verp_delims,
			           BOUNCE_TEMPLATES *ts)
{
    const char *myname = "bounce_notify_verp";
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 0;
    int     postmaster_status;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);
    char   *postmaster;
    VSTRING *verp_buf;
    VSTRING *new_id;

    /*
     * Sanity checks. We must be called only for undeliverable non-bounce
     * messages.
     */
    if (*recipient == 0)
	msg_panic("%s: attempt to bounce a single bounce", myname);
    if (strcasecmp(recipient, mail_addr_double_bounce()) == 0)
	msg_panic("%s: attempt to bounce a double bounce", myname);

    /*
     * Initialize. Open queue file, bounce log, etc.
     */
    bounce_info = bounce_mail_init(service, queue_name, queue_id,
				   encoding, dsn_envid, ts->failure);

    /*
     * If we have no recipient list then we can't send VERP replies. Send
     * *something* anyway so that the mail is not lost in a black hole.
     */
    if (bounce_info->log_handle == 0) {
	DSN_BUF *dsn_buf = dsb_create();
	RCPT_BUF *rcpt_buf = rcpb_create();

	dsb_simple(dsn_buf, "5.0.0", "(error report unavailable)");
	(void) DSN_FROM_DSN_BUF(dsn_buf);
	vstring_strcpy(rcpt_buf->address, "(recipient address unavailable)");
	(void) RECIPIENT_FROM_RCPT_BUF(rcpt_buf);
	bounce_status = bounce_one_service(flags, queue_name, queue_id,
					   encoding, recipient, dsn_envid,
					   dsn_ret, rcpt_buf, dsn_buf, ts);
	rcpb_free(rcpt_buf);
	dsb_free(dsn_buf);
	bounce_mail_free(bounce_info);
	return (bounce_status);
    }
#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_TRACE_FLAGS	0

    /*
     * A non-bounce message was returned. Send a single bounce, one per
     * recipient.
     */
    verp_buf = vstring_alloc(100);
    new_id = vstring_alloc(10);
    while (bounce_log_read(bounce_info->log_handle, bounce_info->rcpt_buf,
			   bounce_info->dsn_buf) != 0) {
	RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;

	/*
	 * Notify the originator, subject to DSN NOTIFY restrictions.
	 * 
	 * Fix 20090114: Use the Postfix original recipient, because that is
	 * what the VERP consumer expects.
	 */
	if (rcpt->dsn_notify != 0		/* compat */
	    && (rcpt->dsn_notify & DSN_NOTIFY_FAILURE) == 0) {
	    bounce_status = 0;
	} else {
	    verp_sender(verp_buf, verp_delims, recipient, rcpt);
	    if ((bounce = post_mail_fopen_nowait(NULL_SENDER, STR(verp_buf),
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {

		/*
		 * Send the bounce message header, some boilerplate text that
		 * pretends that we are a polite mail system, the text with
		 * reason for the bounce, and a copy of the original message.
		 */
		if (bounce_header(bounce, bounce_info, STR(verp_buf),
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
	    } else
		bounce_status = 1;

	    /*
	     * Stop at the first sign of trouble, instead of making the
	     * problem worse.
	     */
	    if (bounce_status != 0)
		break;

	    /*
	     * Optionally, mark this recipient as done.
	     */
	    if (flags & BOUNCE_FLAG_DELRCPT)
		bounce_delrcpt_one(bounce_info);
	}

	/*
	 * Optionally, send a postmaster notice, subject to notify_classes
	 * restrictions.
	 * 
	 * This postmaster notice is not critical, so if it fails don't
	 * retransmit the bounce that we just generated, just log a warning.
	 */
#define SEND_POSTMASTER_SINGLE_BOUNCE_NOTICE (notify_mask & MAIL_ERROR_BOUNCE)

	if (SEND_POSTMASTER_SINGLE_BOUNCE_NOTICE) {

	    /*
	     * Send the text with reason for the bounce, and the headers of
	     * the original message. Don't bother sending the boiler-plate
	     * text. This postmaster notice is not critical, so if it fails
	     * don't retransmit the bounce that we just generated, just log a
	     * warning.
	     */
	    postmaster = var_bounce_rcpt;
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 postmaster,
						 INT_FILT_MASK_BOUNCE,
						 NULL_TRACE_FLAGS,
						 new_id)) != 0) {
		if (bounce_header(bounce, bounce_info, postmaster,
				  POSTMASTER_COPY) == 0
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, DSN_RET_HDRS);
		postmaster_status = post_mail_fclose(bounce);
		if (postmaster_status == 0)
		    msg_info("%s: postmaster non-delivery notification: %s",
			     queue_id, STR(new_id));
	    } else
		postmaster_status = 1;

	    if (postmaster_status)
		msg_warn("%s: postmaster notice failed while bouncing to %s",
			 queue_id, recipient);
	}
    }

    /*
     * Examine the completion status. Delete the bounce log file only when
     * the bounce was posted successfully, and only if we are bouncing for
     * real, not just warning.
     */
    if (bounce_status == 0 && mail_queue_remove(service, queue_id)
	&& errno != ENOENT)
	msg_fatal("remove %s %s: %m", service, queue_id);

    /*
     * Cleanup.
     */
    bounce_mail_free(bounce_info);
    vstring_free(verp_buf);
    vstring_free(new_id);

    return (bounce_status);
}
