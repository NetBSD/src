/*	$NetBSD: exynos_combiner.c,v 1.7.4.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: exynos_combiner.c,v 1.7.4.2 2017/12/03 11:35:56 jdolecek Exp $");

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

#define COMBINER_IESR_OFFSET   0x00
#define COMBINER_IECR_OFFSET   0x04
#define COMBINER_ISTR_OFFSET   0x08
#define COMBINER_IMSR_OFFSET   0x0C
#define COMBINER_GROUP_SIZE    0x10
#define COMBINER_IRQS_PER_BLOCK   8
#define COMBINER_BLOCKS_PER_GROUP 4
#define COMBINER_N_BLOCKS        32

struct exynos_combiner_softc;

struct exynos_combiner_irq_entry {
	int				irq_no;
	int (*irq_handler)(void *);
	void *				irq_arg;
	struct exynos_combiner_irq_entry *irq_next;
	bool				irq_mpsafe;
};

struct exynos_combiner_irq_block {
	int irq_block_no;
	struct exynos_combiner_softc	*irq_sc;
	struct exynos_combiner_irq_entry *irq_entries;
	struct exynos_combiner_irq_block *irq_block_next;
	void *irq_ih;
};

struct exynos_combiner_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct exynos_combiner_irq_block *irq_blocks;
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
	sc->irq_blocks = NULL;

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

	aprint_normal(" @ 0x%08x: interrupt combiner", (uint)addr);
	aprint_naive("\n");
	aprint_normal("\n");

}

static struct exynos_combiner_irq_block *
exynos_combiner_new_block(struct exynos_combiner_softc *sc, int block_no)
{
	struct exynos_combiner_irq_block *n = kmem_zalloc(sizeof(*n),
							  KM_SLEEP);
	n->irq_block_no = block_no;
	n->irq_block_next = sc->irq_blocks;
	sc->irq_blocks = n;
	return n;
}
			  
static struct exynos_combiner_irq_block *
exynos_combiner_get_block(struct exynos_combiner_softc *sc, int block)
{
	for (struct exynos_combiner_irq_block *b = sc->irq_blocks;
	     b; b = b->irq_block_next) {
		if (b->irq_block_no == block)
			return b;
	}
	return NULL;
}

static struct exynos_combiner_irq_entry *
exynos_combiner_new_irq(struct exynos_combiner_irq_block *block,
			int irq, bool mpsafe, int (*func)(void *), void *arg)
{
	struct exynos_combiner_irq_entry * n = kmem_zalloc(sizeof(*n),
							   KM_SLEEP);
	n->irq_no = irq;
	n->irq_handler = func;
	n->irq_next = block->irq_entries;
	n->irq_arg = arg;
	n->irq_mpsafe = mpsafe;
	block->irq_entries = n;
	return n;
}

static struct exynos_combiner_irq_entry *
exynos_combiner_get_irq(struct exynos_combiner_irq_block *b, int irq)
{
	for (struct exynos_combiner_irq_entry *p = b->irq_entries; p;
	     p = p->irq_next) {
		if (p->irq_no == irq)
			return p;
	}
	return NULL;
}

static int
exynos_combiner_irq(void *cookie)
{
	struct exynos_combiner_irq_block *blockp = cookie;
	struct exynos_combiner_softc *sc = blockp->irq_sc;
	int intr = blockp->irq_block_no;
	int iblock = 
		intr / COMBINER_BLOCKS_PER_GROUP * COMBINER_GROUP_SIZE
		+ COMBINER_IESR_OFFSET;
	int istatus =
		bus_space_read_4(sc->sc_bst, sc->sc_bsh, iblock);
	istatus >>= (intr % 4) *8;
	for (int irq = 0; irq < 8; irq++) {
		if (istatus & 1 << irq) {
			struct exynos_combiner_irq_entry *e =
				exynos_combiner_get_irq(blockp, irq);
			if (e) {
				if (!e->irq_mpsafe)
					KERNEL_LOCK(1, curlwp);
				e->irq_handler(e->irq_arg);
				if (!e->irq_mpsafe)
					KERNEL_UNLOCK_ONE(curlwp);
			} else
				printf("%s: Unexpected irq %d, %d\n", __func__,
				       intr, irq);
		}
	}
	return 0;
}

static void *
exynos_combiner_establish(device_t dev, u_int *specifier,
			  int ipl, int flags,
			  int (*func)(void *), void *arg)
{
	struct exynos_combiner_softc * const sc = device_private(dev);
	struct exynos_combiner_irq_block *blockp;
	struct exynos_combiner_irq_entry *entryp;
	const bool mpsafe = (flags & FDT_INTR_MPSAFE) != 0;

	const u_int intr = be32toh(specifier[0]);
	const u_int irq = be32toh(specifier[1]);

	int iblock = 
		intr / COMBINER_BLOCKS_PER_GROUP * COMBINER_GROUP_SIZE
		+ COMBINER_IESR_OFFSET;

	blockp = exynos_combiner_get_block(sc, intr);
	if (!blockp) {
		blockp = exynos_combiner_new_block(sc, intr);
		KASSERT(blockp);
		blockp->irq_ih = fdtbus_intr_establish(sc->sc_phandle, intr,
		    IPL_VM /* XXX */, FDT_INTR_MPSAFE, exynos_combiner_irq,
		    blockp);
	}

	entryp = exynos_combiner_get_irq(blockp, irq);
	if (entryp)
		return NULL;
	entryp = exynos_combiner_new_irq(blockp, irq, mpsafe, func, arg);
	KASSERT(entryp);

	int istatus =
		bus_space_read_4(sc->sc_bst, sc->sc_bsh, iblock);
	istatus |= 1 << (irq + ((intr % 4) * 8));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, iblock, istatus);
	return (void *)istatus;
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

	/* 1st cell is the interrupt block */
	/* 2nd cell is the interrupt number */

	const u_int intr = be32toh(specifier[0]);
	const u_int irq = be32toh(specifier[1]);

	snprintf(buf, buflen, "combiner intr %d irq %d", intr, irq);

	return true;
}
