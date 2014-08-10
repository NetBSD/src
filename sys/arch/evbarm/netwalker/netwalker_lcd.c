/*	$NetBSD: netwalker_lcd.c,v 1.2.2.1 2014/08/10 06:53:56 tls Exp $	*/

/*-
 * Copyright (c) 2011, 2012 Genetec corp. All rights reserved.
 * Written by Hashimoto Kenichi for Genetec corp.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netwalker_lcd.c,v 1.2.2.1 2014/08/10 06:53:56 tls Exp $");

#include "opt_imx51_ipuv3.h"
#include "opt_netwalker_lcd.h"

#include "wsdisplay.h"
#include "ioconf.h"
#include "netwalker_backlight.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/pmf.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <sys/bus.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51_ipuv3var.h>
#include <arm/imx/imx51_ipuv3reg.h>
#include <arm/imx/imxgpiovar.h>

#include <evbarm/netwalker/netwalker_backlightvar.h>

int lcd_match(device_t, cfdata_t, void *);
void lcd_attach(device_t, device_t, void *);

void netwalker_cnattach(void);

#if NWSDISPLAY > 0
static int netwalker_lcd_ioctl(void *, void *, u_long, void *, int,
    struct lwp *);
static int netwalker_lcd_show_screen(void *, void *, int,
    void (*)(void *, int, int), void *);

bool netwalker_lcd_console = 0;

/*
 * wsdisplay glue
 */
static struct imx51_wsscreen_descr netwalker_lcd_stdscreen = {
	.c = {
		.name	      = "std",
		.ncols	      = 0,
		.nrows	      = 0,
		.textops      = NULL,
		.fontwidth    = 8,
		.fontheight   = 16,
		.capabilities = WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		.modecookie   = NULL
	},
	.depth = 16,		/* bits per pixel */
	.flags = RI_CENTER | RI_FULLCLEAR
};

static const struct wsscreen_descr *netwalker_lcd_scr_descr[] = {
	&netwalker_lcd_stdscreen.c,
};

const struct wsscreen_list netwalker_lcd_screen_list = {
	sizeof netwalker_lcd_scr_descr / sizeof netwalker_lcd_scr_descr[0],
	netwalker_lcd_scr_descr
};

struct wsdisplay_accessops netwalker_lcd_accessops = {
	.ioctl	      = netwalker_lcd_ioctl,
	.mmap	      = imx51_ipuv3_mmap,
	.alloc_screen = imx51_ipuv3_alloc_screen,
	.free_screen  = imx51_ipuv3_free_screen,
	.show_screen  = netwalker_lcd_show_screen,
	.load_font    = NULL,
	.pollc	      = NULL,
	.scroll	      = NULL
};
#else
#ifdef LCD_DEBUG
static void draw_test_pattern(struct imx51_ipuv3_softc *,
    struct imx51_ipuv3_screen *);
#endif

/*
 * Interface to LCD framebuffer without wscons
 */
extern struct cfdriver ipu_cd;

dev_type_open(lcdopen);
dev_type_close(lcdclose);
dev_type_ioctl(lcdioctl);
dev_type_mmap(lcdmmap);
const struct cdevsw ipu_cdevsw = {
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

#endif

CFATTACH_DECL_NEW(lcd_netwalker, sizeof (struct imx51_ipuv3_softc),
    lcd_match, lcd_attach, NULL, NULL);

int
lcd_match( device_t parent, cfdata_t cf, void *aux )
{
	return 1;
}

/* Sharp's LCD */
static const struct lcd_panel_geometry sharp_panel =
{
	.panel_width = 1024,	/* Width */
	.panel_height = 600,	/* Height */

	.pixel_clk = 30076000,

	.hsync_width = 8,
	.left  = 20,
	.right = 20,

	.vsync_width = 4,
	.upper = 2,
	.lower = 2,

	.panel_info = 0,
};

void lcd_attach( device_t parent, device_t self, void *aux )
{
	struct imx51_ipuv3_softc *sc = device_private(self);
	struct axi_attach_args *axia = aux;
	bus_space_tag_t iot = axia->aa_iot;

	sc->dev = self;

#if defined(IMXIPUCONSOLE)
	netwalker_lcd_console = 1;
#endif
#if (NWSDISPLAY > 0)
	netwalker_cnattach();
#endif

	/* XXX move this to imx51_ipuv3.c */
	{
		bus_space_handle_t mipi_ioh;
		uint32_t reg;

		if (bus_space_map(iot, 0x83fdc000, 0x1000, 0, &mipi_ioh))
			aprint_error_dev(self, "can't map MIPI HSC");
		else {
			bus_space_write_4(iot, mipi_ioh, 0x000, 0xf00);

			reg = bus_space_read_4(iot, mipi_ioh, 0x800);
			bus_space_write_4(iot, mipi_ioh, 0x800, reg | 0x0ff);

			reg = bus_space_read_4(iot, mipi_ioh, 0x800);
			bus_space_write_4(iot, mipi_ioh, 0x800, reg | 0x10000);
		}
	}

	/* LCD power on */
	gpio_set_direction(GPIO_NO(4, 9), GPIO_DIR_OUT);
	gpio_set_direction(GPIO_NO(4, 10), GPIO_DIR_OUT);
	gpio_set_direction(GPIO_NO(3, 3), GPIO_DIR_OUT);

	gpio_data_write(GPIO_NO(3, 3), 1);
	gpio_data_write(GPIO_NO(4, 9), 1);
	delay(180 * 1000);
	gpio_data_write(GPIO_NO(4, 10), 1);

	gpio_set_direction(GPIO_NO(2, 13), GPIO_DIR_OUT);
	gpio_data_write(GPIO_NO(2, 13), 1);

	imx51_ipuv3_attach_sub(sc, aux, &sharp_panel);

#if NWSDISPLAY == 0
	struct imx51_ipuv3_screen *screen;
	int error;

	error = imx51_ipuv3_new_screen(sc, 16, &screen);
#ifdef LCD_DEBUG
	draw_test_pattern(sc, screen);
#endif
	if (error == 0) {
		sc->active = screen;
		imx51_ipuv3_start_dma(sc, screen);
	}
#else
	struct wsemuldisplaydev_attach_args aa;

#if defined(IMXIPUCONSOLE)
	aa.console = true;
#else
	aa.console = false;
#endif
	aa.scrdata = &netwalker_lcd_screen_list;
	aa.accessops = &netwalker_lcd_accessops;
	aa.accesscookie = &sc->vd;

	(void) config_found(sc->dev, &aa, wsemuldisplaydevprint);
#endif
}

#if NWSDISPLAY > 0
void
netwalker_cnattach(void)
{
	imx51_ipuv3_cnattach(netwalker_lcd_console, &netwalker_lcd_stdscreen,
	    &netwalker_lcd_accessops, &sharp_panel);
	return;
}

/*
 * wsdisplay accessops overrides
 */
static int
netwalker_lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	int res = EINVAL;

	switch (cmd) {
#if NNETWALKER_BACKLIGHT > 0
	case WSDISPLAYIO_GETPARAM:
	case WSDISPLAYIO_SETPARAM:
		res = netwalker_lcd_param_ioctl(cmd, (struct wsdisplay_param *)data);
		break;
#endif
	}

	if (res == EINVAL)
		res = imx51_ipuv3_ioctl(v, vs, cmd, data, flag, l);
	return res;
}

static int
netwalker_lcd_show_screen(void *v, void *cookie, int waitok,
    void (*cb_func)(void *, int, int), void *cb_arg)
{
	int error;

	error = imx51_ipuv3_show_screen(v, cookie, waitok, cb_func, cb_arg);
	if (error)
		return (error);

	return 0;
}
#else

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
	struct imx51_ipuv3_softc *sc =
	    device_lookup_private(&ipu_cd, minor(dev));
	struct imx51_ipuv3_screen *scr = sc->active;

	return bus_dmamem_mmap(sc->dma_tag, scr->segs, scr->nsegs,
	    offset, 0, BUS_DMA_WAITOK|BUS_DMA_COHERENT);
}

int
lcdioctl(dev_t dev, u_long cmd, void *data,
    int fflag, struct lwp *l)
{
	return EOPNOTSUPP;
}

#ifdef LCD_DEBUG
static void
draw_test_pattern(struct imx51_ipuv3_softc *sc,
    struct imx51_ipuv3_screen *scr)
{
	int x, y;
	uint16_t color, *line;
	char *buf = (char *)(scr->buf_va);

	printf("%s: buf_va %p, size 0x%x\n", __func__, buf,
	       (uint)scr->buf_size);
	printf("%s: panel %d x %d\n", __func__,
	    sc->geometry->panel_width,
	    sc->geometry->panel_height);
#define	rgb(r,g,b)	(((r)<<11) | ((g)<<5) | (b))

	for (y=0; y < sc->geometry->panel_height; ++y) {
		line = (uint16_t *)(buf + scr->stride * y);

		for (x=0; x < sc->geometry->panel_width; ++x) {
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

	for (x=0; x < MIN(sc->geometry->panel_height,
		sc->geometry->panel_width); ++x) {
		line = (uint16_t *)(buf + scr->stride * x);
		line[x] = rgb(0x1f, 0x3f, 0x1f);
	}
}
#endif

#endif /* NWSDISPLAY > 0 */


