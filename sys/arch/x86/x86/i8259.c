/*	$NetBSD: i8259.c,v 1.9 2006/10/12 01:30:44 christos Exp $	*/

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i8259.c,v 1.9 2006/10/12 01:30:44 christos Exp $");

#include <sys/param.h> 
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <dev/isa/isareg.h>
#include <dev/ic/i8259reg.h>

#include <machine/pio.h>
#include <machine/cpufunc.h>  
#include <machine/cpu.h>
#include <machine/pic.h>
#include <machine/i8259.h>


#ifndef __x86_64__
#include "mca.h"
#if NMCA > 0
#include <machine/mca_machdep.h>                /* for MCA_system */
#endif
#endif

static void i8259_hwmask(struct pic *, int);
static void i8259_hwunmask(struct pic *, int);
static void i8259_setup(struct pic *, struct cpu_info *, int, int, int);
static void i8259_reinit_irqs(void);

unsigned i8259_imen;

/*
 * Perhaps this should be made into a real device.
 */
struct pic i8259_pic = {
	.pic_dev = {
		.dv_xname = "pic0",
	},
	.pic_type = PIC_I8259,
	.pic_vecbase = 0,
	.pic_apicid = 0,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
	.pic_hwmask = i8259_hwmask,
	.pic_hwunmask = i8259_hwunmask,
	.pic_addroute = i8259_setup,
	.pic_delroute = i8259_setup,
	.pic_level_stubs = i8259_stubs,
	.pic_edge_stubs = i8259_stubs,
};

void
i8259_default_setup(void)
{
#if NMCA > 0
	/* level-triggered interrupts on MCA PS/2s */
	if (MCA_system)
		/* reset; program device, level-triggered, four bytes */
		outb(IO_ICU1 + PIC_ICW1, ICW1_SELECT | ICW1_LTIM | ICW1_IC4);
	else
#endif
		/* reset; program device, four bytes */
		outb(IO_ICU1 + PIC_ICW1, ICW1_SELECT | ICW1_IC4);

	/* starting at this vector index */
	outb(IO_ICU1 + PIC_ICW2, ICU_OFFSET);
	/* slave on line 2 */
	outb(IO_ICU1 + PIC_ICW3, ICW3_CASCADE(IRQ_SLAVE));

#ifdef AUTO_EOI_1
	/* auto EOI, 8086 mode */
	outb(IO_ICU1 + PIC_ICW4, ICW4_AEOI | ICW4_8086);
#else
	/* 8086 mode */
	outb(IO_ICU1 + PIC_ICW4, ICW4_8086);
#endif
	/* leave interrupts masked */
	outb(IO_ICU1 + PIC_OCW1, 0xff);
	/* special mask mode (if available) */
	outb(IO_ICU1 + PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* Read IRR by default. */
	outb(IO_ICU1 + PIC_OCW3, OCW3_SELECT | OCW3_RR);
#ifdef REORDER_IRQ
	/* pri order 3-7, 0-2 (com2 first) */
	outb(IO_ICU1 + PIC_OCW2, OCW2_SELECT | OCW2_R | OCW2_SL |
	    OCW2_ILS(3 - 1));
#endif

#if NMCA > 0
	/* level-triggered interrupts on MCA PS/2s */
	if (MCA_system)
		/* reset; program device, level-triggered, four bytes */
		outb(IO_ICU2 + PIC_ICW1, ICW1_SELECT | ICW1_LTIM | ICW1_IC4);
	else
#endif	
		/* reset; program device, four bytes */
		outb(IO_ICU2 + PIC_ICW1, ICW1_SELECT | ICW1_IC4);

	/* staring at this vector index */
	outb(IO_ICU2 + PIC_ICW2, ICU_OFFSET + 8);
	/* slave connected to line 2 of master */
	outb(IO_ICU2 + PIC_ICW3, ICW3_SIC(IRQ_SLAVE));
#ifdef AUTO_EOI_2
	/* auto EOI, 8086 mode */
	outb(IO_ICU2 + PIC_ICW4, ICW4_AEOI | ICW4_8086);
#else
	/* 8086 mode */
	outb(IO_ICU2 + PIC_ICW4, ICW4_8086);
#endif
	/* leave interrupts masked */
	outb(IO_ICU2 + PIC_OCW1, 0xff);
	/* special mask mode (if available) */
	outb(IO_ICU2 + PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* Read IRR by default. */
	outb(IO_ICU2 + PIC_OCW3, OCW3_SELECT | OCW3_RR);
}

static void
i8259_hwmask(struct pic *pic __unused, int pin)
{
	unsigned port;
	u_int8_t byte;

	i8259_imen |= (1 << pin);
#ifdef PIC_MASKDELAY
	delay(10);
#endif
	if (pin > 7) {
		port = IO_ICU2 + PIC_OCW1;
		byte = i8259_imen >> 8;
	} else {
		port = IO_ICU1 + PIC_OCW1;
		byte = i8259_imen & 0xff;
	}
	outb(port, byte);
}

static void
i8259_hwunmask(struct pic *pic __unused, int pin)
{
	unsigned port;
	u_int8_t byte;

	disable_intr();	/* XXX */
	i8259_imen &= ~(1 << pin);
#ifdef PIC_MASKDELAY
	delay(10);
#endif
	if (pin > 7) {
		port = IO_ICU2 + PIC_OCW1;
		byte = i8259_imen >> 8;
	} else {
		port = IO_ICU1 + PIC_OCW1;
		byte = i8259_imen & 0xff;
	}
	outb(port, byte);
	enable_intr();
}

static void
i8259_reinit_irqs(void)
{
	int irqs, irq;
	struct cpu_info *ci = &cpu_info_primary;

	irqs = 0;
	for (irq = 0; irq < NUM_LEGACY_IRQS; irq++)
		if (ci->ci_isources[irq] != NULL)
			irqs |= 1 << irq;
	if (irqs >= 0x100) /* any IRQs >= 8 in use */
		irqs |= 1 << IRQ_SLAVE;
	i8259_imen = ~irqs;

	outb(IO_ICU1 + PIC_OCW1, i8259_imen);
	outb(IO_ICU2 + PIC_OCW1, i8259_imen >> 8);
}

static void
i8259_setup(struct pic *pic __unused, struct cpu_info *ci __unused,
    int pin __unused, int idtvec __unused, int type __unused)
{
	if (CPU_IS_PRIMARY(ci))
		i8259_reinit_irqs();
}

void
i8259_reinit(void)
{
	i8259_default_setup();
	i8259_reinit_irqs();
}

unsigned
i8259_setmask(unsigned mask)
{
	unsigned old = i8259_imen;

	i8259_imen = mask;
	outb(IO_ICU1 + PIC_OCW1, i8259_imen);
	outb(IO_ICU2 + PIC_OCW1, i8259_imen >> 8);
	return old;
}
