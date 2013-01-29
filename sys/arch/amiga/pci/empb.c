/*	$NetBSD: empb.c,v 1.10 2013/01/29 00:49:43 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

/* Elbox Mediator PCI bridge driver. Currently supports Mediator 1200 models.*/

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
#include <amiga/pci/empbvar.h>
#include <amiga/pci/emmemvar.h>
#include <amiga/pci/empmvar.h>

#include <dev/pci/pciconf.h>

#include "opt_pci.h"

/*#define EMPB_DEBUG 1 */

#define	PCI_CONF_LOCK(s)	(s) = splhigh()
#define	PCI_CONF_UNLOCK(s)	splx((s))

#define WINDOW_LOCK(s)		(s) = splhigh()
#define WINDOW_UNLOCK(s)	splx((s)) 

static int	empb_match(device_t, cfdata_t, void *);
static void	empb_attach(device_t, device_t, void *);
static void	empb_callback(device_t);

static void	empb_empm_attach(struct empb_softc *sc);
static int	empb_empm_print(void *aux, const char *pnp);

static void	empb_find_mem(struct empb_softc *);
static void	empb_switch_bridge(struct empb_softc *, uint8_t);
static void	empb_intr_enable(struct empb_softc *);

pcireg_t	empb_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		empb_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		empb_pci_bus_maxdevs(pci_chipset_tag_t, int); 
void		empb_pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
pcitag_t	empb_pci_make_tag(pci_chipset_tag_t, int, int, int);
void		empb_pci_decompose_tag(pci_chipset_tag_t, pcitag_t, 
		    int *, int *, int *);
int		empb_pci_intr_map(const struct pci_attach_args *, 
		    pci_intr_handle_t *);
const struct evcnt * empb_pci_intr_evcnt(pci_chipset_tag_t, 
		    pci_intr_handle_t);
int		empb_pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);

CFATTACH_DECL_NEW(empb, sizeof(struct empb_softc),
    empb_match, empb_attach, NULL, NULL);

static int
empb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != ZORRO_MANID_ELBOX)
		return 0;

	switch (zap->prodid) {
	case ZORRO_PRODID_MED1K2:
	case ZORRO_PRODID_MED1K2SX:
	case ZORRO_PRODID_MED1K2LT2:
	case ZORRO_PRODID_MED1K2LT4:
	case ZORRO_PRODID_MED1K2TX:
	case ZORRO_PRODID_MEDZIV:	/* ZIV untested! */
		return 1;
	}

	return 0;
}


static void
empb_attach(device_t parent, device_t self, void *aux)
{
	struct empb_softc *sc;
	struct zbus_args *zap;

	volatile char *ba;

	zap = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	ba = zap->va;

	sc->model = zap->prodid;

	switch (sc->model) {
	case ZORRO_PRODID_MED1K2:
		aprint_normal(": ELBOX Mediator PCI 1200\n"); 
		break;
	case ZORRO_PRODID_MED1K2SX:
		aprint_normal(": ELBOX Mediator PCI 1200 SX\n"); 
		break;
	case ZORRO_PRODID_MED1K2LT2:
		aprint_normal(": ELBOX Mediator PCI 1200 LT2\n"); 
		break;
	case ZORRO_PRODID_MED1K2LT4:
		aprint_normal(": ELBOX Mediator PCI 1200 LT4\n"); 
		break;
	case ZORRO_PRODID_MED1K2TX:
		aprint_normal(": ELBOX Mediator PCI 1200 TX\n"); 
		break;
	default:
		aprint_normal(": ELBOX Mediator PCI (unknown)\n"); 
		break;
	}

	/* Setup bus space mappings. */
	sc->pci_confio_area.base = (bus_addr_t) ba + EMPB_BRIDGE_OFF;
	sc->pci_confio_area.absm = &amiga_bus_stride_1swap;

	sc->setup_area.base = (bus_addr_t) ba + EMPB_SETUP_OFF;
	sc->setup_area.absm = &amiga_bus_stride_1;

	/* 
	 * Defer everything until later, we need to wait for possible
	 * emmem attachments.
	 */

	config_defer(self, empb_callback);
}

static void
empb_callback(device_t self) {

	struct empb_softc *sc;
	pci_chipset_tag_t pc;
	struct pcibus_attach_args pba;  
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#endif /* PCI_NETBSD_CONFIGURE */

	sc = device_private(self);
	pc = &sc->apc;

#ifdef EMPB_DEBUG 
	aprint_normal("empb: mapped setup %x->%x, conf/io %x->%x\n",
	    kvtop((void*) sc->setup_area.base), sc->setup_area.base,
	    kvtop((void*) sc->pci_confio_area.base), sc->pci_confio_area.base);
#endif 

	sc->pci_confio_t = &(sc->pci_confio_area);

	/*
	 * We should not map I/O space here, however we have no choice 
	 * since these addresses are shared between configuration space and
	 * I/O space. Not really a problem on m68k, however on PPC... 
	 */
	if (bus_space_map(sc->pci_confio_t, 0, EMPB_BRIDGE_SIZE, 0, 
	    &sc->pci_confio_h)) 
		aprint_error_dev(self,
		    "couldn't map PCI configuration & I/O space\n");

	sc->apc.pci_conf_datat = sc->pci_confio_t;
	sc->apc.pci_conf_datah = sc->pci_confio_h;

	sc->setup_area_t = &(sc->setup_area);

	if (bus_space_map(sc->setup_area_t, 0, EMPB_SETUP_SIZE, 0, 
	    &sc->setup_area_h)) 
		aprint_error_dev(self,
		    "couldn't map Mediator setup space\n");

	empb_find_mem(sc);
	if (sc->pci_mem_win_size == 0)
		aprint_error_dev(self,
		    "couldn't find memory space, check your WINDOW jumper\n");

	/* Initialize the PCI chipset tag. */
	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_bus_maxdevs = empb_pci_bus_maxdevs;
	sc->apc.pc_make_tag = amiga_pci_make_tag;
	sc->apc.pc_decompose_tag = amiga_pci_decompose_tag;
	sc->apc.pc_conf_read = empb_pci_conf_read;
	sc->apc.pc_conf_write = empb_pci_conf_write;
	sc->apc.pc_attach_hook = empb_pci_attach_hook;

	sc->apc.pc_intr_map = empb_pci_intr_map;
	sc->apc.pc_intr_string = amiga_pci_intr_string;
	sc->apc.pc_intr_establish = amiga_pci_intr_establish;
	sc->apc.pc_intr_disestablish = amiga_pci_intr_disestablish;

	sc->apc.pc_conf_hook = empb_pci_conf_hook;
	sc->apc.pc_conf_interrupt = amiga_pci_conf_interrupt;

	sc->apc.cookie = sc;

#ifdef PCI_NETBSD_CONFIGURE
	ioext = extent_create("empbio", 0, EMPB_BRIDGE_SIZE, 
	    NULL, 0, EX_NOWAIT);

	memext = extent_create("empbmem", EMPB_MEM_BASE, EMPB_MEM_END,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(pc, ioext, memext, NULL, 0, CACHELINE_SIZE);

	extent_destroy(ioext);
	extent_destroy(memext);

#endif /* PCI_NETBSD_CONFIGURE */

	pba.pba_iot = &(sc->pci_confio_area);
	pba.pba_dmat = NULL; 
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_OKAY;

	if(sc->pci_mem_win_size > 0) {
		pba.pba_memt = &(sc->pci_mem_win);
		pba.pba_flags |= PCI_FLAGS_MEM_OKAY;
	} else 
		pba.pba_memt = NULL; 

	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	/* Attach power management on SX and TX models. */
	switch (sc->model) {
	case ZORRO_PRODID_MED1K2SX:
	case ZORRO_PRODID_MED1K2TX:
		empb_empm_attach(sc);
	default:
		break;
	}	

	empb_intr_enable(sc);

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static void
empb_empm_attach(struct empb_softc *sc)
{
	struct empm_attach_args aa;
	aa.setup_area_t = sc->setup_area_t;

	config_found_ia(sc->sc_dev, "empmdev", &aa, empb_empm_print);
}

static int 
empb_empm_print(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("empm at %s", pnp);

	return UNCONF;
}

static void 
empb_intr_enable(struct empb_softc *sc) 
{
	bus_space_write_1(sc->setup_area_t, sc->setup_area_h,
	    EMPB_SETUP_INTR_OFF, EMPB_INTR_ENABLE);
}

/*
 * Switch between configuration space and I/O space.
 */
static void
empb_switch_bridge(struct empb_softc *sc, uint8_t mode)
{
	bus_space_write_1(sc->setup_area_t, sc->setup_area_h,
	    EMPB_SETUP_BRIDGE_OFF, mode);
}


/*
 * Try to find a (optional) memory window board.
 */
static void
empb_find_mem(struct empb_softc *sc)
{
	device_t memdev;
	struct emmem_softc *mem_sc;

	memdev = device_find_by_xname("emmem0");
	sc->pci_mem_win_size = 0;

	if(memdev == NULL) {
		return;
	}

	mem_sc = device_private(memdev);

	sc->pci_mem_win.base = (bus_addr_t) mem_sc->sc_base;
	sc->pci_mem_win.absm = &empb_bus_swap;
	sc->pci_mem_win_size = mem_sc->sc_size;
	sc->pci_mem_win_t = &sc->pci_mem_win;

	if(sc->pci_mem_win_size == 8*1024*1024)
		sc->pci_mem_win_mask = EMPB_WINDOW_MASK_8M;
	else if(sc->pci_mem_win_size == 4*1024*1024)
		sc->pci_mem_win_mask = EMPB_WINDOW_MASK_4M;
	else /* disable anyway */
		sc->pci_mem_win_size = 0;

#ifdef EMPB_DEBUG
	aprint_normal("empb: found %x b window at %p, switch mask %x\n", 
	    sc->pci_mem_win_size, (void*) sc->pci_mem_win.base, 
	    sc->pci_mem_win_mask);
#endif /* EMPB_DEBUG */

}

/*
 * Switch memory window position. Return PCI mem address seen at the beginning
 * of window.
 */
bus_addr_t
empb_switch_window(struct empb_softc *sc, bus_addr_t address) 
{
	int s;
	uint16_t win_reg;
#ifdef EMPB_DEBUG
	uint16_t rwin_reg;
#endif /* EMPB_DEBUG */

	WINDOW_LOCK(s);

	win_reg = bswap16((address >> EMPB_WINDOW_SHIFT) 
	    & sc->pci_mem_win_mask);

	bus_space_write_2(sc->setup_area_t, sc->setup_area_h,
	    EMPB_SETUP_WINDOW_OFF, win_reg);

	/* store window pos, like: sc->pci_mem_win_pos = win_reg ? */

#ifdef EMPB_DEBUG
	rwin_reg = bus_space_read_2(sc->setup_area_t, sc->setup_area_h,
	    EMPB_SETUP_WINDOW_OFF);

	aprint_normal("empb: access to %p window switch to %x => reg now %x\n",
	    (void*) address, win_reg, rwin_reg);
#endif /* EMPB_DEBUG */

	WINDOW_UNLOCK(s);	

	return (bus_addr_t)((bswap16(win_reg)) << EMPB_WINDOW_SHIFT);
}


pcireg_t
empb_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t data;
	uint32_t bus, dev, func;
	struct empb_softc *sc;
	int s;

	sc = pc->cookie;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	PCI_CONF_LOCK(s);

	empb_switch_bridge(sc, BRIDGE_CONF);

	data = bus_space_read_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    EMPB_CONF_DEV_STRIDE*dev + EMPB_CONF_FUNC_STRIDE*func + reg);
#ifdef EMPB_DEBUG_CONF
	aprint_normal("empb conf read va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -r-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, data);
#endif /* EMPB_DEBUG_CONF */

	empb_switch_bridge(sc, BRIDGE_IO);

	PCI_CONF_UNLOCK(s);

	return data;
}

void
empb_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	uint32_t bus, dev, func;
	struct empb_softc *sc;
	int s;

	sc = pc->cookie;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	PCI_CONF_LOCK(s);	

	empb_switch_bridge(sc, BRIDGE_CONF);

	bus_space_write_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    EMPB_CONF_DEV_STRIDE*dev + EMPB_CONF_FUNC_STRIDE*func + reg, val);
#ifdef EMPB_DEBUG_CONF
	aprint_normal("empb conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, val);
#endif /* EMPB_DEBUG_CONF */
	empb_switch_bridge(sc, BRIDGE_IO);

	PCI_CONF_UNLOCK(s);	
}

int
empb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno) 
{
	return 6; /* no Mediator with more than 6 slots? */
}

void
empb_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
empb_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/* TODO: add sanity checking */

	*ihp = EMPB_INT; 
	return 0;
}

const struct evcnt *
empb_pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	/* TODO: implement */
	return NULL;
}

int
empb_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, int func,
    pcireg_t id)
{

	/* 
	 * Register information about some known PCI devices with
	 * DMA-able memory.
	 */
	/*if ((PCI_VENDOR(id) == PCI_VENDOR_3DFX) &&
		(PCI_PRODUCT(id) >= PCI_PRODUCT_3DFX_VOODOO3))*/


        return PCI_CONF_DEFAULT;
}
