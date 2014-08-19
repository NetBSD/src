/*	$NetBSD: msiiep.c,v 1.43.12.2 2014/08/20 00:03:24 tls Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: msiiep.c,v 1.43.12.2 2014/08/20 00:03:24 tls Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/promlib.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>
#include <sparc/sparc/pci_fixup.h>

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

/*
 * "Stub" ms-IIep parent that knows how to attach various functions.
 */
static int	msiiep_match(device_t, cfdata_t, void *);
static void	msiiep_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(msiiep, 0, msiiep_match, msiiep_attach, NULL, NULL);


/* sleep in idle spin */
static void	msiiep_cpu_sleep(struct cpu_info *);
volatile uint32_t *msiiep_mid = NULL;


/*
 * The real thing.
 */
static int	mspcic_match(device_t, cfdata_t, void *);
static void	mspcic_attach(device_t, device_t, void *);
static int	mspcic_print(void *, const char *);

CFATTACH_DECL_NEW(mspcic, sizeof(struct mspcic_softc),
    mspcic_match, mspcic_attach, NULL, NULL);


/**
 * Only one PCI controller per MS-IIep and only one MS-IIep per system
 * so don't bother with malloc'ing our tags.
 */

/*
 * PCI chipset tag
 */
static struct sparc_pci_chipset mspcic_pc_tag = { NULL };


/* fixed i/o and one set of i/o cycle translation registers */
struct mspcic_pci_map mspcic_pci_iomap[IOMAP_SIZE] = {
	{ 0x30000000, 0x0, 0x00010000 }		/* fixed i/o (decoded bits) */
};

/* fixed mem and two sets of mem cycle translation registers */
struct mspcic_pci_map mspcic_pci_memmap[MEMMAP_SIZE] = {
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
						uint8_t, uint8_t, uint8_t);
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

/* bus space methods that do byteswapping */
static uint16_t	mspcic_bus_read_2(bus_space_tag_t, bus_space_handle_t,
				  bus_size_t);
static uint32_t	mspcic_bus_read_4(bus_space_tag_t, bus_space_handle_t,
				  bus_size_t);
static uint64_t	mspcic_bus_read_8(bus_space_tag_t, bus_space_handle_t,
				  bus_size_t);
static void	mspcic_bus_write_2(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, uint16_t);
static void	mspcic_bus_write_4(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, uint32_t);
static void	mspcic_bus_write_8(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, uint64_t);

static struct sparc_bus_space_tag mspcic_io_tag;
static struct sparc_bus_space_tag mspcic_mem_tag;


/*
 * DMA tag
 */
static int	mspcic_dmamap_load(bus_dma_tag_t, bus_dmamap_t,
				   void *, bus_size_t, struct proc *, int);
static void	mspcic_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
static int	mspcic_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *,
				  int, size_t, void **, int);

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
msiiep_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	pcireg_t id;

	/* match by PROM name */
	if (strcmp(ma->ma_name, "pci") != 0)
		return (0);

	/*
	 * Verify that PCIC was successfully mapped by bootstrap code.
	 * Since PCIC contains all the registers vital to the kernel,
	 * bootstrap code maps them at a fixed va, MSIIEP_PCIC_VA
	 */
	id = mspcic_read_4(pcic_id);
	if (PCI_VENDOR(id) != PCI_VENDOR_SUN
	    && PCI_PRODUCT(id) != PCI_PRODUCT_SUN_MS_IIep)
		panic("msiiep_match: id %08x", id);

	return (1);
}


static void
msiiep_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct msiiep_attach_args msa;
	bus_space_handle_t hmid;
	struct cpu_info *cur;

	aprint_normal("\n");

	if (bus_space_map(ma->ma_bustag, MSIIEP_MID_PA, 4, 0, &hmid) == 0) {
#ifdef DIAGNOSTICS
		uint32_t mid;

		mid = bus_space_read_4(ma->ma_bustag, hmid, 0);
		printf("MID: %08x\n", mid);
#endif
		msiiep_mid = (volatile uint32_t *)bus_space_vaddr(ma->ma_bustag,
		    hmid);
		cur = curcpu();
		cur->idlespin = msiiep_cpu_sleep;
	}

	/* pass on real mainbus_attach_args */
	msa.msa_ma = ma;

	/* config timer/counter part of PCIC */
	msa.msa_name = "timer";
	config_found(self, &msa, NULL);

	/* config PCI tree */
	msa.msa_name = "pcic";
	config_found(self, &msa, NULL);
}

/* ARGSUSED */
void
msiiep_cpu_sleep(struct cpu_info *ci)
{
	uint32_t reg;

	if (msiiep_mid == 0)
		return;
	reg = *msiiep_mid;
	*msiiep_mid = (reg & MID_MASK) | MID_STANDBY;
}


/* ======================================================================
 *
 *		      Real ms-IIep PCIC driver.
 */

static int
mspcic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct msiiep_attach_args *msa = aux;

	return (strcmp(msa->msa_name, "pcic") == 0);
}


static void
mspcic_attach(device_t parent, device_t self, void *aux)
{
	struct mspcic_softc *sc = device_private(self);
	struct msiiep_attach_args *msa = aux;
	struct mainbus_attach_args *ma = msa->msa_ma;
	int node = ma->ma_node;
	char devinfo[256], buf[32], *model;

	struct pcibus_attach_args pba;

	sc->sc_node = node;

	/* copy parent tags */
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	/* print our PCI device info and bus clock frequency */
	pci_devinfo(mspcic_read_4(pcic_id), mspcic_read_4(pcic_class), 0,
		    devinfo, sizeof(devinfo));
	printf(": %s: clock = %s MHz\n", devinfo,
	       clockfreq(prom_getpropint(node, "clock-frequency", 33333333)));

	mspcic_init_maps();

	/*
	 * On Espresso, we need to adjust the interrupt mapping on
	 * lines 4-7, as onboard devices and cards in the PCI slots use
	 * those lines.  Note, that enabling line 5 (onboard ide) causes
	 * a continuous interrupt stream, so leave that unmapped (0).
	 * Any other device using line 5 can't be used.
	 * Interrupt line wiring for slots is set in pci_machdep.c.
	 * Set the PCI Trdy and Retry Counters to non-zero values, otherwise
	 * we'll lock up when using devices behind a PCI-PCI bridge.
	 */
	model = prom_getpropstringA(prom_findroot(), "model", buf, sizeof(buf));
	if (model != NULL && !strcmp(model, "SUNW,375-0059")) {
		mspcic_write_2(pcic_intr_asgn_sel_hi, (uint16_t) 0x7502);
		mspcic_write_4(pcic_cntrs, (uint32_t) 0x00808000);
	}

	/* init cookies/parents in our statically allocated tags */
	mspcic_io_tag = *sc->sc_bustag;
	mspcic_io_tag.cookie = &mspcic_io_cookie;
	mspcic_io_tag.ranges = NULL;
	mspcic_io_tag.nranges = 0;
	mspcic_io_tag.sparc_bus_map = mspcic_bus_map;
	mspcic_io_tag.sparc_bus_mmap = mspcic_bus_mmap;
	mspcic_io_tag.sparc_intr_establish = mspcic_intr_establish;
	mspcic_io_tag.parent = sc->sc_bustag;

	mspcic_io_tag.sparc_read_2 = mspcic_bus_read_2;
	mspcic_io_tag.sparc_read_4 = mspcic_bus_read_4;
	mspcic_io_tag.sparc_read_8 = mspcic_bus_read_8;
	mspcic_io_tag.sparc_write_2 = mspcic_bus_write_2;
	mspcic_io_tag.sparc_write_4 = mspcic_bus_write_4;
	mspcic_io_tag.sparc_write_8 = mspcic_bus_write_8;

	mspcic_mem_tag = *sc->sc_bustag;
	mspcic_mem_tag.cookie = &mspcic_mem_cookie;
	mspcic_mem_tag.ranges = NULL;
	mspcic_mem_tag.nranges = 0;
	mspcic_mem_tag.sparc_bus_map = mspcic_bus_map;
	mspcic_mem_tag.sparc_bus_mmap = mspcic_bus_mmap;
	mspcic_mem_tag.sparc_intr_establish = mspcic_intr_establish;
	mspcic_mem_tag.parent = sc->sc_bustag;

	mspcic_mem_tag.sparc_read_2 = mspcic_bus_read_2;
	mspcic_mem_tag.sparc_read_4 = mspcic_bus_read_4;
	mspcic_mem_tag.sparc_read_8 = mspcic_bus_read_8;
	mspcic_mem_tag.sparc_write_2 = mspcic_bus_write_2;
	mspcic_mem_tag.sparc_write_4 = mspcic_bus_write_4;
	mspcic_mem_tag.sparc_write_8 = mspcic_bus_write_8;

	mspcic_dma_tag._cookie = sc;
	mspcic_pc_tag.cookie = sc;

	/* save bus tags in softc */
	sc->sc_iot = &mspcic_io_tag;
	sc->sc_memt = &mspcic_mem_tag;
	sc->sc_dmat = &mspcic_dma_tag;

	/*
	 * Attach the PCI bus.
	 */
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_memt;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &mspcic_pc_tag;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;

	mspcic_pci_scan(sc->sc_node);

	config_found_ia(self, "pcibus", &pba, mspcic_print);
}


static int
mspcic_print(void *args, const char *busname)
{

	if (busname == NULL)
		return (UNCONF);
	return (QUIET);
}


/*
 * Get the PIL currently assigned for this interrupt input line.
 */
int
mspcic_assigned_interrupt(int line)
{
	unsigned int intrmap;

	if (line < 0 || line > 7)
		return (-1);

	if (line < 4) {
		intrmap = mspcic_read_2(pcic_intr_asgn_sel);
	} else {
		intrmap = mspcic_read_2(pcic_intr_asgn_sel_hi);
		line -= 4;
	}
	return ((intrmap >> (line * 4)) & 0xf);
}


/* ======================================================================
 *
 *			  BUS space methods
 */

static inline void
mspcic_pci_map_from_reg(struct mspcic_pci_map *m,
			uint8_t sbar, uint8_t pbar, uint8_t sizemask)
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
mspcic_init_maps(void)
{
	struct mspcic_pci_map *m0, *m1, *io;
	int nmem, nio;

#ifdef DEBUG
	printf("mspcic0: SMBAR0 %02x  PMBAR0 %02x  MSIZE0 %02x\n",
	       mspcic_read_1(pcic_smbar0), mspcic_read_1(pcic_pmbar0),
	       mspcic_read_1(pcic_msize0));
	printf("mspcic0: SMBAR1 %02x  PMBAR1 %02x  MSIZE1 %02x\n",
	       mspcic_read_1(pcic_smbar1), mspcic_read_1(pcic_pmbar1),
	       mspcic_read_1(pcic_msize1));
	printf("mspcic0: SIBAR  %02x  PIBAR  %02x  IOSIZE %02x\n",
	       mspcic_read_1(pcic_sibar), mspcic_read_1(pcic_pibar),
	       mspcic_read_1(pcic_iosize));
#endif
	nmem = nio = 1;

	m0 = &mspcic_pci_memmap[nmem];
	mspcic_pci_map_from_reg(m0,
		mspcic_read_1(pcic_smbar0), mspcic_read_1(pcic_pmbar0),
		mspcic_read_1(pcic_msize0));
	if (OVERLAP_FIXED(m0))
		m0 = NULL;
	else
		++nmem;

	m1 = &mspcic_pci_memmap[nmem];
	mspcic_pci_map_from_reg(m1,
		mspcic_read_1(pcic_smbar1), mspcic_read_1(pcic_pmbar1),
		mspcic_read_1(pcic_msize1));
	if (OVERLAP_FIXED(m1) || OVERLAP_MAP(m1, m0))
		m1 = NULL;
	else
		++nmem;

	io = &mspcic_pci_iomap[nio];
	mspcic_pci_map_from_reg(io,
		mspcic_read_1(pcic_sibar), mspcic_read_1(pcic_pibar),
		mspcic_read_1(pcic_iosize));
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
mspcic_pci_map_print(struct mspcic_pci_map *m, const char *msg)
{
	printf("mspcic0: paddr [%08x..%08x] -> pci [%08x..%08x] %s\n",
	       m->sysbase, m->sysbase + m->size - 1,
	       m->pcibase, m->pcibase + m->size - 1,
	       msg);
}
#endif


static bus_addr_t
mspcic_pci_map_find(struct mspcic_pci_map *m, int nmaps,
		    bus_addr_t pciaddr, bus_size_t size)
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
mspcic_bus_map(bus_space_tag_t t, bus_addr_t ba, bus_size_t size,
	       int flags, vaddr_t va, bus_space_handle_t *hp)
{
	struct mspcic_cookie *c = t->cookie;
	bus_addr_t paddr;

	paddr = mspcic_pci_map_find(c->map, c->nmaps, ba, size);
	if (paddr == 0)
		return (EINVAL);
	return (bus_space_map2(t->parent, paddr, size, flags, va, hp));
}


static paddr_t
mspcic_bus_mmap(bus_space_tag_t t, bus_addr_t ba, off_t off,
		int prot, int flags)
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
mspcic_intr_establish(bus_space_tag_t t, int line, int ipl,
		      int (*handler)(void *), void *arg,
		      void (*fastvec)(void))
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
	intr_establish(pil, ipl, ih, fastvec, false);

	return(ih);
}


static uint16_t
mspcic_bus_read_2(bus_space_tag_t space, bus_space_handle_t handle,
		  bus_size_t offset)
{
	uint16_t val = *(volatile uint16_t *)(handle + offset);

	return le16toh(val);
}


static uint32_t
mspcic_bus_read_4(bus_space_tag_t space, bus_space_handle_t handle,
		  bus_size_t offset)
{
	uint32_t val = *(volatile uint32_t *)(handle + offset);

	return le32toh(val);
}


static uint64_t
mspcic_bus_read_8(bus_space_tag_t space, bus_space_handle_t handle,
		  bus_size_t offset)
{
	uint64_t val = *(volatile uint64_t *)(handle + offset);

	return le64toh(val);
}


static void
mspcic_bus_write_2(bus_space_tag_t space, bus_space_handle_t handle,
		   bus_size_t offset, uint16_t value)
{

	(*(volatile uint16_t *)(handle + offset)) = htole16(value);
}


static void
mspcic_bus_write_4(bus_space_tag_t space, bus_space_handle_t handle,
		   bus_size_t offset, uint32_t value)
{

	(*(volatile uint32_t *)(handle + offset)) = htole32(value);
}


static void
mspcic_bus_write_8(bus_space_tag_t space, bus_space_handle_t handle,
		   bus_size_t offset, uint64_t value)
{

	(*(volatile uint64_t *)(handle + offset)) = htole64(value);
}


/* ======================================================================
 *
 *			     DMA methods
 */

static int
mspcic_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map,
		   void *buf, bus_size_t buflen,
		   struct proc *p, int flags)
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
mspcic_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}


static int
mspcic_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs,
		  size_t size, void **kvap, int flags)
{
	struct pglist *mlist;
	struct vm_page *m;
	vaddr_t va;
	int pagesz = PAGE_SIZE;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;

	if (nsegs != 1)
		panic("mspcic_dmamem_map: nsegs = %d", nsegs);

	size = round_page(size);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (void *)va;

	/*
	 * Map the pages allocated in _bus_dmamem_alloc()
	 * to the kernel virtual address space.
	 */
	mlist = segs[0]._ds_mlist;
	TAILQ_FOREACH(m, mlist, pageq.queue) {
		paddr_t pa;

		if (size == 0)
			panic("mspcic_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va,
		    pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE, 0);
		va += pagesz;
		size -= pagesz;
	}
	pmap_update(pmap_kernel());

	return (0);
}
