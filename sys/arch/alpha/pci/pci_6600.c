/* $NetBSD: pci_6600.c,v 1.33 2021/07/04 22:42:36 thorpej Exp $ */

/*-
 * Copyright (c) 1999 by Ross Harvey.  All rights reserved.
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
 *	This product includes software developed by Ross Harvey.
 * 4. The name of Ross Harvey may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROSS HARVEY ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURP0SE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ROSS HARVEY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pci_6600.c,v 1.33 2021/07/04 22:42:36 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <machine/autoconf.h>
#define _ALPHA_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/rpb.h>
#include <machine/alpha.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#define pci_6600() { Generate ctags(1) key. }

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

#define	PCI_NIRQ		64
#define	PCI_SIO_IRQ		55
#define	PCI_STRAY_MAX		5

/*
 * Some Tsunami models have a PCI device (the USB controller) with interrupts
 * tied to ISA IRQ lines.  The IRQ is encoded as:
 *
 *	line = 0xe0 | isa_irq;
 */
#define	DEC_6600_LINE_IS_ISA(line)	((line) >= 0xe0 && (line) <= 0xef)
#define	DEC_6600_LINE_ISA_IRQ(line)	((line) & 0x0f)

static struct tsp_config *sioprimary;

static void	 dec_6600_intr_disestablish(pci_chipset_tag_t, void *);
static void	 *dec_6600_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*func)(void *), void *);
static const char *dec_6600_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *dec_6600_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static int	dec_6600_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

static void	 *dec_6600_pciide_compat_intr_establish(device_t,
		    const struct pci_attach_args *, int,
		    int (*)(void *), void *);

static void	dec_6600_intr_enable(pci_chipset_tag_t, int irq);
static void	dec_6600_intr_disable(pci_chipset_tag_t, int irq);
static void	dec_6600_intr_set_affinity(pci_chipset_tag_t, int,
		    struct cpu_info *);
static void	dec_6600_intr_program(pci_chipset_tag_t);

static void	dec_6600_intr_redistribute(void);

/*
 * We keep 2 software copies of the interrupt enables: one global one,
 * and one per-CPU for setting the interrupt affinity.
 */
static uint64_t	dec_6600_intr_enables __read_mostly;
static uint64_t dec_6600_cpu_intr_enables[4] __read_mostly;

static void
pci_6600_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_6600_intr_map;
	pc->pc_intr_string = dec_6600_intr_string;
	pc->pc_intr_evcnt = dec_6600_intr_evcnt;
	pc->pc_intr_establish = dec_6600_intr_establish;
	pc->pc_intr_disestablish = dec_6600_intr_disestablish;

	pc->pc_pciide_compat_intr_establish = NULL;

	pc->pc_intr_desc = "dec 6600";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = PCI_NIRQ;

	pc->pc_intr_enable = dec_6600_intr_enable;
	pc->pc_intr_disable = dec_6600_intr_disable;
	pc->pc_intr_set_affinity = dec_6600_intr_set_affinity;

	alpha_intr_redistribute = dec_6600_intr_redistribute;

	/* Note eligible CPUs for interrupt routing purposes. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		KASSERT(ci->ci_cpuid < 4);
		pc->pc_eligible_cpus |= __BIT(ci->ci_cpuid);
	}

	/*
	 * System-wide and Pchip-0-only logic...
	 */
	if (sioprimary == NULL) {
		sioprimary = core;
		/*
		 * Unless explicitly routed, all interrupts go to the
		 * primary CPU.
		 */
		dec_6600_cpu_intr_enables[cpu_info_primary.ci_cpuid] =
		    __BITS(0,63);
		pc->pc_pciide_compat_intr_establish =
		    dec_6600_pciide_compat_intr_establish;

		KASSERT(dec_6600_intr_enables == 0);
		dec_6600_intr_program(pc);

		alpha_pci_intr_alloc(pc, PCI_STRAY_MAX);
#if NSIO
		sio_intr_setup(pc, iot);

		mutex_enter(&cpu_lock);
		dec_6600_intr_enable(pc, PCI_SIO_IRQ);
		mutex_exit(&cpu_lock);
#endif
	} else {
		pc->pc_shared_intrs = sioprimary->pc_pc.pc_shared_intrs;
	}
}
ALPHA_PCI_INTR_INIT(ST_DEC_6600, pci_6600_pickintr)
ALPHA_PCI_INTR_INIT(ST_DEC_TITAN, pci_6600_pickintr)

static int
dec_6600_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin, line = pa->pa_intrline;
	pci_chipset_tag_t pc = pa->pa_pc;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("dec_6600_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

#if NSIO == 0
	if (DEC_6600_LINE_IS_ISA(line)) {
		printf("dec_6600_intr_map: ISA IRQ %d for %d/%d/%d\n",
		    DEC_6600_LINE_ISA_IRQ(line), bus, device, function);
		return (1);
	}
#endif

	if (DEC_6600_LINE_IS_ISA(line) == 0 && line >= PCI_NIRQ)
		panic("dec_6600_intr_map: dec 6600 irq too large (%d)",
		    line);

	alpha_pci_intr_handle_init(ihp, line, 0);
	return (0);
}

static const char *
dec_6600_intr_string(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    char * const buf, size_t const len)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	if (DEC_6600_LINE_IS_ISA(irq))
		return (sio_intr_string(NULL /*XXX*/,
		    DEC_6600_LINE_ISA_IRQ(irq), buf, len));
#endif

	return alpha_pci_generic_intr_string(pc, ih, buf, len);
}

static const struct evcnt *
dec_6600_intr_evcnt(pci_chipset_tag_t const pc, pci_intr_handle_t const ih)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	if (DEC_6600_LINE_IS_ISA(irq))
		return (sio_intr_evcnt(NULL /*XXX*/,
		    DEC_6600_LINE_ISA_IRQ(irq)));
#endif

	return alpha_pci_generic_intr_evcnt(pc, ih);
}

static void *
dec_6600_intr_establish(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    int const level, int (*func)(void *), void *arg)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);
	const u_int flags = alpha_pci_intr_handle_get_flags(&ih);

	if (DEC_6600_LINE_IS_ISA(irq))
		return (sio_intr_establish(NULL /*XXX*/,
		    DEC_6600_LINE_ISA_IRQ(irq), IST_LEVEL, level, flags,
		    func, arg));
#endif

	return alpha_pci_generic_intr_establish(pc, ih, level, func, arg);
}

static void
dec_6600_intr_disestablish(pci_chipset_tag_t const pc, void * const cookie)
{
#if NSIO
	struct alpha_shared_intrhand * const ih = cookie;

	/*
	 * We have to determine if this is an ISA IRQ or not!  We do this
	 * by checking to see if the intrhand points back to an intrhead
	 * that points to the sioprimary TSP.  If not, it's an ISA IRQ.
	 * Pretty disgusting, eh?
	 */
	if (ih->ih_intrhead->intr_private != sioprimary) {
		sio_intr_disestablish(NULL /*XXX*/, cookie);
		return;
	}
#endif

	return alpha_pci_generic_intr_disestablish(pc, cookie);
}

static void
dec_6600_intr_program(pci_chipset_tag_t const pc)
{
	unsigned int irq, cpuno, cnt;

	/*
	 * Validate the configuration before we program it: each enabled
	 * IRQ must be routed to exactly one CPU.
	 */
	for (irq = 0; irq < PCI_NIRQ; irq++) {
		if ((dec_6600_intr_enables & __BIT(irq)) == 0)
			continue;
		for (cpuno = 0, cnt = 0; cpuno < 4; cpuno++) {
			if (dec_6600_cpu_intr_enables[cpuno] != 0 &&
			    (pc->pc_eligible_cpus & __BIT(cpuno)) == 0)
				panic("%s: interrupts enabled on non-existent CPU %u",
				    __func__, cpuno);
			if (dec_6600_cpu_intr_enables[cpuno] & __BIT(irq))
				cnt++;
		}
		if (cnt != 1) {
			panic("%s: irq %u enabled on %u CPUs", __func__,
			    irq, cnt);
		}
	}

	const uint64_t enab0 =
	    dec_6600_intr_enables & dec_6600_cpu_intr_enables[0];
	const uint64_t enab1 =
	    dec_6600_intr_enables & dec_6600_cpu_intr_enables[1];
	const uint64_t enab2 =
	    dec_6600_intr_enables & dec_6600_cpu_intr_enables[2];
	const uint64_t enab3 =
	    dec_6600_intr_enables & dec_6600_cpu_intr_enables[3];

	/* Don't touch DIMx registers for non-existent CPUs. */
	uint64_t black_hole;
	volatile uint64_t * const dim0 = (pc->pc_eligible_cpus & __BIT(0)) ?
	    (void *)ALPHA_PHYS_TO_K0SEG(TS_C_DIM0) : &black_hole;
	volatile uint64_t * const dim1 = (pc->pc_eligible_cpus & __BIT(1)) ?
	    (void *)ALPHA_PHYS_TO_K0SEG(TS_C_DIM1) : &black_hole;
	volatile uint64_t * const dim2 = (pc->pc_eligible_cpus & __BIT(2)) ?
	    (void *)ALPHA_PHYS_TO_K0SEG(TS_C_DIM2) : &black_hole;
	volatile uint64_t * const dim3 = (pc->pc_eligible_cpus & __BIT(3)) ?
	    (void *)ALPHA_PHYS_TO_K0SEG(TS_C_DIM3) : &black_hole;

	const unsigned long psl = alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);

	alpha_mb();
	*dim0 = enab0;
	*dim1 = enab1;
	*dim2 = enab2;
	*dim3 = enab3;
	alpha_mb();
	(void) *dim0;
	(void) *dim1;
	(void) *dim2;
	(void) *dim3;
	alpha_mb();

	alpha_pal_swpipl(psl);
}

static void
dec_6600_intr_enable(pci_chipset_tag_t const pc, int const irq)
{

	KASSERT(mutex_owned(&cpu_lock));

	dec_6600_intr_enables |= __BIT(irq);
	dec_6600_intr_program(pc);
}

static void
dec_6600_intr_disable(pci_chipset_tag_t const pc, int const irq)
{

	KASSERT(mutex_owned(&cpu_lock));

	dec_6600_intr_enables &= ~__BIT(irq);
	dec_6600_intr_program(pc);
}

static void
dec_6600_intr_set_affinity(pci_chipset_tag_t const pc, int const irq,
    struct cpu_info * const ci)
{
	const uint64_t intr_bit = __BIT(irq);
	cpuid_t cpuno;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(ci->ci_cpuid < 4);
	KASSERT(pc->pc_eligible_cpus & __BIT(ci->ci_cpuid));

	for (cpuno = 0; cpuno < 4; cpuno++) {
		if (cpuno == ci->ci_cpuid)
			dec_6600_cpu_intr_enables[cpuno] |= intr_bit;
		else
			dec_6600_cpu_intr_enables[cpuno] &= ~intr_bit;
	}

	/* Only program the hardware if the irq is enabled. */
	if (dec_6600_intr_enables & intr_bit)
		dec_6600_intr_program(pc);
}

static void
dec_6600_intr_redistribute(void)
{
	KASSERT(sioprimary != NULL);

	pci_chipset_tag_t const pc = &sioprimary->pc_pc;

	/* ISA interrupts always stay on primary. Shuffle PCI interrupts. */
	alpha_pci_generic_intr_redistribute(pc);
}

static void *
dec_6600_pciide_compat_intr_establish(device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t const pc = pa->pa_pc;

	/*
	 * If this isn't the TSP that holds the PCI-ISA bridge,
	 * all bets are off.
	 */
	if (pc->pc_intr_v != sioprimary)
		return (NULL);

	return sio_pciide_compat_intr_establish(dev, pa, chan, func, arg);
}
