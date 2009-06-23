/*	$NetBSD: upass_listen.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	upass_listen 3
/* SUMMARY
/*	start UNIX-domain file descriptor listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	upass_listen(path, backlog, block_mode)
/*	const char *path;
/*	int	backlog;
/*	int	block_mode;
/*
/*	int	upass_accept(fd)
/*	int	fd;
/* DESCRIPTION
/*	This module implements a listener that receives one file descriptor
/*	across each UNIX-domain connection that is made to it.
/*
/*	upass_listen() creates a listener endpoint with the specified
/*	permissions, and returns a file descriptor to be used for accepting
/*	descriptors.
/*
/*	upass_accept() accepts a descriptor.
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
/*	File descriptor returned by upass_listen().
/* DIAGNOSTICS
/*	Fatal errors: upass_listen() aborts upon any system call failure.
/*	upass_accept() leaves all error handling up to the caller.
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
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <sane_accept.h>
#include <listen.h>

/* upass_accept - accept descriptor */

int     upass_accept(int listen_fd)
{
    const char *myname = "upass_accept";
    int     accept_fd;
    int     recv_fd;

    accept_fd = sane_accept(listen_fd, (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0);
    if (accept_fd < 0) {
	if (errno != EAGAIN)
	    msg_warn("%s: accept connection: %m", myname);
	return (-1);
    } else {
	if ((recv_fd = unix_recv_fd(accept_fd)) < 0)
	    msg_warn("%s: cannot receive file descriptor: %m", myname);
	if (close(accept_fd) < 0)
	    msg_warn("%s: close: %m", myname);
	return (recv_fd);
    }
}

#if 0

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <events.h>
#include <sane_accept.h>
#include <iostuff.h>
#include <listen.h>

 /*
  * It would be nice if a client could make one UNIX-domain connection to a
  * Postfix master service, send multiple descriptors, and have each
  * descriptor handled by the first available child process.
  * 
  * Possible solutions:
  * 
  * - Either the master process accepts the UNIX-domain connection and forwards
  * each descriptor sent by the client to the first available child process.
  * That's what the code below does. Unfortunately, this approach is
  * inconsistent with the Postfix architecture which tries to eliminate the
  * master from connection management as much as possible.
  * 
  * - Or one child processes accepts the UNIX-domain connection and sends a
  * shared socketpair half to the client. The other socketpair half is shared
  * with the master and all the child's siblings. The client then sends its
  * descriptors over the socketpair, and each descriptor is available to any
  * child process that is waiting for work.
  * 
  * If the second solution did not use a shared socketpair, then all the
  * client's descriptors would be available only to the child process that
  * accepted the UNIX-domain connection. That results in poor performance.
  * 
  * Unfortunately, having to receive a descriptor before being able to send one
  * or more descriptors is ugly from the client's point of view.
  */

#define upass_accept(fd)	unix_recv_fd(fd)

/* upass_plumbing - operate the hidden descriptor passing machinery */

static void upass_plumbing(int unused_event, char *context)
{
    const char *myname = "upass_plumbing";
    UPASS_INFO *info = (UNIX_UPASS_INFO *) context;
    int     fd;

    /*
     * Each time a client connects to the hidden UNIX-domain socket, call
     * unix_send_fd() to send one half of the hidden socketpair across a
     * short-lived UNIX-domain connection. Wait until the client closes the
     * UNIX-domain connection before closing the connection. This wait needs
     * to be time limited.
     */
    fd = sane_accept(info->unixsock, (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0);
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_fatal("%s: accept connection: %m", myname);
    } else {
	if (unix_send_fd(fd, info->halfpair) < 0)
	    msg_warn("%s: cannot send file descriptor: %m", myname);
	if (read_wait(fd, 5) < 0)
	    msg_warn("%s: read timeout", myname);
	if (close(fd) < 0)
	    msg_warn("%s: close: %m", myname);
    }
}

/* upass_listen - set up hidden descriptor passing machinery */

int     upass_listen(const char *path, int backlog, int blocking, UPASS_INFO **ip)
{
    int     pair[2];
    UPASS_INFO *info;

    /*
     * Create a UNIX-domain socket with unix_listen() and create a
     * socketpair. One socketpair half is returned to the caller. The other
     * half is part of the hidden machinery, together with the UNIX-domain
     * socket.
     */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
	msg_fatal("socketpair: %m");
    info = (UPASS_INFO *) mymalloc(sizeof(*info));
    info->halfpair = pair[0];
    info->unixsock = unix_listen(path, backlog, blocking);
    event_request_read(info->unixsock, upass_plumbing, (char *) info);
    *ip = info;
    return (pair[1]);
}

/* upass_shutdown - tear down hidden descriptor passing machinery */

void    upass_shutdown(UPASS_INFO *info)
{
    event_disable_readwrite(upass_info->unixsock)
    if (close(info->unixsock) < 0)
	msg_warn("%s: close unixsock: %m", myname);
    if (close(info->halfpair) < 0)
	msg_warn("%s: close halfpair: %m", myname);
    myfree((char *) info);
}

#endif
