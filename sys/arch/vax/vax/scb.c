/*	$NetBSD: scb.c,v 1.2 1999/01/20 07:32:52 ragge Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/trap.h>
#include <machine/scb.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/mtpr.h>

static	void scb_stray __P((int));

static	struct ivec_dsp *scb_vec;
static	volatile int vector, ipl, gotintr;
/*
 * Generates a new SCB.
 */
paddr_t
scb_init(avail_start)
	paddr_t avail_start;
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
		memcpy(&scb_vec[i], &idsptch, sizeof(struct ivec_dsp));
		scb_vec[i].hoppaddr = scb_stray;
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
	return avail_start + (scb_size * 5) * VAX_NBPG;
};

/*
 * Stray interrupt handler.
 * This function must _not_ save any registers (in the reg save mask).
 */
void
scb_stray(arg)
	int arg;
{
	struct	callsframe *cf = FRAMEOFFSET(arg);

	gotintr = 1;
	vector = ((cf->ca_pc - (u_int)scb_vec)/4) & ~3;
	ipl = mfpr(PR_IPL);
	if (cold == 0)
		printf("stray interrupt: vector 0x%x, ipl %d\n", vector, ipl);
#ifdef DEBUG
	else
		printf("config interrupt: vector 0x%x, ipl %d\n", vector, ipl);
#endif
}

/*
 * Fake interrupt handler, to fool some bus' autodetect system.
 * (May I say DW780? :-)
 */
void
scb_fake(vec, br)
	int vec, br;
{
	vector = vec;
	ipl = br;
	gotintr = 1;
}

/*
 * Returns last vector/ipl referenced. Clears vector/ipl after reading.
 */
int
scb_vecref(rvec, ripl)
	int *rvec, *ripl;
{
	if (rvec)
		*rvec = vector;
	if (ripl)
		*ripl = ipl;
	vector = ipl = 0;
	return gotintr;
}

/*
 * Sets a vector to the specified function.
 * Arg may not be greater than 63.
 */
void
scb_vecalloc(vecno, func, arg, stack)
	int vecno;
	void (*func) __P((int));
	int arg, stack;
{
	u_int *iscb = (u_int *)scb; /* XXX */
#ifdef DIAGNOSTIC
	if ((unsigned)arg > 63)
		panic("scb_vecalloc: vecno 0x%x func %p arg %d",
		    vecno, func, arg);
#endif
	scb_vec[vecno/4].pushlarg = arg;
	scb_vec[vecno/4].hoppaddr = func;
	iscb[vecno/4] = (u_int)(&scb_vec[vecno/4]) | stack;
}
