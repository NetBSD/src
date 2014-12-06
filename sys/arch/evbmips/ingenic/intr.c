/*	$NetBSD: intr.c,v 1.1 2014/12/06 14:26:40 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.1 2014/12/06 14:26:40 macallan Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <mips/locore.h>
#include <machine/intr.h>

#include <mips/ingenic/ingenic_regs.h>

extern void ingenic_clockintr(uint32_t);
extern void ingenic_puts(const char *);

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
static const struct ipl_sr_map ingenic_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_SCHED] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =            MIPS_INT_MASK,
    },
};

//#define INGENIC_DEBUG
void
evbmips_intr_init(void)
{
	uint32_t reg;

	ipl_sr_map = ingenic_ipl_sr_map;

	/* mask all peripheral IRQs */
	writereg(JZ_ICMR0, 0xffffffff);
	writereg(JZ_ICMR1, 0xffffffff);

	/* allow mailbox and peripheral interrupts to core 0 only */
	reg = MFC0(12, 4);	/* reset entry and interrupts */
	reg &= 0xffff0000;
	reg |= REIM_IRQ0_M | REIM_MIRQ0_M | REIM_MIRQ1_M;
	MTC0(reg, 12, 4);
}

void
evbmips_iointr(int ipl, vaddr_t pc, uint32_t ipending)
{
	uint32_t id;
#ifdef INGENIC_DEBUG
	char buffer[256];
	
	snprintf(buffer, 256, "pending: %08x CR %08x\n", ipending, MFC0(MIPS_COP_0_CAUSE, 0));
	ingenic_puts(buffer);
#endif
	/* see which core we're on */
	id = MFC0(15, 1) & 7;

	/*
	 * XXX
	 * the manual counts the softint bits as INT0 and INT1, out headers
	 * don't so everything here looks off by two
	 */
	if (ipending & MIPS_INT_MASK_1) {
		/*
		 * this is a mailbox interrupt / IPI
		 * for now just print the message and clear it
		 */
		uint32_t reg;

		/* read pending IPIs */
		reg = MFC0(12, 3);
		if (id == 0) {
			if (reg & CS_MIRQ0_P) {
	
#ifdef INGENIC_DEBUG
				snprintf(buffer, 256, "IPI for core 0, msg %08x\n",
				    MFC0(CP0_CORE_MBOX, 0));
				ingenic_puts(buffer);
#endif
				reg &= (~CS_MIRQ0_P);
				/* clear it */
				MTC0(reg, 12, 3);
			}
		} else if (id == 1) {
			if (reg & CS_MIRQ1_P) {
#ifdef INGENIC_DEBUG
				snprintf(buffer, 256, "IPI for core 1, msg %08x\n",
				    MFC0(CP0_CORE_MBOX, 1));
				ingenic_puts(buffer);
#endif
				reg &= ( 7 - CS_MIRQ1_P);
				/* clear it */
				MTC0(reg, 12, 3);
			}
		}
	}
	if (ipending & MIPS_INT_MASK_2) {
		/* this is a timer interrupt */
		ingenic_clockintr(id);
		ingenic_puts("INT2\n");
	}
	if (ipending & MIPS_INT_MASK_0) {
		/* peripheral interrupt */

		/*
		 * XXX
		 * OS timer interrupts are supposed to show up as INT2 as well
		 * but I haven't seen them there so for now we just weed them
		 * out right here.
		 * The idea is to allow peripheral interrupts on both cores but
		 * block INT0 on core1 so it would see only timer interrupts 
		 * and IPIs. If that doesn't work we'll have to send an IPI to
		 * core1 for each timer tick.  
		 */
		if (readreg(JZ_ICPR0) & 0x08000000)
			ingenic_clockintr(id);
		KASSERT(id == 0);
	}
}
