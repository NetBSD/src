/* $NetBSD: if_cs_isapnp.c,v 1.18.24.2 2016/10/05 20:55:42 skrll Exp $ */

/*-
 * Copyright (c)2001 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs_isapnp.c,v 1.18.24.2 2016/10/05 20:55:42 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/isa/isavar.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

#define DEVNAME(sc) device_xname((sc)->sc_dev)

static int cs_isapnp_match(device_t, cfdata_t, void *);
static void cs_isapnp_attach(device_t, device_t, void *);

#ifdef notyet
CFATTACH_DECL_NEW(cs_isapnp, sizeof(struct cs_softc_isa),
    cs_isapnp_match, cs_isapnp_attach, NULL, NULL);
#else
CFATTACH_DECL_NEW(cs_isapnp, sizeof(struct cs_softc),
    cs_isapnp_match, cs_isapnp_attach, NULL, NULL);
#endif

int
cs_isapnp_match(device_t parent, cfdata_t match, void *aux)
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_cs_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return pri;
}

void
cs_isapnp_attach(device_t parent, device_t self, void *aux)
{
#ifdef notyet
	struct cs_softc_isa *isc = device_private(sc);
	struct cs_softc *sc = &sc->sc_cs;
#else
	struct cs_softc *sc = device_private(self);
#endif
	struct isapnp_attach_args *ipa = aux;
#ifdef notyet
	int i;
#endif

	sc->sc_dev = self;

	printf("\n");

	if (ipa->ipa_nio != 1 || ipa->ipa_nirq != 1 || ipa->ipa_ndrq) {
		aprint_error_dev(self, "unexpected resource requirements\n");
		return;
	}

	if (ipa->ipa_io[0].length != CS8900_IOSIZE) {
		aprint_error_dev(self, "unexpected io size\n");
		return;
	}

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		aprint_error_dev(self, "unable to allocate resources\n");
		return;
	}

	aprint_normal_dev(self, "%s %s\n", ipa->ipa_devident,
	    ipa->ipa_devclass);

#ifdef notyet
#ifdef DEBUG
	printf("%s: nio=%u, nmem=%u, nmem32=%u, ndrq=%u, nirq=%u\n",
	    DEVNAME(sc), ipa->ipa_nio, ipa->ipa_nmem, ipa->ipa_nmem32,
	    ipa->ipa_ndrq, ipa->ipa_nirq);
#endif
	isc->sc_ic = ipa->ipa_ic;
	isc->sc_drq = -1;
#endif
	sc->sc_iot = ipa->ipa_iot;
	sc->sc_ioh = ipa->ipa_io[0].h;
	sc->sc_irq = ipa->ipa_irq[0].num;

#ifdef notyet
	for (i = 0; i < ipa->ipa_nmem; i++) {
		if (ipa->ipa_mem[i].length == CS8900_MEMSIZE) {
#if 0
			u_int16_t id;

			id = CS_READ_PACKET_PAGE_MEM(sc, PKTPG_EISA_NUM);
			if (id != EISA_NUM_CRYSTAL) {
				aprint_verbose_dev(self, "unexpected id(%u)\n",
				    id);
				continue;
			}
			aprint_verbose_dev(self,"correct id(%u) from mem=%u\n",
			    id, (u_int)ipa->ipa_mem[i].h);
#endif

			sc->sc_memt = ipa->ipa_memt;
			sc->sc_memh = ipa->ipa_mem[i].h;
			sc->sc_pktpgaddr = ipa->ipa_mem[i].base;
			sc->sc_cfgflags |= CFGFLG_MEM_MODE;
			aprint_normal_dev(self, "memory mode\n");
			break;
		}
	}
#endif

	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
		ipa->ipa_irq[0].type, IPL_NET, cs_intr, sc);
	if (sc->sc_ih == 0) {
		aprint_error_dev(self, "unable to establish interrupt\n");
		goto fail;
	}

	if (cs_attach(sc, 0, 0, 0, 0)) {
		aprint_error_dev(self, "unable to attach\n");
		goto fail;
	}

	return;

fail:
	if (sc->sc_ih)
		isa_intr_disestablish(ipa->ipa_ic, sc->sc_ih);
	isapnp_unconfig(ipa->ipa_iot, ipa->ipa_memt, ipa);
}
