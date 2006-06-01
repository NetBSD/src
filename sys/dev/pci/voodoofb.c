/*	$NetBSD: voodoofb.c,v 1.4.4.3 2006/06/01 22:36:49 kardel Exp $	*/

/*
 * Copyright (c) 2005, 2006 Michael Lorenz
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * A console driver for 3Dfx Voodoo3 graphics boards 
 * Thanks to Andreas Drewke (andreas_dr@gmx.de) for his Voodoo3 driver for BeOS 
 * which I used as reference / documentation
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: voodoofb.c,v 1.4.4.3 2006/06/01 22:36:49 kardel Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/callout.h>

#include <uvm/uvm_extern.h>

#if defined(macppc) || defined (sparc64) || defined(ofppc)
#define HAVE_OPENFIRMWARE
#endif

#ifdef HAVE_OPENFIRMWARE
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>
#endif

#include <dev/videomode/videomode.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>
#include <dev/pci/voodoofbreg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include "opt_wsemul.h"

struct voodoofb_softc {
	struct device sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_memh;	

	bus_space_tag_t sc_regt;
	bus_space_tag_t sc_fbt;
	bus_space_tag_t sc_ioregt;
	bus_space_handle_t sc_regh;
	bus_space_handle_t sc_fbh;	
	bus_space_handle_t sc_ioregh;	
	bus_addr_t sc_regs, sc_fb, sc_ioreg;
	bus_size_t sc_regsize, sc_fbsize, sc_ioregsize;

	void *sc_ih;
	
	size_t memsize;
	int memtype;

	int bits_per_pixel;
	int width, height, linebytes;

	int sc_mode;
	uint32_t sc_bg;
		
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];	
	int sc_dacw;

	struct vcons_data vd;
};

struct voodoo_regs {
	uint8_t vr_crtc[31];
	uint8_t vr_graph[9];
	uint8_t vr_attr[21];
	uint8_t vr_seq[5];
};
	
static struct vcons_screen voodoofb_console_screen;

extern const u_char rasops_cmap[768];

static int	voodoofb_match(struct device *, struct cfdata *, void *);
static void	voodoofb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(voodoofb, sizeof(struct voodoofb_softc), voodoofb_match, 
    voodoofb_attach, NULL, NULL);

static int	voodoofb_is_console(struct pci_attach_args *);
static void 	voodoofb_init(struct voodoofb_softc *);	

static void	voodoofb_cursor(void *, int, int, int);
static void	voodoofb_putchar(void *, int, int, u_int, long);
static void	voodoofb_copycols(void *, int, int, int, int);
static void	voodoofb_erasecols(void *, int, int, int, long);
static void	voodoofb_copyrows(void *, int, int, int);
static void	voodoofb_eraserows(void *, int, int, long);

#if 0
static int	voodoofb_allocattr(void *, int, int, int, long *);
static void	voodoofb_scroll(void *, void *, int);
static int	voodoofb_load_font(void *, void *, struct wsdisplay_font *);
#endif

static int	voodoofb_putcmap(struct voodoofb_softc *,
			    struct wsdisplay_cmap *);
static int 	voodoofb_getcmap(struct voodoofb_softc *,
			    struct wsdisplay_cmap *);
static int 	voodoofb_putpalreg(struct voodoofb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);
static void	voodoofb_bitblt(struct voodoofb_softc *, int, int, int, int,
			    int, int);
static void	voodoofb_rectfill(struct voodoofb_softc *, int, int, int, int,
			    int);
static void	voodoofb_rectinvert(struct voodoofb_softc *, int, int, int,
			    int); 
static void	voodoofb_setup_mono(struct voodoofb_softc *, int, int, int,
			    int, uint32_t, uint32_t); 
static void	voodoofb_feed_line(struct voodoofb_softc *, int, uint8_t *);

#ifdef VOODOOFB_DEBUG
static void	voodoofb_showpal(struct voodoofb_softc *);
#endif

static void	voodoofb_wait_idle(struct voodoofb_softc *);

static int	voodoofb_intr(void *);

static void	voodoofb_set_videomode(struct voodoofb_softc *,
			    const struct videomode *);

struct wsscreen_descr voodoofb_defaultscreen = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
};

const struct wsscreen_descr *_voodoofb_scrlist[] = {
	&voodoofb_defaultscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list voodoofb_screenlist = {
	sizeof(_voodoofb_scrlist) / sizeof(struct wsscreen_descr *), _voodoofb_scrlist
};

static int	voodoofb_ioctl(void *, void *, u_long, caddr_t, int,
		    struct lwp *);
static paddr_t	voodoofb_mmap(void *, void *, off_t, int);

static void	voodoofb_clearscreen(struct voodoofb_softc *);
static void	voodoofb_init_screen(void *, struct vcons_screen *, int,
			    long *);


struct wsdisplay_accessops voodoofb_accessops = {
	voodoofb_ioctl,
	voodoofb_mmap,
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

/*
 * Inline functions for getting access to register aperture.
 */
static inline void
voodoo3_write32(struct voodoofb_softc *sc, uint32_t reg, uint32_t val)
{
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, val);
}

static inline uint32_t
voodoo3_read32(struct voodoofb_softc *sc, uint32_t reg) 
{
	return bus_space_read_4(sc->sc_regt, sc->sc_regh, reg);
}

static inline void
voodoo3_write_crtc(struct voodoofb_softc *sc, uint8_t reg, uint8_t val)
{
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, CRTC_INDEX - 0x300, reg);
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, CRTC_DATA - 0x300, val);
}

static inline void
voodoo3_write_seq(struct voodoofb_softc *sc, uint8_t reg, uint8_t val)
{
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, SEQ_INDEX - 0x300, reg);
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, SEQ_DATA - 0x300, val);
}

static inline void
voodoo3_write_gra(struct voodoofb_softc *sc, uint8_t reg, uint8_t val)
{
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, GRA_INDEX - 0x300, reg);
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, GRA_DATA - 0x300, val);
}

static inline void
voodoo3_write_attr(struct voodoofb_softc *sc, uint8_t reg, uint8_t val)
{
	volatile uint8_t junk;
	uint8_t index;
	
	junk = bus_space_read_1(sc->sc_ioregt, sc->sc_ioregh, IS1_R - 0x300);
	index = bus_space_read_1(sc->sc_ioregt, sc->sc_ioregh, ATT_IW - 0x300);
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, ATT_IW - 0x300, reg);
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, ATT_IW - 0x300, val);
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, ATT_IW - 0x300, index);
}

static inline void
vga_outb(struct voodoofb_softc *sc, uint32_t reg,  uint8_t val)
{ 
	bus_space_write_1(sc->sc_ioregt, sc->sc_ioregh, reg - 0x300, val); 
}

/* wait until there's room for len bytes in the FIFO */
static inline void
voodoo3_make_room(struct voodoofb_softc *sc, int len) 
{
	while ((voodoo3_read32(sc, STATUS) & 0x1f) < len);
}

static void
voodoofb_wait_idle(struct voodoofb_softc *sc)
{
	int i = 0;

	voodoo3_make_room(sc, 1);
	voodoo3_write32(sc, COMMAND_3D, COMMAND_3D_NOP);

	while (1) {
		i = (voodoo3_read32(sc, STATUS) & STATUS_BUSY) ? 0 : i + 1;
		if(i == 3) break;
	}
}

static int
voodoofb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_DISPLAY_VGA)
		return 0;
	if ((PCI_VENDOR(pa->pa_id)==PCI_VENDOR_3DFX) && 
	    (PCI_PRODUCT(pa->pa_id)>=PCI_PRODUCT_3DFX_VOODOO3))
		return 100;
	return 0;
}

static void
voodoofb_attach(struct device *parent, struct device *self, void *aux)
{
	struct voodoofb_softc *sc = (void *)self;	
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	pci_intr_handle_t ih;
	ulong defattr;
	const char *intrstr;
	int console, width, height, node, i, j;
#ifdef HAVE_OPENFIRMWARE
	int linebytes, depth;
#endif
	uint32_t bg, fg, ul;
		
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_dacw = -1;
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;

	/* the framebuffer */
	if (pci_mapreg_map(pa, 0x14, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_CACHEABLE | BUS_SPACE_MAP_PREFETCHABLE | 
	    BUS_SPACE_MAP_LINEAR, 
	    &sc->sc_fbt, &sc->sc_fbh, &sc->sc_fb, &sc->sc_fbsize)) {
		printf("%s: failed to map the frame buffer.\n", 
		    sc->sc_dev.dv_xname);
	}

	/* memory-mapped registers */
	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_regt, &sc->sc_regh, &sc->sc_regs, &sc->sc_regsize)) {
		printf("%s: failed to map memory-mapped registers.\n", 
		    sc->sc_dev.dv_xname);
	}

	/* IO-mapped registers */
	if (pci_mapreg_map(pa, 0x18, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_ioregt, &sc->sc_ioregh, &sc->sc_ioreg,
	    &sc->sc_ioregsize)) {
		printf("%s: failed to map IO-mapped registers.\n", 
		    sc->sc_dev.dv_xname);
	}
	voodoofb_init(sc);
	
	/* we should read these from the chip instead of depending on OF */
	width = height = -1;
	
#ifdef HAVE_OPENFIRMWARE
	if (OF_getprop(node, "width", &width, 4) != 4)
		OF_interpret("screen-width", 1, &width);
	if (OF_getprop(node, "height", &height, 4) != 4)
		OF_interpret("screen-height", 1, &height);
	if (OF_getprop(node, "linebytes", &linebytes, 4) != 4)
		linebytes = width;			/* XXX */
	if (OF_getprop(node, "depth", &depth, 4) != 4)
		depth = 8;				/* XXX */

	if (width == -1 || height == -1)
		return;

	sc->width = width;
	sc->height = height;
	sc->bits_per_pixel = depth;
	sc->linebytes = linebytes;
	printf("%s: initial resolution %dx%d, %d bit\n", sc->sc_dev.dv_xname,
	    sc->width, sc->height, sc->bits_per_pixel);
#endif

	/* XXX this should at least be configurable via kernel config */
	voodoofb_set_videomode(sc, &videomode_list[16]);

	vcons_init(&sc->vd, sc, &voodoofb_defaultscreen, &voodoofb_accessops);
	sc->vd.init_screen = voodoofb_init_screen;

	console = voodoofb_is_console(pa);

	ri = &voodoofb_console_screen.scr_ri;
	if (console) {
		vcons_init_screen(&sc->vd, &voodoofb_console_screen, 1,
		    &defattr);
		voodoofb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		voodoofb_defaultscreen.textops = &ri->ri_ops;
		voodoofb_defaultscreen.capabilities = ri->ri_caps;
		voodoofb_defaultscreen.nrows = ri->ri_rows;
		voodoofb_defaultscreen.ncols = ri->ri_cols;
		wsdisplay_cnattach(&voodoofb_defaultscreen, ri, 0, 0, defattr);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		voodoofb_set_videomode(sc, &videomode_list[0]);		 
	}

	printf("%s: %d MB aperture at 0x%08x, %d MB registers at 0x%08x\n",
	    sc->sc_dev.dv_xname, (u_int)(sc->sc_fbsize >> 20),
	    (u_int)sc->sc_fb, (u_int)(sc->sc_regsize >> 20), 
	    (u_int)sc->sc_regs);
#ifdef VOODOOFB_DEBUG
	printf("fb: %08lx\n", (ulong)ri->ri_bits);
#endif
	
	j = 0;
	for (i = 0; i < 256; i++) {
		voodoofb_putpalreg(sc, i, rasops_cmap[j], rasops_cmap[j + 1], 
		    rasops_cmap[j + 2]);
		j += 3;
	}

	/* Interrupt. We don't use it for anything yet */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: failed to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}

	intrstr = pci_intr_string(sc->sc_pc, ih);
	sc->sc_ih = pci_intr_establish(sc->sc_pc, ih, IPL_NET, voodoofb_intr, 
	    sc);
	if (sc->sc_ih == NULL) {
		printf("%s: failed to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	rasops_unpack_attr(defattr, &fg, &bg, &ul);
	sc->sc_bg = ri->ri_devcmap[bg];
	voodoofb_clearscreen(sc);

	aa.console = console;
	aa.scrdata = &voodoofb_screenlist;
	aa.accessops = &voodoofb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);
}

static int
voodoofb_putpalreg(struct voodoofb_softc *sc, uint8_t index, uint8_t r, 
    uint8_t g, uint8_t b)
{
	uint32_t color;
	
	sc->sc_cmap_red[index] = r;
	sc->sc_cmap_green[index] = g;
	sc->sc_cmap_blue[index] = b;

	color = (r << 16) | (g << 8) | b;
	voodoo3_make_room(sc, 2);
	voodoo3_write32(sc, DACADDR, index);
	voodoo3_write32(sc, DACDATA, color);

	return 0;
}

static int
voodoofb_putcmap(struct voodoofb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef VOODOOFB_DEBUG
	printf("putcmap: %d %d\n",index, count);
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
		voodoofb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
voodoofb_getcmap(struct voodoofb_softc *sc, struct wsdisplay_cmap *cm)
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

static int
voodoofb_is_console(struct pci_attach_args *pa)
{

#ifdef HAVE_OPENFIRMWARE
	/* check if we're the /chosen console device */
	int chosen, stdout, node, us;
	
	us=pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdout", &stdout, 4);
	node = OF_instance_to_package(stdout);
	return(us == node);
#else
	/* XXX how do we know we're console on i386? */
	return 1;
#endif
}

static void
voodoofb_clearscreen(struct voodoofb_softc *sc)
{
	voodoofb_rectfill(sc, 0, 0, sc->width, sc->height, sc->sc_bg);
}

/*
 * wsdisplay_emulops
 */

static void
voodoofb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voodoofb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			voodoofb_rectinvert(sc, x, y, wi, he);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on)
		{
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			voodoofb_rectinvert(sc, x, y, wi, he);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		ri->ri_flg &= ~RI_CURSOR;
		ri->ri_crow = row;
		ri->ri_ccol = col;
	}
}

#if 0
int
voodoofb_mapchar(void *cookie, int uni, u_int *index)
{
	return 0;
}
#endif

static void
voodoofb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voodoofb_softc *sc = scr->scr_cookie;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		uint8_t *data;
		int fg, bg, uc, i;
		int x, y, wi, he;

		wi = ri->ri_font->fontwidth;
		he = ri->ri_font->fontheight;

		if (!CHAR_IN_FONT(c, ri->ri_font))
			return;
		bg = (u_char)ri->ri_devcmap[(attr >> 16) & 0xf];
		fg = (u_char)ri->ri_devcmap[(attr >> 24) & 0xf];
		x = ri->ri_xorigin + col * wi;
		y = ri->ri_yorigin + row * he;
		if (c == 0x20) {
			voodoofb_rectfill(sc, x, y, wi, he, bg);
		} else {
			uc = c-ri->ri_font->firstchar;
			data = (uint8_t *)ri->ri_font->data + uc * 
			    ri->ri_fontscale;
				voodoofb_setup_mono(sc, x, y, wi, he, fg, bg);		
			for (i = 0; i < he; i++) {
				voodoofb_feed_line(sc, 
				    ri->ri_font->stride, data);
				data += ri->ri_font->stride;
			}
		}
	}
}

static void
voodoofb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voodoofb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;
	
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;		
		voodoofb_bitblt(sc, xs, y, xd, y, width, height);
	}
}

static void
voodoofb_erasecols(void *cookie, int row, int startcol, int ncols, 
    long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voodoofb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;		
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);
	
		voodoofb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
voodoofb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voodoofb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;		
		voodoofb_bitblt(sc, x, ys, x, yd, width, height);
	}
}

static void
voodoofb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct voodoofb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);
		if ((row == 0) && (nrows == ri->ri_rows)) {
			/* clear the whole screen */
			voodoofb_rectfill(sc, 0, 0, ri->ri_width,
			    ri->ri_height, ri->ri_devcmap[bg]);
		} else {
			x = ri->ri_xorigin;
			y = ri->ri_yorigin + ri->ri_font->fontheight * row;
			width = ri->ri_emuwidth;
			height = ri->ri_font->fontheight * nrows;		
			voodoofb_rectfill(sc, x, y, width, height,
			    ri->ri_devcmap[bg]);
		}
	}
}

static void
voodoofb_bitblt(struct voodoofb_softc *sc, int xs, int ys, int xd, int yd, int width, int height) 
{
	uint32_t fmt, blitcmd;
	
	fmt = sc->linebytes | ((sc->bits_per_pixel + 
	    ((sc->bits_per_pixel == 8) ? 0 : 8)) << 13);
	blitcmd = COMMAND_2D_S2S_BITBLT | (ROP_COPY << 24);

	if (xs <= xd) {
	        blitcmd |= BIT(14);
		xs += (width - 1); 
		xd += (width - 1); 
	}
	if (ys <= yd) {
		blitcmd |= BIT(15);
		ys += (height - 1);
		yd += (height - 1);
	}
	voodoo3_make_room(sc, 6);
	
	voodoo3_write32(sc, SRCFORMAT, fmt);
	voodoo3_write32(sc, DSTFORMAT, fmt);
	voodoo3_write32(sc, DSTSIZE,   width | (height << 16));
	voodoo3_write32(sc, DSTXY,     xd | (yd << 16));
	voodoo3_write32(sc, SRCXY, xs | (ys << 16)); 
	voodoo3_write32(sc, COMMAND_2D, blitcmd | SST_2D_GO); 
}
 
static void
voodoofb_rectfill(struct voodoofb_softc *sc, int x, int y, int width, 
    int height, int colour) 
{
	uint32_t fmt, col;
	
	col = (colour << 24) | (colour << 16) | (colour << 8) | colour;
	fmt = sc->linebytes | ((sc->bits_per_pixel + 
	    ((sc->bits_per_pixel == 8) ? 0 : 8)) << 13);

	voodoo3_make_room(sc, 6);
	voodoo3_write32(sc, DSTFORMAT, fmt);
	voodoo3_write32(sc, COLORFORE, colour);
	voodoo3_write32(sc, COLORBACK, colour);
	voodoo3_write32(sc, COMMAND_2D, COMMAND_2D_FILLRECT | (ROP_COPY << 24));
	voodoo3_write32(sc, DSTSIZE,    width | (height << 16));
	voodoo3_write32(sc, LAUNCH_2D,  x | (y << 16));
}

static void
voodoofb_rectinvert(struct voodoofb_softc *sc, int x, int y, int width, 
    int height) 
{
	uint32_t fmt;
	
	fmt = sc->linebytes | ((sc->bits_per_pixel + 
	    ((sc->bits_per_pixel == 8) ? 0 : 8)) << 13);

	voodoo3_make_room(sc, 6);
	voodoo3_write32(sc, DSTFORMAT,	fmt);
	voodoo3_write32(sc, COMMAND_2D,	COMMAND_2D_FILLRECT | 
	    (ROP_INVERT << 24));
	voodoo3_write32(sc, DSTSIZE,	width | (height << 16));
	voodoo3_write32(sc, DSTXY,	x | (y << 16));
	voodoo3_write32(sc, LAUNCH_2D,	x | (y << 16));
}

static void 
voodoofb_setup_mono(struct voodoofb_softc *sc, int xd, int yd, int width, int height, uint32_t fg,
					uint32_t bg) 
{
	uint32_t dfmt, sfmt = sc->linebytes;
	
	dfmt = sc->linebytes | ((sc->bits_per_pixel + 
	    ((sc->bits_per_pixel == 8) ? 0 : 8)) << 13);

	voodoo3_make_room(sc, 9);
	voodoo3_write32(sc, SRCFORMAT,	sfmt);
	voodoo3_write32(sc, DSTFORMAT,	dfmt);
	voodoo3_write32(sc, COLORFORE,	fg);
	voodoo3_write32(sc, COLORBACK,	bg);
	voodoo3_write32(sc, DSTSIZE,	width | (height << 16));
	voodoo3_write32(sc, DSTXY,	xd | (yd << 16));
	voodoo3_write32(sc, SRCXY,	0);
	voodoo3_write32(sc, COMMAND_2D, COMMAND_2D_H2S_BITBLT | 
	    (ROP_COPY << 24) | SST_2D_GO);
	
	/* now feed the data into the chip */
}

static void 
voodoofb_feed_line(struct voodoofb_softc *sc, int count, uint8_t *data)
{
	int i;
	uint32_t latch = 0, bork;
	int shift = 0;
	
	voodoo3_make_room(sc, count);
	for (i = 0; i < count; i++) {
		bork = data[i];
		latch |= (bork << shift);
		if (shift == 24) {
			voodoo3_write32(sc, LAUNCH_2D, latch);
			latch = 0;
			shift = 0;
		} else
			shift += 8;
		}
	if (shift != 24)
		voodoo3_write32(sc, LAUNCH_2D, latch);
}	

#ifdef VOODOOFB_DEBUG
static void
voodoofb_showpal(struct voodoofb_softc *sc) 
{
	int i, x = 0;
	
	for (i = 0; i < 16; i++) {
		voodoofb_rectfill(sc, x, 0, 64, 64, i);
		x += 64;
	}
}
#endif

#if 0
static int
voodoofb_allocattr(void *cookie, int fg, int bg, int flags, long *attrp)
{

	return 0;
}
#endif

/*
 * wsdisplay_accessops
 */

static int
voodoofb_ioctl(void *v, void *vs, u_long cmd, caddr_t data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct voodoofb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
			return 0;

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;
			
		case WSDISPLAYIO_GETCMAP:
			return voodoofb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return voodoofb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		/* PCI config read/write passthrough. */
		case PCI_IOC_CFGREAD:
		case PCI_IOC_CFGWRITE:
			return (pci_devioctl(sc->sc_pc, sc->sc_pcitag,
			    cmd, data, flag, l));
			    
		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;
				if (new_mode != sc->sc_mode)
				{
					sc->sc_mode = new_mode;
					if(new_mode == WSDISPLAYIO_MODE_EMUL)
					{
						int i;
						
						/* restore the palette */
						for (i = 0; i < 256; i++) {
							voodoofb_putpalreg(sc, 
							   i, 
							   sc->sc_cmap_red[i], 
							   sc->sc_cmap_green[i],
							   sc->sc_cmap_blue[i]);
						}
						vcons_redraw_screen(ms);
					}
				}
			}
			return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
voodoofb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct voodoofb_softc *sc = vd->cookie;
	paddr_t pa;
		
	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_fbsize) {
		pa = bus_space_mmap(sc->sc_fbt, offset, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
		return pa;
	}

	if ((offset >= sc->sc_fb) && (offset < (sc->sc_fb + sc->sc_fbsize))) {
		pa = bus_space_mmap(sc->sc_memt, offset, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
		return pa;
	}

	if ((offset >= sc->sc_regs) && (offset < (sc->sc_regs + 
	    sc->sc_regsize))) {
		pa = bus_space_mmap(sc->sc_memt, offset, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
		return pa;
	}

	/* allow mapping of IO space */
	if ((offset >= 0xf2000000) && (offset < 0xf2800000)) {
		pa = bus_space_mmap(sc->sc_iot, offset-0xf2000000, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
		return pa;
	}		
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
voodoofb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct voodoofb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;
	
	ri->ri_depth = sc->bits_per_pixel;
	ri->ri_width = sc->width;
	ri->ri_height = sc->height;
	ri->ri_stride = sc->width;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = bus_space_vaddr(sc->sc_fbt, sc->sc_fbh);

#ifdef VOODOOFB_DEBUG
	printf("addr: %08lx\n", (ulong)ri->ri_bits);
#endif
	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}
	
	rasops_init(ri, sc->height/8, sc->width/8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->height / ri->ri_font->fontheight,
		    sc->width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	ri->ri_ops.copyrows = voodoofb_copyrows;
	ri->ri_ops.copycols = voodoofb_copycols;
	ri->ri_ops.eraserows = voodoofb_eraserows;
	ri->ri_ops.erasecols = voodoofb_erasecols;
	ri->ri_ops.cursor = voodoofb_cursor;
	ri->ri_ops.putchar = voodoofb_putchar;
}

#if 0
int
voodoofb_load_font(void *v, void *cookie, struct wsdisplay_font *data)
{

	return 0;
}
#endif

static int
voodoofb_intr(void *arg)
{
	struct voodoofb_softc *sc = arg;

	voodoo3_write32(sc, V3_STATUS, 0);	/* clear interrupts */
	return 1;
}

/* video mode stuff */

#define REFFREQ 14318	/* .18 */

#define ABS(a) ((a < 0) ? -a : a)

static int
voodoofb_calc_pll(int freq, int *f_out, int isBanshee)
{
	int m, n, k, best_m, best_n, best_k, f_cur, best_error;
	int minm, maxm;

	best_error = freq;
	best_n = best_m = best_k = 0;

	if (isBanshee) {
		minm = 24;
		maxm = 24;
	} else {
		minm = 1;
		maxm = 57; 
		/* This used to be 64, alas it seems the last 8 (funny that ?)
		 * values cause jittering at lower resolutions. I've not done
		 * any calculations to what the adjustment affects clock ranges,
		 * but I can still run at 1600x1200@75Hz */
	}
	for (n = 1; n < 256; n++) {
		f_cur = REFFREQ * (n + 2);
		if (f_cur < freq) {
			f_cur = f_cur / 3;
			if (freq - f_cur < best_error) {
				best_error = freq - f_cur;
				best_n = n;
				best_m = 1;
				best_k = 0;
				continue;
      			}
		}
		for (m = minm; m < maxm; m++) {
			for (k = 0; k < 4; k++) {
				f_cur = REFFREQ * (n + 2) / (m + 2) / (1 << k);
				if (ABS(f_cur - freq) < best_error) {
					best_error = ABS(f_cur - freq);
					best_n = n;
					best_m = m;
					best_k = k;
				}
			}
		}
	}
	n = best_n;
	m = best_m;
	k = best_k;
	*f_out = REFFREQ * (n + 2) / (m + 2) / (1 << k);
	return ( n << 8) | (m << 2) | k;
}

static void
voodoofb_setup_monitor(struct voodoofb_softc *sc, const struct videomode *vm)
{
	struct voodoo_regs mod;
	struct voodoo_regs *mode;
	uint32_t horizontal_display_end, horizontal_sync_start,
		horizontal_sync_end, horizontal_total,
		horizontal_blanking_start, horizontal_blanking_end;
	
	uint32_t vertical_display_enable_end, vertical_sync_start,
		vertical_sync_end, vertical_total, vertical_blanking_start,
		vertical_blanking_end;
		
	uint32_t wd; // CRTC offset

	int i;

	uint8_t misc;

	memset(&mod, 0, sizeof(mode));
	
	mode = &mod;
	
	wd = (vm->hdisplay >> 3) - 1;
	horizontal_display_end	= (vm->hdisplay >> 3) - 1;
	horizontal_sync_start	= (vm->hsync_start >> 3) - 1;
	horizontal_sync_end	= (vm->hsync_end >> 3) - 1;
	horizontal_total  	= (vm->htotal   >> 3) - 1;
	horizontal_blanking_start = horizontal_display_end;
	horizontal_blanking_end = horizontal_total;

	vertical_display_enable_end  = vm->vdisplay - 1;
	vertical_sync_start  	= vm->vsync_start;	// - 1;
	vertical_sync_end	= vm->vsync_end;	// - 1;
	vertical_total		= vm->vtotal - 2;
	vertical_blanking_start	= vertical_display_enable_end;
	vertical_blanking_end	= vertical_total;
	
	misc = 0x0f |
	    (vm->hdisplay < 400 ? 0xa0 :
		vm->hdisplay < 480 ? 0x60 :
		vm->hdisplay < 768 ? 0xe0 : 0x20);
     
	mode->vr_seq[0] = 3;
	mode->vr_seq[1] = 1;
	mode->vr_seq[2] = 8;
	mode->vr_seq[3] = 0;
	mode->vr_seq[4] = 6;
    
	/* crtc regs start */
	mode->vr_crtc[0] = horizontal_total - 4;
	mode->vr_crtc[1] = horizontal_display_end;
	mode->vr_crtc[2] = horizontal_blanking_start;
	mode->vr_crtc[3] = 0x80 | (horizontal_blanking_end & 0x1f);
	mode->vr_crtc[4] = horizontal_sync_start;
    
	mode->vr_crtc[5] = ((horizontal_blanking_end & 0x20) << 2) |
	    (horizontal_sync_end & 0x1f);
	mode->vr_crtc[6] = vertical_total;
	mode->vr_crtc[7] = ((vertical_sync_start & 0x200) >> 2) |
	    ((vertical_display_enable_end & 0x200) >> 3) |
	    ((vertical_total & 0x200) >> 4) |
	    0x10 |
	    ((vertical_blanking_start & 0x100) >> 5) |
	    ((vertical_sync_start  & 0x100) >> 6) |
	    ((vertical_display_enable_end  & 0x100) >> 7) |
	    ((vertical_total  & 0x100) >> 8);
    
	mode->vr_crtc[8] = 0;
	mode->vr_crtc[9] = 0x40 |
	    ((vertical_blanking_start & 0x200) >> 4);
    
	mode->vr_crtc[10] = 0;
	mode->vr_crtc[11] = 0;
	mode->vr_crtc[12] = 0;
	mode->vr_crtc[13] = 0;
	mode->vr_crtc[14] = 0;
	mode->vr_crtc[15] = 0;
 
	mode->vr_crtc[16] = vertical_sync_start;
	mode->vr_crtc[17] = (vertical_sync_end & 0x0f) | 0x20;
	mode->vr_crtc[18] = vertical_display_enable_end;
	mode->vr_crtc[19] = wd; // CRTC offset
	mode->vr_crtc[20] = 0;
	mode->vr_crtc[21] = vertical_blanking_start;
	mode->vr_crtc[22] = vertical_blanking_end + 1;
	mode->vr_crtc[23] = 128;
	mode->vr_crtc[24] = 255;
    
	/* attr regs start */
	mode->vr_attr[0] = 0;
	mode->vr_attr[1] = 0;
	mode->vr_attr[2] = 0;
	mode->vr_attr[3] = 0;
	mode->vr_attr[4] = 0;
	mode->vr_attr[5] = 0;
	mode->vr_attr[6] = 0;
	mode->vr_attr[7] = 0;
	mode->vr_attr[8] = 0;
	mode->vr_attr[9] = 0;
	mode->vr_attr[10] = 0;
	mode->vr_attr[11] = 0;
	mode->vr_attr[12] = 0;
	mode->vr_attr[13] = 0;
	mode->vr_attr[14] = 0;
	mode->vr_attr[15] = 0;
	mode->vr_attr[16] = 1;
	mode->vr_attr[17] = 0;
	mode->vr_attr[18] = 15;
	mode->vr_attr[19] = 0;
	/* attr regs end */

	/* graph regs start */
	mode->vr_graph[0] = 159;
	mode->vr_graph[1] = 127;
	mode->vr_graph[2] = 127;
	mode->vr_graph[3] = 131;
	mode->vr_graph[4] = 130;
	mode->vr_graph[5] = 142;
	mode->vr_graph[6] = 30;
	mode->vr_graph[7] = 245;
	mode->vr_graph[8] = 0;

	vga_outb(sc, MISC_W, misc | 0x01);
    
	for(i = 0; i < 5; i++)
		voodoo3_write_seq(sc, i, mode->vr_seq[i]);
	for (i = 0; i < 0x19; i ++)
        	voodoo3_write_crtc(sc, i, mode->vr_crtc[i]);
	for (i = 0; i < 0x14; i ++)
        	voodoo3_write_attr(sc, i, mode->vr_attr[i]);
	for (i = 0; i < 0x09; i ++)
        	voodoo3_write_gra(sc, i, mode->vr_graph[i]);
}

static void
voodoofb_set_videomode(struct voodoofb_softc *sc, 
    const struct videomode *vm)
{
	uint32_t miscinit0 = 0;
	int vidpll, fout;
	uint32_t vp, vidproc = VIDPROCDEFAULT;
	uint32_t bpp = 1;	/* for now */
	uint32_t bytes_per_row = vm->hdisplay * bpp;

	sc->bits_per_pixel = bpp << 3;
	sc->width = vm->hdisplay;
	sc->height = vm->vdisplay;
	sc->linebytes = bytes_per_row;
	
	voodoofb_setup_monitor(sc, vm);
	vp = voodoo3_read32(sc, VIDPROCCFG);
	
	vidproc &= ~(0x1c0000); /* clear bits 18 to 20, bpp in vidproccfg */
	/* enable bits 18 to 20 to the required bpp */
	vidproc |= ((bpp - 1) << VIDCFG_PIXFMT_SHIFT);
	
	vidpll = voodoofb_calc_pll(vm->dot_clock, &fout, 0);

#ifdef VOODOOFB_DEBUG
	printf("old vidproc: %08x\n", vp);
	printf("pll: %08x %d\n", vidpll, fout);
#endif
	/* bit 10 of vidproccfg, is enabled or disabled as needed */
	switch (bpp) { 
		case 1:
			/*
			 * bit 10 off for palettized modes only, off means
			 * palette is used
			 */
			vidproc &= ~(1 << 10);
			break;
#if 0
		case 2:
			#if __POWERPC__
				miscinit0 = 0xc0000000;
			#endif
			/* bypass palette for 16bit modes */
			vidproc |= (1 << 10);
			break;
		case 4:
			#if __POWERPC__
				miscinit0 = 0x40000000;
			#endif			
			vidproc |= (1 << 10); /* Same for 32bit modes */
			break;
#endif
		default:
			printf("We support only 8 bit for now\n");
			return;
	}
	
	voodoofb_wait_idle(sc);
	
	voodoo3_write32(sc, MISCINIT1, voodoo3_read32(sc, MISCINIT1) | 0x01);

	voodoo3_make_room(sc, 4);
	voodoo3_write32(sc, VGAINIT0, 4928);
	voodoo3_write32(sc, DACMODE, 0);
	voodoo3_write32(sc, VIDDESKSTRIDE, bytes_per_row);
	voodoo3_write32(sc, PLLCTRL0, vidpll);
	
	voodoo3_make_room(sc, 5);
	voodoo3_write32(sc, VIDSCREENSIZE, sc->width | (sc->height << 12));
	voodoo3_write32(sc, VIDDESKSTART,  1024);

	vidproc &= ~VIDCFG_HWCURSOR_ENABLE;
	voodoo3_write32(sc, VIDPROCCFG, vidproc);

	voodoo3_write32(sc, VGAINIT1, 0);
	voodoo3_write32(sc, MISCINIT0, miscinit0);
#ifdef VOODOOFB_DEBUG
	printf("vidproc: %08x\n", vidproc);
#endif
	voodoo3_make_room(sc, 8);
	voodoo3_write32(sc, SRCBASE, 0);
	voodoo3_write32(sc, DSTBASE, 0);
	voodoo3_write32(sc, COMMANDEXTRA_2D, 0);
  	voodoo3_write32(sc, CLIP0MIN,        0);
  	voodoo3_write32(sc, CLIP0MAX,        0x0fff0fff);
  	voodoo3_write32(sc, CLIP1MIN,        0);
  	voodoo3_write32(sc, CLIP1MAX,        0x0fff0fff);
	voodoo3_write32(sc, SRCXY, 0);
	voodoofb_wait_idle(sc);
	printf("%s: switched to %dx%d, %d bit\n", sc->sc_dev.dv_xname,
	    sc->width, sc->height, sc->bits_per_pixel);
}

static void
voodoofb_init(struct voodoofb_softc *sc)
{
	/* XXX */
	uint32_t vgainit0 = 0;
	uint32_t vidcfg = 0;

#ifdef VOODOOFB_DEBUG
	printf("initializing engine...");	
#endif	
	vgainit0 = voodoo3_read32(sc, VGAINIT0);
#ifdef VOODOOFB_DEBUG
	printf("vga: %08x", vgainit0);
#endif
	vgainit0 |=
	    VGAINIT0_8BIT_DAC     |
	    VGAINIT0_EXT_ENABLE   |
	    VGAINIT0_WAKEUP_3C3   |
	    VGAINIT0_ALT_READBACK |
	    VGAINIT0_EXTSHIFTOUT;
	
	vidcfg = voodoo3_read32(sc, VIDPROCCFG);
#ifdef VOODOOFB_DEBUG
	printf(" vidcfg: %08x\n", vidcfg);
#endif
	vidcfg |=
	    VIDCFG_VIDPROC_ENABLE |
	    VIDCFG_DESK_ENABLE;
	vidcfg &= ~VIDCFG_HWCURSOR_ENABLE;
	
	voodoo3_make_room(sc, 2);

	voodoo3_write32(sc, VGAINIT0, vgainit0);
	voodoo3_write32(sc, VIDPROCCFG, vidcfg);

	voodoo3_make_room(sc, 8);
	voodoo3_write32(sc, SRCBASE, 0);
	voodoo3_write32(sc, DSTBASE, 0);
	voodoo3_write32(sc, COMMANDEXTRA_2D, 0);
  	voodoo3_write32(sc, CLIP0MIN,        0);
  	voodoo3_write32(sc, CLIP0MAX,        0x1fff1fff);
  	voodoo3_write32(sc, CLIP1MIN,        0);
  	voodoo3_write32(sc, CLIP1MAX,        0x1fff1fff);
	voodoo3_write32(sc, SRCXY, 0);

	voodoofb_wait_idle(sc);
}
