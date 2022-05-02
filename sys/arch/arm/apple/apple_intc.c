/* $NetBSD: apple_intc.c,v 1.8 2022/05/02 04:39:29 ryo Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apple_intc.c,v 1.8 2022/05/02 04:39:29 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/atomic.h>

#include <dev/fdt/fdtvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/armreg.h>
#include <arm/locore.h>
#include <arm/pic/picvar.h>
#include <arm/fdt/arm_fdtvar.h>

/*
 * AIC registers
 */
#define	AIC_INFO		0x0004
#define	 AIC_INFO_NIRQ		__BITS(15,0)
#define	AIC_WHOAMI		0x2000
#define	AIC_EVENT		0x2004
#define	 AIC_EVENT_TYPE		__BITS(31,16)
#define	  AIC_EVENT_TYPE_NONE	0
#define	  AIC_EVENT_TYPE_IRQ	1
#define	  AIC_EVENT_TYPE_IPI	4
#define	 AIC_EVENT_DATA		__BITS(15,0)
#define	 AIC_EVENT_IPI_OTHER	1
#define	AIC_IPI_SEND		0x2008
#define	AIC_IPI_ACK		0x200c
#define	AIC_IPI_MASK_CLR	0x2028
#define	AIC_IPI_OTHER		__BIT(0)
#define	AIC_AFFINITY(irqno)	(0x3000 + (irqno) * 4)
#define	AIC_SW_SET(irqno)	(0x4000 + (irqno) / 32 * 4)
#define	AIC_SW_CLR(irqno)	(0x4080 + (irqno) / 32 * 4)
#define	AIC_MASK_SET(irqno)	(0x4100 + (irqno) / 32 * 4)
#define	AIC_MASK_CLR(irqno)	(0x4180 + (irqno) / 32 * 4)
#define	 AIC_MASK_BIT(irqno)	__BIT((irqno) & 0x1f)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,aic" },
	DEVICE_COMPAT_EOL
};

struct apple_intc_softc;

struct apple_intc_percpu {
	struct apple_intc_softc *pc_sc;
	u_int pc_cpuid;
	u_int pc_ipimask;

	struct pic_softc pc_pic;
};

#define	LOCALPIC_SOURCE_TIMER	0
#define	LOCALPIC_SOURCE_IPI	1

struct apple_intc_softc {
	device_t sc_dev;		/* device handle */
	bus_space_tag_t sc_bst;		/* mmio tag */
	bus_space_handle_t sc_bsh;	/* mmio handle */
	u_int sc_nirq;			/* number of supported IRQs */
	u_int *sc_cpuid;		/* map of cpu index to AIC CPU ID */
	struct apple_intc_percpu *sc_pc; /* per-CPU data for timer and IPIs */

	struct pic_softc sc_pic;
};

static struct apple_intc_softc *intc_softc;

#define	PICTOSOFTC(pic) container_of(pic, struct apple_intc_softc, sc_pic)
#define	PICTOPERCPU(pic) container_of(pic, struct apple_intc_percpu, pc_pic)

#define AIC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	AIC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
apple_intc_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct apple_intc_softc * const sc = PICTOSOFTC(pic);

	AIC_WRITE(sc, AIC_SW_SET(irqbase), mask);
	AIC_WRITE(sc, AIC_MASK_CLR(irqbase), mask);
}

static void
apple_intc_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
}

static void
apple_intc_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct apple_intc_softc * const sc = PICTOSOFTC(pic);

	KASSERT(is->is_type == IST_LEVEL);

	/* Route to primary PE by default */
	AIC_WRITE(sc, AIC_AFFINITY(is->is_irq), __BIT(0));
	AIC_WRITE(sc, AIC_MASK_CLR(is->is_irq),
	    AIC_MASK_BIT(is->is_irq));
}

static void
apple_intc_set_priority(struct pic_softc *pic, int ipl)
{
}

static void
apple_intc_cpu_init(struct pic_softc *pic, struct cpu_info *ci)
{
	struct apple_intc_softc * const sc = PICTOSOFTC(pic);
	const u_int cpuno = cpu_index(ci);

	sc->sc_cpuid[cpuno] = AIC_READ(sc, AIC_WHOAMI);
}

static const struct pic_ops apple_intc_picops = {
	.pic_unblock_irqs = apple_intc_unblock_irqs,
	.pic_block_irqs = apple_intc_block_irqs,
	.pic_establish_irq = apple_intc_establish_irq,
	.pic_set_priority = apple_intc_set_priority,
#ifdef MULTIPROCESSOR
	.pic_cpu_init = apple_intc_cpu_init,
#endif
};

static void
apple_intc_local_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t mask)
{
	KASSERT(irqbase == 0);

	if ((mask & __BIT(LOCALPIC_SOURCE_TIMER)) != 0) {
		gtmr_cntv_ctl_write(gtmr_cntv_ctl_read() & ~CNTCTL_IMASK);
		isb();
	}
}

static void
apple_intc_local_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t mask)
{
	KASSERT(irqbase == 0);

	if ((mask & __BIT(LOCALPIC_SOURCE_TIMER)) != 0) {
		gtmr_cntv_ctl_write(gtmr_cntv_ctl_read() | CNTCTL_IMASK);
		isb();
	}
}

static void
apple_intc_local_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
}

#ifdef MULTIPROCESSOR
static void
apple_intc_local_ipi_send(struct pic_softc *pic, const kcpuset_t *kcp, u_long ipi)
{
	struct apple_intc_percpu * const pc = PICTOPERCPU(pic);
	struct apple_intc_softc * const sc = pc->pc_sc;
	const u_int target = sc->sc_cpuid[pc->pc_cpuid];

	atomic_or_32(&pc->pc_ipimask, __BIT(ipi));
	AIC_WRITE(sc, AIC_IPI_SEND, __BIT(target));
}
#endif /* MULTIPROCESSOR */

static const struct pic_ops apple_intc_localpicops = {
	.pic_unblock_irqs = apple_intc_local_unblock_irqs,
	.pic_block_irqs = apple_intc_local_block_irqs,
	.pic_establish_irq = apple_intc_local_establish_irq,
#ifdef MULTIPROCESSOR
	.pic_ipi_send = apple_intc_local_ipi_send,
#endif
};

static void *
apple_intc_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct apple_intc_softc * const sc = device_private(dev);

	/* 1st cell is the interrupt type (0=IRQ, 1=FIQ) */
	const u_int type = be32toh(specifier[0]);
	/* 2nd cell is the interrupt number */
	const u_int intno = be32toh(specifier[1]);
	/* 3rd cell is the interrupt flags */

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	if (type == 0)
		return intr_establish_xname(intno, ipl, IST_LEVEL | mpsafe,
		    func, arg, xname);

	/* interate over CPUs for LOCALPIC_SOURCE_TIMER */
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	void *ih = NULL;
	for (CPU_INFO_FOREACH(cii, ci)) {
		const cpuid_t cpuno = cpu_index(ci);
		struct apple_intc_percpu * const pc = &sc->sc_pc[cpuno];
		struct pic_softc * const pic = &pc->pc_pic;
		const int irq = pic->pic_irqbase + LOCALPIC_SOURCE_TIMER;

		void *ihn = intr_establish_xname(irq, ipl, IST_LEVEL | mpsafe,
		    func, arg, xname);
		if (cpuno == 0)
			ih = ihn;
	}
	return ih;
}

static void
apple_intc_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
apple_intc_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	if (!specifier)
		return false;

	/* 1st cell is the interrupt type (0=IRQ, 1=FIQ) */
	const u_int type = be32toh(specifier[0]);
	/* 2nd cell is the interrupt number */
	const u_int intno = be32toh(specifier[1]);

	snprintf(buf, buflen, "%s %u", type == 0 ? "irq" : "fiq", intno);

	return true;
}

static const struct fdtbus_interrupt_controller_func apple_intc_fdt_funcs = {
	.establish = apple_intc_fdt_establish,
	.disestablish = apple_intc_fdt_disestablish,
	.intrstr = apple_intc_fdt_intrstr,
};

static void
apple_intc_mark_pending(struct pic_softc *pic, u_int intno)
{
	const int base = intno & ~0x1f;
	const uint32_t pending = __BIT(intno & 0x1f);
	pic_mark_pending_sources(pic, base, pending);
}

static void
apple_intc_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct apple_intc_softc * const sc = intc_softc;
	struct pic_softc *pic;
	struct intrsource *is;
	const int oldipl = ci->ci_cpl;
	uint16_t evtype, evdata;
	bus_size_t clr_reg;
	uint32_t clr_val;

	ci->ci_data.cpu_nintr++;

	for (;;) {
		const uint32_t ev = AIC_READ(sc, AIC_EVENT);
		evtype = __SHIFTOUT(ev, AIC_EVENT_TYPE);
		evdata = __SHIFTOUT(ev, AIC_EVENT_DATA);

		dsb(sy);
		isb();

		if (evtype == AIC_EVENT_TYPE_IRQ) {
			KASSERT(evdata < sc->sc_nirq);
			pic = &sc->sc_pic;
			is = pic->pic_sources[evdata];
			KASSERT(is != NULL);

			AIC_WRITE(sc, AIC_SW_CLR(evdata),
			    __BIT(evdata & 0x1f));

			clr_reg = AIC_MASK_CLR(evdata);
			clr_val = AIC_MASK_BIT(evdata);
		} else if (evtype == AIC_EVENT_TYPE_IPI) {
			KASSERT(evdata == AIC_EVENT_IPI_OTHER);
			pic = &sc->sc_pc[cpu_index(ci)].pc_pic;
			is = pic->pic_sources[LOCALPIC_SOURCE_IPI];
			KASSERT(is != NULL);

			AIC_WRITE(sc, AIC_IPI_ACK, AIC_IPI_OTHER);

			clr_reg = 0;
			clr_val = 0;
		} else {
			break;
		}

		if (ci->ci_cpl >= is->is_ipl) {
			apple_intc_mark_pending(pic, is->is_irq);
		} else {
			pic_set_priority(ci, is->is_ipl);
			ENABLE_INTERRUPT();
			pic_dispatch(is, frame);
			DISABLE_INTERRUPT();

			if (clr_val != 0) {
				AIC_WRITE(sc, clr_reg, clr_val);
			}
		}
	}

	if (oldipl != IPL_HIGH) {
		pic_do_pending_ints(DAIF_I|DAIF_F, oldipl, frame);
	}
}

static void
apple_intc_fiq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct apple_intc_softc * const sc = intc_softc;
	struct pic_softc * const pic = &sc->sc_pc[cpu_index(ci)].pc_pic;
	const int oldipl = ci->ci_cpl;

	ci->ci_data.cpu_nintr++;

	struct intrsource * const is = pic->pic_sources[LOCALPIC_SOURCE_TIMER];

	dsb(sy);
	isb();

	if (oldipl >= is->is_ipl) {
		apple_intc_mark_pending(pic, LOCALPIC_SOURCE_TIMER);
	} else {
		pic_set_priority(ci, is->is_ipl);
		pic_dispatch(is, frame);
	}

	if (oldipl != IPL_HIGH) {
		pic_do_pending_ints(DAIF_I|DAIF_F, oldipl, frame);
	}
}

#ifdef MULTIPROCESSOR
static int
apple_intc_ipi_handler(void *priv)
{
	struct apple_intc_percpu * const pc = priv;
	struct apple_intc_softc * const sc = pc->pc_sc;
	uint32_t ipimask, bit;

	AIC_WRITE(sc, AIC_IPI_MASK_CLR, AIC_IPI_OTHER);
	ipimask = atomic_swap_32(&pc->pc_ipimask, 0);

	while ((bit = ffs(ipimask)) > 0) {
		const u_int ipi = bit - 1;

		switch (ipi) {
		case IPI_AST:
			pic_ipi_ast(priv);
			break;
		case IPI_NOP:
			pic_ipi_nop(priv);
			break;
#ifdef __HAVE_PREEMPTION
		case IPI_KPREEMPT:
			pic_ipi_kpreempt(priv);
			break;
#endif
		case IPI_XCALL:
			pic_ipi_xcall(priv);
			break;
		case IPI_GENERIC:
			pic_ipi_generic(priv);
			break;
		case IPI_SHOOTDOWN:
			pic_ipi_shootdown(priv);
			break;
#ifdef DDB
		case IPI_DDB:
			pic_ipi_ddb(priv);
			break;
#endif
		}
		ipimask &= ~__BIT(ipi);
	}

	return 1;
}
#endif /* MULTIPROCESSOR */

static int
apple_intc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_intc_attach(device_t parent, device_t self, void *aux)
{
	struct apple_intc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_nirq = AIC_READ(sc, AIC_INFO) & AIC_INFO_NIRQ;

	aprint_naive("\n");
	aprint_normal(": Apple AIC (%u IRQs, 1 FIQ)\n", sc->sc_nirq);
	KASSERT(sc->sc_nirq % 32 == 0);

	sc->sc_pic.pic_ops = &apple_intc_picops;
	sc->sc_pic.pic_maxsources = sc->sc_nirq;
	snprintf(sc->sc_pic.pic_name, sizeof(sc->sc_pic.pic_name), "AIC");
	pic_add(&sc->sc_pic, 0);

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &apple_intc_fdt_funcs);
	if (error) {
		aprint_error_dev(self, "couldn't register with fdtbus: %d\n",
		    error);
		return;
	}

	KASSERT(intc_softc == NULL);
	intc_softc = sc;
	arm_fdt_irq_set_handler(apple_intc_irq_handler);
	arm_fdt_fiq_set_handler(apple_intc_fiq_handler);

	KASSERT(ncpu != 0);
	sc->sc_cpuid = kmem_zalloc(sizeof(*sc->sc_cpuid) * ncpu, KM_SLEEP);
	sc->sc_pc = kmem_zalloc(sizeof(*sc->sc_pc) * ncpu, KM_SLEEP);

	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	for (CPU_INFO_FOREACH(cii, ci)) {
		const cpuid_t cpuno = cpu_index(ci);
		struct apple_intc_percpu * const pc = &sc->sc_pc[cpuno];
		struct pic_softc * const pic = &pc->pc_pic;

		pc->pc_sc = sc;
		pc->pc_cpuid = cpuno;

#ifdef MULTIPROCESSOR
		pic->pic_cpus = ci->ci_kcpuset;
#endif
		pic->pic_ops = &apple_intc_localpicops;
		pic->pic_maxsources = 2;
		snprintf(pic->pic_name, sizeof(pic->pic_name), "AIC/%lu", cpuno);

		pic_add(pic, PIC_IRQBASE_ALLOC);

#ifdef MULTIPROCESSOR
		intr_establish_xname(pic->pic_irqbase + LOCALPIC_SOURCE_IPI,
		    IPL_HIGH, IST_LEVEL | IST_MPSAFE, apple_intc_ipi_handler,
		    pc, "ipi");
#endif
	}

	apple_intc_cpu_init(&sc->sc_pic, curcpu());
}

CFATTACH_DECL_NEW(apple_intc, sizeof(struct apple_intc_softc),
	apple_intc_match, apple_intc_attach, NULL, NULL);
