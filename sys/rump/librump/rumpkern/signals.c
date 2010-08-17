/*	$NetBSD: signals.c,v 1.2.2.3 2010/08/17 06:48:02 uebayasi Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: signals.c,v 1.2.2.3 2010/08/17 06:48:02 uebayasi Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/event.h>
#include <sys/proc.h>
#include <sys/signal.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

#include "rumpkern_if_priv.h"

const struct filterops sig_filtops;
sigset_t sigcantmask;

/* RUMP_SIGMODEL_PANIC */

static void
rumpsig_panic(pid_t target, int signo)
{

	switch (signo) {
	case SIGSYS:
		break;
	default:
		panic("unhandled signal %d", signo);
	}
}

/* RUMP_SIGMODEL_IGNORE */

static void
rumpsig_ignore(pid_t target, int signo)
{

	return;
}

/* RUMP_SIGMODEL_HOST */

static void
rumpsig_host(pid_t target, int signo)
{
	int error;

	rumpuser_kill(target, signo, &error);
}

/* RUMP_SIGMODEL_RAISE */

static void
rumpsig_raise(pid_t target, int signo)
{
	int error;

	rumpuser_kill(RUMPUSER_PID_SELF, signo, &error);
}

typedef void (*rumpsig_fn)(pid_t pid, int sig);

rumpsig_fn rumpsig = rumpsig_panic;

/*
 * Set signal delivery model.  It would be nice if we could
 * take a functional argument.  But then we'd need some
 * OS independent way to represent a signal number and also
 * a method for easy processing (parsing is boring).
 *
 * Plus, upcalls from the rump kernel into process space except
 * via rumpuser is a somewhat gray area now.
 */
void
rump_boot_setsigmodel(enum rump_sigmodel model)
{

	switch (model) {
	case RUMP_SIGMODEL_PANIC:
		atomic_swap_ptr(&rumpsig, rumpsig_panic);
		break;
	case RUMP_SIGMODEL_IGNORE:
		atomic_swap_ptr(&rumpsig, rumpsig_ignore);
		break;
	case RUMP_SIGMODEL_HOST:
		atomic_swap_ptr(&rumpsig, rumpsig_host);
		break;
	case RUMP_SIGMODEL_RAISE:
		atomic_swap_ptr(&rumpsig, rumpsig_raise);
		break;
	}
}

void
psignal(struct proc *p, int sig)
{

	rumpsig(p->p_pid, sig);
}

void
pgsignal(struct pgrp *pgrp, int sig, int checktty)
{

	rumpsig(-pgrp->pg_id, sig);
}

void
kpsignal(struct proc *p, ksiginfo_t *ksi, void *data)
{

	rumpsig(p->p_pid, ksi->ksi_signo);
}

void
kpgsignal(struct pgrp *pgrp, ksiginfo_t *ksi, void *data, int checkctty)
{

	rumpsig(-pgrp->pg_id, ksi->ksi_signo);
}

int
sigispending(struct lwp *l, int signo)
{

	return 0;
}

void
sigpending1(struct lwp *l, sigset_t *ss)
{

	sigemptyset(ss);
}

int
sigismasked(struct lwp *l, int sig)
{

	return 0;
}

void
sigclearall(struct proc *p, const sigset_t *mask, ksiginfoq_t *kq)
{

	/* nada */
}

void
ksiginfo_queue_drain0(ksiginfoq_t *kq)
{

	if (!(CIRCLEQ_EMPTY(kq)))
		panic("how did that get there?");
}

void
siginit(struct proc *p)
{

	/* nada (?) */
}
