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
  * Utility library.
  */
#include <vstream.h>
#include <scan_dir.h>

 /*
  * Global library.
  */
#include <maps.h>

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
typedef struct QMGR_RCPT QMGR_RCPT;
typedef struct QMGR_RCPT_LIST QMGR_RCPT_LIST;
typedef struct QMGR_SCAN QMGR_SCAN;

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
    char   *name;			/* transport name */
    int     dest_concurrency_limit;	/* concurrency per domain */
    int     init_dest_concurrency;	/* init. per-domain concurrency */
    int     recipient_limit;		/* recipients per transaction */
    struct HTABLE *queue_byname;	/* queues indexed by domain */
    QMGR_QUEUE_LIST queue_list;		/* queues, round robin order */
    QMGR_TRANSPORT_LIST peers;		/* linkage */
    char   *reason;			/* why unavailable */
};

#define QMGR_TRANSPORT_STAT_DEAD	(1<<1)
#define QMGR_TRANSPORT_STAT_BUSY	(1<<2)

typedef void (*QMGR_TRANSPORT_ALLOC_NOTIFY) (QMGR_TRANSPORT *, VSTREAM *);
extern QMGR_TRANSPORT *qmgr_transport_select(void);
extern void qmgr_transport_alloc(QMGR_TRANSPORT *, QMGR_TRANSPORT_ALLOC_NOTIFY);
extern void qmgr_transport_throttle(QMGR_TRANSPORT *, const char *);
extern void qmgr_transport_unthrottle(QMGR_TRANSPORT *);
extern QMGR_TRANSPORT *qmgr_transport_create(const char *);
extern QMGR_TRANSPORT *qmgr_transport_find(const char *);

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
    char   *name;			/* domain name */
    int     todo_refcount;		/* queue entries (todo list) */
    int     busy_refcount;		/* queue entries (busy list) */
    int     window;			/* slow open algorithm */
    QMGR_TRANSPORT *transport;		/* transport linkage */
    QMGR_ENTRY_LIST todo;		/* todo queue entries */
    QMGR_ENTRY_LIST busy;		/* messages on the wire */
    QMGR_QUEUE_LIST peers;		/* neighbor queues */
    char   *reason;			/* why unavailable */
};

#define	QMGR_QUEUE_TODO	1		/* waiting for service */
#define QMGR_QUEUE_BUSY	2		/* recipients on the wire */

extern int qmgr_queue_count;

extern QMGR_QUEUE *qmgr_queue_create(QMGR_TRANSPORT *, const char *);
extern QMGR_QUEUE *qmgr_queue_select(QMGR_TRANSPORT *);
extern void qmgr_queue_done(QMGR_QUEUE *);
extern void qmgr_queue_throttle(QMGR_QUEUE *, const char *);
extern void qmgr_queue_unthrottle(QMGR_QUEUE *);
extern QMGR_QUEUE *qmgr_queue_find(QMGR_TRANSPORT *, const char *);

 /*
  * Structure for a recipient list. Initially, it just contains recipient
  * addresses and file offsets. After the address resolver has done its work,
  * each recipient is accompanied by a reference to a specific queues (which
  * implies a specific transport). This is an extended version of similar
  * information maintained by the recipient_list(3) module.
  */
struct QMGR_RCPT {
    long    offset;			/* REC_TYPE_RCPT byte */
    char   *address;			/* complete address */
    QMGR_QUEUE *queue;			/* resolved queue */
};

struct QMGR_RCPT_LIST {
    QMGR_RCPT *info;
    int     len;
    int     avail;
};

extern void qmgr_rcpt_list_init(QMGR_RCPT_LIST *);
extern void qmgr_rcpt_list_add(QMGR_RCPT_LIST *, long, const char *);
extern void qmgr_rcpt_list_free(QMGR_RCPT_LIST *);

 /*
  * Structure of one next-hop queue entry. In order to save some copying
  * effort we allow multiple recipients per transaction.
  */
struct QMGR_ENTRY {
    VSTREAM *stream;			/* delivery process */
    QMGR_MESSAGE *message;		/* message info */
    QMGR_RCPT_LIST rcpt_list;		/* as many as it takes */
    QMGR_QUEUE *queue;			/* parent linkage */
    QMGR_ENTRY_LIST peers;		/* neighbor entries */
};

extern QMGR_ENTRY *qmgr_entry_select(QMGR_QUEUE *);
extern void qmgr_entry_unselect(QMGR_QUEUE *, QMGR_ENTRY *);
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
    VSTREAM *fp;			/* open queue file or null */
    int     refcount;			/* queue entries */
    int     single_rcpt;		/* send one rcpt at a time */
    long    arrival_time;		/* time when queued */
    long    warn_offset;		/* warning bounce flag offset */
    time_t  warn_time;			/* time next warning to be sent */
    long    data_offset;		/* data seek offset */
    char   *queue_name;			/* queue name */
    char   *queue_id;			/* queue file */
    char   *sender;			/* complete address */
    char   *errors_to;			/* error report address */
    char   *return_receipt;		/* confirm receipt address */
    char   *filter_xport;		/* filtering transport */
    char   *inspect_xport;		/* inspecting transport */
    long    data_size;			/* message content size */
    long    rcpt_offset;		/* more recipients here */
    QMGR_RCPT_LIST rcpt_list;		/* complete addresses */
};

#define QMGR_MESSAGE_LOCKED	((QMGR_MESSAGE *) 1)

extern int qmgr_message_count;
extern int qmgr_recipient_count;
extern MAPS *qmgr_relocated;
extern MAPS *qmgr_virtual;

extern void qmgr_message_free(QMGR_MESSAGE *);
extern void qmgr_message_update_warn(QMGR_MESSAGE *);
extern QMGR_MESSAGE *qmgr_message_alloc(const char *, const char *, int);
extern QMGR_MESSAGE *qmgr_message_realloc(QMGR_MESSAGE *);

 /*
  * qmgr_defer.c
  */
extern void qmgr_defer_transport(QMGR_TRANSPORT *, const char *);
extern void qmgr_defer_todo(QMGR_QUEUE *, const char *);
extern void qmgr_defer_recipient(QMGR_MESSAGE *, const char *, const char *);

 /*
  * qmgr_bounce.c
  */
extern void PRINTFLIKE(3, 4) qmgr_bounce_recipient(QMGR_MESSAGE *, QMGR_RCPT *, const char *,...);

 /*
  * qmgr_deliver.c
  */
extern int qmgr_deliver_concurrency;
extern void qmgr_deliver(QMGR_TRANSPORT *, VSTREAM *);

 /*
  * qmgr_active.c
  */
extern void qmgr_active_feed(QMGR_SCAN *, const char *);
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
#define QMGR_FLUSH_DEAD	(1<<2)		/* all sites, all transports */

 /*
  * qmgr_scan.c
  */
extern QMGR_SCAN *qmgr_scan_create(const char *);
extern void qmgr_scan_request(QMGR_SCAN *, int);
extern char *qmgr_scan_next(QMGR_SCAN *);

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
