/*	$NetBSD: isa_machdep.c,v 1.13 2009/08/18 17:02:00 dyoung Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.13 2009/08/18 17:02:00 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <machine/sysconf.h>
#include <machine/autoconf.h>
#include <machine/mainboard.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

static int	mipscoisabusprint(void *auxp, const char *);
static int	isabusmatch(struct device *, struct cfdata *, void *);
static void	isabusattach(struct device *, struct device *, void *);

struct isabus_softc {
	struct device		sc_dev;
	struct mipsco_isa_chipset sc_isa_ic;
};

CFATTACH_DECL(isabus, sizeof(struct isabus_softc),
    isabusmatch, isabusattach, NULL, NULL);

extern struct cfdriver isabus_cd;

static struct mipsco_bus_space	isa_io_bst, isa_mem_bst, isa_ctl_bst;
static struct mipsco_bus_dma_tag isa_dmatag;

static void isa_bus_space_init(struct mipsco_bus_space *, const char *,
				     paddr_t, size_t);
int    isa_intr(void *);


int
isabusmatch(struct device *pdp, struct cfdata *cfp, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, isabus_cd.cd_name) != 0)
		return 0;
	return 1;
}

static void
isa_bus_space_init(struct mipsco_bus_space *bst, const char *type, paddr_t paddr, size_t len)
{
	vaddr_t vaddr = MIPS_PHYS_TO_KSEG1(paddr); /* XXX */

	/* Setup default bus_space */
	mipsco_bus_space_init(bst, type, paddr, vaddr, 0x0, len);

	/* ISA bus maps 1 word for every byte, therefore stride = 2 */
	mipsco_bus_space_set_aligned_stride(bst, 2);

	/*
	 * ISA bus will do an automatic byte swap, but when accessing
	 * memory using bus_space_stream functions we need to byte swap
	 * to reverse the one performed in hardware
	 */
	bst->bs_bswap = 1;

	bst->bs_intr_establish = (void *)isa_intr_establish;

	printf(" %s %p", type, (void *)vaddr);
}


void
isabusattach(struct device *pdp, struct device *dp, void *aux)
{
	struct isabus_softc *sc = (struct isabus_softc *)dp;
	struct mipsco_isa_chipset *ic = &sc->sc_isa_ic;
	struct isabus_attach_args iba;

	printf(":");

	iba.iba_iot	= &isa_io_bst;
	iba.iba_memt	= &isa_mem_bst;
	iba.iba_dmat	= &isa_dmatag;
	iba.iba_ic	= ic;

	isa_bus_space_init(&isa_io_bst,  "isa_io",   PIZAZZ_ISA_IOBASE,
	  	PIZAZZ_ISA_IOSIZE);

	isa_bus_space_init(&isa_mem_bst, "isa_mem",  PIZAZZ_ISA_MEMBASE,
		PIZAZZ_ISA_MEMSIZE);

	isa_bus_space_init(&isa_ctl_bst, "isa_intr", PIZAZZ_ISA_INTRLATCH,
	  	sizeof(u_int32_t));

	_bus_dma_tag_init(&isa_dmatag);

	ic->ic_bst = &isa_ctl_bst;

	if (bus_space_map(ic->ic_bst, 0x00000, sizeof(u_int32_t),
			  BUS_SPACE_MAP_LINEAR, &ic->ic_bsh) != 0) {
		printf(": can't map intrreg\n");
		return;
	}
	
	/* Clear ISA interrupt latch */
	bus_space_write_4(ic->ic_bst, ic->ic_bsh, 0, 0);

	evcnt_attach_dynamic(&ic->ic_intrcnt, EVCNT_TYPE_INTR, NULL,
			     dp->dv_xname, "intr");

	LIST_INIT(&ic->intr_q);
	(*platform.intr_establish)(SYS_INTR_ATBUS, isa_intr, ic);

	printf("\n");
	config_found_ia(dp, "isabus", &iba, mipscoisabusprint);
}

int
mipscoisabusprint(void *auxp, const char *name)
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}

void
isa_attach_hook(struct device *parent, struct device *self, struct isabus_attach_args *iba)
{
}

void
isa_detach_hook(device_t self)
{
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{
	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
isa_intr_establish(isa_chipset_tag_t ic, int intr, int type, int level, int (*ih_fun)(void*), void *ih_arg)
	/* type:   XXX not yet */
	/* level:   XXX not yet */
{
	struct mipsco_intrhand *ih;

	ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		panic("isa_intr_establish: malloc failed");

	ih->ih_fun = ih_fun;
	ih->ih_arg  = ih_arg;
	LIST_INSERT_HEAD(&ic->intr_q, ih, ih_q);
	return ih;
}

void
isa_intr_disestablish(isa_chipset_tag_t ic, void *cookie)
{
	struct mipsco_intrhand *ih = cookie;

	LIST_REMOVE(ih, ih_q);
	free(ih, M_DEVBUF);
}

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	return 0;
}

int
isa_intr(void *arg)
{
	struct mipsco_isa_chipset *ic = (struct mipsco_isa_chipset *)arg;
	struct mipsco_intrhand *ih;
	int rv, handled;

	ic->ic_intrcnt.ev_count++;

	handled = 0;
	LIST_FOREACH(ih, &ic->intr_q, ih_q) {
		/*
		 * The handler returns one of three values:
		 *   0:	This interrupt wasn't for me.
		 *   1: This interrupt was for me.
		 *  -1: This interrupt might have been for me, but I can't say
		 *      for sure.
		 */
		rv = (*ih->ih_fun)(ih->ih_arg);
		handled |= (rv != 0);
	}

	/* Clear ISA interrupt latch */
	bus_space_write_4(ic->ic_bst, ic->ic_bsh, 0, 0);

	return handled;
}
