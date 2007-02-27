/*	$NetBSD: audio.c,v 1.217.2.1 2007/02/27 16:53:46 yamt Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
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
 * This is a (partially) SunOS-compatible /dev/audio driver for NetBSD.
 *
 * This code tries to do something half-way sensible with
 * half-duplex hardware, such as with the SoundBlaster hardware.  With
 * half-duplex hardware allowing O_RDWR access doesn't really make
 * sense.  However, closing and opening the device to "turn around the
 * line" is relatively expensive and costs a card reset (which can
 * take some time, at least for the SoundBlaster hardware).  Instead
 * we allow O_RDWR access, and provide an ioctl to set the "mode",
 * i.e. playing or recording.
 *
 * If you write to a half-duplex device in record mode, the data is
 * tossed.  If you read from the device in play mode, you get silence
 * filled buffers at the rate at which samples are naturally
 * generated.
 *
 * If you try to set both play and record mode on a half-duplex
 * device, playing takes precedence.
 */

/*
 * Todo:
 * - Add softaudio() isr processing for wakeup, poll, signals,
 *   and silence fill.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: audio.c,v 1.217.2.1 2007/02/27 16:53:46 yamt Exp $");

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/audioio.h>
#include <sys/device.h>

#include <dev/audio_if.h>
#include <dev/audiovar.h>

#include <machine/endian.h>

/* #define AUDIO_DEBUG	1 */
#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (audiodebug) printf x
#define DPRINTFN(n,x)	if (audiodebug>(n)) printf x
int	audiodebug = AUDIO_DEBUG;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define ROUNDSIZE(x)	x &= -16	/* round to nice boundary */
#define SPECIFIED(x)	(x != ~0)
#define SPECIFIED_CH(x)	(x != (u_char)~0)

int	audio_blk_ms = AUDIO_BLK_MS;

int	audiosetinfo(struct audio_softc *, struct audio_info *);
int	audiogetinfo(struct audio_softc *, struct audio_info *);

int	audio_open(dev_t, struct audio_softc *, int, int, struct lwp *);
int	audio_close(struct audio_softc *, int, int, struct lwp *);
int	audio_read(struct audio_softc *, struct uio *, int);
int	audio_write(struct audio_softc *, struct uio *, int);
int	audio_ioctl(struct audio_softc *, u_long, caddr_t, int, struct lwp *);
int	audio_poll(struct audio_softc *, int, struct lwp *);
int	audio_kqfilter(struct audio_softc *, struct knote *);
paddr_t	audio_mmap(struct audio_softc *, off_t, int);

int	mixer_open(dev_t, struct audio_softc *, int, int, struct lwp *);
int	mixer_close(struct audio_softc *, int, int, struct lwp *);
int	mixer_ioctl(struct audio_softc *, u_long, caddr_t, int, struct lwp *);
static	void mixer_remove(struct audio_softc *, struct lwp *);
static	void mixer_signal(struct audio_softc *);

void	audio_init_record(struct audio_softc *);
void	audio_init_play(struct audio_softc *);
int	audiostartr(struct audio_softc *);
int	audiostartp(struct audio_softc *);
void	audio_rint(void *);
void	audio_pint(void *);
int	audio_check_params(struct audio_params *);

void	audio_calc_blksize(struct audio_softc *, int);
void	audio_fill_silence(struct audio_params *, uint8_t *, int);
int	audio_silence_copyout(struct audio_softc *, int, struct uio *);

void	audio_init_ringbuffer(struct audio_softc *,
			      struct audio_ringbuffer *, int);
int	audio_initbufs(struct audio_softc *);
void	audio_calcwater(struct audio_softc *);
static inline int audio_sleep_timo(int *, const char *, int);
static inline int audio_sleep(int *, const char *);
static inline void audio_wakeup(int *);
int	audio_drain(struct audio_softc *);
void	audio_clear(struct audio_softc *);
static inline void audio_pint_silence
	(struct audio_softc *, struct audio_ringbuffer *, uint8_t *, int);

int	audio_alloc_ring
	(struct audio_softc *, struct audio_ringbuffer *, int, size_t);
void	audio_free_ring(struct audio_softc *, struct audio_ringbuffer *);
static int audio_setup_pfilters(struct audio_softc *, const audio_params_t *,
				stream_filter_list_t *);
static int audio_setup_rfilters(struct audio_softc *, const audio_params_t *,
				stream_filter_list_t *);
static void audio_destruct_pfilters(struct audio_softc *);
static void audio_destruct_rfilters(struct audio_softc *);
static void audio_stream_dtor(audio_stream_t *);
static int audio_stream_ctor(audio_stream_t *, const audio_params_t *, int);
static void stream_filter_list_append
	(stream_filter_list_t *, stream_filter_factory_t,
	 const audio_params_t *);
static void stream_filter_list_prepend
	(stream_filter_list_t *, stream_filter_factory_t,
	 const audio_params_t *);
static void stream_filter_list_set
	(stream_filter_list_t *, int, stream_filter_factory_t,
	 const audio_params_t *);
int	audio_set_defaults(struct audio_softc *, u_int);

int	audioprobe(struct device *, struct cfdata *, void *);
void	audioattach(struct device *, struct device *, void *);
int	audiodetach(struct device *, int);
int	audioactivate(struct device *, enum devact);

void	audio_powerhook(int, void *);

static void	audio_softintr_rd(void *);
static void	audio_softintr_wr(void *);

struct portname {
	const char *name;
	int mask;
};
static const struct portname itable[] = {
	{ AudioNmicrophone,	AUDIO_MICROPHONE },
	{ AudioNline,		AUDIO_LINE_IN },
	{ AudioNcd,		AUDIO_CD },
	{ 0, 0 }
};
static const struct portname otable[] = {
	{ AudioNspeaker,	AUDIO_SPEAKER },
	{ AudioNheadphone,	AUDIO_HEADPHONE },
	{ AudioNline,		AUDIO_LINE_OUT },
	{ 0, 0 }
};
void	au_setup_ports(struct audio_softc *, struct au_mixer_ports *,
			mixer_devinfo_t *, const struct portname *);
int	au_set_gain(struct audio_softc *, struct au_mixer_ports *,
			int, int);
void	au_get_gain(struct audio_softc *, struct au_mixer_ports *,
			u_int *, u_char *);
int	au_set_port(struct audio_softc *, struct au_mixer_ports *,
			u_int);
int	au_get_port(struct audio_softc *, struct au_mixer_ports *);
int	au_get_lr_value(struct audio_softc *, mixer_ctrl_t *,
			int *, int *);
int	au_set_lr_value(struct audio_softc *, mixer_ctrl_t *,
			int, int);
int	au_portof(struct audio_softc *, char *, int);

typedef struct uio_fetcher {
	stream_fetcher_t base;
	struct uio *uio;
	int usedhigh;
	int last_used;
} uio_fetcher_t;

static void	uio_fetcher_ctor(uio_fetcher_t *, struct uio *, int);
static int	uio_fetcher_fetch_to(stream_fetcher_t *,
				     audio_stream_t *, int);
static int	null_fetcher_fetch_to(stream_fetcher_t *,
				      audio_stream_t *, int);

dev_type_open(audioopen);
dev_type_close(audioclose);
dev_type_read(audioread);
dev_type_write(audiowrite);
dev_type_ioctl(audioioctl);
dev_type_poll(audiopoll);
dev_type_mmap(audiommap);
dev_type_kqfilter(audiokqfilter);

const struct cdevsw audio_cdevsw = {
	audioopen, audioclose, audioread, audiowrite, audioioctl,
	nostop, notty, audiopoll, audiommap, audiokqfilter, D_OTHER
};

/* The default audio mode: 8 kHz mono mu-law */
const struct audio_params audio_default = {
	.sample_rate = 8000,
	.encoding = AUDIO_ENCODING_ULAW,
	.precision = 8,
	.validbits = 8,
	.channels = 1,
};

CFATTACH_DECL(audio, sizeof(struct audio_softc),
    audioprobe, audioattach, audiodetach, audioactivate);

extern struct cfdriver audio_cd;

int
audioprobe(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct audio_attach_args *sa;

	sa = aux;
	DPRINTF(("audioprobe: type=%d sa=%p hw=%p\n",
		 sa->type, sa, sa->hwif));
	return (sa->type == AUDIODEV_TYPE_AUDIO) ? 1 : 0;
}

void
audioattach(struct device *parent, struct device *self, void *aux)
{
	struct audio_softc *sc;
	struct audio_attach_args *sa;
	const struct audio_hw_if *hwp;
	void *hdlp;
	int error;
	mixer_devinfo_t mi;
	int iclass, mclass, oclass, rclass, props;
	int record_master_found, record_source_found;

	sc = (void *)self;
	sa = aux;
	hwp = sa->hwif;
	hdlp = sa->hdl;
#ifdef DIAGNOSTIC
	if (hwp == 0 ||
	    hwp->query_encoding == 0 ||
	    hwp->set_params == 0 ||
	    (hwp->start_output == 0 && hwp->trigger_output == 0) ||
	    (hwp->start_input == 0 && hwp->trigger_input == 0) ||
	    hwp->halt_output == 0 ||
	    hwp->halt_input == 0 ||
	    hwp->getdev == 0 ||
	    hwp->set_port == 0 ||
	    hwp->get_port == 0 ||
	    hwp->query_devinfo == 0 ||
	    hwp->get_props == 0) {
		printf(": missing method\n");
		sc->hw_if = 0;
		return;
	}
#endif

	props = hwp->get_props(hdlp);

	if (props & AUDIO_PROP_FULLDUPLEX)
		printf(": full duplex");
	else
		printf(": half duplex");

	if (props & AUDIO_PROP_MMAP)
		printf(", mmap");
	if (props & AUDIO_PROP_INDEPENDENT)
		printf(", independent");

	printf("\n");

	sc->hw_if = hwp;
	sc->hw_hdl = hdlp;
	sc->sc_dev = parent;
	sc->sc_opencnt = 0;
	sc->sc_writing = sc->sc_waitcomp = 0;

	error = audio_alloc_ring(sc, &sc->sc_pr, AUMODE_PLAY, AU_RING_SIZE);
	if (error) {
		sc->hw_if = NULL;
		printf("audio: could not allocate play buffer\n");
		return;
	}
	error = audio_alloc_ring(sc, &sc->sc_rr, AUMODE_RECORD, AU_RING_SIZE);
	if (error) {
		audio_free_ring(sc, &sc->sc_pr);
		sc->hw_if = NULL;
		printf("audio: could not allocate record buffer\n");
		return;
	}

	if ((error = audio_set_defaults(sc, 0))) {
		printf("audioattach: audio_set_defaults() failed\n");
		sc->hw_if = NULL;
		return;
	}

	sc->sc_sih_rd = softintr_establish(IPL_SOFTSERIAL,
	    audio_softintr_rd, sc);
	sc->sc_sih_wr = softintr_establish(IPL_SOFTSERIAL,
	    audio_softintr_wr, sc);

	iclass = mclass = oclass = rclass = -1;
	sc->sc_inports.index = -1;
	sc->sc_inports.master = -1;
	sc->sc_inports.nports = 0;
	sc->sc_inports.isenum = FALSE;
	sc->sc_inports.allports = 0;
	sc->sc_inports.isdual = FALSE;
	sc->sc_inports.mixerout = -1;
	sc->sc_inports.cur_port = -1;
	sc->sc_outports.index = -1;
	sc->sc_outports.master = -1;
	sc->sc_outports.nports = 0;
	sc->sc_outports.isenum = FALSE;
	sc->sc_outports.allports = 0;
	sc->sc_outports.isdual = FALSE;
	sc->sc_outports.mixerout = -1;
	sc->sc_outports.cur_port = -1;
	sc->sc_monitor_port = -1;
	/*
	 * Read through the underlying driver's list, picking out the class
	 * names from the mixer descriptions. We'll need them to decode the
	 * mixer descriptions on the next pass through the loop.
	 */
	for(mi.index = 0; ; mi.index++) {
		if (hwp->query_devinfo(hdlp, &mi) != 0)
			break;
		 /*
		  * The type of AUDIO_MIXER_CLASS merely introduces a class.
		  * All the other types describe an actual mixer.
		  */
		if (mi.type == AUDIO_MIXER_CLASS) {
			if (strcmp(mi.label.name, AudioCinputs) == 0)
				iclass = mi.mixer_class;
			if (strcmp(mi.label.name, AudioCmonitor) == 0)
				mclass = mi.mixer_class;
			if (strcmp(mi.label.name, AudioCoutputs) == 0)
				oclass = mi.mixer_class;
			if (strcmp(mi.label.name, AudioCrecord) == 0)
				rclass = mi.mixer_class;
		}
	}
	/*
	 * This is where we assign each control in the "audio" model, to the
	 * underlying "mixer" control.  We walk through the whole list once,
	 * assigning likely candidates as we come across them.
	 */
	record_master_found = 0;
	record_source_found = 0;
	for(mi.index = 0; ; mi.index++) {
		if (hwp->query_devinfo(hdlp, &mi) != 0)
			break;
		if (mi.type == AUDIO_MIXER_CLASS)
			continue;
		if (mi.mixer_class == iclass) {
			/*
			 * AudioCinputs is only a fallback, when we don't
			 * find what we're looking for in AudioCrecord, so
			 * check the flags before accepting one of these.
			 */
			if (strcmp(mi.label.name, AudioNmaster) == 0
			    && record_master_found == 0)
				sc->sc_inports.master = mi.index;
			if (strcmp(mi.label.name, AudioNsource) == 0
			    && record_source_found == 0) {
				if (mi.type == AUDIO_MIXER_ENUM) {
				    int i;
				    for(i = 0; i < mi.un.e.num_mem; i++)
					if (strcmp(mi.un.e.member[i].label.name,
						    AudioNmixerout) == 0)
						sc->sc_inports.mixerout =
						    mi.un.e.member[i].ord;
				}
				au_setup_ports(sc, &sc->sc_inports, &mi,
				    itable);
			}
		} else if (mi.mixer_class == mclass) {
			if (strcmp(mi.label.name, AudioNmonitor) == 0)
				sc->sc_monitor_port = mi.index;
		} else if (mi.mixer_class == oclass) {
			if (strcmp(mi.label.name, AudioNmaster) == 0)
				sc->sc_outports.master = mi.index;
			if (strcmp(mi.label.name, AudioNselect) == 0)
				au_setup_ports(sc, &sc->sc_outports, &mi,
				    otable);
		} else if (mi.mixer_class == rclass) {
			/*
			 * These are the preferred mixers for the audio record
			 * controls, so set the flags here, but don't check.
			 */
			if (strcmp(mi.label.name, AudioNmaster) == 0) {
				sc->sc_inports.master = mi.index;
				record_master_found = 1;
			}
#if 1	/* Deprecated. Use AudioNmaster. */
			if (strcmp(mi.label.name, AudioNrecord) == 0) {
				sc->sc_inports.master = mi.index;
				record_master_found = 1;
			}
			if (strcmp(mi.label.name, AudioNvolume) == 0) {
				sc->sc_inports.master = mi.index;
				record_master_found = 1;
			}
#endif
			if (strcmp(mi.label.name, AudioNsource) == 0) {
				if (mi.type == AUDIO_MIXER_ENUM) {
				    int i;
				    for(i = 0; i < mi.un.e.num_mem; i++)
					if (strcmp(mi.un.e.member[i].label.name,
						    AudioNmixerout) == 0)
						sc->sc_inports.mixerout =
						    mi.un.e.member[i].ord;
				}
				au_setup_ports(sc, &sc->sc_inports, &mi,
				    itable);
				record_source_found = 1;
			}
		}
	}
	DPRINTF(("audio_attach: inputs ports=0x%x, input master=%d, "
		 "output ports=0x%x, output master=%d\n",
		 sc->sc_inports.allports, sc->sc_inports.master,
		 sc->sc_outports.allports, sc->sc_outports.master));

	sc->sc_powerhook = powerhook_establish(sc->dev.dv_xname,
	    audio_powerhook, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: can't establish powerhook\n",
		    sc->dev.dv_xname);
}

int
audioactivate(struct device *self, enum devact act)
{
	struct audio_softc *sc;

	sc = (struct audio_softc *)self;
	switch (act) {
	case DVACT_ACTIVATE:
		return EOPNOTSUPP;

	case DVACT_DEACTIVATE:
		sc->sc_dying = TRUE;
		break;
	}
	return 0;
}

int
audiodetach(struct device *self, int flags)
{
	struct audio_softc *sc;
	int maj, mn;
	int s;

	sc = (struct audio_softc *)self;
	DPRINTF(("audio_detach: sc=%p flags=%d\n", sc, flags));

	sc->sc_dying = TRUE;

	wakeup(&sc->sc_wchan);
	wakeup(&sc->sc_rchan);
	s = splaudio();
	if (--sc->sc_refcnt >= 0) {
		if (tsleep(&sc->sc_refcnt, PZERO, "auddet", hz * 120))
			printf("audiodetach: %s didn't detach\n",
			       sc->dev.dv_xname);
	}
	splx(s);

	/* free resources */
	audio_free_ring(sc, &sc->sc_pr);
	audio_free_ring(sc, &sc->sc_rr);
	audio_destruct_pfilters(sc);
	audio_destruct_rfilters(sc);

	/* locate the major number */
	maj = cdevsw_lookup_major(&audio_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = device_unit(self);
	vdevgone(maj, mn | SOUND_DEVICE,    mn | SOUND_DEVICE, VCHR);
	vdevgone(maj, mn | AUDIO_DEVICE,    mn | AUDIO_DEVICE, VCHR);
	vdevgone(maj, mn | AUDIOCTL_DEVICE, mn | AUDIOCTL_DEVICE, VCHR);
	vdevgone(maj, mn | MIXER_DEVICE,    mn | MIXER_DEVICE, VCHR);

	if (sc->sc_powerhook != NULL) {
		powerhook_disestablish(sc->sc_powerhook);
		sc->sc_powerhook = NULL;
	}
	if (sc->sc_sih_rd) {
		softintr_disestablish(sc->sc_sih_rd);
		sc->sc_sih_rd = NULL;
	}
	if (sc->sc_sih_wr) {
		softintr_disestablish(sc->sc_sih_wr);
		sc->sc_sih_wr = NULL;
	}

	return 0;
}

int
au_portof(struct audio_softc *sc, char *name, int class)
{
	mixer_devinfo_t mi;

	for(mi.index = 0;
	    sc->hw_if->query_devinfo(sc->hw_hdl, &mi) == 0;
	    mi.index++)
		if (mi.mixer_class == class && strcmp(mi.label.name, name) == 0)
			return mi.index;
	return -1;
}

void
au_setup_ports(struct audio_softc *sc, struct au_mixer_ports *ports,
	       mixer_devinfo_t *mi, const struct portname *tbl)
{
	int i, j;

	ports->index = mi->index;
	if (mi->type == AUDIO_MIXER_ENUM) {
		ports->isenum = TRUE;
		for(i = 0; tbl[i].name; i++)
		    for(j = 0; j < mi->un.e.num_mem; j++)
			if (strcmp(mi->un.e.member[j].label.name,
							    tbl[i].name) == 0) {
				ports->allports |= tbl[i].mask;
				ports->aumask[ports->nports] = tbl[i].mask;
				ports->misel[ports->nports] =
				    mi->un.e.member[j].ord;
				ports->miport[ports->nports] =
				    au_portof(sc, mi->un.e.member[j].label.name,
				    mi->mixer_class);
				if (ports->mixerout != -1 &&
				    ports->miport[ports->nports++] != -1)
					ports->isdual = TRUE;
			}
	} else if (mi->type == AUDIO_MIXER_SET) {
		for(i = 0; tbl[i].name; i++)
		    for(j = 0; j < mi->un.s.num_mem; j++)
			if (strcmp(mi->un.s.member[j].label.name,
							    tbl[i].name) == 0) {
				ports->allports |= tbl[i].mask;
				ports->aumask[ports->nports] = tbl[i].mask;
				ports->misel[ports->nports] =
				    mi->un.s.member[j].mask;
				ports->miport[ports->nports++] =
				    au_portof(sc, mi->un.s.member[j].label.name,
				    mi->mixer_class);
			}
	}
}

/*
 * Called from hardware driver.  This is where the MI audio driver gets
 * probed/attached to the hardware driver.
 */
struct device *
audio_attach_mi(const struct audio_hw_if *ahwp, void *hdlp, struct device *dev)
{
	struct audio_attach_args arg;

#ifdef DIAGNOSTIC
	if (ahwp == NULL) {
		aprint_error("audio_attach_mi: NULL\n");
		return 0;
	}
#endif
	arg.type = AUDIODEV_TYPE_AUDIO;
	arg.hwif = ahwp;
	arg.hdl = hdlp;
	return config_found(dev, &arg, audioprint);
}

#ifdef AUDIO_DEBUG
void	audio_printsc(struct audio_softc *);
void	audio_print_params(const char *, struct audio_params *);

void
audio_printsc(struct audio_softc *sc)
{
	printf("hwhandle %p hw_if %p ", sc->hw_hdl, sc->hw_if);
	printf("open 0x%x mode 0x%x\n", sc->sc_open, sc->sc_mode);
	printf("rchan 0x%x wchan 0x%x ", sc->sc_rchan, sc->sc_wchan);
	printf("rring used 0x%x pring used=%d\n",
	       audio_stream_get_used(&sc->sc_rr.s),
	       audio_stream_get_used(&sc->sc_pr.s));
	printf("rbus 0x%x pbus 0x%x ", sc->sc_rbus, sc->sc_pbus);
	printf("blksize %d", sc->sc_pr.blksize);
	printf("hiwat %d lowat %d\n", sc->sc_pr.usedhigh, sc->sc_pr.usedlow);
}

void
audio_print_params(const char *s, struct audio_params *p)
{
	printf("%s enc=%u %uch %u/%ubit %uHz\n", s, p->encoding, p->channels,
	       p->validbits, p->precision, p->sample_rate);
}
#endif

int
audio_alloc_ring(struct audio_softc *sc, struct audio_ringbuffer *r,
		 int direction, size_t bufsize)
{
	const struct audio_hw_if *hw;
	void *hdl;

	hw = sc->hw_if;
	hdl = sc->hw_hdl;
	/*
	 * Alloc DMA play and record buffers
	 */
	if (bufsize < AUMINBUF)
		bufsize = AUMINBUF;
	ROUNDSIZE(bufsize);
	if (hw->round_buffersize)
		bufsize = hw->round_buffersize(hdl, direction, bufsize);
	if (hw->allocm)
		r->s.start = hw->allocm(hdl, direction, bufsize,
					M_DEVBUF, M_WAITOK);
	else
		r->s.start = malloc(bufsize, M_DEVBUF, M_WAITOK);
	if (r->s.start == 0)
		return ENOMEM;
	r->s.bufsize = bufsize;
	return 0;
}

void
audio_free_ring(struct audio_softc *sc, struct audio_ringbuffer *r)
{

	if (sc->hw_if->freem)
		sc->hw_if->freem(sc->hw_hdl, r->s.start, M_DEVBUF);
	else
		free(r->s.start, M_DEVBUF);
	r->s.start = 0;
}

static int
audio_setup_pfilters(struct audio_softc *sc, const audio_params_t *pp,
		     stream_filter_list_t *pfilters)
{
	stream_filter_t *pf[AUDIO_MAX_FILTERS];
	audio_stream_t ps[AUDIO_MAX_FILTERS];
	const audio_params_t *from_param;
	audio_params_t *to_param;
	int i, n;

	while (sc->sc_writing) {
		sc->sc_waitcomp = 1;
		(void)tsleep(sc, 0, "audioch", 10*hz);
	}

	memset(pf, 0, sizeof(pf));
	memset(ps, 0, sizeof(ps));
	from_param = pp;
	for (i = 0; i < pfilters->req_size; i++) {
		n = pfilters->req_size - i - 1;
		to_param = &pfilters->filters[n].param;
		audio_check_params(to_param);
		pf[i] = pfilters->filters[n].factory(sc, from_param, to_param);
		if (pf[i] == NULL)
			break;
		if (audio_stream_ctor(&ps[i], from_param, AU_RING_SIZE))
			break;
		if (i > 0)
			pf[i]->set_fetcher(pf[i], &pf[i - 1]->base);
		from_param = to_param;
	}
	if (i < pfilters->req_size) { /* failure */
		DPRINTF(("%s: pfilters failure\n", __func__));
		for (; i >= 0; i--) {
			if (pf[i] != NULL)
				pf[i]->dtor(pf[i]);
			audio_stream_dtor(&ps[i]);
		}
		sc->sc_waitcomp = 0;
		return EINVAL;
	}

	audio_destruct_pfilters(sc);
	memcpy(sc->sc_pfilters, pf, sizeof(pf));
	memcpy(sc->sc_pstreams, ps, sizeof(ps));
	sc->sc_npfilters = pfilters->req_size;
	for (i = 0; i < pfilters->req_size; i++) {
		pf[i]->set_inputbuffer(pf[i], &sc->sc_pstreams[i]);
	}

	/* hardware format and the buffer near to userland */
	if (pfilters->req_size <= 0) {
		sc->sc_pr.s.param = *pp;
		sc->sc_pustream = &sc->sc_pr.s;
	} else {
		sc->sc_pr.s.param = pfilters->filters[0].param;
		sc->sc_pustream = &sc->sc_pstreams[0];
	}
#ifdef AUDIO_DEBUG
	printf("%s: HW-buffer=%p pustream=%p\n",
	       __func__, &sc->sc_pr.s, sc->sc_pustream);
	for (i = 0; i < pfilters->req_size; i++) {
		char num[100];
		snprintf(num, 100, "[%d]", i);
		audio_print_params(num, &sc->sc_pstreams[i].param);
	}
	audio_print_params("[HW]", &sc->sc_pr.s.param);
#endif /* AUDIO_DEBUG */

	sc->sc_waitcomp = 0;
	return 0;
}

static int
audio_setup_rfilters(struct audio_softc *sc, const audio_params_t *rp,
		     stream_filter_list_t *rfilters)
{
	stream_filter_t *rf[AUDIO_MAX_FILTERS];
	audio_stream_t rs[AUDIO_MAX_FILTERS];
	const audio_params_t *to_param;
	audio_params_t *from_param;
	int i;

	memset(rf, 0, sizeof(rf));
	memset(rs, 0, sizeof(rs));
	for (i = 0; i < rfilters->req_size; i++) {
		from_param = &rfilters->filters[i].param;
		audio_check_params(from_param);
		to_param = i + 1 < rfilters->req_size
			? &rfilters->filters[i + 1].param : rp;
		rf[i] = rfilters->filters[i].factory(sc, from_param, to_param);
		if (rf[i] == NULL)
			break;
		if (audio_stream_ctor(&rs[i], to_param, AU_RING_SIZE))
			break;
		if (i > 0) {
			rf[i]->set_fetcher(rf[i], &rf[i - 1]->base);
		} else {
			/* rf[0] has no previous fetcher because
			 * the audio hardware fills data to the
			 * input buffer. */
			rf[0]->set_inputbuffer(rf[0], &sc->sc_rr.s);
		}
	}
	if (i < rfilters->req_size) { /* failure */
		DPRINTF(("%s: rfilters failure\n", __func__));
		for (; i >= 0; i--) {
			if (rf[i] != NULL)
				rf[i]->dtor(rf[i]);
			audio_stream_dtor(&rs[i]);
		}
		return EINVAL;
	}

	audio_destruct_rfilters(sc);
	memcpy(sc->sc_rfilters, rf, sizeof(rf));
	memcpy(sc->sc_rstreams, rs, sizeof(rs));
	sc->sc_nrfilters = rfilters->req_size;
	for (i = 1; i < rfilters->req_size; i++) {
		rf[i]->set_inputbuffer(rf[i], &sc->sc_rstreams[i - 1]);
	}

	/* hardware format and the buffer near to userland */
	if (rfilters->req_size <= 0) {
		sc->sc_rr.s.param = *rp;
		sc->sc_rustream = &sc->sc_rr.s;
	} else {
		sc->sc_rr.s.param = rfilters->filters[0].param;
		sc->sc_rustream = &sc->sc_rstreams[rfilters->req_size - 1];
	}
#ifdef AUDIO_DEBUG
	printf("%s: HW-buffer=%p pustream=%p\n",
	       __func__, &sc->sc_rr.s, sc->sc_rustream);
	audio_print_params("[HW]", &sc->sc_rr.s.param);
	for (i = 0; i < rfilters->req_size; i++) {
		char num[100];
		snprintf(num, 100, "[%d]", i);
		audio_print_params(num, &sc->sc_rstreams[i].param);
	}
#endif /* AUDIO_DEBUG */
	return 0;
}

static void
audio_destruct_pfilters(struct audio_softc *sc)
{
	int i;

	for (i = 0; i < sc->sc_npfilters; i++) {
		sc->sc_pfilters[i]->dtor(sc->sc_pfilters[i]);
		sc->sc_pfilters[i] = NULL;
		audio_stream_dtor(&sc->sc_pstreams[i]);
	}
	sc->sc_npfilters = 0;
}

static void
audio_destruct_rfilters(struct audio_softc *sc)
{
	int i;

	for (i = 0; i < sc->sc_nrfilters; i++) {
		sc->sc_rfilters[i]->dtor(sc->sc_rfilters[i]);
		sc->sc_rfilters[i] = NULL;
		audio_stream_dtor(&sc->sc_rstreams[i]);
	}
	sc->sc_nrfilters = 0;
}

static void
audio_stream_dtor(audio_stream_t *stream)
{

	if (stream->start != NULL)
		free(stream->start, M_DEVBUF);
	memset(stream, 0, sizeof(audio_stream_t));
}

static int
audio_stream_ctor(audio_stream_t *stream, const audio_params_t *param, int size)
{
	int frame_size;

	size = min(size, AU_RING_SIZE);
	stream->bufsize = size;
	stream->start = malloc(size, M_DEVBUF, M_WAITOK);
	if (stream->start == NULL)
		return ENOMEM;
	frame_size = (param->precision + 7) / 8 * param->channels;
	size = (size / frame_size) * frame_size;
	stream->end = stream->start + size;
	stream->inp = stream->start;
	stream->outp = stream->start;
	stream->used = 0;
	stream->param = *param;
	stream->loop = FALSE;
	return 0;
}

static void
stream_filter_list_append(stream_filter_list_t *list,
			  stream_filter_factory_t factory,
			  const audio_params_t *param)
{

	if (list->req_size >= AUDIO_MAX_FILTERS) {
		printf("%s: increase AUDIO_MAX_FILTERS in sys/dev/audio_if.h\n",
		       __func__);
		return;
	}
	list->filters[list->req_size].factory = factory;
	list->filters[list->req_size].param = *param;
	list->req_size++;
}

static void
stream_filter_list_set(stream_filter_list_t *list, int i,
		       stream_filter_factory_t factory,
		       const audio_params_t *param)
{

	if (i < 0 || i >= AUDIO_MAX_FILTERS) {
		printf("%s: invalid index: %d\n", __func__, i);
		return;
	}

	list->filters[i].factory = factory;
	list->filters[i].param = *param;
	if (list->req_size <= i)
		list->req_size = i + 1;
}

static void
stream_filter_list_prepend(stream_filter_list_t *list,
			   stream_filter_factory_t factory,
			   const audio_params_t *param)
{

	if (list->req_size >= AUDIO_MAX_FILTERS) {
		printf("%s: increase AUDIO_MAX_FILTERS in sys/dev/audio_if.h\n",
		       __func__);
		return;
	}
	memmove(&list->filters[1], &list->filters[0],
		sizeof(struct stream_filter_req) * list->req_size);
	list->filters[0].factory = factory;
	list->filters[0].param = *param;
	list->req_size++;
}

int
audioopen(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct audio_softc *sc;
	int error;

	sc = device_lookup(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_dying)
		return EIO;

	if (sc->hw_if->powerstate && sc->sc_opencnt <= 0)
		sc->hw_if->powerstate(sc->hw_hdl, AUDIOPOWER_ON);
	sc->sc_opencnt++;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_open(dev, sc, flags, ifmt, l);
		break;
	case AUDIOCTL_DEVICE:
		error = 0;
		break;
	case MIXER_DEVICE:
		error = mixer_open(dev, sc, flags, ifmt, l);
		break;
	default:
		error = ENXIO;
		break;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return error;
}

int
audioclose(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct audio_softc *sc;
	int unit;
	int error;

	unit = AUDIOUNIT(dev);
	sc = audio_cd.cd_devs[unit];
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_close(sc, flags, ifmt, l);
		break;
	case MIXER_DEVICE:
		error = mixer_close(sc, flags, ifmt, l);
		break;
	case AUDIOCTL_DEVICE:
		error = 0;
		break;
	default:
		error = ENXIO;
		break;
	}

	sc->sc_opencnt--;
	if (sc->hw_if->powerstate && sc->sc_opencnt <= 0)
		sc->hw_if->powerstate(sc->hw_hdl, AUDIOPOWER_OFF);

	return error;
}

int
audioread(dev_t dev, struct uio *uio, int ioflag)
{
	struct audio_softc *sc;
	int error;

	sc = device_lookup(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_dying)
		return EIO;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_read(sc, uio, ioflag);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return error;
}

int
audiowrite(dev_t dev, struct uio *uio, int ioflag)
{
	struct audio_softc *sc;
	int error;

	sc = device_lookup(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_dying)
		return EIO;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_write(sc, uio, ioflag);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return error;
}

int
audioioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct lwp *l)
{
	struct audio_softc *sc;
	int error;

	sc = audio_cd.cd_devs[AUDIOUNIT(dev)];
	if (sc->sc_dying)
		return EIO;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
	case AUDIOCTL_DEVICE:
		error = audio_ioctl(sc, cmd, addr, flag, l);
		break;
	case MIXER_DEVICE:
		error = mixer_ioctl(sc, cmd, addr, flag, l);
		break;
	default:
		error = ENXIO;
		break;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return error;
}

int
audiopoll(dev_t dev, int events, struct lwp *l)
{
	struct audio_softc *sc;
	int revents;

	sc = audio_cd.cd_devs[AUDIOUNIT(dev)];
	if (sc->sc_dying)
		return POLLHUP;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		revents = audio_poll(sc, events, l);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		revents = 0;
		break;
	default:
		revents = POLLERR;
		break;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return revents;
}

int
audiokqfilter(dev_t dev, struct knote *kn)
{
	struct audio_softc *sc;
	int rv;

	sc = audio_cd.cd_devs[AUDIOUNIT(dev)];
	if (sc->sc_dying)
		return 1;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		rv = audio_kqfilter(sc, kn);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		rv = 1;
		break;
	default:
		rv = 1;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return rv;
}

paddr_t
audiommap(dev_t dev, off_t off, int prot)
{
	struct audio_softc *sc;
	paddr_t error;

	sc = audio_cd.cd_devs[AUDIOUNIT(dev)];
	if (sc->sc_dying)
		return -1;

	sc->sc_refcnt++;
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_mmap(sc, off, prot);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = -1;
		break;
	default:
		error = -1;
		break;
	}
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);
	return error;
}

/*
 * Audio driver
 */
void
audio_init_ringbuffer(struct audio_softc *sc, struct audio_ringbuffer *rp,
		      int mode)
{
	int nblks;
	int blksize;

	blksize = rp->blksize;
	if (blksize < AUMINBLK)
		blksize = AUMINBLK;
	if (blksize > rp->s.bufsize / AUMINNOBLK)
		blksize = rp->s.bufsize / AUMINNOBLK;
	ROUNDSIZE(blksize);
	DPRINTF(("audio_init_ringbuffer: MI blksize=%d\n", blksize));
	if (sc->hw_if->round_blocksize)
		blksize = sc->hw_if->round_blocksize(sc->hw_hdl, blksize,
						     mode, &rp->s.param);
	if (blksize <= 0)
		panic("audio_init_ringbuffer: blksize");
	nblks = rp->s.bufsize / blksize;

	DPRINTF(("audio_init_ringbuffer: final blksize=%d\n", blksize));
	rp->blksize = blksize;
	rp->maxblks = nblks;
	rp->s.end = rp->s.start + nblks * blksize;
	rp->s.outp = rp->s.inp = rp->s.start;
	rp->s.used = 0;
	rp->stamp = 0;
	rp->stamp_last = 0;
	rp->fstamp = 0;
	rp->drops = 0;
	rp->pause = FALSE;
	rp->copying = FALSE;
	rp->needfill = FALSE;
	rp->mmapped = FALSE;
}

int
audio_initbufs(struct audio_softc *sc)
{
	const struct audio_hw_if *hw;
	int error;

	DPRINTF(("audio_initbufs: mode=0x%x\n", sc->sc_mode));
	hw = sc->hw_if;
	audio_init_ringbuffer(sc, &sc->sc_rr, AUMODE_RECORD);
	if (hw->init_input && (sc->sc_mode & AUMODE_RECORD)) {
		error = hw->init_input(sc->hw_hdl, sc->sc_rr.s.start,
				       sc->sc_rr.s.end - sc->sc_rr.s.start);
		if (error)
			return error;
	}

	audio_init_ringbuffer(sc, &sc->sc_pr, AUMODE_PLAY);
	sc->sc_sil_count = 0;
	if (hw->init_output && (sc->sc_mode & AUMODE_PLAY)) {
		error = hw->init_output(sc->hw_hdl, sc->sc_pr.s.start,
					sc->sc_pr.s.end - sc->sc_pr.s.start);
		if (error)
			return error;
	}

#ifdef AUDIO_INTR_TIME
#define double u_long
	sc->sc_pnintr = 0;
	sc->sc_pblktime = (u_long)(
	    (double)sc->sc_pr.blksize * 100000 /
	    (double)(sc->sc_pparams.precision / NBBY *
		     sc->sc_pparams.channels *
		     sc->sc_pparams.sample_rate)) * 10;
	DPRINTF(("audio: play blktime = %lu for %d\n",
		 sc->sc_pblktime, sc->sc_pr.blksize));
	sc->sc_rnintr = 0;
	sc->sc_rblktime = (u_long)(
	    (double)sc->sc_rr.blksize * 100000 /
	    (double)(sc->sc_rparams.precision / NBBY *
		     sc->sc_rparams.channels *
		     sc->sc_rparams.sample_rate)) * 10;
	DPRINTF(("audio: record blktime = %lu for %d\n",
		 sc->sc_rblktime, sc->sc_rr.blksize));
#undef double
#endif

	return 0;
}

void
audio_calcwater(struct audio_softc *sc)
{

	/* set high at 100% */
	sc->sc_pr.usedhigh = sc->sc_pustream->end - sc->sc_pustream->start;
	/* set low at 75% of usedhigh */
	sc->sc_pr.usedlow = sc->sc_pr.usedhigh * 3 / 4;
	if (sc->sc_pr.usedlow == sc->sc_pr.usedhigh)
		sc->sc_pr.usedlow -= sc->sc_pr.blksize;

	sc->sc_rr.usedhigh = sc->sc_rustream->end - sc->sc_rustream->start
		- sc->sc_rr.blksize;
	sc->sc_rr.usedlow = 0;
	DPRINTF(("%s: plow=%d phigh=%d rlow=%d rhigh=%d\n", __func__,
		 sc->sc_pr.usedlow, sc->sc_pr.usedhigh,
		 sc->sc_rr.usedlow, sc->sc_rr.usedhigh));
}

static inline int
audio_sleep_timo(int *chan, const char *label, int timo)
{
	int st;

	if (label == NULL)
		label = "audio";

	DPRINTFN(3, ("audio_sleep_timo: chan=%p, label=%s, timo=%d\n",
		     chan, label, timo));
	*chan = 1;
	st = tsleep(chan, PWAIT | PCATCH, label, timo);
	*chan = 0;
#ifdef AUDIO_DEBUG
	if (st != 0 && st != EINTR)
	    DPRINTF(("audio_sleep: woke up st=%d\n", st));
#endif
	return st;
}

static inline int
audio_sleep(int *chan, const char *label)
{

	return audio_sleep_timo(chan, label, 0);
}

/* call at splaudio() */
static inline void
audio_wakeup(int *chan)
{

	DPRINTFN(3, ("audio_wakeup: chan=%p, *chan=%d\n", chan, *chan));
	if (*chan) {
		wakeup(chan);
		*chan = 0;
	}
}

int
audio_open(dev_t dev, struct audio_softc *sc, int flags, int ifmt,
    struct lwp *l)
{
	int error;
	u_int mode;
	const struct audio_hw_if *hw;

	hw = sc->hw_if;
	if (hw == NULL)
		return ENXIO;

	DPRINTF(("audio_open: flags=0x%x sc=%p hdl=%p\n",
		 flags, sc, sc->hw_hdl));

	if (((flags & FREAD) && (sc->sc_open & AUOPEN_READ)) ||
	    ((flags & FWRITE) && (sc->sc_open & AUOPEN_WRITE)))
		return EBUSY;

	if (hw->open != NULL) {
		error = hw->open(sc->hw_hdl, flags);
		if (error)
			return error;
	}

	sc->sc_async_audio = 0;
	sc->sc_rchan = 0;
	sc->sc_wchan = 0;
	sc->sc_sil_count = 0;
	sc->sc_rbus = FALSE;
	sc->sc_pbus = FALSE;
	sc->sc_eof = 0;
	sc->sc_playdrop = 0;

	sc->sc_full_duplex = 
		(flags & (FWRITE|FREAD)) == (FWRITE|FREAD) &&
		(hw->get_props(sc->hw_hdl) & AUDIO_PROP_FULLDUPLEX);

	mode = 0;
	if (flags & FREAD) {
		sc->sc_open |= AUOPEN_READ;
		mode |= AUMODE_RECORD;
	}
	if (flags & FWRITE) {
		sc->sc_open |= AUOPEN_WRITE;
		mode |= AUMODE_PLAY | AUMODE_PLAY_ALL;
	}

	/*
	 * Multiplex device: /dev/audio (MU-Law) and /dev/sound (linear)
	 * The /dev/audio is always (re)set to 8-bit MU-Law mono
	 * For the other devices, you get what they were last set to.
	 */
	if (ISDEVAUDIO(dev)) {
		error = audio_set_defaults(sc, mode);
	} else {
		struct audio_info ai;

		AUDIO_INITINFO(&ai);
		ai.mode = mode;
		error = audiosetinfo(sc, &ai);
	}
	if (error)
		goto bad;

#ifdef DIAGNOSTIC
	/*
	 * Sample rate and precision are supposed to be set to proper
	 * default values by the hardware driver, so that it may give
	 * us these values.
	 */
	if (sc->sc_rparams.precision == 0 || sc->sc_pparams.precision == 0) {
		printf("audio_open: 0 precision\n");
		return EINVAL;
	}
#endif

	/* audio_close() decreases sc_pr.usedlow, recalculate here */
	audio_calcwater(sc);

	DPRINTF(("audio_open: done sc_mode = 0x%x\n", sc->sc_mode));

	return 0;

bad:
	if (hw->close != NULL)
		hw->close(sc->hw_hdl);
	sc->sc_open = 0;
	sc->sc_mode = 0;
	sc->sc_full_duplex = 0;
	return error;
}

/*
 * Must be called from task context.
 */
void
audio_init_record(struct audio_softc *sc)
{
	int s;

	s = splaudio();
	if (sc->hw_if->speaker_ctl &&
	    (!sc->sc_full_duplex || (sc->sc_mode & AUMODE_PLAY) == 0))
		sc->hw_if->speaker_ctl(sc->hw_hdl, SPKR_OFF);
	splx(s);
}

/*
 * Must be called from task context.
 */
void
audio_init_play(struct audio_softc *sc)
{
	int s;

	s = splaudio();
	sc->sc_wstamp = sc->sc_pr.stamp;
	if (sc->hw_if->speaker_ctl)
		sc->hw_if->speaker_ctl(sc->hw_hdl, SPKR_ON);
	splx(s);
}

int
audio_drain(struct audio_softc *sc)
{
	struct audio_ringbuffer *cb;
	int error, drops;
	int s;
	int i, used;

	DPRINTF(("audio_drain: enter busy=%d\n", sc->sc_pbus));
	cb = &sc->sc_pr;
	if (cb->mmapped)
		return 0;

	used = audio_stream_get_used(&sc->sc_pr.s);
	s = splaudio();
	for (i = 0; i < sc->sc_npfilters; i++)
		used += audio_stream_get_used(&sc->sc_pstreams[i]);
	splx(s);
	if (used <= 0)
		return 0;

	if (!sc->sc_pbus) {
		/* We've never started playing, probably because the
		 * block was too short.  Pad it and start now.
		 */
		int cc;
		uint8_t *inp = cb->s.inp;

		cc = cb->blksize - (inp - cb->s.start) % cb->blksize;
		audio_fill_silence(&cb->s.param, inp, cc);
		s = splaudio();
		cb->s.inp = audio_stream_add_inp(&cb->s, inp, cc);
		error = audiostartp(sc);
		splx(s);
		if (error)
			return error;
	}
	/*
	 * Play until a silence block has been played, then we
	 * know all has been drained.
	 * XXX This should be done some other way to avoid
	 * playing silence.
	 */
#ifdef DIAGNOSTIC
	if (cb->copying) {
		printf("audio_drain: copying in progress!?!\n");
		cb->copying = FALSE;
	}
#endif
	drops = cb->drops;
	error = 0;
	s = splaudio();
	while (cb->drops == drops && !error) {
		DPRINTF(("audio_drain: used=%d, drops=%ld\n",
			 audio_stream_get_used(&sc->sc_pr.s), cb->drops));
		/*
		 * When the process is exiting, it ignores all signals and
		 * we can't interrupt this sleep, so we set a timeout
		 * just in case.
		 */
		error = audio_sleep_timo(&sc->sc_wchan, "aud_dr", 30*hz);
		if (sc->sc_dying)
			error = EIO;
	}
	splx(s);
	return error;
}

/*
 * Close an audio chip.
 */
/* ARGSUSED */
int
audio_close(struct audio_softc *sc, int flags, int ifmt,
    struct lwp *l)
{
	const struct audio_hw_if *hw;
	int s;

	DPRINTF(("audio_close: sc=%p\n", sc));
	hw = sc->hw_if;
	s = splaudio();
	/* Stop recording. */
	if ((flags & FREAD) && sc->sc_rbus) {
		/*
		 * XXX Some drivers (e.g. SB) use the same routine
		 * to halt input and output so don't halt input if
		 * in full duplex mode.  These drivers should be fixed.
		 */
		if (!sc->sc_full_duplex || hw->halt_input != hw->halt_output)
			hw->halt_input(sc->hw_hdl);
		sc->sc_rbus = FALSE;
	}
	/*
	 * Block until output drains, but allow ^C interrupt.
	 */
	sc->sc_pr.usedlow = sc->sc_pr.blksize;	/* avoid excessive wakeups */
	/*
	 * If there is pending output, let it drain (unless
	 * the output is paused).
	 */
	if ((flags & FWRITE) && sc->sc_pbus) {
		if (!sc->sc_pr.pause && !audio_drain(sc) && hw->drain)
			(void)hw->drain(sc->hw_hdl);
		hw->halt_output(sc->hw_hdl);
		sc->sc_pbus = FALSE;
	}

	if (hw->close != NULL)
		hw->close(sc->hw_hdl);

	sc->sc_open = 0;
	sc->sc_mode = 0;
	sc->sc_async_audio = 0;
	sc->sc_full_duplex = 0;
	splx(s);
	DPRINTF(("audio_close: done\n"));

	return 0;
}

int
audio_read(struct audio_softc *sc, struct uio *uio, int ioflag)
{
	struct audio_ringbuffer *cb;
	const uint8_t *outp;
	uint8_t *inp;
	int error, s, used, cc, n;

	cb = &sc->sc_rr;
	if (cb->mmapped)
		return EINVAL;

	DPRINTFN(1,("audio_read: cc=%zu mode=%d\n",
		    uio->uio_resid, sc->sc_mode));

	error = 0;
	/*
	 * If hardware is half-duplex and currently playing, return
	 * silence blocks based on the number of blocks we have output.
	 */
	if (!sc->sc_full_duplex && (sc->sc_mode & AUMODE_PLAY)) {
		while (uio->uio_resid > 0 && !error) {
			s = splaudio();
			for(;;) {
				cc = sc->sc_pr.stamp - sc->sc_wstamp;
				if (cc > 0)
					break;
				DPRINTF(("audio_read: stamp=%lu, wstamp=%lu\n",
					 sc->sc_pr.stamp, sc->sc_wstamp));
				if (ioflag & IO_NDELAY) {
					splx(s);
					return EWOULDBLOCK;
				}
				error = audio_sleep(&sc->sc_rchan, "aud_hr");
				if (sc->sc_dying)
					error = EIO;
				if (error) {
					splx(s);
					return error;
				}
			}
			splx(s);

			if (uio->uio_resid < cc)
				cc = uio->uio_resid;
			DPRINTFN(1,("audio_read: reading in write mode, "
				    "cc=%d\n", cc));
			error = audio_silence_copyout(sc, cc, uio);
			sc->sc_wstamp += cc;
		}
		return error;
	}

	while (uio->uio_resid > 0 && !error) {
		s = splaudio();
		while ((used = audio_stream_get_used(sc->sc_rustream)) <= 0) {
			if (!sc->sc_rbus) {
				error = audiostartr(sc);
				if (error) {
					splx(s);
					return error;
				}
			}
			if (ioflag & IO_NDELAY) {
				splx(s);
				return EWOULDBLOCK;
			}
			DPRINTFN(2, ("audio_read: sleep used=%d\n", used));
			error = audio_sleep(&sc->sc_rchan, "aud_rd");
			if (sc->sc_dying)
				error = EIO;
			if (error) {
				splx(s);
				return error;
			}
		}

		outp = sc->sc_rustream->outp;
		inp = sc->sc_rustream->inp;
		cb->copying = TRUE;
		splx(s);

		/*
		 * cc is the amount of data in the sc_rustream excluding
		 * wrapped data
		 */
		cc = outp <= inp ? inp - outp :sc->sc_rustream->end - outp;
		DPRINTFN(1,("audio_read: outp=%p, cc=%d\n", outp, cc));

		n = uio->uio_resid;
		error = uiomove(__UNCONST(outp), cc, uio);
		n -= uio->uio_resid; /* number of bytes actually moved */

		s = splaudio();
		sc->sc_rustream->outp = audio_stream_add_outp
			(sc->sc_rustream, outp, n);
		cb->copying = FALSE;
		splx(s);
	}
	return error;
}

void
audio_clear(struct audio_softc *sc)
{
	int s;

	s = splaudio();
	if (sc->sc_rbus) {
		audio_wakeup(&sc->sc_rchan);
		sc->hw_if->halt_input(sc->hw_hdl);
		sc->sc_rbus = FALSE;
	}
	if (sc->sc_pbus) {
		audio_wakeup(&sc->sc_wchan);
		sc->hw_if->halt_output(sc->hw_hdl);
		sc->sc_pbus = FALSE;
	}
	splx(s);
}

void
audio_calc_blksize(struct audio_softc *sc, int mode)
{
	const audio_params_t *parm;
	struct audio_ringbuffer *rb;

	if (sc->sc_blkset)
		return;

	if (mode == AUMODE_PLAY) {
		rb = &sc->sc_pr;
		parm = &rb->s.param;
	} else {
		rb = &sc->sc_rr;
		parm = &rb->s.param;
	}

	rb->blksize = parm->sample_rate * audio_blk_ms / 1000 *
	     parm->channels * parm->precision / NBBY;

	DPRINTF(("audio_calc_blksize: %s blksize=%d\n",
		 mode == AUMODE_PLAY ? "play" : "record", rb->blksize));
}

void
audio_fill_silence(struct audio_params *params, uint8_t *p, int n)
{
	uint8_t auzero0, auzero1;
	int nfill;

	auzero1 = 0;		/* initialize to please gcc */
	nfill = 1;
	switch (params->encoding) {
	case AUDIO_ENCODING_ULAW:
		auzero0 = 0x7f;
		break;
	case AUDIO_ENCODING_ALAW:
		auzero0 = 0x55;
		break;
	case AUDIO_ENCODING_MPEG_L1_STREAM:
	case AUDIO_ENCODING_MPEG_L1_PACKETS:
	case AUDIO_ENCODING_MPEG_L1_SYSTEM:
	case AUDIO_ENCODING_MPEG_L2_STREAM:
	case AUDIO_ENCODING_MPEG_L2_PACKETS:
	case AUDIO_ENCODING_MPEG_L2_SYSTEM:
	case AUDIO_ENCODING_ADPCM: /* is this right XXX */
	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_SLINEAR_BE:
		auzero0 = 0;/* fortunately this works for any number of bits */
		break;
	case AUDIO_ENCODING_ULINEAR_LE:
	case AUDIO_ENCODING_ULINEAR_BE:
		if (params->precision > 8) {
			nfill = (params->precision + NBBY - 1)/ NBBY;
			auzero0 = 0x80;
			auzero1 = 0;
		} else
			auzero0 = 0x80;
		break;
	default:
		DPRINTF(("audio: bad encoding %d\n", params->encoding));
		auzero0 = 0;
		break;
	}
	if (nfill == 1) {
		while (--n >= 0)
			*p++ = auzero0; /* XXX memset */
	} else /* nfill must no longer be 2 */ {
		if (params->encoding == AUDIO_ENCODING_ULINEAR_LE) {
			int k = nfill;
			while (--k > 0)
				*p++ = auzero1;
			n -= nfill - 1;
		}
		while (n >= nfill) {
			int k = nfill;
			*p++ = auzero0;
			while (--k > 0)
				*p++ = auzero1;

			n -= nfill;
		}
		if (n-- > 0)	/* XXX must be 1 - DIAGNOSTIC check? */
			*p++ = auzero0;
	}
}

int
audio_silence_copyout(struct audio_softc *sc, int n, struct uio *uio)
{
	uint8_t zerobuf[128];
	int error;
	int k;

	audio_fill_silence(&sc->sc_rparams, zerobuf, sizeof zerobuf);

	error = 0;
	while (n > 0 && uio->uio_resid > 0 && !error) {
		k = min(n, min(uio->uio_resid, sizeof zerobuf));
		error = uiomove(zerobuf, k, uio);
		n -= k;
	}
	return error;
}

static int
uio_fetcher_fetch_to(stream_fetcher_t *self, audio_stream_t *p,
    int max_used)
{
	uio_fetcher_t *this;
	int size;
	int stream_space;
	int error;

	this = (uio_fetcher_t *)self;
	this->last_used = audio_stream_get_used(p);
	if (this->last_used >= this->usedhigh)
		return 0;
	/*
	 * uio_fetcher ignores max_used and move the data as
	 * much as possible in order to return the correct value
	 * for audio_prinfo::seek and kfilters.
	 */
	stream_space = audio_stream_get_space(p);
	size = min(this->uio->uio_resid, stream_space);

	/* the first fragment of the space */
	stream_space = p->end - p->inp;
	if (stream_space >= size) {
		error = uiomove(p->inp, size, this->uio);
		if (error)
			return error;
		p->inp = audio_stream_add_inp(p, p->inp, size);
	} else {
		error = uiomove(p->inp, stream_space, this->uio);
		if (error)
			return error;
		p->inp = audio_stream_add_inp(p, p->inp, stream_space);
		error = uiomove(p->start, size - stream_space, this->uio);
		if (error)
			return error;
		p->inp = audio_stream_add_inp(p, p->inp, size - stream_space);
	}
	this->last_used = audio_stream_get_used(p);
	return 0;
}

static int
null_fetcher_fetch_to(stream_fetcher_t *self,
    audio_stream_t *p, int max_used)
{

	return 0;
}

static void
uio_fetcher_ctor(uio_fetcher_t *this, struct uio *u, int h)
{

	this->base.fetch_to = uio_fetcher_fetch_to;
	this->uio = u;
	this->usedhigh = h;
}

int
audio_write(struct audio_softc *sc, struct uio *uio, int ioflag)
{
	uio_fetcher_t ufetcher;
	audio_stream_t stream;
	struct audio_ringbuffer *cb;
	stream_fetcher_t *fetcher;
	stream_filter_t *filter;
	uint8_t *inp, *einp;
	int saveerror, error, s, n, cc, used;

	DPRINTFN(2,("audio_write: sc=%p count=%zu used=%d(hi=%d)\n",
		    sc, uio->uio_resid, audio_stream_get_used(sc->sc_pustream),
		    sc->sc_pr.usedhigh));
	cb = &sc->sc_pr;
	if (cb->mmapped)
		return EINVAL;

	if (uio->uio_resid == 0) {
		sc->sc_eof++;
		return 0;
	}

	/*
	 * If half-duplex and currently recording, throw away data.
	 */
	if (!sc->sc_full_duplex &&
	    (sc->sc_mode & AUMODE_RECORD)) {
		uio->uio_offset += uio->uio_resid;
		uio->uio_resid = 0;
		DPRINTF(("audio_write: half-dpx read busy\n"));
		return 0;
	}

	if (!(sc->sc_mode & AUMODE_PLAY_ALL) && sc->sc_playdrop > 0) {
		n = min(sc->sc_playdrop, uio->uio_resid);
		DPRINTF(("audio_write: playdrop %d\n", n));
		uio->uio_offset += n;
		uio->uio_resid -= n;
		sc->sc_playdrop -= n;
		if (uio->uio_resid == 0)
			return 0;
	}

	/**
	 * setup filter pipeline
	 */
	uio_fetcher_ctor(&ufetcher, uio, cb->usedhigh);
	if (sc->sc_npfilters > 0) {
		fetcher = &sc->sc_pfilters[sc->sc_npfilters - 1]->base;
	} else {
		fetcher = &ufetcher.base;
	}

	error = 0;
	while (uio->uio_resid > 0 && !error) {
		s = splaudio();
		/* wait if the first buffer is occupied */
		while ((used = audio_stream_get_used(sc->sc_pustream))
		    >= cb->usedhigh) {
			DPRINTFN(2, ("audio_write: sleep used=%d lowat=%d "
				     "hiwat=%d\n", used,
				     cb->usedlow, cb->usedhigh));
			if (ioflag & IO_NDELAY) {
				splx(s);
				return EWOULDBLOCK;
			}
			error = audio_sleep(&sc->sc_wchan, "aud_wr");
			if (sc->sc_dying)
				error = EIO;
			if (error) {
				splx(s);
				return error;
			}
		}
		inp = cb->s.inp;
		cb->copying = TRUE;
		stream = cb->s;
		used = stream.used;
		splx(s);

		/*
		 * write to the sc_pustream as much as possible
		 *
		 * work with a temporary audio_stream_t to narrow
		 * splaudio() enclosure
		 */

		sc->sc_writing = 1;

		if (sc->sc_npfilters > 0) {
			filter = sc->sc_pfilters[0];
			filter->set_fetcher(filter, &ufetcher.base);
			fetcher = &sc->sc_pfilters[sc->sc_npfilters - 1]->base;
			cc = cb->blksize * 2;
			error = fetcher->fetch_to(fetcher, &stream, cc);
			if (error != 0) {
				fetcher = &ufetcher.base;
				cc = sc->sc_pustream->end - sc->sc_pustream->start;
				error = fetcher->fetch_to(fetcher, sc->sc_pustream, cc);
			}
		} else {
			fetcher = &ufetcher.base;
			cc = stream.end - stream.start;
			error = fetcher->fetch_to(fetcher, &stream, cc);
		}
		sc->sc_writing = 0;
		if (sc->sc_waitcomp)
			wakeup(sc);

		s = splaudio();
		if (sc->sc_npfilters > 0) {
			cb->fstamp += ufetcher.last_used
			    - audio_stream_get_used(sc->sc_pustream);
		}
		cb->s.used += stream.used - used;
		cb->s.inp = stream.inp;
		einp = cb->s.inp;

		/*
		 * This is a very suboptimal way of keeping track of
		 * silence in the buffer, but it is simple.
		 */
		sc->sc_sil_count = 0;

		/*
		 * If the interrupt routine wants the last block filled AND
		 * the copy did not fill the last block completely it needs to
		 * be padded.
		 */
		if (cb->needfill && inp < einp &&
		    (inp  - cb->s.start) / cb->blksize ==
		    (einp - cb->s.start) / cb->blksize) {
			/* Figure out how many bytes to a block boundary. */
			cc = cb->blksize - (einp - cb->s.start) % cb->blksize;
			DPRINTF(("audio_write: partial fill %d\n", cc));
		} else
			cc = 0;
		cb->needfill = FALSE;
		cb->copying = FALSE;
		if (!sc->sc_pbus && !cb->pause) {
			saveerror = error;
			error = audiostartp(sc);
			if (saveerror != 0) {
				/* Report the first error that occurred. */
				error = saveerror;
			}
		}
		splx(s);
		if (cc != 0) {
			DPRINTFN(1, ("audio_write: fill %d\n", cc));
			audio_fill_silence(&cb->s.param, einp, cc);
		}
	}

	return error;
}

int
audio_ioctl(struct audio_softc *sc, u_long cmd, caddr_t addr, int flag,
	    struct lwp *l)
{
	const struct audio_hw_if *hw;
	struct audio_offset *ao;
	u_long stamp;
	int error, s, offs, fd;
	bool rbus, pbus;

	DPRINTF(("audio_ioctl(%lu,'%c',%lu)\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff));
	hw = sc->hw_if;
	error = 0;
	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;

	case FIONREAD:
		*(int *)addr = audio_stream_get_used(sc->sc_rustream);
		break;

	case FIOASYNC:
		if (*(int *)addr) {
			if (sc->sc_async_audio)
				return EBUSY;
			sc->sc_async_audio = l->l_proc;
			DPRINTF(("audio_ioctl: FIOASYNC %p\n", l->l_proc));
		} else
			sc->sc_async_audio = 0;
		break;

	case AUDIO_FLUSH:
		DPRINTF(("AUDIO_FLUSH\n"));
		rbus = sc->sc_rbus;
		pbus = sc->sc_pbus;
		audio_clear(sc);
		s = splaudio();
		error = audio_initbufs(sc);
		if (error) {
			splx(s);
			return error;
		}
		if ((sc->sc_mode & AUMODE_PLAY) && !sc->sc_pbus && pbus)
			error = audiostartp(sc);
		if (!error &&
		    (sc->sc_mode & AUMODE_RECORD) && !sc->sc_rbus && rbus)
			error = audiostartr(sc);
		splx(s);
		break;

	/*
	 * Number of read (write) samples dropped.  We don't know where or
	 * when they were dropped.
	 */
	case AUDIO_RERROR:
		*(int *)addr = sc->sc_rr.drops;
		break;

	case AUDIO_PERROR:
		*(int *)addr = sc->sc_pr.drops;
		break;

	/*
	 * Offsets into buffer.
	 */
	case AUDIO_GETIOFFS:
		ao = (struct audio_offset *)addr;
		s = splaudio();
		/* figure out where next DMA will start */
		stamp = sc->sc_rustream == &sc->sc_rr.s
			? sc->sc_rr.stamp : sc->sc_rr.fstamp;
		offs = sc->sc_rustream->inp - sc->sc_rustream->start;
		splx(s);
		ao->samples = stamp;
		ao->deltablks =
		  (stamp / sc->sc_rr.blksize) -
		  (sc->sc_rr.stamp_last / sc->sc_rr.blksize);
		sc->sc_rr.stamp_last = stamp;
		ao->offset = offs;
		break;

	case AUDIO_GETOOFFS:
		ao = (struct audio_offset *)addr;
		s = splaudio();
		/* figure out where next DMA will start */
		stamp = sc->sc_pustream == &sc->sc_pr.s
			? sc->sc_pr.stamp : sc->sc_pr.fstamp;
		offs = sc->sc_pustream->outp - sc->sc_pustream->start
			+ sc->sc_pr.blksize;
		splx(s);
		ao->samples = stamp;
		ao->deltablks =
		  (stamp / sc->sc_pr.blksize) -
		  (sc->sc_pr.stamp_last / sc->sc_pr.blksize);
		sc->sc_pr.stamp_last = stamp;
		if (sc->sc_pustream->start + offs >= sc->sc_pustream->end)
			offs = 0;
		ao->offset = offs;
		break;

	/*
	 * How many bytes will elapse until mike hears the first
	 * sample of what we write next?
	 */
	case AUDIO_WSEEK:
		*(u_long *)addr = audio_stream_get_used(sc->sc_rustream);
		break;

	case AUDIO_SETINFO:
		DPRINTF(("AUDIO_SETINFO mode=0x%x\n", sc->sc_mode));
		error = audiosetinfo(sc, (struct audio_info *)addr);
		break;

	case AUDIO_GETINFO:
		DPRINTF(("AUDIO_GETINFO\n"));
		error = audiogetinfo(sc, (struct audio_info *)addr);
		break;

	case AUDIO_DRAIN:
		DPRINTF(("AUDIO_DRAIN\n"));
		error = audio_drain(sc);
		if (!error && hw->drain)
		    error = hw->drain(sc->hw_hdl);
		break;

	case AUDIO_GETDEV:
		DPRINTF(("AUDIO_GETDEV\n"));
		error = hw->getdev(sc->hw_hdl, (audio_device_t *)addr);
		break;

	case AUDIO_GETENC:
		DPRINTF(("AUDIO_GETENC\n"));
		error = hw->query_encoding(sc->hw_hdl,
		    (struct audio_encoding *)addr);
		break;

	case AUDIO_GETFD:
		DPRINTF(("AUDIO_GETFD\n"));
		*(int *)addr = sc->sc_full_duplex;
		break;

	case AUDIO_SETFD:
		DPRINTF(("AUDIO_SETFD\n"));
		fd = *(int *)addr;
		if (hw->get_props(sc->hw_hdl) & AUDIO_PROP_FULLDUPLEX) {
			if (hw->setfd)
				error = hw->setfd(sc->hw_hdl, fd);
			else
				error = 0;
			if (!error)
				sc->sc_full_duplex = fd;
		} else {
			if (fd)
				error = ENOTTY;
			else
				error = 0;
		}
		break;

	case AUDIO_GETPROPS:
		DPRINTF(("AUDIO_GETPROPS\n"));
		*(int *)addr = hw->get_props(sc->hw_hdl);
		break;

	default:
		if (hw->dev_ioctl) {
			error = hw->dev_ioctl(sc->hw_hdl, cmd, addr, flag, l);
		} else {
			DPRINTF(("audio_ioctl: unknown ioctl\n"));
			error = EINVAL;
		}
		break;
	}
	DPRINTF(("audio_ioctl(%lu,'%c',%lu) result %d\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff, error));
	return error;
}

int
audio_poll(struct audio_softc *sc, int events, struct lwp *l)
{
	int revents;
	int s;
	int used;

	DPRINTF(("audio_poll: events=0x%x mode=%d\n", events, sc->sc_mode));

	revents = 0;
	s = splaudio();
	if (events & (POLLIN | POLLRDNORM)) {
		used = audio_stream_get_used(sc->sc_rustream);
		/*
		 * If half duplex and playing, audio_read() will generate
		 * silence at the play rate; poll for silence being
		 * available.  Otherwise, poll for recorded sound.
		 */
		if ((!sc->sc_full_duplex && (sc->sc_mode & AUMODE_PLAY)) ?
		    sc->sc_pr.stamp > sc->sc_wstamp :
		    used > sc->sc_rr.usedlow)
			revents |= events & (POLLIN | POLLRDNORM);
	}

	if (events & (POLLOUT | POLLWRNORM)) {
		used = audio_stream_get_used(sc->sc_pustream);
		/*
		 * If half duplex and recording, audio_write() will throw
		 * away play data, which means we are always ready to write.
		 * Otherwise, poll for play buffer being below its low water
		 * mark.
		 */
		if ((!sc->sc_full_duplex && (sc->sc_mode & AUMODE_RECORD)) ||
		    used <= sc->sc_pr.usedlow)
			revents |= events & (POLLOUT | POLLWRNORM);
	}

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM))
			selrecord(l, &sc->sc_rsel);

		if (events & (POLLOUT | POLLWRNORM))
			selrecord(l, &sc->sc_wsel);
	}

	splx(s);
	return revents;
}

static void
filt_audiordetach(struct knote *kn)
{
	struct audio_softc *sc;
	int s;

	sc = kn->kn_hook;
	s = splaudio();
	SLIST_REMOVE(&sc->sc_rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_audioread(struct knote *kn, long hint)
{
	struct audio_softc *sc;
	int s;

	sc = kn->kn_hook;
	s = splaudio();
	if (!sc->sc_full_duplex && (sc->sc_mode & AUMODE_PLAY))
		kn->kn_data = sc->sc_pr.stamp - sc->sc_wstamp;
	else
		kn->kn_data = audio_stream_get_used(sc->sc_rustream)
			- sc->sc_rr.usedlow;
	splx(s);

	return kn->kn_data > 0;
}

static const struct filterops audioread_filtops =
	{ 1, NULL, filt_audiordetach, filt_audioread };

static void
filt_audiowdetach(struct knote *kn)
{
	struct audio_softc *sc;
	int s;

	sc = kn->kn_hook;
	s = splaudio();
	SLIST_REMOVE(&sc->sc_wsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_audiowrite(struct knote *kn, long hint)
{
	struct audio_softc *sc;
	audio_stream_t *stream;
	int s;

	sc = kn->kn_hook;
	s = splaudio();
	stream = sc->sc_pustream;
	kn->kn_data = (stream->end - stream->start)
		- audio_stream_get_used(stream);
	splx(s);

	return kn->kn_data > 0;
}

static const struct filterops audiowrite_filtops =
	{ 1, NULL, filt_audiowdetach, filt_audiowrite };

int
audio_kqfilter(struct audio_softc *sc, struct knote *kn)
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &audioread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sc->sc_wsel.sel_klist;
		kn->kn_fop = &audiowrite_filtops;
		break;

	default:
		return 1;
	}

	kn->kn_hook = sc;

	s = splaudio();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return 0;
}

paddr_t
audio_mmap(struct audio_softc *sc, off_t off, int prot)
{
	const struct audio_hw_if *hw;
	struct audio_ringbuffer *cb;
	int s;

	DPRINTF(("audio_mmap: off=%lld, prot=%d\n", (long long)off, prot));
	hw = sc->hw_if;
	if (!(hw->get_props(sc->hw_hdl) & AUDIO_PROP_MMAP) || !hw->mappage)
		return -1;
#if 0
/* XXX
 * The idea here was to use the protection to determine if
 * we are mapping the read or write buffer, but it fails.
 * The VM system is broken in (at least) two ways.
 * 1) If you map memory VM_PROT_WRITE you SIGSEGV
 *    when writing to it, so VM_PROT_READ|VM_PROT_WRITE
 *    has to be used for mmapping the play buffer.
 * 2) Even if calling mmap() with VM_PROT_READ|VM_PROT_WRITE
 *    audio_mmap will get called at some point with VM_PROT_READ
 *    only.
 * So, alas, we always map the play buffer for now.
 */
	if (prot == (VM_PROT_READ|VM_PROT_WRITE) ||
	    prot == VM_PROT_WRITE)
		cb = &sc->sc_pr;
	else if (prot == VM_PROT_READ)
		cb = &sc->sc_rr;
	else
		return -1;
#else
	cb = &sc->sc_pr;
#endif

	if ((u_int)off >= cb->s.bufsize)
		return -1;
	if (!cb->mmapped) {
		cb->mmapped = TRUE;
		if (cb == &sc->sc_pr) {
			audio_fill_silence(&cb->s.param, cb->s.start,
					   cb->s.bufsize);
			s = splaudio();
			sc->sc_pustream = &cb->s;
			if (!sc->sc_pbus)
				(void)audiostartp(sc);
			splx(s);
		} else {
			s = splaudio();
			sc->sc_rustream = &cb->s;
			if (!sc->sc_rbus)
				(void)audiostartr(sc);
			splx(s);
		}
	}

	return hw->mappage(sc->hw_hdl, cb->s.start, off, prot);
}

int
audiostartr(struct audio_softc *sc)
{
	int error;

	DPRINTF(("audiostartr: start=%p used=%d(hi=%d) mmapped=%d\n",
		 sc->sc_rr.s.start, audio_stream_get_used(&sc->sc_rr.s),
		 sc->sc_rr.usedhigh, sc->sc_rr.mmapped));

	if (sc->hw_if->trigger_input)
		error = sc->hw_if->trigger_input(sc->hw_hdl, sc->sc_rr.s.start,
		    sc->sc_rr.s.end, sc->sc_rr.blksize,
		    audio_rint, (void *)sc, &sc->sc_rr.s.param);
	else
		error = sc->hw_if->start_input(sc->hw_hdl, sc->sc_rr.s.start,
		    sc->sc_rr.blksize, audio_rint, (void *)sc);
	if (error) {
		DPRINTF(("audiostartr failed: %d\n", error));
		return error;
	}
	sc->sc_rbus = TRUE;
	return 0;
}

int
audiostartp(struct audio_softc *sc)
{
	int error;
	int used;

	used = audio_stream_get_used(&sc->sc_pr.s);
	DPRINTF(("audiostartp: start=%p used=%d(hi=%d blk=%d) mmapped=%d\n",
		 sc->sc_pr.s.start, used, sc->sc_pr.usedhigh,
		 sc->sc_pr.blksize, sc->sc_pr.mmapped));

	if (!sc->sc_pr.mmapped && used < sc->sc_pr.blksize) {
		wakeup(&sc->sc_wchan);
		DPRINTF(("%s: wakeup and return\n", __func__));
		return 0;
	}

	if (sc->hw_if->trigger_output) {
		DPRINTF(("%s: call trigger_output\n", __func__));
		error = sc->hw_if->trigger_output(sc->hw_hdl, sc->sc_pr.s.start,
		    sc->sc_pr.s.end, sc->sc_pr.blksize,
		    audio_pint, (void *)sc, &sc->sc_pr.s.param);
	} else {
		DPRINTF(("%s: call start_output\n", __func__));
		error = sc->hw_if->start_output(sc->hw_hdl,
		    __UNCONST(sc->sc_pr.s.outp), sc->sc_pr.blksize,
		    audio_pint, (void *)sc);
	}
	if (error) {
		DPRINTF(("audiostartp failed: %d\n", error));
		return error;
	}
	sc->sc_pbus = TRUE;
	return 0;
}

/*
 * When the play interrupt routine finds that the write isn't keeping
 * the buffer filled it will insert silence in the buffer to make up
 * for this.  The part of the buffer that is filled with silence
 * is kept track of in a very approximate way: it starts at sc_sil_start
 * and extends sc_sil_count bytes.  If there is already silence in
 * the requested area nothing is done; so when the whole buffer is
 * silent nothing happens.  When the writer starts again sc_sil_count
 * is set to 0.
 */
/* XXX
 * Putting silence into the output buffer should not really be done
 * at splaudio, but there is no softaudio level to do it at yet.
 */
static inline void
audio_pint_silence(struct audio_softc *sc, struct audio_ringbuffer *cb,
		   uint8_t *inp, int cc)
{
	uint8_t *s, *e, *p, *q;

	if (sc->sc_sil_count > 0) {
		s = sc->sc_sil_start; /* start of silence */
		e = s + sc->sc_sil_count; /* end of sil., may be beyond end */
		p = inp;	/* adjusted pointer to area to fill */
		if (p < s)
			p += cb->s.end - cb->s.start;
		q = p + cc;
		/* Check if there is already silence. */
		if (!(s <= p && p <  e &&
		      s <= q && q <= e)) {
			if (s <= p)
				sc->sc_sil_count = max(sc->sc_sil_count, q-s);
			DPRINTFN(5,("audio_pint_silence: fill cc=%d inp=%p, "
				    "count=%d size=%d\n",
				    cc, inp, sc->sc_sil_count,
				    (int)(cb->s.end - cb->s.start)));
			audio_fill_silence(&cb->s.param, inp, cc);
		} else {
			DPRINTFN(5,("audio_pint_silence: already silent "
				    "cc=%d inp=%p\n", cc, inp));

		}
	} else {
		sc->sc_sil_start = inp;
		sc->sc_sil_count = cc;
		DPRINTFN(5, ("audio_pint_silence: start fill %p %d\n",
			     inp, cc));
		audio_fill_silence(&cb->s.param, inp, cc);
	}
}

static void
audio_softintr_rd(void *cookie)
{
	struct audio_softc *sc = cookie;
	struct proc *p;

	audio_wakeup(&sc->sc_rchan);
	selnotify(&sc->sc_rsel, 0);
	if (sc->sc_async_audio != NULL) {
		DPRINTFN(3, ("audio_softintr_rd: sending SIGIO %p\n",
		    sc->sc_async_audio));
		mutex_enter(&proclist_mutex);
		if ((p = sc->sc_async_audio) != NULL)
			psignal(p, SIGIO);
		mutex_exit(&proclist_mutex);
	}
}

static void
audio_softintr_wr(void *cookie)
{
	struct audio_softc *sc = cookie;
	struct proc *p;

	audio_wakeup(&sc->sc_wchan);
	selnotify(&sc->sc_wsel, 0);
	if (sc->sc_async_audio != NULL) {
		DPRINTFN(3, ("audio_softintr_wr: sending SIGIO %p\n",
		    sc->sc_async_audio));
		mutex_enter(&proclist_mutex);
		if ((p = sc->sc_async_audio) != NULL)
			psignal(p, SIGIO);
		mutex_exit(&proclist_mutex);
	}
}

/*
 * Called from HW driver module on completion of DMA output.
 * Start output of new block, wrap in ring buffer if needed.
 * If no more buffers to play, output zero instead.
 * Do a wakeup if necessary.
 */
void
audio_pint(void *v)
{
	stream_fetcher_t null_fetcher;
	struct audio_softc *sc;
	const struct audio_hw_if *hw;
	struct audio_ringbuffer *cb;
	stream_fetcher_t *fetcher;
	uint8_t *inp;
	int cc, used;
	int blksize;
	int error;

	sc = v;
	if (!sc->sc_open)
		return;		/* ignore interrupt if not open */

	hw = sc->hw_if;
	cb = &sc->sc_pr;
	blksize = cb->blksize;
	cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp, blksize);
	cb->stamp += blksize;
	if (cb->mmapped) {
		DPRINTFN(5, ("audio_pint: mmapped outp=%p cc=%d inp=%p\n",
			     cb->s.outp, blksize, cb->s.inp));
		if (hw->trigger_output == NULL)
			(void)hw->start_output(sc->hw_hdl, __UNCONST(cb->s.outp),
			    blksize, audio_pint, (void *)sc);
		return;
	}

#ifdef AUDIO_INTR_TIME
	{
		struct timeval tv;
		u_long t;
		microtime(&tv);
		t = tv.tv_usec + 1000000 * tv.tv_sec;
		if (sc->sc_pnintr) {
			long lastdelta, totdelta;
			lastdelta = t - sc->sc_plastintr - sc->sc_pblktime;
			if (lastdelta > sc->sc_pblktime / 3) {
				printf("audio: play interrupt(%d) off "
				       "relative by %ld us (%lu)\n",
				       sc->sc_pnintr, lastdelta,
				       sc->sc_pblktime);
			}
			totdelta = t - sc->sc_pfirstintr -
				sc->sc_pblktime * sc->sc_pnintr;
			if (totdelta > sc->sc_pblktime) {
				printf("audio: play interrupt(%d) off "
				       "absolute by %ld us (%lu) (LOST)\n",
				       sc->sc_pnintr, totdelta,
				       sc->sc_pblktime);
				sc->sc_pnintr++; /* avoid repeated messages */
			}
		} else
			sc->sc_pfirstintr = t;
		sc->sc_plastintr = t;
		sc->sc_pnintr++;
	}
#endif

	used = audio_stream_get_used(&cb->s);
	/*
	 * "used <= cb->usedlow" should be "used < blksize" ideally.
	 * Some HW drivers such as uaudio(4) does not call audio_pint()
	 * at accurate timing.  If used < blksize, uaudio(4) already
	 * request transfer of garbage data.
	 */
	if (used <= cb->usedlow && !cb->copying && sc->sc_npfilters > 0) {
		/* we might have data in filter pipeline */
		null_fetcher.fetch_to = null_fetcher_fetch_to;
		fetcher = &sc->sc_pfilters[sc->sc_npfilters - 1]->base;
		sc->sc_pfilters[0]->set_fetcher(sc->sc_pfilters[0],
						&null_fetcher);
		used = audio_stream_get_used(sc->sc_pustream);
		cc = cb->s.end - cb->s.start;
		if (blksize * 2 < cc)
			cc = blksize * 2;
		fetcher->fetch_to(fetcher, &cb->s, cc);
		cb->fstamp += used - audio_stream_get_used(sc->sc_pustream);
		used = audio_stream_get_used(&cb->s);
	}
	if (used < blksize) {
		/* we don't have a full block to use */
		if (cb->copying) {
			/* writer is in progress, don't disturb */
			cb->needfill = TRUE;
			DPRINTFN(1, ("audio_pint: copying in progress\n"));
		} else {
			inp = cb->s.inp;
			cc = blksize - (inp - cb->s.start) % blksize;
			if (cb->pause)
				cb->pdrops += cc;
			else {
				cb->drops += cc;
				sc->sc_playdrop += cc;
			}
			audio_pint_silence(sc, cb, inp, cc);
			cb->s.inp = audio_stream_add_inp(&cb->s, inp, cc);

			/* Clear next block so we keep ahead of the DMA. */
			used = audio_stream_get_used(&cb->s);
			if (used + cc < cb->s.end - cb->s.start)
				audio_pint_silence(sc, cb, inp, blksize);
		}
	}

	DPRINTFN(5, ("audio_pint: outp=%p cc=%d\n", cb->s.outp, blksize));
	if (hw->trigger_output == NULL) {
		error = hw->start_output(sc->hw_hdl, __UNCONST(cb->s.outp),
		    blksize, audio_pint, (void *)sc);
		if (error) {
			/* XXX does this really help? */
			DPRINTF(("audio_pint restart failed: %d\n", error));
			audio_clear(sc);
		}
	}

	DPRINTFN(2, ("audio_pint: mode=%d pause=%d used=%d lowat=%d\n",
		     sc->sc_mode, cb->pause,
		     audio_stream_get_used(sc->sc_pustream), cb->usedlow));
	if ((sc->sc_mode & AUMODE_PLAY) && !cb->pause) {
		if (audio_stream_get_used(sc->sc_pustream) <= cb->usedlow)
			softintr_schedule(sc->sc_sih_wr);
	}

	/* Possible to return one or more "phantom blocks" now. */
	if (!sc->sc_full_duplex && sc->sc_rchan)
		softintr_schedule(sc->sc_sih_rd);
}

/*
 * Called from HW driver module on completion of DMA input.
 * Mark it as input in the ring buffer (fiddle pointers).
 * Do a wakeup if necessary.
 */
void
audio_rint(void *v)
{
	stream_fetcher_t null_fetcher;
	struct audio_softc *sc;
	const struct audio_hw_if *hw;
	struct audio_ringbuffer *cb;
	stream_fetcher_t *last_fetcher;
	int cc;
	int used;
	int blksize;
	int error;

	sc = v;
	cb = &sc->sc_rr;
	if (!sc->sc_open)
		return;		/* ignore interrupt if not open */

	hw = sc->hw_if;
	blksize = cb->blksize;
	cb->s.inp = audio_stream_add_inp(&cb->s, cb->s.inp, blksize);
	cb->stamp += blksize;
	if (cb->mmapped) {
		DPRINTFN(2, ("audio_rint: mmapped inp=%p cc=%d\n",
			     cb->s.inp, blksize));
		if (hw->trigger_input == NULL)
			(void)hw->start_input(sc->hw_hdl, cb->s.inp, blksize,
			    audio_rint, (void *)sc);
		return;
	}

#ifdef AUDIO_INTR_TIME
	{
		struct timeval tv;
		u_long t;
		microtime(&tv);
		t = tv.tv_usec + 1000000 * tv.tv_sec;
		if (sc->sc_rnintr) {
			long lastdelta, totdelta;
			lastdelta = t - sc->sc_rlastintr - sc->sc_rblktime;
			if (lastdelta > sc->sc_rblktime / 5) {
				printf("audio: record interrupt(%d) off "
				       "relative by %ld us (%lu)\n",
				       sc->sc_rnintr, lastdelta,
				       sc->sc_rblktime);
			}
			totdelta = t - sc->sc_rfirstintr -
				sc->sc_rblktime * sc->sc_rnintr;
			if (totdelta > sc->sc_rblktime / 2) {
				sc->sc_rnintr++;
				printf("audio: record interrupt(%d) off "
				       "absolute by %ld us (%lu)\n",
				       sc->sc_rnintr, totdelta,
				       sc->sc_rblktime);
				sc->sc_rnintr++; /* avoid repeated messages */
			}
		} else
			sc->sc_rfirstintr = t;
		sc->sc_rlastintr = t;
		sc->sc_rnintr++;
	}
#endif

	if (!cb->pause && sc->sc_nrfilters > 0) {
		null_fetcher.fetch_to = null_fetcher_fetch_to;
		last_fetcher = &sc->sc_rfilters[sc->sc_nrfilters - 1]->base;
		sc->sc_rfilters[0]->set_fetcher(sc->sc_rfilters[0],
						&null_fetcher);
		used = audio_stream_get_used(sc->sc_rustream);
		cc = sc->sc_rustream->end - sc->sc_rustream->start;
		error = last_fetcher->fetch_to
			(last_fetcher, sc->sc_rustream, cc);
		cb->fstamp += audio_stream_get_used(sc->sc_rustream) - used;
		/* XXX what should do for error? */
	}
	used = audio_stream_get_used(&sc->sc_rr.s);
	if (cb->pause) {
		DPRINTFN(1, ("audio_rint: pdrops %lu\n", cb->pdrops));
		cb->pdrops += blksize;
		cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp, blksize);
	} else if (used + blksize > cb->s.end - cb->s.start && !cb->copying) {
		DPRINTFN(1, ("audio_rint: drops %lu\n", cb->drops));
		cb->drops += blksize;
		cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp, blksize);
	}

	DPRINTFN(2, ("audio_rint: inp=%p cc=%d\n", cb->s.inp, blksize));
	if (hw->trigger_input == NULL) {
		error = hw->start_input(sc->hw_hdl, cb->s.inp, blksize,
		    audio_rint, (void *)sc);
		if (error) {
			/* XXX does this really help? */
			DPRINTF(("audio_rint: restart failed: %d\n", error));
			audio_clear(sc);
		}
	}

	softintr_schedule(sc->sc_sih_rd);
}

int
audio_check_params(struct audio_params *p)
{

	if (p->encoding == AUDIO_ENCODING_PCM16) {
		if (p->precision == 8)
			p->encoding = AUDIO_ENCODING_ULINEAR;
		else
			p->encoding = AUDIO_ENCODING_SLINEAR;
	} else if (p->encoding == AUDIO_ENCODING_PCM8) {
		if (p->precision == 8)
			p->encoding = AUDIO_ENCODING_ULINEAR;
		else
			return EINVAL;
	}

	if (p->encoding == AUDIO_ENCODING_SLINEAR)
#if BYTE_ORDER == LITTLE_ENDIAN
		p->encoding = AUDIO_ENCODING_SLINEAR_LE;
#else
		p->encoding = AUDIO_ENCODING_SLINEAR_BE;
#endif
	if (p->encoding == AUDIO_ENCODING_ULINEAR)
#if BYTE_ORDER == LITTLE_ENDIAN
		p->encoding = AUDIO_ENCODING_ULINEAR_LE;
#else
		p->encoding = AUDIO_ENCODING_ULINEAR_BE;
#endif

	switch (p->encoding) {
	case AUDIO_ENCODING_ULAW:
	case AUDIO_ENCODING_ALAW:
		if (p->precision != 8)
			return EINVAL;
		break;
	case AUDIO_ENCODING_ADPCM:
		if (p->precision != 4 && p->precision != 8)
			return EINVAL;
		break;
	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_SLINEAR_BE:
	case AUDIO_ENCODING_ULINEAR_LE:
	case AUDIO_ENCODING_ULINEAR_BE:
		/* XXX is: our zero-fill can handle any multiple of 8 */
		if (p->precision !=  8 && p->precision != 16 &&
		    p->precision != 24 && p->precision != 32)
			return EINVAL;
		if (p->precision == 8 && p->encoding == AUDIO_ENCODING_SLINEAR_BE)
			p->encoding = AUDIO_ENCODING_SLINEAR_LE;
		if (p->precision == 8 && p->encoding == AUDIO_ENCODING_ULINEAR_BE)
			p->encoding = AUDIO_ENCODING_ULINEAR_LE;
		if (p->validbits > p->precision)
			return EINVAL;
		break;
	case AUDIO_ENCODING_MPEG_L1_STREAM:
	case AUDIO_ENCODING_MPEG_L1_PACKETS:
	case AUDIO_ENCODING_MPEG_L1_SYSTEM:
	case AUDIO_ENCODING_MPEG_L2_STREAM:
	case AUDIO_ENCODING_MPEG_L2_PACKETS:
	case AUDIO_ENCODING_MPEG_L2_SYSTEM:
		break;
	default:
		return EINVAL;
	}

	/* sanity check # of channels*/
	if (p->channels < 1 || p->channels > AUDIO_MAX_CHANNELS)
		return EINVAL;

	return 0;
}

int
audio_set_defaults(struct audio_softc *sc, u_int mode)
{
	struct audio_info ai;

	/* default parameters */
	sc->sc_rparams = audio_default;
	sc->sc_pparams = audio_default;
	sc->sc_blkset = FALSE;

	AUDIO_INITINFO(&ai);
	ai.record.sample_rate = sc->sc_rparams.sample_rate;
	ai.record.encoding    = sc->sc_rparams.encoding;
	ai.record.channels    = sc->sc_rparams.channels;
	ai.record.precision   = sc->sc_rparams.precision;
	ai.play.sample_rate   = sc->sc_pparams.sample_rate;
	ai.play.encoding      = sc->sc_pparams.encoding;
	ai.play.channels      = sc->sc_pparams.channels;
	ai.play.precision     = sc->sc_pparams.precision;
	ai.mode		      = mode;

	return audiosetinfo(sc, &ai);
}

int
au_set_lr_value(struct	audio_softc *sc, mixer_ctrl_t *ct, int l, int r)
{

	ct->type = AUDIO_MIXER_VALUE;
	ct->un.value.num_channels = 2;
	ct->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = l;
	ct->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
	if (sc->hw_if->set_port(sc->hw_hdl, ct) == 0)
		return 0;
	ct->un.value.num_channels = 1;
	ct->un.value.level[AUDIO_MIXER_LEVEL_MONO] = (l+r)/2;
	return sc->hw_if->set_port(sc->hw_hdl, ct);
}

int
au_set_gain(struct audio_softc *sc, struct au_mixer_ports *ports,
	    int gain, int balance)
{
	mixer_ctrl_t ct;
	int i, error;
	int l, r;
	u_int mask;
	int nset;

	if (balance == AUDIO_MID_BALANCE) {
		l = r = gain;
	} else if (balance < AUDIO_MID_BALANCE) {
		l = gain;
		r = (balance * gain) / AUDIO_MID_BALANCE;
	} else {
		r = gain;
		l = ((AUDIO_RIGHT_BALANCE - balance) * gain)
		    / AUDIO_MID_BALANCE;
	}
	DPRINTF(("au_set_gain: gain=%d balance=%d, l=%d r=%d\n",
		 gain, balance, l, r));

	if (ports->index == -1) {
	usemaster:
		if (ports->master == -1)
			return 0; /* just ignore it silently */
		ct.dev = ports->master;
		error = au_set_lr_value(sc, &ct, l, r);
	} else {
		ct.dev = ports->index;
		if (ports->isenum) {
			ct.type = AUDIO_MIXER_ENUM;
			error = sc->hw_if->get_port(sc->hw_hdl, &ct);
			if (error)
				return error;
			if (ports->isdual) {
				if (ports->cur_port == -1)
					ct.dev = ports->master;
				else
					ct.dev = ports->miport[ports->cur_port];
				error = au_set_lr_value(sc, &ct, l, r);
			} else {
				for(i = 0; i < ports->nports; i++)
				    if (ports->misel[i] == ct.un.ord) {
					    ct.dev = ports->miport[i];
					    if (ct.dev == -1 ||
						au_set_lr_value(sc, &ct, l, r))
						    goto usemaster;
					    else
						    break;
				    }
			}
		} else {
			ct.type = AUDIO_MIXER_SET;
			error = sc->hw_if->get_port(sc->hw_hdl, &ct);
			if (error)
				return error;
			mask = ct.un.mask;
			nset = 0;
			for(i = 0; i < ports->nports; i++) {
				if (ports->misel[i] & mask) {
				    ct.dev = ports->miport[i];
				    if (ct.dev != -1 &&
					au_set_lr_value(sc, &ct, l, r) == 0)
					    nset++;
				}
			}
			if (nset == 0)
				goto usemaster;
		}
	}
	if (!error)
		mixer_signal(sc);
	return error;
}

int
au_get_lr_value(struct	audio_softc *sc, mixer_ctrl_t *ct, int *l, int *r)
{
	int error;

	ct->un.value.num_channels = 2;
	if (sc->hw_if->get_port(sc->hw_hdl, ct) == 0) {
		*l = ct->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		*r = ct->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
	} else {
		ct->un.value.num_channels = 1;
		error = sc->hw_if->get_port(sc->hw_hdl, ct);
		if (error)
			return error;
		*r = *l = ct->un.value.level[AUDIO_MIXER_LEVEL_MONO];
	}
	return 0;
}

void
au_get_gain(struct audio_softc *sc, struct au_mixer_ports *ports,
	    u_int *pgain, u_char *pbalance)
{
	mixer_ctrl_t ct;
	int i, l, r, n;
	int lgain, rgain;

	lgain = AUDIO_MAX_GAIN / 2;
	rgain = AUDIO_MAX_GAIN / 2;
	if (ports->index == -1) {
	usemaster:
		if (ports->master == -1)
			goto bad;
		ct.dev = ports->master;
		ct.type = AUDIO_MIXER_VALUE;
		if (au_get_lr_value(sc, &ct, &lgain, &rgain))
			goto bad;
	} else {
		ct.dev = ports->index;
		if (ports->isenum) {
			ct.type = AUDIO_MIXER_ENUM;
			if (sc->hw_if->get_port(sc->hw_hdl, &ct))
				goto bad;
			ct.type = AUDIO_MIXER_VALUE;
			if (ports->isdual) {
				if (ports->cur_port == -1)
					ct.dev = ports->master;
				else
					ct.dev = ports->miport[ports->cur_port];
				au_get_lr_value(sc, &ct, &lgain, &rgain);
			} else {
				for(i = 0; i < ports->nports; i++)
				    if (ports->misel[i] == ct.un.ord) {
					    ct.dev = ports->miport[i];
					    if (ct.dev == -1 ||
						au_get_lr_value(sc, &ct,
								&lgain, &rgain))
						    goto usemaster;
					    else
						    break;
				    }
			}
		} else {
			ct.type = AUDIO_MIXER_SET;
			if (sc->hw_if->get_port(sc->hw_hdl, &ct))
				goto bad;
			ct.type = AUDIO_MIXER_VALUE;
			lgain = rgain = n = 0;
			for(i = 0; i < ports->nports; i++) {
				if (ports->misel[i] & ct.un.mask) {
					ct.dev = ports->miport[i];
					if (ct.dev == -1 ||
					    au_get_lr_value(sc, &ct, &l, &r))
						goto usemaster;
					else {
						lgain += l;
						rgain += r;
						n++;
					}
				}
			}
			if (n != 0) {
				lgain /= n;
				rgain /= n;
			}
		}
	}
bad:
	if (lgain == rgain) {	/* handles lgain==rgain==0 */
		*pgain = lgain;
		*pbalance = AUDIO_MID_BALANCE;
	} else if (lgain < rgain) {
		*pgain = rgain;
		/* balance should be > AUDIO_MID_BALANCE */
		*pbalance = AUDIO_RIGHT_BALANCE -
			(AUDIO_MID_BALANCE * lgain) / rgain;
	} else /* lgain > rgain */ {
		*pgain = lgain;
		/* balance should be < AUDIO_MID_BALANCE */
		*pbalance = (AUDIO_MID_BALANCE * rgain) / lgain;
	}
}

int
au_set_port(struct audio_softc *sc, struct au_mixer_ports *ports, u_int port)
{
	mixer_ctrl_t ct;
	int i, error, use_mixerout;

	use_mixerout = 1;
	if (port == 0) {
		if (ports->allports == 0)
			return 0;		/* Allow this special case. */
		else if (ports->isdual) {
			if (ports->cur_port == -1) {
				return 0;
			} else {
				port = ports->aumask[ports->cur_port];
				ports->cur_port = -1;
				use_mixerout = 0;
			}
		}
	}
	if (ports->index == -1)
		return EINVAL;
	ct.dev = ports->index;
	if (ports->isenum) {
		if (port & (port-1))
			return EINVAL; /* Only one port allowed */
		ct.type = AUDIO_MIXER_ENUM;
		error = EINVAL;
		for(i = 0; i < ports->nports; i++)
			if (ports->aumask[i] == port) {
				if (ports->isdual && use_mixerout) {
					ct.un.ord = ports->mixerout;
					ports->cur_port = i;
				} else {
					ct.un.ord = ports->misel[i];
				}
				error = sc->hw_if->set_port(sc->hw_hdl, &ct);
				break;
			}
	} else {
		ct.type = AUDIO_MIXER_SET;
		ct.un.mask = 0;
		for(i = 0; i < ports->nports; i++)
			if (ports->aumask[i] & port)
				ct.un.mask |= ports->misel[i];
		if (port != 0 && ct.un.mask == 0)
			error = EINVAL;
		else
			error = sc->hw_if->set_port(sc->hw_hdl, &ct);
	}
	if (!error)
		mixer_signal(sc);
	return error;
}

int
au_get_port(struct audio_softc *sc, struct au_mixer_ports *ports)
{
	mixer_ctrl_t ct;
	int i, aumask;

	if (ports->index == -1)
		return 0;
	ct.dev = ports->index;
	ct.type = ports->isenum ? AUDIO_MIXER_ENUM : AUDIO_MIXER_SET;
	if (sc->hw_if->get_port(sc->hw_hdl, &ct))
		return 0;
	aumask = 0;
	if (ports->isenum) {
		if (ports->isdual && ports->cur_port != -1) {
			if (ports->mixerout == ct.un.ord)
				aumask = ports->aumask[ports->cur_port];
			else
				ports->cur_port = -1;
		}
		if (aumask == 0)
			for(i = 0; i < ports->nports; i++)
				if (ports->misel[i] == ct.un.ord)
					aumask = ports->aumask[i];
	} else {
		for(i = 0; i < ports->nports; i++)
			if (ct.un.mask & ports->misel[i])
				aumask |= ports->aumask[i];
	}
	return aumask;
}

int
audiosetinfo(struct audio_softc *sc, struct audio_info *ai)
{
	stream_filter_list_t pfilters, rfilters;
	audio_params_t pp, rp;
	struct audio_prinfo *r, *p;
	const struct audio_hw_if *hw;
	audio_stream_t *oldpus, *oldrus;
	int s, setmode;
	int error;
	int np, nr;
	unsigned int blks;
	int oldpblksize, oldrblksize;
	u_int gain;
	bool rbus, pbus;
	bool cleared, modechange, pausechange;
	u_char balance;

	hw = sc->hw_if;
	if (hw == NULL)		/* HW has not attached */
		return ENXIO;

	DPRINTF(("%s sc=%p ai=%p\n", __func__, sc, ai));
	r = &ai->record;
	p = &ai->play;
	rbus = sc->sc_rbus;
	pbus = sc->sc_pbus;
	error = 0;
	cleared = FALSE;
	modechange = FALSE;
	pausechange = FALSE;

	pp = sc->sc_pparams;	/* Temporary encoding storage in */
	rp = sc->sc_rparams;	/* case setting the modes fails. */
	nr = np = 0;

	if (SPECIFIED(p->sample_rate)) {
		pp.sample_rate = p->sample_rate;
		np++;
	}
	if (SPECIFIED(r->sample_rate)) {
		rp.sample_rate = r->sample_rate;
		nr++;
	}
	if (SPECIFIED(p->encoding)) {
		pp.encoding = p->encoding;
		np++;
	}
	if (SPECIFIED(r->encoding)) {
		rp.encoding = r->encoding;
		nr++;
	}
	if (SPECIFIED(p->precision)) {
		pp.precision = p->precision;
		/* we don't have API to specify validbits */
		pp.validbits = p->precision;
		np++;
	}
	if (SPECIFIED(r->precision)) {
		rp.precision = r->precision;
		/* we don't have API to specify validbits */
		rp.validbits = r->precision;
		nr++;
	}
	if (SPECIFIED(p->channels)) {
		pp.channels = p->channels;
		np++;
	}
	if (SPECIFIED(r->channels)) {
		rp.channels = r->channels;
		nr++;
	}
#ifdef AUDIO_DEBUG
	if (audiodebug && nr > 0)
	    audio_print_params("audiosetinfo() Setting record params:", &rp);
	if (audiodebug && np > 0)
	    audio_print_params("audiosetinfo() Setting play params:", &pp);
#endif
	if (nr > 0 && (error = audio_check_params(&rp)))
		return error;
	if (np > 0 && (error = audio_check_params(&pp)))
		return error;

	oldpblksize = sc->sc_pr.blksize;
	oldrblksize = sc->sc_rr.blksize;

	setmode = 0;
	if (nr > 0) {
		if (!cleared) {
			audio_clear(sc);
			cleared = TRUE;
		}
		modechange = TRUE;
		setmode |= AUMODE_RECORD;
	}
	if (np > 0) {
		if (!cleared) {
			audio_clear(sc);
			cleared = TRUE;
		}
		modechange = TRUE;
		setmode |= AUMODE_PLAY;
	}

	if (SPECIFIED(ai->mode)) {
		if (!cleared) {
			audio_clear(sc);
			cleared = TRUE;
		}
		modechange = TRUE;
		sc->sc_mode = ai->mode;
		if (sc->sc_mode & AUMODE_PLAY_ALL)
			sc->sc_mode |= AUMODE_PLAY;
		if ((sc->sc_mode & AUMODE_PLAY) && !sc->sc_full_duplex)
			/* Play takes precedence */
			sc->sc_mode &= ~AUMODE_RECORD;
	}

	oldpus = sc->sc_pustream;
	oldrus = sc->sc_rustream;
	if (modechange) {
		int indep;

		indep = hw->get_props(sc->hw_hdl) & AUDIO_PROP_INDEPENDENT;
		if (!indep) {
			if (setmode == AUMODE_RECORD)
				pp = rp;
			else if (setmode == AUMODE_PLAY)
				rp = pp;
		}
		memset(&pfilters, 0, sizeof(pfilters));
		memset(&rfilters, 0, sizeof(rfilters));
		pfilters.append = stream_filter_list_append;
		pfilters.prepend = stream_filter_list_prepend;
		pfilters.set = stream_filter_list_set;
		rfilters.append = stream_filter_list_append;
		rfilters.prepend = stream_filter_list_prepend;
		rfilters.set = stream_filter_list_set;
		/* Some device drivers change channels/sample_rate and change
		 * no channels/sample_rate. */
		error = hw->set_params(sc->hw_hdl, setmode,
		    sc->sc_mode & (AUMODE_PLAY | AUMODE_RECORD), &pp, &rp,
		    &pfilters, &rfilters);
		if (error) {
			DPRINTF(("%s: hw->set_params() failed with %d\n",
				 __func__, error));
			goto cleanup;
		}

		audio_check_params(&pp);
		audio_check_params(&rp);
		if (!indep) {
			/* XXX for !indep device, we have to use the same
			 * parameters for the hardware, not userland */
			if (setmode == AUMODE_RECORD) {
				pp = rp;
			} else if (setmode == AUMODE_PLAY) {
				rp = pp;
			}
		}

		if (sc->sc_pr.mmapped && pfilters.req_size > 0) {
			DPRINTF(("%s: mmapped, and filters are requested.\n",
				 __func__));
			error = EINVAL;
			goto cleanup;
		}

		/* construct new filter chain */
		if (setmode & AUMODE_PLAY) {
			error = audio_setup_pfilters(sc, &pp, &pfilters);
			if (error)
				goto cleanup;
		}
		if (setmode & AUMODE_RECORD) {
			error = audio_setup_rfilters(sc, &rp, &rfilters);
			if (error)
				goto cleanup;
		}
		DPRINTF(("%s: filter setup is completed.\n", __func__));

		/* userland formats */
		sc->sc_pparams = pp;
		sc->sc_rparams = rp;
	}

	/* Play params can affect the record params, so recalculate blksize. */
	if (nr > 0 || np > 0) {
		audio_calc_blksize(sc, AUMODE_RECORD);
		audio_calc_blksize(sc, AUMODE_PLAY);
	}
#ifdef AUDIO_DEBUG
	if (audiodebug > 1 && nr > 0)
	    audio_print_params("audiosetinfo() After setting record params:", &sc->sc_rparams);
	if (audiodebug > 1 && np > 0)
	    audio_print_params("audiosetinfo() After setting play params:", &sc->sc_pparams);
#endif

	if (SPECIFIED(p->port)) {
		if (!cleared) {
			audio_clear(sc);
			cleared = TRUE;
		}
		error = au_set_port(sc, &sc->sc_outports, p->port);
		if (error)
			goto cleanup;
	}
	if (SPECIFIED(r->port)) {
		if (!cleared) {
			audio_clear(sc);
			cleared = TRUE;
		}
		error = au_set_port(sc, &sc->sc_inports, r->port);
		if (error)
			goto cleanup;
	}
	if (SPECIFIED(p->gain)) {
		au_get_gain(sc, &sc->sc_outports, &gain, &balance);
		error = au_set_gain(sc, &sc->sc_outports, p->gain, balance);
		if (error)
			goto cleanup;
	}
	if (SPECIFIED(r->gain)) {
		au_get_gain(sc, &sc->sc_inports, &gain, &balance);
		error = au_set_gain(sc, &sc->sc_inports, r->gain, balance);
		if (error)
			goto cleanup;
	}

	if (SPECIFIED_CH(p->balance)) {
		au_get_gain(sc, &sc->sc_outports, &gain, &balance);
		error = au_set_gain(sc, &sc->sc_outports, gain, p->balance);
		if (error)
			goto cleanup;
	}
	if (SPECIFIED_CH(r->balance)) {
		au_get_gain(sc, &sc->sc_inports, &gain, &balance);
		error = au_set_gain(sc, &sc->sc_inports, gain, r->balance);
		if (error)
			goto cleanup;
	}

	if (SPECIFIED(ai->monitor_gain) && sc->sc_monitor_port != -1) {
		mixer_ctrl_t ct;

		ct.dev = sc->sc_monitor_port;
		ct.type = AUDIO_MIXER_VALUE;
		ct.un.value.num_channels = 1;
		ct.un.value.level[AUDIO_MIXER_LEVEL_MONO] = ai->monitor_gain;
		error = sc->hw_if->set_port(sc->hw_hdl, &ct);
		if (error)
			goto cleanup;
	}

	if (SPECIFIED_CH(p->pause)) {
		sc->sc_pr.pause = p->pause;
		pbus = !p->pause;
		pausechange = TRUE;
	}
	if (SPECIFIED_CH(r->pause)) {
		sc->sc_rr.pause = r->pause;
		rbus = !r->pause;
		pausechange = TRUE;
	}

	if (SPECIFIED(ai->blocksize)) {
		int pblksize, rblksize;

		/* Block size specified explicitly. */
		if (ai->blocksize == 0) {
			if (!cleared) {
				audio_clear(sc);
				cleared = TRUE;
			}
			sc->sc_blkset = FALSE;
			audio_calc_blksize(sc, AUMODE_RECORD);
			audio_calc_blksize(sc, AUMODE_PLAY);
		} else {
			sc->sc_blkset = TRUE;
			/* check whether new blocksize changes actually */
			if (hw->round_blocksize == NULL) {
				if (!cleared) {
					audio_clear(sc);
					cleared = TRUE;
				}
				sc->sc_pr.blksize = ai->blocksize;
				sc->sc_rr.blksize = ai->blocksize;
			} else {
				pblksize = hw->round_blocksize(sc->hw_hdl,
				    ai->blocksize, AUMODE_PLAY, &sc->sc_pr.s.param);
				rblksize = hw->round_blocksize(sc->hw_hdl,
				    ai->blocksize, AUMODE_RECORD, &sc->sc_rr.s.param);
				if (pblksize != sc->sc_pr.blksize ||
				    rblksize != sc->sc_rr.blksize) {
					if (!cleared) {
						audio_clear(sc);
						cleared = TRUE;
					}
					sc->sc_pr.blksize = ai->blocksize;
					sc->sc_rr.blksize = ai->blocksize;
				}
			}
		}
	}

	if (SPECIFIED(ai->mode)) {
		if (sc->sc_mode & AUMODE_PLAY)
			audio_init_play(sc);
		if (sc->sc_mode & AUMODE_RECORD)
			audio_init_record(sc);
	}

	if (hw->commit_settings) {
		error = hw->commit_settings(sc->hw_hdl);
		if (error)
			goto cleanup;
	}

cleanup:
	if (cleared || pausechange) {
		int init_error;

		s = splaudio();
		init_error = audio_initbufs(sc);
		if (init_error) goto err;
		if (sc->sc_pr.blksize != oldpblksize ||
		    sc->sc_rr.blksize != oldrblksize ||
		    sc->sc_pustream != oldpus ||
		    sc->sc_rustream != oldrus)
			audio_calcwater(sc);
		if ((sc->sc_mode & AUMODE_PLAY) &&
		    pbus && !sc->sc_pbus)
			init_error = audiostartp(sc);
		if (!init_error &&
		    (sc->sc_mode & AUMODE_RECORD) &&
		    rbus && !sc->sc_rbus)
			init_error = audiostartr(sc);
	err:
		splx(s);
		if (init_error)
			return init_error;
	}

	/* Change water marks after initializing the buffers. */
	if (SPECIFIED(ai->hiwat)) {
		blks = ai->hiwat;
		if (blks > sc->sc_pr.maxblks)
			blks = sc->sc_pr.maxblks;
		if (blks < 2)
			blks = 2;
		sc->sc_pr.usedhigh = blks * sc->sc_pr.blksize;
	}
	if (SPECIFIED(ai->lowat)) {
		blks = ai->lowat;
		if (blks > sc->sc_pr.maxblks - 1)
			blks = sc->sc_pr.maxblks - 1;
		sc->sc_pr.usedlow = blks * sc->sc_pr.blksize;
	}
	if (SPECIFIED(ai->hiwat) || SPECIFIED(ai->lowat)) {
		if (sc->sc_pr.usedlow > sc->sc_pr.usedhigh - sc->sc_pr.blksize)
			sc->sc_pr.usedlow =
				sc->sc_pr.usedhigh - sc->sc_pr.blksize;
	}

	return error;
}

int
audiogetinfo(struct audio_softc *sc, struct audio_info *ai)
{
	struct audio_prinfo *r, *p;
	const struct audio_hw_if *hw;

	r = &ai->record;
	p = &ai->play;
	hw = sc->hw_if;
	if (hw == NULL)		/* HW has not attached */
		return ENXIO;

	p->sample_rate = sc->sc_pparams.sample_rate;
	r->sample_rate = sc->sc_rparams.sample_rate;
	p->channels = sc->sc_pparams.channels;
	r->channels = sc->sc_rparams.channels;
	p->precision = sc->sc_pparams.precision;
	r->precision = sc->sc_rparams.precision;
	p->encoding = sc->sc_pparams.encoding;
	r->encoding = sc->sc_rparams.encoding;

	r->port = au_get_port(sc, &sc->sc_inports);
	p->port = au_get_port(sc, &sc->sc_outports);

	r->avail_ports = sc->sc_inports.allports;
	p->avail_ports = sc->sc_outports.allports;

	au_get_gain(sc, &sc->sc_inports,  &r->gain, &r->balance);
	au_get_gain(sc, &sc->sc_outports, &p->gain, &p->balance);

	if (sc->sc_monitor_port != -1) {
		mixer_ctrl_t ct;

		ct.dev = sc->sc_monitor_port;
		ct.type = AUDIO_MIXER_VALUE;
		ct.un.value.num_channels = 1;
		if (sc->hw_if->get_port(sc->hw_hdl, &ct))
			ai->monitor_gain = 0;
		else
			ai->monitor_gain =
				ct.un.value.level[AUDIO_MIXER_LEVEL_MONO];
	} else
		ai->monitor_gain = 0;

	p->seek = audio_stream_get_used(sc->sc_pustream);
	r->seek = audio_stream_get_used(sc->sc_rustream);

	/*
	 * XXX samples should be a value for userland data.
	 * But drops is a value for HW data.
	 */
	p->samples = (sc->sc_pustream == &sc->sc_pr.s
		      ? sc->sc_pr.stamp : sc->sc_pr.fstamp) - sc->sc_pr.drops;
	r->samples = (sc->sc_rustream == &sc->sc_rr.s
		      ? sc->sc_rr.stamp : sc->sc_rr.fstamp) - sc->sc_rr.drops;

	p->eof = sc->sc_eof;
	r->eof = 0;

	p->pause = sc->sc_pr.pause;
	r->pause = sc->sc_rr.pause;

	p->error = sc->sc_pr.drops != 0;
	r->error = sc->sc_rr.drops != 0;

	p->waiting = r->waiting = 0;		/* open never hangs */

	p->open = (sc->sc_open & AUOPEN_WRITE) != 0;
	r->open = (sc->sc_open & AUOPEN_READ) != 0;

	p->active = sc->sc_pbus;
	r->active = sc->sc_rbus;

	p->buffer_size = sc->sc_pustream->bufsize;
	r->buffer_size = sc->sc_rustream->bufsize;

	ai->blocksize = sc->sc_pr.blksize;
	ai->hiwat = sc->sc_pr.usedhigh / sc->sc_pr.blksize;
	ai->lowat = sc->sc_pr.usedlow / sc->sc_pr.blksize;
	ai->mode = sc->sc_mode;

	return 0;
}

/*
 * Mixer driver
 */
int
mixer_open(dev_t dev, struct audio_softc *sc, int flags,
    int ifmt, struct lwp *l)
{
	if (sc->hw_if == NULL)
		return  ENXIO;

	DPRINTF(("mixer_open: flags=0x%x sc=%p\n", flags, sc));

	return 0;
}

/*
 * Remove a process from those to be signalled on mixer activity.
 */
static void
mixer_remove(struct audio_softc *sc, struct lwp *l)
{
	struct mixer_asyncs **pm, *m;
	struct proc *p;

	if (l == NULL)
		return;

	p = l->l_proc;

	for (pm = &sc->sc_async_mixer; *pm; pm = &(*pm)->next) {
		if ((*pm)->proc == p) {
			m = *pm;
			*pm = m->next;
			free(m, M_DEVBUF);
			return;
		}
	}
}

/*
 * Signal all processes waiting for the mixer.
 */
static void
mixer_signal(struct audio_softc *sc)
{
	struct mixer_asyncs *m;

	for (m = sc->sc_async_mixer; m; m = m->next) {
		mutex_enter(&proclist_mutex);
		psignal(m->proc, SIGIO);
		mutex_exit(&proclist_mutex);
	}
}

/*
 * Close a mixer device
 */
/* ARGSUSED */
int
mixer_close(struct audio_softc *sc, int flags, int ifmt,
    struct lwp *l)
{

	DPRINTF(("mixer_close: sc %p\n", sc));
	mixer_remove(sc, l);
	return 0;
}

int
mixer_ioctl(struct audio_softc *sc, u_long cmd, caddr_t addr, int flag,
	    struct lwp *l)
{
	const struct audio_hw_if *hw;
	int error;

	DPRINTF(("mixer_ioctl(%lu,'%c',%lu)\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff));
	hw = sc->hw_if;
	error = EINVAL;
	switch (cmd) {
	case FIOASYNC:
		mixer_remove(sc, l);	/* remove old entry */
		if (*(int *)addr) {
			struct mixer_asyncs *ma;
			ma = malloc(sizeof (struct mixer_asyncs),
				    M_DEVBUF, M_WAITOK);
			ma->next = sc->sc_async_mixer;
			ma->proc = l->l_proc;
			sc->sc_async_mixer = ma;
		}
		error = 0;
		break;

	case AUDIO_GETDEV:
		DPRINTF(("AUDIO_GETDEV\n"));
		error = hw->getdev(sc->hw_hdl, (audio_device_t *)addr);
		break;

	case AUDIO_MIXER_DEVINFO:
		DPRINTF(("AUDIO_MIXER_DEVINFO\n"));
		((mixer_devinfo_t *)addr)->un.v.delta = 0; /* default */
		error = hw->query_devinfo(sc->hw_hdl, (mixer_devinfo_t *)addr);
		break;

	case AUDIO_MIXER_READ:
		DPRINTF(("AUDIO_MIXER_READ\n"));
		error = hw->get_port(sc->hw_hdl, (mixer_ctrl_t *)addr);
		break;

	case AUDIO_MIXER_WRITE:
		DPRINTF(("AUDIO_MIXER_WRITE\n"));
		error = hw->set_port(sc->hw_hdl, (mixer_ctrl_t *)addr);
		if (!error && hw->commit_settings)
			error = hw->commit_settings(sc->hw_hdl);
		if (!error)
			mixer_signal(sc);
		break;

	default:
		if (hw->dev_ioctl)
			error = hw->dev_ioctl(sc->hw_hdl, cmd, addr, flag, l);
		else
			error = EINVAL;
		break;
	}
	DPRINTF(("mixer_ioctl(%lu,'%c',%lu) result %d\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff, error));
	return error;
}
#endif /* NAUDIO > 0 */

#include "midi.h"

#if NAUDIO == 0 && (NMIDI > 0 || NMIDIBUS > 0)
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <dev/audio_if.h>
#endif

#if NAUDIO > 0 || (NMIDI > 0 || NMIDIBUS > 0)
int
audioprint(void *aux, const char *pnp)
{
	struct audio_attach_args *arg;
	const char *type;

	if (pnp != NULL) {
		arg = aux;
		switch (arg->type) {
		case AUDIODEV_TYPE_AUDIO:
			type = "audio";
			break;
		case AUDIODEV_TYPE_MIDI:
			type = "midi";
			break;
		case AUDIODEV_TYPE_OPL:
			type = "opl";
			break;
		case AUDIODEV_TYPE_MPU:
			type = "mpu";
			break;
		default:
			panic("audioprint: unknown type %d", arg->type);
		}
		aprint_normal("%s at %s", type, pnp);
	}
	return UNCONF;
}

#endif /* NAUDIO > 0 || (NMIDI > 0 || NMIDIBUS > 0) */

#if NAUDIO > 0
void
audio_powerhook(int why, void *aux)
{
	struct audio_softc *sc;
	const struct audio_hw_if *hwp;

	sc = (struct audio_softc *)aux;
	hwp = sc->hw_if;

	switch (why) {
	case PWR_SOFTSUSPEND:
		if (sc->sc_pbus == TRUE)
			hwp->halt_output(sc->hw_hdl);
		if (sc->sc_rbus == TRUE)
			hwp->halt_input(sc->hw_hdl);
		break;
	case PWR_SOFTRESUME:
		if (sc->sc_pbus == TRUE)
			audiostartp(sc);
		if (sc->sc_rbus == TRUE)
			audiostartr(sc);
		break;
	}

	return;
}
#endif /* NAUDIO > 0 */
