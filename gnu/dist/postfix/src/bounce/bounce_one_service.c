/*++
/* NAME
/*	bounce_one_service 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_one_service(flags, queue_name, queue_id, encoding,
/*					orig_sender, orig_recipient,
/*					status, why)
/*	int	flags;
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*encoding;
/*	char	*orig_sender;
/*	char	*orig_recipient;
/*	char	*status;
/*	char	*why;
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
#include <post_mail.h>
#include <mail_addr.h>
#include <mail_error.h>
#include <bounce.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_one_service - send a bounce for one recipient */

int     bounce_one_service(int flags, char *queue_name, char *queue_id,
			           char *encoding, char *orig_sender,
			           char *orig_recipient, char *recipient,
			           long offset, char *dsn_status,
			           char *dsn_action, char *why)
{
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 1;
    int     postmaster_status = 1;
    VSTREAM *bounce;
    int     notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				    var_notify_classes);

    /*
     * Initialize. Open queue file, bounce log, etc.
     */
    bounce_info = bounce_mail_one_init(queue_name, queue_id,
				       encoding, orig_recipient,
				       recipient, offset, dsn_status,
				       dsn_action, why);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_TRACE_FLAGS	0
#define BOUNCE_HEADERS		1
#define BOUNCE_ALL		0

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
     * Single bounce failed. Optionally send a double bounce to postmaster.
     */
#define ANY_BOUNCE (MAIL_ERROR_2BOUNCE | MAIL_ERROR_BOUNCE)
#define SKIP_IF_BOUNCE ((notify_mask & ANY_BOUNCE) == 0)

    else if (*orig_sender == 0) {
	if (SKIP_IF_BOUNCE) {
	    bounce_status = 0;
	} else {
	    if ((bounce = post_mail_fopen_nowait(mail_addr_double_bounce(),
						 var_2bounce_rcpt,
						 CLEANUP_FLAG_MASK_INTERNAL,
						 NULL_TRACE_FLAGS)) != 0) {

		/*
		 * Double bounce to Postmaster. This is the last opportunity
		 * for this message to be delivered. Send the text with
		 * reason for the bounce, and the headers of the original
		 * message. Don't bother sending the boiler-plate text.
		 */
		if (!bounce_header(bounce, bounce_info, var_2bounce_rcpt)
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, BOUNCE_ALL);
		bounce_status = post_mail_fclose(bounce);
	    }
	}
    }

    /*
     * Non-bounce failed. Send a single bounce.
     */
    else {
	if ((bounce = post_mail_fopen_nowait(NULL_SENDER, orig_sender,
					     CLEANUP_FLAG_MASK_INTERNAL,
					     NULL_TRACE_FLAGS)) != 0) {

	    /*
	     * Send the bounce message header, some boilerplate text that
	     * pretends that we are a polite mail system, the text with
	     * reason for the bounce, and a copy of the original message.
	     */
	    if (bounce_header(bounce, bounce_info, orig_sender) == 0
		&& bounce_boilerplate(bounce, bounce_info) == 0
		&& bounce_recipient_log(bounce, bounce_info) == 0
		&& bounce_header_dsn(bounce, bounce_info) == 0
		&& bounce_recipient_dsn(bounce, bounce_info) == 0)
		bounce_original(bounce, bounce_info, BOUNCE_ALL);
	    bounce_status = post_mail_fclose(bounce);
	}

	/*
	 * Optionally, send a postmaster notice.
	 * 
	 * This postmaster notice is not critical, so if it fails don't
	 * retransmit the bounce that we just generated, just log a warning.
	 */
#define WANT_IF_BOUNCE ((notify_mask & MAIL_ERROR_BOUNCE))

	if (bounce_status == 0 && (WANT_IF_BOUNCE)
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
						 CLEANUP_FLAG_MASK_INTERNAL,
						 NULL_TRACE_FLAGS)) != 0) {
		if (bounce_header(bounce, bounce_info, var_bounce_rcpt) == 0
		    && bounce_recipient_log(bounce, bounce_info) == 0
		    && bounce_header_dsn(bounce, bounce_info) == 0
		    && bounce_recipient_dsn(bounce, bounce_info) == 0)
		    bounce_original(bounce, bounce_info, BOUNCE_HEADERS);
		postmaster_status = post_mail_fclose(bounce);
	    }
	    if (postmaster_status)
		msg_warn("postmaster notice failed while bouncing to %s",
			 orig_sender);
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

    return (bounce_status);
}
