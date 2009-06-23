/*	$NetBSD: stream_recv_fd.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	stream_recv_fd 3
/* SUMMARY
/*	receive file descriptor
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	stream_recv_fd(fd)
/*	int	fd;
/* DESCRIPTION
/*	stream_recv_fd() receives a file descriptor via the specified
/*	stream. The result value is the received descriptor.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor.
/* DIAGNOSTICS
/*	stream_recv_fd() returns -1 upon failure.
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
#include <errno.h>
#include <stropts.h>
#include <fcntl.h>

#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* stream_recv_fd - receive file descriptor */

int     stream_recv_fd(int fd)
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

#ifdef TEST

 /*
  * Proof-of-concept program. Receive a descriptor (presumably from the
  * stream_send_fd test program) and copy its content until EOF.
  */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <split_at.h>
#include <listen.h>

int     main(int argc, char **argv)
{
    char   *transport;
    char   *endpoint;
    int     listen_sock;
    int     client_sock;
    int     client_fd;
    ssize_t read_count;
    char    buf[1024];

    if (argc != 2
	|| (endpoint = split_at(transport = argv[1], ':')) == 0
	|| *endpoint == 0 || *transport == 0)
	msg_fatal("usage: %s transport:endpoint", argv[0]);

    if (strcmp(transport, "stream") == 0) {
	listen_sock = stream_listen(endpoint, BLOCKING, 0);
    } else {
	msg_fatal("invalid transport name: %s", transport);
    }
    if (listen_sock < 0)
	msg_fatal("listen %s:%s: %m", transport, endpoint);

    client_sock = stream_accept(listen_sock);
    if (client_sock < 0)
	msg_fatal("stream_accept: %m");

    while ((client_fd = stream_recv_fd(client_sock)) >= 0) {
	msg_info("client_fd = %d", client_fd);
	while ((read_count = read(client_fd, buf, sizeof(buf))) > 0)
	    write(1, buf, read_count);
	if (read_count < 0)
	    msg_fatal("read: %m");
	if (close(client_fd) != 0)
	    msg_fatal("close(%d): %m", client_fd);
    }
    exit(0);
}

#endif
