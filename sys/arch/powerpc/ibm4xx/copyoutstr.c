/*	$NetBSD: copyoutstr.c,v 1.3 2003/02/02 20:43:22 matt Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <uvm/uvm_extern.h>
#include <machine/pcb.h>

int
copyoutstr(const void *kaddr, void *udaddr, size_t len, size_t *done)
{
	struct pmap *pm = curproc->p_vmspace->vm_map.pmap;
	int msr, pid, tmp, ctx;
	struct faultbuf env;

	if (setfault(&env)) {
		curpcb->pcb_onfault = 0;
		/* XXXX -- len may be lost on a fault */
		if (done)
			*done = len;
		return EFAULT;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	if (len) {
		asm volatile("mtctr %3;"		/* Set up counter */
			"mfmsr %0;"			/* Save MSR */
			"li %1,0x20; "
			"andc %1,%0,%1; mtmsr %1;"	/* Disable IMMU */
			"mfpid %1;"			/* Save old PID */
			"sync; isync;"

			"li %3,0;"			/* Clear len */

			"1:"
			"mtpid %1;sync;"
			"lbz %2,0(%6); addi %6,%6,1;"	/* Store kernel byte */
			"sync; isync;"
			"mtpid %4; sync;"		/* Load user ctx */
			"stb %2,0(%5);  dcbf 0,%5; addi %5,%5,1;"	/* Load byte */
			"sync; isync;"
			"addi %3,%3,1;"			/* Inc len */
			"or. %2,%2,%2;"
			"bdnzf 2,1b;"			/*
							 * while(ctr-- && !zero)
							 */

			"2: mtpid %1; mtmsr %0;"	/* Restore PID, MSR */
			"sync; isync;"
			: "=&r" (msr), "=&r" (pid), "=&r" (tmp), "+r" (len)
			: "r" (ctx), "r" (udaddr), "r" (kaddr));
	}
	curpcb->pcb_onfault = 0;
	if (done)
		*done = len;
	return 0;
}
