/*	$NetBSD: peekfd.c,v 1.3 2020/03/18 19:05:22 christos Exp $	*/

/*++
/* NAME
/*	peekfd 3
/* SUMMARY
/*	determine amount of data ready to read
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	ssize_t	peekfd(fd)
/*	int	fd;
/* DESCRIPTION
/*	peekfd() attempts to find out how many bytes are available to
/*	be read from the named file descriptor. The result value is
/*	the number of available bytes.
/* DIAGNOSTICS
/*	peekfd() returns -1 in case of trouble. The global \fIerrno\fR
/*	variable reflects the nature of the problem.
/* BUGS
/*	On some systems, non-blocking read() may fail even after a
/*	positive return from peekfd(). The smtp-sink program works
/*	around this by using the readable() function instead.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/ioctl.h>
#ifdef FIONREAD_IN_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef FIONREAD_IN_TERMIOS_H
#include <termios.h>
#endif
#include <unistd.h>

#ifndef SHUT_RDWR
#define SHUT_RDWR  2
#endif

/* Utility library. */

#include "iostuff.h"

/* peekfd - return amount of data ready to read */

ssize_t peekfd(int fd)
{

    /*
     * Anticipate a series of system-dependent code fragments.
     */
#ifdef FIONREAD
    int     count;

#ifdef SUNOS5

    /*
     * With Solaris10, write_wait() hangs in poll() until timeout, when
     * invoked after peekfd() has received an ECONNRESET error indication.
     * This happens when a client sends QUIT and closes the connection
     * immediately.
     */
    if (ioctl(fd, FIONREAD, (char *) &count) < 0) {
	(void) shutdown(fd, SHUT_RDWR);
	return (-1);
    } else {
	return (count);
    }
#else						/* SUNOS5 */
    return (ioctl(fd, FIONREAD, (char *) &count) < 0 ? -1 : count);
#endif						/* SUNOS5 */
#else
#error "don't know how to look ahead"
#endif
}
