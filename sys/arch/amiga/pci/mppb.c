/*	$NetBSD: mppb.c,v 1.1 2011/09/17 16:55:34 rkujawa Exp $ */

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

static int	mppb_match(struct device *, struct cfdata *, void *);
static void	mppb_attach(struct device *, struct device *, void *);
pcireg_t	mppb_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		mppb_pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		mppb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno); 
int		mppb_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, 
		    int func, pcireg_t id);
void		mppb_pci_attach_hook (struct device *parent, 
		    struct device *self, struct pcibus_attach_args *pba);
pcitag_t	mppb_pci_make_tag(pci_chipset_tag_t pc, int bus, int device, 
		    int function);
void		mppb_pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, 
		    int *bp, int *dp, int *fp);

void *		mppb_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t
		    ih, int level, int (*ih_fun)(void *), void *ih_arg);
void		mppb_pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie);
int		mppb_pci_intr_map(const struct pci_attach_args *pa, 
		    pci_intr_handle_t *ihp);
const char *	mppb_pci_intr_string(pci_chipset_tag_t pc,
		    pci_intr_handle_t ih);
const struct evcnt * mppb_pci_intr_evcnt(pci_chipset_tag_t pc, 
		    pci_intr_handle_t ih);

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
		
#ifdef MPPB_DEBUG
	aprint_normal("mppb matched by Zorro ID %d, %d\n", zap->manid,
	    zap->prodid); 
#endif

	return 1;
}


static void
mppb_attach(device_t parent, device_t self, void *aux)
{
	struct mppb_softc *sc;
	struct pcibus_attach_args pba;  
	struct zbus_args *zap;
	pci_chipset_tag_t pc;

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
	aprint_normal("mppb mapped conf %x->%x, mem %x->%x\n, io %x->%x",
	    (zap->pa) + MPPB_CONF_BASE, sc->pci_conf_area.base,
	    (zap->pa) + MPPB_MEM_BASE, sc->pci_mem_area.base,
	    (zap->pa) + MPPB_IO_BASE, sc->pci_io_area.base); 
#endif 

	sc->apc.pci_conf_iot = &(sc->pci_conf_area);

	if (bus_space_map(sc->apc.pci_conf_iot, 0, MPPB_CONF_SIZE, 0, 
	    &sc->apc.pci_conf_ioh)) 
		aprint_error_dev(self,
		    "couldn't map PCI configuration data space\n");
	
	/* Initialize the PCI chipset tag. */
	sc->apc.pc_conf_v = (void*) pc;
	sc->apc.pc_bus_maxdevs = mppb_pci_bus_maxdevs;
	sc->apc.pc_make_tag = mppb_pci_make_tag;
	sc->apc.pc_decompose_tag = mppb_pci_decompose_tag;
	sc->apc.pc_conf_read = mppb_pci_conf_read;
	sc->apc.pc_conf_write = mppb_pci_conf_write;
	sc->apc.pc_attach_hook = mppb_pci_attach_hook;

	sc->apc.pc_intr_map = mppb_pci_intr_map;
	sc->apc.pc_intr_string = mppb_pci_intr_string;
	sc->apc.pc_intr_establish = mppb_pci_intr_establish;
	sc->apc.pc_intr_disestablish = mppb_pci_intr_disestablish;
	/* XXX: pc_conf_interrupt */
        
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
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	data = bus_space_read_4(pc->pci_conf_iot, pc->pci_conf_ioh,
	    (MPPB_CONF_STRIDE*dev) + reg);
#ifdef MPPB_DEBUG
	aprint_normal("mppb conf read va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -r-> data %x\n",
	    pc->pci_conf_ioh, bus, dev, func, reg, data);
#endif
	return data;
}

void
mppb_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	uint32_t bus, dev, func;
	
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	
	bus_space_write_4(pc->pci_conf_iot, pc->pci_conf_ioh,
	    (MPPB_CONF_STRIDE*dev) + reg, val);
#ifdef MPPB_DEBUG
	aprint_normal("mppb conf write va: %lx, bus: %d, dev: %d, "
	    "func: %d, reg: %d -w-> data %x\n",
	    pc->pci_conf_ioh, bus, dev, func, reg, val);
#endif
	
}

int
mppb_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno) 
{
	return 4; /* Prometheus has 4 slots */
}

pcitag_t
mppb_pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	return (bus << 16) | (device << 11) | (function << 8);
}

void
mppb_pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp,
    int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

void
mppb_pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

void *
mppb_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level, 
   int (*ih_fun)(void *), void *ih_arg)
{
	struct isr* pci_isr;
	pci_isr = kmem_zalloc(sizeof(struct isr), KM_SLEEP);

	/* TODO: check for bogus handle */

	pci_isr->isr_intr = ih_fun;
	pci_isr->isr_arg = ih_arg;
	pci_isr->isr_ipl = MPPB_INT;
	add_isr(pci_isr);
	return pci_isr;	
}

void
mppb_pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	remove_isr(cookie);
	kmem_free(cookie, sizeof(struct isr));
}

int
mppb_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/* TODO: add sanity checking */

	*ihp = MPPB_INT; 
	return 0;
}

const char *
mppb_pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih) 
{
	static char str[10];

	sprintf(str, "INT%d", (int) ih);
	return str;
}

const struct evcnt *
mppb_pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	/* TODO: implement */
	return NULL;
}

