/*	$NetBSD: nbbio.c,v 1.1.1.1 2011/03/02 19:32:44 tron Exp $	*/

/*++
/* NAME
/*	nbbio 3
/* SUMMARY
/*	non-blocking buffered I/O
/* SYNOPSIS
/*	#include <nbbio.h>
/*
/*	NBBIO	*nbbio_create(fd, bufsize, label, action, context)
/*	int	fd;
/*	ssize_t	bufsize;
/*	const char *label;
/*	void	(*action)(int event, char *context);
/*	char	*context;
/*
/*	void	nbbio_free(np)
/*	NBBIO	*np;
/*
/*	void	nbbio_enable_read(np, timeout)
/*	NBBIO	*np;
/*	int	timeout;
/*
/*	void	nbbio_enable_write(np, timeout)
/*	NBBIO	*np;
/*	int	timeout;
/*
/*	void	nbbio_disable_readwrite(np)
/*	NBBIO	*np;
/*
/*	void	nbbio_slumber(np, timeout)
/*	NBBIO	*np;
/*	int	timeout;
/*
/*	int	NBBIO_ACTIVE_FLAGS(np)
/*	NBBIO	*np;
/*
/*	int	NBBIO_ERROR_FLAGS(np)
/*	NBBIO	*np;
/*
/*	const ssize_t NBBIO_BUFSIZE(np)
/*	NBBIO	*np;
/*
/*	ssize_t	NBBIO_READ_PEND(np)
/*	NBBIO	*np;
/*
/*	char	*NBBIO_READ_BUF(np)
/*	NBBIO	*np;
/*
/*	const ssize_t NBBIO_WRITE_PEND(np)
/*	NBBIO	*np;
/*
/*	char	*NBBIO_WRITE_BUF(np)
/*	NBBIO	*np;
/* DESCRIPTION
/*	This module implements low-level support for event-driven
/*	I/O on a full-duplex stream. Read/write events are handled
/*	by pseudothreads that run under control by the events(5)
/*	module.  After each I/O operation, the application is
/*	notified via a call-back routine.
/*
/*	It is up to the call-back routine to turn on/off read/write
/*	events as appropriate.  It is an error to leave read events
/*	enabled for a buffer that is full, or to leave write events
/*	enabled for a buffer that is empty.
/*
/*	nbbio_create() creates a pair of buffers of the named size
/*	for the named stream. The label specifies the purpose of
/*	the stream, and is used for diagnostic messages.  The
/*	nbbio(3) event handler invokes the application call-back
/*	routine with the current event type (EVENT_READ etc.) and
/*	with the application-specified context.
/*
/*	nbbio_free() terminates any pseudothreads associated with
/*	the named buffer pair, closes the stream, and destroys the
/*	buffer pair.
/*
/*	nbbio_enable_read() enables a read pseudothread for the
/*	named buffer pair.  It is an error to enable a read
/*	pseudothread while the read buffer is full, or while a read
/*	or write pseudothread is still enabled.
/*
/*	nbbio_enable_write() enables a write pseudothread for the
/*	named buffer pair.  It is an error to enable a write
/*	pseudothread while the write buffer is empty, or while a
/*	read or write pseudothread is still enabled.
/*
/*	nbbio_disable_readwrite() disables any read/write pseudothreads
/*	for the named buffer pair, including timeouts. To ensure
/*	buffer liveness, use nbbio_slumber() instead of
/*	nbbio_disable_readwrite().  It is no error to call this
/*	function while no read/write pseudothread is enabled.
/*
/*	nbbio_slumber() disables any read/write pseudothreads for
/*	the named buffer pair, but keeps the timer active to ensure
/*	buffer liveness. It is no error to call this function while
/*	no read/write pseudothread is enabled.
/*
/*	NBBIO_ERROR_FLAGS() returns the error flags for the named buffer
/*	pair: zero or more of NBBIO_FLAG_EOF (read EOF), NBBIO_FLAG_ERROR
/*	(read/write error) or NBBIO_FLAG_TIMEOUT (time limit
/*	exceeded).
/*
/*	NBBIO_ACTIVE_FLAGS() returns the pseudothread flags for the
/*	named buffer pair: NBBIO_FLAG_READ (read pseudothread is
/*	active), NBBIO_FLAG_WRITE (write pseudothread is active),
/*	or zero (no pseudothread is active).
/*
/*	NBBIO_WRITE_PEND() and NBBIO_WRITE_BUF() evaluate to the
/*	number of to-be-written bytes and the write buffer for the
/*	named buffer pair. NBBIO_WRITE_PEND() must be updated by
/*	the application code that fills the write buffer; no more
/*	than NBBIO_BUFSIZE() bytes may be filled.
/*
/*	NBBIO_READ_PEND() and NBBIO_READ_BUF() evaluate to the
/*	number of unread bytes and the read buffer for the named
/*	buffer pair. NBBIO_READ_PEND() and NBBIO_READ_BUF() must
/*	be updated by the application code that drains the read
/*	buffer.
/* SEE ALSO
/*	events(3) event manager
/* DIAGNOSTICS
/*	Panic: interface violation.
/*
/*	Fatal: out of memory.
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

 /*
  * System library.
  */
#include <sys_defs.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>			/* memmove() */

 /*
  * Utility library.
  */
#include <mymalloc.h>
#include <msg.h>
#include <events.h>
#include <nbbio.h>

/* nbbio_event - non-blocking event handler */

static void nbbio_event(int event, char *context)
{
    const char *myname = "nbbio_event";
    NBBIO  *np = (NBBIO *) context;
    ssize_t count;

    switch (event) {

	/*
	 * Read data into the read buffer. Leave it up to the application to
	 * drain the buffer until it is empty.
	 */
    case EVENT_READ:
	if (np->read_pend == np->bufsize)
	    msg_panic("%s: socket fd=%d: read buffer is full",
		      myname, np->fd);
	if (np->read_pend < 0 || np->read_pend > np->bufsize)
	    msg_panic("%s: socket fd=%d: bad pending read count %ld",
		      myname, np->fd, (long) np->read_pend);
	count = read(np->fd, np->read_buf + np->read_pend,
		     np->bufsize - np->read_pend);
	if (count > 0) {
	    np->read_pend += count;
	    if (msg_verbose)
		msg_info("%s: read %ld on %s fd=%d",
			 myname, (long) count, np->label, np->fd);
	} else if (count == 0) {
	    np->flags |= NBBIO_FLAG_EOF;
	    if (msg_verbose)
		msg_info("%s: read EOF on %s fd=%d",
			 myname, np->label, np->fd);
	} else {
	    if (errno == EAGAIN)
		msg_warn("%s: read() returns EAGAIN on readable descriptor",
			 myname);
	    np->flags |= NBBIO_FLAG_ERROR;
	    if (msg_verbose)
		msg_info("%s: read %s fd=%d: %m", myname, np->label, np->fd);
	}
	break;

	/*
	 * Drain data from the output buffer.  Notify the application
	 * whenever some bytes are written.
	 * 
	 * XXX Enforce a total time limit to ensure liveness when a hostile
	 * receiver sets a very small TCP window size.
	 */
    case EVENT_WRITE:
	if (np->write_pend == 0)
	    msg_panic("%s: socket fd=%d: empty write buffer", myname, np->fd);
	if (np->write_pend < 0 || np->write_pend > np->bufsize)
	    msg_panic("%s: socket fd=%d: bad pending write count %ld",
		      myname, np->fd, (long) np->write_pend);
	count = write(np->fd, np->write_buf, np->write_pend);
	if (count > 0) {
	    np->write_pend -= count;
	    if (np->write_pend > 0)
		memmove(np->write_buf, np->write_buf + count, np->write_pend);
	} else {
	    if (errno == EAGAIN)
		msg_warn("%s: write() returns EAGAIN on writable descriptor",
			 myname);
	    np->flags |= NBBIO_FLAG_ERROR;
	    if (msg_verbose)
		msg_info("%s: write %s fd=%d: %m", myname, np->label, np->fd);
	}
	break;

	/*
	 * Something bad happened.
	 */
    case EVENT_XCPT:
	np->flags |= NBBIO_FLAG_ERROR;
	if (msg_verbose)
	    msg_info("%s: error on %s fd=%d: %m", myname, np->label, np->fd);
	break;

	/*
	 * Something good didn't happen.
	 */
    case EVENT_TIME:
	np->flags |= NBBIO_FLAG_TIMEOUT;
	if (msg_verbose)
	    msg_info("%s: %s timeout on %s fd=%d",
		     myname, NBBIO_OP_NAME(np), np->label, np->fd);
	break;

    default:
	msg_panic("%s: unknown event %d", myname, event);
    }

    /*
     * Application notification. The application will check for any error
     * flags, copy application data from or to our buffer pair, and decide
     * what I/O happens next.
     */
    np->action(event, np->context);
}

/* nbbio_enable_read - enable reading from socket into buffer */

void    nbbio_enable_read(NBBIO *np, int timeout)
{
    const char *myname = "nbbio_enable_read";

    /*
     * Sanity checks.
     */
    if (np->flags & NBBIO_MASK_ACTIVE)
	msg_panic("%s: socket fd=%d is enabled for %s",
		  myname, np->fd, NBBIO_OP_NAME(np));
    if (timeout <= 0)
	msg_panic("%s: socket fd=%d: bad timeout %d",
		  myname, np->fd, timeout);
    if (np->read_pend >= np->bufsize)
	msg_panic("%s: socket fd=%d: read buffer is full",
		  myname, np->fd);

    /*
     * Enable events.
     */
    event_enable_read(np->fd, nbbio_event, (char *) np);
    event_request_timer(nbbio_event, (char *) np, timeout);
    np->flags |= NBBIO_FLAG_READ;
}

/* nbbio_enable_write - enable writing from buffer to socket */

void    nbbio_enable_write(NBBIO *np, int timeout)
{
    const char *myname = "nbbio_enable_write";

    /*
     * Sanity checks.
     */
    if (np->flags & NBBIO_MASK_ACTIVE)
	msg_panic("%s: socket fd=%d is enabled for %s",
		  myname, np->fd, NBBIO_OP_NAME(np));
    if (timeout <= 0)
	msg_panic("%s: socket fd=%d bad timeout %d",
		  myname, np->fd, timeout);
    if (np->write_pend <= 0)
	msg_panic("%s: socket fd=%d: empty write buffer",
		  myname, np->fd);

    /*
     * Enable events.
     */
    event_enable_write(np->fd, nbbio_event, (char *) np);
    event_request_timer(nbbio_event, (char *) np, timeout);
    np->flags |= NBBIO_FLAG_WRITE;
}

/* nbbio_disable_readwrite - disable read/write/timer events */

void    nbbio_disable_readwrite(NBBIO *np)
{
    np->flags &= ~NBBIO_MASK_ACTIVE;
    event_disable_readwrite(np->fd);
    event_cancel_timer(nbbio_event, (char *) np);
}

/* nbbio_slumber - disable read/write events, keep timer */

void    nbbio_slumber(NBBIO *np, int timeout)
{
    np->flags &= ~NBBIO_MASK_ACTIVE;
    event_disable_readwrite(np->fd);
    event_request_timer(nbbio_event, (char *) np, timeout);
}

/* nbbio_create - create socket buffer */

NBBIO  *nbbio_create(int fd, ssize_t bufsize, const char *label,
		             NBBIO_ACTION action, char *context)
{
    NBBIO  *np;

    /*
     * Sanity checks.
     */
    if (fd < 0)
	msg_panic("nbbio_create: bad file descriptor: %d", fd);
    if (bufsize <= 0)
	msg_panic("nbbio_create: bad buffer size: %ld", (long) bufsize);

    /*
     * Create a new buffer pair.
     */
    np = (NBBIO *) mymalloc(sizeof(*np));
    np->fd = fd;
    np->bufsize = bufsize;
    np->label = mystrdup(label);
    np->action = action;
    np->context = context;
    np->flags = 0;

    np->read_buf = mymalloc(bufsize);
    np->read_pend = 0;

    np->write_buf = mymalloc(bufsize);
    np->write_pend = 0;

    return (np);
}

/* nbbio_free - destroy socket buffer */

void    nbbio_free(NBBIO *np)
{
    nbbio_disable_readwrite(np);
    (void) close(np->fd);
    myfree(np->label);
    myfree(np->read_buf);
    myfree(np->write_buf);
    myfree((char *) np);
}
