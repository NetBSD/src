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
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <events.h>
#include <line_wrap.h>
#include <name_mask.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_proto.h>
#include <quote_822_local.h>
#include <mail_params.h>
#include <canon_addr.h>
#include <is_header.h>
#include <record.h>
#include <rec_type.h>
#include <mail_conf.h>
#include <post_mail.h>
#include <mail_addr.h>
#include <mark_corrupt.h>
#include <mail_error.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_header - generate bounce message header */

static int bounce_header(VSTREAM *bounce, VSTRING *buf, const char *dest,
			         const char *boundary, int flush)
{

    /*
     * Print a minimal bounce header. The cleanup service will add other
     * headers and will make all addresses fully qualified.
     */
#define STREQ(a, b) (strcasecmp((a), (b)) == 0)

    post_mail_fprintf(bounce, "From: %s (Mail Delivery System)",
		      MAIL_ADDR_MAIL_DAEMON);

    if (flush) {
	post_mail_fputs(bounce, dest == var_bounce_rcpt
		     || dest == var_2bounce_rcpt || dest == var_delay_rcpt ?
			"Subject: Postmaster Copy: Undelivered Mail" :
			"Subject: Undelivered Mail Returned to Sender");
    } else {
	post_mail_fputs(bounce, dest == var_bounce_rcpt
		     || dest == var_2bounce_rcpt || dest == var_delay_rcpt ?
			"Subject: Postmaster Warning: Delayed Mail" :
			"Subject: Delayed Mail (still being retried)");
    }
    post_mail_fprintf(bounce, "To: %s", STR(quote_822_local(buf, dest)));

    /*
     * MIME header.
     */
    post_mail_fprintf(bounce, "MIME-Version: 1.0");
#ifdef DSN
    post_mail_fprintf(bounce, "Content-Type: %s; report-type=%s;",
		      "multipart/report", "delivery-status");
#else
    post_mail_fprintf(bounce, "Content-Type: multipart/mixed;");
#endif
    post_mail_fprintf(bounce, "\tboundary=\"%s\"", boundary);
    post_mail_fputs(bounce, "");
    post_mail_fputs(bounce, "This is a MIME-encapsulated message.");

    /*
     * More MIME header.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", boundary);
    post_mail_fprintf(bounce, "Content-Description: %s", "Notification");
    post_mail_fprintf(bounce, "Content-Type: %s", "text/plain");
    post_mail_fputs(bounce, "");

    return (vstream_ferror(bounce));
}

/* bounce_boilerplate - generate boiler-plate text */

static int bounce_boilerplate(VSTREAM *bounce, VSTRING *buf,
			              const char *boundary, int flush)
{

    /*
     * Print the message body with the problem report. XXX For now, we use a
     * fixed bounce template. We could use a site-specific parametrized
     * template with ${name} macros and we could do wonderful things such as
     * word wrapping to make the text look nicer. No matter how hard we would
     * try, receiving bounced mail will always suck.
     */
    post_mail_fprintf(bounce, "This is the %s program at host %s.",
		      var_mail_name, var_myhostname);
    post_mail_fputs(bounce, "");
    if (flush) {
	post_mail_fputs(bounce,
	       "I'm sorry to have to inform you that the message returned");
	post_mail_fputs(bounce,
	       "below could not be delivered to one or more destinations.");
    } else {
	post_mail_fputs(bounce,
			"####################################################################");
	post_mail_fputs(bounce,
			"# THIS IS A WARNING ONLY.  YOU DO NOT NEED TO RESEND YOUR MESSAGE. #");
	post_mail_fputs(bounce,
			"####################################################################");
	post_mail_fputs(bounce, "");
	post_mail_fprintf(bounce,
			"Your message could not be delivered for %d hours.",
			  var_delay_warn_time);
	post_mail_fprintf(bounce,
			  "It will be retried until it is %d days old.",
			  var_max_queue_time);
    }

    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce,
		      "For further assistance, please contact <%s>",
		      STR(canon_addr_external(buf, MAIL_ADDR_POSTMASTER)));
    if (flush) {
	post_mail_fputs(bounce, "");
	post_mail_fprintf(bounce,
	       "If you do so, please include this problem report. You can");
	post_mail_fprintf(bounce,
		   "delete your own text from the message returned below.");
    }
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "\t\t\tThe %s program", var_mail_name);
    post_mail_fputs(bounce, "");
    return (vstream_ferror(bounce));
}

/* bounce_print - line_wrap callback */

static void bounce_print(const char *str, int len, int indent, char *context)
{
    VSTREAM *bounce = (VSTREAM *) context;

    post_mail_fprintf(bounce, "%*s%.*s", indent, "", len, str);
}

/* bounce_diagnostics - send bounce log report */

static int bounce_diagnostics(char *service, VSTREAM *bounce, VSTRING *buf,
			              char *queue_id, const char *boundary)
{
    VSTREAM *log;

    /*
     * MIME header.
     */
#ifdef DSN
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", boundary);
    post_mail_fprintf(bounce, "Content-Description: %s", "Delivery error report");
    post_mail_fprintf(bounce, "Content-Type: %s", "message/delivery-status");
    post_mail_fputs(bounce, "");
#endif

    /*
     * If the bounce log cannot be found, do not raise a fatal run-time
     * error. There is nothing we can do about the error, and all we are
     * doing is to inform the sender of a delivery problem, Bouncing a
     * message does not have to be a perfect job. But if the system IS
     * running out of resources, raise a fatal run-time error and force a
     * backoff.
     */
    if ((log = mail_queue_open(service, queue_id, O_RDONLY, 0)) == 0) {
	if (errno != ENOENT)
	    msg_fatal("open %s %s: %m", service, queue_id);
	post_mail_fputs(bounce, "\t--- Delivery error report unavailable ---");
	post_mail_fputs(bounce, "");
    }

    /*
     * Append a copy of the delivery error log. Again, we're doing a best
     * effort, so there is no point raising a fatal run-time error in case of
     * a logfile read error. Wrap long lines, filter non-printable
     * characters, and prepend one blank, so this data can safely be piped
     * into other programs.
     */
    else {

#define LENGTH	79
#define INDENT	4
	while (vstream_ferror(bounce) == 0 && vstring_fgets_nonl(buf, log)) {
	    printable(STR(buf), '_');
	    line_wrap(STR(buf), LENGTH, INDENT, bounce_print, (char *) bounce);
	    if (vstream_ferror(bounce) != 0)
		break;
	}
	if (vstream_fclose(log))
	    msg_warn("read bounce log %s: %m", queue_id);
    }
    return (vstream_ferror(bounce));
}

/* bounce_original - send a copy of the original to the victim */

static int bounce_original(char *service, VSTREAM *bounce, VSTRING *buf,
			           char *queue_name, char *queue_id,
			           const char *boundary, int headers_only)
{
    int     status = 0;
    VSTREAM *src;
    int     rec_type;
    int     bounce_length;

    /*
     * MIME headers.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", boundary);
    post_mail_fprintf(bounce, "Content-Description: %s", "Undelivered Message");
    post_mail_fprintf(bounce, "Content-Type: %s", headers_only ?
		      "text/rfc822-headers" : "message/rfc822");
    post_mail_fputs(bounce, "");

    /*
     * If the original message cannot be found, do not raise a run-time
     * error. There is nothing we can do about the error, and all we are
     * doing is to inform the sender of a delivery problem. Bouncing a
     * message does not have to be a perfect job. But if the system IS
     * running out of resources, raise a fatal run-time error and force a
     * backoff.
     */
    if ((src = mail_queue_open(queue_name, queue_id, O_RDONLY, 0)) == 0) {
	if (errno != ENOENT)
	    msg_fatal("open %s %s: %m", service, queue_id);
	post_mail_fputs(bounce, "\t--- Undelivered message unavailable ---");
	post_mail_fputs(bounce, "");
	return (vstream_ferror(bounce));
    }

    /*
     * Skip over the original message envelope records. If the envelope is
     * corrupted just send whatever we can (remember this is a best effort,
     * it does not have to be perfect).
     */
    while ((rec_type = rec_get(src, buf, 0)) > 0)
	if (rec_type == REC_TYPE_MESG)
	    break;

    /*
     * Copy the original message contents. Limit the amount of bounced text
     * so there is a better chance of the bounce making it back. We're doing
     * raw record output here so that we don't throw away binary transparency
     * yet.
     */
#define IS_HEADER(s) (ISSPACE(*(s)) || is_header(s))

    bounce_length = 0;
    while (status == 0 && (rec_type = rec_get(src, buf, 0)) > 0) {
	if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
	    break;
	if (headers_only && !IS_HEADER(vstring_str(buf)))
	    break;
	if (var_bounce_limit == 0 || bounce_length < var_bounce_limit) {
	    bounce_length += VSTRING_LEN(buf);
	    status = (REC_PUT_BUF(bounce, rec_type, buf) != rec_type);
	}
    }
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s--", boundary);
    if (headers_only == 0 && rec_type != REC_TYPE_XTRA)
	status |= mark_corrupt(src);
    if (vstream_fclose(src))
	msg_warn("read message file %s %s: %m", queue_name, queue_id);
    return (status);
}

/* bounce_notify_service - send a bounce */

int     bounce_notify_service(char *service, char *queue_name,
			         char *queue_id, char *recipient, int flush)
{
    VSTRING *buf = vstring_alloc(100);
    int     bounce_status = 1;
    int     postmaster_status = 1;
    VSTREAM *bounce;
    int     notify_mask = name_mask(mail_error_masks, var_notify_classes);
    VSTRING *boundary = vstring_alloc(100);
    char   *postmaster;

    /*
     * Unique string for multi-part message boundaries.
     */
    vstring_sprintf(boundary, "%s.%ld/%s",
		    queue_id, event_time(), var_myhostname);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_CLEANUP_FLAGS	0
#define BOUNCE_HEADERS		1
#define BOUNCE_ALL		0

    /*
     * The choice of sender address depends on recipient the address. For a
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
						 NULL_CLEANUP_FLAGS,
						 "BOUNCE")) != 0) {

		/*
		 * Double bounce to Postmaster. This is the last opportunity
		 * for this message to be delivered. Send the text with
		 * reason for the bounce, and the headers of the original
		 * message. Don't bother sending the boiler-plate text.
		 */
		if (!bounce_header(bounce, buf, postmaster,
				   STR(boundary), flush)
		    && bounce_diagnostics(service, bounce, buf, queue_id,
					  STR(boundary)) == 0)
		    bounce_original(service, bounce, buf, queue_name, queue_id,
				    STR(boundary),
				    flush ? BOUNCE_ALL : BOUNCE_HEADERS);
		bounce_status = post_mail_fclose(bounce);
	    }
	}
    }

    /*
     * Non-bounce failed. Send a single bounce.
     */
    else {
	if ((bounce = post_mail_fopen_nowait(NULL_SENDER, recipient,
					     NULL_CLEANUP_FLAGS,
					     "BOUNCE")) != 0) {

	    /*
	     * Send the bounce message header, some boilerplate text that
	     * pretends that we are a polite mail system, the text with
	     * reason for the bounce, and a copy of the original message.
	     */
	    if (bounce_header(bounce, buf, recipient, STR(boundary), flush) == 0
		&& bounce_boilerplate(bounce, buf, STR(boundary), flush) == 0
		&& bounce_diagnostics(service, bounce, buf, queue_id,
				      STR(boundary)) == 0)
		bounce_original(service, bounce, buf, queue_name, queue_id,
				STR(boundary),
				flush ? BOUNCE_ALL : BOUNCE_HEADERS);
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
						 NULL_CLEANUP_FLAGS,
						 "BOUNCE")) != 0) {
		if (!bounce_header(bounce, buf, postmaster,
				   STR(boundary), flush)
		    && bounce_diagnostics(service, bounce, buf,
					  queue_id, STR(boundary)) == 0)
		    bounce_original(service, bounce, buf, queue_name, queue_id,
				    STR(boundary), BOUNCE_HEADERS);
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
    vstring_free(buf);
    vstring_free(boundary);

    return (bounce_status);
}
