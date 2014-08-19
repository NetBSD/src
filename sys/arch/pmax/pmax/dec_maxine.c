/* $NetBSD: dec_maxine.c,v 1.63.12.2 2014/08/20 00:03:18 tls Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: dec_maxine.c,v 1.63.12.2 2014/08/20 00:03:18 tls Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <pmax/sysconf.h>

#include <mips/mips/mips_mcclock.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <dev/tc/ioasicreg.h>

#include <pmax/pmax/maxine.h>
#include <pmax/pmax/machdep.h>
#include <pmax/pmax/memc.h>

#include <dev/ic/z8530sc.h>
#include <dev/tc/zs_ioasicvar.h>
#include <pmax/pmax/cons.h>
#include "wsdisplay.h"
#include "xcfb.h"

void		dec_maxine_init(void);		/* XXX */
static void	dec_maxine_bus_reset(void);
static void	dec_maxine_cons_init(void);
static void	dec_maxine_intr(uint32_t, vaddr_t, uint32_t);
static void	dec_maxine_intr_establish(device_t, void *,
		    int, int (*)(void *), void *);

static void	dec_maxine_tc_init(void);

static void	kn02ca_wbflush(void);

/*
 * local declarations
 */
static uint32_t xine_tc3_imask;

static const struct ipl_sr_map dec_maxine_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	/*
	 * MAXINE IOASIC interrupts come through INT 3, while
	 * clock interrupt does via INT 1.  splclock and splstatclock
	 * should block IOASIC activities.
	 */
	[IPL_VM] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_3,
	[IPL_SCHED] =		MIPS_INT_MASK,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

void
dec_maxine_init(void)
{

	platform.iobus = "tcbus";
	platform.bus_reset = dec_maxine_bus_reset;
	platform.cons_init = dec_maxine_cons_init;
	platform.iointr = dec_maxine_intr;
	platform.intr_establish = dec_maxine_intr_establish;
	platform.memsize = memsize_bitmap;
	platform.tc_init = dec_maxine_tc_init;
	/* MAXINE has 1 microsec. free-running high resolution timer */
 
	/* clear any memory errors */
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_TIMEOUT) = 0;
	kn02ca_wbflush();
 
	ioasic_base = MIPS_PHYS_TO_KSEG1(XINE_SYS_ASIC);

	ipl_sr_map = dec_maxine_ipl_sr_map;
 
	/* calibrate cpu_mhz value */  
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_1);

	*(volatile uint32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(volatile uint32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(volatile uint32_t *)(ioasic_base + IOASIC_DTOP_DECODE) = 10;
	*(volatile uint32_t *)(ioasic_base + IOASIC_FLOPPY_DECODE) = 13;
	*(volatile uint32_t *)(ioasic_base + IOASIC_CSR) = 0x00001fc1;
#endif
  
	/* sanitize interrupt mask */
	xine_tc3_imask = 0;
	*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = xine_tc3_imask;
	kn02ca_wbflush();

	cpu_setmodel("Personal DECstation 5000/%d (MAXINE)", mips_options.mips_cpu_mhz);
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_maxine_bus_reset(void)
{

	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_TIMEOUT) = 0;
	kn02ca_wbflush();

	*(volatile uint32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ca_wbflush();
}


static void
dec_maxine_cons_init(void)
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
#if NXCFB > 0
		if (crt == 3) {
			xcfb_cnattach((tc_addr_t)XINE_PHYS_CFB_START);
			dtkbd_cnattach();
			return;
		}
#endif
		if (tcfb_cnattach(crt) > 0) {
			dtkbd_cnattach();
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

static void
dec_maxine_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{
	uint32_t mask;

	switch ((uintptr_t)cookie) {
	case SYS_DEV_OPT0:
		mask = XINE_INTR_TC_0;
		break;
	case SYS_DEV_OPT1:
		mask = XINE_INTR_TC_1;
		break;
	case SYS_DEV_FDC:
		mask = XINE_INTR_FLOPPY;
		break;
	case SYS_DEV_SCSI:
		mask = (IOASIC_INTR_SCSI | IOASIC_INTR_SCSI_PTR_LOAD |
			IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);
		break;
	case SYS_DEV_LANCE:
		mask = IOASIC_INTR_LANCE;
		break;
	case SYS_DEV_SCC0:
		mask = IOASIC_INTR_SCC_0;
		break;
	case SYS_DEV_DTOP:
		mask = XINE_INTR_DTOP_RX;
		break;
	case SYS_DEV_ISDN:
		mask = (IOASIC_INTR_ISDN_TXLOAD | IOASIC_INTR_ISDN_RXLOAD |
			IOASIC_INTR_ISDN_OVRUN);
		break;
	default:
#ifdef DIAGNOSTIC
		printf("warning: enabling unknown intr %p\n", cookie);
#endif
		return;
	}

	xine_tc3_imask |= mask;
	intrtab[(uintptr_t)cookie].ih_func = handler;
	intrtab[(uintptr_t)cookie].ih_arg = arg;

	*(volatile uint32_t *)(ioasic_base + IOASIC_IMSK) = xine_tc3_imask;
	kn02ca_wbflush();
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
dec_maxine_ioasic_intr(void)
{
	bool ifound;
	uint32_t imsk, intr, can_serve, xxxintr;

	do {
		ifound = false;
		intr = *(uint32_t *)(ioasic_base + IOASIC_INTR);
		imsk = *(uint32_t *)(ioasic_base + IOASIC_IMSK);
		can_serve = intr & imsk;

		CHECKINTR(SYS_DEV_DTOP, XINE_INTR_DTOP);
		CHECKINTR(SYS_DEV_SCC0, IOASIC_INTR_SCC_0);
		CHECKINTR(SYS_DEV_LANCE, IOASIC_INTR_LANCE);
		CHECKINTR(SYS_DEV_SCSI, IOASIC_INTR_SCSI);
		/* CHECKINTR(SYS_DEV_OPT2, XINE_INTR_VINT);	*/
		CHECKINTR(SYS_DEV_ISDN, (IOASIC_INTR_ISDN_TXLOAD | IOASIC_INTR_ISDN_RXLOAD));
		/* CHECKINTR(SYS_DEV_FDC, IOASIC_INTR_FDC);	*/
		CHECKINTR(SYS_DEV_OPT1, XINE_INTR_TC_1);
		CHECKINTR(SYS_DEV_OPT0, XINE_INTR_TC_0);
	 
#define ERRORS	(IOASIC_INTR_ISDN_OVRUN|IOASIC_INTR_SCSI_OVRUN|IOASIC_INTR_SCSI_READ_E|IOASIC_INTR_LANCE_READ_E)
#define PTRLOAD (IOASIC_INTR_ISDN_TXLOAD|IOASIC_INTR_ISDN_RXLOAD|IOASIC_INTR_SCSI_PTR_LOAD)
 
	/* 
	 * XXX future project is here XXX
	 * IOASIC DMA completion interrupt (PTR_LOAD) should be checked
	 * here, and DMA pointers serviced as soon as possible.
	 */	
	/*
	 * All of IOASIC device interrupts comes through a single service
	 * request line coupled with MIPS CPU INT 3.
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
			ifound = true;
			*(uint32_t *)(ioasic_base + IOASIC_INTR)
				= intr &~ xxxintr;
		}
	} while (ifound);
}

static void
dec_maxine_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{
	if (ipending & MIPS_INT_MASK_4)
		prom_haltbutton();

	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_1) {
		struct clockframe cf;

		__asm volatile("lbu $0,48(%0)" ::
			"r"(ioasic_base + IOASIC_SLOT_8_START));
		cf.pc = pc;
		cf.sr = status;
		cf.intr = (curcpu()->ci_idepth > 1);
		hardclock(&cf);
		pmax_clock_evcnt.ev_count++;
	}

	if (ipending & MIPS_INT_MASK_3) {
		dec_maxine_ioasic_intr();
	}
	if (ipending & MIPS_INT_MASK_2) {
		kn02ba_errintr();
		pmax_memerr_evcnt.ev_count++;
	}
}

static void
kn02ca_wbflush(void)
{

	/* read once IOASIC_IMSK */
	__asm volatile("lw $0,%0" ::
	    "i"(MIPS_PHYS_TO_KSEG1(XINE_REG_IMSK)));
}

static u_int
dec_maxine_get_timecount(struct timecounter *tc)
{

	return *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(XINE_REG_FCTR);
}

static void
dec_maxine_tc_init(void)
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
		.tc_get_timecount = dec_maxine_get_timecount,
		.tc_quality = 100,
		.tc_frequency = 1000000,
		.tc_counter_mask = ~0,
		.tc_name = "maxine_fctr",
	};

	tc_init(&tc);

#if defined(MIPS3)
	if (MIPS_HAS_CLOCK) {
		tc3.tc_frequency = mips_options.mips_cpu_mhz * 1000000;

		tc_init(&tc3);
	}
#endif
}
