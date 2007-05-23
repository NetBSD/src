/*	$NetBSD: mainbus.c,v 1.18.38.3 2007/05/23 01:45:11 nisimura Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.18.38.3 2007/05/23 01:45:11 nisimura Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/isa_machdep.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

struct conf_args {
	const char *ca_name;
};

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);
int	mainbus_print(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

struct powerpc_isa_chipset genppc_ict;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct conf_args ca;
	struct pcibus_attach_args pba;
#if defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
#endif

	printf("\n");

	ca.ca_name = "cpu";
	config_found_ia(self, "mainbus", &ca, mainbus_print);
	ca.ca_name = "eumb";
	config_found_ia(self, "mainbus", &ca, mainbus_print);

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
#if defined(PCI_NETBSD_CONFIGURE)
	ioext  = extent_create("pciio",  0x00000600, 0x0000ffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x80000000, 0x8fffffff, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(0, ioext, memext, NULL, 0, 32);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif

	pba.pba_iot = &sandpoint_io_space_tag;
	pba.pba_memt = &sandpoint_mem_space_tag;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_pc = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif
}

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct conf_args *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return 0;
	if (cpu_info[0].ci_dev != NULL)
		return 0;

	return 1;
}

void
cpu_attach(struct device *parent, struct device *self, void *aux)
{

	(void) cpu_attach_common(self, 0);
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct conf_args *ca = aux;

	if (pnp)
		aprint_normal("%s at %s", ca->ca_name, pnp);
	return (UNCONF);
}
