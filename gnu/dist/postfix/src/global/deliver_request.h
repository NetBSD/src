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
    char   *sender;			/* envelope sender */
    char   *errors_to;			/* error report address */
    char   *return_receipt;		/* confirm receipt address */
    long    arrival_time;		/* arrival time */
    RECIPIENT_LIST rcpt_list;		/* envelope recipients */
    char   *hop_status;			/* reason if unavailable */
} DELIVER_REQUEST;

#define DEL_REQ_FLAG_DEFLT	(DEL_REQ_FLAG_SUCCESS | DEL_REQ_FLAG_BOUNCE)
#define DEL_REQ_FLAG_SUCCESS	(1<<0)	/* delete successful recipients */
#define DEL_REQ_FLAG_BOUNCE	(1<<1)	/* unimplemented */

typedef struct VSTREAM _deliver_vstream_;
extern DELIVER_REQUEST *deliver_request_read(_deliver_vstream_ *);
extern int deliver_request_done(_deliver_vstream_ *, DELIVER_REQUEST *, int);

extern int deliver_pass(const char *, const char *, DELIVER_REQUEST *, const char *, long);

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
