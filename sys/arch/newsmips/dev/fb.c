/*	$NetBSD: fb.c,v 1.29.6.1 2023/11/05 17:43:58 martin Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2023 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fb.c,v 1.29.6.1 2023/11/05 17:43:58 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/adrsmap.h>

#include <newsmips/dev/hbvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

struct fb_devconfig {
	uint8_t *dc_fbbase;		/* VRAM base address */
	struct rasops_info dc_ri;
	int dc_model;
#define NWB253	0
#define LCDM	1
	int dc_displayid;
#define NWP512	0
#define NWP518	1
#define NWE501	2
	int dc_size;
};

struct fb_softc {
	device_t sc_dev;
	struct fb_devconfig *sc_dc;
	int sc_nscreens;
};

static int fb_match(device_t, cfdata_t, void *);
static void fb_attach(device_t, device_t, void *);

static int fb_common_init(struct fb_devconfig *);
static int fb_is_console(void);

static int fb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t fb_mmap(void *, void *, off_t, int);
static int fb_alloc_screen(void *, const struct wsscreen_descr *, void **,
    int *, int *, long *);
static void fb_free_screen(void *, void *);
static int fb_show_screen(void *, void *, int, void (*)(void *, int, int),
    void *);

void fb_cnattach(void);

static int fb_set_state(struct fb_softc *, int);

static bool fb253_probe(void);
static bool fblcdm_probe(void);
static bool fb_probe_model(struct fb_devconfig *);
static void fb_init(struct fb_devconfig *);
static void fb253_init(void);
static void fblcdm_init(void);

CFATTACH_DECL_NEW(fb, sizeof(struct fb_softc),
    fb_match, fb_attach, NULL, NULL);

static struct fb_devconfig fb_console_dc;

static struct wsdisplay_accessops fb_accessops = {
	.ioctl        = fb_ioctl,
	.mmap         = fb_mmap,
	.alloc_screen = fb_alloc_screen,
	.free_screen  = fb_free_screen,
	.show_screen  = fb_show_screen,
	.load_font    = NULL
};

static struct wsscreen_descr fb_stdscreen = {
	.name = "std",
	.ncols = 0,
	.nrows = 0,
	.textops = NULL,
	.fontwidth = 0,
	.fontheight = 0,
	.capabilities = WSSCREEN_REVERSE
};

static const struct wsscreen_descr *fb_scrlist[] = {
	&fb_stdscreen
};

static struct wsscreen_list fb_screenlist = {
	.nscreens = __arraycount(fb_scrlist),
	.screens  = fb_scrlist
};

#define NWB253_VRAM   0x88000000
#define NWB253_CTLREG ((uint16_t *)0xb8ff0000)
#define NWB253_CRTREG ((uint16_t *)0xb8fe0000)

static const char *nwb253dispname[8] = {
	[NWP512] = "NWB-512",
	[NWP518] = "NWB-518",
	[NWE501] = "NWE-501"
}; /* XXX ? */

#define LCDM_VRAM	0x90200000
#define LCDM_PORT	((uint32_t *)0xb0000000)
#define LCDM_DIMMER	((uint32_t *)0xb0100000)
#define LCDM_DIMMER_ON	0xf0
#define LCDM_DIMMER_OFF	0xf1
#define LCDM_CTRL	((uint8_t *)0xbff50000)	/* XXX no macro in 4.4BSD */
#define LCDM_CRTC	((uint8_t *)0xbff60000)

static int
fb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hb_attach_args *ha = aux;

	if (strcmp(ha->ha_name, "fb") != 0)
		return 0;

	if (fb253_probe() && ha->ha_addr == NWB253_VRAM)
		return 1;
	if (fblcdm_probe() && ha->ha_addr == LCDM_VRAM)
		return 1;

	return 0;
}

static void
fb_attach(device_t parent, device_t self, void *aux)
{
	struct fb_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args waa;
	struct fb_devconfig *dc;
	struct rasops_info *ri;
	int console;
	const char *devname;

	sc->sc_dev = self;

	console = fb_is_console();

	if (console) {
		dc = &fb_console_dc;
		ri = &dc->dc_ri;
		ri->ri_flg &= ~RI_NO_AUTO;
		sc->sc_nscreens = 1;
	} else {
		dc = kmem_zalloc(sizeof(struct fb_devconfig), KM_SLEEP);

		fb_probe_model(dc);
		fb_common_init(dc);
		ri = &dc->dc_ri;

		/* clear screen */
		(*ri->ri_ops.eraserows)(ri, 0, ri->ri_rows, 0);

		fb_init(dc);
	}
	sc->sc_dc = dc;

	switch (dc->dc_model) {
	case NWB253:
		devname = nwb253dispname[dc->dc_displayid];
		break;
	case LCDM:
		devname = "LCD-MONO";
		break;
	default:
		/* should not be here */
		devname = "unknown";
		break;
	}
	aprint_normal(": %s, %d x %d, %dbpp\n", devname,
	    ri->ri_width, ri->ri_height, ri->ri_depth);

	waa.console = console;
	waa.scrdata = &fb_screenlist;
	waa.accessops = &fb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint, CFARGS_NONE);
}

static bool
fb253_probe(void)
{

	if (hb_badaddr(NWB253_CTLREG, 2) || hb_badaddr(NWB253_CRTREG, 2))
		return false;
	if ((*(volatile uint16_t *)NWB253_CTLREG & 7) != 4)
		return false;

	return true;
}

static bool
fblcdm_probe(void)
{

	if (hb_badaddr(LCDM_CTRL, 1))
		return false;
	if (*(volatile uint8_t *)LCDM_CTRL != 0xff)
		return false;

	return true;
}

static bool
fb_probe_model(struct fb_devconfig *dc)
{

	if (fb253_probe()) {
		volatile uint16_t *ctlreg = NWB253_CTLREG;

		dc->dc_model = NWB253;
		dc->dc_displayid = (*ctlreg >> 8) & 0xf;
		return true;
	}
	if (fblcdm_probe()) {
		dc->dc_model = LCDM;
		dc->dc_displayid = 0;	/* no variant */
		return true;
	}

	return false;
}

static int
fb_common_init(struct fb_devconfig *dc)
{
	struct rasops_info *ri = &dc->dc_ri;
	int width, height, stride, xoff, yoff, cols, rows;

	switch (dc->dc_model) {
	case NWB253:
		dc->dc_fbbase = (uint8_t *)NWB253_VRAM;

		switch (dc->dc_displayid) {
		case NWP512:
			width = 816;
			height = 1024;
			break;
		case NWP518:
		case NWE501:
		default:
			width = 1024;
			height = 768;
			break;
		}
		stride = 2048 / 8;
		dc->dc_size = stride * 2048;
		break;

	case LCDM:
		dc->dc_fbbase = (uint8_t *)LCDM_VRAM;
		width = 1120;
		height = 780;
		stride = width / 8;
		dc->dc_size = stride * height;
		break;

	default:
		panic("fb: no valid framebuffer");
	}

	/* initialize rasops */

	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_depth = 1;
	ri->ri_stride = stride;
	ri->ri_bits = dc->dc_fbbase;
	ri->ri_flg = RI_FULLCLEAR;
	if (dc == &fb_console_dc)
		ri->ri_flg |= RI_NO_AUTO;

	rasops_init(ri, 24, 80);
	rows = (height - 2) / ri->ri_font->fontheight;
	cols = ((width - 2) / ri->ri_font->fontwidth) & ~7;
	xoff = ((width - cols * ri->ri_font->fontwidth) / 2 / 8) & ~3;
	yoff = (height - rows * ri->ri_font->fontheight) / 2;
	rasops_reconfig(ri, rows, cols);

	ri->ri_xorigin = xoff;
	ri->ri_yorigin = yoff;
	ri->ri_bits = dc->dc_fbbase + xoff + ri->ri_stride * yoff;

	fb_stdscreen.nrows = ri->ri_rows;
	fb_stdscreen.ncols = ri->ri_cols;
	fb_stdscreen.textops = &ri->ri_ops;
	fb_stdscreen.capabilities = ri->ri_caps;

	return 0;
}

static int
fb_is_console(void)
{
	volatile u_int *dipsw = (void *)DIP_SWITCH;

	if (*dipsw & 7)					/* XXX right? */
		return 1;

	return 0;
}

static int
fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct fb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = dc->dc_ri.ri_height;
		wdf->width = dc->dc_ri.ri_width;
		wdf->depth = dc->dc_ri.ri_depth;
		wdf->cmsize = 0;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = dc->dc_ri.ri_stride;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		return fb_set_state(sc, *(int *)data);

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		break;
	}
	return EPASSTHROUGH;
}

static int
fb_set_state(struct fb_softc *sc, int state)
{
	struct fb_devconfig *dc = sc->sc_dc;
	volatile uint16_t *ctlreg;
	volatile uint32_t *dimmerreg;

	if (state != WSDISPLAYIO_VIDEO_OFF && state != WSDISPLAYIO_VIDEO_ON)
		return EINVAL;

	switch (dc->dc_model) {
	case NWB253:
		if (state == WSDISPLAYIO_VIDEO_OFF) {
			ctlreg = NWB253_CTLREG;
			*ctlreg = 0;			/* stop crtc */
		} else {
			fb253_init();
		}
		break;
	case LCDM:
		dimmerreg = LCDM_DIMMER;
		if (state == WSDISPLAYIO_VIDEO_OFF) {
			*dimmerreg = LCDM_DIMMER_OFF;
		} else {
			*dimmerreg = LCDM_DIMMER_ON;
		}
		break;
	default:
		/* should not be here */
		break;
	}
	return 0;
}

static paddr_t
fb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct fb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;

	if (offset >= dc->dc_size || offset < 0)
		return -1;

	return mips_btop((int)dc->dc_fbbase + offset);
}

static int
fb_alloc_screen(void *v, const struct wsscreen_descr *scrdesc, void **cookiep,
    int *ccolp, int *crowp, long *attrp)
{
	struct fb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_dc->dc_ri;
	long defattr;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*ccolp = *crowp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->sc_nscreens++;

	return 0;
}

static void
fb_free_screen(void *v, void *cookie)
{
	struct fb_softc *sc = v;

	if (sc->sc_dc == &fb_console_dc)
		panic("%s: console", __func__);

	sc->sc_nscreens--;
}

static int
fb_show_screen(void *v, void *cookie, int waitok, void (*cb)(void *, int, int),
    void *cbarg)
{

	return 0;
}

void
fb_cnattach(void)
{
	struct fb_devconfig *dc = &fb_console_dc;
	struct rasops_info *ri = &dc->dc_ri;
	long defattr;

	if (!fb_is_console())
		return;

	if (!fb_probe_model(dc))
		return;

	fb_common_init(dc);
	fb_init(dc);

	/*
	 * Wait CRTC output or LCD backlight become settled
	 * before starting to print kernel greeting messages.
	 */
	delay(500 * 1000);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&fb_stdscreen, ri, 0, ri->ri_rows - 1, defattr);
}

static void
fb_init(struct fb_devconfig *dc)
{

	switch (dc->dc_model) {
	case NWB253:
		fb253_init();
		break;
	case LCDM:
		fblcdm_init();
		break;
	default:
		/* should not be here */
		break;
	}
}

static const uint8_t
nwp512_data1[] = {
	0x00, 0x44,
	0x01, 0x33,
	0x02, 0x3c,
	0x03, 0x38,
	0x04, 0x84,
	0x05, 0x03,
	0x06, 0x80,
	0x07, 0x80,
	0x08, 0x10,
	0x09, 0x07,
	0x0a, 0x20,
	0x0c, 0x00,
	0x0d, 0x00,
	0x1b, 0x03
};

static const uint8_t
nwp512_data2[] = {
	0x1e, 0x08,
	0x20, 0x08,
	0x21, 0x0d
};

static const uint8_t
nwp518_data1[] = {
	0x00, 0x52,
	0x01, 0x40,
	0x02, 0x4a,
	0x03, 0x49,
	0x04, 0x63,
	0x05, 0x02,
	0x06, 0x60,
	0x07, 0x60,
	0x08, 0x10,
	0x09, 0x07,
	0x0a, 0x20,
	0x0c, 0x00,
	0x0d, 0x00,
	0x1b, 0x04
};

static const uint8_t
nwp518_data2[] = {
	0x1e, 0x08,
	0x20, 0x00,
	0x21, 0x00
};

static const uint8_t
nwe501_data1[] = {
	0x00, 0x4b,
	0x01, 0x40,
	0x02, 0x4a,
	0x03, 0x43,
	0x04, 0x64,
	0x05, 0x02,
	0x06, 0x60,
	0x07, 0x60,
	0x08, 0x10,
	0x09, 0x07,
	0x0a, 0x20,
	0x0c, 0x00,
	0x0d, 0x00,
	0x1b, 0x04
};

static const uint8_t
nwe501_data2[] = {
	0x1e, 0x08,
	0x20, 0x00,
	0x21, 0x00
};

static const uint8_t
*crtc_data[3][2] = {
	{ nwp512_data1, nwp512_data2 },
	{ nwp518_data1, nwp518_data2 },
	{ nwe501_data1, nwe501_data2 }
};

static void
fb253_init(void)
{
	volatile uint16_t *ctlreg = NWB253_CTLREG;
	volatile uint16_t *crtreg = NWB253_CRTREG;
	int id = (*ctlreg >> 8) & 0xf;
	const uint8_t *p;
	int i;

	*ctlreg = 0;			/* stop crtc */
	delay(10);

	/* initialize crtc without R3{0,1,2} */
	p = crtc_data[id][0];
	for (i = 0; i < 28; i++) {
		*crtreg++ = *p++;
		delay(10);
	}

	*ctlreg = 0x02;			/* start crtc */
	delay(10);

	/* set crtc control reg */
	p = crtc_data[id][1];
	for (i = 0; i < 6; i++) {
		*crtreg++ = *p++;
		delay(10);
	}
}

static const uint8_t lcdcrtc_data[] = {
	 0, 47,
	 1, 35,
	 9,  0,
	10,  0,
	11,  0,
	12,  0,
	13,  0,
	14,  0,
	15,  0,
	18, 35,
	19, 0x01,
	20, 0x85,
	21,  0,
	22, 0x10
};

static void
fblcdm_init(void)
{
	volatile uint8_t *crtcreg = LCDM_CRTC;
	volatile uint32_t *portreg = LCDM_PORT;
	volatile uint32_t *dimmerreg = LCDM_DIMMER;
	int i;

	/* initialize crtc */
	for (i = 0; i < 28; i++) {
		*crtcreg++ = lcdcrtc_data[i];
		delay(10);
	}

	delay(1000);
	*portreg = 1;
	*dimmerreg = LCDM_DIMMER_ON;
}

#if 0
static struct wsdisplay_font newsrom8x16;
static struct wsdisplay_font newsrom12x24;
static uint8_t fontarea16[96][32];
static uint8_t fontarea24[96][96];

void
initfont(struct rasops_info *ri)
{
	int c, x;

	for (c = 0; c < 96; c++) {
		x = ((c & 0x1f) | ((c & 0xe0) << 2)) << 7;
		memcpy(fontarea16 + c, (uint8_t *)0xb8e00000 + x + 96, 32);
		memcpy(fontarea24 + c, (uint8_t *)0xb8e00000 + x, 96);
	}

	newsrom8x16.name = "rom8x16";
	newsrom8x16.firstchar = 32;
	newsrom8x16.numchars = 96;
	newsrom8x16.encoding = WSDISPLAY_FONTENC_ISO;
	newsrom8x16.fontwidth = 8;
	newsrom8x16.fontheight = 16;
	newsrom8x16.stride = 2;
	newsrom8x16.bitorder = WSDISPLAY_FONTORDER_L2R;
	newsrom8x16.byteorder = WSDISPLAY_FONTORDER_L2R;
	newsrom8x16.data = fontarea16;

	newsrom12x24.name = "rom12x24";
	newsrom12x24.firstchar = 32;
	newsrom12x24.numchars = 96;
	newsrom12x24.encoding = WSDISPLAY_FONTENC_ISO;
	newsrom12x24.fontwidth = 12;
	newsrom12x24.fontheight = 24;
	newsrom12x24.stride = 4;
	newsrom12x24.bitorder = WSDISPLAY_FONTORDER_L2R;
	newsrom12x24.byteorder = WSDISPLAY_FONTORDER_L2R;
	newsrom12x24.data = fontarea24;

	ri->ri_font = &newsrom8x16;
	ri->ri_font = &newsrom12x24;
	ri->ri_wsfcookie = -1;		/* not using wsfont */
}
#endif
