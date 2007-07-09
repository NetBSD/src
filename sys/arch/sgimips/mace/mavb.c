/* $NetBSD: mavb.c,v 1.3 2007/07/09 20:52:27 ad Exp $ */
/* $OpenBSD: mavb.c,v 1.6 2005/04/15 13:05:14 mickey Exp $ */

/*
 * Copyright (c) 2005 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/callout.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <sys/audioio.h>
#include <dev/auconv.h>
#include <dev/audio_if.h>

#include <arch/sgimips/mace/macevar.h>
#include <arch/sgimips/mace/macereg.h>
#include <arch/sgimips/mace/mavbreg.h>

#include <dev/ic/ad1843reg.h>

#undef MAVB_DEBUG

#ifdef MAVB_DEBUG
#define DPRINTF(l,x)	do { if (mavb_debug & (l)) printf x; } while (0)
#define MAVB_DEBUG_INTR		0x0100
int mavb_debug = ~MAVB_DEBUG_INTR;
#else
#define DPRINTF(l,x)	/* nothing */
#endif

/* Repeat delays for volume buttons.  */
#define MAVB_VOLUME_BUTTON_REPEAT_DEL1	400	/* 400ms to start repeating */
#define MAVB_VOLUME_BUTTON_REPEAT_DELN  100	/* 100ms between repeats */

/* XXX We need access to some of the MACE ISA registers.  */
#define MAVB_ISA_NREGS				0x20

/*
 * AD1843 Mixer.
 */

enum {
	AD1843_RECORD_CLASS,
	AD1843_ADC_SOURCE,	/* ADC Source Select */
	AD1843_ADC_GAIN,	/* ADC Input Gain */

	AD1843_INPUT_CLASS,
	AD1843_DAC1_GAIN,	/* DAC1 Analog/Digital Gain/Attenuation */
	AD1843_DAC1_MUTE,	/* DAC1 Analog Mute */
	AD1843_DAC2_GAIN,	/* DAC2 Mix Gain */
	AD1843_AUX1_GAIN,	/* Auxilliary 1 Mix Gain */
	AD1843_AUX2_GAIN,	/* Auxilliary 2 Mix Gain */
	AD1843_AUX3_GAIN,	/* Auxilliary 3 Mix Gain */
	AD1843_MIC_GAIN,	/* Microphone Mix Gain */
	AD1843_MONO_GAIN,	/* Mono Mix Gain */
	AD1843_DAC2_MUTE,	/* DAC2 Mix Mute */
	AD1843_AUX1_MUTE,	/* Auxilliary 1 Mix Mute */
	AD1843_AUX2_MUTE,	/* Auxilliary 2 Mix Mute */
	AD1843_AUX3_MUTE,	/* Auxilliary 3 Mix Mute */
	AD1843_MIC_MUTE,	/* Microphone Mix Mute */
	AD1843_MONO_MUTE,	/* Mono Mix Mute */
	AD1843_SUM_MUTE,	/* Sum Mute */

	AD1843_OUTPUT_CLASS,
	AD1843_MNO_MUTE,	/* Mono Output Mute */
	AD1843_HPO_MUTE		/* Headphone Output Mute */
};

/* ADC Source Select.  The order matches the hardware bits.  */
const char *ad1843_source[] = {
	AudioNline,
	AudioNmicrophone,
	AudioNaux "1",
	AudioNaux "2",
	AudioNaux "3",
	AudioNmono,
	AudioNdac "1",
	AudioNdac "2"
};

/* Mix Control.  The order matches the hardware register numbering.  */
const char *ad1843_input[] = {
	AudioNdac "2",		/* AD1843_DAC2__TO_MIXER */
	AudioNaux "1",
	AudioNaux "2",
	AudioNaux "3",
	AudioNmicrophone,
	AudioNmono		/* AD1843_MISC_SETTINGS */
};

struct mavb_softc {
	struct device sc_dev;
	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamap;

	/* XXX We need access to some of the MACE ISA registers.  */
	bus_space_handle_t sc_isash;

#define MAVB_ISA_RING_SIZE		0x1000
	uint8_t *sc_ring;

	uint8_t *sc_start, *sc_end;
	int sc_blksize;
	void (*sc_intr)(void *);
	void *sc_intrarg;

	void *sc_get;
	int sc_count;

	u_long sc_play_rate;
	u_int sc_play_format;

	struct callout sc_volume_button_ch;
};

/* XXX mavb supports way more than this, but for now I'm going to be
 *     lazy and let auconv work its magic
 */
#define MAVB_NENCODINGS 8
static audio_encoding_t mavb_encoding[MAVB_NENCODINGS] = {
	{ 0, AudioEulinear, AUDIO_ENCODING_ULINEAR, 8,
	  AUDIO_ENCODINGFLAG_EMULATED },
	{ 1, AudioEmulaw, AUDIO_ENCODING_ULAW, 8,
	  AUDIO_ENCODINGFLAG_EMULATED },
	{ 2, AudioEalaw, AUDIO_ENCODING_ALAW, 8,
	  AUDIO_ENCODINGFLAG_EMULATED },
	{ 3, AudioEslinear, AUDIO_ENCODING_SLINEAR, 8,
	  AUDIO_ENCODINGFLAG_EMULATED },
	{ 4, AudioEslinear_le, AUDIO_ENCODING_SLINEAR, 16,
	  AUDIO_ENCODINGFLAG_EMULATED },
	{ 5, AudioEulinear_le, AUDIO_ENCODING_ULINEAR, 16,
	  AUDIO_ENCODINGFLAG_EMULATED },
	{ 6, AudioEslinear_be, AUDIO_ENCODING_SLINEAR, 16,
	  0 },
	{ 7, AudioEulinear_be, AUDIO_ENCODING_ULINEAR, 16,
	  AUDIO_ENCODINGFLAG_EMULATED },
};

#define MAVB_NFORMATS 3
static const struct audio_format mavb_formats[MAVB_NFORMATS] = {
	{ NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	  2, AUFMT_STEREO, 0, { 8000, 48000 } },
	{ NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	  1, AUFMT_MONAURAL, 0, { 8000, 48000 } },
};

struct mavb_codecvar {
	stream_filter_t base;
};

static stream_filter_t *mavb_factory
    (int (*)(stream_fetcher_t *, audio_stream_t *, int));
static void mavb_dtor(stream_filter_t *);

/* XXX I'm going to complain every time I have to copy this macro */
#define DEFINE_FILTER(name)	\
static int \
name##_fetch_to(stream_fetcher_t *, audio_stream_t *, int); \
stream_filter_t *name(struct audio_softc *, \
    const audio_params_t *, const audio_params_t *); \
stream_filter_t * \
name(struct audio_softc *sc, const audio_params_t *from, \
    const audio_params_t *to) \
{ \
	return mavb_factory(name##_fetch_to); \
} \
static int \
name##_fetch_to(stream_fetcher_t *self, audio_stream_t *dst, int max_used)

DEFINE_FILTER(mavb_16to24)
{
	stream_filter_t *this;
	int m, err;

	this = (stream_filter_t *)self;
	max_used = (max_used + 1) & ~1;
	if ((err = this->prev->fetch_to(this->prev, this->src, max_used)))
		return err;
	m = (dst->end - dst->start) & ~1;
	m = min(m, max_used);
	FILTER_LOOP_PROLOGUE(this->src, 2, dst, 4, m) {
		d[3] = 0;
		d[2] = s[1];
		d[1] = s[0];
		d[0] = (s[0] & 0x80) ? 0xff : 0;
	} FILTER_LOOP_EPILOGUE(this->src, dst);

	return 0;
}

DEFINE_FILTER(mavb_mts)
{
	stream_filter_t *this;
	int m, err;

	this = (stream_filter_t *)self;
	max_used = (max_used + 1) & ~1;
	if ((err = this->prev->fetch_to(this->prev, this->src, max_used)))
		return err;
	m = (dst->end - dst->start) & ~1;
	m = min(m, max_used);
	FILTER_LOOP_PROLOGUE(this->src, 4, dst, 8, m) {
		d[3] = d[7] = s[3];
		d[2] = d[6] = s[2];
		d[1] = d[5] = s[1];
		d[0] = d[4] = s[0];
	} FILTER_LOOP_EPILOGUE(this->src, dst);

	return 0;
}

static stream_filter_t *
mavb_factory(int (*fetch_to)(stream_fetcher_t *, audio_stream_t *, int))
{
	struct mavb_codecvar *this;

	this = malloc(sizeof(*this), M_DEVBUF, M_WAITOK | M_ZERO);
	this->base.base.fetch_to = fetch_to;
	this->base.dtor = mavb_dtor;
	this->base.set_fetcher = stream_filter_set_fetcher;
	this->base.set_inputbuffer = stream_filter_set_inputbuffer;

	return &this->base;
}

static void
mavb_dtor(stream_filter_t *this)
{
	if (this != NULL)
		free(this, M_DEVBUF);
}

typedef u_int64_t ad1843_addr_t;

u_int16_t ad1843_reg_read(struct mavb_softc *, ad1843_addr_t);
u_int16_t ad1843_reg_write(struct mavb_softc *, ad1843_addr_t, u_int16_t);
void ad1843_dump_regs(struct mavb_softc *);

int mavb_match(struct device *, struct cfdata *, void *);
void mavb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mavb, sizeof(struct mavb_softc),
    mavb_match, mavb_attach, NULL, NULL);
    
int mavb_open(void *, int);
void mavb_close(void *);
int mavb_query_encoding(void *, struct audio_encoding *);
int mavb_set_params(void *, int, int, struct audio_params *,
		    struct audio_params *, stream_filter_list_t *,
		    stream_filter_list_t *);
int mavb_round_blocksize(void *hdl, int, int, const audio_params_t *);
int mavb_halt_output(void *);
int mavb_halt_input(void *);
int mavb_getdev(void *, struct audio_device *);
int mavb_set_port(void *, struct mixer_ctrl *);
int mavb_get_port(void *, struct mixer_ctrl *);
int mavb_query_devinfo(void *, struct mixer_devinfo *);
size_t mavb_round_buffersize(void *, int, size_t);
int mavb_get_props(void *);
int mavb_trigger_output(void *, void *, void *, int, void (*)(void *),
			void *, const audio_params_t *);
int mavb_trigger_input(void *, void *, void *, int, void (*)(void *),
		       void *, const audio_params_t *);

struct audio_hw_if mavb_sa_hw_if = {
	mavb_open,
	mavb_close,
	0,
	mavb_query_encoding,
	mavb_set_params,
	mavb_round_blocksize,
	0,
	0,
	0,
	0,
	0,
	mavb_halt_output,
	mavb_halt_input,
	0,
	mavb_getdev,
	0,
	mavb_set_port,
	mavb_get_port,
	mavb_query_devinfo,
	0,
	0,
	mavb_round_buffersize,
	0,
	mavb_get_props,
	mavb_trigger_output,
	mavb_trigger_input,
	NULL,
};

struct audio_device mavb_device = {
	"A3",
	"",
	"mavb"
};

int
mavb_open(void *hdl, int flags)
{
	return (0);
}

void
mavb_close(void *hdl)
{
}

int
mavb_query_encoding(void *hdl, struct audio_encoding *ae)
{
	if (ae->index < 0 || ae->index >= MAVB_NENCODINGS)
		return (EINVAL);
	*ae = mavb_encoding[ae->index];

	return (0);
}

static int
mavb_set_play_rate(struct mavb_softc *sc, u_long sample_rate)
{
	if (sample_rate < 4000 || sample_rate > 48000)
		return (EINVAL);

	if (sc->sc_play_rate != sample_rate) {
		ad1843_reg_write(sc, AD1843_CLOCK2_SAMPLE_RATE, sample_rate);
		sc->sc_play_rate = sample_rate;
	}
	return (0);
}

static int
mavb_set_play_format(struct mavb_softc *sc, u_int encoding)
{
	u_int16_t value;
	u_int format;

	switch(encoding) {
	case AUDIO_ENCODING_ULINEAR_BE:
		format = AD1843_PCM8;
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
		format = AD1843_PCM16;
		break;
	case AUDIO_ENCODING_ULAW:
		format = AD1843_ULAW;
		break;
	case AUDIO_ENCODING_ALAW:
		format = AD1843_ALAW;
		break;
	default:
		return (EINVAL);
	}

	if (sc->sc_play_format != format) {
		value = ad1843_reg_read(sc, AD1843_SERIAL_INTERFACE);
		value &= ~AD1843_DA1F_MASK;
		value |= (format << AD1843_DA1F_SHIFT);
		ad1843_reg_write(sc, AD1843_SERIAL_INTERFACE, value);
		sc->sc_play_format = format;
	}
	return (0);
}

int
mavb_set_params(void *hdl, int setmode, int usemode,
    struct audio_params *play, struct audio_params *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct mavb_softc *sc = (struct mavb_softc *)hdl;
	struct audio_params *p;
	stream_filter_list_t *fil;
	int error;

	DPRINTF(1, ("%s: mavb_set_params: sample=%u precision=%d "
	    "channels=%d\n", sc->sc_dev.dv_xname, play->sample_rate,
	    play->precision, play->channels));

	if (setmode & AUMODE_PLAY) {
		if (play->sample_rate < 4000 || play->sample_rate > 48000)
			return (EINVAL);

		p = play;
		fil = pfil;
		if (auconv_set_converter(mavb_formats, MAVB_NFORMATS,
		    AUMODE_PLAY, p, FALSE, fil) < 0)
			return (EINVAL);

		fil->append(fil, mavb_16to24, p);
		if (p->channels == 1)
			fil->append(fil, mavb_mts, p);
		if (fil->req_size > 0)
			p = &fil->filters[0].param;
		
		error = mavb_set_play_rate(sc, p->sample_rate);
		if (error)
			return (error);

		error = mavb_set_play_format(sc, p->encoding);
		if (error)
			return (error);
	}

#if 0
	if (setmode & AUMODE_RECORD) {
		if (rec->sample_rate < 4000 || rec->sample_rate > 48000)
			return (EINVAL);
	}
#endif

	return (0);
}

int
mavb_round_blocksize(void *hdl, int bs, int mode, const audio_params_t *p)
{
	/* Block size should be a multiple of 32.  */
	return (bs + 0x1f) & ~0x1f;
}

int
mavb_halt_output(void *hdl)
{
	struct mavb_softc *sc = (struct mavb_softc *)hdl;

	DPRINTF(1, ("%s: mavb_halt_output called\n", sc->sc_dev.dv_xname));

	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CHANNEL2_CONTROL, 0);
	return (0);
}

int
mavb_halt_input(void *hdl)
{
	return (0);
}

int
mavb_getdev(void *hdl, struct audio_device *ret)
{
	*ret = mavb_device;
	return (0);
}

int
mavb_set_port(void *hdl, struct mixer_ctrl *mc)
{
	struct mavb_softc *sc = (struct mavb_softc *)hdl;
	u_char left, right;
	ad1843_addr_t reg;
	u_int16_t value;

	DPRINTF(1, ("%s: mavb_set_port: dev=%d\n", sc->sc_dev.dv_xname,
	    mc->dev));

	switch (mc->dev) {
	case AD1843_ADC_SOURCE:
		value = ad1843_reg_read(sc, AD1843_ADC_SOURCE_GAIN);
		value &= ~(AD1843_LSS_MASK | AD1843_RSS_MASK);
		value |= ((mc->un.ord << AD1843_LSS_SHIFT) & AD1843_LSS_MASK);
		value |= ((mc->un.ord << AD1843_RSS_SHIFT) & AD1843_RSS_MASK);
		ad1843_reg_write(sc, AD1843_ADC_SOURCE_GAIN, value);
		break;
	case AD1843_ADC_GAIN:
		left = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		right = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		value = ad1843_reg_read(sc, AD1843_ADC_SOURCE_GAIN);
		value &= ~(AD1843_LIG_MASK | AD1843_RIG_MASK);
		value |= ((left >> 4) << AD1843_LIG_SHIFT);
		value |= ((right >> 4) << AD1843_RIG_SHIFT);
		ad1843_reg_write(sc, AD1843_ADC_SOURCE_GAIN, value);
		break;

	case AD1843_DAC1_GAIN:
		left = AUDIO_MAX_GAIN -
		    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		right = AUDIO_MAX_GAIN -
                    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		value = ad1843_reg_read(sc, AD1843_DAC1_ANALOG_GAIN);
		value &= ~(AD1843_LDA1G_MASK | AD1843_RDA1G_MASK);
		value |= ((left >> 2) << AD1843_LDA1G_SHIFT);
		value |= ((right >> 2) << AD1843_RDA1G_SHIFT);
		ad1843_reg_write(sc, AD1843_DAC1_ANALOG_GAIN, value);
		break;
	case AD1843_DAC1_MUTE:
		value = ad1843_reg_read(sc, AD1843_DAC1_ANALOG_GAIN);
		if (mc->un.ord == 0)
			value &= ~(AD1843_LDA1GM | AD1843_RDA1GM);
		else
			value |= (AD1843_LDA1GM | AD1843_RDA1GM);
		ad1843_reg_write(sc, AD1843_DAC1_ANALOG_GAIN, value);
		break;

	case AD1843_DAC2_GAIN:
	case AD1843_AUX1_GAIN:
	case AD1843_AUX2_GAIN:
	case AD1843_AUX3_GAIN:
	case AD1843_MIC_GAIN:
		left = AUDIO_MAX_GAIN -
		    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		right = AUDIO_MAX_GAIN -
                    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		reg = AD1843_DAC2_TO_MIXER + mc->dev - AD1843_DAC2_GAIN;
		value = ad1843_reg_read(sc, reg);
		value &= ~(AD1843_LD2M_MASK | AD1843_RD2M_MASK);
		value |= ((left >> 3) << AD1843_LD2M_SHIFT);
		value |= ((right >> 3) << AD1843_RD2M_SHIFT);
		ad1843_reg_write(sc, reg, value);
		break;
	case AD1843_MONO_GAIN:
		left = AUDIO_MAX_GAIN -
		    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		value &= ~AD1843_MNM_MASK;
		value |= ((left >> 3) << AD1843_MNM_SHIFT);
		ad1843_reg_write(sc, AD1843_MISC_SETTINGS, value);
		break;
	case AD1843_DAC2_MUTE:
	case AD1843_AUX1_MUTE:
	case AD1843_AUX2_MUTE:
	case AD1843_AUX3_MUTE:
	case AD1843_MIC_MUTE:
	case AD1843_MONO_MUTE:	/* matches left channel */
		reg = AD1843_DAC2_TO_MIXER + mc->dev - AD1843_DAC2_MUTE;
		value = ad1843_reg_read(sc, reg);
		if (mc->un.ord == 0)
			value &= ~(AD1843_LD2MM | AD1843_RD2MM);
		else
			value |= (AD1843_LD2MM | AD1843_RD2MM);
		ad1843_reg_write(sc, reg, value);
		break;

	case AD1843_SUM_MUTE:
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		if (mc->un.ord == 0)
			value &= ~AD1843_SUMM;
		else
			value |= AD1843_SUMM;
		ad1843_reg_write(sc, AD1843_MISC_SETTINGS, value);
		break;
		
	case AD1843_MNO_MUTE:
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		if (mc->un.ord == 0)
			value &= ~AD1843_MNOM;
		else
			value |= AD1843_MNOM;
		ad1843_reg_write(sc, AD1843_MISC_SETTINGS, value);
		break;
		
	case AD1843_HPO_MUTE:
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		if (mc->un.ord == 0)
			value &= ~AD1843_HPOM;
		else
			value |= AD1843_HPOM;
		ad1843_reg_write(sc, AD1843_MISC_SETTINGS, value);
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

int
mavb_get_port(void *hdl, struct mixer_ctrl *mc)
{
	struct mavb_softc *sc = (struct mavb_softc *)hdl;
	u_char left, right;
	ad1843_addr_t reg;
	u_int16_t value;

	DPRINTF(1, ("%s: mavb_get_port: dev=%d\n", sc->sc_dev.dv_xname,
	    mc->dev));

	switch (mc->dev) {
	case AD1843_ADC_SOURCE:
		value = ad1843_reg_read(sc, AD1843_ADC_SOURCE_GAIN);
		mc->un.ord = (value & AD1843_LSS_MASK) >> AD1843_LSS_SHIFT;
		break;
	case AD1843_ADC_GAIN:
		value = ad1843_reg_read(sc, AD1843_ADC_SOURCE_GAIN);
		left = (value & AD1843_LIG_MASK) >> AD1843_LIG_SHIFT;
		right = (value & AD1843_RIG_MASK) >> AD1843_RIG_SHIFT;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    (left << 4) | left;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    (right << 2) | right;
		break;

	case AD1843_DAC1_GAIN:
		value = ad1843_reg_read(sc, AD1843_DAC1_ANALOG_GAIN);
		left = (value & AD1843_LDA1G_MASK) >> AD1843_LDA1G_SHIFT;
		right = (value & AD1843_RDA1G_MASK) >> AD1843_RDA1G_SHIFT;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    AUDIO_MAX_GAIN - (left << 2);
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    AUDIO_MAX_GAIN - (right << 2);
		break;
	case AD1843_DAC1_MUTE:
		value = ad1843_reg_read(sc, AD1843_DAC1_ANALOG_GAIN);
		mc->un.ord = (value & AD1843_LDA1GM) ? 1 : 0;
		break;

	case AD1843_DAC2_GAIN:
	case AD1843_AUX1_GAIN:
	case AD1843_AUX2_GAIN:
	case AD1843_AUX3_GAIN:
	case AD1843_MIC_GAIN:
		reg = AD1843_DAC2_TO_MIXER + mc->dev - AD1843_DAC2_GAIN;
		value = ad1843_reg_read(sc, reg);
		left = (value & AD1843_LD2M_MASK) >> AD1843_LD2M_SHIFT;
		right = (value & AD1843_RD2M_MASK) >> AD1843_RD2M_SHIFT;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    AUDIO_MAX_GAIN - (left << 3);
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    AUDIO_MAX_GAIN - (right << 3);
		break;
	case AD1843_MONO_GAIN:
		if (mc->un.value.num_channels != 1)
			return (EINVAL);

		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		left = (value & AD1843_MNM_MASK) >> AD1843_MNM_SHIFT;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
		    AUDIO_MAX_GAIN - (left << 3);
		break;
	case AD1843_DAC2_MUTE:
	case AD1843_AUX1_MUTE:
	case AD1843_AUX2_MUTE:
	case AD1843_AUX3_MUTE:
	case AD1843_MIC_MUTE:
	case AD1843_MONO_MUTE:	/* matches left channel */
		reg = AD1843_DAC2_TO_MIXER + mc->dev - AD1843_DAC2_MUTE;
		value = ad1843_reg_read(sc, reg);
		mc->un.ord = (value & AD1843_LD2MM) ? 1 : 0;
		break;

	case AD1843_SUM_MUTE:
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		mc->un.ord = (value & AD1843_SUMM) ? 1 : 0;
		break;
		
	case AD1843_MNO_MUTE:
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		mc->un.ord = (value & AD1843_MNOM) ? 1 : 0;
		break;
		
	case AD1843_HPO_MUTE:
		value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
		mc->un.ord = (value & AD1843_HPOM) ? 1 : 0;
		break;
		
	default:
		return (EINVAL);
	}

	return (0);
}

int
mavb_query_devinfo(void *hdl, struct mixer_devinfo *di)
{
	int i;

	di->prev = di->next = AUDIO_MIXER_LAST;

	switch (di->index) {
	case AD1843_RECORD_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = AD1843_RECORD_CLASS;
		strlcpy(di->label.name, AudioCrecord, sizeof di->label.name);
		break;

	case AD1843_ADC_SOURCE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = AD1843_RECORD_CLASS;
		di->next = AD1843_ADC_GAIN;
		strlcpy(di->label.name, AudioNsource, sizeof di->label.name);
		di->un.e.num_mem =
			sizeof ad1843_source / sizeof ad1843_source[1];
		for (i = 0; i < di->un.e.num_mem; i++) {
			strlcpy(di->un.e.member[i].label.name,
                            ad1843_source[i],
			    sizeof di->un.e.member[0].label.name);
			di->un.e.member[i].ord = i;
		}
		break;
	case AD1843_ADC_GAIN:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = AD1843_RECORD_CLASS;
		di->prev = AD1843_ADC_SOURCE;
		strlcpy(di->label.name, AudioNvolume, sizeof di->label.name);
		di->un.v.num_channels = 2;
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof di->un.v.units.name);
		break;

	case AD1843_INPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = AD1843_INPUT_CLASS;
		strlcpy(di->label.name, AudioCinputs, sizeof di->label.name);
		break;

	case AD1843_DAC1_GAIN:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = AD1843_INPUT_CLASS;
		di->next = AD1843_DAC1_MUTE;
		strlcpy(di->label.name, AudioNdac "1", sizeof di->label.name);
		di->un.v.num_channels = 2;
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof di->un.v.units.name);
		break;
	case AD1843_DAC1_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = AD1843_INPUT_CLASS;
		di->prev = AD1843_DAC1_GAIN;
		strlcpy(di->label.name, AudioNmute, sizeof di->label.name);
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNoff,
		    sizeof di->un.e.member[0].label.name);
		di->un.e.member[0].ord = 0;
		strlcpy(di->un.e.member[1].label.name, AudioNon,
		    sizeof di->un.e.member[1].label.name);
		di->un.e.member[1].ord = 1;
		break;

	case AD1843_DAC2_GAIN:
	case AD1843_AUX1_GAIN:
	case AD1843_AUX2_GAIN:
	case AD1843_AUX3_GAIN:
	case AD1843_MIC_GAIN:
	case AD1843_MONO_GAIN:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = AD1843_INPUT_CLASS;
		di->next = di->index + AD1843_DAC2_MUTE - AD1843_DAC2_GAIN;
		strlcpy(di->label.name,
                    ad1843_input[di->index - AD1843_DAC2_GAIN],
		    sizeof di->label.name);
		if (di->index == AD1843_MONO_GAIN)
			di->un.v.num_channels = 1;
		else
			di->un.v.num_channels = 2;
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof di->un.v.units.name);
		break;
	case AD1843_DAC2_MUTE:
	case AD1843_AUX1_MUTE:
	case AD1843_AUX2_MUTE:
	case AD1843_AUX3_MUTE:
	case AD1843_MIC_MUTE:
	case AD1843_MONO_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = AD1843_INPUT_CLASS;
		di->prev = di->index + AD1843_DAC2_GAIN - AD1843_DAC2_MUTE;
		strlcpy(di->label.name, AudioNmute, sizeof di->label.name);
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNoff,
		    sizeof di->un.e.member[0].label.name);
		di->un.e.member[0].ord = 0;
		strlcpy(di->un.e.member[1].label.name, AudioNon,
		    sizeof di->un.e.member[1].label.name);
		di->un.e.member[1].ord = 1;
		break;

	case AD1843_SUM_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = AD1843_INPUT_CLASS;
		strlcpy(di->label.name, "sum." AudioNmute,
		    sizeof di->label.name);
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNoff,
		    sizeof di->un.e.member[0].label.name);
		di->un.e.member[0].ord = 0;
		strlcpy(di->un.e.member[1].label.name, AudioNon,
		    sizeof di->un.e.member[1].label.name);
		di->un.e.member[1].ord = 1;
		break;

	case AD1843_OUTPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = AD1843_OUTPUT_CLASS;
		strlcpy(di->label.name, AudioCoutputs, sizeof di->label.name);
		break;

	case AD1843_MNO_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = AD1843_OUTPUT_CLASS;
		strlcpy(di->label.name, AudioNmono "." AudioNmute,
		    sizeof di->label.name);
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNoff,
		    sizeof di->un.e.member[0].label.name);
		di->un.e.member[0].ord = 0;
		strlcpy(di->un.e.member[1].label.name, AudioNon,
		    sizeof di->un.e.member[1].label.name);
		di->un.e.member[1].ord = 1;
		break;

	case AD1843_HPO_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = AD1843_OUTPUT_CLASS;
		strlcpy(di->label.name, AudioNheadphone "." AudioNmute,
		    sizeof di->label.name);
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNoff,
		    sizeof di->un.e.member[0].label.name);
		di->un.e.member[0].ord = 0;
		strlcpy(di->un.e.member[1].label.name, AudioNon,
		    sizeof di->un.e.member[1].label.name);
		di->un.e.member[1].ord = 1;
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

size_t
mavb_round_buffersize(void *hdl, int dir, size_t bufsize)
{

	return bufsize;
}

int
mavb_get_props(void *hdl)
{
	return (AUDIO_PROP_FULLDUPLEX | AUDIO_PROP_INDEPENDENT);
}

static void
mavb_dma_output(struct mavb_softc *sc)
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	u_int64_t write_ptr;
	u_int64_t depth;
	uint8_t *src, *dst;
	int count;

	write_ptr = bus_space_read_8(st, sh, MAVB_CHANNEL2_WRITE_PTR);
	depth = bus_space_read_8(st, sh, MAVB_CHANNEL2_DEPTH);

	dst = sc->sc_ring + write_ptr;
	src = sc->sc_get;

	count = (MAVB_ISA_RING_SIZE - depth - 32);
	while (--count >= 0) {
		*dst++ = *src++;
		if (dst >= sc->sc_ring + MAVB_ISA_RING_SIZE)
			dst = sc->sc_ring;
		if (src >= sc->sc_end)
			src = sc->sc_start;
		if (++sc->sc_count >= sc->sc_blksize) {
			if (sc->sc_intr)
				sc->sc_intr(sc->sc_intrarg);
			sc->sc_count = 0;
		}
	}

	write_ptr = dst - sc->sc_ring;
	bus_space_write_8(st, sh, MAVB_CHANNEL2_WRITE_PTR, write_ptr);
	sc->sc_get = src;
}

int
mavb_trigger_output(void *hdl, void *start, void *end, int blksize,
		    void (*intr)(void *), void *intrarg,
		    const audio_params_t *param)
{
	struct mavb_softc *sc = (struct mavb_softc *)hdl;

	DPRINTF(1, ("%s: mavb_trigger_output: start=%p end=%p "
	    "blksize=%d intr=%p(%p)\n", sc->sc_dev.dv_xname,
	    start, end, blksize, intr, intrarg));

	sc->sc_blksize = blksize;
	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;

	sc->sc_start = sc->sc_get = start;
	sc->sc_end = end;

	sc->sc_count = 0;

	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CHANNEL2_CONTROL,
	    MAVB_CHANNEL_RESET);
	delay(1000);
	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CHANNEL2_CONTROL, 0);

	mavb_dma_output(sc);

	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CHANNEL2_CONTROL,
	    MAVB_CHANNEL_DMA_ENABLE | MAVB_CHANNEL_INT_50);
	return (0);
}

int
mavb_trigger_input(void *hdl, void *start, void *end, int blksize,
		   void (*intr)(void *), void *intrarg,
		   const audio_params_t *param)
{
	return (0);
}

static void
mavb_button_repeat(void *hdl)
{
	struct mavb_softc *sc = (struct mavb_softc *)hdl;
	u_int64_t intmask, control;
	u_int16_t value, left, right;

	DPRINTF(1, ("%s: mavb_repeat called\n", sc->sc_dev.dv_xname));

#define  MAVB_CONTROL_VOLUME_BUTTONS \
    (MAVB_CONTROL_VOLUME_BUTTON_UP | MAVB_CONTROL_VOLUME_BUTTON_DOWN)

	control = bus_space_read_8(sc->sc_st, sc->sc_sh, MAVB_CONTROL);
	if (control & MAVB_CONTROL_VOLUME_BUTTONS) {
		value = ad1843_reg_read(sc, AD1843_DAC1_ANALOG_GAIN);
		left = (value & AD1843_LDA1G_MASK) >> AD1843_LDA1G_SHIFT;
		right = (value & AD1843_RDA1G_MASK) >> AD1843_RDA1G_SHIFT;
		if (control & MAVB_CONTROL_VOLUME_BUTTON_UP) {
			control &= ~MAVB_CONTROL_VOLUME_BUTTON_UP;
			if (left > 0)
				left--;		/* attenuation! */
			if (right > 0)
				right--;
		}
		if (control & MAVB_CONTROL_VOLUME_BUTTON_DOWN) {
			control &= ~MAVB_CONTROL_VOLUME_BUTTON_DOWN;
			if (left < 63)
				left++;
			if (right < 63)
				right++;
		}
		bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CONTROL, control);

		value &= ~(AD1843_LDA1G_MASK | AD1843_RDA1G_MASK);
		value |= (left << AD1843_LDA1G_SHIFT);
		value |= (right << AD1843_RDA1G_SHIFT);
		ad1843_reg_write(sc, AD1843_DAC1_ANALOG_GAIN, value);

		callout_reset(&sc->sc_volume_button_ch,
		    (hz * MAVB_VOLUME_BUTTON_REPEAT_DELN) / 1000,
		    mavb_button_repeat, sc);
	} else {
		/* Enable volume button interrupts again.  */
		intmask = bus_space_read_8(sc->sc_st, sc->sc_isash,
		     MACE_ISA_INT_MASK);
		bus_space_write_8(sc->sc_st, sc->sc_isash, MACE_ISA_INT_MASK,
		     intmask | MACE_ISA_INT_AUDIO_SC);
	}
}

static int
mavb_intr(void *arg)
{
	struct mavb_softc *sc = arg;
	u_int64_t stat, intmask;

	stat = bus_space_read_8(sc->sc_st, sc->sc_isash, MACE_ISA_INT_STATUS);
	DPRINTF(MAVB_DEBUG_INTR, ("%s: mavb_intr: stat = 0x%llx\n",
            sc->sc_dev.dv_xname, stat));

	if (stat & MACE_ISA_INT_AUDIO_SC) {
		/* Disable volume button interrupts.  */
		intmask = bus_space_read_8(sc->sc_st, sc->sc_isash,
		     MACE_ISA_INT_MASK);
		bus_space_write_8(sc->sc_st, sc->sc_isash, MACE_ISA_INT_MASK,
		     intmask & ~MACE_ISA_INT_AUDIO_SC);

		callout_reset(&sc->sc_volume_button_ch,
		    (hz * MAVB_VOLUME_BUTTON_REPEAT_DEL1) / 1000,
		    mavb_button_repeat, sc);
	}

	if (stat & MACE_ISA_INT_AUDIO_DMA2)
		mavb_dma_output(sc);

	return 1;
}

int
mavb_match(struct device *parent, struct cfdata *match, void *aux)
{
	return (1);
}

void
mavb_attach(struct device *parent, struct device *self, void *aux)
{
	struct mavb_softc *sc = (void *)self;
	struct mace_attach_args *maa = aux;
	bus_dma_segment_t seg;
	u_int64_t control;
	u_int16_t value;
	int rseg;

	sc->sc_st = maa->maa_st;
	if (bus_space_subregion(sc->sc_st, maa->maa_sh, maa->maa_offset,
	    0, &sc->sc_sh) != 0) {
		printf(": can't map i/o space\n");
		return;
	}

	/* XXX We need access to some of the MACE ISA registers.  */
	if (bus_space_subregion(sc->sc_st, maa->maa_sh, 0, 0,
	    &sc->sc_isash) != 0) {
		printf(": can't map isa i/o space\n");
		return;
	}

	/* Set up DMA structures.  */
	sc->sc_dmat = maa->maa_dmat;
	if (bus_dmamap_create(sc->sc_dmat, 4 * MAVB_ISA_RING_SIZE, 1,
	    4 * MAVB_ISA_RING_SIZE, 0, 0, &sc->sc_dmamap)) {
		printf(": can't create MACE ISA DMA map\n");
		return;
	}

	if (bus_dmamem_alloc(sc->sc_dmat, 4 * MAVB_ISA_RING_SIZE,
	    MACE_ISA_RING_ALIGN, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		printf(": can't allocate ring buffer\n");
		return;
	}

	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, 4 * MAVB_ISA_RING_SIZE,
	    (void *)&sc->sc_ring, BUS_DMA_COHERENT)) {
		printf(": can't map ring buffer\n");
		return;
	}

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_ring,
	    4 * MAVB_ISA_RING_SIZE, NULL, BUS_DMA_NOWAIT)) {
		printf(": can't load MACE ISA DMA map\n");
		return;
	}

	sc->sc_ring += MAVB_ISA_RING_SIZE; /* XXX */

	bus_space_write_8(sc->sc_st, sc->sc_isash, MACE_ISA_RINGBASE,
	    sc->sc_dmamap->dm_segs[0].ds_addr);

	/* Establish interrupt.  */
	cpu_intr_establish(maa->maa_intr, maa->maa_intrmask,
	    mavb_intr, sc);

	control = bus_space_read_8(sc->sc_st, sc->sc_sh, MAVB_CONTROL);
	if (!(control & MAVB_CONTROL_CODEC_PRESENT)) {
		printf(": no codec present\n");
		return;
	}

	/* 2. Assert the RESET signal.  */
	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CONTROL,
	    MAVB_CONTROL_RESET);
	delay(1);		/* at least 100 ns */

	/* 3. Deassert the RESET signal and enter a wait period to
              allow the AD1843 internal clocks and the external
              crystal oscillator to stabilize.  */
	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CONTROL, 0);
	delay(800);		/* typically 400 us to 800 us */
	if (ad1843_reg_read(sc, AD1843_CODEC_STATUS) & AD1843_INIT) {
		printf(": codec not ready\n");
		return;
	}

	/* 4. Put the conversion sources into standby.  */
	value = ad1843_reg_read(sc, AD1843_FUNDAMENTAL_SETTINGS);
	ad1843_reg_write(sc, AD1843_FUNDAMENTAL_SETTINGS,
	    value & ~AD1843_PDNI);
	delay (500000);		/* approximately 474 ms */
	if (ad1843_reg_read(sc, AD1843_CODEC_STATUS) & AD1843_PDNO) {
		printf(": can't power up conversion resources\n");
		return;
	}

	/* 5. Power up the clock generators and enable clock output pins.  */
	value = ad1843_reg_read(sc, AD1843_FUNDAMENTAL_SETTINGS);
	ad1843_reg_write(sc, AD1843_FUNDAMENTAL_SETTINGS, value | AD1843_C2EN);

	/* 6. Configure conversion resources while they are in standby.  */
	value = ad1843_reg_read(sc, AD1843_CHANNEL_SAMPLE_RATE);
	ad1843_reg_write(sc, AD1843_CHANNEL_SAMPLE_RATE,
	     value | (2 << AD1843_DA1C_SHIFT));

	/* 7. Enable conversion resources.  */
	value = ad1843_reg_read(sc, AD1843_CHANNEL_POWER_DOWN);
	ad1843_reg_write(sc, AD1843_CHANNEL_POWER_DOWN,
	     value | (AD1843_DA1EN | AD1843_AAMEN));

	/* 8. Configure conversion resources while they are enabled.  */
	value = ad1843_reg_read(sc, AD1843_DAC1_ANALOG_GAIN);
	ad1843_reg_write(sc, AD1843_DAC1_ANALOG_GAIN,
            value & ~(AD1843_LDA1GM | AD1843_RDA1GM));
	value = ad1843_reg_read(sc, AD1843_DAC1_DIGITAL_GAIN);
	ad1843_reg_write(sc, AD1843_DAC1_DIGITAL_GAIN,
            value & ~(AD1843_LDA1AM | AD1843_RDA1AM));
	value = ad1843_reg_read(sc, AD1843_MISC_SETTINGS);
	ad1843_reg_write(sc, AD1843_MISC_SETTINGS,
            value & ~(AD1843_HPOM | AD1843_MNOM));

	value = ad1843_reg_read(sc, AD1843_CODEC_STATUS);
	printf(": AD1843 rev %d\n", (u_int)value & AD1843_REVISION_MASK);

	sc->sc_play_rate = 48000;
	sc->sc_play_format = AD1843_PCM8;

	callout_init(&sc->sc_volume_button_ch, 0);

	audio_attach_mi(&mavb_sa_hw_if, sc, &sc->sc_dev);

	return;
}

u_int16_t
ad1843_reg_read(struct mavb_softc *sc, ad1843_addr_t addr)
{
	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CODEC_CONTROL,
            (addr & MAVB_CODEC_ADDRESS_MASK) << MAVB_CODEC_ADDRESS_SHIFT |
	    MAVB_CODEC_READ);
	delay(200);
	return bus_space_read_8(sc->sc_st, sc->sc_sh, MAVB_CODEC_STATUS);
}

u_int16_t
ad1843_reg_write(struct mavb_softc *sc, ad1843_addr_t addr, u_int16_t value)
{
	bus_space_write_8(sc->sc_st, sc->sc_sh, MAVB_CODEC_CONTROL,
	    (addr & MAVB_CODEC_ADDRESS_MASK) << MAVB_CODEC_ADDRESS_SHIFT |
	    (value & MAVB_CODEC_WORD_MASK) << MAVB_CODEC_WORD_SHIFT);
	delay(200);
	return bus_space_read_8(sc->sc_st, sc->sc_sh, MAVB_CODEC_STATUS);
}

void
ad1843_dump_regs(struct mavb_softc *sc)
{
	u_int16_t addr;

	for (addr = 0; addr < AD1843_NREGS; addr++)
		printf("%d: 0x%04x\n", addr, ad1843_reg_read(sc, addr));
}
