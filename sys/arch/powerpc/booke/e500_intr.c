/*	$NetBSD: e500_intr.c,v 1.21.2.3 2017/12/03 11:36:36 jdolecek Exp $	*/
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
#include "opt_multiprocessor.h"
#include "opt_ddb.h"

#define __INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: e500_intr.c,v 1.21.2.3 2017/12/03 11:36:36 jdolecek Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/xcall.h>
#include <sys/ipi.h>
#include <sys/bitops.h>
#include <sys/interrupt.h>

#include <uvm/uvm_extern.h>

#ifdef __HAVE_FAST_SOFTINTS
#include <powerpc/softint.h>
#endif

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/openpicreg.h>

#define	IPL2CTPR(ipl)		((ipl) + 15 - IPL_HIGH)
#define CTPR2IPL(ctpr)		((ctpr) - (15 - IPL_HIGH))

#define	IST_PERCPU_P(ist)	((ist) >= IST_TIMER)

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
	uint8_t is_refcnt;
	bus_size_t is_vpr;
	bus_size_t is_dr;
	char is_source[INTRIDBUF];
	char is_xname[INTRDEVNAMEBUF];
};

#define	INTR_SOURCE_INITIALIZER \
	{ .is_func = e500_intr_spurious, .is_arg = NULL, \
	.is_irq = -1, .is_ipl = IPL_NONE, .is_ist = IST_NONE, \
	.is_source = "", .is_xname = "", }

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
	{ ISOURCE_SRIO_IMU2, "srio-imu2" },
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
		[IST_PULSE]		= 0,				\
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

#ifdef P1025
#define	p1025_external_intr_names	default_external_intr_names
const struct e500_intr_name p1025_onchip_intr_names[] = {
	{ ISOURCE_PCIEX3_MPC8572, "pcie3" },
	{ ISOURCE_ETSEC1_G1_TX, "etsec1-g1-tx" },
	{ ISOURCE_ETSEC1_G1_RX, "etsec1-g1-rx" },
	{ ISOURCE_ETSEC1_G1_ERR, "etsec1-g1-error" },
	{ ISOURCE_ETSEC2_G1_TX, "etsec2-g1-tx" },
	{ ISOURCE_ETSEC2_G1_RX, "etsec2-g1-rx" },
	{ ISOURCE_ETSEC2_G1_ERR, "etsec2-g1-error" },
	{ ISOURCE_ETSEC3_G1_TX, "etsec3-g1-tx" },
	{ ISOURCE_ETSEC3_G1_RX, "etsec3-g1-rx" },
	{ ISOURCE_ETSEC3_G1_ERR, "etsec3-g1-error" },
	{ ISOURCE_QEB_MUECC, "qeb-low" },
	{ ISOURCE_QEB_HIGH, "qeb-crit" },
	{ ISOURCE_DMA2_CHAN1, "dma2-chan1" },
	{ ISOURCE_DMA2_CHAN2, "dma2-chan2" },
	{ ISOURCE_DMA2_CHAN3, "dma2-chan3" },
	{ ISOURCE_DMA2_CHAN4, "dma2-chan4" },
	{ 0, "" },
};

INTR_INFO_DECL(p1025, P1025);
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

#ifdef P1023
#define	p1023_external_intr_names	default_external_intr_names
const struct e500_intr_name p1023_onchip_intr_names[] = {
	{ ISOURCE_FMAN,            "fman" },
	{ ISOURCE_MDIO,            "mdio" },
	{ ISOURCE_QMAN0,           "qman0" },
	{ ISOURCE_BMAN0,           "bman0" },
	{ ISOURCE_QMAN1,           "qman1" },
	{ ISOURCE_BMAN1,           "bman1" },
	{ ISOURCE_QMAN2,           "qman2" },
	{ ISOURCE_BMAN2,           "bman2" },
	{ ISOURCE_SECURITY2_P1023, "sec2" },
	{ ISOURCE_SEC_GENERAL,     "sec-general" },
	{ ISOURCE_DMA2_CHAN1,      "dma2-chan1" },
	{ ISOURCE_DMA2_CHAN2,      "dma2-chan2" },
	{ ISOURCE_DMA2_CHAN3,      "dma2-chan3" },
	{ ISOURCE_DMA2_CHAN4,      "dma2-chan4" },
	{ 0, "" },
};

INTR_INFO_DECL(p1023, P1023);
#endif

static const char ist_names[][12] = {
	[IST_NONE] = "none",
	[IST_EDGE] = "edge",
	[IST_LEVEL_LOW] = "level-",
	[IST_LEVEL_HIGH] = "level+",
	[IST_PULSE] = "pulse",
	[IST_MSI] = "msi",
	[IST_ONCHIP] = "onchip",
	[IST_MSIGROUP] = "msigroup",
	[IST_TIMER] = "timer",
	[IST_IPI] = "ipi",
	[IST_MI] = "msgint",
};

static struct intr_source *e500_intr_sources;
static const struct intr_source *e500_intr_last_source;

static void 	*e500_intr_establish(int, int, int, int (*)(void *), void *,
		    const char *);
static void 	e500_intr_disestablish(void *);
static void 	e500_intr_cpu_attach(struct cpu_info *ci);
static void 	e500_intr_cpu_hatch(struct cpu_info *ci);
static void	e500_intr_cpu_send_ipi(cpuid_t, uintptr_t);
static void 	e500_intr_init(void);
static void 	e500_intr_init_precpu(void);
static const char *e500_intr_string(int, int, char *, size_t);
static const char *e500_intr_typename(int);
static void 	e500_critintr(struct trapframe *tf);
static void 	e500_decrintr(struct trapframe *tf);
static void 	e500_extintr(struct trapframe *tf);
static void 	e500_fitintr(struct trapframe *tf);
static void 	e500_wdogintr(struct trapframe *tf);
static void	e500_spl0(void);
static int 	e500_splraise(int);
static void 	e500_splx(int);
static const char *e500_intr_all_name_lookup(int, int);

const struct intrsw e500_intrsw = {
	.intrsw_establish = e500_intr_establish,
	.intrsw_disestablish = e500_intr_disestablish,
	.intrsw_init = e500_intr_init,
	.intrsw_cpu_attach = e500_intr_cpu_attach,
	.intrsw_cpu_hatch = e500_intr_cpu_hatch,
	.intrsw_cpu_send_ipi = e500_intr_cpu_send_ipi,
	.intrsw_string = e500_intr_string,
	.intrsw_typename = e500_intr_typename,

	.intrsw_critintr = e500_critintr,
	.intrsw_decrintr = e500_decrintr,
	.intrsw_extintr = e500_extintr,
	.intrsw_fitintr = e500_fitintr,
	.intrsw_wdogintr = e500_wdogintr,

	.intrsw_splraise = e500_splraise,
	.intrsw_splx = e500_splx,
	.intrsw_spl0 = e500_spl0,

#ifdef __HAVE_FAST_SOFTINTS
	.intrsw_softint_init_md = powerpc_softint_init_md,
	.intrsw_softint_trigger = powerpc_softint_trigger,
#endif
};

static bool wdog_barked;

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

	name = e500_intr_name_lookup(e500_intr_info.ii_onchip_intr_names, irq);
	if (name == NULL)
	       name = e500_intr_name_lookup(e500_onchip_intr_names, irq);

	return name;
}

static inline void
e500_splset(struct cpu_info *ci, int ipl)
{
	struct cpu_softc * const cpu = ci->ci_softc;

	KASSERT((curlwp->l_pflag & LP_INTR) == 0 || ipl != IPL_NONE);
	const u_int ctpr = IPL2CTPR(ipl);
	KASSERT(openpic_read(cpu, OPENPIC_CTPR) == IPL2CTPR(ci->ci_cpl));
	openpic_write(cpu, OPENPIC_CTPR, ctpr);
	KASSERT(openpic_read(cpu, OPENPIC_CTPR) == ctpr);
#ifdef DIAGNOSTIC
	cpu->cpu_spl_tb[ipl][ci->ci_cpl] = mftb();
#endif
	ci->ci_cpl = ipl;
}

static void
e500_spl0(void)
{
	wrtee(0);

	struct cpu_info * const ci = curcpu();

#ifdef __HAVE_FAST_SOFTINTS
	if (__predict_false(ci->ci_data.cpu_softints != 0)) {
		e500_splset(ci, IPL_HIGH);
		wrtee(PSL_EE);
		powerpc_softint(ci, IPL_NONE,
		    (vaddr_t)__builtin_return_address(0));
		wrtee(0);
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

	/* if we paniced because of watchdog, PSL_CE will be clear.  */
	KASSERT(wdog_barked || (mfmsr() & PSL_CE));

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
	const u_int softints = ci->ci_data.cpu_softints & (IPL_SOFTMASK << ipl);
	if (__predict_false(softints != 0)) {
		e500_splset(ci, IPL_HIGH);
		wrtee(msr);
		powerpc_softint(ci, ipl,
		    (vaddr_t)__builtin_return_address(0));
		wrtee(0);
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

	/* if we paniced because of watchdog, PSL_CE will be clear.  */
	KASSERT(wdog_barked || (mfmsr() & PSL_CE));

	if (old_ipl < ipl) {
		//const
		register_t msr = wrtee(0);
		e500_splset(ci, ipl);
#if 0
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
	if (IST_PERCPU_P(ist) && ist != IST_IPI)
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
	case IST_PULSE:
		ok = false;
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
e500_intr_string(int irq, int ist, char *buf, size_t len)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	struct e500_intr_irq_info ii;

	if (!e500_intr_irq_info_get(ci, irq, IPL_VM, ist, &ii))
		return NULL;

	strlcpy(buf, cpu->cpu_evcnt_intrs[ii.irq_vector].ev_name, len);
	return buf;
}

__CTASSERT(__arraycount(ist_names) == IST_MAX);

static const char *
e500_intr_typename(int ist)
{
	if (IST_NONE <= ist && ist < IST_MAX)
		return ist_names[ist];

	return NULL;
}

static void *
e500_intr_cpu_establish(struct cpu_info *ci, int irq, int ipl, int ist,
	int (*handler)(void *), void *arg, const char *xname)
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

	if (xname == NULL) {
		xname = e500_intr_all_name_lookup(irq, ist);
		if (xname == NULL)
			xname = "unknown";
	}

	struct intr_source * const is = &e500_intr_sources[ii.irq_vector];
	mutex_enter(&e500_intr_lock);
	if (is->is_ipl != IPL_NONE) {
		/* XXX IPI0 is shared by all CPU. */
		if (is->is_ist != IST_IPI ||
		    is->is_irq != irq ||
		    is->is_ipl != ipl ||
		    is->is_ist != ist ||
		    is->is_func != handler ||
		    is->is_arg != arg) {
			mutex_exit(&e500_intr_lock);
			return NULL;
		}
	}

	is->is_func = handler;
	is->is_arg = arg;
	is->is_ipl = ipl;
	is->is_ist = ist;
	is->is_irq = irq;
	is->is_refcnt++;
	is->is_vpr = ii.irq_vpr;
	is->is_dr = ii.irq_dr;
	switch (ist) {
	case IST_EDGE:
	case IST_LEVEL_LOW:
	case IST_LEVEL_HIGH:
		snprintf(is->is_source, sizeof(is->is_source), "extirq %d",
		    irq);
		break;
	case IST_ONCHIP:
		snprintf(is->is_source, sizeof(is->is_source), "irq %d", irq);
		break;
	case IST_MSIGROUP:
		snprintf(is->is_source, sizeof(is->is_source), "msigroup %d",
		    irq);
		break;
	case IST_TIMER:
		snprintf(is->is_source, sizeof(is->is_source), "timer %d", irq);
		break;
	case IST_IPI:
		snprintf(is->is_source, sizeof(is->is_source), "ipi %d", irq);
		break;
	case IST_MI:
		snprintf(is->is_source, sizeof(is->is_source), "mi %d", irq);
		break;
	case IST_PULSE:
	default:
		panic("%s: invalid ist (%d)\n", __func__, ist);
	}
	strlcpy(is->is_xname, xname, sizeof(is->is_xname));

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
	uint32_t dr = openpic_read(cpu, ii.irq_dr);

	dr |= 1 << (IST_PERCPU_P(ist) ? ci->ci_cpuid : 0);

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
e500_intr_establish(int irq, int ipl, int ist, int (*handler)(void *),
    void *arg, const char *xname)
{
	return e500_intr_cpu_establish(curcpu(), irq, ipl, ist, handler, arg,
	    xname);
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

	if (is->is_refcnt-- > 1) {
		mutex_exit(&e500_intr_lock);
		return;
	}

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
	struct cpu_info * const ci = curcpu();
	mtspr(SPR_TSR, TSR_ENW|TSR_WIS);
	wdog_barked = true;
	dump_splhist(ci, NULL);
	dump_trapframe(tf, NULL);
	panic("%s: tf=%p tb=%"PRId64" srr0/srr1=%#lx/%#lx"
	    " cpl=%d idepth=%d, mtxcount=%d",
	    __func__, tf, mftb(), tf->tf_srr0, tf->tf_srr1,
	    ci->ci_cpl, ci->ci_idepth, ci->ci_mtx_count);
}

static void
e500_extintr(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	const int old_ipl = ci->ci_cpl;

	/* if we paniced because of watchdog, PSL_CE will be clear.  */
	KASSERT(wdog_barked || (mfmsr() & PSL_CE));

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
		KASSERTMSG((mfmsr() & PSL_EE) == 0,
		    "%s(%p): MSR[EE] left on (%#lx)!", __func__, tf, mfmsr());
		if (IPL2CTPR(old_ipl) != openpic_read(cpu, OPENPIC_CTPR))
			panic("%s(%p): %d: old_ipl(%u) + %u != OPENPIC_CTPR (%u)",
			    __func__, tf, __LINE__, old_ipl, 
			    15 - IPL_HIGH, openpic_read(cpu, OPENPIC_CTPR));
		const uint32_t iack = openpic_read(cpu, OPENPIC_IACK);
#ifdef DIAGNOSTIC
		const int ipl = iack & 0xf;
#endif
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
			KASSERTMSG(is->is_ipl == ipl,
			    "iack %#x: is %p: irq %d ipl %d != iack ipl %d",
			    iack, is, irq, is->is_ipl, ipl);
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
		KASSERTMSG((mfmsr() & PSL_EE) == 0,
		    "%s(%p): MSR[EE] left on (%#lx)!", __func__, tf, mfmsr());
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
	const u_int softints = ci->ci_data.cpu_softints & (IPL_SOFTMASK << old_ipl);
	if (__predict_false(softints != 0)) {
		KASSERT(old_ipl < IPL_VM);
		e500_splset(ci, IPL_HIGH);	/* pop to high */
		wrtee(PSL_EE);			/* reenable interrupts */
		powerpc_softint(ci, old_ipl,	/* deal with them */
		    tf->tf_srr0);
		wrtee(0);			/* disable interrupts */
		e500_splset(ci, old_ipl);	/* and drop back */
	}
#endif /* __HAVE_FAST_SOFTINTS */
	KASSERT(ci->ci_cpl == old_ipl);

	/*
	 * If we interrupted while power-saving and we need to exit idle,
	 * we need to clear PSL_POW so we won't go back into power-saving.
	 */
	if (__predict_false(tf->tf_srr1 & PSL_POW) && ci->ci_want_resched)
		tf->tf_srr1 &= ~PSL_POW;

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

	const uint16_t svr = (mfspr(SPR_SVR) & ~0x80000) >> 16;
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
#ifdef P1023
	case SVR_P1017v1 >> 16:
	case SVR_P1023v1 >> 16:
		*ii = p1023_intr_info;
		break;
#endif
#ifdef P1025
	case SVR_P1016v1 >> 16:
	case SVR_P1025v1 >> 16:
		*ii = p1025_intr_info;
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
	 * Initialize interrupt handler lock
	 */
	mutex_init(&e500_intr_lock, MUTEX_DEFAULT, IPL_HIGH);

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
e500_intr_init_precpu(void)
{
	struct cpu_info const *ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	bus_addr_t dr;

	/*
	 * timer's DR is set to be delivered to cpu0 as initial value.
	 */
	for (u_int irq = 0; irq < e500_intr_info.ii_timer_sources; irq++) { 
		dr = OPENPIC_GTDR(ci->ci_cpuid, irq);
		openpic_write(cpu, dr, 0);	/* stop delivery */
	}
}

static void
e500_idlespin(void)
{
	KASSERTMSG(curcpu()->ci_cpl == IPL_NONE,
	    "%s: cpu%u: ci_cpl (%d) != 0", __func__, cpu_number(),
	     curcpu()->ci_cpl);
	KASSERTMSG(CTPR2IPL(openpic_read(curcpu()->ci_softc, OPENPIC_CTPR)) == IPL_NONE,
	    "%s: cpu%u: CTPR (%d) != IPL_NONE", __func__, cpu_number(),
	     CTPR2IPL(openpic_read(curcpu()->ci_softc, OPENPIC_CTPR)));
	KASSERT(mfmsr() & PSL_EE);

	if (powersave > 0)
		mtmsr(mfmsr() | PSL_POW);
}

static void
e500_intr_cpu_attach(struct cpu_info *ci)
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

	struct evcnt *evcnt = cpu->cpu_evcnt_intrs;
	for (size_t j = 0; j < info->ii_external_sources; j++, evcnt++) {
		const char *name = e500_intr_external_name_lookup(j);
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR, NULL, xname, name);
	}
	KASSERT(evcnt == cpu->cpu_evcnt_intrs + info->ii_ist_vectors[IST_ONCHIP]);
	for (size_t j = 0; j < info->ii_onchip_sources; j++, evcnt++) {
		if (info->ii_onchip_bitmap[j / 32] & __BIT(j & 31)) {
			const char *name = e500_intr_onchip_name_lookup(j);
			if (name != NULL) {
				evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
				    NULL, xname, name);
#ifdef DIAGNOSTIC
			} else {
				printf("%s: missing evcnt for onchip irq %zu\n",
				    __func__, j);
#endif
			}
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

	ci->ci_idlespin = e500_idlespin;
}

static void
e500_intr_cpu_send_ipi(cpuid_t target, uint32_t ipimsg)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	uint32_t dstmask;

	if (target >= CPU_MAXNUM) {
		CPU_INFO_ITERATOR cii;
		struct cpu_info *dst_ci;

		KASSERT(target == IPI_DST_NOTME || target == IPI_DST_ALL);

		dstmask = 0;
		for (CPU_INFO_FOREACH(cii, dst_ci)) {
			if (target == IPI_DST_ALL || ci != dst_ci) {
				dstmask |= 1 << cpu_index(ci);
				if (ipimsg)
					atomic_or_32(&dst_ci->ci_pending_ipis,
					    ipimsg);
			}
		}
	} else {
		struct cpu_info * const dst_ci = cpu_lookup(target);
		KASSERT(dst_ci != NULL);
		KASSERTMSG(target == cpu_index(dst_ci),
		    "%s: target (%lu) != cpu_index(cpu%u)",
		     __func__, target, cpu_index(dst_ci));
		dstmask = (1 << target);
		if (ipimsg)
			atomic_or_32(&dst_ci->ci_pending_ipis, ipimsg);
	}

	openpic_write(cpu, OPENPIC_IPIDR(0), dstmask);
}

typedef void (*ipifunc_t)(void);

#ifdef __HAVE_PREEMPTION
static void
e500_ipi_kpreempt(void)
{
	poowerpc_softint_trigger(1 << IPL_NONE);
}
#endif

static void
e500_ipi_suspend(void)
{

#ifdef MULTIPROCESSOR
	cpu_pause(NULL);
#endif	/* MULTIPROCESSOR */
}

static const ipifunc_t e500_ipifuncs[] = {
	[ilog2(IPI_XCALL)] =	xc_ipi_handler,
	[ilog2(IPI_GENERIC)] =	ipi_cpu_handler,
	[ilog2(IPI_HALT)] =	e500_ipi_halt,
#ifdef __HAVE_PREEMPTION
	[ilog2(IPI_KPREEMPT)] =	e500_ipi_kpreempt,
#endif
	[ilog2(IPI_TLB1SYNC)] =	e500_tlb1_sync,
	[ilog2(IPI_SUSPEND)] =	e500_ipi_suspend,
};

static int
e500_ipi_intr(void *v)
{
	struct cpu_info * const ci = curcpu();

	ci->ci_ev_ipi.ev_count++;

	uint32_t pending_ipis = atomic_swap_32(&ci->ci_pending_ipis, 0);
	for (u_int ipi = 31; pending_ipis != 0; ipi--, pending_ipis <<= 1) {
		const u_int bits = __builtin_clz(pending_ipis);
		ipi -= bits;
		pending_ipis <<= bits;
		KASSERT(e500_ipifuncs[ipi] != NULL);
		(*e500_ipifuncs[ipi])();
	}
	
	return 1;
}

static void
e500_intr_cpu_hatch(struct cpu_info *ci)
{
	char iname[INTRIDBUF];

	/* Initialize percpu interupts. */
	e500_intr_init_precpu();

	/*
	 * Establish clock interrupt for this CPU.
	 */
	snprintf(iname, sizeof(iname), "%s clock", device_xname(ci->ci_dev));
	if (e500_intr_cpu_establish(ci, E500_CLOCK_TIMER, IPL_CLOCK, IST_TIMER,
	    e500_clock_intr, NULL, iname) == NULL)
		panic("%s: failed to establish clock interrupt!", __func__);

	/*
	 * Establish the IPI interrupts for this CPU.
	 */
	if (e500_intr_cpu_establish(ci, 0, IPL_VM, IST_IPI, e500_ipi_intr,
	    NULL, "ipi") == NULL)
		panic("%s: failed to establish ipi interrupt!", __func__);

	/*
	 * Enable watchdog interrupts.
	 */
	uint32_t tcr = mfspr(SPR_TCR);
	tcr |= TCR_WIE;
	mtspr(SPR_TCR, tcr);
}

static const char *
e500_intr_all_name_lookup(int irq, int ist)
{
	const struct e500_intr_info * const info = &e500_intr_info;

	switch (ist) {
	default:
		if (irq < info->ii_external_sources &&
		    (ist == IST_EDGE ||
		     ist == IST_LEVEL_LOW ||
		     ist == IST_LEVEL_HIGH))
			return e500_intr_name_lookup(
			    info->ii_external_intr_names, irq);
		break;

	case IST_PULSE:
		break;

	case IST_ONCHIP:
		if (irq < info->ii_onchip_sources)
			return e500_intr_onchip_name_lookup(irq);
		break;

	case IST_MSIGROUP:
		if (irq < info->ii_msigroup_sources)
			return e500_intr_name_lookup(e500_msigroup_intr_names,
			    irq);
		break;

	case IST_TIMER:
		if (irq < info->ii_timer_sources)
			return e500_intr_name_lookup(e500_timer_intr_names,
			    irq);
		break;

	case IST_IPI:
		if (irq < info->ii_ipi_sources)
			return e500_intr_name_lookup(e500_ipi_intr_names, irq);
		break;

	case IST_MI:
		if (irq < info->ii_mi_sources)
			return e500_intr_name_lookup(e500_mi_intr_names, irq);
		break;
	}

	return NULL;
}

static void
e500_intr_get_affinity(struct intr_source *is, kcpuset_t *cpuset)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	struct e500_intr_irq_info ii;

	kcpuset_zero(cpuset);

	if (is->is_ipl != IPL_NONE && !IST_PERCPU_P(is->is_ist)) {
		if (e500_intr_irq_info_get(ci, is->is_irq, is->is_ipl,
		    is->is_ist, &ii)) {
			uint32_t dr = openpic_read(cpu, ii.irq_dr);
			while (dr != 0) {
				u_int n = ffs(dr);
				if (n-- == 0)
					break;
				dr &= ~(1 << n);
				kcpuset_set(cpuset, n);
			}
		}
	}
}

static int
e500_intr_set_affinity(struct intr_source *is, const kcpuset_t *cpuset)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	struct e500_intr_irq_info ii;
	uint32_t ecpuset, tcpuset;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(mutex_owned(&e500_intr_lock));
	KASSERT(!kcpuset_iszero(cpuset));

	kcpuset_export_u32(cpuset, &ecpuset, sizeof(ecpuset));
	tcpuset = ecpuset;
	while (tcpuset != 0) {
		u_int cpu_idx = ffs(tcpuset);
		if (cpu_idx-- == 0)
			break;

		tcpuset &= ~(1 << cpu_idx);
		struct cpu_info * const newci = cpu_lookup(cpu_idx);
		if (newci == NULL)
			return EINVAL;
		if ((newci->ci_schedstate.spc_flags & SPCF_NOINTR) != 0)
			return EINVAL;
	}

	if (!e500_intr_irq_info_get(ci, is->is_irq, is->is_ipl, is->is_ist,
	    &ii))
		return ENXIO;

	/*
	 * Update the vector/priority and destination registers keeping the
	 * interrupt masked.
	 */
	const register_t msr = wrtee(0);	/* disable interrupts */

	uint32_t vpr = openpic_read(cpu, ii.irq_vpr);
	openpic_write(cpu, ii.irq_vpr, vpr | VPR_MSK);

	/*
	 * Wait for the Activity (A) bit for the source to be cleared.
	 */
	while (openpic_read(cpu, ii.irq_vpr) & VPR_A)
		continue;

	/*
	 * Update destination register
	 */
	openpic_write(cpu, ii.irq_dr, ecpuset);

	/*
	 * Now unmask the interrupt.
	 */
	openpic_write(cpu, ii.irq_vpr, vpr);

	wrtee(msr);				/* re-enable interrupts */

	return 0;
}

static bool
e500_intr_is_affinity_intrsource(struct intr_source *is,
    const kcpuset_t *cpuset)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	struct e500_intr_irq_info ii;
	bool result = false;

	if (is->is_ipl != IPL_NONE && !IST_PERCPU_P(is->is_ist)) {
		if (e500_intr_irq_info_get(ci, is->is_irq, is->is_ipl,
		    is->is_ist, &ii)) {
			uint32_t dr = openpic_read(cpu, ii.irq_dr);
			while (dr != 0 && !result) {
				u_int n = ffs(dr);
				if (n-- == 0)
					break;
				dr &= ~(1 << n);
				result = kcpuset_isset(cpuset, n);
			}
		}
	}
	return result;
}

static struct intr_source *
e500_intr_get_source(const char *intrid)
{
	struct intr_source *is;

	mutex_enter(&e500_intr_lock);
	for (is = e500_intr_sources; is < e500_intr_last_source; ++is) {
		if (is->is_source[0] == '\0')
			continue;

		if (!strncmp(intrid, is->is_source, sizeof(is->is_source) - 1))
			break;
	}
	if (is == e500_intr_last_source)
		is = NULL;
	mutex_exit(&e500_intr_lock);
	return is;
}

uint64_t
interrupt_get_count(const char *intrid, u_int cpu_idx)
{
	struct cpu_info * const ci = cpu_lookup(cpu_idx);
	struct cpu_softc * const cpu = ci->ci_softc;
	struct intr_source *is;
	struct e500_intr_irq_info ii;

	is = e500_intr_get_source(intrid);
	if (is == NULL)
		return 0;

	if (e500_intr_irq_info_get(ci, is->is_irq, is->is_ipl, is->is_ist, &ii))
		return cpu->cpu_evcnt_intrs[ii.irq_vector].ev_count;
	return 0;
}

void
interrupt_get_assigned(const char *intrid, kcpuset_t *cpuset)
{
	struct intr_source *is;

	kcpuset_zero(cpuset);

	is = e500_intr_get_source(intrid);
	if (is == NULL)
		return;

	mutex_enter(&e500_intr_lock);
	e500_intr_get_affinity(is, cpuset);
	mutex_exit(&e500_intr_lock);
}

void
interrupt_get_available(kcpuset_t *cpuset)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	kcpuset_zero(cpuset);

	mutex_enter(&cpu_lock);
	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((ci->ci_schedstate.spc_flags & SPCF_NOINTR) == 0)
			kcpuset_set(cpuset, cpu_index(ci));
	}
	mutex_exit(&cpu_lock);
}

void
interrupt_get_devname(const char *intrid, char *buf, size_t len)
{
	struct intr_source *is;

	if (len == 0)
		return;

	buf[0] = '\0';

	is = e500_intr_get_source(intrid);
	if (is != NULL)
		strlcpy(buf, is->is_xname, len);
}

struct intrids_handler *
interrupt_construct_intrids(const kcpuset_t *cpuset)
{
	struct intr_source *is;
	struct intrids_handler *ii_handler;
	intrid_t *ids;
	int i, n;

	if (kcpuset_iszero(cpuset))
		return NULL;

	n = 0;
	mutex_enter(&e500_intr_lock);
	for (is = e500_intr_sources; is < e500_intr_last_source; ++is) {
		if (e500_intr_is_affinity_intrsource(is, cpuset))
			++n;
	}
	mutex_exit(&e500_intr_lock);

	const size_t alloc_size = sizeof(int) + sizeof(intrid_t) * n;
	ii_handler = kmem_zalloc(alloc_size, KM_SLEEP);
	ii_handler->iih_nids = n;
	if (n == 0)
		return ii_handler;

	ids = ii_handler->iih_intrids;
	mutex_enter(&e500_intr_lock);
	for (i = 0, is = e500_intr_sources;
	     i < n && is < e500_intr_last_source;
	     ++is) {
		if (!e500_intr_is_affinity_intrsource(is, cpuset))
			continue;

		if (is->is_source[0] != '\0') {
			strlcpy(ids[i], is->is_source, sizeof(ids[0]));
			++i;
		}
	}
	mutex_exit(&e500_intr_lock);

	return ii_handler;
}

void
interrupt_destruct_intrids(struct intrids_handler *ii_handler)
{
	size_t iih_size;

	if (ii_handler == NULL)
		return;

	iih_size = sizeof(int) + sizeof(intrid_t) * ii_handler->iih_nids;
	kmem_free(ii_handler, iih_size);
}

static int
interrupt_distribute_locked(struct intr_source *is, const kcpuset_t *newset,
    kcpuset_t *oldset)
{
	int error;

	KASSERT(mutex_owned(&cpu_lock));

	if (is->is_ipl == IPL_NONE || IST_PERCPU_P(is->is_ist))
		return EINVAL;

	mutex_enter(&e500_intr_lock);
	if (oldset != NULL)
		e500_intr_get_affinity(is, oldset);
	error = e500_intr_set_affinity(is, newset);
	mutex_exit(&e500_intr_lock);

	return error;
}

int
interrupt_distribute(void *ich, const kcpuset_t *newset, kcpuset_t *oldset)
{
	int error;

	mutex_enter(&cpu_lock);
	error = interrupt_distribute_locked(ich, newset, oldset);
	mutex_exit(&cpu_lock);

	return error;
}

int
interrupt_distribute_handler(const char *intrid, const kcpuset_t *newset,
    kcpuset_t *oldset)
{
	struct intr_source *is;
	int error;

	is = e500_intr_get_source(intrid);
	if (is != NULL) {
		mutex_enter(&cpu_lock);
		error = interrupt_distribute_locked(is, newset, oldset);
		mutex_exit(&cpu_lock);
	} else
		error = ENOENT;

	return error;
}
