/*	$NetBSD: pyro.c,v 1.26 2022/01/21 19:14:14 thorpej Exp $	*/
/*	from: $OpenBSD: pyro.c,v 1.20 2010/12/05 15:15:14 kettenis Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
 * Copyright (c) 2003 Henric Jungheim
 * Copyright (c) 2007 Mark Kettenis
 * Copyright (c) 2011 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pyro.c,v 1.26 2022/01/21 19:14:14 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/autoconf.h>

#ifdef DDB
#include <machine/db_machdep.h>
#endif

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/pyrovar.h>

#ifdef DEBUG
#define PDB_PROM        0x01
#define PDB_BUSMAP      0x02
#define PDB_INTR        0x04
#define PDB_CONF        0x08
int pyro_debug = 0x0;
#define DPRINTF(l, s)   do { if (pyro_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#define FIRE_RESET_GEN			0x7010

#define FIRE_RESET_GEN_XIR		0x0000000000000002L

#define FIRE_INTRMAP_INT_CNTRL_NUM_MASK	0x000003c0
#define FIRE_INTRMAP_INT_CNTRL_NUM0	0x00000040
#define FIRE_INTRMAP_INT_CNTRL_NUM1	0x00000080
#define FIRE_INTRMAP_INT_CNTRL_NUM2	0x00000100
#define FIRE_INTRMAP_INT_CNTRL_NUM3	0x00000200
#define FIRE_INTRMAP_T_JPID_SHIFT	26
#define FIRE_INTRMAP_T_JPID_MASK	0x7c000000

#define OBERON_INTRMAP_T_DESTID_SHIFT	21
#define OBERON_INTRMAP_T_DESTID_MASK	0x7fe00000

extern struct sparc_pci_chipset _sparc_pci_chipset;

int pyro_match(device_t, cfdata_t, void *);
void pyro_attach(device_t, device_t, void *);
int pyro_print(void *, const char *);

CFATTACH_DECL_NEW(pyro, sizeof(struct pyro_softc),
    pyro_match, pyro_attach, NULL, NULL);

void pyro_init(struct pyro_softc *, int);
void pyro_init_iommu(struct pyro_softc *, struct pyro_pbm *);

pci_chipset_tag_t pyro_alloc_chipset(struct pyro_pbm *, int,
    pci_chipset_tag_t);
bus_space_tag_t pyro_alloc_mem_tag(struct pyro_pbm *);
bus_space_tag_t pyro_alloc_io_tag(struct pyro_pbm *);
bus_space_tag_t pyro_alloc_config_tag(struct pyro_pbm *);
bus_space_tag_t pyro_alloc_bus_tag(struct pyro_pbm *, const char *, int);
bus_dma_tag_t pyro_alloc_dma_tag(struct pyro_pbm *);

#if 0
int pyro_conf_size(pci_chipset_tag_t, pcitag_t);
#endif
pcireg_t pyro_conf_read(pci_chipset_tag_t, pcitag_t, int);
void pyro_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

static void * pyro_pci_intr_establish(pci_chipset_tag_t pc,
				      pci_intr_handle_t ih, int level,
				      int (*func)(void *), void *arg);

int pyro_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
int pyro_bus_map(bus_space_tag_t, bus_addr_t,
    bus_size_t, int, vaddr_t, bus_space_handle_t *);
paddr_t pyro_bus_mmap(bus_space_tag_t, bus_addr_t, off_t,
    int, int);
void *pyro_intr_establish(bus_space_tag_t, int, int,
    int (*)(void *), void *, void (*)(void));

int pyro_dmamap_create(bus_dma_tag_t, bus_size_t, int,
    bus_size_t, bus_size_t, int, bus_dmamap_t *);

int
pyro_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	char *str;

	if (strcmp(ma->ma_name, "pci") != 0)
		return (0);

	str = prom_getpropstring(ma->ma_node, "compatible");
	if (strcmp(str, "pciex108e,80f0") == 0 ||
	    strcmp(str, "pciex108e,80f8") == 0)
		return (1);

	return (0);
}

void
pyro_attach(device_t parent, device_t self, void *aux)
{
	struct pyro_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	char *str;
	int busa;

	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;
	sc->sc_dmat = ma->ma_dmatag;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_csr = ma->ma_reg[0].ur_paddr;
	sc->sc_xbc = ma->ma_reg[1].ur_paddr;
	sc->sc_ign = INTIGN(ma->ma_upaid << INTMAP_IGN_SHIFT);

	if ((ma->ma_reg[0].ur_paddr & 0x00700000) == 0x00600000)
		busa = 1;
	else
		busa = 0;

	if (bus_space_map(sc->sc_bustag, sc->sc_csr,
	    ma->ma_reg[0].ur_len, BUS_SPACE_MAP_LINEAR, &sc->sc_csrh)) {
		printf(": failed to map csr registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bustag, sc->sc_xbc,
	    ma->ma_reg[1].ur_len, 0, &sc->sc_xbch)) {
		printf(": failed to map xbc registers\n");
		return;
	}

	str = prom_getpropstring(ma->ma_node, "compatible");
	if (strcmp(str, "pciex108e,80f8") == 0)
		sc->sc_oberon = 1;

	pyro_init(sc, busa);
}

void
pyro_init(struct pyro_softc *sc, int busa)
{
	struct pyro_pbm *pbm;
	struct pcibus_attach_args pba;
	int *busranges = NULL, nranges;

	pbm = kmem_zalloc(sizeof(*pbm), KM_SLEEP);
	pbm->pp_sc = sc;
	pbm->pp_bus_a = busa;

	if (prom_getprop(sc->sc_node, "ranges", sizeof(struct pyro_range),
	    &pbm->pp_nrange, (void **)&pbm->pp_range))
		panic("pyro: can't get ranges");

	if (prom_getprop(sc->sc_node, "bus-range", sizeof(int), &nranges,
	    (void **)&busranges))
		panic("pyro: can't get bus-range");

	printf(": \"%s\", rev %d, ign %x, bus %c %d to %d\n",
	    sc->sc_oberon ? "Oberon" : "Fire",
	    prom_getpropint(sc->sc_node, "module-revision#", 0), sc->sc_ign,
	    busa ? 'A' : 'B', busranges[0], busranges[1]);

	printf("%s: ", device_xname(sc->sc_dev));
	pyro_init_iommu(sc, pbm);

	pbm->pp_memt = pyro_alloc_mem_tag(pbm);
	pbm->pp_iot = pyro_alloc_io_tag(pbm);
	pbm->pp_cfgt = pyro_alloc_config_tag(pbm);
	pbm->pp_dmat = pyro_alloc_dma_tag(pbm);
	pbm->pp_flags = (pbm->pp_memt ? PCI_FLAGS_MEM_OKAY : 0) |
		        (pbm->pp_iot ? PCI_FLAGS_IO_OKAY : 0);

	if (bus_space_map(pbm->pp_cfgt, 0, 0x10000000, 0, &pbm->pp_cfgh))
		panic("pyro: can't map config space");

	pbm->pp_pc = pyro_alloc_chipset(pbm, sc->sc_node, &_sparc_pci_chipset);
	pbm->pp_pc->spc_busmax = busranges[1];
	pbm->pp_pc->spc_busnode = kmem_zalloc(sizeof(*pbm->pp_pc->spc_busnode),
	    KM_SLEEP);

#if 0
	pbm->pp_pc->bustag = pbm->pp_cfgt;
	pbm->pp_pc->bushandle = pbm->pp_cfgh;
#endif

	bzero(&pba, sizeof(pba));
	pba.pba_bus = busranges[0];
	pba.pba_pc = pbm->pp_pc;
	pba.pba_flags = pbm->pp_flags;
	pba.pba_dmat = pbm->pp_dmat;
	pba.pba_dmat64 = NULL;	/* XXX */
	pba.pba_memt = pbm->pp_memt;
	pba.pba_iot = pbm->pp_iot;

	free(busranges, M_DEVBUF);

	config_found(sc->sc_dev, &pba, pyro_print,
	    CFARGS(.devhandle = device_handle(sc->sc_dev)));
}

void
pyro_init_iommu(struct pyro_softc *sc, struct pyro_pbm *pbm)
{
	struct iommu_state *is = &pbm->pp_is;
	int tsbsize = 7;
	u_int32_t iobase = -1;
	char *name;

	pbm->pp_sb.sb_is = is;
	is->is_bustag = sc->sc_bustag;

	if (bus_space_subregion(is->is_bustag, sc->sc_csrh,
	    0x40000, 0x100, &is->is_iommu)) {
		panic("pyro: unable to create iommu handle");
	}

	/* We have no STC.  */
	is->is_sb[0] = NULL;

	name = kmem_asprintf("%s dvma", device_xname(sc->sc_dev));

	/* Tell iommu how to set the TSB size.  */
	is->is_flags = IOMMU_TSBSIZE_IN_PTSB;

	/* On Oberon, we need to flush the cache. */
	if (sc->sc_oberon)
		is->is_flags |= IOMMU_FLUSH_CACHE;

	iommu_init(name, is, tsbsize, iobase);
}

int
pyro_print(void *aux, const char *p)
{
	if (p == NULL)
		return (UNCONF);
	return (QUIET);
}

pcireg_t
pyro_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct pyro_pbm *pp = pc->cookie;
	pcireg_t val = (pcireg_t)~0;
	int s;

	DPRINTF(PDB_CONF, ("%s: tag %lx reg %x ", __func__, (long)tag, reg));
	if (PCITAG_NODE(tag) != -1 && (unsigned int)reg < PCI_CONF_SIZE) {
		s = splhigh();
		struct cpu_info *ci = curcpu();
		ci->ci_pci_probe = true;
		membar_Sync();
		val = bus_space_read_4(pp->pp_cfgt, pp->pp_cfgh,
		    (PCITAG_OFFSET(tag) << 4) + reg);
		membar_Sync();
		if (ci->ci_pci_fault)
			val = (pcireg_t)~0;
		ci->ci_pci_probe = ci->ci_pci_fault = false;
		splx(s);
	}
	DPRINTF(PDB_CONF, (" returning %08x\n", (u_int)val));
	return (val);
}

void
pyro_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct pyro_pbm *pp = pc->cookie;

	DPRINTF(PDB_CONF, ("%s: tag %lx; reg %x; data %x", __func__,
		(long)tag, reg, (int)data));

	/* If we don't know it, just punt it.  */
	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(PDB_CONF, (" .. bad addr\n"));
		return;
	}

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

        bus_space_write_4(pp->pp_cfgt, pp->pp_cfgh,
	    (PCITAG_OFFSET(tag) << 4) + reg, data);
	DPRINTF(PDB_CONF, (" .. done\n"));
}

/*
 * Bus-specific interrupt mapping
 */
int
pyro_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct pyro_pbm *pp = pa->pa_pc->cookie;
	struct pyro_softc *sc = pp->pp_sc;
	u_int dev;

	if (*ihp != (pci_intr_handle_t)-1) {
		*ihp |= sc->sc_ign;
		DPRINTF(PDB_INTR, ("%s: not -1 -> ih %lx\n", __func__, (u_long)*ihp));
		return (0);
	}

	/*
	 * We didn't find a PROM mapping for this interrupt.  Try to
	 * construct one ourselves based on the swizzled interrupt pin
	 * and the interrupt mapping for PCI slots documented in the
	 * UltraSPARC-IIi User's Manual.
	 */

	if (pa->pa_intrpin == 0) {
		DPRINTF(PDB_INTR, ("%s: no intrpen\n", __func__));
		return (-1);
	}

	/*
	 * This deserves some documentation.  Should anyone
	 * have anything official looking, please speak up.
	 */
	dev = pa->pa_device - 1;

	*ihp = (pa->pa_intrpin - 1) & INTMAP_PCIINT;
	*ihp |= (dev << 2) & INTMAP_PCISLOT;
	*ihp |= sc->sc_ign;

	DPRINTF(PDB_INTR, ("%s: weird hack -> ih %lx\n", __func__, (u_long)*ihp));
	return (0);
}

bus_space_tag_t
pyro_alloc_mem_tag(struct pyro_pbm *pp)
{
	return (pyro_alloc_bus_tag(pp, "mem", PCI_MEMORY_BUS_SPACE));
}

bus_space_tag_t
pyro_alloc_io_tag(struct pyro_pbm *pp)
{
	return (pyro_alloc_bus_tag(pp, "io", PCI_IO_BUS_SPACE));
}

bus_space_tag_t
pyro_alloc_config_tag(struct pyro_pbm *pp)
{
	return (pyro_alloc_bus_tag(pp, "cfg", PCI_CONFIG_BUS_SPACE));
}

bus_space_tag_t
pyro_alloc_bus_tag(struct pyro_pbm *pbm, const char *name, int type)
{
	struct pyro_softc *sc = pbm->pp_sc;
	struct sparc_bus_space_tag *bt;

	bt = kmem_zalloc(sizeof(*bt), KM_SLEEP);

#if 0
	snprintf(bt->name, sizeof(bt->name), "%s-pbm_%s(%d/%2.2x)",
	    device_xname(sc->sc_dev), name, ss, asi);
#endif

	bt->cookie = pbm;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = pyro_bus_map;
	bt->sparc_bus_mmap = pyro_bus_mmap;
	bt->sparc_intr_establish = pyro_intr_establish;
	return (bt);
}

bus_dma_tag_t
pyro_alloc_dma_tag(struct pyro_pbm *pbm)
{
	struct pyro_softc *sc = pbm->pp_sc;
	bus_dma_tag_t dt, pdt = sc->sc_dmat;

	dt = kmem_zalloc(sizeof(*dt), KM_SLEEP);
	dt->_cookie = pbm;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	dt->_dmamap_create	= pyro_dmamap_create;
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load	= iommu_dvmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	dt->_dmamap_load_raw	= iommu_dvmamap_load_raw;
	dt->_dmamap_unload	= iommu_dvmamap_unload;
	dt->_dmamap_sync	= iommu_dvmamap_sync;
	dt->_dmamem_alloc	= iommu_dvmamem_alloc;
	dt->_dmamem_free	= iommu_dvmamem_free;
	dt->_dmamem_map = iommu_dvmamem_map;
	dt->_dmamem_unmap = iommu_dvmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

pci_chipset_tag_t
pyro_alloc_chipset(struct pyro_pbm *pbm, int node, pci_chipset_tag_t pc)
{
	pci_chipset_tag_t npc;

	npc = kmem_alloc(sizeof *npc, KM_SLEEP);
	memcpy(npc, pc, sizeof *pc);
	npc->cookie = pbm;
	npc->rootnode = node;
	npc->spc_conf_read = pyro_conf_read;
	npc->spc_conf_write = pyro_conf_write;
	npc->spc_intr_map = pyro_intr_map;
	npc->spc_intr_establish = pyro_pci_intr_establish;
	npc->spc_find_ino = NULL;
	return (npc);
}

int
pyro_dmamap_create(bus_dma_tag_t t, bus_size_t size,
    int nsegments, bus_size_t maxsegsz, bus_size_t boundary, int flags,
    bus_dmamap_t *dmamp)
{
	struct pyro_pbm *pbm = t->_cookie;
	int error;

	error = bus_dmamap_create(t->_parent, size, nsegments, maxsegsz,
				  boundary, flags, dmamp);
	if (error == 0)
		(*dmamp)->_dm_cookie = &pbm->pp_sb;
	return error;
}

int
pyro_bus_map(bus_space_tag_t t, bus_addr_t offset,
    bus_size_t size, int flags, vaddr_t unused, bus_space_handle_t *hp)
{
	struct pyro_pbm *pbm = t->cookie;
	struct pyro_softc *sc = pbm->pp_sc;
	int i, ss;

	DPRINTF(PDB_BUSMAP, ("pyro_bus_map: type %d off %qx sz %qx flags %d",
	    t->type,
	    (unsigned long long)offset,
	    (unsigned long long)size,
	    flags));

	/*
	 * BUS_SPACE_MAP_PREFETCHABLE causes hard hangs on schizo, so weed it
	 * out for now until someone can verify whether it works on pyro
	 */
	flags &= ~BUS_SPACE_MAP_PREFETCHABLE;

	ss = sparc_pci_childspace(t->type);
	DPRINTF(PDB_BUSMAP, (" cspace %d", ss));

	if (t->parent == 0 || t->parent->sparc_bus_map == 0) {
		printf("\n_pyro_bus_map: invalid parent");
		return (EINVAL);
	}

	for (i = 0; i < pbm->pp_nrange; i++) {
		bus_addr_t paddr;
		struct pyro_range *pr = &pbm->pp_range[i];

		if (((pr->cspace >> 24) & 0x03) != ss)
			continue;

		paddr = BUS_ADDR(pr->phys_hi, pr->phys_lo + offset);
		return ((*sc->sc_bustag->sparc_bus_map)(t, paddr, size, 
			flags, 0, hp));
	}

	return (EINVAL);
}

paddr_t
pyro_bus_mmap(bus_space_tag_t t, bus_addr_t paddr,
    off_t off, int prot, int flags)
{
	bus_addr_t offset = paddr;
	struct pyro_pbm *pbm = t->cookie;
	struct pyro_softc *sc = pbm->pp_sc;
	int i, ss;

	/*
	 * BUS_SPACE_MAP_PREFETCHABLE causes hard hangs on schizo, so weed it
	 * out for now until someone can verify whether it works on pyro
	 */
	flags &= ~BUS_SPACE_MAP_PREFETCHABLE;

	ss = sparc_pci_childspace(t->type);

	DPRINTF(PDB_BUSMAP, ("pyro_bus_mmap: prot %d flags %d pa %qx\n",
	    prot, flags, (unsigned long long)paddr));

	if (t->parent == 0 || t->parent->sparc_bus_mmap == 0) {
		printf("\n_pyro_bus_mmap: invalid parent");
		return (-1);
	}

	for (i = 0; i < pbm->pp_nrange; i++) {
		struct pyro_range *pr = &pbm->pp_range[i];

		if (((pr->cspace >> 24) & 0x03) != ss)
			continue;

		paddr = BUS_ADDR(pr->phys_hi, pr->phys_lo + offset);
		return (bus_space_mmap(sc->sc_bustag, paddr, off,
				       prot, flags));
	}

	return (-1);
}

void *
pyro_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	struct pyro_pbm *pbm = t->cookie;
	struct pyro_softc *sc = pbm->pp_sc;
	struct intrhand *ih = NULL;
	volatile u_int64_t *intrmapptr = NULL, *intrclrptr = NULL;
	u_int64_t *imapbase, *iclrbase;
	int ino;

	ino = INTINO(ihandle);
	DPRINTF(PDB_INTR, ("%s: ih %lx; level %d ino %#x", __func__, (u_long)ihandle, level, ino));

	if (level == IPL_NONE)
		level = INTLEV(ihandle);
	if (level == IPL_NONE) {
		printf(": no IPL, setting IPL 2.\n");
		level = 2;
	}

	imapbase = (uint64_t *)((uintptr_t)bus_space_vaddr(sc->sc_bustag, sc->sc_csrh) + 0x1000);
	iclrbase = (uint64_t *)((uintptr_t)bus_space_vaddr(sc->sc_bustag, sc->sc_csrh) + 0x1400);
	intrmapptr = &imapbase[ino];
	intrclrptr = &iclrbase[ino];
	DPRINTF(PDB_INTR, (" mapptr %p clrptr %p\n", intrmapptr, intrclrptr));

	ino |= INTVEC(ihandle);

	ih = intrhand_alloc();

	/* Register the map and clear intr registers */
	ih->ih_map = intrmapptr;
	ih->ih_clr = intrclrptr;

	ih->ih_ivec = ihandle;
	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino;
	ih->ih_pending = 0;

	intr_establish(ih->ih_pil, level != IPL_VM, ih);

	if (intrmapptr != NULL) {
		u_int64_t imap;

		imap = *intrmapptr;
		DPRINTF(PDB_INTR, ("%s: read intrmap = %016qx", __func__,
			(unsigned long long)imap));
		imap &= ~FIRE_INTRMAP_INT_CNTRL_NUM_MASK;
		imap |= FIRE_INTRMAP_INT_CNTRL_NUM0;
		DPRINTF(PDB_INTR, ("; set intr group intrmap = %016qx",
			(unsigned long long)imap));
		if (sc->sc_oberon) {
			imap &= ~OBERON_INTRMAP_T_DESTID_MASK;
			imap |= CPU_JUPITERID <<
			    OBERON_INTRMAP_T_DESTID_SHIFT;
		} else {
			imap &= ~FIRE_INTRMAP_T_JPID_MASK;
			imap |= CPU_UPAID << FIRE_INTRMAP_T_JPID_SHIFT;
		}
		DPRINTF(PDB_INTR, ("; set cpuid num intrmap = %016qx",
			(unsigned long long)imap));
		imap |= INTMAP_V;
		*intrmapptr = imap;
		DPRINTF(PDB_INTR, ("; writing intrmap = %016qx",
			(unsigned long long)imap));
		imap = *intrmapptr;
		ih->ih_number |= imap & INTMAP_INR;
		DPRINTF(PDB_INTR, ("; reread intrmap = %016qx, "
				   "set ih_number to %x\n",
				   (unsigned long long)imap, ih->ih_number));
	}
 	if (intrclrptr) {
 		/* set state to IDLE */
		*intrclrptr = 0;
 	}

	return (ih);
}

static void *
pyro_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
	int (*func)(void *), void *arg)
{
	void *cookie;
	struct pyro_pbm *pbm = (struct pyro_pbm *)pc->cookie;

	DPRINTF(PDB_INTR, ("%s: ih %lx; level %d\n", __func__, (u_long)ih, level));
	cookie = bus_intr_establish(pbm->pp_memt, ih, level, func, arg);

	DPRINTF(PDB_INTR, ("%s: returning handle %p\n", __func__, cookie));
	return (cookie);
}
