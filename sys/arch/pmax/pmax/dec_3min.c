/*	$NetBSD: dec_3min.c,v 1.8 1999/02/18 10:24:16 jonathan Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: dec_3min.c,v 1.8 1999/02/18 10:24:16 jonathan Exp $");


#include <sys/types.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/locore.h>		/* wbflush() */
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
#include <pmax/pmax/dec_kn02_subr.h>	/* 3min/maxine memory errors */


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
int		dec_3min_intr __P((u_int mask, u_int pc, 
			      u_int statusReg, u_int causeReg));

void		dec_3min_device_register __P((struct device *, void *));
void		dec_3min_cons_init __P((void));


/*
 * Local declarations.
 */
void dec_3min_mcclock_cpuspeed __P((volatile struct chiptime *mcclock_addr,
			       int clockmask));
u_long	kmin_tc3_imask;


/*
 * Fill in platform struct. 
 */
void
dec_3min_init()
{

	platform.iobus = "tcbus";

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

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	wbflush();

	*(volatile u_int *)IOASIC_REG_INTR(ioasic_base) = 0;
	wbflush();

}

  
void
dec_3min_os_init()
{
	ioasic_base = MIPS_PHYS_TO_KSEG1(KMIN_SYS_ASIC);
	mips_hardware_intr = dec_3min_intr;
	tc_enable_interrupt = dec_3min_enable_intr;
	kmin_tc3_imask = (KMIN_INTR_CLOCK | KMIN_INTR_PSWARN |
		KMIN_INTR_TIMEOUT);

	/*
	 * All the baseboard interrupts come through the I/O ASIC
	 * (at INT_MASK_3), so  it has to be turned off for all the spls.
	 * Since we don't know what kinds of devices are in the
	 * turbochannel option slots, just block them all.
	 */
	Mach_splbio = cpu_spl3;
	Mach_splnet = cpu_spl3;
	Mach_spltty = cpu_spl3;
	Mach_splimp = cpu_spl3;
	Mach_splclock = cpu_spl3;
	Mach_splstatclock = cpu_spl3;
	mcclock_addr = (volatile struct chiptime *)
		MIPS_PHYS_TO_KSEG1(KMIN_SYS_CLOCK);
	dec_3min_mcclock_cpuspeed(mcclock_addr, MIPS_INT_MASK_3);

	/*
	 * Initialize interrupts.
	 */
	*(u_int *)IOASIC_REG_IMSK(ioasic_base) = KMIN_IM0;
	*(u_int *)IOASIC_REG_INTR(ioasic_base) = 0;

	/* clear any memory errors from probes */

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	wbflush();

	/*
	 * The kmin memory hardware seems to wrap  memory addresses
	 * with 4Mbyte SIMMs, which causes the physmem computation
	 * to lose.  Find out how big the SIMMS are and set
	 * max_	physmem accordingly.
	 * XXX Do MAXINEs lose the same way?
	 */
	physmem_boardmax = KMIN_PHYS_MEMORY_END + 1;
	if ((*(int*)(MIPS_PHYS_TO_KSEG1(KMIN_REG_MSR)) &
	     KMIN_MSR_SIZE_16Mb) == 0)
		physmem_boardmax = physmem_boardmax >> 2;
	physmem_boardmax = MIPS_PHYS_TO_KSEG1(physmem_boardmax);
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
	register unsigned int slotno;
	int (*handler) __P((void* softc));
	void *sc;
	int on;
{
	register unsigned mask;

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
dec_3min_intr(mask, pc, statusReg, causeReg)
	unsigned mask;
	unsigned pc;
	unsigned statusReg;
	unsigned causeReg;
{
	register u_int intr;
	register volatile struct chiptime *c = 
	    (volatile struct chiptime *) MIPS_PHYS_TO_KSEG1(KMIN_SYS_CLOCK);
	volatile u_int *imaskp =
		(volatile u_int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_IMSK);
	volatile u_int *intrp =
		(volatile u_int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_INTR);
	unsigned int old_mask;
	struct clockframe cf;
	int temp;
	static int user_warned = 0;

	old_mask = *imaskp & kmin_tc3_imask;
	*imaskp = kmin_tc3_imask |
		 (KMIN_IM0 & ~(KN03_INTR_TC_0|KN03_INTR_TC_1|KN03_INTR_TC_2));

	if (mask & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (mask & MIPS_INT_MASK_3) {
		intr = *intrp;

		/* masked interrupts are still observable */
		intr &= old_mask;
	
		if (intr & IOASIC_INTR_SCSI_PTR_LOAD) {
			*intrp &= ~IOASIC_INTR_SCSI_PTR_LOAD;
#ifdef notdef
			asc_dma_intr();
#endif
		}
	
		if (intr & (IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E))
			*intrp &= ~(IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);

		if (intr & IOASIC_INTR_LANCE_READ_E)
			*intrp &= ~IOASIC_INTR_LANCE_READ_E;

		if (intr & KMIN_INTR_TIMEOUT)
			kn02ba_errintr();
	
		if (intr & KMIN_INTR_CLOCK) {
			extern u_int32_t mips3_cycle_count __P((void));

			temp = c->regc;	/* XXX clear interrupt bits */
			cf.pc = pc;
			cf.sr = statusReg;
#ifdef MIPS3
			if (CPUISMIPS3) {
				latched_cycle_cnt = mips3_cycle_count();
			}
#endif
			hardclock(&cf);
			intrcnt[HARDCLOCK]++;
		}
	
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
	
		if ((intr & IOASIC_INTR_SCSI) &&
		    tc_slot_info[KMIN_SCSI_SLOT].intr) {
			(*(tc_slot_info[KMIN_SCSI_SLOT].intr))
			  (tc_slot_info[KMIN_SCSI_SLOT].sc);
			intrcnt[SCSI_INTR]++;
		}

		if ((intr & IOASIC_INTR_LANCE) &&
		    tc_slot_info[KMIN_LANCE_SLOT].intr) {
			(*(tc_slot_info[KMIN_LANCE_SLOT].intr))
			  (tc_slot_info[KMIN_LANCE_SLOT].sc);
			intrcnt[LANCE_INTR]++;
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
	if ((mask & MIPS_INT_MASK_0) && tc_slot_info[0].intr) {
		(*tc_slot_info[0].intr)(tc_slot_info[0].sc);
		intrcnt[SLOT0_INTR]++;
 	}
	
	if ((mask & MIPS_INT_MASK_1) && tc_slot_info[1].intr) {
		(*tc_slot_info[1].intr)(tc_slot_info[1].sc);
		intrcnt[SLOT1_INTR]++;
	}
	if ((mask & MIPS_INT_MASK_2) && tc_slot_info[2].intr) {
		(*tc_slot_info[2].intr)(tc_slot_info[2].sc);
		intrcnt[SLOT2_INTR]++;
	}

#if 0 /*XXX*/
	if (mask & (MIPS_INT_MASK_2|MIPS_INT_MASK_1|MIPS_INT_MASK_0))
		printf("kmin: slot intr, mask 0x%x\n",
			mask &
			(MIPS_INT_MASK_2|MIPS_INT_MASK_1|MIPS_INT_MASK_0));
#endif
	
	return ((statusReg & ~causeReg & MIPS_HARD_INT_MASK) |
		MIPS_SR_INT_ENA_CUR);
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
	register volatile u_int * ioasic_intrmaskp =
		(volatile u_int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_IMSK);

	register int saved_imask = *ioasic_intrmaskp;

	/* Allow only clock interrupts through ioasic. */
	*ioasic_intrmaskp = KMIN_INTR_CLOCK;
	wbflush();
     
	mc_cpuspeed(mcclock_addr, clockmask);

	*ioasic_intrmaskp = saved_imask;
	wbflush();
}

