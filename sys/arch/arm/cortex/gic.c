/*	$NetBSD: gic.c,v 1.53 2022/03/03 06:26:28 riastradh Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_gic.h"

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gic.c,v 1.53 2022/03/03 06:26:28 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/proc.h>
#include <sys/atomic.h>

#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/cortex/gic_reg.h>
#include <arm/cortex/mpcore_var.h>

#ifdef GIC_SPLFUNCS
#include <arm/cortex/gic_splfuncs.h>
#endif

void armgic_irq_handler(void *);

#define	ARMGIC_SGI_IPIBASE	0

/*
 * SGIs 8-16 are reserved for use by ARM Trusted Firmware.
 */
__CTASSERT(ARMGIC_SGI_IPIBASE + NIPI <= 8);

static int armgic_match(device_t, cfdata_t, void *);
static void armgic_attach(device_t, device_t, void *);

static void armgic_set_priority(struct pic_softc *, int);
static void armgic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void armgic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void armgic_establish_irq(struct pic_softc *, struct intrsource *);
#if 0
static void armgic_source_name(struct pic_softc *, int, char *, size_t);
#endif

#ifdef MULTIPROCESSOR
static void armgic_cpu_init(struct pic_softc *, struct cpu_info *);
static void armgic_ipi_send(struct pic_softc *, const kcpuset_t *, u_long);
static void armgic_get_affinity(struct pic_softc *, size_t, kcpuset_t *);
static int armgic_set_affinity(struct pic_softc *, size_t, const kcpuset_t *);
#endif

static const struct pic_ops armgic_picops = {
	.pic_unblock_irqs = armgic_unblock_irqs,
	.pic_block_irqs = armgic_block_irqs,
	.pic_establish_irq = armgic_establish_irq,
#if 0
	.pic_source_name = armgic_source_name,
#endif
	.pic_set_priority = armgic_set_priority,
#ifdef MULTIPROCESSOR
	.pic_cpu_init = armgic_cpu_init,
	.pic_ipi_send = armgic_ipi_send,
	.pic_get_affinity = armgic_get_affinity,
	.pic_set_affinity = armgic_set_affinity,
#endif
};

#define	PICTOSOFTC(pic)		((struct armgic_softc *)(pic))

static struct armgic_softc {
	struct pic_softc sc_pic;
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_gicch;
	bus_space_handle_t sc_gicdh;
	size_t sc_gic_lines;
	uint32_t sc_gic_type;
	uint32_t sc_gic_valid_lines[1024/32];
	uint32_t sc_enabled_local;
#ifdef MULTIPROCESSOR
	uint32_t sc_target[MAXCPUS];
	uint32_t sc_mptargets;
#endif
	uint32_t sc_bptargets;
} armgic_softc = {
	.sc_pic = {
		.pic_ops = &armgic_picops,
		.pic_name = "armgic",
	},
};

static struct intrsource armgic_dummy_source;

__CTASSERT(NIPL == 8);

/*
 * GIC register are always in little-endian.  It is assumed the bus_space
 * will do any endian conversion required.
 */
static inline uint32_t
gicc_read(struct armgic_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_gicch, o);
}

static inline void
gicc_write(struct armgic_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_gicch, o, v);
}

static inline uint32_t
gicd_read(struct armgic_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_gicdh, o);
}

static inline void
gicd_write(struct armgic_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_gicdh, o, v);
}

static uint32_t
gicd_find_targets(struct armgic_softc *sc)
{
	uint32_t targets = 0;

	/*
	 * GICD_ITARGETSR0 through 7 are read-only, and each field returns
	 * a value that corresponds only to the processor reading the
	 * register. Use this to determine the current processor's
	 * CPU interface number.
	 */
	for (int i = 0; i < 8; i++) {
		targets = gicd_read(sc, GICD_ITARGETSRn(i));
		if (targets != 0)
			break;
	}
	targets |= (targets >> 16);
	targets |= (targets >> 8);
	targets &= 0xff;

	return targets ? targets : 1;
}

/*
 * In the GIC prioritization scheme, lower numbers have higher priority.
 * Only write priorities that could be non-secure.
 */
static inline uint32_t
armgic_ipl_to_priority(int ipl)
{
	return GICC_PMR_NONSECURE
	    | ((IPL_HIGH - ipl) * GICC_PMR_NS_PRIORITIES / NIPL);
}

#if 0
static inline int
armgic_priority_to_ipl(uint32_t priority)
{
	return IPL_HIGH
	    - (priority & ~GICC_PMR_NONSECURE) * NIPL / GICC_PMR_NS_PRIORITIES;
}
#endif

static void
armgic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irq_base / 32;

	if (group == 0)
		sc->sc_enabled_local |= irq_mask;

	gicd_write(sc, GICD_ISENABLERn(group), irq_mask);
}

static void
armgic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irq_base / 32;

	if (group == 0)
		sc->sc_enabled_local &= ~irq_mask;

	gicd_write(sc, GICD_ICENABLERn(group), irq_mask);
}

static void
armgic_set_priority(struct pic_softc *pic, int ipl)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	struct cpu_info * const ci = curcpu();

	if (ipl < ci->ci_hwpl) {
		/* Lowering priority mask */
		ci->ci_hwpl = ipl;
		gicc_write(sc, GICC_PMR, armgic_ipl_to_priority(ipl));
	}
}

#ifdef MULTIPROCESSOR
static void
armgic_get_affinity(struct pic_softc *pic, size_t irq, kcpuset_t *affinity)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irq / 32;
	int n;

	kcpuset_zero(affinity);
	if (group == 0) {
		/* All CPUs are targets for group 0 (SGI/PPI) */
		for (n = 0; n < MAXCPUS; n++) {
			if (sc->sc_target[n] != 0)
				kcpuset_set(affinity, n);
		}
	} else {
		/* Find distributor targets (SPI) */
		const u_int byte_shift = 8 * (irq & 3);
		const bus_size_t targets_reg = GICD_ITARGETSRn(irq / 4);
		const uint32_t targets = gicd_read(sc, targets_reg);
		const uint32_t targets_val = (targets >> byte_shift) & 0xff;

		for (n = 0; n < MAXCPUS; n++) {
			if (sc->sc_target[n] & targets_val)
				kcpuset_set(affinity, n);
		}
	}
}

static int
armgic_set_affinity(struct pic_softc *pic, size_t irq,
    const kcpuset_t *affinity)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irq / 32;
	if (group == 0)
		return EINVAL;

	const u_int byte_shift = 8 * (irq & 3);
	const bus_size_t targets_reg = GICD_ITARGETSRn(irq / 4);
	uint32_t targets_val = 0;
	int n;

	for (n = 0; n < MAXCPUS; n++) {
		if (kcpuset_isset(affinity, n))
			targets_val |= sc->sc_target[n];
	}

	uint32_t targets = gicd_read(sc, targets_reg);
	targets &= ~(0xff << byte_shift);
	targets |= (targets_val << byte_shift);
	gicd_write(sc, targets_reg, targets);

	return 0;
}
#endif

#ifdef __HAVE_PIC_FAST_SOFTINTS
void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep_p)
{
	lwp_t **lp = &l->l_cpu->ci_softlwps[level];
	KASSERT(*lp == NULL || *lp == l);
	*lp = l;
	/*
	 * Really easy.  Just tell it to trigger the local CPU.
	 */
	*machdep_p = GICD_SGIR_TargetListFilter_Me
	    | __SHIFTIN(level, GICD_SGIR_SGIINTID);
}

void
softint_trigger(uintptr_t machdep)
{

	gicd_write(&armgic_softc, GICD_SGIR, machdep);
}
#endif

void
armgic_irq_handler(void *tf)
{
	struct cpu_info * const ci = curcpu();
	struct armgic_softc * const sc = &armgic_softc;
	const int old_ipl = ci->ci_cpl;
	const int old_mtx_count = ci->ci_mtx_count;
	const int old_l_biglocks = ci->ci_curlwp->l_biglocks;
#ifdef DEBUG
	size_t n = 0;
#endif

	ci->ci_data.cpu_nintr++;

	/*
	 * Raise ci_hwpl (and PMR) to ci_cpl and IAR will tell us if the
	 * interrupt that got us here can have its handler run or not.
	 */
	if (ci->ci_hwpl <= old_ipl) {
		ci->ci_hwpl = old_ipl;
		gicc_write(sc, GICC_PMR, armgic_ipl_to_priority(old_ipl));
		/*
		 * we'll get no interrupts when PMR is IPL_HIGH, so bail
		 * early.
		 */
		if (old_ipl == IPL_HIGH) {
			return;
		}
	}

	for (;;) {
		uint32_t iar = gicc_read(sc, GICC_IAR);
		uint32_t irq = __SHIFTOUT(iar, GICC_IAR_IRQ);

		if (irq == GICC_IAR_IRQ_SPURIOUS ||
		    irq == GICC_IAR_IRQ_SSPURIOUS) {
			iar = gicc_read(sc, GICC_IAR);
			irq = __SHIFTOUT(iar, GICC_IAR_IRQ);
			if (irq == GICC_IAR_IRQ_SPURIOUS)
				break;
			if (irq == GICC_IAR_IRQ_SSPURIOUS) {
				break;
			}
		}

		KASSERTMSG(old_ipl != IPL_HIGH, "old_ipl %d pmr %#x hppir %#x",
		    old_ipl, gicc_read(sc, GICC_PMR), gicc_read(sc, GICC_HPPIR));

		//const uint32_t cpuid = __SHIFTOUT(iar, GICC_IAR_CPUID_MASK);
		struct intrsource * const is = sc->sc_pic.pic_sources[irq];
		KASSERT(is != &armgic_dummy_source);

		/*
		 * GIC has asserted IPL for us so we can just update ci_cpl.
		 *
		 * But it's not that simple.  We may have already bumped ci_cpl
		 * due to a high priority interrupt and now we are about to
		 * dispatch one lower than the previous.  It's possible for
		 * that previous interrupt to have deferred some interrupts
		 * so we need deal with those when lowering to the current
		 * interrupt's ipl.
		 *
		 * However, if are just raising ipl, we can just update ci_cpl.
		 */

		/* Surely we can KASSERT(ipl < ci->ci_cpl); */
		const int ipl = is->is_ipl;
		if (__predict_false(ipl < ci->ci_cpl)) {
			pic_do_pending_ints(I32_bit, ipl, tf);
			KASSERT(ci->ci_cpl == ipl);
		} else if (ci->ci_cpl != ipl) {
			KASSERTMSG(ipl > ci->ci_cpl, "ipl %d cpl %d hw-ipl %#x",
			    ipl, ci->ci_cpl,
			    gicc_read(sc, GICC_PMR));
			gicc_write(sc, GICC_PMR, armgic_ipl_to_priority(ipl));
			ci->ci_hwpl = ci->ci_cpl = ipl;
		}
		ENABLE_INTERRUPT();
		pic_dispatch(is, tf);
		DISABLE_INTERRUPT();
		gicc_write(sc, GICC_EOIR, iar);
#ifdef DEBUG
		n++;
		KDASSERTMSG(n < 5, "%s: processed too many (%zu)",
		    ci->ci_data.cpu_name, n);
#endif
	}

	/*
	 * Now handle any pending ints.
	 */
	pic_do_pending_ints(I32_bit, old_ipl, tf);
	KASSERTMSG(ci->ci_cpl == old_ipl, "ci_cpl %d old_ipl %d", ci->ci_cpl, old_ipl);
	KASSERT(old_mtx_count == ci->ci_mtx_count);
	KASSERT(old_l_biglocks == ci->ci_curlwp->l_biglocks);
}

void
armgic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	const size_t group = is->is_irq / 32;
	const u_int irq = is->is_irq & 31;
	const u_int byte_shift = 8 * (irq & 3);
	const u_int twopair_shift = 2 * (irq & 15);

	KASSERTMSG(sc->sc_gic_valid_lines[group] & __BIT(irq),
	    "irq %u: not valid (group[%zu]=0x%08x [0x%08x])",
	    is->is_irq, group, sc->sc_gic_valid_lines[group],
	    (uint32_t)__BIT(irq));

	KASSERTMSG(is->is_type == IST_LEVEL || is->is_type == IST_EDGE,
	    "irq %u: type %u unsupported", is->is_irq, is->is_type);

	const bus_size_t targets_reg = GICD_ITARGETSRn(is->is_irq / 4);
	const bus_size_t cfg_reg = GICD_ICFGRn(is->is_irq / 16);
	uint32_t targets = gicd_read(sc, targets_reg);
	uint32_t cfg = gicd_read(sc, cfg_reg);

	if (group > 0) {
		/*
		 * There are 4 irqs per TARGETS register.  For now bind
		 * to the primary cpu.
		 */
		targets &= ~(0xffU << byte_shift);
#if 0
#ifdef MULTIPROCESSOR
		if (is->is_mpsafe) {
			targets |= sc->sc_mptargets << byte_shift;
		} else
#endif
#endif
		targets |= sc->sc_bptargets << byte_shift;
		gicd_write(sc, targets_reg, targets);

		/*
		 * There are 16 irqs per CFG register.  10=EDGE 00=LEVEL
		 */
		uint32_t new_cfg = cfg;
		uint32_t old_cfg = (cfg >> twopair_shift) & __BITS(1, 0);
		if (is->is_type == IST_LEVEL && (old_cfg & __BIT(1)) != 0) {
			new_cfg &= ~(__BITS(1, 0) << twopair_shift);
		} else if (is->is_type == IST_EDGE && (old_cfg & 2) == 0) {
			new_cfg |= __BIT(1) << twopair_shift;
		}
		if (new_cfg != cfg) {
			gicd_write(sc, cfg_reg, new_cfg);
		}
#ifdef MULTIPROCESSOR
	} else {
		/*
		 * All group 0 interrupts are per processor and MPSAFE by
		 * default.
		 */
		is->is_mpsafe = true;
		is->is_percpu = true;
#endif
	}

	/*
	 * There are 4 irqs per PRIORITY register.  Map the IPL
	 * to GIC priority.
	 */
	const bus_size_t priority_reg = GICD_IPRIORITYRn(is->is_irq / 4);
	uint32_t priority = gicd_read(sc, priority_reg);
	priority &= ~(0xffU << byte_shift);
	priority |= armgic_ipl_to_priority(is->is_ipl) << byte_shift;
	gicd_write(sc, priority_reg, priority);
}

#ifdef MULTIPROCESSOR
static void
armgic_cpu_init_priorities(struct armgic_softc *sc)
{
	/* Set lowest priority, i.e. disable interrupts */
	for (size_t i = 0; i < sc->sc_pic.pic_maxsources; i += 4) {
		const bus_size_t priority_reg = GICD_IPRIORITYRn(i / 4);
		gicd_write(sc, priority_reg, ~0);
	}
}

static void
armgic_cpu_update_priorities(struct armgic_softc *sc)
{
	uint32_t enabled = sc->sc_enabled_local;
	for (size_t i = 0; i < sc->sc_pic.pic_maxsources; i += 4, enabled >>= 4) {
		const bus_size_t priority_reg = GICD_IPRIORITYRn(i / 4);
		uint32_t priority = gicd_read(sc, priority_reg);
		uint32_t byte_mask = 0xff;
		size_t byte_shift = 0;
		for (size_t j = 0; j < 4; j++, byte_mask <<= 8, byte_shift += 8) {
			struct intrsource * const is = sc->sc_pic.pic_sources[i+j];
			priority |= byte_mask;
			if (is == NULL || is == &armgic_dummy_source)
				continue;
			priority &= ~byte_mask;
			priority |= armgic_ipl_to_priority(is->is_ipl) << byte_shift;
		}
		gicd_write(sc, priority_reg, priority);
	}
}

static void
armgic_cpu_init_targets(struct armgic_softc *sc)
{
	/*
	 * Update the mpsafe targets
	 */
	for (size_t irq = 32; irq < sc->sc_pic.pic_maxsources; irq++) {
		struct intrsource * const is = sc->sc_pic.pic_sources[irq];
		const bus_size_t targets_reg = GICD_ITARGETSRn(irq / 4);
		if (is != NULL && is->is_mpsafe) {
			const u_int byte_shift = 8 * (irq & 3);
			uint32_t targets = gicd_read(sc, targets_reg);
#if 0
			targets |= sc->sc_mptargets << byte_shift;
#else
			targets |= sc->sc_bptargets << byte_shift;
#endif
			gicd_write(sc, targets_reg, targets);
		}
	}
}

void
armgic_cpu_init(struct pic_softc *pic, struct cpu_info *ci)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);
	sc->sc_target[cpu_index(ci)] = gicd_find_targets(sc);
	atomic_or_32(&sc->sc_mptargets, sc->sc_target[cpu_index(ci)]);
	KASSERTMSG(ci->ci_cpl == IPL_HIGH, "ipl %d not IPL_HIGH", ci->ci_cpl);
	armgic_cpu_init_priorities(sc);
	if (!CPU_IS_PRIMARY(ci)) {
		if (popcount(sc->sc_mptargets) != 1) {
			armgic_cpu_init_targets(sc);
		}
		if (sc->sc_enabled_local) {
			armgic_cpu_update_priorities(sc);
			gicd_write(sc, GICD_ISENABLERn(0),
			    sc->sc_enabled_local);
		}
	}
	ci->ci_hwpl = ci->ci_cpl;
	gicc_write(sc, GICC_PMR, armgic_ipl_to_priority(ci->ci_cpl));	// set PMR
	gicc_write(sc, GICC_CTRL, GICC_CTRL_V1_Enable);	// enable interrupt
	ENABLE_INTERRUPT();				// allow IRQ exceptions
}

void
armgic_ipi_send(struct pic_softc *pic, const kcpuset_t *kcp, u_long ipi)
{
	struct armgic_softc * const sc = PICTOSOFTC(pic);

#if 0
	if (ipi == IPI_NOP) {
		sev();
		return;
	}
#endif

	uint32_t sgir = __SHIFTIN(ARMGIC_SGI_IPIBASE + ipi, GICD_SGIR_SGIINTID);
	if (kcp != NULL) {
		uint32_t targets_val = 0;
		for (int n = 0; n < MAXCPUS; n++) {
			if (kcpuset_isset(kcp, n))
				targets_val |= sc->sc_target[n];
		}
		sgir |= __SHIFTIN(targets_val, GICD_SGIR_TargetList);
		sgir |= GICD_SGIR_TargetListFilter_List;
	} else {
		if (ncpu == 1)
			return;
		sgir |= GICD_SGIR_TargetListFilter_NotMe;
	}

	gicd_write(sc, GICD_SGIR, sgir);
}
#endif

int
armgic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (strcmp(cf->cf_name, mpcaa->mpcaa_name) != 0)
		return 0;

	return 1;
}

void
armgic_attach(device_t parent, device_t self, void *aux)
{
	struct armgic_softc * const sc = &armgic_softc;
	struct mpcore_attach_args * const mpcaa = aux;

	sc->sc_dev = self;
	device_set_private(self, sc);

	sc->sc_memt = mpcaa->mpcaa_memt;	/* provided for us */
	bus_space_subregion(sc->sc_memt, mpcaa->mpcaa_memh, mpcaa->mpcaa_off1,
	    4096, &sc->sc_gicdh);
	bus_space_subregion(sc->sc_memt, mpcaa->mpcaa_memh, mpcaa->mpcaa_off2,
	    4096, &sc->sc_gicch);

	sc->sc_gic_type = gicd_read(sc, GICD_TYPER);
	sc->sc_pic.pic_maxsources = GICD_TYPER_LINES(sc->sc_gic_type);

	gicc_write(sc, GICC_CTRL, 0);	/* disable all interrupts */
	gicd_write(sc, GICD_CTRL, 0);	/* disable all interrupts */

	gicc_write(sc, GICC_PMR, 0xff);
	uint32_t pmr = gicc_read(sc, GICC_PMR);
	u_int priorities = 1 << popcount32(pmr);

	const uint32_t iidr = gicc_read(sc, GICC_IIDR);
	const int iidr_prod = __SHIFTOUT(iidr, GICC_IIDR_ProductID);
	const int iidr_arch = __SHIFTOUT(iidr, GICC_IIDR_ArchVersion);
	const int iidr_rev = __SHIFTOUT(iidr, GICC_IIDR_Revision);
	const int iidr_imp = __SHIFTOUT(iidr, GICC_IIDR_Implementer);

	/*
	 * Find the boot processor's CPU interface number.
	 */
	sc->sc_bptargets = gicd_find_targets(sc);

	/*
	 * Let's find out how many real sources we have.
	 */
	for (size_t i = 0, group = 0;
	     i < sc->sc_pic.pic_maxsources;
	     i += 32, group++) {
		/*
		 * To figure what sources are real, one enables all interrupts
		 * and then reads back the enable mask so which ones really
		 * got enabled.
		 */
		gicd_write(sc, GICD_ISENABLERn(group), 0xffffffff);
		uint32_t valid = gicd_read(sc, GICD_ISENABLERn(group));

		/*
		 * Now disable (clear enable) them again.
		 */
		gicd_write(sc, GICD_ICENABLERn(group), valid);

		/*
		 * Count how many are valid.
		 */
		sc->sc_gic_lines += popcount32(valid);
		sc->sc_gic_valid_lines[group] = valid;
	}

	aprint_normal(": Generic Interrupt Controller, "
	    "%zu sources (%zu valid)\n",
	    sc->sc_pic.pic_maxsources, sc->sc_gic_lines);
	aprint_debug_dev(sc->sc_dev, "Architecture version %d"
	    " (0x%x:%d rev %d)\n", iidr_arch, iidr_imp, iidr_prod,
	    iidr_rev);

#ifdef MULTIPROCESSOR
	sc->sc_pic.pic_cpus = kcpuset_running;
#endif
	pic_add(&sc->sc_pic, 0);

	/*
	 * Force the GICD to IPL_HIGH and then enable interrupts.
	 */
	struct cpu_info * const ci = curcpu();
	KASSERTMSG(ci->ci_cpl == IPL_HIGH, "ipl %d not IPL_HIGH", ci->ci_cpl);
	armgic_set_priority(&sc->sc_pic, ci->ci_cpl);	// set PMR
	gicd_write(sc, GICD_CTRL, GICD_CTRL_Enable);	// enable Distributer
	gicc_write(sc, GICC_CTRL, GICC_CTRL_V1_Enable);	// enable CPU interrupts
	ENABLE_INTERRUPT();				// allow interrupt exceptions

	/*
	 * For each line that isn't valid, we set the intrsource for it to
	 * point at a dummy source so that pic_intr_establish will fail for it.
	 */
	for (size_t i = 0, group = 0;
	     i < sc->sc_pic.pic_maxsources;
	     i += 32, group++) {
		uint32_t invalid = ~sc->sc_gic_valid_lines[group];
		for (size_t j = 0; invalid && j < 32; j++, invalid >>= 1) {
			if (invalid & 1) {
				sc->sc_pic.pic_sources[i + j] =
				     &armgic_dummy_source;
			}
		}
	}
#ifdef __HAVE_PIC_FAST_SOFTINTS
	intr_establish_xname(SOFTINT_BIO, IPL_SOFTBIO, IST_MPSAFE | IST_EDGE,
	    pic_handle_softint, (void *)SOFTINT_BIO, "softint bio");
	intr_establish_xname(SOFTINT_CLOCK, IPL_SOFTCLOCK, IST_MPSAFE | IST_EDGE,
	    pic_handle_softint, (void *)SOFTINT_CLOCK, "softint clock");
	intr_establish_xname(SOFTINT_NET, IPL_SOFTNET, IST_MPSAFE | IST_EDGE,
	    pic_handle_softint, (void *)SOFTINT_NET, "softint net");
	intr_establish_xname(SOFTINT_SERIAL, IPL_SOFTSERIAL, IST_MPSAFE | IST_EDGE,
	    pic_handle_softint, (void *)SOFTINT_SERIAL, "softint serial");
#endif
#ifdef MULTIPROCESSOR
	armgic_cpu_init(&sc->sc_pic, curcpu());

	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_AST, IPL_VM,
	    IST_MPSAFE | IST_EDGE, pic_ipi_ast, (void *)-1, "IPI ast");
	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_XCALL, IPL_HIGH,
	    IST_MPSAFE | IST_EDGE, pic_ipi_xcall, (void *)-1, "IPI xcall");
	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_GENERIC, IPL_HIGH,
	    IST_MPSAFE | IST_EDGE, pic_ipi_generic, (void *)-1, "IPI generic");
	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_NOP, IPL_VM,
	    IST_MPSAFE | IST_EDGE, pic_ipi_nop, (void *)-1, "IPI nop");
	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_SHOOTDOWN, IPL_SCHED,
	    IST_MPSAFE | IST_EDGE, pic_ipi_shootdown, (void *)-1, "IPI shootdown");
#ifdef DDB
	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_DDB, IPL_HIGH,
	    IST_MPSAFE | IST_EDGE, pic_ipi_ddb, NULL, "IPI ddb");
#endif
#ifdef __HAVE_PREEMPTION
	intr_establish_xname(ARMGIC_SGI_IPIBASE + IPI_KPREEMPT, IPL_VM,
	    IST_MPSAFE | IST_EDGE, pic_ipi_kpreempt, (void *)-1, "IPI kpreempt");
#endif
#endif

	const u_int ppis = popcount32(sc->sc_gic_valid_lines[0] >> 16);
	const u_int sgis = popcount32(sc->sc_gic_valid_lines[0] & 0xffff);
	aprint_normal_dev(sc->sc_dev, "%u Priorities, %zu SPIs, %u PPIs, "
	    "%u SGIs\n",  priorities, sc->sc_gic_lines - ppis - sgis, ppis,
	    sgis);

#ifdef GIC_SPLFUNCS
	gic_spl_init();
#endif
}

CFATTACH_DECL_NEW(armgic, 0,
    armgic_match, armgic_attach, NULL, NULL);
