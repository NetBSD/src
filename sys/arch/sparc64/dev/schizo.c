/*	$NetBSD: schizo.c,v 1.12 2010/01/06 05:55:01 mrg Exp $	*/
/*	$OpenBSD: schizo.c,v 1.55 2008/08/18 20:29:37 brad Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
 * Copyright (c) 2003 Henric Jungheim
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/reboot.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/psl.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/schizoreg.h>
#include <sparc64/dev/schizovar.h>
#include <sparc64/sparc64/cache.h>

#ifdef DEBUG
#define SDB_PROM        0x01
#define SDB_BUSMAP      0x02
#define SDB_INTR        0x04
#define SDB_INTMAP      0x08
#define SDB_CONF        0x10
int schizo_debug = 0x0;
#define DPRINTF(l, s)   do { if (schizo_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

extern struct sparc_pci_chipset _sparc_pci_chipset;

static	int	schizo_match(struct device *, struct cfdata *, void *);
static	void	schizo_attach(struct device *, struct device *, void *);
static	int	schizo_print(void *aux, const char *p);

CFATTACH_DECL(schizo, sizeof(struct schizo_softc),
    schizo_match, schizo_attach, NULL, NULL);

void schizo_init(struct schizo_softc *);
void schizo_init_iommu(struct schizo_softc *, struct schizo_pbm *);

void schizo_set_intr(struct schizo_softc *, struct schizo_pbm *, int,
    int (*handler)(void *), void *, int, const char *);
int schizo_ue(void *);
int schizo_ce(void *);
int schizo_safari_error(void *);
int schizo_pci_error(void *);

pci_chipset_tag_t schizo_alloc_chipset(struct schizo_pbm *, int,
    pci_chipset_tag_t);
bus_space_tag_t schizo_alloc_mem_tag(struct schizo_pbm *);
bus_space_tag_t schizo_alloc_io_tag(struct schizo_pbm *);
bus_space_tag_t schizo_alloc_config_tag(struct schizo_pbm *);
bus_space_tag_t schizo_alloc_bus_tag(struct schizo_pbm *, const char *,
    int);
bus_dma_tag_t schizo_alloc_dma_tag(struct schizo_pbm *);

pcireg_t schizo_conf_read(pci_chipset_tag_t, pcitag_t, int);
void schizo_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

int schizo_bus_map(bus_space_tag_t t, bus_addr_t offset, bus_size_t size,
	           int flags, vaddr_t unused, bus_space_handle_t *hp);
static paddr_t schizo_bus_mmap(bus_space_tag_t t, bus_addr_t paddr,
                               off_t off, int prot, int flags);
static void *schizo_intr_establish(bus_space_tag_t, int, int, int (*)(void *),
	void *, void(*)(void));
static int schizo_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
static void *schizo_pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
                                       int, int (*)(void *), void *);
static int schizo_pci_find_ino(struct pci_attach_args *, pci_intr_handle_t *);
static int schizo_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	bus_size_t, int, bus_dmamap_t *);

int
schizo_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	char *str;

	if (strcmp(ma->ma_name, "pci") != 0)
		return (0);

	str = prom_getpropstring(ma->ma_node, "model");
	if (strcmp(str, "schizo") == 0)
		return (1);

	str = prom_getpropstring(ma->ma_node, "compatible");
	if (strcmp(str, "pci108e,8001") == 0)
		return (1);
	if (strcmp(str, "pci108e,8002") == 0)		/* XMITS */
		return (1);
	if (strcmp(str, "pci108e,a801") == 0)		/* Tomatillo */
		return (1);

	return (0);
}

void
schizo_attach(struct device *parent, struct device *self, void *aux)
{
	struct schizo_softc *sc = (struct schizo_softc *)self;
	struct mainbus_attach_args *ma = aux;
	uint64_t eccctrl;
	char *str;

	printf(": addr %lx", ma->ma_reg[0].ur_paddr);
	str = prom_getpropstring(ma->ma_node, "compatible");
	if (strcmp(str, "pci108e,a801") == 0)
		sc->sc_tomatillo = 1;

	sc->sc_node = ma->ma_node;
	sc->sc_dmat = ma->ma_dmatag;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_ctrl = ma->ma_reg[1].ur_paddr - 0x10000UL;
	sc->sc_reg0 = ma->ma_reg[0];

	if (bus_space_map(sc->sc_bustag, sc->sc_ctrl,
	    sizeof(struct schizo_regs), 0,
	    &sc->sc_ctrlh)) {
		printf(": failed to map registers\n");
		return;
	}

	sc->sc_ign = INTIGN(ma->ma_upaid << INTMAP_IGN_SHIFT);

	/* enable schizo ecc error interrupts */
	eccctrl = schizo_read(sc, SCZ_ECCCTRL);
	eccctrl |= SCZ_ECCCTRL_EE_INTEN |
		   SCZ_ECCCTRL_UE_INTEN |
		   SCZ_ECCCTRL_CE_INTEN;
	schizo_write(sc, SCZ_ECCCTRL, eccctrl);

	schizo_init(sc);
}

void
schizo_init(struct schizo_softc *sc)
{
	struct schizo_pbm *pbm;
	struct pcibus_attach_args pba;
	int *busranges = NULL, nranges;
	u_int64_t /*match,*/ reg;

	pbm = malloc(sizeof(*pbm), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (pbm == NULL)
		panic("schizo: can't alloc schizo pbm");

	pbm->sp_sc = sc;
	pbm->sp_regt = sc->sc_bustag;

	if ((sc->sc_reg0.ur_paddr & 0x00700000) == 0x00600000)
		pbm->sp_bus_a = 1;
	else
		pbm->sp_bus_a = 0;

	if (prom_getprop(sc->sc_node, "ranges", sizeof(struct schizo_range),
	    &pbm->sp_nrange, (void **)&pbm->sp_range))
		panic("schizo: can't get ranges");

	if (prom_getprop(sc->sc_node, "bus-range", sizeof(int), &nranges,
	    (void **)&busranges))
		panic("schizo: can't get bus-range");

	printf(": \"%s\", version %d, ign %x, bus %c %d to %d\n",
	    sc->sc_tomatillo ? "Tomatillo" : "Schizo",
	    prom_getpropint(sc->sc_node, "version#", 0), sc->sc_ign,
	    pbm->sp_bus_a ? 'A' : 'B', busranges[0], busranges[1]);

	if (bus_space_subregion(pbm->sp_regt, sc->sc_ctrlh,
	    pbm->sp_bus_a ? offsetof(struct schizo_regs, pbm_a) :
	    offsetof(struct schizo_regs, pbm_b),
	    sizeof(struct schizo_pbm_regs),
	    &pbm->sp_regh)) {
		panic("schizo: unable to create PBM handle");
	}

	printf("%s: ", sc->sc_dv.dv_xname);
	schizo_init_iommu(sc, pbm);

	pbm->sp_memt = schizo_alloc_mem_tag(pbm);
	pbm->sp_iot = schizo_alloc_io_tag(pbm);
	pbm->sp_cfgt = schizo_alloc_config_tag(pbm);
	pbm->sp_dmat = schizo_alloc_dma_tag(pbm);
	pbm->sp_flags = (pbm->sp_memt ? PCI_FLAGS_MEM_ENABLED : 0) |
		        (pbm->sp_iot ? PCI_FLAGS_IO_ENABLED : 0);

	if (bus_space_map(pbm->sp_cfgt, 0, 0x1000000, 0, &pbm->sp_cfgh))
		panic("schizo: could not map config space");

	pbm->sp_pc = schizo_alloc_chipset(pbm, sc->sc_node,
	    &_sparc_pci_chipset);
	pbm->sp_pc->spc_busmax = busranges[1];
	pbm->sp_pc->spc_busnode = malloc(sizeof(*pbm->sp_pc->spc_busnode),
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (pbm->sp_pc->spc_busnode == NULL)
		panic("schizo: malloc busnode");

	pba.pba_bus = busranges[0];
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pbm->sp_pc;
	pba.pba_flags = pbm->sp_flags;
	pba.pba_dmat = pbm->sp_dmat;
	pba.pba_dmat64 = NULL;	/* XXX */
	pba.pba_memt = pbm->sp_memt;
	pba.pba_iot = pbm->sp_iot;

	free(busranges, M_DEVBUF);

	schizo_pbm_write(pbm, SCZ_PCI_INTR_RETRY, 5);

	/* clear out the bus errors */
	schizo_pbm_write(pbm, SCZ_PCI_CTRL, schizo_pbm_read(pbm, SCZ_PCI_CTRL));
	schizo_pbm_write(pbm, SCZ_PCI_AFSR, schizo_pbm_read(pbm, SCZ_PCI_AFSR));
	schizo_cfg_write(pbm, PCI_COMMAND_STATUS_REG,
	    schizo_cfg_read(pbm, PCI_COMMAND_STATUS_REG));

	reg = schizo_pbm_read(pbm, SCZ_PCI_CTRL);
	/* enable/disable error interrupts and arbiter */
	reg |= SCZ_PCICTRL_EEN | SCZ_PCICTRL_MMU_INT | SCZ_PCICTRL_ARB;
	reg &= ~SCZ_PCICTRL_SBH_INT;
	schizo_pbm_write(pbm, SCZ_PCI_CTRL, reg);

	reg = schizo_pbm_read(pbm, SCZ_PCI_DIAG);
	reg &= ~(SCZ_PCIDIAG_D_RTRYARB | SCZ_PCIDIAG_D_RETRY |
	    SCZ_PCIDIAG_D_INTSYNC);
	schizo_pbm_write(pbm, SCZ_PCI_DIAG, reg);

	if (pbm->sp_bus_a)
		schizo_set_intr(sc, pbm, PIL_HIGH, schizo_pci_error,
		   pbm, SCZ_PCIERR_A_INO, "pci_a");
	else
		schizo_set_intr(sc, pbm, PIL_HIGH, schizo_pci_error,
		   pbm, SCZ_PCIERR_B_INO, "pci_b");

	/* double mapped */
	schizo_set_intr(sc, pbm, PIL_HIGH, schizo_ue, sc, SCZ_UE_INO,
	    "ue");
	schizo_set_intr(sc, pbm, PIL_HIGH, schizo_ce, sc, SCZ_CE_INO,
	    "ce");
	schizo_set_intr(sc, pbm, PIL_HIGH, schizo_safari_error, sc,
	    SCZ_SERR_INO, "safari");

	config_found(&sc->sc_dv, &pba, schizo_print);
}

int
schizo_ue(void *vsc)
{
	struct schizo_softc *sc = vsc;

	panic("%s: uncorrectable error", sc->sc_dv.dv_xname);
	return (1);
}

int
schizo_ce(void *vsc)
{
	struct schizo_softc *sc = vsc;

	panic("%s: correctable error", sc->sc_dv.dv_xname);
	return (1);
}

int
schizo_pci_error(void *vpbm)
{
	struct schizo_pbm *sp = vpbm;
	struct schizo_softc *sc = sp->sp_sc;
	u_int64_t afsr, afar, ctrl, tfar;
	u_int32_t csr;
	char bits[128];

	afsr = schizo_pbm_read(sp, SCZ_PCI_AFSR);
	afar = schizo_pbm_read(sp, SCZ_PCI_AFAR);
	ctrl = schizo_pbm_read(sp, SCZ_PCI_CTRL);
	csr = schizo_cfg_read(sp, PCI_COMMAND_STATUS_REG);

	printf("%s: pci bus %c error\n", sc->sc_dv.dv_xname,
	    sp->sp_bus_a ? 'A' : 'B');

	snprintb(bits, sizeof(bits), SCZ_PCIAFSR_BITS, afsr);
	printf("PCIAFSR=%s\n", bits);
	printf("PCIAFAR=%lx\n", afar);
	snprintb(bits, sizeof(bits), SCZ_PCICTRL_BITS, ctrl);
	printf("PCICTRL=%s\n", bits);
#ifdef PCI_COMMAND_STATUS_BITS
	snprintb(bits, sizeof(bits), PCI_COMMAND_STATUS_BITS, csr);
	printf("PCICSR=%s\n", bits);
#endif

	if (ctrl & SCZ_PCICTRL_MMU_ERR) {
		ctrl = schizo_pbm_read(sp, SCZ_PCI_IOMMU_CTRL);
		printf("IOMMUCTRL=%lx\n", ctrl);

		if ((ctrl & TOM_IOMMU_ERR) == 0)
			goto clear_error;

		if (sc->sc_tomatillo) {
			tfar = schizo_pbm_read(sp, TOM_PCI_IOMMU_TFAR);
			printf("IOMMUTFAR=%lx\n", tfar);
		}

		/* These are non-fatal if target abort was signalled. */
		if ((ctrl & TOM_IOMMU_ERR_MASK) == TOM_IOMMU_INV_ERR ||
		    ctrl & TOM_IOMMU_ILLTSBTBW_ERR ||
		    ctrl & TOM_IOMMU_BADVA_ERR) {
			if (csr & PCI_STATUS_TARGET_TARGET_ABORT) {
				schizo_pbm_write(sp, SCZ_PCI_IOMMU_CTRL, ctrl);
				goto clear_error;
			}
		}
	}

	panic("%s: fatal", sc->sc_dv.dv_xname);

 clear_error:
	schizo_cfg_write(sp, PCI_COMMAND_STATUS_REG, csr);
	schizo_pbm_write(sp, SCZ_PCI_CTRL, ctrl);
	schizo_pbm_write(sp, SCZ_PCI_AFSR, afsr);
	return (1);
}

int
schizo_safari_error(void *vsc)
{
	struct schizo_softc *sc = vsc;

	printf("%s: safari error\n", sc->sc_dv.dv_xname);

	printf("ERRLOG=%lx\n", schizo_read(sc, SCZ_SAFARI_ERRLOG));
	printf("UE_AFSR=%lx\n", schizo_read(sc, SCZ_UE_AFSR));
	printf("UE_AFAR=%lx\n", schizo_read(sc, SCZ_UE_AFAR));
	printf("CE_AFSR=%lx\n", schizo_read(sc, SCZ_CE_AFSR));
	printf("CE_AFAR=%lx\n", schizo_read(sc, SCZ_CE_AFAR));

	panic("%s: fatal", sc->sc_dv.dv_xname);
	return (1);
}

void
schizo_init_iommu(struct schizo_softc *sc, struct schizo_pbm *pbm)
{
	struct iommu_state *is = &pbm->sp_is;
	int *vdma = NULL, nitem, tsbsize = 7;
	u_int32_t iobase = -1;
	vaddr_t va;
	char *name;

	va = (vaddr_t)pbm->sp_flush[0x40];

	/* punch in our copies */
	is->is_bustag = pbm->sp_regt;
	if (bus_space_subregion(is->is_bustag, pbm->sp_regh,
	    offsetof(struct schizo_pbm_regs, iommu),
	    sizeof(struct schizo_iommureg), &is->is_iommu)) {
		printf("schizo: unable to create streaming buffer handle\n");
		is->is_sb[0]->sb_flush = NULL;
	} 

	/* initialize our strbuf_ctl */
	is->is_sb[0] = &pbm->sp_sb;
	pbm->sp_sb.sb_is = is;
	is->is_sb[0]->sb_flush = (void *)(va & ~0x3f);

	if (bus_space_subregion(is->is_bustag, pbm->sp_regh,
	    offsetof(struct schizo_pbm_regs, strbuf),
	    sizeof(struct iommu_strbuf), &is->is_sb[0]->sb_sb)) {
	} 

	name = (char *)malloc(32, M_DEVBUF, M_NOWAIT);
	if (name == NULL)
		panic("couldn't malloc iommu name");
	snprintf(name, 32, "%s dvma", sc->sc_dv.dv_xname);

	/*
	 * Separate the men from the boys.  If the `virtual-dma'
	 * property exists, use it.
	 */
	if (!prom_getprop(sc->sc_node, "virtual-dma", sizeof(vdma), &nitem, 
	    (void **)&vdma)) {
		/* Damn.  Gotta use these values. */
		iobase = vdma[0];
#define	TSBCASE(x)	case 1 << ((x) + 23): tsbsize = (x); break
		switch (vdma[1]) { 
			TSBCASE(1); TSBCASE(2); TSBCASE(3);
			TSBCASE(4); TSBCASE(5); TSBCASE(6);
		default: 
			printf("bogus tsb size %x, using 7\n", vdma[1]);
			TSBCASE(7);
		}
#undef TSBCASE
		DPRINTF(SDB_BUSMAP, ("schizo_init_iommu: iobase=0x%x\n", iobase));
		free(vdma, M_DEVBUF);
	} else {
		DPRINTF(SDB_BUSMAP, ("schizo_init_iommu: getprop failed, "
		    "using iobase=0x%x, tsbsize=%d\n", iobase, tsbsize));
	}

	iommu_init(name, is, tsbsize, iobase);
}

int
schizo_print(void *aux, const char *p)
{

	if (p == NULL)
		return (UNCONF);
	return (QUIET);
}

pcireg_t
schizo_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct schizo_pbm *sp = pc->cookie;
	pcireg_t val = (pcireg_t)~0;

	DPRINTF(SDB_CONF, ("%s: tag %lx reg %x ", __func__, (long)tag, reg));
	if (PCITAG_NODE(tag) != -1)
		val = bus_space_read_4(sp->sp_cfgt, sp->sp_cfgh,
		    PCITAG_OFFSET(tag) + reg);
	DPRINTF(SDB_CONF, (" returning %08x\n", (u_int)val));
	return (val);
}

void
schizo_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct schizo_pbm *sp = pc->cookie;

	DPRINTF(SDB_CONF, ("%s: tag %lx; reg %x; data %x", __func__,
		(long)tag, reg, (int)data));

	/* If we don't know it, just punt it.  */
	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(SDB_CONF, (" .. bad addr\n"));
		return;
	}

        bus_space_write_4(sp->sp_cfgt, sp->sp_cfgh,
	    PCITAG_OFFSET(tag) + reg, data);
	DPRINTF(SDB_CONF, (" .. done\n"));
}

void
schizo_set_intr(struct schizo_softc *sc, struct schizo_pbm *pbm, int ipl,
    int (*handler)(void *), void *arg, int ino, const char *what)
{
	struct intrhand *ih;
	u_int64_t mapoff, clroff;

	DPRINTF(SDB_INTR, ("%s: ino %x ign %x fn %p arg %p", __func__,
	    ino, sc->sc_ign, handler, arg));

	mapoff = offsetof(struct schizo_pbm_regs, imap[ino]);
	clroff = offsetof(struct schizo_pbm_regs, iclr[ino]);
	ino |= sc->sc_ign;

	DPRINTF(SDB_INTR, (" mapoff %lx clroff %lx\n", mapoff, clroff));

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return;
	ih->ih_arg = arg;
	ih->ih_map = (uint64_t *)((char *)sc->sc_reg0.ur_paddr + mapoff);
	ih->ih_clr = (uint64_t *)((char *)sc->sc_reg0.ur_paddr + clroff);
	ih->ih_fun = handler;
	ih->ih_pil = (1<<ipl);
	ih->ih_number = INTVEC(schizo_pbm_read(pbm, mapoff));
	intr_establish(ipl, ipl != IPL_VM, ih);

	schizo_pbm_write(pbm, mapoff,
	    ih->ih_number | INTMAP_V | (CPU_UPAID << INTMAP_TID_SHIFT));
}

bus_space_tag_t
schizo_alloc_mem_tag(struct schizo_pbm *sp)
{
	return (schizo_alloc_bus_tag(sp, "mem",
	    PCI_MEMORY_BUS_SPACE));
}

bus_space_tag_t
schizo_alloc_io_tag(struct schizo_pbm *sp)
{
	return (schizo_alloc_bus_tag(sp, "io",
	    PCI_IO_BUS_SPACE));
}

bus_space_tag_t
schizo_alloc_config_tag(struct schizo_pbm *sp)
{
	return (schizo_alloc_bus_tag(sp, "cfg",
	    PCI_CONFIG_BUS_SPACE));
}

bus_space_tag_t
schizo_alloc_bus_tag(struct schizo_pbm *pbm, const char *name, int type)
{
	struct schizo_softc *sc = pbm->sp_sc;
	bus_space_tag_t bt;

	bt = (bus_space_tag_t) malloc(sizeof(struct sparc_bus_space_tag),
		    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (bt == NULL)
		panic("schizo: could not allocate bus tag");

	bt->cookie = pbm;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = schizo_bus_map;
	bt->sparc_bus_mmap = schizo_bus_mmap;
	bt->sparc_intr_establish = schizo_intr_establish;
	return (bt);
}

bus_dma_tag_t
schizo_alloc_dma_tag(struct schizo_pbm *pbm)
{
	struct schizo_softc *sc = pbm->sp_sc;
	bus_dma_tag_t dt, pdt = sc->sc_dmat;

	dt = malloc(sizeof(*dt), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (dt == NULL)
		panic("schizo: could not alloc dma tag");

	dt->_cookie = pbm;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	dt->_dmamap_create = schizo_dmamap_create;
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load = iommu_dvmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	dt->_dmamap_load_raw = iommu_dvmamap_load_raw;
	dt->_dmamap_unload = iommu_dvmamap_unload;
	dt->_dmamap_sync = iommu_dvmamap_sync;
	dt->_dmamem_alloc = iommu_dvmamem_alloc;
	dt->_dmamem_free = iommu_dvmamem_free;
	dt->_dmamem_map = iommu_dvmamem_map;
	dt->_dmamem_unmap = iommu_dvmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

pci_chipset_tag_t
schizo_alloc_chipset(struct schizo_pbm *pbm, int node, pci_chipset_tag_t pc)
{
	pci_chipset_tag_t npc;

	npc = malloc(sizeof *npc, M_DEVBUF, M_NOWAIT);
	if (npc == NULL)
		panic("schizo: could not allocate pci_chipset_tag_t");
	memcpy(npc, pc, sizeof *pc);
	npc->cookie = pbm;
	npc->rootnode = node;
	npc->spc_conf_read = schizo_conf_read;
	npc->spc_conf_write = schizo_conf_write;
	npc->spc_intr_map = schizo_pci_intr_map;
	npc->spc_intr_establish = schizo_pci_intr_establish;
	npc->spc_find_ino = schizo_pci_find_ino;
	return (npc);
}

int
schizo_dmamap_create(bus_dma_tag_t t, bus_size_t size,
    int nsegments, bus_size_t maxsegsz, bus_size_t boundary, int flags,
    bus_dmamap_t *dmamp)
{
	struct schizo_pbm *pbm = t->_cookie;
	int error;

	error = bus_dmamap_create(t->_parent, size, nsegments, maxsegsz,
				  boundary, flags, dmamp);
	if (error == 0)
		(*dmamp)->_dm_cookie = &pbm->sp_sb;
	return error;
}

static struct schizo_range *
get_schizorange(struct schizo_pbm *pbm, int ss)
{
	int i;

	for (i = 0; i < pbm->sp_nrange; i++) {
		if (((pbm->sp_range[i].cspace >> 24) & 0x03) == ss)
			return (&pbm->sp_range[i]);
	}
	/* not found */
	return (NULL);
}

int
schizo_bus_map(bus_space_tag_t t, bus_addr_t offset, bus_size_t size,
	       int flags, vaddr_t unused, bus_space_handle_t *hp)
{
	bus_addr_t paddr;
	struct schizo_pbm *pbm = t->cookie;
	struct schizo_softc *sc = pbm->sp_sc;
	struct schizo_range *sr;
	int ss;

	DPRINTF(SDB_BUSMAP, ("schizo_bus_map: type %d off %qx sz %qx flags %d",
	    t->type,
	    (unsigned long long)offset,
	    (unsigned long long)size,
	    flags));

	ss = sparc_pci_childspace(t->type);
	DPRINTF(SDB_BUSMAP, (" cspace %d\n", ss));

	sr = get_schizorange(pbm, ss);
	if (sr != NULL) {
		paddr = BUS_ADDR(sr->phys_hi, sr->phys_lo + offset);
		DPRINTF(SDB_BUSMAP, ("%s: mapping paddr "
				     "space %lx offset %lx paddr %qx\n",
			       __func__, (long)ss, (long)offset,
			       (unsigned long long)paddr));
		return ((*sc->sc_bustag->sparc_bus_map)(t, paddr, size, 
			flags, 0, hp));
	}
	DPRINTF(SDB_BUSMAP, ("%s: FAILED\n", __func__));
	return (EINVAL);
}

static paddr_t
schizo_bus_mmap(bus_space_tag_t t, bus_addr_t paddr, off_t off, int prot,
	int flags)
{
	bus_addr_t offset = paddr;
	struct schizo_pbm *pbm = t->cookie;
	struct schizo_softc *sc = pbm->sp_sc;
	struct schizo_range *sr;
	int ss;

	ss = sparc_pci_childspace(t->type);

	DPRINTF(SDB_BUSMAP, ("schizo_bus_mmap: prot %d flags %d pa %qx\n",
	    prot, flags, (unsigned long long)paddr));

	sr = get_schizorange(pbm, ss);
	if (sr != NULL) {
		paddr = BUS_ADDR(sr->phys_hi, sr->phys_lo + offset);
		DPRINTF(SDB_BUSMAP, ("%s: mapping paddr "
				     "space %lx offset %lx paddr %qx\n",
			       __func__, (long)ss, (long)offset,
			       (unsigned long long)paddr));
		return (bus_space_mmap(sc->sc_bustag, paddr, off,
				       prot, flags));
	}
	DPRINTF(SDB_BUSMAP, ("%s: FAILED\n", __func__));
	return (-1);
}

/*
 * Set the IGN for this schizo into the handle.
 */
int
schizo_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct schizo_pbm *pbm = pa->pa_pc->cookie;
	struct schizo_softc *sc = pbm->sp_sc;

	*ihp |= sc->sc_ign;
	DPRINTF(SDB_INTMAP, ("returning IGN adjusted to %x\n", *ihp));
	return (0);
}

static void *
schizo_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	struct schizo_pbm *pbm = t->cookie;
	struct schizo_softc *sc = pbm->sp_sc;
	struct intrhand *ih = NULL;
	uint64_t mapoff, clroff;
	volatile uint64_t *intrmapptr = NULL, *intrclrptr = NULL;
	int ino;
	long vec;

	vec = INTVEC(ihandle);
	ino = INTINO(vec);

	ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	DPRINTF(SDB_INTR, ("\n%s: ihandle %d level %d fn %p arg %p\n", __func__,
	    ihandle, level, handler, arg));

	if (level == IPL_NONE)
		level = INTLEV(vec);
	if (level == IPL_NONE) {
		printf(": no IPL, setting IPL 2.\n");
		level = 2;
	}

	mapoff = offsetof(struct schizo_pbm_regs, imap[ino]);
	clroff = offsetof(struct schizo_pbm_regs, iclr[ino]);

	DPRINTF(SDB_INTR, ("%s: intr %x: %p mapoff %lx clroff %lx\n",
	    __func__, ino, intrlev[ino], mapoff, clroff));

	intrmapptr = (uint64_t *)((char *)sc->sc_reg0.ur_paddr + mapoff);
	intrclrptr = (uint64_t *)((char *)sc->sc_reg0.ur_paddr + clroff);

	if (INTIGN(vec) == 0)
		ino |= schizo_pbm_read(pbm, mapoff) & INTMAP_IGN;
	else
		ino |= vec & INTMAP_IGN;

	/* Register the map and clear intr registers */
	ih->ih_map = intrmapptr;
	ih->ih_clr = intrclrptr;

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino;

	DPRINTF(SDB_INTR, (
	    "; installing handler %p arg %p with inr %x pil %u\n",
	    handler, arg, ino, (u_int)ih->ih_pil));

	intr_establish(ih->ih_pil, level != IPL_VM, ih);

	/*
	 * Enable the interrupt now we have the handler installed.
	 * Read the current value as we can't change it besides the
	 * valid bit so so make sure only this bit is changed.
	 */
	if (intrmapptr) {
		u_int64_t imap;

		imap = schizo_pbm_read(pbm, mapoff);
		DPRINTF(SDB_INTR, ("; read intrmap = %016qx",
			(unsigned long long)imap));
		imap |= INTMAP_V;
		DPRINTF(SDB_INTR, ("; addr of intrmapptr = %p", intrmapptr));
		DPRINTF(SDB_INTR, ("; writing intrmap = %016qx\n",
			(unsigned long long)imap));
		schizo_pbm_write(pbm, mapoff, imap);
		imap = schizo_pbm_read(pbm, mapoff);
		DPRINTF(SDB_INTR, ("; reread intrmap = %016qx",
			(unsigned long long)imap));
		ih->ih_number |= imap & INTMAP_INR;
	}
 	if (intrclrptr) {
 		/* set state to IDLE */
		schizo_pbm_write(pbm, clroff, 0);
 	}

	return (ih);
}

static void *
schizo_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
	int (*func)(void *), void *arg)
{
	void *cookie;
	struct schizo_pbm *pbm = (struct schizo_pbm *)pc->cookie;

	DPRINTF(SDB_INTR, ("%s: ih %lx; level %d", __func__, (u_long)ih, level));
	cookie = bus_intr_establish(pbm->sp_memt, ih, level, func, arg);

	DPRINTF(SDB_INTR, ("; returning handle %p\n", cookie));
	return (cookie);
}

static int
schizo_pci_find_ino(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
#if 0
	struct schizo_pbm *pbm = pa->pa_pc->cookie;
	struct schizo_softc *sc = pbm->sp_sc;
	u_int bus;
	u_int dev;
	u_int pin;
#endif

	DPRINTF(SDB_INTMAP, ("pci_find_ino: pa_tag: node %x, %d:%d:%d\n",
			      PCITAG_NODE(pa->pa_tag), (int)PCITAG_BUS(pa->pa_tag),
			      (int)PCITAG_DEV(pa->pa_tag),
			      (int)PCITAG_FUN(pa->pa_tag)));
	DPRINTF(SDB_INTMAP,
		("pci_find_ino: intrswiz %d, intrpin %d, intrline %d, rawintrpin %d\n",
		 pa->pa_intrswiz, pa->pa_intrpin, pa->pa_intrline, pa->pa_rawintrpin));
	DPRINTF(SDB_INTMAP, ("pci_find_ino: pa_intrtag: node %x, %d:%d:%d\n",
			      PCITAG_NODE(pa->pa_intrtag),
			      (int)PCITAG_BUS(pa->pa_intrtag),
			      (int)PCITAG_DEV(pa->pa_intrtag),
			      (int)PCITAG_FUN(pa->pa_intrtag)));

#if 0
	bus = (pp->pp_id == PSYCHO_PBM_B);
	/*
	 * If we are on a ppb, use the devno on the underlying bus when forming
	 * the ivec.
	 */
	if (pa->pa_intrswiz != 0 && PCITAG_NODE(pa->pa_intrtag) != 0) 
		dev = PCITAG_DEV(pa->pa_intrtag);
	else
		dev = pa->pa_device;
	dev--;

	if (sc->sc_mode == PSYCHO_MODE_PSYCHO &&
	    pp->pp_id == PSYCHO_PBM_B)
		dev--;

	pin = pa->pa_intrpin - 1;
	DPRINTF(SDB_INTMAP, ("pci_find_ino: mode %d, pbm %d, dev %d, pin %d\n",
	    sc->sc_mode, pp->pp_id, dev, pin));

	*ihp = sc->sc_ign | ((bus << 4) & INTMAP_PCIBUS) |
	    ((dev << 2) & INTMAP_PCISLOT) | (pin & INTMAP_PCIINT);
#endif

	return (0);
}
