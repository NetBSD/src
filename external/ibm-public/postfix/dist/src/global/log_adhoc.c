/*	$NetBSD: log_adhoc.c,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

/*++
/* NAME
/*	log_adhoc 3
/* SUMMARY
/*	ad-hoc delivery event logging
/* SYNOPSIS
/*	#include <log_adhoc.h>
/*
/*	void	log_adhoc(id, stats, recipient, relay, dsn, status)
/*	const char *id;
/*	MSG_STATS *stats;
/*	RECIPIENT *recipient;
/*	const char *relay;
/*	DSN *dsn;
/*	const char *status;
/* DESCRIPTION
/*	This module logs delivery events in an ad-hoc manner.
/*
/*	log_adhoc() appends a record to the mail logfile
/*
/*	Arguments:
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The queue id of the original message file.
/* .IP stats
/*	Time stamps from different message delivery stages
/*	and session reuse count.
/* .IP recipient
/*	Recipient information. See recipient_list(3).
/* .IP sender
/*	The sender envelope address.
/* .IP relay
/*	Host we could (not) talk to.
/* .IP status
/*	bounced, deferred, sent, and so on.
/* .IP dsn
/*	Delivery status information. See dsn(3).
/* BUGS
/*	Should be replaced by routines with an attribute-value based
/*	interface instead of an interface that uses a rigid argument list.
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
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <format_tv.h>

/* Global library. */

#include <log_adhoc.h>
#include <mail_params.h>

 /*
  * Don't use "struct timeval" for time differences; use explicit signed
  * types instead. The code below relies on signed values to detect clocks
  * jumping back.
  */
typedef struct {
    int     dt_sec;			/* make sure it's signed */
    int     dt_usec;			/* make sure it's signed */
} DELTA_TIME;

/* log_adhoc - ad-hoc logging */

void    log_adhoc(const char *id, MSG_STATS *stats, RECIPIENT *recipient,
		          const char *relay, DSN *dsn,
		          const char *status)
{
    static VSTRING *buf;
    DELTA_TIME delay;			/* end-to-end delay */
    DELTA_TIME pdelay;			/* time before queue manager */
    DELTA_TIME adelay;			/* queue manager latency */
    DELTA_TIME sdelay;			/* connection set-up latency */
    DELTA_TIME xdelay;			/* transmission latency */
    struct timeval now;

    /*
     * Alas, we need an intermediate buffer for the pre-formatted result.
     * There are several optional fields, and the delay fields are formatted
     * in a manner that is not supported by vstring_sprintf().
     */
    if (buf == 0)
	buf = vstring_alloc(100);

    /*
     * First, critical information that identifies the nature of the
     * transaction.
     */
    vstring_sprintf(buf, "%s: to=<%s>", id, recipient->address);
    if (recipient->orig_addr && *recipient->orig_addr
	&& strcasecmp(recipient->address, recipient->orig_addr) != 0)
	vstring_sprintf_append(buf, ", orig_to=<%s>", recipient->orig_addr);
    vstring_sprintf_append(buf, ", relay=%s", relay);
    if (stats->reuse_count > 0)
	vstring_sprintf_append(buf, ", conn_use=%d", stats->reuse_count + 1);

    /*
     * Next, performance statistics.
     * 
     * Use wall clock time to compute pdelay (before active queue latency) if
     * there is no time stamp for active queue entry. This happens when mail
     * is bounced by the cleanup server.
     * 
     * Otherwise, use wall clock time to compute adelay (active queue latency)
     * if there is no time stamp for hand-off to delivery agent. This happens
     * when mail was deferred or bounced by the queue manager.
     * 
     * Otherwise, use wall clock time to compute xdelay (message transfer
     * latency) if there is no time stamp for delivery completion. In the
     * case of multi-recipient deliveries the delivery agent may specify the
     * delivery completion time, so that multiple recipient records show the
     * same delay values.
     * 
     * Don't compute the sdelay (connection setup latency) if there is no time
     * stamp for connection setup completion.
     * 
     * XXX Apparently, Solaris gettimeofday() can return out-of-range
     * microsecond values.
     */
#define DELTA(x, y, z) \
    do { \
	(x).dt_sec = (y).tv_sec - (z).tv_sec; \
	(x).dt_usec = (y).tv_usec - (z).tv_usec; \
	while ((x).dt_usec < 0) { \
	    (x).dt_usec += 1000000; \
	    (x).dt_sec -= 1; \
	} \
	while ((x).dt_usec >= 1000000) { \
	    (x).dt_usec -= 1000000; \
	    (x).dt_sec += 1; \
	} \
	if ((x).dt_sec < 0) \
	    (x).dt_sec = (x).dt_usec = 0; \
    } while (0)

#define DELTA_ZERO(x) ((x).dt_sec = (x).dt_usec = 0)

#define TIME_STAMPED(x) ((x).tv_sec > 0)

    if (TIME_STAMPED(stats->deliver_done))
	now = stats->deliver_done;
    else
	GETTIMEOFDAY(&now);

    DELTA(delay, now, stats->incoming_arrival);
    DELTA_ZERO(adelay);
    DELTA_ZERO(sdelay);
    DELTA_ZERO(xdelay);
    if (TIME_STAMPED(stats->active_arrival)) {
	DELTA(pdelay, stats->active_arrival, stats->incoming_arrival);
	if (TIME_STAMPED(stats->agent_handoff)) {
	    DELTA(adelay, stats->agent_handoff, stats->active_arrival);
	    if (TIME_STAMPED(stats->conn_setup_done)) {
		DELTA(sdelay, stats->conn_setup_done, stats->agent_handoff);
		DELTA(xdelay, now, stats->conn_setup_done);
	    } else {
		/* No network client. */
		DELTA(xdelay, now, stats->agent_handoff);
	    }
	} else {
	    /* No delivery agent. */
	    DELTA(adelay, now, stats->active_arrival);
	}
    } else {
	/* No queue manager. */
	DELTA(pdelay, now, stats->incoming_arrival);
    }

    /*
     * Round off large time values to an integral number of seconds, and
     * display small numbers with only two significant digits, as long as
     * they do not exceed the time resolution.
     */
#define SIG_DIGS	2
#define PRETTY_FORMAT(b, text, x) \
    do { \
	vstring_strcat((b), text); \
	format_tv((b), (x).dt_sec, (x).dt_usec, SIG_DIGS, var_delay_max_res); \
    } while (0)

    PRETTY_FORMAT(buf, ", delay=", delay);
    PRETTY_FORMAT(buf, ", delays=", pdelay);
    PRETTY_FORMAT(buf, "/", adelay);
    PRETTY_FORMAT(buf, "/", sdelay);
    PRETTY_FORMAT(buf, "/", xdelay);

    /*
     * Finally, the delivery status.
     */
    vstring_sprintf_append(buf, ", dsn=%s, status=%s (%s)",
			   dsn->status, status, dsn->reason);

    /*
     * Ship it off.
     */
    msg_info("%s", vstring_str(buf));
}
