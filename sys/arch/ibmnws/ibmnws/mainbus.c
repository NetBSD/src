/*	$NetBSD: mainbus.c,v 1.15.2.1 2021/04/02 22:17:40 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "opt_pci.h"

/* #include "obio.h" */
#include "pci.h"
#include "isa.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/isa_machdep.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);

struct mainbus_softc {
	device_t sc_dev;		/* device tree glue */
};

CFATTACH_DECL_NEW(mainbus, sizeof(struct mainbus_softc),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
};

/* There can be only one. */
int mainbus_found = 0;
struct powerpc_isa_chipset genppc_ict;
struct genppc_pci_chipset *genppc_pct;

#define	PCI_IO_START	0x00008000
#define	PCI_IO_END	0x0000ffff
#define	PCI_IO_SIZE	((PCI_IO_END - PCI_IO_START) + 1)

#define	PCI_MEM_START	0x00000000
#define	PCI_MEM_END	0x0fffffff
#define	PCI_MEM_SIZE	((PCI_MEM_END - PCI_MEM_START) + 1)

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	if (mainbus_found)
		return 0;
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_softc *sc = device_private(self);
	union mainbus_attach_args mba;
	struct confargs ca;

	mainbus_found = 1;

	aprint_normal("\n");

	sc->sc_dev = self;
	ca.ca_name = "cpu";
	ca.ca_node = 0;
	config_found(self, &ca, mainbus_print,
	    CFARG_IATTR, "mainbus",
	    CFARG_EOL);

#if NOBIO > 0
	obio_reserve_resource_map();
#endif

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	genppc_pct = kmem_alloc(sizeof(struct genppc_pci_chipset), KM_SLEEP);
	ibmnws_pci_get_chipset_tag_indirect (genppc_pct);

#ifdef PCI_NETBSD_CONFIGURE
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
	    PCI_IO_START, PCI_IO_SIZE);
	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    PCI_MEM_START, PCI_MEM_SIZE);

	pci_configure_bus(genppc_pct, pcires, 0, CACHELINESIZE);

	pciconf_resource_fini(pcires);
#endif

	memset(&mba, 0, sizeof(mba));
	mba.mba_pba.pba_iot = &prep_io_space_tag;
	mba.mba_pba.pba_memt = &prep_mem_space_tag;
	mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
	mba.mba_pba.pba_dmat64 = NULL;
	mba.mba_pba.pba_pc = genppc_pct;
	mba.mba_pba.pba_bus = 0;
	mba.mba_pba.pba_bridgetag = NULL;
	mba.mba_pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	config_found(self, &mba.mba_pba, pcibusprint,
	    CFARG_IATTR, "pcibus",
	    CFARG_EOL);
#endif

#if NOBIO > 0
#if 0
	obio_reserve_resource_unmap();

	if (platform->obiodevs != obiodevs_nodev) {
		memset(&mba, 0, sizeof(mba));
		mba.mba_busname = "obio"; /* XXX needs placeholder in pba */
		mba.mba_pba.pba_iot = &isa_io_space_tag;
		mba.mba_pba.pba_memt = &isa_mem_space_tag;
		config_found(self, &mba.mba_pba, mainbus_print,
		    CFARG_IATTR, "mainbus",
		    CFARG_EOL);
	}
#endif
#endif
}

int
mainbus_print(void *aux, const char *pnp)
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_busname, pnp);

	return (UNCONF);
}
