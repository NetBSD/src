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

#include <dev/isa/mpuvar.h>

int	mpu_isapnp_match __P((struct device *, struct cfdata *, void *));
void	mpu_isapnp_attach __P((struct device *, struct device *, void *));

struct mpu_isapnp_softc {
	struct device sc_dev;
	void *sc_ih;

	struct mpu_softc sc_mpu;
};

struct cfattach mpu_isapnp_ca = {
	sizeof(struct mpu_isapnp_softc), mpu_isapnp_match, mpu_isapnp_attach
};

int
mpu_isapnp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (isapnp_devmatch(aux, &isapnp_mpu_devinfo));
}

void
mpu_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mpu_isapnp_softc *sc = (struct mpu_isapnp_softc *)self;
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

	midi_attach_mi(&mpu_midi_hw_if, &sc->sc_mpu, &sc->sc_dev);

	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_AUDIO, mpu_intr, &sc->sc_mpu);
}
