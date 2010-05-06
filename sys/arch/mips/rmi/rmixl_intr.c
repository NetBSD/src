/*	$NetBSD: rmixl_intr.c,v 1.1.2.19 2010/05/06 20:48:39 cliff Exp $	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Platform-specific interrupt support for the RMI XLP, XLR, XLS
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_intr.c,v 1.1.2.19 2010/05/06 20:48:39 cliff Exp $");

#include "opt_ddb.h"
#define	__INTR_PRIVATE

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/cpu.h>
#include <mips/locore.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_cpuvar.h>
#include <mips/rmi/rmixl_intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

// #define IOINTR_DEBUG	1
#ifdef IOINTR_DEBUG
int iointr_debug = IOINTR_DEBUG;
# define DPRINTF(x)	do { if (iointr_debug) printf x ; } while(0)
#else
# define DPRINTF(x)
#endif

#define RMIXL_PICREG_READ(off) \
	RMIXL_IOREG_READ(RMIXL_IO_DEV_PIC + (off))
#define RMIXL_PICREG_WRITE(off, val) \
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PIC + (off), (val))

/*
 * do not clear these when acking EIRR
 * (otherwise they get lost)
 */
#define RMIXL_EIRR_PRESERVE_MASK	\
		((MIPS_INT_MASK_5|MIPS_SOFT_INT_MASK) >> 8)

/*
 * IRT assignments depends on the RMI chip family
 * (XLS1xx vs. XLS2xx vs. XLS3xx vs. XLS6xx)
 * use the right irq (and display string table) for the CPU that's running.
 */

/*
 * rmixl_irtnames_xlrxxx
 * - use for XLRxxx
 */
static const char * const rmixl_irtnames_xlrxxx[NIRTS] = {
	"int 0 (watchdog)",	/*  0 */
	"int 1 (timer0)",	/*  1 */
	"int 2 (timer1)",	/*  2 */
	"int 3 (timer2)",	/*  3 */
	"int 4 (timer3)",	/*  4 */
	"int 5 (timer4)",	/*  5 */
	"int 6 (timer5)",	/*  6 */
	"int 7 (timer6)",	/*  7 */
	"int 8 (timer7)",	/*  8 */
	"int 9 (uart0)",	/*  9 */
	"int 10 (uart1)",	/* 10 */
	"int 11 (i2c0)",	/* 11 */
	"int 12 (i2c1)",	/* 12 */
	"int 13 (pcmcia)",	/* 13 */
	"int 14 (gpio)",	/* 14 */
	"int 15 (hyper)",	/* 15 */
	"int 16 (pcix)",	/* 16 */
	"int 17 (gmac0)",	/* 17 */
	"int 18 (gmac1)",	/* 18 */
	"int 19 (gmac2)",	/* 19 */
	"int 20 (gmac3)",	/* 20 */
	"int 21 (xgs0)",	/* 21 */
	"int 22 (xgs1)",	/* 22 */
	"int 23 (irq23)",	/* 23 */
	"int 24 (hyper_fatal)",	/* 24 */
	"int 25 (bridge_aerr)",	/* 25 */
	"int 26 (bridge_berr)",	/* 26 */
	"int 27 (bridge_tb)",	/* 27 */
	"int 28 (bridge_nmi)",	/* 28 */
	"int 29 (bridge_sram_derr)",	/* 29 */
	"int 30 (gpio_fatal)",	/* 30 */
	"int 31 (reserved)",	/* 31 */
};

/*
 * rmixl_irtnames_xls2xx
 * - use for XLS2xx
 */
static const char * const rmixl_irtnames_xls2xx[NIRTS] = {
	"int 0 (watchdog)",	/*  0 */
	"int 1 (timer0)",	/*  1 */
	"int 2 (timer1)",	/*  2 */
	"int 3 (timer2)",	/*  3 */
	"int 4 (timer3)",	/*  4 */
	"int 5 (timer4)",	/*  5 */
	"int 6 (timer5)",	/*  6 */
	"int 7 (timer6)",	/*  7 */
	"int 8 (timer7)",	/*  8 */
	"int 9 (uart0)",	/*  9 */
	"int 10 (uart1)",	/* 10 */
	"int 11 (i2c0)",	/* 11 */
	"int 12 (i2c1)",	/* 12 */
	"int 13 (pcmcia)",	/* 13 */
	"int 14 (gpio_a)",	/* 14 */
	"int 15 (irq15)",	/* 15 */
	"int 16 (bridge_tb)",	/* 16 */
	"int 17 (gmac0)",	/* 17 */
	"int 18 (gmac1)",	/* 18 */
	"int 19 (gmac2)",	/* 19 */
	"int 20 (gmac3)",	/* 20 */
	"int 21 (irq21)",	/* 21 */
	"int 22 (irq22)",	/* 22 */
	"int 23 (pcie_link2)",	/* 23 */
	"int 24 (pcie_link3)",	/* 24 */
	"int 25 (bridge_err)",	/* 25 */
	"int 26 (pcie_link0)",	/* 26 */
	"int 27 (pcie_link1)",	/* 27 */
	"int 28 (irq28)",	/* 28 */
	"int 29 (pcie_err)",	/* 29 */
	"int 30 (gpio_b)",	/* 30 */
	"int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xls1xx
 * - use for XLS1xx, XLS4xx-Lite
 */
static const char * const rmixl_irtnames_xls1xx[NIRTS] = {
	"int 0 (watchdog)",	/*  0 */
	"int 1 (timer0)",	/*  1 */
	"int 2 (timer1)",	/*  2 */
	"int 3 (timer2)",	/*  3 */
	"int 4 (timer3)",	/*  4 */
	"int 5 (timer4)",	/*  5 */
	"int 6 (timer5)",	/*  6 */
	"int 7 (timer6)",	/*  7 */
	"int 8 (timer7)",	/*  8 */
	"int 9 (uart0)",	/*  9 */
	"int 10 (uart1)",	/* 10 */
	"int 11 (i2c0)",	/* 11 */
	"int 12 (i2c1)",	/* 12 */
	"int 13 (pcmcia)",	/* 13 */
	"int 14 (gpio_a)",	/* 14 */
	"int 15 (irq15)",	/* 15 */
	"int 16 (bridge_tb)",	/* 16 */
	"int 17 (gmac0)",	/* 17 */
	"int 18 (gmac1)",	/* 18 */
	"int 19 (gmac2)",	/* 19 */
	"int 20 (gmac3)",	/* 20 */
	"int 21 (irq21)",	/* 21 */
	"int 22 (irq22)",	/* 22 */
	"int 23 (irq23)",	/* 23 */
	"int 24 (irq24)",	/* 24 */
	"int 25 (bridge_err)",	/* 25 */
	"int 26 (pcie_link0)",	/* 26 */
	"int 27 (pcie_link1)",	/* 27 */
	"int 28 (irq28)",	/* 28 */
	"int 29 (pcie_err)",	/* 29 */
	"int 30 (gpio_b)",	/* 30 */
	"int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xls4xx:
 * - use for XLS4xx, XLS6xx
 */
static const char * const rmixl_irtnames_xls4xx[NIRTS] = {
	"int 0 (watchdog)",	/*  0 */
	"int 1 (timer0)",	/*  1 */
	"int 2 (timer1)",	/*  2 */
	"int 3 (timer2)",	/*  3 */
	"int 4 (timer3)",	/*  4 */
	"int 5 (timer4)",	/*  5 */
	"int 6 (timer5)",	/*  6 */
	"int 7 (timer6)",	/*  7 */
	"int 8 (timer7)",	/*  8 */
	"int 9 (uart0)",	/*  9 */
	"int 10 (uart1)",	/* 10 */
	"int 11 (i2c0)",	/* 11 */
	"int 12 (i2c1)",	/* 12 */
	"int 13 (pcmcia)",	/* 13 */
	"int 14 (gpio_a)",	/* 14 */
	"int 15 (irq15)",	/* 15 */
	"int 16 (bridge_tb)",	/* 16 */
	"int 17 (gmac0)",	/* 17 */
	"int 18 (gmac1)",	/* 18 */
	"int 19 (gmac2)",	/* 19 */
	"int 20 (gmac3)",	/* 20 */
	"int 21 (irq21)",	/* 21 */
	"int 22 (irq22)",	/* 22 */
	"int 23 (irq23)",	/* 23 */
	"int 24 (irq24)",	/* 24 */
	"int 25 (bridge_err)",	/* 25 */
	"int 26 (pcie_link0)",	/* 26 */
	"int 27 (pcie_link1)",	/* 27 */
	"int 28 (pcie_link2)",	/* 28 */
	"int 29 (pcie_link3)",	/* 29 */
	"int 30 (gpio_b)",	/* 30 */
	"int 31 (usb)",		/* 31 */
};

/*
 * rmixl_vecnames_common:
 * - use for unknown cpu implementation
 * - covers all vectors, not just IRT intrs
 */
static const char * const rmixl_vecnames_common[NINTRVECS] = {
	"int 0",	/*  0 */
	"int 1",	/*  1 */
	"int 2",	/*  2 */
	"int 3",	/*  3 */
	"int 4",	/*  4 */
	"int 5",	/*  5 */
	"int 6",	/*  6 */
	"int 7",	/*  7 */
	"int 8",	/*  8 */
	"int 9",	/*  9 */
	"int 10",	/* 10 */
	"int 11",	/* 11 */
	"int 12",	/* 12 */
	"int 13",	/* 13 */
	"int 14",	/* 14 */
	"int 15",	/* 15 */
	"int 16",	/* 16 */
	"int 17",	/* 17 */
	"int 18",	/* 18 */
	"int 19",	/* 19 */
	"int 20",	/* 20 */
	"int 21",	/* 21 */
	"int 22",	/* 22 */
	"int 23",	/* 23 */
	"int 24",	/* 24 */
	"int 25",	/* 25 */
	"int 26",	/* 26 */
	"int 27",	/* 27 */
	"int 28",	/* 28 */
	"int 29",	/* 29 */
	"int 30",	/* 30 */
	"int 31",	/* 31 */
	"int 32 (ipi)",	/* 32 */
	"int 33 (fmn)",	/* 33 */
	"int 34",	/* 34 */
	"int 35",	/* 35 */
	"int 36",	/* 36 */
	"int 37",	/* 37 */
	"int 38",	/* 38 */
	"int 39",	/* 39 */
	"int 40",	/* 40 */
	"int 41",	/* 41 */
	"int 42",	/* 42 */
	"int 43",	/* 43 */
	"int 44",	/* 44 */
	"int 45",	/* 45 */
	"int 46",	/* 46 */
	"int 47",	/* 47 */
	"int 48",	/* 48 */
	"int 49",	/* 49 */
	"int 50",	/* 50 */
	"int 51",	/* 51 */
	"int 52",	/* 52 */
	"int 53",	/* 53 */
	"int 54",	/* 54 */
	"int 55",	/* 55 */
	"int 56",	/* 56 */
	"int 57",	/* 57 */
	"int 58",	/* 58 */
	"int 59",	/* 59 */
	"int 60",	/* 60 */
	"int 61",	/* 61 */
	"int 62",	/* 63 */
	"int 63",	/* 63 */
};

/*
 * mask of CPUs attached
 * once they are attached, this var is read-only so mp safe
 */
static uint32_t cpu_present_mask;

rmixl_intrhand_t rmixl_intrhand[NINTRVECS];

#ifdef DIAGNOSTIC
static int rmixl_pic_init_done;
#endif


static const char *rmixl_intr_string_xlr(int);
static const char *rmixl_intr_string_xls(int);
static uint32_t rmixl_irt_thread_mask(int);
static void rmixl_irt_init(int);
static void rmixl_irt_disestablish(int);
static void rmixl_irt_establish(int, int,
		rmixl_intr_trigger_t, rmixl_intr_polarity_t);

#ifdef MULTIPROCESSOR
static int rmixl_send_ipi(struct cpu_info *, int);
static int rmixl_ipi_intr(void *);
#endif

#if defined(IOINTR_DEBUG) || defined(DIAGNOSTIC)
int rmixl_intrhand_print_subr(int);
int rmixl_intrhand_print(void);
int rmixl_irt_print(void);
#endif


static inline u_int
dclz(uint64_t val)
{
	int nlz;

	asm volatile("dclz %0, %1;"
		: "=r"(nlz) : "r"(val));
	
	return nlz;
}

static inline void
rmixl_irt_entry_print(u_int irq)
{
#if defined(IOINTR_DEBUG) || defined(DDB)
	uint32_t c0, c1;

	if ((irq < 0) || (irq > NIRTS))
		return;
	c0 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irq));
	c1 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irq));
	printf("irt[%d]: %#x, %#x\n", irq, c0, c1);
#endif
}

void
evbmips_intr_init(void)
{
	uint32_t r;

	KASSERT(cpu_rmixlr(mips_options.mips_cpu)
	     || cpu_rmixls(mips_options.mips_cpu));

#ifdef IOINTR_DEBUG
	printf("IPL_NONE=%d, mask %#"PRIx64"\n",
		IPL_NONE, ipl_eimr_map[IPL_NONE]);
	printf("IPL_SOFTCLOCK=%d, mask %#"PRIx64"\n",
		IPL_SOFTCLOCK, ipl_eimr_map[IPL_SOFTCLOCK]);
	printf("IPL_SOFTNET=%d, mask %#"PRIx64"\n",
		IPL_SOFTNET, ipl_eimr_map[IPL_SOFTNET]);
	printf("IPL_VM=%d, mask %#"PRIx64"\n",
		IPL_VM, ipl_eimr_map[IPL_VM]);
	printf("IPL_SCHED=%d, mask %#"PRIx64"\n",
		IPL_SCHED, ipl_eimr_map[IPL_HIGH]);
	printf("IPL_HIGH=%d, mask %#"PRIx64"\n",
		IPL_HIGH, ipl_eimr_map[IPL_NONE]);
#endif

#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done != 0)
		panic("%s: rmixl_pic_init_done %d",
			__func__, rmixl_pic_init_done);
#endif

	/*
	 * initialize (zero) all IRT Entries in the PIC
	 */
	for (int i=0; i < NIRTS; i++)
		rmixl_irt_init(i);

	/*
	 * disable watchdog NMI, timers
	 *
	 * XXX
	 *  WATCHDOG_ENB is preserved because clearing it causes
	 *  hang on the XLS616 (but not on the XLS408)
	 */
	r = RMIXL_PICREG_READ(RMIXL_PIC_CONTROL);
	r &= RMIXL_PIC_CONTROL_RESV|RMIXL_PIC_CONTROL_WATCHDOG_ENB;
	RMIXL_PICREG_WRITE(RMIXL_PIC_CONTROL, r);

#ifdef DIAGNOSTIC
	rmixl_pic_init_done = 1;
#endif

}

/*
 * establish vector for mips3 count/compare clock interrupt
 * this ensures we enable in EIRR,
 * even though cpu_intr() handles the interrupt
 * note the 'mpsafe' arg here is a placeholder only
 */
void *
rmixl_intr_init_clk(void)
{
	int vec = ffs(MIPS_INT_MASK_5 >> 8) - 1;
	void *ih = rmixl_vec_establish(vec, 0, IPL_SCHED, NULL, NULL, false);
	if (ih == NULL)
		panic("%s: establish vec %d failed", __func__, vec);
	
	return ih;
}

#ifdef MULTIPROCESSOR
/*
 * establish IPI interrupt and send function
 */
void *
rmixl_intr_init_ipi(void)
{
	void *ih = rmixl_vec_establish(RMIXL_INTRVEC_IPI, -1, IPL_SCHED,
		rmixl_ipi_intr, NULL, false);
	if (ih == NULL)
		panic("%s: establish vec %d failed",
			__func__, RMIXL_INTRVEC_IPI);

	mips_locoresw.lsw_send_ipi = rmixl_send_ipi;

	return ih;
}
#endif 	/* MULTIPROCESSOR */

/*
 * initialize per-cpu interrupt stuff in softc
 * accumulate per-cpu bits in 'cpu_present_mask'
 */
void
rmixl_intr_init_cpu(struct cpu_info *ci)
{
	struct rmixl_cpu_softc *sc = (void *)ci->ci_softc;
	KASSERT(sc != NULL);

	/* zero the EIRR ? */
	uint64_t eirr = 0;
	asm volatile("dmtc0 %0, $9, 6;" :: "r"(eirr));

	for (int vec=0; vec < NINTRVECS; vec++)
		evcnt_attach_dynamic(&sc->sc_vec_evcnts[vec],
			EVCNT_TYPE_INTR, NULL,
			device_xname(sc->sc_dev),
			rmixl_intr_string(vec));

	KASSERT(ci->ci_cpuid < (sizeof(cpu_present_mask) * 8));
	cpu_present_mask |= 1 << ci->ci_cpuid;
}

/*
 * rmixl_intr_string - return pointer to display name of a PIC-based interrupt
 */
const char *
rmixl_intr_string(int irq)
{
	if (irq < 0 || irq >= NINTRVECS)
		panic("%s: irq index %d out of range, max %d",
			__func__, irq, NIRTS - 1);

	if (irq >= NIRTS)
		return rmixl_vecnames_common[irq];

	switch(cpu_rmixl_chip_type(mips_options.mips_cpu)) {
	case CIDFL_RMI_TYPE_XLR:
		return rmixl_intr_string_xlr(irq);
	case CIDFL_RMI_TYPE_XLS:
		return rmixl_intr_string_xls(irq);
	case CIDFL_RMI_TYPE_XLP:
		panic("%s: RMI XLP not yet supported", __func__);
	}

	return "undefined";	/* appease gcc */
}

static const char *
rmixl_intr_string_xlr(int irq)
{
	return rmixl_irtnames_xlrxxx[irq];
}

static const char *
rmixl_intr_string_xls(int irq)
{
	const char *name;

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		name = rmixl_irtnames_xls1xx[irq];
		break;
	case MIPS_XLS204:
	case MIPS_XLS208:
		name = rmixl_irtnames_xls2xx[irq];
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608:
	case MIPS_XLS616:
		name = rmixl_irtnames_xls4xx[irq];
		break;
	default:
		name = rmixl_vecnames_common[irq];
		break;
	}

	return name;
}

/*
 * rmixl_irt_thread_mask
 *
 *	given a bitmask of cpus, return a, IRT thread mask
 */
static uint32_t
rmixl_irt_thread_mask(int cpumask)
{
	uint32_t irtc0;

#if defined(MULTIPROCESSOR)
#ifndef NOTYET
	if (cpumask == -1)
		return 1;	/* XXX TMP FIXME */
#endif

	/*
	 * discount cpus not present
	 */
	cpumask &= cpu_present_mask;
	
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS204:
	case MIPS_XLS404:
	case MIPS_XLS404LITE:
		irtc0 = ((cpumask >> 2) << 4) | (cpumask & __BITS(1,0));
		irtc0 &= (__BITS(5,4) | __BITS(1,0));
		break;
	case MIPS_XLS108:
	case MIPS_XLS208:
	case MIPS_XLS408:
	case MIPS_XLS408LITE:
	case MIPS_XLS608:
		irtc0 = cpumask & __BITS(7,0);
		break;
	case MIPS_XLS416:
	case MIPS_XLS616:
		irtc0 = cpumask & __BITS(15,0);
		break;
	default:
		panic("%s: unknown cpu ID %#x\n", __func__,
			mips_options.mips_cpu_id);
	}
#else
	irtc0 = 1;
#endif	/* MULTIPROCESSOR */

	return irtc0;
}

/*
 * rmixl_irt_init
 * - invalidate IRT Entry for irq
 * - unmask Thread#0 in low word (assume we only have 1 thread)
 */
static void
rmixl_irt_init(int irq)
{
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), 0);	/* high word */
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irq), 0);	/* low  word */
}

/*
 * rmixl_irt_disestablish
 * - invalidate IRT Entry for irq
 * - writes to IRTENTRYC1 only; leave IRTENTRYC0 as-is
 */
static void
rmixl_irt_disestablish(int irq)
{
	DPRINTF(("%s: irq %d, irtc1 %#x\n", __func__, irq, 0));
	rmixl_irt_init(irq);
}

/*
 * rmixl_irt_establish
 * - construct an IRT Entry for irq and write to PIC
 */
static void
rmixl_irt_establish(int irq, int cpumask, rmixl_intr_trigger_t trigger,
	rmixl_intr_polarity_t polarity)
{
	uint32_t irtc1;
	uint32_t irtc0;

	switch (trigger) {
	case RMIXL_TRIG_EDGE:
	case RMIXL_TRIG_LEVEL:
		break;
	default:
		panic("%s: bad trigger %d\n", __func__, trigger);
	}

	switch (polarity) {
	case RMIXL_POLR_RISING:
	case RMIXL_POLR_HIGH:
	case RMIXL_POLR_FALLING:
	case RMIXL_POLR_LOW:
		break;
	default:
		panic("%s: bad polarity %d\n", __func__, polarity);
	}

	/*
	 * XXX IRT entries are not shared
	 */
	KASSERT(RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irq)) == 0);
	KASSERT(RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irq)) == 0);

	irtc0 = rmixl_irt_thread_mask(cpumask);

	irtc1  = RMIXL_PIC_IRTENTRYC1_VALID;
	irtc1 |= RMIXL_PIC_IRTENTRYC1_GL;	/* local */

	if (trigger == RMIXL_TRIG_LEVEL)
		irtc1 |= RMIXL_PIC_IRTENTRYC1_TRG;

	if ((polarity == RMIXL_POLR_FALLING) || (polarity == RMIXL_POLR_LOW))
		irtc1 |= RMIXL_PIC_IRTENTRYC1_P;

	irtc1 |= irq;	/* route to vector 'irq' */

	/*
	 * write IRT Entry to PIC
	 */
	DPRINTF(("%s: irq %d, irtc0 %#x, irtc1 %#x\n",
		__func__, irq, irtc0, irtc1));
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irq), irtc0);	/* low  word */
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), irtc1);	/* high word */
}

void *
rmixl_vec_establish(int vec, int cpumask, int ipl,
	int (*func)(void *), void *arg, bool mpsafe)
{
	rmixl_intrhand_t *ih;
	int s;

	DPRINTF(("%s: vec %d, cpumask %#x, ipl %d, func %p, arg %p, "
		"vec %d\n",
			__func__, vec, cpumask, ipl, func, arg, vec));
#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done == 0)
		panic("%s: called before evbmips_intr_init", __func__);
#endif

	/*
	 * check args
	 */
	if (vec < 0 || vec >= NINTRVECS)
		panic("%s: vec %d out of range, max %d",
			__func__, vec, NINTRVECS - 1);
	if (ipl <= 0 || ipl >= _IPL_N)
		panic("%s: ipl %d out of range, min %d, max %d",
			__func__, ipl, 1, _IPL_N - 1);

	s = splhigh();

	ih = &rmixl_intrhand[vec];

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_mpsafe = mpsafe;
	ih->ih_irq = vec;
	ih->ih_ipl = ipl;
	ih->ih_cpumask = cpumask;

	splx(s);

	return ih;
}

void *
rmixl_intr_establish(int irq, int cpumask, int ipl,
	rmixl_intr_trigger_t trigger, rmixl_intr_polarity_t polarity,
	int (*func)(void *), void *arg, bool mpsafe)
{
	rmixl_intrhand_t *ih;
	int s;

#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done == 0)
		panic("%s: called before rmixl_pic_init_done", __func__);
#endif

	/*
	 * check args
	 */
	if (irq < 0 || irq >= NINTRVECS)
		panic("%s: irq %d out of range, max %d",
			__func__, irq, NIRTS - 1);
	if (ipl <= 0 || ipl >= _IPL_N)
		panic("%s: ipl %d out of range, min %d, max %d",
			__func__, ipl, 1, _IPL_N - 1);

	DPRINTF(("%s: irq %d, ipl %d\n", __func__, irq, ipl));

	s = splhigh();

	/*
	 * establish vector
	 */
	ih = rmixl_vec_establish(irq, cpumask, ipl, func, arg, mpsafe);

	/*
	 * establish IRT Entry
	 */
	if (irq < 32)
		rmixl_irt_establish(irq, cpumask, trigger, polarity);

	splx(s);

	return ih;
}

void
rmixl_vec_disestablish(void *cookie)
{
	rmixl_intrhand_t *ih = cookie;
	int s;

	KASSERT(ih = &rmixl_intrhand[ih->ih_irq]);

	s = splhigh();

	ih->ih_func = NULL;	/* XXX race */

	splx(s);
}

void
rmixl_intr_disestablish(void *cookie)
{
	rmixl_intrhand_t *ih = cookie;
	int vec;
	int s;

	vec = ih->ih_irq;

	KASSERT(ih = &rmixl_intrhand[vec]);

	s = splhigh();

	/*
	 * disable/invalidate the IRT Entry if needed
	 */
	if (vec < 32)
		rmixl_irt_disestablish(vec);

	/*
	 * disasociate from vector and free the handle
	 */
	rmixl_vec_disestablish(cookie);

	splx(s);
}

void
evbmips_iointr(int ipl, vaddr_t pc, uint32_t pending)
{
	struct rmixl_cpu_softc *sc = (void *)curcpu()->ci_softc;

	DPRINTF(("%s: cpu%ld: ipl %d, pc %#"PRIxVADDR", pending %#x\n",
		__func__, cpu_number(), ipl, pc, pending));

	/*
	 * 'pending' arg is a summary that there is something to do
	 * the real pending status is obtained from EIRR
	 */
	KASSERT(pending == MIPS_INT_MASK_1);

	for (;;) {
		rmixl_intrhand_t *ih;
		uint64_t eirr;
		uint64_t eimr;
		uint64_t vecbit;
		int vec;

		asm volatile("dmfc0 %0, $9, 6;" : "=r"(eirr));

#ifdef IOINTR_DEBUG
		asm volatile("dmfc0 %0, $9, 7;" : "=r"(eimr));
		printf("%s: eirr %#"PRIx64", eimr %#"PRIx64", mask %#"PRIx64"\n",
			__func__, eirr, eimr, ipl_eimr_map[ipl-1]);
#endif	/* IOINTR_DEBUG */

		eirr &= ipl_eimr_map[ipl-1];
		eirr &= ~(MIPS_SOFT_INT_MASK >> 8);	/* mask off soft ints */
		if (eirr == 0)
			break;

		vec = 63 - dclz(eirr);
		ih = &rmixl_intrhand[vec];

		asm volatile("dmfc0 %0, $9, 7;" : "=r"(eimr));
		asm volatile("dmtc0 $0, $9, 7;");
		vecbit = 1ULL << vec;
		KASSERT ((vecbit & eimr) == 0);
		KASSERT ((vecbit & RMIXL_EIRR_PRESERVE_MASK) == 0);
		asm volatile("dmfc0 %0, $9, 6;" : "=r"(eirr));
		eirr &= RMIXL_EIRR_PRESERVE_MASK;
		eirr |= vecbit;
		asm volatile("dmtc0 %0, $9, 6;" :: "r"(eirr));
		asm volatile("dmtc0 %0, $9, 7;" :: "r"(eimr));

		if (vec < 32)
			RMIXL_PICREG_WRITE(RMIXL_PIC_INTRACK,
				(uint32_t)vecbit);

		if (ih->ih_func != NULL) {
#ifdef MULTIPROCESSOR
			if (ih->ih_mpsafe) {
				(void)(*ih->ih_func)(ih->ih_arg);
			} else {
				KERNEL_LOCK(1, NULL);
				(void)(*ih->ih_func)(ih->ih_arg);
				KERNEL_UNLOCK_ONE(NULL);
			}
#else
			(void)(*ih->ih_func)(ih->ih_arg);
#endif /* MULTIPROCESSOR */
		}

		sc->sc_vec_evcnts[vec].ev_count++;
	}
}

#ifdef MULTIPROCESSOR
static int
rmixl_send_ipi(struct cpu_info *ci, int tag)
{
	const cpuid_t cpu = ci->ci_cpuid;
	uint32_t core = (uint32_t)(cpu >> 2);
	uint32_t thread = (uint32_t)(cpu & __BITS(1,0));
	uint64_t req = 1 << tag;
	uint32_t r;
	extern volatile u_long cpus_running;

	if ((cpus_running & 1 << ci->ci_cpuid) == 0)
		return -1;

	KASSERT(tag < NIPIS);

	r = (thread << RMIXL_PIC_IPIBASE_ID_THREAD_SHIFT)
	  | (core << RMIXL_PIC_IPIBASE_ID_CORE_SHIFT)
	  | RMIXL_INTRVEC_IPI;

	atomic_or_64(&ci->ci_request_ipis, req);

	RMIXL_PICREG_WRITE(RMIXL_PIC_IPIBASE, r);

	return 0;
}

static int
rmixl_ipi_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	uint64_t ipi_mask;

	ipi_mask = atomic_swap_64(&ci->ci_request_ipis, 0);
	if (ipi_mask == 0)
		return 0;

	ipi_process(ci, ipi_mask);

	return 1;
}
#endif	/* MULTIPROCESSOR */

#if defined(DIAGNOSTIC) || defined(IOINTR_DEBUG)
int
rmixl_intrhand_print_subr(int vec)
{
	rmixl_intrhand_t *ih = &rmixl_intrhand[vec];
	printf("vec %d: func %p, arg %p, irq %d, ipl %d, mask %#x\n",
		vec, ih->ih_func, ih->ih_arg, ih->ih_irq, ih->ih_ipl,
		ih->ih_cpumask);
	return 0;
}
int
rmixl_intrhand_print(void)
{
	for (int vec=0; vec < NINTRVECS ; vec++)
		rmixl_intrhand_print_subr(vec);
	return 0;
}
int
rmixl_irt_print(void)
{
	printf("%s:\n", __func__);
	for (int irt=0; irt < NIRTS ; irt++)
		rmixl_irt_entry_print(irt);
	return 0;
}
#endif
