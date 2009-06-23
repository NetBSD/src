/*	$NetBSD: sigdelay.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	sigdelay 3
/* SUMMARY
/*	delay/resume signal delivery
/* SYNOPSIS
/*	#include <sigdelay.h>
/*
/*	void	sigdelay()
/*
/*	void	sigresume()
/* DESCRIPTION
/*	sigdelay() delays delivery of signals. Signals that
/*	arrive in the mean time will be queued.
/*
/*	sigresume() resumes delivery of signals. Signals that have
/*	arrived in the mean time will be delivered.
/* DIAGNOSTICS
/*	All errors are fatal.
/* BUGS
/*	The signal queue may be really short (as in: one per signal type).
/*
/*	Some signals such as SIGKILL cannot be blocked.
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

/* Utility library. */

#include "msg.h"
#include "posix_signals.h"
#include "sigdelay.h"

/* Application-specific. */

static sigset_t saved_sigmask;
static sigset_t block_sigmask;
static int suspending;
static int siginit_done;

/* siginit - compute signal mask only once */

static void siginit(void)
{
    int     sig;

    siginit_done = 1;
    sigemptyset(&block_sigmask);
    for (sig = 1; sig < NSIG; sig++)
	sigaddset(&block_sigmask, sig);
}

/* sigresume - deliver delayed signals and disable signal delay */

void    sigresume(void)
{
    if (suspending != 0) {
	suspending = 0;
	if (sigprocmask(SIG_SETMASK, &saved_sigmask, (sigset_t *) 0) < 0)
	    msg_fatal("sigresume: sigprocmask: %m");
    }
}

/* sigdelay - save signal mask and block all signals */

void    sigdelay(void)
{
    if (siginit_done == 0)
	siginit();
    if (suspending == 0) {
	suspending = 1;
	if (sigprocmask(SIG_BLOCK, &block_sigmask, &saved_sigmask) < 0)
	    msg_fatal("sigdelay: sigprocmask: %m");
    }
}

#ifdef TEST

 /*
  * Test program - press Ctrl-C twice while signal delivery is delayed, and
  * see how many signals are delivered when signal delivery is resumed.
  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static void gotsig(int sig)
{
    printf("Got signal %d\n", sig);
}

int     main(int unused_argc, char **unused_argv)
{
    signal(SIGINT, gotsig);
    signal(SIGQUIT, gotsig);

    printf("Delaying signal delivery\n");
    sigdelay();
    sleep(5);
    printf("Resuming signal delivery\n");
    sigresume();
    exit(0);
}

#endif
