/* $NetBSD: dec_3100.c,v 1.52.12.2 2014/08/20 00:03:18 tls Exp $ */

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Copyright (c) 1988 University of Utah.
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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#define __INTR_PRIVATE
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dec_3100.c,v 1.52.12.2 2014/08/20 00:03:18 tls Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/lwp.h>
#include <sys/systm.h>

#include <pmax/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <dev/tc/tcvar.h>		/* tc_addr_t */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn01.h>

#include <pmax/ibus/ibusvar.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>
#include <pmax/pmax/cons.h>

#include "pm.h"

void		dec_3100_init(void);		/* XXX */
static void	dec_3100_bus_reset(void);

static void	dec_3100_cons_init(void);
static void	dec_3100_errintr(void);
static void	dec_3100_intr(uint32_t, vaddr_t, uint32_t);
static void	dec_3100_intr_establish(device_t, void *,
		    int, int (*)(void *), void *);

#define	kn01_wbflush()	wbflush() /* XXX to be corrected XXX */

static const struct ipl_sr_map dec_3100_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTSERIAL] = MIPS_SOFT_INT_MASK,
	[IPL_VM] = MIPS_SPL_0_1_2,
	[IPL_SCHED] = MIPS_SPLHIGH,
	[IPL_DDB] = MIPS_SPLHIGH,
	[IPL_HIGH] = MIPS_SPLHIGH,
    }
};

void
dec_3100_init(void)
{
	const char *submodel;

	platform.iobus = "baseboard";
	platform.bus_reset = dec_3100_bus_reset;
	platform.cons_init = dec_3100_cons_init;
	platform.iointr = dec_3100_intr;
	platform.intr_establish = dec_3100_intr_establish;
	platform.memsize = memsize_scan;
	/* no high resolution timer available */

	ipl_sr_map = dec_3100_ipl_sr_map;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK), MIPS_INT_MASK_3);

	if (mips_options.mips_cpu_mhz < 15)
		submodel = "2100 (PMIN)";
	else
		submodel = "3100 (PMAX)";
	cpu_setmodel("DECstation %s", submodel);
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_3100_bus_reset(void)
{

	/* nothing to do */
	kn01_wbflush();
}

static void
dec_3100_cons_init(void)
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NPM > 0
 		if (pm_cnattach() > 0) {
			dz_ibus_cnsetup(KN01_SYS_DZ);
			dzkbd_cnattach(NULL);
 			return;
 		}
#endif
		printf("No framebuffer device configured: ");
		printf("using serial console\n");
	}
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);	/* XXX */

	dz_ibus_cnsetup(KN01_SYS_DZ);
	dz_ibus_cnattach(kbd);
}

#define CALLINTR(vvv, cp0)					\
    do {							\
	if (ipending & (cp0)) {					\
		intrtab[vvv].ih_count.ev_count++;		\
		(*intrtab[vvv].ih_func)(intrtab[vvv].ih_arg);	\
	}							\
    } while (/*CONSTCOND*/0)

static void
dec_3100_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{

	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_3) {
		struct clockframe cf;

		__asm volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		cf.intr = (curcpu()->ci_idepth > 1);
		hardclock(&cf);
		pmax_clock_evcnt.ev_count++;
	}

	CALLINTR(SYS_DEV_SCSI, MIPS_INT_MASK_0);
	CALLINTR(SYS_DEV_LANCE, MIPS_INT_MASK_1);
	CALLINTR(SYS_DEV_SCC0, MIPS_INT_MASK_2);

	if (ipending & MIPS_INT_MASK_4) {
		dec_3100_errintr();
		pmax_memerr_evcnt.ev_count++;
	}
}

static void
dec_3100_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{

	intrtab[(intptr_t)cookie].ih_func = handler;
	intrtab[(intptr_t)cookie].ih_arg = arg;
}


/*
 * Handle memory errors.
 */
static void
dec_3100_errintr(void)
{
	uint16_t csr;

	csr = *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);

	if (csr & KN01_CSR_MERR) {
		printf("Memory error at 0x%x\n",
		    *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_ERRADR));
		panic("Mem error interrupt");
	}
	csr = (csr & ~KN01_CSR_MBZ) | 0xff;
	*(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR) = csr;
}
