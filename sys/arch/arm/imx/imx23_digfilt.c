/* $NetBSD: imx23_digfilt.c,v 1.9 2026/02/02 06:23:37 skrll Exp $ */

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/audioio.h>
#include <sys/mallocvar.h>

#include <dev/audio/audio_if.h>
#include <dev/fdt/fdtvar.h>

#include <arm/imx/imx23_digfiltreg.h>
#include <arm/imx/imx23var.h>

#define DIGFILT_DMA_NSEGS 1

struct imx23_digfilt_softc {
	device_t sc_dev;
	device_t sc_audiodev;
	struct audio_format sc_format;
	bus_space_handle_t sc_aohdl;
	struct fdtbus_dma *dma_channel;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamp;
	bus_dma_segment_t sc_ds[DIGFILT_DMA_NSEGS];
	bus_space_handle_t sc_hdl;
	bus_space_tag_t	sc_iot;
	kmutex_t sc_intr_lock;
	kmutex_t sc_lock;
	audio_params_t sc_pparam;
	void *sc_buffer;
	void *sc_intarg;
	void (*sc_intr)(void*);
	uint8_t sc_mute;
};

/* Autoconf. */
static int imx23_digfilt_match(device_t, cfdata_t, void *);
static void imx23_digfilt_attach(device_t, device_t, void *);

/* Audio driver interface. */
static int imx23_digfilt_query_format(void *, audio_format_query_t *);
static int imx23_digfilt_set_format(void *, int, const audio_params_t *,
			 const audio_params_t *, audio_filter_reg_t *,
			 audio_filter_reg_t *);
static int imx23_digfilt_round_blocksize(void *, int, int,
					 const audio_params_t *);
static int imx23_digfilt_init_output(void *, void *, int );
static int imx23_digfilt_start_output(void *, void *, int, void (*)(void *),
				      void *);
static int imx23_digfilt_halt_output(void *);
static int imx23_digfilt_getdev(void *, struct audio_device *);
static int imx23_digfilt_set_port(void *, mixer_ctrl_t *);
static int imx23_digfilt_get_port(void *, mixer_ctrl_t *);
static int imx23_digfilt_query_devinfo(void *, mixer_devinfo_t *);
static void *imx23_digfilt_allocm(void *, int, size_t);
static void imx23_digfilt_freem(void *, void *, size_t);
static size_t imx23_digfilt_round_buffersize(void *, int, size_t);
static int imx23_digfilt_get_props(void *);
static void imx23_digfilt_get_locks(void *, kmutex_t **, kmutex_t **);

/* IRQs */
static int imx23_dac_error_intr(void *);
static void imx23_dac_dma_intr(void *);

/* Audio out. */
static void imx23_digfilt_ao_apply_mutes(struct imx23_digfilt_softc *);
static void imx23_digfilt_ao_init(struct imx23_digfilt_softc *);
static void imx23_digfilt_ao_reset(struct imx23_digfilt_softc *);
static void imx23_digfilt_ao_set_rate(struct imx23_digfilt_softc *, int);

#define DIGFILT_BLOCKSIZE_MAX 8192
#define DIGFILT_BLOCKSIZE_ROUND 2048
#define DIGFILT_DMA_CHAIN_LENGTH 3
#define DIGFILT_DMA_CHANNEL 1
#define DIGFILT_MUTE_DAC 1
#define DIGFILT_MUTE_HP 2
#define DIGFILT_MUTE_LINE 4
#define DIGFILT_SOFT_RST_LOOP 455	/* At least 1 us. */

#define AO_RD(sc, reg)							\
	bus_space_read_4(sc->sc_iot, sc->sc_aohdl, (reg))
#define AO_WR(sc, reg, val)						\
	bus_space_write_4(sc->sc_iot, sc->sc_aohdl, (reg), (val))

CFATTACH_DECL_NEW(imx23digfilt, sizeof(struct imx23_digfilt_softc),
		  imx23_digfilt_match, imx23_digfilt_attach, NULL, NULL);

static const struct audio_hw_if imx23_digfilt_hw_if = {
	.open = NULL,
	.close = NULL,
	.query_format = imx23_digfilt_query_format,
	.set_format = imx23_digfilt_set_format,
	.round_blocksize = imx23_digfilt_round_blocksize,
	.commit_settings = NULL,
	.init_output = imx23_digfilt_init_output,
	.init_input = NULL,
	.start_output = imx23_digfilt_start_output,
	.start_input = NULL,
	.halt_output = imx23_digfilt_halt_output,
	.speaker_ctl = NULL,
	.getdev = imx23_digfilt_getdev,
	.set_port = imx23_digfilt_set_port,
	.get_port = imx23_digfilt_get_port,
	.query_devinfo = imx23_digfilt_query_devinfo,
	.allocm = imx23_digfilt_allocm,
	.freem = imx23_digfilt_freem,
	.round_buffersize = imx23_digfilt_round_buffersize,
	.get_props = imx23_digfilt_get_props,
	.trigger_output = NULL,
	.trigger_input = NULL,
	.dev_ioctl = NULL,
	.get_locks = imx23_digfilt_get_locks
};

enum {
	DIGFILT_OUTPUT_CLASS,
	DIGFILT_OUTPUT_DAC_VOLUME,
	DIGFILT_OUTPUT_DAC_MUTE,
	DIGFILT_OUTPUT_HP_VOLUME,
	DIGFILT_OUTPUT_HP_MUTE,
	DIGFILT_OUTPUT_LINE_VOLUME,
	DIGFILT_OUTPUT_LINE_MUTE,
	DIGFILT_ENUM_LAST
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-audioout" },
	DEVICE_COMPAT_EOL
};

static int
imx23_digfilt_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_digfilt_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_digfilt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;
	char intrstr[128];

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	/* Map DIGFILT bus space. */
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	/* Map AUDIOOUT subregion from parent bus space. */
	if (bus_space_subregion(sc->sc_iot, sc->sc_hdl, 0, size,
				&sc->sc_aohdl)) {
		aprint_error_dev(sc->sc_dev,
				 "Unable to submap AUDIOOUT bus space\n");
		return;
	}

	/* acquire DMA channel */
	sc->dma_channel = fdtbus_dma_get(phandle,"tx", imx23_dac_dma_intr, sc);
	if (sc->dma_channel == NULL) {
		aprint_error(": couldn't get dma access\n");
		return;
	}

	/* Allocate DMA for audio buffer. */
	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, DIGFILT_DMA_NSEGS,
				  MAXPHYS, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
				  &sc->sc_dmamp);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Unable to allocate DMA handle\n");
		return;
	}

	imx23_digfilt_ao_reset(sc);	/* Reset AUDIOOUT. */

	uint32_t v = AO_RD(sc, HW_AUDIOOUT_VERSION);
	aprint_normal(": DIGFILT Block v%" __PRIuBIT ".%" __PRIuBIT
		".%" __PRIuBIT "\n",
		__SHIFTOUT(v, HW_AUDIOOUT_VERSION_MAJOR),
		__SHIFTOUT(v, HW_AUDIOOUT_VERSION_MINOR),
		__SHIFTOUT(v, HW_AUDIOOUT_VERSION_STEP));

	imx23_digfilt_ao_init(sc);
	/* Default sample rate 44.1 kHz. */
	imx23_digfilt_ao_set_rate(sc, 44100);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	/* HW supported formats. */
	sc->sc_format.mode = AUMODE_PLAY;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 8;
	sc->sc_format.frequency[0] = 8000;
	sc->sc_format.frequency[1] = 11025;
	sc->sc_format.frequency[2] = 12000;
	sc->sc_format.frequency[3] = 16000;
	sc->sc_format.frequency[4] = 22050;
	sc->sc_format.frequency[5] = 24000;
	sc->sc_format.frequency[6] = 32000;
	sc->sc_format.frequency[7] = 44100;

	sc->sc_audiodev = audio_attach_mi(&imx23_digfilt_hw_if, sc, sc->sc_dev);

	/* Default mutes. */
	sc->sc_mute = DIGFILT_MUTE_LINE;
	imx23_digfilt_ao_apply_mutes(sc);

	/* establish error interrupt */
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SCHED, IST_LEVEL,
					       imx23_dac_error_intr, sc,
					       device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish error interrupt\n");
		return;
	}

	aprint_normal("\n");

	return;
}

static int
imx23_digfilt_query_format(void *priv, audio_format_query_t *afp)
{
	struct imx23_digfilt_softc *sc = priv;

	return audio_query_format(&sc->sc_format, 1, afp);
}

static int
imx23_digfilt_set_format(void *priv, int setmode, const audio_params_t *play,
			 const audio_params_t *rec, audio_filter_reg_t *pfil,
			 audio_filter_reg_t *rfil)
{
	struct imx23_digfilt_softc *sc = priv;

	if ((setmode & AUMODE_PLAY)) {
		sc->sc_pparam = *play;

		/* At this point bitrate should be figured out. */
		imx23_digfilt_ao_set_rate(sc, sc->sc_pparam.sample_rate);
	}

	return 0;
}

static int
imx23_digfilt_round_blocksize(void *priv, int bs, int mode,
			const audio_params_t *param)
{
	int blocksize;

	if (bs > DIGFILT_BLOCKSIZE_MAX)
		blocksize = DIGFILT_BLOCKSIZE_MAX;
	else
		blocksize = bs & ~(DIGFILT_BLOCKSIZE_ROUND-1);
	if (blocksize < DIGFILT_BLOCKSIZE_ROUND)
		blocksize = DIGFILT_BLOCKSIZE_ROUND;

	return blocksize;
}

static int
imx23_digfilt_init_output(void *priv, void *buffer, int size)
{
	return 0;
}

static int
imx23_digfilt_start_output(void *priv, void *start, int bs,
			   void (*intr)(void *), void *intarg)
{
	struct imx23_digfilt_softc *sc = priv;
	struct fdtbus_dma_req req;

	sc->sc_intr = intr;
	sc->sc_intarg = intarg;

	/* synthesize a dma segment with the correct offset+size */
	bus_addr_t offset = (bus_addr_t)start - (bus_addr_t)sc->sc_buffer;
	bus_dma_segment_t dma_seg;
	dma_seg.ds_addr = sc->sc_dmamp->dm_segs[0].ds_addr + offset;
	dma_seg.ds_len = bs;

	/* configure DMA request */
	req.dreq_dir = FDT_DMA_WRITE;
	req.dreq_segs = &dma_seg;
	req.dreq_nsegs = 1;
	req.dreq_block_irq = 0;

	/* configure DMA PIO words */
	uint32_t pio_words[1];
	pio_words[0] = HW_AUDIOOUT_CTRL_WORD_LENGTH |
		       HW_AUDIOOUT_CTRL_RUN;
	req.dreq_datalen = 1;
	req.dreq_data = &pio_words;

	/* send it off*/
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamp, offset, bs,
			BUS_DMASYNC_PREWRITE);
	fdtbus_dma_transfer(sc->dma_channel, &req);

	return 0;
}

static int
imx23_digfilt_halt_output(void *priv)
{
	struct imx23_digfilt_softc *sc = priv;

	/* We have intr lock when this is called */
	sc->sc_intr = NULL;

	return 0;
}

static int
imx23_digfilt_getdev(void *priv, struct audio_device *ad)
{
	struct imx23_digfilt_softc *sc = priv;

	strncpy(ad->name, device_xname(sc->sc_dev), MAX_AUDIO_DEV_LEN);
	strncpy(ad->version, "", MAX_AUDIO_DEV_LEN);
	strncpy(ad->config, "", MAX_AUDIO_DEV_LEN);

	return 0;
}

static int
imx23_digfilt_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct imx23_digfilt_softc *sc = priv;
	uint32_t val;
	uint8_t nvol;

	switch (mc->dev) {
	case DIGFILT_OUTPUT_DAC_VOLUME:
		val = AO_RD(sc, HW_AUDIOOUT_DACVOLUME);
		val &= ~(HW_AUDIOOUT_DACVOLUME_VOLUME_LEFT |
		    HW_AUDIOOUT_DACVOLUME_VOLUME_RIGHT);

		/* DAC volume field is 8 bits. */
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		if (nvol > 0xff)
			nvol = 0xff;

		val |= __SHIFTIN(nvol, HW_AUDIOOUT_DACVOLUME_VOLUME_LEFT);

		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		if (nvol > 0xff)
			nvol = 0xff;

		val |= __SHIFTIN(nvol, HW_AUDIOOUT_DACVOLUME_VOLUME_RIGHT);

		AO_WR(sc, HW_AUDIOOUT_DACVOLUME, val);

		return 0;

	case DIGFILT_OUTPUT_HP_VOLUME:
		val = AO_RD(sc, HW_AUDIOOUT_HPVOL);
		val &= ~(HW_AUDIOOUT_HPVOL_VOL_LEFT |
		    HW_AUDIOOUT_HPVOL_VOL_RIGHT);

		/* HP volume field is 7 bits. */
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		if (nvol > 0x7f)
			nvol = 0x7f;

		nvol = ~nvol;
		val |= __SHIFTIN(nvol, HW_AUDIOOUT_HPVOL_VOL_LEFT);

		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		if (nvol > 0x7f)
			nvol = 0x7f;

		nvol = ~nvol;
		val |= __SHIFTIN(nvol, HW_AUDIOOUT_HPVOL_VOL_RIGHT);

		AO_WR(sc, HW_AUDIOOUT_HPVOL, val);

		return 0;

	case DIGFILT_OUTPUT_LINE_VOLUME:
		return 1;

	case DIGFILT_OUTPUT_DAC_MUTE:
		if (mc->un.ord)
			sc->sc_mute |= DIGFILT_MUTE_DAC;
		else
			sc->sc_mute &= ~DIGFILT_MUTE_DAC;

		imx23_digfilt_ao_apply_mutes(sc);

		return 0;

	case DIGFILT_OUTPUT_HP_MUTE:
		if (mc->un.ord)
			sc->sc_mute |= DIGFILT_MUTE_HP;
		else
			sc->sc_mute &= ~DIGFILT_MUTE_HP;

		imx23_digfilt_ao_apply_mutes(sc);

		return 0;

	case DIGFILT_OUTPUT_LINE_MUTE:
		if (mc->un.ord)
			sc->sc_mute |= DIGFILT_MUTE_LINE;
		else
			sc->sc_mute &= ~DIGFILT_MUTE_LINE;

		imx23_digfilt_ao_apply_mutes(sc);

		return 0;
	}

	return ENXIO;
}

static int
imx23_digfilt_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct imx23_digfilt_softc *sc = priv;
	uint32_t val;
	uint8_t nvol;

	switch (mc->dev) {
	case DIGFILT_OUTPUT_DAC_VOLUME:
		val = AO_RD(sc, HW_AUDIOOUT_DACVOLUME);

		nvol = __SHIFTOUT(val, HW_AUDIOOUT_DACVOLUME_VOLUME_LEFT);
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = nvol;

		nvol = __SHIFTOUT(val, HW_AUDIOOUT_DACVOLUME_VOLUME_RIGHT);
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = nvol;

		return 0;

	case DIGFILT_OUTPUT_HP_VOLUME:
		val = AO_RD(sc, HW_AUDIOOUT_HPVOL);

		nvol = __SHIFTOUT(val, HW_AUDIOOUT_HPVOL_VOL_LEFT);
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = ~nvol & 0x7f;

		nvol = __SHIFTOUT(val, HW_AUDIOOUT_HPVOL_VOL_RIGHT);
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = ~nvol & 0x7f;

		return 0;

	case DIGFILT_OUTPUT_LINE_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 255;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 255;

		return 0;

	case DIGFILT_OUTPUT_DAC_MUTE:
		val = AO_RD(sc, HW_AUDIOOUT_DACVOLUME);

		mc->un.ord = (val & (HW_AUDIOOUT_DACVOLUME_MUTE_LEFT |
		    HW_AUDIOOUT_DACVOLUME_MUTE_RIGHT)) ? 1 : 0;

		return 0;

	case DIGFILT_OUTPUT_HP_MUTE:
		val = AO_RD(sc, HW_AUDIOOUT_HPVOL);

		mc->un.ord = (val & HW_AUDIOOUT_HPVOL_MUTE) ? 1 : 0;

		return 0;

	case DIGFILT_OUTPUT_LINE_MUTE:
		val = AO_RD(sc, HW_AUDIOOUT_SPEAKERCTRL);

		mc->un.ord = (val & HW_AUDIOOUT_SPEAKERCTRL_MUTE) ? 1 : 0;

		return 0;
	}

	return ENXIO;
}

static int
imx23_digfilt_query_devinfo(void *priv, mixer_devinfo_t *di)
{

	switch (di->index) {
	case DIGFILT_OUTPUT_CLASS:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case DIGFILT_OUTPUT_DAC_VOLUME:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->prev = AUDIO_MIXER_LAST;
		di->next = DIGFILT_OUTPUT_DAC_MUTE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case DIGFILT_OUTPUT_DAC_MUTE:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		di->type = AUDIO_MIXER_ENUM;
		di->prev = DIGFILT_OUTPUT_DAC_VOLUME;
		di->next = AUDIO_MIXER_LAST;
mute:
		strlcpy(di->label.name, AudioNmute, sizeof(di->label.name));
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNon,
		    sizeof(di->un.e.member[0].label.name));
		di->un.e.member[0].ord = 1;
		strlcpy(di->un.e.member[1].label.name, AudioNoff,
		    sizeof(di->un.e.member[1].label.name));
		di->un.e.member[1].ord = 0;
		return 0;

	case DIGFILT_OUTPUT_HP_VOLUME:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNheadphone);
		di->type = AUDIO_MIXER_VALUE;
		di->prev = AUDIO_MIXER_LAST;
		di->next = DIGFILT_OUTPUT_HP_MUTE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case DIGFILT_OUTPUT_HP_MUTE:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		di->type = AUDIO_MIXER_ENUM;
		di->prev = DIGFILT_OUTPUT_HP_VOLUME;
		di->next = AUDIO_MIXER_LAST;
		goto mute;

	case DIGFILT_OUTPUT_LINE_VOLUME:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNline);
		di->type = AUDIO_MIXER_VALUE;
		di->prev = AUDIO_MIXER_LAST;
		di->next = DIGFILT_OUTPUT_LINE_MUTE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case DIGFILT_OUTPUT_LINE_MUTE:
		di->mixer_class = DIGFILT_OUTPUT_CLASS;
		di->type = AUDIO_MIXER_ENUM;
		di->prev = DIGFILT_OUTPUT_LINE_VOLUME;
		di->next = AUDIO_MIXER_LAST;
		goto mute;
	}

	return ENXIO;
}

static void *
imx23_digfilt_allocm(void *priv, int direction, size_t size)
{
	struct imx23_digfilt_softc *sc = priv;
	int rsegs;
	int error;

	sc->sc_buffer = NULL;

	/*
	 * AUMODE_PLAY is DMA from memory to device.
	 */
	if (direction != AUMODE_PLAY)
		return NULL;

	error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &sc->sc_ds[0], DIGFILT_DMA_NSEGS, &rsegs, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "bus_dmamem_alloc: %d\n", error);
		goto out;
	}

	error = bus_dmamem_map(sc->sc_dmat, sc->sc_ds, DIGFILT_DMA_NSEGS, size, &sc->sc_buffer, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_map: %d\n", error);
		goto dmamem_free;
	}

	/* After load sc_dmamp is valid. */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamp, sc->sc_buffer, size, NULL, BUS_DMA_NOWAIT|BUS_DMA_WRITE);
	if (error) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load: %d\n", error);
		goto dmamem_unmap;
	}

	memset(sc->sc_buffer, 0x00, size);

	return sc->sc_buffer;

dmamem_unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_buffer, size);
dmamem_free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_ds, DIGFILT_DMA_NSEGS);
out:
	return NULL;
}

static void
imx23_digfilt_freem(void *priv, void *kvap, size_t size)
{
	struct imx23_digfilt_softc *sc = priv;

	bus_dmamem_unmap(sc->sc_dmat, kvap, size);
	bus_dmamem_free(sc->sc_dmat, sc->sc_ds, DIGFILT_DMA_NSEGS);

	return;
}

static size_t
imx23_digfilt_round_buffersize(void *hdl, int direction, size_t bs)
{
	return bs;
}

static int
imx23_digfilt_get_props(void *sc)
{
	return AUDIO_PROP_PLAYBACK;
}

static void
imx23_digfilt_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct imx23_digfilt_softc *sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;

	return;
}

/*
 * IRQ for DAC error.
 */
static int
imx23_dac_error_intr(void *arg)
{
	struct imx23_digfilt_softc *sc = arg;
	AO_WR(sc, HW_AUDIOOUT_CTRL_CLR, HW_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);
	return 1;
}

/*
 * IRQ from DMA.
 */
static void
imx23_dac_dma_intr(void *arg)
{
	struct imx23_digfilt_softc *sc = arg;

	mutex_enter(&sc->sc_intr_lock);

	/* When halting, the driver finishes playing the current block, but
	 * the audio subsystem no longer expects us to call back */
	if (sc->sc_intr != NULL) {
		sc->sc_intr(sc->sc_intarg);
	}

	mutex_exit(&sc->sc_intr_lock);
}

static void
imx23_digfilt_ao_apply_mutes(struct imx23_digfilt_softc *sc)
{

	/* DAC. */
	if (sc->sc_mute & DIGFILT_MUTE_DAC) {
		AO_WR(sc, HW_AUDIOOUT_DACVOLUME_SET,
		    HW_AUDIOOUT_DACVOLUME_MUTE_LEFT |
		    HW_AUDIOOUT_DACVOLUME_MUTE_RIGHT
		);

	} else {
		AO_WR(sc, HW_AUDIOOUT_DACVOLUME_CLR,
		    HW_AUDIOOUT_DACVOLUME_MUTE_LEFT |
		    HW_AUDIOOUT_DACVOLUME_MUTE_RIGHT
		);
	}

	/* HP. */
	if (sc->sc_mute & DIGFILT_MUTE_HP)
		AO_WR(sc, HW_AUDIOOUT_HPVOL_SET, HW_AUDIOOUT_HPVOL_MUTE);
	else
		AO_WR(sc, HW_AUDIOOUT_HPVOL_CLR, HW_AUDIOOUT_HPVOL_MUTE);

	/* Line. */
	if (sc->sc_mute & DIGFILT_MUTE_LINE)
		AO_WR(sc, HW_AUDIOOUT_SPEAKERCTRL_SET,
		    HW_AUDIOOUT_SPEAKERCTRL_MUTE);
	else
		AO_WR(sc, HW_AUDIOOUT_SPEAKERCTRL_CLR,
		    HW_AUDIOOUT_SPEAKERCTRL_MUTE);

	return;
}

/*
 * Initialize audio system.
 */
static void
imx23_digfilt_ao_init(struct imx23_digfilt_softc *sc)
{

	AO_WR(sc, HW_AUDIOOUT_ANACLKCTRL_CLR, HW_AUDIOOUT_ANACLKCTRL_CLKGATE);
	while ((AO_RD(sc, HW_AUDIOOUT_ANACLKCTRL) &
	    HW_AUDIOOUT_ANACLKCTRL_CLKGATE));

	/* Hold headphones outputs at ground. */
	AO_WR(sc, HW_AUDIOOUT_ANACTRL_SET, HW_AUDIOOUT_ANACTRL_HP_HOLD_GND);

	/* Release pull down */
	AO_WR(sc, HW_AUDIOOUT_ANACTRL_CLR, HW_AUDIOOUT_ANACTRL_HP_HOLD_GND);

	AO_WR(sc, HW_AUDIOOUT_ANACTRL_SET, HW_AUDIOOUT_ANACTRL_HP_CLASSAB);

	/* Enable Modules. */
	AO_WR(sc, HW_AUDIOOUT_PWRDN_CLR,
	    HW_AUDIOOUT_PWRDN_RIGHT_ADC |
	    HW_AUDIOOUT_PWRDN_DAC |
	    HW_AUDIOOUT_PWRDN_CAPLESS |
	    HW_AUDIOOUT_PWRDN_HEADPHONE
	);

	return;
}

/*
 * Reset the AUDIOOUT block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
imx23_digfilt_ao_reset(struct imx23_digfilt_softc *sc)
{
	unsigned int loop;

	/* Prepare for soft-reset by making sure that SFTRST is not currently
	* asserted. Also clear CLKGATE so we can wait for its assertion below.
	*/
	AO_WR(sc, HW_AUDIOOUT_CTRL_CLR, HW_AUDIOOUT_CTRL_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((AO_RD(sc, HW_AUDIOOUT_CTRL) & HW_AUDIOOUT_CTRL_SFTRST) ||
	    (loop < DIGFILT_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	AO_WR(sc, HW_AUDIOOUT_CTRL_CLR, HW_AUDIOOUT_CTRL_CLKGATE);

	/* Soft-reset the block. */
	AO_WR(sc, HW_AUDIOOUT_CTRL_SET, HW_AUDIOOUT_CTRL_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(AO_RD(sc, HW_AUDIOOUT_CTRL) & HW_AUDIOOUT_CTRL_CLKGATE));

	/* Bring block out of reset. */
	AO_WR(sc, HW_AUDIOOUT_CTRL_CLR, HW_AUDIOOUT_CTRL_SFTRST);

	loop = 0;
	while ((AO_RD(sc, HW_AUDIOOUT_CTRL) & HW_AUDIOOUT_CTRL_SFTRST) ||
	    (loop < DIGFILT_SOFT_RST_LOOP))
		loop++;

	AO_WR(sc, HW_AUDIOOUT_CTRL_CLR, HW_AUDIOOUT_CTRL_CLKGATE);

	/* Wait until clock is in the NON-gated state. */
	while (AO_RD(sc, HW_AUDIOOUT_CTRL) & HW_AUDIOOUT_CTRL_CLKGATE);

	return;
}

static void
imx23_digfilt_ao_set_rate(struct imx23_digfilt_softc *sc, int sr)
{
	uint32_t val;


	val = AO_RD(sc, HW_AUDIOOUT_DACSRR);


	val &= ~(HW_AUDIOOUT_DACSRR_BASEMULT | HW_AUDIOOUT_DACSRR_SRC_HOLD |
	    HW_AUDIOOUT_DACSRR_SRC_INT | HW_AUDIOOUT_DACSRR_SRC_FRAC);

	switch(sr) {
	case 8000:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x3, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x17, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x0E00, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	case 11025:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x3, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x11, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x0037, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	case 12000:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x3, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x0F, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x13FF, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	case 16000:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x1, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x17, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x0E00, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	case 22050:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x1, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x11, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x0037, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	case 24000:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x1, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x0F, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x13FF, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	case 32000:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x0, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x17, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x0E00, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	default:
		aprint_error_dev(sc->sc_dev, "unknown sample rate: %d\n", sr);
	case 44100:
		val |= (__SHIFTIN(0x1 ,HW_AUDIOOUT_DACSRR_BASEMULT) |
		    __SHIFTIN(0x0, HW_AUDIOOUT_DACSRR_SRC_HOLD) |
		    __SHIFTIN(0x11, HW_AUDIOOUT_DACSRR_SRC_INT) |
		    __SHIFTIN(0x0037, HW_AUDIOOUT_DACSRR_SRC_FRAC));
		break;
	}

	AO_WR(sc, HW_AUDIOOUT_DACSRR, val);

	val = AO_RD(sc, HW_AUDIOOUT_DACSRR);

	return;
}
