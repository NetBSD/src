/*	$NetBSD: mainbus.c,v 1.5 1999/05/05 04:40:00 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include "pci.h"

int	mainbus_match __P((struct device *, struct cfdata *, void *));
void	mainbus_attach __P((struct device *, struct device *, void *));
int	mainbus_print __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

void pci_init();

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
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
	struct pcibus_attach_args pba;
	struct ofbus_attach_args oba;
	struct confargs ca;
	int node, n;

	printf("\n");

	node = OF_peer(0);
	if (node) {
		oba.oba_busname = "ofw";
		oba.oba_phandle = node;
		config_found(self, &oba, NULL);
	}

	ca.ca_name = "cpu";
	config_found(self, &ca, NULL);

	pci_init();

	for (n = 0; n < 2; n++) {
		if (pci_bridges[n].addr) {
			bzero(&pba, sizeof(pba));
			pba.pba_busname = "pci";
			pba.pba_iot = pci_bridges[n].iot;
			pba.pba_memt = pci_bridges[n].memt;
			pba.pba_dmat = &pci_bus_dma_tag;
			pba.pba_bus = pci_bridges[n].bus;
			pba.pba_pc = pci_bridges[n].pc;
			pba.pba_flags =
				PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
			config_found(self, &pba, mainbus_print);
		}
	}
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pa= aux;

	if (pnp)
		printf("%s at %s", pa->pba_busname, pnp);
	printf(" bus %d", pa->pba_bus);
	return UNCONF;
}
