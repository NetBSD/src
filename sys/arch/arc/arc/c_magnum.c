/*	$NetBSD: c_magnum.c,v 1.14.22.1 2007/06/26 18:12:08 garbled Exp $	*/
/*	$OpenBSD: machdep.c,v 1.36 1999/05/22 21:22:19 weingart Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

/*
 * for Magnum derived machines like Microsoft-Jazz and PICA-61.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: c_magnum.c,v 1.14.22.1 2007/06/26 18:12:08 garbled Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <machine/wired_map.h>
#include <mips/locore.h>
#include <mips/pte.h>

#include <dev/isa/isavar.h>

#include <arc/arc/timervar.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/timer_jazziovar.h>
#include <arc/isa/isabrvar.h>

/*
 * chipset-dependent timer routine.
 */

uint32_t timer_magnum_intr(uint32_t, struct clockframe *);
void timer_magnum_init(int);

struct timer_jazzio_config timer_magnum_conf = {
	MIPS_INT_MASK_4,
	timer_magnum_intr,
	timer_magnum_init,
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const uint32_t magnum_ipl_sr_bits[_IPL_N] = {
	[IPL_NONE] = 0,
	[IPL_SOFT] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTCLOCK] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = 
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_SOFTSERIAL] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_BIO] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0|
	    MIPS_INT_MASK_1|
	    MIPS_INT_MASK_2|
	    MIPS_INT_MASK_3,
	[IPL_NET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3,
	[IPL_TTY] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3,
	[IPL_CLOCK] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
};

uint32_t
timer_magnum_intr(uint32_t mask, struct clockframe *cf)
{
	int temp;

	temp = inw(R4030_SYS_IT_STAT);
	last_cp0_count = mips3_cp0_count_read();
	hardclock(cf);
	timer_jazzio_ev.ev_count++;

	return ~MIPS_INT_MASK_4; /* Keep clock interrupts enabled */
}

void
timer_magnum_init(int interval)
{

	if (interval <= 0)
		panic("timer_magnum_init: invalid interval %d", interval);

	out32(R4030_SYS_IT_VALUE, interval - 1);

	/* Enable periodic clock interrupt */
	out32(R4030_SYS_EXT_IMASK, cpu_int_mask);
}

/*
 * chipset-dependent isa bus configuration
 */

int isabr_magnum_intr_status(void);

struct isabr_config isabr_magnum_conf = {
	isabr_magnum_intr_status,
};

int
isabr_magnum_intr_status(void)
{

	return in32(R4030_SYS_ISA_VECTOR) & (ICU_LEN - 1);
}

/*
 * chipset-dependent jazzio bus configuration
 */

void jazzio_magnum_set_iointr_mask(int);

struct jazzio_config jazzio_magnum_conf = {
	PVIS,
	jazzio_magnum_set_iointr_mask,
	R4030_SYS_TL_BASE,
	R4030_SYS_DMA1_REGS,
};

void
jazzio_magnum_set_iointr_mask(int mask)
{

	out16(PICA_SYS_LB_IE, mask);
}

/*
 * chipset-dependent platform routines.
 */

void
c_magnum_set_intr(uint32_t mask,
    uint32_t (*int_hand)(uint32_t, struct clockframe *), int prio)
{

	arc_set_intr(mask, int_hand, prio);

	/* Update external interrupt mask but don't enable clock. */
	out32(R4030_SYS_EXT_IMASK, cpu_int_mask & (~MIPS_INT_MASK_4 >> 10));
}

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
c_magnum_init(void)
{

	/*
	 * Initialize I/O address offset
	 */
	arc_bus_space_init(&jazzio_bus, "jazzio",
	    R4030_P_LOCAL_IO_BASE, R4030_V_LOCAL_IO_BASE,
	    R4030_V_LOCAL_IO_BASE, R4030_S_LOCAL_IO_BASE);
	arc_bus_space_init(&arc_bus_io, "picaisaio",
	    PICA_P_ISA_IO, PICA_V_ISA_IO, 0, PICA_S_ISA_IO);
	arc_bus_space_init(&arc_bus_mem, "picaisamem",
	    PICA_P_ISA_MEM, PICA_V_ISA_MEM, 0, PICA_S_ISA_MEM);

	/*
	 * Initialize wired TLB for I/O space which is used on early stage
	 */
	arc_wired_enter_page(R4030_V_LOCAL_IO_BASE, R4030_P_LOCAL_IO_BASE,
	    R4030_S_LOCAL_IO_BASE);
	arc_wired_enter_page(PICA_V_INT_SOURCE, PICA_P_INT_SOURCE,
	    R4030_S_LOCAL_IO_BASE);

	arc_wired_enter_page(PICA_V_ISA_IO, PICA_P_ISA_IO, PICA_S_ISA_IO);
	arc_wired_enter_page(PICA_V_ISA_MEM, PICA_P_ISA_MEM, PICA_S_ISA_MEM);

	/*
	 * Initialize interrupt priority
	 */
	ipl_sr_bits = magnum_ipl_sr_bits;

	/*
	 * Disable all interrupts. New masks will be set up
	 * during system configuration
	 */
	out16(PICA_SYS_LB_IE,0x000);
	out32(R4030_SYS_EXT_IMASK, 0x00);

	/* common configuration for Magnum derived and NEC EISA machines */
	c_jazz_eisa_init();

	/* chipset-dependent timer configuration */
	timer_jazzio_conf = &timer_magnum_conf;

	/* chipset-dependent jazzio bus configuration */
	jazzio_conf = &jazzio_magnum_conf;

	/* chipset-dependent isa bus configuration */
	isabr_conf = &isabr_magnum_conf;
}
