/*	$NetBSD: plumvideo.c,v 1.18.2.2 2001/08/25 06:15:21 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#undef PLUMVIDEODEBUG
#include "opt_tx39_debug.h"
#include "plumohci.h" /* Plum2 OHCI shared memory allocated on V-RAM */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/ioctl.h>
#include <sys/buf.h>
#include <uvm/uvm_extern.h>

#include <dev/cons.h> /* consdev */

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

#ifdef PLUMVIDEODEBUG
int	plumvideo_debug = 1;
#define	DPRINTF(arg) if (plumvideo_debug) printf arg;
#define	DPRINTFN(n, arg) if (plumvideo_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

struct plumvideo_softc {
	struct device sc_dev;
	tx_chipset_tag_t sc_tc;
	plum_chipset_tag_t sc_pc;

	void *sc_powerhook;	/* power management hook */
	int sc_console;

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

int	plumvideo_match(struct device*, struct cfdata*, void*);
void	plumvideo_attach(struct device*, struct device*, void*);

int	plumvideo_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t	plumvideo_mmap(void *, off_t, int);

struct cfattach plumvideo_ca = {
	sizeof(struct plumvideo_softc), plumvideo_match, plumvideo_attach
};

struct hpcfb_accessops plumvideo_ha = {
	plumvideo_ioctl, plumvideo_mmap
};

int	plumvideo_power(void *, int, long, void *);

int	plumvideo_init(struct plumvideo_softc *, int *);
void	plumvideo_hpcfbinit(struct plumvideo_softc *, int);

void	plumvideo_clut_default(struct plumvideo_softc *);
void	plumvideo_clut_set(struct plumvideo_softc *, u_int32_t *, int, int);
void	plumvideo_clut_get(struct plumvideo_softc *, u_int32_t *, int, int);
void	__plumvideo_clut_access(struct plumvideo_softc *,
				void (*)(bus_space_tag_t, bus_space_handle_t));
static void _flush_cache(void) __attribute__((__unused__)); /* !!! */

#ifdef PLUMVIDEODEBUG
void	plumvideo_dump(struct plumvideo_softc*);
#endif

#define ON	1
#define OFF	0

int
plumvideo_match(struct device *parent, struct cfdata *cf, void *aux)
{
	/*
	 * VRAM area also uses as UHOSTC shared RAM.
	 */
	return (2); /* 1st attach group */
}

void
plumvideo_attach(struct device *parent, struct device *self, void *aux)
{
	struct plum_attach_args *pa = aux;
	struct plumvideo_softc *sc = (void*)self;
	struct hpcfb_attach_args ha;
	int console, reverse_flag;

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
		panic("plumvideo_hpcfbinit: not supported color depth\n");
		/* NOTREACHED */
	case 16:
		fb->hf_class = HPCFB_CLASS_RGBCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_order_flags = HPCFB_REVORDER_BYTE;
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
plumvideo_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct plumvideo_softc *sc = (struct plumvideo_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;
	u_int8_t *r, *g, *b;
	u_int32_t *rgb;
	int idx, error;
	size_t cnt;

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
		cmap = (struct wsdisplay_cmap*)data;
		cnt = cmap->count;
		idx = cmap->index;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    !LEGAL_CLUT_INDEX(idx) ||
		    !LEGAL_CLUT_INDEX(idx + cnt -1)) {
			return (EINVAL);
		}

		if (!uvm_useracc(cmap->red, cnt, B_WRITE) ||
		    !uvm_useracc(cmap->green, cnt, B_WRITE) ||
		    !uvm_useracc(cmap->blue, cnt, B_WRITE)) {
			return (EFAULT);
		}

		error = cmap_work_alloc(&r, &g, &b, &rgb, cnt);
		if (error != 0) {
			cmap_work_free(r, g, b, rgb);
			return  (ENOMEM);
		}
		plumvideo_clut_get(sc, rgb, idx, cnt);
		rgb24_decompose(rgb, r, g, b, cnt);

		copyout(r, cmap->red, cnt);
		copyout(g, cmap->green,cnt);
		copyout(b, cmap->blue, cnt);

		cmap_work_free(r, g, b, rgb);

		return (0);
		
	case WSDISPLAYIO_PUTCMAP:
		cmap = (struct wsdisplay_cmap*)data;
		cnt = cmap->count;
		idx = cmap->index;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    !LEGAL_CLUT_INDEX(idx) ||
		    !LEGAL_CLUT_INDEX(idx + cnt -1)) {
			return (EINVAL);
		}

		if (!uvm_useracc(cmap->red, cnt, B_WRITE) ||
		    !uvm_useracc(cmap->green, cnt, B_WRITE) ||
		    !uvm_useracc(cmap->blue, cnt, B_WRITE)) {
			return (EFAULT);
		}

		error = cmap_work_alloc(&r, &g, &b, &rgb, cnt);
		if (error != 0) {
			cmap_work_free(r, g, b, rgb);
			return  (ENOMEM);
		}
		copyin(cmap->red,   r, cnt);
		copyin(cmap->green, g, cnt);
		copyin(cmap->blue,  b, cnt);
		rgb24_compose(rgb, r, g, b, cnt);
		plumvideo_clut_set(sc, rgb, idx, cnt);

		cmap_work_free(r, g, b, rgb);

		return (0);

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
		 * nothing to do because we have only one configration
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
		 * because we have only one unit and one configration
		 */
		return (0);

	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		/* XXX not implemented yet */
		return (EINVAL);
	}

	return (ENOTTY);
}

paddr_t
plumvideo_mmap(void *ctx, off_t offset, int prot)
{
	struct plumvideo_softc *sc = (struct plumvideo_softc *)ctx;

	if (offset < 0 || (sc->sc_fbconf.hf_bytes_per_plane +
			   sc->sc_fbconf.hf_offset) <  offset) {
		return (-1);
	}
	
	return (mips_btop(PLUM_VIDEO_VRAM_IOBASE_PHYSICAL + offset));
}

void
plumvideo_clut_get(struct plumvideo_softc *sc, u_int32_t *rgb, int beg,
		   int cnt)
{
	static void __plumvideo_clut_get(bus_space_tag_t,
					      bus_space_handle_t);
	static void __plumvideo_clut_get(iot, ioh)
		bus_space_tag_t iot;
		bus_space_handle_t ioh;
	{
		int i;
		
		for (i = 0, beg *= 4; i < cnt; i++, beg += 4) {
			*rgb++ = bus_space_read_4(iot, ioh, beg) &
				0x00ffffff;
		}
	}

	KASSERT(rgb);
	KASSERT(LEGAL_CLUT_INDEX(beg));
	KASSERT(LEGAL_CLUT_INDEX(beg + cnt - 1));
	__plumvideo_clut_access(sc, __plumvideo_clut_get);
}

void
plumvideo_clut_set(struct plumvideo_softc *sc, u_int32_t *rgb, int beg,
		   int cnt)
{
	static void __plumvideo_clut_set(bus_space_tag_t,
					      bus_space_handle_t);
	static void __plumvideo_clut_set(iot, ioh)
		bus_space_tag_t iot;
		bus_space_handle_t ioh;
	{
		int i;
		
		for (i = 0, beg *= 4; i < cnt; i++, beg +=4) {
			bus_space_write_4(iot, ioh, beg,
					  *rgb++ & 0x00ffffff);
		}
	}

	KASSERT(rgb);
	KASSERT(LEGAL_CLUT_INDEX(beg));
	KASSERT(LEGAL_CLUT_INDEX(beg + cnt - 1));
	__plumvideo_clut_access(sc, __plumvideo_clut_set);
}

void
plumvideo_clut_default(struct plumvideo_softc *sc)
{
	static void __plumvideo_clut_default(bus_space_tag_t,
						  bus_space_handle_t);
	static void __plumvideo_clut_default(iot, ioh)
		bus_space_tag_t iot;
		bus_space_handle_t ioh;
	{
		const u_int8_t compo6[6] = { 0,  51, 102, 153, 204, 255 };
		const u_int32_t ansi_color[16] = {
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

	__plumvideo_clut_access(sc, __plumvideo_clut_default);
}

void
__plumvideo_clut_access(struct plumvideo_softc *sc, void (*palette_func)
			(bus_space_tag_t, bus_space_handle_t))
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
	(*palette_func) (sc->sc_clutiot, sc->sc_clutioh);

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
_flush_cache()
{
	MachFlushCache();
}

int
plumvideo_power(void *ctx, int type, long id, void *msg)
{
	struct plumvideo_softc *sc = ctx;
	plum_chipset_tag_t pc = sc->sc_pc;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	int why = (int)msg;

	switch (why) {
	case PWR_RESUME:
		if (!sc->sc_console)
			return 0; /* serial console */

		DPRINTF(("%s: ON\n", sc->sc_dev.dv_xname));
		/* power on */
		/* LCD power on and display on */
		plum_power_establish(pc, PLUM_PWR_LCD);
		/* back-light on */
		plum_power_establish(pc, PLUM_PWR_BKL);
		plum_conf_write(regt, regh, PLUM_VIDEO_PLLUM_REG,
				PLUM_VIDEO_PLLUM_MAX);
		break;
	case PWR_SUSPEND:
		/* FALLTHROUGH */
	case PWR_STANDBY:
		DPRINTF(("%s: OFF\n", sc->sc_dev.dv_xname));
		/* back-light off */
		plum_conf_write(regt, regh, PLUM_VIDEO_PLLUM_REG,
				PLUM_VIDEO_PLLUM_MIN);
		plum_power_disestablish(pc, PLUM_PWR_BKL);
		/* power down */
		plum_power_disestablish(pc, PLUM_PWR_LCD);
		break;
	}

	return 0;
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
		bitdisp(reg);
	}
}
#endif /* PLUMVIDEODEBUG */
