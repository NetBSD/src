/*++
/* NAME
/*	read_wait 3
/* SUMMARY
/*	wait until descriptor becomes readable
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	read_wait(fd, timeout)
/*	int	fd;
/*	int	timeout;
/* DESCRIPTION
/*	read_wait() blocks the current process until the specified file
/*	descriptor becomes readable, or until the deadline is exceeded.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE.
/* .IP timeout
/*	If positive, deadline in seconds. A zero value effects a poll.
/*	A negative value means wait until something happens.
/* DIAGNOSTICS
/*	Panic: interface violation. All system call errors are fatal.
/*
/*	A zero result means success.  When the specified deadline is
/*	exceeded, read_wait() returns -1 and sets errno to ETIMEDOUT.
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

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* read_wait - block with timeout until file descriptor is readable */

int     read_wait(int fd, int timeout)
{
    fd_set  read_fds;
    fd_set  except_fds;
    struct timeval tv;
    struct timeval *tp;

    /*
     * Sanity checks.
     */
    if (FD_SETSIZE <= fd)
	msg_panic("descriptor %d does not fit FD_SETSIZE %d", fd, FD_SETSIZE);

    /*
     * Use select() so we do not depend on alarm() and on signal() handlers.
     * Restart the select when interrupted by some signal. Some select()
     * implementations reduce the time to wait when interrupted, which is
     * exactly what we want.
     */
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    FD_ZERO(&except_fds);
    FD_SET(fd, &except_fds);
    if (timeout >= 0) {
	tv.tv_usec = 0;
	tv.tv_sec = timeout;
	tp = &tv;
    } else {
	tp = 0;
    }

    for (;;) {
	switch (select(fd + 1, &read_fds, (fd_set *) 0, &except_fds, tp)) {
	case -1:
	    if (errno != EINTR)
		msg_fatal("select: %m");
	    continue;
	case 0:
	    errno = ETIMEDOUT;
	    return (-1);
	default:
	    return (0);
	}
    }
}
