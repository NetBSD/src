/* $NetBSD: dec_5100.c,v 1.21 2000/02/29 04:41:53 nisimura Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/locore.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn01.h>		/* common definitions */
#include <pmax/pmax/kn230.h>
#include <pmax/dev/dcvar.h>

#include <pmax/ibus/ibusvar.h>

void		dec_5100_init __P((void));		/* XXX */
static void	dec_5100_bus_reset __P((void));
static void	dec_5100_cons_init __P((void));
static void	dec_5100_device_register __P((struct device *, void *));
static int	dec_5100_intr __P((unsigned, unsigned, unsigned, unsigned));
static void	dec_5100_intr_establish __P((struct device *, void *,
		    int, int (*)(void *), void *));
static void	dec_5100_memintr __P((void));

void
dec_5100_init()
{
	platform.iobus = "baseboard";
	platform.bus_reset = dec_5100_bus_reset;
	platform.cons_init = dec_5100_cons_init;
	platform.device_register = dec_5100_device_register;
	platform.iointr = dec_5100_intr;
	platform.intr_establish = dec_5100_intr_establish;
	platform.memsize = memsize_scan;
	/* no high resolution timer available */

	/* set correct wbflush routine for this motherboard */
	mips_set_wbflush(kn230_wbflush);

	mips_hardware_intr = dec_5100_intr;

	splvec.splbio = MIPS_SPL1;
	splvec.splnet = MIPS_SPL1;
	splvec.spltty = MIPS_SPL_0_1; 
	splvec.splimp = MIPS_SPL_0_1_2;
	splvec.splclock = MIPS_SPL_0_1_2;
	splvec.splstatclock = MIPS_SPL_0_1_2;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK), MIPS_INT_MASK_2);

	sprintf(cpu_model, "DECsystem 5100 (MIPSMATE)");
}

/*
 * Initalize the memory system and I/O buses.
 */
static void
dec_5100_bus_reset()
{
	volatile u_int *icsr_addr =
	   (volatile u_int *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);

	*icsr_addr |= KN230_CSR_INTR_WMERR ;

	/* nothing else to do */
	kn230_wbflush();
}

static void
dec_5100_cons_init()
{
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);	/* XXX */

	dc_cnattach(KN230_SYS_DZ0, 0);
}


static void
dec_5100_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_5100_device_register unimplemented");
}


static void
dec_5100_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{

	intrtab[(int)cookie].ih_func = handler;
	intrtab[(int)cookie].ih_arg = arg;
}


#define CALLINTR(vvv, ibit)						\
    do {								\
	if ((icsr & (ibit)) && intrtab[vvv].ih_func) {			\
		(*intrtab[vvv].ih_func)(intrtab[vvv].ih_arg);		\
		intrcnt[vvv]++;						\
	}								\
    } while (0)

static int
dec_5100_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	u_int32_t icsr;

	if (cpumask & MIPS_INT_MASK_4) {
#ifdef DDB
		Debugger();
#else
		prom_haltbutton();
#endif
	}

	icsr = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);

	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_2) {
		struct clockframe cf;

		__asm __volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_2;
	}

	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_2));

	if (cpumask & MIPS_INT_MASK_0) {
		CALLINTR(SYS_DEV_SCC0, KN230_CSR_INTR_DZ0);
		CALLINTR(SYS_DEV_OPT0, KN230_CSR_INTR_OPT0);
		CALLINTR(SYS_DEV_OPT1, KN230_CSR_INTR_OPT1);
	}

	if (cpumask & MIPS_INT_MASK_1) {
		CALLINTR(SYS_DEV_LANCE, KN230_CSR_INTR_LANCE);
		CALLINTR(SYS_DEV_LANCE, KN230_CSR_INTR_SII);
	}

	if (cpumask & MIPS_INT_MASK_3) {
		dec_5100_memintr();
		intrcnt[ERROR_INTR]++;
	}

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}


/*
 * Handle  write-to-nonexistent-address memory errors  on MIPS_INT_MASK_3.
 * These are reported asynchronously, due to hardware write buffering.
 * we can't easily figure out process context, so just panic.
 *
 * XXX drain writebuffer on contextswitch to avoid panic?
 */
static void
dec_5100_memintr()
{
	u_int32_t icsr;

	/* read icsr and clear error  */
	icsr = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);
	icsr |= KN230_CSR_INTR_WMERR;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(KN230_SYS_ICSR);
	kn230_wbflush();

#ifdef DIAGNOSTIC
		printf("\nMemory interrupt\n");
#endif

	/* ignore errors during probes */
	if (cold)
		return;

	if (icsr & KN230_CSR_INTR_WMERR) {
		panic("write to non-existant memory");
	}
	else {
		panic("stray memory error interrupt");
	}
}
