/*	$NetBSD: imx51_ipuv3var.h,v 1.1.6.1 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 2009, 2011, 2012  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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


#ifndef _ARM_IMX_IMX51_IPUV3_H
#define _ARM_IMX_IMX51_IPUV3_H

#include <sys/bus.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

/* IPUV3 Contoroller */
struct imx51_ipuv3_screen {
	LIST_ENTRY(imx51_ipuv3_screen) link;

	/* Frame buffer */
	bus_dmamap_t dma;
	bus_dma_segment_t segs[1];
	int 	nsegs;
	size_t  buf_size;
	size_t  map_size;
	void 	*buf_va;
	int	depth;
	int	stride;

	/* DMA frame descriptor */
	struct	ipuv3_dma_descriptor *dma_desc;
	paddr_t	dma_desc_pa;

	/* rasterop */
	struct rasops_info rinfo;
};

struct lcd_panel_geometry {
	short panel_width;
	short panel_height;

	uint32_t pixel_clk;

	short hsync_width;
	short left;
	short right;

	short vsync_width;
	short upper;
	short lower;

	short panel_info;
#define IPUV3PANEL_SHARP	(1<<0)		/* sharp panel */
	uint32_t panel_sig_pol;
};

struct imx51_ipuv3_softc {
	device_t		dev;

	/* control register */
	bus_space_tag_t  	iot;
	bus_space_handle_t	cm_ioh;		/* CM */
	bus_space_handle_t	dmfc_ioh;	/* DMFC */
	bus_space_handle_t	di0_ioh;	/* DI 0 */
	bus_space_handle_t	dp_ioh;		/* DP */
	bus_space_handle_t	dc_ioh;		/* DC */
	bus_space_handle_t	idmac_ioh;	/* IDMAC */
	bus_space_handle_t	cpmem_ioh;	/* CPMEM */
	bus_space_handle_t	dctmpl_ioh;	/* DCTMPL */
	bus_dma_tag_t    	dma_tag;

	uint32_t		flags;
#define FLAG_NOUSE_ACBIAS	(1U<<0)

	const struct lcd_panel_geometry *geometry;

	int n_screens;
	LIST_HEAD(, imx51_ipuv3_screen) screens;
	struct imx51_ipuv3_screen *active;
	void *ih;			/* interrupt handler */

	/* virtual console support */
	struct vcons_data vd;
	struct vcons_screen console;
	int mode;
};

/*
 * we need bits-per-pixel value to configure wsdisplay screen
 */
struct imx51_wsscreen_descr {
	struct wsscreen_descr  c;	/* standard descriptor */
	int depth;			/* bits per pixel */
	int flags;			/* rasops flags */
};

struct imx51_ipuv3_softc;

void	imx51_ipuv3_attach_sub(struct imx51_ipuv3_softc *,
	    struct axi_attach_args *, const struct lcd_panel_geometry *);
int	imx51_ipuv3_cnattach(bool, struct imx51_wsscreen_descr *,
	    struct wsdisplay_accessops *,
	    const struct lcd_panel_geometry *);
void	imx51_ipuv3_start_dma(struct imx51_ipuv3_softc *,
	    struct imx51_ipuv3_screen *);

void	imx51_ipuv3_geometry(struct imx51_ipuv3_softc *,
	    const struct lcd_panel_geometry *);
int	imx51_ipuv3_new_screen(struct imx51_ipuv3_softc *, int,
	    struct imx51_ipuv3_screen **);

int	imx51_ipuv3_alloc_screen(void *, const struct wsscreen_descr *,
	    void **, int *, int *, long *);
void	imx51_ipuv3_free_screen(void *, void *);
int	imx51_ipuv3_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t	imx51_ipuv3_mmap(void *, void *, off_t, int);
int	imx51_ipuv3_show_screen(void *, void *, int, void (*)(void *, int, int),
	    void *);

extern const struct wsdisplay_emulops imx51_ipuv3_emulops;

#endif /* _ARM_IMX_IMX51_IPUV3_H */
