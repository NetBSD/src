/*	$NetBSD: mainbus.c,v 1.40 2021/08/07 16:19:03 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.40 2021/08/07 16:19:03 thorpej Exp $");

#include "opt_pci.h"
#include "opt_residual.h"

#include "pnpbus.h"
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

#include <prep/pnpbus/pnpbusvar.h>

#include <machine/platform.h>
#include <machine/residual.h>

int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
	struct pnpbus_attach_args mba_paa;
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
	union mainbus_attach_args mba;
	struct confargs ca;
	int i;
#if NPCI > 0
	struct genppc_pci_chipset_businfo *pbi;
#endif

	mainbus_found = 1;

	aprint_normal("\n");

#if defined(RESIDUAL_DATA_DUMP)
	print_residual_device_info();
#endif

	for (i = 0; i < CPU_MAXNUM; i++) {
		ca.ca_name = "cpu";
		ca.ca_node = i;
		config_found(self, &ca, NULL,
		    CFARGS(.iattr = "mainbus"));
	}

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	genppc_pct = kmem_alloc(sizeof(struct genppc_pci_chipset), KM_SLEEP);
	prep_pci_get_chipset_tag(genppc_pct);

	pbi = kmem_alloc(sizeof(struct genppc_pci_chipset_businfo), KM_SLEEP);
	pbi->pbi_properties = prop_dictionary_create();
        KASSERT(pbi->pbi_properties != NULL);

	SIMPLEQ_INIT(&genppc_pct->pc_pbi);
	SIMPLEQ_INSERT_TAIL(&genppc_pct->pc_pbi, pbi, next);

	/* fix pci interrupt routings on some models */
	setup_pciroutinginfo();

	/* find the primary host bridge */
	setup_pciintr_map(pbi, 0, 0, 0);

#ifdef PCI_NETBSD_CONFIGURE
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
	    PCI_IO_START, PCI_IO_SIZE);
	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    PCI_MEM_START, PCI_MEM_SIZE);

	pci_configure_bus(genppc_pct, pcires, 0, CACHELINESIZE);

	pciconf_resource_fini(pcires);
#endif /* PCI_NETBSD_CONFIGURE */
#endif /* NPCI */

/* scan pnpbus first */
#if NPNPBUS > 0
	mba.mba_paa.paa_name = "pnpbus";
	mba.mba_paa.paa_iot = &genppc_isa_io_space_tag;
	mba.mba_paa.paa_memt = &genppc_isa_mem_space_tag;
	mba.mba_paa.paa_ic = &genppc_ict;
	mba.mba_paa.paa_dmat = &isa_bus_dma_tag;
	config_found(self, &mba.mba_pba, mainbus_print,
	    CFARGS(.iattr = "mainbus"));
#endif /* NPNPBUS */

#if NPCI > 0
	memset(&mba, 0, sizeof(mba));
	mba.mba_pba._pba_busname = NULL;
	mba.mba_pba.pba_iot = &prep_io_space_tag;
	mba.mba_pba.pba_memt = &prep_mem_space_tag;
	mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
	mba.mba_pba.pba_dmat64 = NULL;
	mba.mba_pba.pba_pc = genppc_pct;
	mba.mba_pba.pba_bus = 0;
	mba.mba_pba.pba_bridgetag = NULL;
	mba.mba_pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	config_found(self, &mba.mba_pba, pcibusprint,
	    CFARGS(.iattr = "pcibus"));
#endif /* NPCI */

#ifdef RESIDUAL_DATA_DUMP
	SIMPLEQ_FOREACH(pbi, &genppc_pct->pc_pbi, next)
		printf("%s\n", prop_dictionary_externalize(pbi->pbi_properties));
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
