/*	$NetBSD: plumvideo.c,v 1.40.22.1 2012/11/20 03:01:23 tls Exp $ */

/*-
 * Copyright (c) 1999-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plumvideo.c,v 1.40.22.1 2012/11/20 03:01:23 tls Exp $");

#undef PLUMVIDEODEBUG

#include "plumohci.h" /* Plum2 OHCI shared memory allocated on V-RAM */
#include "bivideo.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/ioctl.h>
#include <sys/buf.h>
#include <uvm/uvm_extern.h>

#include <dev/cons.h> /* consdev */

#include <mips/cache.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/config_hook.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumvideoreg.h>

#include <machine/bootinfo.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/hpc/video_subr.h>

#include <dev/wscons/wsconsio.h>
#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/hpcfbio.h>
#if NBIVIDEO > 0
#include <dev/hpc/bivideovar.h>
#endif

#ifdef PLUMVIDEODEBUG
int	plumvideo_debug = 1;
#define	DPRINTF(arg) if (plumvideo_debug) printf arg;
#define	DPRINTFN(n, arg) if (plumvideo_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

struct plumvideo_softc {
	device_t sc_dev;
	tx_chipset_tag_t sc_tc;
	plum_chipset_tag_t sc_pc;

	void *sc_powerhook;	/* power management hook */
	int sc_console;

	int sc_backlight;
	int sc_brightness;
	int sc_max_brightness;

	/* control register */
	bus_space_tag_t sc_regt;
	bus_space_handle_t sc_regh;
	/* frame buffer */
	bus_space_tag_t sc_fbiot;
	bus_space_handle_t sc_fbioh;
	/* clut buffer (8bpp only) */
	bus_space_tag_t sc_clutiot;
	bus_space_handle_t sc_clutioh;
	/* bitblt */
	bus_space_tag_t sc_bitbltt;
	bus_space_handle_t sc_bitblth;

	struct video_chip sc_chip;
	struct hpcfb_fbconf sc_fbconf;
	struct hpcfb_dspconf sc_dspconf;
};

int	plumvideo_match(device_t, cfdata_t, void *);
void	plumvideo_attach(device_t, device_t, void *);

int	plumvideo_ioctl(void *, u_long, void *, int, struct lwp *);
paddr_t	plumvideo_mmap(void *, off_t, int);

CFATTACH_DECL_NEW(plumvideo, sizeof(struct plumvideo_softc),
    plumvideo_match, plumvideo_attach, NULL, NULL);

struct hpcfb_accessops plumvideo_ha = {
	plumvideo_ioctl, plumvideo_mmap
};

int	plumvideo_power(void *, int, long, void *);

int	plumvideo_init(struct plumvideo_softc *, int *);
void	plumvideo_hpcfbinit(struct plumvideo_softc *, int);

void	plumvideo_clut_default(struct plumvideo_softc *);
void	plumvideo_clut_set(struct plumvideo_softc *, u_int32_t *, int, int);
void	plumvideo_clut_get(struct plumvideo_softc *, u_int32_t *, int, int);
void	__plumvideo_clut_access(struct plumvideo_softc *, u_int32_t *, int, int,
	    void (*)(bus_space_tag_t, bus_space_handle_t, u_int32_t *, int, int));
static void _flush_cache(void) __attribute__((__unused__)); /* !!! */
static void plumvideo_init_backlight(struct plumvideo_softc *);
static void plumvideo_backlight(struct plumvideo_softc *, int);
static void plumvideo_brightness(struct plumvideo_softc *, int);

#ifdef PLUMVIDEODEBUG
void	plumvideo_dump(struct plumvideo_softc*);
#endif

#define ON	1
#define OFF	0

int
plumvideo_match(device_t parent, cfdata_t cf, void *aux)
{
	/*
	 * VRAM area also uses as UHOSTC shared RAM.
	 */
	return (2); /* 1st attach group */
}

void
plumvideo_attach(device_t parent, device_t self, void *aux)
{
	struct plum_attach_args *pa = aux;
	struct plumvideo_softc *sc = device_private(self);
	struct hpcfb_attach_args ha;
	int console, reverse_flag;

	sc->sc_dev = self;
	sc->sc_console = console = cn_tab ? 0 : 1;
	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;
	sc->sc_fbiot = sc->sc_clutiot = sc->sc_bitbltt  = pa->pa_iot;

	printf(": ");

	/* map register area */
	if (bus_space_map(sc->sc_regt, PLUM_VIDEO_REGBASE, 
	    PLUM_VIDEO_REGSIZE, 0, &sc->sc_regh)) {
		printf("register map failed\n");
		return;
	}

	/* initialize backlight and brightness values */
	plumvideo_init_backlight(sc);

	/* power control */
	plumvideo_power(sc, 0, 0,
	    (void *)(console ? PWR_RESUME : PWR_SUSPEND));
	/* Add a hard power hook to power saving */
	sc->sc_powerhook = config_hook(CONFIG_HOOK_PMEVENT,
	    CONFIG_HOOK_PMEVENT_HARDPOWER,
	    CONFIG_HOOK_SHARE,
	    plumvideo_power, sc);
	if (sc->sc_powerhook == 0)
		printf("WARNING unable to establish hard power hook");
	
	/* 
	 *  Initialize LCD controller
	 *	map V-RAM area.
	 *	reinstall bootinfo structure.
	 *	some OHCI shared-buffer hack. XXX
	 */
	if (plumvideo_init(sc, &reverse_flag) != 0)
		return;

	printf("\n");

	/* Attach frame buffer device */
	plumvideo_hpcfbinit(sc, reverse_flag);

#ifdef PLUMVIDEODEBUG
	if (plumvideo_debug > 0)
		plumvideo_dump(sc);
	/* attach debug draw routine (debugging use) */
	video_attach_drawfunc(&sc->sc_chip);
	tx_conf_register_video(sc->sc_pc->pc_tc, &sc->sc_chip);
#endif /* PLUMVIDEODEBUG */

	if(console && hpcfb_cnattach(&sc->sc_fbconf) != 0) {
		panic("plumvideo_attach: can't init fb console");
	}

	ha.ha_console = console;
	ha.ha_accessops = &plumvideo_ha;
	ha.ha_accessctx = sc;
	ha.ha_curfbconf = 0;
	ha.ha_nfbconf = 1;
	ha.ha_fbconflist = &sc->sc_fbconf;
	ha.ha_curdspconf = 0;
	ha.ha_ndspconf = 1;
	ha.ha_dspconflist = &sc->sc_dspconf;

	config_found(self, &ha, hpcfbprint);
#if NBIVIDEO > 0
	/* bivideo is no longer need */
	bivideo_dont_attach = 1;
#endif /* NBIVIDEO > 0 */
}

void
plumvideo_hpcfbinit(struct plumvideo_softc *sc, int reverse_flag)
{
	struct hpcfb_fbconf *fb = &sc->sc_fbconf;
	struct video_chip *chip = &sc->sc_chip;
	vaddr_t fbvaddr = (vaddr_t)sc->sc_fbioh;
	int height = chip->vc_fbheight;
	int width = chip->vc_fbwidth;
	int depth = chip->vc_fbdepth;
	
	memset(fb, 0, sizeof(struct hpcfb_fbconf));
	
	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strncpy(fb->hf_name, "PLUM built-in video", HPCFB_MAXNAMELEN);
	/* frame buffer name		*/
	strncpy(fb->hf_conf_name, "LCD", HPCFB_MAXNAMELEN);
	/* configuration name		*/
	fb->hf_height		= height;
	fb->hf_width		= width;
	fb->hf_baseaddr		= (u_long)fbvaddr;
	fb->hf_offset		= (u_long)fbvaddr - mips_ptob(mips_btop(fbvaddr));
	/* frame buffer start offset   	*/
	fb->hf_bytes_per_line	= (width * depth) / NBBY;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= height * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;
	if (reverse_flag)
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;

	switch (depth) {
	default:
		panic("plumvideo_hpcfbinit: not supported color depth");
		/* NOTREACHED */
	case 16:
		fb->hf_class = HPCFB_CLASS_RGBCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 16;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 16;

		fb->hf_class_data_length = sizeof(struct hf_rgb_tag);
		/* reserved for future use */
		fb->hf_u.hf_rgb.hf_flags = 0;

		fb->hf_u.hf_rgb.hf_red_width = 5;
		fb->hf_u.hf_rgb.hf_red_shift = 11;
		fb->hf_u.hf_rgb.hf_green_width = 6;
		fb->hf_u.hf_rgb.hf_green_shift = 5;
		fb->hf_u.hf_rgb.hf_blue_width = 5;
		fb->hf_u.hf_rgb.hf_blue_shift = 0;
		fb->hf_u.hf_rgb.hf_alpha_width = 0;
		fb->hf_u.hf_rgb.hf_alpha_shift = 0;
		break;

	case 8:
		fb->hf_class = HPCFB_CLASS_INDEXCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 8;
		fb->hf_class_data_length = sizeof(struct hf_indexed_tag);
		/* reserved for future use */
		fb->hf_u.hf_indexed.hf_flags = 0;
		break;
	}
}

int
plumvideo_init(struct plumvideo_softc *sc, int *reverse)
{
	struct video_chip *chip = &sc->sc_chip;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;
	size_t vram_size;
	int bpp, width, height, vram_pitch;

	*reverse = video_reverse_color();
	chip->vc_v = sc->sc_pc->pc_tc;
#if notyet
	/* map BitBlt area */
	if (bus_space_map(sc->sc_bitbltt,
	    PLUM_VIDEO_BITBLT_IOBASE,
	    PLUM_VIDEO_BITBLT_IOSIZE, 0, 
	    &sc->sc_bitblth)) {
		printf(": BitBlt map failed\n");
		return (1);
	}
#endif
	reg = plum_conf_read(regt, regh, PLUM_VIDEO_PLGMD_REG);
	
	switch (reg & PLUM_VIDEO_PLGMD_GMODE_MASK) {
	case PLUM_VIDEO_PLGMD_16BPP:		
#if NPLUMOHCI > 0 /* reserve V-RAM area for USB OHCI */
		/* FALLTHROUGH */
#else
		bpp = 16;
		break;
#endif
	default:
		bootinfo->fb_type = *reverse ? BIFB_D8_FF : BIFB_D8_00;
		reg &= ~PLUM_VIDEO_PLGMD_GMODE_MASK;
		plum_conf_write(regt, regh, PLUM_VIDEO_PLGMD_REG, reg);
		reg |= PLUM_VIDEO_PLGMD_8BPP;
		plum_conf_write(regt, regh, PLUM_VIDEO_PLGMD_REG, reg);
#if notyet
		/* change BitBlt color depth */
		plum_conf_write(sc->sc_bitbltt, sc->sc_bitblth, 0x8, 0);
#endif
		/* FALLTHROUGH */
	case PLUM_VIDEO_PLGMD_8BPP:
		bpp = 8;
		break;
	}
	chip->vc_fbdepth = bpp;

	/*
	 * Get display size from WindowsCE setted.
	 */
	chip->vc_fbwidth = width = bootinfo->fb_width = 
	    plum_conf_read(regt, regh, PLUM_VIDEO_PLHPX_REG) + 1;
	chip->vc_fbheight = height = bootinfo->fb_height = 
	    plum_conf_read(regt, regh, PLUM_VIDEO_PLVT_REG) -
	    plum_conf_read(regt, regh, PLUM_VIDEO_PLVDS_REG);

	/*
	 * set line byte length to bootinfo and LCD controller.
	 */
	vram_pitch = bootinfo->fb_line_bytes = (width * bpp) / NBBY;
	plum_conf_write(regt, regh, PLUM_VIDEO_PLPIT1_REG, vram_pitch);
	plum_conf_write(regt, regh, PLUM_VIDEO_PLPIT2_REG,
	    vram_pitch & PLUM_VIDEO_PLPIT2_MASK);
	plum_conf_write(regt, regh, PLUM_VIDEO_PLOFS_REG, vram_pitch);
	
	/*
	 * boot messages and map CLUT(if any).
	 */
	printf("display mode: ");
	switch (bpp) {
	default:
		printf("disabled ");
		break;
	case 8:
		printf("8bpp ");
		/* map CLUT area */
		if (bus_space_map(sc->sc_clutiot,
		    PLUM_VIDEO_CLUT_LCD_IOBASE,
		    PLUM_VIDEO_CLUT_LCD_IOSIZE, 0, 
		    &sc->sc_clutioh)) {
			printf(": CLUT map failed\n");
			return (1);
		}
		/* install default CLUT */
		plumvideo_clut_default(sc);
		break;
	case 16:
		printf("16bpp ");
		break;
	}
	
	/*
	 * calcurate frame buffer size.
	 */
	reg = plum_conf_read(regt, regh, PLUM_VIDEO_PLGMD_REG);
	vram_size = (width * height * bpp) / NBBY;
	vram_size = mips_round_page(vram_size);
	chip->vc_fbsize = vram_size;

	/*
	 * map V-RAM area.
	 */
	if (bus_space_map(sc->sc_fbiot, PLUM_VIDEO_VRAM_IOBASE,
	    vram_size, 0, &sc->sc_fbioh)) {
		printf(": V-RAM map failed\n");
		return (1);
	}

	bootinfo->fb_addr = (unsigned char *)sc->sc_fbioh;
	chip->vc_fbvaddr = (vaddr_t)sc->sc_fbioh;
	chip->vc_fbpaddr = PLUM_VIDEO_VRAM_IOBASE_PHYSICAL;

	return (0);
}

int
plumvideo_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct plumvideo_softc *sc = v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;
	struct wsdisplay_param *dispparam;
	u_int8_t *r, *g, *b;
	u_int32_t *rgb;
	int idx, error;
	size_t cnt;

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
		cmap = (struct wsdisplay_cmap *)data;
		cnt = cmap->count;
		idx = cmap->index;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    !LEGAL_CLUT_INDEX(idx) ||
		    !LEGAL_CLUT_INDEX(idx + cnt - 1)) {
			return (EINVAL);
		}

		error = cmap_work_alloc(&r, &g, &b, &rgb, cnt);
		if (error)
			goto out;
		plumvideo_clut_get(sc, rgb, idx, cnt);
		rgb24_decompose(rgb, r, g, b, cnt);

		error = copyout(r, cmap->red, cnt);
		if (error)
			goto out;
		error = copyout(g, cmap->green, cnt);
		if (error)
			goto out;
		error = copyout(b, cmap->blue, cnt);

out:
		cmap_work_free(r, g, b, rgb);
		return error;
		
	case WSDISPLAYIO_PUTCMAP:
		cmap = (struct wsdisplay_cmap *)data;
		cnt = cmap->count;
		idx = cmap->index;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    !LEGAL_CLUT_INDEX(idx) ||
		    !LEGAL_CLUT_INDEX(idx + cnt - 1)) {
			return (EINVAL);
		}

		error = cmap_work_alloc(&r, &g, &b, &rgb, cnt);
		if (error)
			goto out;
		error = copyin(cmap->red, r, cnt);
		if (error)
			goto out;
		error = copyin(cmap->green, g, cnt);
		if (error)
			goto out;
		error = copyin(cmap->blue, b, cnt);
		if (error)
			goto out;
		rgb24_compose(rgb, r, g, b, cnt);
		plumvideo_clut_set(sc, rgb, idx, cnt);
		goto out;

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_OFF)
			plumvideo_backlight(sc, 0);
		else
			plumvideo_backlight(sc, 1);
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(int *)data = sc->sc_backlight ?
			WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		return 0;

	case WSDISPLAYIO_GETPARAM:
		dispparam = (struct wsdisplay_param *)data;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			dispparam->min = 0;
			dispparam->max = 1;
			dispparam->curval = sc->sc_backlight;
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			if (sc->sc_max_brightness <= 0)
				return EINVAL;
			dispparam->min = 0;
			dispparam->max = sc->sc_max_brightness;
			dispparam->curval = sc->sc_brightness;
			break;
		default:
			return EINVAL;
		}
		return 0;

	case WSDISPLAYIO_SETPARAM:
		dispparam = (struct wsdisplay_param * )data;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			if (dispparam->curval < 0 || 1 < dispparam->curval)
				return EINVAL;
			plumvideo_backlight(sc, dispparam->curval);
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			if (sc->sc_max_brightness <= 0)
				return EINVAL;
			if (dispparam->curval < 0 ||
			    sc->sc_max_brightness < dispparam->curval)
				return EINVAL;
			plumvideo_brightness(sc, dispparam->curval);
			break;
		default:
			return EINVAL;
		}
		return 0;

	case HPCFBIO_GCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			return (EINVAL);
		}
		*fbconf = sc->sc_fbconf;	/* structure assignment */
		return (0);

	case HPCFBIO_SCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			return (EINVAL);
		}
		/*
		 * nothing to do because we have only one configuration
		 */
		return (0);

	case HPCFBIO_GDSPCONF:
		dspconf = (struct hpcfb_dspconf *)data;
		if ((dspconf->hd_unit_index != 0 &&
		    dspconf->hd_unit_index != HPCFB_CURRENT_UNIT) ||
		    (dspconf->hd_conf_index != 0 &&
			dspconf->hd_conf_index != HPCFB_CURRENT_CONFIG)) {
			return (EINVAL);
		}
		*dspconf = sc->sc_dspconf;	/* structure assignment */
		return (0);

	case HPCFBIO_SDSPCONF:
		dspconf = (struct hpcfb_dspconf *)data;
		if ((dspconf->hd_unit_index != 0 &&
		    dspconf->hd_unit_index != HPCFB_CURRENT_UNIT) ||
		    (dspconf->hd_conf_index != 0 &&
			dspconf->hd_conf_index != HPCFB_CURRENT_CONFIG)) {
			return (EINVAL);
		}
		/*
		 * nothing to do
		 * because we have only one unit and one configuration
		 */
		return (0);

	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		/* XXX not implemented yet */
		return (EINVAL);
	}

	return (EPASSTHROUGH);
}

paddr_t
plumvideo_mmap(void *ctx, off_t offset, int prot)
{
	struct plumvideo_softc *sc = ctx;

	if (offset < 0 || (sc->sc_fbconf.hf_bytes_per_plane +
	    sc->sc_fbconf.hf_offset) <  offset) {
		return (-1);
	}
	
	return (mips_btop(PLUM_VIDEO_VRAM_IOBASE_PHYSICAL + offset));
}

static void __plumvideo_clut_get(bus_space_tag_t, bus_space_handle_t,
    u_int32_t *, int, int);
static void __plumvideo_clut_get(bus_space_tag_t iot, bus_space_handle_t ioh,
    u_int32_t *rgb, int beg, int cnt)
{
	int i;
	
	for (i = 0, beg *= 4; i < cnt; i++, beg += 4) {
		*rgb++ = bus_space_read_4(iot, ioh, beg) &
		    0x00ffffff;
	}
}

void
plumvideo_clut_get(struct plumvideo_softc *sc, u_int32_t *rgb, int beg,
    int cnt)
{
	KASSERT(rgb);
	KASSERT(LEGAL_CLUT_INDEX(beg));
	KASSERT(LEGAL_CLUT_INDEX(beg + cnt - 1));
	__plumvideo_clut_access(sc, rgb, beg, cnt, __plumvideo_clut_get);
}

static void __plumvideo_clut_set(bus_space_tag_t, bus_space_handle_t,
    u_int32_t *, int, int);
static void __plumvideo_clut_set(bus_space_tag_t iot, bus_space_handle_t ioh,
    u_int32_t *rgb, int beg, int cnt)
{
	int i;
	
	for (i = 0, beg *= 4; i < cnt; i++, beg +=4) {
		bus_space_write_4(iot, ioh, beg,
		    *rgb++ & 0x00ffffff);
	}
}

void
plumvideo_clut_set(struct plumvideo_softc *sc, u_int32_t *rgb, int beg,
    int cnt)
{
	KASSERT(rgb);
	KASSERT(LEGAL_CLUT_INDEX(beg));
	KASSERT(LEGAL_CLUT_INDEX(beg + cnt - 1));
	__plumvideo_clut_access(sc, rgb, beg, cnt, __plumvideo_clut_set);
}

static void __plumvideo_clut_default(bus_space_tag_t, bus_space_handle_t,
    u_int32_t *, int, int);
static void __plumvideo_clut_default(bus_space_tag_t iot, bus_space_handle_t ioh,
    u_int32_t *rgb, int beg, int cnt)
{
	static const u_int8_t compo6[6] = { 0, 51, 102, 153, 204, 255 };
	static const u_int32_t ansi_color[16] = {
		0x000000, 0xff0000, 0x00ff00, 0xffff00,
		0x0000ff, 0xff00ff, 0x00ffff, 0xffffff,
		0x000000, 0x800000, 0x008000, 0x808000,
		0x000080, 0x800080, 0x008080, 0x808080,
	};
	int i, r, g, b;

	/* ANSI escape sequence */
	for (i = 0; i < 16; i++) {
		bus_space_write_4(iot, ioh, i << 2, ansi_color[i]);
	}
	/* 16 - 31, gray scale */
	for ( ; i < 32; i++) {
		int j = (i - 16) * 17;
		bus_space_write_4(iot, ioh, i << 2, RGB24(j, j, j));
	}
	/* 32 - 247, RGB color */
	for (r = 0; r < 6; r++) {
		for (g = 0; g < 6; g++) {
			for (b = 0; b < 6; b++) {
				bus_space_write_4(iot, ioh, i << 2,
				    RGB24(compo6[r],
					compo6[g],
					compo6[b]));
				i++;
			}
		}
	}
	/* 248 - 245, just white */
	for ( ; i < 256; i++) {
		bus_space_write_4(iot, ioh, i << 2, 0xffffff);
	}
}

void
plumvideo_clut_default(struct plumvideo_softc *sc)
{
	__plumvideo_clut_access(sc, NULL, 0, 256, __plumvideo_clut_default);
}

void
__plumvideo_clut_access(struct plumvideo_softc *sc, u_int32_t *rgb, int beg,
    int cnt, void (*palette_func)(bus_space_tag_t, bus_space_handle_t,
    u_int32_t *, int, int))
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t val, gmode;

	/* display off */
	val = bus_space_read_4(regt, regh, PLUM_VIDEO_PLGMD_REG);
	gmode = val & PLUM_VIDEO_PLGMD_GMODE_MASK;
	val &= ~PLUM_VIDEO_PLGMD_GMODE_MASK;
	bus_space_write_4(regt, regh, PLUM_VIDEO_PLGMD_REG, val);

	/* palette access disable */
	val &= ~PLUM_VIDEO_PLGMD_PALETTE_ENABLE;
	bus_space_write_4(regt, regh, PLUM_VIDEO_PLGMD_REG, val);

	/* change palette mode to CPU */
	val &= ~PLUM_VIDEO_PLGMD_MODE_DISPLAY;
	bus_space_write_4(regt, regh, PLUM_VIDEO_PLGMD_REG, val);

	/* palette access */
	(*palette_func) (sc->sc_clutiot, sc->sc_clutioh, rgb, beg, cnt);

	/* change palette mode to Display */
	val |= PLUM_VIDEO_PLGMD_MODE_DISPLAY;
	bus_space_write_4(regt, regh, PLUM_VIDEO_PLGMD_REG, val);

	/* palette access enable */
	val |= PLUM_VIDEO_PLGMD_PALETTE_ENABLE;
	bus_space_write_4(regt, regh, PLUM_VIDEO_PLGMD_REG, val);

	/* display on */
	val |= gmode;
	bus_space_write_4(regt, regh, PLUM_VIDEO_PLGMD_REG, val);
}

/* !!! */
static void
_flush_cache(void)
{
	mips_dcache_wbinv_all();
	mips_icache_sync_all();
}

int
plumvideo_power(void *ctx, int type, long id, void *msg)
{
	struct plumvideo_softc *sc = ctx;
	int why = (int)msg;

	switch (why) {
	case PWR_RESUME:
		if (!sc->sc_console)
			return (0); /* serial console */

		DPRINTF(("%s: ON\n", device_xname(sc->sc_dev)));
		/* power on */
		plumvideo_backlight(sc, 1);
		break;
	case PWR_SUSPEND:
		/* FALLTHROUGH */
	case PWR_STANDBY:
		DPRINTF(("%s: OFF\n", device_xname(sc->sc_dev)));
		/* power off */
		plumvideo_backlight(sc, 0);
		break;
	}

	return (0);
}

static void
plumvideo_init_backlight(struct plumvideo_softc *sc)
{
	int val;

	val = -1;
	if (config_hook_call(CONFIG_HOOK_GET, 
	    CONFIG_HOOK_POWER_LCDLIGHT, &val) != -1) {
		/* we can get real backlight state */
		sc->sc_backlight = val;
	}

	val = -1;
	if (config_hook_call(CONFIG_HOOK_GET,
	    CONFIG_HOOK_BRIGHTNESS_MAX, &val) != -1) {
		/* we can get real brightness max */
		sc->sc_max_brightness = val;

		val = -1;
		if (config_hook_call(CONFIG_HOOK_GET, 
		    CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
			/* we can get real brightness */
			sc->sc_brightness = val;
		} else {
			sc->sc_brightness = sc->sc_max_brightness;
		}
	}
}

static void
plumvideo_backlight(struct plumvideo_softc *sc, int on)
{
	plum_chipset_tag_t pc = sc->sc_pc;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;

	sc->sc_backlight = on;
	if (on) {
		/* LCD on */
		plum_power_establish(pc, PLUM_PWR_LCD);
		/* backlight on */
		plum_power_establish(pc, PLUM_PWR_BKL);
		plum_conf_write(regt, regh, PLUM_VIDEO_PLLUM_REG,
				PLUM_VIDEO_PLLUM_MAX);
	} else {
		/* backlight off */
		plum_conf_write(regt, regh, PLUM_VIDEO_PLLUM_REG,
				PLUM_VIDEO_PLLUM_MIN);
		plum_power_disestablish(pc, PLUM_PWR_BKL);
		/* LCD off */
		plum_power_disestablish(pc, PLUM_PWR_LCD);
	}
	/* call machine dependent backlight control */
	config_hook_call(CONFIG_HOOK_SET,
			 CONFIG_HOOK_POWER_LCDLIGHT, (void *)on);
}

static void
plumvideo_brightness(struct plumvideo_softc *sc, int val)
{

	sc->sc_brightness = val;
	/* call machine dependent brightness control */
	if (sc->sc_backlight)
		config_hook_call(CONFIG_HOOK_SET,
				 CONFIG_HOOK_BRIGHTNESS, &val);
}

#ifdef PLUMVIDEODEBUG
void
plumvideo_dump(struct plumvideo_softc *sc)
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;

	plumreg_t reg;
	int i;

	for (i = 0; i < 0x160; i += 4) {
		reg = plum_conf_read(regt, regh, i);
		printf("0x%03x %08x", i, reg);
		dbg_bit_print(reg);
	}
}
#endif /* PLUMVIDEODEBUG */
