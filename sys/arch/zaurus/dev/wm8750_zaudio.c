/*	$NetBSD: wm8750_zaudio.c,v 1.1 2014/09/23 14:49:46 nonaka Exp $	*/
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
 * Copyright (C) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TODO:
 *	- powerhooks (currently only works until first suspend)
 */

#include "opt_cputypes.h"
#include "opt_zaudio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wm8750_zaudio.c,v 1.1 2014/09/23 14:49:46 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/audioio.h>
#include <sys/mutex.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <dev/i2c/i2cvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>
#include <arm/xscale/pxa2x0_i2s.h>
#include <arm/xscale/pxa2x0_dmac.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/zaudiovar.h>
#include <zaurus/dev/wm8750reg.h>
#include <zaurus/dev/wm8750var.h>
#include <zaurus/dev/scoopvar.h>
#include <zaurus/dev/ioexpvar.h>

#define WM8750_ADDRESS  0x1B

/* GPIO pins */
#define GPIO_HP_IN_C3000	116

#define WM8750_OP_SPKR	0
#define WM8750_OP_HP	1
#define WM8750_OP_MIC	2
#define WM8750_OP_NUM	3

#define ZAUDIO_JACK_STATE_OUT	0
#define ZAUDIO_JACK_STATE_IN	1
#define ZAUDIO_JACK_STATE_INS	2
#define ZAUDIO_JACK_STATE_REM	3

static int	wm8750_finalize(device_t);
static bool	wm8750_suspend(device_t, const pmf_qual_t *);
static bool	wm8750_resume(device_t, const pmf_qual_t *);
static void	wm8750_volume_up(device_t);
static void	wm8750_volume_down(device_t);
static void	wm8750_volume_toggle(device_t);

static struct audio_device wm8750_device = {
	"WM8750",
	"1.0",
	"wm"
};

static const struct audio_format wm8750_formats[] = {
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
static const int wm8750_nformats = (int)__arraycount(wm8750_formats);

static void wm8750_init(struct zaudio_softc *);
static int wm8750_jack_intr(void *);
static void wm8750_jack(void *);
static void wm8750_standby(struct zaudio_softc *);
static void wm8750_update_volume(struct zaudio_softc *, int);
static void wm8750_update_mutes(struct zaudio_softc *, int);
static void wm8750_play_setup(struct zaudio_softc *);
/*static*/ void wm8750_record_setup(struct zaudio_softc *);
static int wm8750_query_encoding(void *, struct audio_encoding *);
static int wm8750_set_params(void *, int, int, audio_params_t *,
    audio_params_t *, stream_filter_list_t *, stream_filter_list_t *);
static int wm8750_start_output(void *, void *, int, void (*)(void *), void *);
static int wm8750_start_input(void *, void *, int, void (*)(void *), void *);
static int wm8750_halt_output(void *);
static int wm8750_halt_input(void *);
static int wm8750_getdev(void *, struct audio_device *);
static int wm8750_set_port(void *, struct mixer_ctrl *);
static int wm8750_get_port(void *, struct mixer_ctrl *);
static int wm8750_query_devinfo(void *, struct mixer_devinfo *);

static struct audio_hw_if wm8750_hw_if = {
	.open			= zaudio_open,
	.close			= zaudio_close,
	.drain			= NULL,
	.query_encoding		= wm8750_query_encoding,
	.set_params		= wm8750_set_params,
	.round_blocksize	= zaudio_round_blocksize,
	.commit_settings	= NULL,
	.init_output		= NULL,
	.init_input		= NULL,
	.start_output		= wm8750_start_output,
	.start_input		= wm8750_start_input,
	.halt_output		= wm8750_halt_output,
	.halt_input		= wm8750_halt_input,
	.speaker_ctl		= NULL,
	.getdev			= wm8750_getdev,
	.setfd			= NULL,
	.set_port		= wm8750_set_port,
	.get_port		= wm8750_get_port,
	.query_devinfo		= wm8750_query_devinfo,
	.allocm			= zaudio_allocm,
	.freem			= zaudio_freem,
	.round_buffersize	= zaudio_round_buffersize,
	.mappage		= zaudio_mappage,
	.get_props		= zaudio_get_props,
	.trigger_output		= NULL,
	.trigger_input		= NULL,
	.dev_ioctl		= NULL,
	.get_locks		= zaudio_get_locks,
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

static __inline int
wm8750_write(struct zaudio_softc *sc, int reg, int val)
{
	uint16_t tmp;
	uint8_t cmd;
	uint8_t data;

	tmp = (reg << 9) | (val & 0x1ff);
	cmd = tmp >> 8;
	data = tmp;
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, WM8750_ADDRESS,
	    &cmd, 1, &data, 1, 0);
}

int
wm8750_match(device_t parent, cfdata_t cf, struct i2c_attach_args *ia)
{

	if (ZAURUS_ISC860)
		return 0;

	if (ia->ia_name) {
		/* direct config - check name */
		if (strcmp(ia->ia_name, "zaudio") == 0)
			return 1;
	} else {
		/* indirect config - check typical address */
		if (ia->ia_addr == WM8750_ADDRESS)
			return 1;
	}
	return 0;
}

void
wm8750_attach(device_t parent, device_t self, struct i2c_attach_args *ia)
{
	struct zaudio_softc *sc = device_private(self);
	int error;

	aprint_normal(": I2S, WM8750 Audio\n");
	aprint_naive("\n");

	/* Check for an I2C response from the wm8750 */
	iic_acquire_bus(sc->sc_i2c, 0);
	error = wm8750_write(sc, RESET_REG, 0);
	iic_release_bus(sc->sc_i2c, 0);
	if (error) {
		aprint_error_dev(self, "codec failed to respond\n");
		goto fail_i2c;
	}
	delay(100);

	/* Allocate memory for volume & mute operations */
	sc->sc_volume = kmem_zalloc(sizeof(*sc->sc_volume) * WM8750_OP_NUM,
	    KM_SLEEP);
	sc->sc_unmute = kmem_zalloc(sizeof(*sc->sc_unmute) * WM8750_OP_NUM,
	    KM_SLEEP);
	sc->sc_unmute_toggle = kmem_zalloc(
	    sizeof(*sc->sc_unmute_toggle) * WM8750_OP_NUM, KM_SLEEP);

	/* Speaker on, headphones off by default. */
	sc->sc_volume[WM8750_OP_SPKR].left = 180;
	sc->sc_jack = FALSE;
	UNMUTE(sc, WM8750_OP_SPKR, 1);
	sc->sc_volume[WM8750_OP_HP].left = 180;
	sc->sc_volume[WM8750_OP_HP].right = 180;
	UNMUTE(sc, WM8750_OP_HP, 0);
	sc->sc_volume[WM8750_OP_MIC].left = 180;
	UNMUTE(sc, WM8750_OP_MIC, 0);

	/* Configure headphone jack state change handling. */
	callout_setfunc(&sc->sc_to, wm8750_jack, sc);
	pxa2x0_gpio_set_function(GPIO_HP_IN_C3000, GPIO_IN);
	(void) pxa2x0_gpio_intr_establish(GPIO_HP_IN_C3000, IST_EDGE_BOTH,
	    IPL_BIO, wm8750_jack_intr, sc);

	/* wm8750_init() implicitly depends on ioexp or scoop */
	config_finalize_register(self, wm8750_finalize);

	audio_attach_mi(&wm8750_hw_if, sc, self);

	if (!pmf_device_register(self, wm8750_suspend, wm8750_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	if (!pmf_event_register(self, PMFE_AUDIO_VOLUME_UP,
	    wm8750_volume_up, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_AUDIO_VOLUME_DOWN,
	    wm8750_volume_down, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_AUDIO_VOLUME_TOGGLE,
	    wm8750_volume_toggle, true))
		aprint_error_dev(self, "couldn't register event handler\n");

	return;

fail_i2c:
	pxa2x0_i2s_detach_sub(&sc->sc_i2s);
}

static int
wm8750_finalize(device_t dv)
{
	struct zaudio_softc *sc = device_private(dv);

	wm8750_init(sc);
	return 0;
}

static bool
wm8750_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct zaudio_softc *sc = device_private(dv);

	callout_stop(&sc->sc_to);
	wm8750_standby(sc);

	return true;
}

static bool
wm8750_resume(device_t dv, const pmf_qual_t *qual)
{
	struct zaudio_softc *sc = device_private(dv);

	pxa2x0_i2s_init(&sc->sc_i2s);
	wm8750_init(sc);

	return true;
}

static __inline uint8_t
vol_sadd(int vol, int stride)
{

	vol += stride;
	if (vol > 255)
		return 255;
	return (uint8_t)vol;
}

#ifndef	ZAUDIO_VOLUME_STRIDE
#define	ZAUDIO_VOLUME_STRIDE	8
#endif

static void
wm8750_volume_up(device_t dv)
{
	struct zaudio_softc *sc = device_private(dv);
	int s;

	s = splbio();
	iic_acquire_bus(sc->sc_i2c, 0);

	sc->sc_volume[WM8750_OP_SPKR].left =
	    vol_sadd(sc->sc_volume[WM8750_OP_SPKR].left, ZAUDIO_VOLUME_STRIDE);
	sc->sc_volume[WM8750_OP_HP].left =
	    vol_sadd(sc->sc_volume[WM8750_OP_HP].left, ZAUDIO_VOLUME_STRIDE);
	sc->sc_volume[WM8750_OP_HP].right =
	    vol_sadd(sc->sc_volume[WM8750_OP_HP].right, ZAUDIO_VOLUME_STRIDE);

	wm8750_update_volume(sc, WM8750_OP_SPKR);
	wm8750_update_volume(sc, WM8750_OP_HP);

	iic_release_bus(sc->sc_i2c, 0);
	splx(s);
}

static __inline uint8_t
vol_ssub(int vol, int stride)
{

	vol -= stride;
	if (vol < 0)
		return 0;
	return (uint8_t)vol;
}

static void
wm8750_volume_down(device_t dv)
{
	struct zaudio_softc *sc = device_private(dv);
	int s;

	s = splbio();
	iic_acquire_bus(sc->sc_i2c, 0);

	sc->sc_volume[WM8750_OP_SPKR].left =
	    vol_ssub(sc->sc_volume[WM8750_OP_SPKR].left, ZAUDIO_VOLUME_STRIDE);
	sc->sc_volume[WM8750_OP_HP].left =
	    vol_ssub(sc->sc_volume[WM8750_OP_HP].left, ZAUDIO_VOLUME_STRIDE);
	sc->sc_volume[WM8750_OP_HP].right =
	    vol_ssub(sc->sc_volume[WM8750_OP_HP].right, ZAUDIO_VOLUME_STRIDE);

	wm8750_update_volume(sc, WM8750_OP_SPKR);
	wm8750_update_volume(sc, WM8750_OP_HP);

	iic_release_bus(sc->sc_i2c, 0);
	splx(s);
}

static void
wm8750_volume_toggle(device_t dv)
{
	struct zaudio_softc *sc = device_private(dv);
	int s;

	s = splbio();
	iic_acquire_bus(sc->sc_i2c, 0);

	if (!sc->sc_unmute[WM8750_OP_SPKR] && !sc->sc_unmute[WM8750_OP_HP]) {
		sc->sc_unmute[WM8750_OP_SPKR] =
		    sc->sc_unmute_toggle[WM8750_OP_SPKR];
		sc->sc_unmute[WM8750_OP_HP] =
		    sc->sc_unmute_toggle[WM8750_OP_HP];
	} else {
		sc->sc_unmute[WM8750_OP_SPKR] = 0;
		sc->sc_unmute[WM8750_OP_HP] = 0;
	}
	wm8750_update_mutes(sc, 1);

	iic_release_bus(sc->sc_i2c, 0);
	splx(s);
}

static void
wm8750_init(struct zaudio_softc *sc)
{

	iic_acquire_bus(sc->sc_i2c, 0);

	/* Reset the codec */
	wm8750_write(sc, RESET_REG, 0);
	delay(100);

	/* Switch to standby power only */
	wm8750_write(sc, PWRMGMT1_REG, PWRMGMT1_SET_VMIDSEL(2));
	wm8750_write(sc, PWRMGMT2_REG, 0);

	/* Configure digital interface for I2S */
	wm8750_write(sc, AUDINT_REG, AUDINT_SET_FORMAT(2));

	/* Initialise volume levels */
	wm8750_update_volume(sc, WM8750_OP_SPKR);
	wm8750_update_volume(sc, WM8750_OP_HP);
	wm8750_update_volume(sc, WM8750_OP_MIC);

	scoop_set_headphone(0);
	if (ZAURUS_ISC1000)
		ioexp_set_mic_bias(0);
	else
		scoop_set_mic_bias(0);

	iic_release_bus(sc->sc_i2c, 0);

	/* Assume that the jack state has changed. */ 
	wm8750_jack(sc);
}

static int
wm8750_jack_intr(void *v)
{
	struct zaudio_softc *sc = v;

	if (!callout_active(&sc->sc_to))
		wm8750_jack(sc);

	return 1;
}

static void
wm8750_jack(void *v)
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
				sc->sc_jack = TRUE;
				UNMUTE(sc, WM8750_OP_SPKR, 0);
				UNMUTE(sc, WM8750_OP_HP, 1);
				UNMUTE(sc, WM8750_OP_MIC, 1);
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
				sc->sc_jack = FALSE;
				UNMUTE(sc, WM8750_OP_SPKR, 1);
				UNMUTE(sc, WM8750_OP_HP, 0);
				UNMUTE(sc, WM8750_OP_MIC, 0);
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
		iic_acquire_bus(sc->sc_i2c, 0);
		if (sc->sc_playing)
			wm8750_update_mutes(sc, 1);
		if (sc->sc_recording)
			wm8750_update_mutes(sc, 2);
		iic_release_bus(sc->sc_i2c, 0);
	}
}

static void
wm8750_standby(struct zaudio_softc *sc)
{

	iic_acquire_bus(sc->sc_i2c, 0);

	/* Switch codec to standby power only */
	wm8750_write(sc, PWRMGMT1_REG, PWRMGMT1_SET_VMIDSEL(2));
	wm8750_write(sc, PWRMGMT2_REG, 0);

	scoop_set_headphone(0);
	if (ZAURUS_ISC1000)
		ioexp_set_mic_bias(0);
	else
		scoop_set_mic_bias(0);

	iic_release_bus(sc->sc_i2c, 0);
}

static void
wm8750_update_volume(struct zaudio_softc *sc, int output)
{

	switch (output) {
	case WM8750_OP_SPKR:
		wm8750_write(sc, LOUT2VOL_REG, LOUT2VOL_LO2VU | LOUT2VOL_LO2ZC |
		    LOUT2VOL_SET_LOUT2VOL(sc->sc_volume[WM8750_OP_SPKR].left >> 1));
		wm8750_write(sc, ROUT2VOL_REG, ROUT2VOL_RO2VU | ROUT2VOL_RO2ZC |
		    ROUT2VOL_SET_ROUT2VOL(sc->sc_volume[WM8750_OP_SPKR].left >> 1));
		break;

	case WM8750_OP_HP:
		wm8750_write(sc, LOUT1VOL_REG, LOUT1VOL_LO1VU | LOUT1VOL_LO1ZC |
		    LOUT1VOL_SET_LOUT1VOL(sc->sc_volume[WM8750_OP_HP].left >> 1));
		wm8750_write(sc, ROUT1VOL_REG, ROUT1VOL_RO1VU | ROUT1VOL_RO1ZC |
		    ROUT1VOL_SET_ROUT1VOL(sc->sc_volume[WM8750_OP_HP].right >> 1));
		break;

	case WM8750_OP_MIC:
		wm8750_write(sc, LINVOL_REG, LINVOL_LIVU |
		    LINVOL_SET_LINVOL(sc->sc_volume[WM8750_OP_MIC].left >> 2));
		wm8750_write(sc, RINVOL_REG, RINVOL_RIVU |
		    RINVOL_SET_RINVOL(sc->sc_volume[WM8750_OP_MIC].left >> 2));
		break;
	}
}

static void
wm8750_update_mutes(struct zaudio_softc *sc, int mask)
{
	uint16_t val;

	/* playback */
	if (mask & 1) {
		val = PWRMGMT2_DACL | PWRMGMT2_DACR;
		if (sc->sc_unmute[WM8750_OP_SPKR])
			val |= PWRMGMT2_LOUT2 | PWRMGMT2_ROUT2;
		if (sc->sc_unmute[WM8750_OP_HP])
			val |= PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1;
		wm8750_write(sc, PWRMGMT2_REG, val);
		scoop_set_headphone(sc->sc_unmute[WM8750_OP_HP]);
	}

	/* record */
	if (mask & 2) {
		val = PWRMGMT1_SET_VMIDSEL(1) | PWRMGMT1_VREF;
		if (sc->sc_unmute[WM8750_OP_MIC]) {
			val |= PWRMGMT1_AINL | PWRMGMT1_AINR
			       | PWRMGMT1_ADCL | PWRMGMT1_ADCR | PWRMGMT1_MICB;
		}
		wm8750_write(sc, PWRMGMT1_REG, val);
		if (ZAURUS_ISC1000)
			ioexp_set_mic_bias(sc->sc_unmute[WM8750_OP_MIC]);
		else
			scoop_set_mic_bias(sc->sc_unmute[WM8750_OP_MIC]);
	}
}

static void
wm8750_play_setup(struct zaudio_softc *sc)
{
	int i;

	iic_acquire_bus(sc->sc_i2c, 0);

	/* Program the codec with playback settings */
	for (i = 0; playback_regs[i][0] != 0xffff; i++) {
		wm8750_write(sc, playback_regs[i][0], playback_regs[i][1]);
	}
	wm8750_update_mutes(sc, 1);

	iic_release_bus(sc->sc_i2c, 0);
}

/*static*/ void
wm8750_record_setup(struct zaudio_softc *sc)
{
	int i;

	iic_acquire_bus(sc->sc_i2c, 0);

	/* Program the codec with playback settings */
	for (i = 0; record_regs[i][0] != 0xffff; i++) {
		wm8750_write(sc, record_regs[i][0], record_regs[i][1]);
	}
	wm8750_update_mutes(sc, 2);

	iic_release_bus(sc->sc_i2c, 0);
}

static int
wm8750_query_encoding(void *hdl, struct audio_encoding *aep)
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
wm8750_set_params(void *hdl, int setmode, int usemode, audio_params_t *play,
    audio_params_t *rec, stream_filter_list_t *pfil, stream_filter_list_t *rfil)
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
		i = auconv_set_converter(wm8750_formats, wm8750_nformats,
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
wm8750_halt_output(void *hdl)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	rv = pxa2x0_i2s_halt_output(&sc->sc_i2s);
	if (!sc->sc_recording)
		wm8750_standby(sc);
	sc->sc_playing = 0;

	return rv;
}

static int
wm8750_halt_input(void *hdl)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	rv = pxa2x0_i2s_halt_input(&sc->sc_i2s);
	if (!sc->sc_playing)
		wm8750_standby(sc);
	sc->sc_recording = 0;

	return rv;
}

static int
wm8750_getdev(void *hdl, struct audio_device *ret)
{

	*ret = wm8750_device;
	return 0;
}

#define WM8750_SPKR_LVL		0
#define WM8750_SPKR_MUTE	1
#define WM8750_HP_LVL		2
#define WM8750_HP_MUTE		3
#define WM8750_MIC_LVL		4
#define WM8750_MIC_MUTE		5
#define WM8750_RECORD_SOURCE	6
#define WM8750_OUTPUT_CLASS	7
#define WM8750_INPUT_CLASS	8
#define WM8750_RECORD_CLASS	9

static int
wm8750_set_port(void *hdl, struct mixer_ctrl *mc)
{
	struct zaudio_softc *sc = hdl;
	int error = EINVAL;
	int s;

	s = splbio();
	iic_acquire_bus(sc->sc_i2c, 0);

	switch (mc->dev) {
	case WM8750_SPKR_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			sc->sc_volume[WM8750_OP_SPKR].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		else
			break;
		wm8750_update_volume(sc, WM8750_OP_SPKR);
		error = 0;
		break;

	case WM8750_SPKR_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		UNMUTE(sc, WM8750_OP_SPKR, mc->un.ord ? 1 : 0);
		wm8750_update_mutes(sc, 1);
		error = 0;
		break;

	case WM8750_HP_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1) {
			sc->sc_volume[WM8750_OP_HP].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
			sc->sc_volume[WM8750_OP_HP].right =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		} else if (mc->un.value.num_channels == 2) {
			sc->sc_volume[WM8750_OP_HP].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
			sc->sc_volume[WM8750_OP_HP].right =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		}
		else
			break;
		wm8750_update_volume(sc, WM8750_OP_HP);
		error = 0;
		break;

	case WM8750_HP_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		UNMUTE(sc, WM8750_OP_HP, mc->un.ord ? 1 : 0);
		wm8750_update_mutes(sc, 1);
		error = 0;
		break;

	case WM8750_MIC_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			sc->sc_volume[WM8750_OP_MIC].left =
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		else
			break;
		wm8750_update_volume(sc, WM8750_OP_MIC);
		error = 0;
		break;

	case WM8750_MIC_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		UNMUTE(sc, WM8750_OP_MIC, mc->un.ord ? 1 : 0);
		wm8750_update_mutes(sc, 2);
		error = 0;
		break;

	case WM8750_RECORD_SOURCE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		if (mc->un.ord != 0)
			break;
		/* MIC only */
		error = 0;
		break;
	}

	iic_release_bus(sc->sc_i2c, 0);
	splx(s);

	return error;
}

static int
wm8750_get_port(void *hdl, struct mixer_ctrl *mc)
{
	struct zaudio_softc *sc = hdl;
	int error = EINVAL;

	switch (mc->dev) {
	case WM8750_SPKR_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_volume[WM8750_OP_SPKR].left;
		else
			break;
		error = 0;
		break;

	case WM8750_SPKR_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = sc->sc_unmute[WM8750_OP_SPKR] ? 1 : 0;
		error = 0;
		break;

	case WM8750_HP_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_volume[WM8750_OP_HP].left;
		else if (mc->un.value.num_channels == 2) {
			mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    sc->sc_volume[WM8750_OP_HP].left;
			mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    sc->sc_volume[WM8750_OP_HP].right;
		}
		else
			break;
		error = 0;
		break;

	case WM8750_HP_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = sc->sc_unmute[WM8750_OP_HP] ? 1 : 0;
		error = 0;
		break;

	case WM8750_MIC_LVL:
		if (mc->type != AUDIO_MIXER_VALUE)
			break;
		if (mc->un.value.num_channels == 1)
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_volume[WM8750_OP_MIC].left;
		else
			break;
		error = 0;
		break;

	case WM8750_MIC_MUTE:
		if (mc->type != AUDIO_MIXER_ENUM)
			break;
		mc->un.ord = sc->sc_unmute[WM8750_OP_MIC] ? 1 : 0;
		error = 0;
		break;

	case WM8750_RECORD_SOURCE:
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
wm8750_query_devinfo(void *hdl, struct mixer_devinfo *di)
{

	switch (di->index) {
	case WM8750_SPKR_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = WM8750_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = WM8750_SPKR_MUTE;
		strlcpy(di->label.name, AudioNspeaker, sizeof(di->label.name));
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof(di->un.v.units.name));
		di->un.v.num_channels = 1;
		break;

	case WM8750_SPKR_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = WM8750_OUTPUT_CLASS;
		di->prev = WM8750_SPKR_LVL;
		di->next = AUDIO_MIXER_LAST;
		goto mute;

	case WM8750_HP_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = WM8750_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = WM8750_HP_MUTE;
		strlcpy(di->label.name, AudioNheadphone,
		    sizeof(di->label.name));
		di->un.v.num_channels = 1;
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof(di->un.v.units.name));
		break;

	case WM8750_HP_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = WM8750_OUTPUT_CLASS;
		di->prev = WM8750_HP_LVL;
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

	case WM8750_MIC_LVL:
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = WM8750_INPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = WM8750_MIC_MUTE;
		strlcpy(di->label.name, AudioNmicrophone,
		    sizeof(di->label.name));
		strlcpy(di->un.v.units.name, AudioNvolume,
		    sizeof(di->un.v.units.name));
		di->un.v.num_channels = 1;
		break;

	case WM8750_MIC_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = WM8750_INPUT_CLASS;
		di->prev = WM8750_MIC_LVL;
		di->next = AUDIO_MIXER_LAST;
		goto mute;

	case WM8750_RECORD_SOURCE:
		di->type = AUDIO_MIXER_ENUM;
		di->mixer_class = WM8750_RECORD_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioNsource, sizeof(di->label.name));
		di->un.e.num_mem = 1;
		strlcpy(di->un.e.member[0].label.name, AudioNmicrophone,
		    sizeof(di->un.e.member[0].label.name));
		di->un.e.member[0].ord = 0;
		break;

	case WM8750_OUTPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = WM8750_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCoutputs, sizeof(di->label.name));
		break;

	case WM8750_INPUT_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = WM8750_INPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCinputs, sizeof(di->label.name));
		break;

	case WM8750_RECORD_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = WM8750_RECORD_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		strlcpy(di->label.name, AudioCinputs, sizeof(di->label.name));
		break;

	default:
		return ENXIO;
	}

	return 0;
}

static int
wm8750_start_output(void *hdl, void *block, int bsize, void (*intr)(void *),
    void *intrarg)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	/* Power up codec if we are not already playing. */
	if (!sc->sc_playing) {
		sc->sc_playing = 1;
		wm8750_play_setup(sc);
	}

	/* Start DMA via I2S */
	rv = pxa2x0_i2s_start_output(&sc->sc_i2s, block, bsize, intr, intrarg);
	if (rv) {
		if (!sc->sc_recording)
			wm8750_standby(sc);
		sc->sc_playing = 0;
	}

	return rv;
}

static int
wm8750_start_input(void *hdl, void *block, int bsize, void (*intr)(void *),
    void *intrarg)
{
	struct zaudio_softc *sc = hdl;
	int rv;

	/* Power up codec if we are not already recording. */
	if (!sc->sc_recording) {
		sc->sc_recording = 1;
		wm8750_record_setup(sc);
	}

	/* Start DMA via I2S */
	rv = pxa2x0_i2s_start_input(&sc->sc_i2s, block, bsize, intr, intrarg);
	if (rv) {
		if (!sc->sc_playing)
			wm8750_standby(sc);
		sc->sc_recording = 0;
	}
	return rv;
}
