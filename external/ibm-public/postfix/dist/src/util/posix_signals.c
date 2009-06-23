/*	$NetBSD: posix_signals.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	posix_signals 3
/* SUMMARY
/*	POSIX signal handling compatibility
/* SYNOPSIS
/*	#include <posix_signals.h>
/*
/*	int	sigemptyset(m)
/*	sigset_t *m;
/*
/*	int	sigaddset(set, signum)
/*	sigset_t *set;
/*	int	signum;
/*
/*	int	sigprocmask(how, set, old)
/*	int	how;
/*	sigset_t *set;
/*	sigset_t *old;
/*
/*	int	sigaction(sig, act, oact)
/*	int	sig;
/*	struct sigaction *act;
/*	struct sigaction *oact;
/* DESCRIPTION
/*	These routines emulate the POSIX signal handling interface.
/* AUTHOR(S)
/*	Pieter Schoenmakers
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB Eindhoven
/*	The Netherlands
/*--*/

/* System library. */

#include "sys_defs.h"
#include <signal.h>
#include <errno.h>

/* Utility library.*/

#include "posix_signals.h"

#ifdef MISSING_SIGSET_T

int     sigemptyset(sigset_t *m)
{
    return *m = 0;
}

int     sigaddset(sigset_t *set, int signum)
{
    *set |= sigmask(signum);
    return 0;
}

int     sigprocmask(int how, sigset_t *set, sigset_t *old)
{
    int previous;

    if (how == SIG_BLOCK)
	previous = sigblock(*set);
    else if (how == SIG_SETMASK)
	previous = sigsetmask(*set);
    else if (how == SIG_UNBLOCK) {
	int     m = sigblock(0);

	previous = sigsetmask(m & ~*set);
    } else {
	errno = EINVAL;
	return -1;
    }

    if (old)
	*old = previous;
    return 0;
}

#endif

#ifdef MISSING_SIGACTION

static struct sigaction actions[NSIG] = {};

static int sighandle(int signum)
{
    if (signum == SIGCHLD) {
	/* XXX If the child is just stopped, don't invoke the handler.	 */
    }
    actions[signum].sa_handler(signum);
}

int     sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
    static int initialized = 0;

    if (!initialized) {
	int     i;

	for (i = 0; i < NSIG; i++)
	    actions[i].sa_handler = SIG_DFL;
	initialized = 1;
    }
    if (sig <= 0 || sig >= NSIG) {
	errno = EINVAL;
	return -1;
    }
    if (oact)
	*oact = actions[sig];

    {
	struct sigvec mine = {
	    sighandle, act->sa_mask,
	    act->sa_flags & SA_RESTART ? SV_INTERRUPT : 0
	};

	if (sigvec(sig, &mine, NULL))
	    return -1;
    }

    actions[sig] = *act;
    return 0;
}

#endif
