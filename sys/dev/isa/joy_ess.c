/* $NetBSD: joy_ess.c,v 1.1.14.2 2008/01/21 09:43:19 yamt Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: joy_ess.c,v 1.1.14.2 2008/01/21 09:43:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <sys/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/essvar.h>
#include <dev/ic/joyvar.h>

int joy_ess_match(struct device *, struct cfdata *, void *);
void joy_ess_attach(struct device *, struct device *, void *);

CFATTACH_DECL(joy_ess, sizeof (struct joy_softc),
	      joy_ess_match, joy_ess_attach, NULL, NULL);

int
joy_ess_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct audio_attach_args *aa = aux;

	if (aa->type != AUDIODEV_TYPE_AUX)
		return (0);
	return (1);
}

void
joy_ess_attach(struct device *parent, struct device *self, void *aux)
{
	struct ess_softc *esc = (struct ess_softc *)parent;
	struct joy_softc *sc = (struct joy_softc *)self;

	printf("\n");

	sc->sc_iot = esc->sc_joy_iot;
	sc->sc_ioh = esc->sc_joy_ioh;

	joyattach(sc);
}
