/*	$NetBSD: psycho.c,v 1.112.6.2 2014/08/20 00:03:25 tls Exp $	*/

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
 * Copyright (c) 2001, 2002 Eduardo E. Horvath
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: psycho.c,v 1.112.6.2 2014/08/20 00:03:25 tls Exp $");

#include "opt_ddb.h"

/*
 * Support for `psycho' and `psycho+' UPA to PCI bridge and
 * UltraSPARC IIi and IIe `sabre' PCI controllers.
 */

#ifdef DEBUG
#define PDB_PROM	0x01
#define PDB_BUSMAP	0x02
#define PDB_INTR	0x04
#define PDB_INTMAP	0x08
#define PDB_CONF	0x10
#define PDB_STICK	0x20
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
#include <sys/reboot.h>

#include <uvm/uvm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/psl.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>

#include "ioconf.h"

static pci_chipset_tag_t psycho_alloc_chipset(struct psycho_pbm *, int,
	pci_chipset_tag_t);
static struct extent *psycho_alloc_extent(struct psycho_pbm *, int, int,
	const char *);
static void psycho_get_bus_range(int, int *);
static void psycho_fixup_bus_range(int, int *);
static void psycho_get_ranges(int, struct psycho_ranges **, int *);
static void psycho_set_intr(struct psycho_softc *, int, void *, uint64_t *,
	uint64_t *);

/* chipset handlers */
static pcireg_t	psycho_pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
static void	psycho_pci_conf_write(pci_chipset_tag_t, pcitag_t, int,
				      pcireg_t);
static void	*psycho_pci_intr_establish(pci_chipset_tag_t,
					   pci_intr_handle_t,
					   int, int (*)(void *), void *);
static int	psycho_pci_find_ino(const struct pci_attach_args *,
				    pci_intr_handle_t *);

/* Interrupt handlers */
static int psycho_ue(void *);
static int psycho_ce(void *);
static int psycho_bus_a(void *);
static int psycho_bus_b(void *);
static int psycho_powerfail(void *);
static int psycho_wakeup(void *);


/* IOMMU support */
static void psycho_iommu_init(struct psycho_softc *, int);

/*
 * bus space and bus DMA support for UltraSPARC `psycho'.  note that most
 * of the bus DMA support is provided by the iommu dvma controller.
 */
static struct psycho_ranges *get_psychorange(struct psycho_pbm *, int);

static paddr_t psycho_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static int _psycho_bus_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	vaddr_t, bus_space_handle_t *);
static void *psycho_intr_establish(bus_space_tag_t, int, int, int (*)(void *),
	void *, void(*)(void));

static int psycho_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	bus_size_t, int, bus_dmamap_t *);
static void psycho_sabre_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	bus_size_t, int);

/* base pci_chipset */
extern struct sparc_pci_chipset _sparc_pci_chipset;

/* power button handlers */
static void psycho_register_power_button(struct psycho_softc *sc);
static void psycho_power_button_pressed(void *arg);

/*
 * autoconfiguration
 */
static	int	psycho_match(device_t, cfdata_t, void *);
static	void	psycho_attach(device_t, device_t, void *);
static	int	psycho_print(void *aux, const char *p);

CFATTACH_DECL_NEW(psycho, sizeof(struct psycho_softc),
    psycho_match, psycho_attach, NULL, NULL);

/*
 * "sabre" is the UltraSPARC IIi onboard UPA to PCI bridge.  It manages a
 * single PCI bus and does not have a streaming buffer.  It often has an APB
 * (advanced PCI bridge) connected to it, which was designed specifically for
 * the IIi.  The APB let's the IIi handle two independednt PCI buses, and
 * appears as two "simba"'s underneath the sabre.
 *
 * "psycho" and "psycho+" is a dual UPA to PCI bridge.  It sits on the UPA bus
 * and manages two PCI buses.  "psycho" has two 64-bit 33 MHz buses, while
 * "psycho+" controls both a 64-bit 33 MHz and a 64-bit 66 MHz PCI bus.  You
 * will usually find a "psycho+" since I don't think the original "psycho"
 * ever shipped, and if it did it would be in the U30.
 *
 * Each "psycho" PCI bus appears as a separate OFW node, but since they are
 * both part of the same IC, they only have a single register space.  As such,
 * they need to be configured together, even though the autoconfiguration will
 * attach them separately.
 *
 * On UltraIIi machines, "sabre" itself usually takes pci0, with "simba" often
 * as pci1 and pci2, although they have been implemented with other PCI bus
 * numbers on some machines.
 *
 * On UltraII machines, there can be any number of "psycho+" ICs, each
 * providing two PCI buses.
 *
 *
 * XXXX The psycho/sabre node has an `interrupts' attribute.  They contain
 * the values of the following interrupts in this order:
 *
 * PCI Bus Error	(30)
 * DMA UE		(2e)
 * DMA CE		(2f)
 * Power Fail		(25)
 *
 * We really should attach handlers for each.
 *
 */

#define	ROM_PCI_NAME		"pci"

struct psycho_names {
	const char *p_name;
	int p_type;
} psycho_names[] = {
	{ "SUNW,psycho",	PSYCHO_MODE_PSYCHO	},
	{ "pci108e,8000",	PSYCHO_MODE_PSYCHO	},
	{ "SUNW,sabre",		PSYCHO_MODE_SABRE	},
	{ "pci108e,a000",	PSYCHO_MODE_SABRE	},
	{ "pci108e,a001",	PSYCHO_MODE_SABRE	},
	{ NULL, 0 }
};

struct psycho_softc *psycho0 = NULL;

static	int
psycho_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	char *model = prom_getpropstring(ma->ma_node, "model");
	int i;

	/* match on a name of "pci" and a sabre or a psycho */
	if (strcmp(ma->ma_name, ROM_PCI_NAME) == 0) {
		for (i=0; psycho_names[i].p_name; i++)
			if (strcmp(model, psycho_names[i].p_name) == 0)
				return (1);

		model = prom_getpropstring(ma->ma_node, "compatible");
		for (i=0; psycho_names[i].p_name; i++)
			if (strcmp(model, psycho_names[i].p_name) == 0)
				return (1);
	}
	return (0);
}

#ifdef DEBUG
static void psycho_dump_intmap(struct psycho_softc *sc);
static void
psycho_dump_intmap(struct psycho_softc *sc)
{
	volatile uint64_t *intrmapptr = NULL;

	printf("psycho_dump_intmap: OBIO\n");

	for (intrmapptr = &sc->sc_regs->scsi_int_map;
	     intrmapptr < &sc->sc_regs->ue_int_map;
	     intrmapptr++)
		printf("%p: %llx\n", intrmapptr,
		    (unsigned long long)*intrmapptr);

	printf("\tintmap:pci\n");
	for (intrmapptr = &sc->sc_regs->pcia_slot0_int;
	     intrmapptr <= &sc->sc_regs->pcib_slot3_int;
	     intrmapptr++)
		printf("%p: %llx\n", intrmapptr,
		    (unsigned long long)*intrmapptr);

	printf("\tintmap:ffb\n");
	for (intrmapptr = &sc->sc_regs->ffb0_int_map;
	     intrmapptr <= &sc->sc_regs->ffb1_int_map;
	     intrmapptr++)
		printf("%p: %llx\n", intrmapptr,
		    (unsigned long long)*intrmapptr);
}
#endif

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
static	void
psycho_attach(device_t parent, device_t self, void *aux)
{
	struct psycho_softc *sc = device_private(self);
	struct psycho_softc *osc = NULL;
	struct psycho_pbm *pp;
	struct pcibus_attach_args pba;
	struct mainbus_attach_args *ma = aux;
	struct psycho_ranges *pr;
	prop_dictionary_t dict;
	bus_space_handle_t bh;
	uint64_t csr, mem_base;
	int psycho_br[2], n, i;
	bus_space_handle_t pci_ctl;
	char *model = prom_getpropstring(ma->ma_node, "model");

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;
	sc->sc_last_stick = 0;

	if (psycho0 == NULL)
		psycho0 = sc;
	DPRINTF(PDB_STICK, ("init psycho0 %lx\n", (long)sc));
	/*
	 * Identify the device.
	 */
	for (i=0; psycho_names[i].p_name; i++)
		if (strcmp(model, psycho_names[i].p_name) == 0) {
			sc->sc_mode = psycho_names[i].p_type;
			goto found;
		}

	model = prom_getpropstring(ma->ma_node, "compatible");
	for (i=0; psycho_names[i].p_name; i++)
		if (strcmp(model, psycho_names[i].p_name) == 0) {
			sc->sc_mode = psycho_names[i].p_type;
			goto found;
		}

	panic("unknown psycho model %s", model);
found:

	/*
	 * The psycho gets three register banks:
	 * (0) per-PBM configuration and status registers
	 * (1) per-PBM PCI configuration space, containing only the
	 *     PBM 256-byte PCI header
	 * (2) the shared psycho configuration registers (struct psychoreg)
	 */

	/* Register layouts are different.  stuupid. */
	if (sc->sc_mode == PSYCHO_MODE_PSYCHO) {
		sc->sc_basepaddr = (paddr_t)ma->ma_reg[2].ur_paddr;

		if (ma->ma_naddress > 2) {
			sparc_promaddr_to_handle(sc->sc_bustag,
				ma->ma_address[2], &sc->sc_bh);
			sparc_promaddr_to_handle(sc->sc_bustag,
				ma->ma_address[0], &pci_ctl);

			sc->sc_regs = (struct psychoreg *)
				bus_space_vaddr(sc->sc_bustag, sc->sc_bh);
		} else if (ma->ma_nreg > 2) {

			/* We need to map this in ourselves. */
			if (bus_space_map(sc->sc_bustag,
				ma->ma_reg[2].ur_paddr,
				ma->ma_reg[2].ur_len, BUS_SPACE_MAP_LINEAR,
				&sc->sc_bh))
				panic("psycho_attach: cannot map regs");
			sc->sc_regs = (struct psychoreg *)
				bus_space_vaddr(sc->sc_bustag, sc->sc_bh);

			if (bus_space_map(sc->sc_bustag,
				ma->ma_reg[0].ur_paddr,
				ma->ma_reg[0].ur_len, BUS_SPACE_MAP_LINEAR,
				&pci_ctl))
				panic("psycho_attach: cannot map ctl");
		} else
			panic("psycho_attach: %d not enough registers",
				ma->ma_nreg);

	} else {
		sc->sc_basepaddr = (paddr_t)ma->ma_reg[0].ur_paddr;

		if (ma->ma_naddress) {
			sparc_promaddr_to_handle(sc->sc_bustag,
				ma->ma_address[0], &sc->sc_bh);
			sc->sc_regs = (struct psychoreg *)
				bus_space_vaddr(sc->sc_bustag, sc->sc_bh);

			bus_space_subregion(sc->sc_bustag, sc->sc_bh,
				offsetof(struct psychoreg,  psy_pcictl),
				sizeof(struct pci_ctl), &pci_ctl);
		} else if (ma->ma_nreg) {

			/* We need to map this in ourselves. */
			if (bus_space_map(sc->sc_bustag,
				ma->ma_reg[0].ur_paddr,
				ma->ma_reg[0].ur_len, BUS_SPACE_MAP_LINEAR,
				&sc->sc_bh))
				panic("psycho_attach: cannot map regs");
			sc->sc_regs = (struct psychoreg *)
				bus_space_vaddr(sc->sc_bustag, sc->sc_bh);

			bus_space_subregion(sc->sc_bustag, sc->sc_bh,
				offsetof(struct psychoreg,  psy_pcictl),
				sizeof(struct pci_ctl), &pci_ctl);
		} else
			panic("psycho_attach: %d not enough registers",
				ma->ma_nreg);
	}

	csr = bus_space_read_8(sc->sc_bustag, sc->sc_bh,
		offsetof(struct psychoreg, psy_csr));
	sc->sc_ign = 0x7c0; /* APB IGN is always 0x7c */
	if (sc->sc_mode == PSYCHO_MODE_PSYCHO)
		sc->sc_ign = PSYCHO_GCSR_IGN(csr) << 6;

	aprint_normal_dev(self, "%s: impl %d, version %d: ign %x ",
		model, PSYCHO_GCSR_IMPL(csr), PSYCHO_GCSR_VERS(csr),
		sc->sc_ign);
	/*
	 * Match other psycho's that are already configured against
	 * the base physical address. This will be the same for a
	 * pair of devices that share register space.
	 */
	for (n = 0; n < psycho_cd.cd_ndevs; n++) {

		struct psycho_softc *asc = device_lookup_private(&psycho_cd, n);

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
	csr = bus_space_read_8(sc->sc_bustag, pci_ctl,
		offsetof(struct pci_ctl, pci_csr));
	csr |= PCICTL_MRLM |
	       PCICTL_ARB_PARK |
	       PCICTL_ERRINTEN |
	       PCICTL_4ENABLE;
	csr &= ~(PCICTL_SERR |
		 PCICTL_CPU_PRIO |
		 PCICTL_ARB_PRIO |
		 PCICTL_RTRYWAIT);
	bus_space_write_8(sc->sc_bustag, pci_ctl,
		offsetof(struct pci_ctl, pci_csr), csr);


	/*
	 * Allocate our psycho_pbm
	 */
	pp = sc->sc_psycho_this = malloc(sizeof *pp, M_DEVBUF,
					 M_NOWAIT | M_ZERO);
	if (pp == NULL)
		panic("could not allocate psycho pbm");

	pp->pp_sc = sc;

	/* grab the psycho ranges */
	psycho_get_ranges(sc->sc_node, &pp->pp_range, &pp->pp_nrange);

	/* get the bus-range for the psycho */
	psycho_get_bus_range(sc->sc_node, psycho_br);

	pba.pba_bus = psycho_br[0];
	pba.pba_bridgetag = NULL;

	/* Fix up invalid 0x00-0xff bus-range, as found on SPARCle */
	if (psycho_br[0] == 0 && psycho_br[1] == 0xff)
		psycho_fixup_bus_range(sc->sc_node, psycho_br);

	aprint_normal("bus range %u to %u", psycho_br[0], psycho_br[1]);
	aprint_normal("; PCI bus %d", psycho_br[0]);

	pp->pp_pcictl = pci_ctl;

	/* allocate our tags */
	pp->pp_memt = psycho_alloc_mem_tag(pp);
	pp->pp_iot = psycho_alloc_io_tag(pp);
	pp->pp_dmat = psycho_alloc_dma_tag(pp);
	pp->pp_flags = (pp->pp_memt ? PCI_FLAGS_MEM_OKAY : 0) |
		       (pp->pp_iot ? PCI_FLAGS_IO_OKAY : 0);

	/* allocate a chipset for this */
	pp->pp_pc = psycho_alloc_chipset(pp, sc->sc_node, &_sparc_pci_chipset);
	pp->pp_pc->spc_busmax = psycho_br[1];

	switch((ma->ma_reg[0].ur_paddr) & 0xf000) {
	case 0x2000:
		pp->pp_id = PSYCHO_PBM_A;
		break;
	case 0x4000:
		pp->pp_id = PSYCHO_PBM_B;
		break;
	}

	aprint_normal("\n");

	/* allocate extents for free bus space */
	pp->pp_exmem = psycho_alloc_extent(pp, sc->sc_node, 0x02, "psycho mem");
	pp->pp_exio = psycho_alloc_extent(pp, sc->sc_node, 0x01, "psycho io");

#ifdef DEBUG
	if (psycho_debug & PDB_INTR)
		psycho_dump_intmap(sc);
#endif

	/*
	 * And finally, if we're a sabre or the first of a pair of psycho's to
	 * arrive here, start up the IOMMU and get a config space tag.
	 */
	if (osc == NULL) {
		uint64_t timeo;

		/*
		 * Establish handlers for interesting interrupts....
		 *
		 * XXX We need to remember these and remove this to support
		 * hotplug on the UPA/FHC bus.
		 *
		 * XXX Not all controllers have these, but installing them
		 * is better than trying to sort through this mess.
		 */
		psycho_set_intr(sc, 15, psycho_ue,
			&sc->sc_regs->ue_int_map,
			&sc->sc_regs->ue_clr_int);
		psycho_set_intr(sc, 1, psycho_ce,
			&sc->sc_regs->ce_int_map,
			&sc->sc_regs->ce_clr_int);
		psycho_set_intr(sc, 15, psycho_bus_a,
			&sc->sc_regs->pciaerr_int_map,
			&sc->sc_regs->pciaerr_clr_int);
		/*
		 * Netra X1 may hang when the powerfail interrupt is enabled.
		 */
		if (strcmp(machine_model, "SUNW,UltraAX-i2") != 0) {
			psycho_set_intr(sc, 15, psycho_powerfail,
				&sc->sc_regs->power_int_map,
				&sc->sc_regs->power_clr_int);
			psycho_register_power_button(sc);
		}
		if (sc->sc_mode != PSYCHO_MODE_SABRE) {
			/* sabre doesn't have these interrupts */
			psycho_set_intr(sc, 15, psycho_bus_b,
					&sc->sc_regs->pciberr_int_map,
					&sc->sc_regs->pciberr_clr_int);
			psycho_set_intr(sc, 1, psycho_wakeup,
					&sc->sc_regs->pwrmgt_int_map,
					&sc->sc_regs->pwrmgt_clr_int);
		}

		/*
		 * Apparently a number of machines with psycho and psycho+
		 * controllers have interrupt latency issues.  We'll try
		 * setting the interrupt retry timeout to 0xff which gives us
		 * a retry of 3-6 usec (which is what sysio is set to) for the
		 * moment, which seems to help alleviate this problem.
		 */
		timeo = sc->sc_regs->intr_retry_timer;
		if (timeo > 0xfff) {
#ifdef DEBUG
			printf("decreasing interrupt retry timeout "
				"from %lx to 0xff\n", (long)timeo);
#endif
			sc->sc_regs->intr_retry_timer = 0xff;
		}

		/*
		 * Allocate bus node, this contains a prom node per bus.
		 */
		pp->pp_pc->spc_busnode =
		    malloc(sizeof(*pp->pp_pc->spc_busnode), M_DEVBUF,
				  M_NOWAIT | M_ZERO);
		if (pp->pp_pc->spc_busnode == NULL)
			panic("psycho_attach: malloc busnode");

		/*
		 * Setup IOMMU and PCI configuration if we're the first
		 * of a pair of psycho's to arrive here.
		 *
		 * We should calculate a TSB size based on amount of RAM
		 * and number of bus controllers and number an type of
		 * child devices.
		 *
		 * For the moment, 32KB should be more than enough.
		 */
		sc->sc_is = malloc(sizeof(struct iommu_state),
			M_DEVBUF, M_NOWAIT);
		if (sc->sc_is == NULL)
			panic("psycho_attach: malloc iommu_state");

		/* Point the strbuf_ctl at the iommu_state */
		pp->pp_sb.sb_is = sc->sc_is;

		sc->sc_is->is_sb[0] = sc->sc_is->is_sb[1] = NULL;
		if (prom_getproplen(sc->sc_node, "no-streaming-cache") < 0) {
			struct strbuf_ctl *sb = &pp->pp_sb;
			vaddr_t va = (vaddr_t)&pp->pp_flush[0x40];

			/*
			 * Initialize the strbuf_ctl.
			 *
			 * The flush sync buffer must be 64-byte aligned.
			 */
			sb->sb_flush = (void *)(va & ~0x3f);

			bus_space_subregion(sc->sc_bustag, pci_ctl,
				offsetof(struct pci_ctl, pci_strbuf),
				sizeof (struct iommu_strbuf), &sb->sb_sb);

			/* Point our iommu at the strbuf_ctl */
			sc->sc_is->is_sb[0] = sb;
		}

		psycho_iommu_init(sc, 2);

		sc->sc_configtag = psycho_alloc_config_tag(sc->sc_psycho_this);

		/*
		 * XXX This is a really ugly hack because PCI config space
		 * is explicitly handled with unmapped accesses.
		 */
		i = sc->sc_bustag->type;
		sc->sc_bustag->type = PCI_CONFIG_BUS_SPACE;
		if (bus_space_map(sc->sc_bustag, sc->sc_basepaddr + 0x01000000,
			0x01000000, 0, &bh))
			panic("could not map psycho PCI configuration space");
		sc->sc_bustag->type = i;
		sc->sc_configaddr = bh;
	} else {
		/* Share bus numbers with the pair of mine */
		pp->pp_pc->spc_busnode =
		    osc->sc_psycho_this->pp_pc->spc_busnode;

		/* Just copy IOMMU state, config tag and address */
		sc->sc_is = osc->sc_is;
		sc->sc_configtag = osc->sc_configtag;
		sc->sc_configaddr = osc->sc_configaddr;

		/* Point the strbuf_ctl at the iommu_state */
		pp->pp_sb.sb_is = sc->sc_is;

		if (prom_getproplen(sc->sc_node, "no-streaming-cache") < 0) {
			struct strbuf_ctl *sb = &pp->pp_sb;
			vaddr_t va = (vaddr_t)&pp->pp_flush[0x40];

			/*
			 * Initialize the strbuf_ctl.
			 *
			 * The flush sync buffer must be 64-byte aligned.
			 */
			sb->sb_flush = (void *)(va & ~0x3f);

			bus_space_subregion(sc->sc_bustag, pci_ctl,
				offsetof(struct pci_ctl, pci_strbuf),
				sizeof (struct iommu_strbuf), &sb->sb_sb);

			/* Point our iommu at the strbuf_ctl */
			sc->sc_is->is_sb[1] = sb;
		}
		iommu_reset(sc->sc_is);
	}

	dict = device_properties(self);
	pr = get_psychorange(pp, 2);	/* memory range */
#ifdef DEBUG
	printf("memory range: %08x %08x\n", pr->phys_hi, pr->phys_lo);
#endif
	mem_base = ((uint64_t)pr->phys_hi) << 32 | pr->phys_lo;
	prop_dictionary_set_uint64(dict, "mem_base", mem_base);

	/*
	 * attach the pci.. note we pass PCI A tags, etc., for the sabre here.
	 */
	pba.pba_flags = sc->sc_psycho_this->pp_flags;
	pba.pba_dmat = sc->sc_psycho_this->pp_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_iot = sc->sc_psycho_this->pp_iot;
	pba.pba_memt = sc->sc_psycho_this->pp_memt;
	pba.pba_pc = pp->pp_pc;

	config_found_ia(self, "pcibus", &pba, psycho_print);
}

static	int
psycho_print(void *aux, const char *p)
{

	if (p == NULL)
		return (UNCONF);
	return (QUIET);
}

static void
psycho_set_intr(struct psycho_softc *sc, int ipl, void *handler,
	uint64_t *mapper, uint64_t *clearer)
{
	struct intrhand *ih;

	ih = (struct intrhand *)malloc(sizeof(struct intrhand),
		M_DEVBUF, M_NOWAIT);
	ih->ih_arg = sc;
	ih->ih_map = mapper;
	ih->ih_clr = clearer;
	ih->ih_fun = handler;
	ih->ih_pil = ipl;
	ih->ih_number = INTVEC(*(ih->ih_map));
	ih->ih_pending = 0;
	intr_establish(ipl, ipl != IPL_VM, ih);
	*(ih->ih_map) |= INTMAP_V|(CPU_UPAID << INTMAP_TID_SHIFT);
}

/*
 * power button handlers
 */
static void
psycho_register_power_button(struct psycho_softc *sc)
{
	sysmon_task_queue_init();

	sc->sc_powerpressed = 0;
	sc->sc_smcontext = malloc(sizeof(struct sysmon_pswitch), M_DEVBUF, 0);
	if (!sc->sc_smcontext) {
		aprint_error_dev(sc->sc_dev, "could not allocate power button context\n");
		return;
	}
	memset(sc->sc_smcontext, 0, sizeof(struct sysmon_pswitch));
	sc->sc_smcontext->smpsw_name = device_xname(sc->sc_dev);
	sc->sc_smcontext->smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(sc->sc_smcontext) != 0)
		aprint_error_dev(sc->sc_dev, "unable to register power button with sysmon\n");
}

static void
psycho_power_button_pressed(void *arg)
{
	struct psycho_softc *sc = arg;

	sysmon_pswitch_event(sc->sc_smcontext, PSWITCH_EVENT_PRESSED);
	sc->sc_powerpressed = 0;
}

/*
 * PCI bus support
 */

/*
 * allocate a PCI chipset tag and set it's cookie.
 */
static pci_chipset_tag_t
psycho_alloc_chipset(struct psycho_pbm *pp, int node, pci_chipset_tag_t pc)
{
	pci_chipset_tag_t npc;
	
	npc = malloc(sizeof *npc, M_DEVBUF, M_NOWAIT);
	if (npc == NULL)
		panic("could not allocate pci_chipset_tag_t");
	memcpy(npc, pc, sizeof *pc);
	npc->cookie = pp;
	npc->rootnode = node;
	npc->spc_conf_read = psycho_pci_conf_read;
	npc->spc_conf_write = psycho_pci_conf_write;
	npc->spc_intr_map = NULL;
	npc->spc_intr_establish = psycho_pci_intr_establish;
	npc->spc_find_ino = psycho_pci_find_ino;

	return (npc);
}

/*
 * create extent for free bus space, then allocate assigned regions.
 */
static struct extent *
psycho_alloc_extent(struct psycho_pbm *pp, int node, int ss, const char *name)
{
	struct psycho_registers *pa = NULL;
	struct psycho_ranges *pr;
	struct extent *ex;
	bus_addr_t baddr, addr;
	bus_size_t bsize, size;
	int i, num;

	/* get bus space size */
	pr = get_psychorange(pp, ss);
	if (pr == NULL) {
		printf("psycho_alloc_extent: get_psychorange failed\n");
		return NULL;
	}
	baddr = 0x00000000;
	bsize = BUS_ADDR(pr->size_hi, pr->size_lo);

	/* get available lists */
	num = 0;
	if (prom_getprop(node, "available", sizeof(*pa), &num, &pa)) {
		printf("psycho_alloc_extent: no \"available\" property\n");
		return NULL;
	}

	/* create extent */
	ex = extent_create(name, baddr, bsize - baddr - 1, 0, 0, EX_NOWAIT);
	if (ex == NULL) {
		printf("psycho_alloc_extent: extent_create failed\n");
		goto ret;
	}

	/* allocate assigned regions */
	for (i = 0; i < num; i++)
		if (((pa[i].phys_hi >> 24) & 0x03) == ss) {
			/* allocate bus space */
			addr = BUS_ADDR(pa[i].phys_mid, pa[i].phys_lo);
			size = BUS_ADDR(pa[i].size_hi, pa[i].size_lo);
			if (extent_alloc_region(ex, baddr, addr - baddr,
						EX_NOWAIT)) {
				printf("psycho_alloc_extent: "
				       "extent_alloc_region %" PRIx64 "-%"
				       PRIx64 " failed\n", baddr, addr);
				extent_destroy(ex);
				ex = NULL;
				goto ret;
			}
			baddr = addr + size;
		}
	/* allocate left region if available */
	if (baddr < bsize)
		if (extent_alloc_region(ex, baddr, bsize - baddr, EX_NOWAIT)) {
			printf("psycho_alloc_extent: extent_alloc_region %"
			       PRIx64 "-%" PRIx64 " failed\n", baddr, bsize);
			extent_destroy(ex);
			ex = NULL;
			goto ret;
		}

#ifdef DEBUG
	/* print extent */
	extent_print(ex);
#endif

ret:
	/* return extent */
	free(pa, M_DEVBUF);
	return ex;
}

/*
 * grovel the OBP for various psycho properties
 */
static void
psycho_get_bus_range(int node, int *brp)
{
	int n, error;

	n = 2;
	error = prom_getprop(node, "bus-range", sizeof(*brp), &n, &brp);
	if (error)
		panic("could not get psycho bus-range, error %d", error);
	if (n != 2)
		panic("broken psycho bus-range");
	DPRINTF(PDB_PROM, ("psycho debug: got `bus-range' for node %08x: %u - %u\n",
			   node, brp[0], brp[1]));
}

static void
psycho_fixup_bus_range(int node0, int *brp0)
{
	int node;
	int len, busrange[2], *brp;

	DPRINTF(PDB_PROM,
	    ("psycho debug: fixing up `bus-range' for node %08x: %u - %u\n",
	    node0, brp0[0], brp0[1]));

	/*
	 * Check all nodes under this one and increase the bus range to
	 * match.  Recurse through PCI-PCI bridges.  Cardbus bridges are
	 * fixed up in pccbb_attach_hook().  Assumes that "bus-range" for
	 * PCI-PCI bridges apart from this one is correct.
	 */
	brp0[1] = brp0[0];
	node = prom_firstchild(node0);
	for (node = ((node)); node; node = prom_nextsibling(node)) {
		len = 2;
		brp = busrange;
		if (prom_getprop(node, "bus-range", sizeof(*brp),
		    &len, &brp) != 0)
			break;
		if (len != 2)
			break;
		psycho_fixup_bus_range(node, busrange);
		if (brp0[0] > busrange[0] && busrange[0] >= 0)
			brp0[0] = busrange[0];
		if (brp0[1] < busrange[1] && busrange[1] < 256)
			brp0[1] = busrange[1];
	}

	DPRINTF(PDB_PROM,
	    ("psycho debug: fixed up `bus-range' for node %08x: %u - %u\n",
	    node0, brp[0], brp[1]));
}

static void
psycho_get_ranges(int node, struct psycho_ranges **rp, int *np)
{

	if (prom_getprop(node, "ranges", sizeof(**rp), np, rp))
		panic("could not get psycho ranges");
	DPRINTF(PDB_PROM, ("psycho debug: got `ranges' for node %08x: %d entries\n", node, *np));
}

/*
 * Interrupt handlers.
 */

static int
psycho_ue(void *arg)
{
	struct psycho_softc *sc = (struct psycho_softc *)arg;
	struct psychoreg *regs = sc->sc_regs;
	struct iommu_state *is = sc->sc_is;
	uint64_t afsr = regs->psy_ue_afsr;
	uint64_t afar = regs->psy_ue_afar;
	psize_t size = PAGE_SIZE << is->is_tsbsize;
	char bits[128];

	/*
	 * It's uncorrectable.  Dump the regs and panic.
	 */
	snprintb(bits, sizeof(bits), PSYCHO_UE_AFSR_BITS, afsr);
	aprint_error_dev(sc->sc_dev,
	    "uncorrectable DMA error AFAR %" PRIx64 " AFSR %s\n", afar, bits);

	/* Sometimes the AFAR points to an IOTSB entry */
	if (afar >= is->is_ptsb && afar < is->is_ptsb + size) {
		aprint_error_dev(sc->sc_dev,
		    "IOVA %" PRIx64 " IOTTE %" PRIx64 "\n",
		    (afar - is->is_ptsb) / sizeof(is->is_tsb[0]) * PAGE_SIZE
		    + is->is_dvmabase, ldxa(afar, ASI_PHYS_CACHED));
	}
#ifdef DDB
	Debugger();
#endif
	regs->psy_ue_afar = 0;
	regs->psy_ue_afsr = 0;
	return (1);
}

static int
psycho_ce(void *arg)
{
	struct psycho_softc *sc = (struct psycho_softc *)arg;
	struct psychoreg *regs = sc->sc_regs;

	/*
	 * It's correctable.  Dump the regs and continue.
	 */
	aprint_error_dev(sc->sc_dev,
	    "correctable DMA error AFAR %" PRIx64 " AFSR %" PRIx64 "\n",
	    regs->psy_ce_afar, regs->psy_ce_afsr);
	return (1);
}

static int
psycho_bus_a(void *arg)
{
	struct psycho_softc *sc = (struct psycho_softc *)arg;
	struct psychoreg *regs = sc->sc_regs;

	/*
	 * It's uncorrectable.  Dump the regs and panic.
	 */

	panic("%s: PCI bus A error AFAR %" PRIx64 " AFSR %" PRIx64,
	    device_xname(sc->sc_dev),
	    regs->psy_pcictl[0].pci_afar, regs->psy_pcictl[0].pci_afsr);
	return (1);
}

static int
psycho_bus_b(void *arg)
{
	struct psycho_softc *sc = (struct psycho_softc *)arg;
	struct psychoreg *regs = sc->sc_regs;

	/*
	 * It's uncorrectable.  Dump the regs and panic.
	 */

	panic("%s: PCI bus B error AFAR %" PRIx64 " AFSR %" PRIx64,
	    device_xname(sc->sc_dev),
	    regs->psy_pcictl[0].pci_afar, regs->psy_pcictl[0].pci_afsr);
	return (1);
}

static int
psycho_powerfail(void *arg)
{
	struct psycho_softc *sc = (struct psycho_softc *)arg;

	/*
	 * We lost power. Queue a callback with thread context to
	 * handle all the real work.
	 */
	if (sc->sc_powerpressed == 0 && sc->sc_smcontext != NULL) {
		sc->sc_powerpressed = 1;
		sysmon_task_queue_sched(0, psycho_power_button_pressed, sc);
	}
	return (1);
}

static
int psycho_wakeup(void *arg)
{
	struct psycho_softc *sc = (struct psycho_softc *)arg;

	/*
	 * Gee, we don't really have a framework to deal with this
	 * properly.
	 */
	aprint_error_dev(sc->sc_dev, "power management wakeup\n");
	return (1);
}


/*
 * initialise the IOMMU..
 */
void
psycho_iommu_init(struct psycho_softc *sc, int tsbsize)
{
	char *name;
	struct iommu_state *is = sc->sc_is;
	uint32_t iobase = -1;
	int *vdma = NULL;
	int nitem;

	/* punch in our copies */
	is->is_bustag = sc->sc_bustag;
	bus_space_subregion(sc->sc_bustag, sc->sc_bh,
		offsetof(struct psychoreg, psy_iommu),
		sizeof (struct iommureg),
		&is->is_iommu);

	/*
	 * Separate the men from the boys.  Get the `virtual-dma'
	 * property for sabre and use that to make sure the damn
	 * iommu works.
	 *
	 * We could query the `#virtual-dma-size-cells' and
	 * `#virtual-dma-addr-cells' and DTRT, but I'm lazy.
	 */
	nitem = 0;
	if (!prom_getprop(sc->sc_node, "virtual-dma", sizeof(vdma), &nitem,
		&vdma)) {
		/* Damn.  Gotta use these values. */
		iobase = vdma[0];
#define	TSBCASE(x)	case 1<<((x)+23): tsbsize = (x); break
		switch (vdma[1]) {
			TSBCASE(1); TSBCASE(2); TSBCASE(3);
			TSBCASE(4); TSBCASE(5); TSBCASE(6);
		default:
			printf("bogus tsb size %x, using 7\n", vdma[1]);
			TSBCASE(7);
		}
#undef TSBCASE
	}

	/* give us a nice name.. */
	name = (char *)malloc(32, M_DEVBUF, M_NOWAIT);
	if (name == 0)
		panic("couldn't malloc iommu name");
	snprintf(name, 32, "%s dvma", device_xname(sc->sc_dev));

	iommu_init(name, is, tsbsize, iobase);
}

/*
 * below here is bus space and bus DMA support
 */
bus_space_tag_t
psycho_alloc_bus_tag(struct psycho_pbm *pp, int type)
{
	struct psycho_softc *sc = pp->pp_sc;
	bus_space_tag_t bt;

	bt = (bus_space_tag_t) malloc(sizeof(struct sparc_bus_space_tag),
		    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (bt == NULL)
		panic("could not allocate psycho bus tag");

	bt->cookie = pp;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = _psycho_bus_map;
	bt->sparc_bus_mmap = psycho_bus_mmap;
	bt->sparc_intr_establish = psycho_intr_establish;
	return (bt);
}

bus_dma_tag_t
psycho_alloc_dma_tag(struct psycho_pbm *pp)
{
	struct psycho_softc *sc = pp->pp_sc;
	bus_dma_tag_t dt, pdt = sc->sc_dmatag;

	dt = (bus_dma_tag_t)
		malloc(sizeof(struct sparc_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (dt == NULL)
		panic("could not allocate psycho DMA tag");

	memset(dt, 0, sizeof *dt);
	dt->_cookie = pp;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	dt->_dmamap_create = psycho_dmamap_create;
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load = iommu_dvmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	dt->_dmamap_load_raw = iommu_dvmamap_load_raw;
	dt->_dmamap_unload = iommu_dvmamap_unload;
	if (sc->sc_mode == PSYCHO_MODE_SABRE)
		dt->_dmamap_sync = psycho_sabre_dmamap_sync;
	else
		dt->_dmamap_sync = iommu_dvmamap_sync;
	dt->_dmamem_alloc = iommu_dvmamem_alloc;
	dt->_dmamem_free = iommu_dvmamem_free;
	dt->_dmamem_map = iommu_dvmamem_map;
	dt->_dmamem_unmap = iommu_dvmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

/*
 * bus space support.  <sparc64/dev/psychoreg.h> has a discussion about
 * PCI physical addresses.
 */

static struct psycho_ranges *
get_psychorange(struct psycho_pbm *pp, int ss)
{
	int i;

	for (i = 0; i < pp->pp_nrange; i++) {
		if (((pp->pp_range[i].cspace >> 24) & 0x03) == ss)
			return (&pp->pp_range[i]);
	}
	/* not found */
	return (NULL);
}

static int
_psycho_bus_map(bus_space_tag_t t, bus_addr_t offset, bus_size_t size,
	int flags, vaddr_t unused, bus_space_handle_t *hp)
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	struct psycho_ranges *pr;
	bus_addr_t paddr;
	int ss;

	DPRINTF(PDB_BUSMAP,
		("_psycho_bus_map: type %d off %qx sz %qx flags %d",
			t->type, (unsigned long long)offset,
			(unsigned long long)size, flags));

	ss = sparc_pci_childspace(t->type);
	DPRINTF(PDB_BUSMAP, (" cspace %d", ss));

	pr = get_psychorange(pp, ss);
	if (pr != NULL) {
		paddr = BUS_ADDR(pr->phys_hi, pr->phys_lo + offset);
		DPRINTF(PDB_BUSMAP, ("\n_psycho_bus_map: mapping paddr "
				     "space %lx offset %lx paddr %qx\n",
			       (long)ss, (long)offset,
			       (unsigned long long)paddr));
		return ((*sc->sc_bustag->sparc_bus_map)(t, paddr, size,
			flags, 0, hp));
	}
	DPRINTF(PDB_BUSMAP, (" FAILED\n"));
	return (EINVAL);
}

static paddr_t
psycho_bus_mmap(bus_space_tag_t t, bus_addr_t paddr, off_t off, int prot,
	int flags)
{
	bus_addr_t offset = paddr;
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	struct psycho_ranges *pr;
	int ss;

	ss = sparc_pci_childspace(t->type);

	DPRINTF(PDB_BUSMAP, ("_psycho_bus_mmap: prot %x flags %d pa %qx\n",
		prot, flags, (unsigned long long)paddr));

	pr = get_psychorange(pp, ss);
	if (pr != NULL) {
		paddr = BUS_ADDR(pr->phys_hi, pr->phys_lo + offset);
		DPRINTF(PDB_BUSMAP, ("\n_psycho_bus_mmap: mapping paddr "
				     "space %lx offset %lx paddr %qx\n",
			       (long)ss, (long)offset,
			       (unsigned long long)paddr));
		return (bus_space_mmap(sc->sc_bustag, paddr, off,
				       prot, flags));
	}

	return (-1);
}

/*
 * Get a PCI offset address from bus_space_handle_t.
 */
bus_addr_t
psycho_bus_offset(bus_space_tag_t t, bus_space_handle_t *hp)
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_ranges *pr;
	bus_addr_t addr, offset;
	vaddr_t va;
	int ss;

	addr = hp->_ptr;
	ss = sparc_pci_childspace(t->type);
	DPRINTF(PDB_BUSMAP, ("psycho_bus_offset: type %d addr %" PRIx64
			     " cspace %d", t->type, addr, ss));

	pr = get_psychorange(pp, ss);
	if (pr != NULL) {
		if (!PHYS_ASI(hp->_asi)) {
			va = trunc_page((vaddr_t)addr);
			if (pmap_extract(pmap_kernel(), va, &addr) == FALSE) {
				DPRINTF(PDB_BUSMAP,
					("\n pmap_extract FAILED\n"));
				return (-1);
			}
			addr += hp->_ptr & PGOFSET;
		}
		offset = BUS_ADDR_PADDR(addr) - pr->phys_lo;
		DPRINTF(PDB_BUSMAP, ("\npsycho_bus_offset: paddr %" PRIx64
				     " offset %" PRIx64 "\n", addr, offset));
		return (offset);
	}
	DPRINTF(PDB_BUSMAP, ("\n FAILED\n"));
	return (-1);
}


/*
 * install an interrupt handler for a PCI device
 */
void *
psycho_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	struct intrhand *ih;
	volatile uint64_t *intrmapptr = NULL, *intrclrptr = NULL;
	int64_t imap = 0;
	int ino;
	long vec = INTVEC(ihandle);

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_ivec = ihandle;

	/*
	 * Hunt through all the interrupt mapping regs to look for our
	 * interrupt vector.
	 *
	 * XXX We only compare INOs rather than IGNs since the firmware may
	 * not provide the IGN and the IGN is constant for all device on that
	 * PCI controller.  This could cause problems for the FFB/external
	 * interrupt which has a full vector that can be set arbitrarily.
	 */

	DPRINTF(PDB_INTR, ("\npsycho_intr_establish: ihandle %x vec %lx", ihandle, vec));
	ino = INTINO(vec);
	DPRINTF(PDB_INTR, (" ino %x", ino));

	/* If the device didn't ask for an IPL, use the one encoded. */
	if (level == IPL_NONE) level = INTLEV(vec);
	/* If it still has no level, print a warning and assign IPL 2 */
	if (level == IPL_NONE) {
		printf("ERROR: no IPL, setting IPL 2.\n");
		level = 2;
	}

	DPRINTF(PDB_INTR, ("\npsycho: intr %lx: %p\nHunting for IRQ...\n",
	    (long)ino, intrlev[ino]));

 	/*
 	 * First look for PCI interrupts, otherwise the PCI A slot 0
 	 * INTA# interrupt might match an unused non-PCI (obio)
 	 * interrupt.
 	 */
	for (intrmapptr = &sc->sc_regs->pcia_slot0_int,
		     intrclrptr = &sc->sc_regs->pcia0_clr_int[0];
	     intrmapptr <= &sc->sc_regs->pcib_slot3_int;
	     intrmapptr++, intrclrptr += 4) {
		if (sc->sc_mode == PSYCHO_MODE_PSYCHO &&
		    (intrmapptr == &sc->sc_regs->pcia_slot2_int ||
		     intrmapptr == &sc->sc_regs->pcia_slot3_int))
			continue;
		if (((*intrmapptr ^ vec) & 0x3c) == 0) {
			intrclrptr += vec & 0x3;
			goto found;
		}
	}

	/* Now hunt thru obio. */
	for (intrmapptr = &sc->sc_regs->scsi_int_map,
		     intrclrptr = &sc->sc_regs->scsi_clr_int;
	     intrmapptr < &sc->sc_regs->ue_int_map;
	     intrmapptr++, intrclrptr++) {
		if (INTINO(*intrmapptr) == ino)
			goto found;
	}

	/* Finally check the two FFB slots */
	intrclrptr = NULL; /* XXX? */
	for (intrmapptr = &sc->sc_regs->ffb0_int_map;
	     intrmapptr <= &sc->sc_regs->ffb1_int_map;
	     intrmapptr++) {
		if (INTVEC(*intrmapptr) == ino)
			goto found;
	}

	printf("Cannot find interrupt vector %lx\n", vec);
	return (NULL);

found:
	/* Register the map and clear intr registers */
	ih->ih_map = intrmapptr;
	ih->ih_clr = intrclrptr;

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino | sc->sc_ign;
	ih->ih_pending = 0;

	DPRINTF(PDB_INTR, (
	    "; installing handler %p arg %p with ino %u pil %u\n",
	    handler, arg, (u_int)ino, (u_int)ih->ih_pil));

	intr_establish(ih->ih_pil, level != IPL_VM, ih);

	/*
	 * Enable the interrupt now we have the handler installed.
	 * Read the current value as we can't change it besides the
	 * valid bit so so make sure only this bit is changed.
	 *
	 * XXXX --- we really should use bus_space for this.
	 */
	if (intrmapptr) {
		imap = *intrmapptr;
		DPRINTF(PDB_INTR, ("; read intrmap = %016qx",
			(unsigned long long)imap));

		/* Enable the interrupt */
		imap |= INTMAP_V|(CPU_UPAID << INTMAP_TID_SHIFT);
		DPRINTF(PDB_INTR, ("; addr of intrmapptr = %p", intrmapptr));
		DPRINTF(PDB_INTR, ("; writing intrmap = %016qx\n",
			(unsigned long long)imap));
		*intrmapptr = imap;
		DPRINTF(PDB_INTR, ("; reread intrmap = %016qx",
			(unsigned long long)(imap = *intrmapptr)));
	}
 	if (intrclrptr) {
 		/* set state to IDLE */
 		*intrclrptr = 0;
 	}
	return (ih);
}

/*
 * per-controller driver calls
 */

/* assume we are mapped little-endian/side-effect */
static pcireg_t
psycho_pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct psycho_pbm *pp = pc->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	struct cpu_info *ci = curcpu();
	pcireg_t val = (pcireg_t)~0;
	int s;

	DPRINTF(PDB_CONF, ("%s: tag %lx reg %x ", __func__,
		(long)tag, reg));
	if (PCITAG_NODE(tag) != -1) {

		DPRINTF(PDB_CONF, ("asi=%x addr=%qx (offset=%x) ...",
			sc->sc_configaddr._asi,
			(long long)(sc->sc_configaddr._ptr +
				PCITAG_OFFSET(tag) + reg),
			(int)PCITAG_OFFSET(tag) + reg));

		s = splhigh();
		ci->ci_pci_probe = true;
		membar_Sync();
		val = bus_space_read_4(sc->sc_configtag, sc->sc_configaddr,
			PCITAG_OFFSET(tag) + reg);
		membar_Sync();
		if (ci->ci_pci_fault)
			val = (pcireg_t)~0;
		ci->ci_pci_probe = ci->ci_pci_fault = false;
		splx(s);
	}
#ifdef DEBUG
	else DPRINTF(PDB_CONF, ("%s: bogus pcitag %x\n", __func__,
		(int)PCITAG_OFFSET(tag)));
#endif
	DPRINTF(PDB_CONF, (" returning %08x\n", (u_int)val));

	return (val);
}

static void
psycho_pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct psycho_pbm *pp = pc->cookie;
	struct psycho_softc *sc = pp->pp_sc;

	DPRINTF(PDB_CONF, ("%s: tag %lx; reg %x; data %x; ", __func__,
		(long)PCITAG_OFFSET(tag), reg, (int)data));
	DPRINTF(PDB_CONF, ("asi = %x; readaddr = %qx (offset = %x)\n",
		sc->sc_configaddr._asi,
		(long long)(sc->sc_configaddr._ptr + PCITAG_OFFSET(tag) + reg),
		(int)PCITAG_OFFSET(tag) + reg));

	/* If we don't know it, just punt it.  */
	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(PDB_CONF, ("%s: bad addr", __func__));
		return;
	}
		
	bus_space_write_4(sc->sc_configtag, sc->sc_configaddr,
		PCITAG_OFFSET(tag) + reg, data);
}

static void *
psycho_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
	int (*func)(void *), void *arg)
{
	void *cookie;
	struct psycho_pbm *pp = (struct psycho_pbm *)pc->cookie;

	DPRINTF(PDB_INTR, ("%s: ih %lx; level %d", __func__, (u_long)ih, level));
	cookie = bus_intr_establish(pp->pp_memt, ih, level, func, arg);

	DPRINTF(PDB_INTR, ("; returning handle %p\n", cookie));
	return (cookie);
}

static int
psycho_pci_find_ino(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct psycho_pbm *pp = pa->pa_pc->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	u_int bus;
	u_int dev;
	u_int pin;

	DPRINTF(PDB_INTMAP, ("%s: pa_tag: node %x, %d:%d:%d\n", __func__,
			      PCITAG_NODE(pa->pa_tag), (int)PCITAG_BUS(pa->pa_tag),
			      (int)PCITAG_DEV(pa->pa_tag),
			      (int)PCITAG_FUN(pa->pa_tag)));
	DPRINTF(PDB_INTMAP,
		("%s: intrswiz %d, intrpin %d, intrline %d, rawintrpin %d\n", __func__,
		 pa->pa_intrswiz, pa->pa_intrpin, pa->pa_intrline, pa->pa_rawintrpin));
	DPRINTF(PDB_INTMAP, ("%s: pa_intrtag: node %x, %d:%d:%d\n", __func__,
			      PCITAG_NODE(pa->pa_intrtag),
			      (int)PCITAG_BUS(pa->pa_intrtag),
			      (int)PCITAG_DEV(pa->pa_intrtag),
			      (int)PCITAG_FUN(pa->pa_intrtag)));

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
	DPRINTF(PDB_INTMAP, ("%s: mode %d, pbm %d, dev %d, pin %d\n", __func__,
	    sc->sc_mode, pp->pp_id, dev, pin));

	*ihp = sc->sc_ign | ((bus << 4) & INTMAP_PCIBUS) |
	    ((dev << 2) & INTMAP_PCISLOT) | (pin & INTMAP_PCIINT);

	return (0);
}

/*
 * hooks into the iommu dvma calls.
 */
static int
psycho_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
	bus_size_t maxsegsz, bus_size_t boundary, int flags,
	bus_dmamap_t *dmamp)
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	int error;

	error = bus_dmamap_create(t->_parent, size, nsegments, maxsegsz,
				  boundary, flags, dmamp);
	if (error == 0)
		(*dmamp)->_dm_cookie = &pp->pp_sb;
	return error;
}

/*
 * UltraSPARC IIi and IIe have no streaming buffers, but have PCI DMA
 * Write Synchronization Register (see UltraSPARC-IIi User's Manual
 * section 19.3.0.5).  So use it to synchronize with the DMA writes.
 */
static void
psycho_sabre_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
	bus_size_t len, int ops)
{
	struct psycho_pbm *pp;
	struct psycho_softc *sc;

	/* If len is 0, then there is nothing to do. */
	if (len == 0)
		return;

	if (ops & BUS_DMASYNC_POSTREAD) {
		pp = (struct psycho_pbm *)t->_cookie;
		sc = pp->pp_sc;
		bus_space_read_8(sc->sc_bustag, sc->sc_bh,
		    offsetof(struct psychoreg, pci_dma_write_sync));
	}
	bus_dmamap_sync(t->_parent, map, offset, len, ops);
}

/* US-IIe STICK support */

uint64_t
psycho_getstick(void)
{
	uint64_t stick;

	stick = bus_space_read_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CNT_LOW) |
	    (bus_space_read_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CNT_HIGH) & 0x7fffffff) << 32;
	return stick;
}

uint32_t
psycho_getstick32(void)
{

	return bus_space_read_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CNT_LOW);
}

void
psycho_setstick(long cnt)
{

	/*
	 * looks like we can't actually write the STICK counter, so instead we
	 * prepare sc_last_stick for the coming interrupt setup
	 */
#if 0
	bus_space_write_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CNT_HIGH, (cnt >> 32));
	bus_space_write_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CNT_LOW, (uint32_t)(cnt & 0xffffffff));
#endif

	if (cnt == 0) {
		bus_space_write_8(psycho0->sc_bustag, psycho0->sc_bh,
		    STICK_CMP_HIGH, 0);
		bus_space_write_8(psycho0->sc_bustag, psycho0->sc_bh,
		    STICK_CMP_LOW, 0);
		psycho0->sc_last_stick = 0;
	}

	psycho0->sc_last_stick = psycho_getstick();
	DPRINTF(PDB_STICK, ("stick: %ld\n", psycho0->sc_last_stick));
}

void
psycho_nextstick(long diff)
{
	uint64_t cmp, now;

	/*
	 * there is no way we'll ever overflow
	 * the counter is 63 bits wide, at 12MHz that's >24000 years
	 */
	now = psycho_getstick() + 1000;
	cmp = psycho0->sc_last_stick;
	
	while (cmp < now)
		cmp += diff;
	
	bus_space_write_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CMP_HIGH, (cmp >> 32) & 0x7fffffff);
	bus_space_write_8(psycho0->sc_bustag, psycho0->sc_bh,
	    STICK_CMP_LOW, (cmp & 0xffffffff));
	
	psycho0->sc_last_stick = cmp;
}
