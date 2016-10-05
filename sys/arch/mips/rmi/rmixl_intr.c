/*	$NetBSD: rmixl_intr.c,v 1.8.30.2 2016/10/05 20:55:32 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_intr.c,v 1.8.30.2 2016/10/05 20:55:32 skrll Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#define	__INTR_PRIVATE

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_cpuvar.h>
#include <mips/rmi/rmixl_intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

//#define IOINTR_DEBUG	1
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
 * use the right display string table for the CPU that's running.
 */

/*
 * rmixl_irtnames_xlrxxx
 * - use for XLRxxx
 */
static const char * const rmixl_irtnames_xlrxxx[NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio)",		/* 14 */
	"pic int 15 (hyper)",		/* 15 */
	"pic int 16 (pcix)",		/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (xgs0)",		/* 21 */
	"pic int 22 (xgs1)",		/* 22 */
	"pic int 23 (irq23)",		/* 23 */
	"pic int 24 (hyper_fatal)",	/* 24 */
	"pic int 25 (bridge_aerr)",	/* 25 */
	"pic int 26 (bridge_berr)",	/* 26 */
	"pic int 27 (bridge_tb)",	/* 27 */
	"pic int 28 (bridge_nmi)",	/* 28 */
	"pic int 29 (bridge_sram_derr)",/* 29 */
	"pic int 30 (gpio_fatal)",	/* 30 */
	"pic int 31 (reserved)",	/* 31 */
};

/*
 * rmixl_irtnames_xls2xx
 * - use for XLS2xx
 */
static const char * const rmixl_irtnames_xls2xx[NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio_a)",		/* 14 */
	"pic int 15 (irq15)",		/* 15 */
	"pic int 16 (bridge_tb)",	/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (irq21)",		/* 21 */
	"pic int 22 (irq22)",		/* 22 */
	"pic int 23 (pcie_link2)",	/* 23 */
	"pic int 24 (pcie_link3)",	/* 24 */
	"pic int 25 (bridge_err)",	/* 25 */
	"pic int 26 (pcie_link0)",	/* 26 */
	"pic int 27 (pcie_link1)",	/* 27 */
	"pic int 28 (irq28)",		/* 28 */
	"pic int 29 (pcie_err)",	/* 29 */
	"pic int 30 (gpio_b)",		/* 30 */
	"pic int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xls1xx
 * - use for XLS1xx, XLS4xx-Lite
 */
static const char * const rmixl_irtnames_xls1xx[NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio_a)",		/* 14 */
	"pic int 15 (irq15)",		/* 15 */
	"pic int 16 (bridge_tb)",	/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (irq21)",		/* 21 */
	"pic int 22 (irq22)",		/* 22 */
	"pic int 23 (irq23)",		/* 23 */
	"pic int 24 (irq24)",		/* 24 */
	"pic int 25 (bridge_err)",	/* 25 */
	"pic int 26 (pcie_link0)",	/* 26 */
	"pic int 27 (pcie_link1)",	/* 27 */
	"pic int 28 (irq28)",		/* 28 */
	"pic int 29 (pcie_err)",	/* 29 */
	"pic int 30 (gpio_b)",		/* 30 */
	"pic int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xls4xx:
 * - use for XLS4xx, XLS6xx
 */
static const char * const rmixl_irtnames_xls4xx[NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio_a)",		/* 14 */
	"pic int 15 (irq15)",		/* 15 */
	"pic int 16 (bridge_tb)",	/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (irq21)",		/* 21 */
	"pic int 22 (irq22)",		/* 22 */
	"pic int 23 (irq23)",		/* 23 */
	"pic int 24 (irq24)",		/* 24 */
	"pic int 25 (bridge_err)",	/* 25 */
	"pic int 26 (pcie_link0)",	/* 26 */
	"pic int 27 (pcie_link1)",	/* 27 */
	"pic int 28 (pcie_link2)",	/* 28 */
	"pic int 29 (pcie_link3)",	/* 29 */
	"pic int 30 (gpio_b)",		/* 30 */
	"pic int 31 (usb)",		/* 31 */
};

/*
 * rmixl_vecnames_common:
 * - use for unknown cpu implementation
 * - covers all vectors, not just IRT intrs
 */
static const char * const rmixl_vecnames_common[NINTRVECS] = {
	"vec 0",		/*  0 */
	"vec 1",		/*  1 */
	"vec 2",		/*  2 */
	"vec 3",		/*  3 */
	"vec 4",		/*  4 */
	"vec 5",		/*  5 */
	"vec 6",		/*  6 */
	"vec 7",		/*  7 */
	"vec 8 (ipi 0)",	/*  8 */
	"vec 9 (ipi 1)",	/*  9 */
	"vec 10 (ipi 2)",	/* 10 */
	"vec 11 (ipi 3)",	/* 11 */
	"vec 12 (ipi 4)",	/* 12 */
	"vec 13 (ipi 5)",	/* 13 */
	"vec 14 (ipi 6)",	/* 14 */
	"vec 15 (fmn)",		/* 15 */
	"vec 16",		/* 16 */
	"vec 17",		/* 17 */
	"vec 18",		/* 18 */
	"vec 19",		/* 19 */
	"vec 20",		/* 20 */
	"vec 21",		/* 21 */
	"vec 22",		/* 22 */
	"vec 23",		/* 23 */
	"vec 24",		/* 24 */
	"vec 25",		/* 25 */
	"vec 26",		/* 26 */
	"vec 27",		/* 27 */
	"vec 28",		/* 28 */
	"vec 29",		/* 29 */
	"vec 30",		/* 30 */
	"vec 31",		/* 31 */
	"vec 32",		/* 32 */
	"vec 33",		/* 33 */
	"vec 34",		/* 34 */
	"vec 35",		/* 35 */
	"vec 36",		/* 36 */
	"vec 37",		/* 37 */
	"vec 38",		/* 38 */
	"vec 39",		/* 39 */
	"vec 40",		/* 40 */
	"vec 41",		/* 41 */
	"vec 42",		/* 42 */
	"vec 43",		/* 43 */
	"vec 44",		/* 44 */
	"vec 45",		/* 45 */
	"vec 46",		/* 46 */
	"vec 47",		/* 47 */
	"vec 48",		/* 48 */
	"vec 49",		/* 49 */
	"vec 50",		/* 50 */
	"vec 51",		/* 51 */
	"vec 52",		/* 52 */
	"vec 53",		/* 53 */
	"vec 54",		/* 54 */
	"vec 55",		/* 55 */
	"vec 56",		/* 56 */
	"vec 57",		/* 57 */
	"vec 58",		/* 58 */
	"vec 59",		/* 59 */
	"vec 60",		/* 60 */
	"vec 61",		/* 61 */
	"vec 62",		/* 63 */
	"vec 63",		/* 63 */
};

/*
 * mask of CPUs attached
 * once they are attached, this var is read-only so mp safe
 */
static uint32_t cpu_present_mask;

kmutex_t rmixl_ipi_lock __cacheline_aligned;
				/* covers RMIXL_PIC_IPIBASE */
kmutex_t rmixl_intr_lock __cacheline_aligned;
				/* covers rest of PIC, and rmixl_intrhand[] */
rmixl_intrhand_t rmixl_intrhand[NINTRVECS];

#ifdef DIAGNOSTIC
static int rmixl_pic_init_done;
#endif


static const char *rmixl_intr_string_xlr(int);
static const char *rmixl_intr_string_xls(int);
static uint32_t rmixl_irt_thread_mask(int);
static void rmixl_irt_init(int);
static void rmixl_irt_disestablish(int);
static void rmixl_irt_establish(int, int, int,
		rmixl_intr_trigger_t, rmixl_intr_polarity_t);

#ifdef MULTIPROCESSOR
static int rmixl_send_ipi(struct cpu_info *, int);
static int rmixl_ipi_intr(void *);
#endif

#if defined(DIAGNOSTIC) || defined(IOINTR_DEBUG) || defined(DDB)
int  rmixl_intrhand_print_subr(int);
int  rmixl_intrhand_print(void);
int  rmixl_irt_print(void);
void rmixl_ipl_eimr_map_print(void);
#endif


static inline u_int
dclz(uint64_t val)
{
	int nlz;

	asm volatile("dclz %0, %1;"
		: "=r"(nlz) : "r"(val));

	return nlz;
}

void
evbmips_intr_init(void)
{
	uint32_t r;

	KASSERT(cpu_rmixlr(mips_options.mips_cpu)
	     || cpu_rmixls(mips_options.mips_cpu));


#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done != 0)
		panic("%s: rmixl_pic_init_done %d",
			__func__, rmixl_pic_init_done);
#endif

	mutex_init(&rmixl_ipi_lock, MUTEX_DEFAULT, IPL_HIGH);
	mutex_init(&rmixl_intr_lock, MUTEX_DEFAULT, IPL_HIGH);

	mutex_enter(&rmixl_intr_lock);

	/*
	 * initialize (zero) all IRT Entries in the PIC
	 */
	for (u_int i = 0; i < NIRTS; i++) {
		rmixl_irt_init(i);
	}

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
	mutex_exit(&rmixl_intr_lock);

}

/*
 * establish vector for mips3 count/compare clock interrupt
 * this ensures we enable in EIRR,
 * even though cpu_intr() handles the interrupt
 * note the 'mpsafe' arg here is a placeholder only
 */
void
rmixl_intr_init_clk(void)
{
	const int vec = ffs(MIPS_INT_MASK_5 >> MIPS_INT_MASK_SHIFT) - 1;

	mutex_enter(&rmixl_intr_lock);

	void *ih = rmixl_vec_establish(vec, 0, IPL_SCHED, NULL, NULL, false);
	if (ih == NULL)
		panic("%s: establish vec %d failed", __func__, vec);

	mutex_exit(&rmixl_intr_lock);
}

#ifdef MULTIPROCESSOR
/*
 * establish IPI interrupt and send function
 */
void
rmixl_intr_init_ipi(void)
{
	mutex_enter(&rmixl_intr_lock);

	for (u_int ipi = 0; ipi < NIPIS; ipi++) {
		const u_int vec = RMIXL_INTRVEC_IPI + ipi;
		void * const ih = rmixl_vec_establish(vec, -1, IPL_SCHED,
			rmixl_ipi_intr, (void *)(uintptr_t)ipi, true);
		if (ih == NULL)
			panic("%s: establish ipi %d at vec %d failed",
				__func__, ipi, vec);
	}

	mips_locoresw.lsw_send_ipi = rmixl_send_ipi;

	mutex_exit(&rmixl_intr_lock);
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

	for (int vec=0; vec < NINTRVECS; vec++)
		evcnt_attach_dynamic(&sc->sc_vec_evcnts[vec],
			EVCNT_TYPE_INTR, NULL,
			device_xname(sc->sc_dev),
			rmixl_intr_string(vec));

	KASSERT(cpu_index(ci) < (sizeof(cpu_present_mask) * 8));
	atomic_or_32((volatile uint32_t *)&cpu_present_mask, 1 << cpu_index(ci));
}

/*
 * rmixl_intr_string - return pointer to display name of a PIC-based interrupt
 */
const char *
rmixl_intr_string(int vec)
{
	int irt;

	if (vec < 0 || vec >= NINTRVECS)
		panic("%s: vec index %d out of range, max %d",
			__func__, vec, NINTRVECS - 1);

	if (! RMIXL_VECTOR_IS_IRT(vec))
		return rmixl_vecnames_common[vec];

	irt = RMIXL_VECTOR_IRT(vec);
	switch(cpu_rmixl_chip_type(mips_options.mips_cpu)) {
	case CIDFL_RMI_TYPE_XLR:
		return rmixl_intr_string_xlr(irt);
	case CIDFL_RMI_TYPE_XLS:
		return rmixl_intr_string_xls(irt);
	case CIDFL_RMI_TYPE_XLP:
		panic("%s: RMI XLP not yet supported", __func__);
	}

	return "undefined";	/* appease gcc */
}

static const char *
rmixl_intr_string_xlr(int irt)
{
	return rmixl_irtnames_xlrxxx[irt];
}

static const char *
rmixl_intr_string_xls(int irt)
{
	const char *name;

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		name = rmixl_irtnames_xls1xx[irt];
		break;
	case MIPS_XLS204:
	case MIPS_XLS208:
		name = rmixl_irtnames_xls2xx[irt];
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608:
	case MIPS_XLS616:
		name = rmixl_irtnames_xls4xx[irt];
		break;
	default:
		name = rmixl_vecnames_common[RMIXL_IRT_VECTOR(irt)];
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
 * - initialize IRT Entry for given index
 * - unmask Thread#0 in low word (assume we only have 1 thread)
 */
static void
rmixl_irt_init(int irt)
{
	KASSERT(irt < NIRTS);
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irt), 0);	/* high word */
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irt), 0);	/* low  word */
}

/*
 * rmixl_irt_disestablish
 * - invalidate IRT Entry for given index
 */
static void
rmixl_irt_disestablish(int irt)
{
	KASSERT(mutex_owned(&rmixl_intr_lock));
	DPRINTF(("%s: irt %d, irtc1 %#x\n", __func__, irt, 0));
	rmixl_irt_init(irt);
}

/*
 * rmixl_irt_establish
 * - construct an IRT Entry for irt and write to PIC
 */
static void
rmixl_irt_establish(int irt, int vec, int cpumask, rmixl_intr_trigger_t trigger,
	rmixl_intr_polarity_t polarity)
{
	uint32_t irtc1;
	uint32_t irtc0;

	KASSERT(mutex_owned(&rmixl_intr_lock));

	if (irt >= NIRTS)
		panic("%s: bad irt %d\n", __func__, irt);

	if (! RMIXL_VECTOR_IS_IRT(vec))
		panic("%s: bad vec %d\n", __func__, vec);

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
	KASSERT(RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irt)) == 0);
	KASSERT(RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irt)) == 0);

	irtc0 = rmixl_irt_thread_mask(cpumask);

	irtc1  = RMIXL_PIC_IRTENTRYC1_VALID;
	irtc1 |= RMIXL_PIC_IRTENTRYC1_GL;	/* local */
	KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

	if (trigger == RMIXL_TRIG_LEVEL)
		irtc1 |= RMIXL_PIC_IRTENTRYC1_TRG;
	KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

	if ((polarity == RMIXL_POLR_FALLING) || (polarity == RMIXL_POLR_LOW))
		irtc1 |= RMIXL_PIC_IRTENTRYC1_P;
	KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

	irtc1 |= vec;			/* vector in EIRR */
	KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

	/*
	 * write IRT Entry to PIC
	 */
	DPRINTF(("%s: vec %d (%#x), irt %d, irtc0 %#x, irtc1 %#x\n",
		__func__, vec, vec, irt, irtc0, irtc1));
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irt), irtc0);	/* low  word */
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irt), irtc1);	/* high word */
}

void *
rmixl_vec_establish(int vec, int cpumask, int ipl,
	int (*func)(void *), void *arg, bool mpsafe)
{
	rmixl_intrhand_t *ih;
	uint64_t eimr_bit;
	int s;

	KASSERT(mutex_owned(&rmixl_intr_lock));

	DPRINTF(("%s: vec %d cpumask %#x ipl %d func %p arg %p mpsafe %d\n",
			__func__, vec, cpumask, ipl, func, arg, mpsafe));
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
	if (ih->ih_func != NULL) {
#ifdef DIAGNOSTIC
		printf("%s: intrhand[%d] busy\n", __func__, vec);
#endif
		splx(s);
		return NULL;
	}

	ih->ih_arg = arg;
	ih->ih_mpsafe = mpsafe;
	ih->ih_vec = vec;
	ih->ih_ipl = ipl;
	ih->ih_cpumask = cpumask;

	eimr_bit = (uint64_t)1 << vec;
	for (int i=ih->ih_ipl; --i >= 0; ) {
		KASSERT((ipl_eimr_map[i] & eimr_bit) == 0);
		ipl_eimr_map[i] |= eimr_bit;
	}

	ih->ih_func = func;	/* do this last */

	splx(s);

	return ih;
}

/*
 * rmixl_intr_establish
 * - used to establish an IRT-based interrupt only
 */
void *
rmixl_intr_establish(int irt, int cpumask, int ipl,
	rmixl_intr_trigger_t trigger, rmixl_intr_polarity_t polarity,
	int (*func)(void *), void *arg, bool mpsafe)
{
	rmixl_intrhand_t *ih;
	int vec;

#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done == 0)
		panic("%s: called before rmixl_pic_init_done", __func__);
#endif

	/*
	 * check args
	 */
	if (irt < 0 || irt >= NIRTS)
		panic("%s: irt %d out of range, max %d",
			__func__, irt, NIRTS - 1);
	if (ipl <= 0 || ipl >= _IPL_N)
		panic("%s: ipl %d out of range, min %d, max %d",
			__func__, ipl, 1, _IPL_N - 1);

	vec = RMIXL_IRT_VECTOR(irt);

	DPRINTF(("%s: irt %d, vec %d, ipl %d\n", __func__, irt, vec, ipl));

	mutex_enter(&rmixl_intr_lock);

	/*
	 * establish vector
	 */
	ih = rmixl_vec_establish(vec, cpumask, ipl, func, arg, mpsafe);

	/*
	 * establish IRT Entry
	 */
	rmixl_irt_establish(irt, vec, cpumask, trigger, polarity);

	mutex_exit(&rmixl_intr_lock);

	return ih;
}

void
rmixl_vec_disestablish(void *cookie)
{
	rmixl_intrhand_t *ih = cookie;
	uint64_t eimr_bit;

	KASSERT(mutex_owned(&rmixl_intr_lock));
	KASSERT(ih->ih_vec < NINTRVECS);
	KASSERT(ih == &rmixl_intrhand[ih->ih_vec]);

	ih->ih_func = NULL;	/* do this first */

	eimr_bit = (uint64_t)1 << ih->ih_vec;
	for (int i=ih->ih_ipl; --i >= 0; ) {
		KASSERT((ipl_eimr_map[i] & eimr_bit) != 0);
		ipl_eimr_map[i] ^= eimr_bit;
	}
}

void
rmixl_intr_disestablish(void *cookie)
{
	rmixl_intrhand_t *ih = cookie;
	const int vec = ih->ih_vec;

	KASSERT(vec < NINTRVECS);
	KASSERT(ih == &rmixl_intrhand[vec]);

	mutex_enter(&rmixl_intr_lock);

	/*
	 * disable/invalidate the IRT Entry if needed
	 */
	if (RMIXL_VECTOR_IS_IRT(vec))
		rmixl_irt_disestablish(vec);

	/*
	 * disasociate from vector and free the handle
	 */
	rmixl_vec_disestablish(cookie);

	mutex_exit(&rmixl_intr_lock);
}

void
evbmips_iointr(int ipl, uint32_t pending, struct clockframe *cf)
{
	struct rmixl_cpu_softc *sc = (void *)curcpu()->ci_softc;

	DPRINTF(("%s: cpu%u: ipl %d, pc %#"PRIxVADDR", pending %#x\n",
		__func__, cpu_number(), ipl, cf->pc, pending));

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
		asm volatile("dmfc0 %0, $9, 7;" : "=r"(eimr));

#ifdef IOINTR_DEBUG
		printf("%s: cpu%u: eirr %#"PRIx64", eimr %#"PRIx64", mask %#"PRIx64"\n",
			__func__, cpu_number(), eirr, eimr, ipl_eimr_map[ipl-1]);
#endif	/* IOINTR_DEBUG */

		/*
		 * reduce eirr to
		 * - ints that are enabled at or below this ipl
		 * - exclude count/compare clock and soft ints
		 *   they are handled elsewhere
		 */
		eirr &= ipl_eimr_map[ipl-1];
		eirr &= ~ipl_eimr_map[ipl];
		eirr &= ~((MIPS_INT_MASK_5 | MIPS_SOFT_INT_MASK) >> 8);
		if (eirr == 0)
			break;

		vec = 63 - dclz(eirr);
		ih = &rmixl_intrhand[vec];
		vecbit = 1ULL << vec;
		KASSERT (ih->ih_ipl == ipl);
		KASSERT ((vecbit & eimr) == 0);
		KASSERT ((vecbit & RMIXL_EIRR_PRESERVE_MASK) == 0);

		/*
		 * ack in EIRR, and in PIC if needed,
		 * the irq we are about to handle
		 */
		rmixl_eirr_ack(eimr, vecbit, RMIXL_EIRR_PRESERVE_MASK);
		if (RMIXL_VECTOR_IS_IRT(vec))
			RMIXL_PICREG_WRITE(RMIXL_PIC_INTRACK,
				1 << RMIXL_VECTOR_IRT(vec));

		if (ih->ih_func != NULL) {
#ifdef MULTIPROCESSOR
			if (ih->ih_mpsafe) {
				(void)(*ih->ih_func)(ih->ih_arg);
			} else {
				KASSERTMSG(ipl == IPL_VM,
				    "%s: %s: ipl (%d) != IPL_VM for KERNEL_LOCK",
				    __func__, sc->sc_vec_evcnts[vec].ev_name,
				    ipl);
				KERNEL_LOCK(1, NULL);
				(void)(*ih->ih_func)(ih->ih_arg);
				KERNEL_UNLOCK_ONE(NULL);
			}
#else
			(void)(*ih->ih_func)(ih->ih_arg);
#endif /* MULTIPROCESSOR */
		}
		KASSERT(ipl == ih->ih_ipl);
		KASSERTMSG(curcpu()->ci_cpl >= ipl,
		    "%s: after %s: cpl (%d) < ipl %d",
		    __func__, sc->sc_vec_evcnts[vec].ev_name,
		    ipl, curcpu()->ci_cpl);
		sc->sc_vec_evcnts[vec].ev_count++;
	}
}

#ifdef MULTIPROCESSOR
static int
rmixl_send_ipi(struct cpu_info *ci, int tag)
{
	const cpuid_t cpuid = ci->ci_cpuid;
	uint32_t core = (uint32_t)(cpuid >> 2);
	uint32_t thread = (uint32_t)(cpuid & __BITS(1,0));
	uint64_t req = 1 << tag;
	uint32_t r;

	if (!kcpuset_isset(cpus_running, cpu_index(ci)))
		return -1;

	KASSERT((tag >= 0) && (tag < NIPIS));

	r = (thread << RMIXL_PIC_IPIBASE_ID_THREAD_SHIFT)
	  | (core << RMIXL_PIC_IPIBASE_ID_CORE_SHIFT)
	  | (RMIXL_INTRVEC_IPI + tag);

	mutex_enter(&rmixl_ipi_lock);
	atomic_or_64(&ci->ci_request_ipis, req);
	RMIXL_PICREG_WRITE(RMIXL_PIC_IPIBASE, r);
	mutex_exit(&rmixl_ipi_lock);

	return 0;
}

static int
rmixl_ipi_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	const uint64_t ipi_mask = 1ULL << (uintptr_t)arg;

	KASSERT(ci->ci_cpl >= IPL_SCHED);
	KASSERT((uintptr_t)arg < NIPIS);

	/* if the request is clear, it was previously processed */
	if ((ci->ci_request_ipis & ipi_mask) == 0)
		return 0;

	atomic_or_64(&ci->ci_active_ipis, ipi_mask);
	atomic_and_64(&ci->ci_request_ipis, ~ipi_mask);

	ipi_process(ci, ipi_mask);

	atomic_and_64(&ci->ci_active_ipis, ~ipi_mask);

	return 1;
}
#endif	/* MULTIPROCESSOR */

#if defined(DIAGNOSTIC) || defined(IOINTR_DEBUG) || defined(DDB)
int
rmixl_intrhand_print_subr(int vec)
{
	rmixl_intrhand_t *ih = &rmixl_intrhand[vec];
	printf("vec %d: func %p, arg %p, vec %d, ipl %d, mask %#x\n",
		vec, ih->ih_func, ih->ih_arg, ih->ih_vec, ih->ih_ipl,
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

static inline void
rmixl_irt_entry_print(u_int irt)
{
	uint32_t c0, c1;

	if ((irt < 0) || (irt > NIRTS))
		return;
	c0 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irt));
	c1 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irt));
	printf("irt[%d]: %#x, %#x\n", irt, c0, c1);
}

int
rmixl_irt_print(void)
{
	printf("%s:\n", __func__);
	for (int irt=0; irt < NIRTS ; irt++)
		rmixl_irt_entry_print(irt);
	return 0;
}

void
rmixl_ipl_eimr_map_print(void)
{
	printf("IPL_NONE=%d, mask %#"PRIx64"\n",
		IPL_NONE, ipl_eimr_map[IPL_NONE]);
	printf("IPL_SOFTCLOCK=%d, mask %#"PRIx64"\n",
		IPL_SOFTCLOCK, ipl_eimr_map[IPL_SOFTCLOCK]);
	printf("IPL_SOFTNET=%d, mask %#"PRIx64"\n",
		IPL_SOFTNET, ipl_eimr_map[IPL_SOFTNET]);
	printf("IPL_VM=%d, mask %#"PRIx64"\n",
		IPL_VM, ipl_eimr_map[IPL_VM]);
	printf("IPL_SCHED=%d, mask %#"PRIx64"\n",
		IPL_SCHED, ipl_eimr_map[IPL_SCHED]);
	printf("IPL_DDB=%d, mask %#"PRIx64"\n",
		IPL_DDB, ipl_eimr_map[IPL_DDB]);
	printf("IPL_HIGH=%d, mask %#"PRIx64"\n",
		IPL_HIGH, ipl_eimr_map[IPL_HIGH]);
}

#endif
