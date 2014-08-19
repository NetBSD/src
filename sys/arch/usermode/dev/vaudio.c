/* $NetBSD: vaudio.c,v 1.3.10.1 2014/08/20 00:03:26 tls Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vaudio.c,v 1.3.10.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/audioio.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

#include <dev/audio_if.h>
#include <dev/auconv.h>

static const struct audio_format vaudio_audio_formats[1] = {
	{ NULL, AUMODE_PLAY|AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	  2, AUFMT_STEREO, 0, { 8000, 48000 } },
};

struct vaudio_stream {
	struct vaudio_softc		*st_softc;
	void *				st_sih;
	callout_t			st_callout;
	void				(*st_intr)(void *);
	void *				st_intrarg;
	uint8_t *			st_start;
	uint8_t *			st_end;
	uint8_t *			st_cur;
	int				st_blksize;
	bool				st_running;
};

struct vaudio_softc {
	device_t			sc_dev;
	void *				sc_audiodev;
	const char *			sc_audiopath;
	int				sc_audiofd;
	struct audio_encoding_set *	sc_encodings;
	audio_params_t			sc_pparam;
	audio_params_t			sc_rparam;
	kmutex_t			sc_lock;
	kmutex_t			sc_intr_lock;

	struct vaudio_stream		sc_play;
	struct vaudio_stream		sc_record;
};

static int	vaudio_match(device_t, cfdata_t, void *);
static void	vaudio_attach(device_t, device_t, void *);
static bool	vaudio_shutdown(device_t, int);

static void	vaudio_intr(void *);
static void	vaudio_softintr_play(void *);
static void	vaudio_softintr_record(void *);

static int	vaudio_open(void *, int);
static void	vaudio_close(void *);
static int	vaudio_drain(void *);
static int	vaudio_query_encoding(void *, audio_encoding_t *);
static int	vaudio_set_params(void *, int, int, audio_params_t *,
				  audio_params_t *, stream_filter_list_t *,
				  stream_filter_list_t *);
static int	vaudio_commit_settings(void *);
static int	vaudio_trigger_output(void *, void *, void *, int,
				      void (*)(void *), void *,
				      const audio_params_t *);
static int	vaudio_trigger_input(void *, void *, void *, int,
				     void (*)(void *), void *,
				     const audio_params_t *);
static int	vaudio_halt_output(void *);
static int	vaudio_halt_input(void *);
static int	vaudio_getdev(void *, struct audio_device *);
static int	vaudio_set_port(void *, mixer_ctrl_t *);
static int	vaudio_get_port(void *, mixer_ctrl_t *);
static int	vaudio_query_devinfo(void *, mixer_devinfo_t *);
static int	vaudio_get_props(void *);
static void	vaudio_get_locks(void *, kmutex_t **, kmutex_t **);	

CFATTACH_DECL_NEW(vaudio, sizeof(struct vaudio_softc),
    vaudio_match, vaudio_attach, NULL, NULL);

static const struct audio_hw_if vaudio_hw_if = {
	.open = vaudio_open,
	.close = vaudio_close,
	.drain = vaudio_drain,
	.query_encoding = vaudio_query_encoding,
	.set_params = vaudio_set_params,
	.commit_settings = vaudio_commit_settings,
	.halt_output = vaudio_halt_output,
	.halt_input = vaudio_halt_input,
	.getdev = vaudio_getdev,
	.set_port = vaudio_set_port,
	.get_port = vaudio_get_port,
	.query_devinfo = vaudio_query_devinfo,
	.get_props = vaudio_get_props,
	.trigger_output = vaudio_trigger_output,
	.trigger_input = vaudio_trigger_input,
	.get_locks = vaudio_get_locks,
};

static int
vaudio_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_VAUDIO)
		return 0;

	return 1;
}

static void
vaudio_attach(device_t parent, device_t self, void *opaque)
{
	struct vaudio_softc *sc = device_private(self);
	struct thunkbus_attach_args *taa = opaque;
	int error;

	aprint_naive("\n");
	aprint_normal(": Virtual Audio (device = %s)\n", taa->u.vaudio.device);

	sc->sc_dev = self;

	pmf_device_register1(self, NULL, NULL, vaudio_shutdown);

	sc->sc_audiopath = taa->u.vaudio.device;
	sc->sc_audiofd = thunk_audio_open(sc->sc_audiopath);
	if (sc->sc_audiofd == -1) {
		aprint_error_dev(self, "couldn't open audio device: %d\n",
		    thunk_geterrno());
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	error = auconv_create_encodings(vaudio_audio_formats,
	    __arraycount(vaudio_audio_formats), &sc->sc_encodings);
	if (error) {
		aprint_error_dev(self, "couldn't create encodings\n");
		return;
	}

	sc->sc_play.st_softc = sc;
	sc->sc_play.st_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    vaudio_softintr_play, &sc->sc_play);
	callout_init(&sc->sc_play.st_callout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_play.st_callout, vaudio_intr, &sc->sc_play);

	sc->sc_record.st_softc = sc;
	sc->sc_record.st_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    vaudio_softintr_record, &sc->sc_record);
	callout_init(&sc->sc_record.st_callout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_record.st_callout, vaudio_intr, &sc->sc_record);

	sc->sc_audiodev = audio_attach_mi(&vaudio_hw_if, sc, self);
}

static bool
vaudio_shutdown(device_t self, int flags)
{
	struct vaudio_softc *sc = device_private(self);

	if (sc->sc_audiofd != -1)
		thunk_audio_close(sc->sc_audiofd);

	return true;
}

static void
vaudio_intr(void *opaque)
{
	struct vaudio_stream *st = opaque;

	softint_schedule(st->st_sih);
}

static void
vaudio_softintr_play(void *opaque)
{
	struct vaudio_stream *st = opaque;
	struct vaudio_softc *sc = st->st_softc;

	while (st->st_running) {
		if (thunk_audio_pollout(sc->sc_audiofd) < st->st_blksize)
			break;
		thunk_audio_write(sc->sc_audiofd, st->st_cur, st->st_blksize);
		mutex_spin_enter(&sc->sc_intr_lock);
		st->st_intr(st->st_intrarg);
		mutex_spin_exit(&sc->sc_intr_lock);
		st->st_cur += st->st_blksize;
		if (st->st_cur >= st->st_end)
			st->st_cur = st->st_start;
	}

	if (st->st_running) {
		callout_schedule(&st->st_callout, 1);
	}
}

static void
vaudio_softintr_record(void *opaque)
{
	struct vaudio_stream *st = opaque;
	struct vaudio_softc *sc = st->st_softc;

	while (st->st_running) {
		if (thunk_audio_pollin(sc->sc_audiofd) < st->st_blksize)
			break;
		thunk_audio_read(sc->sc_audiofd, st->st_cur, st->st_blksize);
		mutex_spin_enter(&sc->sc_intr_lock);
		st->st_intr(st->st_intrarg);
		mutex_spin_exit(&sc->sc_intr_lock);
		st->st_cur += st->st_blksize;
		if (st->st_cur >= st->st_end)
			st->st_cur = st->st_start;
	}

	if (st->st_running) {
		callout_schedule(&st->st_callout, 1);
	}
}

static int
vaudio_open(void *opaque, int flags)
{
	return 0;
}

static void
vaudio_close(void *opaque)
{
}

static int
vaudio_drain(void *opaque)
{
	struct vaudio_softc *sc = opaque;

	return thunk_audio_drain(sc->sc_audiofd);
}

static int
vaudio_query_encoding(void *opaque, audio_encoding_t *enc)
{
	struct vaudio_softc *sc = opaque;

	return auconv_query_encoding(sc->sc_encodings, enc);
}

static int
vaudio_set_params(void *opaque, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct vaudio_softc *sc = opaque;
	audio_params_t *p;
	stream_filter_list_t *fil;
	int mode, index;

	for (mode = AUMODE_RECORD; mode != -1;
	    mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;
		p = mode == AUMODE_PLAY ? play : rec;
		fil = mode == AUMODE_PLAY ? pfil : rfil;
		if (p == NULL)
			continue;

		index = auconv_set_converter(vaudio_audio_formats,
		    __arraycount(vaudio_audio_formats), mode, p, TRUE, fil);
		if (index < 0)
			return EINVAL;
		if (fil->req_size > 0)
			p = &fil->filters[0].param;

		if (mode == AUMODE_PLAY)
			sc->sc_pparam = *p;
		else
			sc->sc_rparam = *p;
	}

	return 0;
}

static int
vaudio_commit_settings(void *opaque)
{
	struct vaudio_softc *sc = opaque;
	thunk_audio_config_t pconf, rconf;

	memset(&pconf, 0, sizeof(pconf));
	pconf.sample_rate = sc->sc_pparam.sample_rate;
	pconf.precision = sc->sc_pparam.precision;
	pconf.validbits = sc->sc_pparam.validbits;
	pconf.channels = sc->sc_pparam.channels;

	memset(&rconf, 0, sizeof(rconf));
	rconf.sample_rate = sc->sc_rparam.sample_rate;
	rconf.precision = sc->sc_rparam.precision;
	rconf.validbits = sc->sc_rparam.validbits;
	rconf.channels = sc->sc_rparam.channels;

	return thunk_audio_config(sc->sc_audiofd, &pconf, &rconf);
}

static int
vaudio_trigger_output(void *opaque, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	struct vaudio_softc *sc = opaque;
	struct vaudio_stream *st = &sc->sc_play;

	st->st_intr = intr;
	st->st_intrarg = intrarg;
	st->st_start = st->st_cur = start;
	st->st_end = end;
	st->st_blksize = blksize;
	st->st_running = true;
	callout_schedule(&st->st_callout, 1);

	return 0;
}

static int
vaudio_trigger_input(void *opaque, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	struct vaudio_softc *sc = opaque;
	struct vaudio_stream *st = &sc->sc_record;

	st->st_intr = intr;
	st->st_intrarg = intrarg;
	st->st_start = st->st_cur = start;
	st->st_end = end;
	st->st_blksize = blksize;
	st->st_running = true;
	callout_schedule(&st->st_callout, 1);

	return 0;
}

static int
vaudio_halt_output(void *opaque)
{
	struct vaudio_softc *sc = opaque;

	sc->sc_play.st_running = false;
	callout_halt(&sc->sc_play.st_callout, NULL);

	return 0;
}

static int
vaudio_halt_input(void *opaque)
{
	struct vaudio_softc *sc = opaque;

	sc->sc_record.st_running = false;
	callout_halt(&sc->sc_record.st_callout, NULL);

	return 0;
}

static int
vaudio_getdev(void *opaque, struct audio_device *adev)
{
	struct vaudio_softc *sc = opaque;

	snprintf(adev->name, sizeof(adev->name), "Virtual Audio");
	adev->version[0] = '\0';
	snprintf(adev->config, sizeof(adev->config), "%s", sc->sc_audiopath);

	return 0;
}

static int
vaudio_set_port(void *opaque, mixer_ctrl_t *mc)
{
	return EINVAL;
}

static int
vaudio_get_port(void *opaque, mixer_ctrl_t *mc)
{
	return EINVAL;
}

static int
vaudio_query_devinfo(void *opaque, mixer_devinfo_t *di)
{
	return EINVAL;
}

static int
vaudio_get_props(void *opaque)
{
	/* NB: should query host audio driver for capabilities */
	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE;
}

static void
vaudio_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	struct vaudio_softc *sc = opaque;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
