/*	$NetBSD: tx3912video.c,v 1.27 2002/03/17 19:40:40 atatat Exp $ */

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

#define TX3912VIDEO_DEBUG

#include "hpcfb.h"
#include "bivideo.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <sys/ioctl.h>
#include <sys/buf.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h> /* consdev */

#include <machine/bus.h>
#include <machine/bootinfo.h>
#include <machine/config_hook.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx3912videovar.h>
#include <hpcmips/tx/tx3912videoreg.h>

/* CLUT */
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/hpc/video_subr.h>

#include <dev/wscons/wsconsio.h>
#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/hpcfbio.h>
#if NBIVIDEO > 0
#include <dev/hpc/bivideovar.h>
#endif

#ifdef TX3912VIDEO_DEBUG
int	tx3912video_debug = 1;
#define	DPRINTF(arg) if (tx3912video_debug) printf arg;
#define	DPRINTFN(n, arg) if (tx3912video_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

struct tx3912video_softc {
	struct device sc_dev;
	void *sc_powerhook;	/* power management hook */
	int sc_console;
	struct hpcfb_fbconf sc_fbconf;
	struct hpcfb_dspconf sc_dspconf;
	struct video_chip *sc_chip;
};

/* TX3912 built-in video chip itself */
static struct video_chip tx3912video_chip;

int	tx3912video_power(void *, int, long, void *);
void	tx3912video_framebuffer_init(struct video_chip *);
int	tx3912video_framebuffer_alloc(struct video_chip *, paddr_t, paddr_t *);
void	tx3912video_reset(struct video_chip *);
void	tx3912video_resolution_init(struct video_chip *);
int	tx3912video_match(struct device *, struct cfdata *, void *);
void	tx3912video_attach(struct device *, struct device *, void *);
int	tx3912video_print(void *, const char *);

void	tx3912video_hpcfbinit(struct tx3912video_softc *);
int	tx3912video_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t	tx3912video_mmap(void *, off_t, int);

void	tx3912video_clut_init(struct tx3912video_softc *);
void	tx3912video_clut_install(void *, struct rasops_info *);
void	tx3912video_clut_get(struct tx3912video_softc *, u_int32_t *, int,
	    int);
			     
static int __get_color8(int);
static int __get_color4(int);

struct cfattach tx3912video_ca = {
	sizeof(struct tx3912video_softc), tx3912video_match, 
	tx3912video_attach
};

struct hpcfb_accessops tx3912video_ha = {
	tx3912video_ioctl, tx3912video_mmap, 0, 0, 0, 0,
	tx3912video_clut_install
};

int
tx3912video_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (ATTACH_NORMAL);
}

void
tx3912video_attach(struct device *parent, struct device *self, void *aux)
{
	struct tx3912video_softc *sc = (void *)self;
	struct video_chip *chip;
	const char *depth_print[] = { 
		[TX3912_VIDEOCTRL1_BITSEL_MONOCHROME] = "monochrome",
		[TX3912_VIDEOCTRL1_BITSEL_2BITGREYSCALE] = "2bit greyscale",
		[TX3912_VIDEOCTRL1_BITSEL_4BITGREYSCALE] = "4bit greyscale",
		[TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR] = "8bit color"
	};
	struct hpcfb_attach_args ha;
	tx_chipset_tag_t tc;
	txreg_t val;
	int console;

	sc->sc_console = console = cn_tab ? 0 : 1;
	sc->sc_chip = chip = &tx3912video_chip;

	/* print video module information */
	printf(": %s, frame buffer 0x%08x-0x%08x\n",
	    depth_print[(ffs(chip->vc_fbdepth) - 1) & 0x3],
	    (unsigned)chip->vc_fbpaddr, 
	    (unsigned)(chip->vc_fbpaddr + chip->vc_fbsize));

	/* don't inverse VDAT[3:0] signal */
	tc = chip->vc_v;
	val = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
	val &= ~TX3912_VIDEOCTRL1_INVVID;
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, val);

	/* install default CLUT */
	tx3912video_clut_init(sc);

	/* if serial console, power off video module */
	tx3912video_power(sc, 0, 0, (void *)
	    (console ? PWR_RESUME : PWR_SUSPEND));
	
	/* Add a hard power hook to power saving */
	sc->sc_powerhook = config_hook(CONFIG_HOOK_PMEVENT,
	    CONFIG_HOOK_PMEVENT_HARDPOWER, CONFIG_HOOK_SHARE,
	    tx3912video_power, sc);
	if (sc->sc_powerhook == 0)
		printf("WARNING unable to establish hard power hook");

#ifdef TX3912VIDEO_DEBUG
	/* attach debug draw routine (debugging use) */
	video_attach_drawfunc(sc->sc_chip);
	tx_conf_register_video(tc, sc->sc_chip);
#endif
	
	/* Attach frame buffer device */
	tx3912video_hpcfbinit(sc);

	if (console && hpcfb_cnattach(&sc->sc_fbconf) != 0) {
		panic("tx3912video_attach: can't init fb console");
	}

	ha.ha_console = console;
	ha.ha_accessops = &tx3912video_ha;
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

int
tx3912video_power(void *ctx, int type, long id, void *msg)
{
	struct tx3912video_softc *sc = ctx;
	struct video_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->vc_v;
	int why = (int)msg;
	txreg_t val;

	switch (why) {
	case PWR_RESUME:
		if (!sc->sc_console)
			return (0); /* serial console */

		DPRINTF(("%s: ON\n", sc->sc_dev.dv_xname));
		val = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
		val |= (TX3912_VIDEOCTRL1_DISPON | TX3912_VIDEOCTRL1_ENVID);
		tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, val);
		break;
	case PWR_SUSPEND:
		/* FALLTHROUGH */
	case PWR_STANDBY:
		DPRINTF(("%s: OFF\n", sc->sc_dev.dv_xname));
		val = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
		val &= ~(TX3912_VIDEOCTRL1_DISPON | TX3912_VIDEOCTRL1_ENVID);
		tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, val);
		break;
	}

	return (0);
}

void
tx3912video_hpcfbinit(sc)
	struct tx3912video_softc *sc;
{
	struct video_chip *chip = sc->sc_chip;
	struct hpcfb_fbconf *fb = &sc->sc_fbconf;
	vaddr_t fbvaddr = (vaddr_t)MIPS_PHYS_TO_KSEG1(chip->vc_fbpaddr);
	
	memset(fb, 0, sizeof(struct hpcfb_fbconf));
	
	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strncpy(fb->hf_name, "TX3912 built-in video", HPCFB_MAXNAMELEN);
					/* frame buffer name		*/
	strncpy(fb->hf_conf_name, "LCD", HPCFB_MAXNAMELEN);
					/* configuration name		*/
	fb->hf_height		= chip->vc_fbheight;
	fb->hf_width		= chip->vc_fbwidth;
	fb->hf_baseaddr		= (u_long)fbvaddr;
	fb->hf_offset		= (u_long)fbvaddr -
	    mips_ptob(mips_btop(fbvaddr));
					/* frame buffer start offset   	*/
	fb->hf_bytes_per_line	= (chip->vc_fbwidth * chip->vc_fbdepth)
	    / NBBY;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= chip->vc_fbheight * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;
	if (video_reverse_color())
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;


	switch (chip->vc_fbdepth) {
	default:
		panic("tx3912video_hpcfbinit: not supported color depth\n");
		/* NOTREACHED */
	case 2:
		fb->hf_class = HPCFB_CLASS_GRAYSCALE;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 4;
		fb->hf_pixel_width = 2;
		fb->hf_class_data_length = sizeof(struct hf_gray_tag);
		/* reserved for future use */
		fb->hf_u.hf_gray.hf_flags = 0;
		break;
	case 8:
		fb->hf_order_flags = HPCFB_REVORDER_BYTE | HPCFB_REVORDER_WORD;
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
tx3912video_init(paddr_t fb_start, paddr_t *fb_end)
{
	struct video_chip *chip = &tx3912video_chip;
	tx_chipset_tag_t tc;
	txreg_t reg;
	int fbdepth, reverse, error;
	
	reverse = video_reverse_color();
	chip->vc_v = tc = tx_conf_get_tag();
	
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	
	fbdepth = 1 << (TX3912_VIDEOCTRL1_BITSEL(reg));

	switch (fbdepth) {
	case 2:
		bootinfo->fb_type = reverse ? BIFB_D2_M2L_3 : BIFB_D2_M2L_0;
		break;
	case 4:
		/* XXX should implement rasops4.c */
		fbdepth = 2;
		bootinfo->fb_type = reverse ? BIFB_D2_M2L_3 : BIFB_D2_M2L_0;
		reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	
		TX3912_VIDEOCTRL1_BITSEL_CLR(reg);
		reg = TX3912_VIDEOCTRL1_BITSEL_SET(reg,
		    TX3912_VIDEOCTRL1_BITSEL_2BITGREYSCALE);
		tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);
		break;
	case 8:
		bootinfo->fb_type = reverse ? BIFB_D8_FF : BIFB_D8_00;
		break;
	}

	chip->vc_fbdepth = fbdepth;
	chip->vc_fbwidth = bootinfo->fb_width;
	chip->vc_fbheight= bootinfo->fb_height;

	/* Allocate framebuffer area */
	error = tx3912video_framebuffer_alloc(chip, fb_start, fb_end);
	if (error != 0)
		return (1);

#if notyet 
	tx3912video_resolution_init(chip);
#else
	/* Use Windows CE setting. */
#endif
	/* Set DMA transfer address to VID module */
	tx3912video_framebuffer_init(chip);
	
	/* Syncronize framebuffer addr to frame signal */
	tx3912video_reset(chip);

	bootinfo->fb_line_bytes = (chip->vc_fbwidth * fbdepth) / NBBY;
	bootinfo->fb_addr = (void *)MIPS_PHYS_TO_KSEG1(chip->vc_fbpaddr);
	
	return (0);
}

int
tx3912video_framebuffer_alloc(struct video_chip *chip, paddr_t fb_start,
    paddr_t *fb_end /* buffer allocation hint */)
{
	struct extent_fixed ex_fixed[10];
	struct extent *ex;
	u_long addr, size;
	int error;

	/* calcurate frame buffer size */
	size = (chip->vc_fbwidth * chip->vc_fbheight * chip->vc_fbdepth) /
	    NBBY;

	/* extent V-RAM region */
	ex = extent_create("Frame buffer address", fb_start, *fb_end,
	    0, (caddr_t)ex_fixed, sizeof ex_fixed,
	    EX_NOWAIT);
	if (ex == 0)
		return (1);

	/* Allocate V-RAM area */
	error = extent_alloc_subregion(ex, fb_start, fb_start + size - 1,
	    size, TX3912_FRAMEBUFFER_ALIGNMENT,
	    TX3912_FRAMEBUFFER_BOUNDARY, EX_FAST|EX_NOWAIT, &addr);
	extent_destroy(ex);

	if (error != 0)
		return (1);

	chip->vc_fbpaddr = addr;
	chip->vc_fbvaddr = MIPS_PHYS_TO_KSEG1(addr);
	chip->vc_fbsize = size;
	
	*fb_end = addr + size;

	return (0);
}

void
tx3912video_framebuffer_init(struct video_chip *chip)
{
	u_int32_t fb_addr, fb_size, vaddr, bank, base;
	txreg_t reg;
	tx_chipset_tag_t tc = chip->vc_v;

	fb_addr = chip->vc_fbpaddr;
	fb_size = chip->vc_fbsize;

	/*  XXX currently I don't set DFVAL, so force DF signal toggled on
         *  XXX each frame. */
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
	reg &= ~TX3912_VIDEOCTRL1_DFMODE; 
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);

	/* Set DMA transfer start and end address */
	
	bank = TX3912_VIDEOCTRL3_VIDBANK(fb_addr);
	base = TX3912_VIDEOCTRL3_VIDBASEHI(fb_addr);
	reg = TX3912_VIDEOCTRL3_VIDBANK_SET(0, bank);
	/* Upper address counter */
	reg = TX3912_VIDEOCTRL3_VIDBASEHI_SET(reg, base);
	tx_conf_write(tc, TX3912_VIDEOCTRL3_REG, reg);

	/* Lower address counter  */
	base = TX3912_VIDEOCTRL4_VIDBASELO(fb_addr + fb_size);
	reg = TX3912_VIDEOCTRL4_VIDBASELO_SET(0, base);

	/* Set DF-signal rate */
	reg = TX3912_VIDEOCTRL4_DFVAL_SET(reg, 0); /* XXX not yet*/

	/* Set VIDDONE signal delay after FRAME signal */
	/* XXX not yet*/	
	tx_conf_write(tc, TX3912_VIDEOCTRL4_REG, reg);

	/* Clear frame buffer */
	vaddr = MIPS_PHYS_TO_KSEG1(fb_addr);
	memset((void*)vaddr, 0, fb_size);
}

void
tx3912video_resolution_init(struct video_chip *chip)
{
	int h, v, split, bit8, horzval, lineval;
	tx_chipset_tag_t tc = chip->vc_v;
	txreg_t reg;
	u_int32_t val;
	
	h = chip->vc_fbwidth;
	v = chip->vc_fbheight;
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
	split = reg & TX3912_VIDEOCTRL1_DISPSPLIT;
	bit8  = (TX3912_VIDEOCTRL1_BITSEL(reg) == 
	    TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR);
	val = TX3912_VIDEOCTRL1_BITSEL(reg);

	if ((val == TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR) && !split) {
		/* (LCD horizontal pixels / 8bit) * RGB - 1 */
		horzval = (h / 8) * 3 - 1; 
	} else {
		horzval = h / 4 - 1;
	}
	lineval = (split ? v / 2 : v) - 1;

	/* Video rate */
	/* XXX 
	 *  probably This value should be determined from DFINT and LCDINT 
	 */
	reg = TX3912_VIDEOCTRL2_VIDRATE_SET(0, horzval + 1);
	/* Horizontal size of LCD */
	reg = TX3912_VIDEOCTRL2_HORZVAL_SET(reg, horzval);
	/* # of lines for the LCD */
	reg = TX3912_VIDEOCTRL2_LINEVAL_SET(reg, lineval);
	
	tx_conf_write(tc, TX3912_VIDEOCTRL2_REG, reg);
}

void
tx3912video_reset(struct video_chip *chip)
{
	tx_chipset_tag_t tc = chip->vc_v;
	txreg_t reg;
	
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	

	/* Disable video logic at end of this frame */
	reg |= TX3912_VIDEOCTRL1_ENFREEZEFRAME; 
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);

	/* Wait for end of frame */
	delay(30 * 1000);

	/* Make sure to disable video logic */
	reg &= ~TX3912_VIDEOCTRL1_ENVID;
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);	

	delay(1000);

	/* Enable video logic again */
	reg &= ~TX3912_VIDEOCTRL1_ENFREEZEFRAME; 
	reg |= TX3912_VIDEOCTRL1_ENVID;
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);

	delay(1000);
}

int
tx3912video_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct tx3912video_softc *sc = (struct tx3912video_softc *)v;
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
		tx3912video_clut_get(sc, rgb, idx, cnt);
		rgb24_decompose(rgb, r, g, b, cnt);

		copyout(r, cmap->red, cnt);
		copyout(g, cmap->green,cnt);
		copyout(b, cmap->blue, cnt);

		cmap_work_free(r, g, b, rgb);

		return (0);
		
	case WSDISPLAYIO_PUTCMAP:
		/*
		 * TX3912 can't change CLUT index. R:G:B = 3:3:2
		 */
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

	return (EPASSTHROUGH);
}

paddr_t
tx3912video_mmap(void *ctx, off_t offset, int prot)
{
	struct tx3912video_softc *sc = (struct tx3912video_softc *)ctx;

	if (offset < 0 || (sc->sc_fbconf.hf_bytes_per_plane +
	    sc->sc_fbconf.hf_offset) <  offset) {
		return (-1);
	}

	return (mips_btop(sc->sc_chip->vc_fbpaddr + offset));
}

/*
 * CLUT staff
 */
static const struct {
	int mul, div;
} dither_list [] = {
	[TX3912_VIDEO_DITHER_DUTYCYCLE_1]	= { 1, 1 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_6_7]	= { 6, 7 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_4_5]	= { 4, 5 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_3_4]	= { 3, 4 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_5_7]	= { 5, 7 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_2_3]	= { 2, 3 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_3_5]	= { 3, 5 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_4_7]	= { 4, 7 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_2_4]	= { 2, 4 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_3_7]	= { 3, 7 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_2_5]	= { 2, 5 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_1_3]	= { 1, 3 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_2_7]	= { 2, 7 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_1_5]	= { 1, 5 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_1_7]	= { 1, 7 }, 
	[TX3912_VIDEO_DITHER_DUTYCYCLE_0]	= { 0, 1 }
}, *dlp;

static const int dither_level8[8] = {
	TX3912_VIDEO_DITHER_DUTYCYCLE_0,
	TX3912_VIDEO_DITHER_DUTYCYCLE_2_7,
	TX3912_VIDEO_DITHER_DUTYCYCLE_2_5,
	TX3912_VIDEO_DITHER_DUTYCYCLE_2_4,
	TX3912_VIDEO_DITHER_DUTYCYCLE_3_5,
	TX3912_VIDEO_DITHER_DUTYCYCLE_5_7,
	TX3912_VIDEO_DITHER_DUTYCYCLE_4_5,
	TX3912_VIDEO_DITHER_DUTYCYCLE_1,
};

static const int dither_level4[4] = {
	TX3912_VIDEO_DITHER_DUTYCYCLE_0,
	TX3912_VIDEO_DITHER_DUTYCYCLE_1_3,
	TX3912_VIDEO_DITHER_DUTYCYCLE_5_7,
	TX3912_VIDEO_DITHER_DUTYCYCLE_1,
};

static int
__get_color8(int luti)
{
	KASSERT(luti >=0 && luti < 8);
	dlp = &dither_list[dither_level8[luti]];

	return ((0xff * dlp->mul) / dlp->div);
}

static int
__get_color4(int luti)
{
	KASSERT(luti >=0 && luti < 4);
	dlp = &dither_list[dither_level4[luti]];

	return ((0xff * dlp->mul) / dlp->div);
}

void
tx3912video_clut_get(struct tx3912video_softc *sc, u_int32_t *rgb, int beg,
    int cnt)
{
	int i;

	KASSERT(rgb);
	KASSERT(LEGAL_CLUT_INDEX(beg));
	KASSERT(LEGAL_CLUT_INDEX(beg + cnt - 1));
	
	for (i = beg; i < beg + cnt; i++) {
		*rgb++ =  RGB24(__get_color8((i >> 5) & 0x7),
		    __get_color8((i >> 2) & 0x7),
		    __get_color4(i & 0x3));
	}
}

void
tx3912video_clut_install(void *ctx, struct rasops_info *ri)
{
	struct tx3912video_softc *sc = ctx;
	const int system_cmap[0x10] = {
		TX3912VIDEO_BLACK,
		TX3912VIDEO_RED,
		TX3912VIDEO_GREEN,
		TX3912VIDEO_YELLOW,
		TX3912VIDEO_BLUE,
		TX3912VIDEO_MAGENTA,
		TX3912VIDEO_CYAN,
		TX3912VIDEO_WHITE,
		TX3912VIDEO_DARK_BLACK,
		TX3912VIDEO_DARK_RED,
		TX3912VIDEO_DARK_GREEN,
		TX3912VIDEO_DARK_YELLOW,
		TX3912VIDEO_DARK_BLUE,
		TX3912VIDEO_DARK_MAGENTA,
		TX3912VIDEO_DARK_CYAN,
		TX3912VIDEO_DARK_WHITE,
	};

	KASSERT(ri);
	
	if (sc->sc_chip->vc_fbdepth == 8) {
		/* XXX 2bit gray scale LUT not supported */
		memcpy(ri->ri_devcmap, system_cmap, sizeof system_cmap);
	}
}

void
tx3912video_clut_init(struct tx3912video_softc *sc)
{
	tx_chipset_tag_t tc = sc->sc_chip->vc_v;

	if (sc->sc_chip->vc_fbdepth != 8) {
		return; /* XXX 2bit gray scale LUT not supported */
	}

	/* 
	 * time-based dithering pattern (TOSHIBA recommended pattern)
	 */
	/* 2/3, 1/3 */
	tx_conf_write(tc, TX3912_VIDEOCTRL8_REG, 
	    TX3912_VIDEOCTRL8_PAT2_3_DEFAULT);
	/* 3/4, 2/4 */
	tx_conf_write(tc, TX3912_VIDEOCTRL9_REG, 
	    (TX3912_VIDEOCTRL9_PAT3_4_DEFAULT << 16) |
	    TX3912_VIDEOCTRL9_PAT2_4_DEFAULT);
	/* 4/5, 1/5 */
	tx_conf_write(tc, TX3912_VIDEOCTRL10_REG, 
	    TX3912_VIDEOCTRL10_PAT4_5_DEFAULT);
	/* 3/5, 2/5 */
	tx_conf_write(tc, TX3912_VIDEOCTRL11_REG, 
	    TX3912_VIDEOCTRL11_PAT3_5_DEFAULT);
	/* 6/7, 1/7 */
	tx_conf_write(tc, TX3912_VIDEOCTRL12_REG, 
	    TX3912_VIDEOCTRL12_PAT6_7_DEFAULT);
	/* 5/7, 2/7 */
	tx_conf_write(tc, TX3912_VIDEOCTRL13_REG, 
	    TX3912_VIDEOCTRL13_PAT5_7_DEFAULT);
	/* 4/7, 3/7 */
	tx_conf_write(tc, TX3912_VIDEOCTRL14_REG, 
	    TX3912_VIDEOCTRL14_PAT4_7_DEFAULT);

	/* 
	 * dither-pattern look-up table. (selected by uch)
	 */
	/* red */
	tx_conf_write(tc, TX3912_VIDEOCTRL5_REG,
	    (dither_level8[7] << 28) |
	    (dither_level8[6] << 24) |
	    (dither_level8[5] << 20) |
	    (dither_level8[4] << 16) |
	    (dither_level8[3] << 12) |
	    (dither_level8[2] << 8) |
	    (dither_level8[1] << 4) |
	    (dither_level8[0] << 0));
	/* green */
	tx_conf_write(tc, TX3912_VIDEOCTRL6_REG,
	    (dither_level8[7] << 28) |
	    (dither_level8[6] << 24) |
	    (dither_level8[5] << 20) |
	    (dither_level8[4] << 16) |
	    (dither_level8[3] << 12) |
	    (dither_level8[2] << 8) |
	    (dither_level8[1] << 4) |
	    (dither_level8[0] << 0));
	/* blue (2bit gray scale also use this look-up table) */
	tx_conf_write(tc, TX3912_VIDEOCTRL7_REG,
	    (dither_level4[3] << 12) |
	    (dither_level4[2] << 8) |
	    (dither_level4[1] << 4) |
	    (dither_level4[0] << 0));

	tx3912video_reset(sc->sc_chip);
}
