/* $NetBSD: vesa_text.c,v 1.8.52.1 2008/10/19 22:15:48 haad Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vesa_text.c,v 1.8.52.1 2008/10/19 22:15:48 haad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <machine/frame.h>
#include <machine/kvm86.h>
#include <machine/bus.h>

#include <arch/i386/bios/vesabios.h>
#include <arch/i386/bios/vesabiosreg.h>

static int vesatext_match(device_t, cfdata_t, void *);
static void vesatext_attach(device_t, device_t, void *);

struct vesatextsc {
	int *sc_modes;
	int sc_nmodes;
};

CFATTACH_DECL_NEW(vesatext, sizeof(struct vesatextsc),
    vesatext_match, vesatext_attach, NULL, NULL);

static int
vesatext_match(device_t parent, cfdata_t match, void *aux)
{
	struct vesabiosdev_attach_args *vaa = aux;

	if (strcmp(vaa->vbaa_type, "text"))
		return 0;

	return 1;
}

static void
vesatext_attach(device_t parent, device_t self, void *aux)
{
	struct vesatextsc *sc = device_private(self);
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
