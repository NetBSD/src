/*	$NetBSD: mainbus.c,v 1.2 1996/03/08 20:26:59 cgd Exp $	*/

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

#include <machine/bus.h>

#if 0 /* XXX eisavar.h includes isavar.h, which is not idempotent */
#include <dev/isa/isavar.h>
#endif
#include <dev/eisa/eisavar.h>
#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <i386/isa/isa_machdep.h>
#include <i386/eisa/eisa_machdep.h>

#include "pci.h"

int	mainbus_match __P((struct device *, void *, void *));
void	mainbus_attach __P((struct device *, struct device *, void *));

struct cfdriver mainbuscd =
    { NULL, "mainbus", mainbus_match, mainbus_attach,
      DV_DULL, sizeof(struct device) };

int	mainbus_print __P((void *, char *));

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
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

	printf("\n");

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	if (pci_mode_detect() != 0) {
		struct pcibus_attach_args pba;

		pba.pba_busname = "pci";
		pba.pba_bc = NULL;
		pba.pba_bus = 0;
		pba.pba_maxndevs = pci_mode == 2 ? 16 : 32;
		config_found(self, &pba, mainbus_print);
	}
#endif

	if (!bcmp(ISA_HOLE_VADDR(EISA_ID_PADDR), EISA_ID, EISA_ID_LEN)) {
		struct eisabus_attach_args eba;

		eba.eba_busname = "eisa";
		eba.eba_bc = NULL;
		config_found(self, &eba, mainbus_print);
	}

	if (1 /* XXX ISA NOT YET SEEN */) {
		struct isabus_attach_args iba;

		iba.iba_busname = "isa";
		iba.iba_bc = NULL;
		config_found(self, &iba, mainbus_print);
	}
}

int
mainbus_print(aux, pnp)
	void *aux;
	char *pnp;
{
	char **busname = aux;		/* XXX should be common struct */
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", *busname, pba);
	if (!strcmp(*busname, "pci"))
		printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}
