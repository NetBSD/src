/*	$NetBSD: tx3912video.c,v 1.4 1999/12/23 16:56:16 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "opt_tx39_debug.h"
#include "fb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <machine/bootinfo.h> /* bootinfo */

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx3912videovar.h>
#include <hpcmips/tx/tx3912videoreg.h>

#if NFB > 0
#include <dev/rcons/raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <arch/hpcmips/dev/fbvar.h>
#endif

void tx3912video_framebuffer_init __P((tx_chipset_tag_t, u_int32_t, 
				       u_int32_t));
int  tx3912video_framebuffer_alloc __P((tx_chipset_tag_t, u_int32_t, 
					int, int, int, u_int32_t*, 
					u_int32_t*));
void tx3912video_reset __P((tx_chipset_tag_t));
void tx3912video_resolution_init __P((tx_chipset_tag_t, int, int));
int  tx3912video_fbdepth __P((tx_chipset_tag_t, int));

static u_int32_t framebuffer, framebuffersize;

int	tx3912video_match __P((struct device*, struct cfdata*, void*));
void	tx3912video_attach __P((struct device*, struct device*, void*));
int	tx3912video_print __P((void*, const char*));

struct tx3912video_softc {
	struct device sc_dev;
	u_int32_t sc_fbaddr;
	u_int32_t sc_fbsize;
};

struct fb_attach_args {
	const char *fba_name;
};

struct cfattach tx3912video_ca = {
	sizeof(struct tx3912video_softc), tx3912video_match, 
	tx3912video_attach
};

int
tx3912video_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
tx3912video_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx3912video_softc *sc = (void*)self;
	tx_chipset_tag_t tc = ta->ta_tc;
	struct fb_attach_args fba;
	txreg_t reg;

	printf("\n");
	sc->sc_fbaddr = framebuffer;
	sc->sc_fbsize = framebuffersize;
	printf("TMPR3912 video module [");
	tx3912video_fbdepth(tc, 1);
	printf("] frame buffer: 0x%08x-0x%08x", sc->sc_fbaddr, 
	       sc->sc_fbaddr + sc->sc_fbsize);
	
	if (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) {
		printf("disabled.");
		reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);
		reg &= ~(TX3912_VIDEOCTRL1_DISPON |
			 TX3912_VIDEOCTRL1_ENVID);
		tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);
	}
	printf("\n");

	/* Attach frame buffer device */
#if NFB > 0
	if (!(bootinfo->bi_cnuse & BI_CNUSE_SERIAL)) {
		if (fb_cnattach(0, 0, 0, 0)) {
			panic("tx3912video_attach: can't init fb console");
		}
	}
	fba.fba_name = "fb";
	config_found(self, &fba, tx3912video_print);
#endif
}

int
tx3912video_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

int
tx3912video_init(tc, fb_start, fb_width, fb_height, fb_addr, fb_size, 
		fb_line_bytes)
	tx_chipset_tag_t tc;
	u_int32_t fb_start; /* Physical address */
	int fb_width, fb_height;
	u_int32_t *fb_addr, *fb_size;
	int *fb_line_bytes;
{
 	u_int32_t addr, size;
	int fb_depth;
	
	/* Inquire bit depth */
	fb_depth = tx3912video_fbdepth(tc, 0);
	
	/* Allocate framebuffer area */
	if (tx3912video_framebuffer_alloc(tc, fb_start, fb_width, fb_height,
					 fb_depth, &addr, &size)) {
		return 1;
	}
#if notyet 
	tx3912video_resolution_init(tc, fb_width, fb_height);
#else
	/* Use Windows CE setting. */
#endif
	/* Set DMA transfer address to VID module */
	tx3912video_framebuffer_init(tc, addr, size);
	
	/* Syncronize framebuffer addr to frame signal */
	tx3912video_reset(tc);

	*fb_line_bytes = (fb_width * fb_depth) / 8;
	*fb_addr = addr; /* Phsical address */
	*fb_size = size;

	return 0;
}

 int
tx3912video_framebuffer_alloc(tc, start, h, v, depth, fb_addr, fb_size)
	tx_chipset_tag_t tc;
	u_int32_t start;
	int h, v, depth;
	u_int32_t *fb_addr, *fb_size;
{
	struct extent_fixed ex_fixed[2];
	struct extent *ex;
	u_long addr, size;
	int err;

	/* Calcurate frame buffer size */
	size = (h * v * depth) / 8;
	
	/* Allocate V-RAM area */
	if (!(ex = extent_create("Frame buffer address", start, 
				 start + TX3912_FRAMEBUFFER_MAX, 
				 0, (caddr_t)ex_fixed, sizeof ex_fixed,
 				 EX_NOWAIT))) {
		return 1;
	}
	if((err = extent_alloc_subregion(ex, start, start + size, size, 
					 TX3912_FRAMEBUFFER_ALIGNMENT,
					 TX3912_FRAMEBUFFER_BOUNDARY,
					 EX_FAST|EX_NOWAIT, &addr))) {
		return 1;
	}
	framebuffer = addr;
	framebuffersize = size;
	*fb_addr = addr;
	*fb_size = size;

	return 0;
}

 void
tx3912video_framebuffer_init(tc, fb_addr, fb_size)
	tx_chipset_tag_t tc;
	u_int32_t fb_addr, fb_size;
{
	u_int32_t reg, vaddr, bank, base;

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
	bzero((void*)vaddr, fb_size);
}

 void
tx3912video_resolution_init(tc, h, v)
	tx_chipset_tag_t tc;
	int h;
	int v;
{
	u_int32_t reg, val;
	int split, bit8, horzval, lineval;

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

 int
tx3912video_fbdepth(tc, verbose)
	tx_chipset_tag_t tc;
	int verbose;
{
	u_int32_t reg, val;

	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	
	val = TX3912_VIDEOCTRL1_BITSEL(reg);
	switch (val) {
	case TX3912_VIDEOCTRL1_BITSEL_8BITCOLOR:
		if (verbose)
			printf("8bit color");
		return 8;
	case TX3912_VIDEOCTRL1_BITSEL_4BITGREYSCALE:
		if (verbose)
			printf("4bit greyscale");
		return 4;
	case TX3912_VIDEOCTRL1_BITSEL_2BITGREYSCALE:
		if (verbose)
			printf("2bit greyscale");
		return 2;
	case TX3912_VIDEOCTRL1_BITSEL_MONOCHROME:
		if (verbose)
			printf("monochrome");
		return 1;
	}
	return 0;
}

void
tx3912video_reset(tc)
	tx_chipset_tag_t tc;
{
	u_int32_t reg;
	
	reg = tx_conf_read(tc, TX3912_VIDEOCTRL1_REG);	

	/* Disable video logic at end of this frame */
	reg |= TX3912_VIDEOCTRL1_ENFREEZEFRAME; 
	tx_conf_write(tc, TX3912_VIDEOCTRL1_REG, reg);

	/* Wait for end of frame */
	delay(300 * 1000);

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


