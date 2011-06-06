/*	$NetBSD: ipkdb_glue.c,v 1.10.6.1 2011/06/06 09:06:30 jruoho Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipkdb_glue.c,v 1.10.6.1 2011/06/06 09:06:30 jruoho Exp $");

#include <sys/param.h>

#include <ipkdb/ipkdb.h>

#include <machine/frame.h>
#include <machine/ipkdb.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/vmparam.h>

int ipkdbregs[NREG];

int ipkdb_trap_glue(struct trapframe *);

#ifdef	IPKDBUSERHACK
int ipkdbsr;			/* TEMPRORARY (Really needs some better mechanism)	XXX */
int savesr;
#endif

void
ipkdbinit(void)
{
}

int
ipkdb_poll(void)
{
	/* for now: */
	return 0;
}

int
ipkdb_trap_glue(struct trapframe *tf)
{
	if (!(tf->tf_srr1 & PSL_PR)
	    && (tf->tf_exc == EXC_TRC
		|| (tf->tf_exc == EXC_PGM
		    && (tf->tf_srr1 & 0x20000))
		|| tf->tf_exc == EXC_BPT)) {
#ifdef	IPKDBUSERHACK
		/* XXX see above */
		__asm ("mfsr %0,%1" : "=r"(savesr) : "n"(USER_SR));
#endif
		ipkdbzero(ipkdbregs, sizeof ipkdbregs);
		ipkdbcopy(tf->tf_fixreg, &ipkdbregs[FIX], NFIX * sizeof(int));
		ipkdbregs[PC] = tf->tf_srr0;
		ipkdbregs[PS] = tf->tf_srr1 & ~PSL_BE;
		ipkdbregs[CR] = tf->tf_cr;
		ipkdbregs[LR] = tf->tf_lr;
		ipkdbregs[CTR] = tf->tf_ctr;
		ipkdbregs[XER] = tf->tf_xer;

		switch (ipkdbcmds()) {
		case 2:
		case 0:
			ipkdbregs[PS] &= ~PSL_SE;
			break;
		case 1:
			ipkdbregs[PS] |= PSL_SE;
			break;
		}
		ipkdbcopy(&ipkdbregs[FIX], tf->tf_fixreg, NFIX * sizeof(int));
		tf->tf_srr0 = ipkdbregs[PC];
		tf->tf_srr1 = ipkdbregs[PS];
		tf->tf_cr = ipkdbregs[CR];
		tf->tf_lr = ipkdbregs[LR];
		tf->tf_ctr = ipkdbregs[CTR];
		tf->tf_xer = ipkdbregs[XER];
#ifdef	IPKDBUSERHACK
		__asm ("mtsr %0,%1; isync" :: "n"(USER_SR), "r"(savesr));
#endif
		return 1;
	}
	return 0;
}
