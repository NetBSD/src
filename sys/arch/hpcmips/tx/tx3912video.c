/*	$NetBSD: tx3912video.c,v 1.11 2000/05/02 17:50:52 uch Exp $ */

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
#include "hpcfb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <sys/ioctl.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx3912videovar.h>
#include <hpcmips/tx/tx3912videoreg.h>

#include <dev/wscons/wsconsio.h>
#include <arch/hpcmips/dev/hpcfbvar.h>
#include <arch/hpcmips/dev/hpcfbio.h>

#define TX3912VIDEO_DEBUG

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

struct cfattach tx3912video_ca = {
	sizeof(struct tx3912video_softc), tx3912video_match, 
	tx3912video_attach
};

struct hpcfb_accessops tx3912video_ha = {
	tx3912video_ioctl, tx3912video_mmap
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
	int console = (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) ? 0 : 1;

	sc->sc_chip = chip = &tx3912video_chip;

	/* print video module information */
	printf(": %s, frame buffer 0x%08x-0x%08x\n",
	       depth_print[(ffs(chip->vc_fbdepth) - 1) & 0x3],
	       (unsigned)chip->vc_fbaddr, 
	       (unsigned)(chip->vc_fbaddr + chip->vc_fbsize));

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
	strcpy(fb->hf_name, "TX3912 built-in video");
					/* frame buffer name		*/
	strcpy(fb->hf_conf_name, "LCD");
					/* configuration name		*/
	fb->hf_height		= chip->vc_fbheight;
	fb->hf_width		= chip->vc_fbwidth;
	fb->hf_baseaddr		= mips_ptob(mips_btop(fbcaddr));
	fb->hf_offset		= (u_long)fbcaddr - fb->hf_baseaddr;
					/* frame buffer start offset   	*/
	fb->hf_bytes_per_line	= (chip->vc_fbwidth * chip->vc_fbdepth) / NBBY;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= chip->vc_fbheight * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;

	fb->hf_access_flags |= HPCFB_ACCESS_REVERSE; /* XXX */
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
		fb->hf_class = HPCFB_CLASS_INDEXCOLOR; /* XXX */
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

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
		/* XXX not implemented yet */
		return (EINVAL);
		
	case WSDISPLAYIO_PUTCMAP:
		/* XXX not implemented yet */
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
