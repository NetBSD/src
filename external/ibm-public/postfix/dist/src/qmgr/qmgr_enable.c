/*	$NetBSD: qmgr_enable.c,v 1.1.1.1.2.2 2009/09/15 06:03:31 snj Exp $	*/

/*++
/* NAME
/*	qmgr_enable
/* SUMMARY
/*	enable dead transports or sites
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	void	qmgr_enable_queue(queue)
/*	QMGR_QUEUE *queue;
/*
/*	QMGR_QUEUE *qmgr_enable_transport(transport)
/*	QMGR_TRANSPORT *transport;
/*
/*	void	qmgr_enable_all(void)
/* DESCRIPTION
/*	This module purges dead in-core state information, effectively
/*	re-enabling delivery.
/*
/*	qmgr_enable_queue() enables deliveries to the named dead site.
/*	Empty queues are destroyed. The existed solely to indicate that
/*	a site is dead.
/*
/*	qmgr_enable_transport() enables deliveries via the specified
/*	transport, and calls qmgr_enable_queue() for each destination
/*	on that transport.  Empty queues are destroyed.
/*
/*	qmgr_enable_all() enables all transports and queues.
/*	See above for the side effects caused by doing this.
/* BUGS
/*	The side effects of calling this module can be quite dramatic.
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

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_enable_all - enable transports and queues */

void    qmgr_enable_all(void)
{
    QMGR_TRANSPORT *xport;

    if (msg_verbose)
	msg_info("qmgr_enable_all");

    /*
     * The number of transports does not change as a side effect, so this can
     * be a straightforward loop.
     */
    for (xport = qmgr_transport_list.next; xport; xport = xport->peers.next)
	qmgr_enable_transport(xport);
}

/* qmgr_enable_transport - defer todo entries for named transport */

void    qmgr_enable_transport(QMGR_TRANSPORT *transport)
{
    QMGR_QUEUE *queue;
    QMGR_QUEUE *next;

    /*
     * Proceed carefully. Queues may disappear as a side effect.
     */
    if (transport->flags & QMGR_TRANSPORT_STAT_DEAD) {
	if (msg_verbose)
	    msg_info("enable transport %s", transport->name);
	qmgr_transport_unthrottle(transport);
    }
    for (queue = transport->queue_list.next; queue; queue = next) {
	next = queue->peers.next;
	qmgr_enable_queue(queue);
    }
}

/* qmgr_enable_queue - enable and possibly delete queue */

void    qmgr_enable_queue(QMGR_QUEUE *queue)
{
    if (QMGR_QUEUE_THROTTLED(queue)) {
	if (msg_verbose)
	    msg_info("enable site %s/%s", queue->transport->name, queue->name);
	qmgr_queue_unthrottle(queue);
    }
    if (QMGR_QUEUE_READY(queue) && queue->todo.next == 0 && queue->busy.next == 0)
	qmgr_queue_done(queue);
}
