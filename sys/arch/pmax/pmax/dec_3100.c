/* $NetBSD: dec_3100.c,v 1.23 2000/01/14 13:45:23 simonb Exp $ */

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

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn01.h>

#include <pmax/ibus/ibusvar.h>

#include "dc.h"
#include "le_pmax.h"
#include "sii.h"

void		dec_3100_init __P((void));		/* XXX */
static void	dec_3100_bus_reset __P((void));

static void	dec_3100_cons_init __P((void));
static void	dec_3100_device_register __P((struct device *, void *));
static void	dec_3100_errintr __P((void));
static int	dec_3100_intr __P((unsigned, unsigned, unsigned, unsigned));
static void	dec_3100_intr_disestablish __P((struct device *, void *));
static void	dec_3100_intr_establish __P((struct device *, void *,
		    int, int (*)(void *), void *));

#define	kn01_wbflush()	mips1_wbflush() /* XXX to be corrected XXX */

void
dec_3100_init()
{
	platform.iobus = "baseboard";
	platform.bus_reset = dec_3100_bus_reset;
	platform.cons_init = dec_3100_cons_init;
	platform.device_register = dec_3100_device_register;
	platform.iointr = dec_3100_intr;
	platform.intr_establish = dec_3100_intr_establish;
	platform.intr_disestablish = dec_3100_intr_disestablish;
	platform.memsize = memsize_scan;
	/* no high resolution timer available */

	mips_hardware_intr = dec_3100_intr;

	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL_0_1;
	splvec.spltty = MIPS_SPL_0_1_2;
	splvec.splimp = MIPS_SPLHIGH;				/* ??? */
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK), MIPS_INT_MASK_3);

	sprintf(cpu_model, "DECstation %d100 (PMAX)", cpu_mhz < 15 ? 2 : 3);
}

/*
 * Initalize the memory system and I/O buses.
 */
static void
dec_3100_bus_reset()
{
	/* nothing to do */
	kn01_wbflush();
}

static void
dec_3100_cons_init()
{
	/* notyet */
}


static void
dec_3100_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3100_device_register unimplemented");
}


/*
 * Handle pmax (DECstation 2100/3100) interrupts.
 */
static int
dec_3100_intr(mask, pc, status, cause)
	unsigned mask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	/* handle clock interrupts ASAP */
	if (mask & MIPS_INT_MASK_3) {
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

	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_3));

#if NSII > 0
	if (mask & MIPS_INT_MASK_0) {
		intrcnt[SCSI_INTR]++;
		(*intrtab[3].ih_func)(intrtab[3].ih_arg);
	}
#endif /* NSII */

#if NLE_PMAX > 0
	if (mask & MIPS_INT_MASK_1) {
		/*
		 * tty interrupts were disabled by the splx() call
		 * that re-enables clock interrupts.  A slip or ppp driver
		 * manipulating if queues should have called splimp(),
		 * which would mask out MIPS_INT_MASK_1.
		 */
		(*intrtab[2].ih_func)(intrtab[2].ih_arg);
		intrcnt[LANCE_INTR]++;
	}
#endif /* NLE_PMAX */

#if NDC > 0
	if (mask & MIPS_INT_MASK_2) {
		(*intrtab[1].ih_func)(intrtab[1].ih_arg);
		intrcnt[SERIAL0_INTR]++;
	}
#endif /* NDC */

	if (mask & MIPS_INT_MASK_4) {
		dec_3100_errintr();
		intrcnt[ERROR_INTR]++;
	}
	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

static void
dec_3100_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	int slotno = (int)cookie;

	intrtab[slotno].ih_func = handler;
	intrtab[slotno].ih_arg = arg;
}


static void
dec_3100_intr_disestablish(dev, arg)
	struct device *dev;
	void *arg;
{
	printf("dec_3100_intr_distestablish: not implemented\n");
}


/*
 * Handle memory errors.
 */
static void
dec_3100_errintr()
{
	volatile u_short *sysCSRPtr =
		(u_short *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);
	u_short csr;

	csr = *sysCSRPtr;

	if (csr & KN01_CSR_MERR) {
		printf("Memory error at 0x%x\n",
			*(unsigned *)MIPS_PHYS_TO_KSEG1(KN01_SYS_ERRADR));
		panic("Mem error interrupt");
	}
	*sysCSRPtr = (csr & ~KN01_CSR_MBZ) | 0xff;
}
