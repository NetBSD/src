/*	$NetBSD: tx3912video.c,v 1.12 2000/05/08 21:57:58 uch Exp $ */

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
#define TX3912VIDEO_DEBUG

#include "opt_tx39_debug.h"
#include "hpcfb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <sys/ioctl.h>
#include <sys/buf.h>
#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx3912videovar.h>
#include <hpcmips/tx/tx3912videoreg.h>

/* CLUT */
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <arch/hpcmips/dev/video_subr.h>

#include <dev/wscons/wsconsio.h>
#include <arch/hpcmips/dev/hpcfbvar.h>
#include <arch/hpcmips/dev/hpcfbio.h>

static struct tx3912video_chip {
	tx_chipset_tag_t vc_tc;

	paddr_t vc_fbaddr;
	size_t vc_fbsize;
	int vc_fbdepth;
	int vc_fbwidth;
	int vc_fbheight;

	void (*vc_drawline) __P((int, int, int, int)); /* for debug */
	void (*vc_drawdot) __P((int, int)); /* for debug */
} tx3912video_chip;

struct tx3912video_softc {
	struct device sc_dev;
	struct hpcfb_fbconf sc_fbconf;
	struct hpcfb_dspconf sc_dspconf;
	struct tx3912video_chip *sc_chip;
};

void	tx3912video_framebuffer_init __P((struct tx3912video_chip *));
int	tx3912video_framebuffer_alloc __P((struct tx3912video_chip *, paddr_t,
					paddr_t *));
void	tx3912video_reset __P((struct tx3912video_chip *));
void	tx3912video_resolution_init __P((struct tx3912video_chip *));

int	tx3912video_match __P((struct device *, struct cfdata *, void *));
void	tx3912video_attach __P((struct device *, struct device *, void *));
int	tx3912video_print __P((void *, const char *));

void	tx3912video_hpcfbinit __P((struct tx3912video_softc *));
int	tx3912video_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	tx3912video_mmap __P((void *, off_t, int));

void	tx3912video_clut_init __P((struct tx3912video_softc *));
void	tx3912video_clut_install __P((void *, struct rasops_info *));
void	tx3912video_clut_get __P((struct tx3912video_softc *,
				u_int32_t *, int, int));
static int __get_color8 __P((int));
static int __get_color4 __P((int));

struct cfattach tx3912video_ca = {
	sizeof(struct tx3912video_softc), tx3912video_match, 
	tx3912video_attach
};

struct hpcfb_accessops tx3912video_ha = {
	tx3912video_ioctl, tx3912video_mmap, 0, 0, 0, 0,
	tx3912video_clut_install
};

void	__tx3912video_attach_drawfunc __P((struct tx3912video_chip*));

int
tx3912video_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

void
tx3912video_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tx3912video_softc *sc = (void *)self;
	struct tx3912video_chip *chip;
	const char *depth_print[] = { 
		[TX3912_VIDEOCTRL1_BITSEL_MONOCHROME] = "monochrome",
		[TX3912_VIDEOCTRL1_BITSEL_2BITGREYSCALE] = "2bit greyscale",
		[TX3912_VIDEOCTRL1_BITSEL_4BITGREYSCALE] = "4bit greyscale",
		[TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR] = "8bit color"
	};
	struct hpcfb_attach_args ha;
	tx_chipset_tag_t tc;
	txreg_t val;
	int console = (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) ? 0 : 1;

	sc->sc_chip = chip = &tx3912video_chip;

	/* print video module information */
	printf(": %s, frame buffer 0x%08x-0x%08x\n",
	       depth_print[(ffs(chip->vc_fbdepth) - 1) & 0x3],
	       (unsigned)chip->vc_fbaddr, 
	       (unsigned)(chip->vc_fbaddr + chip->vc_fbsize));

	/* don't inverse VDAT[3:0] signal */
	tc = chip->vc_tc;
	val = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
	val &= ~TX3912_VIDEOCTRL1_INVVID;
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, val);

	/* install default CLUT */
	tx3912video_clut_init(sc);

	/* if serial console, power off video module */
#ifndef TX3912VIDEO_DEBUG
	if (!console) {
		tx_chipset_tag_t tc = ta->ta_tc;
		txreg_t reg;
		printf("%s: power off\n", sc->sc_dev.dv_xname);
		reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
		reg &= ~(TX3912_VIDEOCTRL1_DISPON |
			 TX3912_VIDEOCTRL1_ENVID);
		tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);
	}
#endif /* TX3912VIDEO_DEBUG */

	/* attach debug draw routine (debugging use) */
	__tx3912video_attach_drawfunc(sc->sc_chip);
	
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
}

void
tx3912video_hpcfbinit(sc)
	struct tx3912video_softc *sc;
{
	struct tx3912video_chip *chip = sc->sc_chip;
	struct hpcfb_fbconf *fb = &sc->sc_fbconf;
	caddr_t fbcaddr = (caddr_t)MIPS_PHYS_TO_KSEG1(chip->vc_fbaddr);
	
	memset(fb, 0, sizeof(struct hpcfb_fbconf));
	
	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strncpy(fb->hf_name, "TX3912 built-in video", HPCFB_MAXNAMELEN);
					/* frame buffer name		*/
	strncpy(fb->hf_conf_name, "LCD", HPCFB_MAXNAMELEN);
					/* configuration name		*/
	fb->hf_height		= chip->vc_fbheight;
	fb->hf_width		= chip->vc_fbwidth;
	fb->hf_baseaddr		= mips_ptob(mips_btop(fbcaddr));
	fb->hf_offset		= (u_long)fbcaddr - fb->hf_baseaddr;
					/* frame buffer start offset   	*/
	fb->hf_bytes_per_line	= (chip->vc_fbwidth * chip->vc_fbdepth)
		/ NBBY;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= chip->vc_fbheight * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;

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
		fb->hf_u.hf_gray.hf_flags = 0;	/* reserved for future use */
		break;
	case 8:
		fb->hf_class = HPCFB_CLASS_INDEXCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 8;
		fb->hf_class_data_length = sizeof(struct hf_indexed_tag);
		fb->hf_u.hf_indexed.hf_flags = 0; /* reserved for future use */
		break;
	}
}

int
tx3912video_init(fb_start, fb_end)
	paddr_t fb_start, *fb_end;
{
	struct tx3912video_chip *chip = &tx3912video_chip;
	tx_chipset_tag_t tc;
	txreg_t reg;
	int fbdepth;
	int error;
	
	chip->vc_tc = tc = tx_conf_get_tag();
	
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	
	fbdepth = 1 << (TX3912_VIDEOCTRL1_BITSEL(reg));

	switch (fbdepth) {
	case 2:
		bootinfo->fb_type = BIFB_D2_M2L_0;
		break;
	case 4:
		/* XXX should implement rasops4.c */
		fbdepth = 2;
		bootinfo->fb_type = BIFB_D2_M2L_0;
		reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	
		TX3912_VIDEOCTRL1_BITSEL_CLR(reg);
		reg = TX3912_VIDEOCTRL1_BITSEL_SET(
			reg, TX3912_VIDEOCTRL1_BITSEL_2BITGREYSCALE);
		tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);
		break;
	case 8:
		bootinfo->fb_type = BIFB_D8_FF;
		break;
	}

	tx3912video_chip.vc_fbdepth = fbdepth;
	tx3912video_chip.vc_fbwidth = bootinfo->fb_width;
	tx3912video_chip.vc_fbheight= bootinfo->fb_height;

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
	bootinfo->fb_addr = (void *)MIPS_PHYS_TO_KSEG1(chip->vc_fbaddr);
	
	return (0);
}

 int
tx3912video_framebuffer_alloc(chip, fb_start, fb_end)
	struct tx3912video_chip *chip;
	paddr_t fb_start, *fb_end; /* buffer allocation hint */
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
	error = extent_alloc_subregion(ex, fb_start, fb_start + size, size, 
				       TX3912_FRAMEBUFFER_ALIGNMENT,
				       TX3912_FRAMEBUFFER_BOUNDARY,
				       EX_FAST|EX_NOWAIT, &addr);
	extent_destroy(ex);

	if (error != 0) {
		return (1);
	}

	chip->vc_fbaddr = addr;
	chip->vc_fbsize = size;
	
	*fb_end = addr + size;

	return (0);
}

 void
tx3912video_framebuffer_init(chip)
	struct tx3912video_chip *chip;
{
	u_int32_t fb_addr, fb_size, vaddr, bank, base;
	txreg_t reg;
	tx_chipset_tag_t tc = chip->vc_tc;

	fb_addr = chip->vc_fbaddr;
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
tx3912video_resolution_init(chip)
	struct tx3912video_chip *chip;
{
	int h, v, split, bit8, horzval, lineval;
	tx_chipset_tag_t tc = chip->vc_tc;
	txreg_t reg;
	u_int32_t val;
	
	h = chip->vc_fbwidth;
	v = chip->vc_fbheight;
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
	split = reg & TX3912_VIDEOCTRL1_DISPSPLIT;
	bit8  = (TX3912_VIDEOCTRL1_BITSEL(reg) == 
		 TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR);
	val = TX3912_VIDEOCTRL1_BITSEL(reg);

	if ((val == TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR) &&
	    !split) {
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
tx3912video_reset(chip)
	struct tx3912video_chip *chip;
{
	tx_chipset_tag_t tc = chip->vc_tc;
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
tx3912video_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
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
		return (EINVAL);

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
tx3912video_mmap(ctx, offset, prot)
	void *ctx;
	off_t offset;
	int prot;
{
	struct tx3912video_softc *sc = (struct tx3912video_softc *)ctx;

	if (offset < 0 || (sc->sc_fbconf.hf_bytes_per_plane +
			   sc->sc_fbconf.hf_offset) <  offset) {
		return (-1);
	}

	return (mips_btop(sc->sc_chip->vc_fbaddr + offset));
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
__get_color8(luti)
	int luti;
{
	KASSERT(luti >=0 && luti < 8);
	dlp = &dither_list[dither_level8[luti]];

	return ((0xff * dlp->mul) / dlp->div);
}

static int
__get_color4(luti)
	int luti;
{
	KASSERT(luti >=0 && luti < 4);
	dlp = &dither_list[dither_level4[luti]];

	return ((0xff * dlp->mul) / dlp->div);
}

void
tx3912video_clut_get(sc, rgb, beg, cnt)
	struct tx3912video_softc *sc;
	u_int32_t *rgb;
	int beg, cnt;
{
	int i;

	KASSERT(rgb);
	KASSERT(LEGAL_CLUT_INDEX(beg));
	KASSERT(LEGAL_CLUT_INDEX(beg + cnt - 1));
	
	for (i = 0; i < cnt; i++) {
		rgb[i] =  RGB24(__get_color8((i >> 5) & 0x7),
				__get_color8((i >> 2) & 0x7),
				__get_color4(i & 0x3));
	}
}

void
tx3912video_clut_install(ctx, ri)
	void *ctx;
	struct rasops_info *ri;
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
tx3912video_clut_init(sc)
	struct tx3912video_softc *sc;
{
	tx_chipset_tag_t tc = sc->sc_chip->vc_tc;

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
}

/*
 * Debug routines.
 */
void
tx3912video_calibration_pattern()
{
	struct tx3912video_chip *vc = &tx3912video_chip;
	int x, y;

	x = vc->vc_fbwidth - 40;
	y = vc->vc_fbheight - 40;
	tx3912video_line(40, 40, x , 40);
	tx3912video_line(x , 40, x , y );
	tx3912video_line(x , y , 40, y );
	tx3912video_line(40, y , 40, 40);
	tx3912video_line(40, 40, x , y );
	tx3912video_line(x,  40, 40, y );
}

#define BPP2 ({ \
	u_int8_t bitmap; \
	bitmap = *(volatile u_int8_t*)MIPS_PHYS_TO_KSEG1(addr); \
	*(volatile u_int8_t*)MIPS_PHYS_TO_KSEG1(addr) = \
		(bitmap & ~(0x3 << ((3 - (x % 4)) * 2))); \
})

#define BPP4 ({ \
	u_int8_t bitmap; \
	bitmap = *(volatile u_int8_t*)MIPS_PHYS_TO_KSEG1(addr); \
	*(volatile u_int8_t*)MIPS_PHYS_TO_KSEG1(addr) = \
		(bitmap & ~(0xf << ((1 - (x % 2)) * 4))); \
})

#define BPP8 ({ \
	*(volatile u_int8_t*)MIPS_PHYS_TO_KSEG1(addr) = 0xff; \
})

#define BRESENHAM(a, b, c, d, func) ({ \
	u_int32_t fbaddr = vc->vc_fbaddr; \
	u_int32_t fbwidth = vc->vc_fbwidth; \
	u_int32_t fbdepth = vc->vc_fbdepth; \
	len = a, step = b -1; \
	if (step == 0) \
		return; \
	kstep = len == 0 ? 0 : 1; \
	for (i = k = 0, j = step / 2; i <= step; i++) { \
		x = xbase c; \
		y = ybase d; \
		addr = fbaddr + (((y * fbwidth + x) * fbdepth) >> 3); \
		func; \
		j -= len; \
		while (j < 0) { \
			j += step; \
			k += kstep; \
		} \
	} \
})

#define DRAWLINE(func) ({ \
	if (x < 0) { \
		if (y < 0) { \
			if (_y < _x) { \
				BRESENHAM(_y, _x, -i, -k, func); \
			} else { \
				BRESENHAM(_x, _y, -k, -i, func); \
			} \
		} else { \
			if (_y < _x) { \
				BRESENHAM(_y, _x, -i, +k, func); \
			} else { \
				BRESENHAM(_x, _y, -k, +i, func); \
			} \
		} \
	} else { \
		if (y < 0) { \
			if (_y < _x) { \
				BRESENHAM(_y, _x, +i, -k, func); \
			} else { \
				BRESENHAM(_x, _y, +k, -i, func); \
			} \
		} else { \
			if (_y < _x) { \
				BRESENHAM(_y, _x, +i, +k, func); \
			} else { \
				BRESENHAM(_x, _y, +k, +i, func); \
			} \
		} \
	} \
})

#define LINEFUNC(b) \
static void linebpp##b __P((int, int, int, int)); \
static void \
linebpp##b##(x0, y0, x1, y1) \
	int x0, y0, x1, y1; \
{ \
	struct tx3912video_chip *vc = &tx3912video_chip; \
	u_int32_t addr; \
	int i, j, k, len, step, kstep; \
	int x, _x, y, _y; \
	int xbase, ybase; \
	x = x1 - x0; \
	y = y1 - y0; \
	_x = abs(x); \
	_y = abs(y); \
	xbase = x0; \
	ybase = y0; \
	DRAWLINE(BPP##b##); \
}

#define DOTFUNC(b) \
static void dotbpp##b __P((int, int)); \
static void \
dotbpp##b##(x, y) \
	int x, y; \
{ \
	struct tx3912video_chip *vc = &tx3912video_chip; \
	u_int32_t addr; \
	addr = vc->vc_fbaddr + (((y * vc->vc_fbwidth + x) * \
				 vc->vc_fbdepth) >> 3); \
	BPP##b; \
}

static void linebpp_unimpl __P((int, int, int, int));
static void dotbpp_unimpl __P((int, int));
static 
void linebpp_unimpl(x0, y0, x1, y1)
	int x0, y0, x1, y1;
{
	return;
}
static 
void dotbpp_unimpl(x, y)
	int x, y;
{
	return;
}

LINEFUNC(2)
LINEFUNC(4)
LINEFUNC(8)
DOTFUNC(2)
DOTFUNC(4)
DOTFUNC(8)

void
__tx3912video_attach_drawfunc(vc)
	struct tx3912video_chip *vc;
{
	switch (vc->vc_fbdepth) {
	default:
		vc->vc_drawline = linebpp_unimpl;
		vc->vc_drawdot = dotbpp_unimpl;
		break;
	case 8:
		vc->vc_drawline = linebpp8;
		vc->vc_drawdot = dotbpp8;
		break;
	case 4:
		vc->vc_drawline = linebpp4;
		vc->vc_drawdot = dotbpp4;
		break;
	case 2:
		vc->vc_drawline = linebpp2;
		vc->vc_drawdot = dotbpp2;
		break;
	}
}

void
tx3912video_line(x0, y0, x1, y1)
	int x0, y0, x1, y1;
{
	struct tx3912video_chip *vc = &tx3912video_chip;
	vc->vc_drawline(x0, y0, x1, y1);
}

void
tx3912video_dot(x, y)
	int x, y;
{
	struct tx3912video_chip *vc = &tx3912video_chip;
	vc->vc_drawdot(x, y);
}
