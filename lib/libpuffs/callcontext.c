/*	$NetBSD: callcontext.c,v 1.5 2007/05/10 12:36:44 pooka Exp $	*/

/*
 * Copyright (c) 2006 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__RCSID("$NetBSD: callcontext.c,v 1.5 2007/05/10 12:36:44 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>

#include <assert.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "puffs_priv.h"

/*
 * user stuff
 */

void
puffs_cc_yield(struct puffs_cc *pcc)
{

	assert((pcc->pcc_flags & PCC_DONE) == 0);
	assert(pcc->pcc_flags & PCC_REALCC);

	/* romanes eunt domus */
	swapcontext(&pcc->pcc_uc, &pcc->pcc_uc_ret);
}

void
puffs_cc_continue(struct puffs_cc *pcc)
{

	assert(pcc->pcc_flags & PCC_REALCC);

	/* ramble on */
	swapcontext(&pcc->pcc_uc_ret, &pcc->pcc_uc);
}

struct puffs_usermount *
puffs_cc_getusermount(struct puffs_cc *pcc)
{

	return pcc->pcc_pu;
}

void *
puffs_cc_getspecific(struct puffs_cc *pcc)
{

	return puffs_getspecific(pcc->pcc_pu);
}

struct puffs_cc *
puffs_cc_create(struct puffs_usermount *pu)
{
	struct puffs_cc *volatile pcc;
	stack_t *st;

	pcc = malloc(sizeof(struct puffs_cc));
	if (!pcc)
		return NULL;
	memset(pcc, 0, sizeof(struct puffs_cc));
	pcc->pcc_pu = pu;
	pcc->pcc_priv = (void *)0xdeadbeef;
	pcc->pcc_flags = PCC_REALCC;

	/* initialize both ucontext's */
	if (getcontext(&pcc->pcc_uc) == -1) {
		free(pcc);
		return NULL;
	}
	if (getcontext(&pcc->pcc_uc_ret) == -1) {
		free(pcc);
		return NULL;
	}
	/* return here.  it won't actually be "here" due to swapcontext() */
	pcc->pcc_uc.uc_link = &pcc->pcc_uc_ret;

	/* allocate stack for execution */
	st = &pcc->pcc_uc.uc_stack;
	st->ss_sp = pcc->pcc_stack = malloc(pu->pu_cc_stacksize);
	if (st->ss_sp == NULL) {
		free(pcc);
		return NULL;
	}
	st->ss_size = pu->pu_cc_stacksize;
	st->ss_flags = 0;

	/*
	 * Give us an initial context to jump to.
	 *
	 * XXX: Our manual page says that portable code shouldn't rely on
	 * being able to pass pointers through makecontext().  kjk says
	 * that NetBSD code doesn't need to worry about this.  uwe says
	 * it would be like putting a "keep away from children" sign on a
	 * box of toys.  I didn't ask what simon says; he's probably busy
	 * "fixing" typos in comments.
	 */
	makecontext(&pcc->pcc_uc, (void *)puffs_calldispatcher,
	    1, (uintptr_t)pcc);

	return pcc;
}

void
puffs_cc_destroy(struct puffs_cc *pcc)
{

	if (pcc->pcc_preq)
		free(pcc->pcc_preq);
	free(pcc->pcc_stack);
	free(pcc);
}
