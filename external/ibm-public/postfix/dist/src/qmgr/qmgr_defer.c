/*	$NetBSD: qmgr_defer.c,v 1.1.1.1 2009/06/23 10:08:52 tron Exp $	*/

/*++
/* NAME
/*	qmgr_defer
/* SUMMARY
/*	deal with mail that must be delivered later
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	void	qmgr_defer_recipient(message, recipient, dsn)
/*	QMGR_MESSAGE *message;
/*	RECIPIENT *recipient;
/*	DSN	*dsn;
/*
/*	void	qmgr_defer_todo(queue, dsn)
/*	QMGR_QUEUE *queue;
/*	DSN	*dsn;
/*
/*	void	qmgr_defer_transport(transport, dsn)
/*	QMGR_TRANSPORT *transport;
/*	DSN	*dsn;
/* DESCRIPTION
/*	qmgr_defer_recipient() defers delivery of the named message to
/*	the named recipient. It updates the message structure and writes
/*	a log entry.
/*
/*	qmgr_defer_todo() iterates over all "todo" deliveries queued for
/*	the named site, and calls qmgr_defer_recipient() for each recipient
/*	found.  Side effects caused by qmgr_entry_done(), qmgr_queue_done(),
/*	and by qmgr_active_done(): in-core queue entries will disappear,
/*	in-core queues may disappear, in-core and on-disk messages may
/*	disappear, bounces may be sent, new in-core queues, queue entries
/*	and recipients may appear.
/*
/*	qmgr_defer_transport() calls qmgr_defer_todo() for each queue
/*	that depends on the named transport. See there for side effects.
/*
/*	Arguments:
/* .IP recipient
/*	A recipient address; used for logging purposes, and for updating
/*	the message-specific \fIdefer\fR log.
/* .IP queue
/*	Specifies a queue with delivery requests for a specific next-hop
/*	host (or local user).
/* .IP transport
/*	Specifies a message delivery transport.
/* .IP dsn
/*	See dsn(3).
/* BUGS
/*	The side effects of calling this routine are quite dramatic.
/* DIAGNOSTICS
/*	Panic: consistency check failure. Fatal: out of memory.
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
/*	Preemptive scheduler enhancements:
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <mail_proto.h>
#include <defer.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_defer_transport - defer todo entries for named transport */

void    qmgr_defer_transport(QMGR_TRANSPORT *transport, DSN *dsn)
{
    QMGR_QUEUE *queue;
    QMGR_QUEUE *next;

    if (msg_verbose)
	msg_info("defer transport %s: %s %s",
		 transport->name, dsn->status, dsn->reason);

    /*
     * Proceed carefully. Queues may disappear as a side effect.
     */
    for (queue = transport->queue_list.next; queue; queue = next) {
	next = queue->peers.next;
	qmgr_defer_todo(queue, dsn);
    }
}

/* qmgr_defer_todo - defer all todo queue entries for specific site */

void    qmgr_defer_todo(QMGR_QUEUE *queue, DSN *dsn)
{
    QMGR_ENTRY *entry;
    QMGR_ENTRY *next;
    QMGR_MESSAGE *message;
    RECIPIENT *recipient;
    int     nrcpt;
    QMGR_QUEUE *retry_queue;

    /*
     * Sanity checks.
     */
    if (msg_verbose)
	msg_info("defer site %s: %s %s",
		 queue->name, dsn->status, dsn->reason);

    /*
     * See if we can redirect the deliveries to the retry(8) delivery agent,
     * so that they can be handled asynchronously. If the retry(8) service is
     * unavailable, use the synchronous defer(8) server. With a large todo
     * queue, this blocks the queue manager for a significant time.
     */
    retry_queue = qmgr_error_queue(MAIL_SERVICE_RETRY, dsn);

    /*
     * Proceed carefully. Queue entries may disappear as a side effect.
     */
    for (entry = queue->todo.next; entry != 0; entry = next) {
	next = entry->queue_peers.next;
	if (retry_queue != 0) {
	    qmgr_entry_move_todo(retry_queue, entry);
	    continue;
	}
	message = entry->message;
	for (nrcpt = 0; nrcpt < entry->rcpt_list.len; nrcpt++) {
	    recipient = entry->rcpt_list.info + nrcpt;
	    qmgr_defer_recipient(message, recipient, dsn);
	}
	qmgr_entry_done(entry, QMGR_QUEUE_TODO);
    }
}

/* qmgr_defer_recipient - defer delivery of specific recipient */

void    qmgr_defer_recipient(QMGR_MESSAGE *message, RECIPIENT *recipient,
			             DSN *dsn)
{
    MSG_STATS stats;

    /*
     * Update the message structure and log the message disposition.
     */
    message->flags |= defer_append(message->tflags, message->queue_id,
				 QMGR_MSG_STATS(&stats, message), recipient,
				   "none", dsn);
}
