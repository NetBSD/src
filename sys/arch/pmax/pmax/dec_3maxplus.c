/* $NetBSD: dec_3maxplus.c,v 1.53.6.1 2006/04/22 11:37:52 simonb Exp $ */

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
/*
 * Copyright (c) 1988 University of Utah.
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

__KERNEL_RCSID(0, "$NetBSD: dec_3maxplus.c,v 1.53.6.1 2006/04/22 11:37:52 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mclock CPUspeed estimation */

/* all these to get ioasic_base */
#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <dev/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn03.h>
#include <pmax/pmax/memc.h>

#ifdef WSCONS
#include <dev/ic/z8530sc.h>
#include <dev/tc/zs_ioasicvar.h>
#include <pmax/pmax/cons.h>
#include "wsdisplay.h"
#else
#include <pmax/tc/sccvar.h>
#include "rasterconsole.h"
#endif

void		dec_3maxplus_init __P((void));		/* XXX */
static void	dec_3maxplus_bus_reset __P((void));
static void	dec_3maxplus_cons_init __P((void));
static void 	dec_3maxplus_errintr __P((void));
static void	dec_3maxplus_intr __P((unsigned, unsigned, unsigned, unsigned));
static void	dec_3maxplus_intr_establish __P((struct device *, void *,
		    int, int (*)(void *), void *));

static void	kn03_wbflush __P((void));
static unsigned	kn03_clkread __P((void));

/*
 * Local declarations
 */
static u_int32_t kn03_tc3_imask;
static unsigned latched_cycle_cnt;

void
dec_3maxplus_init()
{
	u_int32_t prodtype;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3maxplus_bus_reset;
	platform.cons_init = dec_3maxplus_cons_init;
	platform.iointr = dec_3maxplus_intr;
	platform.intr_establish = dec_3maxplus_intr_establish;
	platform.memsize = memsize_bitmap;
	platform.clkread = kn03_clkread;
	/* 3MAX+ has IOASIC free-running high resolution timer */

	/* clear any memory errors */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KN03_SYS_ASIC);

	/*
	 * 3MAX+ IOASIC interrupts come through INT 0, while
	 * clock interrupt does via INT 1.  splclock and splstatclock
	 * should block IOASIC activities.
	 */
	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL0;
	splvec.spltty = MIPS_SPL0;
	splvec.splvm = MIPS_SPL0;
	splvec.splclock = MIPS_SPL_0_1;
	splvec.splstatclock = MIPS_SPL_0_1;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_1);

	*(u_int32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(u_int32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(u_int32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(u_int32_t *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(u_int32_t *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif

	/* XXX hard-reset LANCE */
	*(u_int32_t *)(ioasic_base + IOASIC_CSR) |= 0x100;

	/* sanitize interrupt mask */
	kn03_tc3_imask = KN03_INTR_PSWARN;
	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = kn03_tc3_imask;
	kn03_wbflush();

	prodtype = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN03_REG_INTR);
	prodtype &= KN03_INTR_PROD_JUMPER;
	/* the bit persists even if INTR register is assigned value 0 */
	if (prodtype)
		sprintf(cpu_model, "DECstation 5000/%s (3MAXPLUS)",
		    (CPUISMIPS3) ? "260" : "240");
	else
		sprintf(cpu_model, "DECsystem 5900%s (3MAXPLUS)",
		    (CPUISMIPS3) ? "-260" : "");
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
dec_3maxplus_bus_reset()
{
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();

	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn03_wbflush();
}

static void
dec_3maxplus_cons_init()
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
#elif NRASTERCONSOLE > 0
		extern int tcfb_cnattach __P((int));		/* XXX */

		if (tcfb_cnattach(crt) > 0) {
			scc_lk201_cnattach(ioasic_base, 0x180000);
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

#ifdef WSCONS
	zs_ioasic_cnattach(ioasic_base, 0x180000, 1);
#else
	scc_cnattach(ioasic_base, 0x180000);
#endif
}

static void
dec_3maxplus_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	unsigned mask;

	switch ((int)cookie) {
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
	intrtab[(int)cookie].ih_func = handler;
	intrtab[(int)cookie].ih_arg = arg;

	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = kn03_tc3_imask;
	kn03_wbflush();
}

#define CHECKINTR(vvv, bits)					\
    do {							\
	if (can_serve & (bits)) {				\
		ifound = 1;					\
		intrtab[vvv].ih_count.ev_count++;		\
		(*intrtab[vvv].ih_func)(intrtab[vvv].ih_arg);	\
	}							\
    } while (0)

static void
dec_3maxplus_intr(status, cause, pc, ipending)
	unsigned status;
	unsigned cause;
	unsigned pc;
	unsigned ipending;
{
	static int warned = 0;
	unsigned old_buscycle;

	if (ipending & MIPS_INT_MASK_4)
		prom_haltbutton();

	/* handle clock interrupts ASAP */
	old_buscycle = latched_cycle_cnt;
	if (ipending & MIPS_INT_MASK_1) {
		struct clockframe cf;

		__asm volatile("lbu $0,48(%0)" ::
			"r"(ioasic_base + IOASIC_SLOT_8_START));
		latched_cycle_cnt = *(u_int32_t *)(ioasic_base + IOASIC_CTR);
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		pmax_clock_evcnt.ev_count++;
		old_buscycle = latched_cycle_cnt - old_buscycle;
		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_1));

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
		int ifound;
		u_int32_t imsk, intr, can_serve, xxxintr;

		do {
			ifound = 0;
			imsk = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
			intr = *(u_int32_t *)(ioasic_base + IOASIC_INTR);
			can_serve = intr & imsk;

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
				ifound = 1;
				*(u_int32_t *)(ioasic_base + IOASIC_INTR)
					= intr &~ xxxintr;
			}
		} while (ifound);
	}
	if (ipending & MIPS_INT_MASK_3) {
		dec_3maxplus_errintr();
		pmax_memerr_evcnt.ev_count++;
	}

	_splset(MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

/*
 * Handle Memory error. 3max, 3maxplus has ECC.
 * Correct single-bit error, panic on  double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3maxplus_errintr()
{
	u_int32_t erradr, errsyn, csr;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR);
	errsyn = MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRSYN);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_ERRADR) = 0;
	kn03_wbflush();
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN03_SYS_CSR);

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn, csr & KN03_CSR_BNK32M);
}

static void
kn03_wbflush()
{
	/* read once IOASIC SLOT 0 */
	__asm volatile("lw $0,%0" :: "i"(0xbf840000));
}

/*
 * TURBOchannel bus-cycle counter provided by IOASIC;
 * Interpolate micro-seconds since the last RTC clock tick.  The
 * interpolation base is the copy of the bus cycle-counter taken by
 * the RTC interrupt handler.
 */
static unsigned
kn03_clkread()
{
	u_int32_t usec, cycles;

	cycles = *(u_int32_t*)(ioasic_base + IOASIC_CTR);
	cycles = cycles - latched_cycle_cnt;

	/*
	 * Scale from 40ns to microseconds.
	 * Avoid a kernel FP divide (by 25) using the approximation
	 * 1/25 = 40/1000 =~ 41/ 1024, which is good to 0.0975 %
	 */
	usec = cycles + (cycles << 3) + (cycles << 5);
	usec = usec >> 10;

#ifdef CLOCK_DEBUG
	if (usec > 3906 +4) {
		addlog("clkread: usec %d, counter=%lx\n",
		    usec, latched_cycle_cnt);
		stacktrace();
	}
#endif /*CLOCK_DEBUG*/

	return usec;
}
