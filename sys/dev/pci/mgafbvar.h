/*	$NetBSD: mgafbvar.h,v 1.1 2026/03/17 10:03:02 macallan Exp $	*/

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef MGAFBVAR_H
#define MGAFBVAR_H

#include "opt_mgafb.h"

#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

enum mgafb_chip {
	MGAFB_CHIP_2064W,	/* MGA-2064W (Millennium) */
	MGAFB_CHIP_2164W,	/* MGA-2164W (Millennium II) */
	MGAFB_CHIP_1064SG,	/* MGA-1064SG (Mystique) */
};

struct mgafb_chip_info {
	const char	*ci_name;	/* short chip name for messages */
	uint32_t	ci_max_pclk;	/* default max pixel clock (kHz) */
	uint32_t	ci_vram_default;/* fallback VRAM if probe skipped */
	bool		ci_has_tvp3026;	/* external TVP3026 DAC? */
	bool		ci_has_wram;	/* WRAM (vs SGRAM/SDRAM)? */
	bool		ci_probe_vram;	/* can we probe VRAM size? */
};

struct mgafb_softc {
	device_t		sc_dev;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;

	enum mgafb_chip		sc_chip;
	const struct mgafb_chip_info *sc_ci;

	/*
	 * PInS data read from the card's ROM during attach.
	 */
	bool			sc_pins_valid;
	uint32_t		sc_pins_mclk_khz;	/* system / MCLK target */
	uint32_t		sc_pins_maxdac_khz;	/* max pixel clock */

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	bus_addr_t		sc_reg_pa;
	bus_size_t		sc_reg_size;

	bus_space_tag_t		sc_fbt;
	bus_space_handle_t	sc_fbh;
	bus_addr_t		sc_fb_pa;
	bus_size_t		sc_fb_size;

	bus_size_t		sc_vram_size;

	const struct videomode	*sc_videomode;	/* selected video mode */

	int			sc_width;
	int			sc_height;
	int			sc_depth;
	int			sc_stride;	/* bytes per line */
	int			sc_mode;
	int			sc_video;	/* WSDISPLAYIO_VIDEO_ON/OFF */

	struct vcons_data	vd;
	struct vcons_screen	sc_console_screen;
	struct wsscreen_descr	sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list	sc_screenlist;

	uint8_t			sc_cmap_red[256];
	uint8_t			sc_cmap_green[256];
	uint8_t			sc_cmap_blue[256];

	glyphcache		sc_gc;

	struct mgafb_cursor {
		bool		mc_enabled;
		uint8_t		mc_r[2];	/* [0]=bg, [1]=fg */
		uint8_t		mc_g[2];
		uint8_t		mc_b[2];
	}			sc_cursor;

	struct i2c_controller	sc_i2c;
	uint8_t			sc_ddc_ctl;
	bool			sc_edid_valid;
	uint8_t			sc_edid[128];
	struct edid_info	sc_edid_info;
};

#endif /* MGAFBVAR_H */
