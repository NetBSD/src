/*	$NetBSD: ifpga.c,v 1.18 2004/04/24 15:49:00 kleink Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Integrator FPGA core logic support.
 *
 * The integrator board supports the core logic in an FPGA which is loaded
 * at POR with a custom design.  This code supports the default logic as the
 * board is shipped.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ifpga.c,v 1.18 2004/04/24 15:49:00 kleink Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <machine/intr.h>

#include <arm/cpufunc.h>

#include "opt_pci.h"
#include "pci.h"

#include <evbarm/ifpga/ifpgamem.h>
#include <evbarm/ifpga/ifpgavar.h>
#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpga_pcivar.h>
#include <evbarm/dev/v360reg.h>

#include <evbarm/integrator/int_bus_dma.h>
#include "locators.h"

/* Prototypes */
static int  ifpga_match		(struct device *, struct cfdata *, void *);
static void ifpga_attach	(struct device *, struct device *, void *);
static int  ifpga_print		(void *, const char *);
static int  ifpga_pci_print	(void *, const char *);

/* Drive and attach structures */
CFATTACH_DECL(ifpga, sizeof(struct ifpga_softc),
    ifpga_match, ifpga_attach, NULL, NULL);

int ifpga_found;

/* Default UART clock speed (we should make this a boot option).  */
int ifpga_uart_clk = IFPGA_UART_CLK;

#if NPCI > 0
/* PCI handles */
extern struct arm32_pci_chipset ifpga_pci_chipset;
extern struct arm32_bus_dma_tag ifpga_pci_bus_dma_tag;

static struct bus_space ifpga_pci_io_tag;
static struct bus_space ifpga_pci_mem_tag;
#endif /* NPCI > 0 */

static struct bus_space ifpga_bs_tag;

struct ifpga_softc *ifpga_sc;
/*
 * Print the configuration information for children
 */

static int
ifpga_print(void *aux, const char *pnp)
{
	struct ifpga_attach_args *ifa = aux;

	if (ifa->ifa_addr != -1)
		aprint_normal(" addr 0x%lx", (unsigned long)ifa->ifa_addr);
	if (ifa->ifa_irq != -1)
		aprint_normal(" irq %d", ifa->ifa_irq);

	return UNCONF;
}

#if NPCI > 0
static int
ifpga_pci_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pci_pba = (struct pcibus_attach_args *)aux;

	if (pnp)
		aprint_normal("%s at %s", pci_pba->pba_busname, pnp);
	if (strcmp(pci_pba->pba_busname, "pci") == 0)
		aprint_normal(" bus %d", pci_pba->pba_bus);

	return UNCONF;
}
#endif

static int
ifpga_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ifpga_softc *sc = (struct ifpga_softc *)parent;
	struct ifpga_attach_args ifa;
	int tryagain;

	do {
		ifa.ifa_name = "ifpga_periph";
		ifa.ifa_iot = sc->sc_iot;
		ifa.ifa_addr = cf->cf_iobase;
		ifa.ifa_irq = cf->cf_irq;
		ifa.ifa_sc_ioh = sc->sc_sc_ioh;

		tryagain = 0;
		if (config_match(parent, cf, &ifa) > 0) {
			config_attach(parent, cf, &ifa, ifpga_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return 0;
}

static int
ifpga_match(struct device *parent, struct cfdata *cf, void *aux)
{
#if 0
	struct mainbus_attach_args *ma = aux;

	/* Make sure that we're looking for the IFPGA.  */
	if (strcmp(ma->ma_name, ifpga_md.md_name))
		return 0;
#endif

	/* We can only have one instance of the IFPGA.  */
	if (ifpga_found)
		return 0;

	return 1;
}

static void
ifpga_attach(struct device *parent, struct device *self, void *aux)
{
	struct ifpga_softc *sc = (struct ifpga_softc *)self;
	u_int id, sysclk;
#if defined(PCI_NETBSD_CONFIGURE) && NPCI > 0
	struct extent *ioext, *memext, *pmemext;
	struct ifpga_pci_softc *pci_sc;
	struct pcibus_attach_args pci_pba;
#endif

	ifpga_found = 1;

	/* We want a memory-mapped bus space, since the I/O space is sparse. */
	ifpga_create_mem_bs_tag(&ifpga_bs_tag, (void *)IFPGA_IO_BASE);

#if NPCI > 0
	/* But the PCI config space is quite large, so we have a linear region
	   for that pre-allocated.  */

	ifpga_create_io_bs_tag(&ifpga_pci_io_tag, (void *)IFPGA_PCI_IO_VBASE);
	ifpga_create_mem_bs_tag(&ifpga_pci_mem_tag, (void *)0);
#endif

	sc->sc_iot = &ifpga_bs_tag;

	ifpga_sc = sc;

	/* Now map in the IFPGA motherboard registers.  */
	if (bus_space_map(sc->sc_iot, IFPGA_IO_SC_BASE, IFPGA_IO_SC_SIZE, 0,
	    &sc->sc_sc_ioh))
		panic("%s: Cannot map system controller registers", 
		    self->dv_xname);

	id = bus_space_read_4(sc->sc_iot, sc->sc_sc_ioh, IFPGA_SC_ID);

	printf(": Build %d, ", (id & IFPGA_SC_ID_BUILD_MASK) >>
	    IFPGA_SC_ID_BUILD_SHIFT);
	switch (id & IFPGA_SC_ID_REV_MASK)
	{
	case IFPGA_SC_ID_REV_A:
		printf("Rev A, ");
		break;
	case IFPGA_SC_ID_REV_B:
		printf("Rev B, ");
		break;
	}

	printf("Manufacturer ");
	switch (id & IFPGA_SC_ID_MAN_MASK)
	{
	case IFPGA_SC_ID_MAN_ARM:
		printf("ARM Ltd,");
		break;
	default:
		printf("Unknown,");
		break;
	}

	switch (id & IFPGA_SC_ID_ARCH_MASK)
	{
	case IFPGA_SC_ID_ARCH_ASBLE:
		printf(" ASB, Little-endian,");
		break;
	case IFPGA_SC_ID_ARCH_AHBLE:
		printf(" AHB, Little-endian,");
		break;
	default:
		panic(" Unsupported bus");
	}

	printf("\n%s: FPGA ", self->dv_xname);

	switch (id & IFPGA_SC_ID_FPGA_MASK)
	{
	case IFPGA_SC_ID_FPGA_XC4062:
		printf("XC4062");
		break;
	case IFPGA_SC_ID_FPGA_XC4085:
		printf("XC4085");
		break;
	default:
		printf("unknown");
		break;
	}

	sysclk = bus_space_read_1(sc->sc_iot, sc->sc_sc_ioh, IFPGA_SC_OSC);
	sysclk &= IFPGA_SC_OSC_S_VDW;
	sysclk += 8;

	printf(", SYSCLK %d.%02dMHz", sysclk >> 2, (sysclk & 3) * 25);

	/* Map the Interrupt controller */
	if (bus_space_map(sc->sc_iot, IFPGA_IO_IRQ_BASE, IFPGA_IO_IRQ_SIZE, 
	    BUS_SPACE_MAP_LINEAR, &sc->sc_irq_ioh))
		panic("%s: Cannot map irq controller registers",
		    self->dv_xname);

	/* We can write to the IRQ/FIQ controller now.  */
	ifpga_intr_postinit();

	/* Map the core module */
	if (bus_space_map(sc->sc_iot, IFPGA_IO_CM_BASE, IFPGA_IO_CM_SIZE, 0,
	    &sc->sc_cm_ioh))
		panic("%s: Cannot map core module registers", self->dv_xname);

	/* Map the timers */
	if (bus_space_map(sc->sc_iot, IFPGA_IO_TMR_BASE, IFPGA_IO_TMR_SIZE, 0,
	    &sc->sc_tmr_ioh))
		panic("%s: Cannot map timer registers", self->dv_xname);

	printf("\n");

#if NPCI > 0
	pci_sc = malloc(sizeof(struct ifpga_pci_softc), M_DEVBUF, M_WAITOK);
	pci_sc->sc_iot = &ifpga_pci_io_tag;
	pci_sc->sc_memt = &ifpga_pci_mem_tag;

	if (bus_space_map(pci_sc->sc_iot, 0, IFPGA_PCI_IO_VSIZE, 0,
	    &pci_sc->sc_io_ioh)
	    || bus_space_map(pci_sc->sc_iot,
	    IFPGA_PCI_CONF_VBASE - IFPGA_PCI_IO_VBASE, IFPGA_PCI_CONF_VSIZE, 0,
	    &pci_sc->sc_conf_ioh)
	    || bus_space_map(pci_sc->sc_memt, IFPGA_V360_REG_BASE,
	    IFPGA_V360_REG_SIZE, 0, &pci_sc->sc_reg_ioh))
		panic("%s: Cannot map pci memory", self->dv_xname);

	{
		pcireg_t id_reg, class_reg;
		char buf[1000];

		id_reg = bus_space_read_4(pci_sc->sc_memt, pci_sc->sc_reg_ioh,
		    V360_PCI_VENDOR);
		class_reg = bus_space_read_4(pci_sc->sc_memt,
		    pci_sc->sc_reg_ioh, V360_PCI_CC_REV);

		pci_devinfo(id_reg, class_reg, 1, buf, sizeof(buf));
		printf("%s: %s\n", self->dv_xname, buf);
	}

#if defined(PCI_NETBSD_CONFIGURE)
	ioext = extent_create("pciio", 0x00000000,
	    0x00000000 + IFPGA_PCI_IO_VSIZE, M_DEVBUF, NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", IFPGA_PCI_APP0_BASE,
	    IFPGA_PCI_APP0_BASE + IFPGA_PCI_APP0_SIZE,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	pmemext = extent_create("pcipmem", IFPGA_PCI_APP1_BASE,
	    IFPGA_PCI_APP1_BASE + IFPGA_PCI_APP1_SIZE,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	ifpga_pci_chipset.pc_conf_v = (void *)pci_sc;
	pci_configure_bus(&ifpga_pci_chipset, ioext, memext, pmemext, 0,
	    arm_dcache_align);
	extent_destroy(pmemext);
	extent_destroy(memext);
	extent_destroy(ioext);

	printf("pci_configure_bus done\n");
#endif /* PCI_NETBSD_CONFIGURE */
#endif /* NPCI > 0 */

	/* Finally, search for children.  */
	config_search(ifpga_search, self, NULL);

#if NPCI > 0
	integrator_pci_dma_init(&ifpga_pci_bus_dma_tag);

	pci_pba.pba_busname = "pci";
	pci_pba.pba_pc = &ifpga_pci_chipset;
	pci_pba.pba_iot = &ifpga_pci_io_tag;
	pci_pba.pba_memt = &ifpga_pci_mem_tag;
	pci_pba.pba_dmat = &ifpga_pci_bus_dma_tag;
	pci_pba.pba_dmat64 = NULL;
	pci_pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pci_pba.pba_bus = 0;
	pci_pba.pba_bridgetag = NULL;
	
	config_found(self, &pci_pba, ifpga_pci_print);
#endif
}

void
ifpga_reset(void)
{
	bus_space_write_1(ifpga_sc->sc_iot, ifpga_sc->sc_sc_ioh,
	    IFPGA_SC_CTRLS, IFPGA_SC_CTRL_SOFTRESET);
}
