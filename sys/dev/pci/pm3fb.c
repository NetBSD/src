/*
 * Copyright (c) 2015 Naruaki Etomi
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
 * A console driver for Permedia 3 graphics controllers
 * most of the following was adapted from the xf86-video-glint driver's
 * pm3_accel.c, pm3_dac.c and pm2fb framebuffer console driver
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>
#include <sys/atomic.h>

#include <dev/videomode/videomode.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>
#include <dev/pci/pm3reg.h>

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

struct pm3fb_softc {
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
	/* i2c stuff */
	struct i2c_controller sc_i2c;
	uint8_t sc_edid_data[128];
	struct edid_info sc_ei;
	const struct videomode *sc_videomode;
};

static int	pm3fb_match(device_t, cfdata_t, void *);
static void	pm3fb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pm3fb, sizeof(struct pm3fb_softc),
    pm3fb_match, pm3fb_attach, NULL, NULL);

extern const u_char rasops_cmap[768];

static int	pm3fb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	pm3fb_mmap(void *, void *, off_t, int);
static void	pm3fb_init_screen(void *, struct vcons_screen *, int, long *);

static int	pm3fb_putcmap(struct pm3fb_softc *, struct wsdisplay_cmap *);
static int	pm3fb_getcmap(struct pm3fb_softc *, struct wsdisplay_cmap *);
static void	pm3fb_init_palette(struct pm3fb_softc *);
static int	pm3fb_putpalreg(struct pm3fb_softc *, uint8_t, uint8_t, uint8_t, uint8_t);

static void	pm3fb_init(struct pm3fb_softc *);
static inline void pm3fb_wait(struct pm3fb_softc *, int);
static void	pm3fb_flush_engine(struct pm3fb_softc *);
static void	pm3fb_rectfill(struct pm3fb_softc *, int, int, int, int, uint32_t);
static void	pm3fb_bitblt(void *, int, int, int, int, int, int, int);

static void	pm3fb_cursor(void *, int, int, int);
static void	pm3fb_putchar(void *, int, int, u_int, long);
static void	pm3fb_copycols(void *, int, int, int, int);
static void	pm3fb_erasecols(void *, int, int, int, long);
static void	pm3fb_copyrows(void *, int, int, int);
static void	pm3fb_eraserows(void *, int, int, long);

struct wsdisplay_accessops pm3fb_accessops = {
	pm3fb_ioctl,
	pm3fb_mmap,
	NULL,    /* alloc_screen */
	NULL,    /* free_screen */
	NULL,    /* show_screen */
	NULL,    /* load_font */
	NULL,    /* pollc */
	NULL     /* scroll */
};

/* I2C glue */
static int pm3fb_i2c_acquire_bus(void *, int);
static void pm3fb_i2c_release_bus(void *, int);
static int pm3fb_i2c_send_start(void *, int);
static int pm3fb_i2c_send_stop(void *, int);
static int pm3fb_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int pm3fb_i2c_read_byte(void *, uint8_t *, int);
static int pm3fb_i2c_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void pm3fb_i2cbb_set_bits(void *, uint32_t);
static void pm3fb_i2cbb_set_dir(void *, uint32_t);
static uint32_t pm3fb_i2cbb_read(void *);

static void pm3_setup_i2c(struct pm3fb_softc *);

static const struct i2c_bitbang_ops pm3fb_i2cbb_ops = {
pm3fb_i2cbb_set_bits,
	pm3fb_i2cbb_set_dir,
	pm3fb_i2cbb_read,
	{
		PM3_DD_SDA_IN,
		PM3_DD_SCL_IN,
		0,
		0
	}
};

/* mode setting stuff */
static int pm3fb_set_pll(struct pm3fb_softc *, int);
static void pm3fb_write_dac(struct pm3fb_softc *, int, uint8_t);
static void pm3fb_set_mode(struct pm3fb_softc *, const struct videomode *);

static inline void
pm3fb_wait(struct pm3fb_softc *sc, int slots)
{
    uint32_t reg;

    do {
		reg = bus_space_read_4(sc->sc_memt, sc->sc_regh,
		    PM3_INPUT_FIFO_SPACE);
	} while (reg <= slots);
}

static void
pm3fb_flush_engine(struct pm3fb_softc *sc)
{

	pm3fb_wait(sc, 2);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FILTER_MODE, PM3_FM_PASS_SYNC);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SYNC, 0);

	do {
		while (bus_space_read_4(sc->sc_memt, sc->sc_regh, PM3_OUTPUT_FIFO_WORDS) == 0);
	} while (bus_space_read_4(sc->sc_memt, sc->sc_regh, PM3_OUTPUT_FIFO) !=
	    PM3_SYNC_TAG);
}

static int
pm3fb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_3DLABS)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3DLABS_PERMEDIA3)
		return 100;
	return (0);
}

static void
pm3fb_attach(device_t parent, device_t self, void *aux)
{
	struct pm3fb_softc	*sc = device_private(self);
	struct pci_attach_args	*pa = aux;
	struct rasops_info	*ri;
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t	dict;
	unsigned long		defattr;
	bool			is_console;
	uint32_t		flags;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;
	sc->sc_dev = self;

	pci_aprint_devinfo(pa, NULL);

	/*
	 * fill in parameters from properties
	 * if we can't get a usable mode via DDC2 we'll use this to pick one,
	 * which is why we fill them in with some conservative values that
	 * hopefully work as a last resort
	 */
	dict = device_properties(self);
	if (!prop_dictionary_get_uint32(dict, "width", &sc->sc_width)) {
		aprint_error("%s: no width property\n", device_xname(self));
		sc->sc_width = 1280;
	}
	if (!prop_dictionary_get_uint32(dict, "height", &sc->sc_height)) {
		aprint_error("%s: no height property\n", device_xname(self));
		sc->sc_height = 1024;
	}
	if (!prop_dictionary_get_uint32(dict, "depth", &sc->sc_depth)) {
		aprint_error("%s: no depth property\n", device_xname(self));
		sc->sc_depth = 8;
	}

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
	 * Permedia 3 always return 64MB fbsize
	 * 16 MB should be enough -- more just wastes map entries
	 */
	if (sc->sc_fbsize != 0)
	    sc->sc_fbsize = (16 << 20);

	/*
	 * Some Power Mac G4 model could not initialize these registers,
	 * Power Mac G4 (Mirrored Drive Doors), for example
	 */
#if defined(__powerpc__)
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LOCALMEMCAPS, 0x02e311B8);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LOCALMEMTIMINGS, 0x07424905);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LOCALMEMCONTROL, 0x0c000003);
#endif

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

	pm3_setup_i2c(sc);

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &pm3fb_accessops);

	sc->vd.init_screen = pm3fb_init_screen;

	/* init engine here */
	pm3fb_init(sc);

	ri = &sc->sc_console_screen.scr_ri;

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		pm3fb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);
		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		if (sc->sc_console_screen.scr_ri.ri_rows == 0) {
			/* do some minimal setup to avoid weirdnesses later */
			vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
			   &defattr);
		}
	}

	pm3fb_init_palette(sc);

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &pm3fb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
pm3fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct pm3fb_softc *sc = vd->cookie;
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
		return pm3fb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return pm3fb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;
		return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if(new_mode == WSDISPLAYIO_MODE_EMUL) {
				/* first set the video mode */
				if (sc->sc_videomode != NULL) {
					pm3fb_set_mode(sc, sc->sc_videomode);
				}
				/* then initialize the drawing engine */
				pm3fb_init(sc);
				pm3fb_init_palette(sc);
				vcons_redraw_screen(ms);
			} else
				pm3fb_flush_engine(sc);
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

	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;
		return wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
	}
	}
	return EPASSTHROUGH;
}

static paddr_t
pm3fb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct pm3fb_softc *sc = vd->cookie;
	paddr_t pa;

	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_fbsize) {
		pa = bus_space_mmap(sc->sc_memt, sc->sc_fb + offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
		return pa;
	}

	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	if (kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM,
	    NULL, NULL, NULL, NULL) != 0) {
		aprint_normal("%s: mmap() rejected.\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	if ((offset >= sc->sc_fb) && (offset < (sc->sc_fb + sc->sc_fbsize))) {
		pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
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
pm3fb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct pm3fb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;
	if (sc->sc_depth == 8)
		ri->ri_flg |= RI_8BIT_IS_RGB;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_UNDERLINE;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	ri->ri_ops.copyrows = pm3fb_copyrows;
	ri->ri_ops.copycols = pm3fb_copycols;
	ri->ri_ops.cursor = pm3fb_cursor;
	ri->ri_ops.eraserows = pm3fb_eraserows;
	ri->ri_ops.erasecols = pm3fb_erasecols;
	ri->ri_ops.putchar = pm3fb_putchar;
}

static int
pm3fb_putcmap(struct pm3fb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

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
		pm3fb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
pm3fb_getcmap(struct pm3fb_softc *sc, struct wsdisplay_cmap *cm)
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
pm3fb_init_palette(struct pm3fb_softc *sc)
{
	struct rasops_info *ri = &sc->sc_console_screen.scr_ri;
	int i, j = 0;
	uint8_t cmap[768];

	rasops_get_cmap(ri, cmap, sizeof(cmap));

	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = cmap[j];
		sc->sc_cmap_green[i] = cmap[j + 1];
		sc->sc_cmap_blue[i] = cmap[j + 2];
		pm3fb_putpalreg(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
		j += 3;
	}
}

static int
pm3fb_putpalreg(struct pm3fb_softc *sc, uint8_t idx, uint8_t r, uint8_t g, uint8_t b)
{

	pm3fb_wait(sc, 4);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_PAL_WRITE_IDX, idx);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_PAL_DATA, r);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_PAL_DATA, g);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_PAL_DATA, b);
	return 0;
}

static void
pm3fb_write_dac(struct pm3fb_softc *sc, int reg, uint8_t data)
{

	pm3fb_wait(sc, 3);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_INDEX_LOW, reg & 0xff);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_INDEX_HIGH, (reg >> 8) & 0xff);
	bus_space_write_1(sc->sc_memt, sc->sc_regh, PM3_DAC_INDEX_DATA, data);
}

static void
pm3fb_init(struct pm3fb_softc *sc)
{

	pm3fb_wait(sc, 16);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LB_DESTREAD_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LB_DESTREAD_ENABLES, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LB_SOURCEREAD_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LB_WRITE_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FILTER_MODE, PM3_FM_PASS_SYNC);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_STATISTIC_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DELTA_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_RASTERIZER_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SCISSOR_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LINESTIPPLE_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_AREASTIPPLE_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_GID_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DEPTH_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_STENCIL_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_STENCIL_DATA, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_COLORDDA_MODE, 0);

	pm3fb_wait(sc, 16);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTUREADDRESS_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTUREINDEX_MODE0, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTUREINDEX_MODE1, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTUREREAD_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXELLUT_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTUREFILTER_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTURECOMPOSITE_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTURECOLOR_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTURECOMPOSITECOLOR_MODE1, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTURECOMPOSITEALPHA_MODE1, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTURECOMPOSITECOLOR_MODE0, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_TEXTURECOMPOSITEALPHA_MODE0, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FOG_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_CHROMATEST_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_ALPHATEST_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_ANTIALIAS_MODE, 0);

	pm3fb_wait(sc, 16);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_YUV_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_ALPHABLENDCOLOR_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_ALPHABLENDALPHA_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DITHER_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_LOGICALOP_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_ROUTER_MODE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_WINDOW, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_CONFIG2D, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SPANCOLORMASK, 0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_XBIAS, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_YBIAS, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DELTACONTROL, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_BITMASKPATTERN, 0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBDESTREAD_ENABLE,
	    PM3_FBDESTREAD_SET(0xff, 0xff, 0xff));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBDESTREAD_BUFFERADDRESS0, 0);

	pm3fb_wait(sc, 16);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBDESTREAD_BUFFEROFFSET0, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBDESTREAD_BUFFERWIDTH0,
	    PM3_FBDESTREAD_BUFFERWIDTH_WIDTH(sc->sc_stride));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FB_DESTREAD_MODE,
	    PM3_FBDRM_ENABLE | PM3_FBDRM_ENABLE0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBSOURCEREAD_BUFFERADDRESS, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBSOURCEREAD_BUFFEROFFSET, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBSOURCEREAD_BUFFERWIDTH,
	    PM3_FBSOURCEREAD_BUFFERWIDTH_WIDTH(sc->sc_stride));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBSOURCEREAD_MODE,
	    PM3_FBSOURCEREAD_MODE_BLOCKING | PM3_FBSOURCEREAD_MODE_ENABLE);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_PIXEL_SIZE, PM3_PS_8BIT);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBSOFTWAREWRITEMASK, 0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBHARDWAREWRITEMASK, 0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBWRITE_MODE,
	    PM3_FBWRITEMODE_WRITEENABLE | PM3_FBWRITEMODE_OPAQUESPAN | PM3_FBWRITEMODE_ENABLE0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBWRITEBUFFERADDRESS0, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBWRITEBUFFEROFFSET0, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBWRITEBUFFERWIDTH0,
	    PM3_FBWRITEBUFFERWIDTH_WIDTH(sc->sc_stride));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SIZEOF_FRAMEBUFFER, 4095);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DITHER_MODE, PM3_CF_TO_DIM_CF(4));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DXDOM, 0);

	pm3fb_wait(sc, 6);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DXSUB, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DY, 1 << 16);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_STARTXDOM, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_STARTXSUB, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_STARTY, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_COUNT, 0);
}

static void
pm3fb_rectfill(struct pm3fb_softc *sc, int x, int y, int wi, int he,
     uint32_t colour)
{
	pm3fb_wait(sc, 4);

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_CONFIG2D,
	    PM3_CONFIG2D_USECONSTANTSOURCE | PM3_CONFIG2D_FOREGROUNDROP_ENABLE | 
	    (PM3_CONFIG2D_FOREGROUNDROP(0x3)) | PM3_CONFIG2D_FBWRITE_ENABLE);

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FOREGROUNDCOLOR, colour);

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_RECTANGLEPOSITION,
	    (((y) & 0xffff) << 16) | ((x) & 0xffff) );

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_RENDER2D,
	    PM3_RENDER2D_XPOSITIVE | PM3_RENDER2D_YPOSITIVE | 
	    PM3_RENDER2D_OPERATION_NORMAL | PM3_RENDER2D_SPANOPERATION | 
	    (((he) & 0x0fff) << 16) | ((wi) & 0x0fff));

#ifdef PM3FB_DEBUG
	pm3fb_flush_engine(sc);
#endif
}

static void
pm3fb_bitblt(void *cookie, int srcx, int srcy, int dstx, int dsty,
    int width, int height, int rop)
{
	struct pm3fb_softc *sc = cookie;
	int x_align,  offset_x, offset_y; 
	uint32_t dir = 0;

	offset_x = srcx - dstx;
	offset_y = srcy - dsty;

	if (dsty <= srcy) {
		dir |= PM3_RENDER2D_YPOSITIVE;
	}

	if (dstx <= srcx) {
		dir |= PM3_RENDER2D_XPOSITIVE;
	}

	x_align = (srcx & 0x1f);

	pm3fb_wait(sc, 6);

	if (rop == 3){ 
		bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_CONFIG2D,
		    PM3_CONFIG2D_USERSCISSOR_ENABLE | PM3_CONFIG2D_FOREGROUNDROP_ENABLE | PM3_CONFIG2D_BLOCKING |
		    PM3_CONFIG2D_FOREGROUNDROP(rop) | PM3_CONFIG2D_FBWRITE_ENABLE);
	} else {
		bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_CONFIG2D,
		    PM3_CONFIG2D_USERSCISSOR_ENABLE | PM3_CONFIG2D_FOREGROUNDROP_ENABLE | PM3_CONFIG2D_BLOCKING |
		    PM3_CONFIG2D_FOREGROUNDROP(rop) | PM3_CONFIG2D_FBWRITE_ENABLE | PM3_CONFIG2D_FBDESTREAD_ENABLE);
	}

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SCISSORMINXY,
	    ((dsty & 0x0fff) << 16) | (dstx & 0x0fff));

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SCISSORMAXXY,
	    (((dsty + height) & 0x0fff) << 16) | ((dstx + width) & 0x0fff));

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FBSOURCEREAD_BUFFEROFFSET,
	    (((offset_y) & 0xffff) << 16) | ((offset_x) & 0xffff));

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_RECTANGLEPOSITION,
	    (((dsty) & 0xffff) << 16) | ((dstx - x_align) & 0xffff));

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_RENDER2D,
	    dir |
	    PM3_RENDER2D_OPERATION_NORMAL | PM3_RENDER2D_SPANOPERATION | PM3_RENDER2D_FBSOURCEREADENABLE |
	    (((height) & 0x0fff) << 16) | ((width + x_align) & 0x0fff));

#ifdef PM3FB_DEBUG
	pm3fb_flush_engine(sc);
#endif
}

static void
pm3fb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm3fb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			pm3fb_bitblt(sc, x, y, x, y, wi, he, 12);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			pm3fb_bitblt(sc, x, y, x, y, wi, he, 12);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}
}

static void
pm3fb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct pm3fb_softc *sc = scr->scr_cookie;
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
			pm3fb_rectfill(sc, x, y, wi, he, bg);
		} else {
			uc = c - font->firstchar;
			data = (uint8_t *)font->data + uc * ri->ri_fontscale;
			mode = PM3_RM_MASK_MIRROR;

#if BYTE_ORDER == LITTLE_ENDIAN
			switch (ri->ri_font->stride) {
			case 1:
				mode |= 4 << 7; 
				break;
			case 2:
				mode |= 3 << 7;
				break;
			}
#else
			switch (ri->ri_font->stride) {
			case 1:
				mode |= 3 << 7;
				break;
			case 2:
				mode |= 2 << 7;
				break;
			}
#endif
			pm3fb_wait(sc, 8);
			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM3_FOREGROUNDCOLOR, fg);
			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM3_BACKGROUNDCOLOR, bg);

			bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_RASTERIZER_MODE, mode); 

			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM3_CONFIG2D, 
			    PM3_CONFIG2D_USERSCISSOR_ENABLE | 
			    PM3_CONFIG2D_USECONSTANTSOURCE | 
			    PM3_CONFIG2D_FOREGROUNDROP_ENABLE | 
			    PM3_CONFIG2D_FOREGROUNDROP(0x03) | 
			    PM3_CONFIG2D_OPAQUESPAN | 
			    PM3_CONFIG2D_FBWRITE_ENABLE);

			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM3_SCISSORMINXY, ((y & 0x0fff) << 16) | (x & 0x0fff));

			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM3_SCISSORMAXXY, (((y + he) & 0x0fff) << 16) | ((x + wi) & 0x0fff));

			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    PM3_RECTANGLEPOSITION, (((y) & 0xffff)<<16) | ((x) & 0xffff));

			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    PM3_RENDER2D,
			    PM3_RENDER2D_XPOSITIVE |
			    PM3_RENDER2D_YPOSITIVE |
			    PM3_RENDER2D_OPERATION_SYNCONBITMASK |
			    PM3_RENDER2D_SPANOPERATION |
			    ((wi) & 0x0fff) | (((he) & 0x0fff) << 16));

			pm3fb_wait(sc, he);

			switch (ri->ri_font->stride) {
			case 1: {
				uint8_t *data8 = data;
				uint32_t reg;
				for (i = 0; i < he; i++) {
					reg = *data8;
					bus_space_write_4(sc->sc_memt,
					    sc->sc_regh,
					    PM3_BITMASKPATTERN, reg);
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
					    PM3_BITMASKPATTERN, reg);
					data16++;
				}
				break;
			}
			}
		}
	}
}

static void
pm3fb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm3fb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		pm3fb_bitblt(sc, xs, y, xd, y, width, height, 3);
	}
}

static void
pm3fb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm3fb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		pm3fb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
pm3fb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm3fb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight*nrows;
		pm3fb_bitblt(sc, x, ys, x, yd, width, height, 3);
	}
}

static void
pm3fb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct pm3fb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		pm3fb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

/* should be enough */
#define MODE_IS_VALID(m) (((m)->hdisplay < 2048))

static void
pm3_setup_i2c(struct pm3fb_softc *sc)
{
	int i;

	/* Fill in the i2c tag */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = pm3fb_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = pm3fb_i2c_release_bus;
	sc->sc_i2c.ic_send_start = pm3fb_i2c_send_start;
	sc->sc_i2c.ic_send_stop = pm3fb_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = pm3fb_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = pm3fb_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = pm3fb_i2c_write_byte;
	sc->sc_i2c.ic_exec = NULL;

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DISPLAY_DATA, 0);

	/* zero out the EDID buffer */
	memset(sc->sc_edid_data, 0, 128);

	/* Some monitors don't respond first time */
	i = 0;
	while (sc->sc_edid_data[1] == 0 && i < 10) {
		ddc_read_edid(&sc->sc_i2c, sc->sc_edid_data, 128);
		i++;
	}

	if (edid_parse(&sc->sc_edid_data[0], &sc->sc_ei) != -1) {
		/*
		 * Now pick a mode.
		 */
		if ((sc->sc_ei.edid_preferred_mode != NULL)) {
			struct videomode *m = sc->sc_ei.edid_preferred_mode;
			if (MODE_IS_VALID(m)) {
				sc->sc_videomode = m;
			} else {
				aprint_error_dev(sc->sc_dev,
				    "unable to use preferred mode\n");
			}
		}
		/*
		 * if we can't use the preferred mode go look for the
		 * best one we can support
		 */
		if (sc->sc_videomode == NULL) {
			struct videomode *m = sc->sc_ei.edid_modes;

			sort_modes(sc->sc_ei.edid_modes,
			    &sc->sc_ei.edid_preferred_mode,
			    sc->sc_ei.edid_nmodes);
			if (sc->sc_videomode == NULL)
				for (int n = 0; n < sc->sc_ei.edid_nmodes; n++)
					if (MODE_IS_VALID(&m[n])) {
						sc->sc_videomode = &m[n];
						break;
					}
		}
	}
	if (sc->sc_videomode == NULL) {
		/* no EDID data? */
		sc->sc_videomode = pick_mode_by_ref(sc->sc_width,
		    sc->sc_height, 60);
	}
	if (sc->sc_videomode != NULL) {
		pm3fb_set_mode(sc, sc->sc_videomode);
	}
}

/* I2C bitbanging */
static void pm3fb_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct pm3fb_softc *sc = cookie;
	uint32_t out;

	out = bits << 2;	/* bitmasks match the IN bits */

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_DISPLAY_DATA, out);
	delay(100);
}

static void pm3fb_i2cbb_set_dir(void *cookie, uint32_t dir)
{
	/* Nothing to do */
}

static uint32_t pm3fb_i2cbb_read(void *cookie)
{
	struct pm3fb_softc *sc = cookie;
	uint32_t bits;

	bits = bus_space_read_4(sc->sc_memt, sc->sc_regh, PM3_DISPLAY_DATA);
	return bits;
}

/* higher level I2C stuff */
static int
pm3fb_i2c_acquire_bus(void *cookie, int flags)
{
	/* private bus */
	return (0);
}

static void
pm3fb_i2c_release_bus(void *cookie, int flags)
{
	/* private bus */
}

static int
pm3fb_i2c_send_start(void *cookie, int flags)
{

	return (i2c_bitbang_send_start(cookie, flags, &pm3fb_i2cbb_ops));
}

static int
pm3fb_i2c_send_stop(void *cookie, int flags)
{

	return (i2c_bitbang_send_stop(cookie, flags, &pm3fb_i2cbb_ops));
}

static int
pm3fb_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return (i2c_bitbang_initiate_xfer(cookie, addr, flags,
	    &pm3fb_i2cbb_ops));
}

static int
pm3fb_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{

	return (i2c_bitbang_read_byte(cookie, valp, flags, &pm3fb_i2cbb_ops));
}

static int
pm3fb_i2c_write_byte(void *cookie, uint8_t val, int flags)
{
	return (i2c_bitbang_write_byte(cookie, val, flags, &pm3fb_i2cbb_ops));
}

static int
pm3fb_set_pll(struct pm3fb_softc *sc, int freq)
{
	uint8_t bf = 0, bpre = 0, bpost = 0;
	int count;
	unsigned long feedback, prescale, postscale, IntRef, VCO, out_freq, diff,  VCOlow, VCOhigh, bdiff = 1000000;

	freq *= 10; /* convert into 100Hz units */

	for (postscale = 0; postscale <= 5; postscale++) {
		/*
		 * It is pointless going through the main loop if all values of
		 * prescale produce an VCO outside the acceptable range
		 */
		prescale = 1;
		feedback = (prescale * (1UL << postscale) * freq) / (2 * PM3_EXT_CLOCK_FREQ);
		VCOlow = (2 * PM3_EXT_CLOCK_FREQ * feedback) / prescale;
		if (VCOlow > PM3_VCO_FREQ_MAX)
			continue;

		prescale = 255;
		feedback = (prescale * (1UL << postscale) * freq) / (2 * PM3_EXT_CLOCK_FREQ);
		VCOhigh = (2 * PM3_EXT_CLOCK_FREQ * feedback) / prescale;
		if (VCOhigh < PM3_VCO_FREQ_MIN)
			continue;

		for (prescale = 1; prescale <= 255; prescale++) {
			IntRef = PM3_EXT_CLOCK_FREQ / prescale;
			if (IntRef < PM3_INTREF_MIN || IntRef > PM3_INTREF_MAX) {
				if (IntRef > PM3_INTREF_MAX) {
				/*
				 * Hopefully we will get into range as the prescale
				 * value increases
				 */
					continue;
				} else {
					/*
					 * already below minimum and it will only get worse
					 * move to the next postscale value
					 */
					break;
				}
			}

			feedback = (prescale * (1UL << postscale) * freq) / (2 * PM3_EXT_CLOCK_FREQ);

			if (feedback > 255) {
				/*
				 * prescale, feedbackscale & postscale registers
				 * are only 8 bits wide
				 */
				break;
			} else if (feedback == 255) {
				count = 1;
			} else {
				count = 2;
			}

			do {
				VCO = (2 * PM3_EXT_CLOCK_FREQ * feedback) / prescale;
				if (VCO >= PM3_VCO_FREQ_MIN && VCO <= PM3_VCO_FREQ_MAX) {
					out_freq = VCO / (1UL << postscale);
					diff = abs(out_freq - freq);
					if (diff < bdiff) {
						bdiff = diff;
						bf = feedback;
						bpre = prescale;
						bpost = postscale;
						if (diff == 0)
							goto out;
					}
				}
				feedback++;
			} while (--count >= 0);
		}
	}
out:
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_CLOCK0_PRE_SCALE, bpre);
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_CLOCK0_FEEDBACK_SCALE, bf);
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_CLOCK0_POST_SCALE, bpost);
	return 0;
}

static void
pm3fb_set_mode(struct pm3fb_softc *sc, const struct videomode *mode)
{
	int t1, t2, t3, t4, stride;
	uint32_t vclk, tmp1;
	uint8_t sync = 0;

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_BYPASS_MASK, 0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_APERTURE1_CONTROL, 0x00000000);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_APERTURE2_CONTROL, 0x00000000);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FIFODISCONNECT, 0x00000007);

	t1 = mode->hsync_start - mode->hdisplay;
	t2 = mode->vsync_start - mode->vdisplay;
	t3 = mode->hsync_end - mode->hsync_start;
	t4 = mode->vsync_end - mode->vsync_start;
        stride = (mode->hdisplay + 31) & ~31;

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_HORIZ_TOTAL,
	    ((mode->htotal - 1) >> 4));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_HORIZ_SYNC_END,
	    (t1 + t3) >> 4);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_HORIZ_SYNC_START,
	    (t1 >> 4));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_HORIZ_BLANK_END,
	    (mode->htotal - mode->hdisplay) >> 4);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_HORIZ_GATE_END,
	    (mode->htotal - mode->hdisplay) >> 4);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SCREEN_STRIDE,
	    (stride >> 4));

	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_VERT_TOTAL, mode->vtotal - 1);
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_VERT_SYNC_END, t2 + t4 - 1);
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_VERT_SYNC_START, t2 - 1);
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_VERT_BLANK_END, mode->vtotal - mode->vdisplay);

	/*8bpp*/
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_BYAPERTURE1MODE, PM3_BYAPERTUREMODE_PIXELSIZE_8BIT);
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_BYAPERTURE2MODE, PM3_BYAPERTUREMODE_PIXELSIZE_8BIT);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_VIDEO_CONTROL,
	    (PM3_VC_ENABLE | PM3_VC_HSC_ACTIVE_HIGH | PM3_VC_VSC_ACTIVE_HIGH | PM3_VC_PIXELSIZE_8BIT));

	vclk = bus_space_read_4(sc->sc_memt, sc->sc_regh, PM3_V_CLOCK_CTL);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_V_CLOCK_CTL, (vclk & 0xFFFFFFFC));
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_SCREEN_BASE, 0x0);

	tmp1 = bus_space_read_4(sc->sc_memt, sc->sc_regh, PM3_CHIP_CONFIG);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_CHIP_CONFIG, tmp1 & 0xFFFFFFFD);

	pm3fb_set_pll(sc, mode->dot_clock);

	if (mode->flags & VID_PHSYNC)
		sync |= PM3_SC_HSYNC_ACTIVE_HIGH;
	if (mode->flags & VID_PVSYNC)
		sync |= PM3_SC_VSYNC_ACTIVE_HIGH;

	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    PM3_RD_PM3_INDEX_CONTROL, PM3_INCREMENT_DISABLE);
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_SYNC_CONTROL, sync);
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_DAC_CONTROL, 0x00);

	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_PIXEL_SIZE, PM3_DACPS_8BIT);
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_COLOR_FORMAT,
	    (PM3_CF_ORDER_BGR | PM3_CF_VISUAL_256_COLOR));
	pm3fb_write_dac(sc, PM3_RAMDAC_CMD_MISC_CONTROL, PM3_MC_DAC_SIZE_8BIT);

	bus_space_write_4(sc->sc_memt, sc->sc_regh, PM3_FIFOCONTROL, 0x00000905);

	sc->sc_width = mode->hdisplay;
	sc->sc_height = mode->vdisplay;
	sc->sc_depth = 8;
	sc->sc_stride = stride;
	aprint_normal_dev(sc->sc_dev, "pm3 using %d x %d in 8 bit, stride %d\n",
	    sc->sc_width, sc->sc_height, stride);
}
