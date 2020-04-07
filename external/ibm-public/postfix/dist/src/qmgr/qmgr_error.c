/*	$NetBSD: qmgr_error.c,v 1.1.1.1 2009/06/23 10:08:52 tron Exp $	*/

/*++
/* NAME
/*	qmgr_error 3
/* SUMMARY
/*	look up/create error/retry queue
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_TRANSPORT *qmgr_error_transport(service)
/*	const char *service;
/*
/*	QMGR_QUEUE *qmgr_error_queue(service, dsn)
/*	const char *service;
/*	DSN	*dsn;
/*
/*	char	*qmgr_error_nexthop(dsn)
/*	DSN	*dsn;
/* DESCRIPTION
/*	qmgr_error_transport() looks up the error transport for the
/*	specified service. The result is null if the transport is
/*	not available.
/*
/*	qmgr_error_queue() looks up an error queue for the specified
/*	service and problem. The result is null if the queue is not
/*	availabe.
/*
/*	qmgr_error_nexthop() computes the next-hop information for
/*	the specified problem. The result must be passed to myfree().
/*
/*	Arguments:
/* .IP dsn
/*	See dsn(3).
/* .IP service
/*	One of MAIL_SERVICE_ERROR or MAIL_SERVICE_RETRY.
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

#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

/* Application-specific. */

#include "qmgr.h"

/* qmgr_error_transport - look up error transport for specified service */

QMGR_TRANSPORT *qmgr_error_transport(const char *service)
{
    QMGR_TRANSPORT *transport;

    /*
     * Find or create retry transport.
     */
    if ((transport = qmgr_transport_find(service)) == 0)
	transport = qmgr_transport_create(service);
    if (QMGR_TRANSPORT_THROTTLED(transport))
	return (0);

    /*
     * Done.
     */
    return (transport);
}

/* qmgr_error_queue - look up error queue for specified service and problem */

QMGR_QUEUE *qmgr_error_queue(const char *service, DSN *dsn)
{
    QMGR_TRANSPORT *transport;
    QMGR_QUEUE *queue;
    char   *nexthop;

    /*
     * Find or create transport.
     */
    if ((transport = qmgr_error_transport(service)) == 0)
	return (0);

    /*
     * Find or create queue.
     */
    nexthop = qmgr_error_nexthop(dsn);
    if ((queue = qmgr_queue_find(transport, nexthop)) == 0)
	queue = qmgr_queue_create(transport, nexthop, nexthop);
    myfree(nexthop);
    if (QMGR_QUEUE_THROTTLED(queue))
	return (0);

    /*
     * Done.
     */
    return (queue);
}

/* qmgr_error_nexthop - compute next-hop information from problem description */

char   *qmgr_error_nexthop(DSN *dsn)
{
    char   *nexthop;

    nexthop = concatenate(dsn->status, " ", dsn->reason, (char *) 0);
    return (nexthop);
}
