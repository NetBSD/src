/*	$Id: mpcsa_cf.c,v 1.2.4.2 2008/09/18 04:33:24 wrstuden Exp $	*/
/*	$NetBSD: mpcsa_cf.c,v 1.2.4.2 2008/09/18 04:33:24 wrstuden Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpcsa_cf.c,v 1.2.4.2 2008/09/18 04:33:24 wrstuden Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91cfvar.h>
#include <arm/at91/at91piovar.h>

#ifdef MPCA_CF_DEBUG
int mpcsa_cf_debug = MPCA_CF_DEBUG;
#define DPRINTFN(n,x)	if (mpcsa_cf_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)   
#endif  

struct at91_gpio_softc;

struct mpcsa_cf_softc {
	struct at91cf_softc	sc_dev;
	struct at91pio_softc	*sc_pioc;
};

static int mpcsa_cf_match(struct device *, struct cfdata *, void *);
static void mpcsa_cf_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(mpcsa_cf, sizeof(struct mpcsa_cf_softc),
	      mpcsa_cf_match, mpcsa_cf_attach, NULL, NULL);

static int	mpcsa_cf_power_ctl(void *self, int onoff);
static int	mpcsa_cf_card_detect(void *self);
static int	mpcsa_cf_irq_line(void *self);
static void *	mpcsa_cf_intr_establish(void *self, int which, int ipl, int (*ih_func)(void *), void *arg);
static void	mpcsa_cf_intr_disestablish(void *self, int which, void *cookie);

struct at91cf_chipset_tag mpcsa_cf_tag = {
	mpcsa_cf_power_ctl,
	mpcsa_cf_card_detect,
	mpcsa_cf_irq_line,
	mpcsa_cf_intr_establish,
	mpcsa_cf_intr_disestablish
};

static int
mpcsa_cf_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "at91cf") == 0 && strcmp(match->cf_atname, "mpcsa_cf") == 0)
		return 2;
	return 0;
}

static void
mpcsa_cf_attach(struct device *parent, struct device *self, void *aux)
{
	struct mpcsa_cf_softc *sc = device_private(self);
//	at91cf_chipset_tag_t cscf = sc->sc_dev.sc_cscf;
	struct at91pio_softc *pioc;

	if ((pioc = at91pio_sc(AT91_PIOC)) == NULL) {
		printf("no PIOC!\n");
		return;
	}

	// configure PIO!
	sc->sc_pioc = pioc;
	at91pio_in(sc->sc_pioc, 3);	// CFIRQ
	at91pio_in(sc->sc_pioc, 4);	// CFCD
	at91pio_out(sc->sc_pioc, 5);	// CFRESET

	// and call common routine
	at91cf_attach_common(parent, self, aux, &mpcsa_cf_tag);
}

static int
mpcsa_cf_power_ctl(void *self, int onoff)
{
	struct mpcsa_cf_softc *sc = self;
	if (onoff) {
		at91pio_set(sc->sc_pioc, 5);
		return 20000;
	} else {
		at91pio_clear(sc->sc_pioc, 5);
		return 20000;
	}
}

static int	mpcsa_cf_card_detect(void *self)
{
	struct mpcsa_cf_softc *sc = self;
	return at91pio_read(sc->sc_pioc, 4) == 0;
}

static int	mpcsa_cf_irq_line(void *self)
{
	struct mpcsa_cf_softc *sc = self;
	return at91pio_read(sc->sc_pioc, 3);
}

static void *	mpcsa_cf_intr_establish(void *self, int which, int ipl, int (*ih_func)(void *), void *ih_arg)
{
	struct mpcsa_cf_softc *sc = self;
	return at91pio_intr_establish(sc->sc_pioc, which == CD_IRQ ? 4 : 3,
					ipl, ih_func, ih_arg);
}

static void	mpcsa_cf_intr_disestablish(void *self, int which, void *cookie)
{
	struct mpcsa_cf_softc *sc = self;
	at91pio_intr_disestablish(sc->sc_pioc, which == CD_IRQ ? 4 : 3, cookie);
}

