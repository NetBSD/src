/*	$NetBSD: threads.c,v 1.13 2010/10/29 15:32:24 pooka Exp $	*/

/*
 * Copyright (c) 2007-2009 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by
 * The Finnish Cultural Foundation.
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
__KERNEL_RCSID(0, "$NetBSD: threads.c,v 1.13 2010/10/29 15:32:24 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/stdarg.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

struct kthdesc {
	void (*f)(void *);
	void *arg;
	struct lwp *mylwp;
};

static void *
threadbouncer(void *arg)
{
	struct kthdesc *k = arg;
	struct lwp *l = k->mylwp;
	void (*f)(void *);
	void *thrarg;

	f = k->f;
	thrarg = k->arg;

	/* schedule ourselves */
	rumpuser_set_curlwp(l);
	rump_schedule();

	/* free dance struct */
	free(k, M_TEMP);

	if ((curlwp->l_pflag & LP_MPSAFE) == 0)
		KERNEL_LOCK(1, NULL);

	f(thrarg);

	panic("unreachable, should kthread_exit()");
}

int
kthread_create(pri_t pri, int flags, struct cpu_info *ci,
	void (*func)(void *), void *arg, lwp_t **newlp, const char *fmt, ...)
{
	char thrstore[MAXCOMLEN];
	const char *thrname = NULL;
	va_list ap;
	struct kthdesc *k;
	struct lwp *l;
	int rv;

	thrstore[0] = '\0';
	if (fmt) {
		va_start(ap, fmt);
		vsnprintf(thrstore, sizeof(thrstore), fmt, ap);
		va_end(ap);
		thrname = thrstore;
	}

	/*
	 * We don't want a module unload thread.
	 * (XXX: yes, this is a kludge too, and the kernel should
	 * have a more flexible method for configuring which threads
	 * we want).
	 */
	if (strcmp(thrstore, "modunload") == 0) {
		return 0;
	}

	if (!rump_threads) {
		/* fake them */
		if (strcmp(thrstore, "vrele") == 0) {
			printf("rump warning: threads not enabled, not starting"
			   " vrele thread\n");
			return 0;
		} else if (strcmp(thrstore, "cachegc") == 0) {
			printf("rump warning: threads not enabled, not starting"
			   " namecache g/c thread\n");
			return 0;
		} else if (strcmp(thrstore, "nfssilly") == 0) {
			printf("rump warning: threads not enabled, not enabling"
			   " nfs silly rename\n");
			return 0;
		} else if (strcmp(thrstore, "unpgc") == 0) {
			printf("rump warning: threads not enabled, not enabling"
			   " UNP garbage collection\n");
			return 0;
		} else if (strncmp(thrstore, "pmf", sizeof("pmf")-1) == 0) {
			printf("rump warning: threads not enabled, not enabling"
			   " pmf thread\n");
			return 0;
		} else if (strncmp(thrstore, "xcall", sizeof("xcall")-1) == 0) {
			printf("rump warning: threads not enabled, CPU xcall"
			   " not functional\n");
			return 0;
		} else
			panic("threads not available, setenv RUMP_THREADS 1");
	}
	KASSERT(fmt != NULL);

	k = malloc(sizeof(*k), M_TEMP, M_WAITOK);
	k->f = func;
	k->arg = arg;
	k->mylwp = l = rump__lwproc_alloclwp(&proc0);
	l->l_flag |= LW_SYSTEM;
	if (flags & KTHREAD_MPSAFE)
		l->l_pflag |= LP_MPSAFE;
	if (flags & KTHREAD_INTR)
		l->l_pflag |= LP_INTR;
	if (ci) {
		l->l_pflag |= LP_BOUND;
		l->l_target_cpu = ci;
	}
	if (thrname) {
		l->l_name = kmem_alloc(MAXCOMLEN, KM_SLEEP);
		strlcpy(l->l_name, thrname, MAXCOMLEN);
	}
		
	rv = rumpuser_thread_create(threadbouncer, k, thrname,
	    (flags & KTHREAD_JOINABLE) == KTHREAD_JOINABLE, &l->l_ctxlink);
	if (rv)
		return rv;

	if (newlp) {
		*newlp = l;
	} else {
		KASSERT((flags & KTHREAD_JOINABLE) == 0);
	}

	return 0;
}

void
kthread_exit(int ecode)
{

	if ((curlwp->l_pflag & LP_MPSAFE) == 0)
		KERNEL_UNLOCK_LAST(NULL);
	rump_lwproc_releaselwp();
	/* unschedule includes membar */
	rump_unschedule();
	rumpuser_thread_exit();
}

int
kthread_join(struct lwp *l)
{
	int rv;

	KASSERT(l->l_ctxlink != NULL);
	rv = rumpuser_thread_join(l->l_ctxlink);
	membar_consumer();

	return rv;
}
