/*++
/* NAME
/*	qmgr_queue 3
/* SUMMARY
/*	per-destination queues
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	int	qmgr_queue_count;
/*
/*	QMGR_QUEUE *qmgr_queue_create(transport, name, nexthop)
/*	QMGR_TRANSPORT *transport;
/*	const char *name;
/*	const char *nexthop;
/*
/*	void	qmgr_queue_done(queue)
/*	QMGR_QUEUE *queue;
/*
/*	QMGR_QUEUE *qmgr_queue_find(transport, name)
/*	QMGR_TRANSPORT *transport;
/*	const char *name;
/*
/*	QMGR_QUEUE *qmgr_queue_select(transport)
/*	QMGR_TRANSPORT *transport;
/*
/*	void	qmgr_queue_throttle(queue, reason)
/*	QMGR_QUEUE *queue;
/*	const char *reason;
/*
/*	void	qmgr_queue_unthrottle(queue)
/*	QMGR_QUEUE *queue;
/* DESCRIPTION
/*	These routines add/delete/manipulate per-destination queues.
/*	Each queue corresponds to a specific transport and destination.
/*	Each queue has a `todo' list of delivery requests for that
/*	destination, and a `busy' list of delivery requests in progress.
/*
/*	qmgr_queue_count is a global counter for the total number
/*	of in-core queue structures.
/*
/*	qmgr_queue_create() creates an empty named queue for the named
/*	transport and destination. The queue is given an initial
/*	concurrency limit as specified with the
/*	\fIinitial_destination_concurrency\fR configuration parameter,
/*	provided that it does not exceed the transport-specific
/*	concurrency limit.
/*
/*	qmgr_queue_done() disposes of a per-destination queue after all
/*	its entries have been taken care of. It is an error to dispose
/*	of a dead queue.
/*
/*	qmgr_queue_find() looks up the named queue for the named
/*	transport. A null result means that the queue was not found.
/*
/*	qmgr_queue_select() uses a round-robin strategy to select
/*	from the named transport one per-destination queue with a
/*	non-empty `todo' list.
/*
/*	qmgr_queue_throttle() handles a delivery error, and decrements the
/*	concurrency limit for the destination. When the concurrency limit
/*	for a destination becomes zero, qmgr_queue_throttle() starts a timer
/*	to re-enable delivery to the destination after a configurable delay.
/*
/*	qmgr_queue_unthrottle() undoes qmgr_queue_throttle()'s effects.
/*	The concurrency limit for the destination is incremented,
/*	provided that it does not exceed the destination concurrency
/*	limit specified for the transport. This routine implements
/*	"slow open" mode, and eliminates the "thundering herd" problem.
/* DIAGNOSTICS
/*	None
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
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <events.h>
#include <htable.h>

/* Global library. */

#include <mail_params.h>
#include <recipient_list.h>

/* Application-specific. */

#include "qmgr.h"

int     qmgr_queue_count;

/* qmgr_queue_unthrottle_wrapper - in case (char *) != (struct *) */

static void qmgr_queue_unthrottle_wrapper(int unused_event, char *context)
{
    QMGR_QUEUE *queue = (QMGR_QUEUE *) context;

    /*
     * This routine runs when a wakeup timer goes off; it does not run in the
     * context of some queue manipulation. Therefore, it is safe to discard
     * this in-core queue when it is empty and when this site is not dead.
     */
    qmgr_queue_unthrottle(queue);
    if (queue->window > 0 && queue->todo.next == 0 && queue->busy.next == 0)
	qmgr_queue_done(queue);
}

/* qmgr_queue_unthrottle - give this destination another chance */

void    qmgr_queue_unthrottle(QMGR_QUEUE *queue)
{
    char   *myname = "qmgr_queue_unthrottle";
    QMGR_TRANSPORT *transport = queue->transport;

    if (msg_verbose)
	msg_info("%s: queue %s", myname, queue->name);

    /*
     * Special case when this site was dead.
     */
    if (queue->window == 0) {
	event_cancel_timer(qmgr_queue_unthrottle_wrapper, (char *) queue);
	if (queue->reason == 0)
	    msg_panic("%s: queue %s: window 0 reason 0", myname, queue->name);
	myfree(queue->reason);
	queue->reason = 0;
	queue->window = transport->init_dest_concurrency;
	return;
    }

    /*
     * Increase the destination's concurrency limit until we reach the
     * transport's concurrency limit. Allow for a margin the size of the
     * initial destination concurrency, so that we're not too gentle.
     */
    if (transport->dest_concurrency_limit == 0
	|| transport->dest_concurrency_limit > queue->window)
	if (queue->window <= queue->busy_refcount + transport->init_dest_concurrency)
	    queue->window++;
}

/* qmgr_queue_throttle - handle destination delivery failure */

void    qmgr_queue_throttle(QMGR_QUEUE *queue, const char *reason)
{
    char   *myname = "qmgr_queue_throttle";

    /*
     * Sanity checks.
     */
    if (queue->reason)
	msg_panic("%s: queue %s: spurious reason %s",
		  myname, queue->name, queue->reason);
    if (msg_verbose)
	msg_info("%s: queue %s: %s", myname, queue->name, reason);

    /*
     * Decrease the destination's concurrency limit until we reach zero, at
     * which point the destination is declared dead. Decrease the concurrency
     * limit by one, instead of using actual concurrency - 1, to avoid
     * declaring a host dead after just one single delivery failure.
     */
    if (queue->window > 0)
	queue->window--;

    /*
     * Special case for a site that just was declared dead.
     */
    if (queue->window == 0) {
	queue->reason = mystrdup(reason);
	event_request_timer(qmgr_queue_unthrottle_wrapper,
			    (char *) queue, var_min_backoff_time);
    }
}

/* qmgr_queue_select - select in-core queue for delivery */

QMGR_QUEUE *qmgr_queue_select(QMGR_TRANSPORT *transport)
{
    QMGR_QUEUE *queue;

    /*
     * If we find a suitable site, rotate the list to enforce round-robin
     * selection. See similar selection code in qmgr_transport_select().
     */
    for (queue = transport->queue_list.next; queue; queue = queue->peers.next) {
	if (queue->window > queue->busy_refcount && queue->todo.next != 0) {
	    QMGR_LIST_ROTATE(transport->queue_list, queue);
	    if (msg_verbose)
		msg_info("qmgr_queue_select: %s", queue->name);
	    return (queue);
	}
    }
    return (0);
}

/* qmgr_queue_done - delete in-core queue for site */

void    qmgr_queue_done(QMGR_QUEUE *queue)
{
    char   *myname = "qmgr_queue_done";
    QMGR_TRANSPORT *transport = queue->transport;

    /*
     * Sanity checks. It is an error to delete an in-core queue with pending
     * messages or timers.
     */
    if (queue->busy_refcount != 0 || queue->todo_refcount != 0)
	msg_panic("%s: refcount: %d", myname,
		  queue->busy_refcount + queue->todo_refcount);
    if (queue->todo.next || queue->busy.next)
	msg_panic("%s: queue not empty: %s", myname, queue->name);
    if (queue->window <= 0)
	msg_panic("%s: window %d", myname, queue->window);
    if (queue->reason)
	msg_panic("%s: queue %s: spurious reason %s",
		  myname, queue->name, queue->reason);

    /*
     * Clean up this in-core queue.
     */
    QMGR_LIST_UNLINK(transport->queue_list, QMGR_QUEUE *, queue);
    htable_delete(transport->queue_byname, queue->name, (void (*) (char *)) 0);
    myfree(queue->name);
    myfree(queue->nexthop);
    qmgr_queue_count--;
    myfree((char *) queue);
}

/* qmgr_queue_create - create in-core queue for site */

QMGR_QUEUE *qmgr_queue_create(QMGR_TRANSPORT *transport, const char *name,
			              const char *nexthop)
{
    QMGR_QUEUE *queue;

    /*
     * If possible, choose an initial concurrency of > 1 so that one bad
     * message or one bad network won't slow us down unnecessarily.
     */

    queue = (QMGR_QUEUE *) mymalloc(sizeof(QMGR_QUEUE));
    qmgr_queue_count++;
    queue->name = mystrdup(name);
    queue->nexthop = mystrdup(nexthop);
    queue->todo_refcount = 0;
    queue->busy_refcount = 0;
    queue->transport = transport;
    queue->window = transport->init_dest_concurrency;
    QMGR_LIST_INIT(queue->todo);
    QMGR_LIST_INIT(queue->busy);
    queue->reason = 0;
    queue->clog_time_to_warn = 0;
    QMGR_LIST_PREPEND(transport->queue_list, queue);
    htable_enter(transport->queue_byname, name, (char *) queue);
    return (queue);
}

/* qmgr_queue_find - find in-core named queue */

QMGR_QUEUE *qmgr_queue_find(QMGR_TRANSPORT *transport, const char *name)
{
    return ((QMGR_QUEUE *) htable_find(transport->queue_byname, name));
}
