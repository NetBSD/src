/*	$NetBSD: necpb.c,v 1.21.12.1 2006/05/24 15:47:51 tron Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: necpb.c,v 1.21.12.1 2006/05/24 15:47:51 tron Exp $");

#include "opt_pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#define _ARC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/pio.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/platform.h>

#include <mips/cache.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#ifdef PCI_NETBSD_CONFIGURE
#include <dev/pci/pciconf.h>
#endif

#include <arc/jazz/rd94.h>
#include <arc/pci/necpbvar.h>

#include "ioconf.h"

int	necpbmatch(struct device *, struct cfdata *, void *);
void	necpbattach(struct device *, struct device *, void *);

void		necpb_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
int		necpb_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t	necpb_make_tag(pci_chipset_tag_t, int, int, int);
void		necpb_decompose_tag(pci_chipset_tag_t, pcitag_t, int *,
		    int *, int *);
pcireg_t	necpb_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		necpb_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		necpb_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *	necpb_intr_string(pci_chipset_tag_t, pci_intr_handle_t);
void *		necpb_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*func)(void *), void *);
void		necpb_intr_disestablish(pci_chipset_tag_t, void *);
#ifdef PCI_NETBSD_CONFIGURE
void		necpb_conf_interrupt(pci_chipset_tag_t, int, int, int, int,
		    int *);
int		necpb_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);
#endif

uint32_t	necpb_intr(uint32_t, struct clockframe *);


CFATTACH_DECL(necpb, sizeof(struct necpb_softc),
    necpbmatch, necpbattach, NULL, NULL);

static struct necpb_intrhand	*necpb_inttbl[4];

/* There can be only one. */
int necpbfound;
struct necpb_context necpb_main_context;
static long necpb_mem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(10) / sizeof(long)];
static long necpb_io_ex_storage[EXTENT_FIXED_STORAGE_SIZE(10) / sizeof(long)];

int
necpbmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, necpb_cd.cd_name) != 0)
		return 0;

	if (necpbfound)
		return 0;

	return 1;
}

/*
 * Set up the chipset's function pointers.
 */
void
necpb_init(struct necpb_context *ncp)
{
	pci_chipset_tag_t pc;
#ifndef PCI_NETBSD_CONFIGURE
	pcitag_t tag;
	pcireg_t id, class, csr;
	u_int dev;
#endif

	if (ncp->nc_initialized)
		return;

	arc_large_bus_space_init(&ncp->nc_memt, "necpcimem",
	    RD94_P_PCI_MEM, 0, RD94_S_PCI_MEM);
	arc_bus_space_init_extent(&ncp->nc_memt, (caddr_t)necpb_mem_ex_storage,
	    sizeof(necpb_mem_ex_storage));

	arc_bus_space_init(&ncp->nc_iot, "necpciio",
	    RD94_P_PCI_IO, RD94_V_PCI_IO, 0, RD94_S_PCI_IO);
	arc_bus_space_init_extent(&ncp->nc_iot, (caddr_t)necpb_io_ex_storage,
	    sizeof(necpb_io_ex_storage));

	jazz_bus_dma_tag_init(&ncp->nc_dmat);

	pc = &ncp->nc_pc;
	pc->pc_attach_hook = necpb_attach_hook;
	pc->pc_bus_maxdevs = necpb_bus_maxdevs;
	pc->pc_make_tag = necpb_make_tag;
	pc->pc_decompose_tag = necpb_decompose_tag;
	pc->pc_conf_read = necpb_conf_read;
	pc->pc_conf_write = necpb_conf_write;
	pc->pc_intr_map = necpb_intr_map;
	pc->pc_intr_string = necpb_intr_string;
	pc->pc_intr_establish = necpb_intr_establish;
	pc->pc_intr_disestablish = necpb_intr_disestablish;
#ifdef PCI_NETBSD_CONFIGURE
	pc->pc_conf_interrupt = necpb_conf_interrupt;
	pc->pc_conf_hook = necpb_conf_hook;
#endif

#ifndef PCI_NETBSD_CONFIGURE
	/*
	 * XXX:
	 *  NEC's firmware does not configure PCI devices completely.
	 *  We need to disable expansion ROM and enable mem/io/busmaster
	 *  bits here.
	 */
	for (dev = 3; dev <= 5; dev++) {
		tag = necpb_make_tag(pc, 0, dev, 0);
		id = necpb_conf_read(pc, tag, PCI_ID_REG);

		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;

		class = necpb_conf_read(pc, tag, PCI_CLASS_REG);
		csr = necpb_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		if (PCI_CLASS(class) != PCI_CLASS_BRIDGE ||
		    PCI_SUBCLASS(class) != PCI_SUBCLASS_BRIDGE_PCI) {
			csr |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE;
			necpb_conf_write(pc, tag, PCI_MAPREG_ROM, 0);
		}
		csr |= PCI_COMMAND_MASTER_ENABLE;
		necpb_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	}
#endif

	ncp->nc_initialized = 1;
}

void
necpbattach(struct device *parent, struct device *self, void *aux)
{
	struct necpb_softc *sc = (struct necpb_softc *)self;
	struct pcibus_attach_args pba;
	pci_chipset_tag_t pc;
	int i;

	necpbfound = 1;

	printf("\n");

	sc->sc_ncp = &necpb_main_context;
	necpb_init(sc->sc_ncp);

	pc = &sc->sc_ncp->nc_pc;
#ifdef PCI_NETBSD_CONFIGURE
	pc->pc_ioext = extent_create("necpbio", 0x00100000, 0x01ffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	pc->pc_memext = extent_create("necpbmem", 0x08000000, 0x3fffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	pci_configure_bus(pc, pc->pc_ioext, pc->pc_memext, NULL, 0,
	    mips_dcache_align);
#endif

	out32(RD94_SYS_PCI_INTMASK, 0xf);

	for (i = 0; i < 4; i++)
		necpb_inttbl[i] = NULL;

	(*platform->set_intr)(MIPS_INT_MASK_2, necpb_intr, 3);

	pba.pba_iot = &sc->sc_ncp->nc_iot;
	pba.pba_memt = &sc->sc_ncp->nc_memt;
	pba.pba_dmat = &sc->sc_ncp->nc_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

void
necpb_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

int
necpb_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return 32;
}

pcitag_t
necpb_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("necpb_make_tag: bad request");

	tag = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);
	return tag;
}

void
necpb_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp,
   int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
necpb_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	pcireg_t data;
	int s;

	s = splhigh();
	out32(RD94_SYS_PCI_CONFADDR, tag | reg);
	data = in32(RD94_SYS_PCI_CONFDATA);
	out32(RD94_SYS_PCI_CONFADDR, 0);
	splx(s);

	return data;
}

void
necpb_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	int s;

	s = splhigh();
	out32(RD94_SYS_PCI_CONFADDR, tag | reg);
	out32(RD94_SYS_PCI_CONFDATA, data);
	out32(RD94_SYS_PCI_CONFADDR, 0);
	splx(s);
}

int
necpb_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int pin = pa->pa_intrpin;
	int swiz = pa->pa_intrswiz % 4;
	int bus, dev;

	if (pin == 0) {
		/* No IRQ used. */
		*ihp = -1;
		return 1;
	}

	if (pin > 4) {
		printf("necpb_intr_map: bad interrupt pin %d\n", pin);
		*ihp = -1;
		return 1;
	}

	necpb_decompose_tag(pc, intrtag, &bus, &dev, NULL);
	if (bus != 0) {
		printf("necpb_intr_map: unknown bus %d\n", bus);
		*ihp = -1;
		return 1;
	}

	switch (dev) {
	case 3:
		*ihp = (pin - swiz + 2) % 4;
		break;
	case 4:
		*ihp = (pin - swiz + 1) % 4;
		break;
	case 5:
		*ihp = (pin - swiz + 0) % 4;
		break;
	default:
		*ihp = -1;
		return 1;
	}

	return 0;
}

const char *
necpb_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char str[8];

	if (ih >= 4)
		panic("necpb_intr_string: bogus handle %ld", ih);
	sprintf(str, "int %c", 'A' + (int)ih);
	return str;
}

void *
necpb_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{
	struct necpb_intrhand *n, *p;
	uint32_t mask;

	if (ih >= 4)
		panic("necpb_intr_establish: bogus handle");

	n = malloc(sizeof(struct necpb_intrhand), M_DEVBUF, M_NOWAIT);
	if (n == NULL)
		panic("necpb_intr_establish: can't malloc interrupt handle");

	n->ih_func = func;
	n->ih_arg = arg;
	n->ih_next = NULL;
	n->ih_intn = ih;
	strlcpy(n->ih_evname, necpb_intr_string(pc, ih), sizeof(n->ih_evname));
	evcnt_attach_dynamic(&n->ih_evcnt, EVCNT_TYPE_INTR, NULL, "necpb",
	    n->ih_evname);

	if (necpb_inttbl[ih] == NULL) {
		necpb_inttbl[ih] = n;
		mask = in32(RD94_SYS_PCI_INTMASK);
		mask |= 1 << ih;
		out32(RD94_SYS_PCI_INTMASK, mask);
	} else {
		p = necpb_inttbl[ih];
		while (p->ih_next != NULL)
			p = p->ih_next;
		p->ih_next = n;
	}

	return n;
}

void
necpb_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	struct necpb_intrhand *n, *p, *q;
	uint32_t mask;

	n = cookie;

	q = NULL;
	p = necpb_inttbl[n->ih_intn];
	while (p != n) {
		if (p == NULL)
			panic("necpb_intr_disestablish: broken intr table");
		q = p;
		p = p->ih_next;
	}

	if (q == NULL) {
		necpb_inttbl[n->ih_intn] = n->ih_next;
		if (n->ih_next == NULL) {
			mask = in32(RD94_SYS_PCI_INTMASK);
			mask &= ~(1 << n->ih_intn);
			out32(RD94_SYS_PCI_INTMASK, mask);
		}
	} else
		q->ih_next = n->ih_next;

	evcnt_detach(&n->ih_evcnt);

	free(n, M_DEVBUF);
}

/*
 *   Handle PCI/EISA interrupt.
 */
uint32_t
necpb_intr(uint32_t mask, struct clockframe *cf)
{
	uint32_t vector, stat;
	struct necpb_intrhand *p;
	int i, handled;

	handled = 0;
	vector = in32(RD94_SYS_INTSTAT2) & 0xffff;

	if (vector == 0x4000) {
		stat = in32(RD94_SYS_PCI_INTSTAT);
		stat &= in32(RD94_SYS_PCI_INTMASK);
		for (i = 0; i < 4; i++) {
			if (stat & (1 << i)) {
#if 0
				printf("pint %d\n", i);
#endif
				p = necpb_inttbl[i];
				while (p != NULL) {
					if ((*p->ih_func)(p->ih_arg)) {
						p->ih_evcnt.ev_count++;
						handled |= 1;
					}
					p = p->ih_next;
				}
			}
		}
	} else if (vector == 0x8000) {
		printf("eisa_nmi\n");
	} else {
		printf("eint %d\n", vector & 0xff);
#if 0
		if (eisa_intr(vector & 0xff)) {
			handled |= 1;
		}
#endif
	}

	return handled ? ~MIPS_INT_MASK_2 : ~0;
}

#ifdef PCI_NETBSD_CONFIGURE
void
necpb_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int func,
    int swiz, int *iline)
{

	return;
}

int
necpb_conf_hook(pci_chipset_tag_t pc, int bus, int dev, int func,
    pcireg_t id)
{

	/* ignore bogus IDs */
	if (id == 0)
		return 0;

	/* don't configure bridges */
	if (bus == 0 && (dev == 1 || dev == 2))
		return 0;

	return PCI_CONF_DEFAULT;
}
#endif
