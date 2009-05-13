/*	$NetBSD: sig.c,v 1.1.2.2 2009/05/13 19:19:56 jym Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__RCSID("$NetBSD: sig.c,v 1.1.2.2 2009/05/13 19:19:56 jym Exp $");
#endif /* not lint */

#include <assert.h>
#include <util.h>
#include <sys/queue.h>

#include "rcv.h"
#include "extern.h"
#include "sig.h"

/*
 * Mail -- a mail program
 *
 * Signal routines.
 */

static sig_t sigarray[NSIG];

typedef struct q_entry_s {
	int qe_signo;
	sig_t qe_handler;
	struct q_entry_s *qe_next;
} q_entry_t;

static struct {
	q_entry_t *qe_first;
	q_entry_t **qe_last;
} sigqueue = { NULL, &sigqueue.qe_first };
#define SIGQUEUE_INIT(p)  do {\
	(p)->qe_first = NULL;\
	(p)->qe_last = &((p)->qe_first);\
  } while (/*CONSTCOND*/ 0)

/*
 * The routines alloc_entry() and free_entry() manage the queue
 * elements.
 *
 * Currently, they just assign one element per signo from a fix array
 * as we don't support POSIX signal queues.  We leave them as this may
 * change in the future and the modifications will be isolated.
 */
static q_entry_t *
alloc_entry(int signo)
{
	static q_entry_t entries[NSIG];
	q_entry_t *e;

	/*
	 * We currently only post one signal per signal number, so
	 * there is no need to make this complicated.
	 */
	e = &entries[signo];
	if (e->qe_signo != 0)
		return NULL;

	e->qe_signo = signo;
	e->qe_handler = sigarray[signo];
	e->qe_next = NULL;

	return e;
}

static void
free_entry(q_entry_t *e)
{

	e->qe_signo = 0;
	e->qe_handler = NULL;
	e->qe_next = NULL;
}

/*
 * Attempt to post a signal to the sigqueue.
 */
static void
sig_post(int signo)
{
	q_entry_t *e;

	if (sigarray[signo] == SIG_DFL || sigarray[signo] == SIG_IGN)
		return;

	e = alloc_entry(signo);
	if (e != NULL) {
		*sigqueue.qe_last = e;
		sigqueue.qe_last = &e->qe_next;
	}
}

/*
 * Check the sigqueue for any pending signals.  If any are found,
 * preform the required actions and remove them from the queue.
 */
PUBLIC void
sig_check(void)
{
	q_entry_t *e;
	sigset_t nset;
	sigset_t oset;
	void (*handler)(int);
	int signo;

	(void)sigfillset(&nset);
	(void)sigprocmask(SIG_SETMASK, &nset, &oset);

	while ((e = sigqueue.qe_first) != NULL) {
		signo = e->qe_signo;
		handler = e->qe_handler;

		/*
		 * Remove the entry from the queue and free it.
		 */
		sigqueue.qe_first = e->qe_next;
		if (sigqueue.qe_first == NULL)
			sigqueue.qe_last = &sigqueue.qe_first;
		free_entry(e);

		if (handler == SIG_DFL || handler == SIG_IGN) {
			assert(/*CONSTCOND*/ 0);	/* These should not get posted. */
		}
		else {
			(void)sigprocmask(SIG_SETMASK, &oset, NULL);
			handler(signo);
			(void)sigprocmask(SIG_SETMASK, &nset, NULL);
		}
	}
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
}

PUBLIC sig_t
sig_signal(int signo, sig_t handler)
{
	sig_t old_handler;
	sigset_t nset;
	sigset_t oset;

	assert(signo > 0 && signo < NSIG);

	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, signo);
	(void)sigprocmask(SIG_BLOCK, &nset, &oset);

	old_handler = sigarray[signo];
	sigarray[signo] = handler;

	(void)sigprocmask(SIG_SETMASK, &oset, NULL);

	return old_handler;
}

static void
do_default_handler(int signo, int flags)
{
	struct sigaction nsa;
	struct sigaction osa;
	sigset_t nset;
	sigset_t oset;
	int save_errno;

	save_errno = errno;
	(void)sigemptyset(&nsa.sa_mask);
	nsa.sa_flags = flags;
	nsa.sa_handler = SIG_DFL;
	(void)sigaction(signo, &nsa, &osa);

	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, signo);
	(void)sigprocmask(SIG_UNBLOCK, &nset, &oset);

	(void)kill(0, signo);

	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	(void)sigaction(signo, &osa, NULL);
	errno = save_errno;
}

/*
 * Our generic signal handler.
 */
static void
sig_handler(int signo)
{
	sigset_t nset;
	sigset_t oset;

	(void)sigfillset(&nset);
	(void)sigprocmask(SIG_SETMASK, &nset, &oset);

	assert (signo > 0 && signo < NSIG);	/* Should be guaranteed. */

	sig_post(signo);

	switch (signo) {
	case SIGCONT:
		assert(/*CONSTCOND*/ 0);	/* We should not be seeing these. */
		do_default_handler(signo, 0);
		break;

	case SIGTSTP:
	case SIGTTIN:
	case SIGTTOU:
		do_default_handler(signo, 0);
		break;

	case SIGINT:
	case SIGHUP:
	case SIGQUIT:
	case SIGPIPE:
	default:
		if (sigarray[signo] == SIG_DFL)
			do_default_handler(signo, SA_RESTART);
		break;
	}
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
}

/*
 * Setup the signal handlers.
 */
PUBLIC void
sig_setup(void)
{
	sigset_t nset;
	sigset_t oset;
	struct sigaction sa;
	struct sigaction osa;

	/* Block all signals while setting things. */
	(void)sigfillset(&nset);
	(void)sigprocmask(SIG_BLOCK, &nset, &oset);

	/*
	 * Flow Control - SIGTSTP, SIGTTIN, SIGTTOU, SIGCONT:
	 *
	 * We grab SIGTSTP, SIGTTIN, and SIGTTOU so that we post the
	 * signals before suspending so that they are available when
	 * we resume.  If we were to use SIGCONT instead, they will
	 * not get posted until SIGCONT is unblocked, even though the
	 * process has resumed.
	 *
	 * NOTE: We default these to SA_RESTART here, but we need to
	 * change this in certain cases, e.g., when reading from a
	 * tty.
	 */
	(void)sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sig_handler;
	(void)sigaction(SIGTSTP, &sa, NULL);
	(void)sigaction(SIGTTIN, &sa, NULL);
	(void)sigaction(SIGTTOU, &sa, NULL);

	/*
	 * SIGHUP, SIGINT, and SIGQUIT:
	 *
	 * SIGHUP and SIGINT are trapped unless they are being
	 * ignored.
	 *
	 * Currently, we let the default handler deal with SIGQUIT.
	 */
	(void)sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sig_handler;

	if (sigaction(SIGHUP, &sa, &osa) != -1 && osa.sa_handler == SIG_IGN)
		(void)signal(SIGHUP, SIG_IGN);

	if (sigaction(SIGINT, &sa, &osa) != -1 && osa.sa_handler == SIG_IGN)
		(void)signal(SIGINT, SIG_IGN);
#if 0
	if (signal(SIGQUIT, SIG_DFL) == SIG_IGN)
		(void)signal(SIGQUIT, SIG_IGN);
#endif
	/*
	 * SIGCHLD and SIGPIPE:
	 *
	 * SIGCHLD is setup early in main.  The handler lives in
	 * popen.c as it uses internals of that module.
	 *
	 * SIGPIPE is grabbed here.  It is only used in
	 * lex.c:setup_piping(), cmd1.c:type1(), and cmd1.c:pipecmd().
	 */
	(void)sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sig_handler;
	(void)sigaction(SIGPIPE, &sa, NULL);

	/*
	 * Make sure our structures are initialized.
	 * XXX: This should be unnecessary.
	 */
	(void)memset(sigarray, 0, sizeof(sigarray));
	SIGQUEUE_INIT(&sigqueue);

	/* Restore the signal mask. */
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
}

static struct {		/* data shared by sig_hold() and sig_release() */
	int depth;	/* depth of sig_hold() */
	sigset_t oset;	/* old signal mask saved by sig_hold() */
} hold;

/*
 * Hold signals SIGHUP, SIGINT, and SIGQUIT.
 */
PUBLIC void
sig_hold(void)
{
	sigset_t nset;

	if (hold.depth++ == 0) {
		(void)sigemptyset(&nset);
		(void)sigaddset(&nset, SIGHUP);
		(void)sigaddset(&nset, SIGINT);
		(void)sigaddset(&nset, SIGQUIT);
		(void)sigprocmask(SIG_BLOCK, &nset, &hold.oset);
	}
}

/*
 * Release signals SIGHUP, SIGINT, and SIGQUIT.
 */
PUBLIC void
sig_release(void)
{

	if (--hold.depth == 0)
		(void)sigprocmask(SIG_SETMASK, &hold.oset, NULL);
}

/*
 * Unblock and ignore a signal.
 */
PUBLIC int
sig_ignore(int sig, struct sigaction *osa, sigset_t *oset)
{
	struct sigaction act;
	sigset_t nset;
	int error;

	(void)sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	act.sa_handler = SIG_IGN;
	error = sigaction(sig, &act, osa);

	if (error != -1) {
		(void)sigemptyset(&nset);
		(void)sigaddset(&nset, sig);
		(void)sigprocmask(SIG_UNBLOCK, &nset, oset);
	} else if (oset != NULL)
		(void)sigprocmask(SIG_UNBLOCK, NULL, oset);

	return error;
}

/*
 * Restore a signal and the current signal mask.
 */
PUBLIC int
sig_restore(int sig, struct sigaction *osa, sigset_t *oset)
{
	int error;

	error = 0;
	if (oset)
		error = sigprocmask(SIG_SETMASK, oset, NULL);
	if (osa)
		error = sigaction(sig, osa, NULL);

	return error;
}

/*
 * Change the current flags and (optionally) return the old sigaction
 * structure so we can restore things later.  This is used to turn
 * SA_RESTART on or off.
 */
PUBLIC int
sig_setflags(int signo, int flags, struct sigaction *osa)
{
	struct sigaction sa;

	if (sigaction(signo, NULL, &sa) == -1)
		return -1;
	if (osa)
		*osa = sa;
	sa.sa_flags = flags;
	return sigaction(signo, &sa, NULL);
}

