/*	$NetBSD: msg_stats.h,v 1.1.1.1.2.2 2009/09/15 06:02:50 snj Exp $	*/

#ifndef _MSG_STATS_H_INCLUDED_
#define _MSG_STATS_H_INCLUDED_

/*++
/* NAME
/*	msg_stats 3h
/* SUMMARY
/*	message delivery profiling
/* SYNOPSIS
/*	#include <msg_stats.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/time.h>
#include <time.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <attr.h>
#include <vstream.h>

 /*
  * External interface.
  * 
  * This structure contains the time stamps from various mail delivery stages,
  * as well as the connection reuse count. The time stamps provide additional
  * insight into the nature of performance bottle necks.
  * 
  * For convenience, we record absolute time stamps instead of time differences.
  * This is because the decision of what numbers to subtract actually depends
  * on program history. Since we prefer to compute time differences in one
  * place, we postpone this task until the end, in log_adhoc().
  * 
  * A zero time stamp or reuse count means the information is not supplied.
  * 
  * Specifically, a zero active_arrival value means that the message did not
  * reach the queue manager; and a zero agent_handoff time means that the
  * queue manager did not give the message to a delivery agent.
  * 
  * Some network clients update the conn_setup_done value when connection setup
  * fails or completes.
  * 
  * The deliver_done value is usually left at zero, which means use the wall
  * clock time when reporting recipient status information. The exception is
  * with delivery agents that can deliver multiple recipients in a single
  * transaction. These agents explicitly update the deliver_done time stamp
  * to ensure that multiple recipient records show the exact same delay
  * values.
  */
typedef struct {
    struct timeval incoming_arrival;	/* incoming queue entry */
    struct timeval active_arrival;	/* active queue entry */
    struct timeval agent_handoff;	/* delivery agent hand-off */
    struct timeval conn_setup_done;	/* connection set-up done */
    struct timeval deliver_done;	/* transmission done */
    int     reuse_count;		/* connection reuse count */
} MSG_STATS;

#define MSG_STATS_INIT(st) \
    ( \
	memset((char *) (st), 0, sizeof(*(st))), \
	(st) \
    )

#define MSG_STATS_INIT1(st, member, value) \
    ( \
	memset((char *) (st), 0, sizeof(*(st))), \
	((st)->member = (value)), \
	(st) \
    )

#define MSG_STATS_INIT2(st, m1, v1, m2, v2) \
    ( \
	memset((char *) (st), 0, sizeof(*(st))), \
	((st)->m1 = (v1)), \
	((st)->m2 = (v2)), \
	(st) \
    )

extern int msg_stats_scan(ATTR_SCAN_MASTER_FN, VSTREAM *, int, void *);
extern int msg_stats_print(ATTR_PRINT_MASTER_FN, VSTREAM *, int, void *);

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

#endif
