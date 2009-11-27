/*	$NetBSD: locore_c.c,v 1.31 2009/11/27 03:23:13 rmind Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 2002, 2007 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: locore_c.c,v 1.31 2009/11/27 03:23:13 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/sched.h>
#include <sys/proc.h>
#include <sys/ras.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <sh3/locore.h>
#include <sh3/pmap.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

void cpu_switch_prepare(struct lwp *, struct lwp *);

void (*__sh_switch_resume)(struct lwp *);

/*
 * Called from cpu_switchto after olwp's state has been saved.
 * Prepare context switch to nlwp.
 */
void
cpu_switch_prepare(struct lwp *olwp, struct lwp *nlwp)
{
	struct proc *p = nlwp->l_proc;

	/* Check for Restartable Atomic Sequences. */
	if (p->p_raslist != NULL) {
		void *pc;

		pc = ras_lookup(p, (void *)nlwp->l_md.md_regs->tf_spc);
		if (pc != (void *) -1)
			nlwp->l_md.md_regs->tf_spc = (int) pc;
	}
}


/*
 * When no processes are on the run queue, wait for something to come
 * ready.  Separate function for profiling.
 */
void
cpu_idle(void)
{

	__asm volatile("sleep");
}


#ifndef P1_STACK
#ifdef SH3
/*
 * Nothing to do for sh3.
 */
void
sh3_switch_setup(struct lwp *l)
{

    return;
}
#endif /* SH3 */


#ifdef SH4
/*
 * Prepare kernel stack PTE table.  sh4_switch_resume wires these PTEs.
 */
void
sh4_switch_setup(struct lwp *l)
{
	struct md_upte *md_upte;
	uint32_t vpn;
	pt_entry_t *pte;
	int i, e;

	md_upte = l->l_md.md_upte;
	vpn = sh3_trunc_page(uvm_lwp_getuarea(l));
	e = SH4_UTLB_ENTRY - UPAGES;

	for (i = 0; i < UPAGES; ++i) {
		pte = __pmap_kpte_lookup(vpn);
		KDASSERT(pte && *pte != 0);

		/* Address array */
		md_upte->addr = SH4_UTLB_AA | (e << SH4_UTLB_E_SHIFT);
		md_upte->data = vpn | SH4_UTLB_AA_D | SH4_UTLB_AA_V;
		++md_upte;

		/* Data array */
		md_upte->addr = SH4_UTLB_DA1 | (e << SH4_UTLB_E_SHIFT);
		md_upte->data = (*pte & PG_HW_BITS) |
		    SH4_UTLB_DA1_D | SH4_UTLB_DA1_V;
		++md_upte;

		vpn += PAGE_SIZE;
		++e;
	}
}
#endif /* SH4 */
#endif /* !P1_STACK */


/*
 * Copy a NUL-terminated string, at most maxlen characters long.
 * Return the number of characters copied (including the NUL) in
 * *lencopied.  If the string is too long, return ENAMETOOLONG,
 * else return 0.
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
