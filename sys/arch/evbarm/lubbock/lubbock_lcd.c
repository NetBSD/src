/* $NetBSD: lubbock_lcd.c,v 1.6.2.1 2007/03/12 05:47:38 rmind Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * LCD driver for Intel Lubbock.
 *
 * Controlling LCD is almost completely done through PXA2X0's
 * integrated LCD controller.  Codes for it is arm/xscale/pxa2x0_lcd.c.
 *
 * Codes in this file provide platform specific things including:
 *   LCD on/off switch in on-board PLD register.
 *   LCD panel geometry
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lubbock_lcd.c,v 1.6.2.1 2007/03/12 05:47:38 rmind Exp $");

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
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_lcd.h>

#include <arch/evbarm/lubbock/lubbock_reg.h>
#include <arch/evbarm/lubbock/lubbock_var.h>

#include "wsdisplay.h"

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
#if 0
	/* bpp4 needs a patch to rasops4 */
	&lcd_bpp4_screen.c,
#endif
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

CFATTACH_DECL(lcd_obio, sizeof (struct pxa2x0_lcd_softc),  lcd_match,
    lcd_attach, NULL, NULL);

int
lcd_match( struct device *parent, struct cfdata *cf, void *aux )
{
	return 1;
}

static const struct lcd_panel_geometry sharp_LM8V31 =
{
    640,			/* Width */
    480,			/* Height */
    0,				/* No extra lines */

    LCDPANEL_DUAL|LCDPANEL_PASSIVE|LCDPANEL_PCP,
    10,				/* clock divider */
    0xff,			/* AC bias pin freq */

    2,				/* horizontal sync pulse width */
    3,				/* BLW */
    3,				/* ELW */

    1,				/* vertical sync pulse width */
    0,				/* BFW */
    0,				/* EFW */

};

void lcd_attach( struct device *parent, struct device *self, void *aux )
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)self;

	pxa2x0_lcd_attach_sub(sc, aux, &sharp_LM8V31);


#if NWSDISPLAY > 0

	{
		struct wsemuldisplaydev_attach_args aa;

		/* make wsdisplay screen list */
		pxa2x0_lcd_setup_wsscreen( &lcd_bpp16_screen, &sharp_LM8V31, NULL );
		pxa2x0_lcd_setup_wsscreen( &lcd_bpp8_screen, &sharp_LM8V31, NULL );
		pxa2x0_lcd_setup_wsscreen( &lcd_bpp4_screen, &sharp_LM8V31, NULL );

		aa.console = 0;
		aa.scrdata = &lcd_screen_list;
		aa.accessops = &lcd_accessops;
		aa.accesscookie = sc;

		printf( "\n" );

		(void) config_found(self, &aa, wsemuldisplaydevprint);
	}
#else
	{
		struct pxa2x0_lcd_screen *screen;
		int error;

		error = pxa2x0_lcd_new_screen( sc, 8, &screen );
		if( error == 0 ){
			sc->active = screen;
			pxa2x0_lcd_start_dma( sc, screen );
		}

		printf( "\n" );
	}
#endif

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
		reg = bus_space_read_2( osc->sc_iot, osc->sc_obioreg_ioh,
		    LUBBOCK_MISCWR );
		if( *(int *)data == WSDISPLAYIO_VIDEO_ON )
			reg |= MISCWR_LCDDISP;
		else
			reg &= ~MISCWR_LCDDISP;
		bus_space_write_2( osc->sc_iot, osc->sc_obioreg_ioh,
			LUBBOCK_MISCWR, reg );
		break;			/* turn on/off LCD controller */
	}

	return pxa2x0_lcd_ioctl( v, vs, cmd, data, flag, l );
}

int
lcd_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct obio_softc *osc = 
	    (struct obio_softc *) device_parent((struct device *)v);

	pxa2x0_lcd_show_screen(v,cookie,waitok,cb,cbarg);
	
	/* Turn on LCD */
	bus_space_write_4( osc->sc_iot, osc->sc_obioreg_ioh, LUBBOCK_MISCWR,
	    MISCWR_LCDDISP |
	    bus_space_read_4( osc->sc_iot, osc->sc_obioreg_ioh, LUBBOCK_MISCWR ) );

	return (0);
}



#else  /* NWSDISPLAY==0 */

int
lcdopen( dev_t dev, int oflags, int devtype, struct lwp *l )
{
	return 0;
}

int
lcdclose( dev_t dev, int fflag, int devtype, struct lwp *l )
{
	return 0;
}

paddr_t
lcdmmap( dev_t dev, off_t offset, int size )
{
	struct pxa2x0_lcd_softc *sc = device_lookup(&lcd_cd, minor(dev));
	struct pxa2x0_lcd_screen *scr = sc->active;

	return bus_dmamem_mmap( &pxa2x0_bus_dma_tag, scr->segs, scr->nsegs,
	    offset, 0, BUS_DMA_WAITOK|BUS_DMA_COHERENT );
}

int
lcdioctl( dev_t dev, u_long cmd, void *data,
	    int fflag, struct lwp *l )
{
	return EOPNOTSUPP;
}

#endif /* NWSDISPLAY>0 */
