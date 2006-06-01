/*	$NetBSD: intr.c,v 1.30.6.1 2006/06/01 22:35:08 kardel Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.30.6.1 2006/06/01 22:35:08 kardel Exp $");

#define DEFINE_SPLX
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/psl.h>

#define INTS	32
struct iv ivt[INTS];
static int next_sir = SOFTINT;
unsigned int imask[NIPL] = {0xffffffff, 0xffffffff};
unsigned int Cur_pl = 0xffffffff, sirpending, astpending;

static void badhard(void *);
static void badsoft(void *);

/*
 * Initialize the interrupt system.
 */
void
intr_init(void)
{
	int i, sir;
	static u_char icu_table[] = {
		ISRV,	0,	/* Reset interrupt in-service register. */
		ISRV+1,	0,
		CSRC,	0,	/* ICU is not cascaded. */
		CSRC+1,	0,
		FPRT,	0,	/* Reset first priority register. */
		TPL,	0,	/* Initialize all interrupts to FALLING_EDGE. */
		TPL+1,	0,
		ELTG,	0,
		ELTG+1,	0,
		MCTL,	2,	/* Set ICU to fixed priority mode. */
		IPND,	0x40,
		IPND+1,	0x40,	/* Reset interrupt pending register. */
		0xff
	};

	icu_init(icu_table);

	/* Initialize the base software interrupt masks. */
	imask[IPL_SOFTCLOCK] = SIR_CLOCKMASK | imask[IPL_ZERO];
	imask[IPL_SOFTNET] = SIR_NETMASK;

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

	/* Disable IR_SOFT in all priority levels other than IPL_ZERO. */
	for (i = 1; i < NIPL; i++)
		imask[i] |= 1 << IR_SOFT;
}

/*
 * Handle pending software interrupts.
 */
void
check_sir(void *arg)
{
	unsigned int cirpending, mask;
	struct iv *iv;

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
				int s;

				uvmexp.softs++;
				iv->iv_evcnt.ev_count++;
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
intr_establish(int intr, void (*vector)(void *), void *arg, const char *use,
    int blevel, int rlevel, int mode)
{
	int i, soft;

	di();
	soft = intr == SOFTINT;
	if (rlevel < IPL_ZERO || rlevel >= NIPL ||
	    blevel < IPL_ZERO || blevel >= NIPL)
		panic("Illegal interrupt level for %s in intr_establish", use);
	if (intr == SOFTINT) {
		if (next_sir >= INTS)
			panic("No software interrupts left");
		intr = next_sir++;
	} else {
		if (ivt[intr].iv_vec != badhard)
			panic("Interrupt %d already allocated", intr);
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
	ivt[intr].iv_use   = use;
	ivt[intr].iv_level = rlevel;
	evcnt_attach_dynamic(&ivt[intr].iv_evcnt, EVCNT_TYPE_INTR, NULL,
	    soft ? "soft" : "intr", use);
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
	imask[IPL_VM] |= imask[IPL_TTY] | imask[IPL_NET] | imask[IPL_BIO];

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
	 * Update IPL_SOFTCLOCK to reflect the new IPL_ZERO.
	 */
	imask[IPL_SOFTCLOCK] = SIR_CLOCKMASK | imask[IPL_ZERO];

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
badhard(void *arg)
{
	struct intrframe *frame = arg;
	static int bad_count = 0;
	di();
	bad_count++;
	if (bad_count < 5)
		printf("Unknown hardware interrupt: vec=%ld pc=0x%08x "
		    "psr=0x%04x cpl=0x%08lx\n", frame->if_vec,
		    frame->if_regs.r_pc, frame->if_regs.r_psr, frame->if_pl);

	if (bad_count == 5)
		printf("Too many unknown hardware interrupts, quitting "
		    "reporting them.\n");
	ei();
}

/*
 * Default software interrupt handler
 */
static void
badsoft(void *arg)
{
	static int bad_count = 0;

	bad_count++;
	if (bad_count < 5)
		printf("Unknown software interrupt: vec=%d\n", (int)arg);

	if (bad_count == 5)
		printf("Too many unknown software interrupts, quitting reporting them.\n");
}
