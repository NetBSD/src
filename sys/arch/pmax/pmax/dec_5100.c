/* $NetBSD: dec_5100.c,v 1.46.12.2 2014/08/20 00:03:18 tls Exp $ */

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
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_5100.c,v 1.46.12.2 2014/08/20 00:03:18 tls Exp $");

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/lwp.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <pmax/locore.h>
#include <pmax/sysconf.h>

#include <dev/tc/tcvar.h>		/* tc_addr_t */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn01.h>		/* common definitions */
#include <pmax/pmax/kn230.h>

#include <pmax/ibus/ibusvar.h>

#include <pmax/pmax/cons.h>

void		dec_5100_init(void);		/* XXX */
static void	dec_5100_bus_reset(void);
static void	dec_5100_cons_init(void);
static void	dec_5100_intr(uint32_t, vaddr_t, uint32_t);
static void	dec_5100_intr_establish(device_t, void *,
		    int, int (*)(void *), void *);
static void	dec_5100_memintr(void);

static const struct ipl_sr_map dec_5100_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = MIPS_SOFT_INT_MASK,
	[IPL_VM] = MIPS_SPL_0_1,
	[IPL_SCHED] = MIPS_SPLHIGH,
	[IPL_DDB] = MIPS_SPLHIGH,
	[IPL_HIGH] = MIPS_SPLHIGH,
    },
};

void
dec_5100_init(void)
{

	platform.iobus = "baseboard";
	platform.bus_reset = dec_5100_bus_reset;
	platform.cons_init = dec_5100_cons_init;
	platform.iointr = dec_5100_intr;
	platform.intr_establish = dec_5100_intr_establish;
	platform.memsize = memsize_scan;
	/* no high resolution timer available */

	/* set correct wbflush routine for this motherboard */
	mips_set_wbflush(kn230_wbflush);

	ipl_sr_map = dec_5100_ipl_sr_map;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK), MIPS_INT_MASK_2);

	cpu_setmodel("DECsystem 5100 (MIPSMATE)");
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_5100_bus_reset(void)
{
	uint32_t icsr;

	/* clear any memory error condition */
	icsr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);
	icsr |= KN230_CSR_INTR_WMERR;
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR) = icsr;

	/* nothing else to do */
	kn230_wbflush();
}

static void
dec_5100_cons_init(void)
{

	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);	/* XXX */

	dz_ibus_cnsetup(KN230_SYS_DZ0);
	dz_ibus_cnattach(0);
}

static void
dec_5100_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{

	intrtab[(intptr_t)cookie].ih_func = handler;
	intrtab[(intptr_t)cookie].ih_arg = arg;
}


#define CALLINTR(vvv, ibit)						\
    do {								\
	if ((icsr & (ibit)) && intrtab[vvv].ih_func) {			\
		(*intrtab[vvv].ih_func)(intrtab[vvv].ih_arg);		\
		intrtab[vvv].ih_count.ev_count++;			\
	}								\
    } while (/*CONSTCOND*/0)

static void
dec_5100_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{
	uint32_t icsr;

	if (ipending & MIPS_INT_MASK_4) {
#ifdef DDB
		Debugger();
#else
		prom_haltbutton();
#endif
	}

	icsr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);

	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_2) {
		struct clockframe cf;

		__asm volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		cf.intr = (curcpu()->ci_idepth > 1);
		hardclock(&cf);
		pmax_clock_evcnt.ev_count++;
	}

	if (ipending & MIPS_INT_MASK_0) {
		CALLINTR(SYS_DEV_SCC0, KN230_CSR_INTR_DZ0);
		CALLINTR(SYS_DEV_OPT0, KN230_CSR_INTR_OPT0);
		CALLINTR(SYS_DEV_OPT1, KN230_CSR_INTR_OPT1);
	}

	if (ipending & MIPS_INT_MASK_1) {
		CALLINTR(SYS_DEV_LANCE, KN230_CSR_INTR_LANCE);
		CALLINTR(SYS_DEV_SCSI, KN230_CSR_INTR_SII);
	}

	if (ipending & MIPS_INT_MASK_3) {
		dec_5100_memintr();
		pmax_memerr_evcnt.ev_count++;
	}
}


/*
 * Handle write-to-nonexistent-address memory errors on MIPS_INT_MASK_3.
 * These are reported asynchronously, due to hardware write buffering.
 * we can't easily figure out process context, so just panic.
 *
 * XXX drain writebuffer on contextswitch to avoid panic?
 */
static void
dec_5100_memintr(void)
{
	uint32_t icsr;

	/* read icsr and clear error  */
	icsr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);
	icsr |= KN230_CSR_INTR_WMERR;
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR) = icsr;
	kn230_wbflush();

#ifdef DIAGNOSTIC
	printf("\nMemory interrupt\n");
#endif

	/* ignore errors during probes */
	if (cold)
		return;

	if (icsr & KN230_CSR_INTR_WMERR) {
		panic("write to non-existent memory");
	} else {
		panic("stray memory error interrupt");
	}
}
