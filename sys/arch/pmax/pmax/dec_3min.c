/*	$NetBSD: dec_3min.c,v 1.7.4.8 1999/04/26 07:16:12 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3min.c,v 1.7.4.8 1999/04/26 07:16:12 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/sysconf.h>

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/kmin.h>		/* 3min baseboard addresses */
#include <pmax/pmax/dec_kn02_subr.h>	/* 3min/maxine memory errors */
#include <mips/mips/mips_mcclock.h>	/* mcclock CPU speed estimation */

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>
#include <dev/ic/z8530sc.h>
#include <pmax/tc/zs_ioasicvar.h>

#include "wsdisplay.h"

/* XXX XXX XXX */
#define IOASIC_INTR_SCSI 0x00000200
/* XXX XXX XXX */

void dec_3min_init __P((void));
void dec_3min_os_init __P((void));
void dec_3min_bus_reset __P((void));
void dec_3min_device_register __P((struct device *, void *));
void dec_3min_cons_init __P((void));
int  dec_3min_intr __P((unsigned, unsigned, unsigned, unsigned));
void kn02ba_wbflush __P((void));
unsigned kn02ba_clkread __P((void));

void kmin_intr_establish
	__P((struct device *, void *, int, int (*)(void *), void *));

void dec_3min_mcclock_cpuspeed __P((volatile struct chiptime *mcclock_addr,
			       int clockmask));

extern unsigned (*clkread) __P((void));
extern void prom_haltbutton __P((void));
extern void prom_findcons __P((int *, int *, int *));
extern int tc_fb_cnattach __P((int));

extern char cpu_model[];
extern int zs_major;
extern volatile struct chiptime *mcclock_addr;

extern int _splraise_ioasic __P((int));
extern int _spllower_ioasic __P((int));
extern int _splx_ioasic __P((int));
struct splsw spl_3min = {
	{ _spllower_ioasic,	0 },
	{ _splraise_ioasic,	IPL_BIO },
	{ _splraise_ioasic,	IPL_NET },
	{ _splraise_ioasic,	IPL_TTY },
	{ _splraise_ioasic,	IPL_IMP },
	{ _splraise_ioasic,	IPL_CLOCK },
	{ _splx_ioasic,		0 },
};

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
	kn02ba_wbflush();

	*(volatile u_int *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ba_wbflush();
}

void
dec_3min_os_init()
{
	extern int physmem_boardmax;

	/* clear any memory errors from probes */
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KMIN_SYS_ASIC);
	mcclock_addr = (void *)(ioasic_base + IOASIC_SLOT_8_START);
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
	splvec.splbio = MIPS_SPLHIGH;
	splvec.splnet = MIPS_SPLHIGH;
	splvec.spltty = MIPS_SPLHIGH;
	splvec.splimp = MIPS_SPLHIGH;
	splvec.splclock = MIPS_SPLHIGH;
	splvec.splstatclock = MIPS_SPLHIGH;
#endif
	dec_3min_mcclock_cpuspeed(mcclock_addr, MIPS_INT_MASK_3);

	*(volatile u_int *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(volatile u_int *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(volatile u_int *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(volatile u_int *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(volatile u_int *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif
	/*
	 * Initialize interrupts.
	 */
	*(volatile u_int32_t *)(ioasic_base + IOASIC_IMSK)
		= KMIN_INTR_CLOCK|KMIN_INTR_PSWARN|KMIN_INTR_TIMEOUT;
	*(volatile u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;

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

	/* R4000 3MIN can ultilize on-chip counter */
	clkread = kn02ba_clkread;
}

void
dec_3min_cons_init()
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

	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(zs_major, 0);
}

void
dec_3min_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3min_device_register unimplemented");
}

void
kmin_intr_establish(ioa, cookie, level, func, arg)
	struct device *ioa;
	void *cookie, *arg;
	tc_intrlevel_t level;
	int (*func) __P((void *));
{
	int hwint;

	ioasic_intr_establish(ioa, cookie, level, func, arg);

	hwint = 0;
	switch ((int)cookie) {
	case SYS_DEV_OPT0:
		hwint = MIPS_INT_MASK_0; break;
	case SYS_DEV_OPT1:
		hwint = MIPS_INT_MASK_1; break;
	case SYS_DEV_OPT2:
		hwint = MIPS_INT_MASK_2; break;
	}
	if (hwint == 0)
		return;

	hwint |= _splget();
	_splset(hwint);
}

/*
 * Handle 3MIN interrupts.
 */
int
dec_3min_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	struct ioasic_softc *sc = (void *)ioasic_cd.cd_devs[0];
	u_int32_t *imsk = (void *)(sc->sc_base + IOASIC_IMSK);
	u_int32_t *intr = (void *)(sc->sc_base + IOASIC_INTR);
	void *clk = (void *)(sc->sc_base + IOASIC_SLOT_8_START);
	struct clockframe cf;
	static int warned = 0;
#ifdef MIPS3
	extern u_int32_t mips3_cycle_count __P((void));
	extern unsigned latched_cycle_cnt;
#endif

	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (cpumask & MIPS_INT_MASK_3) {
		int ifound;
		u_int32_t can_serve, xxxintr;

		do {
			ifound = 0;
			can_serve = *intr & *imsk;

			if (can_serve & KMIN_INTR_TIMEOUT)
				kn02ba_memerr();
			if (can_serve & KMIN_INTR_CLOCK) {
				__asm __volatile("lbu $0,48(%0)" :: "r"(clk));
				cf.pc = pc;
				cf.sr = status;
#ifdef MIPS3
				if (CPUISMIPS3)
					latched_cycle_cnt = mips3_cycle_count();
#endif
				hardclock(&cf);
				intrcnt[HARDCLOCK]++;
			}

#define	CHECKINTR(slot, bits)					\
	if (can_serve & (bits)) {				\
		ifound = 1;					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	}
#define	CALLINTR(slot) do {					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	} while (0)
			CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
			CHECKINTR(SYS_DEV_SCC1, IOASIC_INTR_SCC_1);
			CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
			CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);

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
				*intr &= ~xxxintr;
			/*printf("IOASIC error condition: %x\n", xxxintr);*/
			}
		} while (ifound);
	}
	if (cpumask & MIPS_INT_MASK_0) {
		CALLINTR(SYS_DEV_OPT0);
	}
	if (cpumask & MIPS_INT_MASK_1) {
		CALLINTR(SYS_DEV_OPT1);
	}
	if (cpumask & MIPS_INT_MASK_2) {
		CALLINTR(SYS_DEV_OPT2);
	}

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_ENA_CUR);
}

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
	volatile u_int32_t *imsk = (void *)(ioasic_base + IOASIC_IMSK);
	u_int32_t saved_imsk;

	saved_imsk = *imsk;
	/* Allow only clock interrupts through ioasic. */
	*imsk = KMIN_INTR_CLOCK;
	kn02ba_wbflush();
    
	mc_cpuspeed(mcclock_addr, clockmask);

	*imsk = saved_imsk;
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
	extern unsigned latched_cycle_cnt;

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
