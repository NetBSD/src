/*	$NetBSD: smdk2410_lcd.c,v 1.10 2014/07/25 08:10:33 dholland Exp $ */

/*
 * Copyright (c) 2004  Genetec Corporation.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smdk2410_lcd.c,v 1.10 2014/07/25 08:10:33 dholland Exp $");

/*
 * LCD driver for Samsung SMDK2410.
 *
 * Controlling LCD is almost completely done through S3C2410's
 * integrated LCD controller.  Codes for it is arm/s3c2xx0/s3c24x0_lcd.c.
 *
 * Codes in this file provide the platform's LCD panel information.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/event.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#include <dev/cons.h> 
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h> 
#include <dev/wscons/wscons_callbacks.h>

#include <sys/bus.h>
#include <arm/s3c2xx0/s3c24x0var.h>
#include <arm/s3c2xx0/s3c24x0reg.h>
#include <arm/s3c2xx0/s3c2410reg.h>
#include <arm/s3c2xx0/s3c24x0_lcd.h>

#include "locators.h"
#include "wsdisplay.h"

int	lcd_match(device_t, cfdata_t, void *);
void	lcd_attach(device_t, device_t, void *);

#ifdef LCD_DEBUG
void draw_test_pattern(struct s3c24x0_lcd_softc *,
	    struct s3c24x0_lcd_screen *scr);
#endif

#if NWSDISPLAY > 0

/*
 * Screen geometries.
 *
 * S3C24x0's LCD controller can have virtual screens that are bigger
 * than actual LCD panel. (XXX: wscons can't manage such screens for now)
 */
struct s3c24x0_wsscreen_descr lcd_bpp16_30x32 = {
	{
		"30x32bpp16", 30, 32,
		&s3c24x0_lcd_emulops,
		8, 10,
		WSSCREEN_WSCOLORS,
	},
	16				/* bits per pixel */
},  lcd_bpp16_30x20 = {
	{
		"30x20bpp16", 30, 20,
		&s3c24x0_lcd_emulops,
		8, 16,
		WSSCREEN_WSCOLORS,
	},
	16
}, lcd_bpp8_30x32 = {
	{
		"30x32bpp8", 30, 32,
		&s3c24x0_lcd_emulops,
		8, 10,
		WSSCREEN_WSCOLORS,
	},
	8
}, lcd_bpp8_40x25 = {			/* with big virtual screen */
	{
		"40x25bpp8", 40, 25,
		&s3c24x0_lcd_emulops,
		8, 16,
		WSSCREEN_WSCOLORS,
	},
	8
}, lcd_bpp4_30x32 = {
	{
		"30x32bpp4", 30, 32,
		&s3c24x0_lcd_emulops,
		8, 10,
		0,			/* 4bpp mode is grayscale */
	},
	4
};


static const struct wsscreen_descr *lcd_scr_descr[] = {
#if 0
	/* bpp4 needs a patch to rasops4 */
	&lcd_bpp4_30x32.c,
#endif
	&lcd_bpp8_30x32.c,
	&lcd_bpp8_40x25.c,
	&lcd_bpp16_30x32.c,
	&lcd_bpp16_30x20.c,
};

#define	N_SCR_DESCR	(sizeof lcd_scr_descr / sizeof lcd_scr_descr[0])

const struct wsscreen_list lcd_screen_list = {
	N_SCR_DESCR,
	lcd_scr_descr
};

const struct wsdisplay_accessops lcd_accessops = {
	s3c24x0_lcd_ioctl,
	s3c24x0_lcd_mmap,
	s3c24x0_lcd_alloc_screen,
	s3c24x0_lcd_free_screen,
	s3c24x0_lcd_show_screen,
	NULL, /* load_font */
};

#else  /* NWSDISPLAY */
/*
 * Interface to LCD framebuffer without wscons
 */
extern struct cfdriver lcd_cd;

dev_type_open(lcdopen);
dev_type_close(lcdclose);
dev_type_ioctl(lcdioctl);
dev_type_mmap(lcdmmap);
const struct cdevsw lcd_cdevsw = {
	.d_open = lcdopen,
	.d_close = lcdclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = lcdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = lcdmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

#endif /* NWSDISPLAY */

CFATTACH_DECL_NEW(lcd_ssio, sizeof (struct s3c24x0_lcd_softc),  lcd_match,
    lcd_attach, NULL, NULL);

int
lcd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct s3c2xx0_attach_args *sa = aux;

	if (sa->sa_addr == SSIOCF_ADDR_DEFAULT)
		sa->sa_addr = S3C2410_LCDC_BASE;

	return 1;
}

static const struct s3c24x0_lcd_panel_info samsung_LTS350Q1 =
{
    240,			/* Width */
    320,			/* Height */
    10*1000*1000,		/* pixel clock = 10MHz */

#define	_(field, val)	(((val)-1) << (field##_SHIFT))

    LCDCON1_PNRMODE_TFT,

    /* LCDCON2: vertical timings */
    _(LCDCON2_VBPD, 2) |
    _(LCDCON2_VFPD, 3) |
    _(LCDCON2_LINEVAL, 320) |
    _(LCDCON2_VPSW, 2),

    /* LCDCON3: horizontal timings */
    _(LCDCON3_HBPD, 7) | 
    _(LCDCON3_HFPD, 3),

    /* LCDCON4: horizontaol pulse width */
    _(LCDCON4_HPSW, 4),

    /* LCDCON5: signal polarities */
    LCDCON5_FRM565 | LCDCON5_INVVLINE | LCDCON5_INVVFRAME,

    /* LPCSEL register */
    LPCSEL_LPC_EN | LPCSEL_RES_SEL | LPCSEL_MODE_SEL,
#undef _
};

void
lcd_attach(device_t parent, device_t self, void *aux)
{
	struct s3c24x0_lcd_softc *sc = device_private(self);
	bus_space_tag_t iot =  s3c2xx0_softc->sc_iot;
	bus_space_handle_t gpio_ioh = s3c2xx0_softc->sc_gpio_ioh;
#if NWSDISPLAY > 0
	struct wsemuldisplaydev_attach_args aa;
#else
	struct s3c24x0_lcd_screen *screen;
#endif


	sc->sc_dev = self;
	aprint_normal( "\n" );

	/* setup GPIO ports for LCD */
	/* XXX: some LCD panels may not need all VD signals */
	gpio_ioh = s3c2xx0_softc->sc_gpio_ioh;
	bus_space_write_4(iot, gpio_ioh, GPIO_PCUP, ~0);
	bus_space_write_4(iot, gpio_ioh, GPIO_PCCON, 0xaaaaaaaa);
	bus_space_write_4(iot, gpio_ioh, GPIO_PDUP, ~0);
	bus_space_write_4(iot, gpio_ioh, GPIO_PDCON, 0xaaaaaaaa);

	s3c24x0_lcd_attach_sub(sc, aux, &samsung_LTS350Q1);

#if NWSDISPLAY > 0

	aa.console = 0;
	aa.scrdata = &lcd_screen_list;
	aa.accessops = &lcd_accessops;
	aa.accesscookie = sc;


	(void) config_found(self, &aa, wsemuldisplaydevprint);
#else

	screen = s3c24x0_lcd_new_screen(sc, 240, 320, 16);

	if( screen ){
		sc->active = screen;
		s3c24x0_lcd_start_dma(sc, screen);
		s3c24x0_lcd_power(sc, 1);
	}
#endif /* NWSDISPLAY */

}

#if NWSDISPLAY == 0

int
lcdopen( dev_t dev, int oflags, int devtype, struct proc *p )
{
	return 0;
}

int
lcdclose( dev_t dev, int fflag, int devtype, struct proc *p )
{
	return 0;
}

paddr_t
lcdmmap( dev_t dev, off_t offset, int size )
{
	struct s3c24x0_lcd_softc *sc =
		device_lookup_private(&lcd_cd, minor(dev));
	struct s3c24x0_lcd_screen *scr = sc->active;

	return bus_dmamem_mmap(sc->dma_tag, scr->segs, scr->nsegs,
	    offset, 0, BUS_DMA_WAITOK|BUS_DMA_COHERENT);
}

int
lcdioctl( dev_t dev, u_long cmd, void *data,
	    int fflag, struct lwp *l )
{
	return EOPNOTSUPP;
}

#endif /* NWSDISPLAY>0 */

#ifdef LCD_DEBUG
static void
draw_test_pattern_16(struct s3c24x0_lcd_softc *sc,
    struct s3c24x0_lcd_screen *scr)
{
	int x, y;
	uint16_t color, *line;
	char *buf = (char *)(scr->buf_va);

#define	rgb(r,g,b)	(((r)<<11) | ((g)<<5) | (b))

	for (y=0; y < sc->panel_info->panel_height; ++y) {
		line = (uint16_t *)(buf + scr->stride * y);

		for (x=0; x < sc->panel_info->panel_width; ++x) {
			switch (((x/30) + (y/10)) % 8) {
			default:
			case 0: color = rgb(0x00, 0x00, 0x00); break;
			case 1: color = rgb(0x00, 0x00, 0x1f); break;
			case 2: color = rgb(0x00, 0x3f, 0x00); break;
			case 3: color = rgb(0x00, 0x3f, 0x1f); break;
			case 4: color = rgb(0x1f, 0x00, 0x00); break;
			case 5: color = rgb(0x1f, 0x00, 0x1f); break;
			case 6: color = rgb(0x1f, 0x3f, 0x00); break;
			case 7: color = rgb(0x1f, 0x3f, 0x1f); break;
			}

			line[x] = color;
		}
	}

	for (x=0; x < MIN(sc->panel_info->panel_height,
		 sc->panel_info->panel_width); ++x) {

		line = (uint16_t *)(buf + scr->stride * x);
		line[x] = rgb(0x1f, 0x3f, 0x1f);
	}
}

static void
draw_test_pattern_8(struct s3c24x0_lcd_softc *sc,
    struct s3c24x0_lcd_screen *scr)
{
	int x, y;
	uint8_t *line;
	char *buf = (char *)(scr->buf_va);


	for (y=0; y < sc->panel_info->panel_height; ++y) {
		line = (uint8_t *)(buf + scr->stride * y);

		for (x=0; x < sc->panel_info->panel_width; ++x) {
			line[x] = (((x/15) + (y/20)) % 16);
		}
	}

	for (x=0; x < MIN(sc->panel_info->panel_height,
		 sc->panel_info->panel_width); ++x) {

		line = (uint8_t *)(buf + scr->stride * x);
		line[x] = 7;
	}
}


void
draw_test_pattern(struct s3c24x0_lcd_softc *sc, struct s3c24x0_lcd_screen *scr)
{
	switch (scr->depth) {
	case 16:
		draw_test_pattern_16(sc, scr);
		break;
	case 8:
		draw_test_pattern_8(sc, scr);
		break;
	}
}
	

#endif /* LCD_DEBUG */

