/*	$NetBSD: zaudio.c,v 1.7.2.1 2009/05/13 17:18:51 jym Exp $	*/
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

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * TODO:
 *	- powerhooks (currently only works until first suspend)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zaudio.c,v 1.7.2.1 2009/05/13 17:18:51 jym Exp $");

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

#define wm8750_write(sc, reg, val) \
	pxa2x0_i2c_write_2(&sc->sc_i2c, WM8750_ADDRESS, \
	    (((reg) << 9) | ((val) & 0x1ff)))

static int	zaudio_match(device_t, cfdata_t, void *);
static void	zaudio_attach(device_t, device_t, void *);
static bool	zaudio_suspend(device_t dv PMF_FN_ARGS);
static bool	zaudio_resume(device_t dv PMF_FN_ARGS);

#define ZAUDIO_OP_SPKR	0
#define ZAUDIO_OP_HP	1
#define ZAUDIO_OP_MIC	2
#define ZAUDIO_OP_NUM	3

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
	device_t		sc_dev;

	/* i2s device softc */
	/* NB: pxa2x0_i2s requires this to be the second struct member */
	struct pxa2x0_i2s_softc	sc_i2s;

	/* i2c device softc */
	struct pxa2x0_i2c_softc	sc_i2c;

	int			sc_playing;
	int			sc_recording;

	struct zaudio_volume	sc_volume[ZAUDIO_OP_NUM];
	char			sc_unmute[ZAUDIO_OP_NUM];

	int			sc_state;
	int			sc_icount;
	struct callout		sc_to; 
};

CFATTACH_DECL_NEW(zaudio, sizeof(struct zaudio_softc), 
    zaudio_match, zaudio_attach, NULL, NULL);

static struct audio_device wm8750_device = {
	"WM8750",
	"1.0",
	"wm"
};

static const struct audio_format zaudio_formats[] = {
	{
		.driver_data	= NULL,
		.mode		= AUMODE_PLAY | AUMODE_RECORD,
		.encoding	= AUDIO_ENCODING_SLINEAR_LE,
		.validbits	= 16,
		.precision	= 16,
		.channels	= 2,
		.channel_mask	= AUFMT_STEREO,
		.frequency_type	= 0,
		.frequency	= { 4000, 48000 }
	},
	{
		.driver_data	= NULL,
		.mode		= AUMODE_PLAY | AUMODE_RECORD,
		.encoding	= AUDIO_ENCODING_SLINEAR_LE,
		.validbits	= 16,
		.precision	= 16,
		.channels	= 1,
		.channel_mask	= AUFMT_MONAURAL,
		.frequency_type	= 0,
		.frequency	= { 4000, 48000 }
	},
	{
		.driver_data	= NULL,
		.mode		= AUMODE_PLAY | AUMODE_RECORD,
		.encoding	= AUDIO_ENCODING_ULINEAR_LE,
		.validbits	= 8,
		.precision	= 8,
		.channels	= 2,
		.channel_mask	= AUFMT_STEREO,
		.frequency_type	= 0,
		.frequency	= { 4000, 48000 }
	},
	{
		.driver_data	= NULL,
		.mode		= AUMODE_PLAY | AUMODE_RECORD,
		.encoding	= AUDIO_ENCODING_ULINEAR_LE,
		.validbits	= 8,
		.precision	= 8,
		.channels	= 1,
		.channel_mask	= AUFMT_MONAURAL,
		.frequency_type	= 0,
		.frequency	= { 4000, 48000 }
	},
};
static const int zaudio_nformats = (int)__arraycount(zaudio_formats);

static void zaudio_init(struct zaudio_softc *);
static int zaudio_jack_intr(void *);
static void zaudio_jack(void *);
static void zaudio_standby(struct zaudio_softc *);
static void zaudio_update_volume(struct zaudio_softc *, int);
static void zaudio_update_mutes(struct zaudio_softc *, int);
static void zaudio_play_setup(struct zaudio_softc *);
/*static*/ void zaudio_record_setup(struct zaudio_softc *);
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
	.open			= zaudio_open,
	.close			= zaudio_close,
	.drain			= NULL,
	.query_encoding		= zaudio_query_encoding,
	.set_params		= zaudio_set_params,
	.round_blocksize	= zaudio_round_blocksize,
	.commit_settings	= NULL,
	.init_output		= NULL,
	.init_input		= NULL,
	.start_output		= zaudio_start_output,
	.start_input		= zaudio_start_input,
	.halt_output		= zaudio_halt_output,
	.halt_input		= zaudio_halt_input,
	.speaker_ctl		= NULL,
	.getdev			= zaudio_getdev,
	.setfd			= NULL,
	.set_port		= zaudio_set_port,
	.get_port		= zaudio_get_port,
	.query_devinfo		= zaudio_query_devinfo,
	.allocm			= zaudio_allocm,
	.freem			= zaudio_freem,
	.round_buffersize	= zaudio_round_buffersize,
	.mappage		= zaudio_mappage,
	.get_props		= zaudio_get_props,
	.trigger_output		= NULL,
	.trigger_input		= NULL,
	.dev_ioctl		= NULL,
	.powerstate		= NULL,
};

static const uint16_t playback_regs[][2] = {
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
	{ LOUTMIX2_REG, 0x000 },
	{ ROUTMIX1_REG, 0x000 },
	{ ROUTMIX2_REG, ROUTMIX2_RD2RO },

	/* End of list */
	{ 0xffff, 0xffff }
};

static const uint16_t record_regs[][2] = {
	/* Unmute DAC */
	{ ADCDACCTL_REG, 0x000 },

	/* 16 bit audio words */
	{ AUDINT_REG, AUDINT_SET_FORMAT(2) },

	/* Enable thermal protection, power, left DAC for both channel */
	{ ADCTL1_REG, ADCTL1_TSDEN | ADCTL1_SET_VSEL(3)
	              | ADCTL1_SET_DATSEL(1) },

	/* Diffrential input select: LINPUT1-RINPUT1, stereo */
	{ ADCINPMODE_REG, 0x000 },

	/* L-R differential, micboost 20dB */
	{ ADCLSPATH_REG, ADCLSPATH_SET_LINSEL(3) | ADCLSPATH_SET_LMICBOOST(2) },
	{ ADCRSPATH_REG, ADCRSPATH_SET_RINSEL(3) | ADCRSPATH_SET_RMICBOOST(2) },

	/* End of list */
	{ 0xffff, 0xffff }
};

static int
zaudio_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
zaudio_attach(device_t parent, device_t self, void *aux)
{
	struct zaudio_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	int rv;

	sc->sc_dev = self;

	aprint_normal(": I2C, I2S, WM8750 Audio\n");
	aprint_naive("\n");

	if (!pmf_device_register(sc->sc_dev, zaudio_suspend, zaudio_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	sc->sc_i2s.sc_iot = pxa->pxa_iot;
	sc->sc_i2s.sc_dmat = pxa->pxa_dmat;
	sc->sc_i2s.sc_size = PXA2X0_I2S_SIZE;
	if (pxa2x0_i2s_attach_sub(&sc->sc_i2s)) {
		aprint_error_dev(sc->sc_dev, "unable to attach I2S\n");
		goto fail_i2s;
	}

	sc->sc_i2c.sc_iot = pxa->pxa_iot;
	sc->sc_i2c.sc_size = PXA2X0_I2C_SIZE;
	if (pxa2x0_i2c_attach_sub(&sc->sc_i2c)) {
		aprint_error_dev(sc->sc_dev, "unable to attach I2C\n");
		goto fail_i2c;
	}

	/* Check for an I2C response from the wm8750 */
	pxa2x0_i2c_open(&sc->sc_i2c);
	rv = wm8750_write(sc, RESET_REG, 0);
	pxa2x0_i2c_close(&sc->sc_i2c);
	if (rv) {
		aprint_error_dev(sc->sc_dev, "codec failed to respond\n");
		goto fail_probe;
	}
	delay(100);

	/* Speaker on, headphones off by default. */
	sc->sc_volume[ZAUDIO_OP_SPKR].left = 240;
	sc->sc_unmute[ZAUDIO_OP_SPKR] = 1;
	sc->sc_volume[ZAUDIO_OP_HP].left = 180;
	sc->sc_volume[ZAUDIO_OP_HP].right = 180;
	sc->sc_unmute[ZAUDIO_OP_HP] = 0;
	sc->sc_volume[ZAUDIO_OP_MIC].left = 240;
	sc->sc_unmute[ZAUDIO_OP_MIC] = 0;

	/* Configure headphone jack state change handling. */
	callout_init(&sc->sc_to, 0);
	callout_setfunc(&sc->sc_to, zaudio_jack, sc);
	pxa2x0_gpio_set_function(GPIO_HP_IN_C3000, GPIO_IN);
	(void) pxa2x0_gpio_intr_establish(GPIO_HP_IN_C3000, IST_EDGE_BOTH,
	    IPL_BIO, zaudio_jack_intr, sc);

	zaudio_init(sc);

	audio_attach_mi(&wm8750_hw_if, sc, sc->sc_dev);

	return;

fail_probe:
	pxa2x0_i2c_detach_sub(&sc->sc_i2c);
fail_i2c:
	pxa2x0_i2s_detach_sub(&sc->sc_i2s);
fail_i2s:
	pmf_device_deregister(self);
}

static bool
zaudio_suspend(device_t dv PMF_FN_ARGS)
{
	struct zaudio_softc *sc = device_private(dv);

	callout_stop(&sc->sc_to);
	zaudio_standby(sc);

	return true;
}

static bool
zaudio_resume(device_t dv PMF_FN_ARGS)
{
	struct zaudio_softc *sc = device_private(dv);

	pxa2x0_i2s_init(&sc->sc_i2s);
	pxa2x0_i2c_init(&sc->sc_i2c);
	zaudio_init(sc);

	return true;
}

static void
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
	zaudio_update_volume(sc, ZAUDIO_OP_MIC);

	pxa2x0_i2c_close(&sc->sc_i2c);

	scoop_set_headphone(0);
	scoop_set_mic_bias(0);

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

static void
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
				sc->sc_unmute[ZAUDIO_OP_MIC] = 1;
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
				sc->sc_unmute[ZAUDIO_OP_MIC] = 0;
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

	if (sc->sc_playing || sc->sc_recording) {
		pxa2x0_i2c_open(&sc->sc_i2c);
		if (sc->sc_playing)
			zaudio_update_mutes(sc, 1);
		if (sc->sc_recording)
			zaudio_update_mutes(sc, 2);
		pxa2x0_i2c_close(&sc->sc_i2c);
	}
}

static void
zaudio_standby(struct zaudio_softc *sc)
{

	pxa2x0_i2c_open(&sc->sc_i2c);

	/* Switch codec to standby power only */
	wm8750_write(sc, PWRMGMT1_REG, PWRMGMT1_SET_VMIDSEL(2));
	wm8750_write(sc, PWRMGMT2_REG, 0);

	pxa2x0_i2c_close(&sc->sc_i2c);

	scoop_set_headphone(0);
	scoop_set_mic_bias(0);
}

static void
zaudio_update_volume(struct zaudio_softc *sc, int output)
{

	switch (output) {
	case ZAUDIO_OP_SPKR:
		wm8750_write(sc, LOUT2VOL_REG, LOUT2VOL_LO2VU | LOUT2VOL_LO2ZC |
		    LOUT2VOL_SET_LOUT2VOL(sc->sc_volume[ZAUDIO_OP_SPKR].left >> 1));
		wm8750_write(sc, ROUT2VOL_REG, ROUT2VOL_RO2VU | ROUT2VOL_RO2ZC |
		    ROUT2VOL_SET_ROUT2VOL(sc->sc_volume[ZAUDIO_OP_SPKR].left >> 1));
		break;

	case ZAUDIO_OP_HP:
		wm8750_write(sc, LOUT1VOL_REG, LOUT1VOL_LO1VU | LOUT1VOL_LO1ZC |
		    LOUT1VOL_SET_LOUT1VOL(sc->sc_volume[ZAUDIO_OP_HP].left >> 1));
		wm8750_write(sc, ROUT1VOL_REG, ROUT1VOL_RO1VU | ROUT1VOL_RO1ZC |
		    ROUT1VOL_SET_ROUT1VOL(sc->sc_volume[ZAUDIO_OP_HP].right >> 1));
		break;

	case ZAUDIO_OP_MIC:
		wm8750_write(sc, LINVOL_REG, LINVOL_LIVU |
		    LINVOL_SET_LINVOL(sc->sc_volume[ZAUDIO_OP_MIC].left >> 2));
		wm8750_write(sc, RINVOL_REG, RINVOL_RIVU |
		    RINVOL_SET_RINVOL(sc->sc_volume[ZAUDIO_OP_MIC].left >> 2));
		break;
	}
}

static void
zaudio_update_mutes(struct zaudio_softc *sc, int mask)
{
	uint16_t val;

	/* playback */
	if (mask & 1) {
		val = PWRMGMT2_DACL | PWRMGMT2_DACR;
		if (sc->sc_unmute[ZAUDIO_OP_SPKR])
			val |= PWRMGMT2_LOUT2 | PWRMGMT2_ROUT2;
		if (sc->sc_unmute[ZAUDIO_OP_HP])
			val |= PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1;
		wm8750_write(sc, PWRMGMT2_REG, val);
		scoop_set_headphone(sc->sc_unmute[ZAUDIO_OP_HP]);
	}

	/* record */
	if (mask & 2) {
		val = PWRMGMT1_SET_VMIDSEL(1) | PWRMGMT1_VREF;
		if (sc->sc_unmute[ZAUDIO_OP_MIC]) {
			val |= PWRMGMT1_AINL | PWRMGMT1_AINR
			       | PWRMGMT1_ADCL | PWRMGMT1_ADCR | PWRMGMT1_MICB;
		}
		wm8750_write(sc, PWRMGMT1_REG, val);
		scoop_set_mic_bias(sc->sc_unmute[ZAUDIO_OP_MIC]);
	}
}

static void
zaudio_play_setup(struct zaudio_softc *sc)
{
	int i;

	pxa2x0_i2c_open(&sc->sc_i2c);

	/* Program the codec with playback settings */
	for (i = 0; playback_regs[i][0] != 0xffff; i++) {
		wm8750_write(sc, playback_regs[i][0], playback_regs[i][1]);
	}
	zaudio_update_mutes(sc, 1);

	pxa2x0_i2c_close(&sc->sc_i2c);
}

/*static*/ void
zaudio_record_setup(struct zaudio_softc *sc)
{
	int i;

	pxa2x0_i2c_open(&sc->sc_i2c);

	/* Program the codec with playback settings */
	for (i = 0; record_regs[i][0] != 0xffff; i++) {
		wm8750_write(sc, record_regs[i][0], record_regs[i][1]);
	}

	zaudio_update_mutes(sc, 2);

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
		i = auconv_set_converter(zaudio_formats, zaudio_nformats,
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
	if (!sc->sc_recording)
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
	if (!sc->sc_playing)
		zaudio_standby(sc);
	sc->sc_recording = 0;

	return rv;
}

static int
zaudio_getdev(void *hdl, struct audio_device *ret)
{

	*ret = wm8750_device;
	return 0;
}

#define ZAUDIO_SPKR_LVL		0
#define ZAUDIO_SPKR_MUTE	1
#define ZAUDIO_HP_LVL		2
#define ZAUDIO_HP_MUTE		3
#define ZAUDIO_MIC_LVL		4
#define ZAUDIO_MIC_MUTE		5
#define ZAUDIO_RECORD_SOURCE	6
#define ZAUDIO_OUTPUT_CLASS	7
#define ZAUDIO_INPUT_CLASS	8
#define ZAUDIO_RECORD_CLASS	9

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
		zaudio_update_mutes(sc, 1);
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
		zaudio_update_mutes(sc, 1);
		error = 0;
		break;

	case ZAUDIO_MIC_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			sc->sc_volume[ZAUDIO_OP_MIC].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		else
			break;
		zaudio_update_volume(sc, ZAUDIO_OP_MIC);
		error = 0;
		break;

	case ZAUDIO_MIC_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		sc->sc_unmute[ZAUDIO_OP_MIC] = mc->un.ord ? 1 : 0;
		zaudio_update_mutes(sc, 2);
		error = 0;
		break;

	case ZAUDIO_RECORD_SOURCE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		if (mc->un.ord != 0)
			break;
		/* MIC only */
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

	case ZAUDIO_MIC_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_volume[ZAUDIO_OP_MIC].left;
		else
			break;
		error = 0;
		break;

	case ZAUDIO_MIC_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = sc->sc_unmute[ZAUDIO_OP_MIC] ? 1 : 0;
		error = 0;
		break;

	case ZAUDIO_RECORD_SOURCE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = 0; /* MIC only */
		error = 0;
		break;
	}

	return error;
}

/*ARGSUSED*/
static int
zaudio_query_devinfo(void *hdl, struct mixer_devinfo *di)
{

	switch (di->index) {
	case ZAUDIO_SPKR_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = ZAUDIO_SPKR_MUTE;
		strlcpy(di->label.name, AudioNspeaker, sizeof(di->label.name));
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

	case ZAUDIO_MIC_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = ZAUDIO_INPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = ZAUDIO_MIC_MUTE;
		strlcpy(di->label.name, AudioNmicrophone,
		    sizeof(di->label.name));
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof(di->un.v.units.name));
		di->un.v.num_channels = 1;
		break;

	case ZAUDIO_MIC_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = ZAUDIO_INPUT_CLASS;
		di->prev = ZAUDIO_MIC_LVL;
		di->next = AUDIO_MIXER_LAST;
		goto mute;

	case ZAUDIO_RECORD_SOURCE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = ZAUDIO_RECORD_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioNsource, sizeof(di->label.name));
		di->un.e.num_mem = 1;
		strlcpy(di->un.e.member[0].label.name, AudioNmicrophone,
		    sizeof(di->un.e.member[0].label.name));
		di->un.e.member[0].ord = 0;
		break;

	case ZAUDIO_OUTPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = ZAUDIO_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCoutputs, sizeof(di->label.name));
		break;

	case ZAUDIO_INPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = ZAUDIO_INPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCinputs, sizeof(di->label.name));
		break;

	case ZAUDIO_RECORD_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = ZAUDIO_RECORD_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCinputs, sizeof(di->label.name));
		break;

	default:
		return ENXIO;
	}

	return 0;
}

static void *
zaudio_allocm(void *hdl, int direction, size_t size, struct malloc_type *type,
    int flags)
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

	return AUDIO_PROP_MMAP|AUDIO_PROP_INDEPENDENT;
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
		if (!sc->sc_recording)
			zaudio_standby(sc);
		sc->sc_playing = 0;
	}
	return rv;
}

static int
zaudio_start_input(void *hdl, void *block, int bsize, void (*intr)(void *),
    void *intrarg)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	/* Power up codec if we are not already recording. */
	if (!sc->sc_recording) {
		sc->sc_recording = 1;
		zaudio_record_setup(sc);
	}

	/* Start DMA via I2S */
	rv = pxa2x0_i2s_start_input(&sc->sc_i2s, block, bsize, intr, intrarg);
	if (rv) {
		if (!sc->sc_playing)
			zaudio_standby(sc);
		sc->sc_recording = 0;
	}
	return rv;
}
