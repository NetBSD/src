/* $NetBSD: bwdsp.c,v 1.2.2.2 2024/02/03 11:47:04 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bwdsp.c,v 1.2.2.2 2024/02/03 11:47:04 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>
#include <dev/audio/audio_dai.h>

#include "mainbus.h"
#include "bwai.h"

#define	BWDSP_MAP_FLAGS		BUS_DMA_NOCACHE

#define DSP_DMA_START_ADDR_H	0x30
#define	DSP_DMA_START_ADDR_L	0x32
#define DSP_DMA_CONTROL_LENGTH	0x36
#define  DSP_DMA_CONTROL_LENGTH_CTRL	__BIT(15)
#define	 DSP_DMA_CONTROL_LENGTH_NUM_CLS	__BITS(14,0)

#define	DSP_DMA_ALIGN		32
#define	DSP_DMA_MAX_BUFSIZE	(DSP_DMA_CONTROL_LENGTH_NUM_CLS * 32)

struct bwdsp_dma {
	LIST_ENTRY(bwdsp_dma)	dma_list;
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
};

struct bwdsp_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	LIST_HEAD(, bwdsp_dma) sc_dmalist;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct audio_format	sc_format;

	audio_dai_tag_t		sc_dai;
};

#define	RD2(sc, reg)			\
	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR2(sc, reg, val)		\
	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
bwdsp_allocdma(struct bwdsp_softc *sc, size_t size,
    size_t align, struct bwdsp_dma *dma)
{
	int error;

	dma->dma_size = size;
	error = bus_dmamem_alloc(sc->sc_dmat, dma->dma_size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs,
	    dma->dma_size, &dma->dma_addr, BUS_DMA_WAITOK | BWDSP_MAP_FLAGS);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->sc_dmat, dma->dma_size, dma->dma_nsegs,
	    dma->dma_size, 0, BUS_DMA_WAITOK, &dma->dma_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_addr,
	    dma->dma_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
free:
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);

	return error;
}

static void
bwdsp_freedma(struct bwdsp_softc *sc, struct bwdsp_dma *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);
}

static int
bwdsp_query_format(void *priv, audio_format_query_t *afp)
{
	struct bwdsp_softc * const sc = priv;

	return audio_query_format(&sc->sc_format, 1, afp);
}

static int
bwdsp_set_format(void *priv, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct bwdsp_softc * const sc = priv;

	return audio_dai_mi_set_format(sc->sc_dai, setmode, play, rec,
	    pfil, rfil);
}

static int
bwdsp_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct bwdsp_softc * const sc = priv;

	return audio_dai_set_port(sc->sc_dai, mc);
}

static int
bwdsp_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct bwdsp_softc * const sc = priv;

	return audio_dai_get_port(sc->sc_dai, mc);
}

static int
bwdsp_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct bwdsp_softc * const sc = priv;

	return audio_dai_query_devinfo(sc->sc_dai, di);
}

static void *
bwdsp_allocm(void *priv, int dir, size_t size)
{
	struct bwdsp_softc * const sc = priv;
	struct bwdsp_dma *dma;
	int error;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);

	error = bwdsp_allocdma(sc, size, DSP_DMA_ALIGN, dma);
	if (error) {
		kmem_free(dma, sizeof(*dma));
		device_printf(sc->sc_dev, "couldn't allocate DMA memory (%d)\n",
		    error);
		return NULL;
	}

	LIST_INSERT_HEAD(&sc->sc_dmalist, dma, dma_list);

	return dma->dma_addr;
}

static void
bwdsp_freem(void *priv, void *addr, size_t size)
{
	struct bwdsp_softc * const sc = priv;
	struct bwdsp_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == addr) {
			bwdsp_freedma(sc, dma);
			LIST_REMOVE(dma, dma_list);
			kmem_free(dma, sizeof(*dma));
			break;
		}
}

static int
bwdsp_getdev(void *priv, struct audio_device *adev)
{
	snprintf(adev->name, sizeof(adev->name), "Broadway DSP");
	snprintf(adev->version, sizeof(adev->version), "");
	snprintf(adev->config, sizeof(adev->config), "bwdsp");

	return 0;
}

static int
bwdsp_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK;
}

static int
bwdsp_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	bs = roundup(bs, DSP_DMA_ALIGN);
	if (bs > DSP_DMA_MAX_BUFSIZE) {
		bs = DSP_DMA_MAX_BUFSIZE;
	}
	return bs;
}

static size_t
bwdsp_round_buffersize(void *priv, int dir, size_t bufsize)
{
	if (bufsize > DSP_DMA_MAX_BUFSIZE) {
		bufsize = DSP_DMA_MAX_BUFSIZE;
	}
	return bufsize;
}

static void
bwdsp_transfer(struct bwdsp_softc *sc, uint32_t phys_addr, size_t bufsize)
{
	if (bufsize != 0) {
		WR2(sc, DSP_DMA_START_ADDR_H, phys_addr >> 16);
		WR2(sc, DSP_DMA_START_ADDR_L, phys_addr & 0xffff);
		WR2(sc, DSP_DMA_CONTROL_LENGTH,
		    DSP_DMA_CONTROL_LENGTH_CTRL | (bufsize / 32));
	} else {
		WR2(sc, DSP_DMA_CONTROL_LENGTH, 0);
	}
}

static int
bwdsp_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct bwdsp_softc * const sc = priv;
	struct bwdsp_dma *dma;
	bus_addr_t pstart;
	bus_size_t psize;
	int error;

	pstart = 0;
	psize = (uintptr_t)end - (uintptr_t)start;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == start) {
			pstart = dma->dma_map->dm_segs[0].ds_addr;
			break;
		}
	if (pstart == 0) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}

	error = audio_dai_trigger(sc->sc_dai, start, end, blksize,
	    intr, intrarg, params, AUMODE_PLAY);
	if (error != 0) {
		return error;
	}

	/* Start DMA transfer */
	bwdsp_transfer(sc, pstart, psize);

	return 0;
}

static int
bwdsp_halt_output(void *priv)
{
	struct bwdsp_softc * const sc = priv;

	/* Stop DMA transfer */
	bwdsp_transfer(sc, 0, 0);

	return audio_dai_halt(sc->sc_dai, AUMODE_PLAY);
}

static void
bwdsp_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct bwdsp_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static const struct audio_hw_if bwdsp_hw_if = {
	.query_format = bwdsp_query_format,
	.set_format = bwdsp_set_format,
	.allocm = bwdsp_allocm,
	.freem = bwdsp_freem,
	.getdev = bwdsp_getdev,
	.set_port = bwdsp_set_port,
	.get_port = bwdsp_get_port,
	.query_devinfo = bwdsp_query_devinfo,
	.get_props = bwdsp_get_props,
	.round_blocksize = bwdsp_round_blocksize,
	.round_buffersize = bwdsp_round_buffersize,
	.trigger_output = bwdsp_trigger_output,
	.halt_output = bwdsp_halt_output,
	.get_locks = bwdsp_get_locks,
};

static void
bwdsp_late_attach(device_t dev)
{
	struct bwdsp_softc * const sc = device_private(dev);

	sc->sc_dai = bwai_dsp_init(&sc->sc_intr_lock);
	if (sc->sc_dai == NULL) {
		aprint_error_dev(dev, "can't find bwai device\n");
		return;
	}

	audio_attach_mi(&bwdsp_hw_if, sc, dev);
}

static int
bwdsp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const maa = aux;

	return strcmp(maa->maa_name, "bwdsp") == 0;
}

static void
bwdsp_attach(device_t parent, device_t self, void *aux)
{
	struct bwdsp_softc * const sc = device_private(self);
	struct mainbus_attach_args * const maa = aux;
	bus_addr_t addr = maa->maa_addr;
	bus_size_t size = 0x200;

	sc->sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_dmat = maa->maa_dmat;
	LIST_INIT(&sc->sc_dmalist);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	aprint_naive("\n");
	aprint_normal(": DSP\n");

	sc->sc_format.mode = AUMODE_PLAY;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_BE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 1;
	sc->sc_format.frequency[0] = 48000;

	config_defer(self, bwdsp_late_attach);
}

CFATTACH_DECL_NEW(bwdsp, sizeof(struct bwdsp_softc),
    bwdsp_match, bwdsp_attach, NULL, NULL);
