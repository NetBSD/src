/*	$NetBSD: locore_c.c,v 1.12 2006/01/23 21:45:02 uwe Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *
 *	@(#)Locore.c
 */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992 Terrence R. Lambert.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)Locore.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: locore_c.c,v 1.12 2006/01/23 21:45:02 uwe Exp $");

#include "opt_lockdebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/sched.h>
#include <sys/proc.h>
#include <sys/ras.h>

#include <uvm/uvm_extern.h>

#include <sh3/locore.h>
#include <sh3/cpu.h>
#include <sh3/pmap.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

void (*__sh_switch_resume)(struct lwp *);
struct lwp *cpu_switch_search(struct lwp *);
struct lwp *cpu_switch_prepare(struct lwp *, struct lwp *);
void idle(void);
int want_resched;

#ifdef LOCKDEBUG
#define	SCHED_LOCK_IDLE()	sched_lock_idle()
#define	SCHED_UNLOCK_IDLE()	sched_unlock_idle()
#else
#define	SCHED_LOCK_IDLE()	do {} while (/* CONSTCOND */ 0)
#define	SCHED_UNLOCK_IDLE()	do {} while (/* CONSTCOND */ 0)
#endif


/*
 * Prepare context switch from olwp to nlwp.
 * This code is shared by cpu_switch and cpu_switchto.
 */
struct lwp *
cpu_switch_prepare(struct lwp *olwp, struct lwp *nlwp)
{

	nlwp->l_stat = LSONPROC;

	if (nlwp != olwp) {
		struct proc *p = nlwp->l_proc;

		curpcb = nlwp->l_md.md_pcb;
		pmap_activate(nlwp);

		/* Check for Restartable Atomic Sequences. */
		if (!LIST_EMPTY(&p->p_raslist)) {
			caddr_t pc;

			pc = ras_lookup(p,
				(caddr_t)nlwp->l_md.md_regs->tf_spc);
			if (pc != (caddr_t) -1)
				nlwp->l_md.md_regs->tf_spc = (int) pc;
		}
	}

	curlwp = nlwp;
	return (nlwp);
}

/*
 * Find the highest priority lwp and prepare to switching to it.
 */
struct lwp *
cpu_switch_search(struct lwp *olwp)
{
	struct prochd *q;
	struct lwp *l;

	curlwp = NULL;

	SCHED_LOCK_IDLE();
	while (sched_whichqs == 0) {
		SCHED_UNLOCK_IDLE();
		idle();
		SCHED_LOCK_IDLE();
	}

	q = &sched_qs[ffs(sched_whichqs) - 1];
	l = q->ph_link;
	remrunqueue(l);
	want_resched = 0;
	SCHED_UNLOCK_IDLE();

	return (cpu_switch_prepare(olwp, l));
}

/*
 * void idle(void):
 *	When no processes are on the run queue, wait for something to come
 *	ready. Separated function for profiling.
 */
void
idle()
{

	spl0();
	uvm_pageidlezero();
	__asm volatile("sleep");
	splsched();
}

#ifndef P1_STACK
#ifdef SH3
/*
 * void sh3_switch_setup(struct lwp *l):
 *	prepare kernel stack PTE table. TLB miss handler check these.
 */
void
sh3_switch_setup(struct lwp *l)
{
	pt_entry_t *pte;
	struct md_upte *md_upte = l->l_md.md_upte;
	u_int32_t vpn;
	int i;

	vpn = (u_int32_t)l->l_addr;
	vpn &= ~PGOFSET;
	for (i = 0; i < UPAGES; i++, vpn += PAGE_SIZE, md_upte++) {
		pte = __pmap_kpte_lookup(vpn);
		KDASSERT(pte && *pte != 0);

		md_upte->addr = vpn;
		md_upte->data = (*pte & PG_HW_BITS) | PG_D | PG_V;
	}
}
#endif /* SH3 */

#ifdef SH4
/*
 * void sh4_switch_setup(struct lwp *l):
 *	prepare kernel stack PTE table. sh4_switch_resume wired this PTE.
 */
void
sh4_switch_setup(struct lwp *l)
{
	pt_entry_t *pte;
	struct md_upte *md_upte = l->l_md.md_upte;
	u_int32_t vpn;
	int i, e;

	vpn = (u_int32_t)l->l_addr;
	vpn &= ~PGOFSET;
	e = SH4_UTLB_ENTRY - UPAGES;
	for (i = 0; i < UPAGES; i++, e++, vpn += PAGE_SIZE) {
		pte = __pmap_kpte_lookup(vpn);
		KDASSERT(pte && *pte != 0);
		/* Address array */
		md_upte->addr = SH4_UTLB_AA | (e << SH4_UTLB_E_SHIFT);
		md_upte->data = vpn | SH4_UTLB_AA_D | SH4_UTLB_AA_V;
		md_upte++;
		/* Data array */
		md_upte->addr = SH4_UTLB_DA1 | (e << SH4_UTLB_E_SHIFT);
		md_upte->data = (*pte & PG_HW_BITS) |
		    SH4_UTLB_DA1_D | SH4_UTLB_DA1_V;
		md_upte++;
	}
}
#endif /* SH4 */
#endif /* !P1_STACK */

/*
 * copystr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
int
copystr(const void *kfaddr, void *kdaddr, size_t maxlen, size_t *lencopied)
{
	const char *from = kfaddr;
	char *to = kdaddr;
	int i;

	for (i = 0; i < maxlen; i++) {
		if ((*to++ = *from++) == '\0') {
			if (lencopied)
				*lencopied = i + 1;
			return (0);
		}
	}

	if (lencopied)
		*lencopied = i;

	return (ENAMETOOLONG);
}
