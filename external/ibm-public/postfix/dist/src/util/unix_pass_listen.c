/*	$NetBSD: unix_pass_listen.c,v 1.1.1.1 2011/03/02 19:32:46 tron Exp $	*/

/*++
/* NAME
/*	unix_pass_listen 3
/* SUMMARY
/*	start UNIX-domain file descriptor listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	unix_pass_listen(path, backlog, block_mode)
/*	const char *path;
/*	int	backlog;
/*	int	block_mode;
/*
/*	int	unix_pass_accept(fd)
/*	int	fd;
/* DESCRIPTION
/*	This module implements a listener that receives one file descriptor
/*	across each UNIX-domain connection that is made to it.
/*
/*	unix_pass_listen() creates a listener endpoint with the specified
/*	permissions, and returns a file descriptor to be used for accepting
/*	descriptors.
/*
/*	unix_pass_accept() accepts a descriptor.
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
/*	File descriptor returned by unix_pass_listen().
/* DIAGNOSTICS
/*	Fatal errors: unix_pass_listen() aborts upon any system call failure.
/*	unix_pass_accept() leaves all error handling up to the caller.
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

/* unix_pass_accept - accept descriptor */

int     unix_pass_accept(int listen_fd)
{
    const char *myname = "unix_pass_accept";
    int     accept_fd;
    int     recv_fd = -1;

    accept_fd = sane_accept(listen_fd, (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0);
    if (accept_fd < 0) {
	if (errno != EAGAIN)
	    msg_warn("%s: accept connection: %m", myname);
	return (-1);
    } else {
	if (read_wait(accept_fd, 100) < 0)
	    msg_warn("%s: timeout receiving file descriptor: %m", myname);
	else if ((recv_fd = unix_recv_fd(accept_fd)) < 0)
	    msg_warn("%s: cannot receive file descriptor: %m", myname);
	if (close(accept_fd) < 0)
	    msg_warn("%s: close: %m", myname);
	return (recv_fd);
    }
}
