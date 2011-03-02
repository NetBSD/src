/*	$NetBSD: stream_pass_connect.c,v 1.1.1.1 2011/03/02 19:32:45 tron Exp $	*/

/*++
/* NAME
/*	stream_pass_connect 3
/* SUMMARY
/*	connect to stream-based descriptor listener
/* SYNOPSIS
/*	#include <connect.h>
/*
/*	int	stream_pass_connect(path, block_mode, timeout)
/*	const char *path;
/*	int	block_mode;
/*	int	timeout;
/* DESCRIPTION
/*	stream_pass_connect() connects to a stream-based descriptor
/*	listener for the specified pathname, and returns the resulting
/*	file descriptor. The next operation is to stream_send_fd()
/*	a file descriptor and then close() the connection once the
/*	server has received the file descriptor.
/*
/*	Arguments:
/* .IP path
/*	Null-terminated string with listener endpoint name.
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking stream, or BLOCKING for
/*	blocking mode. However, a stream connection succeeds or fails
/*	immediately.
/* .IP timeout
/*	This argument is ignored; it is present for compatibility with
/*	other interfaces. Stream connections succeed or fail immediately.
/* DIAGNOSTICS
/*	The result is -1 in case the connection could not be made.
/*	Fatal errors: other system call failures.
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

#ifdef STREAM_CONNECTIONS

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#endif

/* Utility library. */

#include <msg.h>
#include <connect.h>

/* stream_pass_connect - connect to stream-based descriptor listener */

int     stream_pass_connect(const char *path, int block_mode, int unused_timeout)
{
#ifdef STREAM_CONNECTIONS
    const char *myname = "stream_pass_connect";
    int     fifo;

    /*
     * The requested file system object must exist, otherwise we can't reach
     * the server.
     */
    if ((fifo = open(path, O_WRONLY | O_NONBLOCK, 0)) < 0)
	return (-1);

    /*
     * This is for {unix,inet}_connect() compatibility.
     */
    non_blocking(fifo, block_mode);

    return (fifo);
#else
    msg_fatal("stream connections are not implemented");
#endif
}
