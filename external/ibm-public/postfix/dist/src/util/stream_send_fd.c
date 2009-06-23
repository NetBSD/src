/*	$NetBSD: stream_send_fd.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	stream_send_fd 3
/* SUMMARY
/*	send file descriptor
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	stream_send_fd(fd, sendfd)
/*	int	fd;
/*	int	sendfd;
/* DESCRIPTION
/*	stream_send_fd() sends a file descriptor over the specified
/*	stream.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor.
/* .IP sendfd
/*	Another file descriptor.
/* DIAGNOSTICS
/*	stream_send_fd() returns -1 upon failure.
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

#include <sys_defs.h>			/* includes <sys/types.h> */

#ifdef STREAM_CONNECTIONS

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stropts.h>

#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* stream_send_fd - send file descriptor */

int     stream_send_fd(int fd, int sendfd)
{
#ifdef STREAM_CONNECTIONS
    const char *myname = "stream_send_fd";

    if (ioctl(fd, I_SENDFD, sendfd) < 0)
	msg_fatal("%s: send file descriptor %d: %m", myname, sendfd);
    return (0);
#else
            msg_fatal("stream connections are not implemented");
#endif
}

#ifdef TEST

 /*
  * Proof-of-concept program. Open a file and send the descriptor, presumably
  * to the stream_recv_fd test program.
  */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <split_at.h>
#include <connect.h>

int     main(int argc, char **argv)
{
    char   *transport;
    char   *endpoint;
    char   *path;
    int     server_sock;
    int     client_fd;

    if (argc < 3
	|| (endpoint = split_at(transport = argv[1], ':')) == 0
	|| *endpoint == 0 || *transport == 0)
	msg_fatal("usage: %s transport:endpoint file...", argv[0]);

    if (strcmp(transport, "stream") == 0) {
	server_sock = stream_connect(endpoint, BLOCKING, 0);
    } else {
	msg_fatal("invalid transport name: %s", transport);
    }
    if (server_sock < 0)
	msg_fatal("connect %s:%s: %m", transport, endpoint);

    argv += 2;
    while ((path = *argv++) != 0) {
	if ((client_fd = open(path, O_RDONLY, 0)) < 0)
	    msg_fatal("open %s: %m", path);
	msg_info("path=%s client_fd=%d", path, client_fd);
	if (stream_send_fd(server_sock, client_fd) < 0)
	    msg_fatal("send file descriptor: %m");
	if (close(client_fd) != 0)
	    msg_fatal("close(%d): %m", client_fd);
    }
    exit(0);
}

#endif
