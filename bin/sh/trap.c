/*	$NetBSD: trap.c,v 1.55 2020/08/20 23:09:56 kre Exp $	*/

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
__RCSID("$NetBSD: trap.c,v 1.55 2020/08/20 23:09:56 kre Exp $");
#endif
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <termios.h>

#undef	CEOF	/* from <termios.h> but concflicts with sh use */

#include <sys/ioctl.h>
#include <sys/resource.h>

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


MKINIT char sigmode[NSIG];	/* current value of signal */
static volatile sig_atomic_t gotsig[NSIG];/* indicates specified signal received */
volatile sig_atomic_t pendingsigs;	/* indicates some signal received */

int traps_invalid;		/* in a subshell, but trap[] not yet cleared */
static char * volatile trap[NSIG];	/* trap handler commands */
static int in_dotrap;
static int last_trapsig;

static int exiting;		/* exitshell() has been done */
static int exiting_status;	/* the status to use for exit() */

static int getsigaction(int, sig_t *);
STATIC const char *trap_signame(int);
void printsignals(struct output *, int);

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

	i = signalnumber(p);
	if (i == 0)
		i = -1;
	return i;
}

/*
 * return the name of a signal used by the "trap" command
 */
STATIC const char *
trap_signame(int signo)
{
	static char nbuf[12];
	const char *p;

	if (signo == 0)
		return "EXIT";
	p = signalname(signo);
	if (p != NULL)
		return p;
	(void)snprintf(nbuf, sizeof nbuf, "%d", signo);
	return nbuf;
}

#ifdef SMALL
/*
 * Print a list of valid signal names
 */
void
printsignals(struct output *out, int len)
{
	int n;

	if (len != 0)
		outc(' ', out);
	for (n = 1; n < NSIG; n++) {
		outfmt(out, "%s", trap_signame(n));
		if ((n == NSIG/2) || n == (NSIG - 1))
			outstr("\n", out);
		else
			outc(' ', out);
	}
}
#else /* !SMALL */
/*
 * Print the names of all the signals (neatly) to fp
 * "len" gives the number of chars already printed to
 * the current output line (in kill.c, always 0)
 */
void
printsignals(struct output *out, int len)
{
	int sig;
	int nl, pad;
	const char *name;
	int termwidth = 80;

	if ((name = bltinlookup("COLUMNS", 1)) != NULL)
		termwidth = (int)strtol(name, NULL, 10);
	else if (isatty(1)) {
		struct winsize win;

		if (ioctl(1, TIOCGWINSZ, &win) == 0 && win.ws_col > 0)
			termwidth = win.ws_col;
	}

	if (posix)
		pad = 1;
	else
		pad = (len | 7) + 1 - len;

	for (sig = 0; (sig = signalnext(sig)) != 0; ) {
		name = signalname(sig);
		if (name == NULL)
			continue;

		nl = strlen(name);

		if (len > 0 && nl + len + pad >= termwidth) {
			outc('\n', out);
			len = 0;
			pad = 0;
		} else if (pad > 0 && len != 0)
			outfmt(out, "%*s", pad, "");
		else
			pad = 0;

		len += nl + pad;
		if (!posix)
			pad = (nl | 7) + 1 - nl;
		else
			pad = 1;

		outstr(name, out);
	}
	if (len != 0)
		outc('\n', out);
}
#endif /* SMALL */

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

	CTRACE(DBG_TRAP, ("trapcmd: "));
	if (argc == 3 && strcmp(ap[1], "--") == 0)
		argc--;
	if (argc == 2 && strcmp(*ap, "-l") == 0) {
		CTRACE(DBG_TRAP, ("-l\n"));
		out1str("EXIT");
		printsignals(out1, 4);
		return 0;
	}
	if (argc == 2 && strcmp(*ap, "-") == 0) {
		CTRACE(DBG_TRAP, ("-\n"));
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
		traps_invalid = 0;
		return 0;
	}
	if (argc >= 2 && (strcmp(*ap, "-p") == 0 || strcmp(*ap, "-P") == 0)) {
		CTRACE(DBG_TRAP, ("%s ", *ap));
		printonly = 1 + (ap[0][1] == 'p');
		ap++;
		argc--;
	}

	if (argc > 1 && strcmp(*ap, "--") == 0) {
		argc--;
		ap++;
	}

	if (printonly == 1 && argc < 2)
		goto usage;

	if (argc <= 1) {
		int count;

		CTRACE(DBG_TRAP, ("*all*\n"));
		if (printonly) {
			for (count = 0, signo = 0 ; signo < NSIG ; signo++) {
				if (signo == SIGKILL || signo == SIGSTOP)
					continue;
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
			}
			if (count)
				out1str("\n");
		}

		/*
		 * We don't need do deal with SIGSTOP or SIGKILL as a
		 * special case anywhere here, as they cannot be
		 * ignored or caught - the only possibility is default
		 */
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
	CTRACE(DBG_TRAP, ("\n"));

	action = NULL;

	if (!printonly && traps_invalid)
		free_traps();

	if (!printonly && !is_number(*ap)) {
		if ((*ap)[0] == '-' && (*ap)[1] == '\0')
			ap++;			/* reset to default */
		else
			action = *ap++;		/* can be '' for "ignore" */
		argc--;
	}

	if (argc < 2) {		/* there must be at least 1 condition */
 usage:
		out2str("Usage: trap [-l]\n"
			"       trap -p [condition ...]\n"
			"       trap -P  condition ...\n"
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
			/*
			 * we allow SIGSTOP and SIGKILL to be obtained
			 * (action will always be "-") here, if someone
			 * really wants to get that particular output
			 */
			if (printonly == 1) {
				if (trap[signo] != NULL)
					out1fmt("%s\n", trap[signo]);
			} else {
				out1str("trap -- ");
				if (trap[signo] == NULL)
					out1str("-");
				else
					print_quoted(trap[signo]);
				out1fmt(" %s\n", trap_signame(signo));
			}
			continue;
		}

		if ((signo == SIGKILL || signo == SIGSTOP) && action != NULL) {
#ifndef SMALL
			/*
			 * Don't bother with the error message in a SMALL shell
			 * just ignore req and  return error status silently
			 * (POSIX says this is an "undefined" operation so
			 * whatever we do is OK!)
			 *
			 * When we do generate an error, make it attempt match
			 * the user's operand, as best we can reasonably.
			 */
			outfmt(out2, "trap: '%s%s' cannot be %s\n",
			    (!is_alpha(ap[-1][0]) ||
				strncasecmp(ap[-1], "sig", 3) == 0) ? "" :
				is_upper(ap[-1][0]) ? "SIG" : "sig",
			    ap[-1], *action ? "caught" : "ignored");
#endif
			errs = 1;
			continue;
		}

		INTOFF;
		if (action)
			action = savestr(action);

		VTRACE(DBG_TRAP, ("trap for %d from %s%s%s to %s%s%s\n", signo,
			trap[signo] ? "'" : "", trap[signo] ? trap[signo] : "-",
			trap[signo] ? "'" : "", action ?  "'" : "",
			action ? action : "-", action ?  "'" : ""));

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
	char * volatile *tp;

	VTRACE(DBG_TRAP, ("clear_traps(%d)\n", vforked));
	if (!vforked)
		traps_invalid = 1;

	for (tp = &trap[1] ; tp < &trap[NSIG] ; tp++) {
		if (*tp && **tp) {	/* trap not NULL or SIG_IGN */
			INTOFF;
			setsignal(tp - trap, vforked == 1);
			INTON;
		}
	}
	if (vforked == 2)
		free_traps();
}

void
free_traps(void)
{
	char * volatile *tp;

	VTRACE(DBG_TRAP, ("free_traps%s\n", traps_invalid ? "(invalid)" : ""));
	INTOFF;
	for (tp = trap ; tp < &trap[NSIG] ; tp++)
		if (*tp && **tp) {
			ckfree(*tp);
			*tp = NULL;
		}
	traps_invalid = 0;
	INTON;
}

/*
 * See if there are any defined traps
 */
int
have_traps(void)
{
	char * volatile *tp;

	if (traps_invalid)
		return 0;

	for (tp = trap ; tp < &trap[NSIG] ; tp++)
		if (*tp && **tp)	/* trap not NULL or SIG_IGN */
			return 1;
	return 0;
}

/*
 * Set the signal handler for the specified signal.  The routine figures
 * out what it should be set to.
 */
void
setsignal(int signo, int vforked)
{
	int action;
	sig_t sigact = SIG_DFL, sig;
	char *t, tsig;

	if (traps_invalid || (t = trap[signo]) == NULL)
		action = S_DFL;
	else if (*t != '\0')
		action = S_CATCH;
	else
		action = S_IGN;

	VTRACE(DBG_TRAP, ("setsignal(%d%s) -> %d", signo,
	    vforked ? ", VF" : "", action));
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
			if (rootshell && iflag)
				action = S_IGN;
			break;
#if JOBS
		case SIGTSTP:
		case SIGTTOU:
			if (rootshell && mflag)
				action = S_IGN;
			break;
#endif
		}
	}

	/*
	 * Never let users futz with SIGCHLD
	 * instead we will give them pseudo SIGCHLD's
	 * when background jobs complete.
	 */
	if (signo == SIGCHLD)
		action = S_DFL;

	VTRACE(DBG_TRAP, (" -> %d", action));

	t = &sigmode[signo];
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
			VTRACE(DBG_TRAP, (" getsigaction (%d)\n", errno));
			return;
		}
		VTRACE(DBG_TRAP, (" [%s]%s%s", sigact==SIG_IGN ? "IGN" :
		    sigact==SIG_DFL ? "DFL" : "caught",
		    iflag ? "i" : "", mflag ? "m" : ""));

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
	VTRACE(DBG_TRAP, (" tsig=%d\n", tsig));

	if (tsig == S_HARD_IGN || tsig == action)
		return;

	switch (action) {
		case S_DFL:	sigact = SIG_DFL;	break;
		case S_CATCH:	sigact = onsig;		break;
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
	return;
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
	if (sigmode[signo] == 0)
		setsignal(signo, vforked);

	VTRACE(DBG_TRAP, ("ignoresig(%d%s)\n", signo, vforked ? ", VF" : ""));
	if (sigmode[signo] != S_IGN && sigmode[signo] != S_HARD_IGN) {
		signal(signo, SIG_IGN);
		if (!vforked)
			sigmode[signo] = S_IGN;
	}
}

char *
child_trap(void)
{
	char * p;

	p = trap[SIGCHLD];

	if (traps_invalid || (p != NULL && *p == '\0'))
		p = NULL;

	return p;
}


#ifdef mkinit
INCLUDE <signal.h>
INCLUDE "trap.h"
INCLUDE "shell.h"
INCLUDE "show.h"

SHELLPROC {
	char *sm;

	INTOFF;
	clear_traps(2);
	for (sm = sigmode ; sm < sigmode + NSIG ; sm++) {
		if (*sm == S_IGN) {
			*sm = S_HARD_IGN;
			VTRACE(DBG_TRAP, ("SHELLPROC: %d -> hard_ign\n",
			    (sm - sigmode)));
		}
	}
	INTON;
}
#endif



/*
 * Signal handler.
 */

void
onsig(int signo)
{
	int sav_err = errno;

	CTRACE(DBG_SIG, ("onsig(%d), had: pendingsigs %d%s, gotsig[%d]=%d\n",
	    signo, pendingsigs, intpending ? " (SIGINT-pending)" : "",
	    signo, gotsig[signo]));

	/*			This should not be needed.
	signal(signo, onsig);
	*/

	if (signo == SIGINT && (traps_invalid || trap[SIGINT] == NULL)) {
		VTRACE(DBG_SIG, ("onsig(SIGINT), doing it now\n"));
		if (suppressint && !in_dotrap)
			intpending = 1;
		else
			onint();
		errno = sav_err;
		return;
	}

	/*
	 * if the signal will do nothing, no point reporting it
	 */
	if (!traps_invalid && trap[signo] != NULL && trap[signo][0] != '\0' &&
	    signo != SIGCHLD) {
		gotsig[signo] = 1;
		pendingsigs++;
		if (iflag && signo == SIGINT) {
			if (!suppressint) {
				VTRACE(DBG_SIG,
			   ("onsig: -i gotsig[INT]->%d pendingsigs->%d  BANG\n",
				    gotsig[SIGINT], pendingsigs));
				onint();
				errno = sav_err;
				return;
			}
			intpending = 1;
		}
		VTRACE(DBG_SIG, ("onsig: gotsig[%d]->%d pendingsigs->%d%s\n",
		    signo, gotsig[signo], pendingsigs,
		    intpending ? " (SIGINT pending)":""));
	}
	errno = sav_err;
}



/*
 * Called to execute a trap.  Perhaps we should avoid entering new trap
 * handlers while we are executing a trap handler.
 */

void
dotrap(void)
{
	int i;
	char *tr;
	int savestatus;
	struct skipsave saveskip;

	CTRACE(DBG_TRAP|DBG_SIG, ("dotrap[%d]: %d pending, traps %sinvalid\n",
	    in_dotrap, pendingsigs, traps_invalid ? "" : "not "));

	in_dotrap++;
	for (;;) {
		pendingsigs = 0;
		for (i = 1 ; ; i++) {
			if (i >= NSIG) {
				in_dotrap--;
				VTRACE(DBG_TRAP|DBG_SIG, ("dotrap[%d] done\n",
				    in_dotrap));
				return;
			}
			if (gotsig[i])
				break;
		}
		gotsig[i] = 0;

		if (traps_invalid)
			continue;

		tr = trap[i];

		CTRACE(DBG_TRAP|DBG_SIG, ("dotrap %d: %s%s%s\n", i,
		    tr ? "\"" : "", tr ? tr : "NULL", tr ? "\"" : ""));

		if (tr != NULL) {
			last_trapsig = i;
			save_skipstate(&saveskip);
			savestatus = exitstatus;

			tr = savestr(tr);	/* trap code may free trap[i] */
			evalstring(tr, 0);
			ckfree(tr);

			if (current_skipstate() == SKIPNONE ||
			    saveskip.state != SKIPNONE) {
				restore_skipstate(&saveskip);
				exitstatus = savestatus;
			}
		}
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
	CTRACE(DBG_ERRS|DBG_PROCS|DBG_CMDS|DBG_TRAP,
	    ("pid %d: exitshell(%d)\n", getpid(), status));

	exiting = 1;
	exiting_status = status;
	exitshell_savedstatus();
}

void
exitshell_savedstatus(void)
{
	struct jmploc loc;
	char *p;
	volatile int sig = 0;
	int s;
	sigset_t sigs;

	CTRACE(DBG_ERRS|DBG_PROCS|DBG_CMDS|DBG_TRAP,
	  ("pid %d: exitshell_savedstatus()%s $?=%d xs=%d dt=%d ts=%d\n",
	    getpid(), exiting ? " exiting" : "", exitstatus,
	    exiting_status, in_dotrap, last_trapsig));

	if (!exiting) {
		if (in_dotrap && last_trapsig) {
			sig = last_trapsig;
			exiting_status = sig + 128;
		} else
			exiting_status = exitstatus;
	}
	exitstatus = exiting_status;

	if (pendingsigs && !setjmp(loc.loc)) {
		handler = &loc;

		dotrap();
	}

	if (!setjmp(loc.loc)) {
		handler = &loc;

		if (!traps_invalid && (p = trap[0]) != NULL && *p != '\0') {
			reset_eval();
			trap[0] = NULL;
			VTRACE(DBG_TRAP, ("exit trap: \"%s\"\n", p));
			evalstring(p, 0);
		}
	}

	INTOFF;			/*  we're done, no more interrupts. */

	if (!setjmp(loc.loc)) {
		handler = &loc;		/* probably unnecessary */
		flushall();
#if JOBS
		setjobctl(0);
#endif
	}

	if ((s = sig) != 0 && s != SIGSTOP && s != SIGTSTP && s != SIGTTIN &&
	    s != SIGTTOU) {
		struct rlimit nocore;

		/*
		 * if the signal is of the core dump variety, don't...
		 */
		nocore.rlim_cur = nocore.rlim_max = 0;
		(void) setrlimit(RLIMIT_CORE, &nocore);

		signal(s, SIG_DFL);
		sigemptyset(&sigs);
		sigaddset(&sigs, s);
		sigprocmask(SIG_UNBLOCK, &sigs, NULL);

		kill(getpid(), s);
	}
	_exit(exiting_status);
	/* NOTREACHED */
}
