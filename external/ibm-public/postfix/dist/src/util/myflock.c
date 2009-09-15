/*	$NetBSD: myflock.c,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

/*++
/* NAME
/*	myflock 3
/* SUMMARY
/*	lock open file
/* SYNOPSIS
/*	#include <myflock.h>
/*
/*	int	myflock(fd, lock_style, operation)
/*	int	fd;
/*	int	lock_style;
/*	int	operation;
/* DESCRIPTION
/*	myflock() locks or unlocks an entire open file.
/*
/*	In the case of a blocking request, a call that fails due to
/*	forseeable transient problems is retried once per second.
/*
/*	Arguments:
/* .IP fd
/*	The open file to be locked/unlocked.
/* .IP lock_style
/*	One of the following values:
/* .RS
/* .IP	MYFLOCK_STYLE_FLOCK
/*	Use BSD-style flock() locking.
/* .IP	MYFLOCK_STYLE_FCNTL
/*	Use POSIX-style fcntl() locking.
/* .RE
/* .IP operation
/*	One of the following values:
/* .RS
/* .IP	MYFLOCK_OP_NONE
/*	Release any locks the process has on the specified open file.
/* .IP	MYFLOCK_OP_SHARED
/*	Attempt to acquire a shared lock on the specified open file.
/*	This is appropriate for read-only access.
/* .IP	MYFLOCK_OP_EXCLUSIVE
/*	Attempt to acquire an exclusive lock on the specified open
/*	file. This is appropriate for write access.
/* .PP
/*	In addition, setting the MYFLOCK_OP_NOWAIT bit causes the
/*	call to return immediately when the requested lock cannot
/*	be acquired.
/* .RE
/* DIAGNOSTICS
/*	myflock() returns 0 in case of success, -1 in case of failure.
/*	A problem description is returned via the global \fIerrno\fR
/*	variable. In the case of a non-blocking lock request the value
/*	EAGAIN means that a lock is claimed by someone else.
/*
/*	Panic: attempts to use an unsupported file locking method or
/*	to implement an unsupported operation.
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

#include "sys_defs.h"
#include <errno.h>
#include <unistd.h>

#ifdef HAS_FCNTL_LOCK
#include <fcntl.h>
#include <string.h>
#endif

#ifdef HAS_FLOCK_LOCK
#include <sys/file.h>
#endif

/* Utility library. */

#include "msg.h"
#include "myflock.h"

/* myflock - lock/unlock entire open file */

int     myflock(int fd, int lock_style, int operation)
{
    int     status;

    /*
     * Sanity check.
     */
    if ((operation & (MYFLOCK_OP_BITS)) != operation)
	msg_panic("myflock: improper operation type: 0x%x", operation);

    switch (lock_style) {

	/*
	 * flock() does exactly what we need. Too bad it is not standard.
	 */
#ifdef HAS_FLOCK_LOCK
    case MYFLOCK_STYLE_FLOCK:
	{
	    static int lock_ops[] = {
		LOCK_UN, LOCK_SH, LOCK_EX, -1,
		-1, LOCK_SH | LOCK_NB, LOCK_EX | LOCK_NB, -1
	    };

	    while ((status = flock(fd, lock_ops[operation])) < 0
		   && errno == EINTR)
		sleep(1);
	    break;
	}
#endif

	/*
	 * fcntl() is standard and does more than we need, but we can handle
	 * it.
	 */
#ifdef HAS_FCNTL_LOCK
    case MYFLOCK_STYLE_FCNTL:
	{
	    struct flock lock;
	    int     request;
	    static int lock_ops[] = {
		F_UNLCK, F_RDLCK, F_WRLCK
	    };

	    memset((char *) &lock, 0, sizeof(lock));
	    lock.l_type = lock_ops[operation & ~MYFLOCK_OP_NOWAIT];
	    request = (operation & MYFLOCK_OP_NOWAIT) ? F_SETLK : F_SETLKW;
	    while ((status = fcntl(fd, request, &lock)) < 0
		   && errno == EINTR)
		sleep(1);
	    break;
	}
#endif
    default:
	msg_panic("myflock: unsupported lock style: 0x%x", lock_style);
    }

    /*
     * Return a consistent result. Some systems return EACCES when a lock is
     * taken by someone else, and that would complicate error processing.
     */
    if (status < 0 && (operation & MYFLOCK_OP_NOWAIT) != 0)
	if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EACCES)
	    errno = EAGAIN;

    return (status);
}
