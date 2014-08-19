/*	$NetBSD: master_monitor.c,v 1.1.1.1.8.2 2014/08/19 23:59:43 tls Exp $	*/

/*++
/* NAME
/*	master_monitor 3
/* SUMMARY
/*	Postfix master - start-up monitoring
/* SYNOPSIS
/*	#include "master.h"
/*
/*	int     master_monitor(time_limit)
/*	int	time_limit;
/* DESCRIPTION
/*	master_monitor() forks off a background child process, and
/*	returns in the child. The result value is the file descriptor
/*	on which the child process must write one byte after it
/*	completes successful initialization as a daemon process.
/*
/*	The foreground process waits for the child's completion for
/*	a limited amount of time. It terminates with exit status 0
/*	in case of success, non-zero otherwise.
/* DIAGNOSTICS
/*	Fatal errors: system call failure.
/* BUGS
/* SEE ALSO
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
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* Application-specific. */

#include <master.h>

/* master_monitor - fork off a foreground monitor process */

int     master_monitor(int time_limit)
{
    pid_t   pid;
    int     pipes[2];
    char    buf[1];

    /*
     * Sanity check.
     */
    if (time_limit <= 0)
	msg_panic("master_monitor: bad time limit: %d", time_limit);

    /*
     * Set up the plumbing for child-to-parent communication.
     */
    if (pipe(pipes) < 0)
	msg_fatal("pipe: %m");
    close_on_exec(pipes[0], CLOSE_ON_EXEC);
    close_on_exec(pipes[1], CLOSE_ON_EXEC);

    /*
     * Fork the child, and wait for it to report successful initialization.
     */
    switch (pid = fork()) {
    case -1:
	/* Error. */
	msg_fatal("fork: %m");
    case 0:
	/* Child. Initialize as daemon in the background. */
	close(pipes[0]);
	return (pipes[1]);
    default:
	/* Parent. Monitor the child in the foreground. */
	close(pipes[1]);
	switch (timed_read(pipes[0], buf, 1, time_limit, (char *) 0)) {
	default:
	    /* The child process still runs, but something is wrong. */
	    (void) kill(pid, SIGKILL);
	    /* FALLTHROUGH */
	case 0:
	    /* The child process exited prematurely. */
	    msg_fatal("daemon initialization failure");
	case 1:
	    /* The child process initialized successfully. */
	    exit(0);
	}
    }
}
