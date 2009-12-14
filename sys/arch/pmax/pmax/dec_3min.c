/* $NetBSD: dec_3min.c,v 1.66 2009/12/14 00:46:10 matt Exp $ */

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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_3min.c,v 1.66 2009/12/14 00:46:10 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

/* all these to get ioasic_base */
#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <dev/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kmin.h>		/* 3min baseboard addresses */
#include <pmax/pmax/memc.h>		/* 3min/maxine memory errors */

#include <pmax/pmax/cons.h> 
#include <dev/ic/z8530sc.h>                                          
#include <dev/tc/zs_ioasicvar.h>
#include "wsdisplay.h"

void		dec_3min_init(void);		/* XXX */
static void	dec_3min_bus_reset(void);
static void	dec_3min_cons_init(void);
static void	dec_3min_intr(unsigned, unsigned, unsigned, unsigned);
static void	dec_3min_intr_establish(struct device *, void *,
		    int, int (*)(void *), void *);

static void	kn02ba_wbflush(void);

static void	dec_3min_tc_init(void);

/*
 * Local declarations.
 */
static uint32_t kmin_tc3_imask;

static const int dec_3min_ipl2spl_table[] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = _SPL_SOFTCLOCK,
	[IPL_SOFTNET] = _SPL_SOFTNET,
	/*
	 * Since all the motherboard interrupts come through the
	 * IOASIC, it has to be turned off for all the spls and
	 * since we don't know what kinds of devices are in the
	 * TURBOchannel option slots, just splhigh().
	 */
	[IPL_VM] = MIPS_SPL_0_1_2_3,
	[IPL_SCHED] = MIPS_SPL_0_1_2_3,
	[IPL_HIGH] = MIPS_SPL_0_1_2_3,
};

void
dec_3min_init(void)
{

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3min_bus_reset;
	platform.cons_init = dec_3min_cons_init;
	platform.iointr = dec_3min_intr;
	platform.intr_establish = dec_3min_intr_establish;
	platform.memsize = memsize_bitmap;
	platform.tc_init = dec_3min_tc_init;

	/* clear any memory errors */
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KMIN_SYS_ASIC);

	ipl2spl_table = dec_3min_ipl2spl_table;

	/* enable posting of MIPS_INT_MASK_3 to CAUSE register */
	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = KMIN_INTR_CLOCK;
	/* calibrate cpu_mhz value */
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_3);

	*(volatile uint32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(volatile uint32_t *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif

	/* sanitize interrupt mask */
	kmin_tc3_imask = (KMIN_INTR_CLOCK|KMIN_INTR_PSWARN|KMIN_INTR_TIMEOUT);
	*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = kmin_tc3_imask;

	/*
	 * The kmin memory hardware seems to wrap memory addresses
	 * with 4Mbyte SIMMs, which causes the physmem computation
	 * to lose.  Find out how big the SIMMS are and set
	 * max_ physmem accordingly.
	 * XXX Do MAXINEs lose the same way?
	 */
	physmem_boardmax = KMIN_PHYS_MEMORY_END + 1;
	if ((KMIN_MSR_SIZE_16Mb & *(int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_MSR))
	    == 0)
		physmem_boardmax = physmem_boardmax >> 2;
	physmem_boardmax = MIPS_PHYS_TO_KSEG1(physmem_boardmax);

	sprintf(cpu_model, "DECstation 5000/1%d (3MIN)", cpu_mhz);
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_3min_bus_reset(void)
{

	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ba_wbflush();
}

static void
dec_3min_cons_init(void)
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
 		if (tcfb_cnattach(crt) > 0) {
			zs_ioasic_lk201_cnattach(ioasic_base, 0x180000, 0);
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

	zs_ioasic_cnattach(ioasic_base, 0x180000, 1);
}

static void
dec_3min_intr_establish(struct device *dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{
	uint32_t mask;

	switch ((uintptr_t)cookie) {
		/* slots 0-2 don't interrupt through the IOASIC. */
	case SYS_DEV_OPT0:
		mask = MIPS_INT_MASK_0;
		break;
	case SYS_DEV_OPT1:
		mask = MIPS_INT_MASK_1;
		break;
	case SYS_DEV_OPT2:
		mask = MIPS_INT_MASK_2;
		break;

	case SYS_DEV_SCSI:
		mask = (IOASIC_INTR_SCSI | IOASIC_INTR_SCSI_PTR_LOAD |
			IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);
		break;
	case SYS_DEV_LANCE:
		mask = KMIN_INTR_LANCE;
		break;
	case SYS_DEV_SCC0:
		mask = KMIN_INTR_SCC_0;
		break;
	case SYS_DEV_SCC1:
		mask = KMIN_INTR_SCC_1;
		break;
	default:
#ifdef DIAGNOSTIC
		printf("warning: enabling unknown intr %p\n", cookie);
#endif
		return;
	}

#if defined(DEBUG)
	printf("3MIN: imask %x, enabling slot %p, dev %p handler %p\n",
	    kmin_tc3_imask, cookie, dev, handler);
#endif

	/*
	 * Enable the interrupt  handler, and if it's an IOASIC
	 * slot, set the IOASIC interrupt mask.
	 * Otherwise, set the appropriate spl level in the R3000
	 * register.
	 * Be careful to set handlers before enabling, and disable
	 * interrupts before clearing handlers.
	 */

	/* Set the interrupt handler and argument ... */
	intrtab[(uintptr_t)cookie].ih_func = handler;
	intrtab[(uintptr_t)cookie].ih_arg = arg;
	/* ... and set the relevant mask */
	switch ((uintptr_t)cookie) {
	case SYS_DEV_OPT0:
	case SYS_DEV_OPT1:
	case SYS_DEV_OPT2:
		/* it's an option slot */
		{
		int s = splhigh();
		s |= mask;
		splx(s);
		}
		break;
	default:
		/* it's a baseboard device going via the IOASIC */
		kmin_tc3_imask |= mask;
		break;
	}

	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = kmin_tc3_imask;
	kn02ba_wbflush();
}


#define CHECKINTR(slot, bits)					\
    do {							\
	if (can_serve & (bits)) {				\
		intrtab[slot].ih_count.ev_count++;		\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	}							\
    } while (/*CONSTCOND*/0)

static void
dec_3min_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	static int user_warned = 0;
	static int intr_depth = 0;
	uint32_t old_mask;

	intr_depth++;
	old_mask = *(volatile uint32_t *)(ioasic_base + IOASIC_IMSK);

	if (ipending & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (ipending & MIPS_INT_MASK_3) {
		/* NB: status & MIPS_INT_MASK3 must also be set */
		/* masked interrupts are still observable */
		uint32_t intr, imsk, can_serve, turnoff;

		turnoff = 0;
		intr = *(volatile uint32_t *)(ioasic_base + IOASIC_INTR);
		imsk = *(volatile uint32_t *)(ioasic_base + IOASIC_IMSK);
		can_serve = intr & imsk;

		if (intr & IOASIC_INTR_SCSI_PTR_LOAD) {
			turnoff |= IOASIC_INTR_SCSI_PTR_LOAD;
#ifdef notdef
			asc_dma_intr();
#endif
		}

		if (intr & (IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E))
			turnoff |=
			    IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E;

		if (intr & IOASIC_INTR_LANCE_READ_E)
			turnoff |= IOASIC_INTR_LANCE_READ_E;

		if (turnoff)
			*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) =
			    ~turnoff;

		if (intr & KMIN_INTR_TIMEOUT) {
			kn02ba_errintr();
			pmax_memerr_evcnt.ev_count++;
		}

		if (intr & KMIN_INTR_CLOCK) {
			struct clockframe cf;

			__asm volatile("lbu $0,48(%0)" ::
				"r"(ioasic_base + IOASIC_SLOT_8_START));

			cf.pc = pc;
			cf.sr = status;
			hardclock(&cf);
			pmax_clock_evcnt.ev_count++;
		}

		/* If clock interrupts were enabled, re-enable them ASAP. */
		if (old_mask & KMIN_INTR_CLOCK) {
			/* ioctl interrupt mask to splclock and higher */
			*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK)
				= old_mask &
					~(KMIN_INTR_SCC_0|KMIN_INTR_SCC_1 |
					  IOASIC_INTR_LANCE|IOASIC_INTR_SCSI);
			kn02ba_wbflush();
			_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_3));
		}

		if (intr_depth > 1)
			 goto done;

		CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
		CHECKINTR(SYS_DEV_SCC1, IOASIC_INTR_SCC_1);

#ifdef notyet /* untested */
		/* If tty interrupts were enabled, re-enable them ASAP. */
		if ((old_mask & (KMIN_INTR_SCC_1|KMIN_INTR_SCC_0)) ==
		     (KMIN_INTR_SCC_1|KMIN_INTR_SCC_0)) {
			*imaskp = old_mask &
			  ~(KMIN_INTR_SCC_0|KMIN_INTR_SCC_1 |
			  IOASIC_INTR_LANCE|IOASIC_INTR_SCSI);
			kn02ba_wbflush();
		}

		/* XXX until we know about SPLs of TC options. */
		if (intr_depth > 1)
			 goto done;
#endif
		CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
		CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);

		if (user_warned && ((intr & KMIN_INTR_PSWARN) == 0)) {
			printf("%s\n", "Power supply ok now.");
			user_warned = 0;
		}
		if ((intr & KMIN_INTR_PSWARN) && (user_warned < 3)) {
			user_warned++;
			printf("%s\n", "Power supply overheating");
		}
	}
	if ((ipending & MIPS_INT_MASK_0) && intrtab[SYS_DEV_OPT0].ih_func) {
		(*intrtab[SYS_DEV_OPT0].ih_func)(intrtab[SYS_DEV_OPT0].ih_arg);
		intrtab[SYS_DEV_OPT0].ih_count.ev_count++;
 	}

	if ((ipending & MIPS_INT_MASK_1) && intrtab[SYS_DEV_OPT1].ih_func) {
		(*intrtab[SYS_DEV_OPT1].ih_func)(intrtab[SYS_DEV_OPT1].ih_arg);
		intrtab[SYS_DEV_OPT1].ih_count.ev_count++;
	}
	if ((ipending & MIPS_INT_MASK_2) && intrtab[SYS_DEV_OPT2].ih_func) {
		(*intrtab[SYS_DEV_OPT2].ih_func)(intrtab[SYS_DEV_OPT2].ih_arg);
		intrtab[SYS_DEV_OPT2].ih_count.ev_count++;
	}

done:
	/* restore entry state */
	splhigh();
	intr_depth--;
	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = old_mask;

	_splset(MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}



/*
 ************************************************************************
 * Extra functions
 ************************************************************************
 */

static void
kn02ba_wbflush(void)
{

	/* read twice IOASIC_IMSK */
	__asm volatile("lw $0,%0; lw $0,%0" ::
	    "i"(MIPS_PHYS_TO_KSEG1(KMIN_REG_IMSK)));
}

/*
 * Support for using the MIPS 3 clock as a timecounter.
 */

void
dec_3min_tc_init(void)
{
#if defined(MIPS3)
	static struct timecounter tc =  {
		.tc_get_timecount = (timecounter_get_t *)mips3_cp0_count_read,
		.tc_counter_mask = ~0u,
		.tc_name = "mips3_cp0_counter",
		.tc_quality = 100,
	};

	if (MIPS_HAS_CLOCK) {
		tc.tc_frequency = cpu_mhz * 1000000;
		if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT) {
			tc.tc_frequency /= 2;
		}

		tc_init(&tc);
	}
#endif
}
