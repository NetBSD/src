/*	$NetBSD: vpci.c,v 1.13 2022/01/21 19:14:14 thorpej Exp $	*/
/*
 * Copyright (c) 2015 Palle Lyckegaard
 * All rights reserved.
 *
 * Driver for virtual PCIe host bridge on sun4v systems.
 *
 * Based on NetBSD pyro and OpenBSD vpci drivers.
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
__KERNEL_RCSID(0, "$NetBSD: vpci.c,v 1.13 2022/01/21 19:14:14 thorpej Exp $");

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
#include <sparc64/dev/vpcivar.h>

#include <machine/hypervisor.h>

#ifdef DEBUG
#define VDB_PROM             0x01
#define VDB_BUSMAP           0x02
#define VDB_INTR             0x04
#define VDB_CONF_READ        0x08
#define VDB_CONF_WRITE       0x10
#define VDB_CONF             VDB_CONF_READ|VDB_CONF_WRITE
int vpci_debug = 0x00;
#define DPRINTF(l, s)   do { if (vpci_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#if 0
FIXME
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
#endif

extern struct sparc_pci_chipset _sparc_pci_chipset;

int vpci_match(device_t, cfdata_t, void *);
void vpci_attach(device_t, device_t, void *);
int vpci_print(void *, const char *);

CFATTACH_DECL_NEW(vpci, sizeof(struct vpci_softc),
    vpci_match, vpci_attach, NULL, NULL);

void vpci_init(struct vpci_softc */*FIXME, int*/, struct mainbus_attach_args *);
void vpci_init_iommu(struct vpci_softc *, struct vpci_pbm *);
pci_chipset_tag_t vpci_alloc_chipset(struct vpci_pbm *, int,
    pci_chipset_tag_t);
bus_space_tag_t vpci_alloc_mem_tag(struct vpci_pbm *);
bus_space_tag_t vpci_alloc_io_tag(struct vpci_pbm *);
bus_space_tag_t vpci_alloc_config_tag(struct vpci_pbm *);
bus_space_tag_t vpci_alloc_bus_tag(struct vpci_pbm *, const char *, int);
bus_dma_tag_t vpci_alloc_dma_tag(struct vpci_pbm *);

#if 0
int vpci_conf_size(pci_chipset_tag_t, pcitag_t);
#endif
pcireg_t vpci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void vpci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

static void * vpci_pci_intr_establish(pci_chipset_tag_t pc,
				      pci_intr_handle_t ih, int level,
				      int (*func)(void *), void *arg);
void vpci_intr_ack(struct intrhand *);
int vpci_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
int vpci_bus_map(bus_space_tag_t, bus_addr_t,
    bus_size_t, int, vaddr_t, bus_space_handle_t *);
paddr_t vpci_bus_mmap(bus_space_tag_t, bus_addr_t, off_t,
    int, int);
void *vpci_intr_establish(bus_space_tag_t, int, int,
    int (*)(void *), void *, void (*)(void));

int vpci_dmamap_create(bus_dma_tag_t, bus_size_t, int,
    bus_size_t, bus_size_t, int, bus_dmamap_t *);

int
vpci_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	char compat[32];

	if (strcmp(ma->ma_name, "pci") != 0)
		return (0);

	if (OF_getprop(ma->ma_node, "compatible", compat, sizeof(compat)) == -1)
		return (0);

	if (strcmp(compat, "SUNW,sun4v-pci") == 0)
		return (1);

	return (0);
}

void
vpci_attach(device_t parent, device_t self, void *aux)
{
	struct vpci_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
#if 0
FIXME	
	char *str;
	int busa;
#endif
	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;
	sc->sc_dmat = ma->ma_dmatag;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_csr = ma->ma_reg[0].ur_paddr;
#if 0
FIXME	
	sc->sc_xbc = ma->ma_reg[1].ur_paddr;
	sc->sc_ign = INTIGN(ma->ma_upaid << INTMAP_IGN_SHIFT);

	if ((ma->ma_reg[0].ur_paddr & 0x00700000) == 0x00600000)
		busa = 1;
	else
		busa = 0;
#endif
#if 0
FIXME	
	if (bus_space_map(sc->sc_bustag, sc->sc_csr,
	    ma->ma_reg[0].ur_len, BUS_SPACE_MAP_LINEAR, &sc->sc_csrh)) {
		printf(": failed to map csr registers\n");
		return;
	}
#endif 
#if 0
FIXME	
	if (bus_space_map(sc->sc_bustag, sc->sc_xbc,
	    ma->ma_reg[1].ur_len, 0, &sc->sc_xbch)) {
		printf(": failed to map xbc registers\n");
		return;
	}

	str = prom_getpropstring(ma->ma_node, "compatible");
	if (strcmp(str, "pciex108e,80f8") == 0)
		sc->sc_oberon = 1;

#endif	
	vpci_init(sc/*FIXME, busa*/, ma);
}

void
vpci_init(struct vpci_softc *sc/*FIXME, int busa*/, struct mainbus_attach_args *ma)
{
	struct vpci_pbm *pbm;
	struct pcibus_attach_args pba;
	int *busranges = NULL, nranges;

	pbm = kmem_zalloc(sizeof(*pbm), KM_SLEEP);
	pbm->vp_sc = sc;
	pbm->vp_devhandle = (ma->ma_reg[0].ur_paddr >> 32) & 0x0fffffff;
#if 0
FiXME	
	pbm->vp_bus_a = busa;
#endif 

	if (prom_getprop(sc->sc_node, "ranges", sizeof(struct vpci_range),
	    &pbm->vp_nrange, (void **)&pbm->vp_range))
		panic("vpci: can't get ranges");
	for (int range = 0; range < pbm->vp_nrange; range++)
		DPRINTF(VDB_PROM,
			("\nvpci_attach: range %d  cspace %08x  "
			"child_hi %08x  child_lo %08x  phys_hi %08x  phys_lo %08x  "
			"size_hi %08x  size_lo %08x", range,
			pbm->vp_range[range].cspace,
			pbm->vp_range[range].child_hi,
			pbm->vp_range[range].child_lo,
			pbm->vp_range[range].phys_hi,
			pbm->vp_range[range].phys_lo,
			pbm->vp_range[range].size_hi,
			pbm->vp_range[range].size_lo));

	if (prom_getprop(sc->sc_node, "bus-range", sizeof(int), &nranges,
	    (void **)&busranges))
		panic("vpci: can't get bus-range");
	for (int range = 0; range < nranges; range++)
		DPRINTF(VDB_PROM, ("\nvpci_attach: bus-range %d %08x", range, busranges[range]));

 	aprint_normal(": bus %d to %d", busranges[0], busranges[1]);
	
	vpci_init_iommu(sc, pbm);

	pbm->vp_memt = vpci_alloc_mem_tag(pbm);
	pbm->vp_iot = vpci_alloc_io_tag(pbm);
	pbm->vp_cfgt = vpci_alloc_config_tag(pbm);
	pbm->vp_dmat = vpci_alloc_dma_tag(pbm);
	pbm->vp_flags = (pbm->vp_memt ? PCI_FLAGS_MEM_OKAY : 0) |
		        (pbm->vp_iot ? PCI_FLAGS_IO_OKAY : 0);
#if 0
FIXME	
	if (bus_space_map(pbm->vp_cfgt, 0, 0x10000000, 0, &pbm->vp_cfgh))
		panic("vpci: can't map config space");
#endif
	pbm->vp_pc = vpci_alloc_chipset(pbm, sc->sc_node, &_sparc_pci_chipset);
	pbm->vp_pc->spc_busmax = busranges[1];
	pbm->vp_pc->spc_busnode = kmem_zalloc(sizeof(*pbm->vp_pc->spc_busnode),
	    KM_SLEEP);

#if 0
	pbm->vp_pc->bustag = pbm->vp_cfgt;
	pbm->vp_pc->bushandle = pbm->vp_cfgh;
#endif

	bzero(&pba, sizeof(pba));
	pba.pba_bus = busranges[0];
	pba.pba_pc = pbm->vp_pc;
	pba.pba_flags = pbm->vp_flags;
	pba.pba_dmat = pbm->vp_dmat;
	pba.pba_dmat64 = NULL;	/* XXX */
	pba.pba_memt = pbm->vp_memt;
	pba.pba_iot = pbm->vp_iot;

	free(busranges, M_DEVBUF);

	config_found(sc->sc_dev, &pba, vpci_print,
	    CFARGS(.devhandle = device_handle(sc->sc_dev)));
}

void
vpci_init_iommu(struct vpci_softc *sc, struct vpci_pbm *pbm)
{
	struct iommu_state *is = &pbm->vp_is;
	int *vdma = NULL;
	int nitem;
	int tsbsize = 0;
	u_int32_t iobase = -1;
	u_int32_t iolen = 0;
	char *name;

	pbm->vp_sb.sb_is = is;
	is->is_bustag = sc->sc_bustag;

	if (bus_space_subregion(is->is_bustag, sc->sc_csrh,
	    0x40000, 0x100, &is->is_iommu)) {
		panic("vpci: unable to create iommu handle");
	}

	/* Construct tsbsize */
	if (!prom_getprop(sc->sc_node, "virtual-dma", sizeof(vdma), &nitem, 
	    (void **)&vdma)) {
		DPRINTF(VDB_BUSMAP, ("vpci_init_iommu: vdma[0]=0x%x  vdma[1]=0x%x\n",
		    vdma[0], vdma[1]));
		iobase = vdma[0];
		iolen = vdma[1];
		for (tsbsize = 8; (1 << (tsbsize+23)) > iolen;)
			tsbsize--;
		DPRINTF(VDB_BUSMAP, ("vpci_init_iommu: iobase=0x%x  iolen = 0x%x  tsbsize=0x%x\n",
		    iobase, iolen, tsbsize));
		free(vdma, M_DEVBUF);
	} else
		panic("vpci_init_iommu: getprop virtual-dma failed");

	aprint_normal(" vdma %x length %x\n", iobase, iolen);
	aprint_naive("\n");
	
	/* We have no STC.  */
	is->is_sb[0] = NULL;

	name = kmem_asprintf("%s dvma", device_xname(sc->sc_dev));

	/* Tell iommu how to set the TSB size.  */
	is->is_flags = IOMMU_TSBSIZE_IN_PTSB;

	is->is_devhandle = pbm->vp_devhandle;
	iommu_init(name, is, tsbsize, iobase);
}


int
vpci_print(void *aux, const char *p)
{
	if (p == NULL)
		return (UNCONF);
	return (QUIET);
}

pcireg_t
vpci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct vpci_pbm *pbm = pc->cookie;
	uint64_t error_flag, data;

	int64_t hv_rc;
	DPRINTF(VDB_CONF_READ, ("%s: tag %lx reg %x ", __func__, (long)tag, reg));
	hv_rc = hv_pci_config_get(pbm->vp_devhandle, PCITAG_OFFSET(tag), reg, 4,
	    &error_flag, &data);
	if (hv_rc != H_EOK)
		panic("hv_pci_config_get() failed - rc = %" PRId64 "\n",
		    hv_rc);
	
	pcireg_t val = error_flag ? (pcireg_t)~0 : data;
	DPRINTF(VDB_CONF_READ, (" returning %08x\n", (u_int)val));
	return val;
}

void
vpci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	struct vpci_pbm *pbm = pc->cookie;
	uint64_t error_flag;
	int64_t hv_rc;
	DPRINTF(VDB_CONF_WRITE, ("%s: tag %lx; reg %x; data %x", __func__,
		(long)tag, reg, (int)data));
	hv_rc = hv_pci_config_put(pbm->vp_devhandle, PCITAG_OFFSET(tag), reg, 4,
            data, &error_flag);
	if (hv_rc != H_EOK)
		panic("hv_pci_config_put() failed - rc = %" PRId64 "\n",
		    hv_rc);
	DPRINTF(VDB_CONF_WRITE, (" .. done\n"));
}

/*
 * Bus-specific interrupt mapping
 */
int
vpci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct vpci_pbm *pbm = pa->pa_pc->cookie;
	uint64_t devhandle = pbm->vp_devhandle;
	uint64_t devino = INTINO(*ihp);
	DPRINTF(VDB_INTR, ("vpci_intr_map(): devino 0x%lx\n", devino));
	uint64_t sysino;
	int err;

	if (*ihp != (pci_intr_handle_t)-1) {
		err = hv_intr_devino_to_sysino(devhandle, devino, &sysino);
		if (err != H_EOK)
			return (-1);

		KASSERT(sysino == INTVEC(sysino));
		*ihp = sysino;
		DPRINTF(VDB_INTR, ("vpci_intr_map(): sysino 0x%lx\n", sysino));
		return (0);
	}

	return (-1);
}

bus_space_tag_t
vpci_alloc_mem_tag(struct vpci_pbm *vp)
{
	return (vpci_alloc_bus_tag(vp, "mem", PCI_MEMORY_BUS_SPACE));
}

bus_space_tag_t
vpci_alloc_io_tag(struct vpci_pbm *vp)
{
	return (vpci_alloc_bus_tag(vp, "io", PCI_IO_BUS_SPACE));
}

bus_space_tag_t
vpci_alloc_config_tag(struct vpci_pbm *vp)
{
	return (vpci_alloc_bus_tag(vp, "cfg", PCI_CONFIG_BUS_SPACE));
}

bus_space_tag_t
vpci_alloc_bus_tag(struct vpci_pbm *pbm, const char *name, int type)
{
	struct vpci_softc *sc = pbm->vp_sc;
	struct sparc_bus_space_tag *bt;

	bt = kmem_zalloc(sizeof(*bt), KM_SLEEP);

#if 0
	snprintf(bt->name, sizeof(bt->name), "%s-pbm_%s(%d/%2.2x)",
	    device_xname(sc->sc_dev), name, ss, asi);
#endif

	bt->cookie = pbm;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = vpci_bus_map;
	bt->sparc_bus_mmap = vpci_bus_mmap;
	bt->sparc_intr_establish = vpci_intr_establish;
	return (bt);
}

bus_dma_tag_t
vpci_alloc_dma_tag(struct vpci_pbm *pbm)
{
	struct vpci_softc *sc = pbm->vp_sc;
	bus_dma_tag_t dt, pdt = sc->sc_dmat;

	dt = kmem_zalloc(sizeof(*dt), KM_SLEEP);
	dt->_cookie = pbm;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	dt->_dmamap_create	= vpci_dmamap_create;
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load	= iommu_dvmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	dt->_dmamap_load_raw	= iommu_dvmamap_load_raw;
	dt->_dmamap_unload	= iommu_dvmamap_unload;
	dt->_dmamap_sync	= iommu_dvmamap_sync;
	dt->_dmamem_alloc	= iommu_dvmamem_alloc;
	dt->_dmamem_free	= iommu_dvmamem_free;
	dt->_dmamem_map         = iommu_dvmamem_map;
	dt->_dmamem_unmap       = iommu_dvmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

pci_chipset_tag_t
vpci_alloc_chipset(struct vpci_pbm *pbm, int node, pci_chipset_tag_t pc)
{
	pci_chipset_tag_t npc;

	npc = kmem_alloc(sizeof *npc, KM_SLEEP);
	memcpy(npc, pc, sizeof *pc);
	npc->cookie = pbm;
	npc->rootnode = node;
	npc->spc_conf_read = vpci_conf_read;
	npc->spc_conf_write = vpci_conf_write;
	npc->spc_intr_map = vpci_intr_map;
	npc->spc_intr_establish = vpci_pci_intr_establish;
	npc->spc_find_ino = NULL;
	return (npc);
}

int
vpci_dmamap_create(bus_dma_tag_t t, bus_size_t size,
    int nsegments, bus_size_t maxsegsz, bus_size_t boundary, int flags,
    bus_dmamap_t *dmamp)
{
	struct vpci_pbm *pbm = t->_cookie;
	int error;

	error = bus_dmamap_create(t->_parent, size, nsegments, maxsegsz,
				  boundary, flags, dmamp);
	if (error == 0)
		(*dmamp)->_dm_cookie = &pbm->vp_sb;
	return error;
}

int
vpci_bus_map(bus_space_tag_t t, bus_addr_t offset,
    bus_size_t size, int flags, vaddr_t unused, bus_space_handle_t *hp)
{
	struct vpci_pbm *pbm = t->cookie;
	struct vpci_softc *sc = pbm->vp_sc;
	int i, ss;

	DPRINTF(VDB_BUSMAP, ("vpci_bus_map: type %d off %qx sz %qx flags %d",
	    t->type,
	    (unsigned long long)offset,
	    (unsigned long long)size,
	    flags));

	ss = sparc_pci_childspace(t->type);
	DPRINTF(VDB_BUSMAP, (" cspace %d\n", ss));

	if (t->parent == 0 || t->parent->sparc_bus_map == 0) {
		printf("\n_vpci_bus_map: invalid parent");
		return (EINVAL);
	}

	for (i = 0; i < pbm->vp_nrange; i++) {
		bus_addr_t paddr;
		struct vpci_range *pr = &pbm->vp_range[i];

		if (((pr->cspace >> 24) & 0x03) != ss)
			continue;

		paddr = BUS_ADDR(pr->phys_hi, pr->phys_lo + offset);
		return ((*sc->sc_bustag->sparc_bus_map)(t, paddr, size, 
			flags, 0, hp));
	}

	return (EINVAL);
}

paddr_t
vpci_bus_mmap(bus_space_tag_t t, bus_addr_t paddr,
    off_t off, int prot, int flags)
{
	bus_addr_t offset = paddr;
	struct vpci_pbm *pbm = t->cookie;
	struct vpci_softc *sc = pbm->vp_sc;
	int i, ss;

	ss = sparc_pci_childspace(t->type);

	DPRINTF(VDB_BUSMAP, ("vpci_bus_mmap: prot %d flags %d pa %qx\n",
	    prot, flags, (unsigned long long)paddr));

	if (t->parent == 0 || t->parent->sparc_bus_mmap == 0) {
		printf("\n_vpci_bus_mmap: invalid parent");
		return (-1);
	}

	for (i = 0; i < pbm->vp_nrange; i++) {
		struct vpci_range *pr = &pbm->vp_range[i];

		if (((pr->cspace >> 24) & 0x03) != ss)
			continue;

		paddr = BUS_ADDR(pr->phys_hi, pr->phys_lo + offset);
		return (bus_space_mmap(sc->sc_bustag, paddr, off,
				       prot, flags));
	}

	return (-1);
}

void *
vpci_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	struct intrhand *ih = NULL;
	int ino;

	ino = INTINO(ihandle);
	DPRINTF(VDB_INTR, ("%s: ih %lx; level %d ino %#x\n", __func__, (u_long)ihandle, level, ino));

	if (level == IPL_NONE) {
		level = INTLEV(ihandle);
		printf(": IPL_NONE, setting IPL %d.\n", level);
	}
	if (level == IPL_NONE) {
		level = 2;
		printf(": no IPL, setting IPL 2.\n");
	}

	ino |= INTVEC(ihandle);
	DPRINTF(VDB_INTR, ("%s: ih %lx; level %d ino %#x\n", __func__, (u_long)ihandle, level, ino));

	ih = intrhand_alloc();

	ih->ih_ivec = ihandle;
	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino;
	ih->ih_pending = 0;
	ih->ih_ack = vpci_intr_ack;
	intr_establish(ih->ih_pil, level != IPL_VM, ih);

	uint64_t sysino = INTVEC(ihandle);
	DPRINTF(VDB_INTR, ("vpci_intr_establish(): sysino 0x%lx\n", sysino));

	int err;

	err = hv_intr_settarget(sysino, cpus->ci_cpuid);
	if (err != H_EOK)
		printf("hv_intr_settarget(%lu, %u) failed - err = %d\n", 
		       (long unsigned int)sysino, cpus->ci_cpuid, err);

	/* Clear pending interrupts. */
	err = hv_intr_setstate(sysino, INTR_IDLE);
	if (err != H_EOK)
	  printf("hv_intr_setstate(%lu, INTR_IDLE) failed - err = %d\n", 
		(long unsigned int)sysino, err);

	err = hv_intr_setenabled(sysino, INTR_ENABLED);
	if (err != H_EOK)
	  printf("hv_intr_setenabled(%lu) failed - err = %d\n", 
		(long unsigned int)sysino, err);

	DPRINTF(VDB_INTR, ("%s() returning %p\n", __func__, ih));
	return (ih);
}

void
vpci_intr_ack(struct intrhand *ih)
{
	int err;
	err = hv_intr_setstate(ih->ih_number, INTR_IDLE);
	if (err != H_EOK)
	  panic("%s(%u, INTR_IDLE) failed - err = %d\n", 
		__func__, ih->ih_number, err);
}

static void *
vpci_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
	int (*func)(void *), void *arg)
{
	void *cookie;
	struct vpci_pbm *pbm = (struct vpci_pbm *)pc->cookie;

	DPRINTF(VDB_INTR, ("%s: ih %lx; level %d\n", __func__, (u_long)ih, level));
	cookie = bus_intr_establish(pbm->vp_memt, ih, level, func, arg);

	DPRINTF(VDB_INTR, ("%s: returning handle %p\n", __func__, cookie));
	return (cookie);
}
