/* $NetBSD: gicv3.c,v 1.20.2.1 2019/09/22 12:30:15 martin Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_multiprocessor.h"

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gicv3.c,v 1.20.2.1 2019/09/22 12:30:15 martin Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <machine/cpufunc.h>

#include <arm/locore.h>
#include <arm/armreg.h>

#include <arm/cortex/gicv3.h>
#include <arm/cortex/gic_reg.h>

#define	PICTOSOFTC(pic)	\
	((void *)((uintptr_t)(pic) - offsetof(struct gicv3_softc, sc_pic)))
#define	LPITOSOFTC(lpi) \
	((void *)((uintptr_t)(lpi) - offsetof(struct gicv3_softc, sc_lpi)))

#define	IPL_TO_PRIORITY(sc, ipl)	(((0xff - (ipl)) << (sc)->sc_priority_shift) & 0xff)
#define	IPL_TO_PMR(sc, ipl)		(((0xff - (ipl)) << (sc)->sc_pmr_shift) & 0xff)
#define	IPL_TO_LPIPRIO(sc, ipl)		(((0xff - (ipl)) << 4) & 0xff)

static struct gicv3_softc *gicv3_softc;

static inline uint32_t
gicd_read_4(struct gicv3_softc *sc, bus_size_t reg)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh_d, reg);
}

static inline void
gicd_write_4(struct gicv3_softc *sc, bus_size_t reg, uint32_t val)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_d, reg, val);
}

static inline uint64_t
gicd_read_8(struct gicv3_softc *sc, bus_size_t reg)
{
	return bus_space_read_8(sc->sc_bst, sc->sc_bsh_d, reg);
}

static inline void
gicd_write_8(struct gicv3_softc *sc, bus_size_t reg, uint64_t val)
{
	bus_space_write_8(sc->sc_bst, sc->sc_bsh_d, reg, val);
}

static inline uint32_t
gicr_read_4(struct gicv3_softc *sc, u_int index, bus_size_t reg)
{
	KASSERT(index < sc->sc_bsh_r_count);
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh_r[index], reg);
}

static inline void
gicr_write_4(struct gicv3_softc *sc, u_int index, bus_size_t reg, uint32_t val)
{
	KASSERT(index < sc->sc_bsh_r_count);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_r[index], reg, val);
}

static inline uint64_t
gicr_read_8(struct gicv3_softc *sc, u_int index, bus_size_t reg)
{
	KASSERT(index < sc->sc_bsh_r_count);
	return bus_space_read_8(sc->sc_bst, sc->sc_bsh_r[index], reg);
}

static inline void
gicr_write_8(struct gicv3_softc *sc, u_int index, bus_size_t reg, uint64_t val)
{
	KASSERT(index < sc->sc_bsh_r_count);
	bus_space_write_8(sc->sc_bst, sc->sc_bsh_r[index], reg, val);
}

static void
gicv3_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);
	struct cpu_info * const ci = curcpu();
	const u_int group = irqbase / 32;

	if (group == 0) {
		sc->sc_enabled_sgippi |= mask;
		gicr_write_4(sc, ci->ci_gic_redist, GICR_ISENABLER0, mask);
		while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTLR) & GICR_CTLR_RWP)
			;
	} else {
		gicd_write_4(sc, GICD_ISENABLERn(group), mask);
		while (gicd_read_4(sc, GICD_CTRL) & GICD_CTRL_RWP)
			;
	}
}

static void
gicv3_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);
	struct cpu_info * const ci = curcpu();
	const u_int group = irqbase / 32;

	if (group == 0) {
		sc->sc_enabled_sgippi &= ~mask;
		gicr_write_4(sc, ci->ci_gic_redist, GICR_ICENABLER0, mask);
		while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTLR) & GICR_CTLR_RWP)
			;
	} else {
		gicd_write_4(sc, GICD_ICENABLERn(group), mask);
		while (gicd_read_4(sc, GICD_CTRL) & GICD_CTRL_RWP)
			;
	}
}

static void
gicv3_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);
	const u_int group = is->is_irq / 32;
	uint32_t ipriority, icfg;
	uint64_t irouter;
	u_int n;

	const u_int ipriority_val = IPL_TO_PRIORITY(sc, is->is_ipl);
	const u_int ipriority_shift = (is->is_irq & 0x3) * 8;
	const u_int icfg_shift = (is->is_irq & 0xf) * 2;

	if (group == 0) {
		/* SGIs and PPIs are always MP-safe */
		is->is_mpsafe = true;

		/* Update interrupt configuration and priority on all redistributors */
		for (n = 0; n < sc->sc_bsh_r_count; n++) {
			icfg = gicr_read_4(sc, n, GICR_ICFGRn(is->is_irq / 16));
			if (is->is_type == IST_LEVEL)
				icfg &= ~(0x2 << icfg_shift);
			if (is->is_type == IST_EDGE)
				icfg |= (0x2 << icfg_shift);
			gicr_write_4(sc, n, GICR_ICFGRn(is->is_irq / 16), icfg);

			ipriority = gicr_read_4(sc, n, GICR_IPRIORITYRn(is->is_irq / 4));
			ipriority &= ~(0xff << ipriority_shift);
			ipriority |= (ipriority_val << ipriority_shift);
			gicr_write_4(sc, n, GICR_IPRIORITYRn(is->is_irq / 4), ipriority);
		}
	} else {
		if (is->is_mpsafe) {
			/* Route MP-safe interrupts to all participating PEs */
			irouter = GICD_IROUTER_Interrupt_Routing_mode;
		} else {
			/* Route non-MP-safe interrupts to the primary PE only */
			irouter = sc->sc_irouter[0];
		}
		gicd_write_8(sc, GICD_IROUTER(is->is_irq), irouter);

		/* Update interrupt configuration */
		icfg = gicd_read_4(sc, GICD_ICFGRn(is->is_irq / 16));
		if (is->is_type == IST_LEVEL)
			icfg &= ~(0x2 << icfg_shift);
		if (is->is_type == IST_EDGE)
			icfg |= (0x2 << icfg_shift);
		gicd_write_4(sc, GICD_ICFGRn(is->is_irq / 16), icfg);

		/* Update interrupt priority */
		ipriority = gicd_read_4(sc, GICD_IPRIORITYRn(is->is_irq / 4));
		ipriority &= ~(0xff << ipriority_shift);
		ipriority |= (ipriority_val << ipriority_shift);
		gicd_write_4(sc, GICD_IPRIORITYRn(is->is_irq / 4), ipriority);
	}
}

static void
gicv3_set_priority(struct pic_softc *pic, int ipl)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);

	icc_pmr_write(IPL_TO_PMR(sc, ipl));
	arm_isb();
}

static void
gicv3_dist_enable(struct gicv3_softc *sc)
{
	uint32_t gicd_ctrl;
	u_int n;

	/* Disable the distributor */
	gicd_write_4(sc, GICD_CTRL, 0);

	/* Wait for register write to complete */
	while (gicd_read_4(sc, GICD_CTRL) & GICD_CTRL_RWP)
		;

	/* Clear all INTID enable bits */
	for (n = 32; n < sc->sc_pic.pic_maxsources; n += 32)
		gicd_write_4(sc, GICD_ICENABLERn(n / 32), ~0);

	/* Set default priorities to lowest */
	for (n = 32; n < sc->sc_pic.pic_maxsources; n += 4)
		gicd_write_4(sc, GICD_IPRIORITYRn(n / 4), ~0);

	/* Set all interrupts to G1NS */
	for (n = 32; n < sc->sc_pic.pic_maxsources; n += 32) {
		gicd_write_4(sc, GICD_IGROUPRn(n / 32), ~0);
		gicd_write_4(sc, GICD_IGRPMODRn(n / 32), 0);
	}

	/* Set all interrupts level-sensitive by default */
	for (n = 32; n < sc->sc_pic.pic_maxsources; n += 16)
		gicd_write_4(sc, GICD_ICFGRn(n / 16), 0);

	/* Wait for register writes to complete */
	while (gicd_read_4(sc, GICD_CTRL) & GICD_CTRL_RWP)
		;

	/* Enable Affinity routing and G1NS interrupts */
	gicd_ctrl = GICD_CTRL_EnableGrp1A | GICD_CTRL_ARE_NS;
	gicd_write_4(sc, GICD_CTRL, gicd_ctrl);
}

static void
gicv3_redist_enable(struct gicv3_softc *sc, struct cpu_info *ci)
{
	uint32_t icfg;
	u_int n, o;

	/* Clear INTID enable bits */
	gicr_write_4(sc, ci->ci_gic_redist, GICR_ICENABLER0, ~0);

	/* Wait for register write to complete */
	while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTLR) & GICR_CTLR_RWP)
		;

	/* Set default priorities */
	for (n = 0; n < 32; n += 4) {
		uint32_t priority = 0;
		size_t byte_shift = 0;
		for (o = 0; o < 4; o++, byte_shift += 8) {
			struct intrsource * const is = sc->sc_pic.pic_sources[n + o];
			if (is == NULL)
				priority |= 0xff << byte_shift;
			else {
				const u_int ipriority_val = IPL_TO_PRIORITY(sc, is->is_ipl);
				priority |= ipriority_val << byte_shift;
			}
		}
		gicr_write_4(sc, ci->ci_gic_redist, GICR_IPRIORITYRn(n / 4), priority);
	}

	/* Set all interrupts to G1NS */
	gicr_write_4(sc, ci->ci_gic_redist, GICR_IGROUPR0, ~0);
	gicr_write_4(sc, ci->ci_gic_redist, GICR_IGRPMODR0, 0);

	/* Restore PPI configs */
	for (n = 0, icfg = 0; n < 16; n++) {
		struct intrsource * const is = sc->sc_pic.pic_sources[16 + n];
		if (is != NULL && is->is_type == IST_EDGE)
			icfg |= (0x2 << (n * 2));
	}
	gicr_write_4(sc, ci->ci_gic_redist, GICR_ICFGRn(1), icfg);

	/* Restore current enable bits */
	gicr_write_4(sc, ci->ci_gic_redist, GICR_ISENABLER0, sc->sc_enabled_sgippi);

	/* Wait for register write to complete */
	while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTLR) & GICR_CTLR_RWP)
		;
}

static uint64_t
gicv3_cpu_identity(void)
{
	u_int aff3, aff2, aff1, aff0;

	const register_t mpidr = cpu_mpidr_aff_read();
	aff0 = __SHIFTOUT(mpidr, MPIDR_AFF0);
	aff1 = __SHIFTOUT(mpidr, MPIDR_AFF1);
	aff2 = __SHIFTOUT(mpidr, MPIDR_AFF2);
	aff3 = __SHIFTOUT(mpidr, MPIDR_AFF3);

	return __SHIFTIN(aff0, GICR_TYPER_Affinity_Value_Aff0) |
	       __SHIFTIN(aff1, GICR_TYPER_Affinity_Value_Aff1) |
	       __SHIFTIN(aff2, GICR_TYPER_Affinity_Value_Aff2) |
	       __SHIFTIN(aff3, GICR_TYPER_Affinity_Value_Aff3);
}

static u_int
gicv3_find_redist(struct gicv3_softc *sc)
{
	uint64_t gicr_typer;
	u_int n;

	const uint64_t cpu_identity = gicv3_cpu_identity();

	for (n = 0; n < sc->sc_bsh_r_count; n++) {
		gicr_typer = gicr_read_8(sc, n, GICR_TYPER);
		if ((gicr_typer & GICR_TYPER_Affinity_Value) == cpu_identity)
			return n;
	}

	const u_int aff0 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff0);
	const u_int aff1 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff1);
	const u_int aff2 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff2);
	const u_int aff3 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff3);

	panic("%s: could not find GICv3 redistributor for cpu %d.%d.%d.%d",
	    cpu_name(curcpu()), aff3, aff2, aff1, aff0);
}

static uint64_t
gicv3_sgir(struct gicv3_softc *sc)
{
	const uint64_t cpu_identity = gicv3_cpu_identity();	

	const u_int aff0 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff0);
	const u_int aff1 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff1);
	const u_int aff2 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff2);
	const u_int aff3 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff3);

	return __SHIFTIN(__BIT(aff0), ICC_SGIR_EL1_TargetList) |
	       __SHIFTIN(aff1, ICC_SGIR_EL1_Aff1) |
	       __SHIFTIN(aff2, ICC_SGIR_EL1_Aff2) |
	       __SHIFTIN(aff3, ICC_SGIR_EL1_Aff3); 
}

static void
gicv3_cpu_init(struct pic_softc *pic, struct cpu_info *ci)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);
	uint32_t icc_sre, icc_ctlr, gicr_waker;

	ci->ci_gic_redist = gicv3_find_redist(sc);
	ci->ci_gic_sgir = gicv3_sgir(sc);

	/* Store route to CPU for SPIs */
	const uint64_t cpu_identity = gicv3_cpu_identity();
	const u_int aff0 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff0);
	const u_int aff1 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff1);
	const u_int aff2 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff2);
	const u_int aff3 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff3);
	sc->sc_irouter[cpu_index(ci)] =
	    __SHIFTIN(aff0, GICD_IROUTER_Aff0) |
	    __SHIFTIN(aff1, GICD_IROUTER_Aff1) |
	    __SHIFTIN(aff2, GICD_IROUTER_Aff2) |
	    __SHIFTIN(aff3, GICD_IROUTER_Aff3);

	/* Enable System register access and disable IRQ/FIQ bypass */
	icc_sre = ICC_SRE_EL1_SRE | ICC_SRE_EL1_DFB | ICC_SRE_EL1_DIB;
	icc_sre_write(icc_sre);

	/* Mark the connected PE as being awake */
	gicr_waker = gicr_read_4(sc, ci->ci_gic_redist, GICR_WAKER);
	gicr_waker &= ~GICR_WAKER_ProcessorSleep;
	gicr_write_4(sc, ci->ci_gic_redist, GICR_WAKER, gicr_waker);
	while (gicr_read_4(sc, ci->ci_gic_redist, GICR_WAKER) & GICR_WAKER_ChildrenAsleep)
		;

	/* Set initial priority mask */
	gicv3_set_priority(pic, IPL_HIGH);

	/* Set the binary point field to the minimum value */
	icc_bpr1_write(0);

	/* Enable group 1 interrupt signaling */
	icc_igrpen1_write(ICC_IGRPEN_EL1_Enable);

	/* Set EOI mode */
	icc_ctlr = icc_ctlr_read();
	icc_ctlr &= ~ICC_CTLR_EL1_EOImode;
	icc_ctlr_write(icc_ctlr);

	/* Enable redistributor */
	gicv3_redist_enable(sc, ci);

	/* Allow IRQ exceptions */
	cpsie(I32_bit);
}

#ifdef MULTIPROCESSOR
static void
gicv3_ipi_send(struct pic_softc *pic, const kcpuset_t *kcp, u_long ipi)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	uint64_t intid, aff, targets;

	intid = __SHIFTIN(ipi, ICC_SGIR_EL1_INTID);
	if (kcp == NULL) {
		/* Interrupts routed to all PEs, excluding "self" */
		if (ncpu == 1)
			return;
		icc_sgi1r_write(intid | ICC_SGIR_EL1_IRM);
	} else {
		/* Interrupts routed to specific PEs */
		aff = 0;
		targets = 0;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (!kcpuset_isset(kcp, cpu_index(ci)))
				continue;
			if ((ci->ci_gic_sgir & ICC_SGIR_EL1_Aff) != aff) {
				if (targets != 0) {
					icc_sgi1r_write(intid | aff | targets);
					arm_isb();
					targets = 0;
				}
				aff = (ci->ci_gic_sgir & ICC_SGIR_EL1_Aff);
			}
			targets |= (ci->ci_gic_sgir & ICC_SGIR_EL1_TargetList);
		}
		if (targets != 0) {
			icc_sgi1r_write(intid | aff | targets);
			arm_isb();
		}
	}
}

static void
gicv3_get_affinity(struct pic_softc *pic, size_t irq, kcpuset_t *affinity)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irq / 32;
	int n;

	kcpuset_zero(affinity);
	if (group == 0) {
		/* All CPUs are targets for group 0 (SGI/PPI) */
		for (n = 0; n < ncpu; n++) {
			if (sc->sc_irouter[n] != UINT64_MAX)
				kcpuset_set(affinity, n);
		}
	} else {
		/* Find distributor targets (SPI) */
		const uint64_t irouter = gicd_read_8(sc, GICD_IROUTER(irq));
		for (n = 0; n < ncpu; n++) {
			if (irouter == GICD_IROUTER_Interrupt_Routing_mode ||
			    irouter == sc->sc_irouter[n])
				kcpuset_set(affinity, n);
		}
	}
}

static int
gicv3_set_affinity(struct pic_softc *pic, size_t irq, const kcpuset_t *affinity)
{
	struct gicv3_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irq / 32;
	uint64_t irouter;

	if (group == 0)
		return EINVAL;

	const int set = kcpuset_countset(affinity);
	if (set == ncpu)
		irouter = GICD_IROUTER_Interrupt_Routing_mode;
	else if (set == 1)
		irouter = sc->sc_irouter[kcpuset_ffs(affinity) - 1];
	else
		return EINVAL;

	gicd_write_8(sc, GICD_IROUTER(irq), irouter);

	return 0;
}
#endif

static const struct pic_ops gicv3_picops = {
	.pic_unblock_irqs = gicv3_unblock_irqs,
	.pic_block_irqs = gicv3_block_irqs,
	.pic_establish_irq = gicv3_establish_irq,
	.pic_set_priority = gicv3_set_priority,
#ifdef MULTIPROCESSOR
	.pic_cpu_init = gicv3_cpu_init,
	.pic_ipi_send = gicv3_ipi_send,
	.pic_get_affinity = gicv3_get_affinity,
	.pic_set_affinity = gicv3_set_affinity,
#endif
};

static void
gicv3_lpi_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct gicv3_softc * const sc = LPITOSOFTC(pic);
	int bit;

	while ((bit = ffs(mask)) != 0) {
		sc->sc_lpiconf.base[irqbase + bit - 1] |= GIC_LPICONF_Enable;
		if (sc->sc_lpiconf_flush)
			cpu_dcache_wb_range((vaddr_t)&sc->sc_lpiconf.base[irqbase + bit - 1], 1);
		mask &= ~__BIT(bit - 1);
	}

	if (!sc->sc_lpiconf_flush)
		__asm __volatile ("dsb ishst");
}

static void
gicv3_lpi_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct gicv3_softc * const sc = LPITOSOFTC(pic);
	int bit;

	while ((bit = ffs(mask)) != 0) {
		sc->sc_lpiconf.base[irqbase + bit - 1] &= ~GIC_LPICONF_Enable;
		if (sc->sc_lpiconf_flush)
			cpu_dcache_wb_range((vaddr_t)&sc->sc_lpiconf.base[irqbase + bit - 1], 1);
		mask &= ~__BIT(bit - 1);
	}

	if (!sc->sc_lpiconf_flush)
		__asm __volatile ("dsb ishst");
}

static void
gicv3_lpi_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct gicv3_softc * const sc = LPITOSOFTC(pic);

	sc->sc_lpiconf.base[is->is_irq] = IPL_TO_LPIPRIO(sc, is->is_ipl) | GIC_LPICONF_Res1;

	if (sc->sc_lpiconf_flush)
		cpu_dcache_wb_range((vaddr_t)&sc->sc_lpiconf.base[is->is_irq], 1);
	else
		__asm __volatile ("dsb ishst");
}

static void
gicv3_lpi_cpu_init(struct pic_softc *pic, struct cpu_info *ci)
{
	struct gicv3_softc * const sc = LPITOSOFTC(pic);
	struct gicv3_lpi_callback *cb;
	uint64_t propbase, pendbase;
	uint32_t ctlr;

	/* If physical LPIs are not supported on this redistributor, just return. */
	const uint64_t typer = gicr_read_8(sc, ci->ci_gic_redist, GICR_TYPER);
	if ((typer & GICR_TYPER_PLPIS) == 0)
		return;

	/* Interrupt target address for this CPU, used by ITS when GITS_TYPER.PTA == 0 */
	sc->sc_processor_id[cpu_index(ci)] = __SHIFTOUT(typer, GICR_TYPER_Processor_Number);

	/* Disable LPIs before making changes */
	ctlr = gicr_read_4(sc, ci->ci_gic_redist, GICR_CTLR);
	ctlr &= ~GICR_CTLR_Enable_LPIs;
	gicr_write_4(sc, ci->ci_gic_redist, GICR_CTLR, ctlr);
	arm_dsb();

	/* Setup the LPI configuration table */
	propbase = sc->sc_lpiconf.segs[0].ds_addr |
	    __SHIFTIN(ffs(pic->pic_maxsources) - 1, GICR_PROPBASER_IDbits) |
	    __SHIFTIN(GICR_Shareability_IS, GICR_PROPBASER_Shareability) |
	    __SHIFTIN(GICR_Cache_NORMAL_RA_WA_WB, GICR_PROPBASER_InnerCache);
	gicr_write_8(sc, ci->ci_gic_redist, GICR_PROPBASER, propbase);
	propbase = gicr_read_8(sc, ci->ci_gic_redist, GICR_PROPBASER);
	if (__SHIFTOUT(propbase, GICR_PROPBASER_Shareability) != GICR_Shareability_IS) {
		if (__SHIFTOUT(propbase, GICR_PROPBASER_Shareability) == GICR_Shareability_NS) {
			propbase &= ~GICR_PROPBASER_Shareability;
			propbase |= __SHIFTIN(GICR_Shareability_NS, GICR_PROPBASER_Shareability);
			propbase &= ~GICR_PROPBASER_InnerCache;
			propbase |= __SHIFTIN(GICR_Cache_NORMAL_NC, GICR_PROPBASER_InnerCache);
			gicr_write_8(sc, ci->ci_gic_redist, GICR_PROPBASER, propbase);
		}
		sc->sc_lpiconf_flush = true;
	}

	/* Setup the LPI pending table */
	pendbase = sc->sc_lpipend[cpu_index(ci)].segs[0].ds_addr |
	    __SHIFTIN(GICR_Shareability_IS, GICR_PENDBASER_Shareability) |
	    __SHIFTIN(GICR_Cache_NORMAL_RA_WA_WB, GICR_PENDBASER_InnerCache);
	gicr_write_8(sc, ci->ci_gic_redist, GICR_PENDBASER, pendbase);
	pendbase = gicr_read_8(sc, ci->ci_gic_redist, GICR_PENDBASER);
	if (__SHIFTOUT(pendbase, GICR_PENDBASER_Shareability) == GICR_Shareability_NS) {
		pendbase &= ~GICR_PENDBASER_Shareability;
		pendbase |= __SHIFTIN(GICR_Shareability_NS, GICR_PENDBASER_Shareability);
		pendbase &= ~GICR_PENDBASER_InnerCache;
		pendbase |= __SHIFTIN(GICR_Cache_NORMAL_NC, GICR_PENDBASER_InnerCache);
		gicr_write_8(sc, ci->ci_gic_redist, GICR_PENDBASER, pendbase);
	}

	/* Enable LPIs */
	ctlr = gicr_read_4(sc, ci->ci_gic_redist, GICR_CTLR);
	ctlr |= GICR_CTLR_Enable_LPIs;
	gicr_write_4(sc, ci->ci_gic_redist, GICR_CTLR, ctlr);
	arm_dsb();

	/* Setup ITS if present */
	LIST_FOREACH(cb, &sc->sc_lpi_callbacks, list)
		cb->cpu_init(cb->priv, ci);
}

#ifdef MULTIPROCESSOR
static void
gicv3_lpi_get_affinity(struct pic_softc *pic, size_t irq, kcpuset_t *affinity)
{
	struct gicv3_softc * const sc = LPITOSOFTC(pic);
	struct gicv3_lpi_callback *cb;

	LIST_FOREACH(cb, &sc->sc_lpi_callbacks, list)
		cb->get_affinity(cb->priv, irq, affinity);
}

static int
gicv3_lpi_set_affinity(struct pic_softc *pic, size_t irq, const kcpuset_t *affinity)
{
	struct gicv3_softc * const sc = LPITOSOFTC(pic);
	struct gicv3_lpi_callback *cb;
	int error = EINVAL;

	LIST_FOREACH(cb, &sc->sc_lpi_callbacks, list) {
		error = cb->set_affinity(cb->priv, irq, affinity);
		if (error)
			return error;
	}

	return error;
}
#endif

static const struct pic_ops gicv3_lpiops = {
	.pic_unblock_irqs = gicv3_lpi_unblock_irqs,
	.pic_block_irqs = gicv3_lpi_block_irqs,
	.pic_establish_irq = gicv3_lpi_establish_irq,
#ifdef MULTIPROCESSOR
	.pic_cpu_init = gicv3_lpi_cpu_init,
	.pic_get_affinity = gicv3_lpi_get_affinity,
	.pic_set_affinity = gicv3_lpi_set_affinity,
#endif
};

void
gicv3_dma_alloc(struct gicv3_softc *sc, struct gicv3_dma *dma, bus_size_t len, bus_size_t align)
{
	int nsegs, error;

	dma->len = len;
	error = bus_dmamem_alloc(sc->sc_dmat, dma->len, align, 0, dma->segs, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		panic("bus_dmamem_alloc failed: %d", error);
	error = bus_dmamem_map(sc->sc_dmat, dma->segs, nsegs, len, (void **)&dma->base, BUS_DMA_WAITOK);
	if (error)
		panic("bus_dmamem_map failed: %d", error);
	error = bus_dmamap_create(sc->sc_dmat, len, 1, len, 0, BUS_DMA_WAITOK, &dma->map);
	if (error)
		panic("bus_dmamap_create failed: %d", error);
	error = bus_dmamap_load(sc->sc_dmat, dma->map, dma->base, dma->len, NULL, BUS_DMA_WAITOK);
	if (error)
		panic("bus_dmamap_load failed: %d", error);

	memset(dma->base, 0, dma->len);
	bus_dmamap_sync(sc->sc_dmat, dma->map, 0, dma->len, BUS_DMASYNC_PREWRITE);
}

static void
gicv3_lpi_init(struct gicv3_softc *sc)
{
	/*
	 * Allocate LPI configuration table
	 */
	gicv3_dma_alloc(sc, &sc->sc_lpiconf, sc->sc_lpi.pic_maxsources, 0x1000);
	KASSERT((sc->sc_lpiconf.segs[0].ds_addr & ~GICR_PROPBASER_Physical_Address) == 0);

	/*
	 * Allocate LPI pending tables
	 */
	const bus_size_t lpipend_sz = (8192 + sc->sc_lpi.pic_maxsources) / NBBY;
	for (int cpuindex = 0; cpuindex < ncpu; cpuindex++) {
		gicv3_dma_alloc(sc, &sc->sc_lpipend[cpuindex], lpipend_sz, 0x10000);
		KASSERT((sc->sc_lpipend[cpuindex].segs[0].ds_addr & ~GICR_PENDBASER_Physical_Address) == 0);
	}
}

void
gicv3_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct gicv3_softc * const sc = gicv3_softc;
	struct pic_softc *pic;
	const int oldipl = ci->ci_cpl;

	ci->ci_data.cpu_nintr++;

	for (;;) {
		const uint32_t iar = icc_iar1_read();
		arm_dsb();
		const uint32_t irq = __SHIFTOUT(iar, ICC_IAR_INTID);
		if (irq == ICC_IAR_INTID_SPURIOUS)
			break;

		pic = irq >= GIC_LPI_BASE ? &sc->sc_lpi : &sc->sc_pic;
		if (irq - pic->pic_irqbase >= pic->pic_maxsources)
			continue;

		struct intrsource * const is = pic->pic_sources[irq - pic->pic_irqbase];
		KASSERT(is != NULL);

		const bool early_eoi = irq < GIC_LPI_BASE && is->is_type == IST_EDGE;

		const int ipl = is->is_ipl;
		if (__predict_false(ipl < ci->ci_cpl)) {
			pic_do_pending_ints(I32_bit, ipl, frame);
		} else {
			gicv3_set_priority(pic, ipl);
			ci->ci_cpl = ipl;
		}

		if (early_eoi) {
			icc_eoi1r_write(iar);
			arm_isb();
		}

		cpsie(I32_bit);
		pic_dispatch(is, frame);
		cpsid(I32_bit);

		if (!early_eoi) {
			icc_eoi1r_write(iar);
			arm_isb();
		}
	}

	pic_do_pending_ints(I32_bit, oldipl, frame);
}

static int
gicv3_detect_pmr_bits(struct gicv3_softc *sc)
{
	const uint32_t opmr = icc_pmr_read();
	icc_pmr_write(0xbf);
	const uint32_t npmr = icc_pmr_read();
	icc_pmr_write(opmr);

	return NBBY - (ffs(npmr) - 1);
}

static int
gicv3_detect_ipriority_bits(struct gicv3_softc *sc)
{
	const uint32_t oipriorityr = gicd_read_4(sc, GICD_IPRIORITYRn(8));
	gicd_write_4(sc, GICD_IPRIORITYRn(8), oipriorityr | 0xff);
	const uint32_t nipriorityr = gicd_read_4(sc, GICD_IPRIORITYRn(8));
	gicd_write_4(sc, GICD_IPRIORITYRn(8), oipriorityr);

	return NBBY - (ffs(nipriorityr & 0xff) - 1);
}

int
gicv3_init(struct gicv3_softc *sc)
{
	const uint32_t gicd_typer = gicd_read_4(sc, GICD_TYPER);
	const uint32_t gicd_ctrl = gicd_read_4(sc, GICD_CTRL);
	int n;

	KASSERT(CPU_IS_PRIMARY(curcpu()));

	LIST_INIT(&sc->sc_lpi_callbacks);

	for (n = 0; n < MAXCPUS; n++)
		sc->sc_irouter[n] = UINT64_MAX;

	sc->sc_priority_shift = 4;
	sc->sc_pmr_shift = 4;

	if ((gicd_ctrl & GICD_CTRL_DS) == 0) {
		const int pmr_bits = gicv3_detect_pmr_bits(sc);
		const int ipriority_bits = gicv3_detect_ipriority_bits(sc);

		if (ipriority_bits != pmr_bits)
			--sc->sc_priority_shift;

		aprint_verbose_dev(sc->sc_dev, "%d pmr bits, %d ipriority bits\n",
		    pmr_bits, ipriority_bits);
	} else {
		aprint_verbose_dev(sc->sc_dev, "security disabled\n");
	}

	aprint_verbose_dev(sc->sc_dev, "priority shift %d, pmr shift %d\n",
	    sc->sc_priority_shift, sc->sc_pmr_shift);

	sc->sc_pic.pic_ops = &gicv3_picops;
	sc->sc_pic.pic_maxsources = GICD_TYPER_LINES(gicd_typer);
	snprintf(sc->sc_pic.pic_name, sizeof(sc->sc_pic.pic_name), "gicv3");
#ifdef MULTIPROCESSOR
	sc->sc_pic.pic_cpus = kcpuset_running;
#endif
	pic_add(&sc->sc_pic, 0);

	if ((gicd_typer & GICD_TYPER_LPIS) != 0) {
		sc->sc_lpi.pic_ops = &gicv3_lpiops;
		sc->sc_lpi.pic_maxsources = 8192;	/* Min. required by GICv3 spec */
		snprintf(sc->sc_lpi.pic_name, sizeof(sc->sc_lpi.pic_name), "gicv3-lpi");
		pic_add(&sc->sc_lpi, GIC_LPI_BASE);

		gicv3_lpi_init(sc);
	}

	KASSERT(gicv3_softc == NULL);
	gicv3_softc = sc;

	for (int i = 0; i < sc->sc_bsh_r_count; i++) {
		const uint64_t gicr_typer = gicr_read_8(sc, i, GICR_TYPER);
		const u_int aff0 = __SHIFTOUT(gicr_typer, GICR_TYPER_Affinity_Value_Aff0);
		const u_int aff1 = __SHIFTOUT(gicr_typer, GICR_TYPER_Affinity_Value_Aff1);
		const u_int aff2 = __SHIFTOUT(gicr_typer, GICR_TYPER_Affinity_Value_Aff2);
		const u_int aff3 = __SHIFTOUT(gicr_typer, GICR_TYPER_Affinity_Value_Aff3);

		aprint_debug_dev(sc->sc_dev, "redist %d: cpu %d.%d.%d.%d\n",
		    i, aff3, aff2, aff1, aff0);
	}

	gicv3_dist_enable(sc);

	gicv3_cpu_init(&sc->sc_pic, curcpu());
	if ((gicd_typer & GICD_TYPER_LPIS) != 0)
		gicv3_lpi_cpu_init(&sc->sc_lpi, curcpu());

#ifdef __HAVE_PIC_FAST_SOFTINTS
	intr_establish_xname(SOFTINT_BIO, IPL_SOFTBIO, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_BIO, "softint bio");
	intr_establish_xname(SOFTINT_CLOCK, IPL_SOFTCLOCK, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_CLOCK, "softint clock");
	intr_establish_xname(SOFTINT_NET, IPL_SOFTNET, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_NET, "softint net");
	intr_establish_xname(SOFTINT_SERIAL, IPL_SOFTSERIAL, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_SERIAL, "softint serial");
#endif

#ifdef MULTIPROCESSOR
	intr_establish_xname(IPI_AST, IPL_VM, IST_MPSAFE | IST_EDGE, pic_ipi_ast, (void *)-1, "IPI ast");
	intr_establish_xname(IPI_XCALL, IPL_HIGH, IST_MPSAFE | IST_EDGE, pic_ipi_xcall, (void *)-1, "IPI xcall");
	intr_establish_xname(IPI_GENERIC, IPL_HIGH, IST_MPSAFE | IST_EDGE, pic_ipi_generic, (void *)-1, "IPI generic");
	intr_establish_xname(IPI_NOP, IPL_VM, IST_MPSAFE | IST_EDGE, pic_ipi_nop, (void *)-1, "IPI nop");
	intr_establish_xname(IPI_SHOOTDOWN, IPL_SCHED, IST_MPSAFE | IST_EDGE, pic_ipi_shootdown, (void *)-1, "IPI shootdown");
#ifdef DDB
	intr_establish_xname(IPI_DDB, IPL_HIGH, IST_MPSAFE | IST_EDGE, pic_ipi_ddb, NULL, "IPI ddb");
#endif
#ifdef __HAVE_PREEMPTION
	intr_establish_xname(IPI_KPREEMPT, IPL_VM, IST_MPSAFE | IST_EDGE, pic_ipi_kpreempt, (void *)-1, "IPI kpreempt");
#endif
#endif

	return 0;
}
