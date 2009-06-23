/*	$NetBSD: unix_recv_fd.c,v 1.2 2009/06/23 11:41:07 tron Exp $	*/

/*++
/* NAME
/*	unix_recv_fd 3
/* SUMMARY
/*	receive file descriptor
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	unix_recv_fd(fd)
/*	int	fd;
/* DESCRIPTION
/*	unix_recv_fd() receives a file descriptor via the specified
/*	UNIX-domain socket. The result value is the received descriptor.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor.
/* DIAGNOSTICS
/*	unix_recv_fd() returns -1 upon failure.
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
#include <sys/socket.h>
#include <sys/uio.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* unix_recv_fd - receive file descriptor */

int     unix_recv_fd(int fd)
{
    const char *myname = "unix_recv_fd";

    /*
     * This code does not work with version <2.2 Linux kernels, and it does
     * not compile with version <2 Linux libraries.
     */
#ifdef CANT_USE_SEND_RECV_MSG
    msg_warn("%s: your system has no support for file descriptor passing",
	     myname);
    return (-1);
#else
    struct msghdr msg;
    int     newfd;
    struct iovec iov[1];
    char    buf[1];

    /*
     * Adapted from: W. Richard Stevens, UNIX Network Programming, Volume 1,
     * Second edition. Except that we use CMSG_LEN instead of CMSG_SPACE, for
     * portability to LP64 environments.
     */
#if defined(CMSG_SPACE) && !defined(NO_MSGHDR_MSG_CONTROL)
    union {
	struct cmsghdr just_for_alignment;
	char    control[CMSG_SPACE(sizeof(newfd))];
    }       control_un;
    struct cmsghdr *cmptr;

    memset((char *) &msg, 0, sizeof(msg));	/* Fix 200512 */
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);	/* Fix 200506 */
#else
    msg.msg_accrights = (char *) &newfd;
    msg.msg_accrightslen = sizeof(newfd);
#endif

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    /*
     * XXX We don't want to pass any data, just a file descriptor. However,
     * setting msg.msg_iov = 0 and msg.msg_iovlen = 0 causes trouble: we need
     * to read_wait() before we can receive the descriptor, and the code
     * fails after the first descriptor when we attempt to receive a sequence
     * of descriptors.
     */
    iov->iov_base = buf;
    iov->iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if (recvmsg(fd, &msg, 0) < 0)
	return (-1);

#if defined(CMSG_SPACE) && !defined(NO_MSGHDR_MSG_CONTROL)
    if ((cmptr = CMSG_FIRSTHDR(&msg)) != 0
	&& cmptr->cmsg_len == CMSG_LEN(sizeof(newfd))) {
	if (cmptr->cmsg_level != SOL_SOCKET)
	    msg_fatal("%s: control level %d != SOL_SOCKET",
		      myname, cmptr->cmsg_level);
	if (cmptr->cmsg_type != SCM_RIGHTS)
	    msg_fatal("%s: control type %d != SCM_RIGHTS",
		      myname, cmptr->cmsg_type);
	return (*(int *) CMSG_DATA(cmptr));
    } else
	return (-1);
#else
    if (msg.msg_accrightslen == sizeof(newfd))
	return (newfd);
    else
	return (-1);
#endif
#endif
}

#ifdef TEST

 /*
  * Proof-of-concept program. Receive a descriptor (presumably from the
  * unix_send_fd test program) and copy its content until EOF.
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

    if (strcmp(transport, "unix") == 0) {
	listen_sock = unix_listen(endpoint, 10, BLOCKING);
    } else {
	msg_fatal("invalid transport name: %s", transport);
    }
    if (listen_sock < 0)
	msg_fatal("listen %s:%s: %m", transport, endpoint);

    client_sock = accept(listen_sock, (struct sockaddr *) 0, (SOCKADDR_SIZE) 0);
    if (client_sock < 0)
	msg_fatal("accept: %m");

    while ((client_fd = unix_recv_fd(client_sock)) >= 0) {
	msg_info("client_fd = %d", client_fd);
	while ((read_count = read(client_fd, buf, sizeof(buf))) > 0)
	    write(1, buf, read_count);
	if (read_count < 0)
	    msg_fatal("read: %m");
	close(client_fd);
    }
    exit(0);
}

#endif
