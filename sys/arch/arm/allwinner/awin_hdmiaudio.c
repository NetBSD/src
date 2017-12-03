/* $NetBSD: awin_hdmiaudio.c,v 1.7.8.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: awin_hdmiaudio.c,v 1.7.8.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/auvolconv.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

struct awin_hdmiaudio_dma {
	LIST_ENTRY(awin_hdmiaudio_dma)	dma_list;
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
};

struct awin_hdmiaudio_softc {
	device_t		sc_dev;
	device_t		sc_audiodev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	uint32_t		sc_ver;

	LIST_HEAD(, awin_hdmiaudio_dma)	sc_dmalist;

	uint8_t			sc_drqtype_hdmiaudio;
	uint8_t			sc_drqtype_sdram;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct audio_format	sc_format;
	struct audio_encoding_set *sc_encodings;

	struct awin_dma_channel *sc_pdma;
	void			(*sc_pint)(void *);
	void			*sc_pintarg;
	bus_addr_t		sc_pstart;
	bus_addr_t		sc_pend;
	bus_addr_t		sc_pcur;
	int			sc_pblksize;

	uint8_t			sc_swvol;
};

static int	awin_hdmiaudio_match(device_t, cfdata_t, void *);
static void	awin_hdmiaudio_attach(device_t, device_t, void *);
static int	awin_hdmiaudio_rescan(device_t, const char *, const int *);
static void	awin_hdmiaudio_childdet(device_t, device_t);

static int	awin_hdmiaudio_allocdma(struct awin_hdmiaudio_softc *, size_t,
					size_t, struct awin_hdmiaudio_dma *);
static void	awin_hdmiaudio_freedma(struct awin_hdmiaudio_softc *,
				       struct awin_hdmiaudio_dma *);

static void	awin_hdmiaudio_pint(void *);
static int	awin_hdmiaudio_play(struct awin_hdmiaudio_softc *);

#if defined(DDB)
void		awin_hdmiaudio_dump_regs(void);
#endif

static int	awin_hdmiaudio_open(void *, int);
static void	awin_hdmiaudio_close(void *);
static int	awin_hdmiaudio_drain(void *);
static int	awin_hdmiaudio_query_encoding(void *, struct audio_encoding *);
static int	awin_hdmiaudio_set_params(void *, int, int,
					  audio_params_t *, audio_params_t *,
					  stream_filter_list_t *,
					  stream_filter_list_t *);
static int	awin_hdmiaudio_halt_output(void *);
static int	awin_hdmiaudio_halt_input(void *);
static int	awin_hdmiaudio_set_port(void *, mixer_ctrl_t *);
static int	awin_hdmiaudio_get_port(void *, mixer_ctrl_t *);
static int	awin_hdmiaudio_query_devinfo(void *, mixer_devinfo_t *);
static void *	awin_hdmiaudio_allocm(void *, int, size_t);
static void	awin_hdmiaudio_freem(void *, void *, size_t);
static paddr_t	awin_hdmiaudio_mappage(void *, void *, off_t, int);
static int	awin_hdmiaudio_getdev(void *, struct audio_device *);
static int	awin_hdmiaudio_get_props(void *);
static int	awin_hdmiaudio_round_blocksize(void *, int, int,
					       const audio_params_t *);
static size_t	awin_hdmiaudio_round_buffersize(void *, int, size_t);
static int	awin_hdmiaudio_trigger_output(void *, void *, void *, int,
					      void (*)(void *), void *,
					      const audio_params_t *);
static int	awin_hdmiaudio_trigger_input(void *, void *, void *, int,
					     void (*)(void *), void *,
					     const audio_params_t *);
static void	awin_hdmiaudio_get_locks(void *, kmutex_t **, kmutex_t **);

static stream_filter_t *awin_hdmiaudio_swvol_filter(struct audio_softc *,
    const audio_params_t *, const audio_params_t *);
static void	awin_hdmiaudio_swvol_dtor(stream_filter_t *);

static const struct audio_hw_if awin_hdmiaudio_hw_if = {
	.open = awin_hdmiaudio_open,
	.close = awin_hdmiaudio_close,
	.drain = awin_hdmiaudio_drain,
	.query_encoding = awin_hdmiaudio_query_encoding,
	.set_params = awin_hdmiaudio_set_params,
	.halt_output = awin_hdmiaudio_halt_output,
	.halt_input = awin_hdmiaudio_halt_input,
	.allocm = awin_hdmiaudio_allocm,
	.freem = awin_hdmiaudio_freem,
	.mappage = awin_hdmiaudio_mappage,
	.getdev = awin_hdmiaudio_getdev,
	.set_port = awin_hdmiaudio_set_port,
	.get_port = awin_hdmiaudio_get_port,
	.query_devinfo = awin_hdmiaudio_query_devinfo,
	.get_props = awin_hdmiaudio_get_props,
	.round_blocksize = awin_hdmiaudio_round_blocksize,
	.round_buffersize = awin_hdmiaudio_round_buffersize,
	.trigger_output = awin_hdmiaudio_trigger_output,
	.trigger_input = awin_hdmiaudio_trigger_input,
	.get_locks = awin_hdmiaudio_get_locks,
};

enum {
	HDMIAUDIO_OUTPUT_CLASS,
	HDMIAUDIO_INPUT_CLASS,
	HDMIAUDIO_OUTPUT_MASTER_VOLUME,
	HDMIAUDIO_INPUT_DHDMIAUDIO_VOLUME,
	HDMIAUDIO_ENUM_LAST
};

CFATTACH_DECL2_NEW(awin_hdmiaudio, sizeof(struct awin_hdmiaudio_softc),
	awin_hdmiaudio_match, awin_hdmiaudio_attach, NULL, NULL,
	awin_hdmiaudio_rescan, awin_hdmiaudio_childdet);

#define HDMIAUDIO_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define HDMIAUDIO_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
awin_hdmiaudio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_hdmiaudio_attach(device_t parent, device_t self, void *aux)
{
	struct awin_hdmiaudio_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	int error;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	LIST_INIT(&sc->sc_dmalist);
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	sc->sc_ver = HDMIAUDIO_READ(sc, AWIN_HDMI_VERSION_ID_REG);

	const int vmaj = __SHIFTOUT(sc->sc_ver, AWIN_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(sc->sc_ver, AWIN_HDMI_VERSION_ID_L);

	aprint_naive("\n");
	aprint_normal(": HDMI %d.%d\n", vmaj, vmin);

	sc->sc_drqtype_hdmiaudio = awin_chip_id() == AWIN_CHIP_ID_A31 ?
	    AWIN_A31_DMA_DRQ_TYPE_HDMIAUDIO :
	    AWIN_DDMA_CTL_DRQ_HDMI_AUDIO;
	sc->sc_drqtype_sdram = awin_chip_id() == AWIN_CHIP_ID_A31 ?
	    AWIN_A31_DMA_DRQ_TYPE_SDRAM :
	    AWIN_DDMA_CTL_DRQ_SDRAM;

	sc->sc_pdma = awin_dma_alloc("hdmiaudio", awin_hdmiaudio_pint, sc);
	if (sc->sc_pdma == NULL) {
		aprint_error_dev(self, "couldn't allocate play DMA channel\n");
		return;
	}

	sc->sc_format.mode = AUMODE_PLAY|AUMODE_RECORD;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 0;
	sc->sc_format.frequency[0] = sc->sc_format.frequency[1] = 48000;

	error = auconv_create_encodings(&sc->sc_format, 1, &sc->sc_encodings);
	if (error) {
		aprint_error_dev(self, "couldn't create encodings (error=%d)\n",
		    error);
		return;
	}

	sc->sc_swvol = 255;

	awin_hdmiaudio_rescan(self, NULL, NULL);
}

static int
awin_hdmiaudio_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct awin_hdmiaudio_softc *sc = device_private(self);

	if (ifattr_match(ifattr, "audiobus") && sc->sc_audiodev == NULL) {
		if (sc->sc_encodings == NULL)
			return EIO;
		sc->sc_audiodev = audio_attach_mi(&awin_hdmiaudio_hw_if,
		    sc, sc->sc_dev);
	}

	return 0;
}

static void
awin_hdmiaudio_childdet(device_t self, device_t child)
{
	struct awin_hdmiaudio_softc *sc = device_private(self);

	if (sc->sc_audiodev == child)
		sc->sc_audiodev = NULL;
}

static int
awin_hdmiaudio_allocdma(struct awin_hdmiaudio_softc *sc, size_t size, size_t align,
    struct awin_hdmiaudio_dma *dma)
{
	int error;

	dma->dma_size = size;
	error = bus_dmamem_alloc(sc->sc_dmat, dma->dma_size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs,
	    dma->dma_size, &dma->dma_addr, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
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
awin_hdmiaudio_freedma(struct awin_hdmiaudio_softc *sc, struct awin_hdmiaudio_dma *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);
}

static void
awin_hdmiaudio_pint(void *priv)
{
	struct awin_hdmiaudio_softc *sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_pint == NULL) {
		mutex_exit(&sc->sc_intr_lock);
		return;
	}
	sc->sc_pint(sc->sc_pintarg);
	mutex_exit(&sc->sc_intr_lock);

	awin_hdmiaudio_play(sc);
}

static int
awin_hdmiaudio_play(struct awin_hdmiaudio_softc *sc)
{
	int error;

	error = awin_dma_transfer(sc->sc_pdma, sc->sc_pcur,
	    AWIN_CORE_PBASE + AWIN_HDMI_OFFSET + AWIN_HDMI_AUX_TX_FIFO_REG,
	    sc->sc_pblksize);
	if (error) {
		device_printf(sc->sc_dev, "failed to transfer DMA; error %d\n",
		    error);
		return error;
	}

	sc->sc_pcur += sc->sc_pblksize;
	if (sc->sc_pcur >= sc->sc_pend)
		sc->sc_pcur = sc->sc_pstart;

	return 0;
}

static int
awin_hdmiaudio_open(void *priv, int flags)
{
	return 0;
}

static void
awin_hdmiaudio_close(void *priv)
{
}

static int
awin_hdmiaudio_drain(void *priv)
{
	return 0;
}

static int
awin_hdmiaudio_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct awin_hdmiaudio_softc *sc = priv;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
awin_hdmiaudio_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct awin_hdmiaudio_softc *sc = priv;
	int index;

	if (play && (setmode & AUMODE_PLAY)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_PLAY, play, true, pfil);
		if (index < 0)
			return EINVAL;
		if (pfil->req_size > 0)
			play = &pfil->filters[0].param;
		pfil->prepend(pfil, awin_hdmiaudio_swvol_filter, play);
	}

	return 0;
}

static int
awin_hdmiaudio_halt_output(void *priv)
{
	struct awin_hdmiaudio_softc *sc = priv;

	awin_dma_halt(sc->sc_pdma);

	sc->sc_pint = NULL;
	sc->sc_pintarg = NULL;
	awin_hdmi_poweron(false);

	return 0;
}

static int
awin_hdmiaudio_halt_input(void *priv)
{
	return EINVAL;
}

static int
awin_hdmiaudio_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct awin_hdmiaudio_softc *sc = priv;

	switch (mc->dev) {
	case HDMIAUDIO_OUTPUT_MASTER_VOLUME:
	case HDMIAUDIO_INPUT_DHDMIAUDIO_VOLUME:
		sc->sc_swvol = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		return 0;
	}

	return ENXIO;
}

static int
awin_hdmiaudio_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct awin_hdmiaudio_softc *sc = priv;
	uint8_t vol = sc->sc_swvol;

	switch (mc->dev) {
	case HDMIAUDIO_OUTPUT_MASTER_VOLUME:
	case HDMIAUDIO_INPUT_DHDMIAUDIO_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = vol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = vol;
		return 0;
	}

	return ENXIO;
}

static int
awin_hdmiaudio_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	switch (di->index) {
	case HDMIAUDIO_OUTPUT_CLASS:
		di->mixer_class = HDMIAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case HDMIAUDIO_INPUT_CLASS:
		di->mixer_class = HDMIAUDIO_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case HDMIAUDIO_OUTPUT_MASTER_VOLUME:
		di->mixer_class = HDMIAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case HDMIAUDIO_INPUT_DHDMIAUDIO_VOLUME:
		di->mixer_class = HDMIAUDIO_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

static void *
awin_hdmiaudio_allocm(void *priv, int dir, size_t size)
{
	struct awin_hdmiaudio_softc *sc = priv;
	struct awin_hdmiaudio_dma *dma;
	int error;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);

	error = awin_hdmiaudio_allocdma(sc, size, 16, dma);
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
awin_hdmiaudio_freem(void *priv, void *addr, size_t size)
{
	struct awin_hdmiaudio_softc *sc = priv;
	struct awin_hdmiaudio_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == addr) {
			awin_hdmiaudio_freedma(sc, dma);
			LIST_REMOVE(dma, dma_list);
			kmem_free(dma, sizeof(*dma));
			break;
		}
	}
}

static paddr_t
awin_hdmiaudio_mappage(void *priv, void *addr, off_t off, int prot)
{
	struct awin_hdmiaudio_softc *sc = priv;
	struct awin_hdmiaudio_dma *dma;

	if (off < 0)
		return -1;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == addr) {
			return bus_dmamem_mmap(sc->sc_dmat, dma->dma_segs,
			    dma->dma_nsegs, off, prot, BUS_DMA_WAITOK);
		}
	}

	return -1;
}

static int
awin_hdmiaudio_getdev(void *priv, struct audio_device *audiodev)
{
	struct awin_hdmiaudio_softc *sc = priv;
	struct awin_hdmi_info info;
	const char *vendor = NULL, *product = NULL;

	const int vmaj = __SHIFTOUT(sc->sc_ver, AWIN_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(sc->sc_ver, AWIN_HDMI_VERSION_ID_L);

	awin_hdmi_get_info(&info);

	if (info.display_connected && info.display_hdmimode) {
		if (strlen(info.display_vendor) > 0 &&
		    strlen(info.display_product) > 0) {
			vendor = info.display_vendor;
			product = info.display_product;
		} else {
			vendor = "HDMI";
			product = "";
		}
	} else if (info.display_connected) {
		vendor = "DVI";
		product = "(unsupported)";
	} else {
		vendor = "HDMI";
		product = "(disconnected)";
	}

	strlcpy(audiodev->name, vendor, sizeof(audiodev->name));
	strlcpy(audiodev->version, product, sizeof(audiodev->version));
	snprintf(audiodev->config, sizeof(audiodev->config),
	    "HDMI %d.%d", vmaj, vmin);
	return 0;
}

static int
awin_hdmiaudio_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK|AUDIO_PROP_MMAP;
}

static int
awin_hdmiaudio_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	return bs & ~3;
}

static size_t
awin_hdmiaudio_round_buffersize(void *priv, int direction, size_t bufsize)
{
	return bufsize;
}

static int
awin_hdmiaudio_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct awin_hdmiaudio_softc *sc = priv;
	struct awin_hdmiaudio_dma *dma;
	struct awin_hdmi_info info;
	bus_addr_t pstart;
	bus_size_t psize;
	uint32_t dmacfg;
	int error;

	awin_hdmi_get_info(&info);
	if (info.display_connected == false || info.display_hdmimode == false)
		return ENXIO;

	pstart = 0;
	psize = (uintptr_t)end - (uintptr_t)start;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == start) {
			pstart = dma->dma_map->dm_segs[0].ds_addr;
			break;
		}
	}
	if (pstart == 0) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}

	sc->sc_pint = intr;
	sc->sc_pintarg = intrarg;
	sc->sc_pstart = sc->sc_pcur = pstart;
	sc->sc_pend = sc->sc_pstart + psize;
	sc->sc_pblksize = blksize;
	awin_hdmi_poweron(true);

	dmacfg = 0;
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_DATA_WIDTH_32,
			    AWIN_DMA_CTL_DST_DATA_WIDTH);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_DATA_WIDTH_32,
			    AWIN_DMA_CTL_SRC_DATA_WIDTH);
	dmacfg |= AWIN_DMA_CTL_BC_REMAINING;
	dmacfg |= __SHIFTIN(sc->sc_drqtype_sdram,
			    AWIN_DMA_CTL_SRC_DRQ_TYPE);
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		/* NDMA */
		dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_4,
				    AWIN_DMA_CTL_DST_BURST_LEN);
		dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_4,
				    AWIN_DMA_CTL_SRC_BURST_LEN);
		dmacfg |= AWIN_NDMA_CTL_DST_ADDR_NOINCR;
		dmacfg |= __SHIFTIN(sc->sc_drqtype_hdmiaudio,
				    AWIN_DMA_CTL_DST_DRQ_TYPE);
	} else {
		/* DDMA */
		dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_8,
				    AWIN_DMA_CTL_DST_BURST_LEN);
		dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_8,
				    AWIN_DMA_CTL_SRC_BURST_LEN);
		dmacfg |= __SHIFTIN(AWIN_DDMA_CTL_DMA_ADDR_IO,
				    AWIN_DDMA_CTL_DST_ADDR_MODE);
		dmacfg |= __SHIFTIN(AWIN_DDMA_CTL_DMA_ADDR_LINEAR,
				    AWIN_DDMA_CTL_SRC_ADDR_MODE);
		dmacfg |= __SHIFTIN(sc->sc_drqtype_hdmiaudio,
				    AWIN_DDMA_CTL_DST_DRQ_TYPE);
	}
	awin_dma_set_config(sc->sc_pdma, dmacfg);

	error = awin_hdmiaudio_play(sc);
	if (error)
		awin_hdmiaudio_halt_output(sc);

	return error;
}

static int
awin_hdmiaudio_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	return EINVAL;
}

static void
awin_hdmiaudio_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct awin_hdmiaudio_softc *sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static stream_filter_t *
awin_hdmiaudio_swvol_filter(struct audio_softc *asc,
    const audio_params_t *from, const audio_params_t *to)
{
	auvolconv_filter_t *this;
	device_t dev = audio_get_device(asc);
	struct awin_hdmiaudio_softc *sc = device_private(dev);

	this = kmem_alloc(sizeof(auvolconv_filter_t), KM_SLEEP);
	this->base.base.fetch_to = auvolconv_slinear16_le_fetch_to;
	this->base.dtor = awin_hdmiaudio_swvol_dtor;
	this->base.set_fetcher = stream_filter_set_fetcher;
	this->base.set_inputbuffer = stream_filter_set_inputbuffer;
	this->vol = &sc->sc_swvol;

	return (stream_filter_t *)this;
}

static void
awin_hdmiaudio_swvol_dtor(stream_filter_t *this)
{
	if (this)
		kmem_free(this, sizeof(auvolconv_filter_t));
}
