/*	$NetBSD: intc_fdt.c,v 1.2 2023/06/12 19:04:13 skrll Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: intc_fdt.c,v 1.2 2023/06/12 19:04:13 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/kmem.h>
#include <sys/intr.h>

#include <dev/fdt/fdtvar.h>

#include <machine/frame.h>
#include <machine/machdep.h>
#include <machine/sysreg.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "riscv,cpu-intc" },
	DEVICE_COMPAT_EOL
};


struct intc_irqhandler;
struct intc_irq;
struct intc_softc;

typedef int (*intcih_t)(void *);

struct intc_irqhandler {
	struct intc_irq		 *ih_irq;
	intcih_t		  ih_fn;
	void			 *ih_arg;
	TAILQ_ENTRY(intc_irqhandler)
				  ih_next;
};

struct intc_irq {
	struct intc_fdt_softc	*intr_sc;
	void			*intr_ih;
	void			*intr_arg;
	int			 intr_refcnt;
	int			 intr_ipl;
	int			 intr_source;
	int			 intr_istflags;
	struct evcnt *pcpu_evs;
	TAILQ_HEAD(, intc_irqhandler)
				 intr_handlers;
};


struct intc_fdt_softc {
	device_t		 sc_dev;
	bus_space_tag_t		 sc_bst;
	bus_space_handle_t	 sc_bsh;

	struct intc_irq		*sc_irq[IRQ_NSOURCES];

	struct evcnt		*sc_evs[IRQ_NSOURCES];

	struct cpu_info 	*sc_ci;
	cpuid_t			 sc_hartid;
};

static struct intc_fdt_softc *intc_sc;


static void *
intc_intr_establish(struct intc_fdt_softc *sc, u_int source, u_int ipl,
    u_int istflags, int (*func)(void *), void *arg, const char *xname)
{
	if (source > IRQ_NSOURCES)
		return NULL;

	const device_t dev = sc->sc_dev;
	struct intc_irq *irq = sc->sc_irq[source];
	if (irq == NULL) {
		irq = kmem_alloc(sizeof(*irq), KM_SLEEP);
		irq->intr_sc = sc;
		irq->intr_refcnt = 0;
		irq->intr_arg = arg;
		irq->intr_ipl = ipl;
		irq->intr_istflags = istflags;
		irq->intr_source = source;
		TAILQ_INIT(&irq->intr_handlers);
		sc->sc_irq[source] = irq;
	} else {
		if (irq->intr_arg == NULL || arg == NULL) {
			device_printf(dev,
			    "cannot share irq with NULL-arg handler\n");
			return NULL;
		}
		if (irq->intr_ipl != ipl) {
			device_printf(dev,
			    "cannot share irq with different ipl\n");
			return NULL;
		}
		if (irq->intr_istflags != istflags) {
			device_printf(dev,
			    "cannot share irq between mpsafe/non-mpsafe\n");
			return NULL;
		}
	}

	struct intc_irqhandler *irqh = kmem_alloc(sizeof(*irqh), KM_SLEEP);
	irqh->ih_irq = irq;
	irqh->ih_fn = func;
	irqh->ih_arg = arg;

	irq->intr_refcnt++;
	TAILQ_INSERT_TAIL(&irq->intr_handlers, irqh, ih_next);

	/*
	 * XXX interrupt_distribute(9) assumes that any interrupt
	 * handle can be used as an input to the MD interrupt_distribute
	 * implementationm, so we are forced to return the handle
	 * we got back from intr_establish().  Upshot is that the
	 * input to bcm2835_icu_fdt_disestablish() is ambiguous for
	 * shared IRQs, rendering them un-disestablishable.
	 */

	return irqh;
}


static void *
intc_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct intc_fdt_softc * const sc = device_private(dev);

	/*
	 * 1st (and only) cell is the interrupt source, e.g.
	 *  1  IRQ_SUPERVISOR_SOFTWARE
	 *  5  IRQ_SUPERVISOR_TIMER
	 *  9  IRQ_SUPERVISOR_EXTERNAL
	 */

	const u_int source = be32toh(specifier[0]);
	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	return intc_intr_establish(sc, source, ipl, mpsafe, func, arg, xname);
}

static void
intc_fdt_disestablish(device_t dev, void *ih)
{
#if 0
	struct intc_fdt_softc * const sc = device_private(dev);
#endif

}

static const char * const intc_sources[IRQ_NSOURCES] = {
	/* Software interrupts */
	"(reserved)",
	"Supervisor software",
	"Virtual Supervisor software",
	"Machine software",

	/* Timer interrupts */
	"(reserved)",
	"Supervisor timer",
	"Virtual Supervisor timer",
	"Machine timer",

	/* External interrupts */
	"(reserved)",
	"Supervisor external",
	"Virtual Supervisor external",
	"Machine external",

	"(reserved)",
	"Supervisor guest external.",
	"(reserved)",
	"(reserved)"
};

static bool
intc_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	if (!specifier)
		return false;

	struct intc_fdt_softc * const sc = device_private(dev);
	if (sc->sc_ci == NULL)
		return false;

	const u_int source = be32toh(specifier[0]);
	snprintf(buf, buflen, "cpu%lu/%s #%u", sc->sc_hartid,
	    intc_sources[source], source);

	return true;
}


struct fdtbus_interrupt_controller_func intc_fdt_funcs = {
	.establish = intc_fdt_establish,
	.disestablish = intc_fdt_disestablish,
	.intrstr = intc_fdt_intrstr
};


static void
intc_intr_handler(struct trapframe *tf, register_t epc, register_t status,
    register_t cause)
{
	const int ppl = splhigh();
	struct cpu_info * const ci = curcpu();
	unsigned long pending;
	int ipl;

	KASSERT(CAUSE_INTERRUPT_P(cause));

	struct intc_fdt_softc * const sc = intc_sc;

	ci->ci_intr_depth++;
	ci->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&pending))) {
		if (pending == 0)
			continue;

		splx(ipl);

		int source = ffs(pending) - 1;
		struct intc_irq *irq = sc->sc_irq[source];
		KASSERTMSG(irq != NULL, "source %d\n", source);

		if (irq) {
			struct intc_irqhandler *iih;

			bool mpsafe =
			    source != IRQ_SUPERVISOR_EXTERNAL ||
			    (irq->intr_istflags & IST_MPSAFE) != 0;
			struct clockframe cf = {
				.cf_epc = epc,
				.cf_status = status,
				.cf_intr_depth = ci->ci_intr_depth
			};

			if (!mpsafe) {
				KERNEL_LOCK(1, NULL);
			}

			TAILQ_FOREACH(iih, &irq->intr_handlers, ih_next) {
				int handled =
				    iih->ih_fn(iih->ih_arg ? iih->ih_arg : &cf);
				if (handled)
					break;
			}

			if (!mpsafe) {
				KERNEL_UNLOCK_ONE(NULL);
			}
		}
		splhigh();
	}
	ci->ci_intr_depth--;
	splx(ppl);
}



static int
intc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
intc_attach(device_t parent, device_t self, void *aux)
{
	const struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	int error = fdtbus_register_interrupt_controller(self, phandle,
            &intc_fdt_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	struct cpu_info * const ci = device_private(parent);
	if (ci == NULL) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}
	aprint_naive("\n");
	aprint_normal(": local interrupt controller\n");

	struct intc_fdt_softc * const sc = device_private(self);
	intc_sc = sc;

	riscv_intr_set_handler(intc_intr_handler);

	sc->sc_dev = self;
	sc->sc_ci = ci;
	sc->sc_hartid = ci->ci_cpuid;

	intc_intr_establish(sc, IRQ_SUPERVISOR_TIMER, IPL_SCHED, IST_MPSAFE,
	    riscv_timer_intr, NULL, "clock");
#ifdef MULTIPROCESSOR
	intc_intr_establish(sc, IRQ_SUPERVISOR_SOFTWARE, IPL_HIGH, IST_MPSAFE,
	    riscv_ipi_intr, NULL, "ipi");
#endif
}

CFATTACH_DECL_NEW(intc_fdt, sizeof(struct intc_fdt_softc),
	intc_match, intc_attach, NULL, NULL);
