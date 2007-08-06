/*	$NetBSD: mainbus.c,v 1.19.38.4 2007/08/06 13:27:26 ober Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.19.38.4 2007/08/06 13:27:26 ober Exp $");

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include "pci.h"
#include "opt_pci.h"
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <machine/pci_machdep.h>
#include <machine/isa_machdep.h>

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print (void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
  /*struct pnpbus_attach_args mba_paa;*/
};


/* There can be only one */
int mainbus_found = 0;
struct powerpc_isa_chipset genppc_ict;
struct genppc_pci_chipset *genppc_pct;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
  union mainbus_attach_args mba;
  struct confargs ca;

#if NPCI > 0 
  struct genppc_pci_chipset_businfo *pbi;
#ifdef PCI_NETBSD_CONFIGURE
  struct extent *ioext, *memext;
#endif
#endif
	
  mainbus_found = 1;
  
  aprint_normal("\n");


#if defined(RESIDUAL_DATA_DUMP)
  print_residual_device_info();
#endif

	/*
	 * Always find the CPU
	 */
  ca.ca_name = "cpu";
  ca.ca_node = 0;
  config_found_ia(self, "mainbus", &ca, mainbus_print);

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */

#if NPCI > 0
	genppc_pct = malloc(sizeof(struct genppc_pci_chipset), M_DEVBUF,
	    M_NOWAIT);
	KASSERT(genppc_pct != NULL);
	/*prep_pci_get_chipset_tag(genppc_pct);
	 */
	pbi = malloc(sizeof(struct genppc_pci_chipset_businfo),
	    M_DEVBUF, M_NOWAIT);
	KASSERT(pbi != NULL);
	pbi->pbi_properties = prop_dictionary_create();
        KASSERT(pbi->pbi_properties != NULL);

	SIMPLEQ_INIT(&genppc_pct->pc_pbi);
	SIMPLEQ_INSERT_TAIL(&genppc_pct->pc_pbi, pbi, next);

	/* find the primary host bridge */
	/* setup_pciintr_map(pbi, 0, 0, 0);*/

#ifdef PCI_NETBSD_CONFIGURE
	ioext  = extent_create("pciio",  0x00008000, 0x0000ffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x00000000, 0x0fffffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(genppc_pct, ioext, memext, NULL, 0, CACHELINESIZE);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif /* PCI_NETBSD_CONFIGURE */
#endif /* NPCI */

/* scan pnpbus first */
#if NPNPBUS > 0
	mba.mba_paa.paa_iot = &genppc_isa_io_space_tag;
	mba.mba_paa.paa_memt = &genppc_isa_mem_space_tag;
	mba.mba_paa.paa_ic = &genppc_ict;
	mba.mba_paa.paa_dmat = &isa_bus_dma_tag;
	config_found_ia(self, "mainbus", &mba.mba_pba, mainbus_print);
#endif /* NPNPBUS */

#if NPCI > 0
	bzero(&mba, sizeof(mba));
	mba.mba_pba.pba_iot = &genppc_io_space_tag;
	mba.mba_pba.pba_memt = &genppc_mem_space_tag;
	mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
	mba.mba_pba.pba_dmat64 = NULL;
	mba.mba_pba.pba_pc = genppc_pct;
	mba.mba_pba.pba_bus = 0;
	mba.mba_pba.pba_bridgetag = NULL;
	mba.mba_pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	config_found_ia(self, "pcibus", &mba.mba_pba, pcibusprint);
#endif /* NPCI */

#ifdef RESIDUAL_DATA_DUMP
	SIMPLEQ_FOREACH(pbi, &genppc_pct->pc_pbi, next)
		printf("%s\n", prop_dictionary_externalize(pbi->pbi_properties));
#endif


#if 0
#if NPCI > 0
#if defined(PCI_NETBSD_CONFIGURE)
	ioext  = extent_create("pciio",  0x00008000, 0x0000ffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x00000000, 0x0fffffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);
	pci_configure_bus(0, ioext, memext, NULL, 0, 32);
	extent_destroy(ioext);
	extent_destroy(memext);
#endif
	
	mba.mba_pba.pba_iot = &genppc_isa_io_space_tag;
	mba.mba_pba.pba_memt = &genppc_isa_mem_space_tag;
	mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
	mba.mba_pba.pba_dmat64 = NULL;
	mba.mba_pba.pba_bus = 0;
	mba.mba_pba.pba_bridgetag = NULL;
	mba.mba_pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	config_found_ia(self, "pcibus", &mba, pcibusprint);
#endif
#endif
}

int
mainbus_print(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("cpu at %s", pnp);
	return (UNCONF);
}
