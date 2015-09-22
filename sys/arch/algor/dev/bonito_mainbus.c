/*	$NetBSD: bonito_mainbus.c,v 1.15.14.1 2015/09/22 12:05:35 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bonito_mainbus.c,v 1.15.14.1 2015/09/22 12:05:35 skrll Exp $");

#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <algor/autoconf.h>

#include <mips/cpuregs.h>
#include <mips/bonito/bonitoreg.h>

#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032var.h>
#endif

struct bonito_softc {
	struct bonito_config *sc_bonito;
};

int	bonito_mainbus_match(device_t, cfdata_t, void *);
void	bonito_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bonito_mainbus, sizeof(struct bonito_softc),
    bonito_mainbus_match, bonito_mainbus_attach, NULL, NULL);
extern struct cfdriver bonito_cd;

int
bonito_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, bonito_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
bonito_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct bonito_softc *sc = device_private(self);
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

	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

#if defined(ALGOR_P6032)
	    {
		struct p6032_config *acp = &p6032_configuration;

		pba.pba_iot = &acp->ac_iot;
		pba.pba_memt = &acp->ac_memt;
		pba.pba_dmat = &acp->ac_pci_dmat;
		pba.pba_dmat64 = NULL;
		pba.pba_pc = &acp->ac_pc;
	    }
#endif /* ALGOR_P6032 */

	(void) config_found_ia(self, "pcibus", &pba, pcibusprint);
}
