/*++
/* NAME
/*	bounce_notify_util 3
/* SUMMARY
/*	send non-delivery report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	typedef struct {
/* .in +4
/*		/* All private members... */
/* .in -4
/*	} BOUNCE_INFO;
/*
/*	BOUNCE_INFO *bounce_mail_init(service, queue_name, queue_id, flush)
/*	const char *service;
/*	const char *queue_name;
/*	const char *queue_id;
/*	int	flush;
/*
/*	void	bounce_mail_free(bounce_info)
/*	BOUNCE_INFO *bounce_info;
/*
/*	int	bounce_header(fp, bounce_info, recipient)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*	const char *recipient;
/*
/*	int	bounce_boilerplate(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_recipient_log(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_diagnostic_log(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_header_dsn(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_recipient_dsn(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_diagnostic_dsn(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int	bounce_original(fp, bounce_info, headers_only)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*	int	headers_only;
/* DESCRIPTION
/*	This module implements the grunt work of sending a non-delivery
/*	notification. A bounce is sent in a form that satisfies RFC 1894
/*	(delivery status notifications).
/*
/*	bounce_mail_init() bundles up its argument and attempts to
/*	open the corresponding logfile and message file. A BOUNCE_INFO
/*	structure contains all the necessary information about an
/*	undeliverable message.
/*
/*	bounce_mail_free() releases memory allocated by bounce_mail_init()
/*	and closes any files opened by bounce_mail_init().
/*
/*	bounce_header() produces a standard mail header with the specified
/*	recipient and starts a text/plain message segment for the
/*	human-readable problem description.
/*
/*	bounce_boilerplate() produces the standard "sorry" text that
/*	creates the illusion that mail systems are civilized.
/*
/*	bounce_recipient_log() sends a human-readable representation of
/*	logfile information for one recipient, with the recipient address
/*	and with the text why the recipient was undeliverable.
/*
/*	bounce_diagnostic_log() sends a human-readable representation of
/*	logfile information for all undeliverable recipients. This routine
/*	will become obsolete when individual recipients of the same message
/*	can have different sender addresses to bounce to.
/*
/*	bounce_header_dsn() starts a message/delivery-status message
/*	segment and sends the machine-readable information that identifies
/*	the reporting MTA.
/*
/*	bounce_recipient_dsn() sends a machine-readable representation of
/*	logfile information for one recipient, with the recipient address
/*	and with the text why the recipient was undeliverable.
/*
/*	bounce_diagnostic_dsn() sends a machine-readable representation of
/*	logfile information for all undeliverable recipients. This routine
/*	will become obsolete when individual recipients of the same message
/*	can have different sender addresses to bounce to.
/*
/*	bounce_original() starts a message/rfc822 or headers/rfc822
/*	message segment and sends the original message, either full or
/*	message headers only.
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <events.h>
#include <vstring.h>
#include <vstream.h>
#include <line_wrap.h>

/* Global library. */

#include <mail_queue.h>
#include <quote_822_local.h>
#include <mail_params.h>
#include <is_header.h>
#include <record.h>
#include <rec_type.h>
#include <post_mail.h>
#include <mail_addr.h>
#include <mail_error.h>
#include <bounce_log.h>
#include <mail_date.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_mail_init - initialize */

BOUNCE_INFO *bounce_mail_init(const char *service, const char *queue_name,
			              const char *queue_id, int flush)
{
    BOUNCE_INFO *bounce_info;
    int     rec_type;

    /*
     * Bundle up a bunch of parameters and initialize information. that will
     * be discovered on the fly.
     */
    bounce_info = (BOUNCE_INFO *) mymalloc(sizeof(*bounce_info));
    bounce_info->service = service;
    bounce_info->queue_name = queue_name;
    bounce_info->queue_id = queue_id;
    bounce_info->flush = flush;
    bounce_info->buf = vstring_alloc(100);
    bounce_info->arrival_time = 0;
    bounce_info->orig_offs = 0;

    /*
     * Compute a supposedly unique boundary string. This assumes that a queue
     * ID and a hostname contain acceptable characters for a boundary string,
     * but the assumption is not verified.
     */
    vstring_sprintf(bounce_info->buf, "%s.%lu/%s",
		    queue_id, (unsigned long) event_time(), var_myhostname);
    bounce_info->mime_boundary = mystrdup(STR(bounce_info->buf));

    /*
     * If the bounce log cannot be found, do not raise a fatal run-time
     * error. There is nothing we can do about the error, and all we are
     * doing is to inform the sender of a delivery problem, Bouncing a
     * message does not have to be a perfect job. But if the system IS
     * running out of resources, raise a fatal run-time error and force a
     * backoff.
     */
    if ((bounce_info->log_handle = bounce_log_open(bounce_info->service,
						   bounce_info->queue_id,
						   O_RDWR, 0)) == 0
	&& errno != ENOENT)
	msg_fatal("open %s %s: %m", bounce_info->service,
		  bounce_info->queue_id);

    /*
     * If the original message cannot be found, do not raise a run-time
     * error. There is nothing we can do about the error, and all we are
     * doing is to inform the sender of a delivery problem. Bouncing a
     * message does not have to be a perfect job. But if the system IS
     * running out of resources, raise a fatal run-time error and force a
     * backoff.
     */
    if ((bounce_info->orig_fp = mail_queue_open(queue_name, queue_id,
						O_RDONLY, 0)) == 0
	&& errno != ENOENT)
	msg_fatal("open %s %s: %m", service, queue_id);

    /*
     * Skip over the original message envelope records. If the envelope is
     * corrupted just send whatever we can (remember this is a best effort,
     * it does not have to be perfect).
     */
    if (bounce_info->orig_fp != 0) {
	while ((rec_type = rec_get(bounce_info->orig_fp,
				   bounce_info->buf, 0)) > 0) {
	    if (rec_type == REC_TYPE_TIME && bounce_info->arrival_time == 0) {
		if ((bounce_info->arrival_time = atol(STR(bounce_info->buf))) < 0)
		    bounce_info->arrival_time = 0;
	    } else if (rec_type == REC_TYPE_MESG) {
		bounce_info->orig_offs = vstream_ftell(bounce_info->orig_fp);
		break;
	    }
	}
    }
    return (bounce_info);
}

/* bounce_mail_free - undo bounce_mail_init */

void    bounce_mail_free(BOUNCE_INFO *bounce_info)
{
    if (bounce_info->log_handle && bounce_log_close(bounce_info->log_handle))
	msg_warn("%s: read bounce log %s: %m",
		 bounce_info->queue_id, bounce_info->queue_id);
    if (bounce_info->orig_fp && vstream_fclose(bounce_info->orig_fp))
	msg_warn("%s: read message file %s %s: %m",
		 bounce_info->queue_id, bounce_info->queue_name,
		 bounce_info->queue_id);
    vstring_free(bounce_info->buf);
    myfree((char *) bounce_info->mime_boundary);
    myfree((char *) bounce_info);
}

/* bounce_header - generate bounce message header */

int     bounce_header(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
		              const char *dest)
{

    /*
     * Print a minimal bounce header. The cleanup service will add other
     * headers and will make all addresses fully qualified.
     */
#define STREQ(a, b) (strcasecmp((a), (b)) == 0)

    post_mail_fprintf(bounce, "From: %s (Mail Delivery System)",
		      MAIL_ADDR_MAIL_DAEMON);

    /*
     * Non-delivery subject line.
     */
    if (bounce_info->flush) {
	post_mail_fputs(bounce, dest == var_bounce_rcpt
		     || dest == var_2bounce_rcpt || dest == var_delay_rcpt ?
			"Subject: Postmaster Copy: Undelivered Mail" :
			"Subject: Undelivered Mail Returned to Sender");
    }

    /*
     * Delayed mail subject line.
     */
    else {
	post_mail_fputs(bounce, dest == var_bounce_rcpt
		     || dest == var_2bounce_rcpt || dest == var_delay_rcpt ?
			"Subject: Postmaster Warning: Delayed Mail" :
			"Subject: Delayed Mail (still being retried)");
    }
    post_mail_fprintf(bounce, "To: %s",
		      STR(quote_822_local(bounce_info->buf, dest)));

    /*
     * MIME header.
     */
    post_mail_fprintf(bounce, "MIME-Version: 1.0");
    post_mail_fprintf(bounce, "Content-Type: %s; report-type=%s;",
		      "multipart/report", "delivery-status");
    post_mail_fprintf(bounce, "\tboundary=\"%s\"", bounce_info->mime_boundary);
    post_mail_fputs(bounce, "");
    post_mail_fputs(bounce, "This is a MIME-encapsulated message.");

    /*
     * MIME header.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", bounce_info->mime_boundary);
    post_mail_fprintf(bounce, "Content-Description: %s", "Notification");
    post_mail_fprintf(bounce, "Content-Type: %s", "text/plain");
    post_mail_fputs(bounce, "");

    return (vstream_ferror(bounce));
}

/* bounce_boilerplate - generate boiler-plate text */

int     bounce_boilerplate(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
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
    if (bounce_info->flush) {
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
		      "Your message could not be delivered for %.1f hours.",
			  var_delay_warn_time / 3600.0);
	post_mail_fprintf(bounce,
			  "It will be retried until it is %.1f days old.",
			  var_max_queue_time / 86400.0);
    }

    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce,
		      "For further assistance, please send mail to <%s>",
		      MAIL_ADDR_POSTMASTER);
    if (bounce_info->flush) {
	post_mail_fputs(bounce, "");
	post_mail_fprintf(bounce,
	       "If you do so, please include this problem report. You can");
	post_mail_fprintf(bounce,
		   "delete your own text from the message returned below.");
    }
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "\t\t\tThe %s program", var_mail_name);
    return (vstream_ferror(bounce));
}

/* bounce_print - line_wrap callback */

static void bounce_print(const char *str, int len, int indent, char *context)
{
    VSTREAM *bounce = (VSTREAM *) context;

    post_mail_fprintf(bounce, "%*s%.*s", indent, "", len, str);
}

/* bounce_print_wrap - print and wrap a line */

static void bounce_print_wrap(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
			              const char *format,...)
{
    va_list ap;

#define LENGTH	79
#define INDENT	4

    va_start(ap, format);
    vstring_vsprintf(bounce_info->buf, format, ap);
    va_end(ap);
    line_wrap(STR(bounce_info->buf), LENGTH, INDENT,
	      bounce_print, (char *) bounce);
}

/* bounce_recipient_log - send one bounce log report entry */

int     bounce_recipient_log(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{

    /*
     * Mask control and non-ASCII characters (done in bounce_log_read()),
     * wrap long lines and prepend one blank, so this data can safely be
     * piped into other programs. Sort of like TCP Wrapper's safe_finger
     * program.
     */
    post_mail_fputs(bounce, "");
    bounce_print_wrap(bounce, bounce_info, "<%s>: %s",
	 bounce_info->log_handle->recipient, bounce_info->log_handle->text);
    return (vstream_ferror(bounce));
}

/* bounce_diagnostic_log - send bounce log report */

int     bounce_diagnostic_log(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{

    /*
     * Append a copy of the delivery error log. We're doing a best effort, so
     * there is no point raising a fatal run-time error in case of a logfile
     * read error.
     */
    if (bounce_info->log_handle == 0
	|| bounce_log_rewind(bounce_info->log_handle)) {
	post_mail_fputs(bounce, "\t--- Delivery error report unavailable ---");
    } else {
	while (bounce_log_read(bounce_info->log_handle) != 0)
	    if (bounce_recipient_log(bounce, bounce_info) != 0)
		break;
    }
    return (vstream_ferror(bounce));
}

/* bounce_header_dsn - send per-MTA bounce DSN records */

int     bounce_header_dsn(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{

    /*
     * MIME header.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", bounce_info->mime_boundary);
    post_mail_fprintf(bounce, "Content-Description: %s",
		      "Delivery error report");
    post_mail_fprintf(bounce, "Content-Type: %s", "message/delivery-status");

    /*
     * According to RFC 1894: The body of a message/delivery-status consists
     * of one or more "fields" formatted according to the ABNF of RFC 822
     * header "fields" (see [6]).  The per-message fields appear first,
     * followed by a blank line.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "Reporting-MTA: dns; %s", var_myhostname);
#if 0
    post_mail_fprintf(bounce, "Received-From-MTA: dns; %s", "whatever");
#endif
    if (bounce_info->arrival_time > 0)
	post_mail_fprintf(bounce, "Arrival-Date: %s",
			  mail_date(bounce_info->arrival_time));
    return (vstream_ferror(bounce));
}

/* bounce_recipient_dsn - send per-recipient DSN records */

int     bounce_recipient_dsn(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{
    post_mail_fputs(bounce, "");
#if 0
    post_mail_fprintf(bounce, "Original-Recipient: rfc822; %s", "whatever");
#endif
    post_mail_fprintf(bounce, "Final-Recipient: rfc822; %s",
		      bounce_info->log_handle->recipient);
    post_mail_fprintf(bounce, "Action: %s", bounce_info->flush ?
		      "failed" : "delayed");
    post_mail_fprintf(bounce, "Status: %s", bounce_info->log_handle->status);
    bounce_print_wrap(bounce, bounce_info, "Diagnostic-Code: X-Postfix; %s",
		      bounce_info->log_handle->text);
#if 0
    post_mail_fprintf(bounce, "Last-Attempt-Date: %s",
		      bounce_info->log_handle->log_time);
#endif
    if (bounce_info->flush == 0)
	post_mail_fprintf(bounce, "Will-Retry-Until: %s",
		 mail_date(bounce_info->arrival_time + var_max_queue_time));
    return (vstream_ferror(bounce));
}

/* bounce_diagnostic_dsn - send bounce log report, machine readable form */

int     bounce_diagnostic_dsn(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{

    /*
     * Append a copy of the delivery error log. We're doing a best effort, so
     * there is no point raising a fatal run-time error in case of a logfile
     * read error.
     */
    if (bounce_info->log_handle != 0
	&& bounce_log_rewind(bounce_info->log_handle) == 0) {
	while (bounce_log_read(bounce_info->log_handle) != 0)
	    if (bounce_recipient_dsn(bounce, bounce_info) != 0)
		break;
    }
    return (vstream_ferror(bounce));
}

/* bounce_original - send a copy of the original to the victim */

int     bounce_original(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
			        int headers_only)
{
    int     status = 0;
    int     rec_type = 0;
    int     bounce_length;

    /*
     * MIME headers.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", bounce_info->mime_boundary);
    post_mail_fprintf(bounce, "Content-Description: %s", headers_only ?
		      "Undelivered Message Headers" : "Undelivered Message");
    post_mail_fprintf(bounce, "Content-Type: %s", headers_only ?
		      "text/rfc822-headers" : "message/rfc822");
    post_mail_fputs(bounce, "");

    /*
     * Send place holder if original is unavailable.
     */
    if (bounce_info->orig_offs == 0 || vstream_fseek(bounce_info->orig_fp,
				    bounce_info->orig_offs, SEEK_SET) < 0) {
	post_mail_fputs(bounce, "\t--- Undelivered message unavailable ---");
	return (vstream_ferror(bounce));
    }

    /*
     * Copy the original message contents. Limit the amount of bounced text
     * so there is a better chance of the bounce making it back. We're doing
     * raw record output here so that we don't throw away binary transparency
     * yet.
     */
#define IS_HEADER(s) (ISSPACE(*(s)) || is_header(s))

    bounce_length = 0;
    while (status == 0 && (rec_type = rec_get(bounce_info->orig_fp, bounce_info->buf, 0)) > 0) {
	if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
	    break;
	if (headers_only && !IS_HEADER(vstring_str(bounce_info->buf)))
	    break;
	if (var_bounce_limit == 0 || bounce_length < var_bounce_limit) {
	    bounce_length += VSTRING_LEN(bounce_info->buf) + 2;
	    status = (REC_PUT_BUF(bounce, rec_type, bounce_info->buf) != rec_type);
	} else
	    break;
    }

    /*
     * Final MIME headers. These require -- at the end of the boundary
     * string.
     * 
     * XXX This should be a separate bounce_terminate() entry so we can be
     * assured that the terminator will always be sent.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s--", bounce_info->mime_boundary);

    return (status);
}
