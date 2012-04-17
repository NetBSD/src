/*	$NetBSD: pm2fb.c,v 1.7.4.1 2012/04/17 00:07:57 yamt Exp $	*/

/*
 * Copyright (c) 2009 Michael Lorenz
 * All rights reserved.
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

/*
 * A console driver for Permedia 2 graphics controllers
 * tested on sparc64 only so far
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pm2fb.c,v 1.7.4.1 2012/04/17 00:07:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <dev/videomode/videomode.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>
#include <dev/pci/pm2reg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/pci/wsdisplay_pci.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <dev/i2c/ddcvar.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>
#include <dev/videomode/edidreg.h>

#include "opt_pm2fb.h"

#ifdef PM2FB_DEBUG
#define DPRINTF aprint_error
#else
#define DPRINTF while (0) printf
#endif

struct pm2fb_softc {
	device_t sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;

	bus_space_handle_t sc_regh;
	bus_addr_t sc_fb, sc_reg;
	bus_size_t sc_fbsize, sc_regsize;

	int sc_width, sc_height, sc_depth, sc_stride;
	int sc_locked;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	/* engine stuff */
	uint32_t sc_pprod;
	/* i2c stuff */
	struct i2c_controller sc_i2c;
	uint8_t sc_edid_data[128];
};

static int	pm2fb_match(device_t, cfdata_t, void *);
static void	pm2fb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pm2fb, sizeof(struct pm2fb_softc),
    pm2fb_match, pm2fb_attach, NULL, NULL);

extern const u_char rasops_cmap[768];

static int	pm2fb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	pm2fb_mmap(void *, void *, off_t, int);
static void	pm2fb_init_screen(void *, struct vcons_screen *, int, long *);

static int	pm2fb_putcmap(struct pm2fb_softc *, struct wsdisplay_cmap *);
static int 	pm2fb_getcmap(struct pm2fb_softc *, struct wsdisplay_cmap *);
static void	pm2fb_restore_palette(struct pm2fb_softc *);
static int 	pm2fb_putpalreg(struct pm2fb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);

static void	pm2fb_init(struct pm2fb_softc *);
static void	pm2fb_flush_engine(struct pm2fb_softc *);
static void	pm2fb_rectfill(struct pm2fb_softc *, int, int, int, int,
			    uint32_t);
static void	pm2fb_bitblt(struct pm2fb_softc *, int, int, int, int, int,
			    int, int);

static void	pm2fb_cursor(void *, int, int, int);
static void	pm2fb_putchar(void *, int, int, u_int, long);
static void	pm2fb_copycols(void *, int, int, int, int);
static void	pm2fb_erasecols(void *, int, int, int, long);
static void	pm2fb_copyrows(void *, int, int, int);
static void	pm2fb_eraserows(void *, int, int, long);

struct wsdisplay_accessops pm2fb_accessops = {
	pm2fb_ioctl,
	pm2fb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

/* I2C glue */
static int pm2fb_i2c_acquire_bus(void *, int);
static void pm2fb_i2c_release_bus(void *, int);
static int pm2fb_i2c_send_start(void *, int);
static int pm2fb_i2c_send_stop(void *, int);
static int pm2fb_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int pm2fb_i2c_read_byte(void *, uint8_t *, int);
static int pm2fb_i2c_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void pm2fb_i2cbb_set_bits(void *, uint32_t);
static void pm2fb_i2cbb_set_dir(void *, uint32_t);
static uint32_t pm2fb_i2cbb_read(void *);

static void pm2_setup_i2c(struct pm2fb_softc *);

static const struct i2c_bitbang_ops pm2fb_i2cbb_ops = {
	pm2fb_i2cbb_set_bits,
	pm2fb_i2cbb_set_dir,
	pm2fb_i2cbb_read,
	{
		PM2_DD_SDA_IN,
		PM2_DD_SCL_IN,
		0,
		0
	}
};

static inline void
pm2fb_wait(struct pm2fb_softc *sc, int slots)
{
	uint32_t reg;

	do {
		reg = bus_space_read_4(sc->sc_memt, sc->sc_regh, 
			PM2_INPUT_FIFO_SPACE);
	} while (reg <= slots);
}

static void
pm2fb_flush_engine(struct pm2fb_softc *sc)
{

	pm2fb_wait(sc, 2);

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_FILTER_MODE,
	    PM2FLT_PASS_SYNC);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_SYNC, 0);
	do {
		while (bus_space_read_4(sc->sc_memt, sc->sc_regh, 
			PM2_OUTPUT_FIFO_WORDS) == 0);
	} while (bus_space_read_4(sc->sc_memt, sc->sc_regh, PM2_OUTPUT_FIFO) != 
	    0x188);
}

static int
pm2fb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_3DLABS)
		return 0;

	/* only cards tested on so far - likely need a list */
	if ((PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3DLABS_PERMEDIA2) ||
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3DLABS_PERMEDIA2V))
		return 100;
	return (0);
}

static void
pm2fb_attach(device_t parent, device_t self, void *aux)
{
	struct pm2fb_softc	*sc = device_private(self);
	struct pci_attach_args	*pa = aux;
	struct rasops_info	*ri;
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t	dict;
	unsigned long		defattr;
	bool			is_console;
	int i, j;
	uint32_t flags;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;
	sc->sc_dev = self;

	pci_aprint_devinfo(pa, NULL);

	/* fill in parameters from properties */
	dict = device_properties(self);
	if (!prop_dictionary_get_uint32(dict, "width", &sc->sc_width)) {
		aprint_error("%s: no width property\n", device_xname(self));
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "height", &sc->sc_height)) {
		aprint_error("%s: no height property\n", device_xname(self));
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "depth", &sc->sc_depth)) {
		aprint_error("%s: no depth property\n", device_xname(self));
		return;
	}
	/*
	 * don't look at the linebytes property - The Raptor firmware lies
	 * about it. Get it from width * depth >> 3 instead.
	 */
	sc->sc_stride = sc->sc_width * (sc->sc_depth >> 3);

	prop_dictionary_get_bool(dict, "is_console", &is_console);

	pci_mapreg_info(pa->pa_pc, pa->pa_tag, 0x14, PCI_MAPREG_TYPE_MEM,
	    &sc->sc_fb, &sc->sc_fbsize, &flags);

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_memt, &sc->sc_regh, &sc->sc_reg, &sc->sc_regsize)) {
		aprint_error("%s: failed to map registers.\n",
		    device_xname(sc->sc_dev));
	}

	/*
	 * XXX yeah, casting the fb address to uint32_t is formally wrong
	 * but as far as I know there are no PM2 with 64bit BARs
	 */
	aprint_normal("%s: %d MB aperture at 0x%08x\n", device_xname(self),
	    (int)(sc->sc_fbsize >> 20), (uint32_t)sc->sc_fb);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_locked = 0;

	pm2_setup_i2c(sc);

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &pm2fb_accessops);
	sc->vd.init_screen = pm2fb_init_screen;

	/* init engine here */
	pm2fb_init(sc);

	ri = &sc->sc_console_screen.scr_ri;

	j = 0;
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		pm2fb_putpalreg(sc, i, rasops_cmap[j], rasops_cmap[j + 1],
		    rasops_cmap[j + 2]);
		j += 3;
	}

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		pm2fb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);
		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &pm2fb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);	
}

static int
pm2fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct pm2fb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	/* PCI config read/write passthrough. */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_pcitag,
		    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
		    sc->sc_pcitag, data);

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;
		wdf = (void *)data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return pm2fb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return pm2fb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;
		return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if(new_mode == WSDISPLAYIO_MODE_EMUL) {
				pm2fb_restore_palette(sc);
				vcons_redraw_screen(ms);
			} else
				pm2fb_flush_engine(sc);
		}
		}
		return 0;
	case WSDISPLAYIO_GET_EDID: {
		struct wsdisplayio_edid_info *d = data;
		d->data_size = 128;
		if (d->buffer_size < 128)
			return EAGAIN;
		return copyout(sc->sc_edid_data, d->edid_data, 128);
	}
	}
	return EPASSTHROUGH;
}

static paddr_t
pm2fb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct pm2fb_softc *sc = vd->cookie;
	paddr_t pa;

	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_fbsize) {
		pa = bus_space_mmap(sc->sc_memt, sc->sc_fb + offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR);
		return pa;
	}

	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	if (kauth_authorize_machdep(kauth_cred_get(), KAUTH_MACHDEP_UNMANAGEDMEM,
	    NULL, NULL, NULL, NULL) != 0) {
		aprint_normal("%s: mmap() rejected.\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	if ((offset >= sc->sc_fb) && (offset < (sc->sc_fb + sc->sc_fbsize))) {
		pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR);
		return pa;
	}

	if ((offset >= sc->sc_reg) && 
	    (offset < (sc->sc_reg + sc->sc_regsize))) {
		pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR);
		return pa;
	}

#ifdef PCI_MAGIC_IO_RANGE
	/* allow mapping of IO space */
	if ((offset >= PCI_MAGIC_IO_RANGE) &&
	    (offset < PCI_MAGIC_IO_RANGE + 0x10000)) {
		pa = bus_space_mmap(sc->sc_iot, offset - PCI_MAGIC_IO_RANGE,
		    0, prot, BUS_SPACE_MAP_LINEAR);
		return pa;
	}
#endif

	return -1;
}

static void
pm2fb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct pm2fb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	ri->ri_ops.copyrows = pm2fb_copyrows;
	ri->ri_ops.copycols = pm2fb_copycols;
	ri->ri_ops.cursor = pm2fb_cursor;
	ri->ri_ops.eraserows = pm2fb_eraserows;
	ri->ri_ops.erasecols = pm2fb_erasecols;
	ri->ri_ops.putchar = pm2fb_putchar;
}

static int
pm2fb_putcmap(struct pm2fb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef PM2FB_DEBUG
	aprint_debug("putcmap: %d %d\n",index, count);
#endif
	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++) {
		pm2fb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
pm2fb_getcmap(struct pm2fb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static void
pm2fb_restore_palette(struct pm2fb_softc *sc)
{
	int i;

	for (i = 0; i < (1 << sc->sc_depth); i++) {
		pm2fb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static int
pm2fb_putpalreg(struct pm2fb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM2_DAC_PAL_WRITE_IDX, idx);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM2_DAC_DATA, r);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM2_DAC_DATA, g);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM2_DAC_DATA, b);
	return 0;
}

static void
pm2fb_init(struct pm2fb_softc *sc)
{
	pm2fb_flush_engine(sc);

	pm2fb_wait(sc, 8);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_SCREEN_BASE, 0);
#if 0
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_BYPASS_MASK, 
		0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_FB_WRITE_MASK, 
		0xffffffff);
#endif
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_HW_WRITEMASK, 
		0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_SW_WRITEMASK, 
		0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_WRITE_MODE, 
		PM2WM_WRITE_EN);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_SCREENSIZE,
	    (sc->sc_height << 16) | sc->sc_width);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_SCISSOR_MODE, 
	    PM2SC_SCREEN_EN);
	pm2fb_wait(sc, 8);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DITHER_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_ALPHA_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DDA_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_TEX_COLOUR_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_TEX_ADDRESS_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_TEX_READ_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_TEX_LUT_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_YUV_MODE, 0);
	pm2fb_wait(sc, 8);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DEPTH_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DEPTH, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_STENCIL_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_STIPPLE_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_ROP_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_WINDOW_ORIGIN, 0);
	sc->sc_pprod = bus_space_read_4(sc->sc_memt, sc->sc_regh, 
	    PM2_FB_READMODE) &
	    (PM2FB_PP0_MASK | PM2FB_PP1_MASK | PM2FB_PP2_MASK);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_FB_READMODE, 
	    sc->sc_pprod);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_TEXMAP_FORMAT, 
	    sc->sc_pprod);
	pm2fb_wait(sc, 8);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DY, 1 << 16);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DXDOM, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_STARTXDOM, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_STARTXSUB, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_STARTY, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_COUNT, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_SCISSOR_MINYX, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_SCISSOR_MAXYX,
	    0x0fff0fff);
	pm2fb_flush_engine(sc);
}

static void
pm2fb_rectfill(struct pm2fb_softc *sc, int x, int y, int wi, int he,
     uint32_t colour)
{

	pm2fb_wait(sc, 7);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DDA_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_CONFIG,
	    PM2RECFG_WRITE_EN);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_BLOCK_COLOUR,
	    colour);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_RECT_START,
	    (y << 16) | x);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_RECT_SIZE,
	    (he << 16) | wi);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_RENDER,
	    PM2RE_RECTANGLE | PM2RE_INC_X | PM2RE_INC_Y | PM2RE_FASTFILL);
}

static void
pm2fb_bitblt(struct pm2fb_softc *sc, int xs, int ys, int xd, int yd,
    int wi, int he, int rop)
{
	uint32_t dir = 0;

	if (yd <= ys) {
		dir |= PM2RE_INC_Y;
	}
	if (xd <= xs) {
		dir |= PM2RE_INC_X;
	}
	pm2fb_wait(sc, 7);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_DDA_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_MODE, 0);
	if (rop == 3) {
		bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_CONFIG,
		    PM2RECFG_READ_SRC | PM2RECFG_WRITE_EN | PM2RECFG_ROP_EN |
		    PM2RECFG_PACKED | (rop << 6));
	} else {
		bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_CONFIG,
		    PM2RECFG_READ_SRC | PM2RECFG_READ_DST | PM2RECFG_WRITE_EN |
		    PM2RECFG_PACKED | PM2RECFG_ROP_EN | (rop << 6));
	}
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_RECT_START,
	    (yd << 16) | xd);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_RECT_SIZE,
	    (he << 16) | wi);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_SOURCE_DELTA,
	    (((ys - yd) & 0xfff) << 16) | ((xs - xd) & 0xfff));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_RE_RENDER,
	    PM2RE_RECTANGLE | dir);
}

static void
pm2fb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm2fb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			pm2fb_bitblt(sc, x, y, x, y, wi, he, 12);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			pm2fb_bitblt(sc, x, y, x, y, wi, he, 12);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}

}

static void
pm2fb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct pm2fb_softc *sc = scr->scr_cookie;
	uint32_t mode;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		void *data;
		uint32_t fg, bg;
		int uc, i;
		int x, y, wi, he;

		wi = font->fontwidth;
		he = font->fontheight;

		if (!CHAR_IN_FONT(c, font))
			return;
		bg = ri->ri_devcmap[(attr >> 16) & 0xf];
		fg = ri->ri_devcmap[(attr >> 24) & 0xf];
		x = ri->ri_xorigin + col * wi;
		y = ri->ri_yorigin + row * he;
		if (c == 0x20) {
			pm2fb_rectfill(sc, x, y, wi, he, bg);
		} else {
			uc = c - font->firstchar;
			data = (uint8_t *)font->data + uc * ri->ri_fontscale;

			mode = PM2RM_MASK_MIRROR;
			switch (ri->ri_font->stride) {
				case 1:
					mode |= 3 << 7;
					break;
				case 2:
					mode |= 2 << 7;
					break;
			}

			pm2fb_wait(sc, 8);

			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM2_RE_MODE, mode);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_CONFIG, PM2RECFG_WRITE_EN);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_BLOCK_COLOUR, bg);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_RECT_START, (y << 16) | x);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_RECT_SIZE, (he << 16) | wi);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_RENDER,
			    PM2RE_RECTANGLE |
			    PM2RE_INC_X | PM2RE_INC_Y | PM2RE_FASTFILL);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_BLOCK_COLOUR, fg);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM2_RE_RENDER,
			    PM2RE_RECTANGLE | PM2RE_SYNC_ON_MASK |
			    PM2RE_INC_X | PM2RE_INC_Y | PM2RE_FASTFILL);

			pm2fb_wait(sc, he);
			switch (ri->ri_font->stride) {
			case 1: {
				uint8_t *data8 = data;
				uint32_t reg;
				for (i = 0; i < he; i++) {
					reg = *data8;
					bus_space_write_4(sc->sc_memt, 
					    sc->sc_regh,
					    PM2_RE_BITMASK, reg);
					data8++;
				}
				break;
				}
			case 2: {
				uint16_t *data16 = data;
				uint32_t reg;
				for (i = 0; i < he; i++) {
					reg = *data16;
					bus_space_write_4(sc->sc_memt, 
					    sc->sc_regh,
					    PM2_RE_BITMASK, reg);
					data16++;
				}
				break;
			}
			}
		}
	}
}

static void
pm2fb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm2fb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		pm2fb_bitblt(sc, xs, y, xd, y, width, height, 3);
	}
}

static void
pm2fb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm2fb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		pm2fb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
pm2fb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm2fb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight*nrows;
		pm2fb_bitblt(sc, x, ys, x, yd, width, height, 3);
	}
}

static void
pm2fb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm2fb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		pm2fb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
pm2_setup_i2c(struct pm2fb_softc *sc)
{
#ifdef PM2FB_DEBUG
	struct edid_info ei;
#endif
	int i;

	/* Fill in the i2c tag */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = pm2fb_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = pm2fb_i2c_release_bus;
	sc->sc_i2c.ic_send_start = pm2fb_i2c_send_start;
	sc->sc_i2c.ic_send_stop = pm2fb_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = pm2fb_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = pm2fb_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = pm2fb_i2c_write_byte;
	sc->sc_i2c.ic_exec = NULL;

	DPRINTF("data: %08x\n", bus_space_read_4(sc->sc_memt, sc->sc_regh,
		PM2_DISPLAY_DATA));

	/* make sure we're in i2c mode */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_DISPLAY_DATA, 0);

	/* zero out the EDID buffer */
	memset(sc->sc_edid_data, 0, 128);

	/* Some monitors don't respond first time */
	i = 0;
	while (sc->sc_edid_data[1] == 0 && i++ < 3)
		ddc_read_edid(&sc->sc_i2c, sc->sc_edid_data, 128);
#ifdef PM2FB_DEBUG
	if (edid_parse(&sc->sc_edid_data[0], &ei) != -1) {
		edid_print(&ei);
	}
#endif
}

/* I2C bitbanging */
static void pm2fb_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct pm2fb_softc *sc = cookie;
	uint32_t out;

	out = bits << 2;	/* bitmasks match the IN bits */

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM2_DISPLAY_DATA, out);
}

static void pm2fb_i2cbb_set_dir(void *cookie, uint32_t dir)
{
	/* Nothing to do */
}

static uint32_t pm2fb_i2cbb_read(void *cookie)
{
	struct pm2fb_softc *sc = cookie;
	uint32_t bits;

	bits = bus_space_read_4(sc->sc_memt, sc->sc_regh, PM2_DISPLAY_DATA);

	return bits;
}

/* higher level I2C stuff */
static int
pm2fb_i2c_acquire_bus(void *cookie, int flags)
{
	/* private bus */
	return (0);
}

static void
pm2fb_i2c_release_bus(void *cookie, int flags)
{
	/* private bus */
}

static int
pm2fb_i2c_send_start(void *cookie, int flags)
{
	return (i2c_bitbang_send_start(cookie, flags, &pm2fb_i2cbb_ops));
}

static int
pm2fb_i2c_send_stop(void *cookie, int flags)
{

	return (i2c_bitbang_send_stop(cookie, flags, &pm2fb_i2cbb_ops));
}

static int
pm2fb_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &pm2fb_i2cbb_ops));
}

static int
pm2fb_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return (i2c_bitbang_read_byte(cookie, valp, flags, &pm2fb_i2cbb_ops));
}

static int
pm2fb_i2c_write_byte(void *cookie, uint8_t val, int flags)
{
	return (i2c_bitbang_write_byte(cookie, val, flags, &pm2fb_i2cbb_ops));
}
