/* $NetBSD: dec_3max.c,v 1.54.12.2 2014/08/20 00:03:18 tls Exp $ */

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

#define	__INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dec_3max.c,v 1.54.12.2 2014/08/20 00:03:18 tls Exp $");

#include "dzkbd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/lwp.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <dev/tc/tcvar.h>		/* tc_addr_t */

#include <pmax/sysconf.h>

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn02.h>
#include <pmax/pmax/memc.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>
#include <pmax/pmax/cons.h>
#include "wsdisplay.h"

void		dec_3max_init(void);		/* XXX */
static void	dec_3max_bus_reset(void);

static void	dec_3max_cons_init(void);
static void	dec_3max_errintr(void);
static void	dec_3max_intr(uint32_t, vaddr_t, uint32_t);
static void	dec_3max_intr_establish(device_t, void *,
		    int, int (*)(void *), void *);


#define	kn02_wbflush()	wbflush()	/* XXX to be corrected XXX */

static const struct ipl_sr_map dec_3max_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = MIPS_SOFT_INT_MASK,
	[IPL_VM] = MIPS_SPL0,
	[IPL_SCHED] = MIPS_SPLHIGH,
	[IPL_DDB] = MIPS_SPLHIGH,
	[IPL_HIGH] = MIPS_SPLHIGH,
    },
};

void
dec_3max_init(void)
{
	uint32_t csr;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3max_bus_reset;
	platform.cons_init = dec_3max_cons_init;
	platform.iointr = dec_3max_intr;
	platform.intr_establish = dec_3max_intr_establish;
	platform.memsize = memsize_bitmap;
	/* no high resolution timer available */

	/* clear any memory errors */
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	ipl_sr_map = dec_3max_ipl_sr_map;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK), MIPS_INT_MASK_1);

	/*
	 * Enable ECC memory correction, turn off LEDs, and
	 * disable all TURBOchannel interrupts.
	 */
	csr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	csr &= ~(KN02_CSR_WRESERVED|KN02_CSR_IOINTEN|KN02_CSR_CORRECT|0xff);
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
	kn02_wbflush();

	cpu_setmodel("DECstation 5000/200 (3MAX)");
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_3max_bus_reset(void)
{

	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN) = 0;
	kn02_wbflush();
}

static void
dec_3max_cons_init(void)
{
	int kbd, crt, screen;
	extern int tcfb_cnattach(int);		/* XXX */

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
 		if (kbd == 7 && tcfb_cnattach(crt) > 0) {
			dz_ibus_cnsetup(KN02_SYS_DZ);
			dzkbd_cnattach(NULL);
 			return;
 		}
#endif
		printf("No framebuffer device configured for slot %d: ", crt);
		printf("using serial console\n");
	}
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);	/* XXX */

	dz_ibus_cnsetup(KN02_SYS_DZ);
	dz_ibus_cnattach(kbd);
}

static const struct {
	int cookie;
	int intrbit;
} kn02intrs[] = {
	{ SYS_DEV_OPT0,	 KN02_IP_SLOT0 },
	{ SYS_DEV_OPT1,	 KN02_IP_SLOT1 },
	{ SYS_DEV_OPT2,	 KN02_IP_SLOT2 },
	{ SYS_DEV_SCSI,	 KN02_IP_SCSI },
	{ SYS_DEV_LANCE, KN02_IP_LANCE },
	{ SYS_DEV_SCC0,	 KN02_IP_DZ },
};

static void
dec_3max_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{
	int i;
	uint32_t csr;

	for (i = 0; i < __arraycount(kn02intrs); i++) {
		if (kn02intrs[i].cookie == (intptr_t)cookie)
			goto found;
	}
	panic("intr_establish: invalid cookie %p", cookie);

found:
	intrtab[(intptr_t)cookie].ih_func = handler;
	intrtab[(intptr_t)cookie].ih_arg = arg;

	csr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) &
	    0x00ffff00;
	csr |= (kn02intrs[i].intrbit << 16);
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
}


#define CALLINTR(vvv)						\
	do {							\
		intrtab[vvv].ih_count.ev_count++;		\
		(*intrtab[vvv].ih_func)(intrtab[vvv].ih_arg);	\
	} while (/*CONSTCOND*/0)

static void
dec_3max_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{
	static bool warned = false;
	uint32_t csr;

	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_1) {
		struct clockframe cf;

		csr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		if ((csr & KN02_CSR_PSWARN) && !warned) {
			warned = true;
			printf("WARNING: power supply is overheating!\n");
		} else if (warned && !(csr & KN02_CSR_PSWARN)) {
			warned = false;
			printf("WARNING: power supply is OK again\n");
		}

		__asm volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		cf.intr = (curcpu()->ci_idepth > 1);
		hardclock(&cf);
		pmax_clock_evcnt.ev_count++;

		/* keep clock interrupts enabled when we return */
	}

	if (ipending & MIPS_INT_MASK_0) {
		csr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		csr &= (csr >> KN02_CSR_IOINTEN_SHIFT);
		if (csr & (KN02_IP_DZ | KN02_IP_LANCE | KN02_IP_SCSI)) {
			if (csr & KN02_IP_DZ)
				CALLINTR(SYS_DEV_SCC0);
			if (csr & KN02_IP_LANCE)
				CALLINTR(SYS_DEV_LANCE);
			if (csr & KN02_IP_SCSI)
				CALLINTR(SYS_DEV_SCSI);
		}
		if (csr & (KN02_IP_SLOT2 | KN02_IP_SLOT1 | KN02_IP_SLOT0)) {
			if (csr & KN02_IP_SLOT2)
				CALLINTR(SYS_DEV_OPT2);
			if (csr & KN02_IP_SLOT1)
				CALLINTR(SYS_DEV_OPT1);
			if (csr & KN02_IP_SLOT0)
				CALLINTR(SYS_DEV_OPT0);
		}
	}
	if (ipending & MIPS_INT_MASK_3) {
		dec_3max_errintr();
		pmax_memerr_evcnt.ev_count++;
	}
}


/*
 * Handle Memory error.   3max, 3maxplus has ECC.
 * Correct single-bit error, panic on double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3max_errintr(void)
{
	uint32_t erradr, csr;
	vaddr_t errsyn;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR);
	errsyn = MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN);
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();
	csr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn, csr & KN02_CSR_BNK32M);
}
