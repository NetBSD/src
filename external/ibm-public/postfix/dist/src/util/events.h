/*	$NetBSD: events.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _EVENTS_H_INCLUDED_
#define _EVENTS_H_INCLUDED_

/*++
/* NAME
/*	events 3h
/* SUMMARY
/*	event manager
/* SYNOPSIS
/*	#include <events.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>

 /*
  * External interface.
  */
typedef void (*EVENT_NOTIFY_RDWR) (int, char *);
typedef void (*EVENT_NOTIFY_TIME) (int, char *);

extern time_t event_time(void);
extern void event_enable_read(int, EVENT_NOTIFY_RDWR, char *);
extern void event_enable_write(int, EVENT_NOTIFY_RDWR, char *);
extern void event_disable_readwrite(int);
extern time_t event_request_timer(EVENT_NOTIFY_TIME, char *, int);
extern int event_cancel_timer(EVENT_NOTIFY_TIME, char *);
extern void event_loop(int);
extern void event_drain(int);

 /*
  * Event codes.
  */
#define EVENT_READ	(1<<0)		/* read event */
#define EVENT_WRITE	(1<<1)		/* write event */
#define EVENT_XCPT	(1<<2)		/* exception */
#define EVENT_TIME	(1<<3)		/* timer event */

#define EVENT_ERROR	EVENT_XCPT

 /*
  * Dummies.
  */
#define EVENT_NULL_TYPE	0
#define EVENT_NULL_CONTEXT	((char *) 0)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/* CREATION DATE
/*	Wed Jan 29 17:00:03 EST 1997
/*--*/

#endif
