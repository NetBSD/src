/*	$NetBSD: plumvideo.c,v 1.10 2000/05/21 11:22:25 uch Exp $ */

/*-
 * Copyright (c) 1999, 2000 UCHIYAMA Yasushi.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/ioctl.h>
#include <sys/buf.h>
#include <vm/vm.h>

#include <dev/cons.h> /* consdev */

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumvideoreg.h>

#include <machine/bootinfo.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <arch/hpcmips/dev/video_subr.h>

#include <dev/wscons/wsconsio.h>
#include <arch/hpcmips/dev/hpcfbvar.h>
#include <arch/hpcmips/dev/hpcfbio.h>


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

	plum_chipset_tag_t sc_pc;
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

	int sc_width;
	int sc_height;
	int sc_depth;

	struct hpcfb_fbconf sc_fbconf;
	struct hpcfb_dspconf sc_dspconf;
};

int	plumvideo_match __P((struct device*, struct cfdata*, void*));
void	plumvideo_attach __P((struct device*, struct device*, void*));

int	plumvideo_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	plumvideo_mmap __P((void *, off_t, int));

struct cfattach plumvideo_ca = {
	sizeof(struct plumvideo_softc), plumvideo_match, plumvideo_attach
};

struct hpcfb_accessops plumvideo_ha = {
	plumvideo_ioctl, plumvideo_mmap
};

int	plumvideo_init __P((struct plumvideo_softc*));
void	plumvideo_hpcfbinit __P((struct plumvideo_softc *));

void	plumvideo_clut_default __P((struct plumvideo_softc *));
void	plumvideo_clut_set __P((struct plumvideo_softc *, u_int32_t *, int,
				int));
void	plumvideo_clut_get __P((struct plumvideo_softc *, u_int32_t *, int,
				int));
void	__plumvideo_clut_access __P((struct plumvideo_softc *,
				     void (*) __P((bus_space_tag_t,
						   bus_space_handle_t))));
static void _flush_cache __P((void)) __attribute__((__unused__)); /* !!! */

#ifdef PLUMVIDEODEBUG
void	plumvideo_dump __P((struct plumvideo_softc*));
#endif

int
plumvideo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	/*
	 * VRAM area also uses as UHOSTC shared RAM.
	 */
	return (2); /* 1st attach group */
}

void
plumvideo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct plum_attach_args *pa = aux;
	struct plumvideo_softc *sc = (void*)self;
	struct hpcfb_attach_args ha;
	int console;

	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;
	sc->sc_fbiot = sc->sc_clutiot = sc->sc_bitbltt  = pa->pa_iot;

	printf(": ");
	/*
	 * map register area
	 */
	if (bus_space_map(sc->sc_regt, PLUM_VIDEO_REGBASE, 
			  PLUM_VIDEO_REGSIZE, 0, &sc->sc_regh)) {
		printf(": register map failed\n");
		return;
	}

	/*
	 * Power control
	 */
#ifndef PLUMVIDEODEBUG
	if (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) {
		/* LCD power on and display off */
		plum_power_disestablish(sc->sc_pc, PLUM_PWR_LCD);
		/* power off V-RAM */
		plum_power_disestablish(sc->sc_pc, PLUM_PWR_EXTPW2);
		/* power off LCD */
		plum_power_disestablish(sc->sc_pc, PLUM_PWR_EXTPW1);
		/* power off RAMDAC */
		plum_power_disestablish(sc->sc_pc, PLUM_PWR_EXTPW0);
		/* back-light off */
		plum_power_disestablish(sc->sc_pc, PLUM_PWR_BKL);
	} else 
#endif
	{
		/* LCD power on and display on */
		plum_power_establish(sc->sc_pc, PLUM_PWR_LCD);
		/* supply power to V-RAM */
		plum_power_establish(sc->sc_pc, PLUM_PWR_EXTPW2);
		/* supply power to LCD */
		plum_power_establish(sc->sc_pc, PLUM_PWR_EXTPW1);
		/* back-light on */
		plum_power_establish(sc->sc_pc, PLUM_PWR_BKL);
	}

	/* 
	 *  Initialize LCD controller
	 *	map V-RAM area.
	 *	reinstall bootinfo structure.
	 *	some OHCI shared-buffer hack. XXX
	 */
	if (plumvideo_init(sc) != 0)
		return;

	printf("\n");

#ifdef PLUMVIDEODEBUG
	if (plumvideo_debug)
		plumvideo_dump(sc);
#endif
	/* Attach frame buffer device */
	plumvideo_hpcfbinit(sc);

	console = cn_tab ? 0 : 1;
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
plumvideo_hpcfbinit(sc)
	struct plumvideo_softc *sc;
{
	struct hpcfb_fbconf *fb = &sc->sc_fbconf;
	vaddr_t fbvaddr = (vaddr_t)sc->sc_fbioh;
	
	memset(fb, 0, sizeof(struct hpcfb_fbconf));
	
	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strncpy(fb->hf_name, "PLUM built-in video", HPCFB_MAXNAMELEN);
	/* frame buffer name		*/
	strncpy(fb->hf_conf_name, "LCD", HPCFB_MAXNAMELEN);
	/* configuration name		*/
	fb->hf_height		= sc->sc_height;
	fb->hf_width		= sc->sc_width;
	fb->hf_baseaddr		= mips_ptob(mips_btop(fbvaddr));
	fb->hf_offset		= (u_long)fbvaddr - fb->hf_baseaddr;
	/* frame buffer start offset   	*/
	fb->hf_bytes_per_line	= (sc->sc_width * sc->sc_depth) / NBBY;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= sc->sc_height * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;

	switch (sc->sc_depth) {
	default:
		panic("plumvideo_hpcfbinit: not supported color depth\n");
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
plumvideo_init(sc)
	struct plumvideo_softc *sc;
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;
	size_t vram_size;
	int bpp, vram_pitch;
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
#ifdef PLUM_BIG_OHCI_BUFFER
		printf("(16bpp disabled) ");
		/* FALLTHROUGH */
#else /* PLUM_BIG_OHCI_BUFFER */
		bpp = 16;
		break;
#endif /* PLUM_BIG_OHCI_BUFFER */
	default:
		bootinfo->fb_type = BIFB_D8_FF; /* over ride */
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
	sc->sc_depth = bpp;

	/*
	 * Get display size from WindowsCE setted.
	 */
	sc->sc_width = bootinfo->fb_width = 
		plum_conf_read(regt, regh, PLUM_VIDEO_PLHPX_REG) + 1;
	sc->sc_height = bootinfo->fb_height = 
		plum_conf_read(regt, regh, PLUM_VIDEO_PLVT_REG) -
		plum_conf_read(regt, regh, PLUM_VIDEO_PLVDS_REG);

	/*
	 * set line byte length to bootinfo and LCD controller.
	 */
	bootinfo->fb_line_bytes = (sc->sc_width * bpp) / NBBY;

	vram_pitch = sc->sc_width / (8 / bpp);
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
	vram_size = (sc->sc_width * sc->sc_height * bpp) / NBBY;
	vram_size = mips_round_page(vram_size);

	/*
	 * map V-RAM area.
	 */
	if (bus_space_map(sc->sc_fbiot, PLUM_VIDEO_VRAM_IOBASE,
			  vram_size, 0, &sc->sc_fbioh)) {
		printf(": V-RAM map failed\n");
		return (1);
	}

	bootinfo->fb_addr = (unsigned char *)sc->sc_fbioh;

	return (0);
}

int
plumvideo_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct plumvideo_softc *sc = (struct plumvideo_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;
	u_int8_t *r, *g, *b;
	u_int32_t *rgb;
	int idx, cnt, error;

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

int
plumvideo_mmap(ctx, offset, prot)
	void *ctx;
	off_t offset;
	int prot;
{
	struct plumvideo_softc *sc = (struct plumvideo_softc *)ctx;

	if (offset < 0 || (sc->sc_fbconf.hf_bytes_per_plane +
			   sc->sc_fbconf.hf_offset) <  offset) {
		return (-1);
	}
	
	return (mips_btop(PLUM_VIDEO_VRAM_IOBASE_PHYSICAL + offset));
}

void
plumvideo_clut_get(sc, rgb, beg, cnt)
	struct plumvideo_softc *sc;
	u_int32_t *rgb;
	int beg, cnt;
{
	static void __plumvideo_clut_get __P((bus_space_tag_t,
					      bus_space_handle_t));
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
plumvideo_clut_set(sc, rgb, beg, cnt)
	struct plumvideo_softc *sc;
	u_int32_t *rgb;
	int beg, cnt;
{
	static void __plumvideo_clut_set __P((bus_space_tag_t,
					      bus_space_handle_t));
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
plumvideo_clut_default(sc)
	struct plumvideo_softc *sc;
{
	static void __plumvideo_clut_default __P((bus_space_tag_t,
						  bus_space_handle_t));
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
__plumvideo_clut_access(sc, palette_func)
	struct plumvideo_softc *sc;
	void (*palette_func) __P((bus_space_tag_t, bus_space_handle_t));
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

#ifdef PLUMVIDEODEBUG
void
plumvideo_dump(sc)
	struct plumvideo_softc *sc;
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
