/*	$NetBSD: dec_3min.c,v 1.17 1999/05/25 07:37:08 nisimura Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: dec_3min.c,v 1.17 1999/05/25 07:37:08 nisimura Exp $");


#include <sys/types.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/autoconf.h>		/* intr_arg_t */
#include <machine/sysconf.h>

#include <mips/mips_param.h>		/* hokey spl()s */
#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

/* all these to get ioasic_base */
#include <sys/device.h>			/* struct cfdata for.. */
#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <dev/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/pmax/clockreg.h>
#include <pmax/pmax/turbochannel.h>
#include <pmax/pmax/pmaxtype.h>

#include <pmax/pmax/machdep.h>		/* XXXjrs replace with vectors */

#include <pmax/pmax/kmin.h>		/* 3min baseboard addresses */
#include <pmax/pmax/memc.h>		/* 3min/maxine memory errors */


/*
 * forward declarations
 */
void		dec_3min_init __P((void));
void		dec_3min_os_init __P((void));
void		dec_3min_bus_reset __P((void));
void		dec_3maxplus_device_register __P((struct device *, void *));

void		dec_3min_enable_intr
		   __P ((u_int slotno, int (*handler) __P((intr_arg_t sc)),
			 intr_arg_t sc, int onoff));
int		dec_3min_intr __P((unsigned, unsigned, unsigned, unsigned));
void		dec_3min_device_register __P((struct device *, void *));
void		dec_3min_cons_init __P((void));


/*
 * Local declarations.
 */
void dec_3min_mcclock_cpuspeed __P((volatile struct chiptime *mcclock_addr,
			       int clockmask));
u_long	kmin_tc3_imask;

void kn02ba_wbflush __P((void));
unsigned kn02ba_clkread __P((void));
extern unsigned (*clkread) __P((void));

/*
 * Fill in platform struct.
 */
void
dec_3min_init()
{
	platform.iobus = "tc3min";

	platform.os_init = dec_3min_os_init;
	platform.bus_reset = dec_3min_bus_reset;
	platform.cons_init = dec_3min_cons_init;
	platform.device_register = dec_3min_device_register;

	dec_3min_os_init();

	sprintf(cpu_model, "DECstation 5000/1%d (3MIN)", cpu_mhz);
}


/*
 * Initalize the memory system and I/O buses.
 */
void
dec_3min_bus_reset()
{

	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ba_wbflush();
}


void
dec_3min_os_init()
{
	ioasic_base = MIPS_PHYS_TO_KSEG1(KMIN_SYS_ASIC);
	mips_hardware_intr = dec_3min_intr;
	tc_enable_interrupt = dec_3min_enable_intr;	/* XXX */
	mcclock_addr = (void *)(ioasic_base + IOASIC_SLOT_8_START);

	/* R4000 3MIN can ultilize on-chip counter */
	clkread = kn02ba_clkread;

	/*
	 * All the baseboard interrupts come through the I/O ASIC
	 * (at INT_MASK_3), so  it has to be turned off for all the spls.
	 * Since we don't know what kinds of devices are in the
	 * turbochannel option slots, just block them all.
	 */
	splvec.splbio = MIPS_SPL_0_1_2_3;
	splvec.splnet = MIPS_SPL_0_1_2_3;
	splvec.spltty = MIPS_SPL_0_1_2_3;
	splvec.splimp = MIPS_SPL_0_1_2_3;
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;

	dec_3min_mcclock_cpuspeed(mcclock_addr, MIPS_INT_MASK_3);

	*(u_int32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(u_int32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(u_int32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(u_int32_t *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(u_int32_t *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif
	/*
	 * Initialize interrupts.
	 */
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = KMIN_IM0;
	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;

	/*
	 * The kmin memory hardware seems to wrap  memory addresses
	 * with 4Mbyte SIMMs, which causes the physmem computation
	 * to lose.  Find out how big the SIMMS are and set
	 * max_	physmem accordingly.
	 */
	physmem_boardmax = KMIN_PHYS_MEMORY_END + 1;
	if ((*(int *)(MIPS_PHYS_TO_KSEG1(KMIN_REG_MSR)) &
	     KMIN_MSR_SIZE_16Mb) == 0)
		physmem_boardmax = physmem_boardmax >> 2;
	physmem_boardmax = MIPS_PHYS_TO_KSEG1(physmem_boardmax);

	/* clear any memory errors from probes */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	kmin_tc3_imask =
		(KMIN_INTR_CLOCK | KMIN_INTR_PSWARN | KMIN_INTR_TIMEOUT);

	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) =
		kmin_tc3_imask |
		(KMIN_IM0 & ~(KN03_INTR_TC_0|KN03_INTR_TC_1|KN03_INTR_TC_2));
}


void
dec_3min_cons_init()
{
	/* notyet */
}


void
dec_3min_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3min_device_register unimplemented");
}


void
dec_3min_enable_intr(slotno, handler, sc, on)
	unsigned int slotno;
	int (*handler) __P((void* softc));
	void *sc;
	int on;
{
	unsigned mask;

	switch (slotno) {
		/* slots 0-2 don't interrupt through the IOASIC. */
	case 0:
		mask = MIPS_INT_MASK_0;	break;
	case 1:
		mask = MIPS_INT_MASK_1; break;
	case 2:
		mask = MIPS_INT_MASK_2; break;

	case KMIN_SCSI_SLOT:
		mask = (IOASIC_INTR_SCSI | IOASIC_INTR_SCSI_PTR_LOAD |
			IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);
		break;

	case KMIN_LANCE_SLOT:
		mask = KMIN_INTR_LANCE;
		break;
	case KMIN_SCC0_SLOT:
		mask = KMIN_INTR_SCC_0;
		break;
	case KMIN_SCC1_SLOT:
		mask = KMIN_INTR_SCC_1;
		break;
	case KMIN_ASIC_SLOT:
		mask = KMIN_INTR_ASIC;
		break;
	default:
		return;
	}

#if defined(DEBUG) || defined(DIAGNOSTIC)
	printf("3MIN: imask %lx, %sabling slot %d, sc %p handler %p\n",
	       kmin_tc3_imask, (on? "en" : "dis"), slotno, sc, handler);
#endif

	/*
	 * Enable the interrupt  handler, and if it's an IOASIC
	 * slot, set the IOASIC interrupt mask.
	 * Otherwise, set the appropriate spl level in the R3000
	 * register.
	 * Be careful to set handlers  before enabling, and disable
	 * interrupts before clearing handlers.
	 */

	if (on) {
		/* Set the interrupt handler and argument ... */
		tc_slot_info[slotno].intr = handler;
		tc_slot_info[slotno].sc = sc;

		/* ... and set the relevant mask */
		if (slotno <= 2) {
			/* it's an option slot */
			int s = splhigh();
			s  |= mask;
			splx(s);
		} else {
			/* it's a baseboard device going via the ASIC */
			kmin_tc3_imask |= mask;
		}
	} else {
		/* Clear the relevant mask... */
		if (slotno <= 2) {
			/* it's an option slot */
			int s = splhigh();
			printf("kmin_intr: cannot disable option slot %d\n",
			    slotno);
			s &= ~mask;
			splx(s);
		} else {
			/* it's a baseboard device going via the ASIC */
			kmin_tc3_imask &= ~mask;
		}
		/* ... and clear the handler */
		tc_slot_info[slotno].intr = 0;
		tc_slot_info[slotno].sc = 0;
	}
}



/*
 * 3min hardware interrupts. (DECstation 5000/1xx)
 */
int
dec_3min_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	static int user_warned = 0;
	static int intr_depth = 0;
	u_int32_t old_mask;

	intr_depth++;
	old_mask = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);

	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (cpumask & MIPS_INT_MASK_3) {
		/* NB: status & MIPS_INT_MASK3 must also be set */
		/* masked interrupts are still observable */
		u_int32_t intr, imsk, turnoff;

		turnoff = 0;
		intr = *(u_int32_t *)(ioasic_base + IOASIC_INTR);
		imsk = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
		intr &= imsk;

		if (intr & IOASIC_INTR_SCSI_PTR_LOAD) {
			turnoff |= IOASIC_INTR_SCSI_PTR_LOAD;
#ifdef notdef
			asc_dma_intr();
#endif
		}

		if (intr & (IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E))
			turnoff |= IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E;

		if (intr & IOASIC_INTR_LANCE_READ_E)
			turnoff |= IOASIC_INTR_LANCE_READ_E;

		if (turnoff)
			*(u_int32_t *)(ioasic_base + IOASIC_INTR) = ~turnoff;

		if (intr & KMIN_INTR_TIMEOUT)
			kn02ba_errintr();

		if (intr & KMIN_INTR_CLOCK) {
			struct clockframe cf;
			struct chiptime *clk;
			volatile int temp;
			extern u_int32_t mips3_cycle_count __P((void));

			clk = (void *)(ioasic_base + IOASIC_SLOT_8_START);
			temp = clk->regc;	/* XXX clear interrupt bits */

#ifdef MIPS3
			if (CPUISMIPS3) {
				latched_cycle_cnt = mips3_cycle_count();
			}
#endif
			cf.pc = pc;
			cf.sr = status;
			hardclock(&cf);
			intrcnt[HARDCLOCK]++;
		}

		/* If clock interrups were enabled, re-enable them ASAP. */
		if (old_mask & KMIN_INTR_CLOCK) {
			/* ioctl interrupt mask to splclock and higher */
			*(u_int32_t *)(ioasic_base + IOASIC_IMSK)
				= old_mask &
					~(KMIN_INTR_SCC_0|KMIN_INTR_SCC_1 |
					  IOASIC_INTR_LANCE|IOASIC_INTR_SCSI);
			kn02ba_wbflush();
			_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_3));
		}

		if (intr_depth > 1)
			 goto done;

		if ((intr & KMIN_INTR_SCC_0) &&
		    tc_slot_info[KMIN_SCC0_SLOT].intr) {
			(*(tc_slot_info[KMIN_SCC0_SLOT].intr))
			  (tc_slot_info[KMIN_SCC0_SLOT].sc);
			intrcnt[SERIAL0_INTR]++;
		}

		if ((intr & KMIN_INTR_SCC_1) &&
		    tc_slot_info[KMIN_SCC1_SLOT].intr) {
			(*(tc_slot_info[KMIN_SCC1_SLOT].intr))
			  (tc_slot_info[KMIN_SCC1_SLOT].sc);
			intrcnt[SERIAL1_INTR]++;
		}

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
		if ((intr & IOASIC_INTR_LANCE) &&
		    tc_slot_info[KMIN_LANCE_SLOT].intr) {
			(*(tc_slot_info[KMIN_LANCE_SLOT].intr))
			  (tc_slot_info[KMIN_LANCE_SLOT].sc);
			intrcnt[LANCE_INTR]++;
		}

		if ((intr & IOASIC_INTR_SCSI) &&
		    tc_slot_info[KMIN_SCSI_SLOT].intr) {
			(*(tc_slot_info[KMIN_SCSI_SLOT].intr))
			  (tc_slot_info[KMIN_SCSI_SLOT].sc);
			intrcnt[SCSI_INTR]++;
		}

		if (user_warned && ((intr & KMIN_INTR_PSWARN) == 0)) {
			printf("%s\n", "Power supply ok now.");
			user_warned = 0;
		}
		if ((intr & KMIN_INTR_PSWARN) && (user_warned < 3)) {
			user_warned++;
			printf("%s\n", "Power supply overheating");
		}
	}
	if ((cpumask & MIPS_INT_MASK_0) && tc_slot_info[0].intr) {
		(*tc_slot_info[0].intr)(tc_slot_info[0].sc);
		intrcnt[SLOT0_INTR]++;
 	}

	if ((cpumask & MIPS_INT_MASK_1) && tc_slot_info[1].intr) {
		(*tc_slot_info[1].intr)(tc_slot_info[1].sc);
		intrcnt[SLOT1_INTR]++;
	}
	if ((cpumask & MIPS_INT_MASK_2) && tc_slot_info[2].intr) {
		(*tc_slot_info[2].intr)(tc_slot_info[2].sc);
		intrcnt[SLOT2_INTR]++;
	}

done:
	/* restore entry state */
	splhigh();
	intr_depth--;
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = old_mask;

	
	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}



/*
 ************************************************************************
 * Extra functions
 ************************************************************************
 */




/*
 * Count instructions between 4ms mcclock interrupt requests,
 * using the ioasic clock-interrupt-pending bit to determine
 * when clock ticks occur.
 * Set up iosiac to allow only clock interrupts, then
 * call
 */
void
dec_3min_mcclock_cpuspeed(mcclock_addr, clockmask)
	volatile struct chiptime *mcclock_addr;
	int clockmask;
{
	u_int32_t saved_imask;

	saved_imask = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);

	/* Allow only clock interrupts through ioasic. */
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = KMIN_INTR_CLOCK;
	kn02ba_wbflush();

	mc_cpuspeed(mcclock_addr, clockmask);

	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = saved_imask;
	kn02ba_wbflush();
}

void
kn02ba_wbflush()
{
	/* read twice IOASIC_INTR register */
	__asm __volatile("lw $0,0xbc040120; lw $0,0xbc040120");
}

unsigned
kn02ba_clkread()
{
#ifdef MIPS3
	extern u_int32_t mips3_cycle_count __P((void));
	extern u_long latched_cycle_cnt;

	if (CPUISMIPS3) {
		u_int32_t mips3_cycles;

		mips3_cycles = mips3_cycle_count() - latched_cycle_cnt;
#if 0
		/* XXX divides take 78 cycles: approximate with * 41/2048 */
		return (mips3_cycles / cpu_mhz);
#else
		return((mips3_cycles >> 6) + (mips3_cycles >> 8) +
		       (mips3_cycles >> 11));
#endif
	}
#endif
	return 0;
}
