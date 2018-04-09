/* $NetBSD: pl041.c,v 1.4 2018/04/09 10:16:46 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: pl041.c,v 1.4 2018/04/09 10:16:46 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/audioio.h>

#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/ic/ac97var.h>
#include <dev/ic/ac97reg.h>

#include <dev/ic/pl041var.h>

#define	AACIRXCR		0x00
#define	AACITXCR		0x04
#define	 AACITXCR_TXFEN		__BIT(16)
#define	 AACITXCR_TXCM		__BIT(15)
#define	 AACITXCR_TSIZE		__BITS(14,13)
#define	  AACITXCR_TSIZE_16	__SHIFTIN(0, AACITXCR_TSIZE)
#define	  AACITXCR_TX(n)	__BIT(n)
#define	 AACITXCR_TXEN		__BIT(0)
#define	AACISR			0x08
#define	 AACISR_TXFF		__BIT(5)
#define	 AACISR_TXHE		__BIT(3)
#define	AACIISR			0x0c
#define	 AACIISR_URINTR		__BIT(5)
#define	 AACIISR_TXINTR		__BIT(2)
#define	AACIIE			0x10
#define	 AACIIE_TXUIE		__BIT(5)
#define	 AACIIE_TXIE		__BIT(2)
#define	AACISL1RX		0x50
#define	AACISL1TX		0x54
#define	AACISL2RX		0x58
#define	AACISL2TX		0x5c
#define	AACISLFR		0x68
#define	AACISLISTAT		0x6c
#define	AACISLIEN		0x70
#define	AACIINTCLR		0x74
#define	AACIMAINCR		0x78
#define	AACIRESET		0x7c
#define	AACISYNC		0x80
#define	AACIALLINTS		0x84
#define	AACIMAINFR		0x88
#define	AACIDR			0x90

#define	AACI_FIFO_DEPTH		512
#define	AACI_BLOCK_ALIGN	4

#define	AACI_READ(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	AACI_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	AACI_WRITE_MULTI(sc, reg, val, cnt)	\
	bus_space_write_multi_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val), (cnt))

static const struct audio_format aaci_format = {
	.mode = AUMODE_PLAY,
	.encoding = AUDIO_ENCODING_SLINEAR_LE,
	.validbits = 16,
	.precision = 16,
	.channels = 2,
	.channel_mask = AUFMT_STEREO,
	.frequency_type = 0,
	.frequency = { 48000, 48000 }
};

static void
aaci_write_data(struct aaci_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_pint == NULL)
		return;

	while ((AACI_READ(sc, AACISR) & AACISR_TXHE) != 0) {
		const int len = min(AACI_FIFO_DEPTH / 2, min(sc->sc_pblkresid,
		    (uintptr_t)sc->sc_pend - (uintptr_t)sc->sc_pcur));
		KASSERT((len & 3) == 0);
		AACI_WRITE_MULTI(sc, AACIDR, sc->sc_pcur, len >> 2);
		sc->sc_pcur += (len >> 2);
		if (sc->sc_pcur == sc->sc_pend)
			sc->sc_pcur = sc->sc_pstart;
		sc->sc_pblkresid -= len;
		if (sc->sc_pblkresid == 0) {
			sc->sc_pblkresid = sc->sc_pblksize;
			sc->sc_pint(sc->sc_pintarg);
		}
	}
}

static int
aaci_query_encoding(void *priv, struct audio_encoding *enc)
{
	struct aaci_softc * const sc = priv;

	return auconv_query_encoding(sc->sc_encodings, enc);
}

static int
aaci_set_params(void *priv, int setmode, int usermode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	int index;

	if (play && (setmode & AUMODE_PLAY) != 0) {
		index = auconv_set_converter(&aaci_format, 1, AUMODE_PLAY,
		    play, true, pfil);
		if (index < 0)
			return EINVAL;
	}

	return 0;
}

static int
aaci_getdev(void *priv, struct audio_device *audiodev)
{
	snprintf(audiodev->name, sizeof(audiodev->name), "ARM");
	snprintf(audiodev->version, sizeof(audiodev->version),
	    "PrimeCell AACI");
	snprintf(audiodev->config, sizeof(audiodev->config), "aaci");
	return 0;
}

static int
aaci_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct aaci_softc * const sc = priv;

	return sc->sc_ac97_codec->vtbl->mixer_set_port(sc->sc_ac97_codec, mc);
}

static int
aaci_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct aaci_softc * const sc = priv;

	return sc->sc_ac97_codec->vtbl->mixer_get_port(sc->sc_ac97_codec, mc);
}

static int
aaci_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct aaci_softc * const sc = priv;

	return sc->sc_ac97_codec->vtbl->query_devinfo(sc->sc_ac97_codec, di);
}

static int
aaci_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK;
}

static int
aaci_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct aaci_softc * const sc = priv;

	sc->sc_pcur = start;
	sc->sc_pstart = start;
	sc->sc_pend = end;
	sc->sc_pblksize = blksize;
	sc->sc_pblkresid = blksize;
	sc->sc_pint = intr;
	sc->sc_pintarg = intrarg;

	AACI_WRITE(sc, AACIIE, AACIIE_TXIE | AACIIE_TXUIE);
	AACI_WRITE(sc, AACITXCR, AACITXCR_TXFEN | AACITXCR_TXEN |
	    AACITXCR_TXCM | AACITXCR_TSIZE_16 |
	    AACITXCR_TX(AC97_SLOT_PCM_L) | AACITXCR_TX(AC97_SLOT_PCM_R));

	return 0;
}

static int
aaci_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	return ENXIO;
}

static int
aaci_halt_output(void *priv)
{
	struct aaci_softc * const sc = priv;

	sc->sc_pint = NULL;

	AACI_WRITE(sc, AACITXCR, 0);
	AACI_WRITE(sc, AACIIE, 0);

	return 0;
}

static int
aaci_halt_input(void *priv)
{
	return ENXIO;
}

static int
aaci_round_blocksize(void *priv, int bs, int mode, const audio_params_t *params)
{
	return roundup(bs, AACI_BLOCK_ALIGN);
}

static void
aaci_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct aaci_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static const struct audio_hw_if aaci_hw_if = {
	.query_encoding = aaci_query_encoding,
	.set_params = aaci_set_params,
	.getdev = aaci_getdev,
	.set_port = aaci_set_port,
	.get_port = aaci_get_port,
	.query_devinfo = aaci_query_devinfo,
	.get_props = aaci_get_props,
	.trigger_output = aaci_trigger_output,
	.trigger_input = aaci_trigger_input,
	.halt_output = aaci_halt_output,
	.halt_input = aaci_halt_input,
	.round_blocksize = aaci_round_blocksize,
	.get_locks = aaci_get_locks,
};

static int
aaci_ac97_attach(void *priv, struct ac97_codec_if *codec)
{
	struct aaci_softc * const sc = priv;

	sc->sc_ac97_codec = codec;

	return 0;
}

static int
aaci_ac97_read(void *priv, uint8_t reg, uint16_t *val)
{
	struct aaci_softc * const sc = priv;

	AACI_WRITE(sc, AACISL1TX, (reg << 12) | (1 << 19));

	if (AACI_READ(sc, AACISL1RX) != (reg << 12))
		return 1;

	*val = AACI_READ(sc, AACISL2RX) >> 4;
	return 0;
}

static int
aaci_ac97_write(void *priv, uint8_t reg, uint16_t val)
{
	struct aaci_softc * const sc = priv;

	AACI_WRITE(sc, AACISL2TX, val << 4);
	AACI_WRITE(sc, AACISL1TX, (reg << 12) | (0 << 19));

	return 0;
}

void
aaci_attach(struct aaci_softc *sc)
{
	int error;

	aprint_naive("\n");
	aprint_normal(": Advanced Audio CODEC\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	sc->sc_ac97_host.arg = sc;
	sc->sc_ac97_host.attach = aaci_ac97_attach;
	sc->sc_ac97_host.read = aaci_ac97_read;
	sc->sc_ac97_host.write = aaci_ac97_write;
	error = ac97_attach(&sc->sc_ac97_host, sc->sc_dev, &sc->sc_lock);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't attach codec (%d)\n",
		    error);
		return;
	}

	error = auconv_create_encodings(&aaci_format, 1, &sc->sc_encodings);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't create encodings (%d)\n",
		    error);
		return;
	}

	sc->sc_audiodev = audio_attach_mi(&aaci_hw_if, sc, sc->sc_dev);
}

int
aaci_intr(void *priv)
{
	struct aaci_softc * const sc = priv;
	uint32_t isr;

	if (sc->sc_audiodev == NULL)
		return 0;

	isr = AACI_READ(sc, AACIISR);

	if (isr & AACIISR_URINTR)
		AACI_WRITE(sc, AACIINTCLR, AACIISR_URINTR);

	if (isr & AACIISR_TXINTR) {
		mutex_enter(&sc->sc_intr_lock);
		if (sc->sc_pint == NULL)
			AACI_WRITE(sc, AACIIE, 0);
		aaci_write_data(sc);
		mutex_exit(&sc->sc_intr_lock);
	}

	return 1;
}
