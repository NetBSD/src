/* $Id: dec_maxine.c,v 1.6.4.1 1998/10/15 00:42:44 nisimura Exp $ */
/*	$NetBSD: dec_maxine.c,v 1.6.4.1 1998/10/15 00:42:44 nisimura Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: dec_maxine.c,v 1.6.4.1 1998/10/15 00:42:44 nisimura Exp $");

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
#include <pmax/pmax/maxine.h>		/* baseboard addresses (constants) */
#include <pmax/pmax/dec_kn02_subr.h>	/* 3min/maxine memory errors */

#include "wsdisplay.h"

/* XXX XXX XXX */
#define	IOASIC_INTR_SCSI 0x00000200
/* XXX XXX XXX */

void dec_maxine_init __P((void));
void dec_maxine_os_init __P((void));
void dec_maxine_bus_reset __P((void));
void dec_maxine_device_register __P((struct device *, void *));
void dec_maxine_cons_init __P((void));
int  dec_maxine_intr __P((unsigned, unsigned, unsigned, unsigned));
void kn02ca_wbflush __P((void));

extern void prom_haltbutton __P((void));
extern volatile struct chiptime *mcclock_addr; /* XXX */
extern int _splraise_ioasic __P((int));
extern int _spllower_ioasic __P((int));
extern int _splx_ioasic __P((int));
extern void kn02ca_errintr __P((void));
extern u_long latched_cycle_cnt;
extern char cpu_model[];

struct splsw spl_maxine = {
	{ _spllower_ioasic,	0 },
	{ _splraise_ioasic,	IPL_BIO },
	{ _splraise_ioasic,	IPL_NET },
	{ _splraise_ioasic,	IPL_TTY },
	{ _splraise_ioasic,	IPL_IMP },
	{ _splraise,		MIPS_SPL_0_1_3 },
	{ _splx_ioasic,		0 },
};

/*
 * Fill in platform struct. 
 */
void
dec_maxine_init()
{
	platform.iobus = "tcbus";

	platform.os_init = dec_maxine_os_init;
	platform.bus_reset = dec_maxine_bus_reset;
	platform.cons_init = dec_maxine_cons_init;
	platform.device_register = dec_maxine_device_register;

	dec_maxine_os_init();

	sprintf(cpu_model, "Personal DECstation 5000/%d (MAXINE)", cpu_mhz);
}

/*
 * Set up OS level stuff: spls, etc.
 */
void
dec_maxine_os_init()
{
	/* clear any memory errors from probes */
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(XINE_REG_TIMEOUT) = 0;
	kn02ca_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(XINE_SYS_ASIC);
	mcclock_addr = (void *)(ioasic_base + IOASIC_SLOT_8_START);
	mips_hardware_intr = dec_maxine_intr;

	/*
	 * MAXINE IOASIC interrupts come through INT 3, while
	 * clock interrupt does via INT 1.  splclock and splstatclock
	 * should block IOASIC activities.
	 */
	splvec.splbio = MIPS_SPL3;
	splvec.splnet = MIPS_SPL3;
	splvec.spltty = MIPS_SPL3;
	splvec.splimp = MIPS_SPL3;
	splvec.splclock = MIPS_SPL_0_1_3;
	splvec.splstatclock = MIPS_SPL_0_1_3;

	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_1);

	/*
	 * Initialize interrupts.
	 */
	*(volatile u_int *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ca_wbflush();
}


/*
 * Initalize the memory system and I/O buses.
 */
void
dec_maxine_bus_reset()
{
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(XINE_REG_TIMEOUT) = 0;
	kn02ca_wbflush();

	*(volatile u_int *)IOASIC_REG_INTR(ioasic_base) = 0;
	kn02ca_wbflush();
}

#include <dev/cons.h>
#include <sys/termios.h>

extern void prom_findcons __P((int *, int *, int *));
extern int tc_fb_cnattach __P((int));
extern void xcfb_cnattach __P((tc_addr_t));
extern int zs_ioasic_cnattach __P((tc_addr_t, tc_offset_t, int, int, int));
extern int zs_major;

void
dec_maxine_cons_init()
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

#if NWSDISPLAY > 0
	if (screen > 0) {
		if (crt == 3) {
			xcfb_cnattach(0x08000000 + MIPS_KSEG1_START);
			return;
		}
		if (tc_fb_cnattach(crt) > 0)
			return;
		printf("No framebuffer device configured for slot %d\n", crt);
		printf("Using serial console\n");
	}
#endif
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
	if (zs_ioasic_cnattach(ioasic_base, 0x100000, 1,
	    9600, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
		panic("can't init serial console");

	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(zs_major, 0);
}

void
dec_maxine_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_maxine_device_register unimplemented");
}

/*
 * Handle MAXINE interrupts.
 */
int
dec_maxine_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	struct ioasic_softc *sc = (void *)ioasic_cd.cd_devs[0];
	u_int32_t *imsk = (u_int32_t *)sc->sc_ioasic_imsk;
	u_int32_t *intr = (u_int32_t *)sc->sc_ioasic_intr;
	volatile struct chiptime *clk = (void *)sc->sc_ioasic_rtc;
	volatile int temp;
	struct clockframe cf;

	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_1) {
		temp = clk->regc;	/* XXX clear interrupt bits */
		cf.pc = pc;
		cf.sr = status;
		latched_cycle_cnt =
		    *(u_int32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_FCTR);
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;
		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrups were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_ENA_CUR | (status & MIPS_INT_MASK_1));

	if (cpumask & MIPS_INT_MASK_3) {
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

			/* CHECKINTR(SYS_DEV_FDC, IOASIC_INTR_FDC);	*/
			CHECKINTR(SYS_DEV_OPT0, XINE_INTR_TC_0);
			/* CHECKINTR(SYS_DEV_ISDN, IOASIC_INTR_ISDN);	*/
			CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);
			CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
			CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
			/* CHECKINTR(SYS_DEV_OPT2, XINE_INTR_VINT);	*/
			CHECKINTR(SYS_DEV_OPT1, XINE_INTR_TC_1);
			CHECKINTR(SYS_DEV_DTOP, XINE_INTR_DTOP);

#define	ERRORS	(IOASIC_INTR_ISDN_OVRUN|IOASIC_INTR_ISDN_READ_E|IOASIC_INTR_SCSI_OVRUN|IOASIC_INTR_SCSI_READ_E|IOASIC_INTR_LANCE_READ_E)
#define	PTRLOAD	(IOASIC_INTR_ISDN_PTR_LOAD|IOASIC_INTR_SCSI_PTR_LOAD)
	/*
	 * XXX future project is here XXX
	 * IOASIC DMA completion interrupt (PTR_LOAD) should be checked
	 * here, and DMA pointers serviced as soon as possible.
	 */
	/*
	 * All of IOASIC device interrupts comes through a single service
	 * request line coupled with MIPS cpu INT 3.
	 * Disabling INT 3 makes entire IOASIC interrupt services blocked,
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
	if (cpumask & MIPS_INT_MASK_2)
		kn02ba_errintr();

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_ENA_CUR);
}

void
kn02ca_wbflush()
{
	__asm __volatile("lw  $2,0xbc040120");
}
