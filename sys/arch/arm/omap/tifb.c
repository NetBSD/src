/*	$NetBSD: tifb.c,v 1.3 2014/08/22 20:01:16 jakllsch Exp $	*/

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
 * Copyright 2013 Oleksandr Tymoshenko <gonzo@freebsd.org>
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
 */

/*
 * A framebuffer driver for TI 35xx built-in video controller
 * tested on beaglebone
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tifb.c,v 1.3 2014/08/22 20:01:16 jakllsch Exp $");

#include "opt_omap.h"

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
#include <arm/omap/tifbreg.h>
#include <arm/omap/omap_var.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_obioreg.h>
#ifdef TI_AM335X
#  include <arm/omap/am335x_prcm.h>
#  include <arm/omap/omap2_prcm.h>
#  include <arm/omap/sitara_cm.h>
#  include <arm/omap/sitara_cmreg.h>
#endif

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/videomode/edidvar.h>

#include "locators.h"

struct panel_info {
	uint32_t panel_width;
	uint32_t panel_height;
	uint32_t panel_hfp;
	uint32_t panel_hbp;
	uint32_t panel_hsw;
	uint32_t panel_vfp;
	uint32_t panel_vbp;
	uint32_t panel_vsw;
	uint32_t ac_bias;
	uint32_t ac_bias_intrpt;
	uint32_t dma_burst_sz;
	uint32_t bpp;
	uint32_t fdd;
	uint32_t invert_line_clock;
	uint32_t invert_frm_clock;
	uint32_t sync_edge;
	uint32_t sync_ctrl;
	uint32_t panel_pxl_clk;
	uint32_t panel_invert_pxl_clk;
};

/* for chalk elec cape with 12" panel */
struct panel_info default_panel_info = {
	.panel_width = 1280,
	.panel_height = 800,
	.panel_hfp = 48,
	.panel_hbp = 80,
	.panel_hsw = 32,
	.panel_vfp = 2,
	.panel_vbp = 15,
	.panel_vsw = 6,
	.ac_bias = 255,
	.ac_bias_intrpt = 0,
	.dma_burst_sz = 16,
	.bpp = 16,
	.fdd = 16,
	.invert_line_clock = 0,
	.invert_frm_clock = 0,
	.sync_edge = 0,
	.sync_ctrl = 1,
	.panel_pxl_clk = 69300000,
	.panel_invert_pxl_clk = 0,
};

struct tifb_softc {
	device_t sc_dev;

	void *sc_ih;
	bus_space_tag_t sc_iot;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_regh;
	bus_dmamap_t sc_dmamap;
	bus_dma_segment_t sc_dmamem[1];
	size_t sc_vramsize;
	size_t sc_palettesize;

	int sc_stride;
	int sc_locked;
	void *sc_fbaddr, *sc_vramaddr;

	bus_addr_t sc_fbhwaddr;
	uint16_t *sc_palette;
	uint32_t sc_dispc_config;
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

	struct panel_info *sc_panel;
};

#define TIFB_READ(sc, reg) bus_space_read_4(sc->sc_iot, sc->sc_regh, reg)
#define TIFB_WRITE(sc, reg, val) bus_space_write_4(sc->sc_iot, sc->sc_regh, reg, val)

static int	tifb_match(device_t, cfdata_t, void *);
static void	tifb_attach(device_t, device_t, void *);
static int	tifb_intr(void *);


#ifdef TI_AM335X
static void am335x_clk_lcdc_activate(void);
static int  am335x_clk_get_arm_disp_freq(unsigned int *);
#endif


CFATTACH_DECL_NEW(tifb, sizeof(struct tifb_softc),
    tifb_match, tifb_attach, NULL, NULL);

static int	tifb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	tifb_mmap(void *, void *, off_t, int);
static void	tifb_init_screen(void *, struct vcons_screen *, int, long *);

static int	tifb_putcmap(struct tifb_softc *, struct wsdisplay_cmap *);
static int 	tifb_getcmap(struct tifb_softc *, struct wsdisplay_cmap *);
#if 0
static void	tifb_restore_palette(struct tifb_softc *);
static void 	tifb_putpalreg(struct tifb_softc *, int, uint8_t,
			    uint8_t, uint8_t);

static int	tifb_set_depth(struct tifb_softc *, int);
#endif
static void	tifb_set_video(struct tifb_softc *, int);

struct wsdisplay_accessops tifb_accessops = {
	tifb_ioctl,
	tifb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

extern const u_char rasops_cmap[768];

static struct evcnt ev_sync_lost;
static struct evcnt ev_palette;
static struct evcnt ev_eof0;
static struct evcnt ev_eof1;
static struct evcnt ev_fifo_underflow;
static struct evcnt ev_ac_bias;
static struct evcnt ev_others;


static uint32_t
am335x_lcd_calc_divisor(uint32_t reference, uint32_t freq)
{
	uint32_t div;
	/* Raster mode case: divisors are in range from 2 to 255 */
	for (div = 2; div < 255; div++)
		if (reference/div <= freq)
			return (div);

	return (255);
}

static int
tifb_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if ((obio->obio_addr == -1) || (obio->obio_size == 0))
		return 0;
	return 1;
}

static void
tifb_attach(device_t parent, device_t self, void *aux)
{
	struct tifb_softc	*sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct rasops_info	*ri;
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t	dict;
	/* prop_data_t		edid_data; XXX */
	unsigned long		defattr;
	bool			is_console = false;
	uint32_t		reg, timing0, timing1, timing2, burst_log;
	int			segs, i, div, ref_freq;

#ifdef TI_AM335X
	int ret;
	const char *mode;
	u_int state;
	struct tifb_padconf {
		const char *padname;
		const char *padmode;
	};
	const struct tifb_padconf tifb_padconf_data[] = {
		{"LCD_DATA0",  "lcd_data0"},
		{"LCD_DATA1",  "lcd_data1"},
		{"LCD_DATA2",  "lcd_data2"},
		{"LCD_DATA3",  "lcd_data3"},
		{"LCD_DATA4",  "lcd_data4"},
		{"LCD_DATA5",  "lcd_data5"},
		{"LCD_DATA6",  "lcd_data6"},
		{"LCD_DATA7",  "lcd_data7"},
		{"LCD_DATA8",  "lcd_data8"},
		{"LCD_DATA9",  "lcd_data9"},
		{"LCD_DATA10", "lcd_data10"},
		{"LCD_DATA11", "lcd_data11"},
		{"LCD_DATA12", "lcd_data12"},
		{"LCD_DATA13", "lcd_data13"},
		{"LCD_DATA14", "lcd_data14"},
		{"LCD_DATA15", "lcd_data15"},
		{"GPMC_AD15",  "lcd_data16"},
		{"GPMC_AD14",  "lcd_data17"},
		{"GPMC_AD13",  "lcd_data18"},
		{"GPMC_AD12",  "lcd_data19"},
		{"GPMC_AD11",  "lcd_data20"},
		{"GPMC_AD10",  "lcd_data21"},
		{"GPMC_AD9",   "lcd_data22"},
		{"GPMC_AD8",   "lcd_data23"},
	};
	const struct tifb_padconf tifb_padconf_ctrl[] = {
		{"LCD_VSYNC", "lcd_vsync"},
		{"LCD_HSYNC", "lcd_hsync"},
		{"LCD_PCLK",  "lcd_pclk"},
		{"LCD_AC_BIAS_EN", "lcd_ac_bias_en"},
	};
#endif

	evcnt_attach_dynamic(&ev_sync_lost, EVCNT_TYPE_MISC, NULL,
	    "lcd", "sync lost");
	evcnt_attach_dynamic(&ev_palette, EVCNT_TYPE_MISC, NULL,
	    "lcd", "palette loaded");
	evcnt_attach_dynamic(&ev_eof0, EVCNT_TYPE_MISC, NULL,
	    "lcd", "eof0");
	evcnt_attach_dynamic(&ev_eof1, EVCNT_TYPE_MISC, NULL,
	    "lcd", "eof1");
	evcnt_attach_dynamic(&ev_fifo_underflow, EVCNT_TYPE_MISC, NULL,
	    "lcd", "fifo underflow");
	evcnt_attach_dynamic(&ev_ac_bias, EVCNT_TYPE_MISC, NULL,
	    "lcd", "ac bias");
	evcnt_attach_dynamic(&ev_others, EVCNT_TYPE_MISC, NULL,
	    "lcd", "others");

	sc->sc_iot = obio->obio_iot;
	sc->sc_dev = self;
	sc->sc_dmat = obio->obio_dmat;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
	    &sc->sc_regh)) {
		aprint_error(": couldn't map register space\n");
		return;
	}

	sc->sc_panel = &default_panel_info;/* XXX */

	switch(sc->sc_panel->bpp) {
	case 16:
	case 24:
	case 32:
		break;
	default:
		aprint_error_dev(self, "bogus display bpp, not attaching\n");
		return;
	}

	sc->sc_stride = sc->sc_panel->panel_width * sc->sc_panel->bpp / 8;

	if (sc->sc_panel->panel_width == 0 ||
	    sc->sc_panel->panel_height == 0) {
		aprint_error_dev(self, "bogus display size, not attaching\n");
		return;
	}

	if (obio->obio_intr == OBIOCF_INTR_DEFAULT) {
		aprint_error(": no interrupt\n");
		bus_space_unmap(obio->obio_iot, sc->sc_regh,
		    obio->obio_size);
		return;
	}
	sc->sc_ih = intr_establish(obio->obio_intr, IPL_NET, IST_LEVEL,
	    tifb_intr, sc);
	KASSERT(sc->sc_ih != NULL);

	dict = device_properties(self);
	prop_dictionary_get_bool(dict, "is_console", &is_console);
#if 0
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
#endif

	/* setup video DMA */
	switch(sc->sc_panel->bpp) {
	case 8:
		sc->sc_palettesize = 512;
		break;
	default:
		sc->sc_palettesize = 32;
		break;
	}

	sc->sc_vramsize = sc->sc_palettesize +
	    sc->sc_stride * sc->sc_panel->panel_height;

	if (bus_dmamem_alloc(sc->sc_dmat, sc->sc_vramsize, 0, 0,
	    sc->sc_dmamem, 1, &segs, BUS_DMA_NOWAIT) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to allocate video memory\n");
		return;
	}

	if (bus_dmamem_map(sc->sc_dmat, sc->sc_dmamem, 1, sc->sc_vramsize,
	    &sc->sc_vramaddr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to map video RAM\n");
		return;
	}
	sc->sc_fbaddr = (uint8_t *)sc->sc_vramaddr + sc->sc_palettesize;
	sc->sc_palette = sc->sc_vramaddr;

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

	memset((void *)sc->sc_vramaddr, 0, sc->sc_vramsize);

	sc->sc_palette[0] = 0x4000;
#if 0 /* XXX */
	if (sc->sc_panel->bpp == 8) {
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
	tifb_restore_palette(sc);
#endif

#ifdef TI_AM335X
	/* configure output pins */
	for (i = 0; i < ((sc->sc_panel->bpp == 16) ? 16 : 24); i++) {
		if (sitara_cm_padconf_get(tifb_padconf_data[i].padname,
		    &mode, &state) == 0) {
			aprint_debug(": %s mode %s state %d ",
			    tifb_padconf_data[i].padname, mode, state);
		}
		if (sitara_cm_padconf_set(tifb_padconf_data[i].padname,
		    tifb_padconf_data[i].padmode, 0) != 0) {
			aprint_error(": can't switch %s pad from %s to %s\n", tifb_padconf_data[i].padname, mode, tifb_padconf_data[i].padmode);
			return;
		}
	}
	for (i = 0; i < __arraycount(tifb_padconf_ctrl); i++) {
		if (sitara_cm_padconf_get(tifb_padconf_ctrl[i].padname,
		    &mode, &state) == 0) {
			aprint_debug(": %s mode %s state %d ",
			    tifb_padconf_ctrl[i].padname, mode, state);
		}
		if (sitara_cm_padconf_set(tifb_padconf_ctrl[i].padname,
		    tifb_padconf_ctrl[i].padmode, 0) != 0) {
			aprint_error(": can't switch %s pad from %s to %s\n", tifb_padconf_ctrl[i].padname, mode, tifb_padconf_ctrl[i].padmode);
			return;
		}
	}
	am335x_clk_lcdc_activate();
	/* get reference clk freq */
	ret = am335x_clk_get_arm_disp_freq(&ref_freq);
	if (ret != 0) {
		aprint_error_dev(self, "display clock not enabled\n");
		return;
	}
#endif
	aprint_normal(": TI LCD controller\n");
	aprint_debug_dev(self, ": fb@%p, palette@%p\n", sc->sc_fbaddr,
	    sc->sc_palette);
	/* Only raster mode is supported */
	reg = CTRL_RASTER_MODE;
	div = am335x_lcd_calc_divisor(ref_freq, sc->sc_panel->panel_pxl_clk);
	reg |= (div << CTRL_DIV_SHIFT);
	TIFB_WRITE(sc, LCD_CTRL, reg);
	aprint_debug_dev(self, ": ref_freq %d div %d\n", ref_freq, div);

	/* Set timing */
	timing0 = timing1 = timing2 = 0;

	/* Horizontal back porch */
	timing0 |= (sc->sc_panel->panel_hbp & 0xff) << RASTER_TIMING_0_HBP_SHIFT;
	timing2 |= ((sc->sc_panel->panel_hbp >> 8) & 3) << RASTER_TIMING_2_HBPHI_SHIFT;
	/* Horizontal front porch */
	timing0 |= (sc->sc_panel->panel_hfp & 0xff) << RASTER_TIMING_0_HFP_SHIFT;
	timing2 |= ((sc->sc_panel->panel_hfp >> 8) & 3) << RASTER_TIMING_2_HFPHI_SHIFT;
	/* Horizontal sync width */   
	timing0 |= (sc->sc_panel->panel_hsw & 0x3f) << RASTER_TIMING_0_HSW_SHIFT;
	timing2 |= ((sc->sc_panel->panel_hsw >> 6) & 0xf) << RASTER_TIMING_2_HSWHI_SHIFT
;

	/* Vertical back porch, front porch, sync width */
	timing1 |= (sc->sc_panel->panel_vbp & 0xff) << RASTER_TIMING_1_VBP_SHIFT;
	timing1 |= (sc->sc_panel->panel_vfp & 0xff) << RASTER_TIMING_1_VFP_SHIFT;
	timing1 |= (sc->sc_panel->panel_vsw & 0x3f) << RASTER_TIMING_1_VSW_SHIFT;
	/* Pixels per line */
	timing0 |= (((sc->sc_panel->panel_width - 1) >> 10) & 1)
	    << RASTER_TIMING_0_PPLMSB_SHIFT;
	timing0 |= (((sc->sc_panel->panel_width - 1) >> 4) & 0x3f)
	    << RASTER_TIMING_0_PPLLSB_SHIFT;

	/* Lines per panel */
	timing1 |= ((sc->sc_panel->panel_height - 1) & 0x3ff)
	    << RASTER_TIMING_1_LPP_SHIFT;
	timing2 |= (((sc->sc_panel->panel_height - 1) >> 10 ) & 1)
	    << RASTER_TIMING_2_LPP_B10_SHIFT;

	/* clock signal settings */   
	if (sc->sc_panel->sync_ctrl)
		timing2 |= RASTER_TIMING_2_PHSVS;
	if (sc->sc_panel->sync_edge)
		timing2 |= RASTER_TIMING_2_PHSVS_RISE;
	else
		timing2 |= RASTER_TIMING_2_PHSVS_FALL;
	if (sc->sc_panel->invert_line_clock)  
		timing2 |= RASTER_TIMING_2_IHS;
	if (sc->sc_panel->invert_frm_clock)   
		timing2 |= RASTER_TIMING_2_IVS;
	if (sc->sc_panel->panel_invert_pxl_clk)
		timing2 |= RASTER_TIMING_2_IPC;

	/* AC bias */
	timing2 |= (sc->sc_panel->ac_bias << RASTER_TIMING_2_ACB_SHIFT);
	timing2 |= (sc->sc_panel->ac_bias_intrpt << RASTER_TIMING_2_ACBI_SHIFT);

	TIFB_WRITE(sc, LCD_RASTER_TIMING_0, timing0);
	TIFB_WRITE(sc, LCD_RASTER_TIMING_1, timing1);
	TIFB_WRITE(sc, LCD_RASTER_TIMING_2, timing2);
	aprint_debug_dev(self, ": timings 0x%x 0x%x 0x%x\n",
	    timing0, timing1, timing2);

	/* DMA settings */
	reg = LCDDMA_CTRL_FB0_FB1;    
	/* Find power of 2 for current burst size */
	switch (sc->sc_panel->dma_burst_sz) { 
	case 1:
		burst_log = 0;
		break;
	case 2:
		burst_log = 1;
		break;
	case 4:
		burst_log = 2;
		break;
	case 8:
		burst_log = 3;
		break;
	case 16:
	default:
		burst_log = 4;
		break;
	}
	reg |= (burst_log << LCDDMA_CTRL_BURST_SIZE_SHIFT);
	/* XXX: FIFO TH */
	reg |= (0 << LCDDMA_CTRL_TH_FIFO_RDY_SHIFT);
	TIFB_WRITE(sc, LCD_LCDDMA_CTRL, reg);
	aprint_debug_dev(self, ": LCD_LCDDMA_CTRL 0x%x\n", reg);

	TIFB_WRITE(sc, LCD_LCDDMA_FB0_BASE, sc->sc_dmamem->ds_addr);
	TIFB_WRITE(sc, LCD_LCDDMA_FB0_CEILING,
	    sc->sc_dmamem->ds_addr + sc->sc_vramsize - 1);
	TIFB_WRITE(sc, LCD_LCDDMA_FB1_BASE, sc->sc_dmamem->ds_addr);
	TIFB_WRITE(sc, LCD_LCDDMA_FB1_CEILING,
	    sc->sc_dmamem->ds_addr + sc->sc_vramsize - 1);
	aprint_debug_dev(self, ": LCD_LCDDMA 0x%x 0x%x\n",
	    (u_int)sc->sc_dmamem->ds_addr, (u_int)(sc->sc_dmamem->ds_addr + sc->sc_vramsize - 1));

	/* Enable LCD */
	reg = RASTER_CTRL_LCDTFT;     
	reg |= (sc->sc_panel->fdd << RASTER_CTRL_REQDLY_SHIFT);
	reg |= (PALETTE_DATA_ONLY << RASTER_CTRL_PALMODE_SHIFT);
	if (sc->sc_panel->bpp >= 24)
		reg |= RASTER_CTRL_TFT24;
	if (sc->sc_panel->bpp == 32)
		reg |= RASTER_CTRL_TFT24_UNPACKED;
	TIFB_WRITE(sc, LCD_RASTER_CTRL, reg);
	aprint_debug_dev(self, ": LCD_RASTER_CTRL 0x%x\n", reg);

	TIFB_WRITE(sc, LCD_CLKC_ENABLE,
	    CLKC_ENABLE_DMA | CLKC_ENABLE_LDID | CLKC_ENABLE_CORE);     

	TIFB_WRITE(sc, LCD_CLKC_RESET, CLKC_RESET_MAIN);
	DELAY(100);
	TIFB_WRITE(sc, LCD_CLKC_RESET, 0);
	aprint_debug_dev(self, ": LCD_CLKC_ENABLE 0x%x\n", TIFB_READ(sc, LCD_CLKC_ENABLE));

	reg = IRQ_EOF1 | IRQ_EOF0 | IRQ_FUF | IRQ_PL |
	    IRQ_ACB | IRQ_SYNC_LOST |  IRQ_RASTER_DONE |
	    IRQ_FRAME_DONE;
	TIFB_WRITE(sc, LCD_IRQENABLE_SET, reg);

	reg = TIFB_READ(sc, LCD_RASTER_CTRL);
	reg |= RASTER_CTRL_LCDEN;     
	TIFB_WRITE(sc, LCD_RASTER_CTRL, reg);
	aprint_debug_dev(self, ": LCD_RASTER_CTRL 0x%x\n", TIFB_READ(sc, LCD_RASTER_CTRL));

	TIFB_WRITE(sc, LCD_SYSCONFIG, 
	    SYSCONFIG_STANDBY_SMART | SYSCONFIG_IDLE_SMART);
	aprint_debug_dev(self, ": LCD_SYSCONFIG 0x%x\n", TIFB_READ(sc, LCD_SYSCONFIG));

	/* attach wscons */
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
	    &tifb_accessops);
	sc->vd.init_screen = tifb_init_screen;

	/* init engine here */

	ri = &sc->sc_console_screen.scr_ri;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
	if (is_console) {
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	}

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &tifb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
tifb_intr(void *v)
{
	struct tifb_softc *sc = v;
	uint32_t reg;

	reg = TIFB_READ(sc, LCD_IRQSTATUS);
	TIFB_WRITE(sc, LCD_IRQSTATUS, reg);

	if (reg & IRQ_SYNC_LOST) {
		ev_sync_lost.ev_count ++;
		reg = TIFB_READ(sc, LCD_RASTER_CTRL);
		reg &= ~RASTER_CTRL_LCDEN;
		TIFB_WRITE(sc, LCD_RASTER_CTRL, reg);

		reg = TIFB_READ(sc, LCD_RASTER_CTRL);
		reg |= RASTER_CTRL_LCDEN;
		TIFB_WRITE(sc, LCD_RASTER_CTRL, reg);
		return 0;
	}

	if (reg & IRQ_PL) {
		ev_palette.ev_count ++;
		reg = TIFB_READ(sc, LCD_RASTER_CTRL);
		reg &= ~RASTER_CTRL_LCDEN;
		TIFB_WRITE(sc, LCD_RASTER_CTRL, reg);

		reg = TIFB_READ(sc, LCD_RASTER_CTRL);
		reg |= RASTER_CTRL_LCDEN;
		TIFB_WRITE(sc, LCD_RASTER_CTRL, reg);
		return 0;
	}

	if (reg & IRQ_EOF0) {
		ev_eof0.ev_count ++;
		TIFB_WRITE(sc, LCD_LCDDMA_FB0_BASE, sc->sc_dmamem->ds_addr);
		TIFB_WRITE(sc, LCD_LCDDMA_FB0_CEILING,
		    sc->sc_dmamem->ds_addr + sc->sc_vramsize - 1);
		reg &= ~IRQ_EOF0;
	}

	if (reg & IRQ_EOF1) {
		ev_eof1.ev_count ++;
		TIFB_WRITE(sc, LCD_LCDDMA_FB1_BASE, sc->sc_dmamem->ds_addr);
		TIFB_WRITE(sc, LCD_LCDDMA_FB1_CEILING,
		    sc->sc_dmamem->ds_addr + sc->sc_vramsize - 1);
		reg &= ~IRQ_EOF1;     
	}

	if (reg & IRQ_FUF) {
		ev_fifo_underflow.ev_count ++;
		/* TODO: Handle FUF */
		reg =~ IRQ_FUF;
	}

	if (reg & IRQ_ACB) {
		ev_ac_bias.ev_count ++;
		/* TODO: Handle ACB */
		reg =~ IRQ_ACB;
	}
	if (reg)
		ev_others.ev_count ++;
	return 0;
}

static int
tifb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct tifb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {

		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_GENFB;
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
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return tifb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return tifb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->sc_stride;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;

				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
#if 0
					if (new_mode == WSDISPLAYIO_MODE_EMUL) {
						tifb_set_depth(sc, 16);
						vcons_redraw_screen(ms);
					} else {
						tifb_set_depth(sc, 32);
					}
#endif
				}
			}
			return 0;

		case WSDISPLAYIO_GET_FBINFO:
			{
				struct wsdisplayio_fbinfo *fbi = data;
				int ret;
				
				ret = wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
				fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
				fbi->fbi_fboffset = sc->sc_palettesize;
				return ret;
			}

		case WSDISPLAYIO_GVIDEO:
			{
				int *on = data;
				*on = 1; /* XXX sc->sc_video_is_on; */
			}
			return 0;

		case WSDISPLAYIO_SVIDEO:
			{
				int *on = data;
				tifb_set_video(sc, *on);
			}
			return 0;

	}
	return EPASSTHROUGH;
}

static paddr_t
tifb_mmap(void *v, void *vs, off_t offset, int prot)
{
	paddr_t pa = -1;
	struct vcons_data *vd = v;
	struct tifb_softc *sc = vd->cookie;

	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_stride * sc->sc_panel->panel_height) {
		pa = bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmamem, 1,
		    offset, prot, BUS_DMA_PREFETCHABLE);
		return pa;
	}
	return pa;
}

static void
tifb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct tifb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_panel->bpp;
	ri->ri_width = sc->sc_panel->panel_width;
	ri->ri_height = sc->sc_panel->panel_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = (char *)sc->sc_fbaddr;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_panel->panel_height / 8, sc->sc_panel->panel_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_panel->panel_height / ri->ri_font->fontheight,
		    sc->sc_panel->panel_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

static int
tifb_putcmap(struct tifb_softc *sc, struct wsdisplay_cmap *cm)
{
	return EINVAL; /* XXX */
#if 0
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
		tifb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
#endif
}

static int
tifb_getcmap(struct tifb_softc *sc, struct wsdisplay_cmap *cm)
{
	return EINVAL; /* XXX */
#if 0
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
#endif
}

#if 0
static void
tifb_restore_palette(struct tifb_softc *sc)
{
	int i;

	for (i = 0; i < 256; i++) {
		tifb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static void
tifb_putpalreg(struct tifb_softc *sc, int idx, uint8_t r, uint8_t g,
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
tifb_set_depth(struct tifb_softc *sc, int d)
{
	uint32_t reg;

	reg = OMAP_VID_ATTR_ENABLE |
	      OMAP_VID_ATTR_BURST_16x32 |
	      OMAP_VID_ATTR_REPLICATION;
	switch (d) {
		case 16:
			reg |= OMAP_VID_ATTR_RGB16;
			break;
		case 32:
			reg |= OMAP_VID_ATTR_RGB24;
			break;
		default:
			aprint_error_dev(sc->sc_dev,
			    "unsupported depth (%d)\n", d);
			return EINVAL;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_regh,
	    OMAPFB_DISPC_VID1_ATTRIBUTES, reg);

	/*
	 * now tell the video controller that we're done mucking around and
	 * actually update its settings
	 */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPFB_DISPC_CONTROL,
	    reg | OMAP_DISPC_CTRL_GO_LCD | OMAP_DISPC_CTRL_GO_DIGITAL);

	sc->sc_panel->bpp = d;
	sc->sc_stride = sc->sc_panel->panel_width * (sc->sc_panel->bpp >> 3);

	/* clear the screen here */
	memset(sc->sc_fbaddr, 0, sc->sc_stride * sc->sc_panel->panel_height);
	return 0;
}
#endif /* 0 */

static void
tifb_set_video(struct tifb_softc *sc, int on)
{
#if 0
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
#endif
}

#ifdef TI_AM335X
static void
am335x_clk_lcdc_activate(void)
{
	/* Bypass mode */
	prcm_write_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_CLKMODE_DPLL_DISP,
	    AM335X_PRCM_CM_CLKMODE_DPLL_MN_BYP_MODE);

	/* Make sure it's in bypass mode */
	while (!(prcm_read_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_IDLEST_DPLL_DISP)
	    & (1 << 8)))
		DELAY(10);

	/*
	 * For now set frequency to  5xSYSFREQ
	 * More flexible control might be required
	 */
	prcm_write_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_CLKSEL_DPLL_DISP,
	    (5 << 8) | 0);

	/* Locked mode */
	prcm_write_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_CLKMODE_DPLL_DISP, 0x7);

	int timeout = 10000;
	while ((!(prcm_read_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_IDLEST_DPLL_DISP)
	    & (1 << 0))) && timeout--)
		DELAY(10);

	/*set MODULEMODE to ENABLE(2) */
	prcm_write_4(AM335X_PRCM_CM_PER, CM_PER_LCDC_CLKCTRL, 2);

	/* wait for MODULEMODE to become ENABLE(2) */
	while ((prcm_read_4(AM335X_PRCM_CM_PER, CM_PER_LCDC_CLKCTRL) & 0x3) != 2)
		DELAY(10);

	/* wait for IDLEST to become Func(0) */
	while(prcm_read_4(AM335X_PRCM_CM_PER, CM_PER_LCDC_CLKCTRL) & (3<<16))
		DELAY(10);
}

static int
am335x_clk_get_arm_disp_freq(unsigned int *freq)
{
	uint32_t reg;
#define DPLL_BYP_CLKSEL(reg)    ((reg>>23) & 1)
#define DPLL_DIV(reg)	   ((reg & 0x7f)+1)
#define DPLL_MULT(reg)	  ((reg>>8) & 0x7FF)
	
	reg = prcm_read_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_CLKSEL_DPLL_DISP);
	
	/*Check if we are running in bypass */
	if (DPLL_BYP_CLKSEL(reg))
		return ENXIO;
		
	*freq = DPLL_MULT(reg) * (omap_sys_clk / DPLL_DIV(reg));
	return(0);
}
#endif
