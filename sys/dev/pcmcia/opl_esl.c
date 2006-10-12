/* $NetBSD: opl_esl.c,v 1.13 2006/10/12 01:31:50 christos Exp $ */

/*
 * Copyright (c) 2001 Jared D. McNeill <jmcneill@invisible.ca>
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
 *	This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of the author nor the names of any contributors may
 f
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: opl_esl.c,v 1.13 2006/10/12 01:31:50 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/select.h>
#include <sys/audioio.h>
#include <sys/midiio.h>

#include <machine/bus.h>

#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/ic/oplreg.h>
#include <dev/ic/oplvar.h>

#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/eslvar.h>

int	opl_esl_match(struct device *, struct cfdata *, void *);
void	opl_esl_attach(struct device *, struct device *, void *);
int	opl_esl_detach(struct device *, int);

CFATTACH_DECL(opl_esl, sizeof(struct opl_softc),
    opl_esl_match, opl_esl_attach, opl_esl_detach, NULL);

int
opl_esl_match(struct device *parent, struct cfdata *match __unused, void *aux)
{
	struct audio_attach_args *aa = (struct audio_attach_args *)aux;
	struct esl_pcmcia_softc *ssc = (struct esl_pcmcia_softc *)parent;

	if (aa->type != AUDIODEV_TYPE_OPL)
		return (0);

	return opl_match(ssc->sc_iot, ssc->sc_ioh, 0);
}

void
opl_esl_attach(struct device *parent, struct device *self, void *aux __unused)
{
	struct esl_pcmcia_softc *ssc = (struct esl_pcmcia_softc *)parent;
	struct opl_softc *sc = (struct opl_softc *)self;

	sc->iot = ssc->sc_iot;
	sc->ioh = ssc->sc_ioh;
	sc->offs = 0;
	strcpy(sc->syn.name, "ESS AudioDrive ");

	opl_attach(sc);
}

int
opl_esl_detach(struct device *self, int flags)
{
	struct opl_softc *sc = (struct opl_softc *)self;
	int rv;

	rv = opl_detach(sc, flags);

	return (rv);
}
