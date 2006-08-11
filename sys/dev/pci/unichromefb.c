/* $NetBSD: unichromefb.c,v 1.1.4.2 2006/08/11 15:44:26 yamt Exp $ */

/*-
 * Copyright (c) 2006 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
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

/*
 * Copyright 1998-2006 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2006 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) OR COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: unichromefb.c,v 1.1.4.2 2006/08/11 15:44:26 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/pci/unichromereg.h>
#include <dev/pci/unichromemode.h>
#include <dev/pci/unichromehw.h>
#include <dev/pci/unichromeconfig.h>

/* XXX */
#define UNICHROMEFB_DEPTH	32

struct unichromefb_softc {
	struct device		sc_dev;
	struct vcons_data	sc_vd;
	void *			sc_fbbase;
	unsigned int		sc_fbaddr;
	unsigned int		sc_fbsize;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;

	int			sc_width;
	int			sc_height;
	int			sc_depth;
	int			sc_stride;

	int			sc_wsmode;
};

static int unichromefb_match(struct device *, struct cfdata *, void *);
static void unichromefb_attach(struct device *, struct device *, void *);

/* XXX */
int unichromefb_cnattach(void);

struct wsscreen_descr unichromefb_stdscreen = {
	"fb",
	0, 0,
	NULL,
	8, 16,
};

static int	unichromefb_ioctl(void *, void *, u_long, caddr_t, int,
				  struct lwp *);
static paddr_t	unichromefb_mmap(void *, void *, off_t, int);

static void	unichromefb_init_screen(void *, struct vcons_screen *,
					int, long *);

/* hardware access */
static uint8_t	uni_rd(struct unichromefb_softc *, int, uint8_t);
static void	uni_wr(struct unichromefb_softc *, int, uint8_t, uint8_t);
static void	uni_wr_mask(struct unichromefb_softc *, int, uint8_t,
			    uint8_t, uint8_t);
static void	uni_wr_x(struct unichromefb_softc *, struct io_reg *, int);
#if notyet
static void	uni_wr_dac(struct unichromefb_softc *, uint8_t, uint8_t,
			   uint8_t, uint8_t);
#endif

/* helpers */
static struct VideoModeTable *	uni_getmode(int);
static void	uni_setmode(struct unichromefb_softc *, int, int);
static void	uni_crt_lock(struct unichromefb_softc *);
static void	uni_crt_unlock(struct unichromefb_softc *);
static void	uni_crt_enable(struct unichromefb_softc *);
static void	uni_screen_enable(struct unichromefb_softc *);
static void	uni_set_start(struct unichromefb_softc *);
static void	uni_set_crtc(struct unichromefb_softc *,
			     struct crt_mode_table *, int, int, int);
static void	uni_load_crtc(struct unichromefb_softc *, struct display_timing,
			      int);
static void	uni_load_reg(struct unichromefb_softc *, int, int,
			     struct io_register *, int);
static void	uni_fix_crtc(struct unichromefb_softc *);
static void	uni_load_offset(struct unichromefb_softc *, int, int, int);
static void	uni_load_fetchcnt(struct unichromefb_softc *, int, int, int);
static void	uni_load_fifo(struct unichromefb_softc *, int, int, int);
static void	uni_set_depth(struct unichromefb_softc *, int, int);
static uint32_t	uni_get_clkval(struct unichromefb_softc *, int);
static void	uni_set_vclk(struct unichromefb_softc *, uint32_t, int);

struct wsdisplay_accessops unichromefb_accessops = {
	unichromefb_ioctl,
	unichromefb_mmap,
	NULL,
	NULL,
	NULL,
	NULL,
};

static struct vcons_screen unichromefb_console_screen;

const struct wsscreen_descr *_unichromefb_scrlist[] = {
	&unichromefb_stdscreen,
};

struct wsscreen_list unichromefb_screenlist = {
	sizeof(_unichromefb_scrlist) / sizeof(struct wsscreen_descr *),
	_unichromefb_scrlist
};

CFATTACH_DECL(unichromefb, sizeof(struct unichromefb_softc),
    unichromefb_match, unichromefb_attach, NULL, NULL);

static int
unichromefb_match(struct device *parent, struct cfdata *match, void *opaque)
{
	struct pci_attach_args *pa;

	pa = (struct pci_attach_args *)opaque;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_DISPLAY_VGA)
		return 0;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_VIATECH)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_VIATECH_VT3314_IG:
		return 10;	/* beat vga(4) */
	}

	return 0;
}

static void
unichromefb_attach(struct device *parent, struct device *self, void *opaque)
{
	struct unichromefb_softc *sc;
	struct pci_attach_args *pa;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args aa;
	uint8_t val;
	long defattr;

	sc = (struct unichromefb_softc *)self;
	pa = (struct pci_attach_args *)opaque;

	/* XXX */
	sc->sc_width = 640;
	sc->sc_height = 480;
	sc->sc_depth = UNICHROMEFB_DEPTH;
	sc->sc_stride = sc->sc_width * (sc->sc_depth / 8);

	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;

	sc->sc_iot = pa->pa_iot;
	if (bus_space_map(sc->sc_iot, VIA_REGBASE, 0x20, 0, &sc->sc_ioh)) {
		aprint_error(": failed to map I/O registers\n");
		return;
	}

	val = uni_rd(sc, VIASR, SR30);
	sc->sc_fbaddr = val << 24;
	sc->sc_fbsize = sc->sc_width * sc->sc_height * (sc->sc_depth / 8);
	sc->sc_memt = pa->pa_memt;
	if (bus_space_map(sc->sc_memt, sc->sc_fbaddr, sc->sc_fbsize,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_memh)) {
		aprint_error(": failed to map aperture at 0x%08x/0x%x\n",
		    sc->sc_fbaddr, sc->sc_fbsize);
		return;
	}
	sc->sc_fbbase = (caddr_t)bus_space_vaddr(sc->sc_memt, sc->sc_memh);
	/*memset(sc->sc_fbbase, 0, sc->sc_fbsize);*/

	aprint_naive("\n");
	aprint_normal(": VIA UniChrome frame buffer\n");

	ri = &unichromefb_console_screen.scr_ri;
	memset(ri, 0, sizeof(struct rasops_info));

	vcons_init(&sc->sc_vd, sc, &unichromefb_stdscreen,
	    &unichromefb_accessops);
	sc->sc_vd.init_screen = unichromefb_init_screen;

	uni_setmode(sc, VIA_RES_640X480, sc->sc_depth);

	aprint_normal("%s: fb %dx%dx%d @%p\n", sc->sc_dev.dv_xname,
	       sc->sc_width, sc->sc_height, sc->sc_depth, sc->sc_fbbase);
	delay(5*1000*1000);

	unichromefb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	vcons_init_screen(&sc->sc_vd, &unichromefb_console_screen, 1, &defattr);

	unichromefb_stdscreen.ncols = ri->ri_cols;
	unichromefb_stdscreen.nrows = ri->ri_rows;
	unichromefb_stdscreen.textops = &ri->ri_ops;
	unichromefb_stdscreen.capabilities = ri->ri_caps;
	unichromefb_stdscreen.modecookie = NULL;

	wsdisplay_cnattach(&unichromefb_stdscreen, ri, 0, 0, defattr);

	aa.console = 1; /* XXX */
	aa.scrdata = &unichromefb_screenlist;
	aa.accessops = &unichromefb_accessops;
	aa.accesscookie = &sc->sc_vd;

	config_found(self, &aa, wsemuldisplaydevprint);

	return;
}

static int
unichromefb_ioctl(void *v, void *vs, u_long cmd, caddr_t data, int flag,
		  struct lwp *l)
{
	struct vcons_data *vd;
	struct unichromefb_softc *sc;
	struct wsdisplay_fbinfo *fb;

	vd = (struct vcons_data *)v;
	sc = (struct unichromefb_softc *)vd->cookie;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;
	case WSDISPLAYIO_GINFO:
		if (vd->active != NULL) {
			fb = (struct wsdisplay_fbinfo *)data;
			fb->width = sc->sc_width;
			fb->height = sc->sc_height;
			fb->depth = sc->sc_depth;
			fb->cmsize = 256;
			return 0;
		} else
			return ENODEV;
	case WSDISPLAYIO_GVIDEO:
			return ENODEV;
	case WSDISPLAYIO_SVIDEO:
			return ENODEV;
	case WSDISPLAYIO_GETCMAP:
			return EINVAL;
	case WSDISPLAYIO_PUTCMAP:
			return EINVAL;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;
		return 0;
	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int *)data;
			if (new_mode != sc->sc_wsmode) {
				sc->sc_wsmode = new_mode;
				if (new_mode == WSDISPLAYIO_MODE_EMUL)
					vcons_redraw_screen(vd->active);
			}
		}
		return 0;
	case WSDISPLAYIO_SSPLASH:
		return ENODEV;
	case WSDISPLAYIO_SPROGRESS:
		return ENODEV;
	}

	return EPASSTHROUGH;
}

static paddr_t
unichromefb_mmap(void *v, void *vs, off_t offset, int prot)
{
	return -1;
}

static void
unichromefb_init_screen(void *c, struct vcons_screen *scr, int existing,
			long *defattr)
{
	struct unichromefb_softc *sc;
	struct rasops_info *ri;

	sc = (struct unichromefb_softc *)c;
	ri = &scr->scr_ri;
	ri->ri_flg = RI_CENTER;
	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_bits = sc->sc_fbbase;

	ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 8;
	ri->ri_rpos = 16;
	ri->ri_gpos = 8;
	ri->ri_bpos = 0;

	rasops_init(ri, sc->sc_height / 16, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width / ri->ri_font->fontwidth);

	return;
}

/*
 * hardware access
 */
static uint8_t
uni_rd(struct unichromefb_softc *sc, int off, uint8_t idx)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, off, idx);
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, off + 1);
}

static void
uni_wr(struct unichromefb_softc *sc, int off, uint8_t idx, uint8_t val)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, off, idx);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, off + 1, val);
}

static void
uni_wr_mask(struct unichromefb_softc *sc, int off, uint8_t idx,
    uint8_t val, uint8_t mask)
{
	uint8_t tmp;

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, off, idx);
	tmp = bus_space_read_1(sc->sc_iot, sc->sc_ioh, off + 1);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, off + 1,
	    ((val & mask) | (tmp & ~mask)));
}

#if notyet
static void
uni_wr_dac(struct unichromefb_softc *sc, uint8_t idx,
    uint8_t r, uint8_t g, uint8_t b)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LUT_INDEX_WRITE, idx);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LUT_DATA, r);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LUT_DATA, g);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LUT_DATA, b);
}
#endif

static void
uni_wr_x(struct unichromefb_softc *sc, struct io_reg *tbl, int num)
{
	int i;
	uint8_t tmp;

	for (i = 0; i < num; i++) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, tbl[i].port,
		    tbl[i].index);
		tmp = bus_space_read_1(sc->sc_iot, sc->sc_iot,
		    tbl[i].port + 1);
		tmp = (tmp & (~tbl[i].mask)) | tbl[i].value;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, tbl[i].index + 1,
		    tmp);
	}
}

/*
 * helpers
 */
static struct VideoModeTable *
uni_getmode(int mode)
{
	int i;

	for (i = 0; i < NUM_TOTAL_MODETABLE; i++)
		if (CLE266Modes[i].ModeIndex == mode)
			return &CLE266Modes[i];

	return NULL;
}

static void
uni_setmode(struct unichromefb_softc *sc, int idx, int bpp)
{
	struct VideoModeTable *vtbl;
	struct crt_mode_table *crt;
	int i;

	/* XXX */
	vtbl = uni_getmode(idx);
	if (vtbl == NULL)
		panic("%s: unsupported mode: %d\n", sc->sc_dev.dv_xname, idx);

	crt = vtbl->crtc;

	(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAStatus);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAAR, 0);

	/* XXX assume CN900 for now */
	uni_wr_x(sc, CN900_ModeXregs, NUM_TOTAL_CN900_ModeXregs);

	/* Fill VPIT params */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAWMisc, VPIT.Misc);

	/* Write sequencer */
	for (i = 1; i <= StdSR; i++) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIASR, i);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIASR + 1,
		    VPIT.SR[i - 1]);
	}

	uni_set_start(sc);

	uni_set_crtc(sc, crt, idx, bpp / 8, IGA1);

	for (i = 0; i < StdGR; i++) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAGR, i);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAGR + 1,
		    VPIT.GR[i]);
	}

	for (i = 0; i < StdAR; i++) {
		(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAStatus);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAAR, i);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAAR + 1,
		    VPIT.AR[i]);
	}

	(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAStatus);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAAR, 0x20);

	uni_set_crtc(sc, crt, idx, bpp / 8, IGA1);
	/* set crt output path */
	uni_wr_mask(sc, VIASR, SR16, 0x00, BIT6);

	uni_crt_enable(sc);
	uni_screen_enable(sc);

	return;
}

static void
uni_crt_lock(struct unichromefb_softc *sc)
{
	uni_wr_mask(sc, VIACR, CR11, BIT7, BIT7);
}

static void
uni_crt_unlock(struct unichromefb_softc *sc)
{
	uni_wr_mask(sc, VIACR, CR11, 0, BIT7);
	uni_wr_mask(sc, VIACR, CR47, 0, BIT0);
}

static void
uni_crt_enable(struct unichromefb_softc *sc)
{
	uni_wr_mask(sc, VIACR, CR36, 0, BIT5+BIT4);
}

static void
uni_screen_enable(struct unichromefb_softc *sc)
{
	uni_wr_mask(sc, VIASR, SR01, 0, BIT5);
}

static void
uni_set_start(struct unichromefb_softc *sc)
{
	uni_crt_unlock(sc);

	uni_wr(sc, VIACR, CR0C, 0x00);
	uni_wr(sc, VIACR, CR0D, 0x00);
	uni_wr(sc, VIACR, CR34, 0x00);
	uni_wr_mask(sc, VIACR, CR48, 0x00, BIT0 + BIT1);

	uni_wr(sc, VIACR, CR62, 0x00);
	uni_wr(sc, VIACR, CR63, 0x00);
	uni_wr(sc, VIACR, CR64, 0x00);
	uni_wr(sc, VIACR, CRA3, 0x00);

	uni_crt_lock(sc);
}

static void
uni_set_crtc(struct unichromefb_softc *sc, struct crt_mode_table *ctbl,
    int mode, int bpp_byte, int iga)
{
	struct VideoModeTable *vtbl;
	struct display_timing crtreg;
	int i;
	int index;
	int haddr, vaddr;
	uint8_t val;
	uint32_t pll_d_n;

	index = 0;

	vtbl = uni_getmode(mode);
	for (i = 0; i < vtbl->mode_array; i++) {
		index = i;
		if (ctbl[i].refresh_rate == 60)
			break;
	}

	crtreg = ctbl[index].crtc;

	haddr = crtreg.hor_addr;
	vaddr = crtreg.ver_addr;

	val = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIARMisc);
	if (ctbl[index].h_sync_polarity == NEGATIVE) {
		if (ctbl[index].v_sync_polarity == NEGATIVE)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAWMisc,
			    (val & (~(BIT6+BIT7))) | (BIT6+BIT7));
		else
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAWMisc,
			    (val & (~(BIT6+BIT7))) | (BIT6));
	} else {
		if (ctbl[index].v_sync_polarity == NEGATIVE)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAWMisc,
			    (val & (~(BIT6+BIT7))) | (BIT7));
		else
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAWMisc,
			    (val & (~(BIT6+BIT7))));
	}

	if (iga == IGA1) {
		uni_crt_unlock(sc);
		uni_wr(sc, VIACR, CR09, 0x00);
		uni_wr_mask(sc, VIACR, CR11, 0x00, BIT4+BIT5+BIT6);
		uni_wr_mask(sc, VIACR, CR17, 0x00, BIT7);
	}

	uni_load_crtc(sc, crtreg, iga);
	uni_fix_crtc(sc);
	uni_crt_lock(sc);
	uni_wr_mask(sc, VIACR, CR17, 0x80, BIT7);

	uni_load_offset(sc, haddr, bpp_byte, iga);
	uni_load_fetchcnt(sc, haddr, bpp_byte, iga);
	uni_load_fifo(sc, iga, haddr, vaddr);

	uni_set_depth(sc, bpp_byte, iga);
	pll_d_n = uni_get_clkval(sc, ctbl[index].clk);
	uni_set_vclk(sc, pll_d_n, iga);
}

static void
uni_load_crtc(struct unichromefb_softc *sc,
    struct display_timing device_timing, int iga)
{
	int regnum, val;
	struct io_register *reg;
	int i;

	regnum = val = 0;
	reg = NULL;

	uni_crt_unlock(sc);

	for (i = 0; i < 12; i++) {
		switch (iga) {
		case IGA1:
			switch (i) {
			case H_TOTAL_INDEX:
				val = IGA1_HOR_TOTAL_FORMULA(
				    device_timing.hor_total);
				regnum = iga1_crtc_reg.hor_total.reg_num;
				reg = iga1_crtc_reg.hor_total.reg;
				break;
			case H_ADDR_INDEX:
				val = IGA1_HOR_ADDR_FORMULA(
				    device_timing.hor_addr);
				regnum = iga1_crtc_reg.hor_addr.reg_num;
				reg = iga1_crtc_reg.hor_addr.reg;
				break;
			case H_BLANK_START_INDEX:
				val = IGA1_HOR_BLANK_START_FORMULA(
				    device_timing.hor_blank_start);
				regnum = iga1_crtc_reg.hor_blank_start.reg_num;
				reg = iga1_crtc_reg.hor_blank_start.reg;
				break;
			case H_BLANK_END_INDEX:
				val = IGA1_HOR_BLANK_END_FORMULA(
				    device_timing.hor_blank_start,
				    device_timing.hor_blank_end);
				regnum = iga1_crtc_reg.hor_blank_end.reg_num;
				reg = iga1_crtc_reg.hor_blank_end.reg;
				break;
			case H_SYNC_START_INDEX:
				val = IGA1_HOR_SYNC_START_FORMULA(
				    device_timing.hor_sync_start);
				regnum = iga1_crtc_reg.hor_sync_start.reg_num;
				reg = iga1_crtc_reg.hor_sync_start.reg;
				break;
			case H_SYNC_END_INDEX:
				val = IGA1_HOR_SYNC_END_FORMULA(
				    device_timing.hor_sync_start,
				    device_timing.hor_sync_end);
				regnum = iga1_crtc_reg.hor_sync_end.reg_num;
				reg = iga1_crtc_reg.hor_sync_end.reg;
				break;
			case V_TOTAL_INDEX:
				val = IGA1_VER_TOTAL_FORMULA(
				    device_timing.ver_total);
				regnum = iga1_crtc_reg.ver_total.reg_num;
				reg = iga1_crtc_reg.ver_total.reg;
				break;
			case V_ADDR_INDEX:
				val = IGA1_VER_ADDR_FORMULA(
				    device_timing.ver_addr);
				regnum = iga1_crtc_reg.ver_addr.reg_num;
				reg = iga1_crtc_reg.ver_addr.reg;
				break;
			case V_BLANK_START_INDEX:
				val = IGA1_VER_BLANK_START_FORMULA(
				    device_timing.ver_blank_start);
				regnum = iga1_crtc_reg.ver_blank_start.reg_num;
				reg = iga1_crtc_reg.ver_blank_start.reg;
				break;
			case V_BLANK_END_INDEX:
				val = IGA1_VER_BLANK_END_FORMULA(
				    device_timing.ver_blank_start,
				    device_timing.ver_blank_end);
				regnum = iga1_crtc_reg.ver_blank_end.reg_num;
				reg = iga1_crtc_reg.ver_blank_end.reg;
				break;
			case V_SYNC_START_INDEX:
				val = IGA1_VER_SYNC_START_FORMULA(
				    device_timing.ver_sync_start);
				regnum = iga1_crtc_reg.ver_sync_start.reg_num;
				reg = iga1_crtc_reg.ver_sync_start.reg;
				break;
			case V_SYNC_END_INDEX:
				val = IGA1_VER_SYNC_END_FORMULA(
				    device_timing.ver_sync_start,
				    device_timing.ver_sync_end);
				regnum = iga1_crtc_reg.ver_sync_end.reg_num;
				reg = iga1_crtc_reg.ver_sync_end.reg;
				break;
			}
			break;
		case IGA2:
			printf("%s: %s: IGA2 not supported\n",
			    sc->sc_dev.dv_xname, __func__);
			break;
		}

		uni_load_reg(sc, val, regnum, reg, VIACR);
	}

	uni_crt_lock(sc);
}

static void
uni_load_reg(struct unichromefb_softc *sc, int timing, int regnum,
    struct io_register *reg, int type)
{
	int regmask, bitnum, data;
	int i, j;
	int shift_next_reg;
	int startidx, endidx, cridx;
	uint16_t getbit;

	bitnum = 0;

	for (i = 0; i < regnum; i++) {
		regmask = data = 0;
		startidx = reg[i].start_bit;
		endidx = reg[i].end_bit;
		cridx = reg[i].io_addr;

		shift_next_reg = bitnum;

		for (j = startidx; j <= endidx; j++) {
			regmask = regmask | (BIT0 << j);
			getbit = (timing & (BIT0 << bitnum));
			data = data | ((getbit >> shift_next_reg) << startidx);
			++bitnum;
		}

		if (type == VIACR)
			uni_wr_mask(sc, VIACR, cridx, data, regmask);
		else
			uni_wr_mask(sc, VIASR, cridx, data, regmask);
	}

	return;
}

static void
uni_fix_crtc(struct unichromefb_softc *sc)
{
	uni_wr_mask(sc, VIACR, CR03, 0x80, BIT7);
	uni_wr(sc, VIACR, CR18, 0xff);
	uni_wr_mask(sc, VIACR, CR07, 0x10, BIT4);
	uni_wr_mask(sc, VIACR, CR09, 0x40, BIT6);
	uni_wr_mask(sc, VIACR, CR35, 0x10, BIT4);
	uni_wr_mask(sc, VIACR, CR33, 0x06, BIT0+BIT1+BIT2);
	uni_wr(sc, VIACR, CR17, 0xe3);
	uni_wr(sc, VIACR, CR08, 0x00);
	uni_wr(sc, VIACR, CR14, 0x00);

	return;
}

static void
uni_load_offset(struct unichromefb_softc *sc, int haddr, int bpp, int iga)
{

	switch (iga) {
	case IGA1:
		uni_load_reg(sc,
		    IGA1_OFFSET_FORMULA(haddr, bpp),
		    offset_reg.iga1_offset_reg.reg_num,
		    offset_reg.iga1_offset_reg.reg,
		    VIACR);
		break;
	default:
		printf("%s: %s: only IGA1 is supported\n", sc->sc_dev.dv_xname,
		    __func__);
		break;
	}

	return;
}

static void
uni_load_fetchcnt(struct unichromefb_softc *sc, int haddr, int bpp, int iga)
{

	switch (iga) {
	case IGA1:
		uni_load_reg(sc,
		    IGA1_FETCH_COUNT_FORMULA(haddr, bpp),
		    fetch_count_reg.iga1_fetch_count_reg.reg_num,
		    fetch_count_reg.iga1_fetch_count_reg.reg,
		    VIASR);
		break;
	default:
		printf("%s: %s: only IGA1 is supported\n", sc->sc_dev.dv_xname,
		    __func__);
		break;
	}

	return;
}

static void
uni_load_fifo(struct unichromefb_softc *sc, int iga, int horact, int veract)
{
	int val, regnum;
	struct io_register *reg;
	int iga1_fifo_max_depth, iga1_fifo_threshold;
	int iga1_fifo_high_threshold, iga1_display_queue_expire_num;

	reg = NULL;
	iga1_fifo_max_depth = iga1_fifo_threshold = 0;
	iga1_fifo_high_threshold = iga1_display_queue_expire_num = 0;

	switch (iga) {
	case IGA1:
		/* XXX if (type == CN900) { */
		iga1_fifo_max_depth = CN900_IGA1_FIFO_MAX_DEPTH;
		iga1_fifo_threshold = CN900_IGA1_FIFO_THRESHOLD;
		iga1_fifo_high_threshold = CN900_IGA1_FIFO_HIGH_THRESHOLD;
		if (horact > 1280 && veract > 1024)
			iga1_display_queue_expire_num = 16;
		else
			iga1_display_queue_expire_num =
			    CN900_IGA1_DISPLAY_QUEUE_EXPIRE_NUM;
		/* XXX } */

		/* set display FIFO depth select */
		val = IGA1_FIFO_DEPTH_SELECT_FORMULA(iga1_fifo_max_depth);
		regnum =
		    display_fifo_depth_reg.iga1_fifo_depth_select_reg.reg_num;
		reg = display_fifo_depth_reg.iga1_fifo_depth_select_reg.reg;
		uni_load_reg(sc, val, regnum, reg, VIASR);

		/* set display FIFO threshold select */
		val = IGA1_FIFO_THRESHOLD_FORMULA(iga1_fifo_threshold);
		regnum = fifo_threshold_select_reg.iga1_fifo_threshold_select_reg.reg_num;
		reg = fifo_threshold_select_reg.iga1_fifo_threshold_select_reg.reg;
		uni_load_reg(sc, val, regnum, reg, VIASR);

		/* set display FIFO high threshold select */
		val = IGA1_FIFO_HIGH_THRESHOLD_FORMULA(iga1_fifo_high_threshold);
		regnum = fifo_high_threshold_select_reg.iga1_fifo_high_threshold_select_reg.reg_num;
		reg = fifo_high_threshold_select_reg.iga1_fifo_high_threshold_select_reg.reg;
		uni_load_reg(sc, val, regnum, reg, VIASR);

		/* set display queue expire num */
		val = IGA1_DISPLAY_QUEUE_EXPIRE_NUM_FORMULA(iga1_display_queue_expire_num);
		regnum = display_queue_expire_num_reg.iga1_display_queue_expire_num_reg.reg_num;
		reg = display_queue_expire_num_reg.iga1_display_queue_expire_num_reg.reg;
		uni_load_reg(sc, val, regnum, reg, VIASR);

		break;
	default:
		printf("%s: %s: only IGA1 is supported\n", sc->sc_dev.dv_xname,
		    __func__);
		break;
	}

	return;
}

static void
uni_set_depth(struct unichromefb_softc *sc, int bpp, int iga)
{
	switch (iga) {
	case IGA1:
		switch (bpp) {
		case MODE_32BPP:
			uni_wr_mask(sc, VIASR, SR15, 0xae, 0xfe);
			break;
		case MODE_16BPP:
			uni_wr_mask(sc, VIASR, SR15, 0xb6, 0xfe);
			break;
		case MODE_8BPP:
			uni_wr_mask(sc, VIASR, SR15, 0x22, 0xfe);
			break;
		default:
			printf("%s: %s: mode (%d) unsupported\n",
			    sc->sc_dev.dv_xname, __func__, bpp);
		}
		break;
	default:
		printf("%s: %s: only IGA1 is supported\n", sc->sc_dev.dv_xname,
		    __func__);
		break;
	}
}

static uint32_t
uni_get_clkval(struct unichromefb_softc *sc, int clk)
{
	int i;

	for (i = 0; i < NUM_TOTAL_PLL_TABLE; i++) {
		if (clk == pll_value[i].clk) {
			/* XXX only CN900 supported for now */
			return pll_value[i].k800_pll;
		}
	}

	aprint_error("%s: can't find matching PLL value\n",
	    sc->sc_dev.dv_xname);

	return 0;
}

static void
uni_set_vclk(struct unichromefb_softc *sc, uint32_t clk, int iga)
{
	uint8_t val;

	/* hardware reset on */
	uni_wr_mask(sc, VIACR, CR17, 0x00, BIT7);

	switch (iga) {
	case IGA1:
		/* XXX only CN900 is supported */
		uni_wr(sc, VIASR, SR44, clk / 0x10000);
		uni_wr(sc, VIASR, SR45, (clk & 0xffff) / 0x100);
		uni_wr(sc, VIASR, SR46, clk % 0x100);
		break;
	default:
		printf("%s: %s: only IGA1 is supported\n", sc->sc_dev.dv_xname,
		    __func__);
		break;
	}

	/* hardware reset off */
	uni_wr_mask(sc, VIACR, CR17, 0x80, BIT7);

	/* reset pll */
	switch (iga) {
	case IGA1:
		uni_wr_mask(sc, VIASR, SR40, 0x02, BIT1);
		uni_wr_mask(sc, VIASR, SR40, 0x00, BIT1);
		break;
	}

	/* good to go */
	val = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIARMisc);
	val |= (BIT2+BIT3);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIAWMisc, val);

	return;
}

/* XXX */
int
unichromefb_cnattach(void)
{
	return 0;
}
