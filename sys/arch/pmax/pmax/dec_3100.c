/* $NetBSD: dec_3100.c,v 1.6.2.10 1999/11/12 11:07:20 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3100.c,v 1.6.2.10 1999/11/12 11:07:20 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/cons.h>
#include <sys/termios.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <pmax/pmax/kn01.h>
#include <mips/mips/mips_mcclock.h>

#include <machine/bus.h>
#include <pmax/ibus/ibusvar.h>

#include "wsdisplay.h"

void dec_3100_init __P((void));
void dec_3100_bus_reset __P((void));
void dec_3100_cons_init __P((void));
void dec_3100_device_register __P((struct device *, void *));
int  dec_3100_intr __P((unsigned, unsigned, unsigned, unsigned));
void dec_3100_intr_establish __P((struct device *, void *,
		int, int (*)(void *), void *));
void dec_3100_intr_disestablish __P((struct device *, void *));

static void dec_3100_memerr __P((void));

extern void kn01_wbflush __P((void));
extern void prom_findcons __P((int *, int *, int *));
extern int pm_cnattach __P((u_int32_t));
extern int dc_cnattach __P((paddr_t, int, int, int));
extern void dckbd_cnattach __P((paddr_t));

struct splsw spl_3100 = {
	{ _spllower,	0 },
	{ _splraise,	MIPS_SPL0 },
	{ _splraise,	MIPS_SPL_0_1	},
	{ _splraise,	MIPS_SPL_0_1_2	},
	{ _splraise,	MIPS_SPLHIGH },
	{ _splraise,	MIPS_SPL_0_1_2_3 },
	{ _splset,	0 },
};


void
dec_3100_init()
{
	extern char cpu_model[];

	platform.iobus = "baseboard";
	platform.bus_reset = dec_3100_bus_reset;
	platform.cons_init = dec_3100_cons_init;
	platform.device_register = dec_3100_device_register;
	platform.iointr = dec_3100_intr;
	/* no high resolution timer available */

	mips_hardware_intr = dec_3100_intr;

#ifdef NEWSPL
	__spl = &spl_3100;
#else
	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL_0_1;
	splvec.spltty = MIPS_SPL_0_1_2;
	splvec.splimp = MIPS_SPLHIGH;				/* ??? */
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;
#endif

	/* calibrate cpu_mhz value */
	mc_cpuspeed(
	    (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK), MIPS_INT_MASK_3);

	sprintf(cpu_model, "DECstation %d100 (PMAX)", cpu_mhz < 15 ? 3 : 2);
}

void
dec_3100_bus_reset()
{
	/* nothing to do */
	kn01_wbflush();
}

void
dec_3100_cons_init()
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
		dckbd_cnattach(KN01_SYS_DZ);
		pm_cnattach(0x0fc00000 + MIPS_KSEG1_START);
		return;
#else
		printf("No framebuffer device configured: ");
		printf("using serial console\n");
#endif
	}
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);        /* XXX */

	if (dc_cnattach(KN01_SYS_DZ, 3,
	    9600, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
		panic("can't init serial console");
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


#define	CHECKINTR(slot, cp0) 					\
	if (cpumask & (cp0)) {					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
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
	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_3) {
		struct clockframe cf;

		__asm __volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;
	
		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_3;
	}
	/* allow clock interrupt posted when enabled */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_3));

	CHECKINTR(SYS_DEV_SCSI, MIPS_INT_MASK_0);
	CHECKINTR(SYS_DEV_LANCE, MIPS_INT_MASK_1);
	CHECKINTR(SYS_DEV_SCC0, MIPS_INT_MASK_2);

#undef CHECKINTR

	if (cpumask & MIPS_INT_MASK_4) {
		dec_3100_memerr();
		intrcnt[ERROR_INTR]++;
		/* video vertical retrace interrupt comes here, too. */
	}

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

/*
 * Handle memory errors.
 */
static void
dec_3100_memerr()
{
	u_int16_t csr;

	csr = *(u_int16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);
	if (csr & KN01_CSR_MERR) {
		printf("Memory error at 0x%x\n",
			*(unsigned *)MIPS_PHYS_TO_KSEG1(KN01_SYS_ERRADR));
		panic("Mem error interrupt");
	}
	csr = (csr & ~KN01_CSR_MBZ) | 0xff;
	*(u_int16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR) = csr;
}
