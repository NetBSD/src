/*++
/* NAME
/*	qmgr 3h
/* SUMMARY
/*	queue manager data structures
/* SYNOPSIS
/*	#include "qmgr.h"
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/time.h>
#include <time.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <scan_dir.h>

 /*
  * Global library.
  */
#include <recipient_list.h>
#include <dsn.h>

 /*
  * The queue manager is built around lots of mutually-referring structures.
  * These typedefs save some typing.
  */
typedef struct QMGR_TRANSPORT QMGR_TRANSPORT;
typedef struct QMGR_QUEUE QMGR_QUEUE;
typedef struct QMGR_ENTRY QMGR_ENTRY;
typedef struct QMGR_MESSAGE QMGR_MESSAGE;
typedef struct QMGR_TRANSPORT_LIST QMGR_TRANSPORT_LIST;
typedef struct QMGR_QUEUE_LIST QMGR_QUEUE_LIST;
typedef struct QMGR_ENTRY_LIST QMGR_ENTRY_LIST;
typedef struct QMGR_SCAN QMGR_SCAN;
typedef struct QMGR_FEEDBACK QMGR_FEEDBACK;

 /*
  * Hairy macros to update doubly-linked lists.
  */
#define QMGR_LIST_ROTATE(head, object) { \
    head.next->peers.prev = head.prev; \
    head.prev->peers.next = head.next; \
    head.next = object->peers.next; \
    if (object->peers.next) \
	head.next->peers.prev = 0; \
    head.prev = object; \
    object->peers.next = 0; \
}

#define QMGR_LIST_UNLINK(head, type, object) { \
    type   next = object->peers.next; \
    type   prev = object->peers.prev; \
    if (prev) prev->peers.next = next; \
    else head.next = next; \
    if (next) next->peers.prev = prev; \
    else head.prev = prev; \
    object->peers.next = object->peers.prev = 0; \
}

#define QMGR_LIST_APPEND(head, object) { \
    object->peers.next = head.next; \
    object->peers.prev = 0; \
    if (head.next) { \
	head.next->peers.prev = object; \
    } else { \
	head.prev = object; \
    } \
    head.next = object; \
}

#define QMGR_LIST_PREPEND(head, object) { \
    object->peers.prev = head.prev; \
    object->peers.next = 0; \
    if (head.prev) { \
	head.prev->peers.next = object; \
    } else { \
	head.next = object; \
    } \
    head.prev = object; \
}

#define QMGR_LIST_INIT(head) { \
    head.prev = 0; \
    head.next = 0; \
}

 /*
  * Transports are looked up by name (when we have resolved a message), or
  * round-robin wise (when we want to distribute resources fairly).
  */
struct QMGR_TRANSPORT_LIST {
    QMGR_TRANSPORT *next;
    QMGR_TRANSPORT *prev;
};

extern struct HTABLE *qmgr_transport_byname;	/* transport by name */
extern QMGR_TRANSPORT_LIST qmgr_transport_list;	/* transports, round robin */

 /*
  * Delivery agents provide feedback, as hints that Postfix should expend
  * more or fewer resources on a specific destination domain. The main.cf
  * file specifies how feedback affects delivery concurrency: add/subtract a
  * constant, a ratio of constants, or a constant divided by the delivery
  * concurrency; and it specifies how much feedback must accumulate between
  * concurrency updates.
  */
struct QMGR_FEEDBACK {
    int     hysteresis;			/* to pass, need to be this tall */
    double  base;			/* pre-computed from main.cf */
    int     index;			/* none, window, sqrt(window) */
};

#define QMGR_FEEDBACK_IDX_NONE		0	/* no window dependence */
#define QMGR_FEEDBACK_IDX_WIN		1	/* 1/window dependence */
#if 0
#define QMGR_FEEDBACK_IDX_SQRT_WIN	2	/* 1/sqrt(window) dependence */
#endif

#ifdef QMGR_FEEDBACK_IDX_SQRT_WIN
#include <math.h>
#endif

extern void qmgr_feedback_init(QMGR_FEEDBACK *, const char *, const char *, const char *, const char *);

#ifndef QMGR_FEEDBACK_IDX_SQRT_WIN
#define QMGR_FEEDBACK_VAL(fb, win) \
    ((fb).index == QMGR_FEEDBACK_IDX_NONE ? (fb).base : (fb).base / (win))
#else
#define QMGR_FEEDBACK_VAL(fb, win) \
    ((fb).index == QMGR_FEEDBACK_IDX_NONE ? (fb).base : \
    (fb).index == QMGR_FEEDBACK_IDX_WIN ? (fb).base / (win) : \
    (fb).base / sqrt(win))
#endif

 /*
  * Each transport (local, smtp-out, bounce) can have one queue per next hop
  * name. Queues are looked up by next hop name (when we have resolved a
  * message destination), or round-robin wise (when we want to deliver
  * messages fairly).
  */
struct QMGR_QUEUE_LIST {
    QMGR_QUEUE *next;
    QMGR_QUEUE *prev;
};

struct QMGR_TRANSPORT {
    int     flags;			/* blocked, etc. */
    int     pending;			/* incomplete DA connections */
    char   *name;			/* transport name */
    int     dest_concurrency_limit;	/* concurrency per domain */
    int     init_dest_concurrency;	/* init. per-domain concurrency */
    int     recipient_limit;		/* recipients per transaction */
    struct HTABLE *queue_byname;	/* queues indexed by domain */
    QMGR_QUEUE_LIST queue_list;		/* queues, round robin order */
    QMGR_TRANSPORT_LIST peers;		/* linkage */
    DSN    *dsn;			/* why unavailable */
    QMGR_FEEDBACK pos_feedback;		/* positive feedback control */
    QMGR_FEEDBACK neg_feedback;		/* negative feedback control */
    int     fail_cohort_limit;		/* flow shutdown control */
    int     rate_delay;			/* suspend per delivery */
};

#define QMGR_TRANSPORT_STAT_DEAD	(1<<1)

typedef void (*QMGR_TRANSPORT_ALLOC_NOTIFY) (QMGR_TRANSPORT *, VSTREAM *);
extern QMGR_TRANSPORT *qmgr_transport_select(void);
extern void qmgr_transport_alloc(QMGR_TRANSPORT *, QMGR_TRANSPORT_ALLOC_NOTIFY);
extern void qmgr_transport_throttle(QMGR_TRANSPORT *, DSN *);
extern void qmgr_transport_unthrottle(QMGR_TRANSPORT *);
extern QMGR_TRANSPORT *qmgr_transport_create(const char *);
extern QMGR_TRANSPORT *qmgr_transport_find(const char *);

#define QMGR_TRANSPORT_THROTTLED(t)	((t)->flags & QMGR_TRANSPORT_STAT_DEAD)

 /*
  * Each next hop (e.g., a domain name) has its own queue of pending message
  * transactions. The "todo" queue contains messages that are to be delivered
  * to this next hop. When a message is elected for transmission, it is moved
  * from the "todo" queue to the "busy" queue. Messages are taken from the
  * "todo" queue in sequence. An initial destination delivery concurrency > 1
  * ensures that one problematic message will not block all other traffic to
  * that next hop.
  */
struct QMGR_ENTRY_LIST {
    QMGR_ENTRY *next;
    QMGR_ENTRY *prev;
};

struct QMGR_QUEUE {
    int     dflags;			/* delivery request options */
    time_t  last_done;			/* last delivery completion */
    char   *name;			/* domain name or address */
    char   *nexthop;			/* domain name */
    int     todo_refcount;		/* queue entries (todo list) */
    int     busy_refcount;		/* queue entries (busy list) */
    int     window;			/* slow open algorithm */
    double  success;			/* accumulated positive feedback */
    double  failure;			/* accumulated negative feedback */
    double  fail_cohorts;		/* pseudo-cohort failure count */
    QMGR_TRANSPORT *transport;		/* transport linkage */
    QMGR_ENTRY_LIST todo;		/* todo queue entries */
    QMGR_ENTRY_LIST busy;		/* messages on the wire */
    QMGR_QUEUE_LIST peers;		/* neighbor queues */
    DSN    *dsn;			/* why unavailable */
    time_t  clog_time_to_warn;		/* time of next warning */
};

#define	QMGR_QUEUE_TODO	1		/* waiting for service */
#define QMGR_QUEUE_BUSY	2		/* recipients on the wire */

extern int qmgr_queue_count;

extern QMGR_QUEUE *qmgr_queue_create(QMGR_TRANSPORT *, const char *, const char *);
extern QMGR_QUEUE *qmgr_queue_select(QMGR_TRANSPORT *);
extern void qmgr_queue_done(QMGR_QUEUE *);
extern void qmgr_queue_throttle(QMGR_QUEUE *, DSN *);
extern void qmgr_queue_unthrottle(QMGR_QUEUE *);
extern QMGR_QUEUE *qmgr_queue_find(QMGR_TRANSPORT *, const char *);
extern void qmgr_queue_suspend(QMGR_QUEUE *, int);

 /*
  * Exclusive queue states. Originally there were only two: "throttled" and
  * "not throttled". It was natural to encode these in the queue window size.
  * After 10 years it's not practical to rip out all the working code and
  * change representations, so we just clean up the names a little.
  * 
  * Note: only the "ready" state can reach every state (including itself);
  * non-ready states can reach only the "ready" state. Other transitions are
  * forbidden, because they would result in dangling event handlers.
  */
#define QMGR_QUEUE_STAT_THROTTLED	0	/* back-off timer */
#define QMGR_QUEUE_STAT_SUSPENDED	-1	/* voluntary delay timer */
#define QMGR_QUEUE_STAT_SAVED		-2	/* delayed cleanup timer */
#define QMGR_QUEUE_STAT_BAD		-3	/* can't happen */

#define QMGR_QUEUE_READY(q)	((q)->window > 0)
#define QMGR_QUEUE_THROTTLED(q)	((q)->window == QMGR_QUEUE_STAT_THROTTLED)
#define QMGR_QUEUE_SUSPENDED(q)	((q)->window == QMGR_QUEUE_STAT_SUSPENDED)
#define QMGR_QUEUE_SAVED(q)	((q)->window == QMGR_QUEUE_STAT_SAVED)
#define QMGR_QUEUE_BAD(q)	((q)->window <= QMGR_QUEUE_STAT_BAD)

#define QMGR_QUEUE_STATUS(q) ( \
	    QMGR_QUEUE_READY(q) ? "ready" : \
	    QMGR_QUEUE_THROTTLED(q) ? "throttled" : \
	    QMGR_QUEUE_SUSPENDED(q) ? "suspended" : \
	    QMGR_QUEUE_SAVED(q) ? "saved" : \
	    "invalid queue status" \
	)

 /*
  * Structure of one next-hop queue entry. In order to save some copying
  * effort we allow multiple recipients per transaction.
  */
struct QMGR_ENTRY {
    VSTREAM *stream;			/* delivery process */
    QMGR_MESSAGE *message;		/* message info */
    RECIPIENT_LIST rcpt_list;		/* as many as it takes */
    QMGR_QUEUE *queue;			/* parent linkage */
    QMGR_ENTRY_LIST peers;		/* neighbor entries */
};

extern QMGR_ENTRY *qmgr_entry_select(QMGR_QUEUE *);
extern void qmgr_entry_unselect(QMGR_QUEUE *, QMGR_ENTRY *);
extern void qmgr_entry_move_todo(QMGR_QUEUE *, QMGR_ENTRY *);
extern void qmgr_entry_done(QMGR_ENTRY *, int);
extern QMGR_ENTRY *qmgr_entry_create(QMGR_QUEUE *, QMGR_MESSAGE *);

 /*
  * All common in-core information about a message is kept here. When all
  * recipients have been tried the message file is linked to the "deferred"
  * queue (some hosts not reachable), to the "bounce" queue (some recipients
  * were rejected), and is then removed from the "active" queue.
  */
struct QMGR_MESSAGE {
    int     flags;			/* delivery problems */
    int     qflags;			/* queuing flags */
    int     tflags;			/* tracing flags */
    long    tflags_offset;		/* offset for killing */
    int     rflags;			/* queue file read flags */
    VSTREAM *fp;			/* open queue file or null */
    int     refcount;			/* queue entries */
    int     single_rcpt;		/* send one rcpt at a time */
    struct timeval arrival_time;	/* start of receive transaction */
    time_t  create_time;		/* queue file create time */
    struct timeval active_time;		/* time of entry into active queue */
    long    warn_offset;		/* warning bounce flag offset */
    time_t  warn_time;			/* time next warning to be sent */
    long    data_offset;		/* data seek offset */
    char   *queue_name;			/* queue name */
    char   *queue_id;			/* queue file */
    char   *encoding;			/* content encoding */
    char   *sender;			/* complete address */
    char   *dsn_envid;			/* DSN envelope ID */
    int     dsn_ret;			/* DSN headers/full */
    char   *verp_delims;		/* VERP delimiters */
    char   *filter_xport;		/* filtering transport */
    char   *inspect_xport;		/* inspecting transport */
    char   *redirect_addr;		/* info@spammer.tld */
    long    data_size;			/* data segment size */
    long    cont_length;		/* message content length */
    long    rcpt_offset;		/* more recipients here */
    char   *client_name;		/* client hostname */
    char   *client_addr;		/* client address */
    char   *client_port;		/* client port */
    char   *client_proto;		/* client protocol */
    char   *client_helo;		/* helo parameter */
    char   *sasl_method;		/* SASL method */
    char   *sasl_username;		/* SASL user name */
    char   *sasl_sender;		/* SASL sender */
    char   *rewrite_context;		/* address qualification */
    RECIPIENT_LIST rcpt_list;		/* complete addresses */
};

 /*
  * Flags 0-15 are reserved for qmgr_user.h.
  */
#define QMGR_READ_FLAG_SEEN_ALL_NON_RCPT	(1<<16)

#define QMGR_MESSAGE_LOCKED	((QMGR_MESSAGE *) 1)

extern int qmgr_message_count;
extern int qmgr_recipient_count;

extern void qmgr_message_free(QMGR_MESSAGE *);
extern void qmgr_message_update_warn(QMGR_MESSAGE *);
extern void qmgr_message_kill_record(QMGR_MESSAGE *, long);
extern QMGR_MESSAGE *qmgr_message_alloc(const char *, const char *, int, mode_t);
extern QMGR_MESSAGE *qmgr_message_realloc(QMGR_MESSAGE *);

#define QMGR_MSG_STATS(stats, message) \
    MSG_STATS_INIT2(stats, \
		    incoming_arrival, message->arrival_time, \
		    active_arrival, message->active_time)

 /*
  * qmgr_defer.c
  */
extern void qmgr_defer_transport(QMGR_TRANSPORT *, DSN *);
extern void qmgr_defer_todo(QMGR_QUEUE *, DSN *);
extern void qmgr_defer_recipient(QMGR_MESSAGE *, RECIPIENT *, DSN *);

 /*
  * qmgr_bounce.c
  */
extern void qmgr_bounce_recipient(QMGR_MESSAGE *, RECIPIENT *, DSN *);

 /*
  * qmgr_deliver.c
  */
extern int qmgr_deliver_concurrency;
extern void qmgr_deliver(QMGR_TRANSPORT *, VSTREAM *);

 /*
  * qmgr_active.c
  */
extern int qmgr_active_feed(QMGR_SCAN *, const char *);
extern void qmgr_active_drain(void);
extern void qmgr_active_done(QMGR_MESSAGE *);

 /*
  * qmgr_move.c
  */
extern void qmgr_move(const char *, const char *, time_t);

 /*
  * qmgr_enable.c
  */
extern void qmgr_enable_all(void);
extern void qmgr_enable_transport(QMGR_TRANSPORT *);
extern void qmgr_enable_queue(QMGR_QUEUE *);

 /*
  * Queue scan context.
  */
struct QMGR_SCAN {
    char   *queue;			/* queue name */
    int     flags;			/* private, this run */
    int     nflags;			/* private, next run */
    struct SCAN_DIR *handle;		/* scan */
};

 /*
  * Flags that control queue scans or destination selection. These are
  * similar to the QMGR_REQ_XXX request codes.
  */
#define QMGR_SCAN_START	(1<<0)		/* start now/restart when done */
#define QMGR_SCAN_ALL	(1<<1)		/* all queue file time stamps */
#define QMGR_FLUSH_ONCE	(1<<2)		/* unthrottle once */
#define QMGR_FLUSH_DFXP	(1<<3)		/* override defer_transports */
#define QMGR_FLUSH_EACH	(1<<4)		/* unthrottle per message */

 /*
  * qmgr_scan.c
  */
extern QMGR_SCAN *qmgr_scan_create(const char *);
extern void qmgr_scan_request(QMGR_SCAN *, int);
extern char *qmgr_scan_next(QMGR_SCAN *);

 /*
  * qmgr_error.c
  */
extern QMGR_TRANSPORT *qmgr_error_transport(const char *);
extern QMGR_QUEUE *qmgr_error_queue(const char *, DSN *);
extern char *qmgr_error_nexthop(DSN *);

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
