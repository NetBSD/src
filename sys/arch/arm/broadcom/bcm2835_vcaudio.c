/* $NetBSD: bcm2835_vcaudio.c,v 1.2.4.4 2017/12/03 11:35:52 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_vcaudio.c,v 1.2.4.4 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/auvolconv.h>

#include <interface/compat/vchi_bsd.h>
#include <interface/vchiq_arm/vchiq_netbsd.h>
#include <interface/vchi/vchi.h>

#include "bcm2835_vcaudioreg.h"

/* levels with 5% volume step */
static int vcaudio_levels[] = {
	-10239, -4605, -3794, -3218, -2772,
	-2407, -2099, -1832, -1597, -1386,
	-1195, -1021, -861, -713, -575,
	-446, -325, -210, -102, 0,
};

#define vol2db(vol)	vcaudio_levels[((vol) * 20) >> 8]
#define vol2vc(vol)	((uint32_t)(-(vol2db((vol)) << 8) / 100))

enum {
	VCAUDIO_OUTPUT_CLASS,
	VCAUDIO_INPUT_CLASS,
	VCAUDIO_OUTPUT_MASTER_VOLUME,
	VCAUDIO_INPUT_DAC_VOLUME,
	VCAUDIO_OUTPUT_AUTO_VOLUME,
	VCAUDIO_OUTPUT_HEADPHONE_VOLUME,
	VCAUDIO_OUTPUT_HDMI_VOLUME,
	VCAUDIO_OUTPUT_SELECT,
	VCAUDIO_ENUM_LAST,
};

enum vcaudio_dest {
	VCAUDIO_DEST_AUTO = 0,
	VCAUDIO_DEST_HP = 1,
	VCAUDIO_DEST_HDMI = 2,
};

/*
 * Maximum message size is 4000 bytes and VCHIQ can accept 16 messages.
 *
 * 4000 bytes of 16bit 48kHz stereo is approximately 21ms.
 *
 * We get complete messages at ~10ms intervals.
 *
 * Setting blocksize to 4 x 1600 means that we send approx 33ms of audio. We
 * prefill by two blocks before starting audio meaning we have 50ms of latency.
 * 
 * Six messages of 1600 bytes was chosen working back from a desired latency of
 * 50ms.
 */

#define VCAUDIO_MSGSIZE		1600
#define VCAUDIO_NUMMSGS		4
#define VCAUDIO_BLOCKSIZE	(VCAUDIO_MSGSIZE * VCAUDIO_NUMMSGS)
#define VCAUDIO_BUFFERSIZE	128000
#define VCAUDIO_PREFILLCOUNT	2

struct vcaudio_softc {
	device_t			sc_dev;
	device_t			sc_audiodev;

	lwp_t				*sc_lwp;

	kmutex_t			sc_lock;
	kmutex_t			sc_intr_lock;
	kcondvar_t			sc_datacv;

	kmutex_t			sc_msglock;
	kcondvar_t			sc_msgcv;

	struct audio_format		sc_format;
	struct audio_encoding_set	*sc_encodings;

	void				(*sc_pint)(void *);
	void				*sc_pintarg;
	audio_params_t			sc_pparam;
	bool				sc_started;
	int				sc_pblkcnt;	// prefill block count
	int				sc_abytes;	// available bytes
	int				sc_pbytes;	// played bytes
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

	int				sc_hwvol[3];
	enum vcaudio_dest		sc_dest;

	uint8_t				sc_swvol;
};

static int	vcaudio_match(device_t, cfdata_t, void *);
static void	vcaudio_attach(device_t, device_t, void *);
static int	vcaudio_rescan(device_t, const char *, const int *);
static void	vcaudio_childdet(device_t, device_t);

static int	vcaudio_init(struct vcaudio_softc *);
static void	vcaudio_service_callback(void *,
    const VCHI_CALLBACK_REASON_T, void *);
static int	vcaudio_msg_sync(struct vcaudio_softc *, VC_AUDIO_MSG_T *,
    size_t);
static void	vcaudio_worker(void *);

static int	vcaudio_open(void *, int);
static void	vcaudio_close(void *);
static int	vcaudio_query_encoding(void *, struct audio_encoding *);
static int	vcaudio_set_params(void *, int, int,
    audio_params_t *, audio_params_t *,
    stream_filter_list_t *, stream_filter_list_t *);
static int	vcaudio_halt_output(void *);
static int	vcaudio_halt_input(void *);
static int	vcaudio_set_port(void *, mixer_ctrl_t *);
static int	vcaudio_get_port(void *, mixer_ctrl_t *);
static int	vcaudio_query_devinfo(void *, mixer_devinfo_t *);
static int	vcaudio_getdev(void *, struct audio_device *);
static int	vcaudio_get_props(void *);

static int	vcaudio_round_blocksize(void *, int, int,
    const audio_params_t *);
static size_t	vcaudio_round_buffersize(void *, int, size_t);

static int	vcaudio_trigger_output(void *, void *, void *, int,
    void (*)(void *), void *, const audio_params_t *);
static int	vcaudio_trigger_input(void *, void *, void *, int,
    void (*)(void *), void *, const audio_params_t *);

static void	vcaudio_get_locks(void *, kmutex_t **, kmutex_t **);

static stream_filter_t *vcaudio_swvol_filter(struct audio_softc *,
    const audio_params_t *, const audio_params_t *);
static void	vcaudio_swvol_dtor(stream_filter_t *);

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
    vcaudio_match, vcaudio_attach, NULL, NULL, vcaudio_rescan,
    vcaudio_childdet);

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
	cv_init(&sc->sc_msgcv, "msg");
	cv_init(&sc->sc_datacv, "data");
	sc->sc_success = -1;

	error = kthread_create(PRI_BIO, KTHREAD_MPSAFE, NULL, vcaudio_worker,
	    sc, &sc->sc_lwp, "vcaudio");
	if (error) {
		aprint_error(": couldn't create thread (%d)\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": auds\n");

	error = vcaudio_rescan(self, NULL, NULL);
	if (error)
		aprint_error_dev(self, "not configured\n");

}

static int
vcaudio_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct vcaudio_softc *sc = device_private(self);
	int error;

	if (ifattr_match(ifattr, "audiobus") && sc->sc_audiodev == NULL) {
		error = vcaudio_init(sc);
		if (error) {
			return error;
		}

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

	sc->sc_swvol = 255;
	sc->sc_hwvol[VCAUDIO_DEST_AUTO] = 255;
	sc->sc_hwvol[VCAUDIO_DEST_HP] = 255;
	sc->sc_hwvol[VCAUDIO_DEST_HDMI] = 255;
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
		aprint_error_dev(sc->sc_dev,
		    "couldn't init vchi instance (%d)\n", error);
		return EIO;
	}

	error = vchi_connect(NULL, 0, sc->sc_instance);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't connect vchi (%d)\n", error);
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
		aprint_error_dev(sc->sc_dev, "couldn't open service (%d)\n",
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
		aprint_error_dev(sc->sc_dev,
		    "couldn't send OPEN message (%d)\n", error);
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_CONFIG;
	msg.u.config.channels = 2;
	msg.u.config.samplerate = 48000;
	msg.u.config.bps = 16;
	error = vcaudio_msg_sync(sc, &msg, sizeof(msg));
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't send CONFIG message (%d)\n", error);
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_CONTROL;
	msg.u.control.volume = vol2vc(sc->sc_hwvol[sc->sc_dest]);
	msg.u.control.dest = sc->sc_dest;
	error = vcaudio_msg_sync(sc, &msg, sizeof(msg));
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't send CONTROL message (%d)\n", error);
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
			int perr = (msg.u.complete.count & __BIT(30)) != 0;
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

			if (sched && sc->sc_pint) {
				intr(intrarg);
				sc->sc_abytes += sc->sc_pblksize;
				cv_signal(&sc->sc_datacv);
			}
			mutex_exit(&sc->sc_intr_lock);
		}
		break;
	default:
		break;
	}
}

static void
vcaudio_worker(void *priv)
{
	struct vcaudio_softc *sc = priv;
	VC_AUDIO_MSG_T msg;
	void (*intr)(void *);
	void *intrarg;
	void *block;
	int error, resid, off, nb, count;

	mutex_enter(&sc->sc_intr_lock);

	while (true) {
		intr = sc->sc_pint;
		intrarg = sc->sc_pintarg;

		if (intr == NULL || intrarg == NULL) {
			cv_wait_sig(&sc->sc_datacv, &sc->sc_intr_lock);
			continue;
		}

		KASSERT(sc->sc_pblksize != 0);

		if (sc->sc_abytes < sc->sc_pblksize) {
			cv_wait_sig(&sc->sc_datacv, &sc->sc_intr_lock);
			continue;
		}
		count = sc->sc_pblksize;

		memset(&msg, 0, sizeof(msg));
		msg.type = VC_AUDIO_MSG_TYPE_WRITE;
		msg.u.write.max_packet = VCAUDIO_MSGSIZE;
		msg.u.write.count = count;
		msg.u.write.callback = intr;
		msg.u.write.cookie = intrarg;
		msg.u.write.silence = 0;

	    	block = (uint8_t *)sc->sc_pstart + sc->sc_ppos;
		resid = count;
		off = 0;

		vchi_service_use(sc->sc_service);

		error = vchi_msg_queue(sc->sc_service, &msg, sizeof(msg),
		    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
		if (error) {
			printf("%s: failed to write (%d)\n", __func__, error);
			goto done;
		}

		while (resid > 0) {
			nb = min(resid, msg.u.write.max_packet);
			error = vchi_msg_queue(sc->sc_service,
			    (char *)block + off, nb,
			    VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
			if (error) {
				/* XXX What to do here? */
				device_printf(sc->sc_dev,
				    "failed to queue data (%d)\n", error);
				goto done;
			}
			off += nb;
			resid -= nb;
		}

		sc->sc_abytes -= count;
		sc->sc_ppos += count;
		if ((uint8_t *)sc->sc_pstart + sc->sc_ppos >=
		    (uint8_t *)sc->sc_pend)
			sc->sc_ppos = 0;

		if (!sc->sc_started) {
        		++sc->sc_pblkcnt;

			if (sc->sc_pblkcnt == VCAUDIO_PREFILLCOUNT) {

				memset(&msg, 0, sizeof(msg));
				msg.type = VC_AUDIO_MSG_TYPE_START;
				error = vchi_msg_queue(sc->sc_service, &msg,
				    sizeof(msg), VCHI_FLAGS_BLOCK_UNTIL_QUEUED,
				    NULL);
				if (error) {
					device_printf(sc->sc_dev,
					    "failed to start (%d)\n", error);
					goto done;
				}
				sc->sc_started = true;
				sc->sc_pbytes = 0;
			} else {
				intr(intrarg);
				sc->sc_abytes += sc->sc_pblksize;
			}
		}

done:
		vchi_service_release(sc->sc_service);
	}
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
		if (pfil->req_size > 0)
			play = &pfil->filters[0].param;
		pfil->prepend(pfil, vcaudio_swvol_filter, play);
	}

	return 0;
}

static int
vcaudio_halt_output(void *priv)
{
	struct vcaudio_softc *sc = priv;
	VC_AUDIO_MSG_T msg;
	int error = 0;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	vchi_service_use(sc->sc_service);
	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_STOP;
	msg.u.stop.draining = 0;
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
vcaudio_set_volume(struct vcaudio_softc *sc, enum vcaudio_dest dest,
    int hwvol)
{
	VC_AUDIO_MSG_T msg;
	int error;

	sc->sc_hwvol[dest] = hwvol;
	if (dest != sc->sc_dest)
		return 0;

	vchi_service_use(sc->sc_service);

	memset(&msg, 0, sizeof(msg));
	msg.type = VC_AUDIO_MSG_TYPE_CONTROL;
	msg.u.control.volume = vol2vc(hwvol);
	msg.u.control.dest = dest;

	error = vcaudio_msg_sync(sc, &msg, sizeof(msg));
	if (error) {
		device_printf(sc->sc_dev,
		    "couldn't send CONTROL message (%d)\n", error);
	}

	vchi_service_release(sc->sc_service);

	return error;
}

static int
vcaudio_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct vcaudio_softc *sc = priv;

	switch (mc->dev) {
	case VCAUDIO_OUTPUT_MASTER_VOLUME:
	case VCAUDIO_INPUT_DAC_VOLUME:
		sc->sc_swvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		return 0;
	case VCAUDIO_OUTPUT_AUTO_VOLUME:
		return vcaudio_set_volume(sc, VCAUDIO_DEST_AUTO,
		    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT]);
	case VCAUDIO_OUTPUT_HEADPHONE_VOLUME:
		return vcaudio_set_volume(sc, VCAUDIO_DEST_HP,
		    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT]);
	case VCAUDIO_OUTPUT_HDMI_VOLUME:
		return vcaudio_set_volume(sc, VCAUDIO_DEST_HDMI,
		    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT]);
	case VCAUDIO_OUTPUT_SELECT:
		if (mc->un.ord < 0 || mc->un.ord > 2)
			return EINVAL;
		sc->sc_dest = mc->un.ord;
		return vcaudio_set_volume(sc, mc->un.ord,
		    sc->sc_hwvol[mc->un.ord]);
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
		    sc->sc_swvol;
		return 0;
	case VCAUDIO_OUTPUT_AUTO_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    sc->sc_hwvol[VCAUDIO_DEST_AUTO];
		return 0;
	case VCAUDIO_OUTPUT_HEADPHONE_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    sc->sc_hwvol[VCAUDIO_DEST_HP];
		return 0;
	case VCAUDIO_OUTPUT_HDMI_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    sc->sc_hwvol[VCAUDIO_DEST_HDMI];
		return 0;
	case VCAUDIO_OUTPUT_SELECT:
		mc->un.ord = sc->sc_dest;
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
	case VCAUDIO_OUTPUT_AUTO_VOLUME:
		di->mixer_class = VCAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, "auto");
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		di->un.v.delta = 13;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case VCAUDIO_OUTPUT_HEADPHONE_VOLUME:
		di->mixer_class = VCAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNheadphone);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		di->un.v.delta = 13;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case VCAUDIO_OUTPUT_HDMI_VOLUME:
		di->mixer_class = VCAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, "hdmi");
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		di->un.v.delta = 13;
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
	case VCAUDIO_OUTPUT_SELECT:
		di->mixer_class = VCAUDIO_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNselect);
		di->type = AUDIO_MIXER_ENUM;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.e.num_mem = 3;
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[0].label.name, "auto");
		di->un.e.member[1].ord = 1;
		strcpy(di->un.e.member[1].label.name, AudioNheadphone);
		di->un.e.member[2].ord = 2;
		strcpy(di->un.e.member[2].label.name, "hdmi");
		return 0;
	}

	return ENXIO;
}

static int
vcaudio_getdev(void *priv, struct audio_device *audiodev)
{
	struct vcaudio_softc *sc = priv;

	snprintf(audiodev->name, sizeof(audiodev->name), "vchiq auds");
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
	return VCAUDIO_BLOCKSIZE;
}

static size_t
vcaudio_round_buffersize(void *priv, int direction, size_t bufsize)
{

	return VCAUDIO_BUFFERSIZE;
}

static int
vcaudio_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct vcaudio_softc *sc = priv;

	ASSERT_SLEEPABLE();
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	sc->sc_pparam = *params;
	sc->sc_pint = intr;
	sc->sc_pintarg = intrarg;
	sc->sc_ppos = 0;
	sc->sc_pstart = start;
	sc->sc_pend = end;
	sc->sc_pblksize = blksize;
	sc->sc_pblkcnt = 0;
	sc->sc_pbytes = 0;
	sc->sc_abytes = blksize;

	cv_signal(&sc->sc_datacv);

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

static stream_filter_t *
vcaudio_swvol_filter(struct audio_softc *asc,
    const audio_params_t *from, const audio_params_t *to)
{
	auvolconv_filter_t *this;
	device_t dev = audio_get_device(asc);
	struct vcaudio_softc *sc = device_private(dev);

	this = kmem_alloc(sizeof(auvolconv_filter_t), KM_SLEEP);
	this->base.base.fetch_to = auvolconv_slinear16_le_fetch_to;
	this->base.dtor = vcaudio_swvol_dtor;
	this->base.set_fetcher = stream_filter_set_fetcher;
	this->base.set_inputbuffer = stream_filter_set_inputbuffer;
	this->vol = &sc->sc_swvol;

	return (stream_filter_t *)this;
}

static void
vcaudio_swvol_dtor(stream_filter_t *this)
{
	if (this)
		kmem_free(this, sizeof(auvolconv_filter_t));
}
