/*	$NetBSD: psycho.c,v 1.3 2000/04/08 03:08:20 mrg Exp $	*/

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

/*
 * PCI support for UltraSPARC `psycho'
 */

#undef DEBUG
#define DEBUG

#ifdef DEBUG
#define	PDB_PROM	0x1
#define	PDB_IOMMU	0x2
int psycho_debug = 0x0;
#define DPRINTF(l, s)   do { if (psycho_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>

static pci_chipset_tag_t psycho_alloc_chipset __P((struct psycho_pbm *, int,
						   pci_chipset_tag_t));
static void psycho_get_bus_range __P((int, int *));
static void psycho_get_ranges __P((int, struct psycho_ranges **, int *));
static void psycho_get_registers __P((int, struct psycho_registers **, int *));
static void psycho_get_intmap __P((int, struct psycho_interrupt_map **, int *));
static void psycho_get_intmapmask __P((int, struct psycho_interrupt_map_mask *));

/* IOMMU support */
static void psycho_iommu_init __P((struct psycho_softc *));

extern struct sparc_pci_chipset _sparc_pci_chipset;

/*
 * autoconfiguration
 */
static	int	psycho_match __P((struct device *, struct cfdata *, void *));
static	void	psycho_attach __P((struct device *, struct device *, void *));
static	int	psycho_print __P((void *aux, const char *p));

static	void	sabre_init __P((struct psycho_softc *, struct pcibus_attach_args *));
static	void	psycho_init __P((struct psycho_softc *, struct pcibus_attach_args *));

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
	 * XXX use the prom address for the psycho registers?  we do so far.
	 */
	sc->sc_regs = (struct psychoreg *)(u_long)ma->ma_address[0];
	sc->sc_basepaddr = (paddr_t)ma->ma_reg[0].ur_paddr;

	/*
	 * call the model-specific initialisation routine.
	 */
	if (strcmp(model, ROM_SABRE_MODEL) == 0)
		sabre_init(sc, &pba);
	else if (strcmp(model, ROM_PSYCHO_MODEL) == 0)
		psycho_init(sc, &pba);
#ifdef DIAGNOSTIC
	else
		panic("psycho_attach: unknown model %s?", model);
#endif

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
sabre_init(sc, pba)
	struct psycho_softc *sc;
	struct pcibus_attach_args *pba;
{
	struct psycho_pbm *pp;
	bus_space_handle_t bh;
	u_int64_t csr;
	int node;
	int sabre_br[2], simba_br[2];

	/* who? said a voice, incredulous */
	sc->sc_mode = PSYCHO_MODE_SABRE;
	printf("sabre: ");

	/* setup the PCI control register; there is only one for the sabre */
	csr = bus_space_read_8(sc->sc_bustag, &sc->sc_regs->psy_pcictl[0].pci_csr, 0);
	csr = PCICTL_SERR | PCICTL_ARB_PARK | PCICTL_ERRINTEN | PCICTL_4ENABLE;
	bus_space_write_8(sc->sc_bustag, &sc->sc_regs->psy_pcictl[0].pci_csr, 0, csr);

	/* allocate a pair of psycho_pbm's for our simba's */
	sc->sc_sabre = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	sc->sc_simba_a = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	sc->sc_simba_b = malloc(sizeof *pp, M_DEVBUF, M_NOWAIT);
	if (sc->sc_simba_a == NULL || sc->sc_simba_b == NULL)
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

		if (strcmp(name, ROM_PCI_NAME) != 0)
			continue;

		model = getpropstring(node, "model");
		if (strcmp(model, ROM_SIMBA_MODEL) != 0)
			continue;

		psycho_get_bus_range(node, simba_br);

		if (simba_br[0] == 1) {		/* PCI B */
			pp = sc->sc_simba_b;
			pp->pp_pcictl = &sc->sc_regs->psy_pcictl[1];
			pp->pp_sb_diag = &sc->sc_regs->psy_strbufdiag[1];
			who = 'b';
		} else {			/* PCI A */
			pp = sc->sc_simba_a;
			pp->pp_pcictl = &sc->sc_regs->psy_pcictl[0];
			pp->pp_sb_diag = &sc->sc_regs->psy_strbufdiag[0];
			who = 'a';
		}
		/* link us in .. */
		pp->pp_sc = sc;
		
		printf("; simba %c, PCI bus %d", who, simba_br[0]);

		/* grab the simba registers, interrupt map and map mask */
		psycho_get_registers(node, &pp->pp_regs, &pp->pp_nregs);
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
	pp->pp_sb_diag = sc->sc_psycho_this->pp_sb_diag;
	pba->pba_pc = psycho_alloc_chipset(pp, sc->sc_node,
	    sc->sc_psycho_this->pp_pc);

	printf("\n");

	/* and finally start up the IOMMU ... */
	psycho_iommu_init(sc);

	/*
	 * get us a config space tag, and punch in the physical address
	 * of the PCI configuration space.  note that we use unmapped
	 * access to PCI configuration space, relying on the bus space
	 * macros to provide the proper ASI based on the bus tag.
	 */
	sc->sc_configtag = psycho_alloc_config_tag(sc->sc_simba_a);
#if 0
	sc->sc_configaddr = (paddr_t)sc->sc_basepaddr + 0x01000000;
#else
	if (bus_space_map2(sc->sc_bustag,
			  PCI_CONFIG_BUS_SPACE,
			  sc->sc_basepaddr + 0x01000000,
			  0x0100000,
			  0,
			  0,
			  &bh))
		panic("could not map sabre PCI configuration space");
	sc->sc_configaddr = (paddr_t)bh;
#endif
}

/*
 * SUNW,psycho initialisation ..
 *	- XXX what do we do here?
 *
 * i think that an attaching psycho should here find it's partner psycho
 * and if they haven't been attached yet, allocate both psycho_pbm's and
 * fill them both in here, and when the partner attaches, there is little
 * to do... perhaps keep a static array of what psycho have been found so
 * far (or perhaps those that have not yet been finished).  .mrg.
 * note that the partner can be found via matching `ranges' properties.
 */
static void
psycho_init(sc, pba)
	struct psycho_softc *sc;
	struct pcibus_attach_args *pba;
{
#if 1
	panic("can't do SUNW,psycho yet");
#else

	/*
	 * OK, so the deal here is:
	 *	- given our base register address, search our sibling
	 *	  devices for a match.
	 *	- if we find a match, we are attaching an almost
	 *	  already setup PCI bus, the partner already done.
	 *	- otherwise, we are doing the hard slog.
	 */
	for (n = 0; n < psycho_cd.cd_ndevs; n++) {
		psycho_softc *osc = &psycho_cd.cd_devs[n];

		/*
		 * I am not myself.
		 */
		if (osc == sc || osc->sc_regs != sc->sc_regs)
			continue;

		/*
		 * OK, so we found a matching regs that wasn't me,
		 * so that must make me partly attached.  Finish it.
		 */
	}

	/* Oh, dear.  OK, lets get started */

	/* who? said a voice, incredulous */
	sc->sc_mode = PSYCHO_MODE_PSYCHO_A;
	printf("psycho: ");
#endif
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
	DPRINTF(PDB_PROM, ("psycho debug: got `reg' for node %08x: %d entries\n", node, *np));
}

static void
psycho_get_intmap(node, imp, np)
	int node;
	struct psycho_interrupt_map **imp;
	int *np;
{

	if (getprop(node, "interrupt-map", sizeof(**imp), np, (void **)imp))
		panic("could not get psycho interrupt-map");
	DPRINTF(PDB_PROM, ("psycho debug: got `interupt-map' for node %08x\n", node));
}

static void
psycho_get_intmapmask(node, immp)
	int node;
	struct psycho_interrupt_map_mask *immp;
{
	int n;

	if (getprop(node, "interrupt-map-mask", sizeof(*immp), &n,
	    (void **)&immp))
		panic("could not get psycho interrupt-map-mask");
	if (n != 1)
		panic("broken psycho interrupt-map-mask");
	DPRINTF(PDB_PROM, ("psycho debug: got `interrupt-map-mask' for node %08x\n", node));
}

/* 
 * IOMMU code
 */

/*
 * initialise the IOMMU..
 */
void
psycho_iommu_init(sc)
	struct psycho_softc *sc;
{
	char *name;

	/* punch in our copies */
	sc->sc_is.is_bustag = sc->sc_bustag;
	sc->sc_is.is_iommu = &sc->sc_regs->psy_iommu;
	sc->sc_is.is_sb = &sc->sc_regs->psy_iommu_strbuf;

	/* give us a nice name.. */
	name = (char *)malloc(32, M_DEVBUF, M_NOWAIT);
	if (name == 0)
		panic("couldn't malloc iommu name");
	snprintf(name, 32, "%s dvma", sc->sc_dev.dv_xname);

	/* XXX XXX XXX FIX ME tsbsize XXX XXX XXX */
	iommu_init(name, &sc->sc_is, 0);
}
