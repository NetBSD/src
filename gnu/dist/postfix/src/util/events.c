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
/*	void	(*callback)(int event, char *context);
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
/*
/*	void	event_drain(time_limit)
/*	int	time_limit;
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
/*	pair does not schedule a new event, but updates the time of event
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
/*	is already enabled (same channel, same read or write operation,
/*	but perhaps a different callback or context). On systems with
/*	kernel-based event filters this is preferred usage, because
/*	each disable and enable request would cost a system call.
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
/*	exception (actually, any event other than read or write).
/* .RE
/* .IP context
/*	Application context given to event_enable_read() (event_enable_write()).
/* .PP
/*	event_disable_readwrite() disables further I/O events on the specified
/*	I/O channel. The application is allowed to cancel non-existing
/*	I/O event requests.
/*
/*	event_drain() repeatedly calls event_loop() until no more timer
/*	events or I/O events are pending or until the time limit is reached.
/*	This routine must not be called from an event_whatever() callback
/*	routine. Note: this function ignores pending timer events, and
/*	assumes that no new I/O events will be registered.
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
#include <limits.h>			/* INT_MAX */

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Application-specific. */

#include "mymalloc.h"
#include "msg.h"
#include "iostuff.h"
#include "ring.h"
#include "events.h"

#if !defined(EVENTS_STYLE)
#error "must define EVENTS_STYLE"
#endif

 /*
  * Traditional BSD-style select(2). Works everywhere, but has a built-in
  * upper bound on the number of file descriptors, and that limit is hard to
  * change on Linux. Is sometimes emulated with SYSV-style poll(2) which
  * doesn't have the file descriptor limit, but unfortunately does not help
  * to improve the performance of servers with lots of connections.
  */
#define EVENT_ALLOC_INCR		10

#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
typedef fd_set EVENT_MASK;

#define EVENT_MASK_BYTE_COUNT(mask)	sizeof(*(mask))
#define EVENT_MASK_ZERO(mask)		FD_ZERO(mask)
#define EVENT_MASK_SET(fd, mask)	FD_SET((fd), (mask))
#define EVENT_MASK_ISSET(fd, mask)	FD_ISSET((fd), (mask))
#define EVENT_MASK_CLR(fd, mask)	FD_CLR((fd), (mask))
#else

 /*
  * Kernel-based event filters (kqueue, /dev/poll, epoll). We use the
  * following file descriptor mask structure which is expanded on the fly.
  */
typedef struct {
    char   *data;			/* bit mask */
    size_t  data_len;			/* data byte count */
} EVENT_MASK;

 /* Bits per byte, byte in vector, bit offset in byte, bytes per set. */
#define EVENT_MASK_NBBY		(8)
#define EVENT_MASK_FD_BYTE(fd, mask) \
	(((unsigned char *) (mask)->data)[(fd) / EVENT_MASK_NBBY])
#define EVENT_MASK_FD_BIT(fd)	(1 << ((fd) % EVENT_MASK_NBBY))
#define EVENT_MASK_BYTES_NEEDED(len) \
	(((len) + (EVENT_MASK_NBBY -1)) / EVENT_MASK_NBBY)
#define EVENT_MASK_BYTE_COUNT(mask)	((mask)->data_len)

 /* Memory management. */
#define EVENT_MASK_ALLOC(mask, bit_len) do { \
	size_t _byte_len = EVENT_MASK_BYTES_NEEDED(bit_len); \
	(mask)->data = mymalloc(_byte_len); \
	memset((mask)->data, 0, _byte_len); \
	(mask)->data_len = _byte_len; \
    } while (0)
#define EVENT_MASK_REALLOC(mask, bit_len) do { \
	size_t _byte_len = EVENT_MASK_BYTES_NEEDED(bit_len); \
	size_t _old_len = (mask)->data_len; \
	(mask)->data = myrealloc((mask)->data, _byte_len); \
	memset((mask)->data + _old_len, 0, _byte_len - _old_len); \
	(mask)->data_len = _byte_len; \
    } while (0)
#define EVENT_MASK_FREE(mask)	myfree((mask)->data)

 /* Set operations, modeled after FD_ZERO/SET/ISSET/CLR. */
#define EVENT_MASK_ZERO(mask) \
	memset((mask)->data, 0, (mask)->data_len)
#define EVENT_MASK_SET(fd, mask) \
	(EVENT_MASK_FD_BYTE((fd), (mask)) |= EVENT_MASK_FD_BIT(fd))
#define EVENT_MASK_ISSET(fd, mask) \
	(EVENT_MASK_FD_BYTE((fd), (mask)) & EVENT_MASK_FD_BIT(fd))
#define EVENT_MASK_CLR(fd, mask) \
	(EVENT_MASK_FD_BYTE((fd), (mask)) &= ~EVENT_MASK_FD_BIT(fd))
#endif

 /*
  * I/O events.
  */
typedef struct EVENT_FDTABLE EVENT_FDTABLE;

struct EVENT_FDTABLE {
    EVENT_NOTIFY_RDWR callback;
    char   *context;
};
static EVENT_MASK event_rmask;		/* enabled read events */
static EVENT_MASK event_wmask;		/* enabled write events */
static EVENT_MASK event_xmask;		/* for bad news mostly */
static int event_fdlimit;		/* per-process open file limit */
static EVENT_FDTABLE *event_fdtable;	/* one slot per file descriptor */
static int event_fdslots;		/* number of file descriptor slots */
static int event_max_fd = -1;		/* highest fd number seen */

 /*
  * FreeBSD kqueue supports no system call to find out what descriptors are
  * registered in the kernel-based filter. To implement our own sanity checks
  * we maintain our own descriptor bitmask.
  * 
  * FreeBSD kqueue does support application context pointers. Unfortunately,
  * changing that information would cost a system call, and some of the
  * competitors don't support application context. To keep the implementation
  * simple we maintain our own table with call-back information.
  * 
  * FreeBSD kqueue silently unregisters a descriptor from its filter when the
  * descriptor is closed, so our information could get out of sync with the
  * kernel. But that will never happen, because we have to meticulously
  * unregister a file descriptor before it is closed, to avoid errors on
  * systems that are built with EVENTS_STYLE == EVENTS_STYLE_SELECT.
  */
#if (EVENTS_STYLE == EVENTS_STYLE_KQUEUE)
#include <sys/event.h>

 /*
  * Some early FreeBSD implementations don't have the EV_SET macro.
  */
#ifndef EV_SET
#define EV_SET(kp, id, fi, fl, ffl, da, ud) do { \
        (kp)->ident = (id); \
        (kp)->filter = (fi); \
        (kp)->flags = (fl); \
        (kp)->fflags = (ffl); \
        (kp)->data = (da); \
        (kp)->udata = (ud); \
    } while(0)
#endif

 /*
  * Macros to initialize the kernel-based filter; see event_init().
  */
static int event_kq;			/* handle to event filter */

#define EVENT_REG_INIT_HANDLE(er, n) do { \
	er = event_kq = kqueue(); \
    } while (0)
#define EVENT_REG_INIT_TEXT	"kqueue"

 /*
  * Macros to update the kernel-based filter; see event_enable_read(),
  * event_enable_write() and event_disable_readwrite().
  */
#define EVENT_REG_FD_OP(er, fh, ev, op) do { \
	struct kevent dummy; \
	EV_SET(&dummy, (fh), (ev), (op), 0, 0, 0); \
	(er) = kevent(event_kq, &dummy, 1, 0, 0, 0); \
    } while (0)

#define EVENT_REG_ADD_OP(e, f, ev) EVENT_REG_FD_OP((e), (f), (ev), EV_ADD)
#define EVENT_REG_ADD_READ(e, f)   EVENT_REG_ADD_OP((e), (f), EVFILT_READ)
#define EVENT_REG_ADD_WRITE(e, f)  EVENT_REG_ADD_OP((e), (f), EVFILT_WRITE)
#define EVENT_REG_ADD_TEXT         "kevent EV_ADD"

#define EVENT_REG_DEL_OP(e, f, ev) EVENT_REG_FD_OP((e), (f), (ev), EV_DELETE)
#define EVENT_REG_DEL_READ(e, f)   EVENT_REG_DEL_OP((e), (f), EVFILT_READ)
#define EVENT_REG_DEL_WRITE(e, f)  EVENT_REG_DEL_OP((e), (f), EVFILT_WRITE)
#define EVENT_REG_DEL_TEXT         "kevent EV_DELETE"

 /*
  * Macros to retrieve event buffers from the kernel; see event_loop().
  */
typedef struct kevent EVENT_BUFFER;

#define EVENT_BUFFER_READ(event_count, event_buf, buflen, delay) do { \
	struct timespec ts; \
	struct timespec *tsp; \
	if ((delay) < 0) { \
	    tsp = 0; \
	} else { \
	    tsp = &ts; \
	    ts.tv_nsec = 0; \
	    ts.tv_sec = (delay); \
	} \
	(event_count) = kevent(event_kq, (struct kevent *) 0, 0, (event_buf), \
			  (buflen), (tsp)); \
    } while (0)
#define EVENT_BUFFER_READ_TEXT	"kevent"

 /*
  * Macros to process event buffers from the kernel; see event_loop().
  */
#define EVENT_GET_FD(bp)	((bp)->ident)
#define EVENT_GET_TYPE(bp)	((bp)->filter)
#define EVENT_TEST_READ(bp)	(EVENT_GET_TYPE(bp) == EVFILT_READ)
#define EVENT_TEST_WRITE(bp)	(EVENT_GET_TYPE(bp) == EVFILT_WRITE)

#endif

 /*
  * Solaris /dev/poll does not support application context, so we have to
  * maintain our own. This has the benefit of avoiding an expensive system
  * call just to change a call-back function or argument.
  * 
  * Solaris /dev/poll does have a way to query if a specific descriptor is
  * registered. However, we maintain a descriptor mask anyway because a) it
  * avoids having to make an expensive system call to find out if something
  * is registered, b) some EVENTS_STYLE_MUMBLE implementations need a
  * descriptor bitmask anyway and c) we use the bitmask already to implement
  * sanity checks.
  */
#if (EVENTS_STYLE == EVENTS_STYLE_DEVPOLL)
#include <sys/devpoll.h>
#include <fcntl.h>

 /*
  * Macros to initialize the kernel-based filter; see event_init().
  */
static int event_pollfd;		/* handle to file descriptor set */

#define EVENT_REG_INIT_HANDLE(er, n) do { \
	er = event_pollfd = open("/dev/poll", O_RDWR); \
	if (event_pollfd >= 0) close_on_exec(event_pollfd, CLOSE_ON_EXEC); \
    } while (0)
#define EVENT_REG_INIT_TEXT	"open /dev/poll"

 /*
  * Macros to update the kernel-based filter; see event_enable_read(),
  * event_enable_write() and event_disable_readwrite().
  */
#define EVENT_REG_FD_OP(er, fh, ev) do { \
	struct pollfd dummy; \
	dummy.fd = (fh); \
	dummy.events = (ev); \
	(er) = write(event_pollfd, (char *) &dummy, \
	    sizeof(dummy)) != sizeof(dummy) ? -1 : 0; \
    } while (0)

#define EVENT_REG_ADD_READ(e, f)  EVENT_REG_FD_OP((e), (f), POLLIN)
#define EVENT_REG_ADD_WRITE(e, f) EVENT_REG_FD_OP((e), (f), POLLOUT)
#define EVENT_REG_ADD_TEXT        "write /dev/poll"

#define EVENT_REG_DEL_BOTH(e, f)  EVENT_REG_FD_OP((e), (f), POLLREMOVE)
#define EVENT_REG_DEL_TEXT        "write /dev/poll"

 /*
  * Macros to retrieve event buffers from the kernel; see event_loop().
  */
typedef struct pollfd EVENT_BUFFER;

#define EVENT_BUFFER_READ(event_count, event_buf, buflen, delay) do { \
	struct dvpoll dvpoll; \
	dvpoll.dp_fds = (event_buf); \
	dvpoll.dp_nfds = (buflen); \
	dvpoll.dp_timeout = (delay) < 0 ? -1 : (delay) * 1000; \
	(event_count) = ioctl(event_pollfd, DP_POLL, &dvpoll); \
    } while (0)
#define EVENT_BUFFER_READ_TEXT	"ioctl DP_POLL"

 /*
  * Macros to process event buffers from the kernel; see event_loop().
  */
#define EVENT_GET_FD(bp)	((bp)->fd)
#define EVENT_GET_TYPE(bp)	((bp)->revents)
#define EVENT_TEST_READ(bp)	(EVENT_GET_TYPE(bp) & POLLIN)
#define EVENT_TEST_WRITE(bp)	(EVENT_GET_TYPE(bp) & POLLOUT)

#endif

 /*
  * Linux epoll supports no system call to find out what descriptors are
  * registered in the kernel-based filter. To implement our own sanity checks
  * we maintain our own descriptor bitmask.
  * 
  * Linux epoll does support application context pointers. Unfortunately,
  * changing that information would cost a system call, and some of the
  * competitors don't support application context. To keep the implementation
  * simple we maintain our own table with call-back information.
  * 
  * Linux epoll silently unregisters a descriptor from its filter when the
  * descriptor is closed, so our information could get out of sync with the
  * kernel. But that will never happen, because we have to meticulously
  * unregister a file descriptor before it is closed, to avoid errors on
  */
#if (EVENTS_STYLE == EVENTS_STYLE_EPOLL)
#include <sys/epoll.h>

 /*
  * Macros to initialize the kernel-based filter; see event_init().
  */
static int event_epollfd;		/* epoll handle */

#define EVENT_REG_INIT_HANDLE(er, n) do { \
	er = event_epollfd = epoll_create(n); \
	if (event_epollfd >= 0) close_on_exec(event_epollfd, CLOSE_ON_EXEC); \
    } while (0)
#define EVENT_REG_INIT_TEXT	"epoll_create"

 /*
  * Macros to update the kernel-based filter; see event_enable_read(),
  * event_enable_write() and event_disable_readwrite().
  */
#define EVENT_REG_FD_OP(er, fh, ev, op) do { \
	struct epoll_event dummy; \
	dummy.events = (ev); \
	dummy.data.fd = (fh); \
	(er) = epoll_ctl(event_epollfd, (op), (fh), &dummy); \
    } while (0)

#define EVENT_REG_ADD_OP(e, f, ev) EVENT_REG_FD_OP((e), (f), (ev), EPOLL_CTL_ADD)
#define EVENT_REG_ADD_READ(e, f)   EVENT_REG_ADD_OP((e), (f), EPOLLIN)
#define EVENT_REG_ADD_WRITE(e, f)  EVENT_REG_ADD_OP((e), (f), EPOLLOUT)
#define EVENT_REG_ADD_TEXT         "epoll_ctl EPOLL_CTL_ADD"

#define EVENT_REG_DEL_OP(e, f, ev) EVENT_REG_FD_OP((e), (f), (ev), EPOLL_CTL_DEL)
#define EVENT_REG_DEL_READ(e, f)   EVENT_REG_DEL_OP((e), (f), EPOLLIN)
#define EVENT_REG_DEL_WRITE(e, f)  EVENT_REG_DEL_OP((e), (f), EPOLLOUT)
#define EVENT_REG_DEL_TEXT         "epoll_ctl EPOLL_CTL_DEL"

 /*
  * Macros to retrieve event buffers from the kernel; see event_loop().
  */
typedef struct epoll_event EVENT_BUFFER;

#define EVENT_BUFFER_READ(event_count, event_buf, buflen, delay) do { \
	(event_count) = epoll_wait(event_epollfd, (event_buf), (buflen), \
				  (delay) < 0 ? -1 : (delay) * 1000); \
    } while (0)
#define EVENT_BUFFER_READ_TEXT	"epoll_wait"

 /*
  * Macros to process event buffers from the kernel; see event_loop().
  */
#define EVENT_GET_FD(bp)	((bp)->data.fd)
#define EVENT_GET_TYPE(bp)	((bp)->events)
#define EVENT_TEST_READ(bp)	(EVENT_GET_TYPE(bp) & EPOLLIN)
#define EVENT_TEST_WRITE(bp)	(EVENT_GET_TYPE(bp) & EPOLLOUT)

#endif

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
    int     err;

    if (!EVENT_INIT_NEEDED())
	msg_panic("event_init: repeated call");

    /*
     * Initialize the file descriptor masks and the call-back table. Where
     * possible we extend these data structures on the fly. With select(2)
     * based implementations we can only handle FD_SETSIZE open files.
     */
#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
    if ((event_fdlimit = open_limit(FD_SETSIZE)) < 0)
	msg_fatal("unable to determine open file limit");
#else
    if ((event_fdlimit = open_limit(INT_MAX)) < 0)
	msg_fatal("unable to determine open file limit");
#endif
    if (event_fdlimit < FD_SETSIZE / 2 && event_fdlimit < 256)
	msg_warn("could allocate space for only %d open files", event_fdlimit);
    event_fdslots = EVENT_ALLOC_INCR;
    event_fdtable = (EVENT_FDTABLE *)
	mymalloc(sizeof(EVENT_FDTABLE) * event_fdslots);
    for (fdp = event_fdtable; fdp < event_fdtable + event_fdslots; fdp++) {
	fdp->callback = 0;
	fdp->context = 0;
    }

    /*
     * Initialize the I/O event request masks.
     */
#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
    EVENT_MASK_ZERO(&event_rmask);
    EVENT_MASK_ZERO(&event_wmask);
    EVENT_MASK_ZERO(&event_xmask);
#else
    EVENT_MASK_ALLOC(&event_rmask, event_fdslots);
    EVENT_MASK_ALLOC(&event_wmask, event_fdslots);
    EVENT_MASK_ALLOC(&event_xmask, event_fdslots);

    /*
     * Initialize the kernel-based filter.
     */
    EVENT_REG_INIT_HANDLE(err, event_fdslots);
    if (err < 0)
	msg_fatal("%s: %m", EVENT_REG_INIT_TEXT);
#endif

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

/* event_extend - make room for more descriptor slots */

static void event_extend(int fd)
{
    const char *myname = "event_extend";
    int     old_slots = event_fdslots;
    int     new_slots = (event_fdslots > fd / 2 ?
			 2 * old_slots : fd + EVENT_ALLOC_INCR);
    EVENT_FDTABLE *fdp;
    int     err;

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);
    event_fdtable = (EVENT_FDTABLE *)
	myrealloc((char *) event_fdtable, sizeof(EVENT_FDTABLE) * new_slots);
    event_fdslots = new_slots;
    for (fdp = event_fdtable + old_slots;
	 fdp < event_fdtable + new_slots; fdp++) {
	fdp->callback = 0;
	fdp->context = 0;
    }

    /*
     * Initialize the I/O event request masks.
     */
#if (EVENTS_STYLE != EVENTS_STYLE_SELECT)
    EVENT_MASK_REALLOC(&event_rmask, new_slots);
    EVENT_MASK_REALLOC(&event_wmask, new_slots);
    EVENT_MASK_REALLOC(&event_xmask, new_slots);
#endif
#ifdef EVENT_REG_UPD_HANDLE
    EVENT_REG_UPD_HANDLE(err, new_slots);
    if (err < 0)
	msg_fatal("%s: %s: %m", myname, EVENT_REG_UPD_TEXT);
#endif
}

/* event_time - look up cached time of day */

time_t  event_time(void)
{
    if (EVENT_INIT_NEEDED())
	event_init();

    return (event_present);
}

/* event_drain - loop until all pending events are done */

void    event_drain(int time_limit)
{
    EVENT_MASK zero_mask;
    time_t  max_time;

    if (EVENT_INIT_NEEDED())
	return;

#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
    EVENT_MASK_ZERO(&zero_mask);
#else
    EVENT_MASK_ALLOC(&zero_mask, event_fdslots);
#endif
    (void) time(&event_present);
    max_time = event_present + time_limit;
    while (event_present < max_time
	   && (event_timer_head.pred != &event_timer_head
	       || memcmp(&zero_mask, &event_xmask,
			 EVENT_MASK_BYTE_COUNT(&zero_mask)) != 0))
	event_loop(1);
#if (EVENTS_STYLE != EVENTS_STYLE_SELECT)
    EVENT_MASK_FREE(&zero_mask);
#endif
}

/* event_enable_read - enable read events */

void    event_enable_read(int fd, EVENT_NOTIFY_RDWR callback, char *context)
{
    const char *myname = "event_enable_read";
    EVENT_FDTABLE *fdp;
    int     err;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (fd < 0 || fd >= event_fdlimit)
	msg_panic("%s: bad file descriptor: %d", myname, fd);

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);

    if (fd >= event_fdslots)
	event_extend(fd);

    /*
     * Disallow mixed (i.e. read and write) requests on the same descriptor.
     */
    if (EVENT_MASK_ISSET(fd, &event_wmask))
	msg_panic("%s: fd %d: read/write I/O request", myname, fd);

    /*
     * Postfix 2.4 allows multiple event_enable_read() calls on the same
     * descriptor without requiring event_disable_readwrite() calls between
     * them. With kernel-based filters (kqueue, /dev/poll, epoll) it's
     * wasteful to make system calls when we change only application
     * call-back information. It has a noticeable effect on smtp-source
     * performance.
     */
    if (EVENT_MASK_ISSET(fd, &event_rmask) == 0) {
	EVENT_MASK_SET(fd, &event_xmask);
	EVENT_MASK_SET(fd, &event_rmask);
	if (event_max_fd < fd)
	    event_max_fd = fd;
#if (EVENTS_STYLE != EVENTS_STYLE_SELECT)
	EVENT_REG_ADD_READ(err, fd);
	if (err < 0)
	    msg_fatal("%s: %s: %m", myname, EVENT_REG_ADD_TEXT);
#endif
    }
    fdp = event_fdtable + fd;
    if (fdp->callback != callback || fdp->context != context) {
	fdp->callback = callback;
	fdp->context = context;
    }
}

/* event_enable_write - enable write events */

void    event_enable_write(int fd, EVENT_NOTIFY_RDWR callback, char *context)
{
    const char *myname = "event_enable_write";
    EVENT_FDTABLE *fdp;
    int     err;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (fd < 0 || fd >= event_fdlimit)
	msg_panic("%s: bad file descriptor: %d", myname, fd);

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);

    if (fd >= event_fdslots)
	event_extend(fd);

    /*
     * Disallow mixed (i.e. read and write) requests on the same descriptor.
     */
    if (EVENT_MASK_ISSET(fd, &event_rmask))
	msg_panic("%s: fd %d: read/write I/O request", myname, fd);

    /*
     * Postfix 2.4 allows multiple event_enable_write() calls on the same
     * descriptor without requiring event_disable_readwrite() calls between
     * them. With kernel-based filters (kqueue, /dev/poll, epoll) it's
     * incredibly wasteful to make unregister and register system calls when
     * we change only application call-back information. It has a noticeable
     * effect on smtp-source performance.
     */
    if (EVENT_MASK_ISSET(fd, &event_wmask) == 0) {
	EVENT_MASK_SET(fd, &event_xmask);
	EVENT_MASK_SET(fd, &event_wmask);
	if (event_max_fd < fd)
	    event_max_fd = fd;
#if (EVENTS_STYLE != EVENTS_STYLE_SELECT)
	EVENT_REG_ADD_WRITE(err, fd);
	if (err < 0)
	    msg_fatal("%s: %s: %m", myname, EVENT_REG_ADD_TEXT);
#endif
    }
    fdp = event_fdtable + fd;
    if (fdp->callback != callback || fdp->context != context) {
	fdp->callback = callback;
	fdp->context = context;
    }
}

/* event_disable_readwrite - disable request for read or write events */

void    event_disable_readwrite(int fd)
{
    const char *myname = "event_disable_readwrite";
    EVENT_FDTABLE *fdp;
    int     err;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * Sanity checks.
     */
    if (fd < 0 || fd >= event_fdlimit)
	msg_panic("%s: bad file descriptor: %d", myname, fd);

    if (msg_verbose > 2)
	msg_info("%s: fd %d", myname, fd);

    /*
     * Don't complain when there is nothing to cancel. The request may have
     * been canceled from another thread.
     */
    if (fd >= event_fdslots)
	return;
#if (EVENTS_STYLE != EVENTS_STYLE_SELECT)
#ifdef EVENT_REG_DEL_BOTH
    /* XXX Can't seem to disable READ and WRITE events selectively. */
    if (EVENT_MASK_ISSET(fd, &event_rmask)
	|| EVENT_MASK_ISSET(fd, &event_wmask)) {
	EVENT_REG_DEL_BOTH(err, fd);
	if (err < 0)
	    msg_fatal("%s: %s: %m", myname, EVENT_REG_DEL_TEXT);
    }
#else
    if (EVENT_MASK_ISSET(fd, &event_rmask)) {
	EVENT_REG_DEL_READ(err, fd);
	if (err < 0)
	    msg_fatal("%s: %s: %m", myname, EVENT_REG_DEL_TEXT);
    } else if (EVENT_MASK_ISSET(fd, &event_wmask)) {
	EVENT_REG_DEL_WRITE(err, fd);
	if (err < 0)
	    msg_fatal("%s: %s: %m", myname, EVENT_REG_DEL_TEXT);
    }
#endif						/* EVENT_REG_DEL_BOTH */
#endif						/* != EVENTS_STYLE_SELECT */
    EVENT_MASK_CLR(fd, &event_xmask);
    EVENT_MASK_CLR(fd, &event_rmask);
    EVENT_MASK_CLR(fd, &event_wmask);
    fdp = event_fdtable + fd;
    fdp->callback = 0;
    fdp->context = 0;
}

/* event_request_timer - (re)set timer */

time_t  event_request_timer(EVENT_NOTIFY_TIME callback, char *context, int delay)
{
    const char *myname = "event_request_timer";
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
    const char *myname = "event_cancel_timer";
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
    const char *myname = "event_loop";
    static int nested;

#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
    fd_set  rmask;
    fd_set  wmask;
    fd_set  xmask;
    struct timeval tv;
    struct timeval *tvp;
    int     new_max_fd;

#else
    EVENT_BUFFER event_buf[100];
    EVENT_BUFFER *bp;

#endif
    int     event_count;
    EVENT_TIMER *timer;
    int     fd;
    EVENT_FDTABLE *fdp;
    int     select_delay;

    if (EVENT_INIT_NEEDED())
	event_init();

    /*
     * XXX Also print the select() masks?
     */
    if (msg_verbose > 2) {
	RING   *ring;

	FOREACH_QUEUE_ENTRY(ring, &event_timer_head) {
	    timer = RING_TO_TIMER(ring);
	    msg_info("%s: time left %3d for 0x%lx 0x%lx", myname,
		     (int) (timer->when - event_present),
		     (long) timer->callback, (long) timer->context);
	}
    }

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
#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
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

    event_count = select(event_max_fd + 1, &rmask, &wmask, &xmask, tvp);
    if (event_count < 0) {
	if (errno != EINTR)
	    msg_fatal("event_loop: select: %m");
	return;
    }
#else
    EVENT_BUFFER_READ(event_count, event_buf,
		      sizeof(event_buf) / sizeof(event_buf[0]),
		      select_delay);
    if (event_count < 0) {
	if (errno != EINTR)
	    msg_fatal("event_loop: " EVENT_BUFFER_READ_TEXT ": %m");
	return;
    }
#endif

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
#if (EVENTS_STYLE == EVENTS_STYLE_SELECT)
    if (event_count > 0) {
	for (new_max_fd = 0, fd = 0; fd <= event_max_fd; fd++) {
	    if (FD_ISSET(fd, &event_xmask)) {
		new_max_fd = fd;
		/* In case event_fdtable is updated. */
		fdp = event_fdtable + fd;
		if (FD_ISSET(fd, &xmask)) {
		    if (msg_verbose > 2)
			msg_info("%s: exception fd=%d act=0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		    fdp->callback(EVENT_XCPT, fdp->context);
		} else if (FD_ISSET(fd, &wmask)) {
		    if (msg_verbose > 2)
			msg_info("%s: write fd=%d act=0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		    fdp->callback(EVENT_WRITE, fdp->context);
		} else if (FD_ISSET(fd, &rmask)) {
		    if (msg_verbose > 2)
			msg_info("%s: read fd=%d act=0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		    fdp->callback(EVENT_READ, fdp->context);
		}
	    }
	}
	event_max_fd = new_max_fd;
    }
#else
    for (bp = event_buf; bp < event_buf + event_count; bp++) {
	fd = EVENT_GET_FD(bp);
	if (fd < 0 || fd > event_max_fd)
	    msg_panic("%s: bad file descriptor: %d", myname, fd);
	if (EVENT_MASK_ISSET(fd, &event_xmask)) {
	    fdp = event_fdtable + fd;
	    if (EVENT_TEST_READ(bp)) {
		if (msg_verbose > 2)
		    msg_info("%s: read fd=%d act=0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		fdp->callback(EVENT_READ, fdp->context);
	    } else if (EVENT_TEST_WRITE(bp)) {
		if (msg_verbose > 2)
		    msg_info("%s: write fd=%d act=0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback,
			     (long) fdp->context);
		fdp->callback(EVENT_WRITE, fdp->context);
	    } else {
		if (msg_verbose > 2)
		    msg_info("%s: other fd=%d act=0x%lx 0x%lx", myname,
			     fd, (long) fdp->callback, (long) fdp->context);
		fdp->callback(EVENT_XCPT, fdp->context);
	    }
	}
    }
#endif
    nested--;
}

#ifdef TEST

 /*
  * Proof-of-concept test program for the event manager. Schedule a series of
  * events at one-second intervals and let them happen, while echoing any
  * lines read from stdin.
  */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/* timer_event - display event */

static void timer_event(int unused_event, char *context)
{
    printf("%ld: %s\n", (long) event_present, context);
    fflush(stdout);
}

/* echo - echo text received on stdin */

static void echo(int unused_event, char *unused_context)
{
    char    buf[BUFSIZ];

    if (fgets(buf, sizeof(buf), stdin) == 0)
	exit(0);
    printf("Result: %s", buf);
}

int     main(int argc, char **argv)
{
    if (argv[1])
	msg_verbose = atoi(argv[1]);
    event_request_timer(timer_event, "3 first", 3);
    event_request_timer(timer_event, "3 second", 3);
    event_request_timer(timer_event, "4 first", 4);
    event_request_timer(timer_event, "4 second", 4);
    event_request_timer(timer_event, "2 first", 2);
    event_request_timer(timer_event, "2 second", 2);
    event_enable_read(fileno(stdin), echo, (char *) 0);
    for (;;)
	event_loop(-1);
}

#endif
