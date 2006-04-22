/* $NetBSD: vesa_text.c,v 1.6.6.1 2006/04/22 11:37:31 simonb Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vesa_text.c,v 1.6.6.1 2006/04/22 11:37:31 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <machine/frame.h>
#include <machine/kvm86.h>
#include <machine/bus.h>

#include <arch/i386/bios/vesabios.h>
#include <arch/i386/bios/vesabiosreg.h>

static int vesatext_match(struct device *, struct cfdata *, void *);
static void vesatext_attach(struct device *, struct device *, void *);

struct vesatextsc {
	struct device sc_dev;
	int *sc_modes;
	int sc_nmodes;
};

CFATTACH_DECL(vesatext, sizeof(struct vesatextsc),
    vesatext_match, vesatext_attach, NULL, NULL);

static int
vesatext_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct vesabiosdev_attach_args *vaa = aux;

	if (strcmp(vaa->vbaa_type, "text"))
		return (0);

	return (1);
}

static void
vesatext_attach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
	struct vesatextsc *sc = (struct vesatextsc *)dev;
	struct vesabiosdev_attach_args *vaa = aux;
	int i;

	aprint_naive("\n");
	aprint_normal(": VESA text device\n");

	sc->sc_modes = malloc(vaa->vbaa_nmodes * sizeof(int),
			      M_DEVBUF, M_NOWAIT);
	sc->sc_nmodes = vaa->vbaa_nmodes;
	for (i = 0; i < vaa->vbaa_nmodes; i++)
		sc->sc_modes[i] = vaa->vbaa_modes[i];
}
