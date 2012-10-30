/*	$NetBSD: intr.c,v 1.32.8.1 2012/10/30 17:18:57 yamt Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * All rights reserved.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Soft interrupt and other generic interrupt functions.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.32.8.1 2012/10/30 17:18:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>
 
u_int spl_masks[NIPL];

extern u_int irqmasks[];

void
set_spl_masks(void)
{
	int loop;

	for (loop = 0; loop < NIPL; ++loop) {
		spl_masks[loop] = 0xffffffff;
	}

	spl_masks[IPL_VM]	  = irqmasks[IPL_VM];
	spl_masks[IPL_SCHED]	  = irqmasks[IPL_SCHED];
	spl_masks[IPL_HIGH]	  = irqmasks[IPL_HIGH];
	spl_masks[IPL_SOFTSERIAL] = irqmasks[IPL_SOFTSERIAL];
	spl_masks[IPL_SOFTNET]	  = irqmasks[IPL_SOFTNET];
	spl_masks[IPL_SOFTBIO]	  = irqmasks[IPL_SOFTBIO];
	spl_masks[IPL_SOFTCLOCK]  = irqmasks[IPL_SOFTCLOCK];
	spl_masks[IPL_NONE]	  = irqmasks[IPL_NONE];

}

#ifdef DIAGNOSTIC
void
dump_spl_masks(void)
{
	int loop;

	for (loop = 0; loop < NIPL; ++loop)
		printf("spl_masks[%d]=%08x\n", loop, spl_masks[loop]);
}
#endif

/* End of intr.c */
