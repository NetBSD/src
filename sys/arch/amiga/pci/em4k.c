/*	$NetBSD: em4k.c,v 1.2.4.3 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

/* Elbox Mediator PCI 4000 driver. */

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

#include <amiga/dev/zbusvar.h>
#include <amiga/pci/empbreg.h>
#include <amiga/pci/em4kvar.h>
#include <amiga/pci/emmemvar.h>

#include <dev/pci/pciconf.h>

#include "opt_pci.h"

static int	em4k_match(device_t, cfdata_t, void *);
static void	em4k_attach(device_t, device_t, void *);
static void	em4k_callback(device_t);

static void	em4k_find_mem(struct em4k_softc *);
static void	em4k_intr_enable(struct em4k_softc *);

pcireg_t	em4k_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		em4k_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		em4k_pci_bus_maxdevs(pci_chipset_tag_t, int); 
void		em4k_pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
pcitag_t	em4k_pci_make_tag(pci_chipset_tag_t, int, int, int);
void		em4k_pci_decompose_tag(pci_chipset_tag_t, pcitag_t, 
		    int *, int *, int *);
int		em4k_pci_intr_map(const struct pci_attach_args *, 
		    pci_intr_handle_t *);
const struct evcnt * em4k_pci_intr_evcnt(pci_chipset_tag_t, 
		    pci_intr_handle_t);
int		em4k_pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);

#ifdef PCI_NETBSD_CONFIGURE
static void	em4k_pci_configure(struct em4k_softc *sc);
#endif /* PCI_NETBSD_CONFIGURE */

CFATTACH_DECL_NEW(em4k, sizeof(struct em4k_softc),
    em4k_match, em4k_attach, NULL, NULL);

static int
em4k_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != ZORRO_MANID_ELBOX)
		return 0;

	switch (zap->prodid) {
	case ZORRO_PRODID_MED4K:
	case ZORRO_PRODID_MED4KMKII:
		return 1;
	}

	return 0;
}


static void
em4k_attach(device_t parent, device_t self, void *aux)
{
	struct em4k_softc *sc;
	struct zbus_args *zap;

	volatile char *ba;

	zap = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	ba = zap->va;

	sc->model = zap->prodid;

	switch (sc->model) {
	case ZORRO_PRODID_MED4K:
		aprint_normal(": ELBOX Mediator PCI 4000\n"); 
		break;
	case ZORRO_PRODID_MED4KMKII:
		aprint_normal(": ELBOX Mediator PCI 4000 MK-II\n"); 
		break;
	default:
		aprint_normal(": ELBOX Mediator PCI (unknown)\n"); 
		break;
	}

	/* Setup bus space mappings. */
	sc->pci_conf_area.base = (bus_addr_t) ba + EM4K_CONF_OFF;
	sc->pci_conf_area.absm = &amiga_bus_stride_1swap;

	sc->pci_io_area.base = (bus_addr_t) ba + EM4K_IO_OFF;
	sc->pci_io_area.absm = &amiga_bus_stride_1swap;

	sc->setup_area.base = (bus_addr_t) ba + EM4K_SETUP_OFF;
	sc->setup_area.absm = &amiga_bus_stride_1;

	/* 
	 * Defer everything until later, we need to wait for possible
	 * emmem attachments.
	 */

	config_defer(self, em4k_callback);
}

static void
em4k_callback(device_t self) {

	struct em4k_softc *sc;
	pci_chipset_tag_t pc;
	struct pcibus_attach_args pba;  

	sc = device_private(self);
	pc = &sc->apc;

#ifdef EM4K_DEBUG 
	aprint_normal("em4k: mapped setup %x->%x, conf %x->%x, io %x->%x\n",
	    kvtop((void*) sc->setup_area.base), sc->setup_area.base,
	    kvtop((void*) sc->pci_conf_area.base), sc->pci_conf_area.base,
	    kvtop((void*) sc->pci_io_area.base), sc->pci_io_area.base);
#endif 

	sc->pci_conf_t = &(sc->pci_conf_area);
	sc->pci_io_t = &(sc->pci_io_area);

	if (bus_space_map(sc->pci_conf_t, 0, EM4K_CONF_SIZE, 0, 
	    &sc->pci_conf_h)) 
		aprint_error_dev(self, "couldn't map PCI config space\n");

	if (bus_space_map(sc->pci_io_t, 0, EM4K_IO_SIZE, 0, 
	    &sc->pci_io_h)) 
		aprint_error_dev(self, "couldn't map PCI I/O space\n");

	sc->apc.pci_conf_datat = sc->pci_conf_t;
	sc->apc.pci_conf_datah = sc->pci_conf_h;

	sc->setup_area_t = &(sc->setup_area);

	if (bus_space_map(sc->setup_area_t, 0, EM4K_SETUP_SIZE, 0, 
	    &sc->setup_area_h)) 
		aprint_error_dev(self,
		    "couldn't map Mediator setup space\n");

	em4k_find_mem(sc);
	if (sc->pci_mem_win_size == 0)
		aprint_error_dev(self,
		    "couldn't find memory space, this shouldn't happen!\n");

	/* Initialize the PCI chipset tag. */
	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_bus_maxdevs = em4k_pci_bus_maxdevs;
	sc->apc.pc_make_tag = amiga_pci_make_tag;
	sc->apc.pc_decompose_tag = amiga_pci_decompose_tag;
	sc->apc.pc_conf_read = em4k_pci_conf_read;
	sc->apc.pc_conf_write = em4k_pci_conf_write;
	sc->apc.pc_attach_hook = em4k_pci_attach_hook;

	sc->apc.pc_intr_map = em4k_pci_intr_map;
	sc->apc.pc_intr_string = amiga_pci_intr_string;
	sc->apc.pc_intr_establish = amiga_pci_intr_establish;
	sc->apc.pc_intr_disestablish = amiga_pci_intr_disestablish;

	sc->apc.pc_conf_hook = em4k_pci_conf_hook;
	sc->apc.pc_conf_interrupt = amiga_pci_conf_interrupt;

	sc->apc.cookie = sc;

#ifdef PCI_NETBSD_CONFIGURE
	em4k_pci_configure(sc);
#endif /* PCI_NETBSD_CONFIGURE */

	pba.pba_iot = &(sc->pci_io_area);
	pba.pba_dmat = NULL; 
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_OKAY;

	if (sc->pci_mem_win_size > 0) {
		pba.pba_memt = &(sc->pci_mem_win);
		pba.pba_flags |= PCI_FLAGS_MEM_OKAY;
	} else
		pba.pba_memt = NULL;

	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	/* Set correct window position. */
	bus_space_write_1(sc->setup_area_t, sc->setup_area_h,
	    EM4K_SETUP_WINDOW_OFF, kvtop((void*) sc->pci_mem_win.base) >> 
	    EM4K_WINDOW_SHIFT);

	em4k_intr_enable(sc);

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

#ifdef PCI_NETBSD_CONFIGURE
static void
em4k_pci_configure(struct em4k_softc *sc)
{
	struct extent *ioext, *memext;

	/* I/O addresses are relative to I/O space address. */
	ioext = extent_create("em4kio", 0, EM4K_IO_SIZE, 
	    NULL, 0, EX_NOWAIT);

	/*
	 * Memory space addresses are absolute (and keep in mind that
	 * they are in a separate address space.
	 */
	memext = extent_create("em4kmem", kvtop((void*) sc->pci_mem_win.base),
	    kvtop((void*) sc->pci_mem_win.base) + sc->pci_mem_win_size,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(&sc->apc, ioext, memext, NULL, 0, CACHELINE_SIZE);

	extent_destroy(ioext);
	extent_destroy(memext);
}
#endif /* PCI_NETBSD_CONFIGURE */

static void 
em4k_intr_enable(struct em4k_softc *sc) 
{
	bus_space_write_1(sc->setup_area_t, sc->setup_area_h,
	    EM4K_SETUP_INTR_OFF, EMPB_INTR_ENABLE);
}

/*
 * Try to find a (optional) memory window board.
 */
static void
em4k_find_mem(struct em4k_softc *sc)
{
	device_t memdev;
	struct emmem_softc *mem_sc;

	memdev = device_find_by_xname("emmem0");
	sc->pci_mem_win_size = 0;

	if (memdev == NULL) {
		return;
	}

	mem_sc = device_private(memdev);

	sc->pci_mem_win.base = (bus_addr_t) mem_sc->sc_base;
	sc->pci_mem_win.absm = &amiga_bus_stride_1swap_abs;
	sc->pci_mem_win_size = mem_sc->sc_size;
	sc->pci_mem_win_t = &sc->pci_mem_win;

	if (sc->pci_mem_win_size == 512*1024*1024)
		sc->pci_mem_win_mask = EM4K_WINDOW_MASK_512M;
	else if (sc->pci_mem_win_size == 256*1024*1024)
		sc->pci_mem_win_mask = EM4K_WINDOW_MASK_256M;
	else /* disable anyway */
		sc->pci_mem_win_size = 0;

#ifdef EM4K_DEBUG
	aprint_normal("em4k: found %x b window at %p, switch mask %x\n", 
	    sc->pci_mem_win_size, (void*) sc->pci_mem_win.base, 
	    sc->pci_mem_win_mask);
#endif /* EM4K_DEBUG */

}

pcireg_t
em4k_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t data;
	uint32_t bus, dev, func;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	data = bus_space_read_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    EMPB_CONF_DEV_STRIDE*dev + EMPB_CONF_FUNC_STRIDE*func + reg);
#ifdef EM4K_DEBUG_CONF
	aprint_normal("em4k conf read va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -r-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, data);
#endif /* EM4K_DEBUG_CONF */

	return data;
}

void
em4k_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	uint32_t bus, dev, func;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	bus_space_write_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    EMPB_CONF_DEV_STRIDE*dev + EMPB_CONF_FUNC_STRIDE*func + reg, val);
#ifdef EM4K_DEBUG_CONF
	aprint_normal("em4k conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, val);
#endif /* EM4K_DEBUG_CONF */
}

int
em4k_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno) 
{
	return 6; /* no Mediator with more than 6 slots? */
}

void
em4k_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
em4k_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/* TODO: add sanity checking */

	*ihp = EMPB_INT; 
	return 0;
}

const struct evcnt *
em4k_pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	/* TODO: implement */
	return NULL;
}

int
em4k_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, int func,
    pcireg_t id)
{
	return PCI_CONF_DEFAULT;
}
