/*	$NetBSD: msiiep.c,v 1.5 2002/03/28 11:59:56 pk Exp $ */

/*
 * Copyright (c) 2001 Valeriy E. Ushakov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/promlib.h>
#include <machine/idprom.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>

/**
 * ms-IIep PCIC registers are mapped at fixed VA
 */
#define msiiep ((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)


/**
 * Only one PCI controller per MS-IIep and only one MS-IIep per system
 * so don't bother with malloc'ing our tags.
 */

/*
 * PCI chipset tag
 */
static struct sparc_pci_chipset msiiep_pc_tag = { NULL };


/*
 * Bus space tags for memory and i/o.
 */

static void *msiiep_intr_establish(bus_space_tag_t, int, int, int,
				   int (*)(void *), void *);

static struct sparc_bus_space_tag msiiep_io_tag = {
	NULL,			/* cookie */
	NULL,			/* parent bus tag */
	NULL,			/* bus_space_map */ 
	NULL,			/* bus_space_unmap */
	NULL,			/* bus_space_subregion */
	NULL,			/* bus_space_barrier */ 
	NULL,			/* bus_space_mmap */ 
	msiiep_intr_establish	/* bus_intr_establish */
};

static struct sparc_bus_space_tag msiiep_mem_tag = {
	NULL,			/* cookie */
	NULL,			/* parent bus tag */
	NULL,			/* bus_space_map */ 
	NULL,			/* bus_space_unmap */
	NULL,			/* bus_space_subregion */
	NULL,			/* bus_space_barrier */ 
	NULL,			/* bus_space_mmap */ 
	msiiep_intr_establish	/* bus_intr_establish */
};


/*
 * DMA tag
 */
static int	msiiep_dmamap_load(bus_dma_tag_t, bus_dmamap_t,
				   void *, bus_size_t, struct proc *, int);
static void	msiiep_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
static int	msiiep_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *,
				  int, size_t, caddr_t *, int);

static struct sparc_bus_dma_tag msiiep_dma_tag = {
	NULL,			/* _cookie */

	_bus_dmamap_create,
	_bus_dmamap_destroy,
	msiiep_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	msiiep_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	msiiep_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};


/*
 * Autoconfiguration
 */
static int	msiiep_match(struct device *, struct cfdata *, void *);
static void	msiiep_attach(struct device *, struct device *, void *);
static int	msiiep_print(void *, const char *);

struct cfattach msiiep_ca = {
	sizeof(struct msiiep_softc), msiiep_match, msiiep_attach
};


/* Auxiliary functions */
static void	msiiep_getidprom(void);



static int
msiiep_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct mainbus_attach_args *ma = aux;
	pcireg_t id;

	/* match by PROM name */
	if (strcmp(ma->ma_name, "pci") != 0)
		return (0);

	/*
	 * Verify that PCIC was successfully mapped by bootstrap code.
	 * Since PCIC contains all the registers vital to the kernel,
	 * bootstrap code maps them at a fixed va, MSIIEP_PCIC_VA, and
	 * switches the endian-swapping mode on.
	 */
	id = msiiep->pcic_id;
	if (PCI_VENDOR(id) != PCI_VENDOR_SUN
	    && PCI_PRODUCT(id) != PCI_PRODUCT_SUN_MS_IIep)
		panic("msiiep_match: id %08x", id);

	return (1);
}

static void
msiiep_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	extern void timerattach_msiiep(void); /* clock.c */

	struct msiiep_softc *sc = (struct msiiep_softc *)self;
	struct mainbus_attach_args *ma = aux;
	int node = ma->ma_node;
	char devinfo[256];

	struct pcibus_attach_args pba;

	sc->sc_node = node;
	sc->sc_clockfreq = PROM_getpropint(node, "clock-frequency", 33333333);

	/* copy parent tags */
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	/*
	 * PCIC registers are mapped at a fixed VA because counters,
	 * interrupt registers etc are there.  Just save that mapping.
	 */
	sc->sc_bh = (bus_space_handle_t)MSIIEP_PCIC_VA;

	/*
	 * Ok, we know that we are on ms-IIep and the easy way to get
	 * idprom is to read it from root property (try this at prom:
	 * "see idprom@ seeprom see ee-read" if you don't believe me ;-).
	 */
	msiiep_getidprom();

	/* print our PCI device info and bus clock frequency */
	pci_devinfo(msiiep->pcic_id, msiiep->pcic_class, 0, devinfo);
	printf("%s: %s: clock = %s MHz\n",
	       self->dv_xname, devinfo, clockfreq(sc->sc_clockfreq));

	/* timers are PCIC registers */
	printf("%s: configuring timer:", self->dv_xname);
	config_found(self, "timer", NULL);

	/* chipset tag */
	msiiep_pc_tag.cookie = sc;

	/* I/O tag */
	msiiep_io_tag.cookie = sc;
	msiiep_io_tag.parent = sc->sc_bustag;

	/* memory tag */
	msiiep_mem_tag.cookie = sc;
	msiiep_mem_tag.parent = sc->sc_bustag;

	/* DMA tag */
	msiiep_dma_tag._cookie = sc;

	/* link them up to softc */
	sc->sc_pct = &msiiep_pc_tag; 
	sc->sc_iot = &msiiep_io_tag;
	sc->sc_memt = &msiiep_mem_tag;
	sc->sc_dmat = &msiiep_dma_tag;

	/*
	 * Attach the PCI bus.
	 */
	pba.pba_busname = "pci";
	pba.pba_bus = 0;
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_memt;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_pc = sc->sc_pct;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found(self, &pba, msiiep_print);
}


static int
msiiep_print(args, busname)
	void *args;
	const char *busname;
{

	if (busname == NULL)
		return (UNCONF);
	return (QUIET);
}


/*
 * Turn PCIC endian swapping on/off.  The kernel runs with endian
 * swapping turned on early in bootstrap(), but we need to turn it off
 * before we pass control to PROM's repl (e.g. in OF_enter and OF_exit).
 * PROM expects PCIC to be in little endian mode and would wedge if we
 * didn't turn endian swapping off.
 */
void
msiiep_swap_endian(on)
	int on;
{
	u_int8_t pioctl;

	pioctl = msiiep->pcic_pio_ctrl;
	if (on)
		pioctl |= MSIIEP_PIO_CTRL_BIG_ENDIAN;
	else
		pioctl &= ~MSIIEP_PIO_CTRL_BIG_ENDIAN;
	msiiep->pcic_pio_ctrl = pioctl;

	/* read it back to make sure transaction completed */
	pioctl = msiiep->pcic_pio_ctrl;
}


/*
 * Get the PIL currently assigned for this interrupt input line.
 */
int
msiiep_assigned_interrupt(line)
	int line;
{
	unsigned int intrmap;

	if (line < 0 || line > 7)
		return (-1);

	if (line < 4) {
		intrmap = msiiep->pcic_intr_asgn_sel;
	} else {
		intrmap = msiiep->pcic_intr_asgn_sel_hi;
		line -= 4;
	}
	return ((intrmap >> (line * 4)) & 0xf);
}

/*
 * idprom is in /pci/ebus/gpio but it's a pain to access.
 * fortunately the PROM  sets "idprom" property on the root node.
 * XXX: the idprom stuff badly needs to be factored out....
 */
struct idprom msiiep_idprom_store;

static void
msiiep_getidprom()
{
	extern void establish_hostid(struct idprom *); /* clock.c */
	struct idprom *idp;
	int nitems;

	idp = &msiiep_idprom_store;
	nitems = 1;
	if (PROM_getprop(prom_findroot(), "idprom",
			 sizeof(struct idprom), &nitems,
			 (void **)&idp) != 0)
		panic("unable to get \"idprom\" property from root node");
	establish_hostid(idp);
}


/* ======================================================================
 *
 *			  BUS space methods
 */

/*
 * Install an interrupt handler.
 *
 * Bus-specific interrupt argument is 'line', an interrupt input line
 * for ms-IIep.  The PIL for is programmable via pcic interrupt
 * assignment select registers (but we use existing assignments).
 */
static void *
msiiep_intr_establish(t, line, ipl, flags, handler, arg)
	bus_space_tag_t t;
	int line;
	int ipl;
	int flags;
	int (*handler)(void *);
	void *arg;
{
	struct intrhand *ih;
	int pil;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	/* use pil set-up by prom */
	pil = msiiep_assigned_interrupt(line);
	if (pil == -1)
		panic("msiiep_intr_establish: line %d", line);

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(pil, ih);

	return(ih);
}


/* ======================================================================
 *
 *			     DMA methods
 */

static int
msiiep_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	pmap_t pmap;
	paddr_t pa;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	if (!pmap_extract(pmap, (vaddr_t)buf, &pa))
		panic("msiiep_dmamap_load: dma memory not mapped");

	/* we always use just one segment */
	map->dm_nsegs = 1;
	map->dm_segs[0].ds_addr = pa;
	map->dm_segs[0].ds_len = buflen;
	map->dm_mapsize = buflen;

	return (0);
}

static void
msiiep_dmamap_unload(t, dmam)
	bus_dma_tag_t t;
	bus_dmamap_t dmam;
{

	panic("msiiep_dmamap_unload: not implemented");
}


static int
msiiep_dmamem_map(tag, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t tag;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct pglist *mlist;
	struct vm_page *m;
	vaddr_t va;
	int pagesz = PAGE_SIZE;

	if (nsegs != 1)
		panic("msiiep_dmamem_map: nsegs = %d", nsegs);

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (caddr_t)va;

	/*
	 * Map the pages allocated in _bus_dmamem_alloc()
	 * to the kernel virtual address space.
	 */
	mlist = segs[0]._ds_mlist;
	TAILQ_FOREACH(m, mlist, pageq) {
		paddr_t pa;

		if (size == 0)
			panic("msiiep_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va, pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE);
		va += pagesz;
		size -= pagesz;
	}
	pmap_update(pmap_kernel());

	return (0);
}
