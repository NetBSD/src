/* $NetBSD: sunxi_codec.h,v 1.4 2017/10/07 21:53:16 jmcneill Exp $ */

/*-
 * Copyright (c) 2014-2017 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_SUNXI_CODEC_H
#define _ARM_SUNXI_CODEC_H

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/fdt/fdtvar.h>

#include "h3_codec.h"

struct sunxi_codec_softc;

struct sunxi_codec_conf {
	const char	*name;

	/* initialize codec */
	int		(*init)(struct sunxi_codec_softc *);
	/* toggle DAC/ADC mute */
	void		(*mute)(struct sunxi_codec_softc *, int, u_int);
	/* mixer controls */
	int		(*set_port)(struct sunxi_codec_softc *,
				    mixer_ctrl_t *);
	int		(*get_port)(struct sunxi_codec_softc *,
				    mixer_ctrl_t *);
	int		(*query_devinfo)(struct sunxi_codec_softc *,
					 mixer_devinfo_t *);

	/* register map */
	bus_size_t	DPC,
			DAC_FIFOC,
			DAC_FIFOS,
			DAC_TXDATA,
			ADC_FIFOC,
			ADC_FIFOS,
			ADC_RXDATA,
			DAC_CNT,
			ADC_CNT;
};

struct sunxi_codec_chan {
	struct sunxi_codec_softc *ch_sc;
	u_int			ch_mode;

	struct fdtbus_dma	*ch_dma;
	struct fdtbus_dma_req	ch_req;

	audio_params_t		ch_params;

	bus_addr_t		ch_start_phys;
	bus_addr_t		ch_end_phys;
	bus_addr_t		ch_cur_phys;
	int			ch_blksize;

	void			(*ch_intr)(void *);
	void			*ch_intrarg;
};

struct sunxi_codec_dma {
	LIST_ENTRY(sunxi_codec_dma) dma_list;
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
};

struct sunxi_codec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;
	bus_addr_t		sc_baseaddr;

	struct sunxi_codec_conf	*sc_cfg;
	void			*sc_codec_priv;

	struct fdtbus_gpio_pin	*sc_pin_pa;

	LIST_HEAD(, sunxi_codec_dma) sc_dmalist;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct audio_format	sc_format;
	struct audio_encoding_set *sc_encodings;

	struct sunxi_codec_chan	sc_pchan;
	struct sunxi_codec_chan	sc_rchan;
};

#if NH3_CODEC > 0
extern const struct sunxi_codec_conf sun8i_h3_codecconf;
#define	H3_CODEC_COMPATDATA	\
	{ "allwinner,sun8i-h3-codec",	(uintptr_t)&sun8i_h3_codecconf }
#else
#define	H3_CODEC_COMPATDATA
#endif

extern const struct sunxi_codec_conf sun4i_a10_codecconf;
#define	A10_CODEC_COMPATDATA	\
	{ "allwinner,sun4i-a10-codec",	(uintptr_t)&sun4i_a10_codecconf }, \
	{ "allwinner,sun7i-a20-codec",	(uintptr_t)&sun4i_a10_codecconf }

extern const struct sunxi_codec_conf sun6i_a31_codecconf;
#define	A31_CODEC_COMPATDATA	\
	{ "allwinner,sun6i-a31-codec",	(uintptr_t)&sun6i_a31_codecconf }

#endif /* !_ARM_SUNXI_CODEC_H */
