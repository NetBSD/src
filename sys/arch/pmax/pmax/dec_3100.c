/*	$NetBSD: dec_3100.c,v 1.6.2.2 1998/10/20 02:46:40 nisimura Exp $ */

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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_3100.c,v 1.6.2.2 1998/10/20 02:46:40 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPU speed estimation */
#include <pmax/pmax/clockreg.h>
#include <pmax/pmax/pmaxtype.h>

#include <pmax/pmax/kn01.h>		/* common definitions */

#include <dev/tc/tcvar.h>		/* !!! */
#include <pmax/ibus/ibusvar.h>

#include "wsdisplay.h"

void dec_3100_init __P((void));
void dec_3100_os_init __P((void));
void dec_3100_bus_reset __P((void));
void dec_3100_cons_init __P((void));
void dec_3100_device_register __P((struct device *, void *));
int  dec_3100_intr __P((unsigned, unsigned, unsigned, unsigned));
void dec_3100_intr_establish __P((struct device *, void *,
		int, int (*)(void *), void *));
void dec_3100_intr_disestablish __P((struct device *, void *));

static void dec_3100_memerr __P((void));

extern void kn01_wbflush __P((void));
extern unsigned nullclkread __P((void));
extern unsigned (*clkread) __P((void));
extern volatile struct chiptime *mcclock_addr;
extern char cpu_model[];

struct splsw spl_3100 = {
	{ _spllower,	0 },
	{ _splraise,	MIPS_SPL0 },
	{ _splraise,	MIPS_SPL_0_1	},
	{ _splraise,	MIPS_SPL_0_1_2	},
	{ _splraise,	MIPS_SPLHIGH },
	{ _splraise,	MIPS_SPL_0_1_2_3 },
	{ _splset,	0 },
};

/*
 * Fill in platform struct. 
 */
void
dec_3100_init()
{
	platform.iobus = "ibus";

	platform.os_init = dec_3100_os_init;
	platform.bus_reset = dec_3100_bus_reset;
	platform.cons_init = dec_3100_cons_init;
	platform.device_register = dec_3100_device_register;

	strcpy(cpu_model, "DECstation 2100 or 3100 (PMAX)");

	dec_3100_os_init();
}

void
dec_3100_os_init()
{
	/*
	 * Set up interrupt handling and I/O addresses.
	 */
	mips_hardware_intr = dec_3100_intr;

	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL_0_1;
	splvec.spltty = MIPS_SPL_0_1_2;
	splvec.splimp = MIPS_SPLHIGH;				/* ??? */
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;

	mcclock_addr = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK);
	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_3);

	/* no high resolution timer circuit; possibly never called */
	clkread = nullclkread;
}

/*
 * Initalize the memory system and I/O buses.
 */
void
dec_3100_bus_reset()
{
	/* nothing to do */
	(void)kn01_wbflush();
}

#include <dev/cons.h>
#include <sys/termios.h>

extern void prom_findcons __P((int *, int *, int *));
extern int pm_cnattach __P((tc_addr_t));
extern int dc_cnattach __P((tc_addr_t, tc_offset_t, int, int, int));
extern int dc_major;

void
dec_3100_cons_init()
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

#if NWSDISPLAY > 0
	if (screen > 0) {
		if (pm_cnattach(0x0fc00000 + MIPS_KSEG1_START) > 0)
			return;
		printf("No framebuffer device configured");
		printf("Using serial console\n");
	}	
#endif
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);        /* XXX */

	if (dc_cnattach(0x1c000000, 0x0, 3,
	    9600, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
		panic("can't init serial console");

	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(dc_major, 3);
}

void
dec_3100_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3100_device_register unimplemented");
}

void
dec_3100_intr_establish(ioa, cookie, level, func, arg)
	struct device *ioa;
	void *cookie, *arg;
	int level;
	int (*func) __P((void *));
{
	int dev = (int)cookie;

	if (dev < 0 || dev >= MAX_DEV_NCOOKIES)
		panic("dec_3100_intr_establish: invalid cookie %d", dev);

	intrtab[dev].ih_func = func;
	intrtab[dev].ih_arg = arg;
}

void
dec_3100_intr_disestablish(dev, cookie)
	struct device *dev;
	void *cookie;
{
	printf("dec_3100_intr_distestablish: not implemented\n");
}

/*
 * Handle pmax interrupts.
 */
int
dec_3100_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	volatile struct chiptime *clk = 
			(void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK);
	struct clockframe cf;
	volatile int temp;

	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_3) {
		temp = clk->regc;	/* XXX clear interrupt bits */
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_3;
	}

	/* If clock interrupts were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_ENA_CUR | (status & MIPS_INT_MASK_3));

#define	CHECKINTR(slot, cp0) 					\
	if (cpumask & (cp0)) {					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	}

	CHECKINTR(SYS_DEV_SCSI, MIPS_INT_MASK_0);
	CHECKINTR(SYS_DEV_LANCE, MIPS_INT_MASK_1);
	CHECKINTR(SYS_DEV_SCC0, MIPS_INT_MASK_2);

#undef CHECKINTR

	if (cpumask & MIPS_INT_MASK_4) {
		dec_3100_memerr();
		intrcnt[ERROR_INTR]++;
	}

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_ENA_CUR);
}

/*
 * Handle memory errors.
 */
static void
dec_3100_memerr()
{
	volatile u_int16_t *p = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);
	u_int16_t csr;

	csr = *p;
	if (csr & KN01_CSR_MERR) {
		printf("Memory error at 0x%x\n",
			*(unsigned *)MIPS_PHYS_TO_KSEG1(KN01_SYS_ERRADR));
		panic("Mem error interrupt");
	}
	*p = (csr & ~KN01_CSR_MBZ) | 0xff;
}
