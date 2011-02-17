/*	$NetBSD: e500_intr.c,v 1.2.2.1 2011/02/17 11:59:55 bouyer Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include "opt_mpc85xx.h"

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/openpicreg.h>

#define	IPL2CTPR(ipl)		((ipl) + 15 - IPL_HIGH)
#define CTPR2IPL(ctpr)		((ctpr) - (15 - IPL_HIGH))

#define	IST_PERCPU_P(ist)	((ist) >= IST_TIMER)

#define	IPL_SOFTMASK \
	    ((1 << IPL_SOFTSERIAL) | (1 << IPL_SOFTNET   )	\
	    |(1 << IPL_SOFTBIO   ) | (1 << IPL_SOFTCLOCK ))

#define SOFTINT2IPL_MAP \
	    ((IPL_SOFTSERIAL << (4*SOFTINT_SERIAL))	\
	    |(IPL_SOFTNET    << (4*SOFTINT_NET   ))	\
	    |(IPL_SOFTBIO    << (4*SOFTINT_BIO   ))	\
	    |(IPL_SOFTCLOCK  << (4*SOFTINT_CLOCK )))
#define	SOFTINT2IPL(si_level)	((SOFTINT2IPL_MAP >> (4 * si_level)) & 0x0f)

struct e500_intr_irq_info {
	bus_addr_t irq_vpr;
	bus_addr_t irq_dr;
	u_int irq_vector;
};

struct intr_source {
	int (*is_func)(void *);
	void *is_arg;
	int8_t is_ipl;
	uint8_t is_ist;
	uint8_t is_irq;
	bus_size_t is_vpr;
	bus_size_t is_dr;
};

#define	INTR_SOURCE_INITIALIZER \
	{ .is_func = e500_intr_spurious, .is_arg = NULL, \
	.is_irq = -1, .is_ipl = IPL_NONE, .is_ist = IST_NONE, }

struct e500_intr_name {
	uint8_t in_irq;
	const char in_name[15];
};

static const struct e500_intr_name e500_onchip_intr_names[] = {
	{ ISOURCE_L2, "l2" },
	{ ISOURCE_ECM, "ecm" },
	{ ISOURCE_DDR, "ddr" },
	{ ISOURCE_LBC, "lbc" },
	{ ISOURCE_DMA_CHAN1, "dma-chan1" },
	{ ISOURCE_DMA_CHAN2, "dma-chan2" },
	{ ISOURCE_DMA_CHAN3, "dma-chan3" },
	{ ISOURCE_DMA_CHAN4, "dma-chan4" },
	{ ISOURCE_PCI1, "pci1" },
	{ ISOURCE_PCIEX2, "pcie2" },
	{ ISOURCE_PCIEX	, "pcie1" },
	{ ISOURCE_PCIEX3, "pcie3" },
	{ ISOURCE_USB1, "usb1" },
	{ ISOURCE_ETSEC1_TX, "etsec1-tx" },
	{ ISOURCE_ETSEC1_RX, "etsec1-rx" },
	{ ISOURCE_ETSEC3_TX, "etsec3-tx" },
	{ ISOURCE_ETSEC3_RX, "etsec3-rx" },
	{ ISOURCE_ETSEC3_ERR, "etsec3-err" },
	{ ISOURCE_ETSEC1_ERR, "etsec1-err" },
	{ ISOURCE_ETSEC2_TX, "etsec2-tx" },
	{ ISOURCE_ETSEC2_RX, "etsec2-rx" },
	{ ISOURCE_ETSEC4_TX, "etsec4-tx" },
	{ ISOURCE_ETSEC4_RX, "etsec4-rx" },
	{ ISOURCE_ETSEC4_ERR, "etsec4-err" },
	{ ISOURCE_ETSEC2_ERR, "etsec2-err" },
	{ ISOURCE_DUART, "duart" },
	{ ISOURCE_I2C, "i2c" },
	{ ISOURCE_PERFMON, "perfmon" },
	{ ISOURCE_SECURITY1, "sec1" },
	{ ISOURCE_GPIO, "gpio" },
	{ ISOURCE_SRIO_EWPU, "srio-ewpu" },
	{ ISOURCE_SRIO_ODBELL, "srio-odbell" },
	{ ISOURCE_SRIO_IDBELL, "srio-idbell" },
	{ ISOURCE_SRIO_OMU1, "srio-omu1" },
	{ ISOURCE_SRIO_IMU1, "srio-imu1" },
	{ ISOURCE_SRIO_OMU2, "srio-omu2" },
	{ ISOURCE_SECURITY2, "sec2" },
	{ ISOURCE_SPI, "spi" },
	{ ISOURCE_ETSEC1_PTP, "etsec1-ptp" },
	{ ISOURCE_ETSEC2_PTP, "etsec2-ptp" },
	{ ISOURCE_ETSEC3_PTP, "etsec3-ptp" },
	{ ISOURCE_ETSEC4_PTP, "etsec4-ptp" },
	{ ISOURCE_ESDHC, "esdhc" },
	{ 0, "" },
};

const struct e500_intr_name default_external_intr_names[] = {
	{ 0, "" },
};

static const struct e500_intr_name e500_msigroup_intr_names[] = {
	{ 0, "msigroup0" },
	{ 1, "msigroup1" },
	{ 2, "msigroup2" },
	{ 3, "msigroup3" },
	{ 4, "msigroup4" },
	{ 5, "msigroup5" },
	{ 6, "msigroup6" },
	{ 7, "msigroup7" },
	{ 0, "" },
};

static const struct e500_intr_name e500_timer_intr_names[] = {
	{ 0, "timer0" },
	{ 1, "timer1" },
	{ 2, "timer2" },
	{ 3, "timer3" },
	{ 0, "" },
};

static const struct e500_intr_name e500_ipi_intr_names[] = {
	{ 0, "ipi0" },
	{ 1, "ipi1" },
	{ 2, "ipi2" },
	{ 3, "ipi3" },
	{ 0, "" },
};

static const struct e500_intr_name e500_mi_intr_names[] = {
	{ 0, "mi0" },
	{ 1, "mi1" },
	{ 2, "mi2" },
	{ 3, "mi3" },
	{ 0, "" },
};

struct e500_intr_info {
	u_int ii_external_sources;
	uint32_t ii_onchip_bitmap[2];
	u_int ii_onchip_sources;
	u_int ii_msigroup_sources;
	u_int ii_ipi_sources;			/* per-cpu */
	u_int ii_timer_sources;			/* per-cpu */
	u_int ii_mi_sources;			/* per-cpu */
	u_int ii_percpu_sources;
	const struct e500_intr_name *ii_external_intr_names;
	const struct e500_intr_name *ii_onchip_intr_names;
	u_int8_t ii_ist_vectors[IST_MAX+1];
};

static kmutex_t e500_intr_lock __cacheline_aligned;
static struct e500_intr_info e500_intr_info;

#define	INTR_INFO_DECL(lc_chip, UC_CHIP)				\
static const struct e500_intr_info lc_chip##_intr_info = {		\
	.ii_external_sources = UC_CHIP ## _EXTERNALSOURCES,		\
	.ii_onchip_bitmap = UC_CHIP ## _ONCHIPBITMAP,			\
	.ii_onchip_sources = UC_CHIP ## _ONCHIPSOURCES,			\
	.ii_msigroup_sources = UC_CHIP ## _MSIGROUPSOURCES,		\
	.ii_timer_sources = UC_CHIP ## _TIMERSOURCES,			\
	.ii_ipi_sources = UC_CHIP ## _IPISOURCES,			\
	.ii_mi_sources = UC_CHIP ## _MISOURCES,				\
	.ii_percpu_sources = UC_CHIP ## _TIMERSOURCES			\
	    + UC_CHIP ## _IPISOURCES + UC_CHIP ## _MISOURCES, 		\
	.ii_external_intr_names = lc_chip ## _external_intr_names,	\
	.ii_onchip_intr_names = lc_chip ## _onchip_intr_names,		\
	.ii_ist_vectors = {						\
		[IST_NONE]		= ~0,				\
		[IST_EDGE]		= 0,				\
		[IST_LEVEL_LOW]		= 0,				\
		[IST_LEVEL_HIGH]	= 0,				\
		[IST_ONCHIP]		= UC_CHIP ## _EXTERNALSOURCES,	\
		[IST_MSIGROUP]		= UC_CHIP ## _EXTERNALSOURCES	\
					    + UC_CHIP ## _ONCHIPSOURCES, \
		[IST_TIMER]		= UC_CHIP ## _EXTERNALSOURCES	\
					    + UC_CHIP ## _ONCHIPSOURCES	\
					    + UC_CHIP ## _MSIGROUPSOURCES, \
		[IST_IPI]		= UC_CHIP ## _EXTERNALSOURCES	\
					    + UC_CHIP ## _ONCHIPSOURCES	\
					    + UC_CHIP ## _MSIGROUPSOURCES \
					    + UC_CHIP ## _TIMERSOURCES,	\
		[IST_MI]		= UC_CHIP ## _EXTERNALSOURCES	\
					    + UC_CHIP ## _ONCHIPSOURCES	\
					    + UC_CHIP ## _MSIGROUPSOURCES \
					    + UC_CHIP ## _TIMERSOURCES	\
					    + UC_CHIP ## _IPISOURCES,	\
		[IST_MAX]		= UC_CHIP ## _EXTERNALSOURCES	\
					    + UC_CHIP ## _ONCHIPSOURCES	\
					    + UC_CHIP ## _MSIGROUPSOURCES \
					    + UC_CHIP ## _TIMERSOURCES	\
					    + UC_CHIP ## _IPISOURCES	\
					    + UC_CHIP ## _MISOURCES,	\
	},								\
}

#ifdef MPC8536
#define	mpc8536_external_intr_names	default_external_intr_names
const struct e500_intr_name mpc8536_onchip_intr_names[] = {
	{ ISOURCE_SATA2, "sata2" },
	{ ISOURCE_USB2, "usb2" },
	{ ISOURCE_USB3, "usb3" },
	{ ISOURCE_SATA1, "sata1" },
	{ 0, "" },
};

INTR_INFO_DECL(mpc8536, MPC8536);
#endif

#ifdef MPC8544
#define	mpc8544_external_intr_names	default_external_intr_names
const struct e500_intr_name mpc8544_onchip_intr_names[] = {
	{ 0, "" },
};

INTR_INFO_DECL(mpc8544, MPC8544);
#endif
#ifdef MPC8548
#define	mpc8548_external_intr_names	default_external_intr_names
const struct e500_intr_name mpc8548_onchip_intr_names[] = {
	{ ISOURCE_PCI1, "pci1" },
	{ ISOURCE_PCI2, "pci2" },
	{ 0, "" },
};

INTR_INFO_DECL(mpc8548, MPC8548);
#endif
#ifdef MPC8555
#define	mpc8555_external_intr_names	default_external_intr_names
const struct e500_intr_name mpc8555_onchip_intr_names[] = {
	{ ISOURCE_PCI2, "pci2" },
	{ ISOURCE_CPM, "CPM" },
	{ 0, "" },
};

INTR_INFO_DECL(mpc8555, MPC8555);
#endif
#ifdef MPC8568
#define	mpc8568_external_intr_names	default_external_intr_names
const struct e500_intr_name mpc8568_onchip_intr_names[] = {
	{ ISOURCE_QEB_LOW, "QEB low" },
	{ ISOURCE_QEB_PORT, "QEB port" },
	{ ISOURCE_QEB_IECC, "QEB iram ecc" },
	{ ISOURCE_QEB_MUECC, "QEB ram ecc" },
	{ ISOURCE_TLU1, "tlu1" },
	{ ISOURCE_QEB_HIGH, "QEB high" },
	{ 0, "" },
};

INTR_INFO_DECL(mpc8568, MPC8568);
#endif
#ifdef MPC8572
#define	mpc8572_external_intr_names	default_external_intr_names
const struct e500_intr_name mpc8572_onchip_intr_names[] = {
	{ ISOURCE_PCIEX3_MPC8572, "pcie3" },
	{ ISOURCE_FEC, "fec" },
	{ ISOURCE_PME_GENERAL, "pme" },
	{ ISOURCE_TLU1, "tlu1" },
	{ ISOURCE_TLU2, "tlu2" },
	{ ISOURCE_PME_CHAN1, "pme-chan1" },
	{ ISOURCE_PME_CHAN2, "pme-chan2" },
	{ ISOURCE_PME_CHAN3, "pme-chan3" },
	{ ISOURCE_PME_CHAN4, "pme-chan4" },
	{ ISOURCE_DMA2_CHAN1, "dma2-chan1" },
	{ ISOURCE_DMA2_CHAN2, "dma2-chan2" },
	{ ISOURCE_DMA2_CHAN3, "dma2-chan3" },
	{ ISOURCE_DMA2_CHAN4, "dma2-chan4" },
	{ 0, "" },
};

INTR_INFO_DECL(mpc8572, MPC8572);
#endif
#ifdef P2020
#define	p20x0_external_intr_names	default_external_intr_names
const struct e500_intr_name p20x0_onchip_intr_names[] = {
	{ ISOURCE_PCIEX3_MPC8572, "pcie3" },
	{ ISOURCE_DMA2_CHAN1, "dma2-chan1" },
	{ ISOURCE_DMA2_CHAN2, "dma2-chan2" },
	{ ISOURCE_DMA2_CHAN3, "dma2-chan3" },
	{ ISOURCE_DMA2_CHAN4, "dma2-chan4" },
	{ 0, "" },
};

INTR_INFO_DECL(p20x0, P20x0);
#endif

static const char ist_names[][12] = {
	[IST_NONE] = "none",
	[IST_EDGE] = "edge",
	[IST_LEVEL_LOW] = "level-",
	[IST_LEVEL_HIGH] = "level+",
	[IST_MSI] = "msi",
	[IST_ONCHIP] = "onchip",
	[IST_MSIGROUP] = "msigroup",
	[IST_TIMER] = "timer",
	[IST_IPI] = "ipi",
	[IST_MI] = "msgint",
};

static struct intr_source *e500_intr_sources;
static const struct intr_source *e500_intr_last_source;

static void 	*e500_intr_establish(int, int, int, int (*)(void *), void *);
static void 	e500_intr_disestablish(void *);
static void 	e500_intr_cpu_init(struct cpu_info *ci);
static void 	e500_intr_init(void);
static const char *e500_intr_string(int, int);
static void 	e500_critintr(struct trapframe *tf);
static void 	e500_decrintr(struct trapframe *tf);
static void 	e500_extintr(struct trapframe *tf);
static void 	e500_fitintr(struct trapframe *tf);
static void 	e500_wdogintr(struct trapframe *tf);
static void	e500_spl0(void);
static int 	e500_splraise(int);
static void 	e500_splx(int);
#ifdef __HAVE_FAST_SOFTINTS
static void 	e500_softint_init_md(lwp_t *l, u_int si_level, uintptr_t *machdep_p);
static void 	e500_softint_trigger(uintptr_t machdep);
#endif

const struct intrsw e500_intrsw = {
	.intrsw_establish = e500_intr_establish,
	.intrsw_disestablish = e500_intr_disestablish,
	.intrsw_init = e500_intr_init,
	.intrsw_cpu_init = e500_intr_cpu_init,
	.intrsw_string = e500_intr_string,

	.intrsw_critintr = e500_critintr,
	.intrsw_decrintr = e500_decrintr,
	.intrsw_extintr = e500_extintr,
	.intrsw_fitintr = e500_fitintr,
	.intrsw_wdogintr = e500_wdogintr,

	.intrsw_splraise = e500_splraise,
	.intrsw_splx = e500_splx,
	.intrsw_spl0 = e500_spl0,

#ifdef __HAVE_FAST_SOFTINTS
	.intrsw_softint_init_md = e500_softint_init_md,
	.intrsw_softint_trigger = e500_softint_trigger,
#endif
};

static inline uint32_t 
openpic_read(struct cpu_softc *cpu, bus_size_t offset)
{

	return bus_space_read_4(cpu->cpu_bst, cpu->cpu_bsh,
	    OPENPIC_BASE + offset);
}

static inline void 
openpic_write(struct cpu_softc *cpu, bus_size_t offset, uint32_t val)
{

	return bus_space_write_4(cpu->cpu_bst, cpu->cpu_bsh,
	    OPENPIC_BASE + offset, val);
}

static const char *
e500_intr_external_name_lookup(int irq)
{
	prop_array_t extirqs = board_info_get_object("external-irqs");
	prop_string_t irqname = prop_array_get(extirqs, irq);
	KASSERT(irqname != NULL);
	KASSERT(prop_object_type(irqname) == PROP_TYPE_STRING);

	return prop_string_cstring_nocopy(irqname);
}

static const char *
e500_intr_name_lookup(const struct e500_intr_name *names, int irq)
{
	for (; names->in_name[0] != '\0'; names++) {
		if (names->in_irq == irq)
			return names->in_name;
	}

	return NULL;
}

static const char *
e500_intr_onchip_name_lookup(int irq)
{
	const char *name;

	return e500_intr_name_lookup(e500_intr_info.ii_onchip_intr_names, irq);
	if (name != NULL)
		return name;

	name = e500_intr_name_lookup(e500_onchip_intr_names, irq);
}

#ifdef __HAVE_FAST_SOFTINTS
static inline void
e500_softint_deliver(struct cpu_info *ci, struct cpu_softc *cpu,
	int ipl, int si_level)
{
	KASSERT(ci->ci_data.cpu_softints & (1 << ipl));
	ci->ci_data.cpu_softints ^= 1 << ipl;
	softint_fast_dispatch(cpu->cpu_softlwps[si_level], ipl);
	KASSERT(cpu->cpu_softlwps[si_level]->l_ctxswtch == 0);
	KASSERTMSG(ci->ci_cpl == IPL_HIGH,
	    ("%s: cpl (%d) != HIGH", __func__, ci->ci_cpl));
}

static inline void
e500_softint(struct cpu_info *ci, struct cpu_softc *cpu, int old_ipl)
{
	const u_int softint_mask = (IPL_SOFTMASK << old_ipl) & IPL_SOFTMASK;
	u_int softints;

	KASSERT(ci->ci_mtx_count == 0);
	KASSERT(ci->ci_cpl == IPL_HIGH);
	while ((softints = (ci->ci_data.cpu_softints & softint_mask)) != 0) {
		KASSERT(old_ipl < IPL_SOFTSERIAL);
		if (softints & (1 << IPL_SOFTSERIAL)) {
			e500_softint_deliver(ci, cpu, IPL_SOFTSERIAL,
			    SOFTINT_SERIAL);
			continue;
		}
		KASSERT(old_ipl < IPL_SOFTNET);
		if (softints & (1 << IPL_SOFTNET)) {
			e500_softint_deliver(ci, cpu, IPL_SOFTNET,
			    SOFTINT_NET);
			continue;
		}
		KASSERT(old_ipl < IPL_SOFTBIO);
		if (softints & (1 << IPL_SOFTBIO)) {
			e500_softint_deliver(ci, cpu, IPL_SOFTBIO,
			    SOFTINT_BIO);
			continue;
		}
		KASSERT(old_ipl < IPL_SOFTCLOCK);
		if (softints & (1 << IPL_SOFTCLOCK)) {
			e500_softint_deliver(ci, cpu, IPL_SOFTCLOCK,
			    SOFTINT_CLOCK);
			continue;
		}
	}
}
#endif /* __HAVE_FAST_SOFTINTS */

static inline void
e500_splset(struct cpu_info *ci, int ipl)
{
	struct cpu_softc * const cpu = ci->ci_softc;
	//KASSERT(!cpu_intr_p() || ipl >= IPL_VM);
	KASSERT((curlwp->l_pflag & LP_INTR) == 0 || ipl != IPL_NONE);
#if 0
	u_int ctpr = ipl;
	KASSERT(openpic_read(cpu, OPENPIC_CTPR) == ci->ci_cpl);
#elif 0
	u_int old_ctpr = (ci->ci_cpl >= IPL_VM ? 15 : ci->ci_cpl);
	u_int ctpr = (ipl >= IPL_VM ? 15 : ipl);
	KASSERT(openpic_read(cpu, OPENPIC_CTPR) == old_ctpr);
#else
	u_int old_ctpr = IPL2CTPR(ci->ci_cpl);
	u_int ctpr = IPL2CTPR(ipl);
	KASSERT(openpic_read(cpu, OPENPIC_CTPR) == old_ctpr);
#endif
	openpic_write(cpu, OPENPIC_CTPR, ctpr);
	KASSERT(openpic_read(cpu, OPENPIC_CTPR) == ctpr);
	ci->ci_cpl = ipl;
}

static void
e500_spl0(void)
{
	struct cpu_info * const ci = curcpu();

	wrtee(0);

#ifdef __HAVE_FAST_SOFTINTS
	if (__predict_false(ci->ci_data.cpu_softints != 0)) {
		e500_splset(ci, IPL_HIGH);
		e500_softint(ci, ci->ci_softc, IPL_NONE);
	}
#endif /* __HAVE_FAST_SOFTINTS */
	e500_splset(ci, IPL_NONE);

	wrtee(PSL_EE);
}

static void
e500_splx(int ipl)
{
	struct cpu_info * const ci = curcpu();
	const int old_ipl = ci->ci_cpl;

	KASSERT(mfmsr() & PSL_CE);

	if (ipl == old_ipl)
		return;

	if (__predict_false(ipl > old_ipl)) {
		printf("%s: %p: cpl=%u: ignoring splx(%u) to raise ipl\n",
		    __func__, __builtin_return_address(0), old_ipl, ipl);
		if (old_ipl == IPL_NONE)
			Debugger();
	}

	// const
	register_t msr = wrtee(0);
#ifdef __HAVE_FAST_SOFTINTS
	const u_int softints = (ci->ci_data.cpu_softints << ipl) & IPL_SOFTMASK;
	if (__predict_false(softints != 0)) {
		e500_splset(ci, IPL_HIGH);
		e500_softint(ci, ci->ci_softc, ipl);
	}
#endif /* __HAVE_FAST_SOFTINTS */
	e500_splset(ci, ipl);
#if 1
	if (ipl < IPL_VM && old_ipl >= IPL_VM)
		msr = PSL_EE;
#endif
	wrtee(msr);
}

static int
e500_splraise(int ipl)
{
	struct cpu_info * const ci = curcpu();
	const int old_ipl = ci->ci_cpl;

	KASSERT(mfmsr() & PSL_CE);

	if (old_ipl < ipl) {
		//const
		register_t msr = wrtee(0);
		e500_splset(ci, ipl);
#if 1
		if (old_ipl < IPL_VM && ipl >= IPL_VM)
			msr = 0;
#endif
		wrtee(msr);
	} else if (ipl == IPL_NONE) {
		panic("%s: %p: cpl=%u: attempt to splraise(IPL_NONE)",
		    __func__, __builtin_return_address(0), old_ipl);
#if 0
	} else if (old_ipl > ipl) {
		printf("%s: %p: cpl=%u: ignoring splraise(%u) to lower ipl\n",
		    __func__, __builtin_return_address(0), old_ipl, ipl);
#endif
	}

	return old_ipl;
}

#ifdef __HAVE_FAST_SOFTINTS
static void
e500_softint_init_md(lwp_t *l, u_int si_level, uintptr_t *machdep_p)
{
	struct cpu_info * const ci = l->l_cpu;
	struct cpu_softc * const cpu = ci->ci_softc;

	*machdep_p = 1 << SOFTINT2IPL(si_level);
	KASSERT(*machdep_p & IPL_SOFTMASK);
	cpu->cpu_softlwps[si_level] = l;
}

static void
e500_softint_trigger(uintptr_t machdep)
{
	struct cpu_info * const ci = curcpu();

	atomic_or_uint(&ci->ci_data.cpu_softints, machdep);
	if (machdep == (1 << IPL_SOFTBIO))
		printf("%s(%u): cpl=%u\n", __func__, machdep, ci->ci_cpl);
}
#endif /* __HAVE_FAST_SOFTINTS */

static int
e500_intr_spurious(void *arg)
{
	return 0;
}

static bool
e500_intr_irq_info_get(struct cpu_info *ci, u_int irq, int ipl, int ist,
	struct e500_intr_irq_info *ii)
{
	const struct e500_intr_info * const info = &e500_intr_info;
	bool ok;

#if DEBUG > 2
	printf("%s(%p,irq=%u,ipl=%u,ist=%u,%p)\n", __func__, ci, irq, ipl, ist, ii);
#endif

	if (ipl < IPL_VM || ipl > IPL_HIGH) {
#if DEBUG > 2
		printf("%s:%d ipl=%u\n", __func__, __LINE__, ipl);
#endif
		return false;
	}

	if (ist <= IST_NONE || ist >= IST_MAX) {
#if DEBUG > 2
		printf("%s:%d ist=%u\n", __func__, __LINE__, ist);
#endif
		return false;
	}

	ii->irq_vector = irq + info->ii_ist_vectors[ist];
	if (IST_PERCPU_P(ist))
		ii->irq_vector += ci->ci_cpuid * info->ii_percpu_sources;

	switch (ist) {
	default:
		ii->irq_vpr = OPENPIC_EIVPR(irq);
		ii->irq_dr  = OPENPIC_EIDR(irq);
		ok = irq < info->ii_external_sources
		    && (ist == IST_EDGE
			|| ist == IST_LEVEL_LOW
			|| ist == IST_LEVEL_HIGH);
		break;
	case IST_ONCHIP:
		ii->irq_vpr = OPENPIC_IIVPR(irq);
		ii->irq_dr  = OPENPIC_IIDR(irq);
		ok = irq < 32 * __arraycount(info->ii_onchip_bitmap);
#if DEBUG > 2
		printf("%s: irq=%u: ok=%u\n", __func__, irq, ok);
#endif
		ok = ok && (info->ii_onchip_bitmap[irq/32] & (1 << (irq & 31)));
#if DEBUG > 2
		printf("%s: %08x%08x -> %08x%08x: ok=%u\n", __func__,
		    irq < 32 ? 0 : (1 << irq), irq < 32 ? (1 << irq) : 0,
		    info->ii_onchip_bitmap[1], info->ii_onchip_bitmap[0],
		    ok);
#endif
		break;
	case IST_MSIGROUP:
		ii->irq_vpr = OPENPIC_MSIVPR(irq);
		ii->irq_dr  = OPENPIC_MSIDR(irq);
		ok = irq < info->ii_msigroup_sources
		    && ipl == IPL_VM;
		break;
	case IST_TIMER:
		ii->irq_vpr = OPENPIC_GTVPR(ci->ci_cpuid, irq);
		ii->irq_dr  = OPENPIC_GTDR(ci->ci_cpuid, irq);
		ok = irq < info->ii_timer_sources;
#if DEBUG > 2
		printf("%s: IST_TIMER irq=%u: ok=%u\n", __func__, irq, ok);
#endif
		break;
	case IST_IPI:
		ii->irq_vpr = OPENPIC_IPIVPR(irq);
		ii->irq_dr  = OPENPIC_IPIDR(irq);
		ok = irq < info->ii_ipi_sources;
		break;
	case IST_MI:
		ii->irq_vpr = OPENPIC_MIVPR(irq);
		ii->irq_dr  = OPENPIC_MIDR(irq);
		ok = irq < info->ii_mi_sources;
		break;
	}

	return ok;
}

static const char *
e500_intr_string(int irq, int ist)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	struct e500_intr_irq_info ii;

	if (!e500_intr_irq_info_get(ci, irq, IPL_VM, ist, &ii))
		return NULL;

	return cpu->cpu_evcnt_intrs[ii.irq_vector].ev_name;
}

static void *
e500_intr_cpu_establish(struct cpu_info *ci, int irq, int ipl, int ist,
	int (*handler)(void *), void *arg)
{
	struct cpu_softc * const cpu = ci->ci_softc;
	struct e500_intr_irq_info ii;

	KASSERT(ipl >= IPL_VM && ipl <= IPL_HIGH);
	KASSERT(ist > IST_NONE && ist < IST_MAX && ist != IST_MSI);

	if (!e500_intr_irq_info_get(ci, irq, ipl, ist, &ii)) {
		printf("%s: e500_intr_irq_info_get(%p,%u,%u,%u,%p) failed\n",
		    __func__, ci, irq, ipl, ist, &ii);
		return NULL;
	}

	struct intr_source * const is = &e500_intr_sources[ii.irq_vector];
	mutex_enter(&e500_intr_lock);
	if (is->is_ipl != IPL_NONE)
		return NULL;

	is->is_func = handler;
	is->is_arg = arg;
	is->is_ipl = ipl;
	is->is_ist = ist;
	is->is_irq = irq;
	is->is_vpr = ii.irq_vpr;
	is->is_dr = ii.irq_dr;

	uint32_t vpr = VPR_PRIORITY_MAKE(IPL2CTPR(ipl))
	    | VPR_VECTOR_MAKE(((ii.irq_vector + 1) << 4) | ipl)
	    | (ist == IST_LEVEL_LOW
		? VPR_LEVEL_LOW
		: (ist == IST_LEVEL_HIGH
		    ? VPR_LEVEL_HIGH
		    : (ist == IST_ONCHIP
		      ? VPR_P_HIGH
		      : 0)));

	/*
	 * All interrupts go to the primary except per-cpu interrupts which get
	 * routed to the appropriate cpu.
	 */
	uint32_t dr = IST_PERCPU_P(ist) ? 1 << ci->ci_cpuid : 1;

	/*
	 * Update the vector/priority and destination registers keeping the
	 * interrupt masked.
	 */
	const register_t msr = wrtee(0);	/* disable interrupts */
	openpic_write(cpu, ii.irq_vpr, vpr | VPR_MSK);
	openpic_write(cpu, ii.irq_dr, dr);

	/*
	 * Now unmask the interrupt.
	 */
	openpic_write(cpu, ii.irq_vpr, vpr);

	wrtee(msr);				/* re-enable interrupts */

	mutex_exit(&e500_intr_lock);

	return is;
}

static void *
e500_intr_establish(int irq, int ipl, int ist,
	int (*handler)(void *), void *arg)
{
	return e500_intr_cpu_establish(curcpu(), irq, ipl, ist, handler, arg);
}

static void
e500_intr_disestablish(void *vis)
{
	struct cpu_softc * const cpu = curcpu()->ci_softc;
	struct intr_source * const is = vis;
	struct e500_intr_irq_info ii;

	KASSERT(e500_intr_sources <= is);
	KASSERT(is < e500_intr_last_source);
	KASSERT(!cpu_intr_p());

	bool ok = e500_intr_irq_info_get(curcpu(), is->is_irq, is->is_ipl,
	    is->is_ist, &ii);
	(void)ok;	/* appease gcc */
	KASSERT(ok);
	KASSERT(is - e500_intr_sources == ii.irq_vector);

	mutex_enter(&e500_intr_lock);
	/*
	 * Mask the source using the mask (MSK) bit in the vector/priority reg.
	 */
	uint32_t vpr = openpic_read(cpu, ii.irq_vpr);
	openpic_write(cpu, ii.irq_vpr, VPR_MSK | vpr);

	/*
	 * Wait for the Activity (A) bit for the source to be cleared.
	 */
	while (openpic_read(cpu, ii.irq_vpr) & VPR_A)
		;

	/*
	 * Now the source can be modified.
	 */
	openpic_write(cpu, ii.irq_dr, 0);		/* stop delivery */
	openpic_write(cpu, ii.irq_vpr, VPR_MSK);	/* mask/reset it */

	*is = (struct intr_source)INTR_SOURCE_INITIALIZER;

	mutex_exit(&e500_intr_lock);
}

static void
e500_critintr(struct trapframe *tf)
{
	panic("%s: srr0/srr1=%#lx/%#lx", __func__, tf->tf_srr0, tf->tf_srr1);
}

static void
e500_decrintr(struct trapframe *tf)
{
	panic("%s: srr0/srr1=%#lx/%#lx", __func__, tf->tf_srr0, tf->tf_srr1);
}

static void
e500_fitintr(struct trapframe *tf)
{
	panic("%s: srr0/srr1=%#lx/%#lx", __func__, tf->tf_srr0, tf->tf_srr1);
}

static void
e500_wdogintr(struct trapframe *tf)
{
	mtspr(SPR_TSR, TSR_ENW|TSR_WIS);
	panic("%s: tf=%p tb=%"PRId64" srr0/srr1=%#lx/%#lx", __func__, tf,
	    mftb(), tf->tf_srr0, tf->tf_srr1);
}

static void
e500_extintr(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	const int old_ipl = ci->ci_cpl;

	KASSERT(mfmsr() & PSL_CE);

#if 0
//	printf("%s(%p): idepth=%d enter\n", __func__, tf, ci->ci_idepth);
	if ((register_t)tf >= (register_t)curlwp->l_addr + USPACE              
	    || (register_t)tf < (register_t)curlwp->l_addr + NBPG) {            
		printf("%s(entry): pid %d.%d (%s): srr0/srr1=%#lx/%#lx: invalid tf addr %p\n",  
		    __func__, curlwp->l_proc->p_pid, curlwp->l_lid,
		    curlwp->l_proc->p_comm, tf->tf_srr0, tf->tf_srr1, tf);
	}       
#endif


	ci->ci_data.cpu_nintr++;
	tf->tf_cf.cf_idepth = ci->ci_idepth++;
	cpu->cpu_pcpls[ci->ci_idepth] = old_ipl;
#if 1
	if (mfmsr() & PSL_EE)
		panic("%s(%p): MSR[EE] is on (%#lx)!", __func__, tf, mfmsr());
	if (old_ipl == IPL_HIGH
	    || IPL2CTPR(old_ipl) != openpic_read(cpu, OPENPIC_CTPR))
		panic("%s(%p): old_ipl(%u) == IPL_HIGH(%u) "
		    "|| old_ipl + %u != OPENPIC_CTPR (%u)",
		    __func__, tf, old_ipl, IPL_HIGH,
		    15 - IPL_HIGH, openpic_read(cpu, OPENPIC_CTPR));
#else
	if (old_ipl >= IPL_VM)
		panic("%s(%p): old_ipl(%u) >= IPL_VM(%u) CTPR=%u",
		    __func__, tf, old_ipl, IPL_VM, openpic_read(cpu, OPENPIC_CTPR));
#endif

	for (;;) {
		/*
		 * Find out the pending interrupt.
		 */
	if (mfmsr() & PSL_EE)
		panic("%s(%p): MSR[EE] turned on (%#lx)!", __func__, tf, mfmsr());
		if (IPL2CTPR(old_ipl) != openpic_read(cpu, OPENPIC_CTPR))
			panic("%s(%p): %d: old_ipl(%u) + %u != OPENPIC_CTPR (%u)",
			    __func__, tf, __LINE__, old_ipl, 
			    15 - IPL_HIGH, openpic_read(cpu, OPENPIC_CTPR));
		const uint32_t iack = openpic_read(cpu, OPENPIC_IACK);
		const int ipl = iack & 0xf;
		const int irq = (iack >> 4) - 1;
#if 0
		printf("%s: iack=%d ipl=%d irq=%d <%s>\n",
		    __func__, iack, ipl, irq,
		    (iack != IRQ_SPURIOUS ?
			cpu->cpu_evcnt_intrs[irq].ev_name : "spurious"));
#endif
		if (IPL2CTPR(old_ipl) != openpic_read(cpu, OPENPIC_CTPR))
			panic("%s(%p): %d: old_ipl(%u) + %u != OPENPIC_CTPR (%u)",
			    __func__, tf, __LINE__, old_ipl, 
			    15 - IPL_HIGH, openpic_read(cpu, OPENPIC_CTPR));
		if (iack == IRQ_SPURIOUS)
			break;
		
		struct intr_source * const is = &e500_intr_sources[irq];
		if (__predict_true(is < e500_intr_last_source)) {
			/*
			 * Timer interrupts get their argument overriden with
			 * the pointer to the trapframe.
			 */
			KASSERT(is->is_ipl == ipl);
			void *arg = (is->is_ist == IST_TIMER ? tf : is->is_arg);
			if (is->is_ipl <= old_ipl)
				panic("%s(%p): %s (%u): is->is_ipl (%u) <= old_ipl (%u)\n",
				    __func__, tf,
				    cpu->cpu_evcnt_intrs[irq].ev_name, irq,
				    is->is_ipl, old_ipl);
			KASSERT(is->is_ipl > old_ipl);
			e500_splset(ci, is->is_ipl);	/* change IPL */
			if (__predict_false(is->is_func == NULL)) {
				aprint_error_dev(ci->ci_dev,
				    "interrupt from unestablished irq %d\n",
				    irq);
			} else {
				int (*func)(void *) = is->is_func;
				wrtee(PSL_EE);
				int rv = (*func)(arg);
				wrtee(0);
#if DEBUG > 2
				printf("%s: %s handler %p(%p) returned %d\n",
				    __func__,
				    cpu->cpu_evcnt_intrs[irq].ev_name,
				    func, arg, rv);
#endif
				if (rv == 0)
					cpu->cpu_evcnt_spurious_intr.ev_count++;
			}
			e500_splset(ci, old_ipl);	/* restore IPL */
			cpu->cpu_evcnt_intrs[irq].ev_count++;
		} else {
			aprint_error_dev(ci->ci_dev,
			    "interrupt from illegal irq %d\n", irq);
			cpu->cpu_evcnt_spurious_intr.ev_count++;
		}
		/*
		 * If this is a nested interrupt, simply ack it and exit
		 * because the loop we interrupted will complete looking
		 * for interrupts.
		 */
	if (mfmsr() & PSL_EE)
		panic("%s(%p): MSR[EE] left on (%#lx)!", __func__, tf, mfmsr());
		if (IPL2CTPR(old_ipl) != openpic_read(cpu, OPENPIC_CTPR))
			panic("%s(%p): %d: old_ipl(%u) + %u != OPENPIC_CTPR (%u)",
			    __func__, tf, __LINE__, old_ipl, 
			    15 - IPL_HIGH, openpic_read(cpu, OPENPIC_CTPR));

		openpic_write(cpu, OPENPIC_EOI, 0);
		if (IPL2CTPR(old_ipl) != openpic_read(cpu, OPENPIC_CTPR))
			panic("%s(%p): %d: old_ipl(%u) + %u != OPENPIC_CTPR (%u)",
			    __func__, tf, __LINE__, old_ipl, 
			    15 - IPL_HIGH, openpic_read(cpu, OPENPIC_CTPR));
		if (ci->ci_idepth > 0)
			break;
	}

	ci->ci_idepth--;

#ifdef __HAVE_FAST_SOFTINTS
	/*
	 * Before exiting, deal with any softints that need to be dealt with.
	 */
	const u_int softints = (ci->ci_data.cpu_softints << old_ipl) & IPL_SOFTMASK;
	if (__predict_false(softints != 0)) {
		KASSERT(old_ipl < IPL_VM);
		e500_splset(ci, IPL_HIGH);	/* pop to high */
		e500_softint(ci, cpu, old_ipl);	/* deal with them */
		e500_splset(ci, old_ipl);	/* and drop back */
	}
#endif /* __HAVE_FAST_SOFTINTS */
#if 1
	KASSERT(ci->ci_cpl == old_ipl);
#else
	e500_splset(ci, old_ipl);		/* and drop back */
#endif

//	printf("%s(%p): idepth=%d exit\n", __func__, tf, ci->ci_idepth);
}

static void
e500_intr_init(void)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	const uint32_t frr = openpic_read(cpu, OPENPIC_FRR);
	const u_int nirq = FRR_NIRQ_GET(frr) + 1;
//	const u_int ncpu = FRR_NCPU_GET(frr) + 1;
	struct intr_source *is;
	struct e500_intr_info * const ii = &e500_intr_info;

	const uint16_t svr = mfspr(SPR_SVR) >> 16;
	switch (svr) {
#ifdef MPC8536
	case SVR_MPC8536v1 >> 16:
		*ii = mpc8536_intr_info;
		break;
#endif
#ifdef MPC8544
	case SVR_MPC8544v1 >> 16:
		*ii = mpc8544_intr_info;
		break;
#endif
#ifdef MPC8548
	case SVR_MPC8543v1 >> 16:
	case SVR_MPC8548v1 >> 16:
		*ii = mpc8548_intr_info;
		break;
#endif
#ifdef MPC8555
	case SVR_MPC8541v1 >> 16:
	case SVR_MPC8555v1 >> 16:
		*ii = mpc8555_intr_info;
		break;
#endif
#ifdef MPC8568
	case SVR_MPC8568v1 >> 16:
		*ii = mpc8568_intr_info;
		break;
#endif
#ifdef MPC8572
	case SVR_MPC8572v1 >> 16:
		*ii = mpc8572_intr_info;
		break;
#endif
#ifdef P2020
	case SVR_P2010v2 >> 16:
	case SVR_P2020v2 >> 16:
		*ii = p20x0_intr_info;
		break;
#endif
	default:
		panic("%s: don't know how to deal with SVR %#lx",
		    __func__, mfspr(SPR_SVR));
	}

	/*
	 * We need to be in mixed mode.
	 */
	openpic_write(cpu, OPENPIC_GCR, GCR_M);

	/*
	 * Make we and the openpic both agree about the current SPL level.
	 */
	e500_splset(ci, ci->ci_cpl);

	/*
	 * Allow the required number of interrupt sources.
	 */
	is = kmem_zalloc(nirq * sizeof(*is), KM_SLEEP);
	KASSERT(is);
	e500_intr_sources = is;
	e500_intr_last_source = is + nirq;

	/*
	 * Initialize all the external interrupts as active low.
	 */
	for (u_int irq = 0; irq < e500_intr_info.ii_external_sources; irq++) { 
		openpic_write(cpu, OPENPIC_EIVPR(irq),
		    VPR_VECTOR_MAKE(irq) | VPR_LEVEL_LOW);
	}
}

static void
e500_intr_cpu_init(struct cpu_info *ci)
{
	struct cpu_softc * const cpu = ci->ci_softc;
	const char * const xname = device_xname(ci->ci_dev);

	const u_int32_t frr = openpic_read(cpu, OPENPIC_FRR);
	const u_int nirq = FRR_NIRQ_GET(frr) + 1;
//	const u_int ncpu = FRR_NCPU_GET(frr) + 1;

	const struct e500_intr_info * const info = &e500_intr_info;

	cpu->cpu_clock_gtbcr = OPENPIC_GTBCR(ci->ci_cpuid, E500_CLOCK_TIMER);

	cpu->cpu_evcnt_intrs =
	    kmem_zalloc(nirq * sizeof(cpu->cpu_evcnt_intrs[0]), KM_SLEEP);
	KASSERT(cpu->cpu_evcnt_intrs);

	struct evcnt *evcnt = cpu->cpu_evcnt_intrs;
	for (size_t j = 0; j < info->ii_external_sources; j++, evcnt++) {
		const char *name = e500_intr_external_name_lookup(j);
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR, NULL, xname, name);
	}
	KASSERT(evcnt == cpu->cpu_evcnt_intrs + info->ii_ist_vectors[IST_ONCHIP]);
	for (size_t j = 0; j < info->ii_onchip_sources; j++, evcnt++) {
		const char *name = e500_intr_onchip_name_lookup(j);
		if (name != NULL) {
			evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
			    NULL, xname, name);
		}
	}

	KASSERT(evcnt == cpu->cpu_evcnt_intrs + info->ii_ist_vectors[IST_MSIGROUP]);
	for (size_t j = 0; j < info->ii_msigroup_sources; j++, evcnt++) {
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
		    NULL, xname, e500_msigroup_intr_names[j].in_name);
	}

	KASSERT(evcnt == cpu->cpu_evcnt_intrs + info->ii_ist_vectors[IST_TIMER]);
	evcnt += ci->ci_cpuid * info->ii_percpu_sources;
	for (size_t j = 0; j < info->ii_timer_sources; j++, evcnt++) {
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
		    NULL, xname, e500_timer_intr_names[j].in_name);
	}

	for (size_t j = 0; j < info->ii_ipi_sources; j++, evcnt++) {
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
		    NULL, xname, e500_ipi_intr_names[j].in_name);
	}

	for (size_t j = 0; j < info->ii_mi_sources; j++, evcnt++) {
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
		    NULL, xname, e500_mi_intr_names[j].in_name);
	}

	/*
	 * Establish interrupt for this CPU.
	 */
	if (e500_intr_cpu_establish(ci, E500_CLOCK_TIMER, IPL_CLOCK, IST_TIMER,
	    e500_clock_intr, NULL) == NULL)
		panic("%s: failed to establish clock interrupt!", __func__);

	/*
	 * Enable watchdog interrupts.
	 */
	uint32_t tcr = mfspr(SPR_TCR);
	tcr |= TCR_WIE;
	mtspr(SPR_TCR, tcr);
}
