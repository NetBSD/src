/*	$NetBSD: bthset.c,v 1.1 2006/06/19 15:44:45 gdamore Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bthset.c,v 1.1 2006/06/19 15:44:45 gdamore Exp $");

#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <netbt/bluetooth.h>
#include <netbt/rfcomm.h>
#include <netbt/sco.h>

#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/mulaw.h>

#include <dev/bluetooth/btdev.h>
#include <dev/bluetooth/bthset.h>

#undef DPRINTF
#undef DPRINTFN

#ifdef BTHSET_DEBUG
int bthset_debug = BTHSET_DEBUG;
#define DPRINTF(fmt, args...)		do {		\
	if (bthset_debug)				\
		printf("%s: "fmt, __func__ , ##args);	\
} while (/* CONSTCOND */0)

#define DPRINTFN(n, fmt, args...)	do {		\
	if (bthset_debug > (n))				\
		printf("%s: "fmt, __func__ , ##args);	\
} while (/* CONSTCOND */0)
#else
#define DPRINTF(...)
#define DPRINTFN(...)
#endif

/*****************************************************************************
 *
 *	Bluetooth SCO Audio Headset device
 */

MALLOC_DEFINE(M_BTHSET, "bthset", "Bluetooth Headset Memory");

/* bthset softc */
struct bthset_softc {
	struct btdev		 sc_btdev;	/* device+ */
	uint16_t		 sc_flags;

	struct device		*sc_audio;	/* MI audio device */
	void			*sc_intr;	/* interrupt cookie */

	/* Bluetooth */
	bdaddr_t		 sc_laddr;	/* local address */
	bdaddr_t		 sc_raddr;	/* remote address */
	uint16_t		 sc_state;	/* link state */
	struct sco_pcb		*sc_sco;	/* SCO handle */
	int			 sc_mtu;	/* SCO mtu */
	uint8_t			 sc_channel;	/* RFCOMM channel */
	int			 sc_err;	/* stored error */

	/* Receive */
	int			 sc_rx_want;	/* bytes wanted */
	uint8_t			*sc_rx_block;	/* receive block */
	void		       (*sc_rx_intr)(void *);	/* callback */
	void			*sc_rx_intrarg;	/* callback arg */
	struct mbuf		*sc_rx_mbuf;	/* leftover mbuf */

	/* Transmit */
	int			 sc_tx_size;	/* bytes to send */
	int			 sc_tx_pending;	/* packets pending */
	uint8_t			*sc_tx_block;	/* transmit block */
	void		       (*sc_tx_intr)(void *);	/* callback */
	void			*sc_tx_intrarg;	/* callback arg */
	void			*sc_tx_buf;	/* transmit buffer */
	int			 sc_tx_refcnt;	/* buffer refcnt */

	/* mixer data */
	int			 sc_vgs;	/* speaker volume */
	int			 sc_vgm;	/* mic volume */
};

/* sc_state */
#define BTHSET_CLOSED		0
#define BTHSET_WAIT_CONNECT	1
#define BTHSET_OPEN		2

/* sc_flags */
#define BTHSET_GETMTU		(1 << 0)

/* autoconf(9) glue */
static int  bthset_match(struct device *, struct cfdata *, void *);
static void bthset_attach(struct device *, struct device *, void *);
static int  bthset_detach(struct device *, int);

CFATTACH_DECL(bthset, sizeof(struct bthset_softc),
    bthset_match, bthset_attach, bthset_detach, NULL);

/* audio(9) glue */
static int bthset_open(void *, int);
static void bthset_close(void *);
static int bthset_query_encoding(void *, struct audio_encoding *);
static int bthset_set_params(void *, int, int, audio_params_t *, audio_params_t *,
				stream_filter_list_t *, stream_filter_list_t *);
static int bthset_round_blocksize(void *, int, int, const audio_params_t *);
static int bthset_start_output(void *, void *, int, void (*)(void *), void *);
static int bthset_start_input(void *, void *, int, void (*)(void *), void *);
static int bthset_halt_output(void *);
static int bthset_halt_input(void *);
static int bthset_getdev(void *, struct audio_device *);
static int bthset_setfd(void *, int);
static int bthset_set_port(void *, mixer_ctrl_t *);
static int bthset_get_port(void *, mixer_ctrl_t *);
static int bthset_query_devinfo(void *, mixer_devinfo_t *);
static void *bthset_allocm(void *, int, size_t, struct malloc_type *, int);
static void bthset_freem(void *, void *, struct malloc_type *);
static int bthset_get_props(void *);
static int bthset_dev_ioctl(void *, u_long, caddr_t, int, struct lwp *);

static const struct audio_hw_if bthset_if = {
	bthset_open,		/* open */
	bthset_close,		/* close */
	NULL,			/* drain */
	bthset_query_encoding,	/* query_encoding */
	bthset_set_params,	/* set_params */
	bthset_round_blocksize,	/* round_blocksize */
	NULL,			/* commit_settings */
	NULL,			/* init_output */
	NULL,			/* init_input */
	bthset_start_output,	/* start_output */
	bthset_start_input,	/* start_input */
	bthset_halt_output,	/* halt_output */
	bthset_halt_input,	/* halt_input */
	NULL,			/* speaker_ctl */
	bthset_getdev,		/* getdev */
	bthset_setfd,		/* setfd */
	bthset_set_port,	/* set_port */
	bthset_get_port,	/* get_port */
	bthset_query_devinfo,	/* query_devinfo */
	bthset_allocm,		/* allocm */
	bthset_freem,		/* freem */
	NULL,			/* round_buffersize */
	NULL,			/* mappage */
	bthset_get_props,	/* get_props */
	NULL,			/* trigger_output */
	NULL,			/* trigger_input */
	bthset_dev_ioctl,	/* dev_ioctl */
	NULL,			/* powerstate */
};

static const struct audio_device bthset_device = {
	"Bluetooth Audio",
	"",
	"bthset"
};

/* bluetooth(9) glue for SCO */
static void  bthset_sco_connecting(void *);
static void  bthset_sco_connected(void *);
static void  bthset_sco_disconnected(void *, int);
static void *bthset_sco_newconn(void *, struct sockaddr_bt *, struct sockaddr_bt *);
static void  bthset_sco_complete(void *, int);
static void  bthset_sco_input(void *, struct mbuf *);

static const struct btproto bthset_sco_proto = {
	bthset_sco_connecting,
	bthset_sco_connected,
	bthset_sco_disconnected,
	bthset_sco_newconn,
	bthset_sco_complete,
	bthset_sco_input,
};


/*****************************************************************************
 *
 *	bthset definitions
 */

/*
 * bthset mixer class
 */
#define BTHSET_VGS		0
#define BTHSET_VGM		1
#define BTHSET_INPUT_CLASS	2
#define BTHSET_OUTPUT_CLASS	3

/* connect timeout */
#define BTHSET_TIMEOUT		(30 * hz)

/* misc bthset functions */
static void bthset_extfree(struct mbuf *, caddr_t, size_t, void *);
static void bthset_intr(void *);


/*****************************************************************************
 *
 *	bthset autoconf(9) routines
 */

static int
bthset_match(struct device *self, struct cfdata *cfdata, void *aux)
{
	struct btdev_attach_args *bda = (struct btdev_attach_args *)aux;

	if (bda->bd_type != BTDEV_HSET
	    || bdaddr_any(&bda->bd_raddr)
	    || bda->bd_hset.hset_channel < RFCOMM_CHANNEL_MIN
	    || bda->bd_hset.hset_channel > RFCOMM_CHANNEL_MAX)
		return 0;

	return 1;
}

static void
bthset_attach(struct device *parent, struct device *self, void *aux)
{
	struct bthset_softc *sc = (struct bthset_softc *)self;
	struct btdev_attach_args *bda = (struct btdev_attach_args *)aux;

	/*
	 * Init softc
	 */
	sc->sc_vgs = 200;
	sc->sc_vgm = 200;
	sc->sc_state = BTHSET_CLOSED;

	/*
	 * copy in our configuration info
	 */
	bdaddr_copy(&sc->sc_laddr, &bda->bd_laddr);
	bdaddr_copy(&sc->sc_raddr, &bda->bd_raddr);
	sc->sc_channel = bda->bd_hset.hset_channel;
	sc->sc_mtu = bda->bd_hset.hset_mtu;
	if (sc->sc_mtu == 0)
		sc->sc_flags |= BTHSET_GETMTU;
	else
		aprint_verbose(" mtu %d", sc->sc_mtu);

	aprint_normal("\n");

	DPRINTF("sc=%p\n", sc);

	/*
	 * set up transmit interrupt
	 */
	sc->sc_intr = softintr_establish(IPL_SOFTNET, bthset_intr, sc);
	if (sc->sc_intr == NULL) {
		aprint_error("%s: softintr_establish failed\n", btdev_name(sc));
		return;
	}

	/*
	 * attach audio device
	 */
	sc->sc_audio = audio_attach_mi(&bthset_if, sc, &sc->sc_btdev.sc_dev);
	if (sc->sc_audio == NULL) {
		aprint_error("%s: audio_attach_mi failed\n", btdev_name(sc));
		return;
	}
}

static int
bthset_detach(struct device *self, int flags)
{
	struct bthset_softc *sc = (struct bthset_softc *)self;
	int s;

	DPRINTF("sc=%p\n", sc);

	if (sc->sc_sco != NULL) {
		DPRINTF("sc_sco=%p\n", sc->sc_sco);
		s = splsoftnet();
		sco_disconnect(sc->sc_sco, 0);
		sco_detach(&sc->sc_sco);
		sc->sc_sco = NULL;
		splx(s);
	}

	if (sc->sc_audio != NULL) {
		DPRINTF("sc_audio=%p\n", sc->sc_audio);
		config_detach(sc->sc_audio, flags);
		sc->sc_audio = NULL;
	}

	if (sc->sc_intr != NULL) {
		softintr_disestablish(sc->sc_intr);
		sc->sc_intr = NULL;
	}

	if (sc->sc_rx_mbuf != NULL) {
		m_freem(sc->sc_rx_mbuf);
		sc->sc_rx_mbuf = NULL;
	}

	if (sc->sc_tx_refcnt > 0) {
		printf("%s: tx_refcnt=%d!\n",
			btdev_name(sc), sc->sc_tx_refcnt);

		if ((flags & DETACH_FORCE) == 0)
			return EAGAIN;
	}

	return 0;
}


/*****************************************************************************
 *
 *	bluetooth(9) methods for SCO
 *
 *	All these are called from Bluetooth Protocol code, in a soft
 *	interrupt context at IPL_SOFTNET.
 */

static void
bthset_sco_connecting(void *arg)
{
//	struct bthset_softc *sc = arg;

	DPRINTF("%s\n", btdev_name(arg));

	/* dont care */
}

static void
bthset_sco_connected(void *arg)
{
	struct bthset_softc *sc = arg;

	DPRINTF("%s\n", btdev_name(sc));

	KASSERT(sc->sc_sco != NULL);
	KASSERT(sc->sc_state == BTHSET_WAIT_CONNECT);

	sc->sc_state = BTHSET_OPEN;
	wakeup(sc);
}

static void
bthset_sco_disconnected(void *arg, int err)
{
	struct bthset_softc *sc = arg;

	DPRINTF("%s sc_state %d\n", btdev_name(sc), sc->sc_state);

	KASSERT(sc->sc_sco != NULL);

	sc->sc_err = err;
	sco_detach(&sc->sc_sco);

	switch (sc->sc_state) {
	case BTHSET_CLOSED:		/* dont think this can happen */
		break;

	case BTHSET_WAIT_CONNECT:	/* connect failed */
		wakeup(sc);
		break;

	case BTHSET_OPEN:		/* link lost, XXX */
		break;

	default:
		UNKNOWN(sc->sc_state);
	}

	sc->sc_state = BTHSET_CLOSED;
}

static void *
bthset_sco_newconn(void *arg, struct sockaddr_bt *laddr, struct sockaddr_bt *raddr)
{
//	struct bthset_softc *sc = arg;

	DPRINTF("%s\n", btdev_name(arg));

	return NULL;
}

static void
bthset_sco_complete(void *arg, int count)
{
	struct bthset_softc *sc = arg;
	int s;

	DPRINTFN(10, "%s count %d\n", btdev_name(sc), count);

	s = splaudio();
	if (sc->sc_tx_pending > 0) {
		sc->sc_tx_pending -= count;
		if (sc->sc_tx_pending == 0)
			(*sc->sc_tx_intr)(sc->sc_tx_intrarg);
	}
	splx(s);
}

static void
bthset_sco_input(void *arg, struct mbuf *m)
{
	struct bthset_softc *sc = arg;
	int len, s;

	DPRINTFN(10, "%s len=%d\n", btdev_name(sc), m->m_pkthdr.len);

	s = splaudio();
	if (sc->sc_rx_want == 0) {
		m_freem(m);
	} else {
		KASSERT(sc->sc_rx_intr != NULL);
		KASSERT(sc->sc_rx_block != NULL);

		len = MIN(sc->sc_rx_want, m->m_pkthdr.len);
		m_copydata(m, 0, len, sc->sc_rx_block);

		sc->sc_rx_want -= len;
		sc->sc_rx_block += len;

		if (len > m->m_pkthdr.len) {
			if (sc->sc_rx_mbuf != NULL)
				m_freem(sc->sc_rx_mbuf);

			m_adj(m, len);
			sc->sc_rx_mbuf = m;
		} else {
			m_freem(m);
		}

		if (sc->sc_rx_want == 0)
			(*sc->sc_rx_intr)(sc->sc_rx_intrarg);
	}
	splx(s);
}


/*****************************************************************************
 *
 *	audio(9) methods
 *
 */

static int
bthset_open(void *hdl, int flags)
{
	struct sockaddr_bt sa;
	struct bthset_softc *sc = hdl;
	int err, s;

	DPRINTF("%s flags 0x%x\n", btdev_name(sc), flags);
	// flags FREAD & FWRITE?

	if (sc->sc_sco != NULL)
		return EIO;

	s = splsoftnet();

	err = sco_attach(&sc->sc_sco, &bthset_sco_proto, sc);
	if (err)
		goto done;

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, &sc->sc_laddr);

	err = sco_bind(sc->sc_sco, &sa);
	if (err) {
		sco_detach(&sc->sc_sco);
		goto done;
	}

	bdaddr_copy(&sa.bt_bdaddr, &sc->sc_raddr);
	err = sco_connect(sc->sc_sco, &sa);
	if (err) {
		sco_detach(&sc->sc_sco);
		goto done;
	}

	sc->sc_state = BTHSET_WAIT_CONNECT;
	while (err == 0 && sc->sc_state == BTHSET_WAIT_CONNECT)
		err = tsleep(sc, PWAIT | PCATCH, "bthset", BTHSET_TIMEOUT);

	switch (sc->sc_state) {
	case BTHSET_CLOSED:		/* disconnected */
		sco_detach(&sc->sc_sco);
		err = sc->sc_err;
		break;

	case BTHSET_WAIT_CONNECT:	/* error */
		sco_detach(&sc->sc_sco);
		break;

	case BTHSET_OPEN:		/* hurrah */
		if (sc->sc_flags & BTHSET_GETMTU)
			sco_getopt(sc->sc_sco, SO_SCO_MTU, &sc->sc_mtu);

		break;

	default:
		UNKNOWN(sc->sc_state);
		break;
	}

done:
	splx(s);

	DPRINTF("done err=%d, sc_state=%d, sc_mtu=%d\n",
			err, sc->sc_state, sc->sc_mtu);
	return err;
}

static void
bthset_close(void *hdl)
{
	struct bthset_softc *sc = hdl;
	int s;

	DPRINTF("%s\n", btdev_name(sc));

	if (sc->sc_sco != NULL) {
		s = splsoftnet();
		sco_disconnect(sc->sc_sco, 0);
		sco_detach(&sc->sc_sco);
		splx(s);
	}

	if (sc->sc_rx_mbuf != NULL) {
		m_freem(sc->sc_rx_mbuf);
		sc->sc_rx_mbuf = NULL;
	}

	sc->sc_rx_want = 0;
	sc->sc_rx_block = NULL;
	sc->sc_rx_intr = NULL;
	sc->sc_rx_intrarg = NULL;

	sc->sc_tx_size = 0;
	sc->sc_tx_block = NULL;
	sc->sc_tx_pending = 0;
	sc->sc_tx_intr = NULL;
	sc->sc_tx_intrarg = NULL;
}

static int
bthset_query_encoding(void *hdl, struct audio_encoding *ae)
{
//	struct bthset_softc *sc = hdl;
	int err = 0;

	DPRINTF("%s index %d\n", btdev_name(hdl), ae->index);

	switch (ae->index) {
	case 0:
		strcpy(ae->name, AudioEslinear_le);
		ae->encoding = AUDIO_ENCODING_SLINEAR_LE;
		ae->precision = 16;
		ae->flags = 0;
		break;

	default:
		err = EINVAL;
	}

	return err;
}

static int
bthset_set_params(void *hdl, int setmode, int usemode,
		audio_params_t *play, audio_params_t *rec,
		stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
//	struct bthset_softc *sc = hdl;
	audio_params_t hw;
	int err = 0;

	DPRINTF("%s setmode 0x%x usemode 0x%x\n", btdev_name(hdl), setmode, usemode);
	DPRINTF("rate %d, precision %d, channels %d encoding %d\n",
		play->sample_rate, play->precision, play->channels, play->encoding);

	if ((play->precision != 16 && play->precision != 8)
	    || play->sample_rate < 7500
	    || play->sample_rate > 8500
	    || play->channels != 1)
		return EINVAL;

	play->sample_rate = 8000;
	hw = *play;

	switch (play->encoding) {
	case AUDIO_ENCODING_ULAW:
		hw.encoding = AUDIO_ENCODING_SLINEAR_LE;
		pfil->append(pfil, mulaw_to_linear16, &hw);
		break;

	case AUDIO_ENCODING_ALAW:
		hw.encoding = AUDIO_ENCODING_SLINEAR_LE;
		pfil->append(pfil, alaw_to_linear16, &hw);
		break;

	case AUDIO_ENCODING_SLINEAR_LE:
		break;

	case AUDIO_ENCODING_SLINEAR_BE:
		if (play->precision == 16) {
			hw.encoding = AUDIO_ENCODING_SLINEAR_LE;
			pfil->append(pfil, swap_bytes, &hw);
		}
		break;

	case AUDIO_ENCODING_ULINEAR_LE:
	case AUDIO_ENCODING_ULINEAR_BE:
	default:
		err = EINVAL;
	}

	return err;
}

/*
 * If we have an MTU value to use, round the blocksize to that.
 */
static int
bthset_round_blocksize(void *hdl, int bs, int mode, const audio_params_t *param)
{
	struct bthset_softc *sc = hdl;

	if (sc->sc_mtu > 0)
		bs = (bs / sc->sc_mtu) * sc->sc_mtu;
	
	DPRINTF("%s mode=0x%x, bs=%d, sc_mtu=%d\n",
			btdev_name(sc), mode, bs, sc->sc_mtu);

	return bs;
}

/*
 * Start Output
 *
 * We dont want to be calling the network stack at splaudio() so make
 * a note of what is to be sent, and schedule an interrupt to bundle
 * it up and queue it.
 */
static int
bthset_start_output(void *hdl, void *block, int blksize,
		void (*intr)(void *), void *intrarg)
{
	struct bthset_softc *sc = hdl;

	DPRINTFN(5, "%s blksize %d\n", btdev_name(sc), blksize);

	sc->sc_tx_block = block;
	sc->sc_tx_pending = 0;
	sc->sc_tx_size = blksize;
	sc->sc_tx_intr = intr;
	sc->sc_tx_intrarg = intrarg;

	softintr_schedule(sc->sc_intr);
	return 0;
}

/*
 * Start Input
 *
 * When the SCO link is up, we are getting data in any case, so all we do
 * is note what we want and where to put it and let the sco_input routine
 * fill in the data.
 *
 * If there was any leftover data that didnt fit in the last block, retry 
 * it now.
 */
static int
bthset_start_input(void *hdl, void *block, int blksize,
		void (*intr)(void *), void *intrarg)
{
	struct bthset_softc *sc = hdl;
	struct mbuf *m;

	DPRINTFN(5, "%s blksize %d\n", btdev_name(sc), blksize);

	if (sc->sc_sco == NULL)
		return EINVAL;

	sc->sc_rx_want = blksize;
	sc->sc_rx_block = block;
	sc->sc_rx_intr = intr;
	sc->sc_rx_intrarg = intrarg;

	if (sc->sc_rx_mbuf != NULL) {
		m = sc->sc_rx_mbuf;
		sc->sc_rx_mbuf = NULL;
		bthset_sco_input(sc, m);
	}

	return 0;
}

/*
 * Halt Output
 *
 * This doesnt really halt the output, but it will look
 * that way to the audio driver. The current block will
 * still be transmitted.
 */
static int
bthset_halt_output(void *hdl)
{
	struct bthset_softc *sc = hdl;

	DPRINTFN(5, "%s\n", btdev_name(sc));

	sc->sc_tx_size = 0;
	sc->sc_tx_block = NULL;
	sc->sc_tx_pending = 0;
	sc->sc_tx_intr = NULL;
	sc->sc_tx_intrarg = NULL;

	return 0;
}

/*
 * Halt Input
 *
 * This doesnt really halt the input, but it will look
 * that way to the audio driver. Incoming data will be
 * discarded.
 */
static int
bthset_halt_input(void *hdl)
{
	struct bthset_softc *sc = hdl;

	DPRINTFN(5, "%s\n", btdev_name(sc));

	sc->sc_rx_want = 0;
	sc->sc_rx_block = NULL;
	sc->sc_rx_intr = NULL;
	sc->sc_rx_intrarg = NULL;

	if (sc->sc_rx_mbuf != NULL) {
		m_freem(sc->sc_rx_mbuf);
		sc->sc_rx_mbuf = NULL;
	}

	return 0;
}

static int
bthset_getdev(void *hdl, struct audio_device *ret)
{

	*ret = bthset_device;
	return 0;
}

static int
bthset_setfd(void *hdl, int fd)
{
//	struct bthset_softc *sc = hdl;

	DPRINTF("%s set %s duplex\n", btdev_name(hdl), fd ? "full" : "half");

	return 0;
}

static int
bthset_set_port(void *hdl, mixer_ctrl_t *mc)
{
	struct bthset_softc *sc = hdl;
	int err = 0;

	DPRINTF("%s dev %d type %d\n", btdev_name(sc), mc->dev, mc->type);

	switch (mc->dev) {
	case BTHSET_VGS:
		if (mc->type != AUDIO_MIXER_VALUE ||
		    mc->un.value.num_channels != 1) {
			err = EINVAL;
			break;
		}

		sc->sc_vgs = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		break;

	case BTHSET_VGM:
		if (mc->type != AUDIO_MIXER_VALUE ||
		    mc->un.value.num_channels != 1) {
			err = EINVAL;
			break;
		}

		sc->sc_vgm = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		break;

	default:
		err = EINVAL;
		break;
	}

	return err;
}

static int
bthset_get_port(void *hdl, mixer_ctrl_t *mc)
{
	struct bthset_softc *sc = hdl;
	int err = 0;

	DPRINTF("%s dev %d\n", btdev_name(sc), mc->dev);

	switch (mc->dev) {
	case BTHSET_VGS:
		mc->type = AUDIO_MIXER_VALUE;
		mc->un.value.num_channels = 1;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_vgs;
		break;

	case BTHSET_VGM:
		mc->type = AUDIO_MIXER_VALUE;
		mc->un.value.num_channels = 1;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_vgm;
		break;

	default:
		err = EINVAL;
		break;
	}

	return err;
}

static int
bthset_query_devinfo(void *hdl, mixer_devinfo_t *di)
{
//	struct bthset_softc *sc = hdl;
	int err = 0;

	DPRINTF("%s index %d\n", btdev_name(hdl), di->index);

	switch(di->index) {
	case BTHSET_VGS:
		di->mixer_class = BTHSET_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNspeaker);
		di->type = AUDIO_MIXER_VALUE;
		strcpy(di->un.v.units.name, AudioNvolume);
		di->un.v.num_channels = 1;
		di->un.v.delta = BTHSET_DELTA;
		break;

	case BTHSET_VGM:
		di->mixer_class = BTHSET_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNmicrophone);
		di->type = AUDIO_MIXER_VALUE;
		strcpy(di->un.v.units.name, AudioNvolume);
		di->un.v.num_channels = 1;
		di->un.v.delta = BTHSET_DELTA;
		break;

	case BTHSET_INPUT_CLASS:
		di->mixer_class = BTHSET_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		break;

	default:
		err = ENXIO;
		break;
	}

	return err;
}

/*
 * Allocate Ring Buffers.
 */
static void *
bthset_allocm(void *hdl, int direction, size_t size,
		struct malloc_type *type, int flags)
{
	struct bthset_softc *sc = hdl;
	void *addr;

	DPRINTF("%s: size %d direction %d\n",
			btdev_name(hdl), size, direction);

	addr = malloc(size, type, flags);

	if (direction == AUMODE_PLAY) {
		sc->sc_tx_buf = addr;
		sc->sc_tx_refcnt = 0;
	}

	return addr;
}

/*
 * Free Ring Buffers.
 *
 * Because we used external memory for the tx mbufs, we dont
 * want to free the memory until all the mbufs are done with
 *
 * Just to be sure, dont free if something is still pending.
 * This would be a memory leak but at least there is a warning..
 */
static void
bthset_freem(void *hdl, void *addr, struct malloc_type *type)
{
	struct bthset_softc *sc = hdl;
	int count = hz / 2;

	if (addr == sc->sc_tx_buf) {
		DPRINTF("%s: tx_refcnt=%d\n",
			btdev_name(sc), sc->sc_tx_refcnt);

		sc->sc_tx_buf = NULL;

		while (sc->sc_tx_refcnt> 0 && count-- > 0)
			tsleep(sc, PWAIT, "drain", 1);

		if (sc->sc_tx_refcnt > 0) {
			printf("%s: ring buffer unreleased!\n", btdev_name(sc));
			return;
		}
	}

	free(addr, type);
}

static int
bthset_get_props(void *hdl)
{

	return AUDIO_PROP_FULLDUPLEX;
}

/*
 * Handle private ioctl. We pass information out about how to talk
 * to the device and mixer.
 */
static int
bthset_dev_ioctl(void *hdl, u_long cmd, caddr_t addr, int flag, struct lwp *l)
{
	struct bthset_softc *sc = hdl;
	struct bthset_info *bi = (struct bthset_info *)addr;
	int err = 0;

	DPRINTF("%s cmd 0x%lx flag %d\n", btdev_name(sc), cmd, flag);

	switch (cmd) {
	case BTHSET_GETINFO:
		memset(bi, 0, sizeof(*bi));
		bdaddr_copy(&bi->laddr, &sc->sc_laddr);
		bdaddr_copy(&bi->raddr, &sc->sc_raddr);
		bi->channel = sc->sc_channel;
		bi->vgs = BTHSET_VGS;
		bi->vgm = BTHSET_VGM;
		break;

	default:
		err = EPASSTHROUGH;
		break;
	}

	return err;
}


/*****************************************************************************
 *
 *	misc bthset functions
 *
 */

/*
 * Our transmit interrupt. This is triggered when a new block is to be
 * sent.  We send mtu sized chunks of the block as mbufs with external
 * storage to sco_send()
 */
static void
bthset_intr(void *arg)
{
	struct bthset_softc *sc = arg;
	struct mbuf *m;
	uint8_t *block;
	int mlen, size;

	DPRINTFN(10, "%s block %p size %d\n",
		btdev_name(sc), sc->sc_tx_block, sc->sc_tx_size);

	block = sc->sc_tx_block;
	size = sc->sc_tx_size;
	sc->sc_tx_block = NULL;
	sc->sc_tx_size = 0;

	while (size > 0) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL)
			break;

		mlen = MIN(sc->sc_mtu, size);

		/* I think M_DEVBUF is true but not relevant */
		MEXTADD(m, block, mlen, M_DEVBUF, bthset_extfree, sc);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			break;
		}
		sc->sc_tx_refcnt++;

		m->m_pkthdr.len = m->m_len = mlen;
		sc->sc_tx_pending++;

		if (sco_send(sc->sc_sco, m) > 0) {
			sc->sc_tx_pending--;
			m_free(m);
			break;
		}

		block += mlen;
		size -= mlen;
	}
}

/*
 * Release the mbuf, we keep a reference count on the tx buffer so
 * that we dont release it before its free.
 */
static void
bthset_extfree(struct mbuf *m, caddr_t addr, size_t size, void *arg)
{
	struct bthset_softc *sc = arg;

	if (m != NULL)
		pool_cache_put(&mbpool_cache, m);

	sc->sc_tx_refcnt--;
}
