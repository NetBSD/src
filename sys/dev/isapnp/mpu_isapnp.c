/*	$NetBSD: mpu_isapnp.c,v 1.11.8.2 2006/05/24 10:57:53 yamt Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpu_isapnp.c,v 1.11.8.2 2006/05/24 10:57:53 yamt Exp $");

#include "midi.h"

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
#include <dev/midi_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

#include <dev/ic/mpuvar.h>

int	mpu_isapnp_match(struct device *, struct cfdata *, void *);
void	mpu_isapnp_attach(struct device *, struct device *, void *);

struct mpu_isapnp_softc {
	struct device sc_dev;
	void *sc_ih;

	struct mpu_softc sc_mpu;
};

CFATTACH_DECL(mpu_isapnp, sizeof(struct mpu_isapnp_softc),
    mpu_isapnp_match, mpu_isapnp_attach, NULL, NULL);

int
mpu_isapnp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_mpu_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return (pri);
}

void
mpu_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mpu_isapnp_softc *sc = device_private(self);
	struct isapnp_attach_args *ipa = aux;

	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_mpu.iot = ipa->ipa_iot;
	sc->sc_mpu.ioh = ipa->ipa_io[0].h;

	if (!mpu_find(&sc->sc_mpu)) {
		printf("%s: find failed\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: %s %s\n", sc->sc_dev.dv_xname, ipa->ipa_devident,
	       ipa->ipa_devclass);

	sc->sc_mpu.model = "Roland MPU-401 MIDI UART";

	midi_attach_mi(&mpu_midi_hw_if, &sc->sc_mpu, &sc->sc_dev);

	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_AUDIO, mpu_intr, &sc->sc_mpu);
}
