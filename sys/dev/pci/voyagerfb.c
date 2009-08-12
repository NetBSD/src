/*	$NetBSD: voyagerfb.c,v 1.1 2009/08/12 19:28:00 macallan Exp $	*/

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
 * A console driver for Silicon Motion SM502 / Voyager GX  graphics controllers
 * tested on GDIUM only so far
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: voyagerfb.c,v 1.1 2009/08/12 19:28:00 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <dev/videomode/videomode.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>
#include <dev/ic/sm502reg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/i2c/i2cvar.h>

struct voyagerfb_softc {
	device_t sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;

	bus_space_handle_t sc_fbh;
	bus_space_handle_t sc_regh;
	bus_addr_t sc_fb, sc_reg;
	bus_size_t sc_fbsize, sc_regsize;

	int sc_width, sc_height, sc_depth, sc_stride;
	int sc_locked;
	void *sc_fbaddr;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	uint8_t *sc_dataport;
	int sc_mode;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
};

static int	voyagerfb_match(device_t, cfdata_t, void *);
static void	voyagerfb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(voyagerfb, sizeof(struct voyagerfb_softc),
    voyagerfb_match, voyagerfb_attach, NULL, NULL);

extern const u_char rasops_cmap[768];

static int	voyagerfb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	voyagerfb_mmap(void *, void *, off_t, int);
static void	voyagerfb_init_screen(void *, struct vcons_screen *, int,
		 long *);

static int	voyagerfb_putcmap(struct voyagerfb_softc *,
		 struct wsdisplay_cmap *);
static int 	voyagerfb_getcmap(struct voyagerfb_softc *,
		 struct wsdisplay_cmap *);
static void	voyagerfb_restore_palette(struct voyagerfb_softc *);
static int 	voyagerfb_putpalreg(struct voyagerfb_softc *, int, uint8_t,
			    uint8_t, uint8_t);

static void	voyagerfb_init(struct voyagerfb_softc *);

static void	voyagerfb_rectfill(struct voyagerfb_softc *, int, int, int, int,
			    uint32_t);
static void	voyagerfb_bitblt(struct voyagerfb_softc *, int, int, int, int,
			    int, int, int);

static void	voyagerfb_cursor(void *, int, int, int);
static void	voyagerfb_putchar(void *, int, int, u_int, long);
static void	voyagerfb_copycols(void *, int, int, int, int);
static void	voyagerfb_erasecols(void *, int, int, int, long);
static void	voyagerfb_copyrows(void *, int, int, int);
static void	voyagerfb_eraserows(void *, int, int, long);

struct wsdisplay_accessops voyagerfb_accessops = {
	voyagerfb_ioctl,
	voyagerfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

/* wait for FIFO empty so we can feed it another command */
static inline void
voyagerfb_ready(struct voyagerfb_softc *sc)
{
	do {} while ((bus_space_read_4(sc->sc_memt, sc->sc_regh, 
	    SM502_SYSTEM_CTRL) & SM502_SYSCTL_FIFO_EMPTY) == 0);
}

/* wait for the drawing engine to be idle */
static inline void
voyagerfb_wait(struct voyagerfb_softc *sc)
{
	do {} while ((bus_space_read_4(sc->sc_memt, sc->sc_regh, 
	    SM502_SYSTEM_CTRL) & SM502_SYSCTL_ENGINE_BUSY) != 0);
}

static int
voyagerfb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_SILMOTION)
		return 0;

	/* only chip tested on so far - may need a list */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SILMOTION_SM502)
		return 100;
	return (0);
}

static void
voyagerfb_attach(device_t parent, device_t self, void *aux)
{
	struct voyagerfb_softc	*sc = device_private(self);
	struct pci_attach_args	*pa = aux;
	struct rasops_info	*ri;
	char devinfo[256];
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t	dict;
	unsigned long		defattr;
	uint32_t		reg;
	bool			is_console;
	int i, j;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;
	sc->sc_dev = self;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s\n", devinfo);

	dict = device_properties(self);
	prop_dictionary_get_bool(dict, "is_console", &is_console);

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_LINEAR,
	    &sc->sc_memt, &sc->sc_fbh, &sc->sc_fb, &sc->sc_fbsize)) {
		aprint_error("%s: failed to map the frame buffer.\n",
		    device_xname(sc->sc_dev));
	}
	sc->sc_fbaddr = bus_space_vaddr(sc->sc_memt, sc->sc_fbh);

	if (pci_mapreg_map(pa, 0x14, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_memt, &sc->sc_regh, &sc->sc_reg, &sc->sc_regsize)) {
		aprint_error("%s: failed to map registers.\n",
		    device_xname(sc->sc_dev));
	}
	sc->sc_dataport = bus_space_vaddr(sc->sc_memt, sc->sc_regh);
	sc->sc_dataport += SM502_DATAPORT;

	reg = bus_space_read_8(sc->sc_memt, sc->sc_regh, SM502_PANEL_DISP_CRTL);
	switch (reg & SM502_PDC_DEPTH_MASK) {
		case SM502_PDC_8BIT:
			sc->sc_depth = 8;
			break;
		case SM502_PDC_16BIT:
			sc->sc_depth = 16;
			break;
		case SM502_PDC_32BIT:
			sc->sc_depth = 24;
			break;
		default:
			panic("%s: unsupported depth", device_xname(self));
	}
	sc->sc_stride = (bus_space_read_4(sc->sc_memt, sc->sc_regh, 	
		SM502_PANEL_FB_OFFSET) & SM502_FBA_WIN_STRIDE_MASK) >> 16;
	sc->sc_width = (bus_space_read_4(sc->sc_memt, sc->sc_regh, 	
		SM502_PANEL_FB_WIDTH) & SM502_FBW_WIN_WIDTH_MASK) >> 16;
	sc->sc_height = (bus_space_read_4(sc->sc_memt, sc->sc_regh, 	
		SM502_PANEL_FB_HEIGHT) & SM502_FBH_WIN_HEIGHT_MASK) >> 16;

	printf("%s: %d x %d, %d bit, stride %d\n", device_xname(self), 
		sc->sc_width, sc->sc_height, sc->sc_depth, sc->sc_stride);
	/*
	 * XXX yeah, casting the fb address to uint32_t is formally wrong
	 * but as far as I know there are no SM502 with 64bit BARs
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

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &voyagerfb_accessops);
	sc->vd.init_screen = voyagerfb_init_screen;

	/* init engine here */
	voyagerfb_init(sc);

	ri = &sc->sc_console_screen.scr_ri;

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}

	j = 0;
	if (sc->sc_depth <= 8) {
		for (i = 0; i < (1 << sc->sc_depth); i++) {

			sc->sc_cmap_red[i] = rasops_cmap[j];
			sc->sc_cmap_green[i] = rasops_cmap[j + 1];
			sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
			voyagerfb_putpalreg(sc, i, rasops_cmap[j],
			    rasops_cmap[j + 1], rasops_cmap[j + 2]);
			j += 3;
		}
	}

	voyagerfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
	    ri->ri_devcmap[(defattr >> 16) & 0xff]);

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &voyagerfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
voyagerfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct voyagerfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {

		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
			return 0;

		/* PCI config read/write passthrough. */
		case PCI_IOC_CFGREAD:
		case PCI_IOC_CFGWRITE:
			return (pci_devioctl(sc->sc_pc, sc->sc_pcitag,
			    cmd, data, flag, l));

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
			return voyagerfb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return voyagerfb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->sc_stride;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;

				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if(new_mode == WSDISPLAYIO_MODE_EMUL) {
						voyagerfb_restore_palette(sc);
						vcons_redraw_screen(ms);
					}
				}
			}
			return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
voyagerfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct voyagerfb_softc *sc = vd->cookie;
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
	if (kauth_authorize_generic(kauth_cred_get(), KAUTH_GENERIC_ISSUSER,
	    NULL) != 0) {
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

	return -1;
}

static void
voyagerfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct voyagerfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = (char *)sc->sc_fbaddr;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	ri->ri_ops.copyrows = voyagerfb_copyrows;
	ri->ri_ops.copycols = voyagerfb_copycols;
	ri->ri_ops.eraserows = voyagerfb_eraserows;
	ri->ri_ops.erasecols = voyagerfb_erasecols;
	ri->ri_ops.cursor = voyagerfb_cursor;
	ri->ri_ops.putchar = voyagerfb_putchar;
}

static int
voyagerfb_putcmap(struct voyagerfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef VOYAGERFB_DEBUG
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
		voyagerfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
voyagerfb_getcmap(struct voyagerfb_softc *sc, struct wsdisplay_cmap *cm)
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
voyagerfb_restore_palette(struct voyagerfb_softc *sc)
{
	int i;

	for (i = 0; i < (1 << sc->sc_depth); i++) {
		voyagerfb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static int
voyagerfb_putpalreg(struct voyagerfb_softc *sc, int idx, uint8_t r,
    uint8_t g, uint8_t b)
{
	uint32_t reg;

	reg = (r << 16) | (g << 8) | b;
	/* XXX we should probably write the CRT palette too */
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    SM502_PALETTE_PANEL + (idx << 2), reg);
	return 0;
}

static void
voyagerfb_init(struct voyagerfb_softc *sc)
{

	voyagerfb_wait(sc);
	/* disable colour compare */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_COLOR_COMP_MASK, 0);
	/* allow writes to all planes */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_PLANEMASK,
	    0xffffffff);
	/* disable clipping */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_CLIP_TOP_LEFT, 0);
	/* source and destination in local memory, no offset */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_SRC_BASE, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_DST_BASE, 0);
	/* pitch is screen stride */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_PITCH,
	    sc->sc_width | (sc->sc_width << 16));
	/* window is screen width */
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_WINDOW_WIDTH,
	    sc->sc_width | (sc->sc_width << 16));
	switch (sc->sc_depth) {
		case 8:
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    SM502_STRETCH, SM502_STRETCH_8BIT);
			break;
		case 16:
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    SM502_STRETCH, SM502_STRETCH_16BIT);
			break;
		case 24:
		case 32:
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    SM502_STRETCH, SM502_STRETCH_32BIT);
			break;
	}
}

static void
voyagerfb_rectfill(struct voyagerfb_softc *sc, int x, int y, int wi, int he,
     uint32_t colour)
{

	voyagerfb_ready(sc);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_CONTROL,
	    ROP_COPY |
	    SM502_CTRL_USE_ROP2 |
	    SM502_CTRL_CMD_RECTFILL |
	    SM502_CTRL_QUICKSTART_E);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_FOREGROUND,
	    colour);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_DST,
	    (x << 16) | y);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_DIMENSION,
	    (wi << 16) | he);
}

static void
voyagerfb_bitblt(struct voyagerfb_softc *sc, int xs, int ys, int xd, int yd,
    int wi, int he, int rop)
{
	uint32_t cmd;

	cmd = (rop & 0xf) | SM502_CTRL_USE_ROP2 | SM502_CTRL_CMD_BITBLT |
	      SM502_CTRL_QUICKSTART_E;

	voyagerfb_ready(sc);

	if (xd <= xs) {
		/* left to right */
	} else {
		cmd |= SM502_CTRL_R_TO_L;
		xs += he - 1;
		xd += wi - 1;
	}
	
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_CONTROL, cmd);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_SRC,
	    (xs << 16) | ys);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_DST,
	    (xd << 16) | yd);
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_DIMENSION,
	    (wi << 16) | he);
}

static void
voyagerfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voyagerfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			voyagerfb_bitblt(sc, x, y, x, y, wi, he, ROP_INVERT);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			voyagerfb_bitblt(sc, x, y, x, y, wi, he, ROP_INVERT);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}

}

static inline void
voyagerfb_feed8(struct voyagerfb_softc *sc, uint8_t *data, int len)
{
	uint32_t *port = (uint32_t *)sc->sc_dataport;
	int i;

	for (i = 0; i < ((len + 3) & 0xfffc); i++) {
		*port = *data;
		data++;
	}
}

static inline void
voyagerfb_feed16(struct voyagerfb_softc *sc, uint16_t *data, int len)
{
	uint32_t *port = (uint32_t *)sc->sc_dataport;
	int i;

	len = len << 1;
	for (i = 0; i < ((len + 1) & 0xfffe); i++) {
		*port = *data;
		data++;
	}
}


static void
voyagerfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voyagerfb_softc *sc = scr->scr_cookie;
	uint32_t cmd;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		int fg, bg, uc;
		uint8_t *data;
		int x, y, wi, he;
		wi = ri->ri_font->fontwidth;
		he = ri->ri_font->fontheight;

		if (!CHAR_IN_FONT(c, ri->ri_font))
			return;
		bg = ri->ri_devcmap[(attr >> 16) & 0x0f];
		fg = ri->ri_devcmap[(attr >> 24) & 0x0f];
		x = ri->ri_xorigin + col * wi;
		y = ri->ri_yorigin + row * he;
		if (c == 0x20) {
			voyagerfb_rectfill(sc, x, y, wi, he, bg);
		} else {
			uc = c - ri->ri_font->firstchar;
			data = (uint8_t *)ri->ri_font->data + uc * 
			    ri->ri_fontscale;
			cmd = ROP_COPY |
			      SM502_CTRL_USE_ROP2 |
			      SM502_CTRL_CMD_HOSTWRT |
			      SM502_CTRL_HOSTBLT_MONO |
			      SM502_CTRL_QUICKSTART_E | 
			      SM502_CTRL_MONO_PACK_32BIT;
			voyagerfb_ready(sc);
			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    SM502_CONTROL, cmd);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    SM502_FOREGROUND, fg);
			bus_space_write_4(sc->sc_memt, sc->sc_regh, 
			    SM502_BACKGROUND, bg);
			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    SM502_SRC, 0);
			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    SM502_DST, (x << 16) | y);
			bus_space_write_4(sc->sc_memt, sc->sc_regh,
			    SM502_DIMENSION, (wi << 16) | he);
			/* now feed the data, padded to 32bit */
			switch (ri->ri_font->stride) {
			case 1:
				voyagerfb_feed8(sc, data, ri->ri_fontscale);
				break;
			case 2:
				voyagerfb_feed16(sc, (uint16_t *)data, 
				    ri->ri_fontscale);
				break;
			
			}	
		}
	}
}

static void
voyagerfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voyagerfb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		voyagerfb_bitblt(sc, xs, y, xd, y, width, height, ROP_COPY);
	}
}

static void
voyagerfb_erasecols(void *cookie, int row, int startcol, int ncols,
     long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voyagerfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		voyagerfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
voyagerfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voyagerfb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;
	int i;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		if ((nrows > 1) && (dstrow > srcrow)) {
			/*
			 * the blitter can't do bottom-up copies so we have
			 * to copy line by line here
			 * should probably use a command sequence
			 */
			for (i = 0; i < nrows; i++) {
				voyagerfb_bitblt(sc, x, ys, x, yd, width, 
				    ri->ri_font->fontheight, ROP_COPY);
				ys += ri->ri_font->fontheight;
				yd += ri->ri_font->fontheight;
			}
		} else
			voyagerfb_bitblt(sc, x, ys, x, yd, width, height, 
			    ROP_COPY);
	}
}

static void
voyagerfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voyagerfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		voyagerfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}
