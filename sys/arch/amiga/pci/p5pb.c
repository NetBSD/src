/*	$NetBSD: p5pb.c,v 1.5 2012/01/10 20:29:50 rkujawa Exp $ */

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
#include <amiga/dev/p5busvar.h>
#include <amiga/pci/p5pbreg.h>
#include <amiga/pci/p5pbvar.h>
#include <amiga/pci/p5membarvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

/* Initial CVPPC/BVPPC resolution as configured by the firmware */
#define P5GFX_WIDTH		640
#define P5GFX_HEIGHT		480
#define P5GFX_DEPTH		8
#define P5GFX_LINEBYTES		640

static int	p5pb_match(struct device *, struct cfdata *, void *);
static void	p5pb_attach(struct device *, struct device *, void *);
void		p5pb_set_props(struct p5pb_softc *sc);
pcireg_t	p5pb_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		p5pb_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		p5pb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno); 
int		p5pb_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, 
		    int func, pcireg_t id);
void		p5pb_pci_attach_hook (struct device *parent, 
		    struct device *self, struct pcibus_attach_args *pba);
pcitag_t	p5pb_pci_make_tag(pci_chipset_tag_t pc, int bus, int device, 
		    int function);
void		p5pb_pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, 
		    int *bp, int *dp, int *fp);
int		p5pb_pci_intr_map(const struct pci_attach_args *pa, 
		    pci_intr_handle_t *ihp);
bool		p5pb_bus_map_cvppc(struct p5pb_softc *sc);
bool		p5pb_bus_map_grex(struct p5pb_softc *sc);
bool		p5pb_bus_map_common(struct p5pb_softc *sc);
uint8_t		p5pb_find_resources(struct p5pb_softc *sc);

CFATTACH_DECL_NEW(p5pb, sizeof(struct p5pb_softc),
    p5pb_match, p5pb_attach, NULL, NULL);

static int
p5pb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct p5bus_attach_args *p5baa;

	p5baa = (struct p5bus_attach_args *) aux;

	if (strcmp(p5baa->p5baa_name, "p5pb") == 0)
		return 1;

	return 0;
}


static void
p5pb_attach(device_t parent, device_t self, void *aux)
{
	struct p5pb_softc *sc; 
	struct pcibus_attach_args pba;  

	sc = device_private(self);
	pci_chipset_tag_t pc = &sc->apc;
	sc->sc_dev = self;

	if (p5pb_find_resources(sc) > 0) {
		sc->p5pb_bus_map = &p5pb_bus_map_grex;
	} else {
#ifdef P5PB_OLD_FIRMWARE
		sc->p5pb_bus_map = &p5pb_bus_map_cvppc;
#else
		aprint_normal(": no PCI bridges detected\n");
		return;
#endif /* P5PB_OLD_FIRMWARE */
	}

	aprint_normal(": Phase5 CVPPC/BVPPC/G-REX PCI bridge\n");

	if(!p5pb_bus_map_common(sc)) {
		aprint_error_dev(self,
		    "couldn't map PCI configuration and I/O spaces\n");
		return;
	}

	if(!(sc->p5pb_bus_map(sc))) {
		aprint_error_dev(self,
		    "couldn't map PCI memory space\n");
		return;
	}

#ifdef P5PB_DEBUG
	aprint_normal("p5pb: mapped %x -> %x, %x -> %x\n, %x -> %x\n",
	    P5BUS_PCI_CONF_BASE, sc->pci_conf_area.base,
	    P5BUS_PCI_IO_BASE, sc->pci_io_area.base,
	    P5BUS_PCI_MEM_BASE, sc->pci_mem_area.base ); 
#endif 

	/* Initialize the PCI chipset tag. */
	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_bus_maxdevs = p5pb_pci_bus_maxdevs;
	sc->apc.pc_make_tag = amiga_pci_make_tag;
	sc->apc.pc_decompose_tag = amiga_pci_decompose_tag;
	sc->apc.pc_conf_read = p5pb_pci_conf_read;
	sc->apc.pc_conf_write = p5pb_pci_conf_write;
	sc->apc.pc_attach_hook = p5pb_pci_attach_hook;
      
	sc->apc.pc_intr_map = p5pb_pci_intr_map; 
	sc->apc.pc_intr_string = amiga_pci_intr_string;	
	sc->apc.pc_intr_establish = amiga_pci_intr_establish;
	sc->apc.pc_intr_disestablish = amiga_pci_intr_disestablish;
 
	pba.pba_iot = &(sc->pci_io_area);
	pba.pba_memt = &(sc->pci_mem_area);
	pba.pba_dmat = NULL; 
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY; 
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

#ifdef P5PB_GENFB
	p5pb_set_props(sc);
#endif /* P5PB_GENFB */

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

/* 
 * Find autoconfigured resuorces (for boards running G-REX firmware). Return the
 * total number of found resources.
 */
uint8_t
p5pb_find_resources(struct p5pb_softc *sc) 
{
	uint8_t i, rv;
	struct p5pb_autoconf_entry *auto_entry;
	struct p5membar_softc *membar_sc;
	device_t p5membar_dev;
	
	rv = 0;

	TAILQ_INIT(&sc->auto_bars);

	/* 255 should be enough for everybody */
	for(i = 0; i < 255; i++) {

		if ((p5membar_dev = 
		    device_find_by_driver_unit("p5membar", i)) != NULL) {

			rv++;

			membar_sc = device_private(p5membar_dev);
			if (membar_sc->sc_type == P5MEMBAR_TYPE_INTERNAL)
				continue;

			auto_entry = 
			    kmem_alloc(sizeof(struct p5pb_autoconf_entry), 
			    KM_SLEEP);

			auto_entry->base = membar_sc->sc_base;
			auto_entry->size = membar_sc->sc_size;

			TAILQ_INSERT_TAIL(&sc->auto_bars, auto_entry, entries);
		}
	}	
	return rv;	
}

/*
 * Set properties needed to support fb driver. These are read later during
 * autoconfg in device_register().
 */
void
p5pb_set_props(struct p5pb_softc *sc) 
{
	prop_dictionary_t dict;
	device_t dev;
	
	dev = sc->sc_dev;
	dict = device_properties(dev);
	
	prop_dictionary_set_uint32(dict, "width", P5GFX_WIDTH);
	prop_dictionary_set_uint32(dict, "height", P5GFX_HEIGHT);
	prop_dictionary_set_uint8(dict, "depth", P5GFX_DEPTH);
	prop_dictionary_set_uint16(dict, "linebytes", P5GFX_LINEBYTES);
	prop_dictionary_set_uint64(dict, "address", P5BUS_PCI_MEM_BASE);
#if (NGENFB > 0)
	/*
	 * Framebuffer starts at P5BUS_PCI_MEM_BASE, but genfb needs virtual
	 * address.
	 */
	prop_dictionary_set_uint64(dict, "virtual_address",
	    sc->pci_mem_area.base);
#endif
}

pcireg_t
p5pb_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t data;
	uint32_t bus, dev, func;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	data = bus_space_read_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    (func<<5) + reg);
#ifdef P5PB_DEBUG
	aprint_normal("p5pb conf read va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -r-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, data);
#endif
	return data;
}

void
p5pb_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	uint32_t bus, dev, func;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	bus_space_write_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    (func << 5) + reg, val);
#ifdef P5PB_DEBUG
	aprint_normal("p5pb conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, val);
#endif
	
}

int
p5pb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno) 
{
	/* G-REX has max 5 slots. CVPPC/BVPPC has only 1. */
	return 1;
}

void
p5pb_pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

int
p5pb_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/* TODO: add sanity checking */

	*ihp = 2; 
	return 0;
}

/* PCI memory mapping done G-REX-style. */
bool
p5pb_bus_map_grex(struct p5pb_softc *sc)
{
	struct p5pb_autoconf_entry *membar_entry;
	bus_addr_t bar_address;
	bus_addr_t pci_mem_highest;

	pci_mem_highest = P5BUS_PCI_MEM_BASE;

#ifdef P5PB_DEBUG
	aprint_normal("p5pb: p5pb_bus_map_grex called\n");
#endif /* P5PB_DEBUG */

	/* Determine the highest address used by any PCI card. */
	TAILQ_FOREACH(membar_entry, &sc->auto_bars, entries) {

		bar_address = (bus_addr_t) membar_entry->base;
		if ((bar_address + membar_entry->size) > pci_mem_highest)
			pci_mem_highest = bar_address + membar_entry->size;

#ifdef P5PB_DEBUG
		aprint_normal("p5pb: memory BAR at %p, highest address %x\n", 
		    membar_entry->base, pci_mem_highest);
#endif /* P5PB_DEBUG */
	}

	sc->pci_mem_area.base = (bus_addr_t) zbusmap(
	    (void *) P5BUS_PCI_MEM_BASE, pci_mem_highest - P5BUS_PCI_MEM_BASE);
	sc->pci_mem_area.absm = &amiga_bus_stride_1swap_abs;

	return true;
}

/* Map things common for all supported bridges. */
bool
p5pb_bus_map_common(struct p5pb_softc *sc)
{
#ifdef P5PB_DEBUG
	aprint_normal("p5pb: p5pb_bus_map_common called\n");
#endif /* P5PB_DEBUG */

	sc->pci_conf_area.base = (bus_addr_t) zbusmap(
	    (void *) P5BUS_PCI_CONF_BASE, P5BUS_PCI_CONF_SIZE);
	sc->pci_conf_area.absm = &amiga_bus_stride_1;

	sc->pci_io_area.base = (bus_addr_t) zbusmap(
	    (void *) P5BUS_PCI_IO_BASE, P5BUS_PCI_IO_SIZE);
	sc->pci_io_area.absm = &amiga_bus_stride_1swap_abs;

	sc->apc.pci_conf_datat = &(sc->pci_conf_area);
	sc->apc.pci_conf_addresst = &(sc->pci_conf_area);

	if (bus_space_map(sc->apc.pci_conf_datat, OFF_PCI_CONF_DATA, 
	    256, 0, &sc->apc.pci_conf_datah)) 
		return false;

	/* XXX */
	/* if (bus_space_map(sc->apc.pci_conf_addresst, OFF_PCI_CONF_ADDR, 
	    256, 0, &sc->apc.pci_conf_addressh)) 
		return false; */

	return true;
}

/* Hard-coded memory mapping for CVPPC/BVPPC (without G-REX firmware). */
bool
p5pb_bus_map_cvppc(struct p5pb_softc *sc) 
{
#ifdef P5PB_DEBUG
	aprint_normal("p5pb: p5pb_bus_map_cvppc called\n");
#endif /* P5PB_DEBUG */
	sc->pci_mem_area.base = (bus_addr_t) zbusmap(
	    (void *) P5BUS_PCI_MEM_BASE, P5BUS_PCI_MEM_SIZE);
	sc->pci_mem_area.absm = &amiga_bus_stride_1swap_abs;

	return true;
}

