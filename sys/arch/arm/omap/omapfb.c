/*	$NetBSD: omapfb.c,v 1.21 2013/02/09 13:28:59 jmcneill Exp $	*/

/*
 * Copyright (c) 2010 Michael Lorenz
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
 * A console driver for OMAP 3530's built-in video controller
 * tested on beagleboard only so far
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omapfb.c,v 1.21 2013/02/09 13:28:59 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <dev/videomode/videomode.h>

#include <sys/bus.h>
#include <arm/omap/omapfbreg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_obioreg.h>
#include <arm/omap/omap3_sdmareg.h>
#include <arm/omap/omap3_sdmavar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/videomode/edidvar.h>

#include "omapdma.h"

struct omapfb_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_regh;
	bus_dmamap_t sc_dmamap;
	bus_dma_segment_t sc_dmamem[1];
	size_t sc_vramsize;

	int sc_width, sc_height, sc_depth, sc_stride;
	int sc_locked;
	void *sc_fbaddr, *sc_vramaddr;
	bus_addr_t sc_fbhwaddr;
	uint32_t *sc_clut;
	uint32_t sc_dispc_config;
	int sc_video_is_on;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode;
	uint8_t sc_cmap_red[256], sc_cmap_green[256], sc_cmap_blue[256];
	void (*sc_putchar)(void *, int, int, u_int, long);

	uint8_t sc_edid_data[1024];
	size_t sc_edid_size;
};

static int	omapfb_match(device_t, cfdata_t, void *);
static void	omapfb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(omapfb, sizeof(struct omapfb_softc),
    omapfb_match, omapfb_attach, NULL, NULL);

static int	omapfb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	omapfb_mmap(void *, void *, off_t, int);
static void	omapfb_init_screen(void *, struct vcons_screen *, int, long *);

static int	omapfb_putcmap(struct omapfb_softc *, struct wsdisplay_cmap *);
static int 	omapfb_getcmap(struct omapfb_softc *, struct wsdisplay_cmap *);
static void	omapfb_restore_palette(struct omapfb_softc *);
static void 	omapfb_putpalreg(struct omapfb_softc *, int, uint8_t,
			    uint8_t, uint8_t);

static int	omapfb_set_depth(struct omapfb_softc *, int);
static void	omapfb_set_video(struct omapfb_softc *, int);

#if NOMAPDMA > 0
static void	omapfb_init(struct omapfb_softc *);
static void	omapfb_wait_idle(struct omapfb_softc *);
static void	omapfb_rectfill(struct omapfb_softc *, int, int, int, int,
			    uint32_t);
static void	omapfb_bitblt(struct omapfb_softc *, int, int, int, int, int,
			    int, int);

static void	omapfb_cursor(void *, int, int, int);
static void	omapfb_putchar(void *, int, int, u_int, long);
static void	omapfb_copycols(void *, int, int, int, int);
static void	omapfb_erasecols(void *, int, int, int, long);
static void	omapfb_copyrows(void *, int, int, int);
static void	omapfb_eraserows(void *, int, int, long);
#endif /* NOMAPDMA > 0 */

struct wsdisplay_accessops omapfb_accessops = {
	omapfb_ioctl,
	omapfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

uint32_t venc_mode_ntsc[] = {
	0x00000000, 0x00000001, 0x00008040, 0x00000359,
	0x0000020c, 0x00000000, 0x043f2631, 0x00000000,
	0x00000102, 0x0000016c, 0x0000012f, 0x00000043,
	0x00000038, 0x00000007, 0x00000001, 0x00000038,
	0x21f07c1f, 0x00000000, 0x01310011, 0x0000f003,
	0x00000000, 0x069300f4, 0x0016020c, 0x00060107,
	0x008e0350, 0x000f0359, 0x01a00000, 0x020701a0,
	0x01ac0024, 0x020d01ac, 0x00000006, 0x03480078,
	0x02060024, 0x0001008a, 0x01ac0106, 0x01060006,
	0x00140001, 0x00010001, 0x00f90000, 0x0000000d,
	0x00000000};

extern const u_char rasops_cmap[768];

static int
omapfb_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if ((obio->obio_addr == -1) || (obio->obio_size == 0))
		return 0;
	return 1;
}

static void
omapfb_attach(device_t parent, device_t self, void *aux)
{
	struct omapfb_softc	*sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct rasops_info	*ri;
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t	dict;
	prop_data_t		edid_data;
	unsigned long		defattr;
	bool			is_console = false;
	uint32_t		sz, reg;
	int			segs, i, j, adr;

	sc->sc_iot = obio->obio_iot;
	sc->sc_dev = self;
	sc->sc_dmat = obio->obio_dmat;
	
	printf(": OMAP onboard video\n");
	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
	    &sc->sc_regh)) {
		aprint_error(": couldn't map register space\n");
		return;
	}

	sc->sc_video_is_on = 1;

	/*
	 * XXX
	 * different u-boot versions initialize the graphics controller in
	 * different ways, so we look for the display resolution in a few
	 * different places...
	 */
	sz = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_SIZE);
	if (sz == 0) {
		sz = bus_space_read_4(sc->sc_iot, sc->sc_regh,
		    OMAPFB_DISPC_SIZE_LCD);
	}
	if (sz == 0) {
		sz = bus_space_read_4(sc->sc_iot, sc->sc_regh,
		    OMAPFB_DISPC_SIZE_DIG);
	}
	
	/* ... and make sure it ends up where we need it */
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_SIZE, sz);

	sc->sc_width = (sz & 0xfff) + 1;
	sc->sc_height = ((sz & 0x0fff0000 ) >> 16) + 1;
	sc->sc_depth = 16;
	sc->sc_stride = sc->sc_width << 1;

	if (sc->sc_width == 1 || sc->sc_height == 1) {
		aprint_error_dev(self, "bogus display size, not attaching\n");
		return;
	}

	printf("%s: firmware set up %d x %d\n", device_xname(self),
	    sc->sc_width, sc->sc_height);
#if 0
	printf("DSS revision: %08x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DSS_REVISION));
#endif	
	dict = device_properties(self);
	prop_dictionary_get_bool(dict, "is_console", &is_console);
	edid_data = prop_dictionary_get(dict, "EDID");

	if (edid_data != NULL) {
		struct edid_info ei;

		sc->sc_edid_size = min(prop_data_size(edid_data), 1024);
		memset(sc->sc_edid_data, 0, sizeof(sc->sc_edid_data));
		memcpy(sc->sc_edid_data, prop_data_data_nocopy(edid_data),
		    sc->sc_edid_size);

		edid_parse(sc->sc_edid_data, &ei);
		edid_print(&ei);
	}

	/* setup video DMA */
	sc->sc_vramsize = (12 << 20) + 0x1000; /* 12MB + CLUT */

	if (bus_dmamem_alloc(sc->sc_dmat, sc->sc_vramsize, 0, 0,
	    sc->sc_dmamem, 1, &segs, BUS_DMA_NOWAIT) != 0) {
		panic("boo!\n");
		aprint_error_dev(sc->sc_dev,
		"failed to allocate video memory\n");
		return;
	}

	if (bus_dmamem_map(sc->sc_dmat, sc->sc_dmamem, 1, sc->sc_vramsize, 
	    &sc->sc_vramaddr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to map video RAM\n");
		return;
	}
	sc->sc_fbaddr = (uint8_t *)sc->sc_vramaddr + 0x1000;
	sc->sc_clut = sc->sc_vramaddr;

	if (bus_dmamap_create(sc->sc_dmat, sc->sc_vramsize, 1, sc->sc_vramsize,
	    0, BUS_DMA_NOWAIT, &sc->sc_dmamap) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to create DMA map\n");
		return;
	}

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_vramaddr,
	    sc->sc_vramsize, NULL, BUS_DMA_NOWAIT) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to load DMA map\n");
		return;
	}

	if (sc->sc_depth == 8) {
		j = 0;
		for (i = 0; i < 256; i++) {
			sc->sc_cmap_red[i] = rasops_cmap[j];
			sc->sc_cmap_green[i] = rasops_cmap[j + 1];
			sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
			j += 3;
		}
	} else {
		for (i = 0; i < 256; i++) {
			sc->sc_cmap_red[i] = i;
			sc->sc_cmap_green[i] = i;
			sc->sc_cmap_blue[i] = i;
		}
	}	
	omapfb_restore_palette(sc);

	/* now that we have video memory, stick it to the video controller */
	
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_SYSCONFIG);
	reg &= ~(OMAP_DISPC_SYSC_STANDBY_MASK | OMAP_DISPC_SYSC_IDLE_MASK);
	reg |= OMAP_DISPC_SYSC_SMART_STANDBY | OMAP_DISPC_SYSC_SMART_IDLE |
	       OMAP_DISPC_SYSC_WAKEUP_ENABLE | OMAP_SYSCONF_AUTOIDLE;
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_SYSCONFIG, reg);

	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DSS_SYSCONFIG, 
	    OMAP_SYSCONF_AUTOIDLE);
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONFIG);
	reg = OMAP_DISPC_CTRL_ACTIVE_MTRX;
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONFIG, reg);
	sc->sc_dispc_config = reg;

	sc->sc_fbhwaddr = sc->sc_dmamem->ds_addr + 0x1000;
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_BASE_0, 
	    sc->sc_fbhwaddr);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_TABLE_BASE, 
	    sc->sc_dmamem->ds_addr);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_POSITION, 
	    0);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_PRELOAD, 0x60);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_GFX_ATTRIBUTES, 
	    OMAP_DISPC_ATTR_ENABLE |
	    OMAP_DISPC_ATTR_BURST_16x32 |
	    /*OMAP_DISPC_ATTR_8BIT*/OMAP_DISPC_ATTR_RGB16
	    | OMAP_DISPC_ATTR_REPLICATION);
#if 0
	printf("dss_control: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DSS_CONTROL));
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DSS_CONTROL,
	    /*OMAP_DSSCTRL_DISPC_CLK_SWITCH |*/
	    OMAP_DSSCTRL_CLOCK_MODE |
	    OMAP_DSSCTRL_VENC_CLOCK_4X |
	    OMAP_DSSCTRL_DAC_DEMEN);
#endif

	/* VENC to NTSC mode */
	adr = OMAPFB_VENC_F_CONTROL;
#if 0
	for (i = 0; i < __arraycount(venc_mode_ntsc); i++) {
		bus_space_write_4(sc->sc_iot, sc->sc_regh, adr, 
		    venc_mode_ntsc[i]);
		adr += 4;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_VENC_F_CONTROL, 
		    venc_mode_ntsc[0]);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_VENC_SYNC_CTRL, 
		    venc_mode_ntsc[2]);
		    
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_DEFAULT_COLOR_1,
	    0x00ff0000);
#endif

	/* now we make sure the video output is actually running */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL,
	    reg | OMAP_DISPC_CTRL_GO_LCD | OMAP_DISPC_CTRL_GO_DIGITAL);

#ifdef OMAPFB_DEBUG
	printf("attr: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_ATTRIBUTES));
	printf("preload: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_PRELOAD));
	printf("config: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_CONFIG));
	printf("control: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_CONTROL));
	printf("dss_control: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DSS_CONTROL));
	printf("threshold: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_FIFO_THRESH));
	printf("GFX size: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_SIZE));
	printf("row inc: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_ROW_INC));
	printf("pixel inc: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_PIXEL_INC));
#endif

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
	    &omapfb_accessops);
	sc->vd.init_screen = omapfb_init_screen;

	/* init engine here */
#if NOMAPDMA > 0
	omapfb_init(sc);
#endif

	ri = &sc->sc_console_screen.scr_ri;
	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
#if NOMAPDMA > 0
	omapfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
	    ri->ri_devcmap[(defattr >> 16) & 0xff]);
#endif
	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

	if (is_console)
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);

	vcons_replay_msgbuf(&sc->sc_console_screen);

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &omapfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
#ifdef OMAPFB_DEBUG
#if NOMAPDMA > 0
	omapfb_rectfill(sc, 100, 100, 100, 100, 0xe000);
	omapfb_rectfill(sc, 100, 200, 100, 100, 0x01f8);
	omapfb_rectfill(sc, 200, 100, 100, 100, 0x01f8);
	omapfb_rectfill(sc, 200, 200, 100, 100, 0xe000);
	omapfb_bitblt(sc, 100, 100, 400, 100, 200, 200, 0);
	/* let's see if we can draw something */
	printf("OMAPDMAC_CDAC: %08x\n", omapdma_read_ch_reg(0, OMAPDMAC_CDAC));
	printf("OMAPDMAC_CSR: %08x\n", omapdma_read_ch_reg(0, OMAPDMAC_CSR));
	printf("OMAPDMAC_CCR: %08x\n", omapdma_read_ch_reg(0, OMAPDMAC_CCR));
#endif
#endif	
}

static int
omapfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct omapfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {

		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_OMAP3;
			return 0;

		case WSDISPLAYIO_GET_BUSID:
			{
				struct wsdisplayio_bus_id *busid;

				busid = data;
				busid->bus_type = WSDISPLAYIO_BUS_SOC;
				return 0;
			}

		case WSDISPLAYIO_GINFO:
			if (ms == NULL)
				return ENODEV;
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = 32;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return omapfb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return omapfb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->sc_width * 4;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;

				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL) {
						omapfb_set_depth(sc, 16);
						vcons_redraw_screen(ms);
					} else {
						omapfb_set_depth(sc, 32);
					}						
				}
			}
			return 0;

		case WSDISPLAYIO_GET_FBINFO:
			{
				struct wsdisplayio_fbinfo *fbi = data;

				fbi->fbi_width = sc->sc_width;
				fbi->fbi_height = sc->sc_height;
				fbi->fbi_stride = sc->sc_width << 2;
				fbi->fbi_bitsperpixel = 32;
				fbi->fbi_pixeltype = WSFB_RGB;
				fbi->fbi_subtype.fbi_rgbmasks.red_offset = 16;
				fbi->fbi_subtype.fbi_rgbmasks.red_size = 8;
				fbi->fbi_subtype.fbi_rgbmasks.green_offset = 8;
				fbi->fbi_subtype.fbi_rgbmasks.green_size = 8;
				fbi->fbi_subtype.fbi_rgbmasks.blue_offset = 0;
				fbi->fbi_subtype.fbi_rgbmasks.blue_size = 8;
				fbi->fbi_subtype.fbi_rgbmasks.alpha_offset = 0;
				fbi->fbi_subtype.fbi_rgbmasks.alpha_size = 0;
				fbi->fbi_flags = 0;
				fbi->fbi_fbsize = sc->sc_vramsize;
				fbi->fbi_fboffset = 0;
				fbi->fbi_flags = WSFB_VRAM_IS_RAM;

			}
			return 0;

		case WSDISPLAYIO_GVIDEO:
			{
				int *on = data;
				*on = sc->sc_video_is_on;
			}
			return 0;

		case WSDISPLAYIO_SVIDEO:
			{
				int *on = data;
				omapfb_set_video(sc, *on);
			}
			return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
omapfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	paddr_t pa = -1;
	struct vcons_data *vd = v;
	struct omapfb_softc *sc = vd->cookie;

	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_vramsize) {
		pa = bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmamem, 1,
		    offset + 0x1000, prot, BUS_DMA_PREFETCHABLE);
		return pa;
	}
	return pa;
}

static void
omapfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct omapfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = (char *)sc->sc_fbaddr;

#if NOMAPDMA < 1
	scr->scr_flags |= VCONS_DONT_READ;
#endif

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;

#if NOMAPDMA > 0
	ri->ri_ops.copyrows = omapfb_copyrows;
	ri->ri_ops.copycols = omapfb_copycols;
	ri->ri_ops.eraserows = omapfb_eraserows;
	ri->ri_ops.erasecols = omapfb_erasecols;
	ri->ri_ops.cursor = omapfb_cursor;
	sc->sc_putchar = ri->ri_ops.putchar;
	ri->ri_ops.putchar = omapfb_putchar;
#endif
}

static int
omapfb_putcmap(struct omapfb_softc *sc, struct wsdisplay_cmap *cm)
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
		omapfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
omapfb_getcmap(struct omapfb_softc *sc, struct wsdisplay_cmap *cm)
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
omapfb_restore_palette(struct omapfb_softc *sc)
{
	int i;

	for (i = 0; i < 256; i++) {
		omapfb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static void
omapfb_putpalreg(struct omapfb_softc *sc, int idx, uint8_t r, uint8_t g,
    uint8_t b)
{
	uint32_t reg;

	if ((idx < 0) || (idx > 255))
		return;
	/* whack the DAC */
	reg = (r << 16) | (g << 8) | b;
	sc->sc_clut[idx] = reg;
	
}

static int
omapfb_set_depth(struct omapfb_softc *sc, int d)
{
	uint32_t reg;

	reg = OMAP_DISPC_ATTR_ENABLE |
	      OMAP_DISPC_ATTR_BURST_16x32 |
	      OMAP_DISPC_ATTR_REPLICATION;
	switch (d) {
		case 16:
			reg |= OMAP_DISPC_ATTR_RGB16;
			break;
		case 32:
			reg |= OMAP_DISPC_ATTR_RGB24;
			break;
		default:
			aprint_error_dev(sc->sc_dev, "unsupported depth (%d)\n", d);
			return EINVAL;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_GFX_ATTRIBUTES, reg); 

	/*
	 * now tell the video controller that we're done mucking around and 
	 * actually update its settings
	 */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL,
	    reg | OMAP_DISPC_CTRL_GO_LCD | OMAP_DISPC_CTRL_GO_DIGITAL);

	sc->sc_depth = d;
	sc->sc_stride = sc->sc_width * (sc->sc_depth >> 3);

	/* clear the screen here */
#if NOMAPDMA > 0
	omapfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height, 0);
#else
	memset(sc->sc_fbaddr, 0, sc->sc_stride * sc->sc_height);
#endif
	return 0;
}

static void
omapfb_set_video(struct omapfb_softc *sc, int on)
{
	uint32_t reg;

	if (on == sc->sc_video_is_on)
		return;
	if (on) {
		bus_space_write_4(sc->sc_iot, sc->sc_regh,
		    OMAPFB_DISPC_CONFIG, sc->sc_dispc_config);
		on = 1;
	} else {
		bus_space_write_4(sc->sc_iot, sc->sc_regh,
		    OMAPFB_DISPC_CONFIG, sc->sc_dispc_config |
		    OMAP_DISPC_CFG_VSYNC_GATED | OMAP_DISPC_CFG_HSYNC_GATED |
		    OMAP_DISPC_CFG_PIXELCLK_GATED |
		    OMAP_DISPC_CFG_PIXELDATA_GATED);
	}

	/*
	 * now tell the video controller that we're done mucking around and 
	 * actually update its settings
	 */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL,
	    reg | OMAP_DISPC_CTRL_GO_LCD | OMAP_DISPC_CTRL_GO_DIGITAL);

	aprint_debug_dev(sc->sc_dev, "%s %08x\n", __func__,
	    bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONFIG));
	sc->sc_video_is_on = on;
}

#if NOMAPDMA > 0
static void
omapfb_init(struct omapfb_softc *sc)
{
	omapdma_write_ch_reg(0, OMAPDMAC_CLNK_CTRL, 0);
	omapdma_write_ch_reg(0, OMAPDMAC_CICRI, 0);
	omapdma_write_ch_reg(0, OMAPDMAC_CSDPI,
	    CSDPI_SRC_BURST_64 | CSDPI_DST_BURST_64 |
	    CSDPI_WRITE_POSTED | CSDPI_DATA_TYPE_16);
}

static void
omapfb_wait_idle(struct omapfb_softc *sc)
{
	while ((omapdma_read_ch_reg(0, OMAPDMAC_CCR) & CCR_WR_ACTIVE) != 0);
}

static void
omapfb_rectfill(struct omapfb_softc *sc, int x, int y, int wi, int he,
     uint32_t colour)
{
	int bpp = sc->sc_depth >> 3;	/* bytes per pixel */
	int width_in_bytes = wi * bpp;
	uint32_t daddr;

	daddr = sc->sc_fbhwaddr + sc->sc_stride * y + x * bpp;
	omapfb_wait_idle(sc);

	/*
	 * stupid hardware
	 * in 32bit mode the DMA controller always writes 0 into the upper
	 * byte, so we can use this mode only if we actually want that
	 */
	if (((colour & 0xff00) == 0) &&
	   (((daddr | width_in_bytes) & 3) == 0)) {
		/*
		 * everything is properly aligned so we can copy stuff in
		 * 32bit chunks instead of pixel by pixel
		 */
		wi = wi >> 1;

		/* just in case */
		colour |= colour << 16;

		omapdma_write_ch_reg(0, OMAPDMAC_CSDPI,
		    CSDPI_SRC_BURST_64 | CSDPI_DST_BURST_64 |
		    CSDPI_DST_PACKED | CSDPI_WRITE_POSTED_EXCEPT_LAST |
		    CSDPI_DATA_TYPE_32);
	} else {
		omapdma_write_ch_reg(0, OMAPDMAC_CSDPI,
		    CSDPI_SRC_BURST_64 | CSDPI_DST_BURST_64 |
		    CSDPI_DST_PACKED | CSDPI_WRITE_POSTED_EXCEPT_LAST |
		    CSDPI_DATA_TYPE_16);
	}

	omapdma_write_ch_reg(0, OMAPDMAC_CEN, wi);
	omapdma_write_ch_reg(0, OMAPDMAC_CFN, he);
	omapdma_write_ch_reg(0, OMAPDMAC_CDSA, daddr);
	omapdma_write_ch_reg(0, OMAPDMAC_CCR,
	    CCR_CONST_FILL_ENABLE | CCR_DST_AMODE_DOUBLE_INDEX |
	    CCR_SRC_AMODE_CONST_ADDR);
	omapdma_write_ch_reg(0, OMAPDMAC_CDEI, 1);
	omapdma_write_ch_reg(0, OMAPDMAC_CDFI,
	    (sc->sc_stride - width_in_bytes) + 1);
	omapdma_write_ch_reg(0, OMAPDMAC_COLOR, colour);
	omapdma_write_ch_reg(0, OMAPDMAC_CCR,
	    CCR_CONST_FILL_ENABLE | CCR_DST_AMODE_DOUBLE_INDEX |
	    CCR_SRC_AMODE_CONST_ADDR | CCR_ENABLE);
}

static void
omapfb_bitblt(struct omapfb_softc *sc, int xs, int ys, int xd, int yd,
    int wi, int he, int rop)
{
	int bpp = sc->sc_depth >> 3;	/* bytes per pixel */
	int width_in_bytes = wi * bpp;
	
	int hstep, vstep;
	uint32_t saddr, daddr;

	/*
	 * TODO:
	 * - use 32bit transfers if we're properly aligned
	 */
	saddr = sc->sc_fbhwaddr + sc->sc_stride * ys + xs * bpp;
	daddr = sc->sc_fbhwaddr + sc->sc_stride * yd + xd * bpp;
		
	if (ys < yd) {
		/* need to go vertically backwards */
		vstep = 1 - (sc->sc_stride + width_in_bytes);
		saddr += sc->sc_stride * (he - 1);
		daddr += sc->sc_stride * (he - 1);
	} else
		vstep = (sc->sc_stride - width_in_bytes) + 1;
	if ((xs < xd) && (ys == yd)) {
		/*
		 * need to go horizontally backwards, only needed if source
		 * and destination pixels are on the same line
		 */
		hstep = 1 - (sc->sc_depth >> 2);
		vstep = sc->sc_stride + bpp * (wi - 1) + 1;
		saddr += bpp * (wi - 1);
		daddr += bpp * (wi - 1);
	} else
		hstep = 1;

	omapfb_wait_idle(sc);
	if (((saddr | daddr | width_in_bytes) & 3) == 0) {
		/*
		 * everything is properly aligned so we can copy stuff in
		 * 32bit chunks instead of pixel by pixel
		 */
		wi = wi >> 1;
		omapdma_write_ch_reg(0, OMAPDMAC_CSDPI,
		    CSDPI_SRC_BURST_64 | CSDPI_DST_BURST_64 |
		    CSDPI_DST_PACKED | CSDPI_WRITE_POSTED_EXCEPT_LAST |
		    CSDPI_DATA_TYPE_32);
	} else {
		omapdma_write_ch_reg(0, OMAPDMAC_CSDPI,
		    CSDPI_SRC_BURST_64 | CSDPI_DST_BURST_64 |
		    CSDPI_DST_PACKED | CSDPI_WRITE_POSTED_EXCEPT_LAST |
		    CSDPI_DATA_TYPE_16);
	}

	omapdma_write_ch_reg(0, OMAPDMAC_CEN, wi);
	omapdma_write_ch_reg(0, OMAPDMAC_CFN, he);
	omapdma_write_ch_reg(0, OMAPDMAC_CSSA, saddr);
	omapdma_write_ch_reg(0, OMAPDMAC_CDSA, daddr);
	omapdma_write_ch_reg(0, OMAPDMAC_CCR,
	    CCR_DST_AMODE_DOUBLE_INDEX |
	    CCR_SRC_AMODE_DOUBLE_INDEX);
	omapdma_write_ch_reg(0, OMAPDMAC_CSEI, hstep);
	omapdma_write_ch_reg(0, OMAPDMAC_CSFI, vstep);
	omapdma_write_ch_reg(0, OMAPDMAC_CDEI, hstep);
	omapdma_write_ch_reg(0, OMAPDMAC_CDFI, vstep);
	omapdma_write_ch_reg(0, OMAPDMAC_CCR,
	    CCR_DST_AMODE_DOUBLE_INDEX |
	    CCR_SRC_AMODE_DOUBLE_INDEX | CCR_ENABLE);
}

static void
omapfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct omapfb_softc *sc = scr->scr_cookie;
	int wi, he, pos;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	pos = col + row * ri->ri_cols;
#ifdef WSDISPLAY_SCROLLSUPPORT
	pos += scr->scr_offset_to_zero;
#endif
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		if (ri->ri_flg & RI_CURSOR) {
			omapfb_putchar(cookie, row, col, scr->scr_chars[pos],
			    scr->scr_attrs[pos]);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			omapfb_putchar(cookie, row, col, scr->scr_chars[pos],
			    scr->scr_attrs[pos] ^ 0x0f0f0000);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}
}

static void
omapfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct omapfb_softc *sc = scr->scr_cookie;

	if (c == 0x20) {
		uint32_t fg, bg, ul; 
		rasops_unpack_attr(attr, &fg, &bg, &ul);
		omapfb_rectfill(sc,
		    ri->ri_xorigin + ri->ri_font->fontwidth * col,
		    ri->ri_yorigin + ri->ri_font->fontheight * row,
		    ri->ri_font->fontwidth,
		    ri->ri_font->fontheight,
		    ri->ri_devcmap[bg]);
		return;
	}
	omapfb_wait_idle(sc);
	sc->sc_putchar(cookie, row, col, c, attr);	
}

static void
omapfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct omapfb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		omapfb_bitblt(sc, xs, y, xd, y, width, height, 12);
	}
}

static void
omapfb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct omapfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		omapfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
omapfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct omapfb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		omapfb_bitblt(sc, x, ys, x, yd, width, height, 12);
	}
}

static void
omapfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct omapfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		omapfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}
#endif /* NOMAPDMA > 0 */
