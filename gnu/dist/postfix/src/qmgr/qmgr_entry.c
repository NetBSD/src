/*++
/* NAME
/*	qmgr_entry 3
/* SUMMARY
/*	per-site queue entries
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_ENTRY *qmgr_entry_create(peer, message)
/*      QMGR_PEER *peer;
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
/*
/*	void	qmgr_entry_move_todo(dst, entry)
/*	QMGR_QUEUE *dst;
/*	QMGR_ENTRY *entry;
/* DESCRIPTION
/*	These routines add/delete/manipulate per-site message
/*	delivery requests.
/*
/*	qmgr_entry_create() creates an entry for the named peer and message,
/*      and appends the entry to the peer's list and its queue's todo list.
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
/*	qmgr_entry_done() discards its peer structure when the peer
/*      is not referenced anymore.
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
/*	qmgr_entry_select() selects first entry from the named
/*	per-site queue's `todo' list for actual delivery. The entry is
/*	moved to the queue's `busy' list: the list of messages being
/*	delivered. The entry is also removed from its peer list.
/*
/*	qmgr_entry_unselect() takes the named entry off the named
/*	per-site queue's `busy' list and moves it to the queue's
/*	`todo' list. The entry is also prepended to its peer list again.
/*
/*	qmgr_entry_move_todo() moves the specified "todo" queue entry
/*	to the specified "todo" queue.
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
/*
/*	Preemptive scheduler enhancements:
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
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
#include <deliver_request.h>		/* opportunistic session caching */

/* Application-specific. */

#include "qmgr.h"

/* qmgr_entry_select - select queue entry for delivery */

QMGR_ENTRY *qmgr_entry_select(QMGR_PEER *peer)
{
    const char *myname = "qmgr_entry_select";
    QMGR_ENTRY *entry;
    QMGR_QUEUE *queue;

    if ((entry = peer->entry_list.next) != 0) {
	queue = entry->queue;
	QMGR_LIST_UNLINK(queue->todo, QMGR_ENTRY *, entry, queue_peers);
	queue->todo_refcount--;
	QMGR_LIST_APPEND(queue->busy, entry, queue_peers);
	queue->busy_refcount++;
	QMGR_LIST_UNLINK(peer->entry_list, QMGR_ENTRY *, entry, peer_peers);
	peer->job->selected_entries++;

	/*
	 * With opportunistic session caching, the delivery agent must not
	 * only 1) save a session upon completion, but also 2) reuse a cached
	 * session upon the next delivery request. In order to not miss out
	 * on 2), we have to make caching sticky or else we get silly
	 * behavior when the in-memory queue drains. Specifically, new
	 * connections must not be made as long as cached connections exist.
	 * 
	 * Safety: don't enable opportunistic session caching unless the queue
	 * manager is able to schedule concurrent or back-to-back deliveries
	 * (we need to recognize back-to-back deliveries for transports with
	 * concurrency 1).
	 * 
	 * If caching has previously been enabled, but is not now, fetch any
	 * existing entries from the cache, but don't add new ones.
	 */
#define CONCURRENT_OR_BACK_TO_BACK_DELIVERY() \
	    (queue->busy_refcount > 1 || BACK_TO_BACK_DELIVERY())

#define BACK_TO_BACK_DELIVERY() \
		(queue->last_done + 1 >= event_time())

	/*
	 * Turn on session caching after we get up to speed. Don't enable
	 * session caching just because we have concurrent deliveries. This
	 * prevents unnecessary session caching when we have a burst of mail
	 * <= the initial concurrency limit.
	 */
	if ((queue->dflags & DEL_REQ_FLAG_CONN_STORE) == 0) {
	    if (BACK_TO_BACK_DELIVERY()) {
		if (msg_verbose)
		    msg_info("%s: allowing on-demand session caching for %s",
			     myname, queue->name);
		queue->dflags |= DEL_REQ_FLAG_CONN_MASK;
	    }
	}

	/*
	 * Turn off session caching when concurrency drops and we're running
	 * out of steam. This is what prevents from turning off session
	 * caching too early, and from making new connections while old ones
	 * are still cached.
	 */
	else {
	    if (!CONCURRENT_OR_BACK_TO_BACK_DELIVERY()) {
		if (msg_verbose)
		    msg_info("%s: disallowing on-demand session caching for %s",
			     myname, queue->name);
		queue->dflags &= ~DEL_REQ_FLAG_CONN_STORE;
	    }
	}
    }
    return (entry);
}

/* qmgr_entry_unselect - unselect queue entry for delivery */

void    qmgr_entry_unselect(QMGR_ENTRY *entry)
{
    QMGR_PEER *peer = entry->peer;
    QMGR_QUEUE *queue = entry->queue;

    /*
     * Move the entry back to the todo lists. In case of the peer list, put
     * it back to the beginning, so the select()/unselect() does not reorder
     * entries. We use this in qmgr_message_assign() to put recipients into
     * existing entries when possible.
     */
    QMGR_LIST_UNLINK(queue->busy, QMGR_ENTRY *, entry, queue_peers);
    queue->busy_refcount--;
    QMGR_LIST_APPEND(queue->todo, entry, queue_peers);
    queue->todo_refcount++;
    QMGR_LIST_PREPEND(peer->entry_list, entry, peer_peers);
    peer->job->selected_entries--;
}

/* qmgr_entry_move_todo - move entry between todo queues */

void    qmgr_entry_move_todo(QMGR_QUEUE *dst_queue, QMGR_ENTRY *entry)
{
    const char *myname = "qmgr_entry_move_todo";
    QMGR_TRANSPORT *dst_transport = dst_queue->transport;
    QMGR_MESSAGE *message = entry->message;
    QMGR_QUEUE *src_queue = entry->queue;
    QMGR_PEER *dst_peer, *src_peer = entry->peer;
    QMGR_JOB *dst_job, *src_job = src_peer->job;
    QMGR_ENTRY *new_entry;
    int     rcpt_count = entry->rcpt_list.len;

    if (entry->stream != 0)
	msg_panic("%s: queue %s entry is busy", myname, src_queue->name);
    if (QMGR_QUEUE_THROTTLED(dst_queue))
	msg_panic("%s: destination queue %s is throttled", myname, dst_queue->name);
    if (QMGR_TRANSPORT_THROTTLED(dst_transport))
	msg_panic("%s: destination transport %s is throttled",
		  myname, dst_transport->name);

    /*
     * Create new entry, swap the recipients between the two entries,
     * adjusting the job counters accordingly, then dispose of the old entry.
     * 
     * Note that qmgr_entry_done() will also take care of adjusting the
     * recipient limits of all the message jobs, so we do not have to do that
     * explicitly for the new job here.
     * 
     * XXX This does not enforce the per-entry recipient limit, but that is not
     * a problem as long as qmgr_entry_move_todo() is called only to bounce
     * or defer mail.
     */
    dst_job = qmgr_job_obtain(message, dst_transport);
    dst_peer = qmgr_peer_obtain(dst_job, dst_queue);

    new_entry = qmgr_entry_create(dst_peer, message);

    recipient_list_swap(&entry->rcpt_list, &new_entry->rcpt_list);

    src_job->rcpt_count -= rcpt_count;
    dst_job->rcpt_count += rcpt_count;

    qmgr_entry_done(entry, QMGR_QUEUE_TODO);
}

/* qmgr_entry_done - dispose of queue entry */

void    qmgr_entry_done(QMGR_ENTRY *entry, int which)
{
    const char *myname = "qmgr_entry_done";
    QMGR_QUEUE *queue = entry->queue;
    QMGR_MESSAGE *message = entry->message;
    QMGR_PEER *peer = entry->peer;
    QMGR_JOB *sponsor, *job = peer->job;
    QMGR_TRANSPORT *transport = job->transport;

    /*
     * Take this entry off the in-core queue.
     */
    if (entry->stream != 0)
	msg_panic("%s: file is open", myname);
    if (which == QMGR_QUEUE_BUSY) {
	QMGR_LIST_UNLINK(queue->busy, QMGR_ENTRY *, entry, queue_peers);
	queue->busy_refcount--;
    } else if (which == QMGR_QUEUE_TODO) {
	QMGR_LIST_UNLINK(peer->entry_list, QMGR_ENTRY *, entry, peer_peers);
	job->selected_entries++;
	QMGR_LIST_UNLINK(queue->todo, QMGR_ENTRY *, entry, queue_peers);
	queue->todo_refcount--;
    } else {
	msg_panic("%s: bad queue spec: %d", myname, which);
    }

    /*
     * Decrease the in-core recipient counts and free the recipient list and
     * the structure itself.
     */
    job->rcpt_count -= entry->rcpt_list.len;
    message->rcpt_count -= entry->rcpt_list.len;
    qmgr_recipient_count -= entry->rcpt_list.len;
    recipient_list_free(&entry->rcpt_list);
    myfree((char *) entry);

    /*
     * Make sure that the transport of any retired or finishing job that
     * donated recipient slots to this message gets them back first. Then, if
     * possible, pass the remaining unused recipient slots to the next job on
     * the job list.
     */
    for (sponsor = message->job_list.next; sponsor; sponsor = sponsor->message_peers.next) {
	if (sponsor->rcpt_count >= sponsor->rcpt_limit || sponsor == job)
	    continue;
	if (sponsor->stack_level < 0 || message->rcpt_offset == 0)
	    qmgr_job_move_limits(sponsor);
    }
    if (message->rcpt_offset == 0) {
	qmgr_job_move_limits(job);
    }

    /*
     * If the queue was blocking some of the jobs on the job list, check if
     * the concurrency limit has lifted. If there are still some pending
     * deliveries, give it a try and unmark all transport blockers at once.
     * The qmgr_job_entry_select() will do the rest. In either case make sure
     * the queue is not marked as a blocker anymore, with extra handling of
     * queues which were declared dead.
     * 
     * Note that changing the blocker status also affects the candidate cache.
     * Most of the cases would be automatically recognized by the current job
     * change, but we play safe and reset the cache explicitly below.
     * 
     * Keeping the transport blocker tag odd is an easy way to make sure the tag
     * never matches jobs that are not explicitly marked as blockers.
     */
    if (queue->blocker_tag == transport->blocker_tag) {
	if (queue->window > queue->busy_refcount && queue->todo.next != 0) {
	    transport->blocker_tag += 2;
	    transport->job_current = transport->job_list.next;
	    transport->candidate_cache_current = 0;
	}
	if (queue->window > queue->busy_refcount || QMGR_QUEUE_THROTTLED(queue))
	    queue->blocker_tag = 0;
    }

    /*
     * When there are no more entries for this peer, discard the peer
     * structure.
     */
    peer->refcount--;
    if (peer->refcount == 0)
	qmgr_peer_free(peer);

    /*
     * Maintain back-to-back delivery status.
     */
    if (which == QMGR_QUEUE_BUSY)
	queue->last_done = event_time();

    /*
     * Suspend a rate-limited queue, so that mail trickles out.
     */
    if (which == QMGR_QUEUE_BUSY && transport->rate_delay > 0) {
	if (queue->window > 1)
	    msg_panic("%s: queue %s/%s: window %d > 1 on rate-limited service",
		      myname, transport->name, queue->name, queue->window);
	if (QMGR_QUEUE_THROTTLED(queue))	/* XXX */
	    qmgr_queue_unthrottle(queue);
	if (QMGR_QUEUE_READY(queue))
	    qmgr_queue_suspend(queue, transport->rate_delay);
    }

    /*
     * When the in-core queue for this site is empty and when this site is
     * not dead or suspended, discard the in-core queue. When this site is
     * dead, but the number of in-core queues exceeds some threshold, get rid
     * of this in-core queue anyway, in order to avoid running out of memory.
     */
    if (queue->todo.next == 0 && queue->busy.next == 0) {
	if (QMGR_QUEUE_THROTTLED(queue) && qmgr_queue_count > 2 * var_qmgr_rcpt_limit)
	    qmgr_queue_unthrottle(queue);
	if (QMGR_QUEUE_READY(queue))
	    qmgr_queue_done(queue);
    }

    /*
     * Update the in-core message reference count. When the in-core message
     * structure has no more references, dispose of the message.
     */
    message->refcount--;
    if (message->refcount == 0)
	qmgr_active_done(message);
}

/* qmgr_entry_create - create queue todo entry */

QMGR_ENTRY *qmgr_entry_create(QMGR_PEER *peer, QMGR_MESSAGE *message)
{
    QMGR_ENTRY *entry;
    QMGR_QUEUE *queue = peer->queue;

    /*
     * Sanity check.
     */
    if (QMGR_QUEUE_THROTTLED(queue))
	msg_panic("qmgr_entry_create: dead queue: %s", queue->name);

    /*
     * Create the delivery request.
     */
    entry = (QMGR_ENTRY *) mymalloc(sizeof(QMGR_ENTRY));
    entry->stream = 0;
    entry->message = message;
    recipient_list_init(&entry->rcpt_list, RCPT_LIST_INIT_QUEUE);
    message->refcount++;
    entry->peer = peer;
    QMGR_LIST_APPEND(peer->entry_list, entry, peer_peers);
    peer->refcount++;
    entry->queue = queue;
    QMGR_LIST_APPEND(queue->todo, entry, queue_peers);
    queue->todo_refcount++;
    peer->job->read_entries++;

    /*
     * Warn if a destination is falling behind while the active queue
     * contains a non-trivial amount of single-recipient email. When a
     * destination takes up more and more space in the active queue, then
     * other mail will not get through and delivery performance will suffer.
     * 
     * XXX At this point in the code, the busy reference count is still less
     * than the concurrency limit (otherwise this code would not be invoked
     * in the first place) so we have to make make some awkward adjustments
     * below.
     * 
     * XXX The queue length test below looks at the active queue share of an
     * individual destination. This catches the case where mail for one
     * destination is falling behind because it has to round-robin compete
     * with many other destinations. However, Postfix will also perform
     * poorly when most of the active queue is tied up by a small number of
     * concurrency limited destinations. The queue length test below detects
     * such conditions only indirectly.
     * 
     * XXX This code does not detect the case that the active queue is being
     * starved because incoming mail is pounding the disk.
     */
    if (var_helpful_warnings && var_qmgr_clog_warn_time > 0) {
	int     queue_length = queue->todo_refcount + queue->busy_refcount;
	time_t  now;
	QMGR_TRANSPORT *transport;
	double  active_share;

	if (queue_length > var_qmgr_active_limit / 5
	    && (now = event_time()) >= queue->clog_time_to_warn) {
	    active_share = queue_length / (double) qmgr_message_count;
	    msg_warn("mail for %s is using up %d of %d active queue entries",
		     queue->nexthop, queue_length, qmgr_message_count);
	    if (active_share < 0.9)
		msg_warn("this may slow down other mail deliveries");
	    transport = queue->transport;
	    if (transport->dest_concurrency_limit > 0
	    && transport->dest_concurrency_limit <= queue->busy_refcount + 1)
		msg_warn("you may need to increase the main.cf %s%s from %d",
			 transport->name, _DEST_CON_LIMIT,
			 transport->dest_concurrency_limit);
	    else if (queue->window > var_qmgr_active_limit * active_share)
		msg_warn("you may need to increase the main.cf %s from %d",
			 VAR_QMGR_ACT_LIMIT, var_qmgr_active_limit);
	    else if (queue->peers.next != queue->peers.prev)
		msg_warn("you may need a separate master.cf transport for %s",
			 queue->nexthop);
	    else {
		msg_warn("you may need to reduce %s connect and helo timeouts",
			 transport->name);
		msg_warn("so that Postfix quickly skips unavailable hosts");
		msg_warn("you may need to increase the main.cf %s and %s",
			 VAR_MIN_BACKOFF_TIME, VAR_MAX_BACKOFF_TIME);
		msg_warn("so that Postfix wastes less time on undeliverable mail");
		msg_warn("you may need to increase the master.cf %s process limit",
			 transport->name);
	    }
	    msg_warn("please avoid flushing the whole queue when you have");
	    msg_warn("lots of deferred mail, that is bad for performance");
	    msg_warn("to turn off these warnings specify: %s = 0",
		     VAR_QMGR_CLOG_WARN_TIME);
	    queue->clog_time_to_warn = now + var_qmgr_clog_warn_time;
	}
    }
    return (entry);
}
