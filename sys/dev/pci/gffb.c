/*	$NetBSD: gffb.c,v 1.1 2013/09/18 14:30:45 macallan Exp $	*/

/*
 * Copyright (c) 2007, 2012 Michael Lorenz
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
 * A console driver for nvidia geforce graphics controllers
 * tested on macppc only so far
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gffb.c,v 1.1 2013/09/18 14:30:45 macallan Exp $");

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
#include <dev/pci/gffbreg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/pci/wsdisplay_pci.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#include <dev/i2c/i2cvar.h>

#include "opt_gffb.h"
#include "opt_vcons.h"

#ifdef GFFB_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while(0) printf
#endif

struct gffb_softc {
	device_t sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;

	bus_space_handle_t sc_regh, sc_fbh;
	bus_addr_t sc_fb, sc_reg;
	bus_size_t sc_fbsize, sc_regsize;
	uint8_t *sc_fbaddr;

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
	glyphcache sc_gc;
};

static int	gffb_match(device_t, cfdata_t, void *);
static void	gffb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gffb, sizeof(struct gffb_softc),
    gffb_match, gffb_attach, NULL, NULL);

static int	gffb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	gffb_mmap(void *, void *, off_t, int);
static void	gffb_init_screen(void *, struct vcons_screen *, int, long *);

static int	gffb_putcmap(struct gffb_softc *, struct wsdisplay_cmap *);
static int 	gffb_getcmap(struct gffb_softc *, struct wsdisplay_cmap *);
static void	gffb_restore_palette(struct gffb_softc *);
static int 	gffb_putpalreg(struct gffb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);

static void	gffb_init(struct gffb_softc *);

#if notyet
static void	gffb_flush_engine(struct gffb_softc *);
static void	gffb_rectfill(struct gffb_softc *, int, int, int, int,
			    uint32_t);
static void	gffb_bitblt(void *, int, int, int, int, int,
			    int, int);

static void	gffb_cursor(void *, int, int, int);
static void	gffb_putchar(void *, int, int, u_int, long);
static void	gffb_putchar_aa(void *, int, int, u_int, long);
static void	gffb_copycols(void *, int, int, int, int);
static void	gffb_erasecols(void *, int, int, int, long);
static void	gffb_copyrows(void *, int, int, int);
static void	gffb_eraserows(void *, int, int, long);
#endif /* notyet */

struct wsdisplay_accessops gffb_accessops = {
	gffb_ioctl,
	gffb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

static int
gffb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NVIDIA)
		return 0;

	/* only card tested on so far - likely need a list */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NVIDIA_GEFORCE2MX)
		return 100;
	return (0);
}

static void
gffb_attach(device_t parent, device_t self, void *aux)
{
	struct gffb_softc	*sc = device_private(self);
	struct pci_attach_args	*pa = aux;
	struct rasops_info	*ri;
	bus_space_tag_t		tag;
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t	dict;
	unsigned long		defattr;
	bool			is_console;
	int			i, j;
	uint8_t			cmap[768];

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

#ifdef GLYPHCACHE_DEBUG
	/* leave some visible VRAM unused so we can see the glyph cache */
	sc->sc_height -= 200;
#endif

	if (!prop_dictionary_get_uint32(dict, "depth", &sc->sc_depth)) {
		aprint_error("%s: no depth property\n", device_xname(self));
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "linebytes", &sc->sc_stride)) {
		aprint_error("%s: no linebytes property\n",
		    device_xname(self));
		return;
	}

	prop_dictionary_get_bool(dict, "is_console", &is_console);

	if (pci_mapreg_map(pa, 0x14, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_PREFETCHABLE | BUS_SPACE_MAP_LINEAR,
	    &tag, &sc->sc_fbh, &sc->sc_fb, &sc->sc_fbsize)) {
		aprint_error("%s: failed to map the framebuffer.\n",
		    device_xname(sc->sc_dev));
	}
	sc->sc_fbaddr = bus_space_vaddr(tag, sc->sc_fbh);

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM, 0,
	    &tag, &sc->sc_regh, &sc->sc_reg, &sc->sc_regsize)) {
		aprint_error("%s: failed to map registers.\n",
		    device_xname(sc->sc_dev));
	}

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
	    &gffb_accessops);
	sc->vd.init_screen = gffb_init_screen;

	/* init engine here */
	gffb_init(sc);

	ri = &sc->sc_console_screen.scr_ri;

#if notyet
	sc->sc_gc.gc_bitblt = gffb_bitblt;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = R128_ROP3_S;
#endif

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

#if notyet
		gffb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);
#else
		memset(sc->sc_fbaddr,
		    ri->ri_devcmap[(defattr >> 16) & 0xff],
		    sc->sc_height * sc->sc_stride);
#endif
		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
#if notyet
		glyphcache_init(&sc->sc_gc, sc->sc_height + 5,
				(0x800000 / sc->sc_stride) - sc->sc_height - 5,
				sc->sc_width,
				ri->ri_font->fontwidth,
				ri->ri_font->fontheight,
				defattr);
#endif
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		if (sc->sc_console_screen.scr_ri.ri_rows == 0) {
			/* do some minimal setup to avoid weirdnesses later */
			vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
			    &defattr);
		} else
			(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
#if notyet
		glyphcache_init(&sc->sc_gc, sc->sc_height + 5,
				(0x800000 / sc->sc_stride) - sc->sc_height - 5,
				sc->sc_width,
				ri->ri_font->fontwidth,
				ri->ri_font->fontheight,
				defattr);
#endif
	}

	j = 0;
	rasops_get_cmap(ri, cmap, sizeof(cmap));
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = cmap[j];
		sc->sc_cmap_green[i] = cmap[j + 1];
		sc->sc_cmap_blue[i] = cmap[j + 2];
		gffb_putpalreg(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
		j += 3;
	}

	/* no suspend/resume support yet */
	pmf_device_register(sc->sc_dev, NULL, NULL);

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &gffb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
gffb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct gffb_softc *sc = vd->cookie;
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
		return gffb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return gffb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;
		return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if(new_mode == WSDISPLAYIO_MODE_EMUL) {
				gffb_init(sc);
				gffb_restore_palette(sc);
#if notyet
				glyphcache_wipe(&sc->sc_gc);
				gffb_rectfill(sc, 0, 0, sc->sc_width,
				    sc->sc_height, ms->scr_ri.ri_devcmap[
				    (ms->scr_defattr >> 16) & 0xff]);
#endif
				vcons_redraw_screen(ms);
			}
		}
		}
		return 0;
#if notyet
	case WSDISPLAYIO_GET_EDID: {
		struct wsdisplayio_edid_info *d = data;
		return wsdisplayio_get_edid(sc->sc_dev, d);
	}
#endif
	}
	return EPASSTHROUGH;
}

static paddr_t
gffb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct gffb_softc *sc = vd->cookie;
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

#ifdef OFB_ALLOW_OTHERS
	if (offset >= 0x80000000) {
		pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR);
		return pa;
	}
#endif
	return -1;
}

static void
gffb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct gffb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_bits = sc->sc_fbaddr;
	ri->ri_flg = RI_CENTER;
	if (sc->sc_depth == 8)
		ri->ri_flg |= RI_8BIT_IS_RGB | RI_ENABLE_ALPHA;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	scr->scr_flags |= VCONS_DONT_READ;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
#if notyet
	ri->ri_ops.copyrows = gffb_copyrows;
	ri->ri_ops.copycols = gffb_copycols;
	ri->ri_ops.eraserows = gffb_eraserows;
	ri->ri_ops.erasecols = gffb_erasecols;
	ri->ri_ops.cursor = gffb_cursor;
	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = gffb_putchar_aa;
	} else
		ri->ri_ops.putchar = gffb_putchar;
#endif
}

static int
gffb_putcmap(struct gffb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef R128FB_DEBUG
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
		gffb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
gffb_getcmap(struct gffb_softc *sc, struct wsdisplay_cmap *cm)
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
gffb_restore_palette(struct gffb_softc *sc)
{
	int i;

	for (i = 0; i < (1 << sc->sc_depth); i++) {
		gffb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static int
gffb_putpalreg(struct gffb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{
	/* port 0 */
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO0 + GFFB_PEL_IW, idx);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO0 + GFFB_PEL_D, r);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO0 + GFFB_PEL_D, g);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO0 + GFFB_PEL_D, b);

	/* port 1 */
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO1 + GFFB_PEL_IW, idx);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO1 + GFFB_PEL_D, r);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO1 + GFFB_PEL_D, g);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO1 + GFFB_PEL_D, b);
	
	return 0;
}

static void
gffb_init(struct gffb_softc *sc)
{
#ifdef GFFB_DEBUG
	int i, j;

	/* dump the RAMDAC1 register space */
	for (i = 0; i < 0x2000; i+= 32) {
		printf("%04x:", i);
		for (j = 0; j < 32; j += 4) {
			printf(" %08x", bus_space_read_4(sc->sc_memt, sc->sc_regh, GFFB_RAMDAC1 + i + j));
		}
		printf("\n");
	}
#endif

	/* init display start */
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    GFFB_CRTC0 + GFFB_DISPLAYSTART, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_regh,
	    GFFB_CRTC1 + GFFB_DISPLAYSTART, 0);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO0 + GFFB_PEL_MASK, 0xff);
	bus_space_write_1(sc->sc_memt, sc->sc_regh,
	    GFFB_PDIO1 + GFFB_PEL_MASK, 0xff);
}
