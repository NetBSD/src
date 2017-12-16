/*	$NetBSD: audio.c,v 1.445 2017/12/16 16:04:20 nat Exp $	*/

/*-
 * Copyright (c) 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Locking: there are two locks.
 *
 * - sc_lock, provided by the underlying driver.  This is an adaptive lock,
 *   returned in the second parameter to hw_if->get_locks().  It is known
 *   as the "thread lock".
 *
 *   It serializes access to state in all places except the 
 *   driver's interrupt service routine.  This lock is taken from process
 *   context (example: access to /dev/audio).  It is also taken from soft
 *   interrupt handlers in this module, primarily to serialize delivery of
 *   wakeups.  This lock may be used/provided by modules external to the
 *   audio subsystem, so take care not to introduce a lock order problem. 
 *   LONG TERM SLEEPS MUST NOT OCCUR WITH THIS LOCK HELD.
 *
 * - sc_intr_lock, provided by the underlying driver.  This may be either a
 *   spinlock (at IPL_SCHED or IPL_VM) or an adaptive lock (IPL_NONE or
 *   IPL_SOFT*), returned in the first parameter to hw_if->get_locks().  It
 *   is known as the "interrupt lock".
 *
 *   It provides atomic access to the device's hardware state, and to audio
 *   channel data that may be accessed by the hardware driver's ISR.
 *   In all places outside the ISR, sc_lock must be held before taking
 *   sc_intr_lock.  This is to ensure that groups of hardware operations are
 *   made atomically.  SLEEPS CANNOT OCCUR WITH THIS LOCK HELD.
 *
 * List of hardware interface methods, and which locks are held when each
 * is called by this module:
 *
 *	METHOD			INTR	THREAD  NOTES
 *	----------------------- ------- -------	-------------------------
 *	open 			x	x
 *	close 			x	x
 *	drain 			x	x
 *	query_encoding		-	x
 *	set_params 		-	x
 *	round_blocksize		-	x
 *	commit_settings		-	x
 *	init_output 		x	x
 *	init_input 		x	x
 *	start_output 		x	x
 *	start_input 		x	x
 *	halt_output 		x	x
 *	halt_input 		x	x
 *	speaker_ctl 		x	x
 *	getdev 			-	x
 *	setfd 			-	x
 *	set_port 		-	x
 *	get_port 		-	x
 *	query_devinfo 		-	x
 *	allocm 			-	-	Called at attach time
 *	freem 			-	-	Called at attach time
 *	round_buffersize 	-	x
 *	mappage 		-	-	Mem. unchanged after attach
 *	get_props 		-	x
 *	trigger_output 		x	x
 *	trigger_input 		x	x
 *	dev_ioctl 		-	x
 *	get_locks 		-	-	Called at attach time
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: audio.c,v 1.445 2017/12/16 16:04:20 nat Exp $");

#ifdef _KERNEL_OPT
#include "audio.h"
#include "midi.h"
#endif

#if NAUDIO > 0

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/audioio.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kthread.h>
#include <sys/cpu.h>
#include <sys/mman.h>

#include <dev/audio_if.h>
#include <dev/audiovar.h>
#include <dev/auconv.h>
#include <dev/auvolconv.h>

#include <machine/endian.h>

#include <uvm/uvm.h>

#include "ioconf.h"

/* #define AUDIO_DEBUG	1 */
#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (audiodebug) printf x
#define DPRINTFN(n,x)	if (audiodebug>(n)) printf x
int	audiodebug = AUDIO_DEBUG;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define PREFILL_BLOCKS	3	/* no. audioblocks required to start stream */
#define ROUNDSIZE(x)	(x) &= -16	/* round to nice boundary */
#define SPECIFIED(x)	((int)(x) != ~0)
#define SPECIFIED_CH(x)	((x) != (u_char)~0)

/* #define AUDIO_PM_IDLE */
#ifdef AUDIO_PM_IDLE
int	audio_idle_timeout = 30;
#endif

#define HW_LOCK(x)	do { \
	if ((x) == sc->sc_hwvc) \
		mutex_enter(sc->sc_intr_lock); \
} while (0)

#define HW_UNLOCK(x)	do { \
	if ((x) == sc->sc_hwvc) \
		mutex_exit(sc->sc_intr_lock); \
} while (0)

int	audio_blk_ms = AUDIO_BLK_MS;

int	audiosetinfo(struct audio_softc *, struct audio_info *, bool,
		     struct virtual_channel *);
int	audiogetinfo(struct audio_softc *, struct audio_info *, int,
		     struct virtual_channel *);

int	audio_open(dev_t, struct audio_softc *, int, int, struct lwp *,
		   struct file **);
int	audio_close(struct audio_softc *, int, struct audio_chan *);
int	audio_read(struct audio_softc *, struct uio *, int,
		   struct virtual_channel *);
int	audio_write(struct audio_softc *, struct uio *, int,
		    struct virtual_channel *);
int	audio_ioctl(dev_t, struct audio_softc *, u_long, void *, int,
		    struct lwp *, struct audio_chan *);
int	audio_poll(struct audio_softc *, int, struct lwp *,
		   struct virtual_channel *);
int	audio_kqfilter(struct audio_chan *, struct knote *);
int 	audio_mmap(struct audio_softc *, off_t *, size_t, int, int *, int *,
		   struct uvm_object **, int *, struct virtual_channel *);
static	int audio_fop_mmap(struct file *, off_t *, size_t, int, int *, int *,
			   struct uvm_object **, int *);

int	mixer_open(dev_t, struct audio_softc *, int, int, struct lwp *,
		   struct file **);
int	mixer_close(struct audio_softc *, int, struct audio_chan *);
int	mixer_ioctl(struct audio_softc *, u_long, void *, int, struct lwp *);
static	void mixer_remove(struct audio_softc *);
static	void mixer_signal(struct audio_softc *);
static	void grow_mixer_states(struct audio_softc *, int);
static	void shrink_mixer_states(struct audio_softc *, int);

void	audio_init_record(struct audio_softc *, struct virtual_channel *);
void	audio_init_play(struct audio_softc *, struct virtual_channel *);
int	audiostartr(struct audio_softc *, struct virtual_channel *);
int	audiostartp(struct audio_softc *, struct virtual_channel *);
void	audio_rint(void *);
void	audio_pint(void *);
void	audio_mix(void *);
void	audio_upmix(void *);
void	audio_play_thread(void *);
void	audio_rec_thread(void *);
void	recswvol_func(struct audio_softc *, struct audio_ringbuffer *,
		      size_t, struct virtual_channel *);
void	mix_func(struct audio_softc *, struct audio_ringbuffer *,
		 struct virtual_channel *);
int	mix_write(void *);
int	mix_read(void *);
int	audio_check_params(struct audio_params *);

static void	audio_calc_latency(struct audio_softc *);
static void	audio_setblksize(struct audio_softc *,
				 struct virtual_channel *, int, int);
int	audio_calc_blksize(struct audio_softc *, const audio_params_t *);
void	audio_fill_silence(const struct audio_params *, uint8_t *, int);
int	audio_silence_copyout(struct audio_softc *, int, struct uio *);

static int	audio_allocbufs(struct audio_softc *);
void	audio_init_ringbuffer(struct audio_softc *,
			      struct audio_ringbuffer *, int);
int	audio_initbufs(struct audio_softc *, struct virtual_channel *);
void	audio_calcwater(struct audio_softc *, struct virtual_channel *);
int	audio_drain(struct audio_softc *, struct virtual_channel *);
void	audio_clear(struct audio_softc *, struct virtual_channel *);
void	audio_clear_intr_unlocked(struct audio_softc *sc,
				  struct virtual_channel *);
int	audio_alloc_ring(struct audio_softc *, struct audio_ringbuffer *, int,
			 size_t);
void	audio_free_ring(struct audio_softc *, struct audio_ringbuffer *);
static int audio_setup_pfilters(struct audio_softc *, const audio_params_t *,
			      stream_filter_list_t *, struct virtual_channel *);
static int audio_setup_rfilters(struct audio_softc *, const audio_params_t *,
			      stream_filter_list_t *, struct virtual_channel *);
static void audio_destroy_pfilters(struct virtual_channel *);
static void audio_destroy_rfilters(struct virtual_channel *);
static void audio_stream_dtor(audio_stream_t *);
static int audio_stream_ctor(audio_stream_t *, const audio_params_t *, int);
static void stream_filter_list_append(stream_filter_list_t *,
		stream_filter_factory_t, const audio_params_t *);
static void stream_filter_list_prepend(stream_filter_list_t *,
	    	stream_filter_factory_t, const audio_params_t *);
static void stream_filter_list_set(stream_filter_list_t *, int,
		stream_filter_factory_t, const audio_params_t *);
int	audio_set_defaults(struct audio_softc *, u_int,
						struct virtual_channel *);
static int audio_sysctl_frequency(SYSCTLFN_PROTO);
static int audio_sysctl_precision(SYSCTLFN_PROTO);
static int audio_sysctl_channels(SYSCTLFN_PROTO);
static int audio_sysctl_latency(SYSCTLFN_PROTO);
static int audio_sysctl_usemixer(SYSCTLFN_PROTO);

static int	audiomatch(device_t, cfdata_t, void *);
static void	audioattach(device_t, device_t, void *);
static int	audiodetach(device_t, int);
static int	audioactivate(device_t, enum devact);
static void	audiochilddet(device_t, device_t);
static int	audiorescan(device_t, const char *, const int *);

static int	audio_modcmd(modcmd_t, void *);

#ifdef AUDIO_PM_IDLE
static void	audio_idle(void *);
static void	audio_activity(device_t, devactive_t);
#endif

static bool	audio_suspend(device_t dv, const pmf_qual_t *);
static bool	audio_resume(device_t dv, const pmf_qual_t *);
static void	audio_volume_down(device_t);
static void	audio_volume_up(device_t);
static void	audio_volume_toggle(device_t);

static void	audio_mixer_capture(struct audio_softc *);
static void	audio_mixer_restore(struct audio_softc *);

static int	audio_get_props(struct audio_softc *);
static bool	audio_can_playback(struct audio_softc *);
static bool	audio_can_capture(struct audio_softc *);

static void	audio_softintr_rd(void *);
static void	audio_softintr_wr(void *);

static int	audio_enter(dev_t, krw_t, struct audio_softc **);
static void	audio_exit(struct audio_softc *);
static int	audio_waitio(struct audio_softc *, kcondvar_t *,
			     struct virtual_channel *);

static int audioclose(struct file *);
static int audioread(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int audiowrite(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int audioioctl(struct file *, u_long, void *);
static int audiopoll(struct file *, int);
static int audiokqfilter(struct file *, struct knote *);
static int audiostat(struct file *, struct stat *);

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
static int
	audio_get_port(struct audio_softc *, mixer_ctrl_t *);
static int
	audio_set_port(struct audio_softc *, mixer_ctrl_t *);
static int
	audio_query_devinfo(struct audio_softc *, mixer_devinfo_t *);
static int audio_set_params (struct audio_softc *, int, int,
		 audio_params_t *, audio_params_t *,
		 stream_filter_list_t *, stream_filter_list_t *,
		 const struct virtual_channel *);
static int
audio_query_encoding(struct audio_softc *, struct audio_encoding *);
static int audio_set_vchan_defaults(struct audio_softc *, u_int);
static int vchan_autoconfig(struct audio_softc *);
int	au_get_lr_value(struct audio_softc *, mixer_ctrl_t *, int *, int *);
int	au_set_lr_value(struct audio_softc *, mixer_ctrl_t *, int, int);
int	au_portof(struct audio_softc *, char *, int);

typedef struct uio_fetcher {
	stream_fetcher_t base;
	struct uio *uio;
	int usedhigh;
	int last_used;
} uio_fetcher_t;

static void	uio_fetcher_ctor(uio_fetcher_t *, struct uio *, int);
static int	uio_fetcher_fetch_to(struct audio_softc *, stream_fetcher_t *,
				     audio_stream_t *, int);
static int	null_fetcher_fetch_to(struct audio_softc *, stream_fetcher_t *,
				      audio_stream_t *, int);

static dev_type_open(audioopen);
/* XXXMRG use more dev_type_xxx */

const struct cdevsw audio_cdevsw = {
	.d_open = audioopen,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

const struct fileops audio_fileops = {
	.fo_name = "audio",
	.fo_read = audioread,
	.fo_write = audiowrite,
	.fo_ioctl = audioioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_stat = audiostat,
	.fo_poll = audiopoll,
	.fo_close = audioclose,
	.fo_mmap = audio_fop_mmap,
	.fo_kqfilter = audiokqfilter,
	.fo_restart = fnullop_restart
};

/* The default audio mode: 8 kHz mono mu-law */
const struct audio_params audio_default = {
	.sample_rate = 8000,
	.encoding = AUDIO_ENCODING_ULAW,
	.precision = 8,
	.validbits = 8,
	.channels = 1,
};

int auto_config_precision[] = { 16, 8, 32 };
int auto_config_channels[] = { 2, AUDIO_MAX_CHANNELS, 10, 8, 6, 4, 1 };
int auto_config_freq[] = { 48000, 44100, 96000, 192000, 32000,
			   22050, 16000, 11025, 8000, 4000 };

CFATTACH_DECL3_NEW(audio, sizeof(struct audio_softc),
    audiomatch, audioattach, audiodetach, audioactivate, audiorescan,
    audiochilddet, DVF_DETACH_SHUTDOWN);

static int
audiomatch(device_t parent, cfdata_t match, void *aux)
{
	struct audio_attach_args *sa;

	sa = aux;
	DPRINTF(("%s: type=%d sa=%p hw=%p\n",
		 __func__, sa->type, sa, sa->hwif));
	return (sa->type == AUDIODEV_TYPE_AUDIO) ? 1 : 0;
}

static void
audioattach(device_t parent, device_t self, void *aux)
{
	struct audio_softc *sc;
	struct audio_attach_args *sa;
	struct virtual_channel *vc;
	const struct audio_hw_if *hwp;
	const struct sysctlnode *node;
	void *hdlp;
	int error;
	mixer_devinfo_t mi;
	int iclass, mclass, oclass, rclass, props;
	int record_master_found, record_source_found;

	sc = device_private(self);
	sc->dev = self;
	sa = aux;
	hwp = sa->hwif;
	hdlp = sa->hdl;
	sc->sc_opens = 0;
	sc->sc_recopens = 0;
	sc->sc_aivalid = false;
 	sc->sc_ready = true;
	sc->sc_latency = audio_blk_ms * PREFILL_BLOCKS;

 	sc->sc_format[0].mode = AUMODE_PLAY | AUMODE_RECORD;
 	sc->sc_format[0].encoding =
#if BYTE_ORDER == LITTLE_ENDIAN
		 AUDIO_ENCODING_SLINEAR_LE;
#else
		 AUDIO_ENCODING_SLINEAR_BE;
#endif
 	sc->sc_format[0].precision = 16;
 	sc->sc_format[0].validbits = 16;
 	sc->sc_format[0].channels = 2;
 	sc->sc_format[0].channel_mask = AUFMT_STEREO;
 	sc->sc_format[0].frequency_type = 1;
 	sc->sc_format[0].frequency[0] = 44100;

	sc->sc_trigger_started = false;
	sc->sc_rec_started = false;
	sc->sc_dying = false;
	SIMPLEQ_INIT(&sc->sc_audiochan);

	vc = kmem_zalloc(sizeof(struct virtual_channel), KM_SLEEP);
	sc->sc_hwvc = vc;
	vc->sc_open = 0;
	vc->sc_mode = 0;
	vc->sc_npfilters = 0;
	vc->sc_nrfilters = 0;
	memset(vc->sc_pfilters, 0, sizeof(vc->sc_pfilters));
	memset(vc->sc_rfilters, 0, sizeof(vc->sc_rfilters));
	vc->sc_lastinfovalid = false;
	vc->sc_swvol = 255;
	vc->sc_recswvol = 255;

	if (auconv_create_encodings(sc->sc_format, VAUDIO_NFORMATS,
	    &sc->sc_encodings) != 0) {
		aprint_error_dev(self, "couldn't create encodings\n");
		return;
	}

	cv_init(&sc->sc_rchan, "audiord");
	cv_init(&sc->sc_wchan, "audiowr");
	cv_init(&sc->sc_lchan, "audiolk");
	cv_init(&sc->sc_condvar,"play");
	cv_init(&sc->sc_rcondvar,"record");

	if (hwp == NULL || hwp->get_locks == NULL) {
		aprint_error(": missing method\n");
		panic("audioattach");
	}

	hwp->get_locks(hdlp, &sc->sc_intr_lock, &sc->sc_lock);

#ifdef DIAGNOSTIC
	if (hwp->query_encoding == NULL ||
	    hwp->set_params == NULL ||
	    (hwp->start_output == NULL && hwp->trigger_output == NULL) ||
	    (hwp->start_input == NULL && hwp->trigger_input == NULL) ||
	    hwp->halt_output == NULL ||
	    hwp->halt_input == NULL ||
	    hwp->getdev == NULL ||
	    hwp->set_port == NULL ||
	    hwp->get_port == NULL ||
	    hwp->query_devinfo == NULL ||
	    hwp->get_props == NULL) {
		aprint_error(": missing method\n");
		return;
	}
#endif

	sc->hw_if = hwp;
	sc->hw_hdl = hdlp;
	sc->sc_dev = parent;

	mutex_enter(sc->sc_lock);
	props = audio_get_props(sc);
	mutex_exit(sc->sc_lock);

	if (props & AUDIO_PROP_FULLDUPLEX)
		aprint_normal(": full duplex");
	else
		aprint_normal(": half duplex");

	if (props & AUDIO_PROP_PLAYBACK)
		aprint_normal(", playback");
	if (props & AUDIO_PROP_CAPTURE)
		aprint_normal(", capture");
	if (props & AUDIO_PROP_MMAP)
		aprint_normal(", mmap");
	if (props & AUDIO_PROP_INDEPENDENT)
		aprint_normal(", independent");

	aprint_naive("\n");
	aprint_normal("\n");

	mutex_enter(sc->sc_lock);
	if (audio_allocbufs(sc) != 0) {
		aprint_error_dev(sc->sc_dev,
			"could not allocate ring buffer\n");
		mutex_exit(sc->sc_lock);
		return;
	}
	mutex_exit(sc->sc_lock);

	sc->sc_lastgain = 128;
	sc->sc_multiuser = false;
	sc->sc_usemixer = true;

	error = vchan_autoconfig(sc);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "%s: audio_set_vchan_defaults() "
		    "failed\n", __func__);
	}

	sc->sc_sih_rd = softint_establish(SOFTINT_SERIAL | SOFTINT_MPSAFE,
	    audio_softintr_rd, sc);
	sc->sc_sih_wr = softint_establish(SOFTINT_SERIAL | SOFTINT_MPSAFE,
	    audio_softintr_wr, sc);

	iclass = mclass = oclass = rclass = -1;
	sc->sc_inports.index = -1;
	sc->sc_inports.master = -1;
	sc->sc_inports.nports = 0;
	sc->sc_inports.isenum = false;
	sc->sc_inports.allports = 0;
	sc->sc_inports.isdual = false;
	sc->sc_inports.mixerout = -1;
	sc->sc_inports.cur_port = -1;
	sc->sc_outports.index = -1;
	sc->sc_outports.master = -1;
	sc->sc_outports.nports = 0;
	sc->sc_outports.isenum = false;
	sc->sc_outports.allports = 0;
	sc->sc_outports.isdual = false;
	sc->sc_outports.mixerout = -1;
	sc->sc_outports.cur_port = -1;
	sc->sc_monitor_port = -1;
	/*
	 * Read through the underlying driver's list, picking out the class
	 * names from the mixer descriptions. We'll need them to decode the
	 * mixer descriptions on the next pass through the loop.
	 */
	mutex_enter(sc->sc_lock);
	for(mi.index = 0; ; mi.index++) {
		if (audio_query_devinfo(sc, &mi) != 0)
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
	mutex_exit(sc->sc_lock);

	/* Allocate save area.  Ensure non-zero allocation. */
	sc->sc_static_nmixer_states = mi.index;
	sc->sc_static_nmixer_states++;
	sc->sc_nmixer_states = sc->sc_static_nmixer_states;
	sc->sc_mixer_state = kmem_zalloc(sizeof(mixer_ctrl_t) *
	    (sc->sc_nmixer_states + 1), KM_SLEEP);

	/*
	 * This is where we assign each control in the "audio" model, to the
	 * underlying "mixer" control.  We walk through the whole list once,
	 * assigning likely candidates as we come across them.
	 */
	record_master_found = 0;
	record_source_found = 0;
	mutex_enter(sc->sc_lock);
	for(mi.index = 0; ; mi.index++) {
		if (audio_query_devinfo(sc, &mi) != 0)
			break;
		KASSERT(mi.index < sc->sc_nmixer_states);
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
			if (strcmp(mi.label.name, AudioNdac) == 0 &&
			    sc->sc_outports.master == -1)
				sc->sc_outports.master = mi.index;
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
	mutex_exit(sc->sc_lock);
	DPRINTF(("audio_attach: inputs ports=0x%x, input master=%d, "
		 "output ports=0x%x, output master=%d\n",
		 sc->sc_inports.allports, sc->sc_inports.master,
		 sc->sc_outports.allports, sc->sc_outports.master));

	/* sysctl set-up for alternate configs */
	sysctl_createv(&sc->sc_log, 0, NULL, &node,
		0,
		CTLTYPE_NODE, device_xname(sc->sc_dev),
		SYSCTL_DESCR("audio format information"),
		NULL, 0,
		NULL, 0,
		CTL_HW,
		CTL_CREATE, CTL_EOL);

	if (node != NULL) {
		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "frequency",
			SYSCTL_DESCR("intermediate frequency"),
			audio_sysctl_frequency, 0,
			(void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);

		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "precision",
			SYSCTL_DESCR("intermediate precision"),
			audio_sysctl_precision, 0,
			(void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);

		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "channels",
			SYSCTL_DESCR("intermediate channels"),
			audio_sysctl_channels, 0,
			(void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);

		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "latency",
			SYSCTL_DESCR("latency"),
			audio_sysctl_latency, 0,
			(void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);

		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_BOOL, "multiuser",
			SYSCTL_DESCR("allow multiple user acess"),
			NULL, 0,
			&sc->sc_multiuser, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);

		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_BOOL, "usemixer",
			SYSCTL_DESCR("allow in-kernel mixing"),
			audio_sysctl_usemixer, 0,
			(void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);
	}

	selinit(&sc->sc_rsel);
	selinit(&sc->sc_wsel);

#ifdef AUDIO_PM_IDLE
	callout_init(&sc->sc_idle_counter, 0);
	callout_setfunc(&sc->sc_idle_counter, audio_idle, self);
#endif

	if (!pmf_device_register(self, audio_suspend, audio_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
#ifdef AUDIO_PM_IDLE
	if (!device_active_register(self, audio_activity))
		aprint_error_dev(self, "couldn't register activity handler\n");
#endif

	if (!pmf_event_register(self, PMFE_AUDIO_VOLUME_DOWN,
	    audio_volume_down, true))
		aprint_error_dev(self, "couldn't add volume down handler\n");
	if (!pmf_event_register(self, PMFE_AUDIO_VOLUME_UP,
	    audio_volume_up, true))
		aprint_error_dev(self, "couldn't add volume up handler\n");
	if (!pmf_event_register(self, PMFE_AUDIO_VOLUME_TOGGLE,
	    audio_volume_toggle, true))
		aprint_error_dev(self, "couldn't add volume toggle handler\n");

#ifdef AUDIO_PM_IDLE
	callout_schedule(&sc->sc_idle_counter, audio_idle_timeout * hz);
#endif
	kthread_create(PRI_SOFTSERIAL, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN, NULL,
	    audio_rec_thread, sc, &sc->sc_recthread, "audiorec");
	kthread_create(PRI_SOFTSERIAL, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN, NULL,
	    audio_play_thread, sc, &sc->sc_playthread, "audiomix");
	audiorescan(self, "audio", NULL);
}

static int
audioactivate(device_t self, enum devact act)
{
	struct audio_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		mutex_enter(sc->sc_lock);
		sc->sc_dying = true;
		mutex_enter(sc->sc_intr_lock);
		cv_broadcast(&sc->sc_condvar);
		cv_broadcast(&sc->sc_rcondvar);
		cv_broadcast(&sc->sc_wchan);
		cv_broadcast(&sc->sc_rchan);
		cv_broadcast(&sc->sc_lchan);
		mutex_exit(sc->sc_intr_lock);
		mutex_exit(sc->sc_lock);
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static int
audiodetach(device_t self, int flags)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	int maj, mn, rc;

	sc = device_private(self);
	DPRINTF(("audio_detach: sc=%p flags=%d\n", sc, flags));

	/* Start draining existing accessors of the device. */
	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;
	mutex_enter(sc->sc_lock);
	sc->sc_dying = true;
	cv_broadcast(&sc->sc_wchan);
	cv_broadcast(&sc->sc_rchan);
	mutex_enter(sc->sc_intr_lock);
	cv_broadcast(&sc->sc_condvar);
	cv_broadcast(&sc->sc_rcondvar);
	mutex_exit(sc->sc_intr_lock);
	mutex_exit(sc->sc_lock);
	kthread_join(sc->sc_playthread);
	kthread_join(sc->sc_recthread);
	mutex_enter(sc->sc_lock);
	cv_destroy(&sc->sc_condvar);
	cv_destroy(&sc->sc_rcondvar);
	mutex_exit(sc->sc_lock);

	/* delete sysctl nodes */
	sysctl_teardown(&sc->sc_log);

	/* locate the major number */
	maj = cdevsw_lookup_major(&audio_cdevsw);

	/*
	 * Nuke the vnodes for any open instances (calls close).
	 * Will wait until any activity on the device nodes has ceased.
	 *
	 * XXXAD NOT YET.
	 *
	 * XXXAD NEED TO PREVENT NEW REFERENCES THROUGH AUDIO_ENTER().
	 */
	mn = device_unit(self);
	vdevgone(maj, mn | SOUND_DEVICE,    mn | SOUND_DEVICE, VCHR);
	vdevgone(maj, mn | AUDIO_DEVICE,    mn | AUDIO_DEVICE, VCHR);
	vdevgone(maj, mn | AUDIOCTL_DEVICE, mn | AUDIOCTL_DEVICE, VCHR);
	vdevgone(maj, mn | MIXER_DEVICE,    mn | MIXER_DEVICE, VCHR);

	pmf_event_deregister(self, PMFE_AUDIO_VOLUME_DOWN,
	    audio_volume_down, true);
	pmf_event_deregister(self, PMFE_AUDIO_VOLUME_UP,
	    audio_volume_up, true);
	pmf_event_deregister(self, PMFE_AUDIO_VOLUME_TOGGLE,
	    audio_volume_toggle, true);

#ifdef AUDIO_PM_IDLE
	callout_halt(&sc->sc_idle_counter, sc->sc_lock);

	device_active_deregister(self, audio_activity);
#endif

	pmf_device_deregister(self);

	/* free resources */
	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		audio_free_ring(sc, &chan->vc->sc_mpr);
		audio_free_ring(sc, &chan->vc->sc_mrr);
	}
	audio_free_ring(sc, &sc->sc_hwvc->sc_mpr);
	audio_free_ring(sc, &sc->sc_hwvc->sc_mrr);
	audio_free_ring(sc, &sc->sc_mixring.sc_mpr);
	audio_free_ring(sc, &sc->sc_mixring.sc_mrr);
	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		audio_destroy_pfilters(chan->vc);
		audio_destroy_rfilters(chan->vc);
	}
	audio_destroy_pfilters(sc->sc_hwvc);
	audio_destroy_rfilters(sc->sc_hwvc);

	auconv_delete_encodings(sc->sc_encodings);

	if (sc->sc_sih_rd) {
		softint_disestablish(sc->sc_sih_rd);
		sc->sc_sih_rd = NULL;
	}
	if (sc->sc_sih_wr) {
		softint_disestablish(sc->sc_sih_wr);
		sc->sc_sih_wr = NULL;
	}

	kmem_free(sc->sc_hwvc, sizeof(struct virtual_channel));
	kmem_free(sc->sc_mixer_state, sizeof(mixer_ctrl_t) *
	    (sc->sc_nmixer_states + 1));

#ifdef AUDIO_PM_IDLE
	callout_destroy(&sc->sc_idle_counter);
#endif
	seldestroy(&sc->sc_rsel);
	seldestroy(&sc->sc_wsel);

	cv_destroy(&sc->sc_rchan);
	cv_destroy(&sc->sc_wchan);
	cv_destroy(&sc->sc_lchan);

	return 0;
}

static void
audiochilddet(device_t self, device_t child)
{

	/* we hold no child references, so do nothing */
}

static int
audiosearch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{

	if (config_match(parent, cf, aux))
		config_attach_loc(parent, cf, locs, aux, NULL);

	return 0;
}

static int
audiorescan(device_t self, const char *ifattr, const int *flags)
{
	struct audio_softc *sc = device_private(self);

	if (!ifattr_match(ifattr, "audio"))
		return 0;

	config_search_loc(audiosearch, sc->dev, "audio", NULL, NULL);

	return 0;
}


int
au_portof(struct audio_softc *sc, char *name, int class)
{
	mixer_devinfo_t mi;

	for (mi.index = 0; audio_query_devinfo(sc, &mi) == 0; mi.index++) {
		if (mi.mixer_class == class && strcmp(mi.label.name, name) == 0)
			return mi.index;
	}
	return -1;
}

void
au_setup_ports(struct audio_softc *sc, struct au_mixer_ports *ports,
	       mixer_devinfo_t *mi, const struct portname *tbl)
{
	int i, j;

	ports->index = mi->index;
	if (mi->type == AUDIO_MIXER_ENUM) {
		ports->isenum = true;
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
				    ports->miport[ports->nports] != -1)
					ports->isdual = true;
				++ports->nports;
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
				ports->miport[ports->nports] =
				    au_portof(sc, mi->un.s.member[j].label.name,
				    mi->mixer_class);
				++ports->nports;
			}
	}
}

/*
 * Called from hardware driver.  This is where the MI audio driver gets
 * probed/attached to the hardware driver.
 */
device_t
audio_attach_mi(const struct audio_hw_if *ahwp, void *hdlp, device_t dev)
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
	struct virtual_channel *vc;

	vc = sc->sc_hwvc;

	printf("hwhandle %p hw_if %p ", sc->hw_hdl, sc->hw_if);
	printf("open 0x%x mode 0x%x\n", vc->sc_open, vc->sc_mode);
	printf("rchan 0x%x wchan 0x%x ", cv_has_waiters(&sc->sc_rchan),
	    cv_has_waiters(&sc->sc_wchan));
	printf("rring used 0x%x pring used=%d\n",
	    audio_stream_get_used(&vc->sc_mrr.s),
	    audio_stream_get_used(&vc->sc_mpr.s));
	printf("rbus 0x%x pbus 0x%x ", vc->sc_rbus, vc->sc_pbus);
	printf("blksize %d", vc->sc_mpr.blksize);
	printf("hiwat %d lowat %d\n", vc->sc_mpr.usedhigh,
	    vc->sc_mpr.usedlow);
}

void
audio_print_params(const char *s, struct audio_params *p)
{
	printf("%s enc=%u %uch %u/%ubit %uHz\n", s, p->encoding, p->channels,
	       p->validbits, p->precision, p->sample_rate);
}
#endif

/* Allocate all ring buffers. called from audioattach() */
static int
audio_allocbufs(struct audio_softc *sc)
{
	struct virtual_channel *vc;
	int error;

	vc = sc->sc_hwvc;

	sc->sc_mixring.sc_mpr.s.start = NULL;
	vc->sc_mpr.s.start = NULL;
	sc->sc_mixring.sc_mrr.s.start = NULL;
	vc->sc_mrr.s.start = NULL;

	if (audio_can_playback(sc)) {
		error = audio_alloc_ring(sc, &sc->sc_mixring.sc_mpr,
		    AUMODE_PLAY, AU_RING_SIZE);
		if (error)
			goto bad_play1;

		error = audio_alloc_ring(sc, &vc->sc_mpr,
		    AUMODE_PLAY, AU_RING_SIZE);
		if (error)
			goto bad_play2;
	}
	if (audio_can_capture(sc)) {
		error = audio_alloc_ring(sc, &sc->sc_mixring.sc_mrr,
		    AUMODE_RECORD, AU_RING_SIZE);
		if (error)
			goto bad_rec1;

		error = audio_alloc_ring(sc, &vc->sc_mrr,
		    AUMODE_RECORD, AU_RING_SIZE);
		if (error)
			goto bad_rec2;
	}
	return 0;

bad_rec2:
	if (sc->sc_mixring.sc_mrr.s.start != NULL)
		audio_free_ring(sc, &sc->sc_mixring.sc_mrr);
bad_rec1:
	if (vc->sc_mpr.s.start != NULL)
		audio_free_ring(sc, &vc->sc_mpr);
bad_play2:
	if (sc->sc_mixring.sc_mpr.s.start != NULL)
		audio_free_ring(sc, &sc->sc_mixring.sc_mpr);
bad_play1:
	return error;
}

int
audio_alloc_ring(struct audio_softc *sc, struct audio_ringbuffer *r,
		 int direction, size_t bufsize)
{
	const struct audio_hw_if *hw;
	struct virtual_channel *vc;
	void *hdl;
	vaddr_t vstart;
	vsize_t vsize;
	int error;

	vc = sc->sc_hwvc;
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

	if (hw->allocm && (r == &vc->sc_mpr || r == &vc->sc_mrr)) {
		/* Hardware ringbuffer.	 No dedicated uvm object.*/
		r->uobj = NULL;
		r->s.start = hw->allocm(hdl, direction, bufsize);
		if (r->s.start == NULL)
			return ENOMEM;
	} else {
		/* Software ringbuffer.	 */
		vstart = 0;

		/* Get a nonzero multiple of PAGE_SIZE.	 */
		vsize = roundup2(MAX(bufsize, PAGE_SIZE), PAGE_SIZE);

		/* Create a uvm anonymous object.  */
		r->uobj = uao_create(vsize, 0);

		/* Map it into the kernel virtual address space.  */
		error = uvm_map(kernel_map, &vstart, vsize, r->uobj, 0, 0,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_NONE,
			UVM_ADV_RANDOM, 0));
		if (error) {
			uao_detach(r->uobj);	/* release reference */
			r->uobj = NULL;		/* paranoia */
			return error;
		}

		error = uvm_map_pageable(kernel_map, vstart, vstart + vsize,
		    false, 0);
		if (error) {
			uvm_unmap(kernel_map, vstart, vstart + vsize);
			uao_detach(r->uobj);
			r->uobj = NULL;		/* paranoia */
			return error;
		}
		r->s.start = (void *)vstart;
	}

	r->s.bufsize = bufsize;

	return 0;
}

void
audio_free_ring(struct audio_softc *sc, struct audio_ringbuffer *r)
{
	struct virtual_channel *vc;
	vaddr_t vstart;
	vsize_t vsize;

	if (r->s.start == NULL)
		return;

	vc = sc->sc_hwvc;

	if (sc->hw_if->freem && (r == &vc->sc_mpr || r == &vc->sc_mrr)) {
		 /* Hardware ringbuffer.  */
		KASSERT(r->uobj == NULL);
		sc->hw_if->freem(sc->hw_hdl, r->s.start, r->s.bufsize);
	} else {
		/* Software ringbuffer.  */
		vstart = (vaddr_t)r->s.start;
		vsize = roundup2(MAX(r->s.bufsize, PAGE_SIZE), PAGE_SIZE);

		/*
		 * Unmap the kernel mapping.  uvm_unmap releases the
		 * reference to the uvm object, and this should be the
		 * last virtual mapping of the uvm object, so no need
		 * to explicitly release (`detach') the object.
		 */
		uvm_unmap(kernel_map, vstart, vstart + vsize);

		r->uobj = NULL;		/* paranoia */
	}

	r->s.start = NULL;
}

static int
audio_setup_pfilters(struct audio_softc *sc, const audio_params_t *pp,
		     stream_filter_list_t *pfilters, struct virtual_channel *vc)
{
	stream_filter_t *pf[AUDIO_MAX_FILTERS], *of[AUDIO_MAX_FILTERS];
	audio_stream_t ps[AUDIO_MAX_FILTERS], os[AUDIO_MAX_FILTERS];
	const audio_params_t *from_param;
	audio_params_t *to_param;
	int i, n, onfilters;

	KASSERT(mutex_owned(sc->sc_lock));

	/* Construct new filters. */
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
		return EINVAL;
	}

	/* Swap in new filters. */
	HW_LOCK(vc);
	memcpy(of, vc->sc_pfilters, sizeof(of));
	memcpy(os, vc->sc_pstreams, sizeof(os));
	onfilters = vc->sc_npfilters;
	memcpy(vc->sc_pfilters, pf, sizeof(pf));
	memcpy(vc->sc_pstreams, ps, sizeof(ps));
	vc->sc_npfilters = pfilters->req_size;
	for (i = 0; i < pfilters->req_size; i++)
		pf[i]->set_inputbuffer(pf[i], &vc->sc_pstreams[i]);

	/* hardware format and the buffer near to userland */
	if (pfilters->req_size <= 0) {
		vc->sc_mpr.s.param = *pp;
		vc->sc_pustream = &vc->sc_mpr.s;
	} else {
		vc->sc_mpr.s.param = pfilters->filters[0].param;
		vc->sc_pustream = &vc->sc_pstreams[0];
	}
	HW_UNLOCK(vc);

	/* Destroy old filters. */
	for (i = 0; i < onfilters; i++) {
		of[i]->dtor(of[i]);
		audio_stream_dtor(&os[i]);
	}

#ifdef AUDIO_DEBUG
	if (audiodebug) {
		printf("%s: HW-buffer=%p pustream=%p\n",
		       __func__, &vc->sc_mpr.s, vc->sc_pustream);
		for (i = 0; i < pfilters->req_size; i++) {
			char num[100];
			snprintf(num, 100, "[%d]", i);
			audio_print_params(num, &vc->sc_pstreams[i].param);
		}
		audio_print_params("[HW]", &vc->sc_mpr.s.param);
	}
#endif /* AUDIO_DEBUG */

	return 0;
}

static int
audio_setup_rfilters(struct audio_softc *sc, const audio_params_t *rp,
		     stream_filter_list_t *rfilters, struct virtual_channel *vc)
{
	stream_filter_t *rf[AUDIO_MAX_FILTERS], *of[AUDIO_MAX_FILTERS];
	audio_stream_t rs[AUDIO_MAX_FILTERS], os[AUDIO_MAX_FILTERS];
	const audio_params_t *to_param;
	audio_params_t *from_param;
	int i, onfilters;

	KASSERT(mutex_owned(sc->sc_lock));

	/* Construct new filters. */
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
			rf[0]->set_inputbuffer(rf[0], &vc->sc_mrr.s);
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

	/* Swap in new filters. */
	HW_LOCK(vc);
	memcpy(of, vc->sc_rfilters, sizeof(of));
	memcpy(os, vc->sc_rstreams, sizeof(os));
	onfilters = vc->sc_nrfilters;
	memcpy(vc->sc_rfilters, rf, sizeof(rf));
	memcpy(vc->sc_rstreams, rs, sizeof(rs));
	vc->sc_nrfilters = rfilters->req_size;
	for (i = 1; i < rfilters->req_size; i++)
		rf[i]->set_inputbuffer(rf[i], &vc->sc_rstreams[i - 1]);

	/* hardware format and the buffer near to userland */
	if (rfilters->req_size <= 0) {
		vc->sc_mrr.s.param = *rp;
		vc->sc_rustream = &vc->sc_mrr.s;
	} else {
		vc->sc_mrr.s.param = rfilters->filters[0].param;
		vc->sc_rustream = &vc->sc_rstreams[rfilters->req_size - 1];
	}
	HW_UNLOCK(vc);

#ifdef AUDIO_DEBUG
	if (audiodebug) {
		printf("%s: HW-buffer=%p rustream=%p\n",
		       __func__, &vc->sc_mrr.s, vc->sc_rustream);
		audio_print_params("[HW]", &vc->sc_mrr.s.param);
		for (i = 0; i < rfilters->req_size; i++) {
			char num[100];
			snprintf(num, 100, "[%d]", i);
			audio_print_params(num, &vc->sc_rstreams[i].param);
		}
	}
#endif /* AUDIO_DEBUG */

	/* Destroy old filters. */
	for (i = 0; i < onfilters; i++) {
		of[i]->dtor(of[i]);
		audio_stream_dtor(&os[i]);
	}

	return 0;
}

static void
audio_destroy_pfilters(struct virtual_channel *vc)
{
	int i;

	for (i = 0; i < vc->sc_npfilters; i++) {
		vc->sc_pfilters[i]->dtor(vc->sc_pfilters[i]);
		vc->sc_pfilters[i] = NULL;
		audio_stream_dtor(&vc->sc_pstreams[i]);
	}
	vc->sc_npfilters = 0;
}

static void
audio_destroy_rfilters(struct virtual_channel *vc)
{
	int i;

	for (i = 0; i < vc->sc_nrfilters; i++) {
		vc->sc_rfilters[i]->dtor(vc->sc_rfilters[i]);
		vc->sc_rfilters[i] = NULL;
		audio_stream_dtor(&vc->sc_pstreams[i]);
	}
	vc->sc_nrfilters = 0;
}

static void
audio_stream_dtor(audio_stream_t *stream)
{

	if (stream->start != NULL)
		kmem_free(stream->start, stream->bufsize);
	memset(stream, 0, sizeof(audio_stream_t));
}

static int
audio_stream_ctor(audio_stream_t *stream, const audio_params_t *param, int size)
{
	int frame_size;

	size = min(size, AU_RING_SIZE);
	stream->bufsize = size;
	stream->start = kmem_zalloc(size, KM_SLEEP);
	frame_size = (param->precision + 7) / 8 * param->channels;
	size = (size / frame_size) * frame_size;
	stream->end = stream->start + size;
	stream->inp = stream->start;
	stream->outp = stream->start;
	stream->used = 0;
	stream->param = *param;
	stream->loop = false;
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

/*
 * Look up audio device and acquire locks for device access.
 */
static int
audio_enter(dev_t dev, krw_t rw, struct audio_softc **scp)
{

	struct audio_softc *sc;

	/* First, find the device and take sc_lock. */
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL || sc->hw_if == NULL)
		return ENXIO;
	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		return EIO;
	}

	*scp = sc;
	return 0;
}

/*
 * Release reference to device acquired with audio_enter().
 */
static void
audio_exit(struct audio_softc *sc)
{
	cv_broadcast(&sc->sc_lchan);
	mutex_exit(sc->sc_lock);
}

/*
 * Wait for I/O to complete, releasing device lock.
 */
static int
audio_waitio(struct audio_softc *sc, kcondvar_t *chan, struct virtual_channel *vc)
{
	struct audio_chan *vchan;
	bool found = false;
	int error;

	KASSERT(mutex_owned(sc->sc_lock));
	cv_broadcast(&sc->sc_lchan);

	/* Wait for pending I/O to complete. */
	error = cv_wait_sig(chan, sc->sc_lock);

	if (!sc->sc_usemixer || vc == sc->sc_hwvc)
		return error;

	found = false;
	SIMPLEQ_FOREACH(vchan, &sc->sc_audiochan, entries) {
		if (vchan->vc == vc) {
			found = true;
			break;
		}
	}
	if (found == false)
		error = EIO;

	return error;
}

/* Exported interfaces for audiobell. */
int
audiobellopen(dev_t dev, int flags, int ifmt, struct lwp *l,
	      struct file **fp)
{
	struct audio_softc *sc;
	int error;

	if ((error = audio_enter(dev, RW_WRITER, &sc)) != 0)
		return error;
	device_active(sc->dev, DVA_SYSTEM);
	switch (AUDIODEV(dev)) {
	case AUDIO_DEVICE:
		error = audio_open(dev, sc, flags, ifmt, l, fp);
		break;
	default:
		error = EINVAL;
		break;
	}
	audio_exit(sc);

	return error;
}

int
audiobellclose(struct file *fp)
{

	return audioclose(fp);
}

int
audiobellwrite(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	   int ioflag)
{

	return audiowrite(fp, offp, uio, cred, ioflag);
}

int
audiobellioctl(struct file *fp, u_long cmd, void *addr)
{

	return audioioctl(fp, cmd, addr);
}

static int
audioopen(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct audio_softc *sc;
	struct file *fp;
	int error;

	if ((error = audio_enter(dev, RW_WRITER, &sc)) != 0)
		return error;
	device_active(sc->dev, DVA_SYSTEM);
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
	case AUDIOCTL_DEVICE:
		error = audio_open(dev, sc, flags, ifmt, l, &fp);
		break;
	case MIXER_DEVICE:
		error = mixer_open(dev, sc, flags, ifmt, l, &fp);
		break;
	default:
		error = ENXIO;
		break;
	}
	audio_exit(sc);

	return error;
}

static int
audioclose(struct file *fp)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	int error;
	dev_t dev;

	chan = fp->f_audioctx;
	if (chan == NULL)	/* XXX:NS Why is this needed. */
		return EIO;

	dev = chan->dev;

	if ((error = audio_enter(dev, RW_WRITER, &sc)) != 0)
		return error;

	device_active(sc->dev, DVA_SYSTEM);
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
	case AUDIOCTL_DEVICE:
		error = audio_close(sc, fp->f_flag, chan);
		break;
	case MIXER_DEVICE:
		error = mixer_close(sc, fp->f_flag, chan);
		break;
	default:
		error = ENXIO;
		break;
	}
	if (error == 0) {
		kmem_free(fp->f_audioctx, sizeof(struct audio_chan));
		fp->f_audioctx = NULL;
	}

	audio_exit(sc);

	return error;
}

static int
audioread(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	  int ioflag)
{
	struct audio_softc *sc;
	struct virtual_channel *vc;
	int error;
	dev_t dev;

	if (fp->f_audioctx == NULL)
		return EIO;

	dev = fp->f_audioctx->dev;

	if ((error = audio_enter(dev, RW_READER, &sc)) != 0)
		return error;

	if (fp->f_flag & O_NONBLOCK)
		ioflag |= IO_NDELAY;

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		vc = fp->f_audioctx->vc;
		error = audio_read(sc, uio, ioflag, vc);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}
	audio_exit(sc);

	return error;
}

static int
audiowrite(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	   int ioflag)
{
	struct audio_softc *sc;
	struct virtual_channel *vc;
	int error;
	dev_t dev;

	if (fp->f_audioctx == NULL)
		return EIO;

	dev = fp->f_audioctx->dev;

	if ((error = audio_enter(dev, RW_READER, &sc)) != 0)
		return error;

	if (fp->f_flag & O_NONBLOCK)
		ioflag |= IO_NDELAY;

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		vc = fp->f_audioctx->vc;
		error = audio_write(sc, uio, ioflag, vc);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}
	audio_exit(sc);

	return error;
}

static int
audioioctl(struct file *fp, u_long cmd, void *addr)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	struct lwp *l = curlwp;
	int error;
	krw_t rw;
	dev_t dev;

	if (fp->f_audioctx == NULL)
		return EIO;

	chan = fp->f_audioctx;
	dev = chan->dev;

	/* Figure out which lock type we need. */
	switch (cmd) {
	case AUDIO_FLUSH:
	case AUDIO_SETINFO:
	case AUDIO_DRAIN:
	case AUDIO_SETFD:
		rw = RW_WRITER;
		break;
	default:
		rw = RW_READER;
		break;
	}

	if ((error = audio_enter(dev, rw, &sc)) != 0)
		return error;

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
	case AUDIOCTL_DEVICE:
		device_active(sc->dev, DVA_SYSTEM);
		if (IOCGROUP(cmd) == IOCGROUP(AUDIO_MIXER_READ))
			error = mixer_ioctl(sc, cmd, addr, fp->f_flag, l);
		else
			error = audio_ioctl(dev, sc, cmd, addr, fp->f_flag, l,
			    chan);
		break;
	case MIXER_DEVICE:
		error = mixer_ioctl(sc, cmd, addr, fp->f_flag, l);
		break;
	default:
		error = ENXIO;
		break;
	}
	audio_exit(sc);

	return error;
}

static int
audiostat(struct file *fp, struct stat *st)
{
	if (fp->f_audioctx == NULL)
		return EIO;

	memset(st, 0, sizeof(*st));

	st->st_dev = fp->f_audioctx->dev;

	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;
	return 0;
}

static int
audiopoll(struct file *fp, int events)
{
	struct audio_softc *sc;
	struct virtual_channel *vc;
	struct lwp *l = curlwp;
	int revents;
	dev_t dev;

	if (fp->f_audioctx == NULL)
		return POLLERR;

	dev = fp->f_audioctx->dev;

	/* Don't bother with device level lock here. */
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return POLLERR;
	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		return POLLERR;
	}

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		vc = fp->f_audioctx->vc;
		revents = audio_poll(sc, events, l, vc);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		revents = 0;
		break;
	default:
		revents = POLLERR;
		break;
	}
	mutex_exit(sc->sc_lock);

	return revents;
}

static int
audiokqfilter(struct file *fp, struct knote *kn)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	int error;
	dev_t dev;

	chan = fp->f_audioctx;
	dev = chan->dev;

	/* Don't bother with device level lock here. */
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;
	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		return EIO;
	}
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_kqfilter(chan, kn);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}
	mutex_exit(sc->sc_lock);

	return error;
}

static int
audio_fop_mmap(struct file *fp, off_t *offp, size_t len, int prot, int *flagsp,
	     int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	struct virtual_channel *vc;
	dev_t dev;
	int error;

	chan = fp->f_audioctx;
	dev = chan->dev;
	vc = chan->vc;
	error = 0;

	if ((error = audio_enter(dev, RW_READER, &sc)) != 0)
		return error;
	device_active(sc->dev, DVA_SYSTEM); /* XXXJDM */

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_mmap(sc, offp, len, prot, flagsp, advicep,
		    uobjp, maxprotp, vc);	
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
	default:
		error = ENOTSUP;
		break;
	}
	audio_exit(sc);

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
	if (blksize > (int)(rp->s.bufsize / AUMINNOBLK))
		blksize = rp->s.bufsize / AUMINNOBLK;
	ROUNDSIZE(blksize);
	DPRINTF(("audio_init_ringbuffer: MI blksize=%d\n", blksize));

	struct virtual_channel *hwvc = sc->sc_hwvc;

	int tmpblksize = 1; 	 
	/* round blocksize to a power of 2 */ 	 
	while (tmpblksize < blksize)
		tmpblksize <<= 1; 	 

	blksize = tmpblksize;

	if (sc->hw_if->round_blocksize &&
	    (rp == &hwvc->sc_mpr || rp == &hwvc->sc_mrr || rp ==
	    &sc->sc_mixring.sc_mpr || rp == &sc->sc_mixring.sc_mrr)) {
		blksize = sc->hw_if->round_blocksize(sc->hw_hdl, blksize,
		    mode, &rp->s.param);
	}

	if (blksize <= 0)
		panic("audio_init_ringbuffer: blksize=%d", blksize);
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
	rp->copying = false;
	rp->needfill = false;
	rp->mmapped = false;
	memset(rp->s.start, 0, blksize * 2);
}

int
audio_initbufs(struct audio_softc *sc, struct virtual_channel *vc)
{
	const struct audio_hw_if *hw;
	int error;

	if (vc == NULL)
		vc = sc->sc_hwvc;

	DPRINTF(("audio_initbufs: mode=0x%x\n", vc->sc_mode));
	hw = sc->hw_if;
	if (audio_can_capture(sc) &&
		((vc->sc_open & AUOPEN_READ) || vc == sc->sc_hwvc)) {
		audio_init_ringbuffer(sc, &vc->sc_mrr,
		    AUMODE_RECORD);
		if (sc->sc_recopens == 0 && (vc->sc_open & AUOPEN_READ)) {
			if (hw->init_input) {
				error = hw->init_input(sc->hw_hdl,
				    vc->sc_mrr.s.start,
				    vc->sc_mrr.s.end - vc->sc_mrr.s.start);
				if (error)
					return error;
			}
		}
	}

	if (audio_can_playback(sc) &&
		((vc->sc_open & AUOPEN_WRITE) || vc == sc->sc_hwvc)) {
		audio_init_ringbuffer(sc, &vc->sc_mpr,
		    AUMODE_PLAY);
		if (sc->sc_opens == 0 && (vc->sc_open & AUOPEN_WRITE)) {
			if (hw->init_output) {
				error = hw->init_output(sc->hw_hdl,
				    vc->sc_mpr.s.start,
				    vc->sc_mpr.s.end - vc->sc_mpr.s.start);
				if (error)
					return error;
			}
		}
	}

#ifdef AUDIO_INTR_TIME
	if (audio_can_playback(sc)) {
		sc->sc_pnintr = 0;
		sc->sc_pblktime = (int64_t)vc->sc_mpr.blksize * 1000000 /
		    (vc->sc_pparams.channels *
		     vc->sc_pparams.sample_rate *
		     vc->sc_pparams.precision / NBBY);
		DPRINTF(("audio: play blktime = %" PRId64 " for %d\n",
			 sc->sc_pblktime, vc->sc_mpr.blksize));
	}
	if (audio_can_capture(sc)) {
		sc->sc_rnintr = 0;
		sc->sc_rblktime = (int64_t)vc->sc_mrr.blksize * 1000000 /
		    (vc->sc_rparams.channels *
		     vc->sc_rparams.sample_rate *
		     vc->sc_rparams.precision / NBBY);
		DPRINTF(("audio: record blktime = %" PRId64 " for %d\n",
			 sc->sc_rblktime, vc->sc_mrr.blksize));
	}
#endif

	return 0;
}

void
audio_calcwater(struct audio_softc *sc, struct virtual_channel *vc)
{
	/* set high at 100% */
	if (audio_can_playback(sc) && vc && vc->sc_pustream) {
		vc->sc_mpr.usedhigh =
		    vc->sc_pustream->end - vc->sc_pustream->start;
		/* set low at 75% of usedhigh */
		vc->sc_mpr.usedlow = vc->sc_mpr.usedhigh * 3 / 4;
		if (vc->sc_mpr.usedlow == vc->sc_mpr.usedhigh)
			vc->sc_mpr.usedlow -= vc->sc_mpr.blksize;
	}

	if (audio_can_capture(sc) && vc && vc->sc_rustream) {
		vc->sc_mrr.usedhigh =
		    vc->sc_rustream->end - vc->sc_rustream->start -
		    vc->sc_mrr.blksize;
		vc->sc_mrr.usedlow = 0;
		DPRINTF(("%s: plow=%d phigh=%d rlow=%d rhigh=%d\n", __func__,
			 vc->sc_mpr.usedlow, vc->sc_mpr.usedhigh,
			 vc->sc_mrr.usedlow, vc->sc_mrr.usedhigh));
	}
}

int
audio_open(dev_t dev, struct audio_softc *sc, int flags, int ifmt,
    struct lwp *l, struct file **nfp)
{
	struct file *fp;
	int error, fd, n;
	u_int mode;
	const struct audio_hw_if *hw;
	struct virtual_channel *vc;
	struct audio_chan *chan;

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->sc_usemixer && !sc->sc_ready)
		return ENXIO;

	hw = sc->hw_if;
	if (hw == NULL)
		return ENXIO;

	n = 1;
	chan = SIMPLEQ_LAST(&sc->sc_audiochan, audio_chan, entries);
	if (chan != NULL)
		n = chan->chan + 1;

	chan = kmem_zalloc(sizeof(struct audio_chan), KM_SLEEP);
	if (sc->sc_usemixer)
		vc = kmem_zalloc(sizeof(struct virtual_channel), KM_SLEEP);
	else
		vc = sc->sc_hwvc;
	chan->vc = vc;

	if (sc->sc_usemixer) {
		vc->sc_open = 0;
		vc->sc_mode = 0;
		vc->sc_nrfilters = 0;
		memset(vc->sc_rfilters, 0,
		    sizeof(vc->sc_rfilters));
		vc->sc_rbus = false;
		vc->sc_npfilters = 0;
		memset(vc->sc_pfilters, 0,
		    sizeof(vc->sc_pfilters));
		vc->sc_draining = false;
		vc->sc_pbus = false;
		vc->sc_lastinfovalid = false;
		vc->sc_swvol = 255;
		vc->sc_recswvol = 255;
	} else {
		if (sc->sc_opens > 0 || sc->sc_recopens > 0 ) {
			kmem_free(chan, sizeof(struct audio_chan));
			return EBUSY;
		}
	}

	DPRINTF(("audio_open: flags=0x%x sc=%p hdl=%p\n",
		 flags, sc, sc->hw_hdl));

	if (sc->sc_usemixer) {
		error = audio_alloc_ring(sc, &vc->sc_mpr, AUMODE_PLAY,
		    AU_RING_SIZE);
		if (error)
			goto bad;
		error = audio_alloc_ring(sc, &vc->sc_mrr, AUMODE_RECORD,
		    AU_RING_SIZE);
		if (error)
			goto bad;
	}

	if (!sc->sc_usemixer || sc->sc_opens + sc->sc_recopens == 0) {
		sc->sc_credentials = kauth_cred_get();
		kauth_cred_hold(sc->sc_credentials);
		if (hw->open != NULL) {
			mutex_enter(sc->sc_intr_lock);
			error = hw->open(sc->hw_hdl, flags);
			mutex_exit(sc->sc_intr_lock);
			if (error) {
				goto bad;
			}
		}
		audio_initbufs(sc, NULL);
		if (sc->sc_usemixer && audio_can_playback(sc))
			audio_init_ringbuffer(sc, &sc->sc_mixring.sc_mpr,
			    AUMODE_PLAY);
		if (sc->sc_usemixer && audio_can_capture(sc))
			audio_init_ringbuffer(sc, &sc->sc_mixring.sc_mrr,
			    AUMODE_RECORD);
		sc->schedule_wih = false;
		sc->schedule_rih = false;
		sc->sc_last_drops = 0;
		sc->sc_eof = 0;
		vc->sc_rbus = false;
		sc->sc_async_audio = 0;
	} else if (sc->sc_multiuser == false) {
		/* XXX:NS Should be handled correctly. */
		/* Do we allow multi user access */
		if (kauth_cred_geteuid(sc->sc_credentials) !=
		    kauth_cred_geteuid(kauth_cred_get()) &&
		    kauth_cred_geteuid(kauth_cred_get()) != 0) {
			error = EPERM;
			goto bad;
		}
	}

	mutex_enter(sc->sc_intr_lock);
	vc->sc_full_duplex = 
		(flags & (FWRITE|FREAD)) == (FWRITE|FREAD) &&
		(audio_get_props(sc) & AUDIO_PROP_FULLDUPLEX);
	mutex_exit(sc->sc_intr_lock);

	mode = 0;
	if (flags & FREAD) {
		vc->sc_open |= AUOPEN_READ;
		mode |= AUMODE_RECORD;
	}
	if (flags & FWRITE) {
		vc->sc_open |= AUOPEN_WRITE;
		mode |= AUMODE_PLAY | AUMODE_PLAY_ALL;
	}

	/*
	 * Multiplex device: /dev/audio (MU-Law) and /dev/sound (linear)
	 * The /dev/audio is always (re)set to 8-bit MU-Law mono
	 * For the other devices, you get what they were last set to.
	 */
	if (ISDEVSOUND(dev) && sc->sc_aivalid == true) {
		sc->sc_ai.mode = mode;
		sc->sc_ai.play.port = ~0;
		sc->sc_ai.record.port = ~0;
		error = audiosetinfo(sc, &sc->sc_ai, true, vc);
	} else
		error = audio_set_defaults(sc, mode, vc);
	if (error)
		goto bad;

#ifdef DIAGNOSTIC
	/*
	 * Sample rate and precision are supposed to be set to proper
	 * default values by the hardware driver, so that it may give
	 * us these values.
	 */
	if (vc->sc_rparams.precision == 0 || vc->sc_pparams.precision == 0) {
		printf("audio_open: 0 precision\n");
		goto bad;
	}
#endif

	/* audio_close() decreases sc_mpr[n].usedlow, recalculate here */
	audio_calcwater(sc, vc);

	error = fd_allocfile(&fp, &fd);
	if (error)
		goto bad;

	DPRINTF(("audio_open: done sc_mode = 0x%x\n", vc->sc_mode));

	if (sc->sc_usemixer)
		grow_mixer_states(sc, 2);
	if (flags & FREAD)
		sc->sc_recopens++;
	if (flags & FWRITE)
		sc->sc_opens++;
	chan->dev = dev;
	chan->chan = n;
	chan->deschan = n;
	if (sc->sc_usemixer)
		SIMPLEQ_INSERT_TAIL(&sc->sc_audiochan, chan, entries);

	error = fd_clone(fp, fd, flags, &audio_fileops, chan);
	KASSERT(error == EMOVEFD);

	*nfp = fp;
	return error;

bad:
	audio_destroy_pfilters(vc);
	audio_destroy_rfilters(vc);
	if (hw->close != NULL && sc->sc_opens == 0 && sc->sc_recopens == 0)
		hw->close(sc->hw_hdl);
	mutex_exit(sc->sc_lock);
	if (sc->sc_usemixer) {
		audio_free_ring(sc, &vc->sc_mpr);
		audio_free_ring(sc, &vc->sc_mrr);
		mutex_enter(sc->sc_lock);
		kmem_free(vc, sizeof(struct virtual_channel));
	} else
		mutex_enter(sc->sc_lock);

	kmem_free(chan, sizeof(struct audio_chan));
	return error;
}

/*
 * Must be called from task context.
 */
void
audio_init_record(struct audio_softc *sc, struct virtual_channel *vc)
{

	KASSERT(mutex_owned(sc->sc_lock));
	
	if (sc->sc_recopens != 0)
		return;

	mutex_enter(sc->sc_intr_lock);
	if (sc->hw_if->speaker_ctl &&
	    (!vc->sc_full_duplex || (vc->sc_mode & AUMODE_PLAY) == 0))
		sc->hw_if->speaker_ctl(sc->hw_hdl, SPKR_OFF);
	mutex_exit(sc->sc_intr_lock);
}

/*
 * Must be called from task context.
 */
void
audio_init_play(struct audio_softc *sc, struct virtual_channel *vc)
{

	KASSERT(mutex_owned(sc->sc_lock));
	
	if (sc->sc_opens != 0)
		return;

	mutex_enter(sc->sc_intr_lock);
	vc->sc_wstamp = vc->sc_mpr.stamp;
	if (sc->hw_if->speaker_ctl)
		sc->hw_if->speaker_ctl(sc->hw_hdl, SPKR_ON);
	mutex_exit(sc->sc_intr_lock);
}

int
audio_drain(struct audio_softc *sc, struct virtual_channel *vc)
{
	struct audio_ringbuffer *cb;
	int error, cc, i, used;
	uint drops;
	bool hw = false;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(mutex_owned(sc->sc_intr_lock));
	
	error = 0;
	DPRINTF(("audio_drain: enter busy=%d\n", vc->sc_pbus));
	cb = &vc->sc_mpr;
	if (cb->mmapped)
		return 0;

	used = audio_stream_get_used(&cb->s);
	if (vc == sc->sc_hwvc && sc->sc_usemixer) {
		hw = true;
		used += audio_stream_get_used(&sc->sc_mixring.sc_mpr.s);
	}
	for (i = 0; i < vc->sc_npfilters; i++)
		used += audio_stream_get_used(&vc->sc_pstreams[i]);
	if (used <= 0)
		return 0;

	if (hw == false && !vc->sc_pbus) {
		/* We've never started playing, probably because the
		 * block was too short.  Pad it and start now.
		 */
		uint8_t *inp = cb->s.inp;
		int blksize = sc->sc_mixring.sc_mpr.blksize;

		cc = blksize - (inp - cb->s.start) % blksize;
		audio_fill_silence(&cb->s.param, inp, cc);
		cb->s.inp = audio_stream_add_inp(&cb->s, inp, cc);
		mutex_exit(sc->sc_intr_lock);
		error = audiostartp(sc, vc);
		mutex_enter(sc->sc_intr_lock);
		if (error)
			return error;
	} else if (hw == true) {
		used = cb->blksize - (sc->sc_mixring.sc_mpr.s.inp -
		    sc->sc_mixring.sc_mpr.s.start) % cb->blksize;
		while (used > 0) {
			cc = sc->sc_mixring.sc_mpr.s.end -
			    sc->sc_mixring.sc_mpr.s.inp;
			if (cc > used)
				cc = used;
			audio_fill_silence(&cb->s.param,
			    sc->sc_mixring.sc_mpr.s.inp, cc);
			sc->sc_mixring.sc_mpr.s.inp =
			    audio_stream_add_inp(&sc->sc_mixring.sc_mpr.s,
				sc->sc_mixring.sc_mpr.s.inp, cc);
			used -= cc;
		}
		mix_write(sc);
	}
	/*
	 * Play until a silence block has been played, then we
	 * know all has been drained.
	 * XXX This should be done some other way to avoid
	 * playing silence.
	 */
#ifdef DIAGNOSTIC
	if (cb->copying) {
		DPRINTF(("audio_drain: copying in progress!?!\n"));
		cb->copying = false;
	}
#endif
	vc->sc_draining = true;

	drops = cb->drops;
	if (vc == sc->sc_hwvc)
		drops += cb->blksize;
	else if (sc->sc_usemixer)
		drops += sc->sc_mixring.sc_mpr.blksize * PREFILL_BLOCKS;

	error = 0;
	while (cb->drops <= drops && !error) {
		DPRINTF(("audio_drain: vc=%p used=%d, drops=%ld\n",
			vc,
			audio_stream_get_used(&vc->sc_mpr.s),
			cb->drops));
		mutex_exit(sc->sc_intr_lock);
		error = audio_waitio(sc, &sc->sc_wchan, vc);
		mutex_enter(sc->sc_intr_lock);
		if (sc->sc_dying)
			error = EIO;
	}
	vc->sc_draining = false;

	return error;
}

/*
 * Close an audio chip.
 */
/* ARGSUSED */
int
audio_close(struct audio_softc *sc, int flags, struct audio_chan *chan)
{
	struct virtual_channel *vc;
	const struct audio_hw_if *hw;

	KASSERT(mutex_owned(sc->sc_lock));
	
	if (sc->sc_opens == 0 && sc->sc_recopens == 0)
		return ENXIO;

	vc = chan->vc;

	hw = sc->hw_if;
	if (hw == NULL)
		return ENXIO;
	mutex_enter(sc->sc_intr_lock);
	DPRINTF(("audio_close: sc=%p\n", sc));
	/* Stop recording. */
	if (sc->sc_recopens == 1 && (flags & FREAD) && vc->sc_rbus) {
		/*
		 * XXX Some drivers (e.g. SB) use the same routine
		 * to halt input and output so don't halt input if
		 * in full duplex mode.  These drivers should be fixed.
		 */
		if (!vc->sc_full_duplex || hw->halt_input != hw->halt_output)
			hw->halt_input(sc->hw_hdl);
		vc->sc_rbus = false;
	}
	/*
	 * Block until output drains, but allow ^C interrupt.
	 */
	vc->sc_mpr.usedlow = vc->sc_mpr.blksize;  /* avoid excessive wakeups */
	/*
	 * If there is pending output, let it drain (unless
	 * the output is paused).
	 */
	if ((flags & FWRITE) && vc->sc_pbus) {
		if (!vc->sc_mpr.pause)
			audio_drain(sc, chan->vc);
		vc->sc_pbus = false;
	}
	if ((flags & FWRITE) && (sc->sc_opens == 1)) {
		if (vc->sc_mpr.mmapped == false)
			audio_drain(sc, sc->sc_hwvc);
		if (hw->drain)
			(void)hw->drain(sc->hw_hdl);
		hw->halt_output(sc->hw_hdl);
		sc->sc_trigger_started = false;
	}
	if ((flags & FREAD) && (sc->sc_recopens == 1))
		sc->sc_rec_started = false;

	if (sc->sc_opens + sc->sc_recopens == 1 && hw->close != NULL)
		hw->close(sc->hw_hdl);
	mutex_exit(sc->sc_intr_lock);

	if (sc->sc_opens + sc->sc_recopens == 1) {
		sc->sc_async_audio = 0;
		kauth_cred_free(sc->sc_credentials);
	}

	vc->sc_open = 0;
	vc->sc_mode = 0;
	vc->sc_full_duplex = 0;

	audio_destroy_pfilters(vc);
	audio_destroy_rfilters(vc);

	if (flags & FREAD)
		sc->sc_recopens--;
	if (flags & FWRITE)
		sc->sc_opens--;
	if (sc->sc_usemixer) {
		shrink_mixer_states(sc, 2);
		SIMPLEQ_REMOVE(&sc->sc_audiochan, chan, audio_chan, entries);
		mutex_exit(sc->sc_lock);
		audio_free_ring(sc, &vc->sc_mpr);
		audio_free_ring(sc, &vc->sc_mrr);
		mutex_enter(sc->sc_lock);
		kmem_free(vc, sizeof(struct virtual_channel));
	}

	return 0;
}

int
audio_read(struct audio_softc *sc, struct uio *uio, int ioflag,
	   struct virtual_channel *vc)
{
	struct audio_ringbuffer *cb;
	const uint8_t *outp;
	uint8_t *inp;
	int error, used, n;
	uint cc;

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->hw_if == NULL)
		return ENXIO;

	cb = &vc->sc_mrr;
	if (cb->mmapped)
		return EINVAL;

	DPRINTFN(1,("audio_read: cc=%zu mode=%d\n",
		    uio->uio_resid, vc->sc_mode));

#ifdef AUDIO_PM_IDLE
	if (device_is_active(&sc->dev) || sc->sc_idle)
		device_active(&sc->dev, DVA_SYSTEM);
#endif

	error = 0;
	/*
	 * If hardware is half-duplex and currently playing, return
	 * silence blocks based on the number of blocks we have output.
	 */
	if (!vc->sc_full_duplex && (vc->sc_mode & AUMODE_PLAY)) {
		while (uio->uio_resid > 0 && !error) {
			for(;;) {
				/*
				 * No need to lock, as any wakeup will be
				 * held for us while holding sc_lock.
				 */
				cc = vc->sc_mpr.stamp - vc->sc_wstamp;
				if (cc > 0)
					break;
				DPRINTF(("audio_read: stamp=%lu, wstamp=%lu\n",
					 vc->sc_mpr.stamp, vc->sc_wstamp));
				if (ioflag & IO_NDELAY)
					return EWOULDBLOCK;
				error = audio_waitio(sc, &sc->sc_rchan, vc);
				if (sc->sc_dying)
					error = EIO;
				if (error)
					return error;
			}

			if (uio->uio_resid < cc)
				cc = uio->uio_resid;
			DPRINTFN(1,("audio_read: reading in write mode, "
				    "cc=%d\n", cc));
			error = audio_silence_copyout(sc, cc, uio);
			vc->sc_wstamp += cc;
		}
		return error;
	}

	while (uio->uio_resid > 0 && !error) {
		while ((used = audio_stream_get_used(vc->sc_rustream)) <= 0) {
			if (!vc->sc_rbus && !vc->sc_mrr.pause)
				error = audiostartr(sc, vc);
			if (error)
				return error;
			if (ioflag & IO_NDELAY)
				return EWOULDBLOCK;
			DPRINTFN(2, ("audio_read: sleep used=%d\n", used));
			error = audio_waitio(sc, &sc->sc_rchan, vc);
			if (sc->sc_dying)
				error = EIO;
			if (error)
				return error;
		}

		outp = vc->sc_rustream->outp;
		inp = vc->sc_rustream->inp;
		cb->copying = true;

		/*
		 * cc is the amount of data in the sc_rustream excluding
		 * wrapped data.  Note the tricky case of inp == outp, which
		 * must mean the buffer is full, not empty, because used > 0.
		 */
		cc = outp < inp ? inp - outp :vc->sc_rustream->end - outp;
		DPRINTFN(1,("audio_read: outp=%p, cc=%d\n", outp, cc));

		n = uio->uio_resid;
		mutex_exit(sc->sc_lock);
		error = uiomove(__UNCONST(outp), cc, uio);
		mutex_enter(sc->sc_lock);
		n -= uio->uio_resid; /* number of bytes actually moved */

		vc->sc_rustream->outp = audio_stream_add_outp
			(vc->sc_rustream, outp, n);
		cb->copying = false;
	}
	return error;
}

void
audio_clear(struct audio_softc *sc, struct virtual_channel *vc)
{

	KASSERT(mutex_owned(sc->sc_intr_lock));

	if (vc->sc_rbus) {
		cv_broadcast(&sc->sc_rchan);
		if (sc->sc_recopens == 1) {
			sc->hw_if->halt_input(sc->hw_hdl);
			sc->sc_rec_started = false;
		}
		vc->sc_rbus = false;
		vc->sc_mrr.pause = false;
	}
	if (vc->sc_pbus) {
		cv_broadcast(&sc->sc_wchan);
		vc->sc_pbus = false;
		vc->sc_mpr.pause = false;
	}
}

void
audio_clear_intr_unlocked(struct audio_softc *sc, struct virtual_channel *vc)
{

	mutex_enter(sc->sc_intr_lock);
	audio_clear(sc, vc);
	mutex_exit(sc->sc_intr_lock);
}

static void
audio_calc_latency(struct audio_softc *sc)
{
	const struct audio_params *ap = &sc->sc_vchan_params;

	if (ap->sample_rate == 0 || ap->channels == 0 || ap->precision == 0)
		return;

	sc->sc_latency = sc->sc_hwvc->sc_mpr.blksize * 1000 * PREFILL_BLOCKS
	    * NBBY / ap->sample_rate / ap->channels / ap->precision;
}

static void
audio_setblksize(struct audio_softc *sc, struct virtual_channel *vc,
    int blksize, int mode)
{
	struct audio_ringbuffer *mixcb, *cb;
	audio_params_t *parm;
	audio_stream_t *stream;

	if (mode == AUMODE_RECORD) {
		mixcb = &sc->sc_mixring.sc_mrr;
		cb = &vc->sc_mrr;
		parm = &vc->sc_rparams;
		stream = vc->sc_rustream;
	} else {
		mixcb = &sc->sc_mixring.sc_mpr;
		cb = &vc->sc_mpr;
		parm = &vc->sc_pparams;
		stream = vc->sc_pustream;
	}

	if (sc->sc_usemixer && vc == sc->sc_hwvc) {
		mixcb->blksize = audio_calc_blksize(sc, parm);
		cb->blksize = audio_calc_blksize(sc, &cb->s.param);
	} else {
		cb->blksize = audio_calc_blksize(sc, &stream->param);
		if ((!sc->sc_usemixer && SPECIFIED(blksize)) ||
		    (SPECIFIED(blksize) && blksize > cb->blksize))
			cb->blksize = blksize;
	}
}

int
audio_calc_blksize(struct audio_softc *sc, const audio_params_t *parm)
{
	int blksize;

	blksize = parm->sample_rate * sc->sc_latency * parm->channels /
	    1000 * parm->precision / NBBY / PREFILL_BLOCKS;
	return blksize;
}

void
audio_fill_silence(const struct audio_params *params, uint8_t *p, int n)
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
	case AUDIO_ENCODING_AC3:
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
	struct virtual_channel *vc;
	uint8_t zerobuf[128];
	int error;
	int k;

	vc = sc->sc_hwvc;
	audio_fill_silence(&vc->sc_rparams, zerobuf, sizeof zerobuf);

	error = 0;
	while (n > 0 && uio->uio_resid > 0 && !error) {
		k = min(n, min(uio->uio_resid, sizeof zerobuf));
		mutex_exit(sc->sc_lock);
		error = uiomove(zerobuf, k, uio);
		mutex_enter(sc->sc_lock);
		n -= k;
	}

	return error;
}

static int
uio_fetcher_fetch_to(struct audio_softc *sc, stream_fetcher_t *self,
    audio_stream_t *p, int max_used)
{
	uio_fetcher_t *this;
	int size;
	int stream_space;
	int error;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());

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
		mutex_exit(sc->sc_lock);
		error = uiomove(p->inp, size, this->uio);
		mutex_enter(sc->sc_lock);
		if (error)
			return error;
		p->inp = audio_stream_add_inp(p, p->inp, size);
	} else {
		mutex_exit(sc->sc_lock);
		error = uiomove(p->inp, stream_space, this->uio);
		mutex_enter(sc->sc_lock);
		if (error)
			return error;
		p->inp = audio_stream_add_inp(p, p->inp, stream_space);
		mutex_exit(sc->sc_lock);
		error = uiomove(p->start, size - stream_space, this->uio);
		mutex_enter(sc->sc_lock);
		if (error)
			return error;
		p->inp = audio_stream_add_inp(p, p->inp, size - stream_space);
	}
	this->last_used = audio_stream_get_used(p);
	return 0;
}

static int
null_fetcher_fetch_to(struct audio_softc *sc, stream_fetcher_t *self,
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
audio_write(struct audio_softc *sc, struct uio *uio, int ioflag,
	    struct virtual_channel *vc)
{
	uio_fetcher_t ufetcher;
	audio_stream_t stream;
	struct audio_ringbuffer *cb;
	stream_fetcher_t *fetcher;
	stream_filter_t *filter;
	uint8_t *inp, *einp;
	int saveerror, error, m, cc, used;

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->hw_if == NULL)
		return ENXIO;

	cb = &vc->sc_mpr;

	DPRINTFN(2,("audio_write: sc=%p count=%zu used=%d(hi=%d)\n",
		    sc, uio->uio_resid, audio_stream_get_used(vc->sc_pustream),
		    vc->sc_mpr.usedhigh));
	if (vc->sc_mpr.mmapped)
		return EINVAL;

	if (uio->uio_resid == 0) {
		sc->sc_eof++;
		return 0;
	}

#ifdef AUDIO_PM_IDLE
	if (device_is_active(&sc->dev) || sc->sc_idle)
		device_active(&sc->dev, DVA_SYSTEM);
#endif

	/*
	 * If half-duplex and currently recording, throw away data.
	 */
	if (!vc->sc_full_duplex &&
	    (vc->sc_mode & AUMODE_RECORD)) {
		uio->uio_offset += uio->uio_resid;
		uio->uio_resid = 0;
		DPRINTF(("audio_write: half-dpx read busy\n"));
		return 0;
	}

	if (!(vc->sc_mode & AUMODE_PLAY_ALL) && vc->sc_playdrop > 0) {
		m = min(vc->sc_playdrop, uio->uio_resid);
		DPRINTF(("audio_write: playdrop %d\n", m));
		uio->uio_offset += m;
		uio->uio_resid -= m;
		vc->sc_playdrop -= m;
		if (uio->uio_resid == 0)
			return 0;
	}

	/**
	 * setup filter pipeline
	 */
	uio_fetcher_ctor(&ufetcher, uio, vc->sc_mpr.usedhigh);
	if (vc->sc_npfilters > 0) {
		fetcher = &vc->sc_pfilters[vc->sc_npfilters - 1]->base;
	} else {
		fetcher = &ufetcher.base;
	}

	error = 0;
	while (uio->uio_resid > 0 && !error) {
		/* wait if the first buffer is occupied */
		while ((used = audio_stream_get_used(vc->sc_pustream)) >=
							 cb->usedhigh) {
			DPRINTFN(2, ("audio_write: sleep used=%d lowat=%d "
				     "hiwat=%d\n", used,
				     cb->usedlow, cb->usedhigh));
			if (ioflag & IO_NDELAY)
				return EWOULDBLOCK;
			error = audio_waitio(sc, &sc->sc_wchan, vc);
			if (sc->sc_dying)
				error = EIO;
			if (error)
				return error;
		}
		inp = cb->s.inp;
		cb->copying = true;
		stream = cb->s;
		used = stream.used;

		/* Write to the sc_pustream as much as possible. */
		if (vc->sc_npfilters > 0) {
			filter = vc->sc_pfilters[0];
			filter->set_fetcher(filter, &ufetcher.base);
			fetcher = &vc->sc_pfilters[vc->sc_npfilters - 1]->base;
			if (sc->sc_usemixer)
				cc = sc->sc_mixring.sc_mpr.blksize * 2;
			else
				cc = vc->sc_mpr.blksize * 2;
			error = fetcher->fetch_to(sc, fetcher, &stream, cc);
			if (error != 0) {
				fetcher = &ufetcher.base;
				cc = vc->sc_pustream->end -
				    vc->sc_pustream->start;
				error = fetcher->fetch_to(sc, fetcher,
				    vc->sc_pustream, cc);
			}
		} else {
			fetcher = &ufetcher.base;
			cc = stream.end - stream.start;
			error = fetcher->fetch_to(sc, fetcher, &stream, cc);
		}
		if (vc->sc_npfilters > 0) {
			cb->fstamp += ufetcher.last_used
			    - audio_stream_get_used(vc->sc_pustream);
		}
		cb->s.used += stream.used - used;
		cb->s.inp = stream.inp;
		einp = cb->s.inp;

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
		cb->needfill = false;
		cb->copying = false;
		if (!vc->sc_pbus && !cb->pause) {
			saveerror = error;
			error = audiostartp(sc, vc);
			if (saveerror != 0) {
				/* Report the first error that occurred. */
				error = saveerror;
			}
		}
		if (cc != 0) {
			DPRINTFN(1, ("audio_write: fill %d\n", cc));
			audio_fill_silence(&cb->s.param, einp, cc);
		}
	}

	return error;
}

int
audio_ioctl(dev_t dev, struct audio_softc *sc, u_long cmd, void *addr, int flag,
	    struct lwp *l, struct audio_chan *chan)
{
	const struct audio_hw_if *hw;
	struct audio_chan *pchan;
	struct virtual_channel *vc;
	struct audio_offset *ao;
	u_long stamp;
	int error, offs, fd;
	bool rbus, pbus;

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->sc_usemixer) {
		SIMPLEQ_FOREACH(pchan, &sc->sc_audiochan, entries) {
			if (pchan->chan == chan->deschan)
				break;
		}
		if (pchan == NULL)
			return ENXIO;
	} else
		pchan = chan;

	vc = pchan->vc;

	DPRINTF(("audio_ioctl(%lu,'%c',%lu)\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff));
	hw = sc->hw_if;
	if (hw == NULL)
		return ENXIO;
	error = 0;
	switch (cmd) {
	case AUDIO_GETCHAN:
		if ((int *)addr != NULL)
			*(int*)addr = chan->chan;
		break;
	case AUDIO_SETCHAN:
		if ((int *)addr != NULL && *(int*)addr > 0)
			chan->deschan = *(int*)addr;
		break;
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;

	case FIONREAD:
		*(int *)addr = audio_stream_get_used(vc->sc_rustream);
		break;

	case FIOASYNC:
		if (*(int *)addr) {
			if (sc->sc_async_audio != 0)
				error = EBUSY;
			else
				sc->sc_async_audio = pchan->chan;
			DPRINTF(("audio_ioctl: FIOASYNC chan %d\n",
			    pchan->chan));
		} else
			sc->sc_async_audio = 0;
		break;

	case AUDIO_FLUSH:
		DPRINTF(("AUDIO_FLUSH\n"));
		rbus = vc->sc_rbus;
		pbus = vc->sc_pbus;
		mutex_enter(sc->sc_intr_lock);
		audio_clear(sc, vc);
		error = audio_initbufs(sc, vc);
		if (error) {
			mutex_exit(sc->sc_intr_lock);
			return error;
		}
		mutex_exit(sc->sc_intr_lock);
		if ((vc->sc_mode & AUMODE_PLAY) && !vc->sc_pbus && pbus)
			error = audiostartp(sc, vc);
		if (!error &&
		    (vc->sc_mode & AUMODE_RECORD) && !vc->sc_rbus && rbus)
			error = audiostartr(sc, vc);
		break;

	/*
	 * Number of read (write) samples dropped.  We don't know where or
	 * when they were dropped.
	 */
	case AUDIO_RERROR:
		*(int *)addr = vc->sc_mrr.drops;
		break;

	case AUDIO_PERROR:
		*(int *)addr = vc->sc_mpr.drops;
		break;

	/*
	 * Offsets into buffer.
	 */
	case AUDIO_GETIOFFS:
		ao = (struct audio_offset *)addr;
		HW_LOCK(vc);
		/* figure out where next DMA will start */
		stamp = vc->sc_rustream == &vc->sc_mrr.s
			? vc->sc_mrr.stamp : vc->sc_mrr.fstamp;
		offs = vc->sc_rustream->inp - vc->sc_rustream->start;
		HW_UNLOCK(vc);
		ao->samples = stamp;
		ao->deltablks =
		  (stamp / vc->sc_mrr.blksize) -
		  (vc->sc_mrr.stamp_last / vc->sc_mrr.blksize);
		vc->sc_mrr.stamp_last = stamp;
		ao->offset = offs;
		break;

	case AUDIO_GETOOFFS:
		ao = (struct audio_offset *)addr;
		HW_LOCK(vc);
		/* figure out where next DMA will start */
		stamp = vc->sc_pustream == &vc->sc_mpr.s
			? vc->sc_mpr.stamp : vc->sc_mpr.fstamp;
		offs = vc->sc_pustream->outp - vc->sc_pustream->start
			+ vc->sc_mpr.blksize;
		HW_UNLOCK(vc);
		ao->samples = stamp;
		ao->deltablks =
		  (stamp / vc->sc_mpr.blksize) -
		  (vc->sc_mpr.stamp_last / vc->sc_mpr.blksize);
		vc->sc_mpr.stamp_last = stamp;
		if (vc->sc_pustream->start + offs >= vc->sc_pustream->end)
			offs = 0;
		ao->offset = offs;
		break;

	/*
	 * How many bytes will elapse until mike hears the first
	 * sample of what we write next?
	 */
	case AUDIO_WSEEK:
		*(u_long *)addr = audio_stream_get_used(vc->sc_pustream);
		break;

	case AUDIO_SETINFO:
		DPRINTF(("AUDIO_SETINFO mode=0x%x\n", vc->sc_mode));
		error = audiosetinfo(sc, (struct audio_info *)addr, false, vc);
		if (!error && ISDEVSOUND(dev)) {
			error = audiogetinfo(sc, &sc->sc_ai, 0, vc);
			sc->sc_aivalid = true;
		}
		break;

	case AUDIO_GETINFO:
		DPRINTF(("AUDIO_GETINFO\n"));
		error = audiogetinfo(sc, (struct audio_info *)addr, 0, vc);
		break;

	case AUDIO_GETBUFINFO:
		DPRINTF(("AUDIO_GETBUFINFO\n"));
		error = audiogetinfo(sc, (struct audio_info *)addr, 1, vc);
		break;

	case AUDIO_DRAIN:
		DPRINTF(("AUDIO_DRAIN\n"));
		mutex_enter(sc->sc_intr_lock);
		error = audio_drain(sc, pchan->vc);
		if (!error && sc->sc_opens == 1 && hw->drain)
		    error = hw->drain(sc->hw_hdl);
		mutex_exit(sc->sc_intr_lock);
		break;

	case AUDIO_GETDEV:
		DPRINTF(("AUDIO_GETDEV\n"));
		error = hw->getdev(sc->hw_hdl, (audio_device_t *)addr);
		break;

	case AUDIO_GETENC:
		DPRINTF(("AUDIO_GETENC\n"));
		error = audio_query_encoding(sc,
		    (struct audio_encoding *)addr);
		break;

	case AUDIO_GETFD:
		DPRINTF(("AUDIO_GETFD\n"));
		*(int *)addr = vc->sc_full_duplex;
		break;

	case AUDIO_SETFD:
		DPRINTF(("AUDIO_SETFD\n"));
		fd = *(int *)addr;
		if (audio_get_props(sc) & AUDIO_PROP_FULLDUPLEX) {
			if (hw->setfd)
				error = hw->setfd(sc->hw_hdl, fd);
			else
				error = 0;
			if (!error)
				vc->sc_full_duplex = fd;
		} else {
			if (fd)
				error = ENOTTY;
			else
				error = 0;
		}
		break;

	case AUDIO_GETPROPS:
		DPRINTF(("AUDIO_GETPROPS\n"));
		*(int *)addr = audio_get_props(sc);
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
audio_poll(struct audio_softc *sc, int events, struct lwp *l,
	   struct virtual_channel *vc)
{
	int revents;
	int used;

	KASSERT(mutex_owned(sc->sc_lock));

	DPRINTF(("audio_poll: events=0x%x mode=%d\n", events, vc->sc_mode));

	revents = 0;
	HW_LOCK(vc);
	if (events & (POLLIN | POLLRDNORM)) {
		used = audio_stream_get_used(vc->sc_rustream);
		/*
		 * If half duplex and playing, audio_read() will generate
		 * silence at the play rate; poll for silence being
		 * available.  Otherwise, poll for recorded sound.
		 */
		if ((!vc->sc_full_duplex && (vc->sc_mode & AUMODE_PLAY))
		     ? vc->sc_mpr.stamp > vc->sc_wstamp :
		    used > vc->sc_mrr.usedlow)
			revents |= events & (POLLIN | POLLRDNORM);
	}

	if (events & (POLLOUT | POLLWRNORM)) {
		used = audio_stream_get_used(vc->sc_pustream);
		/*
		 * If half duplex and recording, audio_write() will throw
		 * away play data, which means we are always ready to write.
		 * Otherwise, poll for play buffer being below its low water
		 * mark.
		 */
		if ((!vc->sc_full_duplex && (vc->sc_mode & AUMODE_RECORD)) ||
		    (!(vc->sc_mode & AUMODE_PLAY_ALL) && vc->sc_playdrop > 0) ||
		    (used <= vc->sc_mpr.usedlow))
			revents |= events & (POLLOUT | POLLWRNORM);
	}
	HW_UNLOCK(vc);

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM))
			selrecord(l, &sc->sc_rsel);

		if (events & (POLLOUT | POLLWRNORM))
			selrecord(l, &sc->sc_wsel);
	}

	return revents;
}

static void
filt_audiordetach(struct knote *kn)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	dev_t dev;

	chan = kn->kn_hook;
	dev = chan->dev;
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return;


	mutex_enter(sc->sc_intr_lock);
	SLIST_REMOVE(&sc->sc_rsel.sel_klist, kn, knote, kn_selnext);
	mutex_exit(sc->sc_intr_lock);
}

static int
filt_audioread(struct knote *kn, long hint)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	struct virtual_channel *vc;
	dev_t dev;

	chan = kn->kn_hook;
	dev = chan->dev;
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	vc = chan->vc;
	mutex_enter(sc->sc_intr_lock);
	if (!vc->sc_full_duplex && (vc->sc_mode & AUMODE_PLAY))
		kn->kn_data = vc->sc_mpr.stamp - vc->sc_wstamp;
	else
		kn->kn_data = audio_stream_get_used(vc->sc_rustream)
			- vc->sc_mrr.usedlow;
	mutex_exit(sc->sc_intr_lock);

	return kn->kn_data > 0;
}

static const struct filterops audioread_filtops = {
	.f_isfd = 1,
	.f_attach = NULL,
	.f_detach = filt_audiordetach,
	.f_event = filt_audioread,
};

static void
filt_audiowdetach(struct knote *kn)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	dev_t dev;

	chan = kn->kn_hook;
	dev = chan->dev;
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return;

	mutex_enter(sc->sc_intr_lock);
	SLIST_REMOVE(&sc->sc_wsel.sel_klist, kn, knote, kn_selnext);
	mutex_exit(sc->sc_intr_lock);
}

static int
filt_audiowrite(struct knote *kn, long hint)
{
	struct audio_softc *sc;
	struct audio_chan *chan;
	audio_stream_t *stream;
	dev_t dev;

	chan = kn->kn_hook;
	dev = chan->dev;
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	mutex_enter(sc->sc_intr_lock);

	stream = chan->vc->sc_pustream;
	kn->kn_data = (stream->end - stream->start)
		- audio_stream_get_used(stream);
	mutex_exit(sc->sc_intr_lock);

	return kn->kn_data > 0;
}

static const struct filterops audiowrite_filtops = {
	.f_isfd = 1,
	.f_attach = NULL,
	.f_detach = filt_audiowdetach,
	.f_event = filt_audiowrite,
};

int
audio_kqfilter(struct audio_chan *chan, struct knote *kn)
{
	struct audio_softc *sc;
	struct klist *klist;
	dev_t dev;

	dev = chan->dev;

	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL)
		return ENXIO;

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
		return EINVAL;
	}

	kn->kn_hook = chan;

	mutex_enter(sc->sc_intr_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(sc->sc_intr_lock);

	return 0;
}

int
audio_mmap(struct audio_softc *sc, off_t *offp, size_t len, int prot,
    int *flagsp, int *advicep, struct uvm_object **uobjp, int *maxprotp,
    struct virtual_channel *vc)
{
	struct audio_ringbuffer *cb;

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->hw_if == NULL)
		return ENXIO;

	DPRINTF(("audio_mmap: off=%lld, prot=%d\n", (long long)(*offp), prot));
	if (!(audio_get_props(sc) & AUDIO_PROP_MMAP))
		return ENOTSUP;

	if (*offp < 0)
		return EINVAL;

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
		cb = &vc->sc_mpr;
	else if (prot == VM_PROT_READ)
		cb = &vc->sc_mrr;
	else
		return EINVAL;
#else
	cb = &vc->sc_mpr;
#endif

	if (len > cb->s.bufsize || *offp > (uint)(cb->s.bufsize - len))
		return EOVERFLOW;

	if (!cb->mmapped) {
		cb->mmapped = true;
		if (cb == &vc->sc_mpr) {
			audio_fill_silence(&cb->s.param, cb->s.start,
					   cb->s.bufsize);
			vc->sc_pustream = &cb->s;
			if (!vc->sc_pbus && !vc->sc_mpr.pause)
				(void)audiostartp(sc, vc);
		} else if (cb == &vc->sc_mrr) {
			vc->sc_rustream = &cb->s;
			if (!vc->sc_rbus && !sc->sc_mixring.sc_mrr.pause)
				(void)audiostartr(sc, vc);
		}
	}

	/* get ringbuffer */
	*uobjp = cb->uobj;

	/* Acquire a reference for the mmap.  munmap will release.*/
	uao_reference(*uobjp);
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;
	*flagsp = MAP_SHARED;
	return 0;
}

int
audiostartr(struct audio_softc *sc, struct virtual_channel *vc)
{
	int error;

	KASSERT(mutex_owned(sc->sc_lock));

	DPRINTF(("audiostartr: start=%p used=%d(hi=%d) mmapped=%d\n",
		 vc->sc_mrr.s.start, audio_stream_get_used(&vc->sc_mrr.s),
		 vc->sc_mrr.usedhigh, vc->sc_mrr.mmapped));

	if (!audio_can_capture(sc))
		return EINVAL;
	if (vc == sc->sc_hwvc && sc->sc_usemixer)
		return 0;

	error = 0;
	if (sc->sc_rec_started == false) {
		mutex_enter(sc->sc_intr_lock);
		error = mix_read(sc);
		if (sc->sc_usemixer)
			cv_broadcast(&sc->sc_rcondvar);
		mutex_exit(sc->sc_intr_lock);
	}
	vc->sc_rbus = true;

	return error;
}

int
audiostartp(struct audio_softc *sc, struct virtual_channel *vc)
{
	int error, used;

	KASSERT(mutex_owned(sc->sc_lock));

	error = 0;
	used = audio_stream_get_used(&vc->sc_mpr.s);
	DPRINTF(("audiostartp: start=%p used=%d(hi=%d blk=%d) mmapped=%d\n",
		 vc->sc_mpr.s.start, used, vc->sc_mpr.usedhigh,
		 vc->sc_mpr.blksize, vc->sc_mpr.mmapped));

	if (!audio_can_playback(sc))
		return EINVAL;
	if (vc == sc->sc_hwvc && sc->sc_usemixer)
		return 0;

	int blksize;
	if (sc->sc_usemixer)
		blksize = sc->sc_mixring.sc_mpr.blksize;
	else
		blksize = vc->sc_mpr.blksize;

	if (!vc->sc_mpr.mmapped && used < blksize) {
		cv_broadcast(&sc->sc_wchan);
		DPRINTF(("%s: wakeup and return\n", __func__));
		return 0;
	}
	
	vc->sc_pbus = true;
	if (sc->sc_trigger_started == false) {
		if (sc->sc_usemixer) {
			audio_mix(sc);
			audio_mix(sc);
			audio_mix(sc);
		}
		mutex_enter(sc->sc_intr_lock);
		error = mix_write(sc);
		if (error)
			goto done;
		if (sc->sc_usemixer) {
			vc = sc->sc_hwvc;
			vc->sc_mpr.s.outp =
			    audio_stream_add_outp(&vc->sc_mpr.s,
			      vc->sc_mpr.s.outp, vc->sc_mpr.blksize);
			error = mix_write(sc);
			cv_broadcast(&sc->sc_condvar);
		}
done:
		mutex_exit(sc->sc_intr_lock);
	}

	return error;
}

static void
audio_softintr_rd(void *cookie)
{
	struct audio_softc *sc = cookie;
	proc_t *p;
	pid_t pid;

	mutex_enter(sc->sc_lock);
	cv_broadcast(&sc->sc_rchan);
	selnotify(&sc->sc_rsel, 0, NOTE_SUBMIT);
	if ((pid = sc->sc_async_audio) != 0) {
		DPRINTFN(3, ("audio_softintr_rd: sending SIGIO %d\n", pid));
		mutex_enter(proc_lock);
		if ((p = proc_find(pid)) != NULL)
			psignal(p, SIGIO);
		mutex_exit(proc_lock);
	}
	mutex_exit(sc->sc_lock);
}

static void
audio_softintr_wr(void *cookie)
{
	struct audio_softc *sc = cookie;
	proc_t *p;
	pid_t pid;

	mutex_enter(sc->sc_lock);
	cv_broadcast(&sc->sc_wchan);
	selnotify(&sc->sc_wsel, 0, NOTE_SUBMIT);
	if ((pid = sc->sc_async_audio) != 0) {
		DPRINTFN(3, ("audio_softintr_wr: sending SIGIO %d\n", pid));
		mutex_enter(proc_lock);
		if ((p = proc_find(pid)) != NULL)
			psignal(p, SIGIO);
		mutex_exit(proc_lock);
	}
	mutex_exit(sc->sc_lock);
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
	struct audio_softc *sc;
	struct audio_ringbuffer *cb;
	struct virtual_channel *vc;
	int blksize, cc, used;

	sc = v;
	vc = sc->sc_hwvc;
	blksize = vc->sc_mpr.blksize;

	if (sc->sc_dying == true || sc->sc_trigger_started == false)
		return;

	if (sc->sc_usemixer)
		cb = &sc->sc_mixring.sc_mpr;
	else
		cb = &vc->sc_mpr;

	if (vc->sc_draining && cb->drops != sc->sc_last_drops) {
		vc->sc_mpr.drops += blksize;
		cv_broadcast(&sc->sc_wchan);
	}

	sc->sc_last_drops = cb->drops;

	vc->sc_mpr.s.outp = audio_stream_add_outp(&vc->sc_mpr.s,
	    vc->sc_mpr.s.outp, blksize);

	if (audio_stream_get_used(&cb->s) < blksize) {
		DPRINTFN(3, ("HW RING - INSERT SILENCE\n"));
		used = blksize;
		while (used > 0) {
			cc = cb->s.end - cb->s.inp;
			if (cc > used)
				cc = used;
			audio_fill_silence(&cb->s.param, cb->s.inp, cc);
			cb->s.inp =
			    audio_stream_add_inp(&cb->s, cb->s.inp, cc);
			used -= cc;
		}
		vc->sc_mpr.drops += blksize;
	}

	mix_write(sc);

	if (sc->sc_usemixer)
		cv_broadcast(&sc->sc_condvar);
	else
		cv_broadcast(&sc->sc_wchan);
}

void
audio_mix(void *v)
{
	stream_fetcher_t null_fetcher;
	struct audio_softc *sc;
	struct audio_chan *chan;
	struct virtual_channel *vc;
	struct audio_ringbuffer *cb;
	stream_fetcher_t *fetcher;
	uint8_t *inp;
	int cc, cc1, used, blksize;
	
	sc = v;

	DPRINTF(("PINT MIX\n"));
	sc->schedule_rih = false;
	sc->schedule_wih = false;
	sc->sc_writeme = false;

	if (sc->sc_dying == true)
		return;

	blksize = sc->sc_mixring.sc_mpr.blksize;
	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		vc = chan->vc;

		if (!vc->sc_open)
			continue;
		if (!vc->sc_pbus)
			continue;

		cb = &vc->sc_mpr;

		sc->sc_writeme = true;

		inp = cb->s.inp;
		cb->stamp += blksize;
		if (cb->mmapped) {
			DPRINTF(("audio_pint: vc=%p mmapped outp=%p cc=%d "
				 "inp=%p\n", vc, cb->s.outp, blksize,
				  cb->s.inp));
			mutex_enter(sc->sc_intr_lock);
			mix_func(sc, cb, vc);
			cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp,
			    blksize);
			mutex_exit(sc->sc_intr_lock);
			continue;
		}

#ifdef AUDIO_INTR_TIME
		{
			struct timeval tv;
			int64_t t;
			microtime(&tv);
			t = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
			if (sc->sc_pnintr) {
				int64_t lastdelta, totdelta;
				lastdelta = t - sc->sc_plastintr -
				    sc->sc_pblktime;
				if (lastdelta > sc->sc_pblktime / 3) {
					printf("audio: play interrupt(%d) off "
					       "relative by %" PRId64 " us "
					       "(%" PRId64 ")\n",
					       sc->sc_pnintr, lastdelta,
					       sc->sc_pblktime);
				}
				totdelta = t - sc->sc_pfirstintr -
				    sc->sc_pblktime * sc->sc_pnintr;
				if (totdelta > sc->sc_pblktime) {
					printf("audio: play interrupt(%d) "
					       "off absolute by %" PRId64 " us "
					       "(%" PRId64 ") (LOST)\n",
					       sc->sc_pnintr, totdelta,
					       sc->sc_pblktime);
					sc->sc_pnintr++;
					/* avoid repeated messages */
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
		if (used <= sc->sc_hwvc->sc_mpr.usedlow && !cb->copying &&
		    vc->sc_npfilters > 0) {
			/* we might have data in filter pipeline */
			null_fetcher.fetch_to = null_fetcher_fetch_to;
			fetcher = &vc->sc_pfilters[vc->sc_npfilters - 1]->base;
			vc->sc_pfilters[0]->set_fetcher(vc->sc_pfilters[0],
							&null_fetcher);
			used = audio_stream_get_used(vc->sc_pustream);
			cc = cb->s.end - cb->s.start;
			if (blksize * 2 < cc)
				cc = blksize * 2;
			fetcher->fetch_to(sc, fetcher, &cb->s, cc);
			cb->fstamp += used -
			    audio_stream_get_used(vc->sc_pustream);
			used = audio_stream_get_used(&cb->s);
		}
		if (used < blksize) {
			/* we don't have a full block to use */
			if (cb->copying) {
				/* writer is in progress, don't disturb */
				cb->needfill = true;
				DPRINTFN(1, ("audio_pint: copying in "
					 "progress\n"));
			} else {
				DPRINTF(("audio_pint: used < blksize vc=%p "
					  "used=%d blksize=%d\n", vc, used,
					  blksize));
				inp = cb->s.inp;
				cc = blksize - (inp - cb->s.start) % blksize;
				if (cb->pause)
					cb->pdrops += cc;
				else {
					cb->drops += cc;
					vc->sc_playdrop += cc;
				}

				audio_fill_silence(&cb->s.param, inp, cc);
				cb->s.inp = audio_stream_add_inp(&cb->s, inp,
				    cc);

				/* Clear next block to keep ahead of the DMA. */
				used = audio_stream_get_used(&cb->s);
				if (used + blksize < cb->s.end - cb->s.start) {
					audio_fill_silence(&cb->s.param, cb->s.inp,
					    blksize);
				}
			}
		}

		DPRINTFN(5, ("audio_pint: vc=%p outp=%p used=%d cc=%d\n", vc,
			 cb->s.outp, used, blksize));
		mutex_enter(sc->sc_intr_lock);
		mix_func(sc, cb, vc);
		mutex_exit(sc->sc_intr_lock);
		cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp, blksize);

		DPRINTFN(2, ("audio_pint: vc=%p mode=%d pause=%d used=%d "
			     "lowat=%d\n", vc, vc->sc_mode, cb->pause,
			     audio_stream_get_used(&cb->s), cb->usedlow));

		if ((vc->sc_mode & AUMODE_PLAY) && !cb->pause) {
			if (audio_stream_get_used(vc->sc_pustream) <= cb->usedlow)
				sc->schedule_wih = true;
		}
		/* Possible to return one or more "phantom blocks" now. */
		if (!vc->sc_full_duplex && vc->sc_mode & AUMODE_RECORD)
				sc->schedule_rih = true;
	}
	mutex_enter(sc->sc_intr_lock);

	vc = sc->sc_hwvc;
	cb = &sc->sc_mixring.sc_mpr;
	inp = cb->s.inp;
	cc = blksize - (inp - cb->s.start) % blksize;
	if (sc->sc_writeme == false) {
		DPRINTFN(3, ("MIX RING EMPTY - INSERT SILENCE\n"));
		audio_fill_silence(&vc->sc_pustream->param, inp, cc);
		sc->sc_mixring.sc_mpr.drops += cc;
	} else
		cc = blksize;
	cb->s.inp = audio_stream_add_inp(&cb->s, cb->s.inp, cc);
	cc = blksize;
	cc1 = sc->sc_mixring.sc_mpr.s.end - sc->sc_mixring.sc_mpr.s.inp;
	if (cc1 < cc) {
		audio_fill_silence(&vc->sc_pustream->param,
		    sc->sc_mixring.sc_mpr.s.inp, cc1);
		cc -= cc1;
		audio_fill_silence(&vc->sc_pustream->param,
		    sc->sc_mixring.sc_mpr.s.start, cc);
	} else
		audio_fill_silence(&vc->sc_pustream->param,
		    sc->sc_mixring.sc_mpr.s.inp, cc);
	mutex_exit(sc->sc_intr_lock);

	kpreempt_disable();
	if (sc->schedule_wih == true)
		softint_schedule(sc->sc_sih_wr);

	if (sc->schedule_rih == true)
		softint_schedule(sc->sc_sih_rd);
	kpreempt_enable();
}

/*
 * Called from HW driver module on completion of DMA input.
 * Mark it as input in the ring buffer (fiddle pointers).
 * Do a wakeup if necessary.
 */
void
audio_rint(void *v)
{
	struct audio_softc *sc;
	int blksize;

	sc = v;

	KASSERT(mutex_owned(sc->sc_intr_lock));

	if (sc->sc_dying == true || sc->sc_rec_started == false)
		return;

	blksize = audio_stream_get_used(&sc->sc_mixring.sc_mrr.s);
	sc->sc_mixring.sc_mrr.s.outp =
	    audio_stream_add_outp(&sc->sc_mixring.sc_mrr.s,
		sc->sc_mixring.sc_mrr.s.outp, blksize);
	mix_read(sc);

	if (sc->sc_usemixer)
		cv_broadcast(&sc->sc_rcondvar);
	else
		cv_broadcast(&sc->sc_rchan);
}

void
audio_upmix(void *v)
{
	stream_fetcher_t null_fetcher;
	struct audio_softc *sc;
	struct audio_chan *chan;
	struct audio_ringbuffer *cb;
	stream_fetcher_t *last_fetcher;
	struct virtual_channel *vc;
	int cc, used, blksize, cc1;

	sc = v;
	blksize = sc->sc_mixring.sc_mrr.blksize;

	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		vc = chan->vc;

		if (!(vc->sc_open & AUOPEN_READ))
			continue;
		if (!vc->sc_rbus)
			continue;

		cb = &vc->sc_mrr;

		blksize = audio_stream_get_used(&sc->sc_mixring.sc_mrr.s);
		if (audio_stream_get_space(&cb->s) < blksize) {
			cb->drops += blksize;
			cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp,
			    sc->sc_mixring.sc_mrr.blksize);
			continue;
		}

		cc = blksize;
		if (cb->s.inp + blksize > cb->s.end)
			cc = cb->s.end - cb->s.inp;
		mutex_enter(sc->sc_intr_lock);
		memcpy(cb->s.inp, sc->sc_mixring.sc_mrr.s.start, cc);
		if (cc < blksize && cc != 0) {
			cc1 = cc;
			cc = blksize - cc;
			memcpy(cb->s.start,
			    sc->sc_mixring.sc_mrr.s.start + cc1, cc);
		}
		mutex_exit(sc->sc_intr_lock);

		cc = blksize;
		recswvol_func(sc, cb, blksize, vc);

		cb->s.inp = audio_stream_add_inp(&cb->s, cb->s.inp, blksize);
		cb->stamp += blksize;
		if (cb->mmapped) {
			DPRINTFN(2, ("audio_rint: mmapped inp=%p cc=%d\n",
			     	cb->s.inp, blksize));
			continue;
		}

#ifdef AUDIO_INTR_TIME
		{
			struct timeval tv;
			int64_t t;
			microtime(&tv);
			t = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
			if (sc->sc_rnintr) {
				int64_t lastdelta, totdelta;
				lastdelta = t - sc->sc_rlastintr -
				    sc->sc_rblktime;
				if (lastdelta > sc->sc_rblktime / 5) {
					printf("audio: record interrupt(%d) "
					       "off relative by %" PRId64 " us "
					       "(%" PRId64 ")\n",
					       sc->sc_rnintr, lastdelta,
					       sc->sc_rblktime);
				}
				totdelta = t - sc->sc_rfirstintr -
				    sc->sc_rblktime * sc->sc_rnintr;
				if (totdelta > sc->sc_rblktime / 2) {
					sc->sc_rnintr++;
					printf("audio: record interrupt(%d) "
					       "off absolute by %" PRId64 " us "
					       "(%" PRId64 ")\n",
					       sc->sc_rnintr, totdelta,
					       sc->sc_rblktime);
					sc->sc_rnintr++;
					/* avoid repeated messages */
				}
			} else
				sc->sc_rfirstintr = t;
			sc->sc_rlastintr = t;
			sc->sc_rnintr++;
		}
#endif

		if (!cb->pause && vc->sc_nrfilters > 0) {
			null_fetcher.fetch_to = null_fetcher_fetch_to;
			last_fetcher =
			    &vc->sc_rfilters[vc->sc_nrfilters - 1]->base;
			vc->sc_rfilters[0]->set_fetcher(vc->sc_rfilters[0],
							&null_fetcher);
			used = audio_stream_get_used(vc->sc_rustream);
			cc = vc->sc_rustream->end - vc->sc_rustream->start;
			last_fetcher->fetch_to
				(sc, last_fetcher, vc->sc_rustream, cc);
			cb->fstamp += audio_stream_get_used(vc->sc_rustream) -
			    used;
			/* XXX what should do for error? */
		}
		used = audio_stream_get_used(&vc->sc_mrr.s);
		if (cb->pause) {
			DPRINTFN(1, ("audio_rint: pdrops %lu\n", cb->pdrops));
			cb->pdrops += blksize;
			cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp,
			    blksize);
		} else if (used + blksize > cb->s.end - cb->s.start &&
								!cb->copying) {
			DPRINTFN(1, ("audio_rint: drops %lu\n", cb->drops));
			cb->drops += blksize;
			cb->s.outp = audio_stream_add_outp(&cb->s, cb->s.outp,
			    blksize);
		}
	}
	kpreempt_disable();
	softint_schedule(sc->sc_sih_rd);
	kpreempt_enable();
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
		if (p->precision == 8 && p->encoding ==
		    AUDIO_ENCODING_SLINEAR_BE)
			p->encoding = AUDIO_ENCODING_SLINEAR_LE;
		if (p->precision == 8 && p->encoding ==
		    AUDIO_ENCODING_ULINEAR_BE)
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
	case AUDIO_ENCODING_AC3:
		break;
	default:
		return EINVAL;
	}

	/* sanity check # of channels*/
	if (p->channels < 1 || p->channels > AUDIO_MAX_CHANNELS)
		return EINVAL;

	return 0;
}

/*
 * set some parameters from sc->sc_vchan_params.
 */
static int
audio_set_vchan_defaults(struct audio_softc *sc, u_int mode)
{
	struct virtual_channel *vc;
	struct audio_info ai;
	int error;

	KASSERT(mutex_owned(sc->sc_lock));

	vc = sc->sc_hwvc;

	/* default parameters */
	vc->sc_rparams = sc->sc_vchan_params;
	vc->sc_pparams = sc->sc_vchan_params;

	AUDIO_INITINFO(&ai);
	ai.record.sample_rate = sc->sc_vchan_params.sample_rate;
	ai.record.encoding    = sc->sc_vchan_params.encoding;
	ai.record.channels    = sc->sc_vchan_params.channels;
	ai.record.precision   = sc->sc_vchan_params.precision;
	ai.record.pause	      = false;
	ai.play.sample_rate   = sc->sc_vchan_params.sample_rate;
	ai.play.encoding      = sc->sc_vchan_params.encoding;
	ai.play.channels      = sc->sc_vchan_params.channels;
	ai.play.precision     = sc->sc_vchan_params.precision;
	ai.play.pause         = false;
	ai.mode		      = mode;

	sc->sc_format[0].encoding = sc->sc_vchan_params.encoding;
	sc->sc_format[0].channels = sc->sc_vchan_params.channels;
	sc->sc_format[0].precision = sc->sc_vchan_params.precision;
	sc->sc_format[0].validbits = sc->sc_vchan_params.precision;
	sc->sc_format[0].frequency_type = 1;
	sc->sc_format[0].frequency[0] = sc->sc_vchan_params.sample_rate;

	auconv_delete_encodings(sc->sc_encodings);
	error = auconv_create_encodings(sc->sc_format, VAUDIO_NFORMATS,
	    &sc->sc_encodings);

	if (error == 0)
		error = audiosetinfo(sc, &ai, true, vc);

	return error;
}

int
audio_set_defaults(struct audio_softc *sc, u_int mode,
		   struct virtual_channel *vc)
{
	struct audio_info ai;

	KASSERT(mutex_owned(sc->sc_lock));

	/* default parameters */
	vc->sc_rparams = audio_default;
	vc->sc_pparams = audio_default;

	AUDIO_INITINFO(&ai);
	ai.record.sample_rate = vc->sc_rparams.sample_rate;
	ai.record.encoding    = vc->sc_rparams.encoding;
	ai.record.channels    = vc->sc_rparams.channels;
	ai.record.precision   = vc->sc_rparams.precision;
	ai.record.pause	      = false;
	ai.play.sample_rate   = vc->sc_pparams.sample_rate;
	ai.play.encoding      = vc->sc_pparams.encoding;
	ai.play.channels      = vc->sc_pparams.channels;
	ai.play.precision     = vc->sc_pparams.precision;
	ai.play.pause         = false;
	ai.mode		      = mode;

	return audiosetinfo(sc, &ai, true, vc);
}

int
au_set_lr_value(struct	audio_softc *sc, mixer_ctrl_t *ct, int l, int r)
{

	KASSERT(mutex_owned(sc->sc_lock));

	ct->type = AUDIO_MIXER_VALUE;
	ct->un.value.num_channels = 2;
	ct->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = l;
	ct->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
	if (audio_set_port(sc, ct) == 0)
		return 0;
	ct->un.value.num_channels = 1;
	ct->un.value.level[AUDIO_MIXER_LEVEL_MONO] = (l+r)/2;
	return audio_set_port(sc, ct);
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

	KASSERT(mutex_owned(sc->sc_lock));

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
			error = audio_get_port(sc, &ct);
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
			error = audio_get_port(sc, &ct);
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

	KASSERT(mutex_owned(sc->sc_lock));

	ct->un.value.num_channels = 2;
	if (audio_get_port(sc, ct) == 0) {
		*l = ct->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		*r = ct->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
	} else {
		ct->un.value.num_channels = 1;
		error = audio_get_port(sc, ct);
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

	KASSERT(mutex_owned(sc->sc_lock));

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
			if (audio_get_port(sc, &ct))
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
			if (audio_get_port(sc, &ct))
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

	KASSERT(mutex_owned(sc->sc_lock));

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
				error = audio_set_port(sc, &ct);
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
			error = audio_set_port(sc, &ct);
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

	KASSERT(mutex_owned(sc->sc_lock));

	if (ports->index == -1)
		return 0;
	ct.dev = ports->index;
	ct.type = ports->isenum ? AUDIO_MIXER_ENUM : AUDIO_MIXER_SET;
	if (audio_get_port(sc, &ct))
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
audiosetinfo(struct audio_softc *sc, struct audio_info *ai, bool reset,
	     struct virtual_channel *vc)
{
	stream_filter_list_t pfilters, rfilters;
	audio_params_t pp, rp;
	struct audio_prinfo *r, *p;
	const struct audio_hw_if *hw;
	audio_stream_t *oldpus, *oldrus;
	int setmode;
	int error;
	int np, nr;
	int blks;
	u_int gain;
	bool rbus, pbus;
	bool cleared, modechange, pausechange;
	u_char balance;

	KASSERT(mutex_owned(sc->sc_lock));

	hw = sc->hw_if;
	if (hw == NULL)		/* HW has not attached */
		return ENXIO;

	DPRINTF(("%s sc=%p ai=%p\n", __func__, sc, ai));
	r = &ai->record;
	p = &ai->play;
	rbus = vc->sc_rbus;
	pbus = vc->sc_pbus;
	error = 0;
	cleared = false;
	modechange = false;
	pausechange = false;

	pp = vc->sc_pparams;	/* Temporary encoding storage in */
	rp = vc->sc_rparams;	/* case setting the modes fails. */
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

	if (!audio_can_capture(sc))
		nr = 0;
	if (!audio_can_playback(sc))
		np = 0;

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

	setmode = 0;
	if (nr > 0) {
		if (!cleared) {
			audio_clear_intr_unlocked(sc, vc);
			cleared = true;
		}
		modechange = true;
		setmode |= AUMODE_RECORD;
	}
	if (np > 0) {
		if (!cleared) {
			audio_clear_intr_unlocked(sc, vc);
			cleared = true;
		}
		modechange = true;
		setmode |= AUMODE_PLAY;
	}

	if (SPECIFIED(ai->mode)) {
		if (!cleared) {
			audio_clear_intr_unlocked(sc, vc);
			cleared = true;
		}
		modechange = true;
		vc->sc_mode = ai->mode;
		if (vc->sc_mode & AUMODE_PLAY_ALL)
			vc->sc_mode |= AUMODE_PLAY;
		if ((vc->sc_mode & AUMODE_PLAY) && !vc->sc_full_duplex)
			/* Play takes precedence */
			vc->sc_mode &= ~AUMODE_RECORD;
	}

	oldpus = vc->sc_pustream;
	oldrus = vc->sc_rustream;
	if (modechange || reset) {
		int indep;

		indep = audio_get_props(sc) & AUDIO_PROP_INDEPENDENT;
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
		error = audio_set_params(sc, setmode,
		    vc->sc_mode & (AUMODE_PLAY | AUMODE_RECORD), &pp, &rp,
		    &pfilters, &rfilters, vc);
		if (error) {
			DPRINTF(("%s: audio_set_params() failed with %d\n",
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

		if (vc->sc_mpr.mmapped && pfilters.req_size > 0) {
			DPRINTF(("%s: mmapped, and filters are requested.\n",
				 __func__));
			error = EINVAL;
			goto cleanup;
		}

		/* construct new filter chain */
		if (setmode & AUMODE_PLAY) {
			error = audio_setup_pfilters(sc, &pp, &pfilters, vc);
			if (error)
				goto cleanup;
		}
		if (setmode & AUMODE_RECORD) {
			error = audio_setup_rfilters(sc, &rp, &rfilters, vc);
			if (error)
				goto cleanup;
		}
		DPRINTF(("%s: filter setup is completed.\n", __func__));

		/* userland formats */
		vc->sc_pparams = pp;
		vc->sc_rparams = rp;
	}

#ifdef AUDIO_DEBUG
	if (audiodebug > 1 && nr > 0) {
	    audio_print_params("audiosetinfo() After setting record params:",
		&vc->sc_rparams);
	}
	if (audiodebug > 1 && np > 0) {
	    audio_print_params("audiosetinfo() After setting play params:",
		&vc->sc_pparams);
	}
#endif

	if (SPECIFIED(p->port)) {
		if (!cleared) {
			audio_clear_intr_unlocked(sc, vc);
			cleared = true;
		}
		error = au_set_port(sc, &sc->sc_outports, p->port);
		if (error)
			goto cleanup;
	}
	if (SPECIFIED(r->port)) {
		if (!cleared) {
			audio_clear_intr_unlocked(sc, vc);
			cleared = true;
		}
		error = au_set_port(sc, &sc->sc_inports, r->port);
		if (error)
			goto cleanup;
	}
	if (SPECIFIED(p->gain))
		vc->sc_swvol = p->gain;

	if (SPECIFIED(r->gain))
		vc->sc_recswvol = r->gain;

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
		error = audio_set_port(sc, &ct);
		if (error)
			goto cleanup;
	}

	if (SPECIFIED_CH(p->pause)) {
		vc->sc_mpr.pause = p->pause;
		pbus = !p->pause;
		pausechange = true;
	}
	if (SPECIFIED_CH(r->pause)) {
		vc->sc_mrr.pause = r->pause;
		rbus = !r->pause;
		pausechange = true;
	}

	if (SPECIFIED(ai->mode)) {
		if (vc->sc_mode & AUMODE_PLAY)
			audio_init_play(sc, vc);
		if (vc->sc_mode & AUMODE_RECORD)
			audio_init_record(sc, vc);
	}

	if (nr > 0)
		audio_setblksize(sc, vc, ai->blocksize, AUMODE_RECORD);
	if (np > 0)
		audio_setblksize(sc, vc, ai->blocksize, AUMODE_PLAY);

	if (hw->commit_settings && sc->sc_opens + sc->sc_recopens == 0) {
		error = hw->commit_settings(sc->hw_hdl);
		if (error)
			goto cleanup;
	}

	vc->sc_lastinfo = *ai;
	vc->sc_lastinfovalid = true;

cleanup:
	if (error == 0 && (cleared || pausechange|| reset)) {
		int init_error;

		init_error = (pausechange == 1 && reset == 0) ? 0 :
		    audio_initbufs(sc, vc);
		if (init_error) goto err;
		if (reset || vc->sc_pustream != oldpus ||
		    vc->sc_rustream != oldrus)
			audio_calcwater(sc, vc);
		if ((vc->sc_mode & AUMODE_PLAY) &&
		    pbus && !vc->sc_pbus)
			init_error = audiostartp(sc, vc);
		if (!init_error &&
		    (vc->sc_mode & AUMODE_RECORD) &&
		    rbus && !vc->sc_rbus)
			init_error = audiostartr(sc, vc);
	err:
		if (init_error)
			return init_error;
	}

	/* Change water marks after initializing the buffers. */
	if (SPECIFIED(ai->hiwat)) {
		blks = ai->hiwat;
		if (blks > vc->sc_mpr.maxblks)
			blks = vc->sc_mpr.maxblks;
		if (blks < PREFILL_BLOCKS + 1)
			blks = PREFILL_BLOCKS + 1;
		vc->sc_mpr.usedhigh = blks * vc->sc_mpr.blksize;
	}
	if (SPECIFIED(ai->lowat)) {
		blks = ai->lowat;
		if (blks > vc->sc_mpr.maxblks - 1)
			blks = vc->sc_mpr.maxblks - 1;
		if (blks < PREFILL_BLOCKS)
			blks = PREFILL_BLOCKS;
		vc->sc_mpr.usedlow = blks * vc->sc_mpr.blksize;
	}
	if (SPECIFIED(ai->hiwat) || SPECIFIED(ai->lowat)) {
		if (vc->sc_mpr.usedlow > vc->sc_mpr.usedhigh -
		    vc->sc_mpr.blksize) {
			vc->sc_mpr.usedlow =
				vc->sc_mpr.usedhigh - vc->sc_mpr.blksize;
		}
	}

	return error;
}

int
audiogetinfo(struct audio_softc *sc, struct audio_info *ai, int buf_only_mode,
	     struct virtual_channel *vc)
{
	struct audio_prinfo *r, *p;
	const struct audio_hw_if *hw;

	KASSERT(mutex_owned(sc->sc_lock));

	r = &ai->record;
	p = &ai->play;
	hw = sc->hw_if;
	if (hw == NULL)		/* HW has not attached */
		return ENXIO;

	p->sample_rate = vc->sc_pparams.sample_rate;
	r->sample_rate = vc->sc_rparams.sample_rate;
	p->channels = vc->sc_pparams.channels;
	r->channels = vc->sc_rparams.channels;
	p->precision = vc->sc_pparams.precision;
	r->precision = vc->sc_rparams.precision;
	p->encoding = vc->sc_pparams.encoding;
	r->encoding = vc->sc_rparams.encoding;

	if (buf_only_mode) {
		r->port = 0;
		p->port = 0;

		r->avail_ports = 0;
		p->avail_ports = 0;

		r->gain = 0;
		r->balance = 0;

		p->gain = 0;
		p->balance = 0;
	} else {
		r->port = au_get_port(sc, &sc->sc_inports);
		p->port = au_get_port(sc, &sc->sc_outports);

		r->avail_ports = sc->sc_inports.allports;
		p->avail_ports = sc->sc_outports.allports;

		au_get_gain(sc, &sc->sc_inports, &r->gain, &r->balance);
		au_get_gain(sc, &sc->sc_outports, &p->gain, &p->balance);
	}

	if (sc->sc_monitor_port != -1 && buf_only_mode == 0) {
		mixer_ctrl_t ct;

		ct.dev = sc->sc_monitor_port;
		ct.type = AUDIO_MIXER_VALUE;
		ct.un.value.num_channels = 1;
		if (audio_get_port(sc, &ct))
			ai->monitor_gain = 0;
		else
			ai->monitor_gain =
				ct.un.value.level[AUDIO_MIXER_LEVEL_MONO];
	} else
		ai->monitor_gain = 0;

	p->seek = audio_stream_get_used(vc->sc_pustream);
	r->seek = audio_stream_get_used(vc->sc_rustream);

	/*
	 * XXX samples should be a value for userland data.
	 * But drops is a value for HW data.
	 */
	p->samples = (vc->sc_pustream == &vc->sc_mpr.s
	    ? vc->sc_mpr.stamp : vc->sc_mpr.fstamp) - vc->sc_mpr.drops;
	r->samples = (vc->sc_rustream == &vc->sc_mrr.s
	    ? vc->sc_mrr.stamp : vc->sc_mrr.fstamp) - vc->sc_mrr.drops;

	p->eof = sc->sc_eof;
	r->eof = 0;

	p->pause = vc->sc_mpr.pause;
	r->pause = vc->sc_mrr.pause;

	p->error = vc->sc_mpr.drops != 0;
	r->error = vc->sc_mrr.drops != 0;

	p->waiting = r->waiting = 0;		/* open never hangs */

	p->open = (vc->sc_open & AUOPEN_WRITE) != 0;
	r->open = (vc->sc_open & AUOPEN_READ) != 0;

	p->active = vc->sc_pbus;
	r->active = vc->sc_rbus;

	p->buffer_size = vc->sc_pustream ? vc->sc_pustream->bufsize : 0;
	r->buffer_size = vc->sc_rustream ? vc->sc_rustream->bufsize : 0;

	ai->blocksize = vc->sc_mpr.blksize;
	if (vc->sc_mpr.blksize > 0) {
		ai->hiwat = vc->sc_mpr.usedhigh / vc->sc_mpr.blksize;
		ai->lowat = vc->sc_mpr.usedlow / vc->sc_mpr.blksize;
	} else
		ai->hiwat = ai->lowat = 0;
	ai->mode = vc->sc_mode;

	return 0;
}

/*
 * Mixer driver
 */
int
mixer_open(dev_t dev, struct audio_softc *sc, int flags,
    int ifmt, struct lwp *l, struct file **nfp)
{
	struct file *fp;
	struct audio_chan *chan;
	int error, fd;

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->hw_if == NULL)
		return  ENXIO;

	DPRINTF(("mixer_open: flags=0x%x sc=%p\n", flags, sc));

	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	chan = kmem_zalloc(sizeof(struct audio_chan), KM_SLEEP);
	chan->dev = dev;

	error = fd_clone(fp, fd, flags, &audio_fileops, chan);
	KASSERT(error == EMOVEFD);

	*nfp = fp;
	return error;
}

/*
 * Remove a process from those to be signalled on mixer activity.
 */
static void
mixer_remove(struct audio_softc *sc)
{
	struct mixer_asyncs **pm, *m;
	pid_t pid;

	KASSERT(mutex_owned(sc->sc_lock));

	pid = curproc->p_pid;
	for (pm = &sc->sc_async_mixer; *pm; pm = &(*pm)->next) {
		if ((*pm)->pid == pid) {
			m = *pm;
			*pm = m->next;
			kmem_free(m, sizeof(*m));
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
	proc_t *p;

	for (m = sc->sc_async_mixer; m; m = m->next) {
		mutex_enter(proc_lock);
		if ((p = proc_find(m->pid)) != NULL)
			psignal(p, SIGIO);
		mutex_exit(proc_lock);
	}
}

/*
 * Close a mixer device
 */
/* ARGSUSED */
int
mixer_close(struct audio_softc *sc, int flags, struct audio_chan *chan)
{

	KASSERT(mutex_owned(sc->sc_lock));
	if (sc->hw_if == NULL)
		return ENXIO;

	DPRINTF(("mixer_close: sc %p\n", sc));
	mixer_remove(sc);

	return 0;
}

int
mixer_ioctl(struct audio_softc *sc, u_long cmd, void *addr, int flag,
	    struct lwp *l)
{
	const struct audio_hw_if *hw;
	struct mixer_asyncs *ma;
	mixer_ctrl_t *mc;
	int error;

	DPRINTF(("mixer_ioctl(%lu,'%c',%lu)\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff));
	hw = sc->hw_if;
	if (hw == NULL)
		return ENXIO;
	error = EINVAL;

	/* we can return cached values if we are sleeping */
	if (cmd != AUDIO_MIXER_READ)
		device_active(sc->dev, DVA_SYSTEM);

	switch (cmd) {
	case FIOASYNC:
		if (*(int *)addr) {
			ma = kmem_alloc(sizeof(struct mixer_asyncs), KM_SLEEP);
		} else {
			ma = NULL;
		}
		mixer_remove(sc);	/* remove old entry */
		if (ma != NULL) {
			ma->next = sc->sc_async_mixer;
			ma->pid = curproc->p_pid;
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
		error = audio_query_devinfo(sc, (mixer_devinfo_t *)addr);
		break;

	case AUDIO_MIXER_READ:
		DPRINTF(("AUDIO_MIXER_READ\n"));
		mc = (mixer_ctrl_t *)addr;

		if (device_is_active(sc->sc_dev))
			error = audio_get_port(sc, mc);
		else if (mc->dev < 0 || mc->dev >= sc->sc_nmixer_states)
			error = ENXIO;
		else {
			int dev = mc->dev;
			memcpy(mc, &sc->sc_mixer_state[dev],
			    sizeof(mixer_ctrl_t));
			error = 0;
		}
		break;

	case AUDIO_MIXER_WRITE:
		DPRINTF(("AUDIO_MIXER_WRITE\n"));
		error = audio_set_port(sc, (mixer_ctrl_t *)addr);
		if (!error && hw->commit_settings)
			error = hw->commit_settings(sc->hw_hdl);
		if (!error)
			mixer_signal(sc);
		break;

	default:
		if (hw->dev_ioctl) {
			error = hw->dev_ioctl(sc->hw_hdl, cmd, addr, flag, l);
		} else
			error = EINVAL;
		break;
	}
	DPRINTF(("mixer_ioctl(%lu,'%c',%lu) result %d\n",
		 IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff, error));
	return error;
}
#endif /* NAUDIO > 0 */

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
device_t
audio_get_device(struct audio_softc *sc)
{
	return sc->sc_dev;
}
#endif

#if NAUDIO > 0
static void
audio_mixer_capture(struct audio_softc *sc)
{
	mixer_devinfo_t mi;
	mixer_ctrl_t *mc;

	KASSERT(mutex_owned(sc->sc_lock));

	for (mi.index = 0;; mi.index++) {
		if (audio_query_devinfo(sc, &mi) != 0)
			break;
		KASSERT(mi.index < sc->sc_nmixer_states);
		if (mi.type == AUDIO_MIXER_CLASS)
			continue;
		mc = &sc->sc_mixer_state[mi.index];
		mc->dev = mi.index;
		mc->type = mi.type;
		mc->un.value.num_channels = mi.un.v.num_channels;
		(void)audio_get_port(sc, mc);
	}

	return;
}

static void
audio_mixer_restore(struct audio_softc *sc)
{
	mixer_devinfo_t mi;
	mixer_ctrl_t *mc;

	KASSERT(mutex_owned(sc->sc_lock));

	for (mi.index = 0; ; mi.index++) {
		if (audio_query_devinfo(sc, &mi) != 0)
			break;
		if (mi.type == AUDIO_MIXER_CLASS)
			continue;
		mc = &sc->sc_mixer_state[mi.index];
		(void)audio_set_port(sc, mc);
	}
	if (sc->hw_if->commit_settings)
		sc->hw_if->commit_settings(sc->hw_hdl);

	return;
}

#ifdef AUDIO_PM_IDLE
static void
audio_idle(void *arg)
{
	device_t dv = arg;
	struct audio_softc *sc = device_private(dv);

#ifdef PNP_DEBUG
	extern int pnp_debug_idle;
	if (pnp_debug_idle)
		printf("%s: idle handler called\n", device_xname(dv));
#endif

	sc->sc_idle = true;

	/* XXX joerg Make pmf_device_suspend handle children? */
	if (!pmf_device_suspend(dv, PMF_Q_SELF))
		return;

	if (!pmf_device_suspend(sc->sc_dev, PMF_Q_SELF))
		pmf_device_resume(dv, PMF_Q_SELF);
}

static void
audio_activity(device_t dv, devactive_t type)
{
	struct audio_softc *sc = device_private(dv);

	if (type != DVA_SYSTEM)
		return;

	callout_schedule(&sc->sc_idle_counter, audio_idle_timeout * hz);

	sc->sc_idle = false;
	if (!device_is_active(dv)) {
		/* XXX joerg How to deal with a failing resume... */
		pmf_device_resume(sc->sc_dev, PMF_Q_SELF);
		pmf_device_resume(dv, PMF_Q_SELF);
	}
}
#endif

static bool
audio_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct audio_softc *sc = device_private(dv);
	struct audio_chan *chan;
	const struct audio_hw_if *hwp = sc->hw_if;
	struct virtual_channel *vc;
	bool pbus, rbus;

	pbus = rbus = false;
	mutex_enter(sc->sc_lock);
	audio_mixer_capture(sc);
	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		vc = chan->vc;
		if (vc->sc_pbus && !pbus)
			pbus = true;
		if (vc->sc_rbus && !rbus)
			rbus = true;
	}
	mutex_enter(sc->sc_intr_lock);
	if (pbus == true)
		hwp->halt_output(sc->hw_hdl);
	if (rbus == true)
		hwp->halt_input(sc->hw_hdl);
	mutex_exit(sc->sc_intr_lock);
#ifdef AUDIO_PM_IDLE
	callout_halt(&sc->sc_idle_counter, sc->sc_lock);
#endif
	mutex_exit(sc->sc_lock);

	return true;
}

static bool
audio_resume(device_t dv, const pmf_qual_t *qual)
{
	struct audio_softc *sc = device_private(dv);
	struct audio_chan *chan;
	struct virtual_channel *vc;
	
	mutex_enter(sc->sc_lock);
	sc->sc_trigger_started = false;
	sc->sc_rec_started = false;

	audio_set_vchan_defaults(sc,
	    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);

	audio_mixer_restore(sc);
	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		vc = chan->vc;
		if (vc->sc_lastinfovalid == true)
			audiosetinfo(sc, &vc->sc_lastinfo, true, vc);
		if (vc->sc_pbus == true && !vc->sc_mpr.pause)
			audiostartp(sc, vc);
		if (vc->sc_rbus == true && !vc->sc_mrr.pause)
			audiostartr(sc, vc);
	}
	mutex_exit(sc->sc_lock);

	return true;
}

static void
audio_volume_down(device_t dv)
{
	struct audio_softc *sc = device_private(dv);
	mixer_devinfo_t mi;
	int newgain;
	u_int gain;
	u_char balance;

	mutex_enter(sc->sc_lock);
	if (sc->sc_outports.index == -1 && sc->sc_outports.master != -1) {
		mi.index = sc->sc_outports.master;
		mi.un.v.delta = 0;
		if (audio_query_devinfo(sc, &mi) == 0) {
			au_get_gain(sc, &sc->sc_outports, &gain, &balance);
			newgain = gain - mi.un.v.delta;
			if (newgain < AUDIO_MIN_GAIN)
				newgain = AUDIO_MIN_GAIN;
			au_set_gain(sc, &sc->sc_outports, newgain, balance);
		}
	}
	mutex_exit(sc->sc_lock);
}

static void
audio_volume_up(device_t dv)
{
	struct audio_softc *sc = device_private(dv);
	mixer_devinfo_t mi;
	u_int gain, newgain;
	u_char balance;

	mutex_enter(sc->sc_lock);
	if (sc->sc_outports.index == -1 && sc->sc_outports.master != -1) {
		mi.index = sc->sc_outports.master;
		mi.un.v.delta = 0;
		if (audio_query_devinfo(sc, &mi) == 0) {
			au_get_gain(sc, &sc->sc_outports, &gain, &balance);
			newgain = gain + mi.un.v.delta;
			if (newgain > AUDIO_MAX_GAIN)
				newgain = AUDIO_MAX_GAIN;
			au_set_gain(sc, &sc->sc_outports, newgain, balance);
		}
	}
	mutex_exit(sc->sc_lock);
}

static void
audio_volume_toggle(device_t dv)
{
	struct audio_softc *sc = device_private(dv);
	u_int gain, newgain;
	u_char balance;

	mutex_enter(sc->sc_lock);
	au_get_gain(sc, &sc->sc_outports, &gain, &balance);
	if (gain != 0) {
		sc->sc_lastgain = gain;
		newgain = 0;
	} else
		newgain = sc->sc_lastgain;
	au_set_gain(sc, &sc->sc_outports, newgain, balance);
	mutex_exit(sc->sc_lock);
}

static int
audio_get_props(struct audio_softc *sc)
{
	const struct audio_hw_if *hw;
	int props;

	KASSERT(mutex_owned(sc->sc_lock));

	hw = sc->hw_if;
	props = hw->get_props(sc->hw_hdl);

	/*
	 * if neither playback nor capture properties are reported,
	 * assume both are supported by the device driver
	 */
	if ((props & (AUDIO_PROP_PLAYBACK|AUDIO_PROP_CAPTURE)) == 0)
		props |= (AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE);

	props |= AUDIO_PROP_MMAP;

	return props;
}

static bool
audio_can_playback(struct audio_softc *sc)
{
	return audio_get_props(sc) & AUDIO_PROP_PLAYBACK ? true : false;
}

static bool
audio_can_capture(struct audio_softc *sc)
{
	return audio_get_props(sc) & AUDIO_PROP_CAPTURE ? true : false;
}

int
mix_read(void *arg)
{
	struct audio_softc *sc = arg;
	struct virtual_channel *vc;
	stream_filter_t *filter;
	stream_fetcher_t *fetcher;
	stream_fetcher_t null_fetcher;
	int cc, cc1, blksize, error;
	uint8_t *inp;

	vc = sc->sc_hwvc;
	blksize = vc->sc_mrr.blksize;
	cc = blksize;
	error = 0;

	if (sc->hw_if->trigger_input && sc->sc_rec_started == false) {
		DPRINTF(("%s: call trigger_input\n", __func__));
		sc->sc_rec_started = true;
		error = sc->hw_if->trigger_input(sc->hw_hdl, vc->sc_mrr.s.start,
		    vc->sc_mrr.s.end, vc->sc_mrr.blksize,
		    audio_rint, (void *)sc, &vc->sc_mrr.s.param);
	} else if (sc->hw_if->start_input) {
		DPRINTF(("%s: call start_input\n", __func__));
		sc->sc_rec_started = true;
		error = sc->hw_if->start_input(sc->hw_hdl,
		    vc->sc_mrr.s.inp, vc->sc_mrr.blksize,
		    audio_rint, (void *)sc);
	}
	if (error) {
		/* XXX does this really help? */
		DPRINTF(("audio_upmix restart failed: %d\n", error));
		audio_clear(sc, sc->sc_hwvc);
		sc->sc_rec_started = false;
		return error;
	}

	inp = vc->sc_mrr.s.inp;
	vc->sc_mrr.s.inp = audio_stream_add_inp(&vc->sc_mrr.s, inp, cc);

	if (vc->sc_nrfilters > 0) {
		cc = vc->sc_rustream->end - vc->sc_rustream->start;
		null_fetcher.fetch_to = null_fetcher_fetch_to;
		filter = vc->sc_rfilters[0];
		filter->set_fetcher(filter, &null_fetcher);
		fetcher = &vc->sc_rfilters[vc->sc_nrfilters - 1]->base;
		fetcher->fetch_to(sc, fetcher, vc->sc_rustream, cc);
	}

	blksize = audio_stream_get_used(vc->sc_rustream);
	cc1 = blksize;
	if (vc->sc_rustream->outp + blksize > vc->sc_rustream->end)
		cc1 = vc->sc_rustream->end - vc->sc_rustream->outp;
	memcpy(sc->sc_mixring.sc_mrr.s.start, vc->sc_rustream->outp, cc1);
	if (cc1 < blksize) {
		memcpy(sc->sc_mixring.sc_mrr.s.start + cc1,
		    vc->sc_rustream->start, blksize - cc1);
	}
	sc->sc_mixring.sc_mrr.s.inp =
	    audio_stream_add_inp(&sc->sc_mixring.sc_mrr.s,
		sc->sc_mixring.sc_mrr.s.inp, blksize);
	vc->sc_rustream->outp = audio_stream_add_outp(vc->sc_rustream,
	    vc->sc_rustream->outp, blksize);
	
	return error;
}

int
mix_write(void *arg)
{
	struct audio_softc *sc = arg;
	struct virtual_channel *vc;
	stream_filter_t *filter;
	stream_fetcher_t *fetcher;
	stream_fetcher_t null_fetcher;
	int cc, cc1, cc2, error, used;
	const uint8_t *orig;
	uint8_t *tocopy;

	vc = sc->sc_hwvc;
	error = 0;

	if (sc->sc_usemixer &&
	    audio_stream_get_used(vc->sc_pustream) <=
				sc->sc_mixring.sc_mpr.blksize) {
		tocopy = vc->sc_pustream->inp;
		orig = sc->sc_mixring.sc_mpr.s.outp;
		used = sc->sc_mixring.sc_mpr.blksize;

		while (used > 0) {
			cc = used;
			cc1 = vc->sc_pustream->end - tocopy;
			cc2 = sc->sc_mixring.sc_mpr.s.end - orig;
			if (cc > cc1)
				cc = cc1;
			if (cc > cc2)
				cc = cc2;
			memcpy(tocopy, orig, cc);
			orig += cc;
			tocopy += cc;

			if (tocopy >= vc->sc_pustream->end)
				tocopy = vc->sc_pustream->start;
			if (orig >= sc->sc_mixring.sc_mpr.s.end)
				orig = sc->sc_mixring.sc_mpr.s.start;

			used -= cc;
		}

		vc->sc_pustream->inp = audio_stream_add_inp(vc->sc_pustream,
		    vc->sc_pustream->inp, sc->sc_mixring.sc_mpr.blksize);

		sc->sc_mixring.sc_mpr.s.outp =
		    audio_stream_add_outp(&sc->sc_mixring.sc_mpr.s,
		    	sc->sc_mixring.sc_mpr.s.outp,
			sc->sc_mixring.sc_mpr.blksize);
	}

	if (vc->sc_npfilters > 0 &&
	    (sc->sc_usemixer || sc->sc_trigger_started)) {
		null_fetcher.fetch_to = null_fetcher_fetch_to;
		filter = vc->sc_pfilters[0];
		filter->set_fetcher(filter, &null_fetcher);
		fetcher = &vc->sc_pfilters[vc->sc_npfilters - 1]->base;
		fetcher->fetch_to(sc, fetcher, &vc->sc_mpr.s,
		    vc->sc_mpr.blksize * 2);
 	}

	if (sc->hw_if->trigger_output && sc->sc_trigger_started == false) {
		DPRINTF(("%s: call trigger_output\n", __func__));
		sc->sc_trigger_started = true;
		error = sc->hw_if->trigger_output(sc->hw_hdl,
		    vc->sc_mpr.s.start, vc->sc_mpr.s.end, vc->sc_mpr.blksize,
		    audio_pint, (void *)sc, &vc->sc_mpr.s.param);
	} else if (sc->hw_if->start_output) {
		DPRINTF(("%s: call start_output\n", __func__));
		sc->sc_trigger_started = true;
		error = sc->hw_if->start_output(sc->hw_hdl,
		    __UNCONST(vc->sc_mpr.s.outp), vc->sc_mpr.blksize,
		    audio_pint, (void *)sc);
	}

	if (error) {
		/* XXX does this really help? */
		DPRINTF(("audio_mix restart failed: %d\n", error));
		audio_clear(sc, sc->sc_hwvc);
		sc->sc_trigger_started = false;
	}

	return error;
}

#define DEF_MIX_FUNC(bits, type, bigger_type, MINVAL, MAXVAL)		\
	static void							\
	mix_func##bits(struct audio_softc *sc, struct audio_ringbuffer *cb, \
		  struct virtual_channel *vc)				\
	{								\
		int blksize, cc, cc1, cc2, m, resid;			\
		bigger_type product;					\
		bigger_type result;					\
		type *orig, *tomix;					\
									\
		blksize = sc->sc_mixring.sc_mpr.blksize;		\
		resid = blksize;					\
									\
		tomix = __UNCONST(cb->s.outp);				\
		orig = (type *)(sc->sc_mixring.sc_mpr.s.inp);		\
									\
		while (resid > 0) {					\
			cc = resid;					\
			cc1 = sc->sc_mixring.sc_mpr.s.end -		\
			    (uint8_t *)orig;				\
			cc2 = cb->s.end - (uint8_t *)tomix;		\
			if (cc > cc1)					\
				cc = cc1;				\
			if (cc > cc2)					\
				cc = cc2;				\
									\
			for (m = 0; m < (cc / (bits / NBBY)); m++) {	\
				if (vc->sc_swvol == 255)		\
					goto vol_done;			\
				tomix[m] = (bigger_type)tomix[m] *	\
				    (bigger_type)(vc->sc_swvol) / 255;	\
vol_done:								\
				result = (bigger_type)orig[m] + tomix[m]; \
				if (sc->sc_opens == 1)			\
					goto adj_done;			\
				product = (bigger_type)orig[m] * tomix[m]; \
				if (orig[m] > 0 && tomix[m] > 0)	\
					result -= product / MAXVAL;	\
				else if (orig[m] < 0 && tomix[m] < 0)	\
					result -= product / MINVAL;	\
adj_done:								\
				orig[m] = result;			\
			}						\
									\
			if (&orig[m] >=					\
			    (type *)sc->sc_mixring.sc_mpr.s.end)	\
				orig =					\
				 (type *)sc->sc_mixring.sc_mpr.s.start;	\
			if (&tomix[m] >= (type *)cb->s.end)		\
				tomix = (type *)cb->s.start;		\
									\
			resid -= cc;					\
		}							\
	}								\

DEF_MIX_FUNC(8, int8_t, int32_t, INT8_MIN, INT8_MAX);
DEF_MIX_FUNC(16, int16_t, int32_t, INT16_MIN, INT16_MAX);
DEF_MIX_FUNC(32, int32_t, int64_t, INT32_MIN, INT32_MAX);

void
mix_func(struct audio_softc *sc, struct audio_ringbuffer *cb,
	 struct virtual_channel *vc)
{
	switch (sc->sc_vchan_params.precision) {
	case 8:
		mix_func8(sc, cb, vc);
		break;
	case 16:
		mix_func16(sc, cb, vc);
		break;
	case 24:
	case 32:
		mix_func32(sc, cb, vc);
		break;
	default:
		break;
	}
}

#define DEF_RECSWVOL_FUNC(bits, type, bigger_type)			\
	static void						\
	recswvol_func##bits(struct audio_softc *sc,			\
	    struct audio_ringbuffer *cb, size_t blksize,		\
	    struct virtual_channel *vc)					\
	{								\
		int cc, cc1, m, resid;					\
		type *orig;						\
									\
		orig = (type *) cb->s.inp;				\
		resid = blksize;					\
									\
		while (resid > 0) {					\
			cc = resid;					\
			cc1 = cb->s.end - (uint8_t *)orig;		\
			if (cc > cc1)					\
				cc = cc1;				\
									\
			for (m = 0; m < (cc / (bits / 8)); m++) {	\
				orig[m] = (bigger_type)(orig[m] *	\
				    (bigger_type)(vc->sc_recswvol) / 256);\
			}						\
			orig = (type *) cb->s.start;			\
									\
			resid -= cc;					\
		}							\
	}								\

DEF_RECSWVOL_FUNC(8, int8_t, int16_t);
DEF_RECSWVOL_FUNC(16, int16_t, int32_t);
DEF_RECSWVOL_FUNC(32, int32_t, int64_t);

void
recswvol_func(struct audio_softc *sc, struct audio_ringbuffer *cb,
    size_t blksize, struct virtual_channel *vc)
{
	switch (sc->sc_vchan_params.precision) {
	case 8:
		recswvol_func8(sc, cb, blksize, vc);
		break;
	case 16:
		recswvol_func16(sc, cb, blksize, vc);
		break;
	case 24:
	case 32:
		recswvol_func32(sc, cb, blksize, vc);
		break;
	default:
		break;
	}
}

static uint8_t *
find_vchan_vol(struct audio_softc *sc, int d)
{
	struct audio_chan *chan;
	size_t j, n = (size_t)d / 2;
	
	j = 0;
	SIMPLEQ_FOREACH(chan, &sc->sc_audiochan, entries) {
		if (j == n)
			break;
		j++;
	}
	return (d & 1) == 0 ?
	    &chan->vc->sc_swvol : &chan->vc->sc_recswvol;
}

static int
audio_set_port(struct audio_softc *sc, mixer_ctrl_t *mc)
{
	KASSERT(mutex_owned(sc->sc_lock));

	int d = mc->dev - sc->sc_static_nmixer_states;
	int u = sc->sc_nmixer_states - sc->sc_static_nmixer_states;

	if (d == -1 || d >= u)
		return 0;
	if (d < 0)
		return sc->hw_if->set_port(sc->hw_hdl, mc);

	uint8_t *level = &mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
	uint8_t *vol = find_vchan_vol(sc, d);
	*vol = *level;
	return 0;
}


static int
audio_get_port(struct audio_softc *sc, mixer_ctrl_t *mc)
{

	KASSERT(mutex_owned(sc->sc_lock));

	int d = mc->dev - sc->sc_static_nmixer_states;
	int u = sc->sc_nmixer_states - sc->sc_static_nmixer_states;

	if (d == -1 || d >= u)
		return 0;
	if (d < 0)
		return sc->hw_if->get_port(sc->hw_hdl, mc);

	u_char *level = &mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
	uint8_t *vol = find_vchan_vol(sc, d);
	*level = *vol;
	return 0;

}

static int
audio_query_devinfo(struct audio_softc *sc, mixer_devinfo_t *di)
{
	char mixLabel[255];

	KASSERT(mutex_owned(sc->sc_lock));

	if (sc->sc_static_nmixer_states == 0 || sc->sc_nmixer_states == 0)
		goto hardware;

	if (di->index >= sc->sc_static_nmixer_states - 1 &&
	    di->index < sc->sc_nmixer_states) {
		if (di->index == sc->sc_static_nmixer_states - 1) {
			di->mixer_class = sc->sc_static_nmixer_states -1;
			di->next = di->prev = AUDIO_MIXER_LAST;
			strcpy(di->label.name, AudioCvirtchan);
			di->type = AUDIO_MIXER_CLASS;
		} else if ((di->index - sc->sc_static_nmixer_states) % 2 == 0) {
			di->mixer_class = sc->sc_static_nmixer_states -1;
			snprintf(mixLabel, sizeof(mixLabel), AudioNdac"%d",
			    (di->index - sc->sc_static_nmixer_states) / 2);
			strcpy(di->label.name, mixLabel);
			di->type = AUDIO_MIXER_VALUE;
			di->next = di->prev = AUDIO_MIXER_LAST;
			di->un.v.num_channels = 1;
			strcpy(di->un.v.units.name, AudioNvolume);
		} else {
			di->mixer_class = sc->sc_static_nmixer_states -1;
			snprintf(mixLabel, sizeof(mixLabel),
			    AudioNmicrophone "%d",
			    (di->index - sc->sc_static_nmixer_states) / 2);
			strcpy(di->label.name, mixLabel);
			di->type = AUDIO_MIXER_VALUE;
			di->next = di->prev = AUDIO_MIXER_LAST;
			di->un.v.num_channels = 1;
			strcpy(di->un.v.units.name, AudioNvolume);
		}

		return 0;
	}

hardware:
	return sc->hw_if->query_devinfo(sc->hw_hdl, di);
}

static int
audio_set_params(struct audio_softc *sc, int setmode, int usemode,
		 audio_params_t *play, audio_params_t *rec,
		 stream_filter_list_t *pfil, stream_filter_list_t *rfil,
		 const struct virtual_channel *vc)
{
	int error = 0;

	KASSERT(mutex_owned(sc->sc_lock));

	if (vc == sc->sc_hwvc) {
		sc->sc_ready = true;
		if (sc->sc_usemixer && sc->sc_vchan_params.precision == 8)
			play->encoding = rec->encoding = AUDIO_ENCODING_SLINEAR;
		error = sc->hw_if->set_params(sc->hw_hdl, setmode, usemode,
 		    play, rec, pfil, rfil);
		if (error != 0)
			sc->sc_ready = false;

		return error;
	}

	if (setmode & AUMODE_PLAY && auconv_set_converter(sc->sc_format,
	    VAUDIO_NFORMATS, AUMODE_PLAY, play, true, pfil) < 0)
		return EINVAL;

	if (setmode & AUMODE_RECORD && auconv_set_converter(sc->sc_format,
	    VAUDIO_NFORMATS, AUMODE_RECORD, rec, true, rfil) < 0)
		return EINVAL;

	return 0;
}

static int
audio_query_encoding(struct audio_softc *sc, struct audio_encoding *ae)
{
	KASSERT(mutex_owned(sc->sc_lock));

	return auconv_query_encoding(sc->sc_encodings, ae);
}

void
audio_play_thread(void *v)
{
	struct audio_softc *sc;
	
	sc = (struct audio_softc *)v;

	for (;;) {
		mutex_enter(sc->sc_intr_lock);
		cv_wait_sig(&sc->sc_condvar, sc->sc_intr_lock);
		if (sc->sc_dying) {
			mutex_exit(sc->sc_intr_lock);
			kthread_exit(0);
		}

		while (sc->sc_usemixer &&
		    audio_stream_get_used(&sc->sc_mixring.sc_mpr.s) <
						sc->sc_mixring.sc_mpr.blksize) {
			mutex_exit(sc->sc_intr_lock);
			mutex_enter(sc->sc_lock);
			audio_mix(sc);
			mutex_exit(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
		}
		mutex_exit(sc->sc_intr_lock);
	}
}

void
audio_rec_thread(void *v)
{
	struct audio_softc *sc;
	
	sc = (struct audio_softc *)v;

	for (;;) {
		mutex_enter(sc->sc_intr_lock);
		cv_wait_sig(&sc->sc_rcondvar, sc->sc_intr_lock);
		if (sc->sc_dying) {
			mutex_exit(sc->sc_intr_lock);
			kthread_exit(0);
		}
		mutex_exit(sc->sc_intr_lock);

		mutex_enter(sc->sc_lock);
		audio_upmix(sc);
		mutex_exit(sc->sc_lock);
	}

}

/* sysctl helper to set common audio frequency */
static int
audio_sysctl_frequency(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	int t, error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_vchan_params.sample_rate;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(sc->sc_lock);

	/* This may not change when a virtual channel is open */
	if (sc->sc_opens || sc->sc_recopens) {
		mutex_exit(sc->sc_lock);
		return EBUSY;
	}

	if (t <= 0) {
		mutex_exit(sc->sc_lock);
		return EINVAL;
	}

	sc->sc_vchan_params.sample_rate = t;
	error = audio_set_vchan_defaults(sc,
	    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);
	if (error)
		aprint_error_dev(sc->sc_dev, "Error setting frequency, "
				 "please check hardware capabilities\n");
	if (error == 0)
		audio_calc_latency(sc);
	mutex_exit(sc->sc_lock);

	return error;
}

/* sysctl helper to set common audio precision */
static int
audio_sysctl_precision(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	int t, error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_vchan_params.precision;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(sc->sc_lock);

	/* This may not change when a virtual channel is open */
	if (sc->sc_opens || sc->sc_recopens) {
		mutex_exit(sc->sc_lock);
		return EBUSY;
	}

	if (t == 0 || (t != 8 && t != 16 && t != 24 && t != 32)) {
		mutex_exit(sc->sc_lock);
		return EINVAL;
	}

	sc->sc_vchan_params.precision = t;

	if (sc->sc_vchan_params.precision != 8) {
		sc->sc_vchan_params.encoding =
#if BYTE_ORDER == LITTLE_ENDIAN
			 AUDIO_ENCODING_SLINEAR_LE;
#else
			 AUDIO_ENCODING_SLINEAR_BE;
#endif
	} else
		sc->sc_vchan_params.encoding = AUDIO_ENCODING_SLINEAR_LE;

	error = audio_set_vchan_defaults(sc,
	    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);
	if (error)
		aprint_error_dev(sc->sc_dev, "Error setting precision, "
				 "please check hardware capabilities\n");
	if (error == 0)
		audio_calc_latency(sc);
	mutex_exit(sc->sc_lock);

	return error;
}

/* sysctl helper to enable/disable channel mixing */
static int
audio_sysctl_usemixer(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	bool t;
	int error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_usemixer;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(sc->sc_lock);

	/* This may not change when a virtual channel is open */
	if (sc->sc_opens) {
		mutex_exit(sc->sc_lock);
		return EBUSY;
	}

	sc->sc_usemixer = t;
	audio_destroy_pfilters(sc->sc_hwvc);
	audio_destroy_rfilters(sc->sc_hwvc);
	if (t) {
		error = audio_set_vchan_defaults(sc,
		    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);
		if (error)
			aprint_error_dev(sc->sc_dev, "Error setting precision, "
					 "please check hardware capabilities\n");
	}

	if (sc->sc_usemixer) {
		if (error == 0)
			audio_calc_latency(sc);
	} else
		sc->sc_latency = audio_blk_ms * PREFILL_BLOCKS;
		
	mutex_exit(sc->sc_lock);

	return error;
}

/* sysctl helper to set common audio channels */
static int
audio_sysctl_channels(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	int t, error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_vchan_params.channels;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(sc->sc_lock);

	/* This may not change when a virtual channel is open */
	if (sc->sc_opens || sc->sc_recopens) {
		mutex_exit(sc->sc_lock);
		return EBUSY;
	}

	if (t <= 0 || (t !=1 && t % 2 != 0)) {
		mutex_exit(sc->sc_lock);
		return EINVAL;
	}

	sc->sc_vchan_params.channels = t;
	error = audio_set_vchan_defaults(sc,
	    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);
	if (error)
		aprint_error_dev(sc->sc_dev, "Error setting channels, "
				 "please check hardware capabilities\n");
	if (error == 0)
		audio_calc_latency(sc);
	mutex_exit(sc->sc_lock);

	return error;
}

/* sysctl helper to set audio latency */
static int
audio_sysctl_latency(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	int t, error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_latency;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(sc->sc_lock);

	/* This may not change when a virtual channel is open */
	if (sc->sc_opens || sc->sc_recopens) {
		mutex_exit(sc->sc_lock);
		return EBUSY;
	}

	if (t < 0 || t > 4000) {
		mutex_exit(sc->sc_lock);
		return EINVAL;
	}

	if (t == 0)
		sc->sc_latency = audio_blk_ms * PREFILL_BLOCKS;
	else
		sc->sc_latency = (unsigned int)t;

	error = audio_set_vchan_defaults(sc,
	    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Error setting latency, "
				 "latency restored to default\n");
		sc->sc_latency = audio_blk_ms * PREFILL_BLOCKS;
		error = audio_set_vchan_defaults(sc,
	    	    AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD);
	}

	audio_calc_latency(sc);
	mutex_exit(sc->sc_lock);

	return error;
}

static int
vchan_autoconfig(struct audio_softc *sc)
{
	struct virtual_channel *vc;
	uint i, j, k;
	int error;

	vc = sc->sc_hwvc;
	error = 0;

	mutex_enter(sc->sc_lock);

#if BYTE_ORDER == LITTLE_ENDIAN
	sc->sc_vchan_params.encoding = AUDIO_ENCODING_SLINEAR_LE;
#else
	sc->sc_vchan_params.encoding = AUDIO_ENCODING_SLINEAR_BE;
#endif

	for (i = 0; i < __arraycount(auto_config_precision); i++) {
		sc->sc_vchan_params.precision = auto_config_precision[i];
		sc->sc_vchan_params.validbits = auto_config_precision[i];
		for (j = 0; j < __arraycount(auto_config_channels); j++) {
			sc->sc_vchan_params.channels = auto_config_channels[j];
			for (k = 0; k < __arraycount(auto_config_freq); k++) {
				sc->sc_vchan_params.sample_rate =
				    auto_config_freq[k];
				error = audio_set_vchan_defaults(sc,
				    AUMODE_PLAY | AUMODE_PLAY_ALL |
				    AUMODE_RECORD);
				if (vc->sc_npfilters > 0 &&
				    (vc->sc_mpr.s.param.sample_rate !=
				     sc->sc_vchan_params.sample_rate ||
				    vc->sc_mpr.s.param.channels !=
				     sc->sc_vchan_params.channels))
					error = EINVAL;

				if (error == 0) {
					aprint_normal_dev(sc->sc_dev,
					    "Virtual format configured - "
					    "Format SLINEAR, precision %d, "
					    "channels %d, frequency %d\n",
						sc->sc_vchan_params.precision,
						sc->sc_vchan_params.channels,
						sc->sc_vchan_params.sample_rate);

					goto found;
				}
			}
		}
	}

found:
	if (error == 0) {
		audio_calc_latency(sc);
		aprint_normal_dev(sc->sc_dev, "Latency: %d milliseconds\n",
		    sc->sc_latency);
	} else {
		aprint_error_dev(sc->sc_dev, "Virtual format auto config failed!\n");
		aprint_error_dev(sc->sc_dev, "Please check hardware capabilities\n");
	}
	mutex_exit(sc->sc_lock);

	return error;
}

static void
grow_mixer_states(struct audio_softc *sc, int count)
{
	mixer_ctrl_t *tmp_mixer_state;
	size_t origlen = sizeof(mixer_ctrl_t) * (sc->sc_nmixer_states + 1);
	size_t newlen = sizeof(mixer_ctrl_t) * count + origlen;

	tmp_mixer_state = kmem_zalloc(newlen, KM_SLEEP);
	memcpy(tmp_mixer_state, sc->sc_mixer_state, origlen);

	sc->sc_nmixer_states += count;
	kmem_free(sc->sc_mixer_state, origlen);
	sc->sc_mixer_state = tmp_mixer_state;
}	

static void
shrink_mixer_states(struct audio_softc *sc, int count)
{
	mixer_ctrl_t *tmp_mixer_state;
	size_t origlen = sizeof(mixer_ctrl_t) * (sc->sc_nmixer_states + 1);
	size_t newlen = origlen - sizeof(mixer_ctrl_t) * count;

	tmp_mixer_state = kmem_zalloc(newlen, KM_SLEEP);
	memcpy(tmp_mixer_state, sc->sc_mixer_state, newlen);

	sc->sc_nmixer_states -= count;
	kmem_free(sc->sc_mixer_state, origlen);
	sc->sc_mixer_state = tmp_mixer_state;
}	

#endif /* NAUDIO > 0 */

#ifdef _MODULE

devmajor_t audio_bmajor = -1, audio_cmajor = -1;

#include "ioconf.c"

#endif

MODULE(MODULE_CLASS_DRIVER, audio, NULL);

static int
audio_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = devsw_attach(audio_cd.cd_name, NULL, &audio_bmajor,
		    &audio_cdevsw, &audio_cmajor);
		if (error)
			break;

		error = config_init_component(cfdriver_ioconf_audio,
		    cfattach_ioconf_audio, cfdata_ioconf_audio);
		if (error) {
			devsw_detach(NULL, &audio_cdevsw);
		}
		break;
	case MODULE_CMD_FINI:
		devsw_detach(NULL, &audio_cdevsw);
		error = config_fini_component(cfdriver_ioconf_audio,
		   cfattach_ioconf_audio, cfdata_ioconf_audio);
		if (error)
			devsw_attach(audio_cd.cd_name, NULL, &audio_bmajor,
			    &audio_cdevsw, &audio_cmajor);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
