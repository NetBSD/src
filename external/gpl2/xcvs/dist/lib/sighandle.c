/* sighandle.c -- Library routines for manipulating chains of signal handlers
   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */
#include <sys/cdefs.h>
__RCSID("$NetBSD: sighandle.c,v 1.2 2016/05/17 14:00:09 christos Exp $");

/* Written by Paul Sander, HaL Computer Systems, Inc. <paul@hal.com>
   Brian Berliner <berliner@Sun.COM> added POSIX support */

/*************************************************************************
 *
 * signal.c -- This file contains code that manipulates chains of signal
 *             handlers.
 *
 *             Facilities are provided to register a signal handler for
 *             any specific signal.  When a signal is received, all of the
 *             registered signal handlers are invoked in the reverse order
 *             in which they are registered.  Note that the signal handlers
 *             must not themselves make calls to the signal handling
 *             facilities.
 *
 *************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "system.h"

#include <sys/types.h>
#include <stdio.h>
#include <signal.h>

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
#if __STDC__
char *calloc(unsigned nelem, unsigned size);
char *malloc(unsigned size);
#else
char *calloc();
char *malloc();
#endif /* __STDC__ */
#endif /* STDC_HEADERS */

/* Define the highest signal number (usually) */
#ifndef SIGMAX
#define	SIGMAX	64
#endif

/* Define linked list of signal handlers structure */
struct SIG_hlist {
	RETSIGTYPE		(*handler)();
	struct SIG_hlist	*next;
};

/*
 * Define array of lists of signal handlers.  Note that this depends on
 * the implementation to initialize each element to a null pointer.
 */

static	struct SIG_hlist	**SIG_handlers;

/* Define array of default signal vectors */

#ifdef POSIX_SIGNALS
static	struct sigaction	*SIG_defaults;
#else
#ifdef BSD_SIGNALS
static	struct sigvec		*SIG_defaults;
#else
static	RETSIGTYPE		(**SIG_defaults) (int);
#endif
#endif

/* Critical section housekeeping */
static	int		SIG_crSectNest = 0;	/* Nesting level */
#ifdef POSIX_SIGNALS
static	sigset_t	SIG_crSectMask;		/* Signal mask */
#else
static	int		SIG_crSectMask;		/* Signal mask */
#endif

/*
 * Initialize the signal handler arrays
 */

static int SIG_init()
{
	int i;
#ifdef POSIX_SIGNALS
	sigset_t sigset_test;
#endif

	if (SIG_defaults && SIG_handlers)	/* already allocated */
		return (0);

#ifdef POSIX_SIGNALS
	(void) sigfillset(&sigset_test);
	for (i = 1; i < SIGMAX && sigismember(&sigset_test, i) == 1; i++)
		;
	if (i < SIGMAX)
		i = SIGMAX;
	i++;
	if (!SIG_defaults)
		SIG_defaults = (struct sigaction *)
			calloc(i, sizeof(struct sigaction));
	(void) sigemptyset(&SIG_crSectMask);
#else
	i = SIGMAX+1;
#ifdef BSD_SIGNALS
	if (!SIG_defaults)
		SIG_defaults = (struct sigvec *)
			calloc(i, sizeof(struct sigvec));
#else
	if (!SIG_defaults)
		SIG_defaults = ( RETSIGTYPE (**) (int) )
			calloc( i, sizeof( RETSIGTYPE (**) (int) ) );
#endif
	SIG_crSectMask = 0;
#endif
	if (!SIG_handlers)
		SIG_handlers = (struct SIG_hlist **)
			calloc(i, sizeof(struct SIG_hlist *));
	return (!SIG_defaults || !SIG_handlers);
}



/*
 * The following begins a critical section.
 */
void SIG_beginCrSect (void)
{
	if (SIG_init() == 0)
	{
		if (SIG_crSectNest == 0)
		{
#ifdef POSIX_SIGNALS
			sigset_t sigset_mask;

			(void) sigfillset(&sigset_mask);
			(void) sigprocmask(SIG_SETMASK,
					   &sigset_mask, &SIG_crSectMask);
#else
#ifdef BSD_SIGNALS
			SIG_crSectMask = sigblock(~0);
#else
			/* TBD */
#endif
#endif
		}
		SIG_crSectNest++;
	}
}



/*
 * The following ends a critical section.
 */
void SIG_endCrSect (void)
{
	if (SIG_init() == 0)
	{
		SIG_crSectNest--;
		if (SIG_crSectNest == 0)
		{
#ifdef POSIX_SIGNALS
			(void) sigprocmask(SIG_SETMASK, &SIG_crSectMask, NULL);
#else
#ifdef BSD_SIGNALS
			(void) sigsetmask(SIG_crSectMask);
#else
			/* TBD */
#endif
#endif
		}
	}
}



/*
 * The following invokes each signal handler in the reverse order in which
 * they were registered.
 */
static RETSIGTYPE SIG_handle (int sig)
{
	struct SIG_hlist	*this;

	/* Dispatch signal handlers */
	/* This crit section stuff is a CVSism - we know that our interrupt
	 * handlers will always end up exiting and we don't want them to be
	 * interrupted themselves.
	 */
	SIG_beginCrSect();
	this = SIG_handlers[sig];
	while (this != (struct SIG_hlist *) NULL)
	{
		(*this->handler)(sig);
		this = this->next;
	}
	SIG_endCrSect();

	return;
}

/*
 * The following registers a signal handler.  If the handler is already
 * registered, it is not registered twice, nor is the order in which signal
 * handlers are invoked changed.  If this is the first signal handler
 * registered for a given signal, the old sigvec structure is saved for
 * restoration later.
 */

int SIG_register(int sig, RETSIGTYPE (*fn)())
{
	int			val;
	struct SIG_hlist	*this;
#ifdef POSIX_SIGNALS
	struct sigaction	act;
	sigset_t		sigset_mask, sigset_omask;
#else
#ifdef BSD_SIGNALS
	struct sigvec		vec;
	int			mask;
#endif
#endif

	/* Initialize */
	if (SIG_init() != 0)
		return (-1);
	val = 0;

	/* Block this signal while we look at handler chain */
#ifdef POSIX_SIGNALS
	(void) sigemptyset(&sigset_mask);
	(void) sigaddset(&sigset_mask, sig);
	(void) sigprocmask(SIG_BLOCK, &sigset_mask, &sigset_omask);
#else
#ifdef BSD_SIGNALS
	mask = sigblock(sigmask(sig));
#endif
#endif

	/* See if this handler was already registered */
	this = SIG_handlers[sig];
	while (this != (struct SIG_hlist *) NULL)
	{
		if (this->handler == fn) break;
		this = this->next;
	}

	/* Register the new handler only if it is not already registered. */
	if (this == (struct SIG_hlist *) NULL)
	{

		/*
		 * If this is the first handler registered for this signal,
		 * set up the signal handler dispatcher
		 */

		if (SIG_handlers[sig] == (struct SIG_hlist *) NULL)
		{
#ifdef POSIX_SIGNALS
			act.sa_handler = SIG_handle;
			(void) sigemptyset(&act.sa_mask);
			act.sa_flags = 0;
			val = sigaction(sig, &act, &SIG_defaults[sig]);
#else
#ifdef BSD_SIGNALS
			memset (&vec, 0, sizeof (vec));
			vec.sv_handler = SIG_handle;
			val = sigvec(sig, &vec, &SIG_defaults[sig]);
#else
			if ((SIG_defaults[sig] = signal(sig, SIG_handle)) == SIG_ERR)
				val = -1;
#endif
#endif
		}

		/* If not, register it */
		if ((val == 0) && (this == (struct SIG_hlist *) NULL))
		{
			this = (struct SIG_hlist *)
			                      malloc(sizeof(struct SIG_hlist));
			if (this == NULL)
			{
				val = -1;
			}
			else
			{
				this->handler = fn;
				this->next = SIG_handlers[sig];
				SIG_handlers[sig] = this;
			}
		}
	}

	/* Unblock the signal */
#ifdef POSIX_SIGNALS
	(void) sigprocmask(SIG_SETMASK, &sigset_omask, NULL);
#else
#ifdef BSD_SIGNALS
	(void) sigsetmask(mask);
#endif
#endif

	return val;
}



/*
 * The following deregisters a signal handler.  If the last signal handler for
 * a given signal is deregistered, the default sigvec information is restored.
 */

int SIG_deregister(int sig, RETSIGTYPE (*fn)())
{
	int			val;
	struct SIG_hlist	*this;
	struct SIG_hlist	*last;
#ifdef POSIX_SIGNALS
	sigset_t		sigset_mask, sigset_omask;
#else
#ifdef BSD_SIGNALS
	int			mask;
#endif
#endif

	/* Initialize */
	if (SIG_init() != 0)
		return (-1);
	val = 0;
	last = (struct SIG_hlist *) NULL;

	/* Block this signal while we look at handler chain */
#ifdef POSIX_SIGNALS
	(void) sigemptyset(&sigset_mask);
	(void) sigaddset(&sigset_mask, sig);
	(void) sigprocmask(SIG_BLOCK, &sigset_mask, &sigset_omask);
#else
#ifdef BSD_SIGNALS
	mask = sigblock(sigmask(sig));
#endif
#endif

	/* Search for the signal handler */
	this = SIG_handlers[sig];
	while ((this != (struct SIG_hlist *) NULL) && (this->handler != fn))
	{
		last = this;
		this = this->next;
	}

	/* If it was registered, remove it */
	if (this != (struct SIG_hlist *) NULL)
	{
		if (last == (struct SIG_hlist *) NULL)
		{
			SIG_handlers[sig] = this->next;
		}
		else
		{
			last->next = this->next;
		}
		free((char *) this);
	}

	/* Restore default behavior if there are no registered handlers */
	if (SIG_handlers[sig] == (struct SIG_hlist *) NULL)
	{
#ifdef POSIX_SIGNALS
		val = sigaction(sig, &SIG_defaults[sig],
				(struct sigaction *) NULL);
#else
#ifdef BSD_SIGNALS
		val = sigvec(sig, &SIG_defaults[sig], (struct sigvec *) NULL);
#else
		if (signal(sig, SIG_defaults[sig]) == SIG_ERR)
			val = -1;
#endif
#endif
	}

	/* Unblock the signal */
#ifdef POSIX_SIGNALS
	(void) sigprocmask(SIG_SETMASK, &sigset_omask, NULL);
#else
#ifdef BSD_SIGNALS
	(void) sigsetmask(mask);
#endif
#endif

	return val;
}



/*
 * Return nonzero if currently in a critical section.
 * Otherwise return zero.
 */

int SIG_inCrSect (void)
{
	return SIG_crSectNest > 0;
}
