/*	$NetBSD: if_ep_isapnp.c,v 1.7 1997/04/27 09:46:50 veego Exp $	*/

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

#include "bpfilter.h" 
 
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

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h> 
#endif
 
#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif
  
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>

#ifdef __BROKEN_INDIRECT_CONFIG
int ep_isapnp_match __P((struct device *, void *, void *));
#else
int ep_isapnp_match __P((struct device *, struct cfdata *, void *));
#endif
void ep_isapnp_attach __P((struct device *, struct device *, void *));

struct cfattach ep_isapnp_ca = {
	sizeof(struct ep_softc), ep_isapnp_match, ep_isapnp_attach
};

int
ep_isapnp_match(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct isapnp_attach_args *ipa = aux;

	if (strcmp(ipa->ipa_devlogic, "TCM5090") &&
	    strcmp(ipa->ipa_devlogic, "TCM5091") &&
	    strcmp(ipa->ipa_devlogic, "TCM5094") &&
	    strcmp(ipa->ipa_devlogic, "TCM5095") &&
	    strcmp(ipa->ipa_devlogic, "TCM5098"))
		return (0);

	return (1);
}

void
ep_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ep_softc *sc = (void *)self;
	struct isapnp_attach_args *ipa = aux;
	u_short conn = 0;

	printf("\n");

	sc->sc_iot = ipa->ipa_iot;
	sc->sc_ioh = ipa->ipa_io[0].h;
	sc->bustype = EP_BUS_ISA;

	GO_WINDOW(0);
	conn = bus_space_read_2(sc->sc_iot, sc->sc_ioh, EP_W0_CONFIG_CTRL);

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: %s %s\n", sc->sc_dev.dv_xname, ipa->ipa_devident,
	    ipa->ipa_devclass);

	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    IST_EDGE, IPL_NET, epintr, sc);

	if (strcmp(ipa->ipa_devlogic, "TCM5090") &&
	    strcmp(ipa->ipa_devlogic, "TCM5091") &&
	    strcmp(ipa->ipa_devlogic, "TCM5094") &&
	    strcmp(ipa->ipa_devlogic, "TCM5095") &&
	    strcmp(ipa->ipa_devlogic, "TCM5098")) {
		epconfig(sc, EP_CHIPSET_UNKNOWN);	/* XXX: 3c515 ? */
	} else {
		epconfig(sc, EP_CHIPSET_3C509);
	}
}
