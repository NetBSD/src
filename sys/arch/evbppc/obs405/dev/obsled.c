/*	$NetBSD: obsled.c,v 1.1 2005/01/24 18:47:37 shige Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obsled.c,v 1.1 2005/01/24 18:47:37 shige Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <machine/obs405.h>

#include <powerpc/ibm4xx/dev/gpiovar.h>

struct obsled_softc {
        struct device sc_dev;
	gpio_tag_t sc_tag;
        int sc_addr;
};

static void	obsled_attach(struct device *, struct device *, void *);
static int	obsled_match(struct device *, struct cfdata *, void *);

CFATTACH_DECL(obsled, sizeof(struct obsled_softc),
	obsled_match, obsled_attach, NULL, NULL);

static int
obsled_match(struct device *parent, struct cfdata *cf, void *aux)
{
        struct gpio_attach_args *ga = aux;

	/* XXX: support only OpenBlockS266 LED */
        if (ga->ga_addr == OBS405_GPIO_LED1)
                return (1);
        else if (ga->ga_addr == OBS405_GPIO_LED2)
                return (1);
        else if (ga->ga_addr == OBS405_GPIO_LED4)
                return (1);

        return (0);
}

static void
obsled_attach(struct device *parent, struct device *self, void *aux)
{
        struct obsled_softc *sc = (struct obsled_softc *)self;
        struct gpio_attach_args *ga = aux;
	int led = (1 << sc->sc_dev.dv_unit);

        aprint_naive(": OpenBlockS LED%d\n", led);
        aprint_normal(": OpenBlockS LED%d\n", led);

        sc->sc_tag = ga->ga_tag;
        sc->sc_addr = ga->ga_addr;

	obs405_led_set(OBS405_LED_OFF);
#if 0
	{
		gpio_tag_t tag = sc->sc_tag;
	 	(*(tag)->io_or_write)((tag)->cookie, sc->sc_addr, 0);
	}
#endif
}

void
obs405_led_set(int led)
{
	struct device *dp = NULL;
	struct devicelist *dlp = &alldevs;

        for (dp = TAILQ_FIRST(dlp); dp != NULL; dp = TAILQ_NEXT(dp, dv_list)) {
		if (dp->dv_cfdata != NULL
			&& strcmp(dp->dv_cfdata->cf_name, "obsled") == 0) {
			struct obsled_softc *sc = (struct obsled_softc *)dp;
			gpio_tag_t tag = sc->sc_tag;
			int bit = (led & (1 << dp->dv_unit)) >> dp->dv_unit;
	 		(*(tag)->io_or_write)((tag)->cookie,
					sc->sc_addr, (~bit & 1));
		}
	}
}
