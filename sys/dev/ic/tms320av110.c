/*	$NetBSD: tms320av110.c,v 1.8.20.1 2001/10/08 20:11:03 nathanw Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/tms320av110reg.h>
#include <dev/ic/tms320av110var.h>

#include <machine/bus.h>

int tav_open __P((void*, int));
void tav_close __P((void *));
int tav_drain __P((void *));
int tav_query_encoding __P((void *, struct audio_encoding *));
int tav_set_params __P((void *, int, int, struct audio_params *, 
    struct audio_params *));
int tav_round_blocksize __P((void *, int));
int tav_init_output __P((void *, void *, int));
int tav_start_output __P((void *, void *, int, void (*)(void *), void *));
int tav_start_input __P((void *, void *, int, void (*)(void *), void *));
int tav_halt_output __P((void *));
int tav_halt_input __P((void *));
int tav_speaker_ctl __P((void *, int));
int tav_getdev __P((void *, struct audio_device *));
int tav_setfd __P((void *, int));
int tav_set_port __P((void *, mixer_ctrl_t *));
int tav_get_port __P((void *, mixer_ctrl_t *));
int tav_query_devinfo __P((void *, mixer_devinfo_t *));
int tav_get_props __P((void *));

struct audio_hw_if tav_audio_if = {
	tav_open,
	tav_close,
	0 /* tav_drain*/,		/* optional */
	tav_query_encoding,
	tav_set_params,
	tav_round_blocksize,
	0 /* commit_settings */,	/* optional */
	tav_init_output,		/* optional */
	0 /* tav_init_input */,		/* optional */
	tav_start_output,
	tav_start_input,
	tav_halt_output,
	tav_halt_input,
	tav_speaker_ctl,		/* optional */
	tav_getdev,
	0 /* setfd */,			/* optional */
	tav_set_port,
	tav_get_port,
	tav_query_devinfo,
	0 /* alloc */,			/* optional */
	0 /* free */,			/* optional */
	0 /* round_buffersize */,	/* optional */
	0 /* mappage */,		/* optional */
	tav_get_props,
	0 /* dev_ioctl */		/* optional */
};

void
tms320av110_attach_mi(sc)
	struct tav_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

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

	audio_attach_mi(&tav_audio_if, sc, &sc->sc_dev);
}

int
tms320av110_intr(p)
	void *p;
{
        struct tav_softc *sc = p;
	u_int16_t intlist;

	intlist = tav_read_short(sc->sc_iot, sc->sc_ioh, TAV_INTR)
	    /* & tav_read_short(sc->sc_iot, sc->sc_ioh, TAV_INTR_EN)*/;

	if (!intlist)
		return 0;

	/* ack now, so that we don't miss later interupts */
	if (sc->sc_intack)
		(sc->sc_intack)(sc);

	if (intlist & TAV_INTR_LOWWATER) {
		(*sc->sc_intr)(sc->sc_intrarg);
	}

	if (intlist & TAV_INTR_PCM_OUTPUT_UNDERFLOW) {
		 wakeup(sc);
	}

	return 1;
}

struct audio_encoding tav_encodings[] = {
	{0, AudioEmpeg_l2_stream, AUDIO_ENCODING_MPEG_L2_STREAM, 1, 0,},
	{1, AudioEmpeg_l2_packets, AUDIO_ENCODING_MPEG_L2_PACKETS, 1, 0,},
	{2, AudioEmpeg_l2_system, AUDIO_ENCODING_MPEG_L2_SYSTEM, 1, 0,},
	{3, AudioEslinear_be, AUDIO_ENCODING_SLINEAR_BE, 16, 0,},
};

int
tav_open(hdl, flags)
	void *hdl;
	int flags;
{
        struct tav_softc *sc;

	sc = hdl;

	/* dummy */
	return 0;
}

void
tav_close(hdl)
        void *hdl;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	/* re"start" chip, also clears interupts and interupt enable */
	tav_write_short(iot, ioh, TAV_INTR_EN, 0);
	if (sc->sc_intack)
		(*sc->sc_intack)(sc);
}

int
tav_drain(hdl)
        void *hdl;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int16_t mask;

        sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	/*
	 * tsleep waiting for underflow interupt.
	 */
	if (tav_read_short(iot, ioh, TAV_BUFF)) {
		mask = tav_read_short(iot, ioh, TAV_INTR_EN);
		tav_write_short(iot, ioh, TAV_INTR_EN,
		    mask|TAV_INTR_PCM_OUTPUT_UNDERFLOW);

		/* still more than zero? */
		if (tav_read_short(iot, ioh, TAV_BUFF))
			(void)tsleep(sc, PCATCH, "tavdrain", 32*hz);

		/* can be really that long for mpeg */

		mask = tav_read_short(iot, ioh, TAV_INTR_EN);
		tav_write_short(iot, ioh, TAV_INTR_EN,
		    mask & ~TAV_INTR_PCM_OUTPUT_UNDERFLOW);
	}
	
	return 0;
}

int
tav_query_encoding(hdl, ae)
        void *hdl;
	struct audio_encoding *ae;
{
        struct tav_softc *sc;

	sc = hdl;
	if (ae->index >= sizeof(tav_encodings)/sizeof(*ae))
		return EINVAL;

	*ae = tav_encodings[ae->index];

	return 0;
}

int
tav_start_input(hdl, block, bsize, intr, intrarg)
	void *hdl;
	void *block;
	int bsize;
	void (*intr) __P((void *));
	void *intrarg;
{
	return ENOTTY;
}

int
tav_halt_input(hdl)
	void *hdl;
{
	return ENOTTY;
}

int
tav_start_output(hdl, block, bsize, intr, intrarg)
	void *hdl;
	void *block;
	int bsize;
	void (*intr) __P((void *));
	void *intrarg;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t *ptr;
	int count;


        sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	ptr = block;
	count = bsize;
	
	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;

	bus_space_write_multi_1(iot, ioh, TAV_DATAIN, ptr, count);
	tav_write_short(iot, ioh, TAV_INTR_EN, TAV_INTR_LOWWATER);

	return 0;
}

int
tav_init_output(hdl, buffer, size)
	void *hdl;
	void *buffer;
	int size;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

        sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	tav_write_byte(iot, ioh, TAV_PLAY, 1);
	tav_write_byte(iot, ioh, TAV_MUTE, 0);

	return 0;
}

int
tav_halt_output(hdl)
	void *hdl;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

        sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	tav_write_byte(iot, ioh, TAV_PLAY, 0);

	return 0;
}

int
tav_getdev(hdl, ret)
	void *hdl;
	struct audio_device *ret;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

        sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	strncpy(ret->name, "tms320av110", MAX_AUDIO_DEV_LEN);
	sprintf(ret->version, "%u", /* guaranteed to be <= 4 in length */
	    tav_read_byte(iot, ioh, TAV_VERSION));
	strncpy(ret->config, sc->sc_dev.dv_xname, MAX_AUDIO_DEV_LEN);

	return 0;
}

int
tav_round_blocksize(hdl, size)
	void *hdl;
	int size;
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
tav_get_props(hdl)
        void *hdl;
{
	return 0;
}

int
tav_set_params(hdl, setmode, usemode, p, r)
	void *hdl;
        int setmode, usemode;
        struct  audio_params *p, *r;
{
        struct tav_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

        sc = hdl;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	if (!(setmode & AUMODE_PLAY))
		return 0;

	if (p->encoding == AUDIO_ENCODING_ULAW)
		p->encoding = AUDIO_ENCODING_MPEG_L2_STREAM;

	switch(p->encoding) {
	default:
		return EINVAL;

	case AUDIO_ENCODING_SLINEAR_BE:

		/* XXX: todo: add 8bit and mono using software */ 
		p->precision = 16;
		p->channels = 2;

		/* XXX: this might depend on the specific board. 
		   should be handled by the backend */

		p->sample_rate = 44100;

		bus_space_write_1(iot, ioh, TAV_STR_SEL, 
		    TAV_STR_SEL_AUDIO_BYPASS);
		break;

	/* XXX: later: add ULINEAR, and LE using software encoding */

	case AUDIO_ENCODING_MPEG_L1_STREAM:
		/* FALLTHROUGH */
	case AUDIO_ENCODING_MPEG_L2_STREAM:
		bus_space_write_1(iot, ioh, TAV_STR_SEL,
		    TAV_STR_SEL_MPEG_AUDIO_STREAM);
		p->sample_rate = 44100;
		p->precision = 1;
		break;

	case AUDIO_ENCODING_MPEG_L1_PACKETS:
		/* FALLTHROUGH */
	case AUDIO_ENCODING_MPEG_L2_PACKETS:
		bus_space_write_1(iot, ioh, TAV_STR_SEL,
		    TAV_STR_SEL_MPEG_AUDIO_PACKETS);
		p->sample_rate = 44100;
		p->precision = 1;
		break;

	case AUDIO_ENCODING_MPEG_L1_SYSTEM:
		/* FALLTHROUGH */
	case AUDIO_ENCODING_MPEG_L2_SYSTEM:
		bus_space_write_1(iot, ioh, TAV_STR_SEL,
		    TAV_STR_SEL_MPEG_SYSTEM_STREAM);
		p->sample_rate = 44100;
		p->precision = 1;
		break;
	}
	tav_write_byte(iot, ioh, TAV_RESTART, 1);
	do {
		delay(10);
	} while (tav_read_byte(iot, ioh, TAV_RESTART));

	return 0;
}

int
tav_set_port(hdl, mc)
        void *hdl;
	mixer_ctrl_t *mc;
{
        struct tav_softc *sc;

	sc = hdl;
	/* dummy */
	return 0;
}

int
tav_get_port(hdl, mc)
        void *hdl;
	mixer_ctrl_t *mc;
{
        struct tav_softc *sc;

	sc = hdl;
	/* dummy */
	return 0;
}

int
tav_query_devinfo(hdl, di)
	void *hdl;
	mixer_devinfo_t *di;
{
	return ENXIO;
}

int
tav_speaker_ctl(hdl, value)
	void *hdl;
	int value;
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
