/*++
/* NAME
/*	qmgr_entry 3
/* SUMMARY
/*	per-site queue entries
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_ENTRY *qmgr_entry_create(queue, message)
/*	QMGR_QUEUE *queue;
/*	QMGR_MESSAGE *message;
/*
/*	void	qmgr_entry_done(entry, which)
/*	QMGR_ENTRY *entry;
/*	int	which;
/*
/*	QMGR_ENTRY *qmgr_entry_select(queue)
/*	QMGR_QUEUE *queue;
/*
/*	void	qmgr_entry_unselect(queue, entry)
/*	QMGR_QUEUE *queue;
/*	QMGR_ENTRY *entry;
/* DESCRIPTION
/*	These routines add/delete/manipulate per-site message
/*	delivery requests.
/*
/*	qmgr_entry_create() creates an entry for the named queue and
/*	message, and appends the entry to the queue's todo list.
/*	Filling in and cleaning up the recipients is the responsibility
/*	of the caller.
/*
/*	qmgr_entry_done() discards a per-site queue entry.  The
/*	\fIwhich\fR argument is either QMGR_QUEUE_BUSY for an entry
/*	of the site's `busy' list (i.e. queue entries that have been
/*	selected for actual delivery), or QMGR_QUEUE_TODO for an entry
/*	of the site's `todo' list (i.e. queue entries awaiting selection
/*	for actual delivery).
/*
/*	qmgr_entry_done() triggers cleanup of the per-site queue when
/*	the site has no pending deliveries, and the site is either
/*	alive, or the site is dead and the number of in-core queues
/*	exceeds a configurable limit (see qmgr_queue_done()).
/*
/*	qmgr_entry_done() triggers special action when the last in-core
/*	queue entry for a message is done with: either read more
/*	recipients from the queue file, delete the queue file, or move
/*	the queue file to the deferred queue; send bounce reports to the
/*	message originator (see qmgr_active_done()).
/*
/*	qmgr_entry_select() selects the next entry from the named
/*	per-site queue's `todo' list for actual delivery. The entry is
/*	moved to the queue's `busy' list: the list of messages being
/*	delivered.
/*
/*	qmgr_entry_unselect() takes the named entry off the named
/*	per-site queue's `busy' list and moves it to the queue's
/*	`todo' list.
/* DIAGNOSTICS
/*	Panic: interface violations, internal inconsistencies.
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
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <events.h>
#include <vstream.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_entry_select - select queue entry for delivery */

QMGR_ENTRY *qmgr_entry_select(QMGR_QUEUE *queue)
{
    QMGR_ENTRY *entry;

    if ((entry = queue->todo.prev) != 0) {
	QMGR_LIST_UNLINK(queue->todo, QMGR_ENTRY *, entry);
	queue->todo_refcount--;
	QMGR_LIST_APPEND(queue->busy, entry);
	queue->busy_refcount++;
    }
    return (entry);
}

/* qmgr_entry_unselect - unselect queue entry for delivery */

void    qmgr_entry_unselect(QMGR_QUEUE *queue, QMGR_ENTRY *entry)
{
    QMGR_LIST_UNLINK(queue->busy, QMGR_ENTRY *, entry);
    queue->busy_refcount--;
    QMGR_LIST_APPEND(queue->todo, entry);
    queue->todo_refcount++;
}

/* qmgr_entry_done - dispose of queue entry */

void    qmgr_entry_done(QMGR_ENTRY *entry, int which)
{
    QMGR_QUEUE *queue = entry->queue;
    QMGR_MESSAGE *message = entry->message;

    /*
     * Take this entry off the in-core queue.
     */
    if (entry->stream != 0)
	msg_panic("qmgr_entry_done: file is open");
    if (which == QMGR_QUEUE_BUSY) {
	QMGR_LIST_UNLINK(queue->busy, QMGR_ENTRY *, entry);
	queue->busy_refcount--;
    } else if (which == QMGR_QUEUE_TODO) {
	QMGR_LIST_UNLINK(queue->todo, QMGR_ENTRY *, entry);
	queue->todo_refcount--;
    } else {
	msg_panic("qmgr_entry_done: bad queue spec: %d", which);
    }

    /*
     * Free the recipient list and decrease the in-core recipient count
     * accordingly.
     */
    qmgr_recipient_count -= entry->rcpt_list.len;
    qmgr_rcpt_list_free(&entry->rcpt_list);

    myfree((char *) entry);

    /*
     * When the in-core queue for this site is empty and when this site is
     * not dead, discard the in-core queue. When this site is dead, but the
     * number of in-core queues exceeds some threshold, get rid of this
     * in-core queue anyway, in order to avoid running out of memory.
     */
    if (queue->todo.next == 0 && queue->busy.next == 0) {
	if (queue->window == 0 && qmgr_queue_count > 2 * var_qmgr_rcpt_limit)
	    qmgr_queue_unthrottle(queue);
	if (queue->window > 0)
	    qmgr_queue_done(queue);
    }

    /*
     * Update the in-core message reference count. When the in-core message
     * structure has no more references, dispose of the message.
     * 
     * When the in-core recipient count falls below a threshold, and this
     * message has more recipients, read more recipients now. If we read more
     * recipients as soon as the recipient count falls below the in-core
     * recipient limit, we do not give other messages a chance until this
     * message is delivered. That's good for mailing list deliveries, bad for
     * one-to-one mail. If we wait until the in-core recipient count drops
     * well below the in-core recipient limit, we give other mail a chance,
     * but we also allow list deliveries to become interleaved. In the worst
     * case, people near the start of a mailing list get a burst of postings
     * today, while people near the end of the list get that same burst of
     * postings a whole day later.
     */
#define FUDGE(x)	((x) * (var_qmgr_fudge / 100.0))
    message->refcount--;
    if (message->rcpt_offset > 0
	&& qmgr_recipient_count < FUDGE(var_qmgr_rcpt_limit))
	qmgr_message_realloc(message);
    if (message->refcount == 0)
	qmgr_active_done(message);
}

/* qmgr_entry_create - create queue todo entry */

QMGR_ENTRY *qmgr_entry_create(QMGR_QUEUE *queue, QMGR_MESSAGE *message)
{
    QMGR_ENTRY *entry;

    /*
     * Sanity check.
     */
    if (queue->window == 0)
	msg_panic("qmgr_entry_create: dead queue: %s", queue->name);

    entry = (QMGR_ENTRY *) mymalloc(sizeof(QMGR_ENTRY));
    entry->stream = 0;
    entry->message = message;
    qmgr_rcpt_list_init(&entry->rcpt_list);
    message->refcount++;
    entry->queue = queue;
    QMGR_LIST_APPEND(queue->todo, entry);
    queue->todo_refcount++;
    return (entry);
}
