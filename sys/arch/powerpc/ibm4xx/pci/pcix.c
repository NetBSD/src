/*	$NetBSD: pcix.c,v 1.2 2026/06/17 15:08:54 rkujawa Exp $	*/

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
 * IBM PLB-PCIX host bridge, as found on the 440GP/440SPe/460EX
 *
 * XXX The bridge addresses are currently straight from 460EX include
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcix.c,v 1.2 2026/06/17 15:08:54 rkujawa Exp $");

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

#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/pci_machdep.h>
#include <powerpc/pci_machdep.h>

/* Internal register offsets (relative to the internal register base) */
#define	PCIX0_POM0LAL	0x68
#define	PCIX0_POM0LAH	0x6c
#define	PCIX0_POM0SA	0x70
#define	PCIX0_POM0PCIAL	0x74
#define	PCIX0_POM0PCIAH	0x78
#define	PCIX0_POM1LAL	0x7c
#define	PCIX0_POM1LAH	0x80
#define	PCIX0_POM1SA	0x84
#define	PCIX0_POM1PCIAL	0x88
#define	PCIX0_POM1PCIAH	0x8c
#define	PCIX0_POM2SA	0x90
#define	PCIX0_PIM0SAL	0x98
#define	PCIX0_PIM0LAL	0x9c
#define	PCIX0_PIM0LAH	0xa0
#define	PCIX0_PIM1SA	0xa4
#define	PCIX0_PIM1LAL	0xa8
#define	PCIX0_PIM1LAH	0xac
#define	PCIX0_PIM2SAL	0xb0
#define	PCIX0_PIM0SAH	0xf8

#define	PCIC_CFGADDR	0
#define	PCIC_CFGDATA	4

static int	pcix_match(device_t, cfdata_t, void *);
static void	pcix_attach(device_t, device_t, void *);
static int	pcix_print(void *, const char *);

CFATTACH_DECL_NEW(pcix, 0, pcix_match, pcix_attach, NULL, NULL);

static pcireg_t	pcix_conf_read(void *, pcitag_t, int);
static void	pcix_conf_write(void *, pcitag_t, int, pcireg_t);
static void	pcix_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
static int	pcix_conf_hook(void *, int, int, int, pcireg_t);

/* CFGADDR/CFGDATA pair */
static struct powerpc_bus_space pcix_cfg_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,			/* offset */
	AMCC460EX_PCIX0_CFG_PLBA,	/* extent base */
	AMCC460EX_PCIX0_CFG_PLBA + 8,	/* extent limit */
};

/* Internal (window) registers */
static struct powerpc_bus_space pcix_reg_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
	AMCC460EX_PCIX0_REGS_PLBA,
	AMCC460EX_PCIX0_REGS_PLBA + 0x100,
};

/* PCI I/O: bus 0x0000-0xffff -> PLB I/O window */
static struct powerpc_bus_space pcix_io_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_IO_TYPE,
	AMCC460EX_PCIX0_IO_PLBA,	/* offset */
	0x00000000,			/* extent base */
	0x00010000,			/* extent limit */
};

/*
 * PCI memory: bus addr = low 32 bits of PLB addr.  
 */
static struct powerpc_bus_space pcix_mem_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
	AMCC460EX_PCIX0_MEM_BASE,
	AMCC460EX_PCIX0_PMEM_BASE + AMCC460EX_PCIX0_PMEM_SIZE,
};

static bus_space_handle_t pcix_cfg_ioh;
static bus_space_handle_t pcix_reg_ioh;

static struct genppc_pci_chipset pcix_chipset = {
	.pc_conf_v =		NULL,
	.pc_attach_hook =	pcix_attach_hook,
	.pc_bus_maxdevs =	ibm4xx_pci_bus_maxdevs,
	.pc_make_tag =		ibm4xx_pci_make_tag,
	.pc_conf_read =		pcix_conf_read,
	.pc_conf_write =	pcix_conf_write,

	.pc_intr_v =		&pcix_chipset,
	.pc_intr_map =		genppc_pci_intr_map,
	.pc_intr_string =	genppc_pci_intr_string,
	.pc_intr_evcnt =	genppc_pci_intr_evcnt,
	.pc_intr_establish =	genppc_pci_intr_establish,
	.pc_intr_disestablish =	genppc_pci_intr_disestablish,
	.pc_intr_setattr =	ibm4xx_pci_intr_setattr,
	.pc_intr_type =		genppc_pci_intr_type,
	.pc_intr_alloc =	genppc_pci_intr_alloc,
	.pc_intr_release =	genppc_pci_intr_release,
	.pc_intx_alloc =	genppc_pci_intx_alloc,

	.pc_msi_v =		&pcix_chipset,
	GENPPC_PCI_MSI_INITIALIZER,

	.pc_msix_v =		&pcix_chipset,
	GENPPC_PCI_MSIX_INITIALIZER,

	.pc_conf_interrupt =	ibm4xx_pci_conf_interrupt,
	.pc_decompose_tag =	ibm4xx_pci_decompose_tag,
	.pc_conf_hook =		pcix_conf_hook,
};

static void
pcix_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
pcix_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{

	return PCI_CONF_DEFAULT;
}

/*
 * Translate the tag
 */
static uint32_t
pcix_cfg_addr(pcitag_t tag, int reg)
{
	uint32_t addr = (tag & 0x00ffff00) | (reg & 0xfc);

	if (tag & 0x00ff0000)		/* bus number != 0 -> Type 1 */
		addr |= 1;
	return addr;
}

static pcireg_t
pcix_conf_read(void *v, pcitag_t tag, int reg)
{
	pcireg_t data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	bus_space_write_4(&pcix_cfg_tag, pcix_cfg_ioh, PCIC_CFGADDR,
	    pcix_cfg_addr(tag, reg));
	data = bus_space_read_4(&pcix_cfg_tag, pcix_cfg_ioh, PCIC_CFGDATA);
	bus_space_write_4(&pcix_cfg_tag, pcix_cfg_ioh, PCIC_CFGADDR, 0);
	return data;
}

static void
pcix_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	bus_space_write_4(&pcix_cfg_tag, pcix_cfg_ioh, PCIC_CFGADDR,
	    pcix_cfg_addr(tag, reg));
	bus_space_write_4(&pcix_cfg_tag, pcix_cfg_ioh, PCIC_CFGDATA, data);
	bus_space_write_4(&pcix_cfg_tag, pcix_cfg_ioh, PCIC_CFGADDR, 0);
}

#define	PCIX_REG_WRITE(reg, val) \
	bus_space_write_4(&pcix_reg_tag, pcix_reg_ioh, (reg), (val))

static void
pcix_setup_windows(void)
{

	/* POM0: PLB -> PCI memory, 128MB */
	PCIX_REG_WRITE(PCIX0_POM0SA, 0);		/* disable while */
	PCIX_REG_WRITE(PCIX0_POM0LAL, AMCC460EX_PCIX0_MEM_BASE);
	PCIX_REG_WRITE(PCIX0_POM0LAH, AMCC460EX_PCIX0_MEM_PLBA_H);
	PCIX_REG_WRITE(PCIX0_POM0PCIAL, AMCC460EX_PCIX0_MEM_BASE);
	PCIX_REG_WRITE(PCIX0_POM0PCIAH, 0);
	PCIX_REG_WRITE(PCIX0_POM0SA, ~(0x08000000 - 1) | 1);

	/* POM1: PLB -> PCI prefetchable memory, identity mapped, 256MB */
	PCIX_REG_WRITE(PCIX0_POM1SA, 0);		/* disable while */
	PCIX_REG_WRITE(PCIX0_POM1LAL, AMCC460EX_PCIX0_PMEM_BASE);
	PCIX_REG_WRITE(PCIX0_POM1LAH, AMCC460EX_PCIX0_PMEM_PLBA_H);
	PCIX_REG_WRITE(PCIX0_POM1PCIAL, AMCC460EX_PCIX0_PMEM_BASE);
	PCIX_REG_WRITE(PCIX0_POM1PCIAH, 0);
	PCIX_REG_WRITE(PCIX0_POM1SA, ~(AMCC460EX_PCIX0_PMEM_SIZE - 1) | 1);

	/* POM2 unused */
	PCIX_REG_WRITE(PCIX0_POM2SA, 0);

	/* PIM0: PCI 0 -> PLB 0, 2GB (covers all of RAM) */
	PCIX_REG_WRITE(PCIX0_PIM0SAL, 0);		/* disable while */
	PCIX_REG_WRITE(PCIX0_PIM0LAL, 0);
	PCIX_REG_WRITE(PCIX0_PIM0LAH, 0);
	PCIX_REG_WRITE(PCIX0_PIM0SAH, 0xffffffff);
	PCIX_REG_WRITE(PCIX0_PIM0SAL, ~(0x80000000U - 1) | 1);

	/* PIM1/PIM2 unused */
	PCIX_REG_WRITE(PCIX0_PIM1SA, 0);
	PCIX_REG_WRITE(PCIX0_PIM2SAL, 0);
}

static int
pcix_match(device_t parent, cfdata_t cf, void *aux)
{
	struct plb_attach_args *paa = aux;

	if (strcmp(paa->plb_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
pcix_attach(device_t parent, device_t self, void *aux)
{
	struct plb_attach_args *paa = aux;
	struct pcibus_attach_args pba;
	pci_chipset_tag_t pc = &pcix_chipset;

	aprint_normal(": PLB-PCIX host bridge\n");

	if (bus_space_init(&pcix_cfg_tag, "pcixcfg", NULL, 0) ||
	    bus_space_map(&pcix_cfg_tag, AMCC460EX_PCIX0_CFG_PLBA, 8, 0,
	      &pcix_cfg_ioh))
		panic("pcix_attach: can't map config registers");
	if (bus_space_init(&pcix_reg_tag, "pcixreg", NULL, 0) ||
	    bus_space_map(&pcix_reg_tag, AMCC460EX_PCIX0_REGS_PLBA, 0x100, 0,
	      &pcix_reg_ioh))
		panic("pcix_attach: can't map bridge registers");
	if (bus_space_init(&pcix_io_tag, "pcixio", NULL, 0))
		panic("pcix_attach: can't init IO tag");
	if (bus_space_init(&pcix_mem_tag, "pcixmem", NULL, 0))
		panic("pcix_attach: can't init MEM tag");

	pcix_setup_windows();

#ifdef PCI_NETBSD_CONFIGURE
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
	    0x1000, 0x10000 - 0x1000);
	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    AMCC460EX_PCIX0_MEM_BASE, AMCC460EX_PCIX0_MEM_SIZE);
	pciconf_resource_add(pcires, PCICONF_RESOURCE_PREFETCHABLE_MEM,
	    AMCC460EX_PCIX0_PMEM_BASE, AMCC460EX_PCIX0_PMEM_SIZE);

	pci_configure_bus(pc, pcires, 0, 32);
	pciconf_resource_fini(pcires);
#endif

	pba.pba_iot = &pcix_io_tag;
	pba.pba_memt = &pcix_mem_tag;
	pba.pba_dmat = paa->plb_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY;
	config_found(self, &pba, pcix_print, CFARGS_NONE);
}

static int
pcix_print(void *aux, const char *p)
{

	if (p == NULL)
		return UNCONF;
	return QUIET;
}
