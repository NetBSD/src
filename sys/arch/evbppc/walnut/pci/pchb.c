/*	$NetBSD: pchb.c,v 1.3 2003/07/15 01:37:38 lukem Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pchb.c,v 1.3 2003/07/15 01:37:38 lukem Exp $");

#include "pci.h"
#include "opt_pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _IBM4XX_BUS_DMA_PRIVATE
#include <machine/walnut.h>

#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

static int	pchbmatch(struct device *, struct cfdata *, void *);
static void	pchbattach(struct device *, struct device *, void *);
static int	pchbprint(void *, const char *);

CFATTACH_DECL(pchb, sizeof(struct device),
    pchbmatch, pchbattach, NULL, NULL);

static int pcifound = 0;

static int
pchbmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct plb_attach_args *paa = aux;
	/* XXX chipset tag unused by walnut, so just pass 0 */
	pci_chipset_tag_t pc = 0;
	pcitag_t tag; 
	int class, id;

	/* match only pchb devices */
	if (strcmp(paa->plb_name, cf->cf_name) != 0)
		return 0;

	pci_machdep_init();
	tag = pci_make_tag(pc, 0, 0, 0);

	class = pci_conf_read(pc, tag, PCI_CLASS_REG);
	id = pci_conf_read(pc, tag, PCI_ID_REG);

	/*
	 * Match all known PCI host chipsets.
	 */
	if (PCI_CLASS(class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_HOST) {
		switch (PCI_VENDOR(id)) {
		case PCI_VENDOR_IBM:
			switch (PCI_PRODUCT(id)) {
			case PCI_PRODUCT_IBM_405GP:
				return (!pcifound);
			}
			break;
		}
	}
	return (0);
}

static void
pchbattach(struct device *parent, struct device *self, void *aux)
{
	struct pcibus_attach_args pba;
	char devinfo[256];
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#ifdef PCI_CONFIGURE_VERBOSE
	extern int pci_conf_debug;

	pci_conf_debug = 1;
#endif
#endif
	pci_chipset_tag_t pc = 0;
	pcitag_t tag; 
	int class, id;

	pci_machdep_init();
	tag = pci_make_tag(pc, 0, 0, 0);

	class = pci_conf_read(pc, tag, PCI_CLASS_REG);
	id = pci_conf_read(pc, tag, PCI_ID_REG);

	printf("\n");
	pcifound++;
	/*
	 * All we do is print out a description.  Eventually, we
	 * might want to add code that does something that's
	 * possibly chipset-specific.
	 */

	pci_devinfo(id, class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(class));

	pci_machdep_init(); /* Redundant... */
	ibm4xx_setup_pci();
#ifdef PCI_CONFIGURE_VERBOSE
	ibm4xx_show_pci_map();
#endif

#ifdef PCI_NETBSD_CONFIGURE
	memext = extent_create("pcimem", MIN_PCI_MEMADDR_NOPREFETCH,
	    MIN_PCI_MEMADDR_NOPREFETCH + 0x1fffffff, M_DEVBUF, NULL, 0,
	    EX_NOWAIT);
	ioext = extent_create("pciio", MIN_PCI_PCI_IOADDR,
	    MIN_PCI_PCI_IOADDR + 0xffff, M_DEVBUF, NULL, 0, EX_NOWAIT);
	pci_configure_bus(0, ioext, memext, NULL, 0, 32);
	extent_destroy(memext);
	extent_destroy(ioext);
#endif /* PCI_NETBSD_CONFIGURE */

#ifdef PCI_CONFIGURE_VERBOSE
	printf("running config_found PCI\n");
#endif
	pba.pba_busname = "pci";
	/* IO window located @ e8000000 and maps to 0-0xffff */
	pba.pba_iot = ibm4xx_make_bus_space_tag(MIN_PLB_PCI_IOADDR, 0);
	/* PCI memory window is directly mapped */
	pba.pba_memt = ibm4xx_make_bus_space_tag(0, 0);
	pba.pba_dmat = &ibm4xx_default_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_MEM_ENABLED | PCI_FLAGS_IO_ENABLED;
	config_found(self, &pba, pchbprint);
}


static int
pchbprint(void *aux, const char *p)
{

	if (p == NULL)
		return (UNCONF);
	return (QUIET);
}

#if 0
static void
scan_pci_bus(void)
{
	pcitag_t tag;
	int i, x;

	for (i=0;i<32;i++){
		tag = pci_make_tag(0, 0, i, 0);
		x = pci_conf_read(0, tag, 0);
		printf("%d tag=%08x : %08x\n", i, tag, x);
#if 0
		if (PCI_VENDOR(x) == PCI_VENDOR_INTEL
		    && PCI_PRODUCT(x) == PCI_PRODUCT_INTEL_80960_RP) {
			/* Do not configure PCI bus analyzer */
			continue;
		}
		x = pci_conf_read(0, tag, PCI_COMMAND_STATUS_REG);
		x |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(0, tag, PCI_COMMAND_STATUS_REG, x);
#endif
	}
}
#endif
