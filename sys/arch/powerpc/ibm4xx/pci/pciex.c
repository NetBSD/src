/*	$NetBSD: pciex.c,v 1.1 2026/06/14 00:02:35 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
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

/*
 * IBM PLB-PCIE root complex 
 * as found on the 440SPe/460EX family of SoCs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciex.c,v 1.1 2026/06/14 00:02:35 rkujawa Exp $");

#ifdef _KERNEL_OPT
#include "opt_pci.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>

#include <powerpc/pcb.h>

#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/pci_machdep.h>
#include <powerpc/pci_machdep.h>

#define	PCIEX_NPORTS	2

/* PEGPL DCR offsets */
#define	PEGPL_CFGBAH	0x00
#define	PEGPL_CFGBAL	0x01
#define	PEGPL_CFGMSK	0x02
#define	PEGPL_OMR1BAH	0x06
#define	PEGPL_OMR1BAL	0x07
#define	PEGPL_OMR1MSKH	0x08
#define	PEGPL_OMR1MSKL	0x09

struct pciex_softc {
	struct genppc_pci_chipset sc_pc;	/* must be first */
	device_t sc_dev;
	int sc_port;
	bus_space_handle_t sc_cfgh;
};

static int	pciex_match(device_t, cfdata_t, void *);
static void	pciex_attach(device_t, device_t, void *);
static int	pciex_print(void *, const char *);

CFATTACH_DECL_NEW(pciex, sizeof(struct pciex_softc),
    pciex_match, pciex_attach, NULL, NULL);

static pcireg_t	pciex_conf_read(void *, pcitag_t, int);
static void	pciex_conf_write(void *, pcitag_t, int, pcireg_t);
static void	pciex_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
static int	pciex_conf_hook(void *, int, int, int, pcireg_t);
static int	pciex_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static void	pciex_conf_interrupt(void *, int, int, int, int, int *);

/* ECAM config windows */
static struct powerpc_bus_space pciex_cfg_tag[PCIEX_NPORTS] = {
	{
		_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
		0x00000000,
		AMCC460EX_PCIE0_CFG_PLBA,
		AMCC460EX_PCIE0_CFG_PLBA + AMCC460EX_PCIE_CFG_SIZE,
	},
	{
		_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
		0x00000000,
		AMCC460EX_PCIE1_CFG_PLBA,
		AMCC460EX_PCIE1_CFG_PLBA + AMCC460EX_PCIE_CFG_SIZE,
	},
};

static struct powerpc_bus_space pciex_mem_tag[PCIEX_NPORTS] = {
	{
		_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
		AMCC460EX_PCIE0_MEM_PLBA - AMCC460EX_PCIE_MEM_BASE,
		AMCC460EX_PCIE_MEM_BASE,
		AMCC460EX_PCIE_MEM_BASE + AMCC460EX_PCIE_MEM_SIZE,
	},
	{
		_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
		AMCC460EX_PCIE1_MEM_PLBA - AMCC460EX_PCIE_MEM_BASE,
		AMCC460EX_PCIE_MEM_BASE,
		AMCC460EX_PCIE_MEM_BASE + AMCC460EX_PCIE_MEM_SIZE,
	},
};

static const struct genppc_pci_chipset pciex_chipset_template = {
	.pc_conf_v =		NULL,		/* set to softc */
	.pc_attach_hook =	pciex_attach_hook,
	.pc_bus_maxdevs =	ibm4xx_pci_bus_maxdevs,
	.pc_make_tag =		ibm4xx_pci_make_tag,
	.pc_conf_read =		pciex_conf_read,
	.pc_conf_write =	pciex_conf_write,

	.pc_intr_v =		NULL,		/* set to softc */
	.pc_intr_map =		pciex_intr_map,
	.pc_intr_string =	genppc_pci_intr_string,
	.pc_intr_evcnt =	genppc_pci_intr_evcnt,
	.pc_intr_establish =	genppc_pci_intr_establish,
	.pc_intr_disestablish =	genppc_pci_intr_disestablish,
	.pc_intr_setattr =	ibm4xx_pci_intr_setattr,
	.pc_intr_type =		genppc_pci_intr_type,
	.pc_intr_alloc =	genppc_pci_intr_alloc,
	.pc_intr_release =	genppc_pci_intr_release,
	.pc_intx_alloc =	genppc_pci_intx_alloc,

	.pc_msi_v =		NULL,		/* set to softc */
	GENPPC_PCI_MSI_INITIALIZER,

	.pc_msix_v =		NULL,		/* set to softc */
	GENPPC_PCI_MSIX_INITIALIZER,

	.pc_conf_interrupt =	pciex_conf_interrupt,
	.pc_decompose_tag =	ibm4xx_pci_decompose_tag,
	.pc_conf_hook =		pciex_conf_hook,
};

static int
pciex_port_inta(int port)
{

	return port == 0 ? AMCC460EX_PCIE0_INTA_IRQ : AMCC460EX_PCIE1_INTA_IRQ;
}

static void
pciex_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
pciex_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{

	return PCI_CONF_DEFAULT;
}

/*
 * INTA-INTD are wired to consecutive UIC3 inputs per port.  
 * At least on 460EX, this may or may not be true for other SoCs...
 */
static int
pciex_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct pciex_softc *sc = pa->pa_pc->pc_intr_v;

	if (pa->pa_intrpin == 0 || pa->pa_intrpin > 4)
		return 1;
	*ihp = pciex_port_inta(sc->sc_port) + pa->pa_intrpin - 1;
	return 0;
}

static void
pciex_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{
	struct pciex_softc *sc = v;

	*iline = pciex_port_inta(sc->sc_port) + (pin - 1 + swiz + dev) % 4;
}

/*
 * Config access: ECAM offset is the standard tag (bus<<16|dev<<11|
 * func<<8) shifted left by 4. 
 */
static pcireg_t
pciex_conf_read(void *v, pcitag_t tag, int reg)
{
	struct pciex_softc *sc = v;
	struct faultbuf env;
	pcireg_t data;
	int bus;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;
	ibm4xx_pci_decompose_tag(v, tag, &bus, NULL, NULL);
	if (bus >= AMCC460EX_PCIE_CFG_SIZE >> 20)
		return (pcireg_t) -1;

	if (setfault(&env)) {
		curpcb->pcb_onfault = NULL;
		return (pcireg_t) -1;
	}
	data = bus_space_read_4(&pciex_cfg_tag[sc->sc_port], sc->sc_cfgh,
	    (tag << 4) | reg);
	curpcb->pcb_onfault = NULL;
	return data;
}

static void
pciex_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	struct pciex_softc *sc = v;
	struct faultbuf env;
	int bus;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;
	ibm4xx_pci_decompose_tag(v, tag, &bus, NULL, NULL);
	if (bus >= AMCC460EX_PCIE_CFG_SIZE >> 20)
		return;

	if (setfault(&env)) {
		curpcb->pcb_onfault = NULL;
		return;
	}
	bus_space_write_4(&pciex_cfg_tag[sc->sc_port], sc->sc_cfgh,
	    (tag << 4) | reg, data);
	curpcb->pcb_onfault = NULL;
}

/*
 * Program the PEGPL config and outbound memory windows.
 * mtdcr() needs compile-time constant DCR numbers.
 */
#define	PCIEX_PROGRAM_PORT(base, cfg_plba, mem_plba)			\
do {									\
	mtdcr((base) + PEGPL_CFGMSK, 0);				\
	mtdcr((base) + PEGPL_CFGBAH, AMCC460EX_PCIE_CFG_PA_HIGH);	\
	mtdcr((base) + PEGPL_CFGBAL, (cfg_plba));			\
	mtdcr((base) + PEGPL_CFGMSK,				\
	    ~(AMCC460EX_PCIE_CFG_SIZE - 1) | 1);			\
	mtdcr((base) + PEGPL_OMR1BAH, AMCC460EX_PCIE_MEM_PA_HIGH);	\
	mtdcr((base) + PEGPL_OMR1BAL, (mem_plba));			\
	mtdcr((base) + PEGPL_OMR1MSKH, 0x7fffffff);			\
	mtdcr((base) + PEGPL_OMR1MSKL,				\
	    ~(AMCC460EX_PCIE_MEM_SIZE - 1) | 3);			\
} while (0)

static void
pciex_setup_windows(int port)
{

	if (port == 0)
		PCIEX_PROGRAM_PORT(AMCC460EX_PCIE0_DCR_BASE,
		    AMCC460EX_PCIE0_CFG_PLBA, AMCC460EX_PCIE0_MEM_PLBA);
	else
		PCIEX_PROGRAM_PORT(AMCC460EX_PCIE1_DCR_BASE,
		    AMCC460EX_PCIE1_CFG_PLBA, AMCC460EX_PCIE1_MEM_PLBA);
}

static int
pciex_match(device_t parent, cfdata_t cf, void *aux)
{
	struct plb_attach_args *paa = aux;

	if (strcmp(paa->plb_name, cf->cf_name) != 0)
		return 0;
	if (cf->cf_unit >= PCIEX_NPORTS)
		return 0;

	return 1;
}

static void
pciex_attach(device_t parent, device_t self, void *aux)
{
	struct pciex_softc *sc = device_private(self);
	struct plb_attach_args *paa = aux;
	struct pcibus_attach_args pba;
	pci_chipset_tag_t pc;

	sc->sc_dev = self;
	sc->sc_port = device_unit(self);
	sc->sc_pc = pciex_chipset_template;
	sc->sc_pc.pc_conf_v = sc;
	sc->sc_pc.pc_intr_v = sc;
	sc->sc_pc.pc_msi_v = sc;
	sc->sc_pc.pc_msix_v = sc;
	pc = &sc->sc_pc;

	aprint_normal(": PLB-PCIE root complex, port %d\n", sc->sc_port);

	/*
	 * Touch the port only if firmware trained the link 
	 */
	if ((mfsdr(sc->sc_port ? AMCC460EX_PESDR1_LOOP :
	    AMCC460EX_PESDR0_LOOP) & AMCC460EX_PESDR_LOOP_LNKUP) == 0) {
		aprint_normal_dev(self,
		    "link is not up, port not configured by firmware\n");
		return;
	}

	if (bus_space_init(&pciex_cfg_tag[sc->sc_port], "pciexcfg", NULL, 0) ||
	    bus_space_map(&pciex_cfg_tag[sc->sc_port],
	      pciex_cfg_tag[sc->sc_port].pbs_base, AMCC460EX_PCIE_CFG_SIZE,
	      0, &sc->sc_cfgh)) {
		aprint_error_dev(self, "can't map config window\n");
		return;
	}
	if (bus_space_init(&pciex_mem_tag[sc->sc_port], "pciexmem", NULL, 0)) {
		aprint_error_dev(self, "can't init MEM tag\n");
		return;
	}

	pciex_setup_windows(sc->sc_port);

#ifdef PCI_NETBSD_CONFIGURE
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    AMCC460EX_PCIE_MEM_BASE, AMCC460EX_PCIE_MEM_SIZE);

	pci_configure_bus(pc, pcires, 0, 32);
	pciconf_resource_fini(pcires);
#endif

	pba.pba_iot = NULL;
	pba.pba_memt = &pciex_mem_tag[sc->sc_port];
	pba.pba_dmat = paa->plb_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY;
	config_found(self, &pba, pciex_print, CFARGS_NONE);
}

static int
pciex_print(void *aux, const char *p)
{

	if (p == NULL)
		return UNCONF;
	return QUIET;
}
