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
/*	void	qmgr_queue_throttle(queue, dsn)
/*	QMGR_QUEUE *queue;
/*	DSN	*dsn;
/*
/*	void	qmgr_queue_unthrottle(queue)
/*	QMGR_QUEUE *queue;
/*
/*	void	qmgr_queue_suspend(queue, delay)
/*	QMGR_QUEUE *queue;
/*	int	delay;
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
/*	concurrency limit for the destination, with a lower bound of 1.
/*	When the cohort failure bound is reached, qmgr_queue_throttle()
/*	sets the concurrency limit to zero and starts a timer
/*	to re-enable delivery to the destination after a configurable delay.
/*
/*	qmgr_queue_unthrottle() undoes qmgr_queue_throttle()'s effects.
/*	The concurrency limit for the destination is incremented,
/*	provided that it does not exceed the destination concurrency
/*	limit specified for the transport. This routine implements
/*	"slow open" mode, and eliminates the "thundering herd" problem.
/*
/*	qmgr_queue_suspend() suspends delivery for this destination
/*	briefly.
/* DIAGNOSTICS
/*	Panic: consistency check failure.
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
#include <mail_proto.h>			/* QMGR_LOG_WINDOW */

/* Application-specific. */

#include "qmgr.h"

int     qmgr_queue_count;

#define QMGR_ERROR_OR_RETRY_QUEUE(queue) \
	(strcmp(queue->transport->name, MAIL_SERVICE_RETRY) == 0 \
	    || strcmp(queue->transport->name, MAIL_SERVICE_ERROR) == 0)

#define QMGR_LOG_FEEDBACK(feedback) \
	if (var_conc_feedback_debug && !QMGR_ERROR_OR_RETRY_QUEUE(queue)) \
	    msg_info("%s: feedback %g", myname, feedback);

#define QMGR_LOG_WINDOW(queue) \
	if (var_conc_feedback_debug && !QMGR_ERROR_OR_RETRY_QUEUE(queue)) \
	    msg_info("%s: queue %s: limit %d window %d success %g failure %g fail_cohorts %g", \
		    myname, queue->name, queue->transport->dest_concurrency_limit, \
		    queue->window, queue->success, queue->failure, queue->fail_cohorts);

/* qmgr_queue_resume - resume delivery to destination */

static void qmgr_queue_resume(int event, char *context)
{
    QMGR_QUEUE *queue = (QMGR_QUEUE *) context;
    const char *myname = "qmgr_queue_resume";

    /*
     * Sanity checks.
     */
    if (!QMGR_QUEUE_SUSPENDED(queue))
	msg_panic("%s: bad queue status: %s", myname, QMGR_QUEUE_STATUS(queue));

    /*
     * We can't simply force delivery on this queue: the transport's pending
     * count may already be maxed out, and there may be other constraints
     * that definitely should be none of our business. The best we can do is
     * to play by the same rules as everyone else: let qmgr_active_drain()
     * and round-robin selection take care of message selection.
     */
    queue->window = 1;

    /*
     * Every event handler that leaves a queue in the "ready" state should
     * remove the queue when it is empty.
     */
    if (QMGR_QUEUE_READY(queue) && queue->todo.next == 0 && queue->busy.next == 0)
	qmgr_queue_done(queue);
}

/* qmgr_queue_suspend - briefly suspend a destination */

void    qmgr_queue_suspend(QMGR_QUEUE *queue, int delay)
{
    const char *myname = "qmgr_queue_suspend";

    /*
     * Sanity checks.
     */
    if (!QMGR_QUEUE_READY(queue))
	msg_panic("%s: bad queue status: %s", myname, QMGR_QUEUE_STATUS(queue));
    if (queue->busy_refcount > 0)
	msg_panic("%s: queue is busy", myname);

    /*
     * Set the queue status to "suspended". No-one is supposed to remove a
     * queue in suspended state.
     */
    queue->window = QMGR_QUEUE_STAT_SUSPENDED;
    event_request_timer(qmgr_queue_resume, (char *) queue, delay);
}

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
    if (QMGR_QUEUE_READY(queue) && queue->todo.next == 0 && queue->busy.next == 0)
	qmgr_queue_done(queue);
}

/* qmgr_queue_unthrottle - give this destination another chance */

void    qmgr_queue_unthrottle(QMGR_QUEUE *queue)
{
    const char *myname = "qmgr_queue_unthrottle";
    QMGR_TRANSPORT *transport = queue->transport;
    double  feedback;

    if (msg_verbose)
	msg_info("%s: queue %s", myname, queue->name);

    /*
     * Sanity checks.
     */
    if (!QMGR_QUEUE_THROTTLED(queue) && !QMGR_QUEUE_READY(queue))
	msg_panic("%s: bad queue status: %s", myname, QMGR_QUEUE_STATUS(queue));

    /*
     * Don't restart the negative feedback hysteresis cycle with every
     * positive feedback. Restart it only when we make a positive concurrency
     * adjustment (i.e. at the end of a positive feedback hysteresis cycle).
     * Otherwise negative feedback would be too aggressive: negative feedback
     * takes effect immediately at the start of its hysteresis cycle.
     */
    queue->fail_cohorts = 0;

    /*
     * Special case when this site was dead.
     */
    if (QMGR_QUEUE_THROTTLED(queue)) {
	event_cancel_timer(qmgr_queue_unthrottle_wrapper, (char *) queue);
	if (queue->dsn == 0)
	    msg_panic("%s: queue %s: window 0 status 0", myname, queue->name);
	dsn_free(queue->dsn);
	queue->dsn = 0;
	/* Back from the almost grave, best concurrency is anyone's guess. */
	if (queue->busy_refcount > 0)
	    queue->window = queue->busy_refcount;
	else
	    queue->window = transport->init_dest_concurrency;
	queue->success = queue->failure = 0;
	QMGR_LOG_WINDOW(queue);
	return;
    }

    /*
     * Increase the destination's concurrency limit until we reach the
     * transport's concurrency limit. Allow for a margin the size of the
     * initial destination concurrency, so that we're not too gentle.
     * 
     * Why is the concurrency increment based on preferred concurrency and not
     * on the number of outstanding delivery requests? The latter fluctuates
     * wildly when deliveries complete in bursts (artificial benchmark
     * measurements), and does not account for cached connections.
     * 
     * Keep the window within reasonable distance from actual concurrency
     * otherwise negative feedback will be ineffective. This expression
     * assumes that busy_refcount changes gradually. This is invalid when
     * deliveries complete in bursts (artificial benchmark measurements).
     */
    if (transport->dest_concurrency_limit == 0
	|| transport->dest_concurrency_limit > queue->window)
	if (queue->window < queue->busy_refcount + transport->init_dest_concurrency) {
	    feedback = QMGR_FEEDBACK_VAL(transport->pos_feedback, queue->window);
	    QMGR_LOG_FEEDBACK(feedback);
	    queue->success += feedback;
	    /* Prepare for overshoot (feedback > hysteresis, rounding error). */
	    while (queue->success + feedback / 2 >= transport->pos_feedback.hysteresis) {
		queue->window += transport->pos_feedback.hysteresis;
		queue->success -= transport->pos_feedback.hysteresis;
		queue->failure = 0;
	    }
	    /* Prepare for overshoot. */
	    if (transport->dest_concurrency_limit > 0
		&& queue->window > transport->dest_concurrency_limit)
		queue->window = transport->dest_concurrency_limit;
	}
    QMGR_LOG_WINDOW(queue);
}

/* qmgr_queue_throttle - handle destination delivery failure */

void    qmgr_queue_throttle(QMGR_QUEUE *queue, DSN *dsn)
{
    const char *myname = "qmgr_queue_throttle";
    QMGR_TRANSPORT *transport = queue->transport;
    double  feedback;

    /*
     * Sanity checks.
     */
    if (!QMGR_QUEUE_READY(queue))
	msg_panic("%s: bad queue status: %s", myname, QMGR_QUEUE_STATUS(queue));
    if (queue->dsn)
	msg_panic("%s: queue %s: spurious reason %s",
		  myname, queue->name, queue->dsn->reason);
    if (msg_verbose)
	msg_info("%s: queue %s: %s %s",
		 myname, queue->name, dsn->status, dsn->reason);

    /*
     * Don't restart the positive feedback hysteresis cycle with every
     * negative feedback. Restart it only when we make a negative concurrency
     * adjustment (i.e. at the start of a negative feedback hysteresis
     * cycle). Otherwise positive feedback would be too weak (positive
     * feedback does not take effect until the end of its hysteresis cycle).
     */

    /*
     * This queue is declared dead after a configurable number of
     * pseudo-cohort failures.
     */
    if (QMGR_QUEUE_READY(queue)) {
	queue->fail_cohorts += 1.0 / queue->window;
	if (transport->fail_cohort_limit > 0
	    && queue->fail_cohorts >= transport->fail_cohort_limit)
	    queue->window = QMGR_QUEUE_STAT_THROTTLED;
    }

    /*
     * Decrease the destination's concurrency limit until we reach 1. Base
     * adjustments on the concurrency limit itself, instead of using the
     * actual concurrency. The latter fluctuates wildly when deliveries
     * complete in bursts (artificial benchmark measurements).
     * 
     * Even after reaching 1, we maintain the negative hysteresis cycle so that
     * negative feedback can cancel out positive feedback.
     */
    if (QMGR_QUEUE_READY(queue)) {
	feedback = QMGR_FEEDBACK_VAL(transport->neg_feedback, queue->window);
	QMGR_LOG_FEEDBACK(feedback);
	queue->failure -= feedback;
	/* Prepare for overshoot (feedback > hysteresis, rounding error). */
	while (queue->failure - feedback / 2 < 0) {
	    queue->window -= transport->neg_feedback.hysteresis;
	    queue->success = 0;
	    queue->failure += transport->neg_feedback.hysteresis;
	}
	/* Prepare for overshoot. */
	if (queue->window < 1)
	    queue->window = 1;
    }

    /*
     * Special case for a site that just was declared dead.
     */
    if (QMGR_QUEUE_THROTTLED(queue)) {
	queue->dsn = DSN_COPY(dsn);
	event_request_timer(qmgr_queue_unthrottle_wrapper,
			    (char *) queue, var_min_backoff_time);
	queue->dflags = 0;
    }
    QMGR_LOG_WINDOW(queue);
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
    const char *myname = "qmgr_queue_done";
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
    if (!QMGR_QUEUE_READY(queue))
	msg_panic("%s: bad queue status: %s", myname, QMGR_QUEUE_STATUS(queue));
    if (queue->dsn)
	msg_panic("%s: queue %s: spurious reason %s",
		  myname, queue->name, queue->dsn->reason);

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
    queue->dflags = 0;
    queue->last_done = 0;
    queue->name = mystrdup(name);
    queue->nexthop = mystrdup(nexthop);
    queue->todo_refcount = 0;
    queue->busy_refcount = 0;
    queue->transport = transport;
    queue->window = transport->init_dest_concurrency;
    queue->success = queue->failure = queue->fail_cohorts = 0;
    QMGR_LIST_INIT(queue->todo);
    QMGR_LIST_INIT(queue->busy);
    queue->dsn = 0;
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
