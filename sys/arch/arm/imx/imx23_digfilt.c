/* $Id: imx23_digfilt.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $ */

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
#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <sys/mallocvar.h>
#include <arm/imx/imx23_digfiltreg.h>
#include <arm/imx/imx23_rtcvar.h>
#include <arm/imx/imx23_clkctrlvar.h>
#include <arm/imx/imx23_apbdmavar.h>
#include <arm/imx/imx23_icollreg.h>
#include <arm/imx/imx23var.h>

#include <arm/pic/picvar.h>

/* Autoconf. */
static int digfilt_match(device_t, cfdata_t, void *);
static void digfilt_attach(device_t, device_t, void *);
static int digfilt_activate(device_t, enum devact);

/* Audio driver interface. */
static int digfilt_drain(void *);
static int digfilt_query_encoding(void *, struct audio_encoding *);
static int digfilt_set_params(void *, int, int, audio_params_t *,
    audio_params_t *, stream_filter_list_t *,
    stream_filter_list_t *);
static int digfilt_round_blocksize(void *, int, int, const audio_params_t *);
static int digfilt_init_output(void *, void *, int );
static int digfilt_start_output(void *, void *, int, void (*)(void *), void *);
static int digfilt_halt_output(void *);
static int digfilt_getdev(void *, struct audio_device *);
static int digfilt_set_port(void *, mixer_ctrl_t *);
static int digfilt_get_port(void *, mixer_ctrl_t *);
static int digfilt_query_devinfo(void *, mixer_devinfo_t *);
static void *digfilt_allocm(void *, int, size_t);
static void digfilt_freem(void *, void *, size_t);
static size_t digfilt_round_buffersize(void *, int, size_t);
static int digfilt_get_props(void *);
static void digfilt_get_locks(void *, kmutex_t **, kmutex_t **);

/* IRQs */
static int dac_error_intr(void *);
static int dac_dma_intr(void *);

struct digfilt_softc;

/* Audio out. */
static void *digfilt_ao_alloc_dmachain(void *, size_t);
static void digfilt_ao_apply_mutes(struct digfilt_softc *);
static void digfilt_ao_init(struct digfilt_softc *);
static void digfilt_ao_reset(struct digfilt_softc *);
static void digfilt_ao_set_rate(struct digfilt_softc *, int);

/* Audio in. */
#if 0
static void digfilt_ai_reset(struct digfilt_softc *);
#endif

#define DIGFILT_DMA_NSEGS 1
#define DIGFILT_BLOCKSIZE_MAX 4096
#define DIGFILT_BLOCKSIZE_ROUND 512
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
#define AI_RD(sc, reg)							\
	bus_space_read_4(sc->sc_iot, sc->sc_aihdl, (reg))
#define AI_WR(sc, reg, val)						\
	bus_space_write_4(sc->sc_iot, sc->sc_aihdl, (reg), (val))

struct digfilt_softc {
	device_t sc_dev;
	device_t sc_audiodev;
	struct audio_format sc_format;
	struct audio_encoding_set *sc_encodings;
	bus_space_handle_t sc_aihdl;
	bus_space_handle_t sc_aohdl;
	apbdma_softc_t sc_dmac;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamp;
	bus_dmamap_t sc_c_dmamp;
	bus_dma_segment_t sc_ds[DIGFILT_DMA_NSEGS];
	bus_dma_segment_t sc_c_ds[DIGFILT_DMA_NSEGS];
	bus_space_handle_t sc_hdl;
	kmutex_t sc_intr_lock;
	bus_space_tag_t	sc_iot;
	kmutex_t sc_lock;
	audio_params_t sc_pparam;
	void *sc_buffer;
	void *sc_dmachain;
	void *sc_intarg;
	void (*sc_intr)(void*);
	uint8_t sc_mute;
	uint8_t sc_cmd_index;
};

CFATTACH_DECL3_NEW(digfilt,
	sizeof(struct digfilt_softc),
	digfilt_match,
	digfilt_attach,
	NULL,
	digfilt_activate,
	NULL,
	NULL,
	0);

static const struct audio_hw_if digfilt_hw_if = {
	.open = NULL,
	.close = NULL,
	.drain = digfilt_drain,
	.query_encoding = digfilt_query_encoding,
	.set_params = digfilt_set_params,
	.round_blocksize = digfilt_round_blocksize,
	.commit_settings = NULL,
	.init_output = digfilt_init_output,
	.init_input = NULL,
	.start_output = digfilt_start_output,
	.start_input = NULL,
	.halt_output = digfilt_halt_output,
	.speaker_ctl = NULL,
	.getdev = digfilt_getdev,
	.setfd = NULL,
	.set_port = digfilt_set_port,
	.get_port = digfilt_get_port,
	.query_devinfo = digfilt_query_devinfo,
	.allocm = digfilt_allocm,
	.freem = digfilt_freem,
	.round_buffersize = digfilt_round_buffersize,
	.mappage = NULL,
	.get_props = digfilt_get_props,
	.trigger_output = NULL,
	.trigger_input = NULL,
	.dev_ioctl = NULL,
	.get_locks = digfilt_get_locks
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

static int
digfilt_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if (aa->aa_addr == HW_DIGFILT_BASE && aa->aa_size == HW_DIGFILT_SIZE)
		return 1;
	else
		return 0;
}

static void
digfilt_attach(device_t parent, device_t self, void *aux)
{
	struct apb_softc *sc_parent = device_private(parent);
	struct digfilt_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	static int digfilt_attached = 0;
	int error;
	uint32_t v;
	void *intr;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	/* This driver requires DMA functionality from the bus.
	 * Parent bus passes handle to the DMA controller instance. */
	if (sc_parent->dmac == NULL) {
		aprint_error_dev(sc->sc_dev, "DMA functionality missing\n");
		return;
	}
	sc->sc_dmac = device_private(sc_parent->dmac);
	
	if (aa->aa_addr == HW_DIGFILT_BASE && digfilt_attached) {
		aprint_error_dev(sc->sc_dev, "DIGFILT already attached\n");
		return;
	}

	/* Allocate DMA for audio buffer. */
	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, DIGFILT_DMA_NSEGS,
		MAXPHYS, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &sc->sc_dmamp);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to allocate DMA handle\n");
		return;
	}

	/* Allocate for DMA chain. */
	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, DIGFILT_DMA_NSEGS,
		MAXPHYS, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &sc->sc_c_dmamp);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to allocate DMA handle\n");
		return;
	}

	/* Map DIGFILT bus space. */
	if (bus_space_map(sc->sc_iot, HW_DIGFILT_BASE, HW_DIGFILT_SIZE, 0,
	    &sc->sc_hdl)) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to map DIGFILT bus space\n");
		return;
	}

	/* Map AUDIOOUT subregion from parent bus space. */
	if (bus_space_subregion(sc->sc_iot, sc->sc_hdl,
	    (HW_AUDIOOUT_BASE - HW_DIGFILT_BASE), HW_AUDIOOUT_SIZE,
	    &sc->sc_aohdl)) {
		aprint_error_dev(sc->sc_dev,
			"Unable to submap AUDIOOUT bus space\n");
		return;
	}

	/* Map AUDIOIN subregion from parent bus space. */
	if (bus_space_subregion(sc->sc_iot, sc->sc_hdl,
	    (HW_AUDIOIN_BASE - HW_DIGFILT_BASE), HW_AUDIOIN_SIZE,
	    &sc->sc_aihdl)) {
		aprint_error_dev(sc->sc_dev,
			"Unable to submap AUDIOIN bus space\n");
		return;
	}

	/* Enable clocks to the DIGFILT block. */
	clkctrl_en_filtclk();
	delay(10);

	digfilt_ao_reset(sc);	/* Reset AUDIOOUT. */
	/* Not yet: digfilt_ai_reset(sc); */
	
	v = AO_RD(sc, HW_AUDIOOUT_VERSION);
	aprint_normal(": DIGFILT Block v%" __PRIuBIT ".%" __PRIuBIT
		".%" __PRIuBIT "\n",
		__SHIFTOUT(v, HW_AUDIOOUT_VERSION_MAJOR),
		__SHIFTOUT(v, HW_AUDIOOUT_VERSION_MINOR),
		__SHIFTOUT(v, HW_AUDIOOUT_VERSION_STEP));

	digfilt_ao_init(sc);
	digfilt_ao_set_rate(sc, 44100);	/* Default sample rate 44.1 kHz. */

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	/* HW supported formats. */
	sc->sc_format.mode = AUMODE_PLAY|AUMODE_RECORD;
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

	if (auconv_create_encodings(&sc->sc_format, 1, &sc->sc_encodings)) {
		aprint_error_dev(self, "could not create encodings\n");
		return;
	}

	sc->sc_audiodev = audio_attach_mi(&digfilt_hw_if, sc, sc->sc_dev);

	/* Default mutes. */
	sc->sc_mute = DIGFILT_MUTE_LINE;
	digfilt_ao_apply_mutes(sc);

	/* Allocate DMA safe memory for the DMA chain. */
	sc->sc_dmachain = digfilt_ao_alloc_dmachain(sc,
		sizeof(struct apbdma_command) * DIGFILT_DMA_CHAIN_LENGTH);
	if (sc->sc_dmachain == NULL) {
		aprint_error_dev(self, "digfilt_ao_alloc_dmachain failed\n");
		return;
	}

	intr = intr_establish(IRQ_DAC_DMA, IPL_SCHED, IST_LEVEL, dac_dma_intr,
			sc);
	if (intr == NULL) {
		aprint_error_dev(sc->sc_dev,
			"Unable to establish IRQ for DAC_DMA\n");
		return;
	}

	intr = intr_establish(IRQ_DAC_ERROR, IPL_SCHED, IST_LEVEL,
		dac_error_intr, sc);
	if (intr == NULL) {
		aprint_error_dev(sc->sc_dev,
			"Unable to establish IRQ for DAC_ERROR\n");
		return;
	}

	/* Initialize DMA channel. */
	apbdma_chan_init(sc->sc_dmac, DIGFILT_DMA_CHANNEL);

	digfilt_attached = 1;

	return;
}

static int
digfilt_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

static int
digfilt_drain(void *priv)
{

	struct digfilt_softc *sc = priv;

	apbdma_wait(sc->sc_dmac, 1);
	sc->sc_cmd_index = 0;
	
	return 0;
}

static int
digfilt_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct digfilt_softc *sc = priv;
	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
digfilt_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct digfilt_softc *sc = priv;
	int index;

	if (play && (setmode & AUMODE_PLAY)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_PLAY, play, true, pfil);
		if (index < 0)
			return EINVAL;
		sc->sc_pparam = pfil->req_size > 0 ?
		    pfil->filters[0].param :
		    *play;

		/* At this point bitrate should be figured out. */
		digfilt_ao_set_rate(sc, sc->sc_pparam.sample_rate);
	}

	return 0;
}

static int
digfilt_round_blocksize(void *priv, int bs, int mode,
const audio_params_t *param)
{
	int blocksize;

	if (bs > DIGFILT_BLOCKSIZE_MAX)
		blocksize = DIGFILT_BLOCKSIZE_MAX;
	else
		blocksize = bs & ~(DIGFILT_BLOCKSIZE_ROUND-1);

	return blocksize;
}

static int
digfilt_init_output(void *priv, void *buffer, int size)
{
	struct digfilt_softc *sc = priv;
	apbdma_command_t dma_cmd;
	int i;
	dma_cmd = sc->sc_dmachain;
	sc->sc_cmd_index = 0;

	/*
	 * Build circular DMA command chain template for later use.
	 */
	for (i = 0; i < DIGFILT_DMA_CHAIN_LENGTH; i++) {
		/* Last entry loops back to first. */
		if (i == DIGFILT_DMA_CHAIN_LENGTH - 1)
			dma_cmd[i].next = (void *)(sc->sc_c_dmamp->dm_segs[0].ds_addr);
		else
			dma_cmd[i].next = (void *)(sc->sc_c_dmamp->dm_segs[0].ds_addr + (sizeof(struct apbdma_command) * (1 + i)));

		dma_cmd[i].control = __SHIFTIN(DIGFILT_BLOCKSIZE_MAX,  APBDMA_CMD_XFER_COUNT) |
		    __SHIFTIN(1, APBDMA_CMD_CMDPIOWORDS) |
		    APBDMA_CMD_SEMAPHORE |
		    APBDMA_CMD_IRQONCMPLT |
		    APBDMA_CMD_CHAIN |
		    __SHIFTIN(APBDMA_CMD_DMA_READ, APBDMA_CMD_COMMAND);

		dma_cmd[i].buffer = (void *)(sc->sc_c_dmamp->dm_segs[0].ds_addr);

		dma_cmd[i].pio_words[0] = HW_AUDIOOUT_CTRL_WORD_LENGTH |
		    HW_AUDIOOUT_CTRL_FIFO_ERROR_IRQ_EN |
		    HW_AUDIOOUT_CTRL_RUN;

	}

	apbdma_chan_set_chain(sc->sc_dmac, DIGFILT_DMA_CHANNEL, sc->sc_c_dmamp);

	return 0;
}

static int
digfilt_start_output(void *priv, void *start, int bs, void (*intr)(void*), void *intarg)
{
	struct digfilt_softc *sc = priv;
	apbdma_command_t dma_cmd;
	bus_addr_t offset;

	sc->sc_intr = intr;
	sc->sc_intarg = intarg;
	dma_cmd = sc->sc_dmachain;

	offset = (bus_addr_t)start - (bus_addr_t)sc->sc_buffer;

	dma_cmd[sc->sc_cmd_index].buffer =
	    (void *)((bus_addr_t)sc->sc_dmamp->dm_segs[0].ds_addr + offset);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamp, offset, bs, BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_c_dmamp,
	    sizeof(struct apbdma_command) * sc->sc_cmd_index, sizeof(struct apbdma_command), BUS_DMASYNC_PREWRITE);

	sc->sc_cmd_index++;
	if (sc->sc_cmd_index > DIGFILT_DMA_CHAIN_LENGTH - 1)
		sc->sc_cmd_index = 0;

	apbdma_run(sc->sc_dmac, DIGFILT_DMA_CHANNEL);

	return 0;
}

static int
digfilt_halt_output(void *priv)
{
	return 0;
}

static int
digfilt_getdev(void *priv, struct audio_device *ad)
{
	struct digfilt_softc *sc = priv;

	strncpy(ad->name, device_xname(sc->sc_dev), MAX_AUDIO_DEV_LEN);
	strncpy(ad->version, "", MAX_AUDIO_DEV_LEN);
	strncpy(ad->config, "", MAX_AUDIO_DEV_LEN);

	return 0;
}

static int
digfilt_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct digfilt_softc *sc = priv;
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

		digfilt_ao_apply_mutes(sc);

		return 0;

	case DIGFILT_OUTPUT_HP_MUTE:
		if (mc->un.ord)
			sc->sc_mute |= DIGFILT_MUTE_HP;
		else
			sc->sc_mute &= ~DIGFILT_MUTE_HP;

		digfilt_ao_apply_mutes(sc);

		return 0;

	case DIGFILT_OUTPUT_LINE_MUTE:
		if (mc->un.ord)
			sc->sc_mute |= DIGFILT_MUTE_LINE;
		else
			sc->sc_mute &= ~DIGFILT_MUTE_LINE;

		digfilt_ao_apply_mutes(sc);

		return 0;
	}

	return ENXIO;
}

static int
digfilt_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct digfilt_softc *sc = priv;
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
digfilt_query_devinfo(void *priv, mixer_devinfo_t *di)
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
digfilt_allocm(void *priv, int direction, size_t size)
{
	struct digfilt_softc *sc = priv;
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
digfilt_freem(void *priv, void *kvap, size_t size)
{
	struct digfilt_softc *sc = priv;

	bus_dmamem_unmap(sc->sc_dmat, kvap, size);
	bus_dmamem_free(sc->sc_dmat, sc->sc_ds, DIGFILT_DMA_NSEGS);

	return;
}

static size_t
digfilt_round_buffersize(void *hdl, int direction, size_t bs)
{
	int bufsize;
	
	bufsize = bs & ~(DIGFILT_BLOCKSIZE_MAX-1);

	return bufsize;
}

static int
digfilt_get_props(void *sc)
{
	return (AUDIO_PROP_PLAYBACK | AUDIO_PROP_INDEPENDENT);
}

static void
digfilt_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct digfilt_softc *sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;

	return;
}

/*
 * IRQ for DAC error.
 */
static int
dac_error_intr(void *arg)
{
	struct digfilt_softc *sc = arg;
	AO_WR(sc, HW_AUDIOOUT_CTRL_CLR, HW_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);
	return 1;
}

/*
 * IRQ from DMA.
 */
static int
dac_dma_intr(void *arg)
{
	struct digfilt_softc *sc = arg;

	unsigned int dma_err;

	mutex_enter(&sc->sc_intr_lock);

	dma_err = apbdma_intr_status(sc->sc_dmac, DIGFILT_DMA_CHANNEL);

	if (dma_err) {
		apbdma_ack_error_intr(sc->sc_dmac, DIGFILT_DMA_CHANNEL);
	}

	sc->sc_intr(sc->sc_intarg);
	apbdma_ack_intr(sc->sc_dmac, DIGFILT_DMA_CHANNEL);

	mutex_exit(&sc->sc_intr_lock);

	/* Return 1 to acknowledge IRQ. */
	return 1;
}

static void *
digfilt_ao_alloc_dmachain(void *priv, size_t size)
{
	struct digfilt_softc *sc = priv;
	int rsegs;
	int error;
	void *kvap;

	kvap = NULL;

	error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &sc->sc_c_ds[0], DIGFILT_DMA_NSEGS, &rsegs, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "bus_dmamem_alloc: %d\n", error);
		goto out;
	}

	error = bus_dmamem_map(sc->sc_dmat, sc->sc_c_ds, DIGFILT_DMA_NSEGS, size, &kvap, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_map: %d\n", error);
		goto dmamem_free;
	}

	/* After load sc_c_dmamp is valid. */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_c_dmamp, kvap, size, NULL, BUS_DMA_NOWAIT|BUS_DMA_WRITE);
	if (error) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load: %d\n", error);
		goto dmamem_unmap;
	}

	memset(kvap, 0x00, size);

	return kvap;

dmamem_unmap:
	bus_dmamem_unmap(sc->sc_dmat, kvap, size);
dmamem_free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_c_ds, DIGFILT_DMA_NSEGS);
out:

	return kvap;
}

static void
digfilt_ao_apply_mutes(struct digfilt_softc *sc)
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
digfilt_ao_init(struct digfilt_softc *sc)
{

	AO_WR(sc, HW_AUDIOOUT_ANACLKCTRL_CLR, HW_AUDIOOUT_ANACLKCTRL_CLKGATE);
	while ((AO_RD(sc, HW_AUDIOOUT_ANACLKCTRL) &
	    HW_AUDIOOUT_ANACLKCTRL_CLKGATE));

	/* Hold headphones outputs at ground. */
	AO_WR(sc, HW_AUDIOOUT_ANACTRL_SET, HW_AUDIOOUT_ANACTRL_HP_HOLD_GND);

	/* Remove pulldown resistors on headphone outputs. */
	rtc_release_gnd(1);

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
digfilt_ao_reset(struct digfilt_softc *sc)
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
digfilt_ao_set_rate(struct digfilt_softc *sc, int sr)
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
		aprint_error_dev(sc->sc_dev, "uknown sample rate: %d\n", sr);
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
#if 0
/*
 * Reset the AUDIOIN block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
digfilt_ai_reset(struct digfilt_softc *sc)
{
	unsigned int loop;

	/* Prepare for soft-reset by making sure that SFTRST is not currently
	* asserted. Also clear CLKGATE so we can wait for its assertion below.
	*/
	AI_WR(sc, HW_AUDIOIN_CTRL_CLR, HW_AUDIOIN_CTRL_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((AI_RD(sc, HW_AUDIOIN_CTRL) & HW_AUDIOIN_CTRL_SFTRST) ||
	    (loop < DIGFILT_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	AI_WR(sc, HW_AUDIOIN_CTRL_CLR, HW_AUDIOIN_CTRL_CLKGATE);

	/* Soft-reset the block. */
	AI_WR(sc, HW_AUDIOIN_CTRL_SET, HW_AUDIOIN_CTRL_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(AI_RD(sc, HW_AUDIOIN_CTRL) & HW_AUDIOIN_CTRL_CLKGATE));

	/* Bring block out of reset. */
	AI_WR(sc, HW_AUDIOIN_CTRL_CLR, HW_AUDIOIN_CTRL_SFTRST);

	loop = 0;
	while ((AI_RD(sc, HW_AUDIOIN_CTRL) & HW_AUDIOIN_CTRL_SFTRST) ||
	    (loop < DIGFILT_SOFT_RST_LOOP))
		loop++;

	AI_WR(sc, HW_AUDIOIN_CTRL_CLR, HW_AUDIOIN_CTRL_CLKGATE);

	/* Wait until clock is in the NON-gated state. */
	while (AI_RD(sc, HW_AUDIOIN_CTRL) & HW_AUDIOIN_CTRL_CLKGATE);

	return;
}
#endif
