/* $NetBSD: dwc_mmc_var.h,v 1.15 2022/01/09 15:03:43 jmcneill Exp $ */

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

#ifndef _DWC_MMC_VAR_H
#define _DWC_MMC_VAR_H

struct dwc_mmc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_clk_bsh;
	bus_dma_tag_t sc_dmat;

	u_int sc_flags;
#define	DWC_MMC_F_DMA		__BIT(0)
#define	DWC_MMC_F_USE_HOLD_REG	__BIT(1)
#define	DWC_MMC_F_PWREN_INV	__BIT(2)
#define	DWC_MMC_F_BROKEN_CD	__BIT(3)
#define	DWC_MMC_F_NON_REMOVABLE	__BIT(4)
	uint32_t sc_fifo_reg;
	uint32_t sc_fifo_depth;
	u_int sc_clock_freq;
	u_int sc_bus_width;
	bool sc_card_inited;
	u_int sc_verid;

	void *sc_ih;
	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_intr_cv;

	int sc_mmc_width;
	int sc_mmc_port;

	u_int sc_ciu_div;

	device_t sc_sdmmc_dev;

	uint32_t sc_idma_xferlen;
	bus_dma_segment_t sc_idma_segs[1];
	int sc_idma_nsegs;
	bus_size_t sc_idma_size;
	bus_dmamap_t sc_idma_map;
	int sc_idma_ndesc;
	void *sc_idma_desc;

	bus_dmamap_t sc_dmabounce_map;
	void *sc_dmabounce_buf;
	size_t sc_dmabounce_buflen;

	uint32_t sc_intr_card;
	uint32_t sc_intr_cardmask;
	struct sdmmc_command *sc_curcmd;
	bool sc_wait_dma;
	bool sc_wait_cmd;
	bool sc_wait_data;

	void (*sc_pre_power_on)(struct dwc_mmc_softc *);
	void (*sc_post_power_on)(struct dwc_mmc_softc *);

	int (*sc_card_detect)(struct dwc_mmc_softc *);
	int (*sc_write_protect)(struct dwc_mmc_softc *);
	void (*sc_set_led)(struct dwc_mmc_softc *, int);
	int (*sc_bus_clock)(struct dwc_mmc_softc *, int);
	int (*sc_signal_voltage)(struct dwc_mmc_softc *, int);
};

int	dwc_mmc_init(struct dwc_mmc_softc *);
int	dwc_mmc_intr(void *);

#endif /* !_DWC_MMC_VAR_H */
