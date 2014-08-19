/* $NetBSD: dec_3maxplus.c,v 1.68.12.2 2014/08/20 00:03:18 tls Exp $ */

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

#define __INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dec_3maxplus.c,v 1.68.12.2 2014/08/20 00:03:18 tls Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <mips/locore.h>

#include <mips/mips/mips_mcclock.h>	/* mclock CPUspeed estimation */

/* all these to get ioasic_base */
#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <dev/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/sysconf.h>

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn03.h>
#include <pmax/pmax/memc.h>

#include <dev/ic/z8530sc.h>
#include <dev/tc/zs_ioasicvar.h>
#include <pmax/pmax/cons.h>
#include "wsdisplay.h"

void		dec_3maxplus_init(void);		/* XXX */
static void	dec_3maxplus_bus_reset(void);
static void	dec_3maxplus_cons_init(void);
static void 	dec_3maxplus_errintr(void);
static void	dec_3maxplus_intr(uint32_t, vaddr_t, uint32_t);
static void	dec_3maxplus_intr_establish(device_t, void *,
		    int, int (*)(void *), void *);

static void	kn03_wbflush(void);

static void	dec_3maxplus_tc_init(void);

/*
 * Local declarations
 */
static uint32_t kn03_tc3_imask;
static unsigned int latched_cycle_cnt;

static const struct ipl_sr_map dec_3maxplus_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	/*
	 * 3MAX+ IOASIC interrupts come through INT 0, while
	 * clock interrupt does via INT 1.  splclock and splstatclock
	 * should block IOASIC activities.
	 */
	[IPL_VM] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0,
	[IPL_SCHED] =		MIPS_INT_MASK,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

void
dec_3maxplus_init(void)
{
	uint32_t prodtype;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3maxplus_bus_reset;
	platform.cons_init = dec_3maxplus_cons_init;
	platform.iointr = dec_3maxplus_intr;
	platform.intr_establish = dec_3maxplus_intr_establish;
	platform.memsize = memsize_bitmap;
	/* 3MAX+ has IOASIC free-running high resolution timer */
	platform.tc_init = dec_3maxplus_tc_init;

	/* clear any memory errors */
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KN03_SYS_ASIC);

	ipl_sr_map = dec_3maxplus_ipl_sr_map;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_1);

	*(volatile uint32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(volatile uint32_t *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif

	/* XXX hard-reset LANCE */
	*(volatile uint32_t *)(ioasic_base + IOASIC_CSR) |= 0x100;

	/* sanitize interrupt mask */
	kn03_tc3_imask = KN03_INTR_PSWARN;
	*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = kn03_tc3_imask;
	kn03_wbflush();

	prodtype = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN03_REG_INTR);
	prodtype &= KN03_INTR_PROD_JUMPER;
	/* the bit persists even if INTR register is assigned value 0 */
	if (prodtype)
		cpu_setmodel("DECstation 5000/%s (3MAXPLUS)",
		    (CPUISMIPS3) ? "260" : "240");
	else
		cpu_setmodel("DECsystem 5900%s (3MAXPLUS)",
		    (CPUISMIPS3) ? "-260" : "");
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_3maxplus_bus_reset(void)
{

	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn03_wbflush();
}

static void
dec_3maxplus_cons_init(void)
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
dec_3maxplus_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{
	uint32_t mask;

	switch ((uintptr_t)cookie) {
	case SYS_DEV_OPT0:
		mask = KN03_INTR_TC_0;
		break;
	case SYS_DEV_OPT1:
		mask = KN03_INTR_TC_1;
		break;
	case SYS_DEV_OPT2:
		mask = KN03_INTR_TC_2;
		break;
	case SYS_DEV_SCSI:
		mask = (IOASIC_INTR_SCSI | IOASIC_INTR_SCSI_PTR_LOAD |
			IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);
		break;
	case SYS_DEV_LANCE:
		mask = KN03_INTR_LANCE | IOASIC_INTR_LANCE_READ_E;
		break;
	case SYS_DEV_SCC0:
		mask = KN03_INTR_SCC_0;
		break;
	case SYS_DEV_SCC1:
		mask = KN03_INTR_SCC_1;
		break;
	default:
#ifdef DIAGNOSTIC
		printf("warning: enabling unknown intr %x\n", (int)cookie);
#endif
		return;
	}

	kn03_tc3_imask |= mask;
	intrtab[(uintptr_t)cookie].ih_func = handler;
	intrtab[(uintptr_t)cookie].ih_arg = arg;

	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = kn03_tc3_imask;
	kn03_wbflush();
}

#define CHECKINTR(vvv, bits)					\
    do {							\
	if (can_serve & (bits)) {				\
		ifound = true;					\
		intrtab[vvv].ih_count.ev_count++;		\
		(*intrtab[vvv].ih_func)(intrtab[vvv].ih_arg);	\
	}							\
    } while (/*CONSTCOND*/0)

static void
dec_3maxplus_ioasic_intr(void)
{
	static bool warned = false;
	bool ifound;
	uint32_t imsk, intr, can_serve, xxxintr;

	do {
		ifound = false;
		imsk = *(uint32_t *)(ioasic_base + IOASIC_IMSK);
		intr = *(uint32_t *)(ioasic_base + IOASIC_INTR);
		can_serve = intr & imsk;

		CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
		CHECKINTR(SYS_DEV_SCC1, IOASIC_INTR_SCC_1);
		CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
		CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);
		CHECKINTR(SYS_DEV_OPT2, KN03_INTR_TC_2);
		CHECKINTR(SYS_DEV_OPT1, KN03_INTR_TC_1);
		CHECKINTR(SYS_DEV_OPT0, KN03_INTR_TC_0);

		if (warned && !(can_serve & KN03_INTR_PSWARN)) {
			printf("%s\n", "Power supply ok now.");
			warned = false;
		}
		if ((can_serve & KN03_INTR_PSWARN) && (warned < 3)) {
			warned = true;
			printf("%s\n", "Power supply overheating");
		}

#define ERRORS	(IOASIC_INTR_SCSI_OVRUN|IOASIC_INTR_SCSI_READ_E|IOASIC_INTR_LANCE_READ_E)
#define PTRLOAD	(IOASIC_INTR_SCSI_PTR_LOAD)
	/*
	 * XXX future project is here XXX
	 * IOASIC DMA completion interrupt (PTR_LOAD) should be checked
	 * here, and DMA pointers serviced as soon as possible.
	 */
	/*
	 * All of IOASIC device interrupts comes through a single service
	 * request line coupled with MIPS CPU INT 0.
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
			ifound = true;
			*(uint32_t *)(ioasic_base + IOASIC_INTR) = intr &~ xxxintr;
		}
	} while (ifound);
}

static void
dec_3maxplus_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{
	unsigned int old_buscycle;

	if (ipending & MIPS_INT_MASK_4)
		prom_haltbutton();

	/* handle clock interrupts ASAP */
	old_buscycle = latched_cycle_cnt;
	if (ipending & MIPS_INT_MASK_1) {
		struct clockframe cf;

		__asm volatile("lbu $0,48(%0)" ::
			"r"(ioasic_base + IOASIC_SLOT_8_START));
		cf.pc = pc;
		cf.sr = status;
		cf.intr = (curcpu()->ci_idepth > 1);
		hardclock(&cf);
		pmax_clock_evcnt.ev_count++;
		old_buscycle = latched_cycle_cnt - old_buscycle;
		/* keep clock interrupts enabled when we return */
	}

#ifdef notdef
	/*
	 * Check for late clock interrupts (allow 10% slop). Be careful
	 * to do so only after calling hardclock(), due to logging cost.
	 * Even then, logging dropped ticks just causes more clock
	 * ticks to be missed.
	 */
	if ((ipending & MIPS_INT_MASK_1) && old_buscycle > (tick+49) * 25) {
		/* XXX need to include <sys/msgbug.h> for msgbufmapped */
		if (msgbufmapped && 0)
			 addlog("kn03: clock intr %d usec late\n",
				 old_buscycle/25);
	}
#endif
	if (ipending & MIPS_INT_MASK_0) {
		dec_3maxplus_ioasic_intr();
	}
	if (ipending & MIPS_INT_MASK_3) {
		dec_3maxplus_errintr();
		pmax_memerr_evcnt.ev_count++;
	}
}

/*
 * Handle Memory error. 3max, 3maxplus has ECC.
 * Correct single-bit error, panic on  double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3maxplus_errintr(void)
{
	uint32_t erradr, csr;
	vaddr_t errsyn;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR);
	errsyn = MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRSYN);
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();
	csr = *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_CSR);

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn, csr & KN03_CSR_BNK32M);
}

static void
kn03_wbflush(void)
{

	/* read once IOASIC SLOT 0 */
	__asm volatile("lw $0,%0" :: "i"(0xbf840000));
}

/*
 * TURBOchannel bus-cycle counter provided by IOASIC;  25 MHz
 */

static u_int
dec_3maxplus_get_timecount(struct timecounter *tc)
{

	return *(volatile uint32_t *)(ioasic_base + IOASIC_CTR);
}

static void
dec_3maxplus_tc_init(void)
{
#if defined(MIPS3)
	static struct timecounter tc3 =  {
		.tc_get_timecount = (timecounter_get_t *)mips3_cp0_count_read,
		.tc_counter_mask = ~0u,
		.tc_name = "mips3_cp0_counter",
		.tc_quality = 200,
	};
#endif
	static struct timecounter tc = {
		.tc_get_timecount = dec_3maxplus_get_timecount,
		.tc_quality = 100,
		.tc_frequency = 25000000,
		.tc_counter_mask = ~0,
		.tc_name = "turbochannel_counter",
	};

	tc_init(&tc);

#if defined(MIPS3)
	if (MIPS_HAS_CLOCK) {
		tc3.tc_frequency = mips_options.mips_cpu_mhz * 1000000;

		tc_init(&tc3);
	}
#endif
}
