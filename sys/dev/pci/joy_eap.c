/* $NetBSD: joy_eap.c,v 1.5 2006/10/12 01:31:31 christos Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <machine/bus.h>

#include <dev/pci/eapreg.h>
#include <dev/pci/eapvar.h>
#include <dev/ic/joyvar.h>

struct joy_eap_aa {
	struct audio_attach_args aa_aaa;
	bus_space_tag_t aa_iot;
	bus_space_handle_t aa_ioh;
};

struct device *
eap_joy_attach(struct device *eapdev, struct eap_gameport_args *gpa)
{
	int i;
	bus_space_handle_t ioh;
	u_int32_t icsc;
	struct joy_eap_aa aa;
	struct device *joydev;

	/*
	 * There are 4 possible locations. Just try to map one of them.
	 * XXX This is questionable for 2 reasons:
	 * - We don't know whether these addresses are usable on our
	 *   PCI bus (might be a secondary one).
	 * - PCI probing is early. ISA devices might conflict.
	 */
	for (i = 0; i < 4; i++) {
		if (bus_space_map(gpa->gpa_iot, 0x200 + i * 8, 1,
		    0, &ioh) == 0)
			break;
	}
	if (i == 4)
		return (0);

	printf("%s: enabling gameport at legacy io port 0x%x\n",
		eapdev->dv_xname, 0x200 + i * 8);

	/* enable gameport on eap */
	icsc = bus_space_read_4(gpa->gpa_iot, gpa->gpa_ioh, EAP_ICSC);
	icsc &= ~E1371_JOY_ASELBITS;
	icsc |= EAP_JYSTK_EN | E1371_JOY_ASEL(i);
	bus_space_write_4(gpa->gpa_iot, gpa->gpa_ioh, EAP_ICSC, icsc);

	aa.aa_aaa.type = AUDIODEV_TYPE_AUX;
	aa.aa_iot = gpa->gpa_iot;
	aa.aa_ioh = ioh;
	joydev = config_found(eapdev, &aa, 0);
	/* this cannot fail */
	KASSERT(joydev != NULL);

	return (joydev);
}

int
eap_joy_detach(struct device *joydev, struct eap_gameport_args *gpa)
{
	int res;
	struct joy_softc *sc = (struct joy_softc *)joydev;
	u_int32_t icsc;

	res = config_detach(joydev, 0);
	if (res)
		return (res);

	/* disable gameport on eap */
	icsc = bus_space_read_4(gpa->gpa_iot, gpa->gpa_ioh, EAP_ICSC);
	icsc &= ~EAP_JYSTK_EN;
	bus_space_write_4(gpa->gpa_iot, gpa->gpa_ioh, EAP_ICSC, icsc);

	bus_space_unmap(sc->sc_iot, sc->sc_ioh, 1);
	return (0);
}

static int
joy_eap_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct joy_eap_aa *eaa = aux;

	if (eaa->aa_aaa.type != AUDIODEV_TYPE_AUX)
		return (0);
	return (1);
}

static void
joy_eap_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct joy_softc *sc = (struct joy_softc *)self;
	struct joy_eap_aa *eaa = aux;

	printf("\n");

	sc->sc_iot = eaa->aa_iot;
	sc->sc_ioh = eaa->aa_ioh;

	joyattach(sc);
}

static int
joy_eap_detach(struct device *self, int flags)
{

	return (joydetach((struct joy_softc *)self, flags));
}

CFATTACH_DECL(joy_eap, sizeof (struct joy_softc),
	joy_eap_match, joy_eap_attach, joy_eap_detach, NULL);
