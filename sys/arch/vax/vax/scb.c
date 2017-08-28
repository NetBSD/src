/*	$NetBSD: scb.c,v 1.18.36.1 2017/08/28 17:51:55 skrll Exp $ */
/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Routines for dynamic allocation/release of SCB vectors.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scb.c,v 1.18.36.1 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/trap.h>
#include <machine/scb.h>
#include <machine/frame.h>
#include <machine/sid.h>

struct scb *scb;
struct ivec_dsp *scb_vec;

void scb_stray(void *);
static volatile int vector, ipl, gotintr;

/*
 * Generates a new SCB.
 */
paddr_t
scb_init(paddr_t avail_start)
{
	struct	ivec_dsp **ivec = (struct ivec_dsp **)avail_start;
	struct	ivec_dsp **old = (struct ivec_dsp **)KERNBASE;
	vaddr_t	vavail = avail_start + KERNBASE;
	int	scb_size = dep_call->cpu_scbsz;
	int	i;

	scb = (struct scb *)vavail;
	scb_vec = (struct ivec_dsp *)(vavail + (scb_size * VAX_NBPG));

	/* Init the whole SCB with interrupt catchers */
	for (i = 0; i < (scb_size * VAX_NBPG)/4; i++) {
		ivec[i] = &scb_vec[i];
		*(uintptr_t *)&ivec[i] |= 1; /* On istack, please */
		scb_vec[i] = idsptch;
		scb_vec[i].hoppaddr = scb_stray;
		scb_vec[i].pushlarg = (void *) (i * 4);
		scb_vec[i].ev = NULL;
	}
	/*
	 * Copy all pre-set interrupt vectors to the new SCB.
	 * It is known that these vectors is at KERNBASE from the
	 * beginning, and that if the vector is zero it should call
	 * stray instead.
	 */
	for (i = 0; i < 64; i++)
		if (old[i])
			ivec[i] = old[i];
	/* Last action: set the SCB */
	mtpr(avail_start, PR_SCBB);

	/* Return new avail_start. Also save space for the dispatchers. */
	return avail_start + (1 + sizeof(struct ivec_dsp) / sizeof(void *))
		* scb_size * VAX_NBPG;
};

/*
 * Stray interrupt handler.
 * This function must _not_ save any registers (in the reg save mask).
 */
void
scb_stray(void *arg)
{
	gotintr = 1;
	vector = ((int) arg) & ~3;
	ipl = mfpr(PR_IPL);

	if (cold == 0) {
		printf("stray interrupt: vector 0x%x, ipl %d\n", vector, ipl);
	} else if (dep_call->cpu_flags & CPU_RAISEIPL) {
		struct icallsframe *icf = (void *) __builtin_frame_address(0);

		icf->ica_psl = (icf->ica_psl & ~PSL_IPL) | ipl << 16;
	}

	mtpr(ipl + 1, PR_IPL);
}

/*
 * Fake interrupt handler, to fool some bus' autodetect system.
 * (May I say DW780? :-)
 */
void
scb_fake(int vec, int br)
{
	vector = vec;
	ipl = br;
	gotintr = 1;
}

/*
 * Returns last vector/ipl referenced. Clears vector/ipl after reading.
 */
int
scb_vecref(int *rvec, int *ripl)
{
	int save;

	if (rvec)
		*rvec = vector;
	if (ripl)
		*ripl = ipl;
	save = gotintr;
	gotintr = vector = ipl = 0;
	mtpr(0, PR_IPL);
	return save;
}

/*
 * Sets a vector to the specified function.
 * Arg may not be greater than 63.
 */
void
scb_vecalloc(int vecno, void (*func)(void *), void *arg,
	int stack, struct evcnt *ev)
{
	struct ivec_dsp *dsp = &scb_vec[vecno / 4];
	dsp->hoppaddr = func;
	dsp->pushlarg = arg;
	dsp->ev = ev;
	((intptr_t *) scb)[vecno/4] = (intptr_t)(dsp) | stack;
}
