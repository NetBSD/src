/*++
/* NAME
/*	stream_listen 3
/* SUMMARY
/*	start stream listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	stream_listen(path, backlog, block_mode)
/*	const char *path;
/*	int	backlog;
/*	int	block_mode;
/*
/*	int	stream_accept(fd)
/*	int	fd;
/* DESCRIPTION
/*	This module implements a substitute local IPC for systems that do
/*	not have properly-working UNIX-domain sockets.
/*
/*	stream_listen() creates a listener endpoint with the specified
/*	permissions, and returns a file descriptor to be used for accepting
/*	connections.
/*
/*	stream_accept() accepts a connection.
/*
/*	Arguments:
/* .IP path
/*	Null-terminated string with connection destination.
/* .IP backlog
/*	This argument exists for compatibility and is ignored.
/* .IP block_mode
/*	Either NON_BLOCKING or BLOCKING. This does not affect the
/*	mode of accepted connections.
/* .IP fd
/*	File descriptor returned by stream_listen().
/* DIAGNOSTICS
/*	Fatal errors: stream_listen() aborts upon any system call failure.
/*	stream_accept() leaves all error handling up to the caller.
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

/* System interfaces. */

#include <sys_defs.h>

#ifdef STREAM_CONNECTIONS

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stropts.h>
#include <fcntl.h>

#endif

/* Utility library. */

#include "msg.h"
#include "listen.h"

/* stream_listen - create stream listener */

int     stream_listen(const char *path, int unused_backlog, int block_mode)
{
#ifdef STREAM_CONNECTIONS

    /*
     * We can't specify a listen backlog, however, sending file descriptors
     * across a FIFO gives us a backlog buffer of 460 on Solaris 2.4/SPARC.
     */
    return (fifo_listen(path, 0622, block_mode));
#else
    msg_fatal("stream connections are not implemented");
#endif
}

/* stream_accept - accept stream connection */

int     stream_accept(int fd)
{
#ifdef STREAM_CONNECTIONS
    struct strrecvfd fdinfo;

    /*
     * This will return EAGAIN on a non-blocking stream when someone else
     * snatched the connection from us.
     */
    if (ioctl(fd, I_RECVFD, &fdinfo) < 0)
	return (-1);
    return (fdinfo.fd);
#else
    msg_fatal("stream connections are not implemented");
#endif
}
