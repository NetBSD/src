/*++
/* NAME
/*	qmgr_deliver 3
/* SUMMARY
/*	deliver one per-site queue entry to that site
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	int	qmgr_deliver_concurrency;
/*
/*	int	qmgr_deliver(transport, fp)
/*	QMGR_TRANSPORT *transport;
/*	VSTREAM	*fp;
/* DESCRIPTION
/*	This module implements the client side of the `queue manager
/*	to delivery agent' protocol. The queue manager uses
/*	asynchronous I/O so that it can drive multiple delivery
/*	agents in parallel. Depending on the outcome of a delivery
/*	attempt, the status of messages, queues and transports is
/*	updated.
/*
/*	qmgr_deliver_concurrency is a global counter that says how
/*	many delivery processes are in use. This can be used, for
/*	example, to control the size of the `active' message queue.
/*
/*	qmgr_deliver() executes when a delivery process announces its
/*	availability for the named transport. It arranges for delivery
/*	of a suitable queue entry.  The \fIfp\fR argument specifies a
/*	stream that is connected to the delivery process. Upon completion
/*	of delivery (successful or not), the stream is closed, so that the
/*	delivery process is released.
/* DIAGNOSTICS
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Scheduler enhancements:
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
/*--*/

/* System library. */

#include <sys_defs.h>
#include <time.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <events.h>
#include <iostuff.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_proto.h>
#include <recipient_list.h>
#include <mail_params.h>
#include <deliver_request.h>
#include <verp_sender.h>

/* Application-specific. */

#include "qmgr.h"

int     qmgr_deliver_concurrency;

 /*
  * Message delivery status codes.
  */
#define DELIVER_STAT_OK		0	/* all recipients delivered */
#define DELIVER_STAT_DEFER	1	/* try some recipients later */
#define DELIVER_STAT_CRASH	2	/* mailer internal problem */

/* qmgr_deliver_initial_reply - retrieve initial delivery process response */

static int qmgr_deliver_initial_reply(VSTREAM *stream)
{
    int     stat;

    if (peekfd(vstream_fileno(stream)) < 0) {
	msg_warn("%s: premature disconnect", VSTREAM_PATH(stream));
	return (DELIVER_STAT_CRASH);
    } else if (attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &stat,
			 ATTR_TYPE_END) != 1) {
	msg_warn("%s: malformed response", VSTREAM_PATH(stream));
	return (DELIVER_STAT_CRASH);
    } else {
	return (stat ? DELIVER_STAT_DEFER : 0);
    }
}

/* qmgr_deliver_final_reply - retrieve final delivery process response */

static int qmgr_deliver_final_reply(VSTREAM *stream, VSTRING *reason)
{
    int     stat;

    if (peekfd(vstream_fileno(stream)) < 0) {
	msg_warn("%s: premature disconnect", VSTREAM_PATH(stream));
	return (DELIVER_STAT_CRASH);
    } else if (attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_WHY, reason,
			 ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &stat,
			 ATTR_TYPE_END) != 2) {
	msg_warn("%s: malformed response", VSTREAM_PATH(stream));
	return (DELIVER_STAT_CRASH);
    } else {
	return (stat ? DELIVER_STAT_DEFER : 0);
    }
}

/* qmgr_deliver_send_request - send delivery request to delivery process */

static int qmgr_deliver_send_request(QMGR_ENTRY *entry, VSTREAM *stream)
{
    QMGR_RCPT_LIST list = entry->rcpt_list;
    QMGR_RCPT *recipient;
    QMGR_MESSAGE *message = entry->message;
    VSTRING *sender_buf = 0;
    char   *sender;
    int     flags;

    /*
     * If variable envelope return path is requested, change prefix+@origin
     * into prefix+user=domain@origin. Note that with VERP there is only one
     * recipient per delivery.
     */
    if (message->verp_delims == 0) {
	sender = message->sender;
    } else {
	sender_buf = vstring_alloc(100);
	verp_sender(sender_buf, message->verp_delims,
		    message->sender, list.info->address);
	sender = vstring_str(sender_buf);
    }

    flags = message->tflags
	| (message->inspect_xport ? DEL_REQ_FLAG_BOUNCE : DEL_REQ_FLAG_DEFLT);
    attr_print(stream, ATTR_FLAG_MORE,
	       ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
	       ATTR_TYPE_STR, MAIL_ATTR_QUEUE, message->queue_name,
	       ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, message->queue_id,
	       ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, message->data_offset,
	       ATTR_TYPE_LONG, MAIL_ATTR_SIZE, message->data_size,
	       ATTR_TYPE_STR, MAIL_ATTR_NEXTHOP, entry->queue->nexthop,
	       ATTR_TYPE_STR, MAIL_ATTR_ENCODING, message->encoding,
	       ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
	       ATTR_TYPE_STR, MAIL_ATTR_ERRTO, message->errors_to,
	       ATTR_TYPE_STR, MAIL_ATTR_RRCPT, message->return_receipt,
	       ATTR_TYPE_LONG, MAIL_ATTR_TIME, message->arrival_time,
	       ATTR_TYPE_STR, MAIL_ATTR_CLIENT_NAME, message->client_name,
	       ATTR_TYPE_STR, MAIL_ATTR_CLIENT_ADDR, message->client_addr,
	       ATTR_TYPE_STR, MAIL_ATTR_PROTO_NAME, message->client_proto,
	       ATTR_TYPE_STR, MAIL_ATTR_HELO_NAME, message->client_helo,
	       ATTR_TYPE_END);
    if (sender_buf != 0)
	vstring_free(sender_buf);
    for (recipient = list.info; recipient < list.info + list.len; recipient++)
	attr_print(stream, ATTR_FLAG_MORE,
		   ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, recipient->offset,
		   ATTR_TYPE_STR, MAIL_ATTR_ORCPT, recipient->orig_rcpt,
		   ATTR_TYPE_STR, MAIL_ATTR_RECIP, recipient->address,
		   ATTR_TYPE_END);
    attr_print(stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_NUM, MAIL_ATTR_OFFSET, 0,
	       ATTR_TYPE_END);
    if (vstream_fflush(stream) != 0) {
	msg_warn("write to process (%s): %m", entry->queue->transport->name);
	return (-1);
    } else {
	if (msg_verbose)
	    msg_info("qmgr_deliver: site `%s'", entry->queue->name);
	return (0);
    }
}

/* qmgr_deliver_abort - transport response watchdog */

static void qmgr_deliver_abort(int unused_event, char *context)
{
    QMGR_ENTRY *entry = (QMGR_ENTRY *) context;
    QMGR_QUEUE *queue = entry->queue;
    QMGR_TRANSPORT *transport = queue->transport;
    QMGR_MESSAGE *message = entry->message;

    msg_fatal("%s: timeout receiving delivery status from transport: %s",
	      message->queue_id, transport->name);
}

/* qmgr_deliver_update - process delivery status report */

static void qmgr_deliver_update(int unused_event, char *context)
{
    QMGR_ENTRY *entry = (QMGR_ENTRY *) context;
    QMGR_QUEUE *queue = entry->queue;
    QMGR_TRANSPORT *transport = queue->transport;
    QMGR_MESSAGE *message = entry->message;
    VSTRING *reason = vstring_alloc(1);
    int     status;

    /*
     * The message transport has responded. Stop the watchdog timer.
     */
    event_cancel_timer(qmgr_deliver_abort, context);

    /*
     * Retrieve the delivery agent status report. The numerical status code
     * indicates if delivery should be tried again. The reason text is sent
     * only when a site should be avoided for a while, so that the queue
     * manager can log why it does not even try to schedule delivery to the
     * affected recipients.
     */
    status = qmgr_deliver_final_reply(entry->stream, reason);

    /*
     * The mail delivery process failed for some reason (although delivery
     * may have been successful). Back off with this transport type for a
     * while. Dispose of queue entries for this transport that await
     * selection (the todo lists). Stay away from queue entries that have
     * been selected (the busy lists), or we would have dangling pointers.
     * The queue itself won't go away before we dispose of the current queue
     * entry.
     */
    if (status == DELIVER_STAT_CRASH) {
	message->flags |= DELIVER_STAT_DEFER;
	qmgr_transport_throttle(transport, "unknown mail transport error");
	msg_warn("transport %s failure -- see a previous warning/fatal/panic logfile record for the problem description",
		 transport->name);
	qmgr_defer_transport(transport, transport->reason);
    }

    /*
     * This message must be tried again.
     * 
     * If we have a problem talking to this site, back off with this site for a
     * while; dispose of queue entries for this site that await selection
     * (the todo list); stay away from queue entries that have been selected
     * (the busy list), or we would have dangling pointers. The queue itself
     * won't go away before we dispose of the current queue entry.
     */
    if (status == DELIVER_STAT_DEFER) {
	message->flags |= DELIVER_STAT_DEFER;
	if (VSTRING_LEN(reason)) {
	    qmgr_queue_throttle(queue, vstring_str(reason));
	    if (queue->window == 0)
		qmgr_defer_todo(queue, queue->reason);
	}
    }

    /*
     * No problems detected. Mark the transport and queue as alive. The queue
     * itself won't go away before we dispose of the current queue entry.
     */
    if (VSTRING_LEN(reason) == 0) {
	qmgr_transport_unthrottle(transport);
	qmgr_queue_unthrottle(queue);
    }

    /*
     * Release the delivery process, and give some other queue entry a chance
     * to be delivered. When all recipients for a message have been tried,
     * decide what to do next with this message: defer, bounce, delete.
     */
    event_disable_readwrite(vstream_fileno(entry->stream));
    if (vstream_fclose(entry->stream) != 0)
	msg_warn("qmgr_deliver_update: close delivery stream: %m");
    entry->stream = 0;
    qmgr_deliver_concurrency--;
    qmgr_entry_done(entry, QMGR_QUEUE_BUSY);
    vstring_free(reason);
}

/* qmgr_deliver - deliver one per-site queue entry */

void    qmgr_deliver(QMGR_TRANSPORT *transport, VSTREAM *stream)
{
    QMGR_ENTRY *entry;

    /*
     * Find out if this delivery process is really available. Once elected,
     * the delivery process is supposed to express its happiness. If there is
     * a problem, wipe the pending deliveries for this transport. This
     * routine runs in response to an external event, so it does not run
     * while some other queue manipulation is happening.
     */
    if (qmgr_deliver_initial_reply(stream) != 0) {
	qmgr_transport_throttle(transport, "mail transport unavailable");
	qmgr_defer_transport(transport, transport->reason);
	(void) vstream_fclose(stream);
	return;
    }

    /*
     * Find a suitable queue entry. Things may have changed since this
     * transport was allocated. If no suitable entry is found,
     * unceremoniously disconnect from the delivery process. The delivery
     * agent request reading routine is prepared for the queue manager to
     * change its mind for no apparent reason.
     */
    if ((entry = qmgr_job_entry_select(transport)) == 0) {
	(void) vstream_fclose(stream);
	return;
    }

    /*
     * Send the queue file info and recipient info to the delivery process.
     * If there is a problem, wipe the pending deliveries for this transport.
     * This routine runs in response to an external event, so it does not run
     * while some other queue manipulation is happening.
     */
    if (qmgr_deliver_send_request(entry, stream) < 0) {
	qmgr_entry_unselect(entry);
	qmgr_transport_throttle(transport, "mail transport unavailable");
	qmgr_defer_transport(transport, transport->reason);
	/* warning: entry may be a dangling pointer here */
	(void) vstream_fclose(stream);
	return;
    }

    /*
     * If we get this far, go wait for the delivery status report.
     */
    qmgr_deliver_concurrency++;
    entry->stream = stream;
    event_enable_read(vstream_fileno(stream),
		      qmgr_deliver_update, (char *) entry);

    /*
     * Guard against broken systems.
     */
    event_request_timer(qmgr_deliver_abort, (char *) entry, var_daemon_timeout);
}
