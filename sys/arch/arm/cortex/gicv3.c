/* $NetBSD: gicv3.c,v 1.2 2018/08/11 00:32:17 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gicv3.c,v 1.2 2018/08/11 00:32:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <arm/locore.h>
#include <arm/armreg.h>

#include <arm/cortex/gicv3.h>
#include <arm/cortex/gic_reg.h>

#define	PICTOSOFTC(pic)	\
	((void *)((uintptr_t)(pic) - offsetof(struct gicv3_softc, sc_pic)))

#define	IPL_TO_PRIORITY(ipl)	((IPL_HIGH - (ipl)) << 4)

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
		while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTRL) & GICR_CTRL_RWP)
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
		while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTRL) & GICR_CTRL_RWP)
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

	const u_int ipriority_val = 0x80 | IPL_TO_PRIORITY(is->is_ipl);
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
			irouter = sc->sc_default_irouter;
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
	icc_pmr_write(IPL_TO_PRIORITY(ipl));
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
	gicd_ctrl = GICD_CTRL_EnableGrp1NS | GICD_CTRL_Enable | GICD_CTRL_ARE_NS;
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
	while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTRL) & GICR_CTRL_RWP)
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
				const u_int ipriority_val = 0x80 | IPL_TO_PRIORITY(is->is_ipl);
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
	while (gicr_read_4(sc, ci->ci_gic_redist, GICR_CTRL) & GICR_CTRL_RWP)
		;
}

static uint64_t
gicv3_cpu_identity(void)
{
	u_int aff3, aff2, aff1, aff0;

#ifdef __aarch64__
	const register_t mpidr = reg_mpidr_el1_read();
	aff0 = __SHIFTOUT(mpidr, MPIDR_AFF0);
	aff1 = __SHIFTOUT(mpidr, MPIDR_AFF1);
	aff2 = __SHIFTOUT(mpidr, MPIDR_AFF2);
	aff3 = __SHIFTOUT(mpidr, MPIDR_AFF3);
#else
	const register_t mpidr = armreg_mpidr_read();
	aff0 = __SHIFTOUT(mpidr, MPIDR_AFF0);
	aff1 = __SHIFTOUT(mpidr, MPIDR_AFF1);
	aff2 = __SHIFTOUT(mpidr, MPIDR_AFF2);
	aff3 = 0;
#endif

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

	if (CPU_IS_PRIMARY(ci)) {
		/* Store route to primary CPU for non-MPSAFE SPIs */
		const uint64_t cpu_identity = gicv3_cpu_identity();
		const u_int aff0 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff0);
		const u_int aff1 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff1);
		const u_int aff2 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff2);
		const u_int aff3 = __SHIFTOUT(cpu_identity, GICR_TYPER_Affinity_Value_Aff3);
		sc->sc_default_irouter =
		    __SHIFTIN(aff0, GICD_IROUTER_Aff0) |
		    __SHIFTIN(aff1, GICD_IROUTER_Aff1) |
		    __SHIFTIN(aff2, GICD_IROUTER_Aff2) |
		    __SHIFTIN(aff3, GICD_IROUTER_Aff3);
	}

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
	icc_pmr_write(IPL_TO_PRIORITY(IPL_HIGH));

	/* Disable preemption */
	const uint32_t icc_bpr = __SHIFTIN(0x7, ICC_BPR_EL1_BinaryPoint);
	icc_bpr1_write(icc_bpr);

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
					targets = 0;
				}
				aff = (ci->ci_gic_sgir & ICC_SGIR_EL1_Aff);
			}
			targets |= (ci->ci_gic_sgir & ICC_SGIR_EL1_TargetList);
		}
		if (targets != 0)
			icc_sgi1r_write(intid | aff | targets);
	}
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
#endif
};

void
gicv3_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct gicv3_softc * const sc = gicv3_softc;
	const int oldipl = ci->ci_cpl;

	ci->ci_data.cpu_nintr++;

	for (;;) {
		const uint32_t iar = icc_iar1_read();
		const uint32_t irq = __SHIFTOUT(iar, ICC_IAR_INTID);
		if (irq == ICC_IAR_INTID_SPURIOUS)
			break;

		if (irq >= sc->sc_pic.pic_maxsources)
			continue;

		struct intrsource * const is = sc->sc_pic.pic_sources[irq];
		KASSERT(is != NULL);

		const int ipl = is->is_ipl;
		if (ci->ci_cpl < ipl)
			pic_set_priority(ci, ipl);

		cpsie(I32_bit);
		pic_dispatch(is, frame);
		cpsid(I32_bit);

		icc_eoi1r_write(iar);
	}

	if (ci->ci_cpl != oldipl)
		pic_set_priority(ci, oldipl);
}

int
gicv3_init(struct gicv3_softc *sc)
{
	const uint32_t gicd_typer = gicd_read_4(sc, GICD_TYPER);

	KASSERT(CPU_IS_PRIMARY(curcpu()));

	sc->sc_pic.pic_ops = &gicv3_picops;
	sc->sc_pic.pic_maxsources = GICD_TYPER_LINES(gicd_typer);
	snprintf(sc->sc_pic.pic_name, sizeof(sc->sc_pic.pic_name), "gicv3");
#ifdef MULTIPROCESSOR
	sc->sc_pic.pic_cpus = kcpuset_running;
#endif
	pic_add(&sc->sc_pic, 0);

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

#ifdef __HAVE_PIC_FAST_SOFTINTS
	intr_establish(SOFTINT_BIO, IPL_SOFTBIO, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_BIO);
	intr_establish(SOFTINT_CLOCK, IPL_SOFTCLOCK, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_CLOCK);
	intr_establish(SOFTINT_NET, IPL_SOFTNET, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_NET);
	intr_establish(SOFTINT_SERIAL, IPL_SOFTSERIAL, IST_MPSAFE | IST_EDGE, pic_handle_softint, (void *)SOFTINT_SERIAL);
#endif

#ifdef MULTIPROCESSOR
	intr_establish(IPI_AST, IPL_VM, IST_MPSAFE | IST_EDGE, pic_ipi_ast, (void *)-1);
	intr_establish(IPI_XCALL, IPL_HIGH, IST_MPSAFE | IST_EDGE, pic_ipi_xcall, (void *)-1);
	intr_establish(IPI_GENERIC, IPL_HIGH, IST_MPSAFE | IST_EDGE, pic_ipi_generic, (void *)-1);
	intr_establish(IPI_NOP, IPL_VM, IST_MPSAFE | IST_EDGE, pic_ipi_nop, (void *)-1);
	intr_establish(IPI_SHOOTDOWN, IPL_SCHED, IST_MPSAFE | IST_EDGE, pic_ipi_shootdown, (void *)-1);
#ifdef DDB
	intr_establish(IPI_DDB, IPL_HIGH, IST_MPSAFE | IST_EDGE, pic_ipi_ddb, NULL);
#endif
#ifdef __HAVE_PREEMPTION
	intr_establish(IPI_KPREEMPT, IPL_VM, IST_MPSAFE | IST_EDGE, pic_ipi_kpreempt, (void *)-1);
#endif
#endif

	return 0;
}
