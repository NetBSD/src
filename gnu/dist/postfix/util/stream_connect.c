/*++
/* NAME
/*	stream_connect 3
/* SUMMARY
/*	connect to stream listener
/* SYNOPSIS
/*	#include <connect.h>
/*
/*	int	stream_connect(path, block_mode, timeout)
/*	const char *path;
/*	int	block_mode;
/*	int	timeout;
/* DESCRIPTION
/*	stream_connect() connects to a stream listener for the specified
/*	pathname, and returns the resulting file descriptor.
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
#include <stropts.h>

#endif

/* Utility library. */

#include <msg.h>
#include <connect.h>

/* stream_connect - connect to stream listener */

int     stream_connect(const char *path, int block_mode, int unused_timeout)
{
#ifdef STREAM_CONNECTIONS
    char   *myname = "stream_connect";
    int     pair[2];
    int     fifo;

    /*
     * The requested file system object must exist, otherwise we can't reach
     * the server.
     */
    if ((fifo = open(path, O_WRONLY | O_NONBLOCK, 0)) < 0)
	return (-1);

    /*
     * Create a pipe, and send one pipe end to the server.
     */
    if (pipe(pair) < 0)
	msg_fatal("%s: pipe: %m", myname);
    if (ioctl(fifo, I_SENDFD, pair[1]) < 0)
	msg_fatal("%s: send file descriptor: %m", myname);
    close(pair[1]);

    /*
     * This is for {unix,inet}_connect() compatibility.
     */
    if (block_mode == NON_BLOCKING)
	non_blocking(pair[0], NON_BLOCKING);

    /*
     * Cleanup.
     */
    close(fifo);

    /*
     * Keep the other end of the pipe.
     */
    return (pair[0]);
#else
    msg_fatal("stream connections are not implemented");
#endif
}
