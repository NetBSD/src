/*++
/* NAME
/*	bounce_notify_service 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_notify_service(queue_name, queue_id, sender, flush)
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*sender;
/*	int	flush;
/* DESCRIPTION
/*	This module implements the server side of the bounce_notify()
/*	(send bounce message) request. If flush is zero, the logfile
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
/*	Fatal error: error opening existing file. Warnings: corrupt
/*	message file. A corrupt message is saved to the "corrupt"
/*	queue for further inspection.
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

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_notify_service - send a bounce */

int     bounce_notify_service(char *service, char *queue_name,
			         char *queue_id, char *recipient, int flush)
{
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 1;
    int     postmaster_status = 1;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);
    char   *postmaster;

    /*
     * Initialize. Open queue file, bounce log, etc.
     */
    bounce_info = bounce_mail_init(service, queue_name, queue_id, flush);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_CLEANUP_FLAGS	0
#define BOUNCE_HEADERS		1
#define BOUNCE_ALL		0

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
     * Single bounce failed. Optionally send a double bounce to postmaster.
     */
#define ANY_BOUNCE (MAIL_ERROR_2BOUNCE | MAIL_ERROR_BOUNCE)
#define SKIP_IF_BOUNCE (flush == 1 && (notify_mask & ANY_BOUNCE) == 0)
#define SKIP_IF_DELAY  (flush == 0 && (notify_mask & MAIL_ERROR_DELAY) == 0)

    else if (*recipient == 0) {
	if (SKIP_IF_BOUNCE || SKIP_IF_DELAY) {
	    bounce_status = 0;
	} else {
	    postmaster = flush ? var_2bounce_rcpt : var_delay_rcpt;
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 postmaster,
						 NULL_CLEANUP_FLAGS)) != 0) {

		/*
		 * Double bounce to Postmaster. This is the last opportunity
		 * for this message to be delivered. Send the text with
		 * reason for the bounce, and the headers of the original
		 * message. Don't bother sending the boiler-plate text.
		 */
		if (!bounce_header(bounce, bounce_info, postmaster)
		    && bounce_diagnostic_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_diagnostic_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, flush ?
				    BOUNCE_ALL : BOUNCE_HEADERS);
		bounce_status = post_mail_fclose(bounce);
	    }
	}
    }

    /*
     * Non-bounce failed. Send a single bounce.
     */
    else {
	if ((bounce = post_mail_fopen_nowait(NULL_SENDER, recipient,
					     NULL_CLEANUP_FLAGS)) != 0) {

	    /*
	     * Send the bounce message header, some boilerplate text that
	     * pretends that we are a polite mail system, the text with
	     * reason for the bounce, and a copy of the original message.
	     */
	    if (bounce_header(bounce, bounce_info, recipient) == 0
		&& bounce_boilerplate(bounce, bounce_info) == 0
		&& bounce_diagnostic_log(bounce, bounce_info) == 0
		&& bounce_header_dsn(bounce, bounce_info) == 0
		&& bounce_diagnostic_dsn(bounce, bounce_info) == 0)
		bounce_original(bounce, bounce_info, flush ?
				BOUNCE_ALL : BOUNCE_HEADERS);
	    bounce_status = post_mail_fclose(bounce);
	}

	/*
	 * Optionally, send a postmaster notice.
	 * 
	 * This postmaster notice is not critical, so if it fails don't
	 * retransmit the bounce that we just generated, just log a warning.
	 */
#define WANT_IF_BOUNCE (flush == 1 && (notify_mask & MAIL_ERROR_BOUNCE))
#define WANT_IF_DELAY  (flush == 0 && (notify_mask & MAIL_ERROR_DELAY))

	if (bounce_status == 0 && (WANT_IF_BOUNCE || WANT_IF_DELAY)
	    && strcasecmp(recipient, mail_addr_double_bounce()) != 0) {

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
		    && bounce_diagnostic_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_diagnostic_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, BOUNCE_HEADERS);
		postmaster_status = post_mail_fclose(bounce);
	    }
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

    return (bounce_status);
}
