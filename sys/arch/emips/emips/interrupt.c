/*	$NetBSD: interrupt.c,v 1.4.12.2 2017/12/03 11:36:01 jdolecek Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.4.12.2 2017/12/03 11:36:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <mips/psl.h>

#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/intr.h>
#include <machine/emipsreg.h>

struct intrhand		 intrtab[MAX_DEV_NCOOKIES];

struct evcnt emips_clock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "clock", "intr");
struct evcnt emips_fpu_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "fpu", "intr");
struct evcnt emips_memerr_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "memerr", "intr");

static const char * const intrnames[MAX_DEV_NCOOKIES] = {
	"int-0", "int-1", "int-2", "int-3", "int-4",
	"int-5", "int-6", "int-7", "int-8", "int-9",
	"int-10", "int-11", "int-12", "int-13", "int-14",
	"int-15", "int-16", "int-17", "int-18", "int-19",
	"int-20", "int-21`", "int-22", "int-23", "int-24",
	"int-25", "int-26", "int-27", "int-28", "int-29",
	"int-30", "int-31"
};

void
intr_init(void)
{
	int i;

	for (i = 0; i < MAX_DEV_NCOOKIES; i++) {
		evcnt_attach_dynamic(&intrtab[i].ih_count,
		    EVCNT_TYPE_INTR, NULL, "emips", intrnames[i]);
	}

	/* I am trying to make this standard so its here. Bah. */
	struct tlbmask tlb;

	/* This is ugly but efficient. Sigh. */
#define TheAic ((struct _Aic *)INTERRUPT_CONTROLLER_DEFAULT_ADDRESS)

	tlb.tlb_hi = INTERRUPT_CONTROLLER_DEFAULT_ADDRESS;
	tlb.tlb_lo0 = INTERRUPT_CONTROLLER_DEFAULT_ADDRESS | 0xf02;
	tlb_write_entry(4, &tlb);

	tlb.tlb_hi = TIMER_DEFAULT_ADDRESS;
	tlb.tlb_lo0 = TIMER_DEFAULT_ADDRESS | 0xf02;
	tlb_write_entry(5, &tlb);
}

/*
 * emips uses one line for all I/O interrupts (0x8000).
 */
void
cpu_intr(int ppl, uint32_t status, vaddr_t pc)
{
	uint32_t ipending;
	int ipl;

	curcpu()->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&ipending))) {
		splx(ipl);
		/* device interrupts */
		if (ipending & MIPS_INT_MASK_5) {
			(*platform.iointr)(status, pc, ipending);
		}
		(void)splhigh();
	}
}

/*
 * Interrupt dispatcher for standard AIC-style interrupt controller
 */
void
emips_aic_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{
	struct clockframe cf;

	cf.pc = pc;
	cf.sr = status;
	cf.intr = (curcpu()->ci_idepth > 1);

	ipending = TheAic->IrqStatus;

	while (ipending) {
		/* Take one (most likely, the only one) */
		int index = ffs(ipending) - 1;
		ipending &= ~(1 << index);

		intrtab[index].ih_count.ev_count++;
		(*intrtab[index].ih_func)(intrtab[index].ih_arg, &cf);
	}
}


void
emips_intr_establish(device_t dev, void *cookie, int level,
	int (*handler) (void *, void *), void *arg)
{
	int index = (int) cookie;

	/*
	 * First disable that interrupt source, in case it was enabled.
	 * This prevents us from getting very confused with ISRs and arguments.
	 */
	TheAic->IrqEnableClear = 1 << index;
    
	/* Second, the argument & isr.  */
	intrtab[index].ih_func = handler;
	intrtab[index].ih_arg = arg;

	/* Third, enable and done.  */
	TheAic->IrqEnable = 1 << index;
}
