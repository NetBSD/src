/*	$NetBSD: dc_ibus.c,v 1.1.2.1 1998/10/26 10:52:52 nisimura Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dc_ibus.c,v 1.1.2.1 1998/10/26 10:52:52 nisimura Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>
#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/dc7085reg.h>	/* XXX dev/dec/dc7085reg.h XXX */
#include <pmax/ibus/dc7085var.h>

int  dcibus_match	__P((struct device *, struct cfdata *, void *));
void dcibus_attach	__P((struct device *, struct device *, void *));

struct cfattach dc_ibus_ca = {
	sizeof(struct dc_softc), dcibus_match, dcibus_attach
};

int dc_major = 16;		/* EXPORT */
int dcintr __P((void *));	/* IMPORT */

int
dcibus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ibus_attach_args *d = aux;

	if (strcmp(d->ia_name, "dc") != 0 &&
	    strcmp(d->ia_name, "mdc") != 0 &&
	    strcmp(d->ia_name, "dc7085") != 0)
		return 0;

	if (badaddr((caddr_t)d->ia_addr, 2))
		return 0;

	return 1;
}

void
dcibus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ibus_attach_args *d = aux;
	struct dc_softc *sc = (struct dc_softc*)self;

	sc->sc_reg = (void *)d->ia_addr;

	ibus_intr_establish(parent, d->ia_cookie, IPL_TTY, dcintr, sc);

	printf(": DC7085\n");
}
