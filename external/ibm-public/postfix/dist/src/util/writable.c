/*	$NetBSD: writable.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	writable 3
/* SUMMARY
/*	test if descriptor is writable
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	writable(fd)
/*	int	fd;
/* DESCRIPTION
/*	writable() asks the kernel if the specified file descriptor
/*	is writable, i.e. a write operation would not block.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE.
/* DIAGNOSTICS
/*	All system call errors are fatal.
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
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef USE_SYSV_POLL
#include <poll.h>
#endif

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* writable - see if file descriptor is writable */

int     writable(int fd)
{
#ifndef USE_SYSV_POLL
    struct timeval tv;
    fd_set  write_fds;
    fd_set  except_fds;

    /*
     * Sanity checks.
     */
    if (fd >= FD_SETSIZE)
	msg_fatal("fd %d does not fit in FD_SETSIZE", fd);

    /*
     * Initialize.
     */
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);
    FD_ZERO(&except_fds);
    FD_SET(fd, &except_fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    /*
     * Loop until we have an authoritative answer.
     */
    for (;;) {
	switch (select(fd + 1, (fd_set *) 0, &write_fds, &except_fds, &tv)) {
	case -1:
	    if (errno != EINTR)
		msg_fatal("select: %m");
	    continue;
	default:
	    return (FD_ISSET(fd, &write_fds));
	case 0:
	    return (0);
	}
    }
#else

    /*
     * System-V poll() is optimal for polling a few descriptors.
     */
    struct pollfd pollfd;

#define DONT_WAIT_FOR_EVENT	0

    pollfd.fd = fd;
    pollfd.events = POLLOUT;
    for (;;) {
	switch (poll(&pollfd, 1, DONT_WAIT_FOR_EVENT)) {
	case -1:
	    if (errno != EINTR)
		msg_fatal("poll: %m");
	    continue;
	case 0:
	    return (0);
	default:
	    if (pollfd.revents & POLLNVAL)
		msg_fatal("poll: %m");
	    return (1);
	}
    }
#endif
}
