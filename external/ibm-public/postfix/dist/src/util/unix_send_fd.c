/*	$NetBSD: unix_send_fd.c,v 1.5.4.1 2012/04/17 00:04:34 yamt Exp $	*/

/*++
/* NAME
/*	unix_send_fd 3
/* SUMMARY
/*	send file descriptor
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	unix_send_fd(fd, sendfd)
/*	int	fd;
/*	int	sendfd;
/* DESCRIPTION
/*	unix_send_fd() sends a file descriptor over the specified
/*	UNIX-domain socket.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor that connects the sending and receiving processes.
/* .IP sendfd
/*	The file descriptor to be sent.
/* DIAGNOSTICS
/*	unix_send_fd() returns -1 upon failure.
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

/* unix_send_fd - send file descriptor */

int     unix_send_fd(int fd, int sendfd)
{

    /*
     * This code does not work with version <2.2 Linux kernels, and it does
     * not compile with version <2 Linux libraries.
     */
#ifdef CANT_USE_SEND_RECV_MSG
    const char *myname = "unix_send_fd";

    msg_warn("%s: your system has no support for file descriptor passing",
	     myname);
    return (-1);
#else
    struct msghdr msg;
    struct iovec iov[1];

    /*
     * Adapted from: W. Richard Stevens, UNIX Network Programming, Volume 1,
     * Second edition. Except that we use CMSG_LEN instead of CMSG_SPACE, for
     * portability to some LP64 environments. See also unix_recv_fd.c.
     */
#if defined(CMSG_SPACE) && !defined(NO_MSGHDR_MSG_CONTROL)
    union {
	struct cmsghdr just_for_alignment;
	char    control[CMSG_SPACE(sizeof(sendfd))];
    }       control_un;
    struct cmsghdr *cmptr;

    memset((char *) &msg, 0, sizeof(msg));	/* Fix 200512 */
    msg.msg_control = control_un.control;
    if (unix_pass_fd_fix & UNIX_PASS_FD_FIX_CMSG_LEN) {
	msg.msg_controllen = CMSG_LEN(sizeof(sendfd));	/* Fix 200506 */
    } else {
	msg.msg_controllen = CMSG_SPACE(sizeof(sendfd));	/* normal */
    }
    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(sendfd));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *(int *) CMSG_DATA(cmptr) = sendfd;
#else
    msg.msg_accrights = (char *) &sendfd;
    msg.msg_accrightslen = sizeof(sendfd);
#endif

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    /*
     * XXX We don't want to pass any data, just a file descriptor. However,
     * setting msg.msg_iov = 0 and msg.msg_iovlen = 0 causes trouble. See the
     * comments in the unix_recv_fd() routine.
     */
    iov->iov_base = "";
    iov->iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    /*
     * The CMSG_LEN send/receive workaround was originally developed for
     * OpenBSD 3.6 on SPARC64. After the workaround was verified to not break
     * Solaris 8 on SPARC64, it was hard-coded with Postfix 2.3 for all
     * platforms because of increasing pressure to work on other things. The
     * workaround does nothing for 32-bit systems.
     * 
     * The investigation was reopened with Postfix 2.7 because the workaround
     * broke with NetBSD 5.0 on 64-bit architectures. This time it was found
     * that OpenBSD <= 4.3 on AMD64 and SPARC64 needed the workaround for
     * sending only. The following platforms worked with and without the
     * workaround: OpenBSD 4.5 on AMD64 and SPARC64, FreeBSD 7.2 on AMD64,
     * Solaris 8 on SPARC64, and Linux 2.6-11 on x86_64.
     * 
     * As this appears to have been an OpenBSD-specific problem, we revert to
     * the Postfix 2.2 behavior. Instead of hard-coding the workaround for
     * all platforms, we now detect sendmsg() errors at run time and turn on
     * the workaround dynamically.
     * 
     * The workaround was made run-time configurable to investigate the problem
     * on multiple platforms. Though set_unix_pass_fd_fix() is over-kill for
     * this specific problem, it is left in place so that it can serve as an
     * example of how to add run-time configurable workarounds to Postfix.
     */
    if (sendmsg(fd, &msg, 0) >= 0)
	return (0);
    if (unix_pass_fd_fix == 0) {
	if (msg_verbose)
	    msg_info("sendmsg error (%m). Trying CMSG_LEN workaround.");
	unix_pass_fd_fix = UNIX_PASS_FD_FIX_CMSG_LEN;
	return (unix_send_fd(fd, sendfd));
    } else {
	return (-1);
    }
#endif
}

#ifdef TEST

 /*
  * Proof-of-concept program. Open a file and send the descriptor, presumably
  * to the unix_recv_fd test program.
  */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <split_at.h>
#include <connect.h>

int     main(int argc, char **argv)
{
    char   *transport;
    char   *endpoint;
    char   *path;
    int     server_sock;
    int     client_fd;

    msg_verbose = 1;

    if (argc < 3
	|| (endpoint = split_at(transport = argv[1], ':')) == 0
	|| *endpoint == 0 || *transport == 0)
	msg_fatal("usage: %s transport:endpoint file...", argv[0]);

    if (strcmp(transport, "unix") == 0) {
	server_sock = unix_connect(endpoint, BLOCKING, 0);
    } else {
	msg_fatal("invalid transport name: %s", transport);
    }
    if (server_sock < 0)
	msg_fatal("connect %s:%s: %m", transport, endpoint);

    argv += 2;
    while ((path = *argv++) != 0) {
	if ((client_fd = open(path, O_RDONLY, 0)) < 0)
	    msg_fatal("open %s: %m", path);
	msg_info("path=%s fd=%d", path, client_fd);
	if (unix_send_fd(server_sock, client_fd) < 0)
	    msg_fatal("send file descriptor: %m");
	if (close(client_fd) != 0)
	    msg_fatal("close(%d): %m", client_fd);
    }
    exit(0);
}

#endif
