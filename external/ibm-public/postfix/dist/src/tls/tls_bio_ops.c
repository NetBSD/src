/*	$NetBSD: tls_bio_ops.c,v 1.1.1.4.6.1 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	tls_bio_ops 3
/* SUMMARY
/*	TLS network basic I/O management
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	int tls_bio_connect(fd, timeout, context)
/*	int	fd;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_accept(fd, timeout, context)
/*	int	fd;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_shutdown(fd, timeout, context)
/*	int	fd;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_read(fd, buf, len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	int	len;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_write(fd, buf, len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	int	len;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/* DESCRIPTION
/*	This module enforces VSTREAM-style timeouts on non-blocking
/*	I/O while performing TLS handshake or input/output operations.
/*
/*	The Postfix VSTREAM read/write routines invoke the
/*	tls_bio_read/write routines to send and receive plain-text
/*	data.  In addition, this module provides tls_bio_connect/accept
/*	routines that trigger the initial TLS handshake.  The
/*	tls_bio_xxx routines invoke the corresponding SSL routines
/*	that translate the requests into TLS protocol messages.
/*
/*	Whenever an SSL operation indicates that network input (or
/*	output) needs to happen, the tls_bio_xxx routines wait for
/*	the network to become readable (or writable) within the
/*	timeout limit, then retry the SSL operation. This works
/*	because the network socket is in non-blocking mode.
/*
/*	tls_bio_connect() performs the SSL_connect() operation.
/*
/*	tls_bio_accept() performs the SSL_accept() operation.
/*
/*	tls_bio_shutdown() performs the SSL_shutdown() operation.
/*
/*	tls_bio_read() performs the SSL_read() operation.
/*
/*	tls_bio_write() performs the SSL_write() operation.
/*
/*	Arguments:
/* .IP fd
/*	Network socket.
/* .IP buf
/*	Read/write buffer.
/* .IP len
/*	Read/write request size.
/* .IP timeout
/*	Read/write timeout.
/* .IP TLScontext
/*	TLS session state.
/* DIAGNOSTICS
/*	A result value > 0 means successful completion.
/*
/*	A result value < 0 means that the requested operation did
/*	not complete due to TLS protocol failure, system call
/*	failure, or for any reason described under "in addition"
/*	below.
/*
/*	A result value of 0 from tls_bio_shutdown() means that the
/*	operation is in progress. A result value of 0 from other
/*	tls_bio_ops(3) operations means that the remote party either
/*	closed the network connection or that it sent a TLS shutdown
/*	request.
/*
/*	Upon return from the tls_bio_ops(3) routines the global
/*	errno value is non-zero when the requested operation did not
/*	complete due to system call failure.
/*
/*	In addition, the result value is set to -1, and the global
/*	errno value is set to ETIMEDOUT, when some network read/write
/*	operation did not complete within the time limit.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/time.h>

#ifndef timersub
/* res = a - b */
#define timersub(a, b, res) do { \
	(res)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(res)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((res)->tv_usec < 0) { \
		(res)->tv_sec--; \
		(res)->tv_usec += 1000000; \
	} \
    } while (0)
#endif

#ifdef USE_TLS

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* tls_bio - perform SSL input/output operation with extreme prejudice */

int     tls_bio(int fd, int timeout, TLS_SESS_STATE *TLScontext,
		        int (*hsfunc) (SSL *),
		        int (*rfunc) (SSL *, void *, int),
		        int (*wfunc) (SSL *, const void *, int),
		        void *buf, int num)
{
    const char *myname = "tls_bio";
    int     status;
    int     err;
    int     enable_deadline;
    struct timeval time_left;		/* amount of time left */
    struct timeval time_deadline;	/* time of deadline */
    struct timeval time_now;		/* time after SSL_mumble() call */

    /*
     * Compensation for interface mis-match: With VSTREAMs, timeout <= 0
     * means wait forever; with the read/write_wait() calls below, we need to
     * specify timeout < 0 instead.
     * 
     * Safety: no time limit means no deadline.
     */
    if (timeout <= 0) {
	timeout = -1;
	enable_deadline = 0;
    }

    /*
     * Deadline management is simpler than with VSTREAMs, because we don't
     * need to decrement a per-stream time limit. We just work within the
     * budget that is available for this tls_bio() call.
     */
    else {
	enable_deadline =
	    vstream_fstat(TLScontext->stream, VSTREAM_FLAG_DEADLINE);
	if (enable_deadline) {
	    GETTIMEOFDAY(&time_deadline);
	    time_deadline.tv_sec += timeout;
	}
    }

    /*
     * If necessary, retry the SSL handshake or read/write operation after
     * handling any pending network I/O.
     */
    for (;;) {
	if (hsfunc)
	    status = hsfunc(TLScontext->con);
	else if (rfunc)
	    status = rfunc(TLScontext->con, buf, num);
	else if (wfunc)
	    status = wfunc(TLScontext->con, buf, num);
	else
	    msg_panic("%s: nothing to do here", myname);
	err = SSL_get_error(TLScontext->con, status);

	/*
	 * Correspondence between SSL_ERROR_* error codes and tls_bio_(read,
	 * write, accept, connect, shutdown) return values (for brevity:
	 * retval).
	 * 
	 * SSL_ERROR_NONE corresponds with retval > 0. With SSL_(read, write)
	 * this is the number of plaintext bytes sent or received. With
	 * SSL_(accept, connect, shutdown) this means that the operation was
	 * completed successfully.
	 * 
	 * SSL_ERROR_WANT_(WRITE, READ) start a new loop iteration, or force
	 * (retval = -1, errno = ETIMEDOUT) when the time limit is exceeded.
	 * 
	 * All other SSL_ERROR_* cases correspond with retval <= 0. With
	 * SSL_(read, write, accept, connect) retval == 0 means that the
	 * remote party either closed the network connection or that it
	 * requested TLS shutdown; with SSL_shutdown() retval == 0 means that
	 * our own shutdown request is in progress. With all operations
	 * retval < 0 means that there was an error. In the latter case,
	 * SSL_ERROR_SYSCALL means that error details are returned via the
	 * errno value.
	 * 
	 * Find out if we must retry the operation and/or if there is pending
	 * network I/O.
	 * 
	 * XXX If we're the first to invoke SSL_shutdown(), then the operation
	 * isn't really complete when the call returns. We could hide that
	 * anomaly here and repeat the call.
	 */
	switch (err) {
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_READ:
	    if (enable_deadline) {
		GETTIMEOFDAY(&time_now);
		timersub(&time_deadline, &time_now, &time_left);
		timeout = time_left.tv_sec + (time_left.tv_usec > 0);
		if (timeout <= 0) {
		    errno = ETIMEDOUT;
		    return (-1);
		}
	    }
	    if (err == SSL_ERROR_WANT_WRITE) {
		if (write_wait(fd, timeout) < 0)
		    return (-1);		/* timeout error */
	    } else {
		if (read_wait(fd, timeout) < 0)
		    return (-1);		/* timeout error */
	    }
	    break;

	    /*
	     * Unhandled cases: SSL_ERROR_WANT_(ACCEPT, CONNECT, X509_LOOKUP)
	     * etc. Historically, Postfix silently treated these as ordinary
	     * I/O errors so we don't really know how common they are. For
	     * now, we just log a warning.
	     */
	default:
	    msg_warn("%s: unexpected SSL_ERROR code %d", myname, err);
	    /* FALLTHROUGH */

	    /*
	     * With tls_timed_read() and tls_timed_write() the caller is the
	     * VSTREAM library module which is unaware of TLS, so we log the
	     * TLS error stack here. In a better world, each VSTREAM I/O
	     * object would provide an error reporting method in addition to
	     * the timed_read and timed_write methods, so that we would not
	     * need to have ad-hoc code like this.
	     */
	case SSL_ERROR_SSL:
	    if (rfunc || wfunc)
		tls_print_errors();
	    /* FALLTHROUGH */
	case SSL_ERROR_ZERO_RETURN:
	case SSL_ERROR_NONE:
	    errno = 0;				/* avoid bogus warnings */
	    /* FALLTHROUGH */
	case SSL_ERROR_SYSCALL:
	    return (status);
	}
    }
}

#endif
