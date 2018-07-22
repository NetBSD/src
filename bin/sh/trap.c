/*	$NetBSD: trap.c,v 1.44 2018/07/22 20:43:58 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)trap.c	8.5 (Berkeley) 6/5/95";
#else
__RCSID("$NetBSD: trap.c,v 1.44 2018/07/22 20:43:58 kre Exp $");
#endif
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "shell.h"
#include "main.h"
#include "nodes.h"	/* for other headers */
#include "eval.h"
#include "jobs.h"
#include "show.h"
#include "options.h"
#include "builtins.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "trap.h"
#include "mystring.h"
#include "var.h"


/*
 * Sigmode records the current value of the signal handlers for the various
 * modes.  A value of zero means that the current handler is not known.
 * S_HARD_IGN indicates that the signal was ignored on entry to the shell,
 */

#define S_DFL 1			/* default signal handling (SIG_DFL) */
#define S_CATCH 2		/* signal is caught */
#define S_IGN 3			/* signal is ignored (SIG_IGN) */
#define S_HARD_IGN 4		/* signal is ignored permenantly */
#define S_RESET 5		/* temporary - to reset a hard ignored sig */


char *trap[NSIG];		/* trap handler commands */
MKINIT char sigmode[NSIG];	/* current value of signal */
static volatile char gotsig[NSIG];/* indicates specified signal received */
volatile int pendingsigs;	/* indicates some signal received */

static int getsigaction(int, sig_t *);
STATIC const char *trap_signame(int);

/*
 * return the signal number described by `p' (as a number or a name)
 * or -1 if it isn't one
 */

static int
signame_to_signum(const char *p)
{
	int i;

	if (is_number(p))
		return number(p);

	if (strcasecmp(p, "exit") == 0 )
		return 0;
	
	if (strncasecmp(p, "sig", 3) == 0)
		p += 3;

	for (i = 0; i < NSIG; ++i)
		if (strcasecmp (p, sys_signame[i]) == 0)
			return i;
	return -1;
}

/*
 * return the name of a signal used by the "trap" command
 */
STATIC const char *
trap_signame(int signo)
{
	static char nbuf[12];
	const char *p = NULL;

	if (signo == 0)
		return "EXIT";
	if (signo > 0 && signo < NSIG)
		p = sys_signame[signo];
	if (p != NULL)
		return p;
	(void)snprintf(nbuf, sizeof nbuf, "%d", signo);
	return nbuf;
}

/*
 * Print a list of valid signal names
 */
static void
printsignals(void)
{
	int n;

	out1str("EXIT ");

	for (n = 1; n < NSIG; n++) {
		out1fmt("%s", trap_signame(n));
		if ((n == NSIG/2) ||  n == (NSIG - 1))
			out1str("\n");
		else
			out1c(' ');
	}
}

/*
 * The trap builtin.
 */

int
trapcmd(int argc, char **argv)
{
	char *action;
	char **ap;
	int signo;
	int errs = 0;
	int printonly = 0;

	ap = argv + 1;

	if (argc == 2 && strcmp(*ap, "-l") == 0) {
		printsignals();
		return 0;
	}
	if (argc == 2 && strcmp(*ap, "-") == 0) {
		for (signo = 0; signo < NSIG; signo++) {
			if (trap[signo] == NULL)
				continue;
			INTOFF;
			ckfree(trap[signo]);
			trap[signo] = NULL;
			if (signo != 0)
				setsignal(signo, 0);
			INTON;
		}
		return 0;
	}
	if (argc >= 2 && strcmp(*ap, "-p") == 0) {
		printonly = 1;
		ap++;
		argc--;
	}

	if (argc > 1 && strcmp(*ap, "--") == 0) {
		argc--;
		ap++;
	}

	if (argc <= 1) {
		int count;

		if (printonly) {
			for (count = 0, signo = 0 ; signo < NSIG ; signo++)
				if (trap[signo] == NULL) {
					if (count == 0)
						out1str("trap -- -");
					out1fmt(" %s", trap_signame(signo));
					/* oh! unlucky 13 */
					if (++count >= 13) {
						out1str("\n");
						count = 0;
					}
				}
			if (count)
				out1str("\n");
		}

		for (count = 0, signo = 0 ; signo < NSIG ; signo++)
			if (trap[signo] != NULL && trap[signo][0] == '\0') {
				if (count == 0)
					out1str("trap -- ''");
				out1fmt(" %s", trap_signame(signo));
				/*
				 * the prefix is 10 bytes, with 4 byte
				 * signal names (common) we have room in
				 * the 70 bytes left on a normal line for
				 * 70/(4+1) signals, that's 14, but to
				 * allow for the occasional longer sig name
				 * we output one less...
				 */
				if (++count >= 13) {
					out1str("\n");
					count = 0;
				}
			}
		if (count)
			out1str("\n");

		for (signo = 0 ; signo < NSIG ; signo++)
			if (trap[signo] != NULL && trap[signo][0] != '\0') {
				out1str("trap -- ");
				print_quoted(trap[signo]);
				out1fmt(" %s\n", trap_signame(signo));
			}

		return 0;
	}

	action = NULL;

	if (!printonly && !is_number(*ap)) {
		if ((*ap)[0] == '-' && (*ap)[1] == '\0')
			ap++;			/* reset to default */
		else
			action = *ap++;		/* can be '' for "ignore" */
		argc--;
	}

	if (argc < 2) {		/* there must be at least 1 condition */
		out2str("Usage: trap [-l]\n"
			"       trap -p [condition ...]\n"
			"       trap action condition ...\n"
			"       trap N condition ...\n");
		return 2;
	}


	while (*ap) {
		signo = signame_to_signum(*ap);

		if (signo < 0 || signo >= NSIG) {
			/* This is not a fatal error, so sayeth posix */
			outfmt(out2, "trap: '%s' bad condition\n", *ap);
			errs = 1;
			ap++;
			continue;
		}
		ap++;

		if (printonly) {
			out1str("trap -- ");
			if (trap[signo] == NULL)
				out1str("-");
			else
				print_quoted(trap[signo]);
			out1fmt(" %s\n", trap_signame(signo));
			continue;
		}

		INTOFF;
		if (action)
			action = savestr(action);

		if (trap[signo])
			ckfree(trap[signo]);

		trap[signo] = action;

		if (signo != 0)
			setsignal(signo, 0);
		INTON;
	}
	return errs;
}



/*
 * Clear traps on a fork or vfork.
 * Takes one arg vfork, to tell it to not be destructive of
 * the parents variables.
 */

void
clear_traps(int vforked)
{
	char **tp;

	for (tp = trap ; tp < &trap[NSIG] ; tp++) {
		if (*tp && **tp) {	/* trap not NULL or SIG_IGN */
			INTOFF;
			if (!vforked) {
				ckfree(*tp);
				*tp = NULL;
			}
			if (tp != &trap[0])
				setsignal(tp - trap, vforked);
			INTON;
		}
	}
}



/*
 * Set the signal handler for the specified signal.  The routine figures
 * out what it should be set to.
 */

sig_t
setsignal(int signo, int vforked)
{
	int action;
	sig_t sigact = SIG_DFL, sig;
	char *t, tsig;

	if ((t = trap[signo]) == NULL)
		action = S_DFL;
	else if (*t != '\0')
		action = S_CATCH;
	else
		action = S_IGN;
	if (rootshell && !vforked && action == S_DFL) {
		switch (signo) {
		case SIGINT:
			if (iflag || minusc || sflag == 0)
				action = S_CATCH;
			break;
		case SIGQUIT:
#ifdef DEBUG
			if (debug)
				break;
#endif
			/* FALLTHROUGH */
		case SIGTERM:
			if (iflag)
				action = S_IGN;
			break;
#if JOBS
		case SIGTSTP:
		case SIGTTOU:
			if (mflag)
				action = S_IGN;
			break;
#endif
		}
	}

	t = &sigmode[signo - 1];
	tsig = *t;
	if (tsig == 0) {
		/*
		 * current setting unknown
		 */
		if (!getsigaction(signo, &sigact)) {
			/*
			 * Pretend it worked; maybe we should give a warning
			 * here, but other shells don't. We don't alter
			 * sigmode, so that we retry every time.
			 */
			return 0;
		}
		if (sigact == SIG_IGN) {
			/*
			 * POSIX 3.14.13 states that non-interactive shells
			 * should ignore trap commands for signals that were
			 * ignored upon entry, and leaves the behavior
			 * unspecified for interactive shells. On interactive
			 * shells, or if job control is on, and we have a job
			 * control related signal, we allow the trap to work.
			 *
			 * This change allows us to be POSIX compliant, and
			 * at the same time override the default behavior if
			 * we need to by setting the interactive flag.
			 */
			if ((mflag && (signo == SIGTSTP ||
			     signo == SIGTTIN || signo == SIGTTOU)) || iflag) {
				tsig = S_IGN;
			} else
				tsig = S_HARD_IGN;
		} else {
			tsig = S_RESET;	/* force to be set */
		}
	}
	if (tsig == S_HARD_IGN || tsig == action)
		return 0;
	switch (action) {
		case S_DFL:	sigact = SIG_DFL;	break;
		case S_CATCH:  	sigact = onsig;		break;
		case S_IGN:	sigact = SIG_IGN;	break;
	}
	sig = signal(signo, sigact);
	if (sig != SIG_ERR) {
		sigset_t ss;
		if (!vforked)
			*t = action;
		if (action == S_CATCH)
			(void)siginterrupt(signo, 1);
		/*
		 * If our parent accidentally blocked signals for
		 * us make sure we unblock them
		 */
		(void)sigemptyset(&ss);
		(void)sigaddset(&ss, signo);
		(void)sigprocmask(SIG_UNBLOCK, &ss, NULL);
	}
	return sig;
}

/*
 * Return the current setting for sig w/o changing it.
 */
static int
getsigaction(int signo, sig_t *sigact)
{
	struct sigaction sa;

	if (sigaction(signo, (struct sigaction *)0, &sa) == -1)
		return 0;
	*sigact = (sig_t) sa.sa_handler;
	return 1;
}

/*
 * Ignore a signal.
 */

void
ignoresig(int signo, int vforked)
{
	if (sigmode[signo - 1] != S_IGN && sigmode[signo - 1] != S_HARD_IGN) {
		signal(signo, SIG_IGN);
	}
	if (!vforked)
		sigmode[signo - 1] = S_HARD_IGN;
}


#ifdef mkinit
INCLUDE <signal.h>
INCLUDE "trap.h"

SHELLPROC {
	char *sm;

	clear_traps(0);
	for (sm = sigmode ; sm < sigmode + NSIG ; sm++) {
		if (*sm == S_IGN)
			*sm = S_HARD_IGN;
	}
}
#endif



/*
 * Signal handler.
 */

void
onsig(int signo)
{
	CTRACE(DBG_SIG, ("Signal %d, had: pending %d, gotsig[%d]=%d\n",
	    signo, pendingsigs, signo, gotsig[signo]));

	signal(signo, onsig);
	if (signo == SIGINT && trap[SIGINT] == NULL) {
		onint();
		return;
	}
	gotsig[signo] = 1;
	pendingsigs++;
}



/*
 * Called to execute a trap.  Perhaps we should avoid entering new trap
 * handlers while we are executing a trap handler.
 */

void
dotrap(void)
{
	int i;
	int savestatus;
	char *tr;

	for (;;) {
		for (i = 1 ; ; i++) {
			if (i >= NSIG) {
				pendingsigs = 0;
				return;
			}
			if (gotsig[i])
				break;
		}
		gotsig[i] = 0;
		savestatus=exitstatus;
		CTRACE(DBG_TRAP|DBG_SIG, ("dotrap %d: \"%s\"\n", i,
		    trap[i] ? trap[i] : "-NULL-"));
		if ((tr = trap[i]) != NULL) {
			tr = savestr(tr);	/* trap code may free trap[i] */
			evalstring(tr, 0);
			ckfree(tr);
		}
		exitstatus=savestatus;
	}
}

int
lastsig(void)
{
	int i;

	for (i = NSIG; --i > 0; )
		if (gotsig[i])
			return i;
	return SIGINT;	/* XXX */
}

/*
 * Controls whether the shell is interactive or not.
 */


void
setinteractive(int on)
{
	static int is_interactive;

	if (on == is_interactive)
		return;
	setsignal(SIGINT, 0);
	setsignal(SIGQUIT, 0);
	setsignal(SIGTERM, 0);
	is_interactive = on;
}



/*
 * Called to exit the shell.
 */

void
exitshell(int status)
{
	struct jmploc loc1, loc2;
	char *p;

	CTRACE(DBG_ERRS|DBG_PROCS|DBG_CMDS|DBG_TRAP,
	    ("pid %d, exitshell(%d)\n", getpid(), status));

	if (setjmp(loc1.loc)) {
		goto l1;
	}
	if (setjmp(loc2.loc)) {
		goto l2;
	}
	handler = &loc1;
	if ((p = trap[0]) != NULL && *p != '\0') {
		trap[0] = NULL;
		VTRACE(DBG_TRAP, ("exit trap: \"%s\"\n", p));
		evalstring(p, 0);
	}
 l1:	handler = &loc2;			/* probably unnecessary */
	flushall();
#if JOBS
	setjobctl(0);
#endif
 l2:	_exit(status);
	/* NOTREACHED */
}
