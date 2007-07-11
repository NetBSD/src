/*	$NetBSD: zaudio.c,v 1.3.6.1 2007/07/11 20:03:40 mjf Exp $	*/
/*	$OpenBSD: zaurus_audio.c,v 1.8 2005/08/18 13:23:02 robert Exp $	*/

/*
 * Copyright (c) 2005 Christopher Pascoe <pascoe@openbsd.org>
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

/*
 * TODO:
 *	- powerhooks (currently only works until first suspend)
 *	- record support
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/audioio.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>
#include <arm/xscale/pxa2x0_i2s.h>
#include <arm/xscale/pxa2x0_dmac.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <zaurus/dev/wm8750reg.h>
#include <zaurus/dev/scoopvar.h>

#define WM8750_ADDRESS  0x1B
#define SPKR_VOLUME	112

#define wm8750_write(sc, reg, val) \
	pxa2x0_i2c_write_2(&sc->sc_i2c, WM8750_ADDRESS, \
	    (((reg) << 9) | ((val) & 0x1ff)))

static int	zaudio_match(struct device *, struct cfdata *, void *);
static void	zaudio_attach(struct device *, struct device *, void *);
static int	zaudio_detach(struct device *, int);
static void	zaudio_power(int, void *);

#define ZAUDIO_OP_SPKR	0
#define ZAUDIO_OP_HP	1

#define ZAUDIO_JACK_STATE_OUT	0
#define ZAUDIO_JACK_STATE_IN	1
#define ZAUDIO_JACK_STATE_INS	2
#define ZAUDIO_JACK_STATE_REM	3

/* GPIO pins */
#define GPIO_HP_IN_C3000	116

struct zaudio_volume {
	u_int8_t		left;
	u_int8_t		right;
};

struct zaudio_softc {
	struct device		sc_dev;

	/* i2s device softc */
	/* NB: pxa2x0_i2s requires this to be the second struct member */
	struct pxa2x0_i2s_softc	sc_i2s;

	/* i2c device softc */
	struct pxa2x0_i2c_softc	sc_i2c;

	void			*sc_powerhook;
	int			sc_playing;

	struct zaudio_volume	sc_volume[2];
	char			sc_unmute[2];

	int			sc_state;
	int			sc_icount;
	struct callout		sc_to; 
};

CFATTACH_DECL(zaudio, sizeof(struct zaudio_softc), 
    zaudio_match, zaudio_attach, zaudio_detach, NULL);

static struct audio_device wm8750_device = {
	"WM8750",
	"1.0",
	"wm"
};

#define ZAUDIO_NFORMATS	4
static const struct audio_format zaudio_formats[ZAUDIO_NFORMATS] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 2, AUFMT_STEREO, 0, {4000, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 1, AUFMT_MONAURAL, 0, {4000, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULINEAR_LE, 8, 8,
	 2, AUFMT_STEREO, 0, {4000, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULINEAR_LE, 8, 8,
	 1, AUFMT_MONAURAL, 0, {4000, 48000}},
};

void zaudio_init(struct zaudio_softc *);
static int zaudio_jack_intr(void *);
void zaudio_jack(void *);
void zaudio_standby(struct zaudio_softc *);
void zaudio_update_volume(struct zaudio_softc *, int);
void zaudio_update_mutes(struct zaudio_softc *);
void zaudio_play_setup(struct zaudio_softc *);
static int zaudio_open(void *, int);
static void zaudio_close(void *);
static int zaudio_query_encoding(void *, struct audio_encoding *);
static int zaudio_set_params(void *, int, int, audio_params_t *,
    audio_params_t *, stream_filter_list_t *, stream_filter_list_t *);
static int zaudio_round_blocksize(void *, int, int, const audio_params_t *);
static int zaudio_start_output(void *, void *, int, void (*)(void *), void *);
static int zaudio_start_input(void *, void *, int, void (*)(void *), void *);
static int zaudio_halt_output(void *);
static int zaudio_halt_input(void *);
static int zaudio_getdev(void *, struct audio_device *);
static int zaudio_set_port(void *, struct mixer_ctrl *);
static int zaudio_get_port(void *, struct mixer_ctrl *);
static int zaudio_query_devinfo(void *, struct mixer_devinfo *);
static void *zaudio_allocm(void *, int, size_t, struct malloc_type *, int);
static void zaudio_freem(void  *, void *, struct malloc_type *);
static size_t zaudio_round_buffersize(void *, int, size_t);
static paddr_t zaudio_mappage(void *, void *, off_t, int);
static int zaudio_get_props(void *);

struct audio_hw_if wm8750_hw_if = {
	zaudio_open,
	zaudio_close,
	NULL,
	zaudio_query_encoding,
	zaudio_set_params,
	zaudio_round_blocksize,
	NULL,
	NULL,
	NULL,
	zaudio_start_output,
	zaudio_start_input,
	zaudio_halt_output,
	zaudio_halt_input,
	NULL,
	zaudio_getdev,
	NULL,
	zaudio_set_port,
	zaudio_get_port,
	zaudio_query_devinfo,
	zaudio_allocm,
	zaudio_freem,
	zaudio_round_buffersize,
	zaudio_mappage,
	zaudio_get_props,
	NULL,
	NULL,
	NULL,
};

static const uint16_t playback_registers[][2] = {
	/* Unmute DAC */
	{ ADCDACCTL_REG, 0x000 },

	/* 16 bit audio words */
	{ AUDINT_REG, AUDINT_SET_FORMAT(2) },

	/* Enable thermal protection, power */
	{ ADCTL1_REG, ADCTL1_TSDEN | ADCTL1_SET_VSEL(3) },

	/* Enable speaker driver, DAC oversampling */
	{ ADCTL2_REG, ADCTL2_ROUT2INV | ADCTL2_DACOSR },

	/* Set DAC voltage references */
	{ PWRMGMT1_REG, PWRMGMT1_SET_VMIDSEL(1) | PWRMGMT1_VREF },

	/* Direct DACs to output mixers */
	{ LOUTMIX1_REG, LOUTMIX1_LD2LO },
	{ ROUTMIX2_REG, ROUTMIX2_RD2RO },

	/* End of list */
	{ 0xffff, 0xffff }
};

static int
zaudio_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return 1;
}

static void
zaudio_attach(struct device *parent, struct device *self, void *aux)
{
	struct zaudio_softc *sc = (struct zaudio_softc *)self;
	struct pxaip_attach_args *pxa = aux;
	int rv;

	sc->sc_powerhook = powerhook_establish(sc->sc_dev.dv_xname,
	    zaudio_power, sc);
	if (sc->sc_powerhook == NULL) {
		printf(": unable to establish powerhook\n");
		return;
	}

	sc->sc_i2s.sc_iot = pxa->pxa_iot;
	sc->sc_i2s.sc_dmat = pxa->pxa_dmat;
	sc->sc_i2s.sc_size = PXA2X0_I2S_SIZE;
	if (pxa2x0_i2s_attach_sub(&sc->sc_i2s)) {
		printf(": unable to attach I2S\n");
		goto fail_i2s;
	}

	sc->sc_i2c.sc_iot = pxa->pxa_iot;
	sc->sc_i2c.sc_size = PXA2X0_I2C_SIZE;
	if (pxa2x0_i2c_attach_sub(&sc->sc_i2c)) {
		printf(": unable to attach I2C\n");
		goto fail_i2c;
	}

	/* Check for an I2C response from the wm8750 */
	pxa2x0_i2c_open(&sc->sc_i2c);
	rv = wm8750_write(sc, RESET_REG, 0);
	pxa2x0_i2c_close(&sc->sc_i2c);

	if (rv) {
		printf(": codec failed to respond\n");
		goto fail_probe;
	}
	delay(100);

	/* Speaker on, headphones off by default. */
	sc->sc_volume[ZAUDIO_OP_SPKR].left = 240;
	sc->sc_unmute[ZAUDIO_OP_SPKR] = 1;
	sc->sc_volume[ZAUDIO_OP_HP].left = 180;
	sc->sc_volume[ZAUDIO_OP_HP].right = 180;
	sc->sc_unmute[ZAUDIO_OP_HP] = 0;

	/* Configure headphone jack state change handling. */
	callout_init(&sc->sc_to, 0);
	callout_setfunc(&sc->sc_to, zaudio_jack, sc);
	pxa2x0_gpio_set_function(GPIO_HP_IN_C3000, GPIO_IN);
	(void)pxa2x0_gpio_intr_establish(GPIO_HP_IN_C3000,
	    IST_EDGE_BOTH, IPL_BIO, zaudio_jack_intr, sc);

	zaudio_init(sc);

	printf(": I2C, I2S, WM8750 Audio\n");

	audio_attach_mi(&wm8750_hw_if, sc, &sc->sc_dev);

	return;

fail_probe:
	pxa2x0_i2c_detach_sub(&sc->sc_i2c);
fail_i2c:
	pxa2x0_i2s_detach_sub(&sc->sc_i2s);
fail_i2s:
	powerhook_disestablish(sc->sc_powerhook);
}

static int
zaudio_detach(struct device *self, int flags)
{
	struct zaudio_softc *sc = (struct zaudio_softc *)self;

	if (sc->sc_powerhook != NULL) {
		powerhook_disestablish(sc->sc_powerhook);
		sc->sc_powerhook = NULL;
	}

	pxa2x0_i2c_detach_sub(&sc->sc_i2c);
	pxa2x0_i2s_detach_sub(&sc->sc_i2s);

	return 0;
}

static void
zaudio_power(int why, void *arg)
{
	struct zaudio_softc *sc = arg;

	switch (why) {
	case PWR_STANDBY:
	case PWR_SUSPEND:
		callout_stop(&sc->sc_to);
		zaudio_standby(sc);
		break;

	case PWR_RESUME:
		pxa2x0_i2s_init(&sc->sc_i2s);
		pxa2x0_i2c_init(&sc->sc_i2c);
		zaudio_init(sc);
		break;
	}
}

void
zaudio_init(struct zaudio_softc *sc)
{

	pxa2x0_i2c_open(&sc->sc_i2c);

	/* Reset the codec */
	wm8750_write(sc, RESET_REG, 0);
	delay(100);

	/* Switch to standby power only */
	wm8750_write(sc, PWRMGMT1_REG, PWRMGMT1_SET_VMIDSEL(2));
	wm8750_write(sc, PWRMGMT2_REG, 0);

	/* Configure digital interface for I2S */
	wm8750_write(sc, AUDINT_REG, AUDINT_SET_FORMAT(2));

	/* Initialise volume levels */
	zaudio_update_volume(sc, ZAUDIO_OP_SPKR);
	zaudio_update_volume(sc, ZAUDIO_OP_HP);

	pxa2x0_i2c_close(&sc->sc_i2c);

	scoop_set_headphone(0);

	/* Assume that the jack state has changed. */ 
	zaudio_jack(sc);

}

static int
zaudio_jack_intr(void *v)
{
	struct zaudio_softc *sc = v;

	if (!callout_active(&sc->sc_to))
		zaudio_jack(sc);
	
	return 1;
}

void
zaudio_jack(void *v)
{
	struct zaudio_softc *sc = v;

	switch (sc->sc_state) {
	case ZAUDIO_JACK_STATE_OUT:
		if (pxa2x0_gpio_get_bit(GPIO_HP_IN_C3000)) {
			sc->sc_state = ZAUDIO_JACK_STATE_INS;
			sc->sc_icount = 0;
		}
		break;

	case ZAUDIO_JACK_STATE_INS:
		if (sc->sc_icount++ > 2) {
			if (pxa2x0_gpio_get_bit(GPIO_HP_IN_C3000)) {
				sc->sc_state = ZAUDIO_JACK_STATE_IN;
				sc->sc_unmute[ZAUDIO_OP_SPKR] = 0;
				sc->sc_unmute[ZAUDIO_OP_HP] = 1;
				goto update_mutes;
			} else 
				sc->sc_state = ZAUDIO_JACK_STATE_OUT;
		}
		break;

	case ZAUDIO_JACK_STATE_IN:
		if (!pxa2x0_gpio_get_bit(GPIO_HP_IN_C3000)) {
			sc->sc_state = ZAUDIO_JACK_STATE_REM;
			sc->sc_icount = 0;
		}
		break;

	case ZAUDIO_JACK_STATE_REM: 
		if (sc->sc_icount++ > 2) {
			if (!pxa2x0_gpio_get_bit(GPIO_HP_IN_C3000)) {
				sc->sc_state = ZAUDIO_JACK_STATE_OUT;
				sc->sc_unmute[ZAUDIO_OP_SPKR] = 1;
				sc->sc_unmute[ZAUDIO_OP_HP] = 0;
				goto update_mutes;
			} else
				sc->sc_state = ZAUDIO_JACK_STATE_IN;
		}
		break;
	}
	
	callout_schedule(&sc->sc_to, hz/4);

	return;

update_mutes:
	callout_stop(&sc->sc_to);

	if (sc->sc_playing) {
		pxa2x0_i2c_open(&sc->sc_i2c);
		zaudio_update_mutes(sc);
		pxa2x0_i2c_close(&sc->sc_i2c);
	}
}

void
zaudio_standby(struct zaudio_softc *sc)
{

	pxa2x0_i2c_open(&sc->sc_i2c);

	/* Switch codec to standby power only */
	wm8750_write(sc, PWRMGMT1_REG, PWRMGMT1_SET_VMIDSEL(2));
	wm8750_write(sc, PWRMGMT2_REG, 0);

	pxa2x0_i2c_close(&sc->sc_i2c);

	scoop_set_headphone(0);
}

void
zaudio_update_volume(struct zaudio_softc *sc, int output)
{

	switch (output) {
	case ZAUDIO_OP_SPKR:
		wm8750_write(sc, LOUT2VOL_REG, LOUT2VOL_LO2VU | LOUT2VOL_LO2ZC |
		    LOUT2VOL_SET_LOUT2VOL(sc->sc_volume[ZAUDIO_OP_SPKR
		    ].left >> 1));
		wm8750_write(sc, ROUT2VOL_REG, ROUT2VOL_RO2VU | ROUT2VOL_RO2ZC |
		    ROUT2VOL_SET_ROUT2VOL(sc->sc_volume[ZAUDIO_OP_SPKR
		    ].left >> 1));
		break;

	case ZAUDIO_OP_HP:
		wm8750_write(sc, LOUT1VOL_REG, LOUT1VOL_LO1VU | LOUT1VOL_LO1ZC |
		    LOUT1VOL_SET_LOUT1VOL(sc->sc_volume[ZAUDIO_OP_HP
		    ].left >> 1));
		wm8750_write(sc, ROUT1VOL_REG, ROUT1VOL_RO1VU | ROUT1VOL_RO1ZC |
		    ROUT1VOL_SET_ROUT1VOL(sc->sc_volume[ZAUDIO_OP_HP
		    ].right >> 1));
		break;
	}
}

void
zaudio_update_mutes(struct zaudio_softc *sc)
{
	uint16_t val;

	val = PWRMGMT2_DACL | PWRMGMT2_DACR;

	if (sc->sc_unmute[ZAUDIO_OP_SPKR])
		val |= PWRMGMT2_LOUT2 | PWRMGMT2_ROUT2;

	if (sc->sc_unmute[ZAUDIO_OP_HP])
		val |= PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1;

	wm8750_write(sc, PWRMGMT2_REG, val);

	scoop_set_headphone(sc->sc_unmute[ZAUDIO_OP_HP]);
}

void
zaudio_play_setup(struct zaudio_softc *sc)
{
	int i;

	pxa2x0_i2c_open(&sc->sc_i2c);

	/* Program the codec with playback settings */
	for (i = 0; playback_registers[i][0] != 0xffff; i++) {
		wm8750_write(sc, playback_registers[i][0],
		    playback_registers[i][1]);
	}
	zaudio_update_mutes(sc);

	pxa2x0_i2c_close(&sc->sc_i2c);
}

/*
 * audio operation functions.
 */
static int
zaudio_open(void *hdl, int flags)
{
	struct zaudio_softc *sc = hdl;

	/* Power on the I2S bus and codec */
	pxa2x0_i2s_open(&sc->sc_i2s);

	return 0;
}

static void
zaudio_close(void *hdl)
{
	struct zaudio_softc *sc = hdl;

	/* Power off the I2S bus and codec */
	pxa2x0_i2s_close(&sc->sc_i2s);
}

static int
zaudio_query_encoding(void *hdl, struct audio_encoding *aep)
{

	switch (aep->index) {
	case 0:
		strlcpy(aep->name, AudioEulinear, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_ULINEAR;
		aep->precision = 8;
		aep->flags = 0;
		break;

	case 1:
		strlcpy(aep->name, AudioEmulaw, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_ULAW;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	case 2:
		strlcpy(aep->name, AudioEalaw, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_ALAW;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	case 3:
		strlcpy(aep->name, AudioEslinear, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_SLINEAR;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	case 4:
		strlcpy(aep->name, AudioEslinear_le, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_SLINEAR_LE;
		aep->precision = 16;
		aep->flags = 0;
		break;

	case 5:
		strlcpy(aep->name, AudioEulinear_le, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_ULINEAR_LE;
		aep->precision = 16;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	case 6:
		strlcpy(aep->name, AudioEslinear_be, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_SLINEAR_BE;
		aep->precision = 16;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	case 7:
		strlcpy(aep->name, AudioEulinear_be, sizeof(aep->name));
		aep->encoding = AUDIO_ENCODING_ULINEAR_BE;
		aep->precision = 16;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	default:
		return EINVAL;
	}

	return 0;
}

static int
zaudio_set_params(void *hdl, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct zaudio_softc *sc = hdl;
	struct audio_params *p;
	stream_filter_list_t *fil;
	int mode, i;

	if (play->sample_rate != rec->sample_rate &&
	    usemode == (AUMODE_PLAY | AUMODE_RECORD)) {
		if (setmode == AUMODE_PLAY) {
			rec->sample_rate = play->sample_rate;
			setmode |= AUMODE_RECORD;
		} else if (setmode == AUMODE_RECORD) {
			play->sample_rate = rec->sample_rate;
			setmode |= AUMODE_PLAY;
		} else
			return EINVAL;
	}

	for (mode = AUMODE_RECORD; mode != -1;
	     mode = (mode == AUMODE_RECORD) ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = (mode == AUMODE_PLAY) ? play : rec;

		if (p->sample_rate < 4000 || p->sample_rate > 48000 ||
		    (p->precision != 8 && p->precision != 16) ||
		    (p->channels != 1 && p->channels != 2))
			return EINVAL;

		fil = (mode == AUMODE_PLAY) ? pfil : rfil;
		i = auconv_set_converter(zaudio_formats, ZAUDIO_NFORMATS,
					 mode, p, false, fil);
		if (i < 0)
			return EINVAL;
	}

	if (setmode == AUMODE_RECORD)
		pxa2x0_i2s_setspeed(&sc->sc_i2s, &rec->sample_rate);
	else
		pxa2x0_i2s_setspeed(&sc->sc_i2s, &play->sample_rate);

	return 0;
}

static int
zaudio_round_blocksize(void *hdl, int bs, int mode, const audio_params_t *param)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_round_blocksize(&sc->sc_i2s, bs, mode, param);
}


static int
zaudio_halt_output(void *hdl)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	rv = pxa2x0_i2s_halt_output(&sc->sc_i2s);
	zaudio_standby(sc);
	sc->sc_playing = 0;

	return rv;
}

static int
zaudio_halt_input(void *hdl)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	rv = pxa2x0_i2s_halt_input(&sc->sc_i2s);

	return rv;
}

static int
zaudio_getdev(void *hdl, struct audio_device *ret)
{
	/* struct zaudio_softc *sc = hdl; */

	*ret = wm8750_device;
	return 0;
}

#define ZAUDIO_SPKR_LVL		0
#define ZAUDIO_SPKR_MUTE	1
#define ZAUDIO_HP_LVL		2
#define ZAUDIO_HP_MUTE		3
#define ZAUDIO_OUTPUT_CLASS	4

static int
zaudio_set_port(void *hdl, struct mixer_ctrl *mc)
{
	struct zaudio_softc *sc = hdl;
	int error = EINVAL;
	int s;

	s = splbio();
	pxa2x0_i2c_open(&sc->sc_i2c);

	switch (mc->dev) {
	case ZAUDIO_SPKR_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			sc->sc_volume[ZAUDIO_OP_SPKR].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		else
			break;
		zaudio_update_volume(sc, ZAUDIO_OP_SPKR);
		error = 0;
		break;

	case ZAUDIO_SPKR_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		sc->sc_unmute[ZAUDIO_OP_SPKR] = mc->un.ord ? 1 : 0;
		zaudio_update_mutes(sc);
		error = 0;
		break;

	case ZAUDIO_HP_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1) {
			sc->sc_volume[ZAUDIO_OP_HP].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
			sc->sc_volume[ZAUDIO_OP_HP].right =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		} else if (mc->un.value.num_channels == 2) {
			sc->sc_volume[ZAUDIO_OP_HP].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
			sc->sc_volume[ZAUDIO_OP_HP].right =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		}
		else
			break;
		zaudio_update_volume(sc, ZAUDIO_OP_HP);
		error = 0;
		break;

	case ZAUDIO_HP_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		sc->sc_unmute[ZAUDIO_OP_HP] = mc->un.ord ? 1 : 0;
		zaudio_update_mutes(sc);
		error = 0;
		break;
	}

	pxa2x0_i2c_close(&sc->sc_i2c);
	splx(s);

	return error;
}

static int
zaudio_get_port(void *hdl, struct mixer_ctrl *mc)
{
	struct zaudio_softc *sc = hdl;
	int error = EINVAL;

	switch (mc->dev) {
	case ZAUDIO_SPKR_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_volume[ZAUDIO_OP_SPKR].left;
		else
			break;
		error = 0;
		break;

	case ZAUDIO_SPKR_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = sc->sc_unmute[ZAUDIO_OP_SPKR] ? 1 : 0;
		error = 0;
		break;

	case ZAUDIO_HP_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_volume[ZAUDIO_OP_HP].left;
		else if (mc->un.value.num_channels == 2) {
			mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    sc->sc_volume[ZAUDIO_OP_HP].left;
			mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    sc->sc_volume[ZAUDIO_OP_HP].right;
		}
		else
			break;
		error = 0;
		break;

	case ZAUDIO_HP_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = sc->sc_unmute[ZAUDIO_OP_HP] ? 1 : 0;
		error = 0;
		break;
	}

	return error;
}

static int
zaudio_query_devinfo(void *hdl, struct mixer_devinfo *di)
{
	/* struct zaudio_softc *sc = hdl; */

	switch (di->index) {
	case ZAUDIO_SPKR_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = ZAUDIO_SPKR_MUTE;
		strlcpy(di->label.name, AudioNspeaker,
		    sizeof(di->label.name));
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof(di->un.v.units.name));
		di->un.v.num_channels = 1;
		break;

	case ZAUDIO_SPKR_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = ZAUDIO_SPKR_LVL;
		di->next = AUDIO_MIXER_LAST;
		goto mute;

	case ZAUDIO_HP_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = ZAUDIO_HP_MUTE;
		strlcpy(di->label.name, AudioNheadphone,
		    sizeof(di->label.name));
		di->un.v.num_channels = 1;
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof(di->un.v.units.name));
		break;

	case ZAUDIO_HP_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = ZAUDIO_HP_LVL;
		di->next = AUDIO_MIXER_LAST;
mute:
		strlcpy(di->label.name, AudioNmute, sizeof(di->label.name));
		di->un.e.num_mem = 2;
		strlcpy(di->un.e.member[0].label.name, AudioNon,
		    sizeof(di->un.e.member[0].label.name));
		di->un.e.member[0].ord = 0;
		strlcpy(di->un.e.member[1].label.name, AudioNoff,
		    sizeof(di->un.e.member[1].label.name));
		di->un.e.member[1].ord = 1;
		break;

	case ZAUDIO_OUTPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCoutputs,
		    sizeof(di->label.name));
		break;

	default:
		return ENXIO;
	}

	return 0;
}

static void *
zaudio_allocm(void *hdl, int direction, size_t size,
    struct malloc_type *type, int flags)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_allocm(&sc->sc_i2s, direction, size, type, flags);
}

static void
zaudio_freem(void *hdl, void *ptr, struct malloc_type *type)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_freem(&sc->sc_i2s, ptr, type);
}

static size_t
zaudio_round_buffersize(void *hdl, int direction, size_t bufsize)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_round_buffersize(&sc->sc_i2s, direction, bufsize);
}

static paddr_t
zaudio_mappage(void *hdl, void *mem, off_t off, int prot)
{
	struct zaudio_softc *sc = hdl;
	
	return pxa2x0_i2s_mappage(&sc->sc_i2s, mem, off, prot);
}

static int
zaudio_get_props(void *hdl)
{

	return AUDIO_PROP_MMAP|AUDIO_PROP_INDEPENDENT|AUDIO_PROP_FULLDUPLEX;
}

static int
zaudio_start_output(void *hdl, void *block, int bsize, void (*intr)(void *),
    void *intrarg)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	/* Power up codec if we are not already playing. */
	if (!sc->sc_playing) {
		sc->sc_playing = 1;
		zaudio_play_setup(sc);
	}

	/* Start DMA via I2S */
	rv = pxa2x0_i2s_start_output(&sc->sc_i2s, block, bsize, intr, intrarg);
	if (rv) {
		zaudio_standby(sc);
		sc->sc_playing = 0;
	}
	return rv;
}

static int
zaudio_start_input(void *hdl, void *block, int bsize, void (*intr)(void *),
    void *intrarg)
{

	return ENXIO;
}
