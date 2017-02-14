/*	$NetBSD: qmgr_transport.c,v 1.2 2017/02/14 01:16:47 christos Exp $	*/

/*++
/* NAME
/*	qmgr_transport 3
/* SUMMARY
/*	per-transport data structures
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_TRANSPORT *qmgr_transport_create(name)
/*	const char *name;
/*
/*	QMGR_TRANSPORT *qmgr_transport_find(name)
/*	const char *name;
/*
/*	QMGR_TRANSPORT *qmgr_transport_select()
/*
/*	void	qmgr_transport_alloc(transport, notify)
/*	QMGR_TRANSPORT *transport;
/*	void	(*notify)(QMGR_TRANSPORT *transport, VSTREAM *fp);
/*
/*	void	qmgr_transport_throttle(transport, dsn)
/*	QMGR_TRANSPORT *transport;
/*	DSN	*dsn;
/*
/*	void	qmgr_transport_unthrottle(transport)
/*	QMGR_TRANSPORT *transport;
/* DESCRIPTION
/*	This module organizes the world by message transport type.
/*	Each transport can have zero or more destination queues
/*	associated with it.
/*
/*	qmgr_transport_create() instantiates a data structure for the
/*	named transport type.
/*
/*	qmgr_transport_find() looks up an existing message transport
/*	data structure.
/*
/*	qmgr_transport_select() attempts to find a transport that
/*	has messages pending delivery.  This routine implements
/*	round-robin search among transports.
/*
/*	qmgr_transport_alloc() allocates a delivery process for the
/*	specified transport type. Allocation is performed asynchronously.
/*	When a process becomes available, the application callback routine
/*	is invoked with as arguments the transport and a stream that
/*	is connected to a delivery process. It is an error to call
/*	qmgr_transport_alloc() while delivery process allocation for
/*	the same transport is in progress.
/*
/*	qmgr_transport_throttle blocks further allocation of delivery
/*	processes for the named transport. Attempts to throttle a
/*	throttled transport are ignored.
/*
/*	qmgr_transport_unthrottle() undoes qmgr_transport_throttle().
/*	Attempts to unthrottle a non-throttled transport are ignored.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>

#include <sys/time.h>			/* FD_SETSIZE */
#include <sys/types.h>			/* FD_SETSIZE */
#include <unistd.h>			/* FD_SETSIZE */

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>			/* FD_SETSIZE */
#endif

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <events.h>
#include <mymalloc.h>
#include <vstream.h>
#include <iostuff.h>

/* Global library. */

#include <mail_proto.h>
#include <recipient_list.h>
#include <mail_conf.h>
#include <mail_params.h>

/* Application-specific. */

#include "qmgr.h"

HTABLE *qmgr_transport_byname;		/* transport by name */
QMGR_TRANSPORT_LIST qmgr_transport_list;/* transports, round robin */

 /*
  * A local structure to remember a delivery process allocation request.
  */
typedef struct QMGR_TRANSPORT_ALLOC QMGR_TRANSPORT_ALLOC;

struct QMGR_TRANSPORT_ALLOC {
    QMGR_TRANSPORT *transport;		/* transport context */
    VSTREAM *stream;			/* delivery service stream */
    QMGR_TRANSPORT_ALLOC_NOTIFY notify;	/* application call-back routine */
};

 /*
  * Connections to delivery agents are managed asynchronously. Each delivery
  * agent connection goes through multiple wait states:
  * 
  * - With Linux/Solaris and old queue manager implementations only, wait for
  * the server to invoke accept().
  * 
  * - Wait for the delivery agent's announcement that it is ready to receive a
  * delivery request.
  * 
  * - Wait for the delivery request completion status.
  * 
  * Older queue manager implementations had only one pending delivery agent
  * connection per transport. With low-latency destinations, the output rates
  * were reduced on Linux/Solaris systems that had the extra wait state.
  * 
  * To maximize delivery agent output rates with low-latency destinations, the
  * following changes were made to the queue manager by the end of the 2.4
  * development cycle:
  * 
  * - The Linux/Solaris accept() wait state was eliminated.
  * 
  * - A pipeline was implemented for pending delivery agent connections. The
  * number of pending delivery agent connections was increased from one to
  * two: the number of before-delivery wait states, plus one extra pipeline
  * slot to prevent the pipeline from stalling easily. Increasing the
  * pipeline much further actually hurt performance.
  * 
  * - To reduce queue manager disk competition with delivery agents, the queue
  * scanning algorithm was modified to import only one message per interrupt.
  * The incoming and deferred queue scans now happen on alternate interrupts.
  * 
  * Simplistically reasoned, a non-zero (incoming + active) queue length is
  * equivalent to a time shift for mail deliveries; this is undesirable when
  * delivery agents are not fully utilized.
  * 
  * On the other hand a non-empty active queue is what allows us to do clever
  * things such as queue file prefetch, concurrency windows, and connection
  * caching; the idea is that such "thinking time" is affordable only after
  * the output channels are maxed out.
  */
#ifndef QMGR_TRANSPORT_MAX_PEND
#define QMGR_TRANSPORT_MAX_PEND	2
#endif

 /*
  * Important note on the _transport_rate_delay implementation: after
  * qmgr_transport_alloc() sets the QMGR_TRANSPORT_STAT_RATE_LOCK flag, all
  * code paths must directly or indirectly invoke qmgr_transport_unthrottle()
  * or qmgr_transport_throttle(). Otherwise, transports with non-zero
  * _transport_rate_delay will become stuck.
  */

/* qmgr_transport_unthrottle_wrapper - in case (char *) != (struct *) */

static void qmgr_transport_unthrottle_wrapper(int unused_event, void *context)
{
    qmgr_transport_unthrottle((QMGR_TRANSPORT *) context);
}

/* qmgr_transport_unthrottle - open the throttle */

void    qmgr_transport_unthrottle(QMGR_TRANSPORT *transport)
{
    const char *myname = "qmgr_transport_unthrottle";

    /*
     * This routine runs after expiration of the timer set by
     * qmgr_transport_throttle(), or whenever a delivery transport has been
     * used without malfunction. In either case, we enable delivery again if
     * the transport was throttled. We always reset the transport rate lock.
     */
    if ((transport->flags & QMGR_TRANSPORT_STAT_DEAD) != 0) {
	if (msg_verbose)
	    msg_info("%s: transport %s", myname, transport->name);
	transport->flags &= ~QMGR_TRANSPORT_STAT_DEAD;
	if (transport->dsn == 0)
	    msg_panic("%s: transport %s: null reason",
		      myname, transport->name);
	dsn_free(transport->dsn);
	transport->dsn = 0;
	event_cancel_timer(qmgr_transport_unthrottle_wrapper,
			   (void *) transport);
    }
    if (transport->flags & QMGR_TRANSPORT_STAT_RATE_LOCK)
	transport->flags &= ~QMGR_TRANSPORT_STAT_RATE_LOCK;
}

/* qmgr_transport_throttle - disable delivery process allocation */

void    qmgr_transport_throttle(QMGR_TRANSPORT *transport, DSN *dsn)
{
    const char *myname = "qmgr_transport_throttle";

    /*
     * We are unable to connect to a deliver process for this type of message
     * transport. Instead of hosing the system by retrying in a tight loop,
     * back off and disable this transport type for a while.
     */
    if ((transport->flags & QMGR_TRANSPORT_STAT_DEAD) == 0) {
	if (msg_verbose)
	    msg_info("%s: transport %s: status: %s reason: %s",
		     myname, transport->name, dsn->status, dsn->reason);
	transport->flags |= QMGR_TRANSPORT_STAT_DEAD;
	if (transport->dsn)
	    msg_panic("%s: transport %s: spurious reason: %s",
		      myname, transport->name, transport->dsn->reason);
	transport->dsn = DSN_COPY(dsn);
	event_request_timer(qmgr_transport_unthrottle_wrapper,
			    (void *) transport, var_transport_retry_time);
    }
}

/* qmgr_transport_abort - transport connect watchdog */

static void qmgr_transport_abort(int unused_event, void *context)
{
    QMGR_TRANSPORT_ALLOC *alloc = (QMGR_TRANSPORT_ALLOC *) context;

    msg_fatal("timeout connecting to transport: %s", alloc->transport->name);
}

/* qmgr_transport_rate_event - delivery process availability notice */

static void qmgr_transport_rate_event(int unused_event, void *context)
{
    QMGR_TRANSPORT_ALLOC *alloc = (QMGR_TRANSPORT_ALLOC *) context;

    alloc->notify(alloc->transport, alloc->stream);
    myfree((void *) alloc);
}

/* qmgr_transport_event - delivery process availability notice */

static void qmgr_transport_event(int unused_event, void *context)
{
    QMGR_TRANSPORT_ALLOC *alloc = (QMGR_TRANSPORT_ALLOC *) context;

    /*
     * This routine notifies the application when the request given to
     * qmgr_transport_alloc() completes.
     */
    if (msg_verbose)
	msg_info("transport_event: %s", alloc->transport->name);

    /*
     * Connection request completed. Stop the watchdog timer.
     */
    event_cancel_timer(qmgr_transport_abort, context);

    /*
     * Disable further read events that end up calling this function, and
     * free up this pending connection pipeline slot.
     */
    if (alloc->stream) {
	event_disable_readwrite(vstream_fileno(alloc->stream));
	non_blocking(vstream_fileno(alloc->stream), BLOCKING);
    }
    alloc->transport->pending -= 1;

    /*
     * Notify the requestor.
     */
    if (alloc->transport->xport_rate_delay > 0) {
	if ((alloc->transport->flags & QMGR_TRANSPORT_STAT_RATE_LOCK) == 0)
	    msg_panic("transport_event: missing rate lock for transport %s",
		      alloc->transport->name);
	event_request_timer(qmgr_transport_rate_event, (void *) alloc,
			    alloc->transport->xport_rate_delay);
    } else {
	alloc->notify(alloc->transport, alloc->stream);
	myfree((void *) alloc);
    }
}

/* qmgr_transport_select - select transport for allocation */

QMGR_TRANSPORT *qmgr_transport_select(void)
{
    QMGR_TRANSPORT *xport;
    QMGR_QUEUE *queue;
    int     need;

    /*
     * If we find a suitable transport, rotate the list of transports to
     * effectuate round-robin selection. See similar selection code in
     * qmgr_peer_select().
     * 
     * This function is called repeatedly until all transports have maxed out
     * the number of pending delivery agent connections, until all delivery
     * agent concurrency windows are maxed out, or until we run out of "todo"
     * queue entries.
     */
#define MIN5af51743e4eef(x, y) ((x) < (y) ? (x) : (y))

    for (xport = qmgr_transport_list.next; xport; xport = xport->peers.next) {
	if ((xport->flags & QMGR_TRANSPORT_STAT_DEAD) != 0
	    || (xport->flags & QMGR_TRANSPORT_STAT_RATE_LOCK) != 0
	    || xport->pending >= QMGR_TRANSPORT_MAX_PEND)
	    continue;
	need = xport->pending + 1;
	for (queue = xport->queue_list.next; queue; queue = queue->peers.next) {
	    if (QMGR_QUEUE_READY(queue) == 0)
		continue;
	    if ((need -= MIN5af51743e4eef(queue->window - queue->busy_refcount,
					  queue->todo_refcount)) <= 0) {
		QMGR_LIST_ROTATE(qmgr_transport_list, xport, peers);
		if (msg_verbose)
		    msg_info("qmgr_transport_select: %s", xport->name);
		return (xport);
	    }
	}
    }
    return (0);
}

/* qmgr_transport_alloc - allocate delivery process */

void    qmgr_transport_alloc(QMGR_TRANSPORT *transport, QMGR_TRANSPORT_ALLOC_NOTIFY notify)
{
    QMGR_TRANSPORT_ALLOC *alloc;

    /*
     * Sanity checks.
     */
    if (transport->flags & QMGR_TRANSPORT_STAT_DEAD)
	msg_panic("qmgr_transport: dead transport: %s", transport->name);
    if (transport->flags & QMGR_TRANSPORT_STAT_RATE_LOCK)
	msg_panic("qmgr_transport: rate-locked transport: %s", transport->name);
    if (transport->pending >= QMGR_TRANSPORT_MAX_PEND)
	msg_panic("qmgr_transport: excess allocation: %s", transport->name);

    /*
     * When this message delivery transport is rate-limited, do not select it
     * again before the end of a message delivery transaction.
     */
    if (transport->xport_rate_delay > 0)
	transport->flags |= QMGR_TRANSPORT_STAT_RATE_LOCK;

    /*
     * Connect to the well-known port for this delivery service, and wake up
     * when a process announces its availability. Allow only a limited number
     * of delivery process allocation attempts for this transport. In case of
     * problems, back off. Do not hose the system when it is in trouble
     * already.
     * 
     * Use non-blocking connect(), so that Linux won't block the queue manager
     * until the delivery agent calls accept().
     * 
     * When the connection to delivery agent cannot be completed, notify the
     * event handler so that it can throttle the transport and defer the todo
     * queues, just like it does when communication fails *after* connection
     * completion.
     * 
     * Before Postfix 2.4, the event handler was not invoked after connect()
     * error, and mail was not deferred. Because of this, mail would be stuck
     * in the active queue after triggering a "connection refused" condition.
     */
    alloc = (QMGR_TRANSPORT_ALLOC *) mymalloc(sizeof(*alloc));
    alloc->transport = transport;
    alloc->notify = notify;
    transport->pending += 1;
    if ((alloc->stream = mail_connect(MAIL_CLASS_PRIVATE, transport->name,
				      NON_BLOCKING)) == 0) {
	msg_warn("connect to transport %s/%s: %m",
		 MAIL_CLASS_PRIVATE, transport->name);
	event_request_timer(qmgr_transport_event, (void *) alloc, 0);
	return;
    }
#if (EVENTS_STYLE != EVENTS_STYLE_SELECT) && defined(CA_VSTREAM_CTL_DUPFD)
#ifndef THRESHOLD_FD_WORKAROUND
#define THRESHOLD_FD_WORKAROUND 128
#endif
    vstream_control(alloc->stream,
		    CA_VSTREAM_CTL_DUPFD(THRESHOLD_FD_WORKAROUND),
		    CA_VSTREAM_CTL_END);
#endif
    event_enable_read(vstream_fileno(alloc->stream), qmgr_transport_event,
		      (void *) alloc);

    /*
     * Guard against broken systems.
     */
    event_request_timer(qmgr_transport_abort, (void *) alloc,
			var_daemon_timeout);
}

/* qmgr_transport_create - create transport instance */

QMGR_TRANSPORT *qmgr_transport_create(const char *name)
{
    QMGR_TRANSPORT *transport;

    if (htable_find(qmgr_transport_byname, name) != 0)
	msg_panic("qmgr_transport_create: transport exists: %s", name);
    transport = (QMGR_TRANSPORT *) mymalloc(sizeof(QMGR_TRANSPORT));
    transport->flags = 0;
    transport->pending = 0;
    transport->name = mystrdup(name);

    /*
     * Use global configuration settings or transport-specific settings.
     */
    transport->dest_concurrency_limit =
	get_mail_conf_int2(name, _DEST_CON_LIMIT,
			   var_dest_con_limit, 0, 0);
    transport->recipient_limit =
	get_mail_conf_int2(name, _DEST_RCPT_LIMIT,
			   var_dest_rcpt_limit, 0, 0);
    transport->init_dest_concurrency =
	get_mail_conf_int2(name, _INIT_DEST_CON,
			   var_init_dest_concurrency, 1, 0);
    transport->xport_rate_delay = get_mail_conf_time2(name, _XPORT_RATE_DELAY,
						      var_xport_rate_delay,
						      's', 0, 0);
    transport->rate_delay = get_mail_conf_time2(name, _DEST_RATE_DELAY,
						var_dest_rate_delay,
						's', 0, 0);

    if (transport->rate_delay > 0)
	transport->dest_concurrency_limit = 1;
    if (transport->dest_concurrency_limit != 0
    && transport->dest_concurrency_limit < transport->init_dest_concurrency)
	transport->init_dest_concurrency = transport->dest_concurrency_limit;

    transport->slot_cost = get_mail_conf_int2(name, _DELIVERY_SLOT_COST,
					      var_delivery_slot_cost, 0, 0);
    transport->slot_loan = get_mail_conf_int2(name, _DELIVERY_SLOT_LOAN,
					      var_delivery_slot_loan, 0, 0);
    transport->slot_loan_factor =
	100 - get_mail_conf_int2(name, _DELIVERY_SLOT_DISCOUNT,
				 var_delivery_slot_discount, 0, 100);
    transport->min_slots = get_mail_conf_int2(name, _MIN_DELIVERY_SLOTS,
					      var_min_delivery_slots, 0, 0);
    transport->rcpt_unused = get_mail_conf_int2(name, _XPORT_RCPT_LIMIT,
						var_xport_rcpt_limit, 0, 0);
    transport->rcpt_per_stack = get_mail_conf_int2(name, _STACK_RCPT_LIMIT,
						var_stack_rcpt_limit, 0, 0);
    transport->refill_limit = get_mail_conf_int2(name, _XPORT_REFILL_LIMIT,
					      var_xport_refill_limit, 1, 0);
    transport->refill_delay = get_mail_conf_time2(name, _XPORT_REFILL_DELAY,
					 var_xport_refill_delay, 's', 1, 0);

    transport->queue_byname = htable_create(0);
    QMGR_LIST_INIT(transport->queue_list);
    transport->job_byname = htable_create(0);
    QMGR_LIST_INIT(transport->job_list);
    QMGR_LIST_INIT(transport->job_bytime);
    transport->job_current = 0;
    transport->job_next_unread = 0;
    transport->candidate_cache = 0;
    transport->candidate_cache_current = 0;
    transport->candidate_cache_time = (time_t) 0;
    transport->blocker_tag = 1;
    transport->dsn = 0;
    qmgr_feedback_init(&transport->pos_feedback, name, _CONC_POS_FDBACK,
		       VAR_CONC_POS_FDBACK, var_conc_pos_feedback);
    qmgr_feedback_init(&transport->neg_feedback, name, _CONC_NEG_FDBACK,
		       VAR_CONC_NEG_FDBACK, var_conc_neg_feedback);
    transport->fail_cohort_limit =
	get_mail_conf_int2(name, _CONC_COHORT_LIM,
			   var_conc_cohort_limit, 0, 0);
    if (qmgr_transport_byname == 0)
	qmgr_transport_byname = htable_create(10);
    htable_enter(qmgr_transport_byname, name, (void *) transport);
    QMGR_LIST_PREPEND(qmgr_transport_list, transport, peers);
    if (msg_verbose)
	msg_info("qmgr_transport_create: %s concurrency %d recipients %d",
		 transport->name, transport->dest_concurrency_limit,
		 transport->recipient_limit);
    return (transport);
}

/* qmgr_transport_find - find transport instance */

QMGR_TRANSPORT *qmgr_transport_find(const char *name)
{
    return ((QMGR_TRANSPORT *) htable_find(qmgr_transport_byname, name));
}
