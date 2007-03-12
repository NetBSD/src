/* $NetBSD: g42xxeb_lcd.c,v 1.7.2.1 2007/03/12 05:47:37 rmind Exp $ */

/*-
 * Copyright (c) 2001, 2002, 2005 Genetec corp.
 * All rights reserved.
 *
 * LCD driver for Genetec G4250EB-X002.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "opt_g42xxlcd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#include <dev/cons.h> 
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h> 
#include <dev/wscons/wscons_callbacks.h>

#include <machine/bus.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_lcd.h>

#include <arch/evbarm/g42xxeb/g42xxeb_reg.h>
#include <arch/evbarm/g42xxeb/g42xxeb_var.h>

#include "wsdisplay.h"
#include "ioconf.h"

int	lcd_match( struct device *, struct cfdata *, void *);
void	lcd_attach( struct device *, struct device *, void *);
int	lcdintr(void *);

#if NWSDISPLAY > 0

/*
 * wsdisplay glue
 */
struct pxa2x0_wsscreen_descr lcd_bpp16_screen = {
	{
		"bpp16", 0, 0,
		&pxa2x0_lcd_emulops,
		0, 0,
		WSSCREEN_WSCOLORS,
	},
	16				/* bits per pixel */
}, lcd_bpp8_screen = {
	{
		"bpp8", 0, 0,
		&pxa2x0_lcd_emulops,
		0, 0,
		WSSCREEN_WSCOLORS,
	},
	8				/* bits per pixel */
}, lcd_bpp4_screen = {
	{
		"bpp4", 0, 0,
		&pxa2x0_lcd_emulops,
		0, 0,
		WSSCREEN_WSCOLORS,
	},
	4				/* bits per pixel */
};


static const struct wsscreen_descr *lcd_scr_descr[] = {
	&lcd_bpp4_screen.c,
	&lcd_bpp8_screen.c,
	&lcd_bpp16_screen.c,
};

const struct wsscreen_list lcd_screen_list = {
	sizeof lcd_scr_descr / sizeof lcd_scr_descr[0],
	lcd_scr_descr
};

int	lcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);

int	lcd_show_screen(void *, void *, int,
	    void (*)(void *, int, int), void *);

const struct wsdisplay_accessops lcd_accessops = {
	lcd_ioctl,
	pxa2x0_lcd_mmap,
	pxa2x0_lcd_alloc_screen,
	pxa2x0_lcd_free_screen,
	lcd_show_screen,
	NULL, /* load_font */
};

#else
/*
 * Interface to LCD framebuffer without wscons
 */
dev_type_open(lcdopen);
dev_type_close(lcdclose);
dev_type_ioctl(lcdioctl);
dev_type_mmap(lcdmmap);
const struct cdevsw lcd_cdevsw = {
	lcdopen, lcdclose, noread, nowrite,
	lcdioctl, nostop, notty, nopoll, lcdmmap, D_TTY
};

#endif

CFATTACH_DECL(lcd_obio, sizeof (struct pxa2x0_lcd_softc), lcd_match, lcd_attach,
    NULL, NULL);

int
lcd_match( struct device *parent, struct cfdata *cf, void *aux )
{
	return 1;
}

#ifdef G4250_LCD_NEC_NL3224BC35
/* NEC's QVGA LCD */
static const struct lcd_panel_geometry nec_NL3224BC35 =
{
    320,			/* Width */
    240,			/* Height */
    0,				/* No extra lines */

    LCDPANEL_SINGLE|LCDPANEL_ACTIVE|LCDPANEL_PCP|
    LCDPANEL_VSP|LCDPANEL_HSP,
    7,				/* clock divider, 6.25MHz at 100MHz */
    0xff,			/* AC bias pin freq */

    6,				/* horizontal sync pulse width */
    70,				/* BLW */
    7,				/* ELW */

    10,				/* vertical sync pulse width */
    11,				/* BFW */
    1,				/* EFW */
};
#endif

#ifdef G4250_LCD_TOSHIBA_LTM035
const struct lcd_panel_geometry toshiba_LTM035 =
{
    240,			/* Width */
    320,			/* Height */
    0,				/* No extra lines */

    LCDPANEL_SINGLE|LCDPANEL_ACTIVE|   /* LCDPANEL_PCP| */
    LCDPANEL_VSP|LCDPANEL_HSP,
    11,				/* clock divider, 4.5 MHz at 100 MHz */
				/* required: 4.28..4.7 MHz  */
    0xff,			/* AC bias pin freq */

    4,				/* horizontal sync pulse width */
    8,				/* BLW (back porch) */
    4,				/* ELW (front porch) */

    2,				/* vertical sync pulse width */
    2,				/* BFW (back porch) */
    3,				/* EFW (front porch) */

};
#endif /* G4250_LCD_TOSHIBA_LTM035 */

void lcd_attach( struct device *parent, struct device *self, void *aux )
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)self;

#ifdef G4250_LCD_TOSHIBA_LTM035
# define PANEL	toshiba_LTM035
#else
# define PANEL	nec_NL3224BC35
#endif

	pxa2x0_lcd_attach_sub(sc, aux, &PANEL);


#if NWSDISPLAY > 0

	{
		struct wsemuldisplaydev_attach_args aa;

		/* make wsdisplay screen list */
		pxa2x0_lcd_setup_wsscreen(&lcd_bpp16_screen, &PANEL, NULL);
		pxa2x0_lcd_setup_wsscreen(&lcd_bpp8_screen, &PANEL, NULL);
		pxa2x0_lcd_setup_wsscreen(&lcd_bpp4_screen, &PANEL, NULL);

		aa.console = 0;
		aa.scrdata = &lcd_screen_list;
		aa.accessops = &lcd_accessops;
		aa.accesscookie = sc;

		printf("\n");

		(void) config_found(self, &aa, wsemuldisplaydevprint);
	}
#else
	{
		struct pxa2x0_lcd_screen *screen;
		int error;

		error = pxa2x0_lcd_new_screen( sc, 16, &screen );
		if (error == 0) {
			sc->active = screen;
			pxa2x0_lcd_start_dma(sc, screen);
		}

		printf("\n");
	}
#endif

#undef PANEL

}

#if NWSDISPLAY > 0

int
lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct obio_softc *osc = 
	    (struct obio_softc *) device_parent((struct device *)v);
	uint16_t reg;

	switch (cmd) {
	case WSDISPLAYIO_SVIDEO:
		reg = bus_space_read_2(osc->sc_iot, osc->sc_obioreg_ioh,
		    G42XXEB_LCDCTL);
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON)
			reg |= LCDCTL_BL_ON;
		else
			reg &= ~LCDCTL_BL_ON;
		bus_space_write_2(osc->sc_iot, osc->sc_obioreg_ioh,
			G42XXEB_LCDCTL, reg);
		bus_space_write_1(osc->sc_iot, osc->sc_obioreg_ioh,
			G42XXEB_LED, reg);
		printf("LCD control: %x\n", reg);
		break;			/* turn on/off LCD controller */
	}

	return pxa2x0_lcd_ioctl(v, vs, cmd, data, flag, l);
}

int
lcd_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct obio_softc *osc = 
	    (struct obio_softc *) device_parent((struct device *)v);
	uint16_t reg;

	pxa2x0_lcd_show_screen(v,cookie,waitok,cb,cbarg);
	
	/* Turn on LCD backlight.
	   XXX: with fixed blightness. want new ioctl to set blightness. */
	reg = bus_space_read_2(osc->sc_iot, osc->sc_obioreg_ioh, G42XXEB_LCDCTL);
	bus_space_write_2(osc->sc_iot, osc->sc_obioreg_ioh, G42XXEB_LCDCTL,
	    (reg & ~LCDCTL_BL_PWN) | 0x4000 | LCDCTL_BL_ON);

	return 0;
}



#else  /* NWSDISPLAY==0 */

int
lcdopen(dev_t dev, int oflags, int devtype, struct lwp *l)
{
	return 0;
}

int
lcdclose(dev_t dev, int fflag, int devtype, struct lwp *l)
{
	return 0;
}

paddr_t
lcdmmap(dev_t dev, off_t offset, int size)
{
	struct pxa2x0_lcd_softc *sc = device_lookup(&lcd_cd, minor(dev));
	struct pxa2x0_lcd_screen *scr = sc->active;

	return bus_dmamem_mmap( &pxa2x0_bus_dma_tag, scr->segs, scr->nsegs,
	    offset, 0, BUS_DMA_WAITOK|BUS_DMA_COHERENT );
}

int
lcdioctl(dev_t dev, u_long cmd, void *data,
	    int fflag, struct lwp *l)
{
	return EOPNOTSUPP;
}

#endif /* NWSDISPLAY>0 */
