/*	$NetBSD: vsvar.h,v 1.11.8.1 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Tetsuya Isaki. All rights reserved.
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

/*
 * OKI MSM6258V ADPCM voice synthesizer device driver.
 */

#define VS_ADDR		(0xe92000)
#define VS_DMA		(3)
#define VS_DMAINTR	(0x6a)

/* XXX: PPI should be defined elsewhere */
#define PPI_ADDR		(0x00e9a000)
#define PPI_MAPSIZE		(0x00002000)
#define PPI_PORTA		0
#define PPI_PORTB		1
#define PPI_PORTC		2
#define PPI_CTRL		3

#define VS_RATE_15K	(15625)
#define VS_RATE_10K	(10417)
#define VS_RATE_7K	 (7813)
#define VS_RATE_5K	 (5208)
#define VS_RATE_3K	 (3906)

#define VS_SRATE_1024	(0x00)
#define VS_SRATE_768	(0x04)
#define VS_SRATE_512	(0x08)

#define VS_PANOUT_LR	(0x00)
#define VS_PANOUT_R	(0x01)
#define VS_PANOUT_L	(0x02)
#define VS_PANOUT_OFF	(0x03)

#define VS_MAX_BUFSIZE	(65536*4) /* XXX: enough? */

/* XXX: msm6258vreg.h */
#define MSM6258_CMD 	0		/* W */
#define MSM6258_CMD_STOP	(0x01)
#define MSM6258_CMD_PLAY_START	(0x02)
#define MSM6258_CMD_REC_START	(0x04)
#define MSM6258_STAT	0		/* R */
#define MSM6258_DATA	1		/* R/W */

struct vs_dma {
	bus_dma_tag_t		vd_dmat;
	bus_dmamap_t		vd_map;
	void *			vd_addr;
	bus_dma_segment_t	vd_segs[1];
	int			vd_nsegs;
	size_t			vd_size;
	struct vs_dma		*vd_next;
};
#define KVADDR(dma)	((void *)(dma)->vd_addr)
#define KVADDR_END(dma) ((void *)((size_t)KVADDR(dma) + (dma)->vd_size)))
#define DMAADDR(dma)	((dma)->vd_map->dm_segs[0].ds_addr)

struct vs_softc {
	device_t sc_dev;
	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_ppi; /* XXX */
	uint8_t *sc_addr;

	bus_dma_tag_t sc_dmat;
	struct dmac_channel_stat *sc_dma_ch;
	struct vs_dma *sc_dmas;
	struct vs_dma *sc_prev_vd;

	struct {
		struct dmac_dma_xfer *xfer;
		int rate;
	} sc_current;
	int sc_active;

	const struct audio_hw_if *sc_hw_if;

	void (*sc_pintr)(void *);
	void (*sc_rintr)(void *);
	void *sc_parg;
	void *sc_rarg;
};
