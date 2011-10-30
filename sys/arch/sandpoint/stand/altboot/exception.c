/* $NetBSD: exception.c,v 1.1 2011/10/30 20:42:09 phx Exp $ */

/*-
 * Copyright (c) 2011 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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

#include <sys/param.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include "globals.h"

#ifdef DEBUG
struct cpu_state {
	uint32_t	gpr[32];
	uint32_t	cr, xer, lr, ctr;
	uint32_t	srr0, srr1, dar, dsisr;
	uint32_t	dmiss, dcmp, hash1, hash2;
	uint32_t	imiss, icmp, rpa;
};

void init_vectors(void);	/* called from early startup */
void exception_handler(unsigned, struct cpu_state *);


void
init_vectors(void)
{
	extern uint8_t trap[], trap_end[];
	uint8_t *exc;
	uint32_t *ip;

	/* install exception handlers */
	for (exc = (uint8_t *)0x100; exc <= (uint8_t *)0x1400; exc += 0x100)
		memcpy(exc, trap, trap_end - trap);

	/* handle NULL-ptr calls from 0x0 to 0xfc */
	for (ip = 0; ip < (uint32_t *)0x100; ip++)
		*ip = 0x48000002 | (uint32_t)trap;

	__syncicache(0, 0x1400);
}

void
exception_handler(unsigned vector, struct cpu_state *st)
{
	static bool in_exception = false;
	uint32_t *fp;
	int i;

	if (vector > 0x1400)
		printf("\nCALL TO NULL POINTER FROM %08x\n", st->lr - 4);
	else
		printf("\nEXCEPTION VECTOR %04x\n", vector);
	if (in_exception) {
		printf("caused in exception handler! Restarting...\n");
		_rtt();
	}
	in_exception = true;

	/* register dump */
	printf("\n SRR0=%08x SRR1=%08x   DAR=%08x DSISR=%08x\n"
	    "   CR=%08x  XER=%08x    LR=%08x   CTR=%08x\n"
	    "DMISS=%08x DCMP=%08x HASH1=%08x HASH2=%08x\n"
	    "IMISS=%08x ICMP=%08x   RPA=%08x\n\n",
	    st->srr0, st->srr1, st->dar, st->dsisr,
	    st->cr, st->xer, st->lr, st->ctr,
	    st->dmiss,st->dcmp,st->hash1,st->hash2,
	    st->imiss,st->icmp,st->rpa);
	for (i = 0; i < 32; i++) {
		if ((i & 7) == 0)
			printf("GPR%02d:", i);
		printf("%c%08x", (i & 7) == 4 ? ':' : ' ', st->gpr[i]);
		if ((i & 7) == 7)
			printf("\n");
	}

	/* V.4-ABI stack frame back trace */
	printf("\nBacktrace:\n");
	i = 0;
	fp = (uint32_t *)st->gpr[1];
	while ((fp = *(uint32_t **)fp) != NULL && i++ < 10)
		printf("%p: called from %08x\n", fp, *(fp + 1) - 4);

	printf("\nHit any key to reboot.\n");
	(void)getchar();
	_rtt();
}

#endif /* DEBUG */
