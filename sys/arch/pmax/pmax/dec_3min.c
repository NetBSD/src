/* $NetBSD: dec_3min.c,v 1.7.4.17 1999/12/06 08:52:15 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3min.c,v 1.7.4.17 1999/12/06 08:52:15 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/sysconf.h>
#include <mips/mips/mips_mcclock.h>

#include <pmax/pmax/kmin.h>
#include <pmax/pmax/memc.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>
#include <dev/ic/z8530sc.h>
#include <pmax/tc/zs_ioasicvar.h>

#include "wsdisplay.h"

void dec_3min_init __P((void));
static void dec_3min_bus_reset __P((void));
static void dec_3min_device_register __P((struct device *, void *));
static void dec_3min_cons_init __P((void));
static int  dec_3min_intr __P((unsigned, unsigned, unsigned, unsigned));
static unsigned kn02ba_clkread __P((void));

void kn02ba_wbflush __P((void));

#ifdef MIPS3
static unsigned latched_cycle_cnt;
extern u_int32_t mips3_cycle_count __P((void));
#endif

extern void prom_haltbutton __P((void));
extern void prom_findcons __P((int *, int *, int *));
extern int tcfb_cnattach __P((int));

extern int _splraise_ioasic __P((int));
extern int _spllower_ioasic __P((int));
extern int _splrestore_ioasic __P((int));
struct splsw spl_3min = {
	{ _spllower_ioasic,	0 },
	{ _splraise_ioasic,	IPL_BIO },
	{ _splraise_ioasic,	IPL_NET },
	{ _splraise_ioasic,	IPL_TTY },
	{ _splraise_ioasic,	IPL_IMP },
	{ _splraise_ioasic,	IPL_CLOCK },
	{ _splrestore_ioasic,	0 },
};


void
dec_3min_init()
{
	extern char cpu_model[];
	extern int physmem_boardmax;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3min_bus_reset;
	platform.cons_init = dec_3min_cons_init;
	platform.device_register = dec_3min_device_register;
	platform.iointr = dec_3min_intr;
	platform.clkread = kn02ba_clkread;

	/* clear any memory errors from probes */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KMIN_SYS_ASIC);
	mips_hardware_intr = dec_3min_intr;

	/*
	 * Since all the motherboard interrupts come through the
	 * IOASIC, it has to be turned off for all the spls and
	 * since we don't know what kinds of devices are in the
	 * TURBOchannel option slots, just splhigh().
	 */
#ifdef NEWSPL
	__spl = &spl_3min;
#else
	splvec.splbio = MIPS_SPL_0_1_2_3;
	splvec.splnet = MIPS_SPL_0_1_2_3;
	splvec.spltty = MIPS_SPL_0_1_2_3;
	splvec.splimp = MIPS_SPL_0_1_2_3;
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;
#endif

	/* enable posting of MIPS_INT_MASK_3 to CAUSE register */
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = KMIN_INTR_CLOCK;
	/* calribrate cpu_mhz value */
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_3);

	*(u_int32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(u_int32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(u_int32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(u_int32_t *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(u_int32_t *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif

	/* sanitize interrupt mask */
	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK)
		= KMIN_INTR_CLOCK|KMIN_INTR_PSWARN|KMIN_INTR_TIMEOUT;

	/*
	 * The kmin memory hardware seems to wrap memory addresses
	 * with 4Mbyte SIMMs, which causes the physmem computation
	 * to lose.  Find out how big the SIMMS are and set
	 * max_	physmem accordingly.
	 * XXX Do MAXINEs lose the same way?
	 */
	physmem_boardmax = KMIN_PHYS_MEMORY_END + 1;
	if ((KMIN_MSR_SIZE_16Mb & *(int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_MSR))
			== 0)
		physmem_boardmax = physmem_boardmax >> 2;
	physmem_boardmax = MIPS_PHYS_TO_KSEG1(physmem_boardmax);

	sprintf(cpu_model, "DECstation 5000/1%d (3MIN)", cpu_mhz);
}

void
dec_3min_bus_reset()
{
	/* clear any memory error condition */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ba_wbflush();
}

void
dec_3min_cons_init()
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
	DELAY(160000000 / 9600);        /* XXX */

	zs_ioasic_cnattach(ioasic_base, 0x180000, 1);
}

void
dec_3min_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3min_device_register unimplemented");
}


#define	CHECKINTR(slot, bits)					\
	if (can_serve & (bits)) {				\
		ifound = 1;					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	}

/*
 * XXX XXX
 * following is under development and never works.
 * XXX XXX
 */
int
dec_3min_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	static int warned = 0;

	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (cpumask & (MIPS_INT_MASK_3 |
			MIPS_INT_MASK_2 | MIPS_INT_MASK_1 | MIPS_INT_MASK_0)) {
		int ifound;
		u_int32_t intr, tcintr, imsk, can_serve, xxxintr;

		tcintr = 0;
		if (cpumask & MIPS_INT_MASK_0)
			tcintr |= KMIN_INTR_TC_0;
		if (cpumask & MIPS_INT_MASK_1)
			tcintr |= KMIN_INTR_TC_1;
		if (cpumask & MIPS_INT_MASK_2)
			tcintr |= KMIN_INTR_TC_2;

		do {
			ifound = 0;
			intr = *(u_int32_t *)(ioasic_base + IOASIC_INTR);
			imsk = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
			if (tcintr) {
				intr |= tcintr;	/* pretend IOASIC devices */
				tcintr = 0;
			}
			can_serve = intr & imsk;

			if (can_serve & KMIN_INTR_TIMEOUT)
				kn02ba_memerr();

			if (can_serve & KMIN_INTR_CLOCK) {
				struct clockframe cf;

				__asm __volatile("lbu $0,48(%0)" ::
					"r"(ioasic_base + IOASIC_SLOT_8_START));
				cf.pc = pc;
				cf.sr = status;
#ifdef MIPS3
				if (CPUISMIPS3)
					latched_cycle_cnt = mips3_cycle_count();
#endif
				hardclock(&cf);
				intrcnt[HARDCLOCK]++;
			}
			CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
			CHECKINTR(SYS_DEV_SCC1, IOASIC_INTR_SCC_1);
			CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
			CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);
			CHECKINTR(SYS_DEV_OPT2, KMIN_INTR_TC_2);
			CHECKINTR(SYS_DEV_OPT1, KMIN_INTR_TC_1);
			CHECKINTR(SYS_DEV_OPT0, KMIN_INTR_TC_0);

			if (warned > 0 && !(can_serve & KMIN_INTR_PSWARN)) {
				printf("%s\n", "Power supply ok now.");
				warned = 0;
			}
			if ((can_serve & KMIN_INTR_PSWARN) && (warned < 3)) {
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
				*(u_int32_t *)(ioasic_base + IOASIC_INTR)
				    &= ~xxxintr;
			/*printf("IOASIC error condition: %x\n", xxxintr);*/
			}

		} while (ifound);
	}

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

void
kn02ba_wbflush()
{
	/* read twice IOASIC_IMSK */
	__asm __volatile("lw $0,%0; lw $0,%0" ::
	    "i"(MIPS_PHYS_TO_KSEG1(KMIN_REG_IMSK)));
}

unsigned
kn02ba_clkread()
{
#ifdef MIPS3
	if (CPUISMIPS3) {
		u_int32_t mips3_cycles;

		mips3_cycles = mips3_cycle_count() - latched_cycle_cnt;
		/* avoid div insn. 78 cycles: approximate with * 41/2048 */
		return	(mips3_cycles >> 6)
			+ (mips3_cycles >> 8)
			+ (mips3_cycles >> 11);
	}
#endif
	return 0;
}

#define KV(x)	MIPS_PHYS_TO_KSEG1(x)
#define C(x)	(void *)(x)

static struct tc_slotdesc tc_kmin_slots[] = {
    { KV(KMIN_PHYS_TC_0_START), C(SYS_DEV_OPT0),  },	/* 0 - opt slot 0 */
    { KV(KMIN_PHYS_TC_1_START), C(SYS_DEV_OPT1),  },	/* 1 - opt slot 1 */
    { KV(KMIN_PHYS_TC_2_START), C(SYS_DEV_OPT2),  },	/* 2 - opt slot 2 */
    { KV(KMIN_PHYS_TC_3_START), C(SYS_DEV_BOGUS), },	/* 3 - IOASIC */
};

static struct tc_builtin tc_ioasic_builtins[] = {
	{ "IOCTL   ",	3, 0x0, C(SYS_DEV_BOGUS), },
};

struct tcbus_attach_args kmin_tc_desc = {	/* global not a const */
	NULL, 0,
	TC_SPEED_12_5_MHZ,
	KMIN_TC_NSLOTS, tc_kmin_slots,
	1, tc_ioasic_builtins,
	ioasic_intr_establish, ioasic_intr_disestablish,
	NULL,
};

/* XXX XXX XXX */
#undef	IOASIC_INTR_SCSI
#define IOASIC_INTR_SCSI 0x000e0200
/* XXX XXX XXX */

struct ioasic_dev kmin_ioasic_devs[] = {
	{ "lance",	0x0c0000, C(SYS_DEV_LANCE), IOASIC_INTR_LANCE,	},
	{ "z8530   ",	0x100000, C(SYS_DEV_SCC0),  IOASIC_INTR_SCC_0,	},
	{ "z8530   ",	0x180000, C(SYS_DEV_SCC1),  IOASIC_INTR_SCC_1,	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS), KMIN_INTR_CLOCK,	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),  IOASIC_INTR_SCSI	},
	{ "(TC0)",	0x0,	  C(SYS_DEV_OPT0),  KMIN_INTR_TC_0	},
	{ "(TC1)",	0x0,	  C(SYS_DEV_OPT1),  KMIN_INTR_TC_1	},
	{ "(TC2)",	0x0,	  C(SYS_DEV_OPT2),  KMIN_INTR_TC_2	},
};
int kmin_builtin_ndevs = 5;
int kmin_ioasic_ndevs = sizeof(kmin_ioasic_devs)/sizeof(kmin_ioasic_devs[0]);
