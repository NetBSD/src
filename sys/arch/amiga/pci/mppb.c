/*	$NetBSD: mppb.c,v 1.8 2015/10/02 05:22:49 msaitoh Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/* Matay Prometheus Zorro-PCI bridge driver. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <m68k/bus_dma.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/pci/mppbreg.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#include "opt_pci.h"

/* Zorro IDs */
#define ZORRO_MANID_MATAY	44359
#define ZORRO_PRODID_PROMETHEUS	1

struct mppb_softc {
	device_t sc_dev;
	volatile char *ba;
	struct bus_space_tag pci_conf_area;
	struct bus_space_tag pci_io_area;
	struct bus_space_tag pci_mem_area;
	struct amiga_pci_chipset apc;	
};

static int	mppb_match(device_t, cfdata_t, void *);
static void	mppb_attach(device_t, device_t, void *);
pcireg_t	mppb_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		mppb_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		mppb_pci_bus_maxdevs(pci_chipset_tag_t, int); 
void		mppb_pci_attach_hook (device_t, device_t,
		    struct pcibus_attach_args *pba);
pcitag_t	mppb_pci_make_tag(pci_chipset_tag_t, int, int, int);
void		mppb_pci_decompose_tag(pci_chipset_tag_t, pcitag_t, 
		    int *, int *, int *);
int		mppb_pci_intr_map(const struct pci_attach_args *, 
		    pci_intr_handle_t *);
const struct evcnt * mppb_pci_intr_evcnt(pci_chipset_tag_t, 
		    pci_intr_handle_t);

CFATTACH_DECL_NEW(mppb, sizeof(struct mppb_softc),
    mppb_match, mppb_attach, NULL, NULL);

static int
mppb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != ZORRO_MANID_MATAY)
		return 0;

	if (zap->prodid != ZORRO_PRODID_PROMETHEUS)
		return 0;
		
	return 1;
}


static void
mppb_attach(device_t parent, device_t self, void *aux)
{
	struct mppb_softc *sc;
	struct pcibus_attach_args pba;  
	struct zbus_args *zap;
	pci_chipset_tag_t pc;
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#endif /* PCI_NETBSD_CONFIGURE */

	zap = aux;
	sc = device_private(self);
	pc = &sc->apc;
	sc->sc_dev = self;
	sc->ba = zap->va;

	aprint_normal(": Matay Prometheus PCI bridge\n"); 

	/* Setup bus space mappings. */
	sc->pci_conf_area.base = (bus_addr_t) sc->ba + MPPB_CONF_BASE;
	sc->pci_conf_area.absm = &amiga_bus_stride_1swap;

	sc->pci_mem_area.base = (bus_addr_t) sc->ba + MPPB_MEM_BASE;
	sc->pci_mem_area.absm = &amiga_bus_stride_1;

	sc->pci_io_area.base = (bus_addr_t) sc->ba + MPPB_IO_BASE;
	sc->pci_io_area.absm = &amiga_bus_stride_1;
	
#ifdef MPPB_DEBUG 
	aprint_normal("mppb mapped conf %x->%x, mem %x->%x\n, io %x->%x\n",
	    kvtop((void*) sc->pci_conf_area.base), sc->pci_conf_area.base,
	    kvtop((void*) sc->pci_mem_area.base), sc->pci_mem_area.base,
	    kvtop((void*) sc->pci_io_area.base), sc->pci_io_area.base); 
#endif 

	sc->apc.pci_conf_datat = &(sc->pci_conf_area);

	if (bus_space_map(sc->apc.pci_conf_datat, 0, MPPB_CONF_SIZE, 0, 
	    &sc->apc.pci_conf_datah)) 
		aprint_error_dev(self,
		    "couldn't map PCI configuration data space\n");
	
	/* Initialize the PCI chipset tag. */
	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_bus_maxdevs = mppb_pci_bus_maxdevs;
	sc->apc.pc_make_tag = amiga_pci_make_tag;
	sc->apc.pc_decompose_tag = amiga_pci_decompose_tag;
	sc->apc.pc_conf_read = mppb_pci_conf_read;
	sc->apc.pc_conf_write = mppb_pci_conf_write;
	sc->apc.pc_attach_hook = mppb_pci_attach_hook;

	sc->apc.pc_intr_map = mppb_pci_intr_map;
	sc->apc.pc_intr_string = amiga_pci_intr_string;
	sc->apc.pc_intr_establish = amiga_pci_intr_establish;
	sc->apc.pc_intr_disestablish = amiga_pci_intr_disestablish;

	sc->apc.pc_conf_hook = amiga_pci_conf_hook;
	sc->apc.pc_conf_interrupt = amiga_pci_conf_interrupt;

#ifdef PCI_NETBSD_CONFIGURE
	ioext = extent_create("mppbio",  MPPB_IO_BASE, 
	    MPPB_IO_BASE + MPPB_IO_SIZE, NULL, 0, EX_NOWAIT);
	memext = extent_create("mppbmem",  MPPB_MEM_BASE, 
	    MPPB_MEM_BASE + MPPB_MEM_SIZE, NULL, 0, EX_NOWAIT);

#ifdef MPPB_DEBUG	
	aprint_normal("mppb: reconfiguring the bus!\n");
#endif /* MPPB_DEBUG */
	pci_configure_bus(pc, ioext, memext, NULL, 0, CACHELINE_SIZE);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif /* PCI_NETBSD_CONFIGURE */

	pba.pba_iot = &(sc->pci_io_area);
	pba.pba_memt = &(sc->pci_mem_area);
	pba.pba_dmat = NULL; 
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

pcireg_t
mppb_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t data;
	uint32_t bus, dev, func;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	data = bus_space_read_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    (MPPB_CONF_STRIDE*dev) + reg);
#ifdef MPPB_DEBUG_CONF
	aprint_normal("mppb conf read va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -r-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, data);
#endif /* MPPB_DEBUG_CONF */
	return data;
}

void
mppb_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	uint32_t bus, dev, func;
	
	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	bus_space_write_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    (MPPB_CONF_STRIDE*dev) + reg, val);
#ifdef MPPB_DEBUG_CONF
	aprint_normal("mppb conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, val);
#endif /* MPPB_DEBUG_CONF */
	
}

int
mppb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno) 
{
	return 4; /* Prometheus has 4 slots */
}

void
mppb_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
mppb_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/* TODO: add sanity checking */

	*ihp = MPPB_INT; 
	return 0;
}

const struct evcnt *
mppb_pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	/* TODO: implement */
	return NULL;
}
