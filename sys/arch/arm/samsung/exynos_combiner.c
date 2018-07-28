/*	$NetBSD: exynos_combiner.c,v 1.7.6.1 2018/07/28 04:37:29 pgoyette Exp $ */

/*-
* Copyright (c) 2015 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Marty Fouts
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

#include "opt_exynos.h"
#include "opt_arm_debug.h"
#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_combiner.c,v 1.7.6.1 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <arm/cortex/gic_intr.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_intr.h>

#include <dev/fdt/fdtvar.h>

#define	COMBINER_GROUP_BASE(group)	(((group) / 4) * 0x10)
#define	COMBINER_GROUP_MASK(group)	(0xff << (((group) % 4) * 8))

#define	COMBINER_IESR_REG(group)	(COMBINER_GROUP_BASE(group) + 0x00)
#define	COMBINER_IECR_REG(group)	(COMBINER_GROUP_BASE(group) + 0x04)
#define	COMBINER_ISTR_REG(group)	(COMBINER_GROUP_BASE(group) + 0x08)
#define	COMBINER_IMSR_REG(group)	(COMBINER_GROUP_BASE(group) + 0x0c)

struct exynos_combiner_softc;

struct exynos_combiner_irq_entry {
	int				irq_no;
	int (*irq_handler)(void *);
	void *				irq_arg;
	struct exynos_combiner_irq_entry *irq_next;
	bool				irq_mpsafe;
};

struct exynos_combiner_irq_group {
	int irq_group_no;
	struct exynos_combiner_softc	*irq_sc;
	struct exynos_combiner_irq_entry *irq_entries;
	struct exynos_combiner_irq_group *irq_group_next;
	void *irq_ih;
	int irq_ipl;
};

struct exynos_combiner_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct exynos_combiner_irq_group *irq_groups;
};

static int exynos_combiner_match(device_t, cfdata_t, void *);
static void exynos_combiner_attach(device_t, device_t, void *);

static void *	exynos_combiner_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *);
static void	exynos_combiner_disestablish(device_t, void *);
static bool	exynos_combiner_intrstr(device_t, u_int  *, char *,
					size_t);

struct fdtbus_interrupt_controller_func exynos_combiner_funcs = {
	.establish = exynos_combiner_establish,
	.disestablish = exynos_combiner_disestablish,
	.intrstr = exynos_combiner_intrstr
};

CFATTACH_DECL_NEW(exynos_intr, sizeof(struct exynos_combiner_softc),
	exynos_combiner_match, exynos_combiner_attach, NULL, NULL);

static int
exynos_combiner_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos4210-combiner",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_combiner_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_combiner_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	error = fdtbus_register_interrupt_controller(self, faa->faa_phandle,
	    &exynos_combiner_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(" @ 0x%08x: interrupt combiner\n", (uint)addr);

}

static struct exynos_combiner_irq_group *
exynos_combiner_new_group(struct exynos_combiner_softc *sc, int group_no)
{
	struct exynos_combiner_irq_group *n = kmem_zalloc(sizeof(*n),
							  KM_SLEEP);
	n->irq_group_no = group_no;
	n->irq_group_next = sc->irq_groups;
	n->irq_sc = sc;
	sc->irq_groups = n;
	return n;
}
			  
static struct exynos_combiner_irq_group *
exynos_combiner_get_group(struct exynos_combiner_softc *sc, int group_no)
{
	for (struct exynos_combiner_irq_group *g = sc->irq_groups;
	     g; g = g->irq_group_next) {
		if (g->irq_group_no == group_no)
			return g;
	}
	return NULL;
}

static struct exynos_combiner_irq_entry *
exynos_combiner_new_irq(struct exynos_combiner_irq_group *group,
			int irq, bool mpsafe, int (*func)(void *), void *arg)
{
	struct exynos_combiner_irq_entry * n = kmem_zalloc(sizeof(*n),
							   KM_SLEEP);
	n->irq_no = irq;
	n->irq_handler = func;
	n->irq_next = group->irq_entries;
	n->irq_arg = arg;
	n->irq_mpsafe = mpsafe;
	group->irq_entries = n;
	return n;
}

static struct exynos_combiner_irq_entry *
exynos_combiner_get_irq(struct exynos_combiner_irq_group *g, int irq)
{
	for (struct exynos_combiner_irq_entry *p = g->irq_entries; p;
	     p = p->irq_next) {
		if (p->irq_no == irq)
			return p;
	}
	return NULL;
}

static int
exynos_combiner_irq(void *cookie)
{
	struct exynos_combiner_irq_group *groupp = cookie;
	struct exynos_combiner_softc *sc = groupp->irq_sc;
	int rv = 0;

	const int group_no = groupp->irq_group_no;

	const uint32_t istr =
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, COMBINER_ISTR_REG(group_no));
	const uint32_t istatus = __SHIFTOUT(istr, COMBINER_GROUP_MASK(group_no));

	for (int irq = 0; irq < 8; irq++) {
		if (istatus & __BIT(irq)) {
			struct exynos_combiner_irq_entry *e =
				exynos_combiner_get_irq(groupp, irq);
			if (e) {
				if (!e->irq_mpsafe)
					KERNEL_LOCK(1, curlwp);
				rv += e->irq_handler(e->irq_arg);
				if (!e->irq_mpsafe)
					KERNEL_UNLOCK_ONE(curlwp);
			} else
				printf("%s: Unexpected irq (%d) on group %d\n", __func__,
				       irq, group_no);
		}
	}

	return !!rv;
}

static void *
exynos_combiner_establish(device_t dev, u_int *specifier,
			  int ipl, int flags,
			  int (*func)(void *), void *arg)
{
	struct exynos_combiner_softc * const sc = device_private(dev);
	struct exynos_combiner_irq_group *groupp;
	struct exynos_combiner_irq_entry *entryp;
	const bool mpsafe = (flags & FDT_INTR_MPSAFE) != 0;
	uint32_t iesr;

	const u_int group = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);

	groupp = exynos_combiner_get_group(sc, group);
	if (!groupp) {
		groupp = exynos_combiner_new_group(sc, group);
		if (arg == NULL) {
			groupp->irq_ih = fdtbus_intr_establish(sc->sc_phandle, group,
			    ipl /* XXX */, flags, func, NULL);
		} else {
			groupp->irq_ih = fdtbus_intr_establish(sc->sc_phandle, group,
			    ipl /* XXX */, FDT_INTR_MPSAFE, exynos_combiner_irq,
			    groupp);
		}
		KASSERT(groupp->irq_ih != NULL);
		groupp->irq_ipl = ipl;
	} else if (groupp->irq_ipl != ipl) {
		aprint_error_dev(dev,
		    "interrupt combiner cannot share interrupts with different ipl\n");
		return NULL;
	}

	if (exynos_combiner_get_irq(groupp, intr) != NULL)
		return NULL;

	entryp = exynos_combiner_new_irq(groupp, intr, mpsafe, func, arg);

	iesr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, COMBINER_IESR_REG(group));
	iesr |= __SHIFTIN((1 << intr), COMBINER_GROUP_MASK(group));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, COMBINER_IESR_REG(group), iesr);

	return entryp;
}

static void
exynos_combiner_disestablish(device_t dev, void *ih)
{
	/* MJF: Find the ih and disable the handler. */
	panic("exynos_combiner_disestablish not implemented");
}

static bool
exynos_combiner_intrstr(device_t dev, u_int *specifier, char *buf,
			size_t buflen)
{

	/* 1st cell is the combiner group number */
	/* 2nd cell is the interrupt number within the group */

	const u_int group = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);

	snprintf(buf, buflen, "interrupt combiner group %d intr %d", group, intr);

	return true;
}
