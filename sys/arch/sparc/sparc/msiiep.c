/*	$NetBSD: msiiep.c,v 1.20 2004/03/17 17:04:59 pk Exp $ */

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msiiep.c,v 1.20 2004/03/17 17:04:59 pk Exp $");

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

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>


/*
 * Autoconfiguration.
 *
 * Normally, sparc autoconfiguration is driven by PROM device tree,
 * however PROMs in ms-IIep machines usually don't have nodes for
 * various important registers that are part of ms-IIep PCI controller.
 * We work around by inserting a dummy device that acts as a parent
 * for device drivers that deal with various functions of PCIC.  The
 * other option is to hack mainbus_attach() to treat ms-IIep specially,
 * but I'd rather insulate the rest of the source from ms-IIep quirks.
 */

/* parent "stub" device that knows how to attach various functions */
static int	msiiep_match(struct device *, struct cfdata *, void *);
static void	msiiep_attach(struct device *, struct device *, void *);
/* static int	msiiep_print(void *, const char *); */

CFATTACH_DECL(msiiep, sizeof(struct device),
    msiiep_match, msiiep_attach, NULL, NULL);

/*
 * The real thing.
 */
static int	mspcic_match(struct device *, struct cfdata *, void *);
static void	mspcic_attach(struct device *, struct device *, void *);
static int	mspcic_print(void *, const char *);

CFATTACH_DECL(mspcic, sizeof(struct mspcic_softc),
    mspcic_match, mspcic_attach, NULL, NULL);

/**
 * ms-IIep PCIC registers are mapped at fixed VA
 */
#define mspcic ((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)


/**
 * Only one PCI controller per MS-IIep and only one MS-IIep per system
 * so don't bother with malloc'ing our tags.
 */

/*
 * PCI chipset tag
 */
static struct sparc_pci_chipset mspcic_pc_tag = { NULL };


/*
 * Bus space tags for memory and i/o.
 */

struct mspcic_pci_map {
	u_int32_t sysbase;
	u_int32_t pcibase;
	u_int32_t size;
};

/* fixed i/o and one set of i/o cycle translation registers */
static struct mspcic_pci_map mspcic_pci_iomap[2] = {
	{ 0x30000000, 0x0, 0x00010000 }		/* fixed i/o (decoded bits) */
};

/* fixed mem and two sets of mem cycle translation registers */
static struct mspcic_pci_map mspcic_pci_memmap[3] = {
	{ 0x30100000, 0x00100000, 0x00f00000 }	/* fixed mem (pass through) */
};

struct mspcic_cookie {
	struct mspcic_pci_map *map;
	int nmaps;
};

static struct mspcic_cookie mspcic_io_cookie = { mspcic_pci_iomap, 0 };
static struct mspcic_cookie mspcic_mem_cookie = { mspcic_pci_memmap, 0 };


static void		mspcic_init_maps(void);
static void		mspcic_pci_map_from_reg(struct mspcic_pci_map *,
						u_int8_t, u_int8_t, u_int8_t);
static bus_addr_t	mspcic_pci_map_find(struct mspcic_pci_map *, int,
					    bus_addr_t, bus_size_t);
#ifdef DEBUG
static void	mspcic_pci_map_print(struct mspcic_pci_map *, const char *);
#endif


static int	mspcic_bus_map(bus_space_tag_t, bus_addr_t, bus_size_t,
			       int, vaddr_t, bus_space_handle_t *);
static paddr_t	mspcic_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static void	*mspcic_intr_establish(bus_space_tag_t, int, int,
				       int (*)(void *), void *, void (*)(void));

static struct sparc_bus_space_tag mspcic_io_tag = {
	&mspcic_io_cookie,	/* cookie */
	NULL,			/* parent bus tag */
	NULL,			/* ranges */
	0,			/* nranges */
	mspcic_bus_map,		/* bus_space_map */
	NULL,			/* bus_space_unmap */
	NULL,			/* bus_space_subregion */
	NULL,			/* bus_space_barrier */
	mspcic_bus_mmap,	/* bus_space_mmap */
	mspcic_intr_establish,	/* bus_intr_establish */
#if __FULL_SPARC_BUS_SPACE
	NULL,			/* read_1 */
	NULL,			/* read_2 */
	NULL,			/* read_4 */
	NULL,			/* read_8 */
	NULL,			/* write_1 */
	NULL,			/* write_2 */
	NULL,			/* write_4 */
	NULL			/* write_8 */
#endif
};

static struct sparc_bus_space_tag mspcic_mem_tag = {
	&mspcic_mem_cookie,	/* cookie */
	NULL,			/* parent bus tag */
	NULL,			/* ranges */
	0,			/* nranges */
	mspcic_bus_map,		/* bus_space_map */ 
	NULL,			/* bus_space_unmap */
	NULL,			/* bus_space_subregion */
	NULL,			/* bus_space_barrier */
	mspcic_bus_mmap,	/* bus_space_mmap */
	mspcic_intr_establish	/* bus_intr_establish */
#if __FULL_SPARC_BUS_SPACE
	NULL,			/* read_1 */
	NULL,			/* read_2 */
	NULL,			/* read_4 */
	NULL,			/* read_8 */
	NULL,			/* write_1 */
	NULL,			/* write_2 */
	NULL,			/* write_4 */
	NULL			/* write_8 */
#endif
};


/*
 * DMA tag
 */
static int	mspcic_dmamap_load(bus_dma_tag_t, bus_dmamap_t,
				   void *, bus_size_t, struct proc *, int);
static void	mspcic_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
static int	mspcic_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *,
				  int, size_t, caddr_t *, int);

static struct sparc_bus_dma_tag mspcic_dma_tag = {
	NULL,			/* _cookie */

	_bus_dmamap_create,
	_bus_dmamap_destroy,
	mspcic_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	mspcic_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	mspcic_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};





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
	id = mspcic->pcic_id;
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
	struct mainbus_attach_args *ma = aux;
	struct msiiep_attach_args msa;

	aprint_normal("\n");

	/* pass on real mainbus_attach_args */
	msa.msa_ma = ma;

	/* config timer/counter part of PCIC */
	msa.msa_name = "timer";
	config_found(self, &msa, NULL);

	/* config PCI tree */
	msa.msa_name = "pcic";
	config_found(self, &msa, NULL);
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

	pioctl = mspcic->pcic_pio_ctrl;
	if (on)
		pioctl |= MSIIEP_PIO_CTRL_BIG_ENDIAN;
	else
		pioctl &= ~MSIIEP_PIO_CTRL_BIG_ENDIAN;
	mspcic->pcic_pio_ctrl = pioctl;

	/* read it back to make sure transaction completed */
	pioctl = mspcic->pcic_pio_ctrl;
}



/* ======================================================================
 *
 *		      Real ms-IIep PCIC driver.
 */

static int
mspcic_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct msiiep_attach_args *msa = aux;

	return (strcmp(msa->msa_name, "pcic") == 0);
}


static void
mspcic_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mspcic_softc *sc = (struct mspcic_softc *)self;
	struct msiiep_attach_args *msa = aux;
	struct mainbus_attach_args *ma = msa->msa_ma;
	int node = ma->ma_node;
	char devinfo[256];

	struct pcibus_attach_args pba;

	sc->sc_node = node;
	sc->sc_clockfreq = prom_getpropint(node, "clock-frequency", 33333333);

	/* copy parent tags */
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	/*
	 * PCIC registers are mapped at a fixed VA because counters,
	 * interrupt registers etc are there.  Just save that mapping.
	 */
	sc->sc_bh = (bus_space_handle_t)MSIIEP_PCIC_VA;

	/* print our PCI device info and bus clock frequency */
	pci_devinfo(mspcic->pcic_id, mspcic->pcic_class, 0, devinfo);
	printf(": %s: clock = %s MHz\n", devinfo, clockfreq(sc->sc_clockfreq));

	mspcic_init_maps();

	/* init cookies/parents in our statically allocated tags */
	mspcic_io_tag.parent = sc->sc_bustag;
	mspcic_mem_tag.parent = sc->sc_bustag;
	mspcic_dma_tag._cookie = sc;
	mspcic_pc_tag.cookie = sc;

	/* save bus tags in softc */
	sc->sc_iot = &mspcic_io_tag;
	sc->sc_memt = &mspcic_mem_tag;
	sc->sc_dmat = &mspcic_dma_tag;

	/*
	 * Attach the PCI bus.
	 */
	pba.pba_busname = "pci";
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_memt;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &mspcic_pc_tag;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found(self, &pba, mspcic_print);
}


static int
mspcic_print(args, busname)
	void *args;
	const char *busname;
{

	if (busname == NULL)
		return (UNCONF);
	return (QUIET);
}


/*
 * Get the PIL currently assigned for this interrupt input line.
 */
int
mspcic_assigned_interrupt(line)
	int line;
{
	unsigned int intrmap;

	if (line < 0 || line > 7)
		return (-1);

	if (line < 4) {
		intrmap = mspcic->pcic_intr_asgn_sel;
	} else {
		intrmap = mspcic->pcic_intr_asgn_sel_hi;
		line -= 4;
	}
	return ((intrmap >> (line * 4)) & 0xf);
}

/* ======================================================================
 *
 *			  BUS space methods
 */

static __inline__ void
mspcic_pci_map_from_reg(m, sbar, pbar, sizemask)
	struct mspcic_pci_map *m;
	u_int8_t sbar, pbar, sizemask;
{
	m->sysbase = 0x30000000 | ((sbar & 0x0f) << 24);
	m->pcibase = pbar << 24;
	m->size = ~((0xf0 | sizemask) << 24) + 1;
}


/* does [al, ar) and [bl, br) overlap? */
#define OVERLAP(al, ar, bl, br) (((al) < (br)) && ((bl) < (ar)))

/* does map "m" overlap with fixed mapping region? */
#define OVERLAP_FIXED(m) OVERLAP((m)->sysbase, (m)->sysbase + (m)->size, \
				0x30000000, 0x31000000)

/* does map "ma" overlap map "mb" (possibly NULL)? */
#define OVERLAP_MAP(ma, mb) \
	((mb != NULL) && OVERLAP((ma)->sysbase, (ma)->sysbase + (ma)->size, \
				 (mb)->sysbase, (mb)->sysbase + (mb)->size))

/*
 * Init auxiliary paddr->pci maps.
 */
static void
mspcic_init_maps()
{
	struct mspcic_pci_map *m0, *m1, *io;
	int nmem, nio;

#ifdef DEBUG
	printf("mspcic0: SMBAR0 %02x  PMBAR0 %02x  MSIZE0 %02x\n",
	       mspcic->pcic_smbar0, mspcic->pcic_pmbar0, mspcic->pcic_msize0);
	printf("mspcic0: SMBAR1 %02x  PMBAR1 %02x  MSIZE1 %02x\n",
	       mspcic->pcic_smbar1, mspcic->pcic_pmbar1, mspcic->pcic_msize1);
	printf("mspcic0: SIBAR  %02x  PIBAR  %02x  IOSIZE %02x\n",
	       mspcic->pcic_sibar, mspcic->pcic_pibar, mspcic->pcic_iosize);
#endif
	nmem = nio = 1;

	m0 = &mspcic_pci_memmap[nmem];
	mspcic_pci_map_from_reg(m0, mspcic->pcic_smbar0, mspcic->pcic_pmbar0,
				mspcic->pcic_msize0);
	if (OVERLAP_FIXED(m0))
		m0 = NULL;
	else
		++nmem;

	m1 = &mspcic_pci_memmap[nmem];
	mspcic_pci_map_from_reg(m1, mspcic->pcic_smbar1, mspcic->pcic_pmbar1,
				mspcic->pcic_msize1);
	if (OVERLAP_FIXED(m1) || OVERLAP_MAP(m1, m0))
		m1 = NULL;
	else
		++nmem;

	io = &mspcic_pci_iomap[nio];
	mspcic_pci_map_from_reg(io, mspcic->pcic_sibar, mspcic->pcic_pibar,
				mspcic->pcic_iosize);
	if (OVERLAP_FIXED(io) || OVERLAP_MAP(io, m0) || OVERLAP_MAP(io, m1))
		io = NULL;
	else
		++nio;

	mspcic_io_cookie.nmaps = nio;
	mspcic_mem_cookie.nmaps = nmem;

#ifdef DEBUG
	mspcic_pci_map_print(&mspcic_pci_iomap[0], "i/o fixed");
	mspcic_pci_map_print(&mspcic_pci_memmap[0], "mem fixed");
	if (m0) mspcic_pci_map_print(m0, "mem map0");
	if (m1) mspcic_pci_map_print(m1, "mem map1");
	if (io) mspcic_pci_map_print(io, "i/0 map");
#endif
}


#ifdef DEBUG
static void
mspcic_pci_map_print(m, msg)
	struct mspcic_pci_map *m;
	const char *msg;
{
	printf("mspcic0: paddr [%08x..%08x] -> pci [%08x..%08x] %s\n",
	       m->sysbase, m->sysbase + m->size - 1,
	       m->pcibase, m->pcibase + m->size - 1,
	       msg);
}
#endif


static bus_addr_t
mspcic_pci_map_find(m, nmaps, pciaddr, size)
	struct mspcic_pci_map *m;
	int nmaps;
	bus_addr_t pciaddr;
	bus_size_t size;
{
	bus_size_t offset;
	int i;

	for (i = 0; i < nmaps; ++i, ++m) {
		offset = pciaddr - m->pcibase;
		if (offset >= 0 && offset + size <= m->size)
			return (m->sysbase + offset);
	}
	return (0);
}


static int
mspcic_bus_map(t, ba, size, flags, va, hp)
	bus_space_tag_t t;
	bus_addr_t ba;
	bus_size_t size;
	int flags;
	vaddr_t va;
	bus_space_handle_t *hp;
{
	struct mspcic_cookie *c = t->cookie;
	bus_addr_t paddr;

	paddr = mspcic_pci_map_find(c->map, c->nmaps, ba, size);
	if (paddr == 0)
		return (EINVAL);
	return (bus_space_map2(t->parent, paddr, size, flags, va, hp));
}


static paddr_t
mspcic_bus_mmap(t, ba, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t ba;
	off_t off;
	int prot;
	int flags;
{
	struct mspcic_cookie *c = t->cookie;
	bus_addr_t paddr;

	/* verify that phys to pci mapping for the target page exists */
	paddr = mspcic_pci_map_find(c->map, c->nmaps, ba + off, PAGE_SIZE);
	if (paddr == 0)
		return (-1);

	return (bus_space_mmap(t->parent, paddr - off, off, prot, flags));
}


/*
 * Install an interrupt handler.
 *
 * Bus-specific interrupt argument is 'line', an interrupt input line
 * for ms-IIep.  The PIL for each line is programmable via pcic interrupt
 * assignment select registers (but we use existing assignments).
 */
static void *
mspcic_intr_establish(t, line, ipl, handler, arg, fastvec)
	bus_space_tag_t t;
	int line;
	int ipl;
	int (*handler)(void *);
	void *arg;
	void (*fastvec)(void);
{
	struct intrhand *ih;
	int pil;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	/* use pil set-up by prom */
	pil = mspcic_assigned_interrupt(line);
	if (pil == -1)
		panic("mspcic_intr_establish: line %d", line);

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(pil, ipl, ih, fastvec);

	return(ih);
}


/* ======================================================================
 *
 *			     DMA methods
 */

static int
mspcic_dmamap_load(t, map, buf, buflen, p, flags)
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
		panic("mspcic_dmamap_load: dma memory not mapped");

	/* we always use just one segment */
	map->dm_nsegs = 1;
	map->dm_segs[0].ds_addr = pa;
	map->dm_segs[0].ds_len = buflen;
	map->dm_mapsize = buflen;

	return (0);
}

static void
mspcic_dmamap_unload(t, dmam)
	bus_dma_tag_t t;
	bus_dmamap_t dmam;
{

	panic("mspcic_dmamap_unload: not implemented");
}


static int
mspcic_dmamem_map(tag, segs, nsegs, size, kvap, flags)
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
		panic("mspcic_dmamem_map: nsegs = %d", nsegs);

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
			panic("mspcic_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va, pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE);
		va += pagesz;
		size -= pagesz;
	}
	pmap_update(pmap_kernel());

	return (0);
}
