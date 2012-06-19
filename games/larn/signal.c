/*	$NetBSD: signal.c,v 1.9 2012/06/19 05:30:44 dholland Exp $	*/

/* "Larn is copyrighted 1986 by Noah Morgan.\n" */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: signal.c,v 1.9 2012/06/19 05:30:44 dholland Exp $");
#endif	/* not lint */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "header.h"
#include "extern.h"

static void s2choose(void);
static void cntlc(int);
static void sgam(int);
static void tstop(int);
static void sigpanic(int);

#define BIT(a) (1<<((a)-1))

static void
s2choose(void)
{				/* text to be displayed if ^C during intro
				 * screen */
	cursor(1, 24);
	lprcat("Press ");
	setbold();
	lprcat("return");
	resetbold();
	lprcat(" to continue: ");
	lflush();
}

static void
cntlc(int n)
{				/* what to do for a ^C */
	if (nosignal)
		return;		/* don't do anything if inhibited */
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	quit();
	if (predostuff == 1)
		s2choose();
	else
		showplayer();
	lflush();
	signal(SIGQUIT, cntlc);
	signal(SIGINT, cntlc);
}

/*
 *	subroutine to save the game if a hangup signal
 */
static void
sgam(int n)
{
	savegame(savefilename);
	wizard = 1;
	died(-257);		/* hangup signal */
}

#ifdef SIGTSTP
static void
tstop(int n)
{				/* control Y	 */
	if (nosignal)
		return;		/* nothing if inhibited */
	lcreat((char *) 0);
	clearvt100();
	lflush();
	signal(SIGTSTP, SIG_DFL);
#ifdef SIGVTALRM
	/*
	 * looks like BSD4.2 or higher - must clr mask for signal to take
	 * effect
	 */
	sigsetmask(sigblock(0) & ~BIT(SIGTSTP));
#endif
	kill(getpid(), SIGTSTP);

	setupvt100();
	signal(SIGTSTP, tstop);
	if (predostuff == 1)
		s2choose();
	else
		drawscreen();
	showplayer();
	lflush();
}
#endif	/* SIGTSTP */

/*
 *	subroutine to issue the needed signal traps  called from main()
 */
void
sigsetup(void)
{
	signal(SIGQUIT, cntlc);
	signal(SIGINT, cntlc);
	signal(SIGKILL, SIG_IGN);
	signal(SIGHUP, sgam);
	signal(SIGILL, sigpanic);
	signal(SIGTRAP, sigpanic);
	signal(SIGIOT, sigpanic);
	signal(SIGEMT, sigpanic);
	signal(SIGFPE, sigpanic);
	signal(SIGBUS, sigpanic);
	signal(SIGSEGV, sigpanic);
	signal(SIGSYS, sigpanic);
	signal(SIGPIPE, sigpanic);
	signal(SIGTERM, sigpanic);
#ifdef SIGTSTP
	signal(SIGTSTP, tstop);
	signal(SIGSTOP, tstop);
#endif	/* SIGTSTP */
}

/*
 *	routine to process a fatal error signal
 */
static void
sigpanic(int sig)
{
	char            buf[128];
	signal(sig, SIG_DFL);
	snprintf(buf, sizeof(buf),
	    "\nLarn - Panic! Signal %d received [SIG%s]", sig,
	    sys_signame[sig]);
	write(2, buf, strlen(buf));
	sleep(2);
	sncbr();
	savegame(savefilename);
	kill(getpid(), sig);	/* this will terminate us */
}
