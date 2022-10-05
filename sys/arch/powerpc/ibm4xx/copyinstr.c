/*	$NetBSD: copyinstr.c,v 1.24 2022/10/05 08:18:00 rin Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: copyinstr.c,v 1.24 2022/10/05 08:18:00 rin Exp $");

#include <sys/param.h>
#include <uvm/uvm_extern.h>
#include <powerpc/ibm4xx/spr.h>
#include <machine/pcb.h>

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	struct pmap *pm = curproc->p_vmspace->vm_map.pmap;
	size_t resid;
	int rv, msr, pid, tmp, ctx;
	struct faultbuf env;

	if (__predict_false(len == 0)) {
		if (done)
			*done = 0;
		return 0;
	}

	if ((rv = setfault(&env))) {
		curpcb->pcb_onfault = NULL;
		if (done)
			*done = 0;
		return rv;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	resid = len;
	__asm volatile (
		"mtctr	%[resid];"		/* Set up counter */
		"mfmsr	%[msr];"		/* Save MSR */
		"li	%[tmp],0x20;"		/* Disable IMMU */
		"andc	%[tmp],%[msr],%[tmp];"
		"mtmsr	%[tmp];"
		"isync;"
		MFPID(%[pid])			/* Save old PID */

	"1:"	MTPID(%[ctx])			/* Load user ctx */
		"isync;"
		"lbz	%[tmp],0(%[uaddr]);"	/* Load byte */
		"addi	%[uaddr],%[uaddr],1;"
		"sync;"

		MTPID(%[pid])
		"isync;"
		"stb	%[tmp],0(%[kaddr]);"	/* Store kernel byte */
		"addi	%[kaddr],%[kaddr],1;"
		"or.	%[tmp],%[tmp],%[tmp];"
		"sync;"
		"bdnzf	eq,1b;"			/* while(ctr-- && !zero) */

		"mtmsr	%[msr];"		/* Restore MSR */
		"isync;"
		"mfctr	%[resid];"		/* Restore resid */

		: [msr] "=&r" (msr), [pid] "=&r" (pid), [tmp] "=&r" (tmp),
		  [resid] "+r" (resid)
		: [ctx] "r" (ctx), [uaddr] "b" (uaddr), [kaddr] "b" (kaddr)
		: "cr0", "ctr");

	curpcb->pcb_onfault = NULL;
	if (done)
		*done = len - resid;
	if (resid == 0 && (char)tmp != '\0')
		return ENAMETOOLONG;
	else
		return 0;
}
