/*	$NetBSD: ipkdb_glue.c,v 1.5 2003/07/15 02:54:48 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ipkdb_glue.c,v 1.5 2003/07/15 02:54:48 lukem Exp $");

#include <sys/cdefs.h>
#include <sys/param.h>

#include <ipkdb/ipkdb.h>

#include <machine/frame.h>
#include <machine/ipkdb.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/vmparam.h>

int ipkdbregs[NREG];

int ipkdb_trap_glue __P((struct trapframe *));

#ifdef	IPKDBUSERHACK
int ipkdbsr;			/* TEMPRORARY (Really needs some better mechanism)	XXX */
int savesr;
#endif

void
ipkdbinit()
{
}

int
ipkdb_poll()
{
	/* for now: */
	return 0;
}

int
ipkdb_trap_glue(frame)
	struct trapframe *frame;
{
	if (!(frame->srr1 & PSL_PR)
	    && (frame->exc == EXC_TRC
		|| (frame->exc == EXC_PGM
		    && (frame->srr1 & 0x20000))
		|| frame->exc == EXC_BPT)) {
#ifdef	IPKDBUSERHACK
		/* XXX see above */
		asm ("mfsr %0,%1" : "=r"(savesr) : "n"(USER_SR));
#endif
		ipkdbzero(ipkdbregs, sizeof ipkdbregs);
		ipkdbcopy(frame->fixreg, &ipkdbregs[FIX], NFIX * sizeof(int));
		ipkdbregs[PC] = frame->srr0;
		ipkdbregs[PS] = frame->srr1 & ~PSL_BE;
		ipkdbregs[CR] = frame->cr;
		ipkdbregs[LR] = frame->lr;
		ipkdbregs[CTR] = frame->ctr;
		ipkdbregs[XER] = frame->xer;

		switch (ipkdbcmds()) {
		case 2:
		case 0:
			ipkdbregs[PS] &= ~PSL_SE;
			break;
		case 1:
			ipkdbregs[PS] |= PSL_SE;
			break;
		}
		ipkdbcopy(&ipkdbregs[FIX], frame->fixreg, NFIX * sizeof(int));
		frame->srr0 = ipkdbregs[PC];
		frame->srr1 = ipkdbregs[PS];
		frame->cr = ipkdbregs[CR];
		frame->lr = ipkdbregs[LR];
		frame->ctr = ipkdbregs[CTR];
		frame->xer = ipkdbregs[XER];
#ifdef	IPKDBUSERHACK
		asm ("mtsr %0,%1; isync" :: "n"(USER_SR), "r"(savesr));
#endif
		return 1;
	}
	return 0;
}
