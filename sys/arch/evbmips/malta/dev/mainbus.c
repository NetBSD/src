/*	$NetBSD: mainbus.c,v 1.17 2021/08/07 16:18:51 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.17 2021/08/07 16:18:51 thorpej Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#if defined(PCI_NETBSD_CONFIGURE)
#include <sys/malloc.h>
#endif

#include <dev/pci/pcivar.h>
#if defined(PCI_NETBSD_CONFIGURE)
#include <dev/pci/pciconf.h>
#endif

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <evbmips/malta/autoconf.h>
#include <evbmips/malta/maltareg.h>
#include <evbmips/malta/maltavar.h>

#if defined(PCI_NETBSD_ENABLE_IDE)
#include <dev/pci/pciide_piix_reg.h>
#endif /* PCI_NETBSD_ENABLE_IDE */

#include "locators.h"
#include "pci.h"

static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);
static int	mainbus_submatch(device_t, cfdata_t, const int *, void *);
static int	mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

/* There can be only one. */
bool mainbus_found;

struct mainbusdev {
	const char *md_name;
	bus_addr_t md_addr;
	int md_intr;
};

const struct mainbusdev mainbusdevs[] = {
	{ "cpu",		-1,			-1 },
	{ "gt",			MALTA_CORECTRL_BASE,	-1 },
	{ "com",		MALTA_CBUSUART,		MALTA_CBUSUART_INTR },
	{ "i2c",		MALTA_I2C_BASE,		-1 },
	{ "gpio",		MALTA_GPIO_BASE,	-1 },
	{ NULL,			0,			0 },
};

#define	PCI_IO_START	0x00001000
#define	PCI_IO_END	0x0000efff
#define	PCI_IO_SIZE	((PCI_IO_END - PCI_IO_START) + 1)

#define	PCI_MEM_START	MALTA_PCIMEM1_BASE
#define	PCI_MEM_SIZE	MALTA_PCIMEM1_SIZE

static int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	if (mainbus_found)
		return (0);

	return (1);
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args ma;
	const struct mainbusdev *md;
#if defined(PCI_NETBSD_ENABLE_IDE) || defined(PCI_NETBSD_CONFIGURE)
	struct malta_config *mcp = &malta_configuration;
	pci_chipset_tag_t pc = &mcp->mc_pc;
#endif

	mainbus_found = true;
	printf("\n");

#if defined(PCI_NETBSD_CONFIGURE)
	struct mips_cache_info * const mci = &mips_cache_info;
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
	    PCI_IO_START, PCI_IO_SIZE);
	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    PCI_MEM_START, PCI_MEM_SIZE);

	pci_configure_bus(pc, pcires, 0, mci->mci_dcache_align);
	pciconf_resource_fini(pcires);
#endif /* PCI_NETBSD_CONFIGURE */

#if defined(PCI_NETBSD_ENABLE_IDE)
	/*
	 * Perhaps PMON has not enabled the IDE controller.  Easy to
	 * fix -- just set the ENABLE bits for each channel in the
	 * IDETIM register.  Just clear all the bits for the channel
	 * except for the ENABLE bits -- the `pciide' driver will
	 * properly configure it later.
	 */
	pcireg_t idetim = 0;
	if (PCI_NETBSD_ENABLE_IDE & 0x01)
		idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE, 0);
	if (PCI_NETBSD_ENABLE_IDE & 0x02)
		idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE, 1);

	/* pciide0 is pci device 10, function 1 */
	pcitag_t idetag = pci_make_tag(pc, 0, 10, 1);
	pci_conf_write(pc, idetag, PIIX_IDETIM, idetim);
#endif
	for (md = mainbusdevs; md->md_name != NULL; md++) {
		ma.ma_name = md->md_name;
		ma.ma_addr = md->md_addr;
		ma.ma_intr = md->md_intr;
		config_found(self, &ma, mainbus_print,
		    CFARGS(.submatch = mainbus_submatch));
	}
}

static int
mainbus_submatch(device_t parent, cfdata_t cf,
		 const int *ldesc, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (cf->cf_loc[MAINBUSCF_ADDR] != MAINBUSCF_ADDR_DEFAULT &&
	    cf->cf_loc[MAINBUSCF_ADDR] != ma->ma_addr)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma = aux;

	if (pnp != 0)
		return QUIET;

	if (pnp)
		aprint_normal("%s at %s", ma->ma_name, pnp);
	if (ma->ma_addr != MAINBUSCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%lx", ma->ma_addr);

	return (UNCONF);
}
