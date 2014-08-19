/*	$NetBSD: pass_accept.c,v 1.1.1.1.8.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	pass_accept 3
/* SUMMARY
/*	start UNIX-domain file descriptor listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	pass_accept(listen_fd)
/*	int	listen_fd;
/*
/*	int	pass_accept_attr(listen_fd, attr)
/*	int	listen_fd;
/*	HTABLE	**attr;
/* DESCRIPTION
/*	This module implements a listener that receives one attribute list
/*	and file descriptor over each a local connection that is made to it.
/*
/*	Arguments:
/* .IP attr
/*	Pointer to attribute list pointer.  In case of error, or
/*	no attributes, the attribute list pointer is set to null.
/* .IP listen_fd
/*	File descriptor returned by LOCAL_LISTEN().
/* DIAGNOSTICS
/*	Warnings: I/O errors, timeout.
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
#include <errno.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <listen.h>
#include <attr.h>

#define PASS_ACCEPT_TMOUT	100

/* pass_accept - accept descriptor */

int     pass_accept(int listen_fd)
{
    const char *myname = "pass_accept";
    int     accept_fd;
    int     recv_fd = -1;

    accept_fd = LOCAL_ACCEPT(listen_fd);
    if (accept_fd < 0) {
	if (errno != EAGAIN)
	    msg_warn("%s: cannot accept connection: %m", myname);
	return (-1);
    } else {
	if (read_wait(accept_fd, PASS_ACCEPT_TMOUT) < 0)
	    msg_warn("%s: timeout receiving file descriptor: %m", myname);
	else if ((recv_fd = LOCAL_RECV_FD(accept_fd)) < 0)
	    msg_warn("%s: cannot receive file descriptor: %m", myname);
	if (close(accept_fd) < 0)
	    msg_warn("%s: close: %m", myname);
	return (recv_fd);
    }
}

/* pass_accept_attr - accept descriptor and attribute list */

int     pass_accept_attr(int listen_fd, HTABLE **attr)
{
    const char *myname = "pass_accept_attr";
    int     accept_fd;
    int     recv_fd = -1;

    *attr = 0;
    accept_fd = LOCAL_ACCEPT(listen_fd);
    if (accept_fd < 0) {
	if (errno != EAGAIN)
	    msg_warn("%s: cannot accept connection: %m", myname);
	return (-1);
    } else {
	if (read_wait(accept_fd, PASS_ACCEPT_TMOUT) < 0)
	    msg_warn("%s: timeout receiving file descriptor: %m", myname);
	else if ((recv_fd = LOCAL_RECV_FD(accept_fd)) < 0)
	    msg_warn("%s: cannot receive file descriptor: %m", myname);
	else if (read_wait(accept_fd, PASS_ACCEPT_TMOUT) < 0
	     || recv_pass_attr(accept_fd, attr, PASS_ACCEPT_TMOUT, 0) < 0) {
	    msg_warn("%s: cannot receive connection attributes: %m", myname);
	    if (close(recv_fd) < 0)
		msg_warn("%s: close: %m", myname);
	    recv_fd = -1;
	}
	if (close(accept_fd) < 0)
	    msg_warn("%s: close: %m", myname);
	return (recv_fd);
    }
}
