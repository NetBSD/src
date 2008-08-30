/*++
/* NAME
/*	non_blocking 3
/* SUMMARY
/*	set/clear non-blocking flag
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	non_blocking(int fd, int on)
/* DESCRIPTION
/*	the \fInon_blocking\fR() function manipulates the non-blocking
/*	flag for the specified open file, and returns the old setting.
/*
/*	Arguments:
/* .IP fd
/*	A file descriptor.
/* .IP on
/*	For non-blocking I/O, specify a non-zero value (or use the
/*	NON_BLOCKING constant); for blocking I/O, specify zero
/*	(or use the BLOCKING constant).
/*
/*	The result is non-zero when the non-blocking flag was enabled.
/* DIAGNOSTICS
/*	All errors are fatal.
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

/* System interfaces. */

#include "sys_defs.h"
#include <fcntl.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"

/* Backwards compatibility */
#ifndef O_NONBLOCK
#define PATTERN	FNDELAY
#else
#define PATTERN	O_NONBLOCK
#endif

/* non_blocking - set/clear non-blocking flag */

int     non_blocking(fd, on)
int     fd;
int     on;
{
    int     flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
	msg_fatal("fcntl: get flags: %m");
    if (fcntl(fd, F_SETFL, on ? flags | PATTERN : flags & ~PATTERN) < 0)
	msg_fatal("fcntl: set non-blocking flag %s: %m", on ? "on" : "off");
    return ((flags & PATTERN) != 0);
}
