#ifndef _DELIVER_REQUEST_H_INCLUDED_
#define _DELIVER_REQUEST_H_INCLUDED_

/*++
/* NAME
/*	deliver_request 3h
/* SUMMARY
/*	mail delivery request protocol, server side
/* SYNOPSIS
/*	#include <deliver_request.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <recipient_list.h>

 /*
  * Structure of a server mail delivery request.
  */
typedef struct DELIVER_REQUEST {
    VSTREAM *fp;			/* stream, shared lock */
    int     flags;			/* see below */
    char   *queue_name;			/* message queue name */
    char   *queue_id;			/* message queue id */
    long    data_offset;		/* offset to message */
    long    data_size;			/* message size */
    char   *nexthop;			/* next hop name */
    char   *encoding;			/* content encoding */
    char   *sender;			/* envelope sender */
    char   *errors_to;			/* error report address */
    char   *return_receipt;		/* confirm receipt address */
    long    arrival_time;		/* arrival time */
    RECIPIENT_LIST rcpt_list;		/* envelope recipients */
    char   *hop_status;			/* reason if unavailable */
    char   *client_name;		/* client hostname */
    char   *client_addr;		/* client address */
    char   *client_proto;		/* client protocol */
    char   *client_helo;		/* helo parameter */
} DELIVER_REQUEST;

 /*
  * Since we can't send null pointers, null strings represent unavailable
  * attributes instead. They're less likely to explode in our face, too.
  */
#define DEL_REQ_ATTR_AVAIL(a)	(*(a))

 /*
  * How to deliver, really?
  */
#define DEL_REQ_FLAG_DEFLT	(DEL_REQ_FLAG_SUCCESS | DEL_REQ_FLAG_BOUNCE)
#define DEL_REQ_FLAG_SUCCESS	(1<<0)	/* delete successful recipients */
#define DEL_REQ_FLAG_BOUNCE	(1<<1)	/* unimplemented */

#define DEL_REQ_FLAG_VERIFY	(1<<8)	/* verify recipient, don't deliver */
#define DEL_REQ_FLAG_EXPAND	(1<<9)	/* verify expansion, don't deliver */
#define DEL_REQ_FLAG_RECORD	(1<<10)	/* record and deliver */

#define DEL_REQ_TRACE_FLAGS_MASK \
	(DEL_REQ_FLAG_VERIFY | DEL_REQ_FLAG_EXPAND | DEL_REQ_FLAG_RECORD)
#define DEL_REQ_TRACE_FLAGS(f)	((f) & DEL_REQ_TRACE_FLAGS_MASK)

#define DEL_REQ_TRACE_ONLY_MASK \
	(DEL_REQ_FLAG_VERIFY | DEL_REQ_FLAG_EXPAND)
#define DEL_REQ_TRACE_ONLY(f)	((f) & DEL_REQ_TRACE_ONLY_MASK)

 /*
  * Per-recipient delivery status. Not to be confused with per-delivery
  * request status.
  */
#define DEL_RCPT_STAT_OK	0
#define DEL_RCPT_STAT_DEFER	1
#define DEL_RCPT_STAT_BOUNCE	2
#define DEL_RCPT_STAT_TODO	3

 /*
  * Delivery request status. Note that there are only FINAL and DEFER. This
  * is because delivery status information can be lost when a delivery agent
  * or queue manager process terminates prematurely. The only distinctions we
  * can rely on are "final delivery completed" (positive confirmation that
  * all recipients are marked as done) and "everything else". In the absence
  * of a definitive statement the queue manager will always have to be
  * prepared for all possibilities.
  */
#define DEL_STAT_FINAL	0		/* delivered or bounced */
#define DEL_STAT_DEFER	(-1)		/* not delivered or bounced */

typedef struct VSTREAM _deliver_vstream_;
extern DELIVER_REQUEST *deliver_request_read(_deliver_vstream_ *);
extern int deliver_request_done(_deliver_vstream_ *, DELIVER_REQUEST *, int);

extern int deliver_pass(const char *, const char *, DELIVER_REQUEST *, const char *, const char *, long);

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
