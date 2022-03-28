/*	$NetBSD: audio.c,v 1.121 2022/03/28 12:39:57 riastradh Exp $	*/

/*-
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
 * Terminology: "sample", "channel", "frame", "block", "track":
 *
 *  channel       frame
 *   |           ........
 *   v           :      :                                    \
 *        +------:------:------:-  -+------+ : +------+-..   |
 *  #0(L) |sample|sample|sample| .. |sample| : |sample|      |
 *        +------:------:------:-  -+------+ : +------+-..   |
 *  #1(R) |sample|sample|sample| .. |sample| : |sample|      |
 *        +------:------:------:-  -+------+ : +------+-..   | track
 *   :           :      :                    :               |
 *        +------:------:------:-  -+------+ : +------+-..   |
 *        |sample|sample|sample| .. |sample| : |sample|      |
 *        +------:------:------:-  -+------+ : +------+-..   |
 *               :      :                                    /
 *               ........
 *
 *        \--------------------------------/   \--------..
 *                     block
 *
 * - A "frame" is the minimum unit in the time axis direction, and consists
 *   of samples for the number of channels.
 * - A "block" is basic length of processing.  The audio layer basically
 *   handles audio data stream block by block, asks underlying hardware to
 *   process them block by block, and then the hardware raises interrupt by
 *   each block.
 * - A "track" is single completed audio stream.
 *
 * For example, the hardware block is assumed to be 10 msec, and your audio
 * track consists of 2.1(=3) channels 44.1kHz 16bit PCM,
 *
 * "channel" = 3
 * "sample" = 2 [bytes]
 * "frame" = 2 [bytes/sample] * 3 [channels] = 6 [bytes]
 * "block" = 44100 [Hz] * (10/1000) [seconds] * 6 [bytes/frame] = 2646 [bytes]
 *
 * The terminologies shown here are only for this MI audio layer.  Note that
 * different terminologies may be used in each manufacturer's datasheet, and
 * each MD driver may follow it.  For example, what we call a "block" is
 * called a "frame" in sys/dev/pci/yds.c.
 */

/*
 * Locking: there are three locks per device.
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
 * - sc_exlock, private to this module.  This is a variable protected by
 *   sc_lock.  It is known as the "critical section".
 *   Some operations release sc_lock in order to allocate memory, to wait
 *   for in-flight I/O to complete, to copy to/from user context, etc.
 *   sc_exlock provides a critical section even under the circumstance.
 *   "+" in following list indicates the interfaces which necessary to be
 *   protected by sc_exlock.
 *
 * List of hardware interface methods, and which locks are held when each
 * is called by this module:
 *
 *	METHOD			INTR	THREAD  NOTES
 *	----------------------- ------- -------	-------------------------
 *	open 			x	x +
 *	close 			x	x +
 *	query_format		-	x
 *	set_format		-	x
 *	round_blocksize		-	x
 *	commit_settings		-	x
 *	init_output 		x	x
 *	init_input 		x	x
 *	start_output 		x	x +
 *	start_input 		x	x +
 *	halt_output 		x	x +
 *	halt_input 		x	x +
 *	speaker_ctl 		x	x
 *	getdev 			-	-
 *	set_port 		-	x +
 *	get_port 		-	x +
 *	query_devinfo 		-	x
 *	allocm 			-	- +
 *	freem 			-	- +
 *	round_buffersize 	-	x
 *	get_props 		-	-	Called at attach time
 *	trigger_output 		x	x +
 *	trigger_input 		x	x +
 *	dev_ioctl 		-	x
 *	get_locks 		-	-	Called at attach time
 *
 * In addition, there is an additional lock.
 *
 * - track->lock.  This is an atomic variable and is similar to the
 *   "interrupt lock".  This is one for each track.  If any thread context
 *   (and software interrupt context) and hardware interrupt context who
 *   want to access some variables on this track, they must acquire this
 *   lock before.  It protects track's consistency between hardware
 *   interrupt context and others.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: audio.c,v 1.121 2022/03/28 12:39:57 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "audio.h"
#include "midi.h"
#endif

#if NAUDIO > 0

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/audioio.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/module.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/vnode.h>

#include <dev/audio/audio_if.h>
#include <dev/audio/audiovar.h>
#include <dev/audio/audiodef.h>
#include <dev/audio/linear.h>
#include <dev/audio/mulaw.h>

#include <machine/endian.h>

#include <uvm/uvm_extern.h>

#include "ioconf.h"

/*
 * 0: No debug logs
 * 1: action changes like open/close/set_format...
 * 2: + normal operations like read/write/ioctl...
 * 3: + TRACEs except interrupt
 * 4: + TRACEs including interrupt
 */
//#define AUDIO_DEBUG 1

#if defined(AUDIO_DEBUG)

int audiodebug = AUDIO_DEBUG;
static void audio_vtrace(struct audio_softc *sc, const char *, const char *,
	const char *, va_list);
static void audio_trace(struct audio_softc *sc, const char *, const char *, ...)
	__printflike(3, 4);
static void audio_tracet(const char *, audio_track_t *, const char *, ...)
	__printflike(3, 4);
static void audio_tracef(const char *, audio_file_t *, const char *, ...)
	__printflike(3, 4);

/* XXX sloppy memory logger */
static void audio_mlog_init(void);
static void audio_mlog_free(void);
static void audio_mlog_softintr(void *);
extern void audio_mlog_flush(void);
extern void audio_mlog_printf(const char *, ...);

static int mlog_refs;		/* reference counter */
static char *mlog_buf[2];	/* double buffer */
static int mlog_buflen;		/* buffer length */
static int mlog_used;		/* used length */
static int mlog_full;		/* number of dropped lines by buffer full */
static int mlog_drop;		/* number of dropped lines by busy */
static volatile uint32_t mlog_inuse;	/* in-use */
static int mlog_wpage;		/* active page */
static void *mlog_sih;		/* softint handle */

static void
audio_mlog_init(void)
{
	mlog_refs++;
	if (mlog_refs > 1)
		return;
	mlog_buflen = 4096;
	mlog_buf[0] = kmem_zalloc(mlog_buflen, KM_SLEEP);
	mlog_buf[1] = kmem_zalloc(mlog_buflen, KM_SLEEP);
	mlog_used = 0;
	mlog_full = 0;
	mlog_drop = 0;
	mlog_inuse = 0;
	mlog_wpage = 0;
	mlog_sih = softint_establish(SOFTINT_SERIAL, audio_mlog_softintr, NULL);
	if (mlog_sih == NULL)
		printf("%s: softint_establish failed\n", __func__);
}

static void
audio_mlog_free(void)
{
	mlog_refs--;
	if (mlog_refs > 0)
		return;

	audio_mlog_flush();
	if (mlog_sih)
		softint_disestablish(mlog_sih);
	kmem_free(mlog_buf[0], mlog_buflen);
	kmem_free(mlog_buf[1], mlog_buflen);
}

/*
 * Flush memory buffer.
 * It must not be called from hardware interrupt context.
 */
void
audio_mlog_flush(void)
{
	if (mlog_refs == 0)
		return;

	/* Nothing to do if already in use ? */
	if (atomic_swap_32(&mlog_inuse, 1) == 1)
		return;
	membar_enter();

	int rpage = mlog_wpage;
	mlog_wpage ^= 1;
	mlog_buf[mlog_wpage][0] = '\0';
	mlog_used = 0;

	atomic_store_release(&mlog_inuse, 0);

	if (mlog_buf[rpage][0] != '\0') {
		printf("%s", mlog_buf[rpage]);
		if (mlog_drop > 0)
			printf("mlog_drop %d\n", mlog_drop);
		if (mlog_full > 0)
			printf("mlog_full %d\n", mlog_full);
	}
	mlog_full = 0;
	mlog_drop = 0;
}

static void
audio_mlog_softintr(void *cookie)
{
	audio_mlog_flush();
}

void
audio_mlog_printf(const char *fmt, ...)
{
	int len;
	va_list ap;

	if (atomic_swap_32(&mlog_inuse, 1) == 1) {
		/* already inuse */
		mlog_drop++;
		return;
	}
	membar_enter();

	va_start(ap, fmt);
	len = vsnprintf(
	    mlog_buf[mlog_wpage] + mlog_used,
	    mlog_buflen - mlog_used,
	    fmt, ap);
	va_end(ap);

	mlog_used += len;
	if (mlog_buflen - mlog_used <= 1) {
		mlog_full++;
	}

	atomic_store_release(&mlog_inuse, 0);

	if (mlog_sih)
		softint_schedule(mlog_sih);
}

/* trace functions */
static void
audio_vtrace(struct audio_softc *sc, const char *funcname, const char *header,
	const char *fmt, va_list ap)
{
	char buf[256];
	int n;

	n = 0;
	buf[0] = '\0';
	n += snprintf(buf + n, sizeof(buf) - n, "%s@%d %s",
	    funcname, device_unit(sc->sc_dev), header);
	n += vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);

	if (cpu_intr_p()) {
		audio_mlog_printf("%s\n", buf);
	} else {
		audio_mlog_flush();
		printf("%s\n", buf);
	}
}

static void
audio_trace(struct audio_softc *sc, const char *funcname, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	audio_vtrace(sc, funcname, "", fmt, ap);
	va_end(ap);
}

static void
audio_tracet(const char *funcname, audio_track_t *track, const char *fmt, ...)
{
	char hdr[16];
	va_list ap;

	snprintf(hdr, sizeof(hdr), "#%d ", track->id);
	va_start(ap, fmt);
	audio_vtrace(track->mixer->sc, funcname, hdr, fmt, ap);
	va_end(ap);
}

static void
audio_tracef(const char *funcname, audio_file_t *file, const char *fmt, ...)
{
	char hdr[32];
	char phdr[16], rhdr[16];
	va_list ap;

	phdr[0] = '\0';
	rhdr[0] = '\0';
	if (file->ptrack)
		snprintf(phdr, sizeof(phdr), "#%d", file->ptrack->id);
	if (file->rtrack)
		snprintf(rhdr, sizeof(rhdr), "#%d", file->rtrack->id);
	snprintf(hdr, sizeof(hdr), "{%s,%s} ", phdr, rhdr);

	va_start(ap, fmt);
	audio_vtrace(file->sc, funcname, hdr, fmt, ap);
	va_end(ap);
}

#define DPRINTF(n, fmt...)	do {	\
	if (audiodebug >= (n)) {	\
		audio_mlog_flush();	\
		printf(fmt);		\
	}				\
} while (0)
#define TRACE(n, fmt...)	do { \
	if (audiodebug >= (n)) audio_trace(sc, __func__, fmt); \
} while (0)
#define TRACET(n, t, fmt...)	do { \
	if (audiodebug >= (n)) audio_tracet(__func__, t, fmt); \
} while (0)
#define TRACEF(n, f, fmt...)	do { \
	if (audiodebug >= (n)) audio_tracef(__func__, f, fmt); \
} while (0)

struct audio_track_debugbuf {
	char usrbuf[32];
	char codec[32];
	char chvol[32];
	char chmix[32];
	char freq[32];
	char outbuf[32];
};

static void
audio_track_bufstat(audio_track_t *track, struct audio_track_debugbuf *buf)
{

	memset(buf, 0, sizeof(*buf));

	snprintf(buf->outbuf, sizeof(buf->outbuf), " out=%d/%d/%d",
	    track->outbuf.head, track->outbuf.used, track->outbuf.capacity);
	if (track->freq.filter)
		snprintf(buf->freq, sizeof(buf->freq), " f=%d/%d/%d",
		    track->freq.srcbuf.head,
		    track->freq.srcbuf.used,
		    track->freq.srcbuf.capacity);
	if (track->chmix.filter)
		snprintf(buf->chmix, sizeof(buf->chmix), " m=%d",
		    track->chmix.srcbuf.used);
	if (track->chvol.filter)
		snprintf(buf->chvol, sizeof(buf->chvol), " v=%d",
		    track->chvol.srcbuf.used);
	if (track->codec.filter)
		snprintf(buf->codec, sizeof(buf->codec), " e=%d",
		    track->codec.srcbuf.used);
	snprintf(buf->usrbuf, sizeof(buf->usrbuf), " usr=%d/%d/H%d",
	    track->usrbuf.head, track->usrbuf.used, track->usrbuf_usedhigh);
}
#else
#define DPRINTF(n, fmt...)	do { } while (0)
#define TRACE(n, fmt, ...)	do { } while (0)
#define TRACET(n, t, fmt, ...)	do { } while (0)
#define TRACEF(n, f, fmt, ...)	do { } while (0)
#endif

#define SPECIFIED(x)	((x) != ~0)
#define SPECIFIED_CH(x)	((x) != (u_char)~0)

/*
 * Default hardware blocksize in msec.
 *
 * We use 10 msec for most modern platforms.  This period is good enough to
 * play audio and video synchronizely.
 * In contrast, for very old platforms, this is usually too short and too
 * severe.  Also such platforms usually can not play video confortably, so
 * it's not so important to make the blocksize shorter.  If the platform
 * defines its own value as __AUDIO_BLK_MS in its <machine/param.h>, it
 * uses this instead.
 *
 * In either case, you can overwrite AUDIO_BLK_MS by your kernel
 * configuration file if you wish.
 */
#if !defined(AUDIO_BLK_MS)
# if defined(__AUDIO_BLK_MS)
#  define AUDIO_BLK_MS __AUDIO_BLK_MS
# else
#  define AUDIO_BLK_MS (10)
# endif
#endif

/* Device timeout in msec */
#define AUDIO_TIMEOUT	(3000)

/* #define AUDIO_PM_IDLE */
#ifdef AUDIO_PM_IDLE
int audio_idle_timeout = 30;
#endif

/* Number of elements of async mixer's pid */
#define AM_CAPACITY	(4)

struct portname {
	const char *name;
	int mask;
};

static int audiomatch(device_t, cfdata_t, void *);
static void audioattach(device_t, device_t, void *);
static int audiodetach(device_t, int);
static int audioactivate(device_t, enum devact);
static void audiochilddet(device_t, device_t);
static int audiorescan(device_t, const char *, const int *);

static int audio_modcmd(modcmd_t, void *);

#ifdef AUDIO_PM_IDLE
static void audio_idle(void *);
static void audio_activity(device_t, devactive_t);
#endif

static bool audio_suspend(device_t dv, const pmf_qual_t *);
static bool audio_resume(device_t dv, const pmf_qual_t *);
static void audio_volume_down(device_t);
static void audio_volume_up(device_t);
static void audio_volume_toggle(device_t);

static void audio_mixer_capture(struct audio_softc *);
static void audio_mixer_restore(struct audio_softc *);

static void audio_softintr_rd(void *);
static void audio_softintr_wr(void *);

static void audio_printf(struct audio_softc *, const char *, ...)
	__printflike(2, 3);
static int audio_exlock_mutex_enter(struct audio_softc *);
static void audio_exlock_mutex_exit(struct audio_softc *);
static int audio_exlock_enter(struct audio_softc *);
static void audio_exlock_exit(struct audio_softc *);
static struct audio_softc *audio_sc_acquire_fromfile(audio_file_t *,
	struct psref *);
static void audio_sc_release(struct audio_softc *, struct psref *);
static int audio_track_waitio(struct audio_softc *, audio_track_t *);

static int audioclose(struct file *);
static int audioread(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int audiowrite(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int audioioctl(struct file *, u_long, void *);
static int audiopoll(struct file *, int);
static int audiokqfilter(struct file *, struct knote *);
static int audiommap(struct file *, off_t *, size_t, int, int *, int *,
	struct uvm_object **, int *);
static int audiostat(struct file *, struct stat *);

static void filt_audiowrite_detach(struct knote *);
static int  filt_audiowrite_event(struct knote *, long);
static void filt_audioread_detach(struct knote *);
static int  filt_audioread_event(struct knote *, long);

static int audio_open(dev_t, struct audio_softc *, int, int, struct lwp *,
	audio_file_t **);
static int audio_close(struct audio_softc *, audio_file_t *);
static void audio_unlink(struct audio_softc *, audio_file_t *);
static int audio_read(struct audio_softc *, struct uio *, int, audio_file_t *);
static int audio_write(struct audio_softc *, struct uio *, int, audio_file_t *);
static void audio_file_clear(struct audio_softc *, audio_file_t *);
static int audio_ioctl(dev_t, struct audio_softc *, u_long, void *, int,
	struct lwp *, audio_file_t *);
static int audio_poll(struct audio_softc *, int, struct lwp *, audio_file_t *);
static int audio_kqfilter(struct audio_softc *, audio_file_t *, struct knote *);
static int audio_mmap(struct audio_softc *, off_t *, size_t, int, int *, int *,
	struct uvm_object **, int *, audio_file_t *);

static int audioctl_open(dev_t, struct audio_softc *, int, int, struct lwp *);

static void audio_pintr(void *);
static void audio_rintr(void *);

static int audio_query_devinfo(struct audio_softc *, mixer_devinfo_t *);

static __inline int audio_track_readablebytes(const audio_track_t *);
static int audio_file_setinfo(struct audio_softc *, audio_file_t *,
	const struct audio_info *);
static int audio_track_setinfo_check(audio_track_t *,
	audio_format2_t *, const struct audio_prinfo *);
static void audio_track_setinfo_water(audio_track_t *,
	const struct audio_info *);
static int audio_hw_setinfo(struct audio_softc *, const struct audio_info *,
	struct audio_info *);
static int audio_hw_set_format(struct audio_softc *, int,
	const audio_format2_t *, const audio_format2_t *,
	audio_filter_reg_t *, audio_filter_reg_t *);
static int audiogetinfo(struct audio_softc *, struct audio_info *, int,
	audio_file_t *);
static bool audio_can_playback(struct audio_softc *);
static bool audio_can_capture(struct audio_softc *);
static int audio_check_params(audio_format2_t *);
static int audio_mixers_init(struct audio_softc *sc, int,
	const audio_format2_t *, const audio_format2_t *,
	const audio_filter_reg_t *, const audio_filter_reg_t *);
static int audio_select_freq(const struct audio_format *);
static int audio_hw_probe(struct audio_softc *, audio_format2_t *, int);
static int audio_hw_validate_format(struct audio_softc *, int,
	const audio_format2_t *);
static int audio_mixers_set_format(struct audio_softc *,
	const struct audio_info *);
static void audio_mixers_get_format(struct audio_softc *, struct audio_info *);
static int audio_sysctl_blk_ms(SYSCTLFN_PROTO);
static int audio_sysctl_multiuser(SYSCTLFN_PROTO);
#if defined(AUDIO_DEBUG)
static int audio_sysctl_debug(SYSCTLFN_PROTO);
static void audio_format2_tostr(char *, size_t, const audio_format2_t *);
static void audio_print_format2(const char *, const audio_format2_t *) __unused;
#endif

static void *audio_realloc(void *, size_t);
static int audio_realloc_usrbuf(audio_track_t *, int);
static void audio_free_usrbuf(audio_track_t *);

static audio_track_t *audio_track_create(struct audio_softc *,
	audio_trackmixer_t *);
static void audio_track_destroy(audio_track_t *);
static audio_filter_t audio_track_get_codec(audio_track_t *,
	const audio_format2_t *, const audio_format2_t *);
static int audio_track_set_format(audio_track_t *, audio_format2_t *);
static void audio_track_play(audio_track_t *);
static int audio_track_drain(struct audio_softc *, audio_track_t *);
static void audio_track_record(audio_track_t *);
static void audio_track_clear(struct audio_softc *, audio_track_t *);

static int audio_mixer_init(struct audio_softc *, int,
	const audio_format2_t *, const audio_filter_reg_t *);
static void audio_mixer_destroy(struct audio_softc *, audio_trackmixer_t *);
static void audio_pmixer_start(struct audio_softc *, bool);
static void audio_pmixer_process(struct audio_softc *);
static void audio_pmixer_agc(audio_trackmixer_t *, int);
static int  audio_pmixer_mix_track(audio_trackmixer_t *, audio_track_t *, int);
static void audio_pmixer_output(struct audio_softc *);
static int  audio_pmixer_halt(struct audio_softc *);
static void audio_rmixer_start(struct audio_softc *);
static void audio_rmixer_process(struct audio_softc *);
static void audio_rmixer_input(struct audio_softc *);
static int  audio_rmixer_halt(struct audio_softc *);

static void mixer_init(struct audio_softc *);
static int mixer_open(dev_t, struct audio_softc *, int, int, struct lwp *);
static int mixer_close(struct audio_softc *, audio_file_t *);
static int mixer_ioctl(struct audio_softc *, u_long, void *, int, struct lwp *);
static void mixer_async_add(struct audio_softc *, pid_t);
static void mixer_async_remove(struct audio_softc *, pid_t);
static void mixer_signal(struct audio_softc *);

static int au_portof(struct audio_softc *, char *, int);

static void au_setup_ports(struct audio_softc *, struct au_mixer_ports *,
	mixer_devinfo_t *, const struct portname *);
static int au_set_lr_value(struct audio_softc *, mixer_ctrl_t *, int, int);
static int au_get_lr_value(struct audio_softc *, mixer_ctrl_t *, int *, int *);
static int au_set_gain(struct audio_softc *, struct au_mixer_ports *, int, int);
static void au_get_gain(struct audio_softc *, struct au_mixer_ports *,
	u_int *, u_char *);
static int au_set_port(struct audio_softc *, struct au_mixer_ports *, u_int);
static int au_get_port(struct audio_softc *, struct au_mixer_ports *);
static int au_set_monitor_gain(struct audio_softc *, int);
static int au_get_monitor_gain(struct audio_softc *);
static int audio_get_port(struct audio_softc *, mixer_ctrl_t *);
static int audio_set_port(struct audio_softc *, mixer_ctrl_t *);

static __inline struct audio_params
format2_to_params(const audio_format2_t *f2)
{
	audio_params_t p;

	/* validbits/precision <-> precision/stride */
	p.sample_rate = f2->sample_rate;
	p.channels    = f2->channels;
	p.encoding    = f2->encoding;
	p.validbits   = f2->precision;
	p.precision   = f2->stride;
	return p;
}

static __inline audio_format2_t
params_to_format2(const struct audio_params *p)
{
	audio_format2_t f2;

	/* precision/stride <-> validbits/precision */
	f2.sample_rate = p->sample_rate;
	f2.channels    = p->channels;
	f2.encoding    = p->encoding;
	f2.precision   = p->validbits;
	f2.stride      = p->precision;
	return f2;
}

/* Return true if this track is a playback track. */
static __inline bool
audio_track_is_playback(const audio_track_t *track)
{

	return ((track->mode & AUMODE_PLAY) != 0);
}

/* Return true if this track is a recording track. */
static __inline bool
audio_track_is_record(const audio_track_t *track)
{

	return ((track->mode & AUMODE_RECORD) != 0);
}

#if 0 /* XXX Not used yet */
/*
 * Convert 0..255 volume used in userland to internal presentation 0..256.
 */
static __inline u_int
audio_volume_to_inner(u_int v)
{

	return v < 127 ? v : v + 1;
}

/*
 * Convert 0..256 internal presentation to 0..255 volume used in userland.
 */
static __inline u_int
audio_volume_to_outer(u_int v)
{

	return v < 127 ? v : v - 1;
}
#endif /* 0 */

static dev_type_open(audioopen);
/* XXXMRG use more dev_type_xxx */

static int
audiounit(dev_t dev)
{

	return AUDIOUNIT(dev);
}

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
	.d_cfdriver = &audio_cd,
	.d_devtounit = audiounit,
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
	.fo_mmap = audiommap,
	.fo_kqfilter = audiokqfilter,
	.fo_restart = fnullop_restart
};

/* The default audio mode: 8 kHz mono mu-law */
static const struct audio_params audio_default = {
	.sample_rate = 8000,
	.encoding = AUDIO_ENCODING_ULAW,
	.precision = 8,
	.validbits = 8,
	.channels = 1,
};

static const char *encoding_names[] = {
	"none",
	AudioEmulaw,
	AudioEalaw,
	"pcm16",
	"pcm8",
	AudioEadpcm,
	AudioEslinear_le,
	AudioEslinear_be,
	AudioEulinear_le,
	AudioEulinear_be,
	AudioEslinear,
	AudioEulinear,
	AudioEmpeg_l1_stream,
	AudioEmpeg_l1_packets,
	AudioEmpeg_l1_system,
	AudioEmpeg_l2_stream,
	AudioEmpeg_l2_packets,
	AudioEmpeg_l2_system,
	AudioEac3,
};

/*
 * Returns encoding name corresponding to AUDIO_ENCODING_*.
 * Note that it may return a local buffer because it is mainly for debugging.
 */
const char *
audio_encoding_name(int encoding)
{
	static char buf[16];

	if (0 <= encoding && encoding < __arraycount(encoding_names)) {
		return encoding_names[encoding];
	} else {
		snprintf(buf, sizeof(buf), "enc=%d", encoding);
		return buf;
	}
}

/*
 * Supported encodings used by AUDIO_GETENC.
 * index and flags are set by code.
 * XXX is there any needs for SLINEAR_OE:>=16/ULINEAR_OE:>=16 ?
 */
static const audio_encoding_t audio_encodings[] = {
	{ 0, AudioEmulaw,	AUDIO_ENCODING_ULAW,		8,  0 },
	{ 0, AudioEalaw,	AUDIO_ENCODING_ALAW,		8,  0 },
	{ 0, AudioEslinear,	AUDIO_ENCODING_SLINEAR,		8,  0 },
	{ 0, AudioEulinear,	AUDIO_ENCODING_ULINEAR,		8,  0 },
	{ 0, AudioEslinear_le,	AUDIO_ENCODING_SLINEAR_LE,	16, 0 },
	{ 0, AudioEulinear_le,	AUDIO_ENCODING_ULINEAR_LE,	16, 0 },
	{ 0, AudioEslinear_be,	AUDIO_ENCODING_SLINEAR_BE,	16, 0 },
	{ 0, AudioEulinear_be,	AUDIO_ENCODING_ULINEAR_BE,	16, 0 },
#if defined(AUDIO_SUPPORT_LINEAR24)
	{ 0, AudioEslinear_le,	AUDIO_ENCODING_SLINEAR_LE,	24, 0 },
	{ 0, AudioEulinear_le,	AUDIO_ENCODING_ULINEAR_LE,	24, 0 },
	{ 0, AudioEslinear_be,	AUDIO_ENCODING_SLINEAR_BE,	24, 0 },
	{ 0, AudioEulinear_be,	AUDIO_ENCODING_ULINEAR_BE,	24, 0 },
#endif
	{ 0, AudioEslinear_le,	AUDIO_ENCODING_SLINEAR_LE,	32, 0 },
	{ 0, AudioEulinear_le,	AUDIO_ENCODING_ULINEAR_LE,	32, 0 },
	{ 0, AudioEslinear_be,	AUDIO_ENCODING_SLINEAR_BE,	32, 0 },
	{ 0, AudioEulinear_be,	AUDIO_ENCODING_ULINEAR_BE,	32, 0 },
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

static struct psref_class *audio_psref_class __read_mostly;

CFATTACH_DECL3_NEW(audio, sizeof(struct audio_softc),
    audiomatch, audioattach, audiodetach, audioactivate, audiorescan,
    audiochilddet, DVF_DETACH_SHUTDOWN);

static int
audiomatch(device_t parent, cfdata_t match, void *aux)
{
	struct audio_attach_args *sa;

	sa = aux;
	DPRINTF(1, "%s: type=%d sa=%p hw=%p\n",
	     __func__, sa->type, sa, sa->hwif);
	return (sa->type == AUDIODEV_TYPE_AUDIO) ? 1 : 0;
}

static void
audioattach(device_t parent, device_t self, void *aux)
{
	struct audio_softc *sc;
	struct audio_attach_args *sa;
	const struct audio_hw_if *hw_if;
	audio_format2_t phwfmt;
	audio_format2_t rhwfmt;
	audio_filter_reg_t pfil;
	audio_filter_reg_t rfil;
	const struct sysctlnode *node;
	void *hdlp;
	bool has_playback;
	bool has_capture;
	bool has_indep;
	bool has_fulldup;
	int mode;
	int error;

	sc = device_private(self);
	sc->sc_dev = self;
	sa = (struct audio_attach_args *)aux;
	hw_if = sa->hwif;
	hdlp = sa->hdl;

	if (hw_if == NULL) {
		panic("audioattach: missing hw_if method");
	}
	if (hw_if->get_locks == NULL || hw_if->get_props == NULL) {
		aprint_error(": missing mandatory method\n");
		return;
	}

	hw_if->get_locks(hdlp, &sc->sc_intr_lock, &sc->sc_lock);
	sc->sc_props = hw_if->get_props(hdlp);

	has_playback = (sc->sc_props & AUDIO_PROP_PLAYBACK);
	has_capture  = (sc->sc_props & AUDIO_PROP_CAPTURE);
	has_indep    = (sc->sc_props & AUDIO_PROP_INDEPENDENT);
	has_fulldup  = (sc->sc_props & AUDIO_PROP_FULLDUPLEX);

#ifdef DIAGNOSTIC
	if (hw_if->query_format == NULL ||
	    hw_if->set_format == NULL ||
	    hw_if->getdev == NULL ||
	    hw_if->set_port == NULL ||
	    hw_if->get_port == NULL ||
	    hw_if->query_devinfo == NULL) {
		aprint_error(": missing mandatory method\n");
		return;
	}
	if (has_playback) {
		if ((hw_if->start_output == NULL &&
		     hw_if->trigger_output == NULL) ||
		    hw_if->halt_output == NULL) {
			aprint_error(": missing playback method\n");
		}
	}
	if (has_capture) {
		if ((hw_if->start_input == NULL &&
		     hw_if->trigger_input == NULL) ||
		    hw_if->halt_input == NULL) {
			aprint_error(": missing capture method\n");
		}
	}
#endif

	sc->hw_if = hw_if;
	sc->hw_hdl = hdlp;
	sc->hw_dev = parent;

	sc->sc_exlock = 1;
	sc->sc_blk_ms = AUDIO_BLK_MS;
	SLIST_INIT(&sc->sc_files);
	cv_init(&sc->sc_exlockcv, "audiolk");
	sc->sc_am_capacity = 0;
	sc->sc_am_used = 0;
	sc->sc_am = NULL;

	/* MMAP is now supported by upper layer.  */
	sc->sc_props |= AUDIO_PROP_MMAP;

	KASSERT(has_playback || has_capture);
	/* Unidirectional device must have neither FULLDUP nor INDEPENDENT. */
	if (!has_playback || !has_capture) {
		KASSERT(!has_indep);
		KASSERT(!has_fulldup);
	}

	mode = 0;
	if (has_playback) {
		aprint_normal(": playback");
		mode |= AUMODE_PLAY;
	}
	if (has_capture) {
		aprint_normal("%c capture", has_playback ? ',' : ':');
		mode |= AUMODE_RECORD;
	}
	if (has_playback && has_capture) {
		if (has_fulldup)
			aprint_normal(", full duplex");
		else
			aprint_normal(", half duplex");

		if (has_indep)
			aprint_normal(", independent");
	}

	aprint_naive("\n");
	aprint_normal("\n");

	/* probe hw params */
	memset(&phwfmt, 0, sizeof(phwfmt));
	memset(&rhwfmt, 0, sizeof(rhwfmt));
	memset(&pfil, 0, sizeof(pfil));
	memset(&rfil, 0, sizeof(rfil));
	if (has_indep) {
		int perror, rerror;

		/* On independent devices, probe separately. */
		perror = audio_hw_probe(sc, &phwfmt, AUMODE_PLAY);
		rerror = audio_hw_probe(sc, &rhwfmt, AUMODE_RECORD);
		if (perror && rerror) {
			aprint_error_dev(self,
			    "audio_hw_probe failed: perror=%d, rerror=%d\n",
			    perror, rerror);
			goto bad;
		}
		if (perror) {
			mode &= ~AUMODE_PLAY;
			aprint_error_dev(self, "audio_hw_probe failed: "
			    "errno=%d, playback disabled\n", perror);
		}
		if (rerror) {
			mode &= ~AUMODE_RECORD;
			aprint_error_dev(self, "audio_hw_probe failed: "
			    "errno=%d, capture disabled\n", rerror);
		}
	} else {
		/*
		 * On non independent devices or uni-directional devices,
		 * probe once (simultaneously).
		 */
		audio_format2_t *fmt = has_playback ? &phwfmt : &rhwfmt;
		error = audio_hw_probe(sc, fmt, mode);
		if (error) {
			aprint_error_dev(self,
			    "audio_hw_probe failed: errno=%d\n", error);
			goto bad;
		}
		if (has_playback && has_capture)
			rhwfmt = phwfmt;
	}

	/* Init hardware. */
	/* hw_probe() also validates [pr]hwfmt.  */
	error = audio_hw_set_format(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
	if (error) {
		aprint_error_dev(self,
		    "audio_hw_set_format failed: errno=%d\n", error);
		goto bad;
	}

	/*
	 * Init track mixers.  If at least one direction is available on
	 * attach time, we assume a success.
	 */
	error = audio_mixers_init(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
	if (sc->sc_pmixer == NULL && sc->sc_rmixer == NULL) {
		aprint_error_dev(self,
		    "audio_mixers_init failed: errno=%d\n", error);
		goto bad;
	}

	sc->sc_psz = pserialize_create();
	psref_target_init(&sc->sc_psref, audio_psref_class);

	selinit(&sc->sc_wsel);
	selinit(&sc->sc_rsel);

	/* Initial parameter of /dev/sound */
	sc->sc_sound_pparams = params_to_format2(&audio_default);
	sc->sc_sound_rparams = params_to_format2(&audio_default);
	sc->sc_sound_ppause = false;
	sc->sc_sound_rpause = false;

	/* XXX TODO: consider about sc_ai */

	mixer_init(sc);
	TRACE(2, "inputs ports=0x%x, input master=%d, "
	    "output ports=0x%x, output master=%d",
	    sc->sc_inports.allports, sc->sc_inports.master,
	    sc->sc_outports.allports, sc->sc_outports.master);

	sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    0,
	    CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("audio test"),
	    NULL, 0,
	    NULL, 0,
	    CTL_HW,
	    CTL_CREATE, CTL_EOL);

	if (node != NULL) {
		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
		    CTLFLAG_READWRITE,
		    CTLTYPE_INT, "blk_ms",
		    SYSCTL_DESCR("blocksize in msec"),
		    audio_sysctl_blk_ms, 0, (void *)sc, 0,
		    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);

		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
		    CTLFLAG_READWRITE,
		    CTLTYPE_BOOL, "multiuser",
		    SYSCTL_DESCR("allow multiple user access"),
		    audio_sysctl_multiuser, 0, (void *)sc, 0,
		    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);

#if defined(AUDIO_DEBUG)
		sysctl_createv(&sc->sc_log, 0, NULL, NULL,
		    CTLFLAG_READWRITE,
		    CTLTYPE_INT, "debug",
		    SYSCTL_DESCR("debug level (0..4)"),
		    audio_sysctl_debug, 0, (void *)sc, 0,
		    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);
#endif
	}

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

#if defined(AUDIO_DEBUG)
	audio_mlog_init();
#endif

	audiorescan(self, NULL, NULL);
	sc->sc_exlock = 0;
	return;

bad:
	/* Clearing hw_if means that device is attached but disabled. */
	sc->hw_if = NULL;
	sc->sc_exlock = 0;
	aprint_error_dev(sc->sc_dev, "disabled\n");
	return;
}

/*
 * Initialize hardware mixer.
 * This function is called from audioattach().
 */
static void
mixer_init(struct audio_softc *sc)
{
	mixer_devinfo_t mi;
	int iclass, mclass, oclass, rclass;
	int record_master_found, record_source_found;

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
	sc->sc_nmixer_states = mi.index;
	sc->sc_mixer_state = kmem_zalloc(sizeof(sc->sc_mixer_state[0]) *
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
}

static int
audioactivate(device_t self, enum devact act)
{
	struct audio_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		mutex_enter(sc->sc_lock);
		sc->sc_dying = true;
		cv_broadcast(&sc->sc_exlockcv);
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
	struct audio_file *file;
	int error;

	sc = device_private(self);
	TRACE(2, "flags=%d", flags);

	/* device is not initialized */
	if (sc->hw_if == NULL)
		return 0;

	/* Start draining existing accessors of the device. */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	/*
	 * This waits currently running sysctls to finish if exists.
	 * After this, no more new sysctls will come.
	 */
	sysctl_teardown(&sc->sc_log);

	mutex_enter(sc->sc_lock);
	sc->sc_dying = true;
	cv_broadcast(&sc->sc_exlockcv);
	if (sc->sc_pmixer)
		cv_broadcast(&sc->sc_pmixer->outcv);
	if (sc->sc_rmixer)
		cv_broadcast(&sc->sc_rmixer->outcv);

	/* Prevent new users */
	SLIST_FOREACH(file, &sc->sc_files, entry) {
		atomic_store_relaxed(&file->dying, true);
	}
	mutex_exit(sc->sc_lock);

	/*
	 * Wait for existing users to drain.
	 * - pserialize_perform waits for all pserialize_read sections on
	 *   all CPUs; after this, no more new psref_acquire can happen.
	 * - psref_target_destroy waits for all extant acquired psrefs to
	 *   be psref_released.
	 */
	pserialize_perform(sc->sc_psz);
	psref_target_destroy(&sc->sc_psref, audio_psref_class);

	/*
	 * We are now guaranteed that there are no calls to audio fileops
	 * that hold sc, and any new calls with files that were for sc will
	 * fail.  Thus, we now have exclusive access to the softc.
	 */
	sc->sc_exlock = 1;

	/*
	 * Clean up all open instances.
	 */
	mutex_enter(sc->sc_lock);
	while ((file = SLIST_FIRST(&sc->sc_files)) != NULL) {
		mutex_enter(sc->sc_intr_lock);
		SLIST_REMOVE_HEAD(&sc->sc_files, entry);
		mutex_exit(sc->sc_intr_lock);
		if (file->ptrack || file->rtrack) {
			mutex_exit(sc->sc_lock);
			audio_unlink(sc, file);
			mutex_enter(sc->sc_lock);
		}
	}
	mutex_exit(sc->sc_lock);

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

	/* Free resources */
	if (sc->sc_pmixer) {
		audio_mixer_destroy(sc, sc->sc_pmixer);
		kmem_free(sc->sc_pmixer, sizeof(*sc->sc_pmixer));
	}
	if (sc->sc_rmixer) {
		audio_mixer_destroy(sc, sc->sc_rmixer);
		kmem_free(sc->sc_rmixer, sizeof(*sc->sc_rmixer));
	}
	if (sc->sc_am)
		kern_free(sc->sc_am);

	seldestroy(&sc->sc_wsel);
	seldestroy(&sc->sc_rsel);

#ifdef AUDIO_PM_IDLE
	callout_destroy(&sc->sc_idle_counter);
#endif

	cv_destroy(&sc->sc_exlockcv);

#if defined(AUDIO_DEBUG)
	audio_mlog_free();
#endif

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

	if (config_probe(parent, cf, aux))
		config_attach(parent, cf, aux, NULL,
		    CFARGS_NONE);

	return 0;
}

static int
audiorescan(device_t self, const char *ifattr, const int *locators)
{
	struct audio_softc *sc = device_private(self);

	config_search(sc->sc_dev, NULL,
	    CFARGS(.search = audiosearch));

	return 0;
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
	return config_found(dev, &arg, audioprint,
	    CFARGS(.iattr = "audiobus"));
}

/*
 * audio_printf() outputs fmt... with the audio device name and MD device
 * name prefixed.  If the message is considered to be related to the MD
 * driver, use this one instead of device_printf().
 */
static void
audio_printf(struct audio_softc *sc, const char *fmt, ...)
{
	va_list ap;

	printf("%s(%s): ", device_xname(sc->sc_dev), device_xname(sc->hw_dev));
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/*
 * Enter critical section and also keep sc_lock.
 * If successful, returns 0 with sc_lock held.  Otherwise returns errno.
 * Must be called without sc_lock held.
 */
static int
audio_exlock_mutex_enter(struct audio_softc *sc)
{
	int error;

	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		return EIO;
	}

	while (__predict_false(sc->sc_exlock != 0)) {
		error = cv_wait_sig(&sc->sc_exlockcv, sc->sc_lock);
		if (sc->sc_dying)
			error = EIO;
		if (error) {
			mutex_exit(sc->sc_lock);
			return error;
		}
	}

	/* Acquire */
	sc->sc_exlock = 1;
	return 0;
}

/*
 * Exit critical section and exit sc_lock.
 * Must be called with sc_lock held.
 */
static void
audio_exlock_mutex_exit(struct audio_softc *sc)
{

	KASSERT(mutex_owned(sc->sc_lock));

	sc->sc_exlock = 0;
	cv_broadcast(&sc->sc_exlockcv);
	mutex_exit(sc->sc_lock);
}

/*
 * Enter critical section.
 * If successful, it returns 0.  Otherwise returns errno.
 * Must be called without sc_lock held.
 * This function returns without sc_lock held.
 */
static int
audio_exlock_enter(struct audio_softc *sc)
{
	int error;

	error = audio_exlock_mutex_enter(sc);
	if (error)
		return error;
	mutex_exit(sc->sc_lock);
	return 0;
}

/*
 * Exit critical section.
 * Must be called without sc_lock held.
 */
static void
audio_exlock_exit(struct audio_softc *sc)
{

	mutex_enter(sc->sc_lock);
	audio_exlock_mutex_exit(sc);
}

/*
 * Get sc from file, and increment reference counter for this sc.
 * This is intended to be used for methods other than open.
 * If successful, returns sc.  Otherwise returns NULL.
 */
struct audio_softc *
audio_sc_acquire_fromfile(audio_file_t *file, struct psref *refp)
{
	int s;
	bool dying;

	/* Block audiodetach while we acquire a reference */
	s = pserialize_read_enter();

	/* If close or audiodetach already ran, tough -- no more audio */
	dying = atomic_load_relaxed(&file->dying);
	if (dying) {
		pserialize_read_exit(s);
		return NULL;
	}

	/* Acquire a reference */
	psref_acquire(refp, &file->sc->sc_psref, audio_psref_class);

	/* Now sc won't go away until we drop the reference count */
	pserialize_read_exit(s);

	return file->sc;
}

/*
 * Decrement reference counter for this sc.
 */
void
audio_sc_release(struct audio_softc *sc, struct psref *refp)
{

	psref_release(refp, &sc->sc_psref, audio_psref_class);
}

/*
 * Wait for I/O to complete, releasing sc_lock.
 * Must be called with sc_lock held.
 */
static int
audio_track_waitio(struct audio_softc *sc, audio_track_t *track)
{
	int error;

	KASSERT(track);
	KASSERT(mutex_owned(sc->sc_lock));

	/* Wait for pending I/O to complete. */
	error = cv_timedwait_sig(&track->mixer->outcv, sc->sc_lock,
	    mstohz(AUDIO_TIMEOUT));
	if (sc->sc_suspending) {
		/* If it's about to suspend, ignore timeout error. */
		if (error == EWOULDBLOCK) {
			TRACET(2, track, "timeout (suspending)");
			return 0;
		}
	}
	if (sc->sc_dying) {
		error = EIO;
	}
	if (error) {
		TRACET(2, track, "cv_timedwait_sig failed %d", error);
		if (error == EWOULDBLOCK)
			audio_printf(sc, "device timeout\n");
	} else {
		TRACET(3, track, "wakeup");
	}
	return error;
}

/*
 * Try to acquire track lock.
 * It doesn't block if the track lock is already acquired.
 * Returns true if the track lock was acquired, or false if the track
 * lock was already acquired.
 */
static __inline bool
audio_track_lock_tryenter(audio_track_t *track)
{

	if (atomic_swap_uint(&track->lock, 1) != 0)
		return false;
	membar_enter();
	return true;
}

/*
 * Acquire track lock.
 */
static __inline void
audio_track_lock_enter(audio_track_t *track)
{

	/* Don't sleep here. */
	while (audio_track_lock_tryenter(track) == false)
		SPINLOCK_BACKOFF_HOOK;
}

/*
 * Release track lock.
 */
static __inline void
audio_track_lock_exit(audio_track_t *track)
{

	atomic_store_release(&track->lock, 0);
}


static int
audioopen(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct audio_softc *sc;
	int error;

	/*
	 * Find the device.  Because we wired the cdevsw to the audio
	 * autoconf instance, the system ensures it will not go away
	 * until after we return.
	 */
	sc = device_lookup_private(&audio_cd, AUDIOUNIT(dev));
	if (sc == NULL || sc->hw_if == NULL)
		return ENXIO;

	error = audio_exlock_enter(sc);
	if (error)
		return error;

	device_active(sc->sc_dev, DVA_SYSTEM);
	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_open(dev, sc, flags, ifmt, l, NULL);
		break;
	case AUDIOCTL_DEVICE:
		error = audioctl_open(dev, sc, flags, ifmt, l);
		break;
	case MIXER_DEVICE:
		error = mixer_open(dev, sc, flags, ifmt, l);
		break;
	default:
		error = ENXIO;
		break;
	}
	audio_exlock_exit(sc);

	return error;
}

static int
audioclose(struct file *fp)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	int bound;
	int error;
	dev_t dev;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;
	error = 0;

	/*
	 * audioclose() must
	 * - unplug track from the trackmixer (and unplug anything from softc),
	 *   if sc exists.
	 * - free all memory objects, regardless of sc.
	 */

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc) {
		switch (AUDIODEV(dev)) {
		case SOUND_DEVICE:
		case AUDIO_DEVICE:
			error = audio_close(sc, file);
			break;
		case AUDIOCTL_DEVICE:
			mutex_enter(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
			SLIST_REMOVE(&sc->sc_files, file, audio_file, entry);
			mutex_exit(sc->sc_intr_lock);
			mutex_exit(sc->sc_lock);
			error = 0;
			break;
		case MIXER_DEVICE:
			mutex_enter(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
			SLIST_REMOVE(&sc->sc_files, file, audio_file, entry);
			mutex_exit(sc->sc_intr_lock);
			mutex_exit(sc->sc_lock);
			error = mixer_close(sc, file);
			break;
		default:
			error = ENXIO;
			break;
		}

		audio_sc_release(sc, &sc_ref);
	}
	curlwp_bindx(bound);

	/* Free memory objects anyway */
	TRACEF(2, file, "free memory");
	if (file->ptrack)
		audio_track_destroy(file->ptrack);
	if (file->rtrack)
		audio_track_destroy(file->rtrack);
	kmem_free(file, sizeof(*file));
	fp->f_audioctx = NULL;

	return error;
}

static int
audioread(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	int ioflag)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	int bound;
	int error;
	dev_t dev;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	if (fp->f_flag & O_NONBLOCK)
		ioflag |= IO_NDELAY;

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_read(sc, uio, ioflag, file);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}

static int
audiowrite(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	int ioflag)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	int bound;
	int error;
	dev_t dev;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	if (fp->f_flag & O_NONBLOCK)
		ioflag |= IO_NDELAY;

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_write(sc, uio, ioflag, file);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}

static int
audioioctl(struct file *fp, u_long cmd, void *addr)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	struct lwp *l = curlwp;
	int bound;
	int error;
	dev_t dev;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
	case AUDIOCTL_DEVICE:
		mutex_enter(sc->sc_lock);
		device_active(sc->sc_dev, DVA_SYSTEM);
		mutex_exit(sc->sc_lock);
		if (IOCGROUP(cmd) == IOCGROUP(AUDIO_MIXER_READ))
			error = mixer_ioctl(sc, cmd, addr, fp->f_flag, l);
		else
			error = audio_ioctl(dev, sc, cmd, addr, fp->f_flag, l,
			    file);
		break;
	case MIXER_DEVICE:
		error = mixer_ioctl(sc, cmd, addr, fp->f_flag, l);
		break;
	default:
		error = ENXIO;
		break;
	}

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}

static int
audiostat(struct file *fp, struct stat *st)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	int bound;
	int error;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	error = 0;
	memset(st, 0, sizeof(*st));

	st->st_dev = file->dev;
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}

static int
audiopoll(struct file *fp, int events)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	struct lwp *l = curlwp;
	int bound;
	int revents;
	dev_t dev;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		revents = POLLERR;
		goto done;
	}

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		revents = audio_poll(sc, events, l, file);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		revents = 0;
		break;
	default:
		revents = POLLERR;
		break;
	}

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return revents;
}

static int
audiokqfilter(struct file *fp, struct knote *kn)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	dev_t dev;
	int bound;
	int error;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_kqfilter(sc, file, kn);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
		error = ENODEV;
		break;
	default:
		error = ENXIO;
		break;
	}

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}

static int
audiommap(struct file *fp, off_t *offp, size_t len, int prot, int *flagsp,
	int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	audio_file_t *file;
	dev_t dev;
	int bound;
	int error;

	KASSERT(fp->f_audioctx);
	file = fp->f_audioctx;
	dev = file->dev;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	mutex_enter(sc->sc_lock);
	device_active(sc->sc_dev, DVA_SYSTEM); /* XXXJDM */
	mutex_exit(sc->sc_lock);

	switch (AUDIODEV(dev)) {
	case SOUND_DEVICE:
	case AUDIO_DEVICE:
		error = audio_mmap(sc, offp, len, prot, flagsp, advicep,
		    uobjp, maxprotp, file);
		break;
	case AUDIOCTL_DEVICE:
	case MIXER_DEVICE:
	default:
		error = ENOTSUP;
		break;
	}

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}


/* Exported interfaces for audiobell. */

/*
 * Open for audiobell.
 * It stores allocated file to *filep.
 * If successful returns 0, otherwise errno.
 */
int
audiobellopen(dev_t dev, audio_file_t **filep)
{
	device_t audiodev = NULL;
	struct audio_softc *sc;
	bool exlock = false;
	int error;

	/*
	 * Find the autoconf instance and make sure it doesn't go away
	 * while we are opening it.
	 */
	audiodev = device_lookup_acquire(&audio_cd, AUDIOUNIT(dev));
	if (audiodev == NULL) {
		error = ENXIO;
		goto out;
	}

	/* If attach failed, it's hopeless -- give up.  */
	sc = device_private(audiodev);
	if (sc->hw_if == NULL) {
		error = ENXIO;
		goto out;
	}

	/* Take the exclusive configuration lock.  */
	error = audio_exlock_enter(sc);
	if (error)
		goto out;
	exlock = true;

	/* Open the audio device.  */
	device_active(sc->sc_dev, DVA_SYSTEM);
	error = audio_open(dev, sc, FWRITE, 0, curlwp, filep);

out:	if (exlock)
		audio_exlock_exit(sc);
	if (audiodev)
		device_release(audiodev);
	return error;
}

/* Close for audiobell */
int
audiobellclose(audio_file_t *file)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	int bound;
	int error;

	error = 0;
	/*
	 * audiobellclose() must
	 * - unplug track from the trackmixer if sc exist.
	 * - free all memory objects, regardless of sc.
	 */
	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc) {
		error = audio_close(sc, file);
		audio_sc_release(sc, &sc_ref);
	}
	curlwp_bindx(bound);

	/* Free memory objects anyway */
	KASSERT(file->ptrack);
	audio_track_destroy(file->ptrack);
	KASSERT(file->rtrack == NULL);
	kmem_free(file, sizeof(*file));
	return error;
}

/* Set sample rate for audiobell */
int
audiobellsetrate(audio_file_t *file, u_int sample_rate)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	struct audio_info ai;
	int bound;
	int error;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done1;
	}

	AUDIO_INITINFO(&ai);
	ai.play.sample_rate = sample_rate;

	error = audio_exlock_enter(sc);
	if (error)
		goto done2;
	error = audio_file_setinfo(sc, file, &ai);
	audio_exlock_exit(sc);

done2:
	audio_sc_release(sc, &sc_ref);
done1:
	curlwp_bindx(bound);
	return error;
}

/* Playback for audiobell */
int
audiobellwrite(audio_file_t *file, struct uio *uio)
{
	struct audio_softc *sc;
	struct psref sc_ref;
	int bound;
	int error;

	bound = curlwp_bind();
	sc = audio_sc_acquire_fromfile(file, &sc_ref);
	if (sc == NULL) {
		error = EIO;
		goto done;
	}

	error = audio_write(sc, uio, 0, file);

	audio_sc_release(sc, &sc_ref);
done:
	curlwp_bindx(bound);
	return error;
}


/*
 * Audio driver
 */

/*
 * Must be called with sc_exlock held and without sc_lock held.
 */
int
audio_open(dev_t dev, struct audio_softc *sc, int flags, int ifmt,
	struct lwp *l, audio_file_t **bellfile)
{
	struct audio_info ai;
	struct file *fp;
	audio_file_t *af;
	audio_ring_t *hwbuf;
	bool fullduplex;
	bool cred_held;
	bool hw_opened;
	bool rmixer_started;
	bool inserted;
	int fd;
	int error;

	KASSERT(sc->sc_exlock);

	TRACE(1, "%sdev=%s flags=0x%x po=%d ro=%d",
	    (audiodebug >= 3) ? "start " : "",
	    ISDEVSOUND(dev) ? "sound" : "audio",
	    flags, sc->sc_popens, sc->sc_ropens);

	fp = NULL;
	cred_held = false;
	hw_opened = false;
	rmixer_started = false;
	inserted = false;

	af = kmem_zalloc(sizeof(*af), KM_SLEEP);
	af->sc = sc;
	af->dev = dev;
	if ((flags & FWRITE) != 0 && audio_can_playback(sc))
		af->mode |= AUMODE_PLAY | AUMODE_PLAY_ALL;
	if ((flags & FREAD) != 0 && audio_can_capture(sc))
		af->mode |= AUMODE_RECORD;
	if (af->mode == 0) {
		error = ENXIO;
		goto bad;
	}

	fullduplex = (sc->sc_props & AUDIO_PROP_FULLDUPLEX);

	/*
	 * On half duplex hardware,
	 * 1. if mode is (PLAY | REC), let mode PLAY.
	 * 2. if mode is PLAY, let mode PLAY if no rec tracks, otherwise error.
	 * 3. if mode is REC, let mode REC if no play tracks, otherwise error.
	 */
	if (fullduplex == false) {
		if ((af->mode & AUMODE_PLAY)) {
			if (sc->sc_ropens != 0) {
				TRACE(1, "record track already exists");
				error = ENODEV;
				goto bad;
			}
			/* Play takes precedence */
			af->mode &= ~AUMODE_RECORD;
		}
		if ((af->mode & AUMODE_RECORD)) {
			if (sc->sc_popens != 0) {
				TRACE(1, "play track already exists");
				error = ENODEV;
				goto bad;
			}
		}
	}

	/* Create tracks */
	if ((af->mode & AUMODE_PLAY))
		af->ptrack = audio_track_create(sc, sc->sc_pmixer);
	if ((af->mode & AUMODE_RECORD))
		af->rtrack = audio_track_create(sc, sc->sc_rmixer);

	/* Set parameters */
	AUDIO_INITINFO(&ai);
	if (bellfile) {
		/* If audiobell, only sample_rate will be set later. */
		ai.play.sample_rate   = audio_default.sample_rate;
		ai.play.encoding      = AUDIO_ENCODING_SLINEAR_NE;
		ai.play.channels      = 1;
		ai.play.precision     = 16;
		ai.play.pause         = 0;
	} else if (ISDEVAUDIO(dev)) {
		/* If /dev/audio, initialize everytime. */
		ai.play.sample_rate   = audio_default.sample_rate;
		ai.play.encoding      = audio_default.encoding;
		ai.play.channels      = audio_default.channels;
		ai.play.precision     = audio_default.precision;
		ai.play.pause         = 0;
		ai.record.sample_rate = audio_default.sample_rate;
		ai.record.encoding    = audio_default.encoding;
		ai.record.channels    = audio_default.channels;
		ai.record.precision   = audio_default.precision;
		ai.record.pause       = 0;
	} else {
		/* If /dev/sound, take over the previous parameters. */
		ai.play.sample_rate   = sc->sc_sound_pparams.sample_rate;
		ai.play.encoding      = sc->sc_sound_pparams.encoding;
		ai.play.channels      = sc->sc_sound_pparams.channels;
		ai.play.precision     = sc->sc_sound_pparams.precision;
		ai.play.pause         = sc->sc_sound_ppause;
		ai.record.sample_rate = sc->sc_sound_rparams.sample_rate;
		ai.record.encoding    = sc->sc_sound_rparams.encoding;
		ai.record.channels    = sc->sc_sound_rparams.channels;
		ai.record.precision   = sc->sc_sound_rparams.precision;
		ai.record.pause       = sc->sc_sound_rpause;
	}
	error = audio_file_setinfo(sc, af, &ai);
	if (error)
		goto bad;

	if (sc->sc_popens + sc->sc_ropens == 0) {
		/* First open */

		sc->sc_cred = kauth_cred_get();
		kauth_cred_hold(sc->sc_cred);
		cred_held = true;

		if (sc->hw_if->open) {
			int hwflags;

			/*
			 * Call hw_if->open() only at first open of
			 * combination of playback and recording.
			 * On full duplex hardware, the flags passed to
			 * hw_if->open() is always (FREAD | FWRITE)
			 * regardless of this open()'s flags.
			 * see also dev/isa/aria.c
			 * On half duplex hardware, the flags passed to
			 * hw_if->open() is either FREAD or FWRITE.
			 * see also arch/evbarm/mini2440/audio_mini2440.c
			 */
			if (fullduplex) {
				hwflags = FREAD | FWRITE;
			} else {
				/* Construct hwflags from af->mode. */
				hwflags = 0;
				if ((af->mode & AUMODE_PLAY) != 0)
					hwflags |= FWRITE;
				if ((af->mode & AUMODE_RECORD) != 0)
					hwflags |= FREAD;
			}

			mutex_enter(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
			error = sc->hw_if->open(sc->hw_hdl, hwflags);
			mutex_exit(sc->sc_intr_lock);
			mutex_exit(sc->sc_lock);
			if (error)
				goto bad;
		}
		/*
		 * Regardless of whether we called hw_if->open (whether
		 * hw_if->open exists) or not, we move to the Opened phase
		 * here.  Therefore from this point, we have to call
		 * hw_if->close (if exists) whenever abort.
		 * Note that both of hw_if->{open,close} are optional.
		 */
		hw_opened = true;

		/*
		 * Set speaker mode when a half duplex.
		 * XXX I'm not sure this is correct.
		 */
		if (1/*XXX*/) {
			if (sc->hw_if->speaker_ctl) {
				int on;
				if (af->ptrack) {
					on = 1;
				} else {
					on = 0;
				}
				mutex_enter(sc->sc_lock);
				mutex_enter(sc->sc_intr_lock);
				error = sc->hw_if->speaker_ctl(sc->hw_hdl, on);
				mutex_exit(sc->sc_intr_lock);
				mutex_exit(sc->sc_lock);
				if (error)
					goto bad;
			}
		}
	} else if (sc->sc_multiuser == false) {
		uid_t euid = kauth_cred_geteuid(kauth_cred_get());
		if (euid != 0 && euid != kauth_cred_geteuid(sc->sc_cred)) {
			error = EPERM;
			goto bad;
		}
	}

	/* Call init_output if this is the first playback open. */
	if (af->ptrack && sc->sc_popens == 0) {
		if (sc->hw_if->init_output) {
			hwbuf = &sc->sc_pmixer->hwbuf;
			mutex_enter(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
			error = sc->hw_if->init_output(sc->hw_hdl,
			    hwbuf->mem,
			    hwbuf->capacity *
			    hwbuf->fmt.channels * hwbuf->fmt.stride / NBBY);
			mutex_exit(sc->sc_intr_lock);
			mutex_exit(sc->sc_lock);
			if (error)
				goto bad;
		}
	}
	/*
	 * Call init_input and start rmixer, if this is the first recording
	 * open.  See pause consideration notes.
	 */
	if (af->rtrack && sc->sc_ropens == 0) {
		if (sc->hw_if->init_input) {
			hwbuf = &sc->sc_rmixer->hwbuf;
			mutex_enter(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
			error = sc->hw_if->init_input(sc->hw_hdl,
			    hwbuf->mem,
			    hwbuf->capacity *
			    hwbuf->fmt.channels * hwbuf->fmt.stride / NBBY);
			mutex_exit(sc->sc_intr_lock);
			mutex_exit(sc->sc_lock);
			if (error)
				goto bad;
		}

		mutex_enter(sc->sc_lock);
		audio_rmixer_start(sc);
		mutex_exit(sc->sc_lock);
		rmixer_started = true;
	}

	/*
	 * This is the last sc_lock section in the function, so we have to
	 * examine sc_dying again before starting the rest tasks.  Because
	 * audiodeatch() may have been invoked (and it would set sc_dying)
	 * from the time audioopen() was executed until now.  If it happens,
	 * audiodetach() may already have set file->dying for all sc_files
	 * that exist at that point, so that audioopen() must abort without
	 * inserting af to sc_files, in order to keep consistency.
	 */
	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		error = ENXIO;
		goto bad;
	}

	/* Count up finally */
	if (af->ptrack)
		sc->sc_popens++;
	if (af->rtrack)
		sc->sc_ropens++;
	mutex_enter(sc->sc_intr_lock);
	SLIST_INSERT_HEAD(&sc->sc_files, af, entry);
	mutex_exit(sc->sc_intr_lock);
	mutex_exit(sc->sc_lock);
	inserted = true;

	if (bellfile) {
		*bellfile = af;
	} else {
		error = fd_allocfile(&fp, &fd);
		if (error)
			goto bad;

		error = fd_clone(fp, fd, flags, &audio_fileops, af);
		KASSERTMSG(error == EMOVEFD, "error=%d", error);
	}

	/* Be nothing else after fd_clone */

	TRACEF(3, af, "done");
	return error;

bad:
	if (inserted) {
		mutex_enter(sc->sc_lock);
		mutex_enter(sc->sc_intr_lock);
		SLIST_REMOVE(&sc->sc_files, af, audio_file, entry);
		mutex_exit(sc->sc_intr_lock);
		if (af->ptrack)
			sc->sc_popens--;
		if (af->rtrack)
			sc->sc_ropens--;
		mutex_exit(sc->sc_lock);
	}

	if (rmixer_started) {
		mutex_enter(sc->sc_lock);
		audio_rmixer_halt(sc);
		mutex_exit(sc->sc_lock);
	}

	if (hw_opened) {
		if (sc->hw_if->close) {
			mutex_enter(sc->sc_lock);
			mutex_enter(sc->sc_intr_lock);
			sc->hw_if->close(sc->hw_hdl);
			mutex_exit(sc->sc_intr_lock);
			mutex_exit(sc->sc_lock);
		}
	}
	if (cred_held) {
		kauth_cred_free(sc->sc_cred);
	}

	/*
	 * Since track here is not yet linked to sc_files,
	 * you can call track_destroy() without sc_intr_lock.
	 */
	if (af->rtrack) {
		audio_track_destroy(af->rtrack);
		af->rtrack = NULL;
	}
	if (af->ptrack) {
		audio_track_destroy(af->ptrack);
		af->ptrack = NULL;
	}

	kmem_free(af, sizeof(*af));
	return error;
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_close(struct audio_softc *sc, audio_file_t *file)
{
	int error;

	/*
	 * Drain first.
	 * It must be done before unlinking(acquiring exlock).
	 */
	if (file->ptrack) {
		mutex_enter(sc->sc_lock);
		audio_track_drain(sc, file->ptrack);
		mutex_exit(sc->sc_lock);
	}

	mutex_enter(sc->sc_lock);
	mutex_enter(sc->sc_intr_lock);
	SLIST_REMOVE(&sc->sc_files, file, audio_file, entry);
	mutex_exit(sc->sc_intr_lock);
	mutex_exit(sc->sc_lock);

	error = audio_exlock_enter(sc);
	if (error) {
		/*
		 * If EIO, this sc is about to detach.  In this case, even if
		 * we don't do subsequent _unlink(), audiodetach() will do it.
		 */
		if (error == EIO)
			return error;

		/* XXX This should not happen but what should I do ? */
		panic("%s: can't acquire exlock: errno=%d", __func__, error);
	}
	audio_unlink(sc, file);
	audio_exlock_exit(sc);

	return 0;
}

/*
 * Unlink this file, but not freeing memory here.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static void
audio_unlink(struct audio_softc *sc, audio_file_t *file)
{
	kauth_cred_t cred = NULL;
	int error;

	mutex_enter(sc->sc_lock);

	TRACEF(1, file, "%spid=%d.%d po=%d ro=%d",
	    (audiodebug >= 3) ? "start " : "",
	    (int)curproc->p_pid, (int)curlwp->l_lid,
	    sc->sc_popens, sc->sc_ropens);
	KASSERTMSG(sc->sc_popens + sc->sc_ropens > 0,
	    "sc->sc_popens=%d, sc->sc_ropens=%d",
	    sc->sc_popens, sc->sc_ropens);

	device_active(sc->sc_dev, DVA_SYSTEM);

	if (file->ptrack) {
		TRACET(3, file->ptrack, "dropframes=%" PRIu64,
		    file->ptrack->dropframes);

		KASSERT(sc->sc_popens > 0);
		sc->sc_popens--;

		/* Call hw halt_output if this is the last playback track. */
		if (sc->sc_popens == 0 && sc->sc_pbusy) {
			error = audio_pmixer_halt(sc);
			if (error) {
				audio_printf(sc,
				    "halt_output failed: errno=%d (ignored)\n",
				    error);
			}
		}

		/* Restore mixing volume if all tracks are gone. */
		if (sc->sc_popens == 0) {
			/* intr_lock is not necessary, but just manners. */
			mutex_enter(sc->sc_intr_lock);
			sc->sc_pmixer->volume = 256;
			sc->sc_pmixer->voltimer = 0;
			mutex_exit(sc->sc_intr_lock);
		}
	}
	if (file->rtrack) {
		TRACET(3, file->rtrack, "dropframes=%" PRIu64,
		    file->rtrack->dropframes);

		KASSERT(sc->sc_ropens > 0);
		sc->sc_ropens--;

		/* Call hw halt_input if this is the last recording track. */
		if (sc->sc_ropens == 0 && sc->sc_rbusy) {
			error = audio_rmixer_halt(sc);
			if (error) {
				audio_printf(sc,
				    "halt_input failed: errno=%d (ignored)\n",
				    error);
			}
		}

	}

	/* Call hw close if this is the last track. */
	if (sc->sc_popens + sc->sc_ropens == 0) {
		if (sc->hw_if->close) {
			TRACE(2, "hw_if close");
			mutex_enter(sc->sc_intr_lock);
			sc->hw_if->close(sc->hw_hdl);
			mutex_exit(sc->sc_intr_lock);
		}
		cred = sc->sc_cred;
		sc->sc_cred = NULL;
	}

	mutex_exit(sc->sc_lock);
	if (cred)
		kauth_cred_free(cred);

	TRACE(3, "done");
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_read(struct audio_softc *sc, struct uio *uio, int ioflag,
	audio_file_t *file)
{
	audio_track_t *track;
	audio_ring_t *usrbuf;
	audio_ring_t *input;
	int error;

	/*
	 * On half-duplex hardware, O_RDWR is treated as O_WRONLY.
	 * However read() system call itself can be called because it's
	 * opened with O_RDWR.  So in this case, deny this read().
	 */
	track = file->rtrack;
	if (track == NULL) {
		return EBADF;
	}

	/* I think it's better than EINVAL. */
	if (track->mmapped)
		return EPERM;

	TRACET(2, track, "resid=%zd ioflag=0x%x", uio->uio_resid, ioflag);

#ifdef AUDIO_PM_IDLE
	error = audio_exlock_mutex_enter(sc);
	if (error)
		return error;

	if (device_is_active(&sc->sc_dev) || sc->sc_idle)
		device_active(&sc->sc_dev, DVA_SYSTEM);

	/* In recording, unlike playback, read() never operates rmixer. */

	audio_exlock_mutex_exit(sc);
#endif

	usrbuf = &track->usrbuf;
	input = track->input;
	error = 0;

	while (uio->uio_resid > 0 && error == 0) {
		int bytes;

		TRACET(3, track,
		    "while resid=%zd input=%d/%d/%d usrbuf=%d/%d/H%d",
		    uio->uio_resid,
		    input->head, input->used, input->capacity,
		    usrbuf->head, usrbuf->used, track->usrbuf_usedhigh);

		/* Wait when buffers are empty. */
		mutex_enter(sc->sc_lock);
		for (;;) {
			bool empty;
			audio_track_lock_enter(track);
			empty = (input->used == 0 && usrbuf->used == 0);
			audio_track_lock_exit(track);
			if (!empty)
				break;

			if ((ioflag & IO_NDELAY)) {
				mutex_exit(sc->sc_lock);
				return EWOULDBLOCK;
			}

			TRACET(3, track, "sleep");
			error = audio_track_waitio(sc, track);
			if (error) {
				mutex_exit(sc->sc_lock);
				return error;
			}
		}
		mutex_exit(sc->sc_lock);

		audio_track_lock_enter(track);
		/* Convert as many blocks as possible. */
		while (usrbuf->used <=
		            track->usrbuf_usedhigh - track->usrbuf_blksize &&
		    input->used > 0) {
			audio_track_record(track);
		}

		/* uiomove from usrbuf as many bytes as possible. */
		bytes = uimin(usrbuf->used, uio->uio_resid);
		while (bytes > 0) {
			int head = usrbuf->head;
			int len = uimin(bytes, usrbuf->capacity - head);
			error = uiomove((uint8_t *)usrbuf->mem + head, len,
			    uio);
			if (error) {
				audio_track_lock_exit(track);
				device_printf(sc->sc_dev,
				    "%s: uiomove(%d) failed: errno=%d\n",
				    __func__, len, error);
				goto abort;
			}
			auring_take(usrbuf, len);
			track->useriobytes += len;
			TRACET(3, track, "uiomove(len=%d) usrbuf=%d/%d/C%d",
			    len,
			    usrbuf->head, usrbuf->used, usrbuf->capacity);
			bytes -= len;
		}

		audio_track_lock_exit(track);
	}

abort:
	return error;
}


/*
 * Clear file's playback and/or record track buffer immediately.
 */
static void
audio_file_clear(struct audio_softc *sc, audio_file_t *file)
{

	if (file->ptrack)
		audio_track_clear(sc, file->ptrack);
	if (file->rtrack)
		audio_track_clear(sc, file->rtrack);
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_write(struct audio_softc *sc, struct uio *uio, int ioflag,
	audio_file_t *file)
{
	audio_track_t *track;
	audio_ring_t *usrbuf;
	audio_ring_t *outbuf;
	int error;

	track = file->ptrack;
	if (track == NULL)
		return EPERM;

	/* I think it's better than EINVAL. */
	if (track->mmapped)
		return EPERM;

	TRACET(2, track, "%sresid=%zd pid=%d.%d ioflag=0x%x",
	    audiodebug >= 3 ? "begin " : "",
	    uio->uio_resid, (int)curproc->p_pid, (int)curlwp->l_lid, ioflag);

	if (uio->uio_resid == 0) {
		track->eofcounter++;
		return 0;
	}

	error = audio_exlock_mutex_enter(sc);
	if (error)
		return error;

#ifdef AUDIO_PM_IDLE
	if (device_is_active(&sc->sc_dev) || sc->sc_idle)
		device_active(&sc->sc_dev, DVA_SYSTEM);
#endif

	/*
	 * The first write starts pmixer.
	 */
	if (sc->sc_pbusy == false)
		audio_pmixer_start(sc, false);
	audio_exlock_mutex_exit(sc);

	usrbuf = &track->usrbuf;
	outbuf = &track->outbuf;
	track->pstate = AUDIO_STATE_RUNNING;
	error = 0;

	while (uio->uio_resid > 0 && error == 0) {
		int bytes;

		TRACET(3, track, "while resid=%zd usrbuf=%d/%d/H%d",
		    uio->uio_resid,
		    usrbuf->head, usrbuf->used, track->usrbuf_usedhigh);

		/* Wait when buffers are full. */
		mutex_enter(sc->sc_lock);
		for (;;) {
			bool full;
			audio_track_lock_enter(track);
			full = (usrbuf->used >= track->usrbuf_usedhigh &&
			    outbuf->used >= outbuf->capacity);
			audio_track_lock_exit(track);
			if (!full)
				break;

			if ((ioflag & IO_NDELAY)) {
				error = EWOULDBLOCK;
				mutex_exit(sc->sc_lock);
				goto abort;
			}

			TRACET(3, track, "sleep usrbuf=%d/H%d",
			    usrbuf->used, track->usrbuf_usedhigh);
			error = audio_track_waitio(sc, track);
			if (error) {
				mutex_exit(sc->sc_lock);
				goto abort;
			}
		}
		mutex_exit(sc->sc_lock);

		audio_track_lock_enter(track);

		/* uiomove to usrbuf as many bytes as possible. */
		bytes = uimin(track->usrbuf_usedhigh - usrbuf->used,
		    uio->uio_resid);
		while (bytes > 0) {
			int tail = auring_tail(usrbuf);
			int len = uimin(bytes, usrbuf->capacity - tail);
			error = uiomove((uint8_t *)usrbuf->mem + tail, len,
			    uio);
			if (error) {
				audio_track_lock_exit(track);
				device_printf(sc->sc_dev,
				    "%s: uiomove(%d) failed: errno=%d\n",
				    __func__, len, error);
				goto abort;
			}
			auring_push(usrbuf, len);
			track->useriobytes += len;
			TRACET(3, track, "uiomove(len=%d) usrbuf=%d/%d/C%d",
			    len,
			    usrbuf->head, usrbuf->used, usrbuf->capacity);
			bytes -= len;
		}

		/* Convert them as many blocks as possible. */
		while (usrbuf->used >= track->usrbuf_blksize &&
		    outbuf->used < outbuf->capacity) {
			audio_track_play(track);
		}

		audio_track_lock_exit(track);
	}

abort:
	TRACET(3, track, "done error=%d", error);
	return error;
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_ioctl(dev_t dev, struct audio_softc *sc, u_long cmd, void *addr, int flag,
	struct lwp *l, audio_file_t *file)
{
	struct audio_offset *ao;
	struct audio_info ai;
	audio_track_t *track;
	audio_encoding_t *ae;
	audio_format_query_t *query;
	u_int stamp;
	u_int offs;
	int fd;
	int index;
	int error;

#if defined(AUDIO_DEBUG)
	const char *ioctlnames[] = {
		" AUDIO_GETINFO",	/* 21 */
		" AUDIO_SETINFO",	/* 22 */
		" AUDIO_DRAIN",		/* 23 */
		" AUDIO_FLUSH",		/* 24 */
		" AUDIO_WSEEK",		/* 25 */
		" AUDIO_RERROR",	/* 26 */
		" AUDIO_GETDEV",	/* 27 */
		" AUDIO_GETENC",	/* 28 */
		" AUDIO_GETFD",		/* 29 */
		" AUDIO_SETFD",		/* 30 */
		" AUDIO_PERROR",	/* 31 */
		" AUDIO_GETIOFFS",	/* 32 */
		" AUDIO_GETOOFFS",	/* 33 */
		" AUDIO_GETPROPS",	/* 34 */
		" AUDIO_GETBUFINFO",	/* 35 */
		" AUDIO_SETCHAN",	/* 36 */
		" AUDIO_GETCHAN",	/* 37 */
		" AUDIO_QUERYFORMAT",	/* 38 */
		" AUDIO_GETFORMAT",	/* 39 */
		" AUDIO_SETFORMAT",	/* 40 */
	};
	int nameidx = (cmd & 0xff);
	const char *ioctlname = "";
	if (21 <= nameidx && nameidx <= 21 + __arraycount(ioctlnames))
		ioctlname = ioctlnames[nameidx - 21];
	TRACEF(2, file, "(%lu,'%c',%lu)%s pid=%d.%d",
	    IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff, ioctlname,
	    (int)curproc->p_pid, (int)l->l_lid);
#endif

	error = 0;
	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;

	case FIONREAD:
		/* Get the number of bytes that can be read. */
		if (file->rtrack) {
			*(int *)addr = audio_track_readablebytes(file->rtrack);
		} else {
			*(int *)addr = 0;
		}
		break;

	case FIOASYNC:
		/* Set/Clear ASYNC I/O. */
		if (*(int *)addr) {
			file->async_audio = curproc->p_pid;
			TRACEF(2, file, "FIOASYNC pid %d", file->async_audio);
		} else {
			file->async_audio = 0;
			TRACEF(2, file, "FIOASYNC off");
		}
		break;

	case AUDIO_FLUSH:
		/* XXX TODO: clear errors and restart? */
		audio_file_clear(sc, file);
		break;

	case AUDIO_RERROR:
		/*
		 * Number of read bytes dropped.  We don't know where
		 * or when they were dropped (including conversion stage).
		 * Therefore, the number of accurate bytes or samples is
		 * also unknown.
		 */
		track = file->rtrack;
		if (track) {
			*(int *)addr = frametobyte(&track->usrbuf.fmt,
			    track->dropframes);
		}
		break;

	case AUDIO_PERROR:
		/*
		 * Number of write bytes dropped.  We don't know where
		 * or when they were dropped (including conversion stage).
		 * Therefore, the number of accurate bytes or samples is
		 * also unknown.
		 */
		track = file->ptrack;
		if (track) {
			*(int *)addr = frametobyte(&track->usrbuf.fmt,
			    track->dropframes);
		}
		break;

	case AUDIO_GETIOFFS:
		/* XXX TODO */
		ao = (struct audio_offset *)addr;
		ao->samples = 0;
		ao->deltablks = 0;
		ao->offset = 0;
		break;

	case AUDIO_GETOOFFS:
		ao = (struct audio_offset *)addr;
		track = file->ptrack;
		if (track == NULL) {
			ao->samples = 0;
			ao->deltablks = 0;
			ao->offset = 0;
			break;
		}
		mutex_enter(sc->sc_lock);
		mutex_enter(sc->sc_intr_lock);
		/* figure out where next DMA will start */
		stamp = track->usrbuf_stamp;
		offs = track->usrbuf.head;
		mutex_exit(sc->sc_intr_lock);
		mutex_exit(sc->sc_lock);

		ao->samples = stamp;
		ao->deltablks = (stamp / track->usrbuf_blksize) -
		    (track->usrbuf_stamp_last / track->usrbuf_blksize);
		track->usrbuf_stamp_last = stamp;
		offs = rounddown(offs, track->usrbuf_blksize)
		    + track->usrbuf_blksize;
		if (offs >= track->usrbuf.capacity)
			offs -= track->usrbuf.capacity;
		ao->offset = offs;

		TRACET(3, track, "GETOOFFS: samples=%u deltablks=%u offset=%u",
		    ao->samples, ao->deltablks, ao->offset);
		break;

	case AUDIO_WSEEK:
		/* XXX return value does not include outbuf one. */
		if (file->ptrack)
			*(u_long *)addr = file->ptrack->usrbuf.used;
		break;

	case AUDIO_SETINFO:
		error = audio_exlock_enter(sc);
		if (error)
			break;
		error = audio_file_setinfo(sc, file, (struct audio_info *)addr);
		if (error) {
			audio_exlock_exit(sc);
			break;
		}
		/* XXX TODO: update last_ai if /dev/sound ? */
		if (ISDEVSOUND(dev))
			error = audiogetinfo(sc, &sc->sc_ai, 0, file);
		audio_exlock_exit(sc);
		break;

	case AUDIO_GETINFO:
		error = audio_exlock_enter(sc);
		if (error)
			break;
		error = audiogetinfo(sc, (struct audio_info *)addr, 1, file);
		audio_exlock_exit(sc);
		break;

	case AUDIO_GETBUFINFO:
		error = audio_exlock_enter(sc);
		if (error)
			break;
		error = audiogetinfo(sc, (struct audio_info *)addr, 0, file);
		audio_exlock_exit(sc);
		break;

	case AUDIO_DRAIN:
		if (file->ptrack) {
			mutex_enter(sc->sc_lock);
			error = audio_track_drain(sc, file->ptrack);
			mutex_exit(sc->sc_lock);
		}
		break;

	case AUDIO_GETDEV:
		error = sc->hw_if->getdev(sc->hw_hdl, (audio_device_t *)addr);
		break;

	case AUDIO_GETENC:
		ae = (audio_encoding_t *)addr;
		index = ae->index;
		if (index < 0 || index >= __arraycount(audio_encodings)) {
			error = EINVAL;
			break;
		}
		*ae = audio_encodings[index];
		ae->index = index;
		/*
		 * EMULATED always.
		 * EMULATED flag at that time used to mean that it could
		 * not be passed directly to the hardware as-is.  But
		 * currently, all formats including hardware native is not
		 * passed directly to the hardware.  So I set EMULATED
		 * flag for all formats.
		 */
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	case AUDIO_GETFD:
		/*
		 * Returns the current setting of full duplex mode.
		 * If HW has full duplex mode and there are two mixers,
		 * it is full duplex.  Otherwise half duplex.
		 */
		error = audio_exlock_enter(sc);
		if (error)
			break;
		fd = (sc->sc_props & AUDIO_PROP_FULLDUPLEX)
		    && (sc->sc_pmixer && sc->sc_rmixer);
		audio_exlock_exit(sc);
		*(int *)addr = fd;
		break;

	case AUDIO_GETPROPS:
		*(int *)addr = sc->sc_props;
		break;

	case AUDIO_QUERYFORMAT:
		query = (audio_format_query_t *)addr;
		mutex_enter(sc->sc_lock);
		error = sc->hw_if->query_format(sc->hw_hdl, query);
		mutex_exit(sc->sc_lock);
		/* Hide internal information */
		query->fmt.driver_data = NULL;
		break;

	case AUDIO_GETFORMAT:
		error = audio_exlock_enter(sc);
		if (error)
			break;
		audio_mixers_get_format(sc, (struct audio_info *)addr);
		audio_exlock_exit(sc);
		break;

	case AUDIO_SETFORMAT:
		error = audio_exlock_enter(sc);
		audio_mixers_get_format(sc, &ai);
		error = audio_mixers_set_format(sc, (struct audio_info *)addr);
		if (error) {
			/* Rollback */
			audio_mixers_set_format(sc, &ai);
		}
		audio_exlock_exit(sc);
		break;

	case AUDIO_SETFD:
	case AUDIO_SETCHAN:
	case AUDIO_GETCHAN:
		/* Obsoleted */
		break;

	default:
		if (sc->hw_if->dev_ioctl) {
			mutex_enter(sc->sc_lock);
			error = sc->hw_if->dev_ioctl(sc->hw_hdl,
			    cmd, addr, flag, l);
			mutex_exit(sc->sc_lock);
		} else {
			TRACEF(2, file, "unknown ioctl");
			error = EINVAL;
		}
		break;
	}
	TRACEF(2, file, "(%lu,'%c',%lu)%s result %d",
	    IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd&0xff, ioctlname,
	    error);
	return error;
}

/*
 * Returns the number of bytes that can be read on recording buffer.
 */
static __inline int
audio_track_readablebytes(const audio_track_t *track)
{
	int bytes;

	KASSERT(track);
	KASSERT(track->mode == AUMODE_RECORD);

	/*
	 * Although usrbuf is primarily readable data, recorded data
	 * also stays in track->input until reading.  So it is necessary
	 * to add it.  track->input is in frame, usrbuf is in byte.
	 */
	bytes = track->usrbuf.used +
	    track->input->used * frametobyte(&track->usrbuf.fmt, 1);
	return bytes;
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_poll(struct audio_softc *sc, int events, struct lwp *l,
	audio_file_t *file)
{
	audio_track_t *track;
	int revents;
	bool in_is_valid;
	bool out_is_valid;

#if defined(AUDIO_DEBUG)
#define POLLEV_BITMAP "\177\020" \
	    "b\10WRBAND\0" \
	    "b\7RDBAND\0" "b\6RDNORM\0" "b\5NVAL\0" "b\4HUP\0" \
	    "b\3ERR\0" "b\2OUT\0" "b\1PRI\0" "b\0IN\0"
	char evbuf[64];
	snprintb(evbuf, sizeof(evbuf), POLLEV_BITMAP, events);
	TRACEF(2, file, "pid=%d.%d events=%s",
	    (int)curproc->p_pid, (int)l->l_lid, evbuf);
#endif

	revents = 0;
	in_is_valid = false;
	out_is_valid = false;
	if (events & (POLLIN | POLLRDNORM)) {
		track = file->rtrack;
		if (track) {
			int used;
			in_is_valid = true;
			used = audio_track_readablebytes(track);
			if (used > 0)
				revents |= events & (POLLIN | POLLRDNORM);
		}
	}
	if (events & (POLLOUT | POLLWRNORM)) {
		track = file->ptrack;
		if (track) {
			out_is_valid = true;
			if (track->usrbuf.used <= track->usrbuf_usedlow)
				revents |= events & (POLLOUT | POLLWRNORM);
		}
	}

	if (revents == 0) {
		mutex_enter(sc->sc_lock);
		if (in_is_valid) {
			TRACEF(3, file, "selrecord rsel");
			selrecord(l, &sc->sc_rsel);
		}
		if (out_is_valid) {
			TRACEF(3, file, "selrecord wsel");
			selrecord(l, &sc->sc_wsel);
		}
		mutex_exit(sc->sc_lock);
	}

#if defined(AUDIO_DEBUG)
	snprintb(evbuf, sizeof(evbuf), POLLEV_BITMAP, revents);
	TRACEF(2, file, "revents=%s", evbuf);
#endif
	return revents;
}

static const struct filterops audioread_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_audioread_detach,
	.f_event = filt_audioread_event,
};

static void
filt_audioread_detach(struct knote *kn)
{
	struct audio_softc *sc;
	audio_file_t *file;

	file = kn->kn_hook;
	sc = file->sc;
	TRACEF(3, file, "called");

	mutex_enter(sc->sc_lock);
	selremove_knote(&sc->sc_rsel, kn);
	mutex_exit(sc->sc_lock);
}

static int
filt_audioread_event(struct knote *kn, long hint)
{
	audio_file_t *file;
	audio_track_t *track;

	file = kn->kn_hook;
	track = file->rtrack;

	/*
	 * kn_data must contain the number of bytes can be read.
	 * The return value indicates whether the event occurs or not.
	 */

	if (track == NULL) {
		/* can not read with this descriptor. */
		kn->kn_data = 0;
		return 0;
	}

	kn->kn_data = audio_track_readablebytes(track);
	TRACEF(3, file, "data=%" PRId64, kn->kn_data);
	return kn->kn_data > 0;
}

static const struct filterops audiowrite_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_audiowrite_detach,
	.f_event = filt_audiowrite_event,
};

static void
filt_audiowrite_detach(struct knote *kn)
{
	struct audio_softc *sc;
	audio_file_t *file;

	file = kn->kn_hook;
	sc = file->sc;
	TRACEF(3, file, "called");

	mutex_enter(sc->sc_lock);
	selremove_knote(&sc->sc_wsel, kn);
	mutex_exit(sc->sc_lock);
}

static int
filt_audiowrite_event(struct knote *kn, long hint)
{
	audio_file_t *file;
	audio_track_t *track;

	file = kn->kn_hook;
	track = file->ptrack;

	/*
	 * kn_data must contain the number of bytes can be write.
	 * The return value indicates whether the event occurs or not.
	 */

	if (track == NULL) {
		/* can not write with this descriptor. */
		kn->kn_data = 0;
		return 0;
	}

	kn->kn_data = track->usrbuf_usedhigh - track->usrbuf.used;
	TRACEF(3, file, "data=%" PRId64, kn->kn_data);
	return (track->usrbuf.used < track->usrbuf_usedlow);
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_kqfilter(struct audio_softc *sc, audio_file_t *file, struct knote *kn)
{
	struct selinfo *sip;

	TRACEF(3, file, "kn=%p kn_filter=%x", kn, (int)kn->kn_filter);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		sip = &sc->sc_rsel;
		kn->kn_fop = &audioread_filtops;
		break;

	case EVFILT_WRITE:
		sip = &sc->sc_wsel;
		kn->kn_fop = &audiowrite_filtops;
		break;

	default:
		return EINVAL;
	}

	kn->kn_hook = file;

	mutex_enter(sc->sc_lock);
	selrecord_knote(sip, kn);
	mutex_exit(sc->sc_lock);

	return 0;
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
audio_mmap(struct audio_softc *sc, off_t *offp, size_t len, int prot,
	int *flagsp, int *advicep, struct uvm_object **uobjp, int *maxprotp,
	audio_file_t *file)
{
	audio_track_t *track;
	vsize_t vsize;
	int error;

	TRACEF(2, file, "off=%lld, prot=%d", (long long)(*offp), prot);

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
		track = file->ptrack;
	else if (prot == VM_PROT_READ)
		track = file->rtrack;
	else
		return EINVAL;
#else
	track = file->ptrack;
#endif
	if (track == NULL)
		return EACCES;

	vsize = roundup2(MAX(track->usrbuf.capacity, PAGE_SIZE), PAGE_SIZE);
	if (len > vsize)
		return EOVERFLOW;
	if (*offp > (uint)(vsize - len))
		return EOVERFLOW;

	/* XXX TODO: what happens when mmap twice. */
	if (!track->mmapped) {
		track->mmapped = true;

		if (!track->is_pause) {
			error = audio_exlock_mutex_enter(sc);
			if (error)
				return error;
			if (sc->sc_pbusy == false)
				audio_pmixer_start(sc, true);
			audio_exlock_mutex_exit(sc);
		}
		/* XXX mmapping record buffer is not supported */
	}

	/* get ringbuffer */
	*uobjp = track->uobj;

	/* Acquire a reference for the mmap.  munmap will release. */
	uao_reference(*uobjp);
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;
	*flagsp = MAP_SHARED;
	return 0;
}

/*
 * /dev/audioctl has to be able to open at any time without interference
 * with any /dev/audio or /dev/sound.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static int
audioctl_open(dev_t dev, struct audio_softc *sc, int flags, int ifmt,
	struct lwp *l)
{
	struct file *fp;
	audio_file_t *af;
	int fd;
	int error;

	KASSERT(sc->sc_exlock);

	TRACE(1, "called");

	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	af = kmem_zalloc(sizeof(*af), KM_SLEEP);
	af->sc = sc;
	af->dev = dev;

	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		kmem_free(af, sizeof(*af));
		fd_abort(curproc, fp, fd);
		return ENXIO;
	}
	mutex_enter(sc->sc_intr_lock);
	SLIST_INSERT_HEAD(&sc->sc_files, af, entry);
	mutex_exit(sc->sc_intr_lock);
	mutex_exit(sc->sc_lock);

	error = fd_clone(fp, fd, flags, &audio_fileops, af);
	KASSERTMSG(error == EMOVEFD, "error=%d", error);

	return error;
}

/*
 * Free 'mem' if available, and initialize the pointer.
 * For this reason, this is implemented as macro.
 */
#define audio_free(mem)	do {	\
	if (mem != NULL) {	\
		kern_free(mem);	\
		mem = NULL;	\
	}	\
} while (0)

/*
 * (Re)allocate 'memblock' with specified 'bytes'.
 * bytes must not be 0.
 * This function never returns NULL.
 */
static void *
audio_realloc(void *memblock, size_t bytes)
{

	KASSERT(bytes != 0);
	audio_free(memblock);
	return kern_malloc(bytes, M_WAITOK);
}

/*
 * (Re)allocate usrbuf with 'newbufsize' bytes.
 * Use this function for usrbuf because only usrbuf can be mmapped.
 * If successful, it updates track->usrbuf.mem, track->usrbuf.capacity and
 * returns 0.  Otherwise, it clears track->usrbuf.mem, track->usrbuf.capacity
 * and returns errno.
 * It must be called before updating usrbuf.capacity.
 */
static int
audio_realloc_usrbuf(audio_track_t *track, int newbufsize)
{
	struct audio_softc *sc;
	vaddr_t vstart;
	vsize_t oldvsize;
	vsize_t newvsize;
	int error;

	KASSERT(newbufsize > 0);
	sc = track->mixer->sc;

	/* Get a nonzero multiple of PAGE_SIZE */
	newvsize = roundup2(MAX(newbufsize, PAGE_SIZE), PAGE_SIZE);

	if (track->usrbuf.mem != NULL) {
		oldvsize = roundup2(MAX(track->usrbuf.capacity, PAGE_SIZE),
		    PAGE_SIZE);
		if (oldvsize == newvsize) {
			track->usrbuf.capacity = newbufsize;
			return 0;
		}
		vstart = (vaddr_t)track->usrbuf.mem;
		uvm_unmap(kernel_map, vstart, vstart + oldvsize);
		/* uvm_unmap also detach uobj */
		track->uobj = NULL;		/* paranoia */
		track->usrbuf.mem = NULL;
	}

	/* Create a uvm anonymous object */
	track->uobj = uao_create(newvsize, 0);

	/* Map it into the kernel virtual address space */
	vstart = 0;
	error = uvm_map(kernel_map, &vstart, newvsize, track->uobj, 0, 0,
	    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_NONE,
	    UVM_ADV_RANDOM, 0));
	if (error) {
		device_printf(sc->sc_dev, "uvm_map failed: errno=%d\n", error);
		uao_detach(track->uobj);	/* release reference */
		goto abort;
	}

	error = uvm_map_pageable(kernel_map, vstart, vstart + newvsize,
	    false, 0);
	if (error) {
		device_printf(sc->sc_dev, "uvm_map_pageable failed: errno=%d\n",
		    error);
		uvm_unmap(kernel_map, vstart, vstart + newvsize);
		/* uvm_unmap also detach uobj */
		goto abort;
	}

	track->usrbuf.mem = (void *)vstart;
	track->usrbuf.capacity = newbufsize;
	memset(track->usrbuf.mem, 0, newvsize);
	return 0;

	/* failure */
abort:
	track->uobj = NULL;		/* paranoia */
	track->usrbuf.mem = NULL;
	track->usrbuf.capacity = 0;
	return error;
}

/*
 * Free usrbuf (if available).
 */
static void
audio_free_usrbuf(audio_track_t *track)
{
	vaddr_t vstart;
	vsize_t vsize;

	vstart = (vaddr_t)track->usrbuf.mem;
	vsize = roundup2(MAX(track->usrbuf.capacity, PAGE_SIZE), PAGE_SIZE);
	if (track->usrbuf.mem != NULL) {
		/*
		 * Unmap the kernel mapping.  uvm_unmap releases the
		 * reference to the uvm object, and this should be the
		 * last virtual mapping of the uvm object, so no need
		 * to explicitly release (`detach') the object.
		 */
		uvm_unmap(kernel_map, vstart, vstart + vsize);

		track->uobj = NULL;
		track->usrbuf.mem = NULL;
		track->usrbuf.capacity = 0;
	}
}

/*
 * This filter changes the volume for each channel.
 * arg->context points track->ch_volume[].
 */
static void
audio_track_chvol(audio_filter_arg_t *arg)
{
	int16_t *ch_volume;
	const aint_t *s;
	aint_t *d;
	u_int i;
	u_int ch;
	u_int channels;

	DIAGNOSTIC_filter_arg(arg);
	KASSERTMSG(arg->srcfmt->channels == arg->dstfmt->channels,
	    "arg->srcfmt->channels=%d, arg->dstfmt->channels=%d",
	    arg->srcfmt->channels, arg->dstfmt->channels);
	KASSERT(arg->context != NULL);
	KASSERTMSG(arg->srcfmt->channels <= AUDIO_MAX_CHANNELS,
	    "arg->srcfmt->channels=%d", arg->srcfmt->channels);

	s = arg->src;
	d = arg->dst;
	ch_volume = arg->context;

	channels = arg->srcfmt->channels;
	for (i = 0; i < arg->count; i++) {
		for (ch = 0; ch < channels; ch++) {
			aint2_t val;
			val = *s++;
			val = AUDIO_SCALEDOWN(val * ch_volume[ch], 8);
			*d++ = (aint_t)val;
		}
	}
}

/*
 * This filter performs conversion from stereo (or more channels) to mono.
 */
static void
audio_track_chmix_mixLR(audio_filter_arg_t *arg)
{
	const aint_t *s;
	aint_t *d;
	u_int i;

	DIAGNOSTIC_filter_arg(arg);

	s = arg->src;
	d = arg->dst;

	for (i = 0; i < arg->count; i++) {
		*d++ = AUDIO_SCALEDOWN(s[0], 1) + AUDIO_SCALEDOWN(s[1], 1);
		s += arg->srcfmt->channels;
	}
}

/*
 * This filter performs conversion from mono to stereo (or more channels).
 */
static void
audio_track_chmix_dupLR(audio_filter_arg_t *arg)
{
	const aint_t *s;
	aint_t *d;
	u_int i;
	u_int ch;
	u_int dstchannels;

	DIAGNOSTIC_filter_arg(arg);

	s = arg->src;
	d = arg->dst;
	dstchannels = arg->dstfmt->channels;

	for (i = 0; i < arg->count; i++) {
		d[0] = s[0];
		d[1] = s[0];
		s++;
		d += dstchannels;
	}
	if (dstchannels > 2) {
		d = arg->dst;
		for (i = 0; i < arg->count; i++) {
			for (ch = 2; ch < dstchannels; ch++) {
				d[ch] = 0;
			}
			d += dstchannels;
		}
	}
}

/*
 * This filter shrinks M channels into N channels.
 * Extra channels are discarded.
 */
static void
audio_track_chmix_shrink(audio_filter_arg_t *arg)
{
	const aint_t *s;
	aint_t *d;
	u_int i;
	u_int ch;

	DIAGNOSTIC_filter_arg(arg);

	s = arg->src;
	d = arg->dst;

	for (i = 0; i < arg->count; i++) {
		for (ch = 0; ch < arg->dstfmt->channels; ch++) {
			*d++ = s[ch];
		}
		s += arg->srcfmt->channels;
	}
}

/*
 * This filter expands M channels into N channels.
 * Silence is inserted for missing channels.
 */
static void
audio_track_chmix_expand(audio_filter_arg_t *arg)
{
	const aint_t *s;
	aint_t *d;
	u_int i;
	u_int ch;
	u_int srcchannels;
	u_int dstchannels;

	DIAGNOSTIC_filter_arg(arg);

	s = arg->src;
	d = arg->dst;

	srcchannels = arg->srcfmt->channels;
	dstchannels = arg->dstfmt->channels;
	for (i = 0; i < arg->count; i++) {
		for (ch = 0; ch < srcchannels; ch++) {
			*d++ = *s++;
		}
		for (; ch < dstchannels; ch++) {
			*d++ = 0;
		}
	}
}

/*
 * This filter performs frequency conversion (up sampling).
 * It uses linear interpolation.
 */
static void
audio_track_freq_up(audio_filter_arg_t *arg)
{
	audio_track_t *track;
	audio_ring_t *src;
	audio_ring_t *dst;
	const aint_t *s;
	aint_t *d;
	aint_t prev[AUDIO_MAX_CHANNELS];
	aint_t curr[AUDIO_MAX_CHANNELS];
	aint_t grad[AUDIO_MAX_CHANNELS];
	u_int i;
	u_int t;
	u_int step;
	u_int channels;
	u_int ch;
	int srcused;

	track = arg->context;
	KASSERT(track);
	src = &track->freq.srcbuf;
	dst = track->freq.dst;
	DIAGNOSTIC_ring(dst);
	DIAGNOSTIC_ring(src);
	KASSERT(src->used > 0);
	KASSERTMSG(src->fmt.channels == dst->fmt.channels,
	    "src->fmt.channels=%d dst->fmt.channels=%d",
	    src->fmt.channels, dst->fmt.channels);
	KASSERTMSG(src->head % track->mixer->frames_per_block == 0,
	    "src->head=%d track->mixer->frames_per_block=%d",
	    src->head, track->mixer->frames_per_block);

	s = arg->src;
	d = arg->dst;

	/*
	 * In order to facilitate interpolation for each block, slide (delay)
	 * input by one sample.  As a result, strictly speaking, the output
	 * phase is delayed by 1/dstfreq.  However, I believe there is no
	 * observable impact.
	 *
	 * Example)
	 * srcfreq:dstfreq = 1:3
	 *
	 *  A - -
	 *  |
	 *  |
	 *  |     B - -
	 *  +-----+-----> input timeframe
	 *  0     1
	 *
	 *  0     1
	 *  +-----+-----> input timeframe
	 *  |     A
	 *  |   x   x
	 *  | x       x
	 *  x          (B)
	 *  +-+-+-+-+-+-> output timeframe
	 *  0 1 2 3 4 5
	 */

	/* Last samples in previous block */
	channels = src->fmt.channels;
	for (ch = 0; ch < channels; ch++) {
		prev[ch] = track->freq_prev[ch];
		curr[ch] = track->freq_curr[ch];
		grad[ch] = curr[ch] - prev[ch];
	}

	step = track->freq_step;
	t = track->freq_current;
//#define FREQ_DEBUG
#if defined(FREQ_DEBUG)
#define PRINTF(fmt...)	printf(fmt)
#else
#define PRINTF(fmt...)	do { } while (0)
#endif
	srcused = src->used;
	PRINTF("upstart step=%d leap=%d", step, track->freq_leap);
	PRINTF(" srcused=%d arg->count=%u", src->used, arg->count);
	PRINTF(" prev=%d curr=%d grad=%d", prev[0], curr[0], grad[0]);
	PRINTF(" t=%d\n", t);

	for (i = 0; i < arg->count; i++) {
		PRINTF("i=%d t=%5d", i, t);
		if (t >= 65536) {
			for (ch = 0; ch < channels; ch++) {
				prev[ch] = curr[ch];
				curr[ch] = *s++;
				grad[ch] = curr[ch] - prev[ch];
			}
			PRINTF(" prev=%d s[%d]=%d",
			    prev[0], src->used - srcused, curr[0]);

			/* Update */
			t -= 65536;
			srcused--;
			if (srcused < 0) {
				PRINTF(" break\n");
				break;
			}
		}

		for (ch = 0; ch < channels; ch++) {
			*d++ = prev[ch] + (aint2_t)grad[ch] * t / 65536;
#if defined(FREQ_DEBUG)
			if (ch == 0)
				printf(" t=%5d *d=%d", t, d[-1]);
#endif
		}
		t += step;

		PRINTF("\n");
	}
	PRINTF("end prev=%d curr=%d\n", prev[0], curr[0]);

	auring_take(src, src->used);
	auring_push(dst, i);

	/* Adjust */
	t += track->freq_leap;

	track->freq_current = t;
	for (ch = 0; ch < channels; ch++) {
		track->freq_prev[ch] = prev[ch];
		track->freq_curr[ch] = curr[ch];
	}
}

/*
 * This filter performs frequency conversion (down sampling).
 * It uses simple thinning.
 */
static void
audio_track_freq_down(audio_filter_arg_t *arg)
{
	audio_track_t *track;
	audio_ring_t *src;
	audio_ring_t *dst;
	const aint_t *s0;
	aint_t *d;
	u_int i;
	u_int t;
	u_int step;
	u_int ch;
	u_int channels;

	track = arg->context;
	KASSERT(track);
	src = &track->freq.srcbuf;
	dst = track->freq.dst;

	DIAGNOSTIC_ring(dst);
	DIAGNOSTIC_ring(src);
	KASSERT(src->used > 0);
	KASSERTMSG(src->fmt.channels == dst->fmt.channels,
	    "src->fmt.channels=%d dst->fmt.channels=%d",
	    src->fmt.channels, dst->fmt.channels);
	KASSERTMSG(src->head % track->mixer->frames_per_block == 0,
	    "src->head=%d track->mixer->frames_per_block=%d",
	    src->head, track->mixer->frames_per_block);

	s0 = arg->src;
	d = arg->dst;
	t = track->freq_current;
	step = track->freq_step;
	channels = dst->fmt.channels;
	PRINTF("downstart step=%d leap=%d", step, track->freq_leap);
	PRINTF(" srcused=%d arg->count=%u", src->used, arg->count);
	PRINTF(" t=%d\n", t);

	for (i = 0; i < arg->count && t / 65536 < src->used; i++) {
		const aint_t *s;
		PRINTF("i=%4d t=%10d", i, t);
		s = s0 + (t / 65536) * channels;
		PRINTF(" s=%5ld", (s - s0) / channels);
		for (ch = 0; ch < channels; ch++) {
			if (ch == 0) PRINTF(" *s=%d", s[ch]);
			*d++ = s[ch];
		}
		PRINTF("\n");
		t += step;
	}
	t += track->freq_leap;
	PRINTF("end t=%d\n", t);
	auring_take(src, src->used);
	auring_push(dst, i);
	track->freq_current = t % 65536;
}

/*
 * Creates track and returns it.
 * Must be called without sc_lock held.
 */
audio_track_t *
audio_track_create(struct audio_softc *sc, audio_trackmixer_t *mixer)
{
	audio_track_t *track;
	static int newid = 0;

	track = kmem_zalloc(sizeof(*track), KM_SLEEP);

	track->id = newid++;
	track->mixer = mixer;
	track->mode = mixer->mode;

	/* Do TRACE after id is assigned. */
	TRACET(3, track, "for %s",
	    mixer->mode == AUMODE_PLAY ? "playback" : "recording");

#if defined(AUDIO_SUPPORT_TRACK_VOLUME)
	track->volume = 256;
#endif
	for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
		track->ch_volume[i] = 256;
	}

	return track;
}

/*
 * Release all resources of the track and track itself.
 * track must not be NULL.  Don't specify the track within the file
 * structure linked from sc->sc_files.
 */
static void
audio_track_destroy(audio_track_t *track)
{

	KASSERT(track);

	audio_free_usrbuf(track);
	audio_free(track->codec.srcbuf.mem);
	audio_free(track->chvol.srcbuf.mem);
	audio_free(track->chmix.srcbuf.mem);
	audio_free(track->freq.srcbuf.mem);
	audio_free(track->outbuf.mem);

	kmem_free(track, sizeof(*track));
}

/*
 * It returns encoding conversion filter according to src and dst format.
 * If it is not a convertible pair, it returns NULL.  Either src or dst
 * must be internal format.
 */
static audio_filter_t
audio_track_get_codec(audio_track_t *track, const audio_format2_t *src,
	const audio_format2_t *dst)
{

	if (audio_format2_is_internal(src)) {
		if (dst->encoding == AUDIO_ENCODING_ULAW) {
			return audio_internal_to_mulaw;
		} else if (dst->encoding == AUDIO_ENCODING_ALAW) {
			return audio_internal_to_alaw;
		} else if (audio_format2_is_linear(dst)) {
			switch (dst->stride) {
			case 8:
				return audio_internal_to_linear8;
			case 16:
				return audio_internal_to_linear16;
#if defined(AUDIO_SUPPORT_LINEAR24)
			case 24:
				return audio_internal_to_linear24;
#endif
			case 32:
				return audio_internal_to_linear32;
			default:
				TRACET(1, track, "unsupported %s stride %d",
				    "dst", dst->stride);
				goto abort;
			}
		}
	} else if (audio_format2_is_internal(dst)) {
		if (src->encoding == AUDIO_ENCODING_ULAW) {
			return audio_mulaw_to_internal;
		} else if (src->encoding == AUDIO_ENCODING_ALAW) {
			return audio_alaw_to_internal;
		} else if (audio_format2_is_linear(src)) {
			switch (src->stride) {
			case 8:
				return audio_linear8_to_internal;
			case 16:
				return audio_linear16_to_internal;
#if defined(AUDIO_SUPPORT_LINEAR24)
			case 24:
				return audio_linear24_to_internal;
#endif
			case 32:
				return audio_linear32_to_internal;
			default:
				TRACET(1, track, "unsupported %s stride %d",
				    "src", src->stride);
				goto abort;
			}
		}
	}

	TRACET(1, track, "unsupported encoding");
abort:
#if defined(AUDIO_DEBUG)
	if (audiodebug >= 2) {
		char buf[100];
		audio_format2_tostr(buf, sizeof(buf), src);
		TRACET(2, track, "src %s", buf);
		audio_format2_tostr(buf, sizeof(buf), dst);
		TRACET(2, track, "dst %s", buf);
	}
#endif
	return NULL;
}

/*
 * Initialize the codec stage of this track as necessary.
 * If successful, it initializes the codec stage as necessary, stores updated
 * last_dst in *last_dstp in any case, and returns 0.
 * Otherwise, it returns errno without modifying *last_dstp.
 */
static int
audio_track_init_codec(audio_track_t *track, audio_ring_t **last_dstp)
{
	audio_ring_t *last_dst;
	audio_ring_t *srcbuf;
	audio_format2_t *srcfmt;
	audio_format2_t *dstfmt;
	audio_filter_arg_t *arg;
	u_int len;
	int error;

	KASSERT(track);

	last_dst = *last_dstp;
	dstfmt = &last_dst->fmt;
	srcfmt = &track->inputfmt;
	srcbuf = &track->codec.srcbuf;
	error = 0;

	if (srcfmt->encoding != dstfmt->encoding
	 || srcfmt->precision != dstfmt->precision
	 || srcfmt->stride != dstfmt->stride) {
		track->codec.dst = last_dst;

		srcbuf->fmt = *dstfmt;
		srcbuf->fmt.encoding = srcfmt->encoding;
		srcbuf->fmt.precision = srcfmt->precision;
		srcbuf->fmt.stride = srcfmt->stride;

		track->codec.filter = audio_track_get_codec(track,
		    &srcbuf->fmt, dstfmt);
		if (track->codec.filter == NULL) {
			error = EINVAL;
			goto abort;
		}

		srcbuf->head = 0;
		srcbuf->used = 0;
		srcbuf->capacity = frame_per_block(track->mixer, &srcbuf->fmt);
		len = auring_bytelen(srcbuf);
		srcbuf->mem = audio_realloc(srcbuf->mem, len);

		arg = &track->codec.arg;
		arg->srcfmt = &srcbuf->fmt;
		arg->dstfmt = dstfmt;
		arg->context = NULL;

		*last_dstp = srcbuf;
		return 0;
	}

abort:
	track->codec.filter = NULL;
	audio_free(srcbuf->mem);
	return error;
}

/*
 * Initialize the chvol stage of this track as necessary.
 * If successful, it initializes the chvol stage as necessary, stores updated
 * last_dst in *last_dstp in any case, and returns 0.
 * Otherwise, it returns errno without modifying *last_dstp.
 */
static int
audio_track_init_chvol(audio_track_t *track, audio_ring_t **last_dstp)
{
	audio_ring_t *last_dst;
	audio_ring_t *srcbuf;
	audio_format2_t *srcfmt;
	audio_format2_t *dstfmt;
	audio_filter_arg_t *arg;
	u_int len;
	int error;

	KASSERT(track);

	last_dst = *last_dstp;
	dstfmt = &last_dst->fmt;
	srcfmt = &track->inputfmt;
	srcbuf = &track->chvol.srcbuf;
	error = 0;

	/* Check whether channel volume conversion is necessary. */
	bool use_chvol = false;
	for (int ch = 0; ch < srcfmt->channels; ch++) {
		if (track->ch_volume[ch] != 256) {
			use_chvol = true;
			break;
		}
	}

	if (use_chvol == true) {
		track->chvol.dst = last_dst;
		track->chvol.filter = audio_track_chvol;

		srcbuf->fmt = *dstfmt;
		/* no format conversion occurs */

		srcbuf->head = 0;
		srcbuf->used = 0;
		srcbuf->capacity = frame_per_block(track->mixer, &srcbuf->fmt);
		len = auring_bytelen(srcbuf);
		srcbuf->mem = audio_realloc(srcbuf->mem, len);

		arg = &track->chvol.arg;
		arg->srcfmt = &srcbuf->fmt;
		arg->dstfmt = dstfmt;
		arg->context = track->ch_volume;

		*last_dstp = srcbuf;
		return 0;
	}

	track->chvol.filter = NULL;
	audio_free(srcbuf->mem);
	return error;
}

/*
 * Initialize the chmix stage of this track as necessary.
 * If successful, it initializes the chmix stage as necessary, stores updated
 * last_dst in *last_dstp in any case, and returns 0.
 * Otherwise, it returns errno without modifying *last_dstp.
 */
static int
audio_track_init_chmix(audio_track_t *track, audio_ring_t **last_dstp)
{
	audio_ring_t *last_dst;
	audio_ring_t *srcbuf;
	audio_format2_t *srcfmt;
	audio_format2_t *dstfmt;
	audio_filter_arg_t *arg;
	u_int srcch;
	u_int dstch;
	u_int len;
	int error;

	KASSERT(track);

	last_dst = *last_dstp;
	dstfmt = &last_dst->fmt;
	srcfmt = &track->inputfmt;
	srcbuf = &track->chmix.srcbuf;
	error = 0;

	srcch = srcfmt->channels;
	dstch = dstfmt->channels;
	if (srcch != dstch) {
		track->chmix.dst = last_dst;

		if (srcch >= 2 && dstch == 1) {
			track->chmix.filter = audio_track_chmix_mixLR;
		} else if (srcch == 1 && dstch >= 2) {
			track->chmix.filter = audio_track_chmix_dupLR;
		} else if (srcch > dstch) {
			track->chmix.filter = audio_track_chmix_shrink;
		} else {
			track->chmix.filter = audio_track_chmix_expand;
		}

		srcbuf->fmt = *dstfmt;
		srcbuf->fmt.channels = srcch;

		srcbuf->head = 0;
		srcbuf->used = 0;
		/* XXX The buffer size should be able to calculate. */
		srcbuf->capacity = frame_per_block(track->mixer, &srcbuf->fmt);
		len = auring_bytelen(srcbuf);
		srcbuf->mem = audio_realloc(srcbuf->mem, len);

		arg = &track->chmix.arg;
		arg->srcfmt = &srcbuf->fmt;
		arg->dstfmt = dstfmt;
		arg->context = NULL;

		*last_dstp = srcbuf;
		return 0;
	}

	track->chmix.filter = NULL;
	audio_free(srcbuf->mem);
	return error;
}

/*
 * Initialize the freq stage of this track as necessary.
 * If successful, it initializes the freq stage as necessary, stores updated
 * last_dst in *last_dstp in any case, and returns 0.
 * Otherwise, it returns errno without modifying *last_dstp.
 */
static int
audio_track_init_freq(audio_track_t *track, audio_ring_t **last_dstp)
{
	audio_ring_t *last_dst;
	audio_ring_t *srcbuf;
	audio_format2_t *srcfmt;
	audio_format2_t *dstfmt;
	audio_filter_arg_t *arg;
	uint32_t srcfreq;
	uint32_t dstfreq;
	u_int dst_capacity;
	u_int mod;
	u_int len;
	int error;

	KASSERT(track);

	last_dst = *last_dstp;
	dstfmt = &last_dst->fmt;
	srcfmt = &track->inputfmt;
	srcbuf = &track->freq.srcbuf;
	error = 0;

	srcfreq = srcfmt->sample_rate;
	dstfreq = dstfmt->sample_rate;
	if (srcfreq != dstfreq) {
		track->freq.dst = last_dst;

		memset(track->freq_prev, 0, sizeof(track->freq_prev));
		memset(track->freq_curr, 0, sizeof(track->freq_curr));

		/* freq_step is the ratio of src/dst when let dst 65536. */
		track->freq_step = (uint64_t)srcfreq * 65536 / dstfreq;

		dst_capacity = frame_per_block(track->mixer, dstfmt);
		mod = (uint64_t)srcfreq * 65536 % dstfreq;
		track->freq_leap = (mod * dst_capacity + dstfreq / 2) / dstfreq;

		if (track->freq_step < 65536) {
			track->freq.filter = audio_track_freq_up;
			/* In order to carry at the first time. */
			track->freq_current = 65536;
		} else {
			track->freq.filter = audio_track_freq_down;
			track->freq_current = 0;
		}

		srcbuf->fmt = *dstfmt;
		srcbuf->fmt.sample_rate = srcfreq;

		srcbuf->head = 0;
		srcbuf->used = 0;
		srcbuf->capacity = frame_per_block(track->mixer, &srcbuf->fmt);
		len = auring_bytelen(srcbuf);
		srcbuf->mem = audio_realloc(srcbuf->mem, len);

		arg = &track->freq.arg;
		arg->srcfmt = &srcbuf->fmt;
		arg->dstfmt = dstfmt;/*&last_dst->fmt;*/
		arg->context = track;

		*last_dstp = srcbuf;
		return 0;
	}

	track->freq.filter = NULL;
	audio_free(srcbuf->mem);
	return error;
}

/*
 * When playing back: (e.g. if codec and freq stage are valid)
 *
 *               write
 *                | uiomove
 *                v
 *  usrbuf      [...............]  byte ring buffer (mmap-able)
 *                | memcpy
 *                v
 *  codec.srcbuf[....]             1 block (ring) buffer   <-- stage input
 *       .dst ----+
 *                | convert
 *                v
 *  freq.srcbuf [....]             1 block (ring) buffer
 *      .dst  ----+
 *                | convert
 *                v
 *  outbuf      [...............]  NBLKOUT blocks ring buffer
 *
 *
 * When recording:
 *
 *  freq.srcbuf [...............]  NBLKOUT blocks ring buffer <-- stage input
 *      .dst  ----+
 *                | convert
 *                v
 *  codec.srcbuf[.....]            1 block (ring) buffer
 *       .dst ----+
 *                | convert
 *                v
 *  outbuf      [.....]            1 block (ring) buffer
 *                | memcpy
 *                v
 *  usrbuf      [...............]  byte ring buffer (mmap-able *)
 *                | uiomove
 *                v
 *               read
 *
 *    *: usrbuf for recording is also mmap-able due to symmetry with
 *       playback buffer, but for now mmap will never happen for recording.
 */

/*
 * Set the userland format of this track.
 * usrfmt argument should have been previously verified by
 * audio_track_setinfo_check().
 * This function may release and reallocate all internal conversion buffers.
 * It returns 0 if successful.  Otherwise it returns errno with clearing all
 * internal buffers.
 * It must be called without sc_intr_lock since uvm_* routines require non
 * intr_lock state.
 * It must be called with track lock held since it may release and reallocate
 * outbuf.
 */
static int
audio_track_set_format(audio_track_t *track, audio_format2_t *usrfmt)
{
	struct audio_softc *sc;
	u_int newbufsize;
	u_int oldblksize;
	u_int len;
	int error;

	KASSERT(track);
	sc = track->mixer->sc;

	/* usrbuf is the closest buffer to the userland. */
	track->usrbuf.fmt = *usrfmt;

	/*
	 * For references, one block size (in 40msec) is:
	 *  320 bytes    = 204 blocks/64KB for mulaw/8kHz/1ch
	 *  7680 bytes   = 8 blocks/64KB for s16/48kHz/2ch
	 *  30720 bytes  = 90 KB/3blocks for s16/48kHz/8ch
	 *  61440 bytes  = 180 KB/3blocks for s16/96kHz/8ch
	 *  245760 bytes = 720 KB/3blocks for s32/192kHz/8ch
	 *
	 * For example,
	 * 1) If usrbuf_blksize = 7056 (s16/44.1k/2ch) and PAGE_SIZE = 8192,
	 *     newbufsize = rounddown(65536 / 7056) = 63504
	 *     newvsize = roundup2(63504, PAGE_SIZE) = 65536
	 *    Therefore it maps 8 * 8K pages and usrbuf->capacity = 63504.
	 *
	 * 2) If usrbuf_blksize = 7680 (s16/48k/2ch) and PAGE_SIZE = 4096,
	 *     newbufsize = rounddown(65536 / 7680) = 61440
	 *     newvsize = roundup2(61440, PAGE_SIZE) = 61440 (= 15 pages)
	 *    Therefore it maps 15 * 4K pages and usrbuf->capacity = 61440.
	 */
	oldblksize = track->usrbuf_blksize;
	track->usrbuf_blksize = frametobyte(&track->usrbuf.fmt,
	    frame_per_block(track->mixer, &track->usrbuf.fmt));
	track->usrbuf.head = 0;
	track->usrbuf.used = 0;
	newbufsize = MAX(track->usrbuf_blksize * AUMINNOBLK, 65536);
	newbufsize = rounddown(newbufsize, track->usrbuf_blksize);
	error = audio_realloc_usrbuf(track, newbufsize);
	if (error) {
		device_printf(sc->sc_dev, "malloc usrbuf(%d) failed\n",
		    newbufsize);
		goto error;
	}

	/* Recalc water mark. */
	if (track->usrbuf_blksize != oldblksize) {
		if (audio_track_is_playback(track)) {
			/* Set high at 100%, low at 75%.  */
			track->usrbuf_usedhigh = track->usrbuf.capacity;
			track->usrbuf_usedlow = track->usrbuf.capacity * 3 / 4;
		} else {
			/* Set high at 100% minus 1block(?), low at 0% */
			track->usrbuf_usedhigh = track->usrbuf.capacity -
			    track->usrbuf_blksize;
			track->usrbuf_usedlow = 0;
		}
	}

	/* Stage buffer */
	audio_ring_t *last_dst = &track->outbuf;
	if (audio_track_is_playback(track)) {
		/* On playback, initialize from the mixer side in order. */
		track->inputfmt = *usrfmt;
		track->outbuf.fmt =  track->mixer->track_fmt;

		if ((error = audio_track_init_freq(track, &last_dst)) != 0)
			goto error;
		if ((error = audio_track_init_chmix(track, &last_dst)) != 0)
			goto error;
		if ((error = audio_track_init_chvol(track, &last_dst)) != 0)
			goto error;
		if ((error = audio_track_init_codec(track, &last_dst)) != 0)
			goto error;
	} else {
		/* On recording, initialize from userland side in order. */
		track->inputfmt = track->mixer->track_fmt;
		track->outbuf.fmt = *usrfmt;

		if ((error = audio_track_init_codec(track, &last_dst)) != 0)
			goto error;
		if ((error = audio_track_init_chvol(track, &last_dst)) != 0)
			goto error;
		if ((error = audio_track_init_chmix(track, &last_dst)) != 0)
			goto error;
		if ((error = audio_track_init_freq(track, &last_dst)) != 0)
			goto error;
	}
#if 0
	/* debug */
	if (track->freq.filter) {
		audio_print_format2("freq src", &track->freq.srcbuf.fmt);
		audio_print_format2("freq dst", &track->freq.dst->fmt);
	}
	if (track->chmix.filter) {
		audio_print_format2("chmix src", &track->chmix.srcbuf.fmt);
		audio_print_format2("chmix dst", &track->chmix.dst->fmt);
	}
	if (track->chvol.filter) {
		audio_print_format2("chvol src", &track->chvol.srcbuf.fmt);
		audio_print_format2("chvol dst", &track->chvol.dst->fmt);
	}
	if (track->codec.filter) {
		audio_print_format2("codec src", &track->codec.srcbuf.fmt);
		audio_print_format2("codec dst", &track->codec.dst->fmt);
	}
#endif

	/* Stage input buffer */
	track->input = last_dst;

	/*
	 * On the recording track, make the first stage a ring buffer.
	 * XXX is there a better way?
	 */
	if (audio_track_is_record(track)) {
		track->input->capacity = NBLKOUT *
		    frame_per_block(track->mixer, &track->input->fmt);
		len = auring_bytelen(track->input);
		track->input->mem = audio_realloc(track->input->mem, len);
	}

	/*
	 * Output buffer.
	 * On the playback track, its capacity is NBLKOUT blocks.
	 * On the recording track, its capacity is 1 block.
	 */
	track->outbuf.head = 0;
	track->outbuf.used = 0;
	track->outbuf.capacity = frame_per_block(track->mixer,
	    &track->outbuf.fmt);
	if (audio_track_is_playback(track))
		track->outbuf.capacity *= NBLKOUT;
	len = auring_bytelen(&track->outbuf);
	track->outbuf.mem = audio_realloc(track->outbuf.mem, len);
	if (track->outbuf.mem == NULL) {
		device_printf(sc->sc_dev, "malloc outbuf(%d) failed\n", len);
		error = ENOMEM;
		goto error;
	}

#if defined(AUDIO_DEBUG)
	if (audiodebug >= 3) {
		struct audio_track_debugbuf m;

		memset(&m, 0, sizeof(m));
		snprintf(m.outbuf, sizeof(m.outbuf), " out=%d",
		    track->outbuf.capacity * frametobyte(&track->outbuf.fmt,1));
		if (track->freq.filter)
			snprintf(m.freq, sizeof(m.freq), " freq=%d",
			    track->freq.srcbuf.capacity *
			    frametobyte(&track->freq.srcbuf.fmt, 1));
		if (track->chmix.filter)
			snprintf(m.chmix, sizeof(m.chmix), " chmix=%d",
			    track->chmix.srcbuf.capacity *
			    frametobyte(&track->chmix.srcbuf.fmt, 1));
		if (track->chvol.filter)
			snprintf(m.chvol, sizeof(m.chvol), " chvol=%d",
			    track->chvol.srcbuf.capacity *
			    frametobyte(&track->chvol.srcbuf.fmt, 1));
		if (track->codec.filter)
			snprintf(m.codec, sizeof(m.codec), " codec=%d",
			    track->codec.srcbuf.capacity *
			    frametobyte(&track->codec.srcbuf.fmt, 1));
		snprintf(m.usrbuf, sizeof(m.usrbuf),
		    " usr=%d", track->usrbuf.capacity);

		if (audio_track_is_playback(track)) {
			TRACET(0, track, "bufsize%s%s%s%s%s%s",
			    m.outbuf, m.freq, m.chmix,
			    m.chvol, m.codec, m.usrbuf);
		} else {
			TRACET(0, track, "bufsize%s%s%s%s%s%s",
			    m.freq, m.chmix, m.chvol,
			    m.codec, m.outbuf, m.usrbuf);
		}
	}
#endif
	return 0;

error:
	audio_free_usrbuf(track);
	audio_free(track->codec.srcbuf.mem);
	audio_free(track->chvol.srcbuf.mem);
	audio_free(track->chmix.srcbuf.mem);
	audio_free(track->freq.srcbuf.mem);
	audio_free(track->outbuf.mem);
	return error;
}

/*
 * Fill silence frames (as the internal format) up to 1 block
 * if the ring is not empty and less than 1 block.
 * It returns the number of appended frames.
 */
static int
audio_append_silence(audio_track_t *track, audio_ring_t *ring)
{
	int fpb;
	int n;

	KASSERT(track);
	KASSERT(audio_format2_is_internal(&ring->fmt));

	/* XXX is n correct? */
	/* XXX memset uses frametobyte()? */

	if (ring->used == 0)
		return 0;

	fpb = frame_per_block(track->mixer, &ring->fmt);
	if (ring->used >= fpb)
		return 0;

	n = (ring->capacity - ring->used) % fpb;

	KASSERTMSG(auring_get_contig_free(ring) >= n,
	    "auring_get_contig_free(ring)=%d n=%d",
	    auring_get_contig_free(ring), n);

	memset(auring_tailptr_aint(ring), 0,
	    n * ring->fmt.channels * sizeof(aint_t));
	auring_push(ring, n);
	return n;
}

/*
 * Execute the conversion stage.
 * It prepares arg from this stage and executes stage->filter.
 * It must be called only if stage->filter is not NULL.
 *
 * For stages other than frequency conversion, the function increments
 * src and dst counters here.  For frequency conversion stage, on the
 * other hand, the function does not touch src and dst counters and
 * filter side has to increment them.
 */
static void
audio_apply_stage(audio_track_t *track, audio_stage_t *stage, bool isfreq)
{
	audio_filter_arg_t *arg;
	int srccount;
	int dstcount;
	int count;

	KASSERT(track);
	KASSERT(stage->filter);

	srccount = auring_get_contig_used(&stage->srcbuf);
	dstcount = auring_get_contig_free(stage->dst);

	if (isfreq) {
		KASSERTMSG(srccount > 0, "freq but srccount=%d", srccount);
		count = uimin(dstcount, track->mixer->frames_per_block);
	} else {
		count = uimin(srccount, dstcount);
	}

	if (count > 0) {
		arg = &stage->arg;
		arg->src = auring_headptr(&stage->srcbuf);
		arg->dst = auring_tailptr(stage->dst);
		arg->count = count;

		stage->filter(arg);

		if (!isfreq) {
			auring_take(&stage->srcbuf, count);
			auring_push(stage->dst, count);
		}
	}
}

/*
 * Produce output buffer for playback from user input buffer.
 * It must be called only if usrbuf is not empty and outbuf is
 * available at least one free block.
 */
static void
audio_track_play(audio_track_t *track)
{
	audio_ring_t *usrbuf;
	audio_ring_t *input;
	int count;
	int framesize;
	int bytes;

	KASSERT(track);
	KASSERT(track->lock);
	TRACET(4, track, "start pstate=%d", track->pstate);

	/* At this point usrbuf must not be empty. */
	KASSERT(track->usrbuf.used > 0);
	/* Also, outbuf must be available at least one block. */
	count = auring_get_contig_free(&track->outbuf);
	KASSERTMSG(count >= frame_per_block(track->mixer, &track->outbuf.fmt),
	    "count=%d fpb=%d",
	    count, frame_per_block(track->mixer, &track->outbuf.fmt));

	/* XXX TODO: is this necessary for now? */
	int track_count_0 = track->outbuf.used;

	usrbuf = &track->usrbuf;
	input = track->input;

	/*
	 * framesize is always 1 byte or more since all formats supported as
	 * usrfmt(=input) have 8bit or more stride.
	 */
	framesize = frametobyte(&input->fmt, 1);
	KASSERT(framesize >= 1);

	/* The next stage of usrbuf (=input) must be available. */
	KASSERT(auring_get_contig_free(input) > 0);

	/*
	 * Copy usrbuf up to 1block to input buffer.
	 * count is the number of frames to copy from usrbuf.
	 * bytes is the number of bytes to copy from usrbuf.  However it is
	 * not copied less than one frame.
	 */
	count = uimin(usrbuf->used, track->usrbuf_blksize) / framesize;
	bytes = count * framesize;

	track->usrbuf_stamp += bytes;

	if (usrbuf->head + bytes < usrbuf->capacity) {
		memcpy((uint8_t *)input->mem + auring_tail(input) * framesize,
		    (uint8_t *)usrbuf->mem + usrbuf->head,
		    bytes);
		auring_push(input, count);
		auring_take(usrbuf, bytes);
	} else {
		int bytes1;
		int bytes2;

		bytes1 = auring_get_contig_used(usrbuf);
		KASSERTMSG(bytes1 % framesize == 0,
		    "bytes1=%d framesize=%d", bytes1, framesize);
		memcpy((uint8_t *)input->mem + auring_tail(input) * framesize,
		    (uint8_t *)usrbuf->mem + usrbuf->head,
		    bytes1);
		auring_push(input, bytes1 / framesize);
		auring_take(usrbuf, bytes1);

		bytes2 = bytes - bytes1;
		memcpy((uint8_t *)input->mem + auring_tail(input) * framesize,
		    (uint8_t *)usrbuf->mem + usrbuf->head,
		    bytes2);
		auring_push(input, bytes2 / framesize);
		auring_take(usrbuf, bytes2);
	}

	/* Encoding conversion */
	if (track->codec.filter)
		audio_apply_stage(track, &track->codec, false);

	/* Channel volume */
	if (track->chvol.filter)
		audio_apply_stage(track, &track->chvol, false);

	/* Channel mix */
	if (track->chmix.filter)
		audio_apply_stage(track, &track->chmix, false);

	/* Frequency conversion */
	/*
	 * Since the frequency conversion needs correction for each block,
	 * it rounds up to 1 block.
	 */
	if (track->freq.filter) {
		int n;
		n = audio_append_silence(track, &track->freq.srcbuf);
		if (n > 0) {
			TRACET(4, track,
			    "freq.srcbuf add silence %d -> %d/%d/%d",
			    n,
			    track->freq.srcbuf.head,
			    track->freq.srcbuf.used,
			    track->freq.srcbuf.capacity);
		}
		if (track->freq.srcbuf.used > 0) {
			audio_apply_stage(track, &track->freq, true);
		}
	}

	if (bytes < track->usrbuf_blksize) {
		/*
		 * Clear all conversion buffer pointer if the conversion was
		 * not exactly one block.  These conversion stage buffers are
		 * certainly circular buffers because of symmetry with the
		 * previous and next stage buffer.  However, since they are
		 * treated as simple contiguous buffers in operation, so head
		 * always should point 0.  This may happen during drain-age.
		 */
		TRACET(4, track, "reset stage");
		if (track->codec.filter) {
			KASSERT(track->codec.srcbuf.used == 0);
			track->codec.srcbuf.head = 0;
		}
		if (track->chvol.filter) {
			KASSERT(track->chvol.srcbuf.used == 0);
			track->chvol.srcbuf.head = 0;
		}
		if (track->chmix.filter) {
			KASSERT(track->chmix.srcbuf.used == 0);
			track->chmix.srcbuf.head = 0;
		}
		if (track->freq.filter) {
			KASSERT(track->freq.srcbuf.used == 0);
			track->freq.srcbuf.head = 0;
		}
	}

	if (track->input == &track->outbuf) {
		track->outputcounter = track->inputcounter;
	} else {
		track->outputcounter += track->outbuf.used - track_count_0;
	}

#if defined(AUDIO_DEBUG)
	if (audiodebug >= 3) {
		struct audio_track_debugbuf m;
		audio_track_bufstat(track, &m);
		TRACET(0, track, "end%s%s%s%s%s%s",
		    m.outbuf, m.freq, m.chvol, m.chmix, m.codec, m.usrbuf);
	}
#endif
}

/*
 * Produce user output buffer for recording from input buffer.
 */
static void
audio_track_record(audio_track_t *track)
{
	audio_ring_t *outbuf;
	audio_ring_t *usrbuf;
	int count;
	int bytes;
	int framesize;

	KASSERT(track);
	KASSERT(track->lock);

	if (auring_get_contig_used(track->input) == 0) {
		TRACET(4, track, "input->used == 0");
		return;
	}

	/* Frequency conversion */
	if (track->freq.filter) {
		if (track->freq.srcbuf.used > 0) {
			audio_apply_stage(track, &track->freq, true);
			/* XXX should input of freq be from beginning of buf? */
		}
	}

	/* Channel mix */
	if (track->chmix.filter)
		audio_apply_stage(track, &track->chmix, false);

	/* Channel volume */
	if (track->chvol.filter)
		audio_apply_stage(track, &track->chvol, false);

	/* Encoding conversion */
	if (track->codec.filter)
		audio_apply_stage(track, &track->codec, false);

	/* Copy outbuf to usrbuf */
	outbuf = &track->outbuf;
	usrbuf = &track->usrbuf;
	/* usrbuf must have at least one free block. */
	KASSERT(usrbuf->used <= track->usrbuf_usedhigh - track->usrbuf_blksize);
	/*
	 * framesize is always 1 byte or more since all formats supported
	 * as usrfmt(=output) have 8bit or more stride.
	 */
	framesize = frametobyte(&outbuf->fmt, 1);
	KASSERT(framesize >= 1);
	/*
	 * count is the number of frames to copy to usrbuf.
	 * bytes is the number of bytes to copy to usrbuf.
	 */
	count = outbuf->used;
	count = uimin(count, track->usrbuf_blksize / framesize);
	bytes = count * framesize;
	if (auring_tail(usrbuf) + bytes < usrbuf->capacity) {
		memcpy((uint8_t *)usrbuf->mem + auring_tail(usrbuf),
		    (uint8_t *)outbuf->mem + outbuf->head * framesize,
		    bytes);
		auring_push(usrbuf, bytes);
		auring_take(outbuf, count);
	} else {
		int bytes1;
		int bytes2;

		bytes1 = auring_get_contig_free(usrbuf);
		KASSERTMSG(bytes1 % framesize == 0,
		    "bytes1=%d framesize=%d", bytes1, framesize);
		memcpy((uint8_t *)usrbuf->mem + auring_tail(usrbuf),
		    (uint8_t *)outbuf->mem + outbuf->head * framesize,
		    bytes1);
		auring_push(usrbuf, bytes1);
		auring_take(outbuf, bytes1 / framesize);

		bytes2 = bytes - bytes1;
		memcpy((uint8_t *)usrbuf->mem + auring_tail(usrbuf),
		    (uint8_t *)outbuf->mem + outbuf->head * framesize,
		    bytes2);
		auring_push(usrbuf, bytes2);
		auring_take(outbuf, bytes2 / framesize);
	}

	/* XXX TODO: any counters here? */

#if defined(AUDIO_DEBUG)
	if (audiodebug >= 3) {
		struct audio_track_debugbuf m;
		audio_track_bufstat(track, &m);
		TRACET(0, track, "end%s%s%s%s%s%s",
		    m.freq, m.chvol, m.chmix, m.codec, m.outbuf, m.usrbuf);
	}
#endif
}

/*
 * Calculate blktime [msec] from mixer(.hwbuf.fmt).
 * Must be called with sc_exlock held.
 */
static u_int
audio_mixer_calc_blktime(struct audio_softc *sc, audio_trackmixer_t *mixer)
{
	audio_format2_t *fmt;
	u_int blktime;
	u_int frames_per_block;

	KASSERT(sc->sc_exlock);

	fmt = &mixer->hwbuf.fmt;
	blktime = sc->sc_blk_ms;

	/*
	 * If stride is not multiples of 8, special treatment is necessary.
	 * For now, it is only x68k's vs(4), 4 bit/sample ADPCM.
	 */
	if (fmt->stride == 4) {
		frames_per_block = fmt->sample_rate * blktime / 1000;
		if ((frames_per_block & 1) != 0)
			blktime *= 2;
	}
#ifdef DIAGNOSTIC
	else if (fmt->stride % NBBY != 0) {
		panic("unsupported HW stride %d", fmt->stride);
	}
#endif

	return blktime;
}

/*
 * Initialize the mixer corresponding to the mode.
 * Set AUMODE_PLAY to the 'mode' for playback or AUMODE_RECORD for recording.
 * sc->sc_[pr]mixer (corresponding to the 'mode') must be zero-filled.
 * This function returns 0 on successful.  Otherwise returns errno.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static int
audio_mixer_init(struct audio_softc *sc, int mode,
	const audio_format2_t *hwfmt, const audio_filter_reg_t *reg)
{
	char codecbuf[64];
	char blkdmsbuf[8];
	audio_trackmixer_t *mixer;
	void (*softint_handler)(void *);
	int len;
	int blksize;
	int capacity;
	size_t bufsize;
	int hwblks;
	int blkms;
	int blkdms;
	int error;

	KASSERT(hwfmt != NULL);
	KASSERT(reg != NULL);
	KASSERT(sc->sc_exlock);

	error = 0;
	if (mode == AUMODE_PLAY)
		mixer = sc->sc_pmixer;
	else
		mixer = sc->sc_rmixer;

	mixer->sc = sc;
	mixer->mode = mode;

	mixer->hwbuf.fmt = *hwfmt;
	mixer->volume = 256;
	mixer->blktime_d = 1000;
	mixer->blktime_n = audio_mixer_calc_blktime(sc, mixer);
	sc->sc_blk_ms = mixer->blktime_n;
	hwblks = NBLKHW;

	mixer->frames_per_block = frame_per_block(mixer, &mixer->hwbuf.fmt);
	blksize = frametobyte(&mixer->hwbuf.fmt, mixer->frames_per_block);
	if (sc->hw_if->round_blocksize) {
		int rounded;
		audio_params_t p = format2_to_params(&mixer->hwbuf.fmt);
		mutex_enter(sc->sc_lock);
		rounded = sc->hw_if->round_blocksize(sc->hw_hdl, blksize,
		    mode, &p);
		mutex_exit(sc->sc_lock);
		TRACE(1, "round_blocksize %d -> %d", blksize, rounded);
		if (rounded != blksize) {
			if ((rounded * NBBY) % (mixer->hwbuf.fmt.stride *
			    mixer->hwbuf.fmt.channels) != 0) {
				audio_printf(sc,
				    "round_blocksize returned blocksize "
				    "indivisible by framesize: "
				    "blksize=%d rounded=%d "
				    "stride=%ubit channels=%u\n",
				    blksize, rounded,
				    mixer->hwbuf.fmt.stride,
				    mixer->hwbuf.fmt.channels);
				return EINVAL;
			}
			/* Recalculation */
			blksize = rounded;
			mixer->frames_per_block = blksize * NBBY /
			    (mixer->hwbuf.fmt.stride *
			     mixer->hwbuf.fmt.channels);
		}
	}
	mixer->blktime_n = mixer->frames_per_block;
	mixer->blktime_d = mixer->hwbuf.fmt.sample_rate;

	capacity = mixer->frames_per_block * hwblks;
	bufsize = frametobyte(&mixer->hwbuf.fmt, capacity);
	if (sc->hw_if->round_buffersize) {
		size_t rounded;
		mutex_enter(sc->sc_lock);
		rounded = sc->hw_if->round_buffersize(sc->hw_hdl, mode,
		    bufsize);
		mutex_exit(sc->sc_lock);
		TRACE(1, "round_buffersize %zd -> %zd", bufsize, rounded);
		if (rounded < bufsize) {
			/* buffersize needs NBLKHW blocks at least. */
			audio_printf(sc,
			    "round_buffersize returned too small buffersize: "
			    "buffersize=%zd blksize=%d\n",
			    rounded, blksize);
			return EINVAL;
		}
		if (rounded % blksize != 0) {
			/* buffersize/blksize constraint mismatch? */
			audio_printf(sc,
			    "round_buffersize returned buffersize indivisible "
			    "by blksize: buffersize=%zu blksize=%d\n",
			    rounded, blksize);
			return EINVAL;
		}
		if (rounded != bufsize) {
			/* Recalculation */
			bufsize = rounded;
			hwblks = bufsize / blksize;
			capacity = mixer->frames_per_block * hwblks;
		}
	}
	TRACE(1, "buffersize for %s = %zu",
	    (mode == AUMODE_PLAY) ? "playback" : "recording",
	    bufsize);
	mixer->hwbuf.capacity = capacity;

	if (sc->hw_if->allocm) {
		/* sc_lock is not necessary for allocm */
		mixer->hwbuf.mem = sc->hw_if->allocm(sc->hw_hdl, mode, bufsize);
		if (mixer->hwbuf.mem == NULL) {
			audio_printf(sc, "allocm(%zu) failed\n", bufsize);
			return ENOMEM;
		}
	} else {
		mixer->hwbuf.mem = kmem_alloc(bufsize, KM_SLEEP);
	}

	/* From here, audio_mixer_destroy is necessary to exit. */
	if (mode == AUMODE_PLAY) {
		cv_init(&mixer->outcv, "audiowr");
	} else {
		cv_init(&mixer->outcv, "audiord");
	}

	if (mode == AUMODE_PLAY) {
		softint_handler = audio_softintr_wr;
	} else {
		softint_handler = audio_softintr_rd;
	}
	mixer->sih = softint_establish(SOFTINT_SERIAL | SOFTINT_MPSAFE,
	    softint_handler, sc);
	if (mixer->sih == NULL) {
		device_printf(sc->sc_dev, "softint_establish failed\n");
		goto abort;
	}

	mixer->track_fmt.encoding = AUDIO_ENCODING_SLINEAR_NE;
	mixer->track_fmt.precision = AUDIO_INTERNAL_BITS;
	mixer->track_fmt.stride = AUDIO_INTERNAL_BITS;
	mixer->track_fmt.channels = mixer->hwbuf.fmt.channels;
	mixer->track_fmt.sample_rate = mixer->hwbuf.fmt.sample_rate;

	if (mixer->hwbuf.fmt.encoding == AUDIO_ENCODING_SLINEAR_OE &&
	    mixer->hwbuf.fmt.precision == AUDIO_INTERNAL_BITS) {
		mixer->swap_endian = true;
		TRACE(1, "swap_endian");
	}

	if (mode == AUMODE_PLAY) {
		/* Mixing buffer */
		mixer->mixfmt = mixer->track_fmt;
		mixer->mixfmt.precision *= 2;
		mixer->mixfmt.stride *= 2;
		/* XXX TODO: use some macros? */
		len = mixer->frames_per_block * mixer->mixfmt.channels *
		    mixer->mixfmt.stride / NBBY;
		mixer->mixsample = audio_realloc(mixer->mixsample, len);
	} else {
		/* No mixing buffer for recording */
	}

	if (reg->codec) {
		mixer->codec = reg->codec;
		mixer->codecarg.context = reg->context;
		if (mode == AUMODE_PLAY) {
			mixer->codecarg.srcfmt = &mixer->track_fmt;
			mixer->codecarg.dstfmt = &mixer->hwbuf.fmt;
		} else {
			mixer->codecarg.srcfmt = &mixer->hwbuf.fmt;
			mixer->codecarg.dstfmt = &mixer->track_fmt;
		}
		mixer->codecbuf.fmt = mixer->track_fmt;
		mixer->codecbuf.capacity = mixer->frames_per_block;
		len = auring_bytelen(&mixer->codecbuf);
		mixer->codecbuf.mem = audio_realloc(mixer->codecbuf.mem, len);
		if (mixer->codecbuf.mem == NULL) {
			device_printf(sc->sc_dev,
			    "malloc codecbuf(%d) failed\n", len);
			error = ENOMEM;
			goto abort;
		}
	}

	/* Succeeded so display it. */
	codecbuf[0] = '\0';
	if (mixer->codec || mixer->swap_endian) {
		snprintf(codecbuf, sizeof(codecbuf), " %s %s:%d",
		    (mode == AUMODE_PLAY) ? "->" : "<-",
		    audio_encoding_name(mixer->hwbuf.fmt.encoding),
		    mixer->hwbuf.fmt.precision);
	}
	blkms = mixer->blktime_n * 1000 / mixer->blktime_d;
	blkdms = (mixer->blktime_n * 10000 / mixer->blktime_d) % 10;
	blkdmsbuf[0] = '\0';
	if (blkdms != 0) {
		snprintf(blkdmsbuf, sizeof(blkdmsbuf), ".%1d", blkdms);
	}
	aprint_normal_dev(sc->sc_dev,
	    "%s:%d%s %dch %dHz, blk %d bytes (%d%sms) for %s\n",
	    audio_encoding_name(mixer->track_fmt.encoding),
	    mixer->track_fmt.precision,
	    codecbuf,
	    mixer->track_fmt.channels,
	    mixer->track_fmt.sample_rate,
	    blksize,
	    blkms, blkdmsbuf,
	    (mode == AUMODE_PLAY) ? "playback" : "recording");

	return 0;

abort:
	audio_mixer_destroy(sc, mixer);
	return error;
}

/*
 * Releases all resources of 'mixer'.
 * Note that it does not release the memory area of 'mixer' itself.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static void
audio_mixer_destroy(struct audio_softc *sc, audio_trackmixer_t *mixer)
{
	int bufsize;

	KASSERT(sc->sc_exlock == 1);

	bufsize = frametobyte(&mixer->hwbuf.fmt, mixer->hwbuf.capacity);

	if (mixer->hwbuf.mem != NULL) {
		if (sc->hw_if->freem) {
			/* sc_lock is not necessary for freem */
			sc->hw_if->freem(sc->hw_hdl, mixer->hwbuf.mem, bufsize);
		} else {
			kmem_free(mixer->hwbuf.mem, bufsize);
		}
		mixer->hwbuf.mem = NULL;
	}

	audio_free(mixer->codecbuf.mem);
	audio_free(mixer->mixsample);

	cv_destroy(&mixer->outcv);

	if (mixer->sih) {
		softint_disestablish(mixer->sih);
		mixer->sih = NULL;
	}
}

/*
 * Starts playback mixer.
 * Must be called only if sc_pbusy is false.
 * Must be called with sc_lock && sc_exlock held.
 * Must not be called from the interrupt context.
 */
static void
audio_pmixer_start(struct audio_softc *sc, bool force)
{
	audio_trackmixer_t *mixer;
	int minimum;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);
	KASSERT(sc->sc_pbusy == false);

	mutex_enter(sc->sc_intr_lock);

	mixer = sc->sc_pmixer;
	TRACE(2, "%smixseq=%d hwseq=%d hwbuf=%d/%d/%d%s",
	    (audiodebug >= 3) ? "begin " : "",
	    (int)mixer->mixseq, (int)mixer->hwseq,
	    mixer->hwbuf.head, mixer->hwbuf.used, mixer->hwbuf.capacity,
	    force ? " force" : "");

	/* Need two blocks to start normally. */
	minimum = (force) ? 1 : 2;
	while (mixer->hwbuf.used < mixer->frames_per_block * minimum) {
		audio_pmixer_process(sc);
	}

	/* Start output */
	audio_pmixer_output(sc);
	sc->sc_pbusy = true;

	TRACE(3, "end   mixseq=%d hwseq=%d hwbuf=%d/%d/%d",
	    (int)mixer->mixseq, (int)mixer->hwseq,
	    mixer->hwbuf.head, mixer->hwbuf.used, mixer->hwbuf.capacity);

	mutex_exit(sc->sc_intr_lock);
}

/*
 * When playing back with MD filter:
 *
 *           track track ...
 *               v v
 *                +  mix (with aint2_t)
 *                |  master volume (with aint2_t)
 *                v
 *    mixsample [::::]                  wide-int 1 block (ring) buffer
 *                |
 *                |  convert aint2_t -> aint_t
 *                v
 *    codecbuf  [....]                  1 block (ring) buffer
 *                |
 *                |  convert to hw format
 *                v
 *    hwbuf     [............]          NBLKHW blocks ring buffer
 *
 * When playing back without MD filter:
 *
 *    mixsample [::::]                  wide-int 1 block (ring) buffer
 *                |
 *                |  convert aint2_t -> aint_t
 *                |  (with byte swap if necessary)
 *                v
 *    hwbuf     [............]          NBLKHW blocks ring buffer
 *
 * mixsample: slinear_NE, wide internal precision, HW ch, HW freq.
 * codecbuf:  slinear_NE, internal precision,      HW ch, HW freq.
 * hwbuf:     HW encoding, HW precision,           HW ch, HW freq.
 */

/*
 * Performs track mixing and converts it to hwbuf.
 * Note that this function doesn't transfer hwbuf to hardware.
 * Must be called with sc_intr_lock held.
 */
static void
audio_pmixer_process(struct audio_softc *sc)
{
	audio_trackmixer_t *mixer;
	audio_file_t *f;
	int frame_count;
	int sample_count;
	int mixed;
	int i;
	aint2_t *m;
	aint_t *h;

	mixer = sc->sc_pmixer;

	frame_count = mixer->frames_per_block;
	KASSERTMSG(auring_get_contig_free(&mixer->hwbuf) >= frame_count,
	    "auring_get_contig_free()=%d frame_count=%d",
	    auring_get_contig_free(&mixer->hwbuf), frame_count);
	sample_count = frame_count * mixer->mixfmt.channels;

	mixer->mixseq++;

	/* Mix all tracks */
	mixed = 0;
	SLIST_FOREACH(f, &sc->sc_files, entry) {
		audio_track_t *track = f->ptrack;

		if (track == NULL)
			continue;

		if (track->is_pause) {
			TRACET(4, track, "skip; paused");
			continue;
		}

		/* Skip if the track is used by process context. */
		if (audio_track_lock_tryenter(track) == false) {
			TRACET(4, track, "skip; in use");
			continue;
		}

		/* Emulate mmap'ped track */
		if (track->mmapped) {
			auring_push(&track->usrbuf, track->usrbuf_blksize);
			TRACET(4, track, "mmap; usr=%d/%d/C%d",
			    track->usrbuf.head,
			    track->usrbuf.used,
			    track->usrbuf.capacity);
		}

		if (track->outbuf.used < mixer->frames_per_block &&
		    track->usrbuf.used > 0) {
			TRACET(4, track, "process");
			audio_track_play(track);
		}

		if (track->outbuf.used > 0) {
			mixed = audio_pmixer_mix_track(mixer, track, mixed);
		} else {
			TRACET(4, track, "skip; empty");
		}

		audio_track_lock_exit(track);
	}

	if (mixed == 0) {
		/* Silence */
		memset(mixer->mixsample, 0,
		    frametobyte(&mixer->mixfmt, frame_count));
	} else {
		if (mixed > 1) {
			/* If there are multiple tracks, do auto gain control */
			audio_pmixer_agc(mixer, sample_count);
		}

		/* Apply master volume */
		if (mixer->volume < 256) {
			m = mixer->mixsample;
			for (i = 0; i < sample_count; i++) {
				*m = AUDIO_SCALEDOWN(*m * mixer->volume, 8);
				m++;
			}

			/*
			 * Recover the volume gradually at the pace of
			 * several times per second.  If it's too fast, you
			 * can recognize that the volume changes up and down
			 * quickly and it's not so comfortable.
			 */
			mixer->voltimer += mixer->blktime_n;
			if (mixer->voltimer * 4 >= mixer->blktime_d) {
				mixer->volume++;
				mixer->voltimer = 0;
#if defined(AUDIO_DEBUG_AGC)
				TRACE(1, "volume recover: %d", mixer->volume);
#endif
			}
		}
	}

	/*
	 * The rest is the hardware part.
	 */

	if (mixer->codec) {
		h = auring_tailptr_aint(&mixer->codecbuf);
	} else {
		h = auring_tailptr_aint(&mixer->hwbuf);
	}

	m = mixer->mixsample;
	if (mixer->swap_endian) {
		for (i = 0; i < sample_count; i++) {
			*h++ = bswap16(*m++);
		}
	} else {
		for (i = 0; i < sample_count; i++) {
			*h++ = *m++;
		}
	}

	/* Hardware driver's codec */
	if (mixer->codec) {
		auring_push(&mixer->codecbuf, frame_count);
		mixer->codecarg.src = auring_headptr(&mixer->codecbuf);
		mixer->codecarg.dst = auring_tailptr(&mixer->hwbuf);
		mixer->codecarg.count = frame_count;
		mixer->codec(&mixer->codecarg);
		auring_take(&mixer->codecbuf, mixer->codecarg.count);
	}

	auring_push(&mixer->hwbuf, frame_count);

	TRACE(4, "done mixseq=%d hwbuf=%d/%d/%d%s",
	    (int)mixer->mixseq,
	    mixer->hwbuf.head, mixer->hwbuf.used, mixer->hwbuf.capacity,
	    (mixed == 0) ? " silent" : "");
}

/*
 * Do auto gain control.
 * Must be called sc_intr_lock held.
 */
static void
audio_pmixer_agc(audio_trackmixer_t *mixer, int sample_count)
{
	struct audio_softc *sc __unused;
	aint2_t val;
	aint2_t maxval;
	aint2_t minval;
	aint2_t over_plus;
	aint2_t over_minus;
	aint2_t *m;
	int newvol;
	int i;

	sc = mixer->sc;

	/* Overflow detection */
	maxval = AINT_T_MAX;
	minval = AINT_T_MIN;
	m = mixer->mixsample;
	for (i = 0; i < sample_count; i++) {
		val = *m++;
		if (val > maxval)
			maxval = val;
		else if (val < minval)
			minval = val;
	}

	/* Absolute value of overflowed amount */
	over_plus = maxval - AINT_T_MAX;
	over_minus = AINT_T_MIN - minval;

	if (over_plus > 0 || over_minus > 0) {
		if (over_plus > over_minus) {
			newvol = (int)((aint2_t)AINT_T_MAX * 256 / maxval);
		} else {
			newvol = (int)((aint2_t)AINT_T_MIN * 256 / minval);
		}

		/*
		 * Change the volume only if new one is smaller.
		 * Reset the timer even if the volume isn't changed.
		 */
		if (newvol <= mixer->volume) {
			mixer->volume = newvol;
			mixer->voltimer = 0;
#if defined(AUDIO_DEBUG_AGC)
			TRACE(1, "auto volume adjust: %d", mixer->volume);
#endif
		}
	}
}

/*
 * Mix one track.
 * 'mixed' specifies the number of tracks mixed so far.
 * It returns the number of tracks mixed.  In other words, it returns
 * mixed + 1 if this track is mixed.
 */
static int
audio_pmixer_mix_track(audio_trackmixer_t *mixer, audio_track_t *track,
	int mixed)
{
	int count;
	int sample_count;
	int remain;
	int i;
	const aint_t *s;
	aint2_t *d;

	/* XXX TODO: Is this necessary for now? */
	if (mixer->mixseq < track->seq)
		return mixed;

	count = auring_get_contig_used(&track->outbuf);
	count = uimin(count, mixer->frames_per_block);

	s = auring_headptr_aint(&track->outbuf);
	d = mixer->mixsample;

	/*
	 * Apply track volume with double-sized integer and perform
	 * additive synthesis.
	 *
	 * XXX If you limit the track volume to 1.0 or less (<= 256),
	 *     it would be better to do this in the track conversion stage
	 *     rather than here.  However, if you accept the volume to
	 *     be greater than 1.0 (> 256), it's better to do it here.
	 *     Because the operation here is done by double-sized integer.
	 */
	sample_count = count * mixer->mixfmt.channels;
	if (mixed == 0) {
		/* If this is the first track, assignment can be used. */
#if defined(AUDIO_SUPPORT_TRACK_VOLUME)
		if (track->volume != 256) {
			for (i = 0; i < sample_count; i++) {
				aint2_t v;
				v = *s++;
				*d++ = AUDIO_SCALEDOWN(v * track->volume, 8)
			}
		} else
#endif
		{
			for (i = 0; i < sample_count; i++) {
				*d++ = ((aint2_t)*s++);
			}
		}
		/* Fill silence if the first track is not filled. */
		for (; i < mixer->frames_per_block * mixer->mixfmt.channels; i++)
			*d++ = 0;
	} else {
		/* If this is the second or later, add it. */
#if defined(AUDIO_SUPPORT_TRACK_VOLUME)
		if (track->volume != 256) {
			for (i = 0; i < sample_count; i++) {
				aint2_t v;
				v = *s++;
				*d++ += AUDIO_SCALEDOWN(v * track->volume, 8);
			}
		} else
#endif
		{
			for (i = 0; i < sample_count; i++) {
				*d++ += ((aint2_t)*s++);
			}
		}
	}

	auring_take(&track->outbuf, count);
	/*
	 * The counters have to align block even if outbuf is less than
	 * one block. XXX Is this still necessary?
	 */
	remain = mixer->frames_per_block - count;
	if (__predict_false(remain != 0)) {
		auring_push(&track->outbuf, remain);
		auring_take(&track->outbuf, remain);
	}

	/*
	 * Update track sequence.
	 * mixseq has previous value yet at this point.
	 */
	track->seq = mixer->mixseq + 1;

	return mixed + 1;
}

/*
 * Output one block from hwbuf to HW.
 * Must be called with sc_intr_lock held.
 */
static void
audio_pmixer_output(struct audio_softc *sc)
{
	audio_trackmixer_t *mixer;
	audio_params_t params;
	void *start;
	void *end;
	int blksize;
	int error;

	mixer = sc->sc_pmixer;
	TRACE(4, "pbusy=%d hwbuf=%d/%d/%d",
	    sc->sc_pbusy,
	    mixer->hwbuf.head, mixer->hwbuf.used, mixer->hwbuf.capacity);
	KASSERTMSG(mixer->hwbuf.used >= mixer->frames_per_block,
	    "mixer->hwbuf.used=%d mixer->frames_per_block=%d",
	    mixer->hwbuf.used, mixer->frames_per_block);

	blksize = frametobyte(&mixer->hwbuf.fmt, mixer->frames_per_block);

	if (sc->hw_if->trigger_output) {
		/* trigger (at once) */
		if (!sc->sc_pbusy) {
			start = mixer->hwbuf.mem;
			end = (uint8_t *)start + auring_bytelen(&mixer->hwbuf);
			params = format2_to_params(&mixer->hwbuf.fmt);

			error = sc->hw_if->trigger_output(sc->hw_hdl,
			    start, end, blksize, audio_pintr, sc, &params);
			if (error) {
				audio_printf(sc,
				    "trigger_output failed: errno=%d\n",
				    error);
				return;
			}
		}
	} else {
		/* start (everytime) */
		start = auring_headptr(&mixer->hwbuf);

		error = sc->hw_if->start_output(sc->hw_hdl,
		    start, blksize, audio_pintr, sc);
		if (error) {
			audio_printf(sc,
			    "start_output failed: errno=%d\n", error);
			return;
		}
	}
}

/*
 * This is an interrupt handler for playback.
 * It is called with sc_intr_lock held.
 *
 * It is usually called from hardware interrupt.  However, note that
 * for some drivers (e.g. uaudio) it is called from software interrupt.
 */
static void
audio_pintr(void *arg)
{
	struct audio_softc *sc;
	audio_trackmixer_t *mixer;

	sc = arg;
	KASSERT(mutex_owned(sc->sc_intr_lock));

	if (sc->sc_dying)
		return;
	if (sc->sc_pbusy == false) {
#if defined(DIAGNOSTIC)
		audio_printf(sc, "DIAGNOSTIC: %s raised stray interrupt\n",
		    device_xname(sc->hw_dev));
#endif
		return;
	}

	mixer = sc->sc_pmixer;
	mixer->hw_complete_counter += mixer->frames_per_block;
	mixer->hwseq++;

	auring_take(&mixer->hwbuf, mixer->frames_per_block);

	TRACE(4,
	    "HW_INT ++hwseq=%" PRIu64 " cmplcnt=%" PRIu64 " hwbuf=%d/%d/%d",
	    mixer->hwseq, mixer->hw_complete_counter,
	    mixer->hwbuf.head, mixer->hwbuf.used, mixer->hwbuf.capacity);

#if defined(AUDIO_HW_SINGLE_BUFFER)
	/*
	 * Create a new block here and output it immediately.
	 * It makes a latency lower but needs machine power.
	 */
	audio_pmixer_process(sc);
	audio_pmixer_output(sc);
#else
	/*
	 * It is called when block N output is done.
	 * Output immediately block N+1 created by the last interrupt.
	 * And then create block N+2 for the next interrupt.
	 * This method makes playback robust even on slower machines.
	 * Instead the latency is increased by one block.
	 */

	/* At first, output ready block. */
	if (mixer->hwbuf.used >= mixer->frames_per_block) {
		audio_pmixer_output(sc);
	}

	bool later = false;

	if (mixer->hwbuf.used < mixer->frames_per_block) {
		later = true;
	}

	/* Then, process next block. */
	audio_pmixer_process(sc);

	if (later) {
		audio_pmixer_output(sc);
	}
#endif

	/*
	 * When this interrupt is the real hardware interrupt, disabling
	 * preemption here is not necessary.  But some drivers (e.g. uaudio)
	 * emulate it by software interrupt, so kpreempt_disable is necessary.
	 */
	kpreempt_disable();
	softint_schedule(mixer->sih);
	kpreempt_enable();
}

/*
 * Starts record mixer.
 * Must be called only if sc_rbusy is false.
 * Must be called with sc_lock && sc_exlock held.
 * Must not be called from the interrupt context.
 */
static void
audio_rmixer_start(struct audio_softc *sc)
{

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);
	KASSERT(sc->sc_rbusy == false);

	mutex_enter(sc->sc_intr_lock);

	TRACE(2, "%s", (audiodebug >= 3) ? "begin" : "");
	audio_rmixer_input(sc);
	sc->sc_rbusy = true;
	TRACE(3, "end");

	mutex_exit(sc->sc_intr_lock);
}

/*
 * When recording with MD filter:
 *
 *    hwbuf     [............]          NBLKHW blocks ring buffer
 *                |
 *                | convert from hw format
 *                v
 *    codecbuf  [....]                  1 block (ring) buffer
 *               |  |
 *               v  v
 *            track track ...
 *
 * When recording without MD filter:
 *
 *    hwbuf     [............]          NBLKHW blocks ring buffer
 *               |  |
 *               v  v
 *            track track ...
 *
 * hwbuf:     HW encoding, HW precision, HW ch, HW freq.
 * codecbuf:  slinear_NE, internal precision, HW ch, HW freq.
 */

/*
 * Distribute a recorded block to all recording tracks.
 */
static void
audio_rmixer_process(struct audio_softc *sc)
{
	audio_trackmixer_t *mixer;
	audio_ring_t *mixersrc;
	audio_file_t *f;
	aint_t *p;
	int count;
	int bytes;
	int i;

	mixer = sc->sc_rmixer;

	/*
	 * count is the number of frames to be retrieved this time.
	 * count should be one block.
	 */
	count = auring_get_contig_used(&mixer->hwbuf);
	count = uimin(count, mixer->frames_per_block);
	if (count <= 0) {
		TRACE(4, "count %d: too short", count);
		return;
	}
	bytes = frametobyte(&mixer->track_fmt, count);

	/* Hardware driver's codec */
	if (mixer->codec) {
		mixer->codecarg.src = auring_headptr(&mixer->hwbuf);
		mixer->codecarg.dst = auring_tailptr(&mixer->codecbuf);
		mixer->codecarg.count = count;
		mixer->codec(&mixer->codecarg);
		auring_take(&mixer->hwbuf, mixer->codecarg.count);
		auring_push(&mixer->codecbuf, mixer->codecarg.count);
		mixersrc = &mixer->codecbuf;
	} else {
		mixersrc = &mixer->hwbuf;
	}

	if (mixer->swap_endian) {
		/* inplace conversion */
		p = auring_headptr_aint(mixersrc);
		for (i = 0; i < count * mixer->track_fmt.channels; i++, p++) {
			*p = bswap16(*p);
		}
	}

	/* Distribute to all tracks. */
	SLIST_FOREACH(f, &sc->sc_files, entry) {
		audio_track_t *track = f->rtrack;
		audio_ring_t *input;

		if (track == NULL)
			continue;

		if (track->is_pause) {
			TRACET(4, track, "skip; paused");
			continue;
		}

		if (audio_track_lock_tryenter(track) == false) {
			TRACET(4, track, "skip; in use");
			continue;
		}

		/*
		 * If the track buffer has less than one block of free space,
		 * make one block free.
		 */
		input = track->input;
		if (input->capacity - input->used < mixer->frames_per_block) {
			int drops = mixer->frames_per_block -
			    (input->capacity - input->used);
			track->dropframes += drops;
			TRACET(4, track, "drop %d frames: inp=%d/%d/%d",
			    drops,
			    input->head, input->used, input->capacity);
			auring_take(input, drops);
		}

		KASSERTMSG(auring_tail(input) % mixer->frames_per_block == 0,
		    "inputtail=%d mixer->frames_per_block=%d",
		    auring_tail(input), mixer->frames_per_block);
		memcpy(auring_tailptr_aint(input),
		    auring_headptr_aint(mixersrc),
		    bytes);
		auring_push(input, count);

		/* XXX sequence counter? */

		audio_track_lock_exit(track);
	}

	auring_take(mixersrc, count);
}

/*
 * Input one block from HW to hwbuf.
 * Must be called with sc_intr_lock held.
 */
static void
audio_rmixer_input(struct audio_softc *sc)
{
	audio_trackmixer_t *mixer;
	audio_params_t params;
	void *start;
	void *end;
	int blksize;
	int error;

	mixer = sc->sc_rmixer;
	blksize = frametobyte(&mixer->hwbuf.fmt, mixer->frames_per_block);

	if (sc->hw_if->trigger_input) {
		/* trigger (at once) */
		if (!sc->sc_rbusy) {
			start = mixer->hwbuf.mem;
			end = (uint8_t *)start + auring_bytelen(&mixer->hwbuf);
			params = format2_to_params(&mixer->hwbuf.fmt);

			error = sc->hw_if->trigger_input(sc->hw_hdl,
			    start, end, blksize, audio_rintr, sc, &params);
			if (error) {
				audio_printf(sc,
				    "trigger_input failed: errno=%d\n",
				    error);
				return;
			}
		}
	} else {
		/* start (everytime) */
		start = auring_tailptr(&mixer->hwbuf);

		error = sc->hw_if->start_input(sc->hw_hdl,
		    start, blksize, audio_rintr, sc);
		if (error) {
			audio_printf(sc,
			    "start_input failed: errno=%d\n", error);
			return;
		}
	}
}

/*
 * This is an interrupt handler for recording.
 * It is called with sc_intr_lock.
 *
 * It is usually called from hardware interrupt.  However, note that
 * for some drivers (e.g. uaudio) it is called from software interrupt.
 */
static void
audio_rintr(void *arg)
{
	struct audio_softc *sc;
	audio_trackmixer_t *mixer;

	sc = arg;
	KASSERT(mutex_owned(sc->sc_intr_lock));

	if (sc->sc_dying)
		return;
	if (sc->sc_rbusy == false) {
#if defined(DIAGNOSTIC)
		audio_printf(sc, "DIAGNOSTIC: %s raised stray interrupt\n",
		    device_xname(sc->hw_dev));
#endif
		return;
	}

	mixer = sc->sc_rmixer;
	mixer->hw_complete_counter += mixer->frames_per_block;
	mixer->hwseq++;

	auring_push(&mixer->hwbuf, mixer->frames_per_block);

	TRACE(4,
	    "HW_INT ++hwseq=%" PRIu64 " cmplcnt=%" PRIu64 " hwbuf=%d/%d/%d",
	    mixer->hwseq, mixer->hw_complete_counter,
	    mixer->hwbuf.head, mixer->hwbuf.used, mixer->hwbuf.capacity);

	/* Distrubute recorded block */
	audio_rmixer_process(sc);

	/* Request next block */
	audio_rmixer_input(sc);

	/*
	 * When this interrupt is the real hardware interrupt, disabling
	 * preemption here is not necessary.  But some drivers (e.g. uaudio)
	 * emulate it by software interrupt, so kpreempt_disable is necessary.
	 */
	kpreempt_disable();
	softint_schedule(mixer->sih);
	kpreempt_enable();
}

/*
 * Halts playback mixer.
 * This function also clears related parameters, so call this function
 * instead of calling halt_output directly.
 * Must be called only if sc_pbusy is true.
 * Must be called with sc_lock && sc_exlock held.
 */
static int
audio_pmixer_halt(struct audio_softc *sc)
{
	int error;

	TRACE(2, "called");
	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	mutex_enter(sc->sc_intr_lock);
	error = sc->hw_if->halt_output(sc->hw_hdl);

	/* Halts anyway even if some error has occurred. */
	sc->sc_pbusy = false;
	sc->sc_pmixer->hwbuf.head = 0;
	sc->sc_pmixer->hwbuf.used = 0;
	sc->sc_pmixer->mixseq = 0;
	sc->sc_pmixer->hwseq = 0;
	mutex_exit(sc->sc_intr_lock);

	return error;
}

/*
 * Halts recording mixer.
 * This function also clears related parameters, so call this function
 * instead of calling halt_input directly.
 * Must be called only if sc_rbusy is true.
 * Must be called with sc_lock && sc_exlock held.
 */
static int
audio_rmixer_halt(struct audio_softc *sc)
{
	int error;

	TRACE(2, "called");
	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	mutex_enter(sc->sc_intr_lock);
	error = sc->hw_if->halt_input(sc->hw_hdl);

	/* Halts anyway even if some error has occurred. */
	sc->sc_rbusy = false;
	sc->sc_rmixer->hwbuf.head = 0;
	sc->sc_rmixer->hwbuf.used = 0;
	sc->sc_rmixer->mixseq = 0;
	sc->sc_rmixer->hwseq = 0;
	mutex_exit(sc->sc_intr_lock);

	return error;
}

/*
 * Flush this track.
 * Halts all operations, clears all buffers, reset error counters.
 * XXX I'm not sure...
 */
static void
audio_track_clear(struct audio_softc *sc, audio_track_t *track)
{

	KASSERT(track);
	TRACET(3, track, "clear");

	audio_track_lock_enter(track);

	track->usrbuf.used = 0;
	/* Clear all internal parameters. */
	if (track->codec.filter) {
		track->codec.srcbuf.used = 0;
		track->codec.srcbuf.head = 0;
	}
	if (track->chvol.filter) {
		track->chvol.srcbuf.used = 0;
		track->chvol.srcbuf.head = 0;
	}
	if (track->chmix.filter) {
		track->chmix.srcbuf.used = 0;
		track->chmix.srcbuf.head = 0;
	}
	if (track->freq.filter) {
		track->freq.srcbuf.used = 0;
		track->freq.srcbuf.head = 0;
		if (track->freq_step < 65536)
			track->freq_current = 65536;
		else
			track->freq_current = 0;
		memset(track->freq_prev, 0, sizeof(track->freq_prev));
		memset(track->freq_curr, 0, sizeof(track->freq_curr));
	}
	/* Clear buffer, then operation halts naturally. */
	track->outbuf.used = 0;

	/* Clear counters. */
	track->dropframes = 0;

	audio_track_lock_exit(track);
}

/*
 * Drain the track.
 * track must be present and for playback.
 * If successful, it returns 0.  Otherwise returns errno.
 * Must be called with sc_lock held.
 */
static int
audio_track_drain(struct audio_softc *sc, audio_track_t *track)
{
	audio_trackmixer_t *mixer;
	int done;
	int error;

	KASSERT(track);
	TRACET(3, track, "start");
	mixer = track->mixer;
	KASSERT(mutex_owned(sc->sc_lock));

	/* Ignore them if pause. */
	if (track->is_pause) {
		TRACET(3, track, "pause -> clear");
		track->pstate = AUDIO_STATE_CLEAR;
	}
	/* Terminate early here if there is no data in the track. */
	if (track->pstate == AUDIO_STATE_CLEAR) {
		TRACET(3, track, "no need to drain");
		return 0;
	}
	track->pstate = AUDIO_STATE_DRAINING;

	for (;;) {
		/* I want to display it before condition evaluation. */
		TRACET(3, track, "pid=%d.%d trkseq=%d hwseq=%d out=%d/%d/%d",
		    (int)curproc->p_pid, (int)curlwp->l_lid,
		    (int)track->seq, (int)mixer->hwseq,
		    track->outbuf.head, track->outbuf.used,
		    track->outbuf.capacity);

		/* Condition to terminate */
		audio_track_lock_enter(track);
		done = (track->usrbuf.used < frametobyte(&track->inputfmt, 1) &&
		    track->outbuf.used == 0 &&
		    track->seq <= mixer->hwseq);
		audio_track_lock_exit(track);
		if (done)
			break;

		TRACET(3, track, "sleep");
		error = audio_track_waitio(sc, track);
		if (error)
			return error;

		/* XXX call audio_track_play here ? */
	}

	track->pstate = AUDIO_STATE_CLEAR;
	TRACET(3, track, "done trk_inp=%d trk_out=%d",
		(int)track->inputcounter, (int)track->outputcounter);
	return 0;
}

/*
 * Send signal to process.
 * This is intended to be called only from audio_softintr_{rd,wr}.
 * Must be called without sc_intr_lock held.
 */
static inline void
audio_psignal(struct audio_softc *sc, pid_t pid, int signum)
{
	proc_t *p;

	KASSERT(pid != 0);

	/*
	 * psignal() must be called without spin lock held.
	 */

	mutex_enter(&proc_lock);
	p = proc_find(pid);
	if (p)
		psignal(p, signum);
	mutex_exit(&proc_lock);
}

/*
 * This is software interrupt handler for record.
 * It is called from recording hardware interrupt everytime.
 * It does:
 * - Deliver SIGIO for all async processes.
 * - Notify to audio_read() that data has arrived.
 * - selnotify() for select/poll-ing processes.
 */
/*
 * XXX If a process issues FIOASYNC between hardware interrupt and
 *     software interrupt, (stray) SIGIO will be sent to the process
 *     despite the fact that it has not receive recorded data yet.
 */
static void
audio_softintr_rd(void *cookie)
{
	struct audio_softc *sc = cookie;
	audio_file_t *f;
	pid_t pid;

	mutex_enter(sc->sc_lock);

	SLIST_FOREACH(f, &sc->sc_files, entry) {
		audio_track_t *track = f->rtrack;

		if (track == NULL)
			continue;

		TRACET(4, track, "broadcast; inp=%d/%d/%d",
		    track->input->head,
		    track->input->used,
		    track->input->capacity);

		pid = f->async_audio;
		if (pid != 0) {
			TRACEF(4, f, "sending SIGIO %d", pid);
			audio_psignal(sc, pid, SIGIO);
		}
	}

	/* Notify that data has arrived. */
	selnotify(&sc->sc_rsel, 0, NOTE_SUBMIT);
	cv_broadcast(&sc->sc_rmixer->outcv);

	mutex_exit(sc->sc_lock);
}

/*
 * This is software interrupt handler for playback.
 * It is called from playback hardware interrupt everytime.
 * It does:
 * - Deliver SIGIO for all async and writable (used < lowat) processes.
 * - Notify to audio_write() that outbuf block available.
 * - selnotify() for select/poll-ing processes if there are any writable
 *   (used < lowat) processes.  Checking each descriptor will be done by
 *   filt_audiowrite_event().
 */
static void
audio_softintr_wr(void *cookie)
{
	struct audio_softc *sc = cookie;
	audio_file_t *f;
	bool found;
	pid_t pid;

	TRACE(4, "called");
	found = false;

	mutex_enter(sc->sc_lock);

	SLIST_FOREACH(f, &sc->sc_files, entry) {
		audio_track_t *track = f->ptrack;

		if (track == NULL)
			continue;

		TRACET(4, track, "broadcast; trkseq=%d out=%d/%d/%d",
		    (int)track->seq,
		    track->outbuf.head,
		    track->outbuf.used,
		    track->outbuf.capacity);

		/*
		 * Send a signal if the process is async mode and
		 * used is lower than lowat.
		 */
		if (track->usrbuf.used <= track->usrbuf_usedlow &&
		    !track->is_pause) {
			/* For selnotify */
			found = true;
			/* For SIGIO */
			pid = f->async_audio;
			if (pid != 0) {
				TRACEF(4, f, "sending SIGIO %d", pid);
				audio_psignal(sc, pid, SIGIO);
			}
		}
	}

	/*
	 * Notify for select/poll when someone become writable.
	 * It needs sc_lock (and not sc_intr_lock).
	 */
	if (found) {
		TRACE(4, "selnotify");
		selnotify(&sc->sc_wsel, 0, NOTE_SUBMIT);
	}

	/* Notify to audio_write() that outbuf available. */
	cv_broadcast(&sc->sc_pmixer->outcv);

	mutex_exit(sc->sc_lock);
}

/*
 * Check (and convert) the format *p came from userland.
 * If successful, it writes back the converted format to *p if necessary and
 * returns 0.  Otherwise returns errno (*p may be changed even in this case).
 */
static int
audio_check_params(audio_format2_t *p)
{

	/*
	 * Convert obsolete AUDIO_ENCODING_PCM encodings.
	 *
	 * AUDIO_ENCODING_PCM16 == AUDIO_ENCODING_LINEAR
	 * So, it's always signed, as in SunOS.
	 *
	 * AUDIO_ENCODING_PCM8 == AUDIO_ENCODING_LINEAR8
	 * So, it's always unsigned, as in SunOS.
	 */
	if (p->encoding == AUDIO_ENCODING_PCM16) {
		p->encoding = AUDIO_ENCODING_SLINEAR;
	} else if (p->encoding == AUDIO_ENCODING_PCM8) {
		if (p->precision == 8)
			p->encoding = AUDIO_ENCODING_ULINEAR;
		else
			return EINVAL;
	}

	/*
	 * Convert obsoleted AUDIO_ENCODING_[SU]LINEAR without endianness
	 * suffix.
	 */
	if (p->encoding == AUDIO_ENCODING_SLINEAR)
		p->encoding = AUDIO_ENCODING_SLINEAR_NE;
	if (p->encoding == AUDIO_ENCODING_ULINEAR)
		p->encoding = AUDIO_ENCODING_ULINEAR_NE;

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
		if (p->precision !=  8 && p->precision != 16 &&
		    p->precision != 24 && p->precision != 32)
			return EINVAL;

		/* 8bit format does not have endianness. */
		if (p->precision == 8) {
			if (p->encoding == AUDIO_ENCODING_SLINEAR_OE)
				p->encoding = AUDIO_ENCODING_SLINEAR_NE;
			if (p->encoding == AUDIO_ENCODING_ULINEAR_OE)
				p->encoding = AUDIO_ENCODING_ULINEAR_NE;
		}

		if (p->precision > p->stride)
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
 * Initialize playback and record mixers.
 * mode (AUMODE_{PLAY,RECORD}) indicates the mixer to be initialized.
 * phwfmt and rhwfmt indicate the hardware format.  pfil and rfil indicate
 * the filter registration information.  These four must not be NULL.
 * If successful returns 0.  Otherwise returns errno.
 * Must be called with sc_exlock held and without sc_lock held.
 * Must not be called if there are any tracks.
 * Caller should check that the initialization succeed by whether
 * sc_[pr]mixer is not NULL.
 */
static int
audio_mixers_init(struct audio_softc *sc, int mode,
	const audio_format2_t *phwfmt, const audio_format2_t *rhwfmt,
	const audio_filter_reg_t *pfil, const audio_filter_reg_t *rfil)
{
	int error;

	KASSERT(phwfmt != NULL);
	KASSERT(rhwfmt != NULL);
	KASSERT(pfil != NULL);
	KASSERT(rfil != NULL);
	KASSERT(sc->sc_exlock);

	if ((mode & AUMODE_PLAY)) {
		if (sc->sc_pmixer == NULL) {
			sc->sc_pmixer = kmem_zalloc(sizeof(*sc->sc_pmixer),
			    KM_SLEEP);
		} else {
			/* destroy() doesn't free memory. */
			audio_mixer_destroy(sc, sc->sc_pmixer);
			memset(sc->sc_pmixer, 0, sizeof(*sc->sc_pmixer));
		}
		error = audio_mixer_init(sc, AUMODE_PLAY, phwfmt, pfil);
		if (error) {
			/* audio_mixer_init already displayed error code */
			audio_printf(sc, "configuring playback mode failed\n");
			kmem_free(sc->sc_pmixer, sizeof(*sc->sc_pmixer));
			sc->sc_pmixer = NULL;
			return error;
		}
	}
	if ((mode & AUMODE_RECORD)) {
		if (sc->sc_rmixer == NULL) {
			sc->sc_rmixer = kmem_zalloc(sizeof(*sc->sc_rmixer),
			    KM_SLEEP);
		} else {
			/* destroy() doesn't free memory. */
			audio_mixer_destroy(sc, sc->sc_rmixer);
			memset(sc->sc_rmixer, 0, sizeof(*sc->sc_rmixer));
		}
		error = audio_mixer_init(sc, AUMODE_RECORD, rhwfmt, rfil);
		if (error) {
			/* audio_mixer_init already displayed error code */
			audio_printf(sc, "configuring record mode failed\n");
			kmem_free(sc->sc_rmixer, sizeof(*sc->sc_rmixer));
			sc->sc_rmixer = NULL;
			return error;
		}
	}

	return 0;
}

/*
 * Select a frequency.
 * Prioritize 48kHz and 44.1kHz.  Otherwise choose the highest one.
 * XXX Better algorithm?
 */
static int
audio_select_freq(const struct audio_format *fmt)
{
	int freq;
	int high;
	int low;
	int j;

	if (fmt->frequency_type == 0) {
		low = fmt->frequency[0];
		high = fmt->frequency[1];
		freq = 48000;
		if (low <= freq && freq <= high) {
			return freq;
		}
		freq = 44100;
		if (low <= freq && freq <= high) {
			return freq;
		}
		return high;
	} else {
		for (j = 0; j < fmt->frequency_type; j++) {
			if (fmt->frequency[j] == 48000) {
				return fmt->frequency[j];
			}
		}
		high = 0;
		for (j = 0; j < fmt->frequency_type; j++) {
			if (fmt->frequency[j] == 44100) {
				return fmt->frequency[j];
			}
			if (fmt->frequency[j] > high) {
				high = fmt->frequency[j];
			}
		}
		return high;
	}
}

/*
 * Choose the most preferred hardware format.
 * If successful, it will store the chosen format into *cand and return 0.
 * Otherwise, return errno.
 * Must be called without sc_lock held.
 */
static int
audio_hw_probe(struct audio_softc *sc, audio_format2_t *cand, int mode)
{
	audio_format_query_t query;
	int cand_score;
	int score;
	int i;
	int error;

	/*
	 * Score each formats and choose the highest one.
	 *
	 *                 +---- priority(0-3)
	 *                 |+--- encoding/precision
	 *                 ||+-- channels
	 * score = 0x000000PEC
	 */

	cand_score = 0;
	for (i = 0; ; i++) {
		memset(&query, 0, sizeof(query));
		query.index = i;

		mutex_enter(sc->sc_lock);
		error = sc->hw_if->query_format(sc->hw_hdl, &query);
		mutex_exit(sc->sc_lock);
		if (error == EINVAL)
			break;
		if (error)
			return error;

#if defined(AUDIO_DEBUG)
		DPRINTF(1, "fmt[%d] %c%c pri=%d %s,%d/%dbit,%dch,", i,
		    (query.fmt.mode & AUMODE_PLAY)   ? 'P' : '-',
		    (query.fmt.mode & AUMODE_RECORD) ? 'R' : '-',
		    query.fmt.priority,
		    audio_encoding_name(query.fmt.encoding),
		    query.fmt.validbits,
		    query.fmt.precision,
		    query.fmt.channels);
		if (query.fmt.frequency_type == 0) {
			DPRINTF(1, "{%d-%d",
			    query.fmt.frequency[0], query.fmt.frequency[1]);
		} else {
			int j;
			for (j = 0; j < query.fmt.frequency_type; j++) {
				DPRINTF(1, "%c%d",
				    (j == 0) ? '{' : ',',
				    query.fmt.frequency[j]);
			}
		}
		DPRINTF(1, "}\n");
#endif

		if ((query.fmt.mode & mode) == 0) {
			DPRINTF(1, "fmt[%d] skip; mode not match %d\n", i,
			    mode);
			continue;
		}

		if (query.fmt.priority < 0) {
			DPRINTF(1, "fmt[%d] skip; unsupported encoding\n", i);
			continue;
		}

		/* Score */
		score = (query.fmt.priority & 3) * 0x100;
		if (query.fmt.encoding == AUDIO_ENCODING_SLINEAR_NE &&
		    query.fmt.validbits == AUDIO_INTERNAL_BITS &&
		    query.fmt.precision == AUDIO_INTERNAL_BITS) {
			score += 0x20;
		} else if (query.fmt.encoding == AUDIO_ENCODING_SLINEAR_OE &&
		    query.fmt.validbits == AUDIO_INTERNAL_BITS &&
		    query.fmt.precision == AUDIO_INTERNAL_BITS) {
			score += 0x10;
		}

		/* Do not prefer surround formats */
		if (query.fmt.channels <= 2)
			score += query.fmt.channels;

		if (score < cand_score) {
			DPRINTF(1, "fmt[%d] skip; score 0x%x < 0x%x\n", i,
			    score, cand_score);
			continue;
		}

		/* Update candidate */
		cand_score = score;
		cand->encoding    = query.fmt.encoding;
		cand->precision   = query.fmt.validbits;
		cand->stride      = query.fmt.precision;
		cand->channels    = query.fmt.channels;
		cand->sample_rate = audio_select_freq(&query.fmt);
		DPRINTF(1, "fmt[%d] candidate (score=0x%x)"
		    " pri=%d %s,%d/%d,%dch,%dHz\n", i,
		    cand_score, query.fmt.priority,
		    audio_encoding_name(query.fmt.encoding),
		    cand->precision, cand->stride,
		    cand->channels, cand->sample_rate);
	}

	if (cand_score == 0) {
		DPRINTF(1, "%s no fmt\n", __func__);
		return ENXIO;
	}
	DPRINTF(1, "%s selected: %s,%d/%d,%dch,%dHz\n", __func__,
	    audio_encoding_name(cand->encoding),
	    cand->precision, cand->stride, cand->channels, cand->sample_rate);
	return 0;
}

/*
 * Validate fmt with query_format.
 * If fmt is included in the result of query_format, returns 0.
 * Otherwise returns EINVAL.
 * Must be called without sc_lock held.
 */
static int
audio_hw_validate_format(struct audio_softc *sc, int mode,
	const audio_format2_t *fmt)
{
	audio_format_query_t query;
	struct audio_format *q;
	int index;
	int error;
	int j;

	for (index = 0; ; index++) {
		query.index = index;
		mutex_enter(sc->sc_lock);
		error = sc->hw_if->query_format(sc->hw_hdl, &query);
		mutex_exit(sc->sc_lock);
		if (error == EINVAL)
			break;
		if (error)
			return error;

		q = &query.fmt;
		/*
		 * Note that fmt is audio_format2_t (precision/stride) but
		 * q is audio_format_t (validbits/precision).
		 */
		if ((q->mode & mode) == 0) {
			continue;
		}
		if (fmt->encoding != q->encoding) {
			continue;
		}
		if (fmt->precision != q->validbits) {
			continue;
		}
		if (fmt->stride != q->precision) {
			continue;
		}
		if (fmt->channels != q->channels) {
			continue;
		}
		if (q->frequency_type == 0) {
			if (fmt->sample_rate < q->frequency[0] ||
			    fmt->sample_rate > q->frequency[1]) {
				continue;
			}
		} else {
			for (j = 0; j < q->frequency_type; j++) {
				if (fmt->sample_rate == q->frequency[j])
					break;
			}
			if (j == query.fmt.frequency_type) {
				continue;
			}
		}

		/* Matched. */
		return 0;
	}

	return EINVAL;
}

/*
 * Set track mixer's format depending on ai->mode.
 * If AUMODE_PLAY is set in ai->mode, it set up the playback mixer
 * with ai.play.*.
 * If AUMODE_RECORD is set in ai->mode, it set up the recording mixer
 * with ai.record.*.
 * All other fields in ai are ignored.
 * If successful returns 0.  Otherwise returns errno.
 * This function does not roll back even if it fails.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static int
audio_mixers_set_format(struct audio_softc *sc, const struct audio_info *ai)
{
	audio_format2_t phwfmt;
	audio_format2_t rhwfmt;
	audio_filter_reg_t pfil;
	audio_filter_reg_t rfil;
	int mode;
	int error;

	KASSERT(sc->sc_exlock);

	/*
	 * Even when setting either one of playback and recording,
	 * both must be halted.
	 */
	if (sc->sc_popens + sc->sc_ropens > 0)
		return EBUSY;

	if (!SPECIFIED(ai->mode) || ai->mode == 0)
		return ENOTTY;

	mode = ai->mode;
	if ((mode & AUMODE_PLAY)) {
		phwfmt.encoding    = ai->play.encoding;
		phwfmt.precision   = ai->play.precision;
		phwfmt.stride      = ai->play.precision;
		phwfmt.channels    = ai->play.channels;
		phwfmt.sample_rate = ai->play.sample_rate;
	}
	if ((mode & AUMODE_RECORD)) {
		rhwfmt.encoding    = ai->record.encoding;
		rhwfmt.precision   = ai->record.precision;
		rhwfmt.stride      = ai->record.precision;
		rhwfmt.channels    = ai->record.channels;
		rhwfmt.sample_rate = ai->record.sample_rate;
	}

	/* On non-independent devices, use the same format for both. */
	if ((sc->sc_props & AUDIO_PROP_INDEPENDENT) == 0) {
		if (mode == AUMODE_RECORD) {
			phwfmt = rhwfmt;
		} else {
			rhwfmt = phwfmt;
		}
		mode = AUMODE_PLAY | AUMODE_RECORD;
	}

	/* Then, unset the direction not exist on the hardware. */
	if ((sc->sc_props & AUDIO_PROP_PLAYBACK) == 0)
		mode &= ~AUMODE_PLAY;
	if ((sc->sc_props & AUDIO_PROP_CAPTURE) == 0)
		mode &= ~AUMODE_RECORD;

	/* debug */
	if ((mode & AUMODE_PLAY)) {
		TRACE(1, "play=%s/%d/%d/%dch/%dHz",
		    audio_encoding_name(phwfmt.encoding),
		    phwfmt.precision,
		    phwfmt.stride,
		    phwfmt.channels,
		    phwfmt.sample_rate);
	}
	if ((mode & AUMODE_RECORD)) {
		TRACE(1, "rec =%s/%d/%d/%dch/%dHz",
		    audio_encoding_name(rhwfmt.encoding),
		    rhwfmt.precision,
		    rhwfmt.stride,
		    rhwfmt.channels,
		    rhwfmt.sample_rate);
	}

	/* Check the format */
	if ((mode & AUMODE_PLAY)) {
		if (audio_hw_validate_format(sc, AUMODE_PLAY, &phwfmt)) {
			TRACE(1, "invalid format");
			return EINVAL;
		}
	}
	if ((mode & AUMODE_RECORD)) {
		if (audio_hw_validate_format(sc, AUMODE_RECORD, &rhwfmt)) {
			TRACE(1, "invalid format");
			return EINVAL;
		}
	}

	/* Configure the mixers. */
	memset(&pfil, 0, sizeof(pfil));
	memset(&rfil, 0, sizeof(rfil));
	error = audio_hw_set_format(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
	if (error)
		return error;

	error = audio_mixers_init(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
	if (error)
		return error;

	/*
	 * Reinitialize the sticky parameters for /dev/sound.
	 * If the number of the hardware channels becomes less than the number
	 * of channels that sticky parameters remember, subsequent /dev/sound
	 * open will fail.  To prevent this, reinitialize the sticky
	 * parameters whenever the hardware format is changed.
	 */
	sc->sc_sound_pparams = params_to_format2(&audio_default);
	sc->sc_sound_rparams = params_to_format2(&audio_default);
	sc->sc_sound_ppause = false;
	sc->sc_sound_rpause = false;

	return 0;
}

/*
 * Store current mixers format into *ai.
 * Must be called with sc_exlock held.
 */
static void
audio_mixers_get_format(struct audio_softc *sc, struct audio_info *ai)
{

	KASSERT(sc->sc_exlock);

	/*
	 * There is no stride information in audio_info but it doesn't matter.
	 * trackmixer always treats stride and precision as the same.
	 */
	AUDIO_INITINFO(ai);
	ai->mode = 0;
	if (sc->sc_pmixer) {
		audio_format2_t *fmt = &sc->sc_pmixer->track_fmt;
		ai->play.encoding    = fmt->encoding;
		ai->play.precision   = fmt->precision;
		ai->play.channels    = fmt->channels;
		ai->play.sample_rate = fmt->sample_rate;
		ai->mode |= AUMODE_PLAY;
	}
	if (sc->sc_rmixer) {
		audio_format2_t *fmt = &sc->sc_rmixer->track_fmt;
		ai->record.encoding    = fmt->encoding;
		ai->record.precision   = fmt->precision;
		ai->record.channels    = fmt->channels;
		ai->record.sample_rate = fmt->sample_rate;
		ai->mode |= AUMODE_RECORD;
	}
}

/*
 * audio_info details:
 *
 * ai.{play,record}.sample_rate		(R/W)
 * ai.{play,record}.encoding		(R/W)
 * ai.{play,record}.precision		(R/W)
 * ai.{play,record}.channels		(R/W)
 *	These specify the playback or recording format.
 *	Ignore members within an inactive track.
 *
 * ai.mode				(R/W)
 *	It specifies the playback or recording mode, AUMODE_*.
 *	Currently, a mode change operation by ai.mode after opening is
 *	prohibited.  In addition, AUMODE_PLAY_ALL no longer makes sense.
 *	However, it's possible to get or to set for backward compatibility.
 *
 * ai.{hiwat,lowat}			(R/W)
 *	These specify the high water mark and low water mark for playback
 *	track.  The unit is block.
 *
 * ai.{play,record}.gain		(R/W)
 *	It specifies the HW mixer volume in 0-255.
 *	It is historical reason that the gain is connected to HW mixer.
 *
 * ai.{play,record}.balance		(R/W)
 *	It specifies the left-right balance of HW mixer in 0-64.
 *	32 means the center.
 *	It is historical reason that the balance is connected to HW mixer.
 *
 * ai.{play,record}.port		(R/W)
 *	It specifies the input/output port of HW mixer.
 *
 * ai.monitor_gain			(R/W)
 *	It specifies the recording monitor gain(?) of HW mixer.
 *
 * ai.{play,record}.pause		(R/W)
 *	Non-zero means the track is paused.
 *
 * ai.play.seek				(R/-)
 *	It indicates the number of bytes written but not processed.
 * ai.record.seek			(R/-)
 *	It indicates the number of bytes to be able to read.
 *
 * ai.{play,record}.avail_ports		(R/-)
 *	Mixer info.
 *
 * ai.{play,record}.buffer_size		(R/-)
 *	It indicates the buffer size in bytes.  Internally it means usrbuf.
 *
 * ai.{play,record}.samples		(R/-)
 *	It indicates the total number of bytes played or recorded.
 *
 * ai.{play,record}.eof			(R/-)
 *	It indicates the number of times reached EOF(?).
 *
 * ai.{play,record}.error		(R/-)
 *	Non-zero indicates overflow/underflow has occurred.
 *
 * ai.{play,record}.waiting		(R/-)
 *	Non-zero indicates that other process waits to open.
 *	It will never happen anymore.
 *
 * ai.{play,record}.open		(R/-)
 *	Non-zero indicates the direction is opened by this process(?).
 *	XXX Is this better to indicate that "the device is opened by
 *	at least one process"?
 *
 * ai.{play,record}.active		(R/-)
 *	Non-zero indicates that I/O is currently active.
 *
 * ai.blocksize				(R/-)
 *	It indicates the block size in bytes.
 *	XXX The blocksize of playback and recording may be different.
 */

/*
 * Pause consideration:
 *
 * Pausing/unpausing never affect [pr]mixer.  This single rule makes
 * operation simple.  Note that playback and recording are asymmetric.
 *
 * For playback,
 *  1. Any playback open doesn't start pmixer regardless of initial pause
 *     state of this track.
 *  2. The first write access among playback tracks only starts pmixer
 *     regardless of this track's pause state.
 *  3. Even a pause of the last playback track doesn't stop pmixer.
 *  4. The last close of all playback tracks only stops pmixer.
 *
 * For recording,
 *  1. The first recording open only starts rmixer regardless of initial
 *     pause state of this track.
 *  2. Even a pause of the last track doesn't stop rmixer.
 *  3. The last close of all recording tracks only stops rmixer.
 */

/*
 * Set both track's parameters within a file depending on ai.
 * Update sc_sound_[pr]* if set.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static int
audio_file_setinfo(struct audio_softc *sc, audio_file_t *file,
	const struct audio_info *ai)
{
	const struct audio_prinfo *pi;
	const struct audio_prinfo *ri;
	audio_track_t *ptrack;
	audio_track_t *rtrack;
	audio_format2_t pfmt;
	audio_format2_t rfmt;
	int pchanges;
	int rchanges;
	int mode;
	struct audio_info saved_ai;
	audio_format2_t saved_pfmt;
	audio_format2_t saved_rfmt;
	int error;

	KASSERT(sc->sc_exlock);

	pi = &ai->play;
	ri = &ai->record;
	pchanges = 0;
	rchanges = 0;

	ptrack = file->ptrack;
	rtrack = file->rtrack;

#if defined(AUDIO_DEBUG)
	if (audiodebug >= 2) {
		char buf[256];
		char p[64];
		int buflen;
		int plen;
#define SPRINTF(var, fmt...) do {	\
	var##len += snprintf(var + var##len, sizeof(var) - var##len, fmt); \
} while (0)

		buflen = 0;
		plen = 0;
		if (SPECIFIED(pi->encoding))
			SPRINTF(p, "/%s", audio_encoding_name(pi->encoding));
		if (SPECIFIED(pi->precision))
			SPRINTF(p, "/%dbit", pi->precision);
		if (SPECIFIED(pi->channels))
			SPRINTF(p, "/%dch", pi->channels);
		if (SPECIFIED(pi->sample_rate))
			SPRINTF(p, "/%dHz", pi->sample_rate);
		if (plen > 0)
			SPRINTF(buf, ",play.param=%s", p + 1);

		plen = 0;
		if (SPECIFIED(ri->encoding))
			SPRINTF(p, "/%s", audio_encoding_name(ri->encoding));
		if (SPECIFIED(ri->precision))
			SPRINTF(p, "/%dbit", ri->precision);
		if (SPECIFIED(ri->channels))
			SPRINTF(p, "/%dch", ri->channels);
		if (SPECIFIED(ri->sample_rate))
			SPRINTF(p, "/%dHz", ri->sample_rate);
		if (plen > 0)
			SPRINTF(buf, ",record.param=%s", p + 1);

		if (SPECIFIED(ai->mode))
			SPRINTF(buf, ",mode=%d", ai->mode);
		if (SPECIFIED(ai->hiwat))
			SPRINTF(buf, ",hiwat=%d", ai->hiwat);
		if (SPECIFIED(ai->lowat))
			SPRINTF(buf, ",lowat=%d", ai->lowat);
		if (SPECIFIED(ai->play.gain))
			SPRINTF(buf, ",play.gain=%d", ai->play.gain);
		if (SPECIFIED(ai->record.gain))
			SPRINTF(buf, ",record.gain=%d", ai->record.gain);
		if (SPECIFIED_CH(ai->play.balance))
			SPRINTF(buf, ",play.balance=%d", ai->play.balance);
		if (SPECIFIED_CH(ai->record.balance))
			SPRINTF(buf, ",record.balance=%d", ai->record.balance);
		if (SPECIFIED(ai->play.port))
			SPRINTF(buf, ",play.port=%d", ai->play.port);
		if (SPECIFIED(ai->record.port))
			SPRINTF(buf, ",record.port=%d", ai->record.port);
		if (SPECIFIED(ai->monitor_gain))
			SPRINTF(buf, ",monitor_gain=%d", ai->monitor_gain);
		if (SPECIFIED_CH(ai->play.pause))
			SPRINTF(buf, ",play.pause=%d", ai->play.pause);
		if (SPECIFIED_CH(ai->record.pause))
			SPRINTF(buf, ",record.pause=%d", ai->record.pause);

		if (buflen > 0)
			TRACE(2, "specified %s", buf + 1);
	}
#endif

	AUDIO_INITINFO(&saved_ai);
	/* XXX shut up gcc */
	memset(&saved_pfmt, 0, sizeof(saved_pfmt));
	memset(&saved_rfmt, 0, sizeof(saved_rfmt));

	/*
	 * Set default value and save current parameters.
	 * For backward compatibility, use sticky parameters for nonexistent
	 * track.
	 */
	if (ptrack) {
		pfmt = ptrack->usrbuf.fmt;
		saved_pfmt = ptrack->usrbuf.fmt;
		saved_ai.play.pause = ptrack->is_pause;
	} else {
		pfmt = sc->sc_sound_pparams;
	}
	if (rtrack) {
		rfmt = rtrack->usrbuf.fmt;
		saved_rfmt = rtrack->usrbuf.fmt;
		saved_ai.record.pause = rtrack->is_pause;
	} else {
		rfmt = sc->sc_sound_rparams;
	}
	saved_ai.mode = file->mode;

	/*
	 * Overwrite if specified.
	 */
	mode = file->mode;
	if (SPECIFIED(ai->mode)) {
		/*
		 * Setting ai->mode no longer does anything because it's
		 * prohibited to change playback/recording mode after open
		 * and AUMODE_PLAY_ALL is obsoleted.  However, it still
		 * keeps the state of AUMODE_PLAY_ALL itself for backward
		 * compatibility.
		 * In the internal, only file->mode has the state of
		 * AUMODE_PLAY_ALL flag and track->mode in both track does
		 * not have.
		 */
		if ((file->mode & AUMODE_PLAY)) {
			mode = (file->mode & (AUMODE_PLAY | AUMODE_RECORD))
			    | (ai->mode & AUMODE_PLAY_ALL);
		}
	}

	pchanges = audio_track_setinfo_check(ptrack, &pfmt, pi);
	if (pchanges == -1) {
#if defined(AUDIO_DEBUG)
		TRACEF(1, file, "check play.params failed: "
		    "%s %ubit %uch %uHz",
		    audio_encoding_name(pi->encoding),
		    pi->precision,
		    pi->channels,
		    pi->sample_rate);
#endif
		return EINVAL;
	}

	rchanges = audio_track_setinfo_check(rtrack, &rfmt, ri);
	if (rchanges == -1) {
#if defined(AUDIO_DEBUG)
		TRACEF(1, file, "check record.params failed: "
		    "%s %ubit %uch %uHz",
		    audio_encoding_name(ri->encoding),
		    ri->precision,
		    ri->channels,
		    ri->sample_rate);
#endif
		return EINVAL;
	}

	if (SPECIFIED(ai->mode)) {
		pchanges = 1;
		rchanges = 1;
	}

	/*
	 * Even when setting either one of playback and recording,
	 * both track must be halted.
	 */
	if (pchanges || rchanges) {
		audio_file_clear(sc, file);
#if defined(AUDIO_DEBUG)
		char nbuf[16];
		char fmtbuf[64];
		if (pchanges) {
			if (ptrack) {
				snprintf(nbuf, sizeof(nbuf), "%d", ptrack->id);
			} else {
				snprintf(nbuf, sizeof(nbuf), "-");
			}
			audio_format2_tostr(fmtbuf, sizeof(fmtbuf), &pfmt);
			DPRINTF(1, "audio track#%s play mode: %s\n",
			    nbuf, fmtbuf);
		}
		if (rchanges) {
			if (rtrack) {
				snprintf(nbuf, sizeof(nbuf), "%d", rtrack->id);
			} else {
				snprintf(nbuf, sizeof(nbuf), "-");
			}
			audio_format2_tostr(fmtbuf, sizeof(fmtbuf), &rfmt);
			DPRINTF(1, "audio track#%s rec  mode: %s\n",
			    nbuf, fmtbuf);
		}
#endif
	}

	/* Set mixer parameters */
	mutex_enter(sc->sc_lock);
	error = audio_hw_setinfo(sc, ai, &saved_ai);
	mutex_exit(sc->sc_lock);
	if (error)
		goto abort1;

	/*
	 * Set to track and update sticky parameters.
	 */
	error = 0;
	file->mode = mode;

	if (SPECIFIED_CH(pi->pause)) {
		if (ptrack)
			ptrack->is_pause = pi->pause;
		sc->sc_sound_ppause = pi->pause;
	}
	if (pchanges) {
		if (ptrack) {
			audio_track_lock_enter(ptrack);
			error = audio_track_set_format(ptrack, &pfmt);
			audio_track_lock_exit(ptrack);
			if (error) {
				TRACET(1, ptrack, "set play.params failed");
				goto abort2;
			}
		}
		sc->sc_sound_pparams = pfmt;
	}
	/* Change water marks after initializing the buffers. */
	if (SPECIFIED(ai->hiwat) || SPECIFIED(ai->lowat)) {
		if (ptrack)
			audio_track_setinfo_water(ptrack, ai);
	}

	if (SPECIFIED_CH(ri->pause)) {
		if (rtrack)
			rtrack->is_pause = ri->pause;
		sc->sc_sound_rpause = ri->pause;
	}
	if (rchanges) {
		if (rtrack) {
			audio_track_lock_enter(rtrack);
			error = audio_track_set_format(rtrack, &rfmt);
			audio_track_lock_exit(rtrack);
			if (error) {
				TRACET(1, rtrack, "set record.params failed");
				goto abort3;
			}
		}
		sc->sc_sound_rparams = rfmt;
	}

	return 0;

	/* Rollback */
abort3:
	if (error != ENOMEM) {
		rtrack->is_pause = saved_ai.record.pause;
		audio_track_lock_enter(rtrack);
		audio_track_set_format(rtrack, &saved_rfmt);
		audio_track_lock_exit(rtrack);
	}
	sc->sc_sound_rpause = saved_ai.record.pause;
	sc->sc_sound_rparams = saved_rfmt;
abort2:
	if (ptrack && error != ENOMEM) {
		ptrack->is_pause = saved_ai.play.pause;
		audio_track_lock_enter(ptrack);
		audio_track_set_format(ptrack, &saved_pfmt);
		audio_track_lock_exit(ptrack);
	}
	sc->sc_sound_ppause = saved_ai.play.pause;
	sc->sc_sound_pparams = saved_pfmt;
	file->mode = saved_ai.mode;
abort1:
	mutex_enter(sc->sc_lock);
	audio_hw_setinfo(sc, &saved_ai, NULL);
	mutex_exit(sc->sc_lock);

	return error;
}

/*
 * Write SPECIFIED() parameters within info back to fmt.
 * Note that track can be NULL here.
 * Return value of 1 indicates that fmt is modified.
 * Return value of 0 indicates that fmt is not modified.
 * Return value of -1 indicates that error EINVAL has occurred.
 */
static int
audio_track_setinfo_check(audio_track_t *track,
	audio_format2_t *fmt, const struct audio_prinfo *info)
{
	const audio_format2_t *hwfmt;
	int changes;

	changes = 0;
	if (SPECIFIED(info->sample_rate)) {
		if (info->sample_rate < AUDIO_MIN_FREQUENCY)
			return -1;
		if (info->sample_rate > AUDIO_MAX_FREQUENCY)
			return -1;
		fmt->sample_rate = info->sample_rate;
		changes = 1;
	}
	if (SPECIFIED(info->encoding)) {
		fmt->encoding = info->encoding;
		changes = 1;
	}
	if (SPECIFIED(info->precision)) {
		fmt->precision = info->precision;
		/* we don't have API to specify stride */
		fmt->stride = info->precision;
		changes = 1;
	}
	if (SPECIFIED(info->channels)) {
		/*
		 * We can convert between monaural and stereo each other.
		 * We can reduce than the number of channels that the hardware
		 * supports.
		 */
		if (info->channels > 2) {
			if (track) {
				hwfmt = &track->mixer->hwbuf.fmt;
				if (info->channels > hwfmt->channels)
					return -1;
			} else {
				/*
				 * This should never happen.
				 * If track == NULL, channels should be <= 2.
				 */
				return -1;
			}
		}
		fmt->channels = info->channels;
		changes = 1;
	}

	if (changes) {
		if (audio_check_params(fmt) != 0)
			return -1;
	}

	return changes;
}

/*
 * Change water marks for playback track if specified.
 */
static void
audio_track_setinfo_water(audio_track_t *track, const struct audio_info *ai)
{
	u_int blks;
	u_int maxblks;
	u_int blksize;

	KASSERT(audio_track_is_playback(track));

	blksize = track->usrbuf_blksize;
	maxblks = track->usrbuf.capacity / blksize;

	if (SPECIFIED(ai->hiwat)) {
		blks = ai->hiwat;
		if (blks > maxblks)
			blks = maxblks;
		if (blks < 2)
			blks = 2;
		track->usrbuf_usedhigh = blks * blksize;
	}
	if (SPECIFIED(ai->lowat)) {
		blks = ai->lowat;
		if (blks > maxblks - 1)
			blks = maxblks - 1;
		track->usrbuf_usedlow = blks * blksize;
	}
	if (SPECIFIED(ai->hiwat) || SPECIFIED(ai->lowat)) {
		if (track->usrbuf_usedlow > track->usrbuf_usedhigh - blksize) {
			track->usrbuf_usedlow = track->usrbuf_usedhigh -
			    blksize;
		}
	}
}

/*
 * Set hardware part of *newai.
 * The parameters handled here are *.port, *.gain, *.balance and monitor_gain.
 * If oldai is specified, previous parameters are stored.
 * This function itself does not roll back if error occurred.
 * Must be called with sc_lock && sc_exlock held.
 */
static int
audio_hw_setinfo(struct audio_softc *sc, const struct audio_info *newai,
	struct audio_info *oldai)
{
	const struct audio_prinfo *newpi;
	const struct audio_prinfo *newri;
	struct audio_prinfo *oldpi;
	struct audio_prinfo *oldri;
	u_int pgain;
	u_int rgain;
	u_char pbalance;
	u_char rbalance;
	int error;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	/* XXX shut up gcc */
	oldpi = NULL;
	oldri = NULL;

	newpi = &newai->play;
	newri = &newai->record;
	if (oldai) {
		oldpi = &oldai->play;
		oldri = &oldai->record;
	}
	error = 0;

	/*
	 * It looks like unnecessary to halt HW mixers to set HW mixers.
	 * mixer_ioctl(MIXER_WRITE) also doesn't halt.
	 */

	if (SPECIFIED(newpi->port)) {
		if (oldai)
			oldpi->port = au_get_port(sc, &sc->sc_outports);
		error = au_set_port(sc, &sc->sc_outports, newpi->port);
		if (error) {
			audio_printf(sc,
			    "setting play.port=%d failed: errno=%d\n",
			    newpi->port, error);
			goto abort;
		}
	}
	if (SPECIFIED(newri->port)) {
		if (oldai)
			oldri->port = au_get_port(sc, &sc->sc_inports);
		error = au_set_port(sc, &sc->sc_inports, newri->port);
		if (error) {
			audio_printf(sc,
			    "setting record.port=%d failed: errno=%d\n",
			    newri->port, error);
			goto abort;
		}
	}

	/* play.{gain,balance} */
	if (SPECIFIED(newpi->gain) || SPECIFIED_CH(newpi->balance)) {
		au_get_gain(sc, &sc->sc_outports, &pgain, &pbalance);
		if (oldai) {
			oldpi->gain = pgain;
			oldpi->balance = pbalance;
		}

		if (SPECIFIED(newpi->gain))
			pgain = newpi->gain;
		if (SPECIFIED_CH(newpi->balance))
			pbalance = newpi->balance;
		error = au_set_gain(sc, &sc->sc_outports, pgain, pbalance);
		if (error) {
			audio_printf(sc,
			    "setting play.gain=%d/balance=%d failed: "
			    "errno=%d\n",
			    pgain, pbalance, error);
			goto abort;
		}
	}

	/* record.{gain,balance} */
	if (SPECIFIED(newri->gain) || SPECIFIED_CH(newri->balance)) {
		au_get_gain(sc, &sc->sc_inports, &rgain, &rbalance);
		if (oldai) {
			oldri->gain = rgain;
			oldri->balance = rbalance;
		}

		if (SPECIFIED(newri->gain))
			rgain = newri->gain;
		if (SPECIFIED_CH(newri->balance))
			rbalance = newri->balance;
		error = au_set_gain(sc, &sc->sc_inports, rgain, rbalance);
		if (error) {
			audio_printf(sc,
			    "setting record.gain=%d/balance=%d failed: "
			    "errno=%d\n",
			    rgain, rbalance, error);
			goto abort;
		}
	}

	if (SPECIFIED(newai->monitor_gain) && sc->sc_monitor_port != -1) {
		if (oldai)
			oldai->monitor_gain = au_get_monitor_gain(sc);
		error = au_set_monitor_gain(sc, newai->monitor_gain);
		if (error) {
			audio_printf(sc,
			    "setting monitor_gain=%d failed: errno=%d\n",
			    newai->monitor_gain, error);
			goto abort;
		}
	}

	/* XXX TODO */
	/* sc->sc_ai = *ai; */

	error = 0;
abort:
	return error;
}

/*
 * Setup the hardware with mixer format phwfmt, rhwfmt.
 * The arguments have following restrictions:
 * - setmode is the direction you want to set, AUMODE_PLAY or AUMODE_RECORD,
 *   or both.
 * - phwfmt and rhwfmt must not be NULL regardless of setmode.
 * - On non-independent devices, phwfmt and rhwfmt must have the same
 *   parameters.
 * - pfil and rfil must be zero-filled.
 * If successful,
 * - pfil, rfil will be filled with filter information specified by the
 *   hardware driver if necessary.
 * and then returns 0.  Otherwise returns errno.
 * Must be called without sc_lock held.
 */
static int
audio_hw_set_format(struct audio_softc *sc, int setmode,
	const audio_format2_t *phwfmt, const audio_format2_t *rhwfmt,
	audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	audio_params_t pp, rp;
	int error;

	KASSERT(phwfmt != NULL);
	KASSERT(rhwfmt != NULL);

	pp = format2_to_params(phwfmt);
	rp = format2_to_params(rhwfmt);

	mutex_enter(sc->sc_lock);
	error = sc->hw_if->set_format(sc->hw_hdl, setmode,
	    &pp, &rp, pfil, rfil);
	if (error) {
		mutex_exit(sc->sc_lock);
		audio_printf(sc, "set_format failed: errno=%d\n", error);
		return error;
	}

	if (sc->hw_if->commit_settings) {
		error = sc->hw_if->commit_settings(sc->hw_hdl);
		if (error) {
			mutex_exit(sc->sc_lock);
			audio_printf(sc,
			    "commit_settings failed: errno=%d\n", error);
			return error;
		}
	}
	mutex_exit(sc->sc_lock);

	return 0;
}

/*
 * Fill audio_info structure.  If need_mixerinfo is true, it will also
 * fill the hardware mixer information.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static int
audiogetinfo(struct audio_softc *sc, struct audio_info *ai, int need_mixerinfo,
	audio_file_t *file)
{
	struct audio_prinfo *ri, *pi;
	audio_track_t *track;
	audio_track_t *ptrack;
	audio_track_t *rtrack;
	int gain;

	KASSERT(sc->sc_exlock);

	ri = &ai->record;
	pi = &ai->play;
	ptrack = file->ptrack;
	rtrack = file->rtrack;

	memset(ai, 0, sizeof(*ai));

	if (ptrack) {
		pi->sample_rate = ptrack->usrbuf.fmt.sample_rate;
		pi->channels    = ptrack->usrbuf.fmt.channels;
		pi->precision   = ptrack->usrbuf.fmt.precision;
		pi->encoding    = ptrack->usrbuf.fmt.encoding;
		pi->pause       = ptrack->is_pause;
	} else {
		/* Use sticky parameters if the track is not available. */
		pi->sample_rate = sc->sc_sound_pparams.sample_rate;
		pi->channels    = sc->sc_sound_pparams.channels;
		pi->precision   = sc->sc_sound_pparams.precision;
		pi->encoding    = sc->sc_sound_pparams.encoding;
		pi->pause       = sc->sc_sound_ppause;
	}
	if (rtrack) {
		ri->sample_rate = rtrack->usrbuf.fmt.sample_rate;
		ri->channels    = rtrack->usrbuf.fmt.channels;
		ri->precision   = rtrack->usrbuf.fmt.precision;
		ri->encoding    = rtrack->usrbuf.fmt.encoding;
		ri->pause       = rtrack->is_pause;
	} else {
		/* Use sticky parameters if the track is not available. */
		ri->sample_rate = sc->sc_sound_rparams.sample_rate;
		ri->channels    = sc->sc_sound_rparams.channels;
		ri->precision   = sc->sc_sound_rparams.precision;
		ri->encoding    = sc->sc_sound_rparams.encoding;
		ri->pause       = sc->sc_sound_rpause;
	}

	if (ptrack) {
		pi->seek = ptrack->usrbuf.used;
		pi->samples = ptrack->usrbuf_stamp;
		pi->eof = ptrack->eofcounter;
		pi->error = (ptrack->dropframes != 0) ? 1 : 0;
		pi->open = 1;
		pi->buffer_size = ptrack->usrbuf.capacity;
	}
	pi->waiting = 0;		/* open never hangs */
	pi->active = sc->sc_pbusy;

	if (rtrack) {
		ri->seek = rtrack->usrbuf.used;
		ri->samples = rtrack->usrbuf_stamp;
		ri->eof = 0;
		ri->error = (rtrack->dropframes != 0) ? 1 : 0;
		ri->open = 1;
		ri->buffer_size = rtrack->usrbuf.capacity;
	}
	ri->waiting = 0;		/* open never hangs */
	ri->active = sc->sc_rbusy;

	/*
	 * XXX There may be different number of channels between playback
	 *     and recording, so that blocksize also may be different.
	 *     But struct audio_info has an united blocksize...
	 *     Here, I use play info precedencely if ptrack is available,
	 *     otherwise record info.
	 *
	 * XXX hiwat/lowat is a playback-only parameter.  What should I
	 *     return for a record-only descriptor?
	 */
	track = ptrack ? ptrack : rtrack;
	if (track) {
		ai->blocksize = track->usrbuf_blksize;
		ai->hiwat = track->usrbuf_usedhigh / track->usrbuf_blksize;
		ai->lowat = track->usrbuf_usedlow / track->usrbuf_blksize;
	}
	ai->mode = file->mode;

	/*
	 * For backward compatibility, we have to pad these five fields
	 * a fake non-zero value even if there are no tracks.
	 */
	if (ptrack == NULL)
		pi->buffer_size = 65536;
	if (rtrack == NULL)
		ri->buffer_size = 65536;
	if (ptrack == NULL && rtrack == NULL) {
		ai->blocksize = 2048;
		ai->hiwat = ai->play.buffer_size / ai->blocksize;
		ai->lowat = ai->hiwat * 3 / 4;
	}

	if (need_mixerinfo) {
		mutex_enter(sc->sc_lock);

		pi->port = au_get_port(sc, &sc->sc_outports);
		ri->port = au_get_port(sc, &sc->sc_inports);

		pi->avail_ports = sc->sc_outports.allports;
		ri->avail_ports = sc->sc_inports.allports;

		au_get_gain(sc, &sc->sc_outports, &pi->gain, &pi->balance);
		au_get_gain(sc, &sc->sc_inports, &ri->gain, &ri->balance);

		if (sc->sc_monitor_port != -1) {
			gain = au_get_monitor_gain(sc);
			if (gain != -1)
				ai->monitor_gain = gain;
		}
		mutex_exit(sc->sc_lock);
	}

	return 0;
}

/*
 * Return true if playback is configured.
 * This function can be used after audioattach.
 */
static bool
audio_can_playback(struct audio_softc *sc)
{

	return (sc->sc_pmixer != NULL);
}

/*
 * Return true if recording is configured.
 * This function can be used after audioattach.
 */
static bool
audio_can_capture(struct audio_softc *sc)
{

	return (sc->sc_rmixer != NULL);
}

/*
 * Get the afp->index'th item from the valid one of format[].
 * If found, stores it to afp->fmt and returns 0.  Otherwise return EINVAL.
 *
 * This is common routines for query_format.
 * If your hardware driver has struct audio_format[], the simplest case
 * you can write your query_format interface as follows:
 *
 * struct audio_format foo_format[] = { ... };
 *
 * int
 * foo_query_format(void *hdl, audio_format_query_t *afp)
 * {
 *   return audio_query_format(foo_format, __arraycount(foo_format), afp);
 * }
 */
int
audio_query_format(const struct audio_format *format, int nformats,
	audio_format_query_t *afp)
{
	const struct audio_format *f;
	int idx;
	int i;

	idx = 0;
	for (i = 0; i < nformats; i++) {
		f = &format[i];
		if (!AUFMT_IS_VALID(f))
			continue;
		if (afp->index == idx) {
			afp->fmt = *f;
			return 0;
		}
		idx++;
	}
	return EINVAL;
}

/*
 * This function is provided for the hardware driver's set_format() to
 * find index matches with 'param' from array of audio_format_t 'formats'.
 * 'mode' is either of AUMODE_PLAY or AUMODE_RECORD.
 * It returns the matched index and never fails.  Because param passed to
 * set_format() is selected from query_format().
 * This function will be an alternative to auconv_set_converter() to
 * find index.
 */
int
audio_indexof_format(const struct audio_format *formats, int nformats,
	int mode, const audio_params_t *param)
{
	const struct audio_format *f;
	int index;
	int j;

	for (index = 0; index < nformats; index++) {
		f = &formats[index];

		if (!AUFMT_IS_VALID(f))
			continue;
		if ((f->mode & mode) == 0)
			continue;
		if (f->encoding != param->encoding)
			continue;
		if (f->validbits != param->precision)
			continue;
		if (f->channels != param->channels)
			continue;

		if (f->frequency_type == 0) {
			if (param->sample_rate < f->frequency[0] ||
			    param->sample_rate > f->frequency[1])
				continue;
		} else {
			for (j = 0; j < f->frequency_type; j++) {
				if (param->sample_rate == f->frequency[j])
					break;
			}
			if (j == f->frequency_type)
				continue;
		}

		/* Then, matched */
		return index;
	}

	/* Not matched.  This should not be happened. */
	panic("%s: cannot find matched format\n", __func__);
}

/*
 * Get or set hardware blocksize in msec.
 * XXX It's for debug.
 */
static int
audio_sysctl_blk_ms(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	audio_format2_t phwfmt;
	audio_format2_t rhwfmt;
	audio_filter_reg_t pfil;
	audio_filter_reg_t rfil;
	int t;
	int old_blk_ms;
	int mode;
	int error;

	node = *rnode;
	sc = node.sysctl_data;

	error = audio_exlock_enter(sc);
	if (error)
		return error;

	old_blk_ms = sc->sc_blk_ms;
	t = old_blk_ms;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto abort;

	if (t < 0) {
		error = EINVAL;
		goto abort;
	}

	if (sc->sc_popens + sc->sc_ropens > 0) {
		error = EBUSY;
		goto abort;
	}
	sc->sc_blk_ms = t;
	mode = 0;
	if (sc->sc_pmixer) {
		mode |= AUMODE_PLAY;
		phwfmt = sc->sc_pmixer->hwbuf.fmt;
	}
	if (sc->sc_rmixer) {
		mode |= AUMODE_RECORD;
		rhwfmt = sc->sc_rmixer->hwbuf.fmt;
	}

	/* re-init hardware */
	memset(&pfil, 0, sizeof(pfil));
	memset(&rfil, 0, sizeof(rfil));
	error = audio_hw_set_format(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
	if (error) {
		goto abort;
	}

	/* re-init track mixer */
	error = audio_mixers_init(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
	if (error) {
		/* Rollback */
		sc->sc_blk_ms = old_blk_ms;
		audio_mixers_init(sc, mode, &phwfmt, &rhwfmt, &pfil, &rfil);
		goto abort;
	}
	error = 0;
abort:
	audio_exlock_exit(sc);
	return error;
}

/*
 * Get or set multiuser mode.
 */
static int
audio_sysctl_multiuser(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *sc;
	bool t;
	int error;

	node = *rnode;
	sc = node.sysctl_data;

	error = audio_exlock_enter(sc);
	if (error)
		return error;

	t = sc->sc_multiuser;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto abort;

	sc->sc_multiuser = t;
	error = 0;
abort:
	audio_exlock_exit(sc);
	return error;
}

#if defined(AUDIO_DEBUG)
/*
 * Get or set debug verbose level. (0..4)
 * XXX It's for debug.
 * XXX It is not separated per device.
 */
static int
audio_sysctl_debug(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int t;
	int error;

	node = *rnode;
	t = audiodebug;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 4)
		return EINVAL;
	audiodebug = t;
	printf("audio: audiodebug = %d\n", audiodebug);
	return 0;
}
#endif /* AUDIO_DEBUG */

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

	if (!pmf_device_suspend(sc->hw_dev, PMF_Q_SELF))
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
		pmf_device_resume(sc->hw_dev, PMF_Q_SELF);
		pmf_device_resume(dv, PMF_Q_SELF);
	}
}
#endif

static bool
audio_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct audio_softc *sc = device_private(dv);
	int error;

	error = audio_exlock_mutex_enter(sc);
	if (error)
		return error;
	sc->sc_suspending = true;
	audio_mixer_capture(sc);

	if (sc->sc_pbusy) {
		audio_pmixer_halt(sc);
		/* Reuse this as need-to-restart flag while suspending */
		sc->sc_pbusy = true;
	}
	if (sc->sc_rbusy) {
		audio_rmixer_halt(sc);
		/* Reuse this as need-to-restart flag while suspending */
		sc->sc_rbusy = true;
	}

#ifdef AUDIO_PM_IDLE
	callout_halt(&sc->sc_idle_counter, sc->sc_lock);
#endif
	audio_exlock_mutex_exit(sc);

	return true;
}

static bool
audio_resume(device_t dv, const pmf_qual_t *qual)
{
	struct audio_softc *sc = device_private(dv);
	struct audio_info ai;
	int error;

	error = audio_exlock_mutex_enter(sc);
	if (error)
		return error;

	sc->sc_suspending = false;
	audio_mixer_restore(sc);
	/* XXX ? */
	AUDIO_INITINFO(&ai);
	audio_hw_setinfo(sc, &ai, NULL);

	/*
	 * During from suspend to resume here, sc_[pr]busy is used as
	 * need-to-restart flag temporarily.  After this point,
	 * sc_[pr]busy is returned to its original usage (busy flag).
	 * And note that sc_[pr]busy must be false to call [pr]mixer_start().
	 */
	if (sc->sc_pbusy) {
		/* pmixer_start() requires pbusy is false */
		sc->sc_pbusy = false;
		audio_pmixer_start(sc, true);
	}
	if (sc->sc_rbusy) {
		/* rmixer_start() requires rbusy is false */
		sc->sc_rbusy = false;
		audio_rmixer_start(sc);
	}

	audio_exlock_mutex_exit(sc);

	return true;
}

#if defined(AUDIO_DEBUG)
static void
audio_format2_tostr(char *buf, size_t bufsize, const audio_format2_t *fmt)
{
	int n;

	n = 0;
	n += snprintf(buf + n, bufsize - n, "%s",
	    audio_encoding_name(fmt->encoding));
	if (fmt->precision == fmt->stride) {
		n += snprintf(buf + n, bufsize - n, " %dbit", fmt->precision);
	} else {
		n += snprintf(buf + n, bufsize - n, " %d/%dbit",
			fmt->precision, fmt->stride);
	}

	snprintf(buf + n, bufsize - n, " %uch %uHz",
	    fmt->channels, fmt->sample_rate);
}
#endif

#if defined(AUDIO_DEBUG)
static void
audio_print_format2(const char *s, const audio_format2_t *fmt)
{
	char fmtstr[64];

	audio_format2_tostr(fmtstr, sizeof(fmtstr), fmt);
	printf("%s %s\n", s, fmtstr);
}
#endif

#ifdef DIAGNOSTIC
void
audio_diagnostic_format2(const char *where, const audio_format2_t *fmt)
{

	KASSERTMSG(fmt, "called from %s", where);

	/* XXX MSM6258 vs(4) only has 4bit stride format. */
	if (fmt->encoding == AUDIO_ENCODING_ADPCM) {
		KASSERTMSG(fmt->stride == 4 || fmt->stride == 8,
		    "called from %s: fmt->stride=%d", where, fmt->stride);
	} else {
		KASSERTMSG(fmt->stride % NBBY == 0,
		    "called from %s: fmt->stride=%d", where, fmt->stride);
	}
	KASSERTMSG(fmt->precision <= fmt->stride,
	    "called from %s: fmt->precision=%d fmt->stride=%d",
	    where, fmt->precision, fmt->stride);
	KASSERTMSG(1 <= fmt->channels && fmt->channels <= AUDIO_MAX_CHANNELS,
	    "called from %s: fmt->channels=%d", where, fmt->channels);

	/* XXX No check for encodings? */
}

void
audio_diagnostic_filter_arg(const char *where, const audio_filter_arg_t *arg)
{

	KASSERT(arg != NULL);
	KASSERT(arg->src != NULL);
	KASSERT(arg->dst != NULL);
	audio_diagnostic_format2(where, arg->srcfmt);
	audio_diagnostic_format2(where, arg->dstfmt);
	KASSERT(arg->count > 0);
}

void
audio_diagnostic_ring(const char *where, const audio_ring_t *ring)
{

	KASSERTMSG(ring, "called from %s", where);
	audio_diagnostic_format2(where, &ring->fmt);
	KASSERTMSG(0 <= ring->capacity && ring->capacity < INT_MAX / 2,
	    "called from %s: ring->capacity=%d", where, ring->capacity);
	KASSERTMSG(0 <= ring->used && ring->used <= ring->capacity,
	    "called from %s: ring->used=%d ring->capacity=%d",
	    where, ring->used, ring->capacity);
	if (ring->capacity == 0) {
		KASSERTMSG(ring->mem == NULL,
		    "called from %s: capacity == 0 but mem != NULL", where);
	} else {
		KASSERTMSG(ring->mem != NULL,
		    "called from %s: capacity != 0 but mem == NULL", where);
		KASSERTMSG(0 <= ring->head && ring->head < ring->capacity,
		    "called from %s: ring->head=%d ring->capacity=%d",
		    where, ring->head, ring->capacity);
	}
}
#endif /* DIAGNOSTIC */


/*
 * Mixer driver
 */

/*
 * Must be called without sc_lock held.
 */
int
mixer_open(dev_t dev, struct audio_softc *sc, int flags, int ifmt,
	struct lwp *l)
{
	struct file *fp;
	audio_file_t *af;
	int error, fd;

	TRACE(1, "flags=0x%x", flags);

	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	af = kmem_zalloc(sizeof(*af), KM_SLEEP);
	af->sc = sc;
	af->dev = dev;

	mutex_enter(sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(sc->sc_lock);
		kmem_free(af, sizeof(*af));
		fd_abort(curproc, fp, fd);
		return ENXIO;
	}
	mutex_enter(sc->sc_intr_lock);
	SLIST_INSERT_HEAD(&sc->sc_files, af, entry);
	mutex_exit(sc->sc_intr_lock);
	mutex_exit(sc->sc_lock);

	error = fd_clone(fp, fd, flags, &audio_fileops, af);
	KASSERT(error == EMOVEFD);

	return error;
}

/*
 * Add a process to those to be signalled on mixer activity.
 * If the process has already been added, do nothing.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static void
mixer_async_add(struct audio_softc *sc, pid_t pid)
{
	int i;

	KASSERT(sc->sc_exlock);

	/* If already exists, returns without doing anything. */
	for (i = 0; i < sc->sc_am_used; i++) {
		if (sc->sc_am[i] == pid)
			return;
	}

	/* Extend array if necessary. */
	if (sc->sc_am_used >= sc->sc_am_capacity) {
		sc->sc_am_capacity += AM_CAPACITY;
		sc->sc_am = kern_realloc(sc->sc_am,
		    sc->sc_am_capacity * sizeof(pid_t), M_WAITOK);
		TRACE(2, "realloc am_capacity=%d", sc->sc_am_capacity);
	}

	TRACE(2, "am[%d]=%d", sc->sc_am_used, (int)pid);
	sc->sc_am[sc->sc_am_used++] = pid;
}

/*
 * Remove a process from those to be signalled on mixer activity.
 * If the process has not been added, do nothing.
 * Must be called with sc_exlock held and without sc_lock held.
 */
static void
mixer_async_remove(struct audio_softc *sc, pid_t pid)
{
	int i;

	KASSERT(sc->sc_exlock);

	for (i = 0; i < sc->sc_am_used; i++) {
		if (sc->sc_am[i] == pid) {
			sc->sc_am[i] = sc->sc_am[--sc->sc_am_used];
			TRACE(2, "am[%d](%d) removed, used=%d",
			    i, (int)pid, sc->sc_am_used);

			/* Empty array if no longer necessary. */
			if (sc->sc_am_used == 0) {
				kern_free(sc->sc_am);
				sc->sc_am = NULL;
				sc->sc_am_capacity = 0;
				TRACE(2, "released");
			}
			return;
		}
	}
}

/*
 * Signal all processes waiting for the mixer.
 * Must be called with sc_exlock held.
 */
static void
mixer_signal(struct audio_softc *sc)
{
	proc_t *p;
	int i;

	KASSERT(sc->sc_exlock);

	for (i = 0; i < sc->sc_am_used; i++) {
		mutex_enter(&proc_lock);
		p = proc_find(sc->sc_am[i]);
		if (p)
			psignal(p, SIGIO);
		mutex_exit(&proc_lock);
	}
}

/*
 * Close a mixer device
 */
int
mixer_close(struct audio_softc *sc, audio_file_t *file)
{
	int error;

	error = audio_exlock_enter(sc);
	if (error)
		return error;
	TRACE(1, "called");
	mixer_async_remove(sc, curproc->p_pid);
	audio_exlock_exit(sc);

	return 0;
}

/*
 * Must be called without sc_lock nor sc_exlock held.
 */
int
mixer_ioctl(struct audio_softc *sc, u_long cmd, void *addr, int flag,
	struct lwp *l)
{
	mixer_devinfo_t *mi;
	mixer_ctrl_t *mc;
	int error;

	TRACE(2, "(%lu,'%c',%lu)",
	    IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd & 0xff);
	error = EINVAL;

	/* we can return cached values if we are sleeping */
	if (cmd != AUDIO_MIXER_READ) {
		mutex_enter(sc->sc_lock);
		device_active(sc->sc_dev, DVA_SYSTEM);
		mutex_exit(sc->sc_lock);
	}

	switch (cmd) {
	case FIOASYNC:
		error = audio_exlock_enter(sc);
		if (error)
			break;
		if (*(int *)addr) {
			mixer_async_add(sc, curproc->p_pid);
		} else {
			mixer_async_remove(sc, curproc->p_pid);
		}
		audio_exlock_exit(sc);
		break;

	case AUDIO_GETDEV:
		TRACE(2, "AUDIO_GETDEV");
		error = sc->hw_if->getdev(sc->hw_hdl, (audio_device_t *)addr);
		break;

	case AUDIO_MIXER_DEVINFO:
		TRACE(2, "AUDIO_MIXER_DEVINFO");
		mi = (mixer_devinfo_t *)addr;

		mi->un.v.delta = 0; /* default */
		mutex_enter(sc->sc_lock);
		error = audio_query_devinfo(sc, mi);
		mutex_exit(sc->sc_lock);
		break;

	case AUDIO_MIXER_READ:
		TRACE(2, "AUDIO_MIXER_READ");
		mc = (mixer_ctrl_t *)addr;

		error = audio_exlock_mutex_enter(sc);
		if (error)
			break;
		if (device_is_active(sc->hw_dev))
			error = audio_get_port(sc, mc);
		else if (mc->dev < 0 || mc->dev >= sc->sc_nmixer_states)
			error = ENXIO;
		else {
			int dev = mc->dev;
			memcpy(mc, &sc->sc_mixer_state[dev],
			    sizeof(mixer_ctrl_t));
			error = 0;
		}
		audio_exlock_mutex_exit(sc);
		break;

	case AUDIO_MIXER_WRITE:
		TRACE(2, "AUDIO_MIXER_WRITE");
		error = audio_exlock_mutex_enter(sc);
		if (error)
			break;
		error = audio_set_port(sc, (mixer_ctrl_t *)addr);
		if (error) {
			audio_exlock_mutex_exit(sc);
			break;
		}

		if (sc->hw_if->commit_settings) {
			error = sc->hw_if->commit_settings(sc->hw_hdl);
			if (error) {
				audio_exlock_mutex_exit(sc);
				break;
			}
		}
		mutex_exit(sc->sc_lock);
		mixer_signal(sc);
		audio_exlock_exit(sc);
		break;

	default:
		if (sc->hw_if->dev_ioctl) {
			mutex_enter(sc->sc_lock);
			error = sc->hw_if->dev_ioctl(sc->hw_hdl,
			    cmd, addr, flag, l);
			mutex_exit(sc->sc_lock);
		} else
			error = EINVAL;
		break;
	}
	TRACE(2, "(%lu,'%c',%lu) result %d",
	    IOCPARM_LEN(cmd), (char)IOCGROUP(cmd), cmd & 0xff, error);
	return error;
}

/*
 * Must be called with sc_lock held.
 */
int
au_portof(struct audio_softc *sc, char *name, int class)
{
	mixer_devinfo_t mi;

	KASSERT(mutex_owned(sc->sc_lock));

	for (mi.index = 0; audio_query_devinfo(sc, &mi) == 0; mi.index++) {
		if (mi.mixer_class == class && strcmp(mi.label.name, name) == 0)
			return mi.index;
	}
	return -1;
}

/*
 * Must be called with sc_lock held.
 */
void
au_setup_ports(struct audio_softc *sc, struct au_mixer_ports *ports,
	mixer_devinfo_t *mi, const struct portname *tbl)
{
	int i, j;

	KASSERT(mutex_owned(sc->sc_lock));

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
 * Must be called with sc_lock && sc_exlock held.
 */
int
au_set_lr_value(struct audio_softc *sc, mixer_ctrl_t *ct, int l, int r)
{

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

/*
 * Must be called with sc_lock && sc_exlock held.
 */
int
au_get_lr_value(struct audio_softc *sc, mixer_ctrl_t *ct, int *l, int *r)
{
	int error;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

/*
 * Must be called with sc_lock && sc_exlock held.
 */
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
	KASSERT(sc->sc_exlock);

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
	TRACE(2, "gain=%d balance=%d, l=%d r=%d", gain, balance, l, r);

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

/*
 * Must be called with sc_lock && sc_exlock held.
 */
void
au_get_gain(struct audio_softc *sc, struct au_mixer_ports *ports,
	u_int *pgain, u_char *pbalance)
{
	mixer_ctrl_t ct;
	int i, l, r, n;
	int lgain, rgain;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

/*
 * Must be called with sc_lock && sc_exlock held.
 */
int
au_set_port(struct audio_softc *sc, struct au_mixer_ports *ports, u_int port)
{
	mixer_ctrl_t ct;
	int i, error, use_mixerout;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

/*
 * Must be called with sc_lock && sc_exlock held.
 */
int
au_get_port(struct audio_softc *sc, struct au_mixer_ports *ports)
{
	mixer_ctrl_t ct;
	int i, aumask;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

/*
 * It returns 0 if success, otherwise errno.
 * Must be called only if sc->sc_monitor_port != -1.
 * Must be called with sc_lock && sc_exlock held.
 */
static int
au_set_monitor_gain(struct audio_softc *sc, int monitor_gain)
{
	mixer_ctrl_t ct;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	ct.dev = sc->sc_monitor_port;
	ct.type = AUDIO_MIXER_VALUE;
	ct.un.value.num_channels = 1;
	ct.un.value.level[AUDIO_MIXER_LEVEL_MONO] = monitor_gain;
	return audio_set_port(sc, &ct);
}

/*
 * It returns monitor gain if success, otherwise -1.
 * Must be called only if sc->sc_monitor_port != -1.
 * Must be called with sc_lock && sc_exlock held.
 */
static int
au_get_monitor_gain(struct audio_softc *sc)
{
	mixer_ctrl_t ct;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	ct.dev = sc->sc_monitor_port;
	ct.type = AUDIO_MIXER_VALUE;
	ct.un.value.num_channels = 1;
	if (audio_get_port(sc, &ct))
		return -1;
	return ct.un.value.level[AUDIO_MIXER_LEVEL_MONO];
}

/*
 * Must be called with sc_lock && sc_exlock held.
 */
static int
audio_set_port(struct audio_softc *sc, mixer_ctrl_t *mc)
{

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	return sc->hw_if->set_port(sc->hw_hdl, mc);
}

/*
 * Must be called with sc_lock && sc_exlock held.
 */
static int
audio_get_port(struct audio_softc *sc, mixer_ctrl_t *mc)
{

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

	return sc->hw_if->get_port(sc->hw_hdl, mc);
}

/*
 * Must be called with sc_lock && sc_exlock held.
 */
static void
audio_mixer_capture(struct audio_softc *sc)
{
	mixer_devinfo_t mi;
	mixer_ctrl_t *mc;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

/*
 * Must be called with sc_lock && sc_exlock held.
 */
static void
audio_mixer_restore(struct audio_softc *sc)
{
	mixer_devinfo_t mi;
	mixer_ctrl_t *mc;

	KASSERT(mutex_owned(sc->sc_lock));
	KASSERT(sc->sc_exlock);

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

static void
audio_volume_down(device_t dv)
{
	struct audio_softc *sc = device_private(dv);
	mixer_devinfo_t mi;
	int newgain;
	u_int gain;
	u_char balance;

	if (audio_exlock_mutex_enter(sc) != 0)
		return;
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
	audio_exlock_mutex_exit(sc);
}

static void
audio_volume_up(device_t dv)
{
	struct audio_softc *sc = device_private(dv);
	mixer_devinfo_t mi;
	u_int gain, newgain;
	u_char balance;

	if (audio_exlock_mutex_enter(sc) != 0)
		return;
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
	audio_exlock_mutex_exit(sc);
}

static void
audio_volume_toggle(device_t dv)
{
	struct audio_softc *sc = device_private(dv);
	u_int gain, newgain;
	u_char balance;

	if (audio_exlock_mutex_enter(sc) != 0)
		return;
	au_get_gain(sc, &sc->sc_outports, &gain, &balance);
	if (gain != 0) {
		sc->sc_lastgain = gain;
		newgain = 0;
	} else
		newgain = sc->sc_lastgain;
	au_set_gain(sc, &sc->sc_outports, newgain, balance);
	audio_exlock_mutex_exit(sc);
}

/*
 * Must be called with sc_lock held.
 */
static int
audio_query_devinfo(struct audio_softc *sc, mixer_devinfo_t *di)
{

	KASSERT(mutex_owned(sc->sc_lock));

	return sc->hw_if->query_devinfo(sc->hw_hdl, di);
}

#endif /* NAUDIO > 0 */

#if NAUDIO == 0 && (NMIDI > 0 || NMIDIBUS > 0)
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <dev/audio/audio_if.h>
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
		case AUDIODEV_TYPE_AUX:
			type = "aux";
			break;
		default:
			panic("audioprint: unknown type %d", arg->type);
		}
		aprint_normal("%s at %s", type, pnp);
	}
	return UNCONF;
}

#endif /* NAUDIO > 0 || (NMIDI > 0 || NMIDIBUS > 0) */

#ifdef _MODULE

devmajor_t audio_bmajor = -1, audio_cmajor = -1;

#include "ioconf.c"

#endif

MODULE(MODULE_CLASS_DRIVER, audio, NULL);

static int
audio_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/* XXX interrupt level? */
		audio_psref_class = psref_class_create("audio", IPL_SOFTSERIAL);
#ifdef _MODULE
		error = devsw_attach(audio_cd.cd_name, NULL, &audio_bmajor,
		    &audio_cdevsw, &audio_cmajor);
		if (error)
			break;

		error = config_init_component(cfdriver_ioconf_audio,
		    cfattach_ioconf_audio, cfdata_ioconf_audio);
		if (error) {
			devsw_detach(NULL, &audio_cdevsw);
		}
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(NULL, &audio_cdevsw);
		error = config_fini_component(cfdriver_ioconf_audio,
		   cfattach_ioconf_audio, cfdata_ioconf_audio);
		if (error)
			devsw_attach(audio_cd.cd_name, NULL, &audio_bmajor,
			    &audio_cdevsw, &audio_cmajor);
#endif
		psref_class_destroy(audio_psref_class);
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
