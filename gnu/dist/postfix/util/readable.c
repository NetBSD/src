/*++
/* NAME
/*	readable 3
/* SUMMARY
/*	test if descriptor is readable
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	readable(fd)
/*	int	fd;
/* DESCRIPTION
/*	readable() asks the kernel if the specified file descriptor
/*	is readable, i.e. a read operation would not block.
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

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* readable - see if file descriptor is readable */

int     readable(int fd)
{
    struct timeval tv;
    fd_set  read_fds;
    fd_set  except_fds;

    /*
     * Sanity checks.
     */
    if (fd >= FD_SETSIZE)
	msg_fatal("fd %d does not fit in FD_SETSIZE", fd);

    /*
     * Initialize.
     */
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    FD_ZERO(&except_fds);
    FD_SET(fd, &except_fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    /*
     * Loop until we have an authoritative answer.
     */
    for (;;) {
	switch (select(fd + 1, &read_fds, (fd_set *) 0, &except_fds, &tv)) {
	case -1:
	    if (errno != EINTR)
		msg_fatal("select: %m");
	    continue;
	default:
	    return (FD_ISSET(fd, &read_fds));
	case 0:
	    return (0);
	}
    }
}
