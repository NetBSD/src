/*	$NetBSD: bonito_mainbus.c,v 1.1 2001/06/25 20:15:57 thorpej Exp $	*/

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

#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <mips/bonito/bonitoreg.h>

#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032var.h>
#endif

struct bonito_softc {
	struct device sc_dev;
	struct bonito_config *sc_bonito;
};

int	bonito_mainbus_match(struct device *, struct cfdata *, void *);
void	bonito_mainbus_attach(struct device *, struct device *, void *);

struct cfattach bonito_mainbus_ca = {
	sizeof(struct bonito_softc), bonito_mainbus_match,
	    bonito_mainbus_attach,
};
extern struct cfdriver bonito_cd;

int	bonito_mainbus_print(void *, const char *);

int
bonito_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, bonito_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
bonito_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct bonito_softc *sc = (void *) self;
	struct pcibus_attach_args pba;
	struct bonito_config *bc;
	pcireg_t rev;

	/*
	 * There is only one PCI controller on an Algorithmics board.
	 */
#if defined(ALGOR_P6032)
	bc = &p6032_configuration.ac_bonito;
#endif
	sc->sc_bonito = bc;

	rev = PCI_REVISION(REGVAL(BONITO_PCICLASS));

	printf(": BONITO Memory and PCI controller, %s rev. %d.%d\n",
	    BONITO_REV_FPGA(rev) ? "FPGA" : "ASIC",
	    BONITO_REV_MAJOR(rev), BONITO_REV_MINOR(rev));

	pba.pba_busname = "pci";
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;

#if defined(ALGOR_P6032)
	    {
		struct p6032_config *acp = &p6032_configuration;

		pba.pba_iot = &acp->ac_iot;
		pba.pba_memt = &acp->ac_memt;
		pba.pba_dmat = &acp->ac_pci_dmat;
		pba.pba_pc = &acp->ac_pc;
	    }
#endif /* ALGOR_P6032 */

	(void) config_found(self, &pba, bonito_mainbus_print);
}

int
bonito_mainbus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to BONITOs; easy. */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
