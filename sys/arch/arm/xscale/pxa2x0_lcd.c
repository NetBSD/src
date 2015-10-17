/* $NetBSD: pxa2x0_lcd.c,v 1.36 2015/10/17 16:34:43 jmcneill Exp $ */

/*
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support PXA2[15]0's integrated LCD controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_lcd.c,v 1.36 2015/10/17 16:34:43 jmcneill Exp $");

#include "opt_pxa2x0_lcd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/kernel.h>			/* for cold */

#include <uvm/uvm_extern.h>

#include <dev/cons.h> 
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h> 
#include <dev/wscons/wscons_callbacks.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <arm/cpufunc.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_lcd.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include "wsdisplay.h"

/*
 * Console variables. These are necessary since console is setup very early,
 * before devices get attached.
 */
struct {
	int				is_console;
	struct pxa2x0_wsscreen_descr	*descr;
	const struct lcd_panel_geometry *geom;
} pxa2x0_lcd_console;

#ifdef PXA2X0_LCD_WRITETHROUGH
int pxa2x0_lcd_writethrough = 1;	/* patchable */
#else
int pxa2x0_lcd_writethrough = 0;
#endif

int		lcdintr(void *);

static void	pxa2x0_lcd_initialize(struct pxa2x0_lcd_softc *, 
		    const struct lcd_panel_geometry *);
#if NWSDISPLAY > 0
static void	pxa2x0_lcd_setup_rasops(struct pxa2x0_lcd_softc *,
		    struct rasops_info *, struct pxa2x0_wsscreen_descr *,
		    const struct lcd_panel_geometry *);
#endif

void
pxa2x0_lcd_geometry(struct pxa2x0_lcd_softc *sc,
    const struct lcd_panel_geometry *info)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint32_t ccr0;
	int lines;

	iot = sc->iot;
	ioh = sc->ioh;
	sc->geometry = info;

	ccr0 = LCCR0_IMASK;
	if (CPU_IS_PXA270)
		ccr0 |= LCCR0_CMDIM|LCCR0_RDSTM|LCCR0_LDDALT;
	if (info->panel_info & LCDPANEL_ACTIVE)
		ccr0 |= LCCR0_PAS;	/* active mode */
	if ((info->panel_info & (LCDPANEL_DUAL|LCDPANEL_ACTIVE))
	    == LCDPANEL_DUAL)
		ccr0 |= LCCR0_SDS; /* dual panel */
	if (info->panel_info & LCDPANEL_MONOCHROME)
		ccr0 |= LCCR0_CMS;
	bus_space_write_4(iot, ioh, LCDC_LCCR0, ccr0);

	bus_space_write_4(iot, ioh, LCDC_LCCR1,
	    (info->panel_width-1)
	    | ((info->hsync_pulse_width-1)<<10)
	    | ((info->end_line_wait-1)<<16)
	    | ((info->beg_line_wait-1)<<24));

	if (info->panel_info & LCDPANEL_DUAL)
		lines = info->panel_height/2 + info->extra_lines;
	else
		lines = info->panel_height + info->extra_lines;

	bus_space_write_4(iot, ioh, LCDC_LCCR2,
	    (lines-1)
	    | (info->vsync_pulse_width<<10)
	    | (info->end_frame_wait<<16)
	    | (info->beg_frame_wait<<24));

	bus_space_write_4(iot, ioh, LCDC_LCCR3,
	    (info->pixel_clock_div<<0)
	    | (info->ac_bias << 8)
	    | ((info->panel_info & 
		(LCDPANEL_VSP|LCDPANEL_HSP|LCDPANEL_PCP|LCDPANEL_OEP))
		<<20)
	    | (4 << 24) /* 16bpp */
	    | ((info->panel_info & LCDPANEL_DPC) ? (1<<27) : 0)
	    );

	bus_space_write_4(iot, ioh, LCDC_LCCR4, (info->pcd_div << 31));
}

/*
 * Initialize the LCD controller.
 */
void
pxa2x0_lcd_initialize(struct pxa2x0_lcd_softc *sc, 
    const struct lcd_panel_geometry *geom)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint32_t lccr0, lscr;
	int nldd;

	iot = sc->iot;
	ioh = sc->ioh;

	/* Check if LCD is enabled before programming, it should not
	 * be enabled while it is being reprogrammed, therefore disable
	 * it first.
	 */
	lccr0 = bus_space_read_4(iot, ioh, LCDC_LCCR0);
	if (lccr0 & LCCR0_ENB) {
		lccr0 |= LCCR0_LDM;
		bus_space_write_4(iot, ioh, LCDC_LCCR0, lccr0);
		lccr0 = bus_space_read_4(iot, ioh, LCDC_LCCR0); /* paranoia */
		lccr0 |= LCCR0_DIS;
		bus_space_write_4(iot, ioh, LCDC_LCCR0, lccr0);
		do {
			lscr = bus_space_read_4(iot, ioh, LCDC_LCSR); 
		} while (!(lscr & LCSR_LDD));
	}

	/* enable clock */
	pxa2x0_clkman_config(CKEN_LCD, 1);

	lccr0 = LCCR0_IMASK;
	if (CPU_IS_PXA270)
		lccr0 |= LCCR0_CMDIM|LCCR0_RDSTM;
	bus_space_write_4(iot, ioh, LCDC_LCCR0, lccr0);

	/*
	 * setup GP[77:58] for LCD
	 */
	/* Always use [FLP]CLK, ACBIAS */
	pxa2x0_gpio_set_function(74, GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(75, GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(76, GPIO_ALT_FN_2_OUT);
	if (!ISSET(sc->flags, FLAG_NOUSE_ACBIAS))
		pxa2x0_gpio_set_function(77, GPIO_ALT_FN_2_OUT);

	if ((geom->panel_info & LCDPANEL_ACTIVE) ||
	    ((geom->panel_info & (LCDPANEL_MONOCHROME|LCDPANEL_DUAL)) ==
	    LCDPANEL_DUAL)) {
		/* active and color dual panel need L_DD[15:0] */
		nldd = 16;
	} else if ((geom->panel_info & LCDPANEL_DUAL) ||
	    !(geom->panel_info & LCDPANEL_MONOCHROME)) {
		/* dual or color need L_DD[7:0] */
		nldd = 8;
	} else {
		/* Otherwise just L_DD[3:0] */
		nldd = 4;
	}

	while (nldd--)
		pxa2x0_gpio_set_function(58 + nldd, GPIO_ALT_FN_2_OUT);

	pxa2x0_lcd_geometry(sc, geom);
}

/*
 * Common driver attachment code.
 */
void
pxa2x0_lcd_attach_sub(struct pxa2x0_lcd_softc *sc, 
    struct pxaip_attach_args *pxa, const struct lcd_panel_geometry *geom)
{
	bus_space_tag_t iot = pxa->pxa_iot;
	bus_space_handle_t ioh;
	int error;

	aprint_normal(": PXA2x0 LCD controller\n");
	aprint_naive("\n");

	sc->n_screens = 0;
	LIST_INIT(&sc->screens);

	/* map controller registers */
	error = bus_space_map(iot, PXA2X0_LCDC_BASE, PXA2X0_LCDC_SIZE, 0, &ioh);
	if (error) {
		aprint_error_dev(sc->dev,
		    "failed to map registers (errno=%d)\n", error);
		return;
	}

	sc->iot = iot;
	sc->ioh = ioh;
	sc->dma_tag = &pxa2x0_bus_dma_tag;

	sc->ih = pxa2x0_intr_establish(PXA2X0_INT_LCD, IPL_BIO, lcdintr, sc);
	if (sc->ih == NULL) {
		aprint_error_dev(sc->dev,
		    "unable to establish interrupt at irq %d\n",
		    PXA2X0_INT_LCD);
		return;
	}

	/* Must disable LCD Controller here, if already enabled. */
	bus_space_write_4(iot, ioh, LCDC_LCCR0, 0);

	pxa2x0_lcd_initialize(sc, geom);

#if NWSDISPLAY > 0
	if (pxa2x0_lcd_console.is_console) {
		struct pxa2x0_wsscreen_descr *descr = pxa2x0_lcd_console.descr;
		struct pxa2x0_lcd_screen *scr;
		struct rasops_info *ri;
		long defattr;

		error = pxa2x0_lcd_new_screen(sc, descr->depth, &scr);
		if (error) {
			aprint_error_dev(sc->dev,
			"unable to create new screen (errno=%d)", error);
			return;
		}

		ri = &scr->rinfo;
		ri->ri_hw = (void *)scr;
		ri->ri_bits = scr->buf_va;
		pxa2x0_lcd_setup_rasops(sc, ri, descr, geom);

		/* assumes 16 bpp */
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

		pxa2x0_lcd_start_dma(sc, scr);
		sc->active = scr;

		wsdisplay_cnattach(&descr->c, ri, ri->ri_ccol, ri->ri_crow,
		    defattr);

		aprint_normal_dev(sc->dev, "console\n");
	}
#endif
}

int
pxa2x0_lcd_cnattach(struct pxa2x0_wsscreen_descr *descr,
    const struct lcd_panel_geometry *geom)
{

	pxa2x0_lcd_console.descr = descr;
	pxa2x0_lcd_console.geom = geom;
	pxa2x0_lcd_console.is_console = 1;

	return 0;
}

/*
 * Interrupt handler.
 */
int
lcdintr(void *arg)
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)arg;
	bus_space_tag_t iot = sc->iot;
	bus_space_handle_t ioh = sc->ioh;
	uint32_t status;

	status = bus_space_read_4(iot, ioh, LCDC_LCSR);
	/* Clear stickey status bits */
	bus_space_write_4(iot, ioh, LCDC_LCSR, status);

	return 1;
}

/*
 * Enable DMA to cause the display to be refreshed periodically.
 * This brings the screen to life...
 */
void
pxa2x0_lcd_start_dma(struct pxa2x0_lcd_softc *sc,
    struct pxa2x0_lcd_screen *scr)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint32_t tmp;
	int val, save;

	iot = sc->iot;
	ioh = sc->ioh;

	bus_dmamap_sync(sc->dma_tag, scr->dma, 0, scr->buf_size,
	    BUS_DMASYNC_PREWRITE);

	save = disable_interrupts(I32_bit);

	switch (scr->depth) {
	case 1: val = 0; break;
	case 2: val = 1; break;
	case 4: val = 2; break;
	case 8: val = 3; break;
	case 16: val = 4; break;
	case 18: val = 5; break;
	case 24: val = 33; break;
	default:
		val = 4; break;		
	}

	tmp = bus_space_read_4(iot, ioh, LCDC_LCCR3);
	if (CPU_IS_PXA270) {
		bus_space_write_4(iot, ioh, LCDC_LCCR3, 
		  (tmp & ~(LCCR3_BPP|LCCR3_BPP3)) | (val << LCCR3_BPP_SHIFT));
	} else {
		bus_space_write_4(iot, ioh, LCDC_LCCR3, 
		    (tmp & ~LCCR3_BPP) | (val << LCCR3_BPP_SHIFT));
	}

	bus_space_write_4(iot, ioh, LCDC_FDADR0, 
	    scr->depth >= 16 ? scr->dma_desc_pa :
	    scr->dma_desc_pa + 2 * sizeof (struct lcd_dma_descriptor));
	bus_space_write_4(iot, ioh, LCDC_FDADR1, 
	    scr->dma_desc_pa + 1 * sizeof (struct lcd_dma_descriptor));

	/* clear status */
	bus_space_write_4(iot, ioh, LCDC_LCSR, 0);

	delay(1000);			/* ??? */

	/* Enable LCDC */
	tmp = bus_space_read_4(iot, ioh, LCDC_LCCR0);
	/*tmp &= ~LCCR0_SFM;*/
	bus_space_write_4(iot, ioh, LCDC_LCCR0, tmp | LCCR0_ENB);

	restore_interrupts(save);
}

/*
 * Disable screen refresh.
 */
static void
pxa2x0_lcd_stop_dma(struct pxa2x0_lcd_softc *sc)
{

	/* Stop LCD DMA after current frame */
	bus_space_write_4(sc->iot, sc->ioh, LCDC_LCCR0,
	    LCCR0_DIS |
	    bus_space_read_4(sc->iot, sc->ioh, LCDC_LCCR0));

	/* wait for disabling done.
	   XXX: use interrupt. */
	while (LCCR0_ENB &
	    bus_space_read_4(sc->iot, sc->ioh, LCDC_LCCR0))
		continue;

	bus_space_write_4(sc->iot, sc->ioh, LCDC_LCCR0,
	    ~LCCR0_DIS &
	    bus_space_read_4(sc->iot, sc->ioh, LCDC_LCCR0));
}

#define _rgb(r,g,b)	(((r)<<11) | ((g)<<5) | b)
#define rgb(r,g,b)	_rgb((r)>>1,g,(b)>>1)

#define L	0x1f			/* low intensity */
#define H	0x3f			/* high intensity */

static uint16_t basic_color_map[] = {
	rgb(	0,   0,   0),		/* black */
	rgb(	L,   0,   0),		/* red */
	rgb(	0,   L,   0),		/* green */
	rgb(	L,   L,   0),		/* brown */
	rgb(	0,   0,   L),		/* blue */
	rgb(	L,   0,   L),		/* magenta */
	rgb(	0,   L,   L),		/* cyan */
	_rgb(0x1c,0x38,0x1c),		/* white */

	rgb(	L,   L,   L),		/* black */
	rgb(	H,   0,   0),		/* red */
	rgb(	0,   H,   0),		/* green */
	rgb(	H,   H,   0),		/* brown */
	rgb(	0,   0,   H),		/* blue */
	rgb(	H,   0,   H),		/* magenta */
	rgb(	0,   H,   H),		/* cyan */
	rgb(	H,   H,   H)
};

#undef H
#undef L

static void
init_palette(uint16_t *buf, int depth)
{
	int i;

	/* convert RGB332 to RGB565 */
	switch (depth) {
	case 8:
	case 4:
#if 0
		for (i=0; i <= 255; ++i) {
			buf[i] = ((9 * ((i>>5) & 0x07)) <<11) |
			    ((9 * ((i>>2) & 0x07)) << 5) |
			    ((21 * (i & 0x03))/2);
		}
#else
		memcpy(buf, basic_color_map, sizeof basic_color_map);
		for (i=16; i < (1<<depth); ++i)
			buf[i] = 0xffff;
#endif
		break;
	case 16:
		/* palette is not needed */
		break;
	default:
		/* other depths are not supported */
		break;
	}
}

/*
 * Create and initialize a new screen buffer.
 */
int
pxa2x0_lcd_new_screen(struct pxa2x0_lcd_softc *sc, int depth,
     struct pxa2x0_lcd_screen **scrpp)
{
	bus_dma_tag_t dma_tag;
	const struct lcd_panel_geometry *geometry;
	struct pxa2x0_lcd_screen *scr = NULL;
	int width, height;
	bus_size_t size;
	int error, palette_size;
	int busdma_flag = (cold ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
	struct lcd_dma_descriptor *desc;
	paddr_t buf_pa, desc_pa;

	dma_tag = sc->dma_tag;
	geometry = sc->geometry;

	width = geometry->panel_width;
	height = geometry->panel_height;
	palette_size = 0;

	switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
		palette_size = (1<<depth) * sizeof (uint16_t);
		/* FALLTHROUGH */
	case 16:
		size = roundup(width,4) * 2 * height;
		break;
	case 18:
	case 24:
		size = roundup(width,4) * 4 * height;
		break;
	case 19:
	case 25:
		aprint_error_dev(sc->dev, "Not supported depth (%d)\n", depth);
		return EINVAL;
	default:
		aprint_error_dev(sc->dev, "Unknown depth (%d)\n", depth);
		return EINVAL;
	}

	scr = malloc(sizeof(*scr), M_DEVBUF, M_NOWAIT);
	if (scr == NULL)
		return ENOMEM;

	memset(scr, 0, sizeof(*scr));

	scr->nsegs = 0;
	scr->depth = depth;
	scr->buf_size = size;
	scr->buf_va = NULL;
	size = roundup(size,16) + 3 * sizeof(struct lcd_dma_descriptor)
	    + palette_size;

	error = bus_dmamem_alloc(dma_tag, size, 16, 0, scr->segs, 1,
	    &scr->nsegs, busdma_flag);

	if (error || scr->nsegs != 1) {
		/* XXX:
		 * Actually we can handle nsegs>1 case by means
		 * of multiple DMA descriptors for a panel.  It
		 * will make code here a bit hairly.
		 */
		if (error == 0)
			error = E2BIG;
		goto bad;
	}

	error = bus_dmamem_map(dma_tag, scr->segs, scr->nsegs, size,
	    (void **)&scr->buf_va,
	    busdma_flag | (pxa2x0_lcd_writethrough ? 0 : BUS_DMA_COHERENT));
	if (error)
		goto bad;

	/* XXX: should we have BUS_DMA_WRITETHROUGH in MI bus_dma(9) API? */
	if (pxa2x0_lcd_writethrough) {
		vaddr_t va, eva;

		va = (vaddr_t)scr->buf_va;
		eva = va + size;
		while (va < eva) {
			/* taken from arm/arm32/bus_dma.c:_bus_dmamem_map() */
			cpu_dcache_wbinv_range(va, PAGE_SIZE);
			cpu_drain_writebuf();
			pt_entry_t * const ptep = vtopte(va);
			const pt_entry_t opte = *ptep;
			const pt_entry_t npte = (opte & ~L2_S_CACHE_MASK)
			    | L2_C;
			l2pte_set(ptep, npte, opte);
			PTE_SYNC(ptep);
			va += PAGE_SIZE;
		}
		cpu_tlb_flushID();
	}

	memset(scr->buf_va, 0, scr->buf_size);

	/* map memory for DMA */
	error = bus_dmamap_create(dma_tag, 1024*1024*2, 1, 1024*1024*2, 0,
	    busdma_flag, &scr->dma);
	if (error)
		goto bad;

	error = bus_dmamap_load(dma_tag, scr->dma, scr->buf_va, size,
	    NULL, busdma_flag);
	if (error)
		goto bad;

	buf_pa = scr->segs[0].ds_addr;
	desc_pa = buf_pa + roundup(size, PAGE_SIZE) - 3 * sizeof(*desc);

	/* make descriptors at the top of mapped memory */
	desc = (struct lcd_dma_descriptor *)(
		(char *)(scr->buf_va) + roundup(size, PAGE_SIZE) -
			  3 * sizeof(*desc));

	desc[0].fdadr = desc_pa;
	desc[0].fsadr = buf_pa;
	desc[0].ldcmd = scr->buf_size;

	if (palette_size) {
		init_palette((uint16_t *)((char *)desc - palette_size), depth);

		desc[2].fdadr = desc_pa; /* chain to panel 0 */
		desc[2].fsadr = desc_pa - palette_size;
		desc[2].ldcmd = palette_size | LDCMD_PAL;
	}

	if (geometry->panel_info & LCDPANEL_DUAL) {
		/* Dual panel */
		desc[1].fdadr = desc_pa + sizeof *desc;
		desc[1].fsadr = buf_pa + scr->buf_size/2;
		desc[0].ldcmd = desc[1].ldcmd = scr->buf_size/2;

	}

#if 0
	desc[0].ldcmd |= LDCMD_SOFINT;
	desc[1].ldcmd |= LDCMD_SOFINT;
#endif

	scr->dma_desc = desc;
	scr->dma_desc_pa = desc_pa;
	scr->map_size = size;		/* used when unmap this. */

	LIST_INSERT_HEAD(&sc->screens, scr, link);
	sc->n_screens++;

	*scrpp = scr;

	return 0;

 bad:
	if (scr) {
		if (scr->buf_va)
			bus_dmamem_unmap(dma_tag, scr->buf_va, size);
		if (scr->nsegs)
			bus_dmamem_free(dma_tag, scr->segs, scr->nsegs);
		free(scr, M_DEVBUF);
	}
	*scrpp = NULL;
	return error;
}

#if NWSDISPLAY > 0
/*
 * Initialize rasops for a screen, as well as struct wsscreen_descr if this
 * is the first screen creation.
 */
static void
pxa2x0_lcd_setup_rasops(struct pxa2x0_lcd_softc *sc, struct rasops_info *rinfo,
    struct pxa2x0_wsscreen_descr *descr,
    const struct lcd_panel_geometry *geom)
{

	rinfo->ri_flg = descr->flags;
	rinfo->ri_depth = descr->depth;
	rinfo->ri_width = geom->panel_width;
	rinfo->ri_height = geom->panel_height;
	rinfo->ri_stride = rinfo->ri_width * rinfo->ri_depth / 8;
#ifdef notyet
	rinfo->ri_wsfcookie = -1;	/* XXX */
#endif

	/* swap B and R */
	if (descr->depth == 16) {
		rinfo->ri_rnum = 5;
		rinfo->ri_rpos = 11;
		rinfo->ri_gnum = 6;
		rinfo->ri_gpos = 5;
		rinfo->ri_bnum = 5;
		rinfo->ri_bpos = 0;
	}

	if (descr->c.nrows == 0) {
		/* get rasops to compute screen size the first time */
		rasops_init(rinfo, 100, 100);
	} else {
		rasops_init(rinfo, descr->c.nrows, descr->c.ncols);
	}

	descr->c.nrows = rinfo->ri_rows;
	descr->c.ncols = rinfo->ri_cols;
	descr->c.capabilities = rinfo->ri_caps;
	descr->c.textops = &rinfo->ri_ops;
}
#endif

/*
 * Power management
 */
void
pxa2x0_lcd_suspend(struct pxa2x0_lcd_softc *sc)
{

	if (sc->active) {
		pxa2x0_lcd_stop_dma(sc);
		pxa2x0_clkman_config(CKEN_LCD, 0);
	}
}

void
pxa2x0_lcd_resume(struct pxa2x0_lcd_softc *sc)
{

	if (sc->active) {
		pxa2x0_lcd_initialize(sc, sc->geometry);
		pxa2x0_lcd_start_dma(sc, sc->active);
	}
}

void
pxa2x0_lcd_power(int why, void *v)
{
	struct pxa2x0_lcd_softc *sc = v;

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		pxa2x0_lcd_suspend(sc);
		break;

	case PWR_RESUME:
		pxa2x0_lcd_resume(sc);
		break;
	}
}

#if NWSDISPLAY > 0
/*
 * Initialize struct wsscreen_descr based on values calculated by 
 * raster operation subsystem.
 */
int
pxa2x0_lcd_setup_wsscreen(struct pxa2x0_wsscreen_descr *descr,
    const struct lcd_panel_geometry *geom,
    const char *fontname)
{
	int width = geom->panel_width;
	int height = geom->panel_height;
	int cookie = -1;
	struct rasops_info rinfo;

	memset(&rinfo, 0, sizeof rinfo);

	if (fontname) {
		wsfont_init();
		cookie = wsfont_find(fontname, 0, 0, 0, 
		    WSDISPLAY_FONTORDER_L2R, WSDISPLAY_FONTORDER_L2R,
		    WSFONT_FIND_BITMAP);
		if (cookie < 0 ||
		    wsfont_lock(cookie, &rinfo.ri_font))
			return -1;
	}
	else {
		/* let rasops_init() choose any font */
	}

	/* let rasops_init calculate # of cols and rows in character */
	rinfo.ri_flg = 0;
	rinfo.ri_depth = descr->depth;
	rinfo.ri_bits = NULL;
	rinfo.ri_width = width;
	rinfo.ri_height = height;
	rinfo.ri_stride = width * rinfo.ri_depth / 8;
#ifdef	CPU_XSCALE_PXA270
	if (rinfo.ri_depth > 16)
		rinfo.ri_stride = width * 4;
#endif
	rinfo.ri_wsfcookie = cookie;

	rasops_init(&rinfo, 100, 100);

	descr->c.nrows = rinfo.ri_rows;
	descr->c.ncols = rinfo.ri_cols;
	descr->c.capabilities = rinfo.ri_caps;

	return cookie;
}

int
pxa2x0_lcd_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct pxa2x0_lcd_softc *sc = v;
	struct pxa2x0_lcd_screen *scr = cookie, *old;
	
	old = sc->active;
	if (old == scr)
		return 0;

	if (old)
		pxa2x0_lcd_stop_dma(sc);
	
	pxa2x0_lcd_start_dma(sc, scr);

	sc->active = scr;
	return 0;
}

int
pxa2x0_lcd_alloc_screen(void *v, const struct wsscreen_descr *_type,
    void **cookiep, int *curxp, int *curyp, long *attrp)
{
	struct pxa2x0_lcd_softc *sc = v;
	struct pxa2x0_lcd_screen *scr;
	const struct pxa2x0_wsscreen_descr *type =
		(const struct pxa2x0_wsscreen_descr *)_type;
	int error;

	error = pxa2x0_lcd_new_screen(sc, type->depth, &scr);
	if (error)
		return error;

	/*
	 * initialize raster operation for this screen.
	 */
	scr->rinfo.ri_flg = 0;
	scr->rinfo.ri_depth = type->depth;
	scr->rinfo.ri_bits = scr->buf_va;
	scr->rinfo.ri_width = sc->geometry->panel_width;
	scr->rinfo.ri_height = sc->geometry->panel_height;
	scr->rinfo.ri_stride = scr->rinfo.ri_width * scr->rinfo.ri_depth / 8;
#ifdef CPU_XSCALE_PXA270
	if (scr->rinfo.ri_depth > 16)
		scr->rinfo.ri_stride = scr->rinfo.ri_width * 4;
#endif
	scr->rinfo.ri_wsfcookie = -1;	/* XXX */

	rasops_init(&scr->rinfo, type->c.nrows, type->c.ncols);

	(*scr->rinfo.ri_ops.allocattr)(&scr->rinfo, 0, 0, 0, attrp);

	*cookiep = scr;
	*curxp = 0;
	*curyp = 0;

	return 0;
}

void
pxa2x0_lcd_free_screen(void *v, void *cookie)
{
	struct pxa2x0_lcd_softc *sc = v;
	struct pxa2x0_lcd_screen *scr = cookie;

	LIST_REMOVE(scr, link);
	sc->n_screens--;
	if (scr == sc->active) {
		/* at first, we need to stop LCD DMA */
		sc->active = NULL;

		printf("lcd_free on active screen\n");

		pxa2x0_lcd_stop_dma(sc);
	}

	if (scr->buf_va)
		bus_dmamem_unmap(sc->dma_tag, scr->buf_va, scr->map_size);
	if (scr->nsegs > 0)
		bus_dmamem_free(sc->dma_tag, scr->segs, scr->nsegs);
	free(scr, M_DEVBUF);
}

int
pxa2x0_lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct pxa2x0_lcd_softc *sc = v;
	struct pxa2x0_lcd_screen *scr = sc->active;  /* ??? */
	struct wsdisplay_fbinfo *wsdisp_info;
	uint32_t ccr0;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_PXALCD;
		return 0;

	case WSDISPLAYIO_GINFO:
		wsdisp_info = (struct wsdisplay_fbinfo *)data;
		wsdisp_info->height = sc->geometry->panel_height;
		wsdisp_info->width = sc->geometry->panel_width;
		wsdisp_info->depth = scr->depth;
		wsdisp_info->cmsize = 0;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = scr->rinfo.ri_stride;
		return 0;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		return EPASSTHROUGH;	/* XXX Colormap */

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON) {
		  /* turn it on */
		}
		else {
		  /* start LCD shutdown */
		  /* sleep until interrupt */
		}
		return 0;

	case WSDISPLAYIO_GVIDEO:
		ccr0 = bus_space_read_4(sc->iot, sc->ioh, LCDC_LCCR0);
		*(u_int *)data = (ccr0 & (LCCR0_ENB|LCCR0_DIS)) == LCCR0_ENB ?
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		return 0;

	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return EPASSTHROUGH;	/* XXX */
	}

	return EPASSTHROUGH;
}

paddr_t
pxa2x0_lcd_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct pxa2x0_lcd_softc *sc = v;
	struct pxa2x0_lcd_screen *scr = sc->active;  /* ??? */

	if (scr == NULL)
		return -1;

	if (offset < 0 ||
	    offset >= scr->rinfo.ri_stride * scr->rinfo.ri_height)
		return -1;

	return bus_dmamem_mmap(sc->dma_tag, scr->segs, scr->nsegs,
	    offset, prot,
	    BUS_DMA_WAITOK | (pxa2x0_lcd_writethrough ? 0 : BUS_DMA_COHERENT));
}


static void
pxa2x0_lcd_cursor(void *cookie, int on, int row, int col)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	(*scr->rinfo.ri_ops.cursor)(&scr->rinfo, on, row, col);
}

static int
pxa2x0_lcd_mapchar(void *cookie, int c, unsigned int *cp)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	return (*scr->rinfo.ri_ops.mapchar)(&scr->rinfo, c, cp);
}

static void
pxa2x0_lcd_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	(*scr->rinfo.ri_ops.putchar)(&scr->rinfo, row, col, uc, attr);
}

static void
pxa2x0_lcd_copycols(void *cookie, int row, int src, int dst, int num)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	(*scr->rinfo.ri_ops.copycols)(&scr->rinfo, row, src, dst, num);
}

static void
pxa2x0_lcd_erasecols(void *cookie, int row, int col, int num, long attr)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	(*scr->rinfo.ri_ops.erasecols)(&scr->rinfo, row, col, num, attr);
}

static void
pxa2x0_lcd_copyrows(void *cookie, int src, int dst, int num)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	(*scr->rinfo.ri_ops.copyrows)(&scr->rinfo, src, dst, num);
}

static void
pxa2x0_lcd_eraserows(void *cookie, int row, int num, long attr)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	(*scr->rinfo.ri_ops.eraserows)(&scr->rinfo, row, num, attr);
}

static int
pxa2x0_lcd_alloc_attr(void *cookie, int fg, int bg, int flg, long *attr)
{
	struct pxa2x0_lcd_screen *scr = cookie;

	return (*scr->rinfo.ri_ops.allocattr)(&scr->rinfo, fg, bg, flg, attr);
}

const struct wsdisplay_emulops pxa2x0_lcd_emulops = {
	pxa2x0_lcd_cursor,
	pxa2x0_lcd_mapchar,
	pxa2x0_lcd_putchar,
	pxa2x0_lcd_copycols,
	pxa2x0_lcd_erasecols,
	pxa2x0_lcd_copyrows,
	pxa2x0_lcd_eraserows,
	pxa2x0_lcd_alloc_attr
};

#endif /* NWSDISPLAY > 0 */
