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
/*	void	qmgr_transport_throttle(transport, reason)
/*	QMGR_TRANSPORT *transport;
/*	const char *reason;
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>

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

/* qmgr_transport_unthrottle_wrapper - in case (char *) != (struct *) */

static void qmgr_transport_unthrottle_wrapper(int unused_event, char *context)
{
    qmgr_transport_unthrottle((QMGR_TRANSPORT *) context);
}

/* qmgr_transport_unthrottle - open the throttle */

void    qmgr_transport_unthrottle(QMGR_TRANSPORT *transport)
{
    char   *myname = "qmgr_transport_unthrottle";

    /*
     * This routine runs after expiration of the timer set by
     * qmgr_transport_throttle(), or whenever a delivery transport has been
     * used without malfunction. In either case, we enable delivery again if
     * the transport was blocked, otherwise the request is ignored.
     */
    if ((transport->flags & QMGR_TRANSPORT_STAT_DEAD) != 0) {
	if (msg_verbose)
	    msg_info("%s: transport %s", myname, transport->name);
	transport->flags &= ~QMGR_TRANSPORT_STAT_DEAD;
	if (transport->reason == 0)
	    msg_panic("%s: transport %s: null reason", myname, transport->name);
	myfree(transport->reason);
	transport->reason = 0;
	event_cancel_timer(qmgr_transport_unthrottle_wrapper,
			   (char *) transport);
    }
}

/* qmgr_transport_throttle - disable delivery process allocation */

void    qmgr_transport_throttle(QMGR_TRANSPORT *transport, const char *reason)
{
    char   *myname = "qmgr_transport_throttle";

    /*
     * We are unable to connect to a deliver process for this type of message
     * transport. Instead of hosing the system by retrying in a tight loop,
     * back off and disable this transport type for a while.
     */
    if ((transport->flags & QMGR_TRANSPORT_STAT_DEAD) == 0) {
	if (msg_verbose)
	    msg_info("%s: transport %s: reason: %s",
		     myname, transport->name, reason);
	transport->flags |= QMGR_TRANSPORT_STAT_DEAD;
	if (transport->reason)
	    msg_panic("%s: transport %s: spurious reason: %s",
		      myname, transport->name, transport->reason);
	transport->reason = mystrdup(reason);
	event_request_timer(qmgr_transport_unthrottle_wrapper,
			    (char *) transport, var_transport_retry_time);
    }
}

/* qmgr_transport_abort - transport connect watchdog */

static void qmgr_transport_abort(int unused_event, char *context)
{
    QMGR_TRANSPORT_ALLOC *alloc = (QMGR_TRANSPORT_ALLOC *) context;

    msg_fatal("timeout connecting to transport: %s", alloc->transport->name);
}

/* qmgr_transport_event - delivery process availability notice */

static void qmgr_transport_event(int unused_event, char *context)
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
     * Disable further read events that end up calling this function.
     */
    event_disable_readwrite(vstream_fileno(alloc->stream));
    alloc->transport->flags &= ~QMGR_TRANSPORT_STAT_BUSY;

    /*
     * Notify the requestor.
     */
    alloc->notify(alloc->transport, alloc->stream);
    myfree((char *) alloc);
}

#ifdef UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT

/* qmgr_transport_connect - handle connection request completion */

static void qmgr_transport_connect(int unused_event, char *context)
{
    QMGR_TRANSPORT_ALLOC *alloc = (QMGR_TRANSPORT_ALLOC *) context;

    /*
     * This code is necessary for some versions of LINUX, where connect(2)
     * blocks until the application performs an accept(2). Reportedly, the
     * same can happen on Solaris 2.5.1.
     */
    event_disable_readwrite(vstream_fileno(alloc->stream));
    non_blocking(vstream_fileno(alloc->stream), BLOCKING);
    event_enable_read(vstream_fileno(alloc->stream),
		      qmgr_transport_event, (char *) alloc);
}

#endif

/* qmgr_transport_select - select transport for allocation */

QMGR_TRANSPORT *qmgr_transport_select(void)
{
    QMGR_TRANSPORT *xport;
    QMGR_QUEUE *queue;

    /*
     * If we find a suitable transport, rotate the list of transports to
     * effectuate round-robin selection. See similar selection code in
     * qmgr_queue_select().
     */
#define STAY_AWAY (QMGR_TRANSPORT_STAT_BUSY | QMGR_TRANSPORT_STAT_DEAD)

    for (xport = qmgr_transport_list.next; xport; xport = xport->peers.next) {
	if (xport->flags & STAY_AWAY)
	    continue;
	for (queue = xport->queue_list.next; queue; queue = queue->peers.next) {
	    if (queue->window > queue->busy_refcount && queue->todo.next != 0) {
		QMGR_LIST_ROTATE(qmgr_transport_list, xport);
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
    VSTREAM *stream;

    /*
     * Sanity checks.
     */
    if (transport->flags & QMGR_TRANSPORT_STAT_DEAD)
	msg_panic("qmgr_transport: dead transport: %s", transport->name);
    if (transport->flags & QMGR_TRANSPORT_STAT_BUSY)
	msg_panic("qmgr_transport: nested allocation: %s", transport->name);

    /*
     * Connect to the well-known port for this delivery service, and wake up
     * when a process announces its availability. In the mean time, block out
     * other delivery process allocation attempts for this transport. In case
     * of problems, back off. Do not hose the system when it is in trouble
     * already.
     */
#ifdef UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#define BLOCK_MODE	NON_BLOCKING
#define ENABLE_EVENTS	event_enable_write
#define EVENT_HANDLER	qmgr_transport_connect
#else
#define BLOCK_MODE	BLOCKING
#define ENABLE_EVENTS	event_enable_read
#define EVENT_HANDLER	qmgr_transport_event
#endif

    if ((stream = mail_connect(MAIL_CLASS_PRIVATE, transport->name, BLOCK_MODE)) == 0) {
	msg_warn("connect to transport %s: %m", transport->name);
	qmgr_transport_throttle(transport, "transport is unavailable");
	return;
    }
    alloc = (QMGR_TRANSPORT_ALLOC *) mymalloc(sizeof(*alloc));
    alloc->stream = stream;
    alloc->transport = transport;
    alloc->notify = notify;
    transport->flags |= QMGR_TRANSPORT_STAT_BUSY;
    ENABLE_EVENTS(vstream_fileno(alloc->stream), EVENT_HANDLER, (char *) alloc);

    /*
     * Guard against broken systems.
     */
    event_request_timer(qmgr_transport_abort, (char *) alloc,
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
    transport->name = mystrdup(name);

    /*
     * Use global configuration settings or transport-specific settings.
     */
    transport->dest_concurrency_limit =
	get_mail_conf_int2(name, "_destination_concurrency_limit",
			   var_dest_con_limit, 0, 0);
    transport->recipient_limit =
	get_mail_conf_int2(name, "_destination_recipient_limit",
			   var_dest_rcpt_limit, 0, 0);

    if (transport->dest_concurrency_limit == 0
	|| transport->dest_concurrency_limit >= var_init_dest_concurrency)
	transport->init_dest_concurrency = var_init_dest_concurrency;
    else
	transport->init_dest_concurrency = transport->dest_concurrency_limit;

    transport->queue_byname = htable_create(0);
    QMGR_LIST_INIT(transport->queue_list);
    transport->reason = 0;
    if (qmgr_transport_byname == 0)
	qmgr_transport_byname = htable_create(10);
    htable_enter(qmgr_transport_byname, name, (char *) transport);
    QMGR_LIST_APPEND(qmgr_transport_list, transport);
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
