/* $NetBSD: dec_maxine.c,v 1.6.4.17 1999/12/06 08:52:15 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_maxine.c,v 1.6.4.17 1999/12/06 08:52:15 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>	

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/sysconf.h>
#include <mips/mips/mips_mcclock.h>

#include <pmax/pmax/maxine.h>
#include <pmax/pmax/memc.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>
#include <dev/ic/z8530sc.h>
#include <pmax/tc/zs_ioasicvar.h>

#include "wsdisplay.h"
#include "xcfb.h"

/* XXX XXX XXX */
#define	IOASIC_INTR_SCSI 0x00000200
/* XXX XXX XXX */

void dec_maxine_init __P((void));
void dec_maxine_bus_reset __P((void));
void dec_maxine_device_register __P((struct device *, void *));
void dec_maxine_cons_init __P((void));
int  dec_maxine_intr __P((unsigned, unsigned, unsigned, unsigned));
void kn02ca_wbflush __P((void));

static unsigned kn02ca_clkread __P((void));
static unsigned latched_cycle_cnt;	/* high resolution timer counter */

extern void prom_haltbutton __P((void));
extern void prom_findcons __P((int *, int *, int *));
extern int xcfb_cnattach __P((void));
extern int tcfb_cnattach __P((int));
extern void dtikbd_cnattach __P((void));

extern char cpu_model[];

extern int _splraise_ioasic __P((int));
extern int _spllower_ioasic __P((int));
extern int _splrestore_ioasic __P((int));
struct splsw spl_maxine = {
	{ _spllower_ioasic,	0 },
	{ _splraise_ioasic,	IPL_BIO },
	{ _splraise_ioasic,	IPL_NET },
	{ _splraise_ioasic,	IPL_TTY },
	{ _splraise_ioasic,	IPL_IMP },
	{ _splraise,		MIPS_SPL_0_1_3 },
	{ _splrestore_ioasic,	0 },
};


void
dec_maxine_init()
{
	platform.iobus = "tcbus";
	platform.bus_reset = dec_maxine_bus_reset;
	platform.cons_init = dec_maxine_cons_init;
	platform.device_register = dec_maxine_device_register;
	platform.iointr = dec_maxine_intr;
	platform.clkread = kn02ca_clkread;
	/* MAXINE has 1 microsec. free-running high resolution timer */

	/* clear any memory errors */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_TIMEOUT) = 0;
	kn02ca_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(XINE_SYS_ASIC);
	mips_hardware_intr = dec_maxine_intr;

	/*
	 * MAXINE IOASIC interrupts come through INT 3, while
	 * clock interrupt does via INT 1.  splclock and splstatclock
	 * should block IOASIC activities.
	 */
#ifdef NEWSPL
	__spl = &spl_maxine;
#else
	splvec.splbio = MIPS_SPL3;
	splvec.splnet = MIPS_SPL3;
	splvec.spltty = MIPS_SPL3;
	splvec.splimp = MIPS_SPL3;
	splvec.splclock = MIPS_SPL_0_1_3;
	splvec.splstatclock = MIPS_SPL_0_1_3;
#endif

	/* calibrate cpu_mhz value */
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_1);

	*(u_int32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(u_int32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(u_int32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(u_int32_t *)(ioasic_base + IOASIC_DTOP_DECODE) = 10;
	*(u_int32_t *)(ioasic_base + IOASIC_FLOPPY_DECODE) = 13;
	*(u_int32_t *)(ioasic_base + IOASIC_CSR) = 0x00001fc1;
#endif

	/* sanitize interrupt mask */
	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = 0;
	kn02ca_wbflush();

	sprintf(cpu_model, "Personal DECstation 5000/%d (MAXINE)", cpu_mhz);
}

void
dec_maxine_bus_reset()
{
	/* clear any memory error condition */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_TIMEOUT) = 0;
	kn02ca_wbflush();

	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ca_wbflush();
}

void
dec_maxine_cons_init()
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
		if (crt == 3) {
#if NXCFB > 0
			xcfb_cnattach();
			dtikbd_cnattach();
			return;
#endif
		}
		else if (tcfb_cnattach(crt) > 0) {
			dtikbd_cnattach();
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
	DELAY(160000000 / 9600);        /* XXX */

	zs_ioasic_cnattach(ioasic_base, 0x100000, 1);
}

void
dec_maxine_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_maxine_device_register unimplemented");
}


#define	CHECKINTR(slot, bits)					\
	if (can_serve & (bits)) {				\
		ifound = 1;					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
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
	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (cpumask & MIPS_INT_MASK_1) {
		struct clockframe cf;

		__asm __volatile("lbu $0,48(%0)" ::
			"r"(ioasic_base + IOASIC_SLOT_8_START));
		latched_cycle_cnt =
			*(u_int32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_FCTR);
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}
	/* allow clock interrupt posted when enabled */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_1));

	if (cpumask & MIPS_INT_MASK_3) {
		int ifound;
		u_int32_t imsk, intr, can_serve, xxxintr;

		do {
			ifound = 0;
			intr = *(u_int32_t *)(ioasic_base + IOASIC_INTR);
			imsk = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
			can_serve = intr & imsk;

			CHECKINTR(SYS_DEV_DTOP, XINE_INTR_DTOP);
			CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
			CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
			CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);
			/* CHECKINTR(SYS_DEV_OPT2, XINE_INTR_VINT);	*/
			/* CHECKINTR(SYS_DEV_ISDN, IOASIC_INTR_ISDN);	*/
			/* CHECKINTR(SYS_DEV_FDC, IOASIC_INTR_FDC);	*/
			CHECKINTR(SYS_DEV_OPT1, XINE_INTR_TC_1);
			CHECKINTR(SYS_DEV_OPT0, XINE_INTR_TC_0);

#define	ERRORS	(IOASIC_INTR_ISDN_OVRUN|IOASIC_INTR_ISDN_READ_E|IOASIC_INTR_SCSI_OVRUN|IOASIC_INTR_SCSI_READ_E|IOASIC_INTR_LANCE_READ_E)
#define	PTRLOAD	(IOASIC_INTR_ISDN_PTR_LOAD|IOASIC_INTR_SCSI_PTR_LOAD)

#if 0
	if (can_serve & IOASIC_INTR_SCSI_PTR_LOAD) {
		extern void asc_ptr_load __P((void *));
		ifound = 1;
		asc_ptr_load(intrtab[SYS_DEV_SCSI].ih_arg);
	}
#endif

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
				*(u_int32_t *)(ioasic_base + IOASIC_INTR)
					= intr &~ xxxintr;
			}
		} while (ifound);
	}
	if (cpumask & MIPS_INT_MASK_2)
		kn02ba_memerr();

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

void
kn02ca_wbflush()
{
	/* read once IOASIC_IMSK */
	__asm __volatile("lw $0,%0" ::
	    "i"(MIPS_PHYS_TO_KSEG1(XINE_REG_IMSK)));
}

unsigned
kn02ca_clkread()
{
	u_int32_t cycles;

	cycles = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_FCTR);
	return cycles - latched_cycle_cnt;
}

#define KV(x)	MIPS_PHYS_TO_KSEG1(x)
#define C(x)	(void *)(x)

static struct tc_slotdesc tc_maxine_slots[] = {
    { KV(XINE_PHYS_TC_0_START), C(SYS_DEV_OPT0),  },	/* 0 - opt slot 0 */
    { KV(XINE_PHYS_TC_1_START), C(SYS_DEV_OPT1),  },	/* 1 - opt slot 1 */
    { KV(XINE_PHYS_CFB_START),  C(SYS_DEV_BOGUS), },	/* 2 - unused */
    { KV(XINE_PHYS_TC_3_START), C(SYS_DEV_BOGUS), },	/* 3 - IOASIC */
};

static struct tc_builtin tc_ioasic_builtins[] = {
	{ "IOCTL   ",	3, 0x0, C(SYS_DEV_BOGUS), },
	{ "PMAG-DV ",	2, 0x0, C(SYS_DEV_BOGUS), },	/* slot 2 disguise */
};

struct tcbus_attach_args xine_tc_desc = {	/* global not a const */
	NULL, 0,
	TC_SPEED_12_5_MHZ,
	XINE_TC_NSLOTS, tc_maxine_slots,
	2, tc_ioasic_builtins,
	ioasic_intr_establish, ioasic_intr_disestablish,
	NULL,
};

/* XXX XXX XXX */
#undef	IOASIC_INTR_SCSI
#define IOASIC_INTR_SCSI      0x000e0200
/* XXX XXX XXX */

struct ioasic_dev xine_ioasic_devs[] = {
	{ "lance",	0x0c0000, C(SYS_DEV_LANCE), IOASIC_INTR_LANCE,	},
	{ "z8530   ",	0x100000, C(SYS_DEV_SCC0),  IOASIC_INTR_SCC_0,	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS), 0,			},
	{ "isdn",	0x240000, C(SYS_DEV_ISDN),  0 /* not yet */,	},
	{ "dtop",	0x280000, C(SYS_DEV_DTOP),  XINE_INTR_DTOP,	},
	{ "fdc",	0x2c0000, C(SYS_DEV_FDC),   0 /* not yet */,	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),  IOASIC_INTR_SCSI	},
	{ "(TC0)",	0x0,	  C(SYS_DEV_OPT0),  XINE_INTR_TC_0	},
	{ "(TC1)",	0x0,	  C(SYS_DEV_OPT1),  XINE_INTR_TC_1	},
	{ "(TC2)",	0x0,	  C(SYS_DEV_OPT2),  XINE_INTR_VINT	},
};
int xine_builtin_ndevs = 7;
int xine_ioasic_ndevs = sizeof(xine_ioasic_devs)/sizeof(xine_ioasic_devs[0]);
