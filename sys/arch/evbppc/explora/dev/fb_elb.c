/*	$NetBSD: fb_elb.c,v 1.1 2003/03/11 10:57:57 hannken Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <machine/explora.h>
#include <machine/bus.h>

#include <evbppc/explora/dev/elbvar.h>

#define FB_NPORTS		65536

struct fb_dev {
	void *fb_vram;
	bus_space_tag_t fb_iot;
	bus_space_handle_t fb_ioh;
	struct rasops_info fb_ri;
};

struct fb_elb_softc {
	struct device sc_dev;
	struct fb_dev *sc_fb;
	int sc_nscreens;
};

static int	fb_elb_probe(struct device *, struct cfdata *, void *);
static void	fb_elb_attach(struct device *, struct device *, void *);
void		fb_cnattach(bus_space_tag_t, bus_addr_t, void *);
static void	fb_init(struct fb_dev *, int);
static int	fb_ioctl(void *, u_long, caddr_t, int, struct proc *);
static paddr_t	fb_mmap(void *, off_t, int);
static int	fb_alloc_screen(void *, const struct wsscreen_descr *, void **,
                    int *, int *, long *);
static void	fb_free_screen(void *, void *);
static int	fb_show_screen(void *, void *, int, void (*)(void *, int, int),
                    void *);

static void	s3_init(struct fb_dev *, int *, int *);

static struct fb_dev console_dev;

static struct wsdisplay_accessops accessops = {
	fb_ioctl,
	fb_mmap,
	fb_alloc_screen,
	fb_free_screen,
	fb_show_screen,
	NULL
};

static struct wsscreen_descr stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	0
};

static const struct wsscreen_descr *scrlist[] = {
	&stdscreen
};

static struct wsscreen_list screenlist = {
	sizeof(scrlist)/sizeof(scrlist[0]), scrlist
};

CFATTACH_DECL(fb_elb, sizeof(struct fb_elb_softc),
    fb_elb_probe, fb_elb_attach, NULL, NULL);

static int
fb_elb_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct elb_attach_args *oaa = aux;

	if (strcmp(oaa->elb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

static void
fb_elb_attach(struct device *parent, struct device *self, void *aux)
{
	struct fb_elb_softc *sc = (void *)self;
	struct elb_attach_args *eaa = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct rasops_info *ri;
	bus_space_handle_t ioh;
	int is_console;

	is_console = ((void *)eaa->elb_base == console_dev.fb_vram);

	if (is_console) {
		sc->sc_fb = &console_dev;
	} else {
		sc->sc_fb = malloc(sizeof(struct fb_dev), M_DEVBUF, M_WAITOK);
		memset(sc->sc_fb, 0, sizeof(struct fb_dev));
	}

	sc->sc_fb->fb_iot = eaa->elb_bt;
	bus_space_map(sc->sc_fb->fb_iot, eaa->elb_base, SIZE_FB,
	    BUS_SPACE_MAP_LINEAR, &ioh);
	sc->sc_fb->fb_vram = bus_space_vaddr(sc->sc_fb->fb_iot, ioh);
	bus_space_map(sc->sc_fb->fb_iot, eaa->elb_base2, FB_NPORTS,
	    0, &sc->sc_fb->fb_ioh);

	fb_init(sc->sc_fb, !is_console);

	ri = &sc->sc_fb->fb_ri;

	printf(": %d x %d\n", ri->ri_rows, ri->ri_cols);

	waa.console = is_console;
	waa.scrdata = &screenlist;
	waa.accessops = &accessops;
	waa.accesscookie = sc;

	config_found_sm(self, &waa, wsemuldisplaydevprint, NULL);
}

static void
fb_init(struct fb_dev *fb, int full)
{
	struct rasops_info *ri = &fb->fb_ri;

	if (full) {
		s3_init(fb, &ri->ri_width, &ri->ri_height);
		ri->ri_depth = 8;
		ri->ri_stride = ri->ri_width;
		ri->ri_bits = fb->fb_vram;
		ri->ri_flg = RI_CENTER;

		rasops_init(ri, 500, 500);
	} else {
		ri->ri_origbits = fb->fb_vram;	/*XXX*/
		rasops_reconfig(ri, 500, 500);
	}

	stdscreen.nrows = ri->ri_rows;
	stdscreen.ncols = ri->ri_cols; 
	stdscreen.textops = &ri->ri_ops;
	stdscreen.capabilities = ri->ri_caps;
}

static int
fb_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct fb_elb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_fb->fb_ri;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX */
		return(0);

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = ri->ri_height;
		wdf->width = ri->ri_width;
		wdf->depth = ri->ri_depth;
		wdf->cmsize = 16; /*XXX*/
		return(0);

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		break;
	}

	return(EPASSTHROUGH);
}

static paddr_t
fb_mmap(void *v, off_t offset, int prot)
{
	return -1;
}

static int
fb_alloc_screen(void *v, const struct wsscreen_descr *scrdesc, void **cookiep,
    int *ccolp, int *crowp, long *attrp)
{
	struct fb_elb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_fb->fb_ri;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*ccolp = *crowp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, attrp);
	sc->sc_nscreens++;

	return(0);
}

static void
fb_free_screen(void *v, void *cookie)
{
	struct fb_elb_softc *sc = v;

	if (sc->sc_fb == &console_dev)
		panic("fb_free_screen: freeing console");

	sc->sc_nscreens--;
}

static int
fb_show_screen(void *v, void *cookie, int waitok, void (*cb)(void *, int, int),
    void *cbarg)
{
	return(0);
}

void
fb_cnattach(bus_space_tag_t iot, bus_addr_t iobase, void *vram)
{
	struct rasops_info *ri = &console_dev.fb_ri;
	long defattr;

	console_dev.fb_iot = iot;
	console_dev.fb_ioh = iobase;
	console_dev.fb_vram = vram;

	fb_init(&console_dev, 1);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

	wsdisplay_cnattach(&stdscreen, ri, 0, 0, defattr);
}

/*
 * S3 support routines
 */

#define S3_CRTC_INDEX		0x83d4
#define S3_CRTC_DATA		0x83d5

#define S3_DAC_RD_INDEX		0x83c7
#define S3_DAC_WR_INDEX		0x83c8
#define S3_DAC_DATA		0x83c9

#define CMAP_SIZE		256

static u_int8_t default_cmap[] = {
	/* black */		  0,   0,   0,
	/* blue */		  0,   0, 192,
	/* green */		  0, 192,   0,
	/* cyan */		  0, 192, 192,
	/* red */		192,   0,   0,
	/* magenta */		192,   0, 192,
	/* brown */		192, 192,   0,
	/* lightgrey */		212, 208, 200,
	/* darkgrey */		200, 192, 188,
	/* lightblue */		  0,   0, 255,
	/* lightgreen */	  0, 255,   0,
	/* lightcyan */		  0, 255, 255,
	/* lightred */		255,   0,   0,
	/* lightmagenta */	255,   0, 255,
	/* yellow */		255, 255,   0,
	/* white */		255, 255, 255,
};

static void
s3_init(struct fb_dev *fb, int *width, int *height)
{
	int i, j, w, h;
	bus_space_tag_t iot = fb->fb_iot;
	bus_space_handle_t ioh = fb->fb_ioh;

	/* Initialize colormap */

	bus_space_write_1(iot, ioh, S3_DAC_WR_INDEX, 0);

	for (i = j = 0; i < CMAP_SIZE*3; i++) {
		bus_space_write_1(iot, ioh, S3_DAC_DATA, default_cmap[j] >> 2);
		j = (j+1) % sizeof(default_cmap)/sizeof(default_cmap[0]);
	}

	/* Retrieve frame buffer geometry */

	bus_space_write_1(iot, ioh, S3_CRTC_INDEX, 1);
	w = bus_space_read_1(iot, ioh, S3_CRTC_DATA);

	bus_space_write_1(iot, ioh, S3_CRTC_INDEX, 18);
	h = bus_space_read_1(iot, ioh, S3_CRTC_DATA);

	bus_space_write_1(iot, ioh, S3_CRTC_INDEX, 7);
	i = bus_space_read_1(iot, ioh, S3_CRTC_DATA);

	h += (i << 7) & 0x100;
	h += (i << 3) & 0x200;

	*width = (w+1) << 3;
	*height = h+1;
}
