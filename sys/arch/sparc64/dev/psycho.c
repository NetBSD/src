/*	$NetBSD: psycho.c,v 1.2.2.6 2001/03/27 15:31:34 bouyer Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_ddb.h"

/*
 * PCI support for UltraSPARC `psycho'
 */

#undef DEBUG
#define DEBUG

#ifdef DEBUG
#define PDB_PROM	0x01
#define PDB_IOMMU	0x02
#define PDB_BUSMAP	0x04
#define PDB_BUSDMA	0x08
#define PDB_INTR	0x10
int psycho_debug = 0x0;
#define DPRINTF(l, s)   do { if (psycho_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/time.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/psl.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>
#include <sparc64/sparc64/cache.h>

#include "ioconf.h"

static pci_chipset_tag_t psycho_alloc_chipset __P((struct psycho_pbm *, int,
						   pci_chipset_tag_t));
static void psycho_get_bus_range __P((int, int *));
static void psycho_get_ranges __P((int, struct psycho_ranges **, int *));
static void psycho_get_registers __P((int, struct psycho_registers **, int *));
static void psycho_get_intmap __P((int, struct psycho_interrupt_map **, int *));
static void psycho_get_intmapmask __P((int, struct psycho_interrupt_map_mask *));

/* IOMMU support */
static void psycho_iommu_init __P((struct psycho_softc *, int));

/*
 * bus space and bus dma support for UltraSPARC `psycho'.  note that most
 * of the bus dma support is provided by the iommu dvma controller.
 */
static int psycho_bus_mmap __P((bus_space_tag_t, bus_type_t, bus_addr_t,
				int, bus_space_handle_t *));
static int _psycho_bus_map __P((bus_space_tag_t, bus_type_t, bus_addr_t,
				bus_size_t, int, vaddr_t,
				bus_space_handle_t *));
static void *psycho_intr_establish __P((bus_space_tag_t, int, int, int,
				int (*) __P((void *)), void *));

static int psycho_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
				   bus_size_t, struct proc *, int));
static void psycho_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static int psycho_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int));
static void psycho_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
				    bus_size_t, int));
int psycho_dmamem_alloc __P((bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t,
			     bus_dma_segment_t *, int, int *, int));
void psycho_dmamem_free __P((bus_dma_tag_t, bus_dma_segment_t *, int));
int psycho_dmamem_map __P((bus_dma_tag_t, bus_dma_segment_t *, int, size_t,
			   caddr_t *, int));
void psycho_dmamem_unmap __P((bus_dma_tag_t, caddr_t, size_t));

/* base pci_chipset */
extern struct sparc_pci_chipset _sparc_pci_chipset;

/*
 * autoconfiguration
 */
static	int	psycho_match __P((struct device *, struct cfdata *, void *));
static	void	psycho_attach __P((struct device *, struct device *, void *));
static	int	psycho_print __P((void *aux, const char *p));

static	void	sabre_init __P((struct psycho_softc *,
				struct mainbus_attach_args *,
				struct pcibus_attach_args *));
static	void	psycho_init __P((struct psycho_softc *,
				struct mainbus_attach_args *,
				struct pcibus_attach_args *));

struct cfattach psycho_ca = {
        sizeof(struct psycho_softc), psycho_match, psycho_attach
};

/*
 * "sabre" is the UltraSPARC IIi onboard PCI interface, normally connected to
 * an APB (advanced PCI bridge), which was designed specifically for the IIi.
 * the APB appears as two "simba"'s underneath the sabre.  real devices
 * typically appear on the "simba"'s only.
 *
 * a pair of "psycho"s sit on the mainbus and have real devices attached to
 * them.  they implemented in the U2P (UPA to PCI).  these two devices share
 * register space and as such need to be configured together, even though the
 * autoconfiguration will attach them separately.
 *
 * each of these appears as two usable PCI busses, though the sabre itself
 * takes pci0 in this case, leaving real devices on pci1 and pci2.  there can
 * be multiple pairs of psycho's, however, in multi-board machines.
 */
#define	ROM_PCI_NAME		"pci"
#define ROM_SABRE_MODEL		"SUNW,sabre"
#define ROM_SIMBA_MODEL		"SUNW,simba"
#define ROM_PSYCHO_MODEL	"SUNW,psycho"

static	int
psycho_match(parent, match, aux)
	struct device	*parent;
	struct cfdata	*match;
	void		*aux;
{
	struct mainbus_attach_args *ma = aux;
	char *model = getpropstring(ma->ma_node, "model");

	/* match on a name of "pci" and a sabre or a psycho */
	if (strcmp(ma->ma_name, ROM_PCI_NAME) == 0 &&
	    (strcmp(model, ROM_SABRE_MODEL) == 0 ||
	     strcmp(model, ROM_PSYCHO_MODEL) == 0))
		return (1);

	return (0);
}

static	void
psycho_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct psycho_softc *sc = (struct psycho_softc *)self;
	struct pcibus_attach_args pba;
	struct mainbus_attach_args *ma = aux;
	char *model = getpropstring(ma->ma_node, "model");

	printf("\n");

	sc->sc_node = ma->ma_node;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	/*
	 * pull in all the information about the psycho as we can.
	 */

	/*
	 * call the model-specific initialisation routine.
	 */
	if (strcmp(model, ROM_SABRE_MODEL) == 0)
		sabre_init(sc, ma, &pba);
	else if (strcmp(model, ROM_PSYCHO_MODEL) == 0)
		psycho_init(sc, ma, &pba);
	else
		panic("psycho_attach: unknown model %s?", model);

	/*
	 * attach the pci.. note we pass PCI A tags, etc., for the sabre here.
	 */
	pba.pba_busname = "pci";
	pba.pba_flags = sc->sc_psycho_this->pp_flags;
	pba.pba_dmat = sc->sc_psycho_this->pp_dmat;
	pba.pba_iot = sc->sc_psycho_this->pp_iot;
	pba.pba_memt = sc->sc_psycho_this->pp_memt;

	config_found(self, &pba, psycho_print);
}

static	int
psycho_print(aux, p)
	void *aux;
	const char *p;
{

	if (p == NULL)
		return (UNCONF);
	return (QUIET);
}

/*
 * SUNW,sabre initialisation ..
 *	- get the sabre's ranges.  this are used for both simba's.
 *	- find the two SUNW,simba's underneath (a and b)
 *	- work out which simba is which via the bus-range property
 *	- get each simba's interrupt-map and interrupt-map-mask.
 *	- turn on the iommu
 */
static void
sabre_init(sc, ma, pba)
	struct psycho_softc *sc;
	struct mainbus_attach_args *ma;
	struct pcibus_attach_args *pba;
{
	struct psycho_pbm *pp;
	bus_space_handle_t bh;
	u_int64_t csr;
	unsigned int node;
	int sabre_br[2], simba_br[2];

	/*
	 * The sabre gets two register banks:
	 * (0) per-PBM PCI configuration space, containing only the
	 *     PBM 256-byte PCI header
	 * (1) the shared psycho configuration registers (struct psychoreg)
	 */
	sc->sc_regs = (struct psychoreg *)(u_long)ma->ma_address[0];
	sc->sc_basepaddr = (paddr_t)ma->ma_reg[0].ur_paddr;
	sc->sc_ign = 0x7c0; /* APB IGN is always 0x7c */

	csr = sc->sc_regs->psy_csr;
	/* csr = bus_space_read_8(sc->sc_bustag, (bus_space_handle_t)(u_long)
		&sc->sc_regs->psy_pcictl[0].pci_csr, 0); */

	/* who? said a voice, incredulous */
	sc->sc_mode = PSYCHO_MODE_SABRE;
	printf("sabre: ign %x ", sc->sc_ign);

	/* setup the PCI control register; there is only one for the sabre */
	csr |= PCICTL_MRLM |
	       PCICTL_ARB_PARK |
	       PCICTL_ERRINTEN |
	       PCICTL_4ENABLE;
	csr &= ~(PCICTL_SERR |
		 PCICTL_CPU_PRIO |
		 PCICTL_ARB_PRIO |
		 PCICTL_RTRYWAIT);
	bus_space_write_8(sc->sc_bustag,
	    (bus_space_handle_t)(u_long)&sc->sc_regs->psy_pcictl[0].pci_csr,
	    0, csr);

	/* allocate a pair of psycho_pbm's for our simba's */
	sc->sc_sabre = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	sc->sc_simba_a = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	sc->sc_simba_b = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	if (sc->sc_sabre == NULL || sc->sc_simba_a == NULL ||
	    sc->sc_simba_b == NULL)
		panic("could not allocate simba pbm's");

	memset(sc->sc_sabre, 0, sizeof *pp);
	memset(sc->sc_simba_a, 0, sizeof *pp);
	memset(sc->sc_simba_b, 0, sizeof *pp);

	/* grab the sabre ranges; use them for both simba's */
	psycho_get_ranges(sc->sc_node, &sc->sc_sabre->pp_range,
	    &sc->sc_sabre->pp_nrange);
	sc->sc_simba_b->pp_range = sc->sc_simba_a->pp_range =
	    sc->sc_sabre->pp_range;
	sc->sc_simba_b->pp_nrange = sc->sc_simba_a->pp_nrange =
	    sc->sc_sabre->pp_nrange;

	/* get the bus-range for the sabre.  we expect 0..2 */
	psycho_get_bus_range(sc->sc_node, sabre_br);

	pba->pba_bus = sabre_br[0];

	printf("bus range %u to %u", sabre_br[0], sabre_br[1]);

	for (node = firstchild(sc->sc_node); node; node = nextsibling(node)) {
		char *name = getpropstring(node, "name");
		char *model, who;
		struct psycho_registers *regs = NULL;
		int nregs, fn;

		if (strcmp(name, ROM_PCI_NAME) != 0)
			continue;

		model = getpropstring(node, "model");
		if (strcmp(model, ROM_SIMBA_MODEL) != 0)
			continue;

		psycho_get_bus_range(node, simba_br);
		psycho_get_registers(node, &regs, &nregs);

		fn = TAG2FN(regs->phys_hi);
		switch (fn) {
		case 0:
			pp = sc->sc_simba_a;
			who = 'a';
			pp->pp_regs = regs;
			pp->pp_nregs = nregs;
			break;
		case 1:
			pp = sc->sc_simba_b;
			who = 'b';
			pp->pp_regs = regs;
			pp->pp_nregs = nregs;
			break;
		default:
			panic("illegal simba funcion %d\n", fn);
		}
		pp->pp_pcictl = &sc->sc_regs->psy_pcictl[0];
		/* link us in .. */
		pp->pp_sc = sc;
		
		printf("; simba %c, PCI bus %d", who, simba_br[0]);

		/* grab the simba registers, interrupt map and map mask */
		psycho_get_intmap(node, &pp->pp_intmap, &pp->pp_nintmap);
		psycho_get_intmapmask(node, &pp->pp_intmapmask);

		/* allocate our tags */
		pp->pp_memt = psycho_alloc_mem_tag(pp);
		pp->pp_iot = psycho_alloc_io_tag(pp);
		pp->pp_dmat = psycho_alloc_dma_tag(pp);
		pp->pp_flags = (pp->pp_memt ? PCI_FLAGS_MEM_ENABLED : 0) |
			       (pp->pp_iot ? PCI_FLAGS_IO_ENABLED : 0);

		/* allocate a chipset for this */
		pp->pp_pc = psycho_alloc_chipset(pp, node, &_sparc_pci_chipset);
		pp->pp_pc->busno = pp->pp_bus = simba_br[0];
	}

	/* setup the rest of the sabre pbm */
	pp = sc->sc_sabre;
	pp->pp_sc = sc;
	pp->pp_memt = sc->sc_psycho_this->pp_memt;
	pp->pp_iot = sc->sc_psycho_this->pp_iot;
	pp->pp_dmat = sc->sc_psycho_this->pp_dmat;
	pp->pp_flags = sc->sc_psycho_this->pp_flags;
	pp->pp_intmap = NULL;
	pp->pp_regs = NULL;
	pp->pp_pcictl = sc->sc_psycho_this->pp_pcictl;
	pba->pba_pc = psycho_alloc_chipset(pp, sc->sc_node,
	    sc->sc_psycho_this->pp_pc);

	printf("\n");


	/* 
	 * SABRE seems to be buggy.  It only appears to work with 128K IOTSB.
	 * I have tried other sizes but they just don't seem to work.  Maybe
	 * more testing is needed.
	 *
	 * The PROM reserves a certain amount of RAM for an IOTSB.  The
	 * problem is that it's not necessarily the full 128K.  So we'll free
	 * this space up and let iommu_init() allocate a full mapping.
	 *
	 * (Otherwise we would need to change the iommu code to handle a
	 * preallocated TSB that may not cover the entire DVMA address
	 * space...
	 *
	 * The information about this memory is shared between the
	 * `virtual-dma' property, which describes the base and size of the
	 * virtual region, and the IOMMU base address register which is the
	 * only known pointer to the RAM.  To free up the memory you need to
	 * read the base addres register and then calculate the size by taking
	 * the virtual size and dividing it by 1K to get the size in bytes.
	 * This range can then be freed up by calling uvm_page_physload().
	 * 
	 */

	/* and finally start up the IOMMU ... */
	psycho_iommu_init(sc, 7);

	/*
	 * get us a config space tag, and punch in the physical address
	 * of the PCI configuration space.  note that we use unmapped
	 * access to PCI configuration space, relying on the bus space
	 * macros to provide the proper ASI based on the bus tag.
	 */
	sc->sc_configtag = psycho_alloc_config_tag(sc->sc_simba_a);
	if (bus_space_map2(sc->sc_bustag,
			  PCI_CONFIG_BUS_SPACE,
			  sc->sc_basepaddr + 0x01000000,
			  0x0100000,
			  0,
			  0,
			  &bh))
		panic("could not map sabre PCI configuration space");
	sc->sc_configaddr = bh;
}

/*
 * SUNW,psycho initialisation ..
 *	- find the per-psycho registers
 *	- figure out the IGN.
 *	- find our partner psycho
 *	- configure ourselves
 *	- bus range, bus, 
 *	- get interrupt-map and interrupt-map-mask
 *	- setup the chipsets.
 *	- if we're the first of the pair, initialise the IOMMU, otherwise
 *	  just copy it's tags and addresses.
 */
static void
psycho_init(sc, ma, pba)
	struct psycho_softc *sc;
	struct mainbus_attach_args *ma;
	struct pcibus_attach_args *pba;
{
	struct psycho_softc *osc = NULL;
	struct psycho_pbm *pp;
	bus_space_handle_t bh;
	u_int64_t csr;
	int psycho_br[2], n;
	struct pci_ctl *pci_ctl;

	/*
	 * The psycho gets three register banks:
	 * (0) per-PBM configuration and status registers
	 * (1) per-PBM PCI configuration space, containing only the
	 *     PBM 256-byte PCI header
	 * (2) the shared psycho configuration registers (struct psychoreg)
	 *
	 * XXX use the prom address for the psycho registers?  we do so far.
	 */
	sc->sc_regs = (struct psychoreg *)(u_long)ma->ma_address[2];
	sc->sc_basepaddr = (paddr_t)ma->ma_reg[2].ur_paddr;
	pci_ctl = (struct pci_ctl *)(u_long)ma->ma_address[0];

	csr = sc->sc_regs->psy_csr;
	sc->sc_ign = PSYCHO_GCSR_IGN(csr) << 6;
	sc->sc_mode = PSYCHO_MODE_PSYCHO;
	printf("psycho: impl %d, version %d: ign %x ",
		PSYCHO_GCSR_IMPL(csr), PSYCHO_GCSR_VERS(csr), sc->sc_ign);

	/*
	 * Match other psycho's that are already configured against
	 * the base physical address. This will be the same for a
	 * pair of devices that share register space.
	 */
	for (n = 0; n < psycho_cd.cd_ndevs; n++) {

		struct psycho_softc *asc =
			(struct psycho_softc *)psycho_cd.cd_devs[n];

		if (asc == NULL || asc == sc)
			/* This entry is not there or it is me */
			continue;

		if (asc->sc_basepaddr != sc->sc_basepaddr)
			/* This is an unrelated psycho */
			continue;

		/* Found partner */
		osc = asc;
		break;
	}


	/* Oh, dear.  OK, lets get started */

	/*
	 * Setup the PCI control register
	 */
	csr = bus_space_read_8(sc->sc_bustag,
			(bus_space_handle_t)(u_long)&pci_ctl->pci_csr, 0);
	csr |= PCICTL_MRLM |
	       PCICTL_ARB_PARK |
	       PCICTL_ERRINTEN |
	       PCICTL_4ENABLE;
	csr &= ~(PCICTL_SERR |
		 PCICTL_CPU_PRIO |
		 PCICTL_ARB_PRIO |
		 PCICTL_RTRYWAIT);
	bus_space_write_8(sc->sc_bustag,
			(bus_space_handle_t)(u_long)&pci_ctl->pci_csr, 0, csr);


	/*
	 * Allocate our psycho_pbm
	 */
	pp = sc->sc_psycho_this = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	if (pp == NULL)
		panic("could not allocate psycho pbm");

	memset(pp, 0, sizeof *pp);

	pp->pp_sc = sc;

	/* grab the psycho ranges */
	psycho_get_ranges(sc->sc_node, &pp->pp_range, &pp->pp_nrange);

	/* get the bus-range for the psycho */
	psycho_get_bus_range(sc->sc_node, psycho_br);

	pba->pba_bus = psycho_br[0];

	printf("bus range %u to %u", psycho_br[0], psycho_br[1]);
	printf("; PCI bus %d", psycho_br[0]);

	pp->pp_pcictl = &sc->sc_regs->psy_pcictl[0];

	/* grab the interrupt map and map mask */
	psycho_get_intmap(sc->sc_node, &pp->pp_intmap, &pp->pp_nintmap);
	psycho_get_intmapmask(sc->sc_node, &pp->pp_intmapmask);

	/* allocate our tags */
	pp->pp_memt = psycho_alloc_mem_tag(pp);
	pp->pp_iot = psycho_alloc_io_tag(pp);
	pp->pp_dmat = psycho_alloc_dma_tag(pp);
	pp->pp_flags = (pp->pp_memt ? PCI_FLAGS_MEM_ENABLED : 0) |
		       (pp->pp_iot ? PCI_FLAGS_IO_ENABLED : 0);

	/* allocate a chipset for this */
	pp->pp_pc = psycho_alloc_chipset(pp, sc->sc_node, &_sparc_pci_chipset);

	/* setup the rest of the psycho pbm */
	pba->pba_pc = psycho_alloc_chipset(pp, sc->sc_node, pp->pp_pc);

	printf("\n");

	/*
	 * And finally, if we're the first of a pair of psycho's to
	 * arrive here, start up the IOMMU and get a config space tag.
	 * Note that we use unmapped access to PCI configuration space,
	 * relying on the bus space macros to provide the proper ASI based
	 * on the bus tag.
	 */
	if (osc == NULL) {
		/*
		 * Setup IOMMU and PCI configuration if we're the first
		 * of a pair of psycho's to arrive here.
		 *
		 * We should calculate a TSB size based on amount of RAM
		 * and number of bus controllers.
		 *
		 * For the moment, 32KB should be more than enough.
		 */
		psycho_iommu_init(sc, 2);

		sc->sc_configtag = psycho_alloc_config_tag(sc->sc_psycho_this);
		if (bus_space_map2(sc->sc_bustag,
				  PCI_CONFIG_BUS_SPACE,
				  sc->sc_basepaddr + 0x01000000,
				  0x0100000,
				  0,
				  0,
				  &bh))
			panic("could not map psycho PCI configuration space");
		sc->sc_configaddr = (off_t)bh;
	} else {
		/* Just copy IOMMU state, config tag and address */
		sc->sc_is = osc->sc_is;
		sc->sc_configtag = osc->sc_configtag;
		sc->sc_configaddr = osc->sc_configaddr;
	}
}

/*
 * PCI bus support
 */

/*
 * allocate a PCI chipset tag and set it's cookie.
 */
static pci_chipset_tag_t
psycho_alloc_chipset(pp, node, pc)
	struct psycho_pbm *pp;
	int node;
	pci_chipset_tag_t pc;
{
	pci_chipset_tag_t npc;
	
	npc = malloc(sizeof *npc, M_DEVBUF, M_NOWAIT);
	if (npc == NULL)
		panic("could not allocate pci_chipset_tag_t");
	memcpy(npc, pc, sizeof *pc);
	npc->cookie = pp;
	npc->node = node;

	return (npc);
}

/*
 * grovel the OBP for various psycho properties
 */
static void
psycho_get_bus_range(node, brp)
	int node;
	int *brp;
{
	int n;

	if (getprop(node, "bus-range", sizeof(*brp), &n, (void **)&brp))
		panic("could not get psycho bus-range");
	if (n != 2)
		panic("broken psycho bus-range");
	DPRINTF(PDB_PROM, ("psycho debug: got `bus-range' for node %08x: %u - %u\n", node, brp[0], brp[1]));
}

static void
psycho_get_ranges(node, rp, np)
	int node;
	struct psycho_ranges **rp;
	int *np;
{

	if (getprop(node, "ranges", sizeof(**rp), np, (void **)rp))
		panic("could not get psycho ranges");
	DPRINTF(PDB_PROM, ("psycho debug: got `ranges' for node %08x: %d entries\n", node, *np));
}

static void
psycho_get_registers(node, rp, np)
	int node;
	struct psycho_registers **rp;
	int *np;
{

	if (getprop(node, "reg", sizeof(**rp), np, (void **)rp))
		panic("could not get psycho registers");
	DPRINTF(PDB_PROM,
	    ("psycho debug: got `reg' for node %08x: %d entries\n", node, *np));
}

static void
psycho_get_intmap(node, imp, np)
	int node;
	struct psycho_interrupt_map **imp;
	int *np;
{

	if (getprop(node, "interrupt-map", sizeof(**imp), np, (void **)imp)) {
		DPRINTF(PDB_PROM,
		    ("psycho debug: no `interupt-map' for node %08x\n", node));
		*imp = 0;
		*np = 0;
	} else
		DPRINTF(PDB_PROM,
		    ("psycho debug: got `interupt-map' for node %08x\n", node));
}

static void
psycho_get_intmapmask(node, immp)
	int node;
	struct psycho_interrupt_map_mask *immp;
{
	int n;

	if (getprop(node, "interrupt-map-mask", sizeof(*immp), &n,
	    (void **)&immp)) {
		DPRINTF(PDB_PROM,
		    ("psycho debug: no `interrupt-map-mask' for node %08x\n",
		    node));
		return;
	}
	if (n != 1)
		panic("broken psycho interrupt-map-mask");
	DPRINTF(PDB_PROM,
	    ("psycho debug: got `interrupt-map-mask' for node %08x\n", node));
}

/*
 * initialise the IOMMU..
 */
void
psycho_iommu_init(sc, tsbsize)
	struct psycho_softc *sc;
	int tsbsize;
{
	char *name;
	struct iommu_state *is;

	is = malloc(sizeof(struct iommu_state), M_DEVBUF, M_NOWAIT);
	if (is == NULL)
		panic("psycho_iommu_init: malloc is");

	sc->sc_is = is;

	/* punch in our copies */
	is->is_bustag = sc->sc_bustag;
	is->is_iommu = &sc->sc_regs->psy_iommu;

	if (getproplen(sc->sc_node, "no-streaming-cache") < 0)
		is->is_sb = 0;
	else
		is->is_sb = &sc->sc_regs->psy_iommu_strbuf;

	/* give us a nice name.. */
	name = (char *)malloc(32, M_DEVBUF, M_NOWAIT);
	if (name == 0)
		panic("couldn't malloc iommu name");
	snprintf(name, 32, "%s dvma", sc->sc_dev.dv_xname);

	iommu_init(name, is, tsbsize);
}

/*
 * below here is bus space and bus dma support
 */
bus_space_tag_t
psycho_alloc_bus_tag(pp, type)
	struct psycho_pbm *pp;
	int type;
{
	struct psycho_softc *sc = pp->pp_sc;
	bus_space_tag_t bt;

	bt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (bt == NULL)
		panic("could not allocate psycho bus tag");

	bzero(bt, sizeof *bt);
	bt->cookie = pp;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = _psycho_bus_map;
	bt->sparc_bus_mmap = psycho_bus_mmap;
	bt->sparc_intr_establish = psycho_intr_establish;
	return (bt);
}

bus_dma_tag_t
psycho_alloc_dma_tag(pp)
	struct psycho_pbm *pp;
{
	struct psycho_softc *sc = pp->pp_sc;
	bus_dma_tag_t dt, pdt = sc->sc_dmatag;

	dt = (bus_dma_tag_t)
		malloc(sizeof(struct sparc_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (dt == NULL)
		panic("could not allocate psycho dma tag");

	bzero(dt, sizeof *dt);
	dt->_cookie = pp;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	PCOPY(_dmamap_create);
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load = psycho_dmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	dt->_dmamap_load_raw = psycho_dmamap_load_raw;
	dt->_dmamap_unload = psycho_dmamap_unload;
	dt->_dmamap_sync = psycho_dmamap_sync;
	dt->_dmamem_alloc = psycho_dmamem_alloc;
	dt->_dmamem_free = psycho_dmamem_free;
	dt->_dmamem_map = psycho_dmamem_map;
	dt->_dmamem_unmap = psycho_dmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

/*
 * bus space support.  <sparc64/dev/psychoreg.h> has a discussion about
 * PCI physical addresses.
 */

static int get_childspace __P((int));

static int
get_childspace(type)
	int type;
{
	int ss;

	switch (type) {
	case PCI_CONFIG_BUS_SPACE:
		ss = 0x00;
		break;
	case PCI_IO_BUS_SPACE:
		ss = 0x01;
		break;
	case PCI_MEMORY_BUS_SPACE:
		ss = 0x02;
		break;
#if 0
	/* we don't do 64 bit memory space */
	case PCI_MEMORY64_BUS_SPACE:
		ss = 0x03;
		break;
#endif
	default:
		panic("get_childspace: unknown bus type");
	}

	return (ss);
}

static int
_psycho_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vaddr_t vaddr;
	bus_space_handle_t *hp;
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	int i, ss;

	DPRINTF(PDB_BUSMAP, ("_psycho_bus_map: type %d off %qx sz %qx flags %d va %p", t->type, (unsigned long long)offset, (unsigned long long)size, flags,
	    (void *)vaddr));

	ss = get_childspace(t->type);
	DPRINTF(PDB_BUSMAP, (" cspace %d", ss));


	for (i = 0; i < pp->pp_nrange; i++) {
		bus_addr_t paddr;

		if (((pp->pp_range[i].cspace >> 24) & 0x03) != ss)
			continue;

		paddr = pp->pp_range[i].phys_lo + offset;
		paddr |= ((bus_addr_t)pp->pp_range[i].phys_hi<<32);
		DPRINTF(PDB_BUSMAP, ("\n_psycho_bus_map: mapping paddr space %lx offset %lx paddr %qx\n",
			       (long)ss, (long)offset,
			       (unsigned long long)paddr));
		return (bus_space_map2(sc->sc_bustag, t->type, paddr,
					size, flags, vaddr, hp));
	}
	DPRINTF(PDB_BUSMAP, (" FAILED\n"));
	return (EINVAL);
}

static int
psycho_bus_mmap(t, btype, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t paddr;
	int flags;
	bus_space_handle_t *hp;
{
	bus_addr_t offset = paddr;
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	int i, ss;

	ss = get_childspace(t->type);

	DPRINTF(PDB_BUSMAP, ("_psycho_bus_mmap: type %d flags %d pa %qx\n", btype, flags, (unsigned long long)paddr));

	for (i = 0; i < pp->pp_nrange; i++) {
		bus_addr_t paddr;

		if (((pp->pp_range[i].cspace >> 24) & 0x03) != ss)
			continue;

		paddr = pp->pp_range[i].phys_lo + offset;
		paddr |= ((bus_addr_t)pp->pp_range[i].phys_hi<<32);
		DPRINTF(PDB_BUSMAP, ("\n_psycho_bus_mmap: mapping paddr space %lx offset %lx paddr %qx\n",
			       (long)ss, (long)offset,
			       (unsigned long long)paddr));
		return (bus_space_mmap(sc->sc_bustag, 0, paddr,
				       flags, hp));
	}

	return (-1);
}

/*
 * interrupt mapping.  this tells what sparc ipl any given ino runs at.
 */
static int pci_ino_to_ipl_table[] = {
	0, 0, 0, 0,	/* PCI A, Slot 0, INTA#/B#/C#/D# */
	0, 0, 0, 0,	/* PCI A, Slot 1, INTA#/B#/C#/D# */
	0, 0, 0, 0,	/* PCI A, Slot 2, INTA#/B#/C#/D# (unavailable) */
	0, 0, 0, 0,	/* PCI A, Slot 3, INTA#/B#/C#/D# (unavailable) */
	0, 0, 0, 0,	/* PCI B, Slot 0, INTA#/B#/C#/D# */
	0, 0, 0, 0,	/* PCI B, Slot 1, INTA#/B#/C#/D# */
	0, 0, 0, 0,	/* PCI B, Slot 2, INTA#/B#/C#/D# */
	0, 0, 0, 0,	/* PCI B, Slot 3, INTA#/B#/C#/D# */

	PIL_SCSI,	/* SCSI */
	PIL_NET,	/* Ethernet */
	3,		/* Parallel */
	PIL_AUD,	/* Audio Record */

	PIL_AUD,	/* Audio Playback */
	14,		/* Power Fail */
	4,		/* Keyboard/Mouse/Serial */
	PIL_FD,		/* Floppy */

	14,		/* Thermal Warning */
	PIL_SER,	/* Keyboard */
	PIL_SER,	/* Mouse */
	PIL_SER,	/* Serial */

	0,		/* Reserved */
	0,		/* Reserved */
	14,		/* Uncorrectable ECC error */
	14,		/* Correctable ECC error */

	14,		/* PCI A bus error */
	14,		/* PCI B bus error */
	14,		/* power management */
};

/*
 * install an interrupt handler for a PCI device
 */
void *
psycho_intr_establish(t, ihandle, level, flags, handler, arg)
	bus_space_tag_t t;
	int ihandle;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	struct intrhand *ih;
	int ino;
	long vec = ihandle; 

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	DPRINTF(PDB_INTR, ("\npsycho_intr_establish: ihandle %x vec %lx", ihandle, vec));
	ino = INTINO(vec);
	DPRINTF(PDB_INTR, (" ino %x", ino));
	if ((flags & BUS_INTR_ESTABLISH_SOFTINTR) == 0) {
		volatile int64_t *intrmapptr, *intrclrptr;
		int64_t intrmap = 0;
		int i;

		DPRINTF(PDB_INTR, ("\npsycho: intr %lx: %p\nHunting for IRQ...\n",
		    (long)ino, intrlev[ino]));
		if ((ino & INTMAP_OBIO) == 0) {
			/*
			 * there are only 8 PCI interrupt INO's available
			 */
			i = INTPCIINOX(vec);

			intrmapptr = &((&sc->sc_regs->pcia_slot0_int)[i]);
			intrclrptr = &sc->sc_regs->pcia0_clr_int[ino];

			DPRINTF(PDB_INTR, ("- turning on PCI intr %d", i));
		} else {
			/*
			 * there are INTPCI_MAXOBINO (0x16) OBIO interrupts
			 * available here (i think).
			 */
			i = INTPCIOBINOX(vec);
			if (i > INTPCI_MAXOBINO)
				panic("ino %ld", vec);

			intrmapptr = &((&sc->sc_regs->scsi_int_map)[i]);
			intrclrptr = &((&sc->sc_regs->scsi_clr_int)[i]);

			DPRINTF(PDB_INTR, ("- turning on OBIO intr %d", i));
		}

		/* Register the map and clear intr registers */
		ih->ih_map = intrmapptr;
		ih->ih_clr = intrclrptr;

		/*
		 * Read the current value as we can't change it besides the
		 * valid bit so so make sure only this bit is changed.
		 */
		intrmap = *intrmapptr;
		DPRINTF(PDB_INTR, ("; read intrmap = %016qx",
		    (unsigned long long)intrmap));

		/* Enable the interrupt */
		intrmap |= INTMAP_V;
		DPRINTF(PDB_INTR, ("; addr of intrmapptr = %p", intrmapptr));
		DPRINTF(PDB_INTR, ("; writing intrmap = %016qx\n",
		    (unsigned long long)intrmap));
		*intrmapptr = intrmap;
		DPRINTF(PDB_INTR, ("; reread intrmap = %016qx",
		    (unsigned long long)(intrmap = *intrmapptr)));
	}
#ifdef NOT_DEBUG
	if (psycho_debug & PDB_INTR) {
		long i;

		for (i = 0; i < 500000000; i++)
			continue;
	}
#endif

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_number = ino | sc->sc_ign;
	/*
	 * If a `device class' level is specified, use it,
	 * else get the PIL from a built-in table.
	 */
	if (level != IPL_NONE)
		ih->ih_pil = level;
	else if (ino > (sizeof(pci_ino_to_ipl_table) / sizeof(int)))
		ih->ih_pil = 0;
	else
		ih->ih_pil = pci_ino_to_ipl_table[ino];

	DPRINTF(PDB_INTR, (
	    "; installing handler %p arg %p with ino %u pil %u\n",
	    handler, arg, (u_int)ino, (u_int)ih->ih_pil));

	intr_establish(ih->ih_pil, ih);
	return (ih);
}

/*
 * hooks into the iommu dvma calls.
 */
int
psycho_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	return (iommu_dvmamap_load(t, sc->sc_is, map, buf, buflen, p, flags));
}

void
psycho_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	iommu_dvmamap_unload(t, sc->sc_is, map);
}

int
psycho_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	return (iommu_dvmamap_load_raw(t, sc->sc_is, map, segs, nsegs, flags, size));
}

void
psycho_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	if (ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) {
		/* Flush the CPU then the IOMMU */
		bus_dmamap_sync(t->_parent, map, offset, len, ops);
		iommu_dvmamap_sync(t, sc->sc_is, map, offset, len, ops);
	}
	if (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) {
		/* Flush the IOMMU then the CPU */
		iommu_dvmamap_sync(t, sc->sc_is, map, offset, len, ops);
		bus_dmamap_sync(t->_parent, map, offset, len, ops);
	}

}

int
psycho_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size;
	bus_size_t alignment;
	bus_size_t boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	return (iommu_dvmamem_alloc(t, sc->sc_is, size, alignment, boundary,
	    segs, nsegs, rsegs, flags));
}

void
psycho_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	iommu_dvmamem_free(t, sc->sc_is, segs, nsegs);
}

int
psycho_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	return (iommu_dvmamem_map(t, sc->sc_is, segs, nsegs, size, kvap, flags));
}

void
psycho_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	iommu_dvmamem_unmap(t, sc->sc_is, kva, size);
}
