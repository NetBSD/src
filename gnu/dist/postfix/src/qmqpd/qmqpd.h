/*++
/* NAME
/*	qmqpd 3h
/* SUMMARY
/*	Postfix QMQP server
/* SYNOPSIS
/*	include "qmqpd.h"
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_stream.h>

 /*
  * Per-session state.
  */
typedef struct {
    int     err;			/* error flags */
    VSTREAM *client;			/* client connection */
    VSTRING *message;			/* message buffer */
    VSTRING *buf;			/* line buffer */
    time_t  time;			/* start of session */
    char   *name;			/* client name */
    char   *addr;			/* client IP address */
    char   *namaddr;			/* name[addr] */
    char   *queue_id;			/* queue file ID */
    VSTREAM *cleanup;			/* cleanup server */
    MAIL_STREAM *dest;			/* cleanup server */
    int     rcpt_count;			/* recipient count */
    char   *reason;			/* exception name */
    char   *sender;			/* sender address */
    char   *recipient;			/* recipient address */
    char   *protocol;			/* protocol name */
    char   *where;			/* protocol state */
    VSTRING *why_rejected;		/* REJECT reason */
} QMQPD_STATE;

 /*
  * QMQP protocol status codes.
  */
#define QMQPD_STAT_OK		'K'
#define QMQPD_STAT_RETRY	'Z'
#define QMQPD_STAT_HARD		'D'

 /*
  * qmqpd_state.c
  */
QMQPD_STATE *qmqpd_state_alloc(VSTREAM *);
void    qmqpd_state_free(QMQPD_STATE *);

 /*
  * qmqpd_peer.c
  */
void    qmqpd_peer_init(QMQPD_STATE *);
void    qmqpd_peer_reset(QMQPD_STATE *);

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
