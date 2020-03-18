/*	$NetBSD: killme_after.c,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

/*++
/* NAME
/*	killme_after 3
/* SUMMARY
/*	programmed death
/* SYNOPSIS
/*	#include <killme_after.h>
/*
/*	void	killme_after(seconds)
/*	unsigned int seconds;
/* DESCRIPTION
/*	The killme_after() function does a best effort to terminate
/*	the process after the specified time, should it still exist.
/*	It is meant to be used in a signal handler, as an insurance
/*	against getting stuck somewhere while preparing for exit.
/* DIAGNOSTICS
/*	None. This routine does a best effort, damn the torpedoes.
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
#include <signal.h>
#include <unistd.h>

/* Utility library. */

#include <killme_after.h>

/* killme_after - self-assured death */

void    killme_after(unsigned int seconds)
{
    struct sigaction sig_action;

    /*
     * Schedule an ALARM signal, and make sure the signal will be delivered
     * even if we are being called from a signal handler and SIGALRM delivery
     * is blocked.
     * 
     * Undocumented: when a process runs with PID 1, Linux won't deliver a
     * signal unless the process specifies a handler (i.e. SIG_DFL is treated
     * as SIG_IGN). Conveniently, _exit() can be used directly as a signal
     * handler. This changes the wait status that a parent would see, but in
     * the case of "init" mode on Linux, no-one would care.
     */
    alarm(0);
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;
    sig_action.sa_handler = (getpid() == 1 ? _exit : SIG_DFL);
    sigaction(SIGALRM, &sig_action, (struct sigaction *) 0);
    alarm(seconds);
    sigaddset(&sig_action.sa_mask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &sig_action.sa_mask, (sigset_t *) 0);
}
