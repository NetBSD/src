/*++
/* NAME
/*	myflock 3
/* SUMMARY
/*	lock open file
/* SYNOPSIS
/*	#include <myflock.h>
/*
/*	int	myflock(fd, how)
/*	int	fd;
/*	int	how;
/*
/*	int	myflock_locked(err)
/*	int	err;
/* DESCRIPTION
/*	myflock() locks or unlocks an entire open file. Depending
/*	on the operating system environment, this function uses the
/*	fcntl() or flock() system calls, so the usual caveats apply.
/*
/*	In the case of a blocking request, a call that fails due to
/*	transient problems is tried again once per second.
/*	In the case of a non-blocking request, use the myflock_locked()
/*	call to distinguish between expected and unexpected failures.
/*
/*	myflock_locked() examines the errno result from a failed
/*	non-blocking lock request, and returns non-zero (true)
/*	when the lock failed because someone else holds it.
/*
/*	Arguments:
/* .IP fd
/*	The open file to be locked/unlocked.
/* .IP how
/*	One of the following values:
/* .RS
/* .IP	MYFLOCK_NONE
/*	Releases any locks the process has on the specified open file.
/* .IP	MYFLOCK_SHARED
/*	Attempts to acquire a shared lock on the specified open file.
/*	This is appropriate for read-only access.
/* .IP	MYFLOCK_EXCLUSIVE
/*	Attempts to acquire an exclusive lock on the specified open
/*	file. This is appropriate for write access.
/* .PP
/*	In addition, setting the MYFLOCK_NOWAIT bit causes the
/*	call to return immediately when the requested lock cannot
/*	be acquired. See the myflock_locked() function on how to deal
/*	with a negative result.
/* .RE
/* DIAGNOSTICS
/*	myflock() returns 0 in case of success, -1 in case of failure.
/*	A problem description is returned via the global \fIerrno\fR
/*	variable.
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

#ifdef USE_FCNTL_LOCK
#include <fcntl.h>
#include <string.h>
#endif

#ifdef USE_FLOCK_LOCK
#include <sys/file.h>
#endif

/* Utility library. */

#include "msg.h"
#include "myflock.h"

/* myflock - lock/unlock entire open file */

int     myflock(int fd, int how)
{

    /*
     * flock() does exactly what we need. Too bad it is not standard.
     */
#ifdef USE_FLOCK_LOCK
    static int lock_ops[] = {
	LOCK_UN, LOCK_SH, LOCK_EX, -1,
	-1, LOCK_SH | LOCK_NB, LOCK_EX | LOCK_NB, -1
    };

    if ((how & (MYFLOCK_BITS)) != how)
	msg_panic("myflock: improper request type: %d", how);
    return (flock(fd, lock_ops[how]));
#endif

    /*
     * fcntl() is standard and does more than we need, but we can handle it.
     */
#ifdef USE_FCNTL_LOCK
    struct flock lock;
    int     request;
    static int lock_ops[] = {
	F_UNLCK, F_RDLCK, F_WRLCK
    };
    int     ret;

    if ((how & (MYFLOCK_BITS)) != how)
	msg_panic("myflock: improper request type: %d", how);
    memset((char *) &lock, 0, sizeof(lock));
    lock.l_type = lock_ops[how & ~MYFLOCK_NOWAIT];
    request = (how & MYFLOCK_NOWAIT) ? F_SETLK : F_SETLKW;
    while ((ret = fcntl(fd, request, &lock)) < 0
	   && request == F_SETLKW
	   && (errno == EINTR || errno == ENOLCK || errno == EDEADLK))
	sleep(1);
    return (ret);
#endif
}

/* myflock_locked - were we locked out or what? */

int     myflock_locked(int err)
{
    return (err == EAGAIN || err == EWOULDBLOCK || err == EACCES);
}
