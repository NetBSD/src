/*	$NetBSD: mainbus.c,v 1.16 2005/06/28 19:46:47 drochner Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.16 2005/06/28 19:46:47 drochner Exp $");

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h"
#include "opt_algor_p6032.h"

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <mips/cache.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#if defined(PCI_NETBSD_CONFIGURE) && defined(PCI_NETBSD_ENABLE_IDE)
#if defined(ALGOR_P5064) || defined(ALGOR_P6032)
#include <dev/pci/pciide_piix_reg.h>
#endif /* ALGOR_P5064 || ALGOR_P6032 */
#endif /* PCI_NETBSD_CONFIGURE && PCI_NETBSD_ENABLE_IDE */

#include "locators.h"
#include "pci.h"

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);
int	mainbus_submatch(struct device *, struct cfdata *,
			 const locdesc_t *, void *);

/* There can be only one. */
int	mainbus_found;

struct mainbusdev {
	const char *md_name;
	bus_addr_t md_addr;
	int md_irq;
};

#if defined(ALGOR_P4032)
#include <algor/algor/algor_p4032reg.h>
#include <algor/algor/algor_p4032var.h>

struct mainbusdev mainbusdevs[] = {
	{ "cpu",		-1,			-1 },
	{ "mcclock",		P4032_RTC,		P4032_IRQ_RTC },
	{ "com",		P4032_COM1,		P4032_IRQ_COM1 },
	{ "com",		P4032_COM2,		P4032_IRQ_COM2 },
	{ "lpt",		P4032_LPT,		P4032_IRQ_LPT },
	{ "pckbc",		P4032_PCKBC,		P4032_IRQ_PCKBC },
	{ "fdc",		P4032_FDC,		P4032_IRQ_FLOPPY },
	{ "vtpbc",		P4032_V962PBC,		-1 },

	{ NULL,			0,			0 },
};
#endif /* ALGOR_P4032 */

#if defined(ALGOR_P5064)
#include <algor/algor/algor_p5064reg.h>
#include <algor/algor/algor_p5064var.h>

struct mainbusdev mainbusdevs[] = {
	{ "cpu",		-1,			-1 },
	{ "vtpbc",		P5064_V360EPC,		-1 },

	{ NULL,			0,			0 },
};
#endif /* ALGOR_P5064 */

#if defined(ALGOR_P6032)
#include <algor/algor/algor_p6032reg.h>
#include <algor/algor/algor_p6032var.h>

struct mainbusdev mainbusdevs[] = {
	{ "cpu",		-1,			-1 },
	{ "bonito",		BONITO_REG_BASE,	-1 },

	{ NULL,			0,			0 },
};
#endif /* ALGOR_P6032 */

int
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (mainbus_found)
		return (0);

	return (1);
}

void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args ma;
	struct mainbusdev *md;
	bus_space_tag_t st;
#if defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
	pci_chipset_tag_t pc;
#if defined(PCI_NETBSD_ENABLE_IDE)
	pcitag_t idetag;
	pcireg_t idetim;
#endif
#endif

	mainbus_found = 1;

	printf("\n");

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
#if defined(ALGOR_P4032)
	/*
	 * Reserve the bottom 64K of the I/O space for ISA devices.
	 */
	ioext  = extent_create("pciio",  0x00010000, 0x000effff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x01000000, 0x07ffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	pc = &p4032_configuration.ac_pc;
#elif defined(ALGOR_P5064)
	/*
	 * Reserve the bottom 512K of the I/O space for ISA devices.
	 * According to the PMON sources, this is a work-around for
	 * a bug in the ISA bridge.
	 */
	ioext  = extent_create("pciio",  0x00080000, 0x00ffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x01000000, 0x07ffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	pc = &p5064_configuration.ac_pc;
#if defined(PCI_NETBSD_ENABLE_IDE)
	idetag = pci_make_tag(pc, 0, 2, 1);
#endif
#elif defined(ALGOR_P6032)
	/*
	 * Reserve the bottom 64K of the I/O space for ISA devices.
	 */
	ioext  = extent_create("pciio",  0x00010000, 0x000effff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x01000000, 0x0affffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	pc = &p6032_configuration.ac_pc;
#if defined(PCI_NETBSD_ENABLE_IDE)
	idetag = pci_make_tag(pc, 0, 17, 1);
#endif
#endif /* ALGOR_P4032 || ALGOR_P5064 || ALGOR_P6032 */

	pci_configure_bus(pc, ioext, memext, NULL, 0, mips_dcache_align);
	extent_destroy(ioext);
	extent_destroy(memext);

#if defined(PCI_NETBSD_ENABLE_IDE)
	/*
	 * Perhaps PMON has not enabled the IDE controller.  Easy to
	 * fix -- just set the ENABLE bits for each channel in the
	 * IDETIM register.  Just clear all the bits for the channel
	 * except for the ENABLE bits -- the `pciide' driver will
	 * properly configure it later.
	 */
	idetim = 0;
	if (PCI_NETBSD_ENABLE_IDE & 0x01)
		idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE, 0);
	if (PCI_NETBSD_ENABLE_IDE & 0x02)
		idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE, 1);
	pci_conf_write(pc, idetag, PIIX_IDETIM, idetim);
#endif
#endif /* NPCI > 0 && defined(PCI_NETBSD_CONFIGURE) */

#if defined(ALGOR_P4032)
	st = &p4032_configuration.ac_lociot;
#elif defined(ALGOR_P5064)
	st = NULL;
#elif defined(ALGOR_P6032)
	st = NULL;
#endif

	for (md = mainbusdevs; md->md_name != NULL; md++) {
		ma.ma_name = md->md_name;
		ma.ma_st = st;
		ma.ma_addr = md->md_addr;
		ma.ma_irq = md->md_irq;
		(void) config_found_sm_loc(self, "mainbus", NULL, &ma,
		    mainbus_print, mainbus_submatch);
	}
}

int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma = aux;

	if (pnp)
		aprint_normal("%s at %s", ma->ma_name, pnp);
	if (ma->ma_addr != (bus_addr_t) -1)
		aprint_normal(" addr 0x%lx", ma->ma_addr);

	return (UNCONF);
}

int
mainbus_submatch(struct device *parent, struct cfdata *cf,
		 const locdesc_t *, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (cf->cf_loc[MAINBUSCF_ADDR] != MAINBUSCF_ADDR_DEFAULT &&
	    cf->cf_loc[MAINBUSCF_ADDR] != ma->ma_addr)
		return (0);

	return (config_match(parent, cf, aux));
}
