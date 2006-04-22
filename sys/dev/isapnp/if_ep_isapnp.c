/*	$NetBSD: if_ep_isapnp.c,v 1.28.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone <jonathan@NetBSD.org>
 * Copyright (c) 1997 Charles M. Hannum.  All rights reserved.
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
 *      This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: if_ep_isapnp.c,v 1.28.6.1 2006/04/22 11:39:09 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

#include <dev/mii/miivar.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>

int ep_isapnp_match(struct device *, struct cfdata *, void *);
void ep_isapnp_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ep_isapnp, sizeof(struct ep_softc),
    ep_isapnp_match, ep_isapnp_attach, NULL, NULL);

int
ep_isapnp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_ep_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return (pri);
}

void
ep_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ep_softc *sc = device_private(self);
	struct isapnp_attach_args *ipa = aux;
	int chipset;

	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: %s %s\n", sc->sc_dev.dv_xname, ipa->ipa_devident,
	    ipa->ipa_devclass);

	sc->sc_iot = ipa->ipa_iot;
	sc->sc_ioh = ipa->ipa_io[0].h;
	sc->bustype = ELINK_BUS_ISA;

	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_NET, epintr, sc);

	sc->enable = NULL;
	sc->disable = NULL;
	sc->enabled = 1;

	if (strcmp(ipa->ipa_devlogic, "TCM5051") == 0) {
		/* 3c515 */
		chipset = ELINK_CHIPSET_CORKSCREW;
	} else {
		/* 3c509 */
		chipset = ELINK_CHIPSET_3C509;
	}

	epconfig(sc, chipset, NULL);
}
