/*	$NetBSD: bounce_notify_util.c,v 1.1.1.1 2009/06/23 10:08:42 tron Exp $	*/

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
/*	BOUNCE_INFO *bounce_mail_init(service, queue_name, queue_id,
/*					encoding, dsn_envid, template)
/*	const char *service;
/*	const char *queue_name;
/*	const char *queue_id;
/*	const char *encoding;
/*	const char *dsn_envid;
/*	const BOUNCE_TEMPLATE *template;
/*
/*	BOUNCE_INFO *bounce_mail_one_init(queue_name, queue_id, encoding,
/*					dsn_envid, dsn_notify, rcpt_buf,
/*					dsn_buf, template)
/*	const char *queue_name;
/*	const char *queue_id;
/*	const char *encoding;
/*	int	dsn_notify;
/*	const char *dsn_envid;
/*	RCPT_BUF *rcpt_buf;
/*	DSN_BUF	*dsn_buf;
/*	const BOUNCE_TEMPLATE *template;
/*
/*	void	bounce_mail_free(bounce_info)
/*	BOUNCE_INFO *bounce_info;
/*
/*	int	bounce_header(fp, bounce_info, recipient, postmaster_copy)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*	const char *recipient;
/*	int	postmaster_copy;
/*
/*	int	bounce_boilerplate(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_recipient_log(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_diagnostic_log(fp, bounce_info, notify_filter)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*	int	notify_filter;
/*
/*	int     bounce_header_dsn(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_recipient_dsn(fp, bounce_info)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*
/*	int     bounce_diagnostic_dsn(fp, bounce_info, notify_filter)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*	int	notify_filter;
/*
/*	int	bounce_original(fp, bounce_info, headers_only)
/*	VSTREAM *fp;
/*	BOUNCE_INFO *bounce_info;
/*	int	headers_only;
/*
/*	void	bounce_delrcpt(bounce_info)
/*	BOUNCE_INFO *bounce_info;
/*
/*	void	bounce_delrcpt_one(bounce_info)
/*	BOUNCE_INFO *bounce_info;
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
/*	bounce_mail_one_init() provides the same function for only
/*	one recipient that is not read from bounce logfile.
/*
/*	bounce_mail_free() releases memory allocated by bounce_mail_init()
/*	and closes any files opened by bounce_mail_init().
/*
/*	bounce_header() produces a standard mail header with the specified
/*	recipient and starts a text/plain message segment for the
/*	human-readable problem description. postmaster_copy is either
/*	POSTMASTER_COPY or NO_POSTMASTER_COPY.
/*
/*	bounce_boilerplate() produces the standard "sorry" text that
/*	creates the illusion that mail systems are civilized.
/*
/*	bounce_recipient_log() sends a human-readable representation of
/*	logfile information for one recipient, with the recipient address
/*	and with the text why the recipient was undeliverable.
/*
/*	bounce_diagnostic_log() sends a human-readable representation of
/*	logfile information for all undeliverable recipients. The
/*	notify_filter specifies what recipient status records should be
/*	reported: DSN_NOTIFY_SUCCESS, DSN_NOTIFY_FAILURE, DSN_NOTIFY_DELAY.
/*	In the absence of DSN NOTIFY information all records are reported.
/*	The result value is -1 in case of error, the number of reported
/*	recipients in case of success.
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
/*	logfile information for all undeliverable recipients. The
/*	notify_filter specifies what recipient status records should be
/*	reported: DSN_NOTIFY_SUCCESS, DSN_NOTIFY_FAILURE, DSN_NOTIFY_DELAY.
/*	In the absence of DSN NOTIFY information all records are reported.
/*	The result value is -1 in case of error, the number of reported
/*	recipients in case of success.
/*
/*	bounce_original() starts a message/rfc822 or text/rfc822-headers
/*	message segment and sends the original message, either full
/*	(DSN_RET_FULL) or message headers only (DSN_RET_HDRS).
/*
/*	bounce_delrcpt() deletes recipients in the logfile from the original
/*	queue file.
/*
/*	bounce_delrcpt_one() deletes one recipient from the original
/*	queue file.
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
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>			/* sscanf() */
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
#include <stringops.h>
#include <myflock.h>

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
#include <mail_proto.h>
#include <lex_822.h>
#include <deliver_completed.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_mail_alloc - initialize */

static BOUNCE_INFO *bounce_mail_alloc(const char *service,
				              const char *queue_name,
				              const char *queue_id,
				              const char *encoding,
				              const char *dsn_envid,
				              RCPT_BUF *rcpt_buf,
				              DSN_BUF *dsn_buf,
				              BOUNCE_TEMPLATE *template,
				              BOUNCE_LOG *log_handle)
{
    BOUNCE_INFO *bounce_info;
    int     rec_type;

    /*
     * Bundle up a bunch of parameters and initialize information that will
     * be discovered on the fly.
     */
    bounce_info = (BOUNCE_INFO *) mymalloc(sizeof(*bounce_info));
    bounce_info->service = service;
    bounce_info->queue_name = queue_name;
    bounce_info->queue_id = queue_id;
    if (strcmp(encoding, MAIL_ATTR_ENC_8BIT) == 0) {
	bounce_info->mime_encoding = "8bit";
    } else if (strcmp(encoding, MAIL_ATTR_ENC_7BIT) == 0) {
	bounce_info->mime_encoding = "7bit";
    } else {
	if (strcmp(encoding, MAIL_ATTR_ENC_NONE) != 0)
	    msg_warn("%s: unknown encoding: %.200s",
		     bounce_info->queue_id, encoding);
	bounce_info->mime_encoding = 0;
    }
    if (dsn_envid && *dsn_envid)
	bounce_info->dsn_envid = dsn_envid;
    else
	bounce_info->dsn_envid = 0;
    bounce_info->template = template;
    bounce_info->buf = vstring_alloc(100);
    bounce_info->sender = vstring_alloc(100);
    bounce_info->arrival_time = 0;
    bounce_info->orig_offs = 0;
    bounce_info->message_size = 0;
    bounce_info->rcpt_buf = rcpt_buf;
    bounce_info->dsn_buf = dsn_buf;
    bounce_info->log_handle = log_handle;

    /*
     * RFC 1894: diagnostic-type is an RFC 822 atom. We use X-$mail_name and
     * must ensure it is valid.
     */
    bounce_info->mail_name = mystrdup(var_mail_name);
    translit(bounce_info->mail_name, " \t\r\n()<>@,;:\\\".[]",
	     "-----------------");

    /*
     * Compute a supposedly unique boundary string. This assumes that a queue
     * ID and a hostname contain acceptable characters for a boundary string,
     * but the assumption is not verified.
     */
    vstring_sprintf(bounce_info->buf, "%s.%lu/%s",
		    queue_id, (unsigned long) event_time(), var_myhostname);
    bounce_info->mime_boundary = mystrdup(STR(bounce_info->buf));

    /*
     * If the original message cannot be found, do not raise a run-time
     * error. There is nothing we can do about the error, and all we are
     * doing is to inform the sender of a delivery problem. Bouncing a
     * message does not have to be a perfect job. But if the system IS
     * running out of resources, raise a fatal run-time error and force a
     * backoff.
     */
    if ((bounce_info->orig_fp = mail_queue_open(queue_name, queue_id,
						O_RDWR, 0)) == 0
	&& errno != ENOENT)
	msg_fatal("open %s %s: %m", service, queue_id);

    /*
     * Get time/size/sender information from the original message envelope
     * records. If the envelope is corrupted just send whatever we can
     * (remember this is a best effort, it does not have to be perfect).
     * 
     * Lock the file for shared use, so that queue manager leaves it alone after
     * restarting.
     */
#define DELIVER_LOCK_MODE (MYFLOCK_OP_SHARED | MYFLOCK_OP_NOWAIT)

    if (bounce_info->orig_fp != 0) {
	if (myflock(vstream_fileno(bounce_info->orig_fp), INTERNAL_LOCK,
		    DELIVER_LOCK_MODE) < 0)
	    msg_fatal("cannot get shared lock on %s: %m",
		      VSTREAM_PATH(bounce_info->orig_fp));
	while ((rec_type = rec_get(bounce_info->orig_fp,
				   bounce_info->buf, 0)) > 0) {

	    /*
	     * Postfix version dependent: data offset in SIZE record.
	     */
	    if (rec_type == REC_TYPE_SIZE) {
		if (bounce_info->message_size == 0)
		    sscanf(STR(bounce_info->buf), "%ld %ld",
			   &bounce_info->message_size,
			   &bounce_info->orig_offs);
		if (bounce_info->message_size < 0)
		    bounce_info->message_size = 0;
		if (bounce_info->orig_offs < 0)
		    bounce_info->orig_offs = 0;
	    }

	    /*
	     * Information for the Arrival-Date: attribute.
	     */
	    else if (rec_type == REC_TYPE_TIME) {
		if (bounce_info->arrival_time == 0
		    && (bounce_info->arrival_time = atol(STR(bounce_info->buf))) < 0)
		    bounce_info->arrival_time = 0;
	    }

	    /*
	     * Information for the X-Postfix-Sender: attribute.
	     */
	    else if (rec_type == REC_TYPE_FROM) {
		quote_822_local_flags(bounce_info->sender,
				      VSTRING_LEN(bounce_info->buf) ?
				      STR(bounce_info->buf) :
				      mail_addr_mail_daemon(), 0);
	    }

	    /*
	     * Backwards compatibility: no data offset in SIZE record.
	     */
	    else if (rec_type == REC_TYPE_MESG) {
		/* XXX Future: sender+recipient after message content. */
		if (VSTRING_LEN(bounce_info->sender) == 0)
		    msg_warn("%s: no sender before message content record",
			     bounce_info->queue_id);
		bounce_info->orig_offs = vstream_ftell(bounce_info->orig_fp);
		break;
	    }
	    if (bounce_info->orig_offs > 0
		&& bounce_info->arrival_time > 0
		&& VSTRING_LEN(bounce_info->sender) > 0)
		break;
	}
    }
    return (bounce_info);
}

/* bounce_mail_init - initialize */

BOUNCE_INFO *bounce_mail_init(const char *service,
			              const char *queue_name,
			              const char *queue_id,
			              const char *encoding,
			              const char *dsn_envid,
			              BOUNCE_TEMPLATE *template)
{
    BOUNCE_INFO *bounce_info;
    BOUNCE_LOG *log_handle;
    RCPT_BUF *rcpt_buf;
    DSN_BUF *dsn_buf;

    /*
     * Initialize the bounce_info structure. If the bounce log cannot be
     * found, do not raise a fatal run-time error. There is nothing we can do
     * about the error, and all we are doing is to inform the sender of a
     * delivery problem, Bouncing a message does not have to be a perfect
     * job. But if the system IS running out of resources, raise a fatal
     * run-time error and force a backoff.
     */
    if ((log_handle = bounce_log_open(service, queue_id, O_RDONLY, 0)) == 0) {
	if (errno != ENOENT)
	    msg_fatal("open %s %s: %m", service, queue_id);
	rcpt_buf = 0;
	dsn_buf = 0;
    } else {
	rcpt_buf = rcpb_create();
	dsn_buf = dsb_create();
    }
    bounce_info = bounce_mail_alloc(service, queue_name, queue_id, encoding,
				    dsn_envid, rcpt_buf, dsn_buf,
				    template, log_handle);
    return (bounce_info);
}

/* bounce_mail_one_init - initialize */

BOUNCE_INFO *bounce_mail_one_init(const char *queue_name,
				          const char *queue_id,
				          const char *encoding,
				          const char *dsn_envid,
				          RCPT_BUF *rcpt_buf,
				          DSN_BUF *dsn_buf,
				          BOUNCE_TEMPLATE *template)
{
    BOUNCE_INFO *bounce_info;

    /*
     * Initialize the bounce_info structure for just one recipient.
     */
    bounce_info = bounce_mail_alloc("none", queue_name, queue_id, encoding,
				    dsn_envid, rcpt_buf, dsn_buf, template,
				    (BOUNCE_LOG *) 0);
    return (bounce_info);
}

/* bounce_mail_free - undo bounce_mail_init */

void    bounce_mail_free(BOUNCE_INFO *bounce_info)
{
    if (bounce_info->log_handle) {
	if (bounce_log_close(bounce_info->log_handle))
	    msg_warn("%s: read bounce log %s: %m",
		     bounce_info->queue_id, bounce_info->queue_id);
	rcpb_free(bounce_info->rcpt_buf);
	dsb_free(bounce_info->dsn_buf);
    }
    if (bounce_info->orig_fp && vstream_fclose(bounce_info->orig_fp))
	msg_warn("%s: read message file %s %s: %m",
		 bounce_info->queue_id, bounce_info->queue_name,
		 bounce_info->queue_id);
    vstring_free(bounce_info->buf);
    vstring_free(bounce_info->sender);
    myfree(bounce_info->mail_name);
    myfree((char *) bounce_info->mime_boundary);
    myfree((char *) bounce_info);
}

/* bounce_header - generate bounce message header */

int     bounce_header(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
		              const char *dest, int postmaster_copy)
{
    BOUNCE_TEMPLATE *template = bounce_info->template;

    /*
     * Print a minimal bounce header. The cleanup service will add other
     * headers and will make all addresses fully qualified.
     */
#define STREQ(a, b) (strcasecmp((a), (b)) == 0)

    /*
     * Generic headers.
     */
    bounce_template_headers(post_mail_fprintf, bounce, template,
			    STR(quote_822_local(bounce_info->buf, dest)),
			    postmaster_copy);

    /*
     * Auto-Submitted header, as per RFC 3834.
     */
    post_mail_fprintf(bounce, "Auto-Submitted: %s", postmaster_copy ?
		      "auto-generated" : "auto-replied");

    /*
     * MIME header. Use 8bit encoding when either the bounced message or the
     * template requires it.
     */
    post_mail_fprintf(bounce, "MIME-Version: 1.0");
    post_mail_fprintf(bounce, "Content-Type: %s; report-type=%s;",
		      "multipart/report", "delivery-status");
    post_mail_fprintf(bounce, "\tboundary=\"%s\"", bounce_info->mime_boundary);
    if (bounce_info->mime_encoding)
	post_mail_fprintf(bounce, "Content-Transfer-Encoding: %s",
		     STREQ(bounce_info->mime_encoding, MAIL_ATTR_ENC_7BIT) ?
			  bounce_template_encoding(template) :
			  bounce_info->mime_encoding);
    post_mail_fputs(bounce, "");
    post_mail_fputs(bounce, "This is a MIME-encapsulated message.");

    /*
     * MIME header.
     */
    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", bounce_info->mime_boundary);
    post_mail_fprintf(bounce, "Content-Description: %s", "Notification");
    post_mail_fprintf(bounce, "Content-Type: %s; charset=%s",
		      "text/plain", bounce_template_charset(template));
    post_mail_fputs(bounce, "");

    return (vstream_ferror(bounce));
}

/* bounce_boilerplate - generate boiler-plate text */

int     bounce_boilerplate(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{

    /*
     * Print the boiler-plate text.
     */
    bounce_template_expand(post_mail_fputs, bounce, bounce_info->template);
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
    RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;
    DSN    *dsn = &bounce_info->dsn_buf->dsn;

    /*
     * Mask control and non-ASCII characters (done in bounce_log_read()),
     * wrap long lines and prepend one blank, so this data can safely be
     * piped into other programs. Sort of like TCP Wrapper's safe_finger
     * program.
     */
#define NON_NULL_EMPTY(s) ((s) && *(s))

    post_mail_fputs(bounce, "");
    if (NON_NULL_EMPTY(rcpt->orig_addr)) {
	bounce_print_wrap(bounce, bounce_info, "<%s> (expanded from <%s>): %s",
			  rcpt->address, rcpt->orig_addr, dsn->reason);
    } else {
	bounce_print_wrap(bounce, bounce_info, "<%s>: %s",
			  rcpt->address, dsn->reason);
    }
    return (vstream_ferror(bounce));
}

/* bounce_diagnostic_log - send bounce log report */

int     bounce_diagnostic_log(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
			              int notify_filter)
{
    RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;
    int     count = 0;

    /*
     * Append a human-readable copy of the delivery error log. We're doing a
     * best effort, so there is no point raising a fatal run-time error in
     * case of a logfile read error.
     * 
     * XXX DSN If the logfile with failed recipients is unavailable, pretend
     * that we found something anyway, so that this notification will not be
     * canceled.
     */
    if (bounce_info->log_handle == 0
	|| bounce_log_rewind(bounce_info->log_handle)) {
	if (IS_FAILURE_TEMPLATE(bounce_info->template)) {
	    post_mail_fputs(bounce, "");
	    post_mail_fputs(bounce, "\t--- Delivery report unavailable ---");
	    count = 1;				/* XXX don't abort */
	}
    } else {
	while (bounce_log_read(bounce_info->log_handle, bounce_info->rcpt_buf,
			       bounce_info->dsn_buf) != 0) {
	    if (rcpt->dsn_notify == 0		/* compat */
		|| (rcpt->dsn_notify & notify_filter)) {
		count++;
		if (bounce_recipient_log(bounce, bounce_info) != 0)
		    break;
	    }
	}
    }
    return (vstream_ferror(bounce) ? -1 : count);
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
		      "Delivery report");
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
    if (NON_NULL_EMPTY(bounce_info->dsn_envid)) {
	post_mail_fprintf(bounce, "Original-Envelope-Id: %s",
			  bounce_info->dsn_envid);
    }
    post_mail_fprintf(bounce, "X-%s-Queue-ID: %s",
		      bounce_info->mail_name, bounce_info->queue_id);
    if (VSTRING_LEN(bounce_info->sender) > 0)
	post_mail_fprintf(bounce, "X-%s-Sender: rfc822; %s",
			  bounce_info->mail_name, STR(bounce_info->sender));
    if (bounce_info->arrival_time > 0)
	post_mail_fprintf(bounce, "Arrival-Date: %s",
			  mail_date(bounce_info->arrival_time));
    return (vstream_ferror(bounce));
}

/* bounce_recipient_dsn - send per-recipient DSN records */

int     bounce_recipient_dsn(VSTREAM *bounce, BOUNCE_INFO *bounce_info)
{
    RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;
    DSN    *dsn = &bounce_info->dsn_buf->dsn;

    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "Final-Recipient: rfc822; %s", rcpt->address);

    /*
     * XXX DSN
     * 
     * RFC 3464 section 6.3.d: "If no ORCPT parameter was provided for this
     * recipient, the Original-Recipient field MUST NOT appear."
     * 
     * This is inconsistent with section 5.2.1.d: "If no ORCPT parameter was
     * present in the RCPT command when the message was received, an ORCPT
     * parameter MAY be added to the RCPT command when the message is
     * relayed.". Postfix adds an ORCPT parameter under these conditions.
     * 
     * Therefore, all down-stream MTAs will send DSNs with Original-Recipient
     * field ontaining this same ORCPT value. When a down-stream MTA can use
     * that information in their DSNs, it makes no sense that an up-stream
     * MTA can't use that same information in its own DSNs.
     * 
     * Postfix always reports an Original-Recipient field, because it is more
     * more useful and more consistent.
     */
    if (NON_NULL_EMPTY(rcpt->dsn_orcpt)) {
	post_mail_fprintf(bounce, "Original-Recipient: %s", rcpt->dsn_orcpt);
    } else if (NON_NULL_EMPTY(rcpt->orig_addr)) {
	post_mail_fprintf(bounce, "Original-Recipient: rfc822; %s",
			  rcpt->orig_addr);
    }
    post_mail_fprintf(bounce, "Action: %s",
		      IS_FAILURE_TEMPLATE(bounce_info->template) ?
		      "failed" : dsn->action);
    post_mail_fprintf(bounce, "Status: %s", dsn->status);
    if (NON_NULL_EMPTY(dsn->mtype) && NON_NULL_EMPTY(dsn->mname))
	bounce_print_wrap(bounce, bounce_info, "Remote-MTA: %s; %s",
			  dsn->mtype, dsn->mname);
    if (NON_NULL_EMPTY(dsn->dtype) && NON_NULL_EMPTY(dsn->dtext))
	bounce_print_wrap(bounce, bounce_info, "Diagnostic-Code: %s; %s",
			  dsn->dtype, dsn->dtext);
    else
	bounce_print_wrap(bounce, bounce_info, "Diagnostic-Code: X-%s; %s",
			  bounce_info->mail_name, dsn->reason);
#if 0
    if (dsn->time > 0)
	post_mail_fprintf(bounce, "Last-Attempt-Date: %s",
			  mail_date(dsn->time));
#endif
    if (IS_DELAY_TEMPLATE(bounce_info->template))
	post_mail_fprintf(bounce, "Will-Retry-Until: %s",
		 mail_date(bounce_info->arrival_time + var_max_queue_time));
    return (vstream_ferror(bounce));
}

/* bounce_diagnostic_dsn - send bounce log report, machine readable form */

int     bounce_diagnostic_dsn(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
			              int notify_filter)
{
    RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;
    int     count = 0;

    /*
     * Append a machine-readable copy of the delivery error log. We're doing
     * a best effort, so there is no point raising a fatal run-time error in
     * case of a logfile read error.
     * 
     * XXX DSN If the logfile with failed recipients is unavailable, pretend
     * that we found something anyway, so that this notification will not be
     * canceled.
     */
    if (bounce_info->log_handle == 0
	|| bounce_log_rewind(bounce_info->log_handle)) {
	if (IS_FAILURE_TEMPLATE(bounce_info->template))
	    count = 1;				/* XXX don't abort */
    } else {
	while (bounce_log_read(bounce_info->log_handle, bounce_info->rcpt_buf,
			       bounce_info->dsn_buf) != 0) {
	    if (rcpt->dsn_notify == 0		/* compat */
		|| (rcpt->dsn_notify & notify_filter)) {
		count++;
		if (bounce_recipient_dsn(bounce, bounce_info) != 0)
		    break;
	    }
	}
    }
    return (vstream_ferror(bounce) ? -1 : count);
}

/* bounce_original - send a copy of the original to the victim */

int     bounce_original(VSTREAM *bounce, BOUNCE_INFO *bounce_info,
			        int headers_only)
{
    int     status = 0;
    int     rec_type = 0;

    /*
     * When truncating a large message, don't damage the MIME structure: send
     * the message headers only.
     */
    if (var_bounce_limit > 0
	&& bounce_info->orig_fp
	&& (bounce_info->message_size <= 0
	    || bounce_info->message_size > var_bounce_limit))
	headers_only = DSN_RET_HDRS;

    /*
     * MIME headers.
     */
#define IS_UNDELIVERED_TEMPLATE(template) \
        (IS_FAILURE_TEMPLATE(template) || IS_DELAY_TEMPLATE(template))

    post_mail_fputs(bounce, "");
    post_mail_fprintf(bounce, "--%s", bounce_info->mime_boundary);
    post_mail_fprintf(bounce, "Content-Description: %s%s",
		      IS_UNDELIVERED_TEMPLATE(bounce_info->template) ?
		      "Undelivered " : "",
		      headers_only == DSN_RET_HDRS ?
		      "Message Headers" : "Message");
    post_mail_fprintf(bounce, "Content-Type: %s",
		      headers_only == DSN_RET_HDRS ?
		      "text/rfc822-headers" : "message/rfc822");
    if (bounce_info->mime_encoding)
	post_mail_fprintf(bounce, "Content-Transfer-Encoding: %s",
			  bounce_info->mime_encoding);
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
     * XXX The cleanup server removes Return-Path: headers. This should be
     * done only with mail that enters via a non-SMTP channel, but changing
     * this now could break other software. Removing Return-Path: could break
     * digital signatures, though this is unlikely. In any case,
     * header_checks are more effective when the Return-Path: header is
     * present, so we prepend one to the bounce message.
     */
    post_mail_fprintf(bounce, "Return-Path: <%s>", STR(bounce_info->sender));

    /*
     * Copy the original message contents. We're doing raw record output here
     * so that we don't throw away binary transparency yet.
     */
#define IS_HEADER(s) (IS_SPACE_TAB(*(s)) || is_header(s))

    while (status == 0 && (rec_type = rec_get(bounce_info->orig_fp, bounce_info->buf, 0)) > 0) {
	if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
	    break;
	if (headers_only == DSN_RET_HDRS
	    && !IS_HEADER(vstring_str(bounce_info->buf)))
	    break;
	status = (REC_PUT_BUF(bounce, rec_type, bounce_info->buf) != rec_type);
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

/* bounce_delrcpt - delete recipients from original queue file */

void    bounce_delrcpt(BOUNCE_INFO *bounce_info)
{
    RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;

    if (bounce_info->orig_fp != 0
	&& bounce_info->log_handle != 0
	&& bounce_log_rewind(bounce_info->log_handle) == 0)
	while (bounce_log_read(bounce_info->log_handle, bounce_info->rcpt_buf,
			       bounce_info->dsn_buf) != 0)
	    if (rcpt->offset > 0)
		deliver_completed(bounce_info->orig_fp, rcpt->offset);
}

/* bounce_delrcpt_one - delete one recipient from original queue file */

void    bounce_delrcpt_one(BOUNCE_INFO *bounce_info)
{
    RECIPIENT *rcpt = &bounce_info->rcpt_buf->rcpt;

    if (bounce_info->orig_fp != 0 && rcpt->offset > 0)
	deliver_completed(bounce_info->orig_fp, rcpt->offset);
}
