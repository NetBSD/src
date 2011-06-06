/*	$NetBSD: copyoutstr.c,v 1.1.4.1 2011/06/06 09:06:25 jruoho Exp $	*/

/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: copyoutstr.c,v 1.1.4.1 2011/06/06 09:06:25 jruoho Exp $");

#include <sys/param.h>
#include <sys/lwp.h>

#include <machine/pcb.h>

int
copyoutstr(const void *kaddr, void *udaddr, size_t len, size_t *done)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	register_t msr, ds_msr, data;
	struct faultbuf env;

	if (__predict_false(len == 0)) {
		if (done)
			*done = 0;
		return 0;
	}

	if (setfault(&env)) {
		pcb->pcb_onfault = NULL;
		/* XXXX -- len may be lost on a fault */
		if (done)
			*done = len;
		return EFAULT;
	}

	__asm volatile(
		"mtctr	%[len]; "			/* Set up counter */
		"mfmsr	%[msr]; "			/* Save MSR */
		"ori	%[ds_msr],%[msr],%[DS]; "	/* Gen MSR[DS]=1 */
		"li	%[len],0; "			/* Clear len */

		"1: "
		"lbzx	%[data],%[len],%[kaddr]; "	/* Load kernel byte */
		"mtmsr	%[ds_msr]; sync; isync; "	/* DS on */
		"stbx	%[data],%[len],%[udaddr]; "	/* Store user byte */
		"mtmsr	%[msr]; sync; isync; "		/* DS off */
		"addi	%[len],%[len],1; "		/* Inc len */
		"or.	%[data],%[data],%[data]; "
		"bdnzf	%%cr2,1b; "		/* while(ctr-- && !zero) */
	    : [msr] "=&r" (msr), [ds_msr] "=&r" (ds_msr),
	      [data] "=&r" (data), [len] "+b" (len)
	    : [udaddr] "b" (udaddr), [kaddr] "b" (kaddr), [DS] "K" (PSL_DS)
	    : "ctr", "cr2");

	pcb->pcb_onfault = NULL;
	if (done)
		*done = len;
	return 0;
}
