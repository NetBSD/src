/*++
/* NAME
/*	events 3
/* SUMMARY
/*	event manager
/* SYNOPSIS
/*	#include <events.h>
/*
/*	time_t	event_time()
/*
/*	void	event_loop(delay)
/*	int	delay;
/*
/*	time_t	event_request_timer(callback, context, delay)
/*	void	(*callback)(int event, char *context);
/*	char	*context;
/*	int	delay;
/*
/*	int	event_cancel_timer(callback, context)
/*	void	(*callback)(char *context);
/*	char	*context;
/*
/*	void	event_enable_read(fd, callback, context)
/*	int	fd;
/*	void	(*callback)(int event, char *context);
/*	char	*context;
/*
/*	void	event_enable_write(fd, callback, context)
/*	int	fd;
/*	void	(*callback)(int event, char *context);
/*	char	*context;
/*
/*	void	event_disable_readwrite(fd)
/*	int	fd;
/* DESCRIPTION
/*	This module delivers I/O and timer events.
/*	Multiple I/O streams and timers can be monitored simultaneously.
/*	Events are delivered via callback routines provided by the
/*	application. When requesting an event, the application can provide
/*	private context that is passed back when the callback routine is
/*	executed.
/*
/*	event_time() returns a cached value of the current time.
/*
/*	event_loop() monitors all I/O channels for which the application has
/*	expressed interest, and monitors the timer request queue.
/*	It notifies the application whenever events of interest happen.
/*	A negative delay value causes the function to pause until something
/*	happens; a positive delay value causes event_loop() to return when
/*	the next event happens or when the delay time in seconds is over,
/*	whatever happens first. A zero delay effectuates a poll.
/*
/*	Note: in order to avoid race conditions, event_loop() cannot
/*	not be called recursively.
/*
/*	event_request_timer() causes the specified callback function to
/*	be called with the specified context argument after \fIdelay\fR
/*	seconds, or as soon as possible thereafter. The delay should
/*	not be negative.
/*	The event argument is equal to EVENT_TIME.
/*	Only one timer request can be active per (callback, context) pair.
/*	Calling event_request_timer() with an existing (callback, context)
/*	pair does not schedule a new event, but updates the moment of
/*	delivery. The result is the absolute time at which the timer is
/*	scheduled to go off.
/*
/*	event_cancel_timer() cancels the specified (callback, context) request.
/*	The application is allowed to cancel non-existing requests. The result
/*	value is the amount of time left before the timer would have gone off,
/*	or -1 in case of no pending timer.
/*
/*	event_enable_read() (event_enable_write()) enables read (write) events
/*	on the named I/O channel. It is up to the application to assemble
/*	partial reads or writes.
/*	An I/O channel cannot handle more than one request at the
/*	same time. The application is allowed to enable an event that
/*	is already enabled (same channel, callback and context).
/*
/*	The manifest constants EVENT_NULL_CONTEXT and EVENT_NULL_TYPE
/*	provide convenient null values.
/*
/*	The callback routine has the following arguments:
/* .IP fd
/*	The stream on which the event happened.
/* .IP event
/*	An indication of the event type:
/* .RS
/* .IP EVENT_READ
/*	read event,
/* .IP EVENT_WRITE
/*	write event,
/* .IP EVENT_XCPT
/*	exception.
/* .RE
/* .IP context
/*	Application context given to event_enable_read() (event_enable_write()).
/* .PP
/*	event_disable_readwrite() disables further I/O events on the specified
/*	I/O channel. The application is allowed to cancel non-existing
/*	I/O event requests.
/* DIAGNOSTICS
/*	Panics: interface violations. Fatal errors: out of memory,
/*	system call failure. Warnings: the number of available
/*	file descriptors is much less than FD_SETSIZE.
/* BUGS
/*	This module is based on event selection. It assumes that the
/*	event_loop() routine is called frequently. This approach is
/*	not suitable for applications with compute-bound loops that
/*	take a significant amount of time.
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

/* System libraries. */

#include "sys_defs.h"
#include <sys/time.h>			/* XXX: 44BSD uses bzero() */
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>			/* offsetof() */
#include <string.h>			/* bzero() prototype for 44BSD */

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Application-specific. */

#include "mymalloc.h"
#include "msg.h"
#include "iostuff.h"
#include "ring.h"
#include "events.h"

 /*
  * I/O events. We pre-allocate one data structure per file descriptor. XXX
  * For now use FD_SETSIZE as defined along with the fd-set type.
  */
typedef struct EVENT_FDTABLE EVENT_FDTABLE;

struct EVENT_FDTABLE {
    EVENT_NOTIFY_RDWR callback;
    char   *context;
};
static fd_set event_rmask;		/* enabled read events */
static fd_set event_wmask;		/* enabled write events */
static fd_set event_xmask;		/* for bad news mostly */
static int event_fdsize;		/* number of file descriptors */
static EVENT_FDTABLE *event_fdtable;	/* one slot per file descriptor */
static int event_max_fd;		/* highest fd number seen */

 /*
  * Timer events. Timer requests are kept sorted, in a circular list. We use
  * the RING abstraction, so we get to use a couple ugly macros.
  */
typedef struct EVENT_TIMER EVENT_TIMER;

struct EVENT_TIMER {
    time_t  when;			/* when event is wanted */
    EVENT_NOTIFY_TIME callback;		/* callback function */
    char   *context;			/* callback context */
    RING    ring;			/* linkage */
};

static RING event_timer_head;		/* timer queue head */

#define RING_TO_TIMER(r) \
	((EVENT_TIMER *) ((char *) (r) - offsetof(EVENT_TIMER, ring)))

#define FOREACH_QUEUE_ENTRY(entry, head) \
	for (entry = ring_succ(head); entry != (head); entry = ring_succ(entry))

#define FIRST_TIMER(head) \
	(ring_succ(head) != (head) ? RING_TO_TIMER(ring_succ(head)) : 0)

 /*
  * Other private data structures.
  */
static time_t event_present;		/* cached time of day */

#define EVENT_INIT_NEEDED()	(event_present == 0)

/* event_init - set up tables and such */

static void event_init(void)
{
    EVENT_FDTABLE *fdp;

    if (!EVENT_INIT_NEEDED())
	msg_panic("event_init: repeated call");

    /*
     * Initialize the file descriptor table. XXX It should be possible to
     * adjust (or at least extend) the table size on the fly.
     */
    if ((event_fdsize = open_limit(FD_SETSIZE)) < 0)
	msg_fatal("unable to determine open file limit");
    if (event_fdsize < FD_SETSIZE / 2 && event_fdsize < 256)
	msg_warn("could allocate space for only %d open files", event_fdsize);
    event_fdtable = (EVENT_FDTABLE *)
	mymalloc(sizeof(EVENT_FDTABLE) * event_fdsize);
    for (fdp = event_fdtable; fdp < event_fdtable + event_fdsize; fdp++) {
	fdp->callback = 0;
	fdp->context = 0;
    }

    /*
     * Initialize the I/O event request masks.
     */
    FD_ZERO(&event_rmask);
    FD_ZERO(&event_wmask);
    FD_ZERO(&event_xmask);

    /*
     * Initialize timer stuff.
     */
    ring_init(&event_timer_head);
    (void) time(&event_present);

    /*
     * Avoid an infinite initialization loop.
     */
    if (EVENT_INIT_NEEDED())
	msg_panic("event_init: unable to initialize");
}

/* event_time - look up cached time of day */

time_t  event_time(void)
{
    if (EVENT_INIT_NEEDED())
	event_init();

    return (event_present);
}

/* event_enable_read - enable read events */

void    event_enable_read(int fd, EVENT_NOTIFY_RDWR callback, char *context)
{
    char   *myname = "event_enable_read";
    EVENT_FDTABLE *fdp;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (fd < 0 || fd >= event_fdsize)
	msg_panic("%s: bad file descriptor: %d", myname, fd);

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);

    /*
     * Disallow multiple requests on the same file descriptor. Allow
     * duplicates of the same request.
     */
    fdp = event_fdtable + fd;
    if (FD_ISSET(fd, &event_xmask)) {
	if (FD_ISSET(fd, &event_rmask)
	    && fdp->callback == callback
	    && fdp->context == context)
	    return;
	msg_panic("%s: fd %d: multiple I/O request", myname, fd);
    }
    FD_SET(fd, &event_xmask);
    FD_SET(fd, &event_rmask);
    fdp->callback = callback;
    fdp->context = context;
    if (event_max_fd < fd)
	event_max_fd = fd;
}

/* event_enable_write - enable write events */

void    event_enable_write(int fd, EVENT_NOTIFY_RDWR callback, char *context)
{
    char   *myname = "event_enable_write";
    EVENT_FDTABLE *fdp;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (fd < 0 || fd >= event_fdsize)
	msg_panic("%s: bad file descriptor: %d", myname, fd);

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);

    /*
     * Disallow multiple requests on the same file descriptor. Allow
     * duplicates of the same request.
     */
    fdp = event_fdtable + fd;
    if (FD_ISSET(fd, &event_xmask)) {
	if (FD_ISSET(fd, &event_wmask)
	    && fdp->callback == callback
	    && fdp->context == context)
	    return;
	msg_panic("%s: fd %d: multiple I/O request", myname, fd);
    }
    FD_SET(fd, &event_xmask);
    FD_SET(fd, &event_wmask);
    fdp->callback = callback;
    fdp->context = context;
    if (event_max_fd < fd)
	event_max_fd = fd;
}

/* event_disable_readwrite - disable request for read or write events */

void    event_disable_readwrite(int fd)
{
    char   *myname = "event_disable_readwrite";
    EVENT_FDTABLE *fdp;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (fd < 0 || fd >= event_fdsize)
	msg_panic("%s: bad file descriptor: %d", myname, fd);

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);

    /*
     * Don't complain when there is nothing to cancel. The request may have
     * been canceled from another thread.
     */
    FD_CLR(fd, &event_xmask);
    FD_CLR(fd, &event_rmask);
    FD_CLR(fd, &event_wmask);
    fdp = event_fdtable + fd;
    fdp->callback = 0;
    fdp->context = 0;
}

/* event_request_timer - (re)set timer */

time_t  event_request_timer(EVENT_NOTIFY_TIME callback, char *context, int delay)
{
    char   *myname = "event_request_timer";
    RING   *ring;
    EVENT_TIMER *timer;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (delay < 0)
	msg_panic("%s: invalid delay: %d", myname, delay);

    /*
     * Make sure we schedule this event at the right time.
     */
    time(&event_present);

    /*
     * See if they are resetting an existing timer request. If so, take the
     * request away from the timer queue so that it can be inserted at the
     * right place.
     */
    FOREACH_QUEUE_ENTRY(ring, &event_timer_head) {
	timer = RING_TO_TIMER(ring);
	if (timer->callback == callback && timer->context == context) {
	    timer->when = event_present + delay;
	    ring_detach(ring);
	    if (msg_verbose > 2)
		msg_info("%s: reset 0x%lx 0x%lx %d", myname,
			 (long) callback, (long) context, delay);
	    break;
	}
    }

    /*
     * If not found, schedule a new timer request.
     */
    if (ring == &event_timer_head) {
	timer = (EVENT_TIMER *) mymalloc(sizeof(EVENT_TIMER));
	timer->when = event_present + delay;
	timer->callback = callback;
	timer->context = context;
	if (msg_verbose > 2)
	    msg_info("%s: set 0x%lx 0x%lx %d", myname,
		     (long) callback, (long) context, delay);
    }

    /*
     * Insert the request at the right place. Timer requests are kept sorted
     * to reduce lookup overhead in the event loop.
     */
    FOREACH_QUEUE_ENTRY(ring, &event_timer_head)
	if (timer->when < RING_TO_TIMER(ring)->when)
	break;
    ring_prepend(ring, &timer->ring);

    return (timer->when);
}

/* event_cancel_timer - cancel timer */

int     event_cancel_timer(EVENT_NOTIFY_TIME callback, char *context)
{
    char   *myname = "event_cancel_timer";
    RING   *ring;
    EVENT_TIMER *timer;
    int     time_left = -1;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * See if they are canceling an existing timer request. Do not complain
     * when the request is not found. It might have been canceled from some
     * other thread.
     */
    FOREACH_QUEUE_ENTRY(ring, &event_timer_head) {
	timer = RING_TO_TIMER(ring);
	if (timer->callback == callback && timer->context == context) {
	    if ((time_left = timer->when - event_present) < 0)
		time_left = 0;
	    ring_detach(ring);
	    myfree((char *) timer);
	    break;
	}
    }
    if (msg_verbose > 2)
	msg_info("%s: 0x%lx 0x%lx %d", myname,
		 (long) callback, (long) context, time_left);
    return (time_left);
}

/* event_loop - wait for the next event */

void    event_loop(int delay)
{
    char   *myname = "event_loop";
    static int nested;
    fd_set  rmask;
    fd_set  wmask;
    fd_set  xmask;
    struct timeval tv;
    struct timeval *tvp;
    EVENT_TIMER *timer;
    int     fd;
    EVENT_FDTABLE *fdp;
    int     select_delay;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Find out when the next timer would go off. Timer requests are sorted.
     * If any timer is scheduled, adjust the delay appropriately.
     */
    if ((timer = FIRST_TIMER(&event_timer_head)) != 0) {
	event_present = time((time_t *) 0);
	if ((select_delay = timer->when - event_present) < 0) {
	    select_delay = 0;
	} else if (delay >= 0 && select_delay > delay) {
	    select_delay = delay;
	}
    } else {
	select_delay = delay;
    }
    if (msg_verbose > 2)
	msg_info("event_loop: select_delay %d", select_delay);

    /*
     * Negative delay means: wait until something happens. Zero delay means:
     * poll. Positive delay means: wait at most this long.
     */
    if (select_delay < 0) {
	tvp = 0;
    } else {
	tvp = &tv;
	tv.tv_usec = 0;
	tv.tv_sec = select_delay;
    }

    /*
     * Pause until the next event happens. When select() has a problem, don't
     * go into a tight loop. Allow select() to be interrupted due to the
     * arrival of a signal.
     */
    rmask = event_rmask;
    wmask = event_wmask;
    xmask = event_xmask;

    if (select(event_max_fd + 1, &rmask, &wmask, &xmask, tvp) < 0) {
	if (errno != EINTR)
	    msg_fatal("event_loop: select: %m");
	return;
    }

    /*
     * Before entering the application call-back routines, make sure we
     * aren't being called from a call-back routine. Doing so would make us
     * vulnerable to all kinds of race conditions.
     */
    if (nested++ > 0)
	msg_panic("event_loop: recursive call");

    /*
     * Deliver timer events. Requests are sorted: we can stop when we reach
     * the future or the list end. Allow the application to update the timer
     * queue while it is being called back. To this end, we repeatedly pop
     * the first request off the timer queue before delivering the event to
     * the application.
     */
    event_present = time((time_t *) 0);

    while ((timer = FIRST_TIMER(&event_timer_head)) != 0) {
	if (timer->when > event_present)
	    break;
	ring_detach(&timer->ring);		/* first this */
	if (msg_verbose > 2)
	    msg_info("%s: timer 0x%lx 0x%lx", myname,
		     (long) timer->callback, (long) timer->context);
	timer->callback(EVENT_TIME, timer->context);	/* then this */
	myfree((char *) timer);
    }

    /*
     * Deliver I/O events. Allow the application to cancel event requests
     * while it is being called back. To this end, we keep an eye on the
     * contents of event_xmask, so that we deliver only events that are still
     * wanted. We do not change the event request masks. It is up to the
     * application to determine when a read or write is complete.
     */
    for (fd = 0, fdp = event_fdtable; fd <= event_max_fd; fd++, fdp++) {
	if (FD_ISSET(fd, &event_xmask)) {
	    if (FD_ISSET(fd, &xmask)) {
		if (msg_verbose > 2)
		    msg_info("%s: exception %d 0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		fdp->callback(EVENT_XCPT, fdp->context);
	    } else if (FD_ISSET(fd, &wmask)) {
		if (msg_verbose > 2)
		    msg_info("%s: write %d 0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		fdp->callback(EVENT_WRITE, fdp->context);
	    } else if (FD_ISSET(fd, &rmask)) {
		if (msg_verbose > 2)
		    msg_info("%s: read %d 0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		fdp->callback(EVENT_READ, fdp->context);
	    }
	}
    }
    nested--;
}

#ifdef TEST

 /*
  * Proof-of-concept test program for the event manager. Print "dingdong"
  * every 5 seconds, while echoing any lines read from stdin. The "dingdong
  * changes case each time it is printed.
  */
#include <stdio.h>
#include <ctype.h>

#define DELAY 5

/* dingdong - print text every DELAY seconds */

static void dingdong(char *context)
{
    printf("%c", *context);
    fflush(stdout);
    *context = (ISUPPER(*context) ? TOLOWER(*context) : TOUPPER(*context));
    event_request_timer(dingdong, context, (char *) DELAY);
}

/* echo - echo text received on stdin */

static void echo(int unused_event, char *unused_context)
{
    char    buf[BUFSIZ];

    if (fgets(buf, sizeof(buf), stdin) == 0)
	exit(0);
    printf("Result: %s", buf);
}

main(void)
{
    static char text[] = "\rdingdong ";
    char   *cp;

    for (cp = text; *cp; cp++)
	event_request_timer(dingdong, cp, (char *) 0);
    event_enable_read(fileno(stdin), echo, (char *) 0);
    for (;;)
	event_loop(-1);
}

#endif
