/* $NetBSD: dec_3max.c,v 1.21 2000/01/09 03:55:57 simonb Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3max.c,v 1.21 2000/01/09 03:55:57 simonb Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/locore.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <pmax/pmax/turbochannel.h>
#include <pmax/pmax/machdep.h>

#include <pmax/pmax/kn02.h>
#include <pmax/pmax/memc.h>

/*
 * forward declarations
 */
void		dec_3max_init __P((void));		/* XXX */
static void	dec_3max_bus_reset __P((void));

static void	dec_3max_enable_intr __P((unsigned slotno,
		    int (*handler)(void *), void *sc, int onoff));
static int	dec_3max_intr __P((unsigned, unsigned, unsigned, unsigned));
static void	dec_3max_cons_init __P((void));
static void	dec_3max_device_register __P((struct device *, void *));

static void	dec_3max_errintr __P((void));

#define	kn02_wbflush()	mips1_wbflush()	/* XXX to be corrected XXX */

void
dec_3max_init()
{
	u_int32_t csr;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3max_bus_reset;
	platform.cons_init = dec_3max_cons_init;
	platform.device_register = dec_3max_device_register;
	platform.iointr = dec_3max_intr;
	platform.memsize = memsize_scan;
	/* no high resolution timer available */

	/* clear any memory errors */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	mips_hardware_intr = dec_3max_intr;
	tc_enable_interrupt = dec_3max_enable_intr;

	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL0;
	splvec.spltty = MIPS_SPL0;
	splvec.splimp = MIPS_SPL0;
	splvec.splclock = MIPS_SPL_0_1;
	splvec.splstatclock = MIPS_SPL_0_1;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK), MIPS_INT_MASK_1);

	/*
	 * Enable ECC memory correction, turn off LEDs, and
	 * disable all TURBOchannel interrupts.
	 */
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	csr &= ~(KN02_CSR_WRESERVED|KN02_CSR_IOINTEN|KN02_CSR_CORRECT|0xff);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
	kn02_wbflush();

	strcpy(cpu_model, "DECstation 5000/200 (3MAX)");
}

/*
 * Initalize the memory system and I/O buses.
 */
static void
dec_3max_bus_reset()
{
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	*(u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN) = 0;
	kn02_wbflush();
}

static void
dec_3max_cons_init()
{
}

static void
dec_3max_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3max_device_register unimplemented");
}


/*
 * Enable/Disable interrupts for a TURBOchannel slot on the 3MAX.
 */
static void
dec_3max_enable_intr(slotno, handler, sc, on)
	u_int slotno;
	int (*handler) __P((void *));
	void *sc;
	int on;
{
	volatile int *p_csr =
		(volatile int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	int csr;
	int s;

#if 0
	printf("3MAX enable_intr: %sabling slot %d, sc %p\n",
	       (on? "en" : "dis"), slotno, sc);
#endif

	if (slotno > TC_MAX_LOGICAL_SLOTS)
		panic("kn02_enable_intr: bogus slot %d\n", slotno);

	if (on)  {
		/*printf("kn02: slot %d handler 0x%x\n", slotno, handler);*/
		tc_slot_info[slotno].intr = handler;
		tc_slot_info[slotno].sc = sc;
	} else {
		tc_slot_info[slotno].intr = 0;
		tc_slot_info[slotno].sc = 0;
	}

	slotno = 1 << (slotno + KN02_CSR_IOINTEN_SHIFT);
	s = splhigh();
	csr = *p_csr & ~(KN02_CSR_WRESERVED | 0xFF);
	if (on)
		*p_csr = csr | slotno;
	else
		*p_csr = csr & ~slotno;
	splx(s);
}


/*
 * Handle hardware interrupts for the KN02. (DECstation 5000/200)
 * Returns spl value.
 */
static int
dec_3max_intr(mask, pc, status, cause)
	unsigned mask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	static int warned = 0;
	unsigned i, m;
	unsigned csr;

	/* handle clock interrupts ASAP */
	if (mask & MIPS_INT_MASK_1) {
		struct clockframe cf;

		csr = *(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		if ((csr & KN02_CSR_PSWARN) && !warned) {
			warned = 1;
			printf("WARNING: power supply is overheating!\n");
		} else if (warned && !(csr & KN02_CSR_PSWARN)) {
			warned = 0;
			printf("WARNING: power supply is OK again\n");
		}

		__asm __volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrups were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_1));

	if (mask & MIPS_INT_MASK_0) {
		static int intr_map[8] = { SLOT0_INTR, SLOT1_INTR, SLOT2_INTR,
					   /* these two bits reserved */
					   STRAY_INTR,  STRAY_INTR,
					   SCSI_INTR, LANCE_INTR,
					   SERIAL0_INTR };

		csr = *(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		m = csr & (csr >> KN02_CSR_IOINTEN_SHIFT) & KN02_CSR_IOINT;
#if 0
		*(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) =
			(csr & ~(KN02_CSR_WRESERVED | 0xFF)) |
			(m << KN02_CSR_IOINTEN_SHIFT);
#endif
		for (i = 0; m; i++, m >>= 1) {
			if (!(m & 1))
				continue;
			intrcnt[intr_map[i]]++;
			if (tc_slot_info[i].intr)
				(*tc_slot_info[i].intr)(tc_slot_info[i].sc);
			else
				printf("spurious interrupt %d\n", i);
		}
#if 0
		*(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) =
			csr & ~(KN02_CSR_WRESERVED | 0xFF);
#endif
	}
	if (mask & MIPS_INT_MASK_3) {
		intrcnt[ERROR_INTR]++;
		dec_3max_errintr();
	}

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}


/*
 * Handle Memory error.   3max, 3maxplus has ECC.
 * Correct single-bit error, panic on  double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3max_errintr()
{
	u_int32_t erradr, errsyn, csr;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR);
	errsyn = MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn, csr & KN02_CSR_BNK32M);
}
