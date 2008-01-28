/*	$NetBSD: callcontext.c,v 1.20 2008/01/28 18:35:49 pooka Exp $	*/

/*
 * Copyright (c) 2006, 2007, 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Research Foundation of Helsinki University of Technology
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
#if !defined(lint)
__RCSID("$NetBSD: callcontext.c,v 1.20 2008/01/28 18:35:49 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/mman.h>

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

#include "puffs_priv.h"

/*
 * Set the following to 1 to not handle each request on a separate
 * stack.  This is highly volatile kludge, therefore no external
 * interface.
 */
int puffs_fakecc;

/*
 * user stuff
 */

/*
 * So, we need to get back to where we came from.  This can happen in two
 * different ways:
 *  1) PCC_MLCONT is set, in which case we need to go to the mainloop
 *  2) It is not set, and we simply jump to pcc_uc_ret.
 */
void
puffs_cc_yield(struct puffs_cc *pcc)
{
	struct puffs_cc *jumpcc;
	int rv;

	assert(puffs_fakecc == 0);

	pcc->pcc_flags &= ~PCC_BORROWED;

	/* romanes eunt domus */
	if ((pcc->pcc_flags & PCC_MLCONT) == 0) {
		swapcontext(&pcc->pcc_uc, &pcc->pcc_uc_ret);
	} else {
		pcc->pcc_flags &= ~PCC_MLCONT;
		rv = puffs__cc_create(pcc->pcc_pu, puffs__theloop, &jumpcc);
		if (rv)
			abort(); /* p-p-p-pa-pa-panic (XXX: fixme) */
		swapcontext(&pcc->pcc_uc, &jumpcc->pcc_uc);
	}
}

/*
 * Internal continue routine.  This has slightly different semantics.
 * We simply make our cc available in the freelist and jump to the 
 * indicated pcc.
 */
void
puffs__cc_cont(struct puffs_cc *pcc)
{
	struct puffs_cc *mycc;

	mycc = puffs_cc_getcc(pcc->pcc_pu);

	/*
	 * XXX: race between setcontenxt() and recycle if
	 * we go multithreaded
	 */
	puffs__cc_destroy(mycc, 1);
	pcc->pcc_flags |= PCC_MLCONT;
	setcontext(&pcc->pcc_uc);
}

void
puffs_cc_continue(struct puffs_cc *pcc)
{

	/* ramble on */
	if (puffs_fakecc)
		pcc->pcc_func(pcc->pcc_farg);
	else 
		swapcontext(&pcc->pcc_uc_ret, &pcc->pcc_uc);
}

/*
 * "Borrows" pcc, *NOT* called from pcc owner.  Acts like continue.
 * So the idea is to use this, give something the context back to
 * run to completion and then jump back to where ever this was called
 * from after the op dispatching is complete (or if the pcc decides to
 * yield again).
 */
void
puffs__goto(struct puffs_cc *loanpcc)
{

	loanpcc->pcc_flags |= PCC_BORROWED;

	swapcontext(&loanpcc->pcc_uc_ret, &loanpcc->pcc_uc);
}

void
puffs_cc_schedule(struct puffs_cc *pcc)
{
	struct puffs_usermount *pu = pcc->pcc_pu;

	assert(pu->pu_state & PU_INLOOP);
	TAILQ_INSERT_TAIL(&pu->pu_sched, pcc, pcc_schedent);
}

int
puffs_cc_getcaller(struct puffs_cc *pcc, pid_t *pid, lwpid_t *lid)
{

	if ((pcc->pcc_flags & PCC_HASCALLER) == 0) {
		errno = ESRCH;
		return -1;
	}

	if (pid)
		*pid = pcc->pcc_pid;
	if (lid)
		*lid = pcc->pcc_lid;
	return 0;
}

static struct puffs_cc fakecc;

static struct puffs_cc *
slowccalloc(struct puffs_usermount *pu)
{
	struct puffs_cc *volatile pcc;
	void *sp;
	size_t stacksize = 1<<pu->pu_cc_stackshift;
	long psize = sysconf(_SC_PAGESIZE);

	if (puffs_fakecc)
		return &fakecc;

	sp = mmap(NULL, stacksize, PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE|MAP_ALIGNED(pu->pu_cc_stackshift), -1, 0);
	if (sp == MAP_FAILED)
		return NULL;

	pcc = sp;
	memset(pcc, 0, sizeof(struct puffs_cc));

	mprotect((uint8_t *)sp + psize, (size_t)psize, PROT_NONE);

	/* initialize both ucontext's */
	if (getcontext(&pcc->pcc_uc) == -1) {
		munmap(pcc, stacksize);
		return NULL;
	}
	if (getcontext(&pcc->pcc_uc_ret) == -1) {
		munmap(pcc, stacksize);
		return NULL;
	}

	return pcc;
}

int
puffs__cc_create(struct puffs_usermount *pu, puffs_ccfunc func,
	struct puffs_cc **pccp)
{
	struct puffs_cc *pcc;
	size_t stacksize = 1<<pu->pu_cc_stackshift;
	stack_t *st;

	/* Do we have a cached copy? */
	if (pu->pu_cc_nstored == 0) {
		pcc = slowccalloc(pu);
		if (pcc == NULL)
			return -1;
		pcc->pcc_pu = pu;
	} else {
		pcc = LIST_FIRST(&pu->pu_ccmagazin);
		assert(pcc != NULL);

		LIST_REMOVE(pcc, pcc_rope);
		pu->pu_cc_nstored--;
	}
	assert(pcc->pcc_pu == pu);

	if (puffs_fakecc) {
		pcc->pcc_func = func;
		pcc->pcc_farg = pcc;
	} else {
		/* link context */
		pcc->pcc_uc.uc_link = &pcc->pcc_uc_ret;

		/* setup stack
		 *
		 * XXX: I guess this should theoretically be preserved by
		 * swapcontext().  However, it gets lost.  So reinit it.
		 */
		st = &pcc->pcc_uc.uc_stack;
		st->ss_sp = pcc;
		st->ss_size = stacksize;
		st->ss_flags = 0;

		/*
		 * Give us an initial context to jump to.
		 *
		 * Our manual page says that portable code shouldn't
		 * rely on being able to pass pointers through makecontext().
		 * kjk says that NetBSD code doesn't need to worry about this.
		 * uwe says it would be like putting a "keep away from
		 * children" sign on a box of toys.
		 */
		makecontext(&pcc->pcc_uc, (void *)func, 1, (uintptr_t)pcc);
	}

	*pccp = pcc;
	return 0;
}

void
puffs__cc_setcaller(struct puffs_cc *pcc, pid_t pid, lwpid_t lid)
{

	pcc->pcc_pid = pid;
	pcc->pcc_lid = lid;
	pcc->pcc_flags |= PCC_HASCALLER;
}

void
puffs__cc_destroy(struct puffs_cc *pcc, int nonuke)
{
	struct puffs_usermount *pu = pcc->pcc_pu;
	size_t stacksize = 1<<pu->pu_cc_stackshift;

	assert(pcc->pcc_flags = 0);

	/* not over limit?  stuff away in the store, otherwise nuke */
	if (nonuke || pu->pu_cc_nstored < PUFFS_CCMAXSTORE) {
		pcc->pcc_pb = NULL;
		LIST_INSERT_HEAD(&pu->pu_ccmagazin, pcc, pcc_rope);
		pu->pu_cc_nstored++;
	} else {
		assert(!puffs_fakecc);
		munmap(pcc, stacksize);
	}
}

struct puffs_cc *
puffs_cc_getcc(struct puffs_usermount *pu)
{
	size_t stacksize = 1<<pu->pu_cc_stackshift;
	uintptr_t bottom;

	if (puffs_fakecc)
		return &fakecc;

	bottom = ((uintptr_t)&bottom) & ~(stacksize-1);
	return (struct puffs_cc *)bottom;
}
