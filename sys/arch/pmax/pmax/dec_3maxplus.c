/*	$NetBSD: dec_3maxplus.c,v 1.9.2.6 1999/03/18 07:27:28 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3maxplus.c,v 1.9.2.6 1999/03/18 07:27:28 nisimura Exp $");
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>	

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <pmax/pmax/pmaxtype.h> 

#include <mips/mips/mips_mcclock.h>	/* mcclock CPU speed estimation */
#include <pmax/pmax/clockreg.h>

#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <pmax/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <pmax/pmax/kn03.h>		/* baseboard addresses (constants) */
#include <pmax/pmax/dec_3max_subr.h>

#include <dev/ic/z8530sc.h>
#include <pmax/tc/zs_ioasicvar.h>	/* console */

#include "wsdisplay.h"

/* XXX XXX XXX */
#define	IOASIC_INTR_SCSI 0x00000200
/* XXX XXX XXX */

void dec_3maxplus_init __P((void));
void dec_3maxplus_os_init __P((void));
void dec_3maxplus_bus_reset __P((void));
void dec_3maxplus_cons_init __P((void));
void dec_3maxplus_device_register __P((struct device *, void *));
int  dec_3maxplus_intr __P((unsigned, unsigned, unsigned, unsigned));
void kn03_wbflush __P((void));
unsigned kn03_clkread __P((void));

static void dec_3maxplus_memerr __P ((void));

extern int _splraise_ioasic __P((int));
extern int _spllower_ioasic __P((int));
extern int _splx_ioasic __P((int));
extern void prom_haltbutton __P((void));
extern unsigned (*clkread) __P((void));
extern u_int32_t latched_cycle_cnt;
extern volatile struct chiptime *mcclock_addr;
extern char cpu_model[];

struct splsw spl_3maxplus = {
	{ _spllower_ioasic,	0 },
	{ _splraise_ioasic,	IPL_BIO },
	{ _splraise_ioasic,	IPL_NET },
	{ _splraise_ioasic,	IPL_TTY },
	{ _splraise_ioasic,	IPL_IMP },
	{ _splraise,		MIPS_SPL_0_1 },
	{ _splx_ioasic,		0 },
};

/*
 * Fill in platform struct. 
 */
void
dec_3maxplus_init()
{

	platform.iobus = "tcioasic";

	platform.os_init = dec_3maxplus_os_init;
	platform.bus_reset = dec_3maxplus_bus_reset;
	platform.cons_init = dec_3maxplus_cons_init;
	platform.device_register = dec_3maxplus_device_register;

	dec_3maxplus_os_init();

	sprintf(cpu_model, "DECstation 5000/2%c0 (3MAXPLUS)",
	    CPUISMIPS3 ? '6' : '4');
}

/*
 * Set up OS level stuff: spls, etc.
 */
void
dec_3maxplus_os_init()
{
	extern void ioasic_init __P((int));

	/* clear any memory errors from probes */
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KN03_SYS_ASIC);
	mcclock_addr = (void *)(ioasic_base + IOASIC_SLOT_8_START);
	mips_hardware_intr = dec_3maxplus_intr;

	/*
	 * 3MAX+ IOASIC interrupts come through INT 0, while
	 * clock interrupt does via INT 1.  splclock and splstatclock
	 * should block IOASIC activities.
	 */
#ifdef NEWSPL
	__spl = &spl_3maxplus;
#else
	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL0;
	splvec.spltty = MIPS_SPL0;
	splvec.splimp = MIPS_SPL0;
	splvec.splclock = MIPS_SPL_0_1;
	splvec.splstatclock = MIPS_SPL_0_1;
#endif

	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_1);

	*(volatile u_int *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(volatile u_int *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(volatile u_int *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(volatile u_int *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(volatile u_int *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif

	/* XXX hard-reset LANCE */
	*(volatile u_int *)(ioasic_base + IOASIC_CSR) |= 0x100;

	/*
	 * Initialize interrupts.
	 */
	*(volatile u_int *)(ioasic_base + IOASIC_IMSK) = KN03_INTR_PSWARN;
	*(volatile u_int *)(ioasic_base + IOASIC_INTR) = 0;
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
	kn03_wbflush();

	*(volatile u_int *)(ioasic_base + IOASIC_INTR) = 0;
	kn03_wbflush();
}


#include <dev/cons.h>
#include <sys/termios.h>

extern void prom_findcons __P((int *, int *, int *));
extern int tc_fb_cnattach __P((int));

void
dec_3maxplus_cons_init()
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
		zs_ioasic_lk201_cnattach(ioasic_base, 0x180000, 0);
		if (tc_fb_cnattach(crt) > 0)
			return;
#endif
		printf("No framebuffer device configured for slot %d: ", crt);
		printf("using serial console\n");
	}
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);        /* XXX */

	/*
	 * Console is channel B of the second SCC.
	 * XXX Should use ctb_line_off to get the
	 * XXX line parameters.
	 */
	if (zs_ioasic_cnattach(ioasic_base, 0x180000, 1,
	    9600, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
		panic("can't init serial console");
}

void
dec_3maxplus_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3maxplus_device_register unimplemented");
}

/*
 * Handle 3MAX+ interrupts.
 */
int
dec_3maxplus_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	struct ioasic_softc *sc = (void *)ioasic_cd.cd_devs[0];
	u_int32_t *imsk = (void *)(sc->sc_base + IOASIC_IMSK);
	u_int32_t *intr = (void *)(sc->sc_base + IOASIC_INTR);
	volatile struct chiptime *clk
		= (void *)(sc->sc_base + IOASIC_SLOT_8_START);
	volatile int temp;
	struct clockframe cf;
	static int warned = 0;
	u_long old_buscycle = latched_cycle_cnt;

	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_1) {
		temp = clk->regc;	/* XXX clear interrupt bits */
		cf.pc = pc;
		cf.sr = status;
		latched_cycle_cnt = *(u_int32_t *)(sc->sc_base + IOASIC_CTR);
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;
		old_buscycle = latched_cycle_cnt - old_buscycle;
		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrups were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_ENA_CUR | (status & MIPS_INT_MASK_1));

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
	if (cpumask & MIPS_INT_MASK_0) {
		int ifound;
		u_int32_t can_serve, xxxintr;

		do {
			ifound = 0;
			can_serve = *intr & *imsk;

#define	CHECKINTR(slot, bits)					\
	if (can_serve & (bits)) {				\
		ifound = 1;					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	}

			CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
			CHECKINTR(SYS_DEV_SCC1, IOASIC_INTR_SCC_1);
			CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
			CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);
			CHECKINTR(SYS_DEV_OPT2, KN03_INTR_TC_2);
			CHECKINTR(SYS_DEV_OPT1, KN03_INTR_TC_1);
			CHECKINTR(SYS_DEV_OPT0, KN03_INTR_TC_0);

			if (warned > 0 && !(can_serve & KN03_INTR_PSWARN)) {
				printf("%s\n", "Power supply ok now.");
				warned = 0;
			}
			if ((can_serve & KN03_INTR_PSWARN) && (warned < 3)) {
				warned++;
				printf("%s\n", "Power supply overheating");
			}

#define	ERRORS	(IOASIC_INTR_ISDN_OVRUN|IOASIC_INTR_ISDN_READ_E|IOASIC_INTR_SCSI_OVRUN|IOASIC_INTR_SCSI_READ_E|IOASIC_INTR_LANCE_READ_E)
#define	PTRLOAD	(IOASIC_INTR_ISDN_PTR_LOAD|IOASIC_INTR_SCSI_PTR_LOAD)
	/*
	 * XXX future project is here XXX 
	 * IOASIC DMA completion interrupt (PTR_LOAD) should be checked
	 * here, and DMA pointers serviced as soon as possible.
	 */
	/*
	 * All of IOASIC device interrupts comes through a single service
	 * request line coupled with MIPS cpu INT 0.
	 * Disabling INT 0 makes entire IOASIC interrupt services blocked,
	 * and it's harmful because it causes DMA overruns during network
	 * disk I/O interrupts.
	 * So, Non-DMA interrupts should be selectively disabled by masking
	 * IOASIC_IMSK register, and INT 3 itself be reenabled immediately,
	 * and made available all the time.
	 * DMA interrupts can then be serviced whilst still servicing
	 * non-DMA interrupts from ioctl devices or TC options.
	 */
			xxxintr = can_serve & (ERRORS | PTRLOAD);
			if (xxxintr) {
				ifound = 1;
				*intr &= ~xxxintr;
			/*printf("IOASIC error condition: %x\n", xxxintr);*/
			}
		} while (ifound);
	}
	if (cpumask & MIPS_INT_MASK_3)
		dec_3maxplus_memerr();

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_ENA_CUR);
}

/*
 * Handle Memory error.  3max, 3maxplus has ECC.
 * Correct single-bit error, panic on  double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3maxplus_memerr()
{
	register u_int erradr, errsyn;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR);
	errsyn = *(u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRSYN);
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn);
}

void
kn03_wbflush()
{
	__asm __volatile("lw  $2,0xbf840000");
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
