/*++
/* NAME
/*	timed_connect 3
/* SUMMARY
/*	connect operation with timeout
/* SYNOPSIS
/*	#include <sys/socket.h>
/*	#include <timed_connect.h>
/*
/*	int	timed_connect(fd, buf, buf_len, timeout)
/*	int	fd;
/*	struct sockaddr	*buf;
/*	unsigned buf_len;
/*	int	timeout;
/* DESCRIPTION
/*	timed_connect() implement a BSD socket connect() operation that is
/*	bounded in time.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE. This descriptor
/*	must be set to non-blocking mode prior to calling timed_connect().
/* .IP buf
/*	Socket address buffer pointer.
/* .IP buf_len
/*	Size of socket address buffer.
/* .IP timeout
/*	The deadline in seconds. This must be a number > 0.
/* DIAGNOSTICS
/*	Panic: interface violations.
/*	When the operation does not complete within the deadline, the
/*	result value is -1, and errno is set to ETIMEDOUT.
/*	All other returns are identical to those of a blocking connect(2)
/*	operation.
/* WARNINGS
/* .ad
/* .fi
/*	A common error is to call timed_connect() without enabling
/*	non-blocking I/O on the socket. In that case, the \fItimeout\fR
/*	parameter takes no effect.
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

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"
#include "timed_connect.h"

/* timed_connect - connect with deadline */

int     timed_connect(int sock, struct sockaddr * sa, int len, int timeout)
{
    int     error;
    SOCKOPT_SIZE error_len;

    /*
     * Sanity check. Just like with timed_wait(), the timeout must be a
     * positive number.
     */
    if (timeout <= 0)
	msg_panic("timed_connect: bad timeout: %d", timeout);

    /*
     * Start the connection, and handle all possible results.
     */
    if (connect(sock, sa, len) == 0)
	return (0);
    if (errno != EINPROGRESS)
	return (-1);

    /*
     * A connection is in progress. Wait for a limited amount of time for
     * something to happen. If nothing happens, report an error.
     */
    if (write_wait(sock, timeout) < 0)
	return (-1);

    /*
     * Something happened. Some Solaris 2 versions have getsockopt() itself
     * return the error, instead of returning it via the parameter list.
     */
    error = 0;
    error_len = sizeof(error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &error, &error_len) < 0)
	return (-1);
    if (error) {
	errno = error;
	return (-1);
    }

    /*
     * No problems.
     */
    return (0);
}
