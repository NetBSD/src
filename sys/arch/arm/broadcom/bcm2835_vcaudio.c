/* $NetBSD: bcm2835_vcaudio.c,v 1.6 2014/09/02 21:46:35 jmcneill Exp $ */

/*-
 * Copyright (c) 2013 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * VideoCore audio interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_vcaudio.c,v 1.6 2014/09/02 21:46:35 jmcneill Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/workqueue.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <interface/compat/vchi_bsd.h>
#include <interface/vchiq_arm/vchiq_netbsd.h>
#include <interface/vchi/vchi.h>

#include "bcm2835_vcaudioreg.h"

#define vol2pct(vol)	(((vol) * 100) / 255)

enum {
	VCAUDIO_OUTPUT_CLASS,
	VCAUDIO_INPUT_CLASS,
	VCAUDIO_OUTPUT_MASTER_VOLUME,
	VCAUDIO_INPUT_DAC_VOLUME,
	VCAUDIO_ENUM_LAST,
};

enum vcaudio_dest {
	VCAUDIO_DEST_AUTO = 0,
	VCAUDIO_DEST_HP = 1,
	VCAUDIO_DEST_HDMI = 2,
};

struct vcaudio_work {
	struct work			vw_wk;
};

struct vcaudio_softc {
	device_t			sc_dev;
	device_t			sc_audiodev;

	kmutex_t			sc_lock;
	kmutex_t			sc_intr_lock;

	kmutex_t			sc_msglock;
	kcondvar_t			sc_msgcv;

	struct audio_format		sc_format;
	struct audio_encoding_set	*sc_encodings;

	void				(*sc_pint)(void *);
	void				*sc_pintarg;
	audio_params_t			sc_pparam;
	bool				sc_started;
	int				sc_pbytes;
	off_t				sc_ppos;
	void				*sc_pstart;
	void				*sc_pend;
	int				sc_pblksize;

	bool				sc_msgdone;
	int				sc_success;

	VCHI_INSTANCE_T			sc_instance;
	VCHI_CONNECTION_T		sc_connection;
	VCHI_SERVICE_HANDLE_T		sc_service;

	short				sc_peer_version;

	struct workqueue		*sc_wq;
	struct vcaudio_work		sc_work;

	int				sc_volume;
	enum vcaudio_dest		sc_dest;
};

static int	vcaudio_match(device_t, cfdata_t, void *);
static void	vcaudio_attach(device_t, device_t, void *);
static int	vcaudio_rescan(device_t, const char *, const int *);
static void	vcaudio_childdet(device_t, device_t);

static int	vcaudio_init(struct vcaudio_softc *);
static void	vcaudio_service_callback(void *,
					 const VCHI_CALLBACK_REASON_T,
					 void *);
static int	vcaudio_msg_sync(struct vcaudio_softc *, VC_AUDIO_MSG_T *, size_t);
static void	vcaudio_worker(struct work *, void *);

static int	vcaudio_open(void *, int);
static void	vcaudio_close(void *);
static int	vcaudio_query_encoding(void *, struct audio_encoding *);
static int	vcaudio_set_params(void *, int, int,
				  audio_params_t *, audio_params_t *,
				  stream_filter_list_t *,
				  stream_filter_list_t *);
static int	vcaudio_halt_output(void *);
static int	vcaudio_halt_input(void *);
static int	vcaudio_set_port(void *, mixer_ctrl_t *);
static int	vcaudio_get_port(void *, mixer_ctrl_t *);
static int	vcaudio_query_devinfo(void *, mixer_devinfo_t *);
static int	vcaudio_getdev(void *, struct audio_device *);
static int	vcaudio_get_props(void *);
static int	vcaudio_round_blocksize(void *, int, int, const audio_params_t *);
static size_t	vcaudio_round_buffersize(void *, int, size_t);
static int	vcaudio_trigger_output(void *, void *, void *, int,
				     void (*)(void *), void *,
				     const audio_params_t *);
static int	vcaudio_trigger_input(void *, void *, void *, int,
				    void (*)(void *), void *,
				    const audio_params_t *);
static void	vcaudio_get_locks(void *, kmutex_t **, kmutex_t **);

static const struct audio_hw_if vcaudio_hw_if = {
	.open = vcaudio_open,
	.close = vcaudio_close,
	.query_encoding = vcaudio_query_encoding,
	.set_params = vcaudio_set_params,
	.halt_output = vcaudio_halt_output,
	.halt_input = vcaudio_halt_input,
	.getdev = vcaudio_getdev,
	.set_port = vcaudio_set_port,
	.get_port = vcaudio_get_port,
	.query_devinfo = vcaudio_query_devinfo,
	.get_props = vcaudio_get_props,
	.round_blocksize = vcaudio_round_blocksize,
	.round_buffersize = vcaudio_round_buffersize,
	.trigger_output = vcaudio_trigger_output,
	.trigger_input = vcaudio_trigger_input,
	.get_locks = vcaudio_get_locks,
};

CFATTACH_DECL2_NEW(vcaudio, sizeof(struct vcaudio_softc),
    vcaudio_match, vcaudio_attach, NULL, NULL, vcaudio_rescan, vcaudio_childdet);

static int
vcaudio_match(device_t parent, cfdata_t match, void *aux)
{
	struct vchiq_attach_args *vaa = aux;

	return !strcmp(vaa->vaa_name, "AUDS");
}

static void
vcaudio_attach(device_t parent, device_t self, void *aux)
{
	struct vcaudio_softc *sc = device_private(self);
	int error;

	sc->sc_dev = self;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_msglock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_msgcv, "vcaudiocv");
	sc->sc_success = -1;
	error = workqueue_create(&sc->sc_wq, "vcaudiowq", vcaudio_worker,
	    sc, PRI_BIO, IPL_SCHED, WQ_MPSAFE);
	if (error) {
		aprint_error(": couldn't create workqueue (%d)\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": AUDS\n");

	vcaudio_rescan(self, NULL, NULL);
}

static int
vcaudio_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct vcaudio_softc *sc = device_private(self);
	int error;

	if (ifattr_match(ifattr, "audiobus") && sc->sc_audiodev == NULL) {
		error = vcaudio_init(sc);
		if (error)
			return error;

		sc->sc_audiodev = audio_attach_mi(&vcaudio_hw_if,
		    sc, sc->sc_dev);
	}
	return 0;
}

static void
vcaudio_childdet(device_t self, device_t child)
{
	struct vcaudio_softc *sc = device_private(self);

	if (sc->sc_audiodev == child)
		sc->sc_audiodev = NULL;
}

static int
vcaudio_init(struct vcaudio_softc *sc)
{
	VC_AUDIO_MSG_T msg;
	int error;

	sc->sc_volume = 128;
	sc->sc_dest = VCAUDIO_DEST_AUTO;

	sc->sc_format.mode = AUMODE_PLAY|AUMODE_RECORD;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 0;
	sc->sc_format.frequency[0] = 48000;
	sc->sc_format.frequency[1] = 48000;

	error = auconv_create_encodings(&sc->sc_format, 1, &sc->sc_encodings);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't create encodings (error=%d)\n", error);
		return error;
	}

	error = vchi_initialise(&sc->sc_instance);
	if (error) {
		device_printf(sc->sc_dev, "couldn't init vchi instance (%d)\n",
		    error);
		return EIO;
	}

	error = vchi_connect(NULL, 0, sc->sc_instance);
	if (error) {
		device_printf(sc->sc_dev, "couldn't connect vchi (%d)\n",
		    error);
		return EIO;
	}

	SERVICE_CREATION_T setup = {
		.version = VCHI_VERSION(VC_AUDIOSERV_VER),
		.service_id = VC_AUDIO_SERVER_NAME,
		.connection = &sc->sc_connection,
		.rx_fifo_size = 0,
		.tx_fifo_size = 0,
		.callback = vcaudio_service_callback,
		.callback_param = sc,
		.want_unaligned_bulk_rx = 1,
		.want_unaligned_bulk_tx = 1,
		.want_crc = 0,
	};

	error = vchi_service_open(sc->sc_instance, &setup, &sc->sc_service);
	if (error) {
		device_printf(sc->sc_dev, "couldn't open service (%d)\n",
		    error);
		return EIO;
	}

	vchi_get_peer_version(sc->sc_service, &sc->sc_peer_version);

	if (sc->sc_peer_version < 2) {
		aprint_error_dev(sc->sc_dev,
		    "peer version (%d) is less than the required version (2)\n",
		    sc->sc_peer_version);
		return EINVAL;
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_OPEN;
	error = vchi_msg_queue(sc->sc_service, &msg, sizeof(msg),
	    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (error) {
		device_printf(sc->sc_dev, "couldn't send OPEN message (%d)\n",
		    error);
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_CONFIG;
	msg.u.config.channels = 2;
	msg.u.config.samplerate = 48000;
	msg.u.config.bps = 16;
	error = vcaudio_msg_sync(sc, &msg, sizeof(msg));
	if (error) {
		device_printf(sc->sc_dev, "couldn't send CONFIG message (%d)\n",
		    error);
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_CONTROL;
	msg.u.control.volume = vol2pct(sc->sc_volume);
	msg.u.control.dest = VCAUDIO_DEST_AUTO;
	error = vcaudio_msg_sync(sc, &msg, sizeof(msg));
	if (error) {
		device_printf(sc->sc_dev, "couldn't send CONTROL message (%d)\n", error);
	}
	vchi_service_release(sc->sc_service);

	return 0;
}

static void
vcaudio_service_callback(void *priv, const VCHI_CALLBACK_REASON_T reason,
    void *msg_handle)
{
	struct vcaudio_softc *sc = priv;
	VC_AUDIO_MSG_T msg;
	int32_t msglen = 0;
	int error;
	void (*intr)(void *) = NULL;
	void *intrarg = NULL;

	if (sc == NULL || reason != VCHI_CALLBACK_MSG_AVAILABLE)
		return;

	memset(&msg, 0, sizeof(msg));
	error = vchi_msg_dequeue(sc->sc_service, &msg, sizeof(msg), &msglen,
	    VCHI_FLAGS_NONE);
	if (error) {
		device_printf(sc->sc_dev, "couldn't dequeue msg (%d)\n",
		    error);
		return;
	}

	switch (msg.type) {
	case VC_AUDIO_MSG_TYPE_RESULT:
		mutex_enter(&sc->sc_msglock);
		sc->sc_success = msg.u.result.success;
		sc->sc_msgdone = true;
		cv_broadcast(&sc->sc_msgcv);
		mutex_exit(&sc->sc_msglock);
		break;
	case VC_AUDIO_MSG_TYPE_COMPLETE:
		intr = msg.u.complete.callback;
		intrarg = msg.u.complete.cookie;
		if (intr && intrarg) {
			int count = msg.u.complete.count & 0xffff;
			int perr = (msg.u.complete.count & 0x40000000) != 0;
			bool sched = false;
			mutex_enter(&sc->sc_intr_lock);
			if (count > 0) {
				sc->sc_pbytes += count;
			}
			if (perr && sc->sc_started) {
#ifdef VCAUDIO_DEBUG
				device_printf(sc->sc_dev, "underrun\n");
#endif
				sched = true;
			}
			if (sc->sc_pbytes >= sc->sc_pblksize) {
				sc->sc_pbytes -= sc->sc_pblksize;
				sched = true;
			}

			if (sched) {
				intr(intrarg);
				workqueue_enqueue(sc->sc_wq, (struct work *)&sc->sc_work, NULL);
			}
			mutex_exit(&sc->sc_intr_lock);
		}
		break;
	default:
		break;
	}
}

static void
vcaudio_worker(struct work *wk, void *priv)
{
	struct vcaudio_softc *sc = priv;
	VC_AUDIO_MSG_T msg;
	void (*intr)(void *);
	void *intrarg;
	void *block;
	int error, resid, off, nb, count;

	mutex_enter(&sc->sc_intr_lock);
	intr = sc->sc_pint;
	intrarg = sc->sc_pintarg;

	if (intr == NULL || intrarg == NULL) {
		mutex_exit(&sc->sc_intr_lock);
		return;
	}

	vchi_service_use(sc->sc_service);

	if (sc->sc_started == false) {
#ifdef VCAUDIO_DEBUG
		device_printf(sc->sc_dev, "starting output\n");
#endif

		memset(&msg, 0, sizeof(msg));
		msg.type = VC_AUDIO_MSG_TYPE_START;
		error = vchi_msg_queue(sc->sc_service, &msg, sizeof(msg),
		    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
		if (error) {
			printf("%s: failed to start (%d)\n", __func__, error);
			goto done;
		}

		sc->sc_started = true;
		sc->sc_pbytes = 0;
		sc->sc_ppos = 0;

		count = sc->sc_pblksize * 2;
	} else {
		count = sc->sc_pblksize;
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_WRITE;
	msg.u.write.max_packet = 4000;
	msg.u.write.count = count;
	msg.u.write.callback = intr;
	msg.u.write.cookie = intrarg;
	msg.u.write.silence = 0;

	error = vchi_msg_queue(sc->sc_service, &msg, sizeof(msg),
	    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (error) {
		printf("%s: failed to write (%d)\n", __func__, error);
		goto done;
	}

	block = (uint8_t *)sc->sc_pstart + sc->sc_ppos;
	resid = count;
	off = 0;
	while (resid > 0) {
		nb = min(resid, msg.u.write.max_packet);
		error = vchi_msg_queue(sc->sc_service,
		    (char *)block + off, nb,
		    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
		if (error) {
			printf("%s: failed to queue data (%d)\n", __func__, error);
			goto done;
		}
		off += nb;
		resid -= nb;
	}

	sc->sc_ppos += count;
	if ((uint8_t *)sc->sc_pstart + sc->sc_ppos >= (uint8_t *)sc->sc_pend)
		sc->sc_ppos = 0;

done:
	mutex_exit(&sc->sc_intr_lock);
	vchi_service_release(sc->sc_service);
}

static int
vcaudio_msg_sync(struct vcaudio_softc *sc, VC_AUDIO_MSG_T *msg, size_t msglen)
{
	int error = 0;

	mutex_enter(&sc->sc_msglock);

	sc->sc_success = -1;
	sc->sc_msgdone = false;

	error = vchi_msg_queue(sc->sc_service, msg, msglen,
	    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (error) {
		printf("%s: failed to queue message (%d)\n", __func__, error);
		goto done;
	}

	while (!sc->sc_msgdone) {
		error = cv_wait_sig(&sc->sc_msgcv, &sc->sc_msglock);
		if (error)
			break;
	}
	if (sc->sc_success != 0)
		error = EIO;
done:
	mutex_exit(&sc->sc_msglock);

	return error;
}

static int
vcaudio_open(void *priv, int flags)
{
	return 0;
}

static void
vcaudio_close(void *priv)
{
}

static int
vcaudio_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct vcaudio_softc *sc = priv;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
vcaudio_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct vcaudio_softc *sc = priv;
	int index;

	if (play && (setmode & AUMODE_PLAY)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_PLAY, play, true, pfil);
		if (index < 0)
			return EINVAL;
	}

	return 0;
}

static int
vcaudio_halt_output(void *priv)
{
	struct vcaudio_softc *sc = priv;
	VC_AUDIO_MSG_T msg;
	int error = 0;

	vchi_service_use(sc->sc_service);
	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_STOP;
	msg.u.stop.draining = 1;
	error = vchi_msg_queue(sc->sc_service, &msg, sizeof(msg),
	    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (error) {
		device_printf(sc->sc_dev, "couldn't send STOP message (%d)\n",
		    error);
	}
	vchi_service_release(sc->sc_service);

	sc->sc_pint = NULL;
	sc->sc_pintarg = NULL;
	sc->sc_started = false;

#ifdef VCAUDIO_DEBUG
	device_printf(sc->sc_dev, "halting output\n");
#endif

	return error;
}

static int
vcaudio_halt_input(void *priv)
{
	return EINVAL;
}

static int
vcaudio_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct vcaudio_softc *sc = priv;
	VC_AUDIO_MSG_T msg;
	int error;

	switch (mc->dev) {
	case VCAUDIO_OUTPUT_MASTER_VOLUME:
	case VCAUDIO_INPUT_DAC_VOLUME:
		sc->sc_volume = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		memset(&msg, 0, sizeof(msg));
		msg.type = VC_AUDIO_MSG_TYPE_CONTROL;
		msg.u.control.volume = vol2pct(sc->sc_volume);
		msg.u.control.dest = VCAUDIO_DEST_AUTO;
		vchi_service_use(sc->sc_service);
		error = vcaudio_msg_sync(sc, &msg, sizeof(msg));
		if (error) {
			device_printf(sc->sc_dev, "couldn't send CONTROL message (%d)\n", error);
		}
		vchi_service_release(sc->sc_service);

		return error;
	}
	return ENXIO;
}

static int
vcaudio_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct vcaudio_softc *sc = priv;

	switch (mc->dev) {
	case VCAUDIO_OUTPUT_MASTER_VOLUME:
	case VCAUDIO_INPUT_DAC_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    sc->sc_volume;
		return 0;
	}
	return ENXIO;
}

static int
vcaudio_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	switch (di->index) {
	case VCAUDIO_OUTPUT_CLASS:
		di->mixer_class = VCAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case VCAUDIO_INPUT_CLASS:
		di->mixer_class = VCAUDIO_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case VCAUDIO_OUTPUT_MASTER_VOLUME:
		di->mixer_class = VCAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case VCAUDIO_INPUT_DAC_VOLUME:
		di->mixer_class = VCAUDIO_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

static int
vcaudio_getdev(void *priv, struct audio_device *audiodev)
{
	struct vcaudio_softc *sc = priv;

	snprintf(audiodev->name, sizeof(audiodev->name), "VCHIQ AUDS");
	snprintf(audiodev->version, sizeof(audiodev->version),
	    "%d", sc->sc_peer_version);
	snprintf(audiodev->config, sizeof(audiodev->config), "vcaudio");

	return 0;
}

static int
vcaudio_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK|AUDIO_PROP_CAPTURE|AUDIO_PROP_INDEPENDENT;
}

static int
vcaudio_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	return PAGE_SIZE;
}

static size_t
vcaudio_round_buffersize(void *priv, int direction, size_t bufsize)
{
	size_t sz;

	sz = (bufsize + 0x3ff) & ~0x3ff;
	if (sz > 32768)
		sz = 32768;

	return sz;
}

static int
vcaudio_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct vcaudio_softc *sc = priv;

	sc->sc_pparam = *params;
	sc->sc_pint = intr;
	sc->sc_pintarg = intrarg;
	sc->sc_ppos = 0;
	sc->sc_pstart = start;
	sc->sc_pend = end;
	sc->sc_pblksize = blksize;
	workqueue_enqueue(sc->sc_wq, (struct work *)&sc->sc_work, NULL);

	return 0;
}

static int
vcaudio_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	return EINVAL;
}

static void
vcaudio_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct vcaudio_softc *sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
