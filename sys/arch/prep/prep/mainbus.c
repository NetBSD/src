/*	$NetBSD: mainbus.c,v 1.19.6.1 2006/04/22 11:37:54 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.19.6.1 2006/04/22 11:37:54 simonb Exp $");

#include "opt_pci.h"
#include "opt_residual.h"

#include "pnpbus.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/isa_machdep.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <prep/pnpbus/pnpbusvar.h>

#include <machine/platform.h>
#include <machine/residual.h>

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
	struct pnpbus_attach_args mba_paa;
};

/* There can be only one. */
int mainbus_found = 0;
struct prep_isa_chipset prep_isa_chipset;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	if (mainbus_found)
		return 0;
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	union mainbus_attach_args mba;
	struct confargs ca;
#if NPCI > 0
	static struct prep_pci_chipset pc;
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#endif
#endif

	mainbus_found = 1;

	printf("\n");

#if defined(RESIDUAL_DATA_DUMP)
	print_residual_device_info();
#endif

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
	prep_pci_get_chipset_tag(&pc);

#ifdef PCI_NETBSD_CONFIGURE
	ioext  = extent_create("pciio",  0x00008000, 0x0000ffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x00000000, 0x0fffffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(&pc, ioext, memext, NULL, 0, CACHELINESIZE);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif

	mba.mba_pba.pba_iot = &prep_io_space_tag;
	mba.mba_pba.pba_memt = &prep_mem_space_tag;
	mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
	mba.mba_pba.pba_dmat64 = NULL;
	mba.mba_pba.pba_pc = &pc;
	mba.mba_pba.pba_bus = 0;
	mba.mba_pba.pba_bridgetag = NULL;
	mba.mba_pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	config_found_ia(self, "pcibus", &mba.mba_pba, pcibusprint);
#endif

#if NPNPBUS > 0
	bzero(&mba, sizeof(mba));
	mba.mba_paa.paa_iot = &prep_isa_io_space_tag;
	mba.mba_paa.paa_memt = &prep_isa_mem_space_tag;
	mba.mba_paa.paa_ic = &prep_isa_chipset;
	config_found_ia(self, "mainbus", &mba.mba_pba, mainbus_print);
#endif
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_busname, pnp);

	return (UNCONF);
}
