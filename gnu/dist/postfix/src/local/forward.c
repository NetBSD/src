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
/*	int	forward_finish(attr, cancel)
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
#include <unistd.h>
#include <time.h>

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

/* Application-specific. */

#include "local.h"

 /*
  * Use one cleanup service connection for each (delivered to, sender) pair.
  */
static HTABLE *forward_dt;

typedef struct FORWARD_INFO {
    VSTREAM *cleanup;			/* clean up service handle */
    char   *queue_id;			/* forwarded message queue id */
    time_t  posting_time;		/* posting time */
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

static FORWARD_INFO *forward_open(char *sender)
{
    VSTRING *buffer = vstring_alloc(100);
    FORWARD_INFO *info;
    VSTREAM *cleanup;

    /*
     * Contact the cleanup service and save the new mail queue id. Request
     * that the cleanup service bounces bad messages to the sender so that we
     * can avoid the trouble of bounce management.
     */
    cleanup = mail_connect(MAIL_CLASS_PRIVATE, MAIL_SERVICE_CLEANUP, BLOCKING);
    if (cleanup == 0)
	return (0);
    close_on_exec(vstream_fileno(cleanup), CLOSE_ON_EXEC);
    if (mail_scan(cleanup, "%s", buffer) != 1) {
	vstream_fclose(cleanup);
	return (0);
    }
    info = (FORWARD_INFO *) mymalloc(sizeof(FORWARD_INFO));
    info->cleanup = cleanup;
    info->queue_id = mystrdup(vstring_str(buffer));
    info->posting_time = time((time_t *) 0);
    mail_print(cleanup, "%d", CLEANUP_FLAG_BOUNCE);

    /*
     * Send initial message envelope information. For bounces, set the
     * designated sender: mailing list owner, posting user, whatever.
     */
    rec_fprintf(cleanup, REC_TYPE_TIME, "%ld", (long) info->posting_time);
    rec_fputs(cleanup, REC_TYPE_FROM, sender);

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
		 attr.delivered, attr.sender, attr.recipient);
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
	if ((info = forward_open(attr.sender)) == 0)
	    return (-1);
	htable_enter(table_snd, attr.sender, (char *) info);
    }

    /*
     * Append the recipient to the message envelope.
     */
    rec_fputs(info->cleanup, REC_TYPE_RCPT, attr.recipient);

    return (vstream_ferror(info->cleanup));
}

/* forward_send - send forwarded message */

static int forward_send(FORWARD_INFO *info, DELIVER_ATTR attr, char *delivered)
{
    char   *myname = "forward_send";
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
		info->queue_id, mail_date(info->posting_time));
    if (local_deliver_hdr_mask & DELIVER_HDR_FWD)
	rec_fprintf(info->cleanup, REC_TYPE_NORM, "Delivered-To: %s",
		    lowercase(vstring_str(buffer)));
    if ((status = vstream_ferror(info->cleanup)) == 0)
	if (vstream_fseek(attr.fp, attr.offset, SEEK_SET) < 0)
	    msg_fatal("%s: seek queue file %s: %m:",
		      myname, VSTREAM_PATH(attr.fp));
    while (status == 0 && (rec_type = rec_get(attr.fp, buffer, 0)) > 0) {
	if (rec_type != REC_TYPE_CONT && rec_type != REC_TYPE_NORM)
	    break;
	status = (REC_PUT_BUF(info->cleanup, rec_type, buffer) != rec_type);
    }
    if (status == 0 && rec_type != REC_TYPE_XTRA)
	status |= mark_corrupt(attr.fp);

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
	    || mail_scan(info->cleanup, "%d", &status) != 1)
	    status = 1;

    /*
     * Log successful forwarding.
     */
    if (status == 0)
	sent(SENT_ATTR(attr), "forwarded as %s", info->queue_id);

    /*
     * Cleanup.
     */
    vstring_free(buffer);
    return (status);
}

/* forward_finish - complete message forwarding requests and clean up */

int     forward_finish(DELIVER_ATTR attr, int cancel)
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
		status |= forward_send(info, attr, delivered);
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
