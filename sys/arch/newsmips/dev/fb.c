/*	$NetBSD: fb.c,v 1.16 2003/05/09 13:36:39 tsutsui Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/adrsmap.h>

#include <newsmips/dev/hbvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

struct fb_devconfig {
	u_char *dc_fbbase;		/* VRAM base address */
	struct rasops_info dc_ri;
};

struct fb_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;
	int sc_nscreens;
};

int fb_match(struct device *, struct cfdata *, void *);
void fb_attach(struct device *, struct device *, void *);

int fb_common_init(struct fb_devconfig *);
int fb_is_console(void);

int fb_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t fb_mmap(void *, off_t, int);
int fb_alloc_screen(void *, const struct wsscreen_descr *, void **, int *,
		    int *, long *);
void fb_free_screen(void *, void *);
int fb_show_screen(void *, void *, int, void (*)(void *, int, int), void *);

void fb_cnattach(void);

static void fb253_init(void);

CFATTACH_DECL(fb, sizeof(struct fb_softc),
    fb_match, fb_attach, NULL, NULL);

struct fb_devconfig fb_console_dc;

struct wsdisplay_accessops fb_accessops = {
	fb_ioctl,
	fb_mmap,
	fb_alloc_screen,
	fb_free_screen,
	fb_show_screen,
	NULL	/* load_font */
};

struct wsscreen_descr fb_stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *fb_scrlist[] = {
	&fb_stdscreen
};

struct wsscreen_list fb_screenlist = {
	sizeof(fb_scrlist) / sizeof(fb_scrlist[0]), fb_scrlist
};

#define NWB253_VRAM   ((u_char *) 0x88000000)
#define NWB253_CTLREG ((u_short *)0xb8ff0000)
#define NWB253_CRTREG ((u_short *)0xb8fe0000)

static char *devname[8] = { "NWB-512", "NWB-518", "NWE-501" };	/* XXX ? */

int
fb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct hb_attach_args *ha = aux;

	if (strcmp(ha->ha_name, "fb") != 0)
		return 0;

	if (hb_badaddr(NWB253_CTLREG, 2) || hb_badaddr(NWB253_CRTREG, 2))
		return 0;
	if ((*(volatile u_short *)NWB253_CTLREG & 7) != 4)
		return 0;

	return 1;
}

void
fb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fb_softc *sc = (void *)self;
	struct wsemuldisplaydev_attach_args waa;
	struct fb_devconfig *dc;
	struct rasops_info *ri;
	int console;
	volatile u_short *ctlreg = NWB253_CTLREG;
	int id;

	console = fb_is_console();

	if (console) {
		dc = &fb_console_dc;
		ri = &dc->dc_ri;
		sc->sc_nscreens = 1;
	} else {
		dc = malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(dc, sizeof(struct fb_devconfig));

		dc->dc_fbbase = NWB253_VRAM;
		fb_common_init(dc);
		ri = &dc->dc_ri;

		/* clear screen */
		(*ri->ri_ops.eraserows)(ri, 0, ri->ri_rows, 0);

		fb253_init();
	}
	sc->sc_dc = dc;

	id = (*ctlreg >> 8) & 0xf;
	printf(": %s, %d x %d, %dbpp\n", devname[id],
	    ri->ri_width, ri->ri_height, ri->ri_depth);

	waa.console = console;
	waa.scrdata = &fb_screenlist;
	waa.accessops = &fb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

int
fb_common_init(dc)
	struct fb_devconfig *dc;
{
	struct rasops_info *ri = &dc->dc_ri;
	volatile u_short *ctlreg = NWB253_CTLREG;
	int id;
	int width, height, xoff, yoff, cols, rows;

	id = (*ctlreg >> 8) & 0xf;

	/* initialize rasops */
	switch (id) {
	case 0:
		width = 816;
		height = 1024;
		break;
	case 1:
	case 2:
		width = 1024;
		height = 768;
		break;
	}

	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_depth = 1;
	ri->ri_stride = 2048 / 8;
	ri->ri_bits = dc->dc_fbbase;
	ri->ri_flg = RI_FULLCLEAR;

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

int
fb_is_console()
{
	volatile u_int *dipsw = (void *)DIP_SWITCH;

	if (*dipsw & 7)					/* XXX right? */
		return 1;

	return 0;
}

int
fb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
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
		wdf->cmsize = 2;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_OFF) {
			volatile u_short *ctlreg = NWB253_CTLREG;
			*ctlreg = 0;			/* stop crtc */
		} else
			fb253_init();
		return 0;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		break;
	}
	return EPASSTHROUGH;
}

paddr_t
fb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct fb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;

	if (offset >= 2048 * 2048 / 8 || offset < 0)
		return -1;

	return mips_btop((int)dc->dc_fbbase + offset);
}

int
fb_alloc_screen(v, scrdesc, cookiep, ccolp, crowp, attrp)
	void *v;
	const struct wsscreen_descr *scrdesc;
	void **cookiep;
	int *ccolp, *crowp;
	long *attrp;
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

void
fb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct fb_softc *sc = v;

	if (sc->sc_dc == &fb_console_dc)
		panic("fb_free_screen: console");

	sc->sc_nscreens--;
}

int
fb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb)(void *, int, int);
	void *cbarg;
{
	return 0;
}

void
fb_cnattach()
{
	struct fb_devconfig *dc = &fb_console_dc;
	struct rasops_info *ri = &dc->dc_ri;
	long defattr;

	if (!fb_is_console())
		return;

	dc->dc_fbbase = NWB253_VRAM;
	fb_common_init(dc);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&fb_stdscreen, ri, 0, ri->ri_rows - 1, defattr);
}

static u_char
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

static u_char
nwp512_data2[] = {
	0x1e, 0x08,
	0x20, 0x08,
	0x21, 0x0d
};

static u_char
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

static u_char
nwp518_data2[] = {
	0x1e, 0x08,
	0x20, 0x00,
	0x21, 0x00
};

static u_char
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

static u_char
nwe501_data2[] = {
	0x1e, 0x08,
	0x20, 0x00,
	0x21, 0x00
};

static u_char
*crtc_data[3][2] = {
	{ nwp512_data1, nwp512_data2 },
	{ nwp518_data1, nwp518_data2 },
	{ nwe501_data1, nwe501_data2 }
};

static void
fb253_init(void)
{
	volatile u_short *ctlreg = NWB253_CTLREG;
	volatile u_short *crtreg = NWB253_CRTREG;
	int id = (*ctlreg >> 8) & 0xf;
	u_char *p;
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

#if 0
static struct wsdisplay_font newsrom8x16;
static struct wsdisplay_font newsrom12x24;
static char fontarea16[96][32];
static char fontarea24[96][96];

void
initfont(ri)
	struct rasops_info *ri;
{
	int c, x;

	for (c = 0; c < 96; c++) {
		x = ((c & 0x1f) | ((c & 0xe0) << 2)) << 7;
		bcopy((char *)0xb8e00000 + x + 96, fontarea16 + c, 32);
		bcopy((char *)0xb8e00000 + x, fontarea24 + c, 96);
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
