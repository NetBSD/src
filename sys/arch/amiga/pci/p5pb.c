/*	$NetBSD: p5pb.c,v 1.11.2.2 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2011, 2012 The NetBSD Foundation, Inc.
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
#include <sys/kmem.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#define _M68K_BUS_DMA_PRIVATE
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
#ifdef PCI_NETBSD_CONFIGURE
#include <dev/pci/pciconf.h>
#endif /* PCI_NETBSD_CONFIGURE */

#include "opt_p5pb.h"
#include "opt_pci.h"

/* Initial CVPPC/BVPPC resolution as configured by the firmware */
#define P5GFX_WIDTH		640
#define P5GFX_HEIGHT		480
#define P5GFX_DEPTH		8
#define P5GFX_LINEBYTES		640

struct m68k_bus_dma_tag p5pb_bus_dma_tag = {
	0,
	0, 
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load_direct,
	_bus_dmamap_load_mbuf_direct,
	_bus_dmamap_load_uio_direct,
	_bus_dmamap_load_raw_direct,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};

static int	p5pb_match(device_t, cfdata_t, void *);
static void	p5pb_attach(device_t, device_t, void *);
void		p5pb_set_props(struct p5pb_softc *);
pcireg_t	p5pb_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		p5pb_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		p5pb_pci_bus_maxdevs_cvppc(pci_chipset_tag_t, int); 
int		p5pb_pci_bus_maxdevs_grex1200(pci_chipset_tag_t, int); 
int		p5pb_pci_bus_maxdevs_grex4000(pci_chipset_tag_t, int); 
int		p5pb_pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);
void		p5pb_pci_attach_hook (device_t, device_t,
		    struct pcibus_attach_args *);
pcitag_t	p5pb_pci_make_tag(pci_chipset_tag_t, int, int, int);
void		p5pb_pci_decompose_tag(pci_chipset_tag_t, pcitag_t, 
		    int *, int *, int *);
int		p5pb_pci_intr_map(const struct pci_attach_args *, 
		    pci_intr_handle_t *);
bool		p5pb_bus_map_memio(struct p5pb_softc *);
bool		p5pb_bus_map_conf(struct p5pb_softc *);
uint8_t		p5pb_find_resources(struct p5pb_softc *);
static bool	p5pb_identify_bridge(struct p5pb_softc *);
void		p5pb_membar_grex(struct p5pb_softc *);
static bool	p5pb_cvppc_probe(struct p5pb_softc *);
#ifdef PCI_NETBSD_CONFIGURE
bool		p5pb_bus_reconfigure(struct p5pb_softc *);
#endif /* PCI_NETBSD_CONFIGURE */
#ifdef P5PB_DEBUG
void		p5pb_usable_ranges(struct p5pb_softc *);
void		p5pb_badaddr_range(struct p5pb_softc *, bus_space_tag_t, 
		    bus_addr_t, size_t);
void		p5pb_conf_search(struct p5pb_softc *, uint16_t);
#endif /* P5PB_DEBUG */

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
	sc->sc_dev = self;
	sc->p5baa = (struct p5bus_attach_args *) aux;

	pci_chipset_tag_t pc = &sc->apc;

	if (!p5pb_bus_map_conf(sc)) {
		aprint_error_dev(self,
		    "couldn't map PCI configuration space\n");
		return;
	}

	if (!p5pb_identify_bridge(sc)) {
		return;
	}

	if (sc->bridge_type == P5PB_BRIDGE_CVPPC) {
		sc->pci_mem_lowest = P5BUS_PCI_MEM_BASE;
		sc->pci_mem_highest = P5BUS_PCI_MEM_BASE + P5BUS_PCI_MEM_SIZE;
	} else {
		p5pb_membar_grex(sc);
	}

	if (!p5pb_bus_map_memio(sc)) {
		aprint_error_dev(self,
		    "couldn't map PCI I/O and memory space\n");
		return;
	}

#ifdef P5PB_DEBUG
	aprint_normal("p5pb: map conf %x -> %x, io %x -> %x, mem %x -> %x\n",
	    kvtop((void*) sc->pci_conf_area.base), sc->pci_conf_area.base,
	    kvtop((void*) sc->pci_io_area.base), sc->pci_io_area.base,
	    kvtop((void*) sc->pci_mem_area.base), sc->pci_mem_area.base ); 
#endif 

	/* Initialize the PCI chipset tag. */

	if (sc->bridge_type == P5PB_BRIDGE_GREX1200)
		sc->apc.pc_bus_maxdevs = p5pb_pci_bus_maxdevs_grex1200;
	else if (sc->bridge_type == P5PB_BRIDGE_GREX4000)
		sc->apc.pc_bus_maxdevs = p5pb_pci_bus_maxdevs_grex4000;
	else
		sc->apc.pc_bus_maxdevs = p5pb_pci_bus_maxdevs_cvppc;

	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_make_tag = amiga_pci_make_tag;
	sc->apc.pc_decompose_tag = amiga_pci_decompose_tag;
	sc->apc.pc_conf_read = p5pb_pci_conf_read;
	sc->apc.pc_conf_write = p5pb_pci_conf_write;
	sc->apc.pc_conf_hook = p5pb_pci_conf_hook;
	sc->apc.pc_conf_interrupt = amiga_pci_conf_interrupt;
	sc->apc.pc_attach_hook = p5pb_pci_attach_hook;
      
	sc->apc.pc_intr_map = p5pb_pci_intr_map; 
	sc->apc.pc_intr_string = amiga_pci_intr_string;	
	sc->apc.pc_intr_establish = amiga_pci_intr_establish;
	sc->apc.pc_intr_disestablish = amiga_pci_intr_disestablish;

#ifdef PCI_NETBSD_CONFIGURE
	/* Never reconfigure the bus on CVPPC/BVPPC, avoid the fb breakage. */
	if (sc->bridge_type != P5PB_BRIDGE_CVPPC) {
		p5pb_bus_reconfigure(sc);
	}
#endif /* PCI_NETBSD_CONFIGURE */

	/* Initialize the bus attachment structure. */
 
	pba.pba_iot = &(sc->pci_io_area);
	pba.pba_memt = &(sc->pci_mem_area);
	pba.pba_dmat = &p5pb_bus_dma_tag; 
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY; 
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	p5pb_set_props(sc);

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

/*
 * Try to detect what kind of bridge are we dealing with. 
 */
static bool 
p5pb_identify_bridge(struct p5pb_softc *sc) 
{
	int pcires_count;	/* Number of AutoConfig(TM) PCI resources */

	pcires_count = p5pb_find_resources(sc);

	switch (pcires_count) {
	case 0:
		/*
		 * Zero AutoConfig(TM) PCI resources, means that there's nothing
		 * OR there's a CVPPC/BVPPC with a pre-44.69 firmware.
		 */ 
		if (p5pb_cvppc_probe(sc)) {
			sc->bridge_type = P5PB_BRIDGE_CVPPC;
			aprint_normal(": Phase5 CVPPC/BVPPC PCI bridge\n");
		} else {
			aprint_normal(": no PCI bridges detected\n");
			return false;
		}
		break;
	case 6:
		/*
		 * We have a slight possibility, that there's a CVPPC/BVPPC with
		 * the new firmware. So check for it first. 
		 */
		if (p5pb_cvppc_probe(sc)) {
			/* New firmware, treat as one-slot GREX. */
			sc->bridge_type = P5PB_BRIDGE_CVPPC;
			aprint_normal(
			    ": Phase5 CVPPC/BVPPC PCI bridge (44.69/44.71)\n");
			break;
		}
	default:
		/* We have a G-REX surely. */

		if (sc->p5baa->p5baa_cardtype == P5_CARDTYPE_CS) {
			sc->bridge_type = P5PB_BRIDGE_GREX4000;
			aprint_normal(": DCE G-REX 4000 PCI bridge\n");
		} else {
			sc->bridge_type = P5PB_BRIDGE_GREX1200;
			aprint_normal(": DCE G-REX 1200 PCI bridge\n");
		}
		break;
	}
	return true;
}

/* 
 * Find AutoConfig(TM) resuorces (for boards running G-REX firmware). Return the
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
 * autoconfg in device_register(). Needed for CVPPC/BVPPC.
 */
void
p5pb_set_props(struct p5pb_softc *sc) 
{
#if NGENFB > 0
	prop_dictionary_t dict;
	device_t dev;
	
	dev = sc->sc_dev;
	dict = device_properties(dev);

	/* genfb needs additional properties, like virtual, physical address */
	/* XXX: currently genfb is supported only on CVPPC/BVPPC */
	prop_dictionary_set_uint64(dict, "virtual_address",
	    sc->pci_mem_area.base);
	prop_dictionary_set_uint64(dict, "address", 
	    kvtop((void*) sc->pci_mem_area.base)); 
#endif
}

pcireg_t
p5pb_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t data;
	uint32_t bus, dev, func;
	uint32_t offset;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	offset = (OFF_PCI_DEVICE << dev) + reg;

	if(func == 0)	/* ugly, ugly hack */
		offset += 0;
	else if(func == 1)
		offset += OFF_PCI_FUNCTION;
	else
		return 0xFFFFFFFF;	

	if(badaddr((void *)__UNVOLATILE(((uint32_t)
	    bus_space_vaddr(pc->pci_conf_datat, pc->pci_conf_datah) 
	    + offset)))) 
		return 0xFFFFFFFF;	

	data = bus_space_read_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    offset);
#ifdef P5PB_DEBUG_CONF
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
	uint32_t offset;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	offset = (OFF_PCI_DEVICE << dev) + reg;

	if(func == 0)	/* ugly, ugly hack */
		offset += 0;
	else if(func == 1)
		offset += OFF_PCI_FUNCTION;
	else
		return;	

	if(badaddr((void *)__UNVOLATILE(((uint32_t)
	    bus_space_vaddr(pc->pci_conf_datat, pc->pci_conf_datah) 
	    + offset)))) 
		return;	

	bus_space_write_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    offset, val);
#ifdef P5PB_DEBUG_CONF
	aprint_normal("p5pb conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, val);
#endif
	
}

int
p5pb_pci_bus_maxdevs_cvppc(pci_chipset_tag_t pc, int busno) 
{
	/* CVPPC/BVPPC has only 1 "slot". */
	return 1;
}

int
p5pb_pci_bus_maxdevs_grex4000(pci_chipset_tag_t pc, int busno) 
{
	/* G-REX 4000 has 4, G-REX 4000T has 3 slots? */
	return 4;
}

int
p5pb_pci_bus_maxdevs_grex1200(pci_chipset_tag_t pc, int busno) 
{
	/* G-REX 1200 has 5 slots. */
	return 4; /* XXX: 5 not yet! */
}

void
p5pb_pci_attach_hook(device_t parent, device_t self,
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

/* Probe for CVPPC/BVPPC. */
static bool
p5pb_cvppc_probe(struct p5pb_softc *sc) 
{
	bus_space_handle_t probe_h;
	uint16_t prodid, manid;
	void* data;
	bool rv;

	manid = 0; prodid = 0;
	rv = false;

	if (bus_space_map(sc->apc.pci_conf_datat, 0, 4, 0, &probe_h)) 
		return rv; 

	data = bus_space_vaddr(sc->apc.pci_conf_datat, probe_h);

	if (badaddr((void *)__UNVOLATILE((uint32_t) data))) {
#ifdef P5PB_DEBUG_PROBE
		aprint_normal("p5pb: CVPPC configuration space not usable!\n");
#endif /* P5PB_DEBUG_PROBE */
	} else {
		prodid = bus_space_read_2(sc->apc.pci_conf_datat, probe_h, 0);	
		manid = bus_space_read_2(sc->apc.pci_conf_datat, probe_h, 2);	

		if ((prodid == P5PB_PM2_PRODUCT_ID) && 
		    (manid == P5PB_PM2_VENDOR_ID)) 
			rv = true; 
	}

#ifdef P5PB_DEBUG_PROBE
	aprint_normal("p5pb: CVPPC probe for PCI ID: %x, %x returns %d\n",
	    manid, prodid, (int) rv);
#endif /* P5PB_DEBUG_PROBE */

	bus_space_unmap(sc->apc.pci_conf_datat, probe_h, 4);
	return rv;	
}

#ifdef PCI_NETBSD_CONFIGURE 
/* Reconfigure the bus. */
bool
p5pb_bus_reconfigure(struct p5pb_softc *sc) 
{
	struct extent		*ioext, *memext; 
	pci_chipset_tag_t	pc;

	pc = &sc->apc;

	ioext = extent_create("p5pbio", 0, P5BUS_PCI_IO_SIZE, NULL, 0,
	    EX_NOWAIT);

	memext = extent_create("p5pbmem", sc->pci_mem_lowest, 
	     sc->pci_mem_highest - 1, NULL, 0, EX_NOWAIT);
	
	if ( (!ioext) || (!memext) ) 
		return false;

#ifdef P5PB_DEBUG 
	aprint_normal("p5pb: reconfiguring the bus!\n");
#endif /* P5PB_DEBUG */
	pci_configure_bus(pc, ioext, memext, NULL, 0, CACHELINE_SIZE);

	extent_destroy(ioext);
	extent_destroy(memext);

	return true; /* TODO: better error handling */
}
#endif /* PCI_NETBSD_CONFIGURE */

/* Determine the PCI memory space (done G-REX-style). */
void
p5pb_membar_grex(struct p5pb_softc *sc)
{
	struct p5pb_autoconf_entry *membar_entry;
	uint32_t bar_address; 

	sc->pci_mem_lowest = 0xFFFFFFFF;
	sc->pci_mem_highest = 0;

	/* Iterate over membar entries to find lowest and highest address. */
	TAILQ_FOREACH(membar_entry, &sc->auto_bars, entries) {

		bar_address = (uint32_t) membar_entry->base;
		if ((bar_address + membar_entry->size) > sc->pci_mem_highest)
			sc->pci_mem_highest = bar_address + membar_entry->size;
		if (bar_address < sc->pci_mem_lowest)
			sc->pci_mem_lowest = bar_address;

#ifdef P5PB_DEBUG_BAR
		aprint_normal("p5pb: %d kB mem BAR at %p, hi = %x, lo = %x\n",
		    membar_entry->size / 1024, membar_entry->base, 
		    sc->pci_mem_highest, sc->pci_mem_lowest);
#endif /* P5PB_DEBUG_BAR */
	}

	aprint_normal("p5pb: %d kB PCI memory space (%8p to %8p)\n",
	    (sc->pci_mem_highest - sc->pci_mem_lowest) / 1024, 
	     (void*) sc->pci_mem_lowest, (void*) sc->pci_mem_highest);

}

bool
p5pb_bus_map_conf(struct p5pb_softc *sc) 
{
	sc->pci_conf_area.base = (bus_addr_t) zbusmap(
	    (void *) P5BUS_PCI_CONF_BASE, P5BUS_PCI_CONF_SIZE);
	sc->pci_conf_area.absm = &amiga_bus_stride_1;

	sc->apc.pci_conf_datat = &(sc->pci_conf_area);

	if (bus_space_map(sc->apc.pci_conf_datat, OFF_PCI_CONF_DATA, 
	    P5BUS_PCI_CONF_SIZE, 0, &sc->apc.pci_conf_datah)) 
		return false;

	return true;
}

/* Map I/O and memory space. */
bool
p5pb_bus_map_memio(struct p5pb_softc *sc)
{
	sc->pci_io_area.base = (bus_addr_t) zbusmap(
	    (void *) P5BUS_PCI_IO_BASE, P5BUS_PCI_IO_SIZE);
	sc->pci_io_area.absm = &amiga_bus_stride_1swap;

	sc->pci_mem_area.base = (bus_addr_t) zbusmap(
	    (void *) sc->pci_mem_lowest, 
	    sc->pci_mem_highest - sc->pci_mem_lowest);
	sc->pci_mem_area.absm = &amiga_bus_stride_1swap_abs;

	return true;
}

int
p5pb_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, 
    int func, pcireg_t id) 
{	
	/* XXX: What should we do on CVPPC/BVPPC? It breaks genfb. */

	return PCI_CONF_DEFAULT;
}

#ifdef P5PB_DEBUG
/* Check which config and I/O ranges are usable. */
void
p5pb_usable_ranges(struct p5pb_softc *sc) 
{
	p5pb_badaddr_range(sc, &(sc->pci_conf_area), 0, P5BUS_PCI_CONF_SIZE);
	p5pb_badaddr_range(sc, &(sc->pci_io_area), 0, P5BUS_PCI_IO_SIZE);
}

void
p5pb_badaddr_range(struct p5pb_softc *sc, bus_space_tag_t bust, bus_addr_t base,
    size_t len)
{
	int i, state, prev_state;
	bus_space_handle_t bush;
	volatile void *data;

	state = -1;
	prev_state = -1;

	bus_space_map(bust, base, len, 0, &bush);

	aprint_normal("p5pb: badaddr range check from %x (%x) to %x (%x)\n", 
	    (bus_addr_t) bush,			/* start VA */
	    (bus_addr_t) kvtop((void*) bush),	/* start PA */
	    (bus_addr_t) bush + len,		/* end VA */
	    (bus_addr_t) kvtop((void*) (bush + len)));/* end PA */

	data = bus_space_vaddr(bust, bush);

	for(i = 0; i < len; i++) {
		state = badaddr((void *)__UNVOLATILE(((uint32_t) data + i)));
		if(state != prev_state) {
			aprint_normal("p5pb: badaddr %p (%x) : %d\n", 
			    (void*) ((uint32_t) data + i),
			    (bus_addr_t) kvtop((void*) ((uint32_t) data + i)), 
			    state);
			prev_state = state;
		}
			
	}

	bus_space_unmap(bust, bush, len);
}

/* Search for 16-bit value in the configuration space. */
void
p5pb_conf_search(struct p5pb_softc *sc, uint16_t val) 
{
	int i, state;
	uint16_t readv;
	void *va;

	va = bus_space_vaddr(sc->apc.pci_conf_datat, sc->apc.pci_conf_datah);

	for (i = 0; i < P5BUS_PCI_CONF_SIZE; i++) {
		state = badaddr((void *)__UNVOLATILE(((uint32_t) va + i)));
		if(state == 0) {
			readv = bus_space_read_2(sc->apc.pci_conf_datat, 
			    sc->apc.pci_conf_datah, i);
			if(readv == val)
				aprint_normal("p5pb: found val %x @ %x (%x)\n",
				    readv, (uint32_t) sc->apc.pci_conf_datah 
				    + i, (bus_addr_t) kvtop((void*) 
				    ((uint32_t) sc->apc.pci_conf_datah + i)));
		}		
	}
}

#endif /* P5PB_DEBUG */

#ifdef P5PB_CONSOLE
void
p5pb_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict, parent_dict;
	struct pci_attach_args *pa = aux;

	if (device_parent(dev) && device_is_a(device_parent(dev), "pci")) {

		dict = device_properties(dev);

		if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY) {

			/* Handle the CVPPC/BVPPC card... */
			if ( ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_TI)
			    && (PCI_PRODUCT(pa->pa_id) ==
			    PCI_PRODUCT_TI_TVP4020) ) ||
			    /* ...and 3Dfx Voodoo 3 in G-REX. */
			    ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_3DFX)
			    && (PCI_PRODUCT(pa->pa_id) ==
			    PCI_PRODUCT_3DFX_VOODOO3) )) {

				parent_dict = device_properties(
				    device_parent(device_parent(dev)));

				prop_dictionary_set_uint32(dict, "width",
				    P5GFX_WIDTH);

				prop_dictionary_set_uint32(dict, "height",
				    P5GFX_HEIGHT);

				prop_dictionary_set_uint32(dict, "depth",
				    P5GFX_DEPTH);

#if (NGENFB > 0)
				prop_dictionary_set_uint32(dict, "linebytes",
				    P5GFX_LINEBYTES);

				prop_dictionary_set(dict, "address",
				    prop_dictionary_get(parent_dict,
				    "address"));
				prop_dictionary_set(dict, "virtual_address",
				    prop_dictionary_get(parent_dict,
				    "virtual_address"));
#endif
				prop_dictionary_set_bool(dict, "is_console",
				    true);
                        }
                }
        }
}
#endif /* P5PB_CONSOLE */
