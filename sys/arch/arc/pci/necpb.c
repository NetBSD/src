/*	$NetBSD: necpb.c,v 1.9 2001/08/17 11:11:57 ur Exp $	*/

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

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <arc/jazz/rd94.h>
#include <arc/pci/necpbvar.h>

int	necpbmatch __P((struct device *, struct cfdata *, void *));
void	necpbattach __P((struct device *, struct device *, void *));

static	int	necpbprint __P((void *, const char *));

void		necpb_attach_hook __P((struct device *, struct device *,
		    struct pcibus_attach_args *));
int		necpb_bus_maxdevs __P((pci_chipset_tag_t, int));
pcitag_t	necpb_make_tag __P((pci_chipset_tag_t, int, int, int));
void		necpb_decompose_tag __P((pci_chipset_tag_t, pcitag_t, int *,
		    int *, int *));
pcireg_t	necpb_conf_read __P((pci_chipset_tag_t, pcitag_t, int));
void		necpb_conf_write __P((pci_chipset_tag_t, pcitag_t, int,
		    pcireg_t));
int		necpb_intr_map __P((struct pci_attach_args *,
		    pci_intr_handle_t *));
const char *	necpb_intr_string __P((pci_chipset_tag_t, pci_intr_handle_t));
void *		necpb_intr_establish __P((pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*func)(void *), void *));
void		necpb_intr_disestablish __P((pci_chipset_tag_t, void *));

int		necpb_intr(unsigned, struct clockframe *);


struct cfattach necpb_ca = {
	sizeof(struct necpb_softc), necpbmatch, necpbattach,
};

extern struct cfdriver necpb_cd;

static struct necpb_intrhand	*necpb_inttbl[4];

/* There can be only one. */
int necpbfound;
struct necpb_context necpb_main_context;
static long necpb_mem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(10) / sizeof(long)];
static long necpb_io_ex_storage[EXTENT_FIXED_STORAGE_SIZE(10) / sizeof(long)];

int
necpbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, necpb_cd.cd_name) != 0)
		return (0);

	if (necpbfound)
		return (0);

	return (1);
}

/*
 * Set up the chipset's function pointers.
 */
void
necpb_init(ncp)
	struct necpb_context *ncp;
{
	pcitag_t tag;
	pcireg_t csr;

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

	ncp->nc_pc.pc_attach_hook = necpb_attach_hook;
	ncp->nc_pc.pc_bus_maxdevs = necpb_bus_maxdevs;
	ncp->nc_pc.pc_make_tag = necpb_make_tag;
	ncp->nc_pc.pc_conf_read = necpb_conf_read;
	ncp->nc_pc.pc_conf_write = necpb_conf_write;
	ncp->nc_pc.pc_intr_map = necpb_intr_map;
	ncp->nc_pc.pc_intr_string = necpb_intr_string;
	ncp->nc_pc.pc_intr_establish = necpb_intr_establish;
	ncp->nc_pc.pc_intr_disestablish = necpb_intr_disestablish;

	/*
	 * XXX:
	 *  NEC's firmware does not configure PCI devices completely.
	 *  We need to disable expansion ROM and enable mem/io/busmaster
	 *  bits here.
	 */
	tag = necpb_make_tag(&ncp->nc_pc, 0, 3, 0);
	csr = necpb_conf_read(&ncp->nc_pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
	    PCI_COMMAND_MASTER_ENABLE;
	necpb_conf_write(&ncp->nc_pc, tag, PCI_COMMAND_STATUS_REG, csr);
	necpb_conf_write(&ncp->nc_pc, tag, PCI_MAPREG_ROM, 0);

	tag = necpb_make_tag(&ncp->nc_pc, 0, 4, 0);
	csr = necpb_conf_read(&ncp->nc_pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
	    PCI_COMMAND_MASTER_ENABLE;
	necpb_conf_write(&ncp->nc_pc, tag, PCI_COMMAND_STATUS_REG, csr);
	necpb_conf_write(&ncp->nc_pc, tag, PCI_MAPREG_ROM, 0);

	tag = necpb_make_tag(&ncp->nc_pc, 0, 5, 0);
	csr = necpb_conf_read(&ncp->nc_pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
	    PCI_COMMAND_MASTER_ENABLE;
	necpb_conf_write(&ncp->nc_pc, tag, PCI_COMMAND_STATUS_REG, csr);
	necpb_conf_write(&ncp->nc_pc, tag, PCI_MAPREG_ROM, 0);

	ncp->nc_initialized = 1;
}

void
necpbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct necpb_softc *sc = (struct necpb_softc *)self;
	struct pcibus_attach_args pba;
	int i;

	necpbfound = 1;

	printf("\n");

	sc->sc_ncp = &necpb_main_context;
	necpb_init(sc->sc_ncp);

	out32(RD94_SYS_PCI_INTMASK, 0xf);

	for (i = 0; i < 4; i++)
		necpb_inttbl[i] = NULL;

	(*platform->set_intr)(MIPS_INT_MASK_2, necpb_intr, 3);

	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_ncp->nc_iot;
	pba.pba_memt = &sc->sc_ncp->nc_memt;
	pba.pba_dmat = &sc->sc_ncp->nc_dmat;
	pba.pba_pc = &sc->sc_ncp->nc_pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;

	config_found(self, &pba, necpbprint);
}

static int
necpbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

void
necpb_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
}

int
necpb_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{
	return (32);
}

pcitag_t
necpb_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("necpb_make_tag: bad request");

	tag = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);
	return (tag);
}

void
necpb_decompose_tag(pc, tag, bp, dp, fp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int *bp, *dp, *fp;
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
necpb_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;
	int s;

	s = splhigh();
	out32(RD94_SYS_PCI_CONFADDR, tag | reg);
	data = in32(RD94_SYS_PCI_CONFDATA);
	out32(RD94_SYS_PCI_CONFADDR, 0);
	splx(s);

	return (data);
}

void
necpb_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	int s;

	s = splhigh();
	out32(RD94_SYS_PCI_CONFADDR, tag | reg);
	out32(RD94_SYS_PCI_CONFDATA, data);
	out32(RD94_SYS_PCI_CONFADDR, 0);
	splx(s);
}

int
necpb_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int pin = pa->pa_intrpin;
	int bus, dev;

	if (pin == 0) {
		/* No IRQ used. */
		*ihp = -1;
		return (1);
	}

	if (pin > 4) {
		printf("necpb_intr_map: bad interrupt pin %d\n", pin);
		*ihp = -1;
		return (1);
	}

	necpb_decompose_tag(pc, intrtag, &bus, &dev, NULL);
	if (bus != 0) {
		*ihp = -1;
		return (1);
	}

	switch (dev) {
	case 3:
		*ihp = (pin+2) % 4;
		break;
	case 4:
		*ihp = (pin+1) % 4;
		break;
	case 5:
		*ihp = (pin) % 4;
		break;
	default:
		*ihp = -1;
		return (1);
	}

	return (0);
}

const char *
necpb_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char str[8];

	if (ih >= 4)
		panic("necpb_intr_string: bogus handle %ld", ih);
	sprintf(str, "int %c", 'A' + (int)ih);
	return (str);
}

void *
necpb_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level, (*func) __P((void *));
	void *arg;
{
	struct necpb_intrhand *n, *p;
	u_int32_t	mask;

	if (ih >= 4)
		panic("necpb_intr_establish: bogus handle");

	n = malloc(sizeof(struct necpb_intrhand), M_DEVBUF, M_NOWAIT);
	if (n == NULL)
		panic("necpb_intr_establish: can't malloc interrupt handle");

	n->ih_func = func;
	n->ih_arg = arg;
	n->ih_next = NULL;
	n->ih_intn = ih;

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
necpb_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	struct necpb_intrhand *n, *p, *q;
	u_int32_t	mask;

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

	free(n, M_DEVBUF);
}

/*
 *   Handle PCI/EISA interrupt.
 */
int
necpb_intr(mask, cf)
	unsigned mask;
	struct clockframe *cf;
{
	u_int32_t vector, stat;
	struct necpb_intrhand *p;
	int a;

	vector = in32(RD94_SYS_INTSTAT2) & 0xffff;

	if (vector == 0x4000) {
		stat = in32(RD94_SYS_PCI_INTSTAT);
		stat &= in32(RD94_SYS_PCI_INTMASK);
		for (a=0; a<4; a++) {
			if (stat & (1 << a)) {
#if 0
				printf("pint %d\n", a);
#endif
				p = necpb_inttbl[a];
				while (p != NULL) {
					(*p->ih_func)(p->ih_arg);
					p = p->ih_next;
				}
			}
		}
	} else if (vector == 0x8000) {
		printf("eisa_nmi\n");
	} else {
		printf("eint %d\n", vector & 0xff);
#if 0
		eisa_intr(vector & 0xff);
#endif
	}

	return (~0);
}
