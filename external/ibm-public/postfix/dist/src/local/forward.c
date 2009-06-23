/*	$NetBSD: forward.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	forward 3
/* SUMMARY
/*	message forwarding
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	forward_init()
/*
/*	int	forward_append(attr)
/*	DELIVER_ATTR attr;
/*
/*	int	forward_finish(request, attr, cancel)
/*	DELIVER_REQUEST *request;
/*	DELIVER_ATTR attr;
/*	int	cancel;
/* DESCRIPTION
/*	This module implements the client interface for message
/*	forwarding.
/*
/*	forward_init() initializes internal data structures.
/*
/*	forward_append() appends a recipient to the list of recipients
/*	that will receive a message with the specified message sender
/*	and delivered-to addresses.
/*
/*	forward_finish() forwards the actual message contents and
/*	releases the memory allocated by forward_init() and by
/*	forward_append(). When the \fIcancel\fR argument is true, no
/*	messages will be forwarded. The \fIattr\fR argument specifies
/*	the original message delivery attributes as they were before
/*	alias or forward expansions.
/* DIAGNOSTICS
/*	A non-zero result means that the requested operation should
/*	be tried again.
/*	Warnings: problems connecting to the forwarding service,
/*	corrupt message file. A corrupt message is saved to the
/*	"corrupt" queue for further inspection.
/*	Fatal: out of memory.
/*	Panic: missing forward_init() or forward_finish() call.
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
#include <sys/time.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <argv.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <iostuff.h>
#include <stringops.h>

/* Global library. */

#include <mail_proto.h>
#include <cleanup_user.h>
#include <sent.h>
#include <record.h>
#include <rec_type.h>
#include <mark_corrupt.h>
#include <mail_date.h>
#include <mail_params.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "local.h"

 /*
  * Use one cleanup service connection for each (delivered to, sender) pair.
  */
static HTABLE *forward_dt;

typedef struct FORWARD_INFO {
    VSTREAM *cleanup;			/* clean up service handle */
    char   *queue_id;			/* forwarded message queue id */
    struct timeval posting_time;	/* posting time */
} FORWARD_INFO;

/* forward_init - prepare for forwarding */

int     forward_init(void)
{

    /*
     * Sanity checks.
     */
    if (forward_dt != 0)
	msg_panic("forward_init: missing forward_finish call");

    forward_dt = htable_create(0);
    return (0);
}

/* forward_open - open connection to cleanup service */

static FORWARD_INFO *forward_open(DELIVER_REQUEST *request, const char *sender)
{
    VSTRING *buffer = vstring_alloc(100);
    FORWARD_INFO *info;
    VSTREAM *cleanup;

    /*
     * Contact the cleanup service and save the new mail queue id. Request
     * that the cleanup service bounces bad messages to the sender so that we
     * can avoid the trouble of bounce management.
     * 
     * In case you wonder what kind of bounces, examples are "too many hops",
     * "message too large", perhaps some others. The reason not to bounce
     * ourselves is that we don't really know who the recipients are.
     */
    cleanup = mail_connect(MAIL_CLASS_PUBLIC, var_cleanup_service, BLOCKING);
    if (cleanup == 0)
	return (0);
    close_on_exec(vstream_fileno(cleanup), CLOSE_ON_EXEC);
    if (attr_scan(cleanup, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, buffer,
		  ATTR_TYPE_END) != 1) {
	vstream_fclose(cleanup);
	return (0);
    }
    info = (FORWARD_INFO *) mymalloc(sizeof(FORWARD_INFO));
    info->cleanup = cleanup;
    info->queue_id = mystrdup(STR(buffer));
    GETTIMEOFDAY(&info->posting_time);

#define FORWARD_CLEANUP_FLAGS (CLEANUP_FLAG_BOUNCE | CLEANUP_FLAG_MASK_INTERNAL)

    attr_print(cleanup, ATTR_FLAG_NONE,
	       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, FORWARD_CLEANUP_FLAGS,
	       ATTR_TYPE_END);

    /*
     * Send initial message envelope information. For bounces, set the
     * designated sender: mailing list owner, posting user, whatever.
     */
    rec_fprintf(cleanup, REC_TYPE_TIME, REC_TYPE_TIME_FORMAT,
		REC_TYPE_TIME_ARG(info->posting_time));
    rec_fputs(cleanup, REC_TYPE_FROM, sender);

    /*
     * Don't send the original envelope ID or full/headers return mask if it
     * was reset due to mailing list expansion.
     */
    if (request->dsn_ret)
	rec_fprintf(cleanup, REC_TYPE_ATTR, "%s=%d",
		    MAIL_ATTR_DSN_RET, request->dsn_ret);
    if (request->dsn_envid && *(request->dsn_envid))
	rec_fprintf(cleanup, REC_TYPE_ATTR, "%s=%s",
		    MAIL_ATTR_DSN_ENVID, request->dsn_envid);

    /*
     * Zero-length attribute values are place holders for unavailable
     * attribute values. See qmgr_message.c. They are not meant to be
     * propagated to queue files.
     */
#define PASS_ATTR(fp, name, value) do { \
    if ((value) && *(value)) \
	rec_fprintf((fp), REC_TYPE_ATTR, "%s=%s", (name), (value)); \
    } while (0)

    PASS_ATTR(cleanup, MAIL_ATTR_LOG_CLIENT_NAME, request->client_name);
    PASS_ATTR(cleanup, MAIL_ATTR_LOG_CLIENT_ADDR, request->client_addr);
    PASS_ATTR(cleanup, MAIL_ATTR_LOG_PROTO_NAME, request->client_proto);
    PASS_ATTR(cleanup, MAIL_ATTR_LOG_HELO_NAME, request->client_helo);
    PASS_ATTR(cleanup, MAIL_ATTR_SASL_METHOD, request->sasl_method);
    PASS_ATTR(cleanup, MAIL_ATTR_SASL_USERNAME, request->sasl_username);
    PASS_ATTR(cleanup, MAIL_ATTR_SASL_SENDER, request->sasl_sender);
    PASS_ATTR(cleanup, MAIL_ATTR_RWR_CONTEXT, request->rewrite_context);

    vstring_free(buffer);
    return (info);
}

/* forward_append - append recipient to message envelope */

int     forward_append(DELIVER_ATTR attr)
{
    FORWARD_INFO *info;
    HTABLE *table_snd;

    /*
     * Sanity checks.
     */
    if (msg_verbose)
	msg_info("forward delivered=%s sender=%s recip=%s",
		 attr.delivered, attr.sender, attr.rcpt.address);
    if (forward_dt == 0)
	msg_panic("forward_append: missing forward_init call");

    /*
     * In order to find the recipient list, first index a table by
     * delivered-to header address, then by envelope sender address.
     */
    if ((table_snd = (HTABLE *) htable_find(forward_dt, attr.delivered)) == 0) {
	table_snd = htable_create(0);
	htable_enter(forward_dt, attr.delivered, (char *) table_snd);
    }
    if ((info = (FORWARD_INFO *) htable_find(table_snd, attr.sender)) == 0) {
	if ((info = forward_open(attr.request, attr.sender)) == 0)
	    return (-1);
	htable_enter(table_snd, attr.sender, (char *) info);
    }

    /*
     * Append the recipient to the message envelope. Don't send the original
     * recipient or notification mask if it was reset due to mailing list
     * expansion.
     */
    if (*attr.rcpt.dsn_orcpt)
	rec_fprintf(info->cleanup, REC_TYPE_ATTR, "%s=%s",
		    MAIL_ATTR_DSN_ORCPT, attr.rcpt.dsn_orcpt);
    if (attr.rcpt.dsn_notify)
	rec_fprintf(info->cleanup, REC_TYPE_ATTR, "%s=%d",
		    MAIL_ATTR_DSN_NOTIFY, attr.rcpt.dsn_notify);
    if (*attr.rcpt.orig_addr)
	rec_fputs(info->cleanup, REC_TYPE_ORCP, attr.rcpt.orig_addr);
    rec_fputs(info->cleanup, REC_TYPE_RCPT, attr.rcpt.address);

    return (vstream_ferror(info->cleanup));
}

/* forward_send - send forwarded message */

static int forward_send(FORWARD_INFO *info, DELIVER_REQUEST *request,
			        DELIVER_ATTR attr, char *delivered)
{
    const char *myname = "forward_send";
    VSTRING *buffer = vstring_alloc(100);
    int     status;
    int     rec_type = 0;

    /*
     * Start the message content segment. Prepend our Delivered-To: header to
     * the message data. Stop at the first error. XXX Rely on the front-end
     * services to enforce record size limits.
     */
    rec_fputs(info->cleanup, REC_TYPE_MESG, "");
    vstring_strcpy(buffer, delivered);
    rec_fprintf(info->cleanup, REC_TYPE_NORM, "Received: by %s (%s)",
		var_myhostname, var_mail_name);
    rec_fprintf(info->cleanup, REC_TYPE_NORM, "\tid %s; %s",
		info->queue_id, mail_date(info->posting_time.tv_sec));
    if (local_deliver_hdr_mask & DELIVER_HDR_FWD)
	rec_fprintf(info->cleanup, REC_TYPE_NORM, "Delivered-To: %s",
		    lowercase(STR(buffer)));
    if ((status = vstream_ferror(info->cleanup)) == 0)
	if (vstream_fseek(attr.fp, attr.offset, SEEK_SET) < 0)
	    msg_fatal("%s: seek queue file %s: %m:",
		      myname, VSTREAM_PATH(attr.fp));
    while (status == 0 && (rec_type = rec_get(attr.fp, buffer, 0)) > 0) {
	if (rec_type != REC_TYPE_CONT && rec_type != REC_TYPE_NORM)
	    break;
	status = (REC_PUT_BUF(info->cleanup, rec_type, buffer) != rec_type);
    }
    if (status == 0 && rec_type != REC_TYPE_XTRA) {
	msg_warn("%s: bad record type: %d in message content",
		 info->queue_id, rec_type);
	status |= mark_corrupt(attr.fp);
    }

    /*
     * Send the end-of-data marker only when there were no errors.
     */
    if (status == 0) {
	rec_fputs(info->cleanup, REC_TYPE_XTRA, "");
	rec_fputs(info->cleanup, REC_TYPE_END, "");
    }

    /*
     * Retrieve the cleanup service completion status only if there are no
     * problems.
     */
    if (status == 0)
	if (vstream_fflush(info->cleanup)
	    || attr_scan(info->cleanup, ATTR_FLAG_MISSING,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_END) != 1)
	    status = 1;

    /*
     * Log successful forwarding.
     * 
     * XXX DSN alias and .forward expansion already report SUCCESS, so don't do
     * it again here.
     */
    if (status == 0) {
	attr.rcpt.dsn_notify =
	    (attr.rcpt.dsn_notify == DSN_NOTIFY_SUCCESS ?
	     DSN_NOTIFY_NEVER : attr.rcpt.dsn_notify & ~DSN_NOTIFY_SUCCESS);
	dsb_update(attr.why, "2.0.0", "relayed", DSB_SKIP_RMTA, DSB_SKIP_REPLY,
		   "forwarded as %s", info->queue_id);
	status = sent(BOUNCE_FLAGS(request), SENT_ATTR(attr));
    }

    /*
     * Cleanup.
     */
    vstring_free(buffer);
    return (status);
}

/* forward_finish - complete message forwarding requests and clean up */

int     forward_finish(DELIVER_REQUEST *request, DELIVER_ATTR attr, int cancel)
{
    HTABLE_INFO **dt_list;
    HTABLE_INFO **dt;
    HTABLE_INFO **sn_list;
    HTABLE_INFO **sn;
    HTABLE *table_snd;
    char   *delivered;
    char   *sender;
    FORWARD_INFO *info;
    int     status = cancel;

    /*
     * Sanity checks.
     */
    if (forward_dt == 0)
	msg_panic("forward_finish: missing forward_init call");

    /*
     * Walk over all delivered-to header addresses and over each envelope
     * sender address.
     */
    for (dt = dt_list = htable_list(forward_dt); *dt; dt++) {
	delivered = dt[0]->key;
	table_snd = (HTABLE *) dt[0]->value;
	for (sn = sn_list = htable_list(table_snd); *sn; sn++) {
	    sender = sn[0]->key;
	    info = (FORWARD_INFO *) sn[0]->value;
	    if (status == 0)
		status |= forward_send(info, request, attr, delivered);
	    if (msg_verbose)
		msg_info("forward_finish: delivered %s sender %s status %d",
			 delivered, sender, status);
	    (void) vstream_fclose(info->cleanup);
	    myfree(info->queue_id);
	    myfree((char *) info);
	}
	myfree((char *) sn_list);
	htable_free(table_snd, (void (*) (char *)) 0);
    }
    myfree((char *) dt_list);
    htable_free(forward_dt, (void (*) (char *)) 0);
    forward_dt = 0;
    return (status);
}
