/*	$NetBSD: posix_signals.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _POSIX_SIGNALS_H_INCLUDED_
#define _POSIX_SIGNALS_H_INCLUDED_
/*++
/* NAME
/*	posix_signals 3h
/* SUMMARY
/*	POSIX signal handling compatibility
/* SYNOPSIS
/*	#include <posix_signals.h>
/* DESCRIPTION
/* .nf

 /*
  * Compatibility interface.
  */

#ifdef MISSING_SIGSET_T

typedef int sigset_t;

enum {
    SIG_BLOCK,
    SIG_UNBLOCK,
    SIG_SETMASK
};

extern int sigemptyset(sigset_t *);
extern int sigaddset(sigset_t *, int);
extern int sigprocmask(int, sigset_t *, sigset_t *);

#endif

#ifdef MISSING_SIGACTION

struct sigaction {
    void    (*sa_handler) ();
    sigset_t sa_mask;
    int     sa_flags;
};

 /* Possible values for sa_flags.  Or them to set multiple.  */
enum {
    SA_RESTART,
    SA_NOCLDSTOP = 4			/* drop the = 4.  */
};

extern int sigaction(int, struct sigaction *, struct sigaction *);

#endif

/* AUTHOR(S)
/*	Pieter Schoenmakers
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB Eindhoven
/*	The Netherlands
/*--*/

#endif
