/*	$NetBSD: vtpbc_mainbus.c,v 1.3 2001/06/14 17:57:26 thorpej Exp $	*/

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

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h"
#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <algor/pci/vtpbcvar.h>

#ifdef ALGOR_P4032
#include <algor/algor/algor_p4032var.h>
#endif

#ifdef ALGOR_P5064
#include <algor/algor/algor_p5064var.h>
#endif
 
struct vtpbc_softc {
	struct device sc_dev;
	struct vtpbc_config *sc_vtpbc;
};

int	vtpbc_mainbus_match(struct device *, struct cfdata *, void *);
void	vtpbc_mainbus_attach(struct device *, struct device *, void *);

struct cfattach vtpbc_mainbus_ca = {
	sizeof(struct vtpbc_softc), vtpbc_mainbus_match, vtpbc_mainbus_attach,
};
extern struct cfdriver vtpbc_cd;

int	vtpbc_mainbus_print(void *, const char *);

int
vtpbc_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, vtpbc_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
vtpbc_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct vtpbc_softc *sc = (void *) self;
	struct pcibus_attach_args pba;
	struct vtpbc_config *vt;

	/*
	 * There is only one PCI controller on an Algorithmics board.
	 */
	vt = &vtpbc_configuration;
	sc->sc_vtpbc = vt;

	if (vt->vt_rev < vtpbc_nrevs)
		printf(": V3 V962, revision %s\n", vtpbc_revs[vt->vt_rev]);
	else
		printf(": V3 V962, unknown revision %d\n", vt->vt_rev);

	printf("%s: PCI memory space base: 0x%08lx\n", sc->sc_dev.dv_xname,
	    (u_long) vt->vt_pci_membase);
	printf("%s: PCI DMA window base: 0x%08lx\n", sc->sc_dev.dv_xname,
	    (u_long) vt->vt_dma_winbase);

	pba.pba_busname = "pci";
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;
#if defined(ALGOR_P4032)
	    {
		struct p4032_config *acp = &p4032_configuration;

		pba.pba_iot = &acp->ac_iot;
		pba.pba_memt = &acp->ac_memt;
		pba.pba_dmat = &acp->ac_pci_dmat;
		pba.pba_pc = &acp->ac_pc;
	    }
#elif defined(ALGOR_P5064)
	    {
		struct p5064_config *acp = &p5064_configuration;

		pba.pba_iot = &acp->ac_iot;
		pba.pba_memt = &acp->ac_memt;
		pba.pba_dmat = &acp->ac_pci_dmat;
		pba.pba_pc = &acp->ac_pc;
	    }
#endif /* ALGOR_P4032 || ALGOR_P5064 */

	(void) config_found(self, &pba, vtpbc_mainbus_print);
}

int
vtpbc_mainbus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to VTPBCs; easy. */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
