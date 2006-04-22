/*	$NetBSD: ess_isapnp.c,v 1.13.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ess_isapnp.c,v 1.13.6.1 2006/04/22 11:39:09 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

#include <dev/isa/essreg.h>
#include <dev/isa/essvar.h>

int	ess_isapnp_match(struct device *, struct cfdata *, void *);
void	ess_isapnp_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ess_isapnp, sizeof(struct ess_softc),
    ess_isapnp_match, ess_isapnp_attach, NULL, NULL);

/*
 * Probe / attach routines.
 */

/*
 * Probe for the ess hardware.
 */
int
ess_isapnp_match(struct device *parent, struct cfdata *match, void *aux)
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_ess_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return pri;
}


/*
 * Attach hardware to driver, attach hardware driver to audio
 * pseudo-device driver.
 */
void
ess_isapnp_attach(struct device *parent, struct device *self, void *aux)
{
	struct ess_softc *sc;
	struct isapnp_attach_args *ipa;

	sc = device_private(self);
	ipa = aux;
	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_ic = ipa->ipa_ic;

	sc->sc_iot = ipa->ipa_iot;
	sc->sc_iobase = ipa->ipa_io[0].base;
	sc->sc_ioh = ipa->ipa_io[0].h;

	sc->sc_audio1.irq = ipa->ipa_irq[0].num;
	sc->sc_audio1.ist = ipa->ipa_irq[0].type;
	sc->sc_audio1.drq = ipa->ipa_drq[0].num;
	sc->sc_audio2.irq = -1;
	sc->sc_audio2.drq = ipa->ipa_drq[1].num;

	if (!essmatch(sc)) {
		printf("%s: essmatch failed\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s:", sc->sc_dev.dv_xname);

	essattach(sc, 0);
}
