/*	$NetBSD: dec_3100.c,v 1.12 1999/05/26 04:23:59 nisimura Exp $	*/

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
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/locore.h>
#include <machine/autoconf.h>		/* intr_arg_t */
#include <machine/sysconf.h>		/* intr_arg_t */

#include <mips/mips_param.h>		/* hokey spl()s */
#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <pmax/pmax/clockreg.h>
#include <pmax/pmax/turbochannel.h>
#include <pmax/pmax/pmaxtype.h>

#include <pmax/pmax/kn01.h>

#include <pmax/ibus/ibusvar.h>

#include "dc_ds.h"
#include "le_pmax.h"
#include "sii.h"

void		dec_3100_init __P((void));
void		dec_3100_os_init __P((void));
void		dec_3100_bus_reset __P((void));

void		dec_3100_enable_intr
		   __P ((u_int slotno, int (*handler) __P((intr_arg_t sc)),
			 intr_arg_t sc, int onoff));
int		dec_3100_intr __P((unsigned, unsigned, unsigned, unsigned));
void		dec_3100_cons_init __P((void));
void		dec_3100_device_register __P((struct device *, void *));

static void	dec_3100_errintr __P((void));

void	dec_3100_intr_establish __P((void* cookie, int level,
			 int (*handler) __P((intr_arg_t)), intr_arg_t arg));
void	dec_3100_intr_disestablish __P((struct ibus_attach_args *ia));

extern unsigned nullclkread __P((void));
extern unsigned (*clkread) __P((void));

extern volatile struct chiptime *mcclock_addr; /* XXX */
extern char cpu_model[];


/*
 * Fill in platform struct.
 */
void
dec_3100_init()
{
	platform.iobus = "baseboard";

	platform.os_init = dec_3100_os_init;
	platform.bus_reset = dec_3100_bus_reset;
	platform.cons_init = dec_3100_cons_init;
	platform.device_register = dec_3100_device_register;

	sprintf(cpu_model, "DECstation %d100 (PMAX)", cpu_mhz < 15 ? 3 : 2);

	dec_3100_os_init();
}


void
dec_3100_os_init()
{
	/*
	 * Set up interrupt handling and I/O addresses.
	 */
	mips_hardware_intr = dec_3100_intr;
	tc_enable_interrupt = dec_3100_enable_intr; /*XXX*/
	mcclock_addr = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK);

	/* no high resolution timer circuit; possibly never called */
	clkread = nullclkread;

	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL_0_1;
	splvec.spltty = MIPS_SPL_0_1_2;
	splvec.splimp = MIPS_SPLHIGH;				/* ??? */
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;

	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_3);
}


/*
 * Initalize the memory system and I/O buses.
 */
void
dec_3100_bus_reset()
{
	/* nothing to do */
	(void)wbflush();
}

void
dec_3100_cons_init()
{
}


void
dec_3100_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3100_device_register unimplemented");
}


/*
 * Enable an interrupt from a slot on the KN01 internal bus.
 *
 * The 4.4bsd kn01 interrupt handler hard-codes r3000 CAUSE register
 * bits to particular device interrupt handlers.  We may choose to store
 * function and softc pointers at some future point.
 */
void
dec_3100_enable_intr(slotno, handler, sc, on)
	unsigned int slotno;
	int (*handler) __P((void* softc));
	void *sc;
	int on;
{
	/*
	 */
	if (on)  {
		tc_slot_info[slotno].intr = handler;
		tc_slot_info[slotno].sc = sc;
	} else {
		tc_slot_info[slotno].intr = 0;
		tc_slot_info[slotno].sc = 0;
	}
}

/*
 * Handle pmax (DECstation 2100/3100) interrupts.
 */
int
dec_3100_intr(mask, pc, status, cause)
	unsigned mask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	/* handle clock interrupts ASAP */
	if (mask & MIPS_INT_MASK_3) {
		struct clockframe cf;
		struct chiptime *clk;
		volatile int temp;

		clk = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK);
		temp = clk->regc;	/* XXX clear interrupt bits */

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
		(*tc_slot_info[3].intr)(tc_slot_info[3].sc);
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
		(*tc_slot_info[2].intr)(tc_slot_info[2].sc);
		intrcnt[LANCE_INTR]++;
	}
#endif /* NLE_PMAX */

#if NDC_DS > 0
	if (mask & MIPS_INT_MASK_2) {
		(*tc_slot_info[1].intr)(tc_slot_info[1].sc);
		intrcnt[SERIAL0_INTR]++;
	}
#endif /* NDC_DS */

	if (mask & MIPS_INT_MASK_4) {
		dec_3100_errintr();
		intrcnt[ERROR_INTR]++;
	}
	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

void
dec_3100_intr_establish(cookie, level, handler, arg)
	void * cookie;
	int level;
	int (*handler) __P((intr_arg_t));
	intr_arg_t arg;
{
	dec_3100_enable_intr((u_int)cookie, handler, arg, 1);
}


void
dec_3100_intr_disestablish(struct ibus_attach_args *ia)
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
