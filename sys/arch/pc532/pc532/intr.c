/*	$NetBSD: intr.c,v 1.18 1997/03/21 08:34:57 matthias Exp $	*/

/*
 * Copyright (c) 1994 Matthias Pfaller.
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
 *	This product includes software developed by Matthias Pfaller.
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

#define DEFINE_SPLX
#include <sys/param.h>
#include <sys/vmmeter.h>
#include <sys/systm.h>

#include <machine/psl.h>

#define INTS	32
struct iv ivt[INTS];
static int next_sir = SOFTINT;
unsigned int imask[NIPL] = {0xffffffff, 0xffffffff};
unsigned int Cur_pl = 0xffffffff, sirpending, astpending;

static void badhard __P((void *));
static void badsoft __P((void *));

/*
 * Initialize the interrupt system.
 */
void
intr_init()
{
	int i, sir;

	/* We don't use the ICU in vectored mode but set this anyway. */
	ICUB(SVCT) = VEC_ICU;

	/* ICU is not cascaded. */	
	ICUW(CSRC) = 0;

	/* We are using the ICU in fixed priority mode. */
	ICUB(MCTL) = 2;

	/* Initialize all interrupts to FALLING_EDGE. */
	ICUW(TPL) = 0;
	ICUW(ELTG) = 0;

	/* Reset first priority register. */
	ICUB(FPRT) = 0;

	/* Reset interrupt in-service register. */
	ICUW(ISRV) = 0;

	for (i = 0; i < 16; i++) {
		ivt[i].iv_vec = badhard;
		ivt[i].iv_level = IPL_ZERO;
	}

	for (i = 16; i < 32; i++) {
		ivt[i].iv_vec = badsoft;
		ivt[i].iv_arg = (void *)i;
		ivt[i].iv_level = IPL_ZERO;
	}

	/*
	 * Initialize the software interrupt system.
	 * To trigger a software interrupt, the corresponding bit is
	 * set in sir_pending. Then an unused ICU interrupt (IR_SOFT)
	 * is used to call check_sir as soon as the current priority
	 * level drops to spl0. check_sir then calls all handlers that
	 * have their bit set in sir_pending.
	 */

	/* Install check_sir as the handler for IR_SOFT. */
	sir = intr_establish(IR_SOFT, check_sir, NULL,
				"softint", IPL_ZERO, IPL_ZERO, LOW_LEVEL);
	if (sir != IR_SOFT)
		panic("Could not allocate IR_SOFT");

	/* Disable IR_SOFT in all priority levels other then IPL_ZERO. */
	for (i = 1; i < NIPL; i++)
		imask[i] |= 1 << IR_SOFT;
}

/*
 * Handle pending software interrupts.
 */
void
check_sir(arg)
	void *arg;
{
	register unsigned int cirpending, mask;
	register struct iv *iv;

#ifdef DIAGNOSTIC
	if (Cur_pl != (imask[IPL_ZERO] | (1 << IR_SOFT)))
		panic("check_sir called with ipl > 0");
#endif

	di();
	while ((cirpending = sirpending) != 0) {
		sirpending &= ~SIR_ALLMASK;
		ei();
		for (iv = ivt + SOFTINT, mask = 0x10000;
		     cirpending & -mask; mask <<= 1, iv++) {
			if ((cirpending & mask) != 0) {
				register int s;
				cnt.v_soft++;
				iv->iv_cnt++;
				s = splraise(iv->iv_mask);
				iv->iv_vec(iv->iv_arg);
				splx(s);
			}
		}
		di();
	}
	/* Avoid unneeded calls to check_sir. */
	clrsofticu(IR_SOFT);
	ei();
	return;
}

/*
 * Establish an interrupt. If intr is set to SOFTINT, a software interrupt
 * is allocated.
 */
int
intr_establish(intr, vector, arg, use, blevel, rlevel, mode)
	int intr;
	void (*vector) __P((void *));
	void *arg;
	char *use;
	int blevel;
	int rlevel;
	int mode;
{
	int i;

	di();
	if (rlevel < IPL_ZERO || rlevel >= NIPL ||
	    blevel < IPL_ZERO || blevel >= NIPL)
		panic("Illegal interrupt level for %s in intr_establish", use);
	if (intr == SOFTINT) {
		if (next_sir >= INTS)
			panic("No software interrupts left");
		intr = next_sir++;
	} else {
		if (ivt[intr].iv_vec != badhard)
			panic("Interrupt %d already allocated\n", intr);
		switch (mode) {
		case RISING_EDGE:
			ICUW(TPL)  |=  (1 << intr);
			ICUW(ELTG) &= ~(1 << intr);
			break;
		case FALLING_EDGE:
			ICUW(TPL)  &= ~(1 << intr);
			ICUW(ELTG) &= ~(1 << intr);
			break;
		case HIGH_LEVEL:
			ICUW(TPL)  |=  (1 << intr);
			ICUW(ELTG) |=  (1 << intr);
			break;
		case LOW_LEVEL:
			ICUW(TPL)  &= ~(1 << intr);
			ICUW(ELTG) |=  (1 << intr);
			break;
		default:
			panic("Unknown interrupt mode");
		}
	}
	ivt[intr].iv_vec   = vector;
	ivt[intr].iv_arg   = arg;
	ivt[intr].iv_cnt   = 0;
	ivt[intr].iv_use   = use;
	ivt[intr].iv_level = rlevel;
	ei();
	imask[blevel] |= 1 << intr;

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, statclock > (tty | net | bio).
	 */
	imask[IPL_CLOCK] |= imask[IPL_TTY] | imask[IPL_NET] | imask[IPL_BIO];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY] | imask[IPL_NET] | imask[IPL_BIO];

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_TTY] |= imask[IPL_NET] | imask[IPL_BIO];
	imask[IPL_NET] |= imask[IPL_BIO];

	/*
	 * Enable this interrupt for spl0.
	 */
	imask[IPL_ZERO] &= ~(1 << intr);

	/*
	 * Update run masks for all handlers.
	 */
	for (i = 0; i < INTS; i++)
		ivt[i].iv_mask = imask[ivt[i].iv_level] |
				 (1 << i) | (1 << IR_SOFT);

	return(intr);
}

/*
 * Default hardware interrupt handler
 */
static void
badhard(arg)
	void *arg;
{
	struct intrframe *frame = arg;
	static int bad_count = 0;
	di();
	bad_count++;
	if (bad_count < 5)
   		printf("Unknown hardware interrupt: vec=%ld pc=0x%08x psr=0x%04x cpl=0x%08lx\n",
		      frame->if_vec, frame->if_regs.r_pc, frame->if_regs.r_psr, frame->if_pl);

	if (bad_count == 5)
		printf("Too many unknown hardware interrupts, quitting reporting them.\n");
	ei();
}

/*
 * Default software interrupt handler
 */
static void
badsoft(arg)
	void *arg;
{
	static int bad_count = 0;
	bad_count++;
	if (bad_count < 5)
		printf("Unknown software interrupt: vec=%d\n", (int)arg);

	if (bad_count == 5)
		printf("Too many unknown software interrupts, quitting reporting them.\n");
}
