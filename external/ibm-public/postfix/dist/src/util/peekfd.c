/*	$NetBSD: peekfd.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

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

    return (ioctl(fd, FIONREAD, (char *) &count) < 0 ? -1 : count);
#else
#error "don't know how to look ahead"
#endif
}
