/*++
/* NAME
/*	bounce_notify_verp 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_notify_verp(service, queue_name, queue_id, sender,
/*					verp_delims, flush)
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*sender;
/*	char	*verp_delims;
/*	int	flush;
/* DESCRIPTION
/*	This module implements the server side of the bounce_notify()
/*	(send bounce message) request. If flush is zero, the logfile
/*	is not removed, and a warning is sent instead of a bounce.
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
/*	Fatal error: error opening existing file. Warnings: corrupt
/*	message file. A corrupt message is saved to the "corrupt"
/*	queue for further inspection.
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

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_notify_verp - send a bounce */

int     bounce_notify_verp(char *service, char *queue_name,
			           char *queue_id, char *recipient,
			           char *verp_delims, int flush)
{
    char   *myname = "bounce_notify_verp";
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 0;
    int     postmaster_status;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);
    char   *postmaster;
    VSTRING *verp_buf = vstring_alloc(100);

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
    bounce_info = bounce_mail_init(service, queue_name, queue_id, flush);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_CLEANUP_FLAGS	0
#define BOUNCE_HEADERS		1
#define BOUNCE_ALL		0

    /*
     * A non-bounce message was returned. Send a single bounce, one per
     * recipient.
     */
    while (bounce_log_read(bounce_info->log_handle) != 0) {

	/*
	 * Notify the originator.
	 */
	verp_sender(verp_buf, verp_delims, recipient,
		    bounce_info->log_handle->recipient);
	if ((bounce = post_mail_fopen_nowait(NULL_SENDER, STR(verp_buf),
					     NULL_CLEANUP_FLAGS)) != 0) {

	    /*
	     * Send the bounce message header, some boilerplate text that
	     * pretends that we are a polite mail system, the text with
	     * reason for the bounce, and a copy of the original message.
	     */
	    if (bounce_header(bounce, bounce_info, STR(verp_buf)) == 0
		&& bounce_boilerplate(bounce, bounce_info) == 0
		&& bounce_recipient_log(bounce, bounce_info) == 0
		&& bounce_header_dsn(bounce, bounce_info) == 0
		&& bounce_recipient_dsn(bounce, bounce_info) == 0)
		bounce_original(bounce, bounce_info, flush ?
				BOUNCE_ALL : BOUNCE_HEADERS);
	    bounce_status = post_mail_fclose(bounce);
	} else
	    bounce_status = 1;

	/*
	 * Stop at the first sign of trouble, instead of making the problem
	 * worse.
	 */
	if (bounce_status != 0)
	    break;

	/*
	 * Mark this recipient as done.
	 */
	bounce_log_delrcpt(bounce_info->log_handle);

	/*
	 * Optionally, send a postmaster notice.
	 * 
	 * This postmaster notice is not critical, so if it fails don't
	 * retransmit the bounce that we just generated, just log a warning.
	 */
#define WANT_IF_BOUNCE (flush == 1 && (notify_mask & MAIL_ERROR_BOUNCE))
#define WANT_IF_DELAY  (flush == 0 && (notify_mask & MAIL_ERROR_DELAY))

	if (WANT_IF_BOUNCE || WANT_IF_DELAY) {

	    /*
	     * Send the text with reason for the bounce, and the headers of
	     * the original message. Don't bother sending the boiler-plate
	     * text. This postmaster notice is not critical, so if it fails
	     * don't retransmit the bounce that we just generated, just log a
	     * warning.
	     */
	    postmaster = flush ? var_bounce_rcpt : var_delay_rcpt;
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 postmaster,
						 NULL_CLEANUP_FLAGS)) != 0) {
		if (bounce_header(bounce, bounce_info, postmaster) == 0
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, BOUNCE_HEADERS);
		postmaster_status = post_mail_fclose(bounce);
	    } else
		postmaster_status = 1;

	    if (postmaster_status)
		msg_warn("postmaster notice failed while bouncing to %s",
			 recipient);
	}
    }

    /*
     * Examine the completion status. Delete the bounce log file only when
     * the bounce was posted successfully, and only if we are bouncing for
     * real, not just warning.
     */
    if (flush != 0 && bounce_status == 0 && mail_queue_remove(service, queue_id)
	&& errno != ENOENT)
	msg_fatal("remove %s %s: %m", service, queue_id);

    /*
     * Cleanup.
     */
    bounce_mail_free(bounce_info);
    vstring_free(verp_buf);

    return (bounce_status);
}
