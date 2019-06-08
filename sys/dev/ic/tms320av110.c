/*	$NetBSD: tms320av110.c,v 1.27 2019/06/08 08:02:38 isaki Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

/*
 * Machine independent part of TMS320AV110 driver.
 *
 * Currently, only minimum support for audio output. For audio/video
 * synchronization, more is needed.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tms320av110.c,v 1.27 2019/06/08 08:02:38 isaki Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <dev/ic/tms320av110reg.h>
#include <dev/ic/tms320av110var.h>

#include <sys/bus.h>

void tav_close(void *);
int tav_query_format(void *, audio_format_query_t *);
int tav_set_format(void *, int,
    const audio_params_t *, const audio_params_t *,
    audio_filter_reg_t *, audio_filter_reg_t *);
int tav_round_blocksize(void *, int, int, const audio_params_t *);
int tav_start_output(void *, void *, int, void (*)(void *), void *);
int tav_start_input(void *, void *, int, void (*)(void *), void *);
int tav_halt_output(void *);
int tav_halt_input(void *);
int tav_speaker_ctl(void *, int);
int tav_getdev(void *, struct audio_device *);
int tav_set_port(void *, mixer_ctrl_t *);
int tav_get_port(void *, mixer_ctrl_t *);
int tav_query_devinfo(void *, mixer_devinfo_t *);
int tav_get_props(void *);
void tav_get_locks(void *, kmutex_t **, kmutex_t **);

const struct audio_hw_if tav_audio_if = {
	.close			= tav_close,
	.query_format		= tav_query_format,
	.set_format		= tav_set_format,
	.round_blocksize	= tav_round_blocksize,
	.start_output		= tav_start_output,
	.start_input		= tav_start_input,
	.halt_output		= tav_halt_output,
	.halt_input		= tav_halt_input,
	.speaker_ctl		= tav_speaker_ctl,	/* optional */
	.getdev			= tav_getdev,
	.set_port		= tav_set_port,
	.get_port		= tav_get_port,
	.query_devinfo		= tav_query_devinfo,
	.get_props		= tav_get_props,
	.get_locks		= tav_get_locks,
};

/*
 * XXX: The frequency might depend on the specific board.
 * should be handled by the backend.
 */
#define TAV_FORMAT(prio, enc, prec) \
	{ \
		.priority	= (prio), \
		.mode		= AUMODE_PLAY, \
		.encoding	= (enc), \
		.validbits	= (prec), \
		.precision	= (prec), \
		.channels	= 2, \
		.channel_mask	= AUFMT_STEREO, \
		.frequency_type	= 1, \
		.frequency	= { 44100 }, \
	}
const struct audio_format tav_formats[] = {
	TAV_FORMAT(-1, AUDIO_ENCODING_MPEG_L2_STREAM, 1),
	TAV_FORMAT(-1, AUDIO_ENCODING_MPEG_L2_PACKETS, 1),
	TAV_FORMAT(-1, AUDIO_ENCODING_MPEG_L2_SYSTEM, 1),
	TAV_FORMAT( 0, AUDIO_ENCODING_SLINEAR_BE, 16),
};
#define TAV_NFORMATS __arraycount(tav_formats)

void
tms320av110_attach_mi(struct tav_softc *sc)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	tav_write_byte(iot, ioh, TAV_RESET, 1);
	while (tav_read_byte(iot, ioh, TAV_RESET))
		delay(250);

	tav_write_byte(iot, ioh, TAV_PCM_ORD, sc->sc_pcm_ord);
	tav_write_byte(iot, ioh, TAV_PCM_18, sc->sc_pcm_18);
	tav_write_byte(iot, ioh, TAV_DIF, sc->sc_dif);
	tav_write_byte(iot, ioh, TAV_PCM_DIV, sc->sc_pcm_div);

	printf(": chip rev. %d, %d bytes buffer\n",
	    tav_read_byte(iot, ioh, TAV_VERSION),
	    TAV_DRAM_SIZE(tav_read_byte(iot, ioh, TAV_DRAM_EXT)));

	tav_write_byte(iot, ioh, TAV_AUD_ID_EN, 0);
	tav_write_byte(iot, ioh, TAV_SKIP, 0);
	tav_write_byte(iot, ioh, TAV_REPEAT, 0);
	tav_write_byte(iot, ioh, TAV_MUTE, 0);
	tav_write_byte(iot, ioh, TAV_PLAY, 1);
	tav_write_byte(iot, ioh, TAV_SYNC_ECM, 0);
	tav_write_byte(iot, ioh, TAV_CRC_ECM, 0);
	tav_write_byte(iot, ioh, TAV_ATTEN_L, 0);
	tav_write_byte(iot, ioh, TAV_ATTEN_R, 0);
	tav_write_short(iot, ioh, TAV_FREE_FORM, 0);
	tav_write_byte(iot, ioh, TAV_SIN_EN, 0);
	tav_write_byte(iot, ioh, TAV_SYNC_ECM, TAV_ECM_REPEAT);
	tav_write_byte(iot, ioh, TAV_CRC_ECM, TAV_ECM_REPEAT);

	sc->sc_active = 0;

	audio_attach_mi(&tav_audio_if, sc, sc->sc_dev);
}

int
tms320av110_intr(void *p)
{
	struct tav_softc *sc;
	uint16_t intlist;

	sc = p;

	mutex_spin_enter(&sc->sc_intr_lock);

	intlist = tav_read_short(sc->sc_iot, sc->sc_ioh, TAV_INTR)
	    /* & tav_read_short(sc->sc_iot, sc->sc_ioh, TAV_INTR_EN)*/;

	if (!intlist) {
		mutex_spin_exit(&sc->sc_intr_lock);
		return 0;
	}

	/* ack now, so that we don't miss later interrupts */
	if (sc->sc_intack)
		(sc->sc_intack)(sc);

	if (intlist & TAV_INTR_LOWWATER) {
		(*sc->sc_intr)(sc->sc_intrarg);
	}

	mutex_spin_exit(&sc->sc_intr_lock);

	return 1;
}

void
tav_close(void *hdl)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	/* re"start" chip, also clears interrupts and interrupt enable */
	tav_write_short(iot, ioh, TAV_INTR_EN, 0);
	if (sc->sc_intack)
		(*sc->sc_intack)(sc);
}

int
tav_query_format(void *hdl, audio_format_query_t *afp)
{

	return audio_query_format(tav_formats, TAV_NFORMATS, afp);
}

int
tav_start_input(void *hdl, void *block, int bsize,
    void (*intr)(void *), void *intrarg)
{

	return ENOTTY;
}

int
tav_halt_input(void *hdl)
{

	return ENOTTY;
}

int
tav_start_output(void *hdl, void *block, int bsize,
    void (*intr)(void *), void *intrarg)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint8_t *ptr;
	int count;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	ptr = block;
	count = bsize;

	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;

	if (sc->sc_active == 0) {
		tav_write_byte(iot, ioh, TAV_PLAY, 1);
		tav_write_byte(iot, ioh, TAV_MUTE, 0);
		sc->sc_active = 1;
	}

	bus_space_write_multi_1(iot, ioh, TAV_DATAIN, ptr, count);
	tav_write_short(iot, ioh, TAV_INTR_EN, TAV_INTR_LOWWATER);

	return 0;
}

int
tav_halt_output(void *hdl)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	tav_write_byte(iot, ioh, TAV_PLAY, 0);
	sc->sc_active = 0;

	return 0;
}

int
tav_getdev(void *hdl, struct audio_device *ret)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	strlcpy(ret->name, "tms320av110", sizeof(ret->name));
	/* guaranteed to be <= 4 in length */
	snprintf(ret->version, sizeof(ret->version), "%u",
	    tav_read_byte(iot, ioh, TAV_VERSION));
	strlcpy(ret->config, device_xname(sc->sc_dev), sizeof(ret->config));

	return 0;
}

int
tav_round_blocksize(void *hdl, int size, int mode, const audio_params_t *param)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int maxhalf;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	maxhalf = TAV_DRAM_HSIZE(tav_read_byte(iot, ioh, TAV_DRAM_EXT));
	if (size > maxhalf)
		size = maxhalf;

	/* XXX should round to 128 bytes limits for audio bypass */
	size &= ~3;

	tav_write_short(iot, ioh, TAV_BALE_LIM, size/8);

	/* the buffer limits are in units of 4 bytes */
	return (size);
}

int
tav_get_props(void *hdl)
{

	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE;
}

void
tav_get_locks(void *hdl, kmutex_t **intr, kmutex_t **thread)
{
	struct tav_softc *sc;

	sc = hdl;
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

int
tav_set_format(void *hdl, int setmode,
    const audio_params_t *p, const audio_params_t *r,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	KASSERT((setmode & AUMODE_PLAY));
	KASSERT(p->encoding == AUDIO_ENCODING_SLINEAR_BE);

	bus_space_write_1(iot, ioh, TAV_STR_SEL, TAV_STR_SEL_AUDIO_BYPASS);

	tav_write_byte(iot, ioh, TAV_RESTART, 1);
	do {
		delay(10);
	} while (tav_read_byte(iot, ioh, TAV_RESTART));

	return 0;
}

int
tav_set_port(void *hdl, mixer_ctrl_t *mc)
{

	/* dummy */
	return 0;
}

int
tav_get_port(void *hdl, mixer_ctrl_t *mc)
{

	/* dummy */
	return 0;
}

int
tav_query_devinfo(void *hdl, mixer_devinfo_t *di)
{
	return ENXIO;
}

int
tav_speaker_ctl(void *hdl, int value)
{
	struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	tav_write_byte(iot, ioh, TAV_MUTE, !value);

	return 0;
}
