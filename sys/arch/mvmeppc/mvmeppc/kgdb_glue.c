/*	$NetBSD: kgdb_glue.c,v 1.1.2.2 2002/02/28 04:11:08 nathanw Exp $	*/

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
#include <sys/param.h>

#include <kgdb/kgdb.h>

#include <machine/frame.h>
#include <machine/kgdb.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/trap.h>

int kgdbregs[NREG];

#ifdef	KGDBUSERHACK
int kgdbsr;			/* TEMPRORARY (Really needs some better mechanism)	XXX */
int savesr;
#endif

void
kgdbinit()
{
}

int
kgdb_poll()
{
	/* for now: */
	return 0;
}

int
kgdb_trap_glue(frame)
	struct trapframe *frame;
{
	if (!(frame->srr1 & PSL_PR)
	    && (frame->exc == EXC_TRC
		|| (frame->exc == EXC_PGM
		    && (frame->srr1 & 0x20000))
		|| frame->exc == EXC_BPT)) {
#ifdef	KGDBUSERHACK
		asm ("mfsr %0,%1" : "=r"(savesr) : "K"(USER_SR)); /* see above		XXX */
#endif
		kgdbcopy(frame, kgdbregs, sizeof kgdbregs);
		kgdbregs[MSR] &= ~PSL_BE;

		switch (kgdbcmds()) {
		case 2:
		case 0:
			kgdbregs[MSR] &= ~PSL_SE;
			break;
		case 1:
			kgdbregs[MSR] |= PSL_SE;
			break;
		}
		kgdbcopy(kgdbregs, frame, sizeof kgdbregs);
#ifdef	KGDBUSERHACK
		asm ("mtsr %0,%1; isync" :: "K"(USER_SR), "r"(savesr));
#endif
		return 1;
	}
	return 0;
}
