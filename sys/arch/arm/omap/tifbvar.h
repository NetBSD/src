/*	$NetBSD: tifbvar.h,v 1.1.18.2 2017/12/03 11:35:55 jdolecek Exp $	*/
/*
 * Copyright (c) 2016 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
struct tifb_panel_info {
	bool panel_tft;
	bool panel_mono;
	int panel_bpp;

	/* display-timings: see Linux's dts. */
	uint32_t panel_pxl_clk;		/* clock-frequency */
	uint32_t panel_width;		/* hactive */
	uint32_t panel_height;		/* vactive */
	uint32_t panel_hfp;		/* hfront-porch */
	uint32_t panel_hbp;		/* hback-porch */
	uint32_t panel_hsw;		/* hsync-len */
	uint32_t panel_vfp;		/* vfront-porch */
	uint32_t panel_vbp;		/* vback-porch */
	uint32_t panel_vsw;		/* vsync-len */
	uint32_t panel_invert_hsync;	/* ! hsync-active */
	uint32_t panel_invert_vsync;	/* ! hsync-active */

	/* panel-info: see Linux's dts. */
	uint32_t panel_ac_bias;		/* ac-bias */
	uint32_t panel_ac_bias_intrpt;	/* ac-bias-intrpt */
	uint32_t panel_dma_burst_sz;	/* dma-burst-sz */
	uint32_t panel_fdd;		/* fdd */
	uint32_t panel_sync_edge;	/* sync-edge */
	uint32_t panel_sync_ctrl;	/* sync-ctrl */
	bool panel_rdorder;		/* raster-order */
	bool panel_tft_alt_mode;	/* tft-alt-mode (opt) */
	uint32_t panel_invert_pxl_clk;	/* invert-pxl-clk (opt) */
};
