/*	$NetBSD: dec_3maxplus.c,v 1.17 1999/05/21 01:09:49 nisimura Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: dec_3maxplus.c,v 1.17 1999/05/21 01:09:49 nisimura Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/autoconf.h>		/* intr_arg_t */
#include <machine/sysconf.h>

#include <mips/mips_param.h>		/* hokey spl()s */
#include <mips/mips/mips_mcclock.h>	/* mclock CPUspeed estimation */

/* all these to get ioasic_base */
#include <sys/device.h>			/* struct cfdata for.. */
#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <dev/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/pmax/clockreg.h>
#include <pmax/pmax/turbochannel.h>
#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/machdep.h>		/* XXXjrs replace with vectors */

#include <pmax/pmax/kn03.h>
#include <pmax/pmax/memc.h>

/*
 * Forward declarations
 */
void		dec_3maxplus_init __P((void));
void		dec_3maxplus_os_init __P((void));
void		dec_3maxplus_bus_reset __P((void));
void		dec_3maxplus_enable_intr
		   __P ((u_int slotno, int (*handler) __P((intr_arg_t sc)),
			 intr_arg_t sc, int onoff));

int		dec_3maxplus_intr __P((u_int mask, u_int pc,
			      u_int statusReg, u_int causeReg));

void		dec_3maxplus_cons_init __P((void));
void		dec_3maxplus_device_register __P((struct device *, void *));
static void 	dec_3maxplus_errintr __P ((void));


void kn03_wbflush __P((void));
unsigned kn03_clkread __P((void));
extern unsigned (*clkread) __P((void));

/*
 * Local declarations
 */
u_long	kn03_tc3_imask;
void	dec_3maxplus_tc_reset __P((void));		/* XXX unused? */


/*
 * Fill in platform struct.
 */
void
dec_3maxplus_init()
{
	volatile u_int *intrp =
	    (volatile u_int *) MIPS_PHYS_TO_KSEG1(KN03_REG_INTR);
	u_int intr;

	platform.iobus = "tcioasic";

	platform.os_init = dec_3maxplus_os_init;
	platform.bus_reset = dec_3maxplus_bus_reset;
	platform.cons_init = dec_3maxplus_cons_init;
	platform.device_register = dec_3maxplus_device_register;

	dec_3maxplus_os_init();

	intr = *intrp;
	if (intr & KN03_INTR_PROD_JUMPER)
		sprintf(cpu_model, "DECstation 5000/2%c0 (3MAXPLUS)",
		    CPUISMIPS3 ? '6' : '4');
	else
		sprintf(cpu_model, "DECsystem 5900%s (3MAXPLUS)",
		    CPUISMIPS3 ? "-260" : "");
}


/*
 * Set up OS level stuff: spls, etc.
 */
void
dec_3maxplus_os_init()
{
	ioasic_base = MIPS_PHYS_TO_KSEG1(KN03_SYS_ASIC);
	mips_hardware_intr = dec_3maxplus_intr;
	tc_enable_interrupt = dec_3maxplus_enable_intr;

	/* clear any pending memory errors. */
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	/*
	 * Reset interrupts.
	 */
	Mach_splbio = Mach_spl0;
	Mach_splnet = Mach_spl0;
	Mach_spltty = Mach_spl0;
	Mach_splimp = Mach_spl0;	/* XXX */
	/*
	 * Clock interrupts at hw priority 1 must block bio,net,tty
	 * at hw priority 0.
	 */
	Mach_splclock = cpu_spl1;
	Mach_splstatclock = cpu_spl1;
	mcclock_addr = (volatile struct chiptime *)
		MIPS_PHYS_TO_KSEG1(KN03_SYS_CLOCK);
	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_1);

	/*
	 * Initialize interrupts.
	 */
	kn03_tc3_imask = KN03_IM0 &
		~(KN03_INTR_TC_0|KN03_INTR_TC_1|KN03_INTR_TC_2);
	*(u_int *)IOASIC_REG_IMSK(ioasic_base) = kn03_tc3_imask;
	*(u_int *)IOASIC_REG_INTR(ioasic_base) = 0;
	kn03_wbflush();

	*(volatile u_int *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(volatile u_int *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(volatile u_int *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(volatile u_int *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(volatile u_int *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif
	/* XXX hard-reset LANCE */
	*(u_int *)IOASIC_REG_CSR(ioasic_base) |= 0x100;

	/* clear any memory errors from probes */
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	/* 3MAX+ has IOASIC free-running high resolution timer */
	clkread = kn03_clkread;
}


/*
 * Initalize the memory system and I/O buses.
 */
void
dec_3maxplus_bus_reset()
{
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	wbflush();

	*(volatile u_int *)IOASIC_REG_INTR(ioasic_base) = 0;
	wbflush();

}


void
dec_3maxplus_cons_init()
{
}


void
dec_3maxplus_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3maxplus_device_register unimplemented");
}



/*
 *	Enable/Disable interrupts.
 *	We pretend we actually have 8 slots even if we really have
 *	only 4: TCslots 0-2 maps to slots 0-2, TCslot3 maps to
 *	slots 3-7 (see pmax/tc/ds-asic-conf.c).
 */
void
dec_3maxplus_enable_intr(slotno, handler, sc, on)
	unsigned int slotno;
	int (*handler) __P((void* softc));
	void *sc;
	int on;
{
	unsigned mask;

#if 0
	printf("3MAXPLUS: imask %x, %sabling slot %d, unit %d addr 0x%x\n",
	       kn03_tc3_imask, (on? "en" : "dis"), slotno, unit, handler);
#endif

	switch (slotno) {
	case 0:
		mask = KN03_INTR_TC_0;
		break;
	case 1:
		mask = KN03_INTR_TC_1;
		break;
	case 2:
		mask = KN03_INTR_TC_2;
		break;
	case KN03_SCSI_SLOT:
		mask = (IOASIC_INTR_SCSI | IOASIC_INTR_SCSI_PTR_LOAD |
			IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);
		break;
	case KN03_LANCE_SLOT:
		mask = KN03_INTR_LANCE;
		mask |= IOASIC_INTR_LANCE_READ_E;
		break;
	case KN03_SCC0_SLOT:
		mask = KN03_INTR_SCC_0;
		break;
	case KN03_SCC1_SLOT:
		mask = KN03_INTR_SCC_1;
		break;
	case KN03_ASIC_SLOT:
		mask = KN03_INTR_ASIC;
		break;
	default:
#ifdef DIAGNOSTIC
		printf("warning: enabling unknown intr %x\n", slotno);
#endif
		goto done;
	}
	if (on) {
		kn03_tc3_imask |= mask;
		tc_slot_info[slotno].intr = handler;
		tc_slot_info[slotno].sc = sc;

	} else {
		kn03_tc3_imask &= ~mask;
		tc_slot_info[slotno].intr = 0;
		tc_slot_info[slotno].sc = 0;
	}
done:
	*(u_int *)IOASIC_REG_IMSK(ioasic_base) = kn03_tc3_imask;
	wbflush();
}


/*
 * 3Max+ hardware interrupts. (DECstation 5000/240)
 */
int
dec_3maxplus_intr(mask, pc, statusReg, causeReg)
	unsigned mask;
	unsigned pc;
	unsigned statusReg;
	unsigned causeReg;
{
	u_int intr;
	volatile struct chiptime *c =
	    (volatile struct chiptime *) MIPS_PHYS_TO_KSEG1(KN03_SYS_CLOCK);
	volatile u_int *imaskp = (volatile u_int *)
		MIPS_PHYS_TO_KSEG1(KN03_REG_IMSK);
	volatile u_int *intrp = (volatile u_int *)
		MIPS_PHYS_TO_KSEG1(KN03_REG_INTR);
	u_int old_mask;
	struct clockframe cf;
	int temp;
	static int user_warned = 0;
	u_long old_buscycle = latched_cycle_cnt;

	old_mask = *imaskp & kn03_tc3_imask;
	*imaskp = kn03_tc3_imask;

	if (mask & MIPS_INT_MASK_4)
		prom_haltbutton();

	/* handle clock interrupts ASAP */
	if (mask & MIPS_INT_MASK_1) {
		temp = c->regc;	/* XXX clear interrupt bits */
		cf.pc = pc;
		cf.sr = statusReg;
		latched_cycle_cnt = *(u_long*)(IOASIC_REG_CTR(ioasic_base));
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;
		old_buscycle = latched_cycle_cnt - old_buscycle;
		/* keep clock interrupts enabled when we return */
		causeReg &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrups were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_ENA_CUR | (statusReg & MIPS_INT_MASK_1));

	/*
	 * Check for late clock interrupts (allow 10% slop). Be careful
	 * to do so only after calling hardclock(), due to logging cost.
	 * Even then, logging dropped ticks just causes more clock
	 * ticks to be missed.
	 */
#ifdef notdef
	if ((mask & MIPS_INT_MASK_1) && old_buscycle > (tick+49) * 25) {
		extern int msgbufmapped;
  		if(msgbufmapped && 0)
			 addlog("kn03: clock intr %d usec late\n",
				 old_buscycle/25);
	}
#endif
	/*
	 * IOCTL asic DMA-related interrupts should be checked here,
	 * and DMA pointers serviced as soon as possible.
	 */

	if (mask & MIPS_INT_MASK_0) {
		intr = *intrp;
		/* masked interrupts are still observable */
		intr &= old_mask;

		if (intr & IOASIC_INTR_SCSI_PTR_LOAD) {
			*intrp &= ~IOASIC_INTR_SCSI_PTR_LOAD;
#ifdef notdef
			asc_dma_intr();
#endif
		}

	/*
	 * XXX
	 * DMA and non-DMA  interrupts from the IOCTl asic all use the
	 * single interrupt request line from the IOCTL asic.
	 * Disabling IOASIC interrupts while servicing network or
	 * disk-driver interrupts causes DMA overruns. NON-dma IOASIC
	 * interrupts should be disabled in the ioasic, and
	 * interrupts from the IOASIC itself should be re-enabled.
	 * DMA interrupts can then be serviced whilst still servicing
	 * non-DMA interrupts from ioctl devices or TC options.
	 */

		if (intr & (IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E))
			*intrp &= ~(IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);

		if (intr & IOASIC_INTR_LANCE_READ_E)
			*intrp &= ~IOASIC_INTR_LANCE_READ_E;

		if ((intr & KN03_INTR_SCC_0) &&
			tc_slot_info[KN03_SCC0_SLOT].intr) {
			(*(tc_slot_info[KN03_SCC0_SLOT].intr))
			(tc_slot_info[KN03_SCC0_SLOT].sc);
			intrcnt[SERIAL0_INTR]++;
		}

		if ((intr & KN03_INTR_SCC_1) &&
			tc_slot_info[KN03_SCC1_SLOT].intr) {
			(*(tc_slot_info[KN03_SCC1_SLOT].intr))
			(tc_slot_info[KN03_SCC1_SLOT].sc);
			intrcnt[SERIAL1_INTR]++;
		}

		if ((intr & KN03_INTR_TC_0) &&
			tc_slot_info[0].intr) {
			(*(tc_slot_info[0].intr))
			(tc_slot_info[0].sc);
			intrcnt[SLOT0_INTR]++;
		}
#ifdef DIAGNOSTIC
		else if (intr & KN03_INTR_TC_0)
			printf ("can't handle tc0 interrupt\n");
#endif /*DIAGNOSTIC*/

		if ((intr & KN03_INTR_TC_1) &&
			tc_slot_info[1].intr) {
			(*(tc_slot_info[1].intr))
			(tc_slot_info[1].sc);
			intrcnt[SLOT1_INTR]++;
		}
#ifdef DIAGNOSTIC
		else if (intr & KN03_INTR_TC_1)
			printf ("can't handle tc1 interrupt\n");
#endif /*DIAGNOSTIC*/

		if ((intr & KN03_INTR_TC_2) &&
			tc_slot_info[2].intr) {
			(*(tc_slot_info[2].intr))
			(tc_slot_info[2].sc);
			intrcnt[SLOT2_INTR]++;
		}
#ifdef DIAGNOSTIC
		else if (intr & KN03_INTR_TC_2)
			printf ("can't handle tc2 interrupt\n");
#endif /*DIAGNOSTIC*/

		if ((intr & KN03_INTR_LANCE) &&
			tc_slot_info[KN03_LANCE_SLOT].intr) {
			(*(tc_slot_info[KN03_LANCE_SLOT].intr))
			(tc_slot_info[KN03_LANCE_SLOT].sc);
			intrcnt[LANCE_INTR]++;
		}

		if ((intr & IOASIC_INTR_SCSI) &&
			tc_slot_info[KN03_SCSI_SLOT].intr) {
			(*(tc_slot_info[KN03_SCSI_SLOT].intr))
			(tc_slot_info[KN03_SCSI_SLOT].sc);
			intrcnt[SCSI_INTR]++;
		}

		if (user_warned && ((intr & KN03_INTR_PSWARN) == 0)) {
			printf("%s\n", "Power supply ok now.");
			user_warned = 0;
		}
		if ((intr & KN03_INTR_PSWARN) && (user_warned < 3)) {
			user_warned++;
			printf("%s\n", "Power supply overheating");
		}
	}
	if (mask & MIPS_INT_MASK_3)
		dec_3maxplus_errintr();
	return ((statusReg & ~causeReg & MIPS_HARD_INT_MASK) |
		MIPS_SR_INT_ENA_CUR);
}


/*
 * Handle Memory error.   3max, 3maxplus has ECC.
 * Correct single-bit error, panic on  double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3maxplus_errintr()
{
	u_int erradr, errsyn;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR);
	errsyn = MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRSYN);
	*(u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	wbflush();

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn);
}

void
kn03_wbflush()
{
	/* read once IOASIC_INTR */
	__asm __volatile("lw $0,0xbf840000");
}

/*
 * TURBOchannel bus-cycle counter provided by IOASIC;
 * Interpolate micro-seconds since the last RTC clock tick.  The
 * interpolation base is the copy of the bus cycle-counter taken by
 * the RTC interrupt handler.
 */
unsigned
kn03_clkread()
{
	return *(u_int32_t *)(ioasic_base + IOASIC_CTR);
}
