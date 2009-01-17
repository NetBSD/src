/*	$NetBSD: obio_lpchc.c,v 1.2.4.2 2009/01/17 13:27:53 mjf Exp $	*/

/*
 * obio attachment for GEMINI LPC Host Controller
 */
#include "opt_gemini.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio_lpchc.c,v 1.2.4.2 2009/01/17 13:27:53 mjf Exp $");

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/param.h>
#include <machine/bus.h>

#include <arm/gemini/gemini_obiovar.h>
#include <arm/gemini/gemini_lpcvar.h>
#include <arm/gemini/gemini_lpchcvar.h>
#include <arm/gemini/gemini_reg.h>

static int  gemini_lpchc_match(struct device *, struct cfdata *, void *);
static void gemini_lpchc_attach(struct device *, struct device *, void *);
static int  gemini_lpchc_print(void *, const char *);

CFATTACH_DECL_NEW(obio_lpchc, sizeof(struct gemini_lpchc_softc),
    gemini_lpchc_match, gemini_lpchc_attach, NULL, NULL);


static int
gemini_lpchc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr == OBIOCF_ADDR_DEFAULT)
		panic("geminilpchc must have addr specified in config.");

	if (obio->obio_addr == GEMINI_LPCHC_BASE)
		return 1;

	return 0;
}

static void
gemini_lpchc_attach(struct device *parent, struct device *self, void *aux)
{
	gemini_lpchc_softc_t *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct gemini_lpchc_attach_args lpchc;
	uint32_t r;
	void *ih=NULL;
	
	sc->sc_dev = self;
	sc->sc_addr = obio->obio_addr;
	sc->sc_size = (obio->obio_size == OBIOCF_SIZE_DEFAULT)
		? GEMINI_LPCHC_SIZE : obio->obio_size;

	sc->sc_iot = obio->obio_iot;

	if (bus_space_map(sc->sc_iot, sc->sc_addr, sc->sc_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_LPCHC_ID);
	aprint_normal("\n%s: device %d, rev %#x ",
		device_xname(self), _LPCHC_ID_DEVICE(r), _LPCHC_ID_REV(r));


	sc->sc_intr = obio->obio_intr;
	if (obio->obio_intr != OBIOCF_INTR_DEFAULT) {
		ih = intr_establish(obio->obio_intr, IPL_SERIAL,
			IST_LEVEL_HIGH, gemini_lpchc_intr, sc);
		if (ih == NULL)
			panic("%s: cannot register intr %d",
				device_xname(self), obio->obio_intr);
	}
	sc->sc_ih = ih;

	gemini_lpchc_init(sc);

	aprint_normal("\n");
	aprint_naive("\n"); 

	lpchc.lpchc_iot  = obio->obio_iot;
	lpchc.lpchc_addr = GEMINI_LPCIO_BASE;	/* XXX sc_addr+offset */
	lpchc.lpchc_size = LPCCF_SIZE_DEFAULT;	/* XXX placeholder */
	lpchc.lpchc_tag  = sc;

	(void)config_found_ia(sc->sc_dev, "lpcbus", &lpchc, gemini_lpchc_print);
}

int
gemini_lpchc_print(void *aux, const char *name)
{
	struct gemini_lpchc_attach_args *lpchc = aux;

	if (lpchc->lpchc_addr != LPCCF_ADDR_DEFAULT)
		aprint_normal(" addr %#lx", lpchc->lpchc_addr);

	return UNCONF;
}
