/* $NetBSD: plic.c,v 1.1 2023/05/07 12:41:48 skrll Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Portions of this code is derived from software contributed to The NetBSD
 * Foundation by Simon Burge.
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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plic.c,v 1.1 2023/05/07 12:41:48 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kmem.h>

#include <riscv/sysreg.h>
#include <riscv/dev/plicreg.h>
#include <riscv/dev/plicvar.h>

#define	PLIC_PRIORITY(irq)	(PLIC_PRIORITY_BASE + (irq) * 4)

#define	PLIC_ENABLE(sc, c, irq)	(PLIC_ENABLE_BASE +		 	       \
				 sc->sc_context[(c)] * PLIC_ENABLE_SIZE +      \
				 ((irq / 32) * sizeof(uint32_t)))

#define	PLIC_CONTEXT(sc, c)	(PLIC_CONTEXT_BASE +			\
				 sc->sc_context[(c)] * PLIC_CONTEXT_SIZE)
#define	PLIC_CLAIM(sc, c)	(PLIC_CONTEXT(sc, c) + PLIC_CLAIM_COMPLETE_OFFS)
#define	PLIC_COMPLETE(sc, c)	PLIC_CLAIM(sc, c)	/* same address */
#define	PLIC_THRESHOLD(sc, c)	(PLIC_CONTEXT(sc, c) + PLIC_THRESHOLD_OFFS)

#define	PLIC_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PLIC_WRITE(sc, reg, val)					\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


struct plic_softc *plic_sc;


void *
plic_intr_establish_xname(u_int irq, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct plic_softc * const sc = plic_sc;
	struct plic_intrhand *ih;

	/* XXX need a better CPU selection method */
//	u_int cidx = cpu_index(curcpu());
	u_int cidx = 0;

	evcnt_attach_dynamic(&sc->sc_intrevs[irq], EVCNT_TYPE_INTR, NULL,
	    "plic", xname);

	ih = &sc->sc_intr[irq];
	KASSERTMSG(ih->ih_func == NULL,
	    "Oops, we need to chain PLIC interrupt handlers");
	if (ih->ih_func != NULL) {
		aprint_error_dev(sc->sc_dev, "irq slot %d already used\n", irq);
		return NULL;
	}
	ih->ih_mpsafe = (flags & IST_MPSAFE) != 0;
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_cidx = cidx;

	plic_set_priority(sc, irq, 1);
	plic_enable(sc, cidx, irq);

	return ih;
}

void
plic_intr_disestablish(void *cookie)
{
	struct plic_softc * const sc = plic_sc;
	struct plic_intrhand * const ih = cookie;
	const u_int cidx = ih->ih_cidx;
	const u_int irq = ih->ih_irq;

	plic_disable(sc, cidx, irq);
	plic_set_priority(sc, irq, 0);

	memset(&sc->sc_intr[irq], 0, sizeof(*sc->sc_intr));
}

int
plic_intr(void *arg)
{
	struct plic_softc * const sc = arg;
	const cpuid_t cpuid = cpu_number();
	const bus_addr_t claim_addr = PLIC_CLAIM(sc, cpuid);
	const bus_addr_t complete_addr = PLIC_COMPLETE(sc, cpuid);
	uint32_t pending;
	int rv = 0;

	while ((pending = PLIC_READ(sc, claim_addr)) > 0) {
		struct plic_intrhand *ih = &sc->sc_intr[pending];

		sc->sc_intrevs[pending].ev_count++;

		KASSERT(ih->ih_func != NULL);
#ifdef MULTIPROCESSOR
		if (!ih->ih_mpsafe) {
			KERNEL_LOCK(1, NULL);
			rv |= ih->ih_func(ih->ih_arg);
			KERNEL_UNLOCK_ONE(NULL);
		} else
#endif
			rv |= ih->ih_func(ih->ih_arg);

		PLIC_WRITE(sc, complete_addr, pending);
	}

	return rv;
}

void
plic_enable(struct plic_softc *sc, u_int cpu, u_int irq)
{
	KASSERT(irq < PLIC_NIRQ);
	const bus_addr_t addr = PLIC_ENABLE(sc, cpu, irq);
	const uint32_t mask = __BIT(irq % 32);

	uint32_t reg = PLIC_READ(sc, addr);
	reg |= mask;
	PLIC_WRITE(sc, addr, reg);
}

void
plic_disable(struct plic_softc *sc, u_int cpu, u_int irq)
{
	KASSERT(irq < PLIC_NIRQ);
	const bus_addr_t addr = PLIC_ENABLE(sc, cpu, irq);
	const uint32_t mask = __BIT(irq % 32);

	uint32_t reg = PLIC_READ(sc, addr);
	reg &= ~mask;
	PLIC_WRITE(sc, addr, reg);
}

void
plic_set_priority(struct plic_softc *sc, u_int irq, uint32_t priority)
{
	KASSERT(irq < PLIC_NIRQ);
	const bus_addr_t addr = PLIC_PRIORITY(irq);

	PLIC_WRITE(sc, addr, priority);
}

void
plic_set_threshold(struct plic_softc *sc, cpuid_t cpu, uint32_t threshold)
{
	const bus_addr_t addr = PLIC_THRESHOLD(sc, cpu);

	PLIC_WRITE(sc, addr, threshold);
}

int
plic_attach_common(struct plic_softc *sc, bus_addr_t addr, bus_size_t size)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	u_int irq;

	sc->sc_intr = kmem_zalloc(sizeof(*sc->sc_intr) * sc->sc_ndev,
	    KM_SLEEP);
	sc->sc_intrevs = kmem_zalloc(sizeof(*sc->sc_intrevs) * sc->sc_ndev,
	    KM_SLEEP);

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error("couldn't map registers\n");
		return -1;
	}

	aprint_naive("\n");
	aprint_normal("RISC-V PLIC (%u IRQs)\n", sc->sc_ndev);

	plic_sc = sc;

	/* Start with all interrupts disabled. */
	for (irq = PLIC_FIRST_IRQ; irq < sc->sc_ndev; irq++) {
		plic_set_priority(sc, irq, 0);
	}

	/* Set priority thresholds for all interrupts to 0 (not masked). */
	for (CPU_INFO_FOREACH(cii, ci)) {
		plic_set_threshold(sc, ci->ci_cpuid, 0);
	}

	return 0;
}
