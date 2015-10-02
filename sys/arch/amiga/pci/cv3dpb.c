/*	$NetBSD: cv3dpb.c,v 1.3 2015/10/02 05:22:49 msaitoh Exp $ */

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
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/pci/cv3dpbreg.h>
#include <amiga/pci/cv3dpbvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

/* Zorro IDs */
#define ZORRO_MANID_P5		8512
#define ZORRO_PRODID_CV643D_Z3	67		/* CV64/3D on Z3 bus */

static int	cv3dpb_match(device_t, cfdata_t, void *);
static void	cv3dpb_attach(device_t, device_t, void *);
pcireg_t	cv3dpb_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		cv3dpb_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		cv3dpb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno); 
int		cv3dpb_pci_intr_map(const struct pci_attach_args *pa, 
		    pci_intr_handle_t *ihp);
static bool	cv3dpb_bus_map(struct cv3dpb_softc *sc);

CFATTACH_DECL_NEW(cv3dpb, sizeof(struct cv3dpb_softc),
    cv3dpb_match, cv3dpb_attach, NULL, NULL);

static int
cv3dpb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != ZORRO_MANID_P5)
		return 0;

	if (zap->prodid != ZORRO_PRODID_CV643D_Z3)
		return 0;
		
#ifdef P5PB_DEBUG
	aprint_normal("cv3dpb matched by Zorro ID %d, %d\n", zap->manid,
	    zap->prodid); 
#endif

	return 10;
}


static void
cv3dpb_attach(device_t parent, device_t self, void *aux)
{
	struct cv3dpb_softc *sc; 
	struct pcibus_attach_args pba;  
	struct zbus_args *zap;

	sc = device_private(self);
	pci_chipset_tag_t pc = &sc->apc;
	sc->sc_dev = self;
	zap = aux;
	
	sc->ba = zap->va;

	if(!(cv3dpb_bus_map(sc))) {
		aprint_error_dev(self,
		    "couldn't map PCI configuration registers\n");
		return;
	}

	aprint_normal(": CyberVision 64/3D PCI bridge\n"); 

#ifdef P5PB_DEBUG
	aprint_normal("cv3dpb: mapped %x -> %x, %x -> %x\n, %x -> %x\n",
	    P5BUS_PCI_CONF_BASE, sc->pci_conf_area.base,
	    P5BUS_PCI_IO_BASE, sc->pci_io_area.base,
	    P5BUS_PCI_MEM_BASE, sc->pci_mem_area.base ); 
#endif 

	/* Initialize the PCI chipset tag. */
	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_bus_maxdevs = cv3dpb_pci_bus_maxdevs;
	sc->apc.pc_make_tag = amiga_pci_make_tag;
	sc->apc.pc_decompose_tag = amiga_pci_decompose_tag;
	sc->apc.pc_conf_read = cv3dpb_pci_conf_read;
	sc->apc.pc_conf_write = cv3dpb_pci_conf_write;
	//sc->apc.pc_attach_hook = cv3dpb_pci_attach_hook;
      
	sc->apc.pc_intr_map = cv3dpb_pci_intr_map; 
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

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

pcireg_t
cv3dpb_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t data;
	uint32_t bus, dev, func;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	data = bus_space_read_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    (func<<5) + reg);
#ifdef P5PB_DEBUG
	aprint_normal("cv3dpb conf read va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -r-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, data);
#endif
	return data;
}

void
cv3dpb_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	uint32_t bus, dev, func;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	bus_space_write_4(pc->pci_conf_datat, pc->pci_conf_datah,
	    (func << 5) + reg, val);
#ifdef P5PB_DEBUG
	aprint_normal("cv3dpb conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_datah, bus, dev, func, reg, val);
#endif
	
}

/* There can be only one. */
int
cv3dpb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno) 
{
	return 1;
}

int
cv3dpb_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/* TODO: add sanity checking */

	*ihp = 2;  /* XXX: untested */
	return 0;
}

bool
cv3dpb_bus_map(struct cv3dpb_softc *sc) {
#ifdef P5PB_DEBUG
	aprint_normal("cv3dpb: cv3dpb_bus_map called, ba = %x\n", 
	    (bus_addr_t) sc->ba);
#endif /* P5PB_DEBUG */

	sc->pci_conf_area.base = (bus_addr_t) sc->ba + CV643D_PCI_CONF_BASE;
	sc->pci_conf_area.absm = &amiga_bus_stride_1;

	sc->pci_mem_area.base = (bus_addr_t) sc->ba + CV643D_PCI_MEM_BASE; 
	sc->pci_mem_area.absm = &amiga_bus_stride_1;

	sc->pci_io_area.base = (bus_addr_t) sc->ba + CV643D_PCI_IO_BASE;
	sc->pci_io_area.absm = &amiga_bus_stride_1;

	sc->apc.pci_conf_datat = &(sc->pci_conf_area);

	if (bus_space_map(sc->apc.pci_conf_datat, 0, 
	    CV643D_PCI_CONF_SIZE, 0, &sc->apc.pci_conf_datah)) 
		return false;
		
	return true;
}

