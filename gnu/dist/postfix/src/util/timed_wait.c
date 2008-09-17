/*++
/* NAME
/*	timed_wait 3
/* SUMMARY
/*	wait operations with timeout
/* SYNOPSIS
/*	#include <timed_wait.h>
/*
/*	int	timed_waitpid(pid, statusp, options, time_limit)
/*	pid_t	pid;
/*	WAIT_STATUS_T *statusp;
/*	int	options;
/*	int	time_limit;
/* DESCRIPTION
/*	\fItimed_waitpid\fR() waits at most \fItime_limit\fR seconds
/*	for process termination.
/*
/*	Arguments:
/* .IP "pid, statusp, options"
/*	The process ID, status pointer and options passed to waitpid(3).
/* .IP time_limit
/*	The time in seconds that timed_waitpid() will wait.
/*	This must be a number > 0.
/* DIAGNOSTICS
/*	Panic: interface violation.
/*
/*	When the time limit is exceeded, the result is -1 and errno
/*	is set to ETIMEDOUT. Otherwise, the result value is the result
/*	from the underlying waitpid() routine.
/* BUGS
/*	If there were a \fIportable\fR way to select() on process status
/*	information, these routines would not have to use a steenkeeng
/*	alarm() timer and signal() handler.
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
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <posix_signals.h>
#include <timed_wait.h>

/* Application-specific. */

static int timed_wait_expired;

/* timed_wait_alarm - timeout handler */

static void timed_wait_alarm(int unused_sig)
{

    /*
     * WARNING WARNING WARNING.
     * 
     * This code runs at unpredictable moments, as a signal handler. This code
     * is here only so that we can break out of waitpid(). Don't put any code
     * here other than for setting a global flag.
     */
    timed_wait_expired = 1;
}

/* timed_waitpid - waitpid with time limit */

int     timed_waitpid(pid_t pid, WAIT_STATUS_T *statusp, int options,
		              int time_limit)
{
    const char *myname = "timed_waitpid";
    struct sigaction action;
    struct sigaction old_action;
    int     time_left;
    int     wpid;

    /*
     * Sanity checks.
     */
    if (time_limit <= 0)
	msg_panic("%s: bad time limit: %d", myname, time_limit);

    /*
     * Set up a timer.
     */
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = timed_wait_alarm;
    if (sigaction(SIGALRM, &action, &old_action) < 0)
	msg_fatal("%s: sigaction(SIGALRM): %m", myname);
    timed_wait_expired = 0;
    time_left = alarm(time_limit);

    /*
     * Wait for only a limited amount of time.
     */
    if ((wpid = waitpid(pid, statusp, options)) < 0 && timed_wait_expired)
	errno = ETIMEDOUT;

    /*
     * Cleanup.
     */
    alarm(0);
    if (sigaction(SIGALRM, &old_action, (struct sigaction *) 0) < 0)
	msg_fatal("%s: sigaction(SIGALRM): %m", myname);
    if (time_left)
	alarm(time_left);

    return (wpid);
}
