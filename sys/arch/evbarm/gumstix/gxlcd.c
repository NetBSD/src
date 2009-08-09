/*	$NetBSD: gxlcd.c,v 1.1 2009/08/09 07:10:13 kiyohara Exp $	*/

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
 * LCD driver for Gumstix consoleLCD-vx and compatible.
 * (based on the Intel Lubbock driver).
 *
 * Controlling LCD is almost completely done through PXA2X0's
 * integrated LCD controller.  Codes for it is arm/xscale/pxa2x0_lcd.c.
 *
 * Codes in this file provide platform specific things including:
 *   LCD on/off switch and backlight
 *   LCD panel geometry
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gxlcd.c,v 1.1 2009/08/09 07:10:13 kiyohara Exp $");

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
#include <arm/xscale/pxa2x0_gpio.h>


#ifndef CURRENT_DISPLAY
#define CURRENT_DISPLAY samsung_lte430wq_f0c
#endif


static int gxlcd_match(device_t, cfdata_t, void *);
static void gxlcd_attach(device_t, device_t, void *);

void gxlcd_cnattach(void);

static int gxlcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static int gxlcd_show_screen(void *, void *, int,
			     void (*)(void *, int, int), void *);


/*
 * wsdisplay glue
 */
static struct pxa2x0_wsscreen_descr gxlcd_std_screen = {
	.c = {
		.name = "std",
		.textops = &pxa2x0_lcd_emulops,
		.fontwidth = 8,
		.fontheight = 16,
		.capabilities = WSSCREEN_WSCOLORS,
	},
	.depth = 16,			/* bits per pixel */
};

static const struct wsscreen_descr *gxlcd_scr_descr[] = {
	&gxlcd_std_screen.c
};

static const struct wsscreen_list gxlcd_screen_list = {
	.nscreens = __arraycount(gxlcd_scr_descr),
	.screens = gxlcd_scr_descr,
};

struct wsdisplay_accessops gxlcd_accessops = {
	gxlcd_ioctl,
	pxa2x0_lcd_mmap,
	pxa2x0_lcd_alloc_screen,
	pxa2x0_lcd_free_screen,
	gxlcd_show_screen,
	NULL,
	NULL,
	NULL,
};

const struct lcd_panel_geometry samsung_lte430wq_f0c =
{
	480,			/* Width */
	272,			/* Height */
	0,			/* No extra lines */

	LCDPANEL_ACTIVE | LCDPANEL_PCP,
	1,			/* clock divider */
	0,			/* AC bias pin freq */

	41,			/* horizontal sync pulse width */
	4,			/* BLW */
	8,			/* ELW */

	10,			/* vertical sync pulse width */
	2,			/* BFW */
	4,			/* EFW */
};

static int gxlcd_console;


CFATTACH_DECL_NEW(gxlcd, sizeof(struct pxa2x0_lcd_softc),
    gxlcd_match, gxlcd_attach, NULL, NULL);


static int
gxlcd_match(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0)
		return 0;

	pxa->pxa_size = PXA2X0_LCDC_SIZE;
	return 1;
}

static void
gxlcd_attach(device_t parent, device_t self, void *aux)
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)self;
	struct wsemuldisplaydev_attach_args aa;

	pxa2x0_lcd_attach_sub(sc, aux, &CURRENT_DISPLAY);

	aa.console = gxlcd_console;
	aa.scrdata = &gxlcd_screen_list;
	aa.accessops = &gxlcd_accessops;
	aa.accesscookie = sc;

	pxa2x0_lcd_setup_wsscreen(&gxlcd_std_screen, &CURRENT_DISPLAY, NULL);

	(void) config_found(self, &aa, wsemuldisplaydevprint);
}

void
gxlcd_cnattach(void)
{

	pxa2x0_lcd_cnattach(&gxlcd_std_screen, &CURRENT_DISPLAY);

	gxlcd_console = 1;
}


/*
 * wsdisplay accessops overrides
 */
static int
gxlcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	int res = EINVAL;

	switch (cmd) {
	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON)
			pxa2x0_gpio_set_function(17, GPIO_IN);
		else
			pxa2x0_gpio_set_function(17, GPIO_OUT | GPIO_CLR);
		break;
	}

	if (res == EINVAL)
		res = pxa2x0_lcd_ioctl(v, vs, cmd, data, flag, l);
	return res;
}

static int
gxlcd_show_screen(void *v, void *cookie, int waitok,
		  void (*cb_func)(void *, int, int), void *cb_arg)
{
	int error;

	error = pxa2x0_lcd_show_screen(v, cookie, waitok, cb_func, cb_arg);
	if (error)
		return (error);

	/* Turn on LCD */
	pxa2x0_gpio_set_function(17, GPIO_IN);

	return 0;
}
