/*	$NetBSD: audiodef.h,v 1.2 2019/05/08 13:40:17 isaki Exp $	*/

/*
 * Copyright (C) 2017 Tetsuya Isaki. All rights reserved.
 * Copyright (C) 2017 Y.Sugahara (moveccr). All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_DEV_AUDIO_AUDIODEF_H_
#define _SYS_DEV_AUDIO_AUDIODEF_H_

/* Number of HW buffer's blocks. */
#define NBLKHW (3)

/* Number of track output buffer's blocks.  Must be > NBLKHW */
#define NBLKOUT	(4)

/* Minimum number of usrbuf's blocks. */
#define AUMINNOBLK	(3)

/*
 * Hardware blocksize in msec.
 * We use 40 msec as default.  (1 / 40ms) = 25 = 5^2.
 * In this case, the number of frames in a block can be an integer
 * even if the frequency is a multiple of 100 (44100, 48000, etc),
 * or even if 15625Hz (vs(4)).
 */
#if !defined(AUDIO_BLK_MS)
#define AUDIO_BLK_MS 40
#endif

/*
 * Whether the playback mixer use single buffer mode.
 * It reduces the latency one block but needs machine power.
 * In case of the double buffer (as default), it increases the latency
 * but can be expected to stabilize even on slower machines.
 */
/* #define AUDIO_HW_SINGLE_BUFFER */

/*
 * Whether supports per-track volume.
 * For now, there are no user interfaces to get/set it.
 */
/* #define AUDIO_SUPPORT_TRACK_VOLUME */

/*
 * Whether use C language's "implementation defined" behavior (note that
 * it's not "undefined" behavior).  It improves performance well.
 */
#define AUDIO_USE_C_IMPLEMENTATION_DEFINED_BEHAVIOR

/* conversion stage */
typedef struct {
	audio_ring_t srcbuf;
	audio_ring_t *dst;
	audio_filter_t filter;
	audio_filter_arg_t arg;
} audio_stage_t;

typedef enum {
	AUDIO_STATE_CLEAR,	/* no data, no need to drain */
	AUDIO_STATE_RUNNING,	/* need to drain */
	AUDIO_STATE_DRAINING,	/* now draining */
} audio_state_t;

typedef struct audio_track {
	/*
	 * AUMODE_PLAY for playback track, or
	 * AUMODE_RECORD for recoding track.
	 * Note that AUMODE_PLAY_ALL is maintained by file->mode, not here.
	 */
	int mode;

	audio_ring_t	usrbuf;		/* user i/o buffer */
	u_int		usrbuf_blksize;	/* usrbuf block size in bytes */
	struct uvm_object *uobj;
	bool		mmapped;	/* device is mmap()-ed */
	u_int		usrbuf_stamp;	/* transferred bytes from/to stage */
	u_int		usrbuf_stamp_last; /* last stamp */
	u_int		usrbuf_usedhigh;/* high water mark in bytes */
	u_int		usrbuf_usedlow;	/* low water mark in bytes */

	/*
	 * Track input format.  It means usrbuf.fmt for playback, or
	 * mixer->trackfmt for recording.
	 */
	audio_format2_t	inputfmt;

	/*
	 * Pointer to track (conversion stage's) input buffer.
	 * Must be protected by track lock (only for recording track).
	 */
	audio_ring_t	*input;
	/*
	 * Track (conversion stage's) output buffer.
	 * Must be protected by track lock (only for playback track).
	 */
	audio_ring_t	outbuf;

	audio_stage_t	codec;		/* encoding conversion stage */
	audio_stage_t	chvol;		/* channel volume stage */
	audio_stage_t	chmix;		/* channel mix stage */
	audio_stage_t	freq;		/* frequency conversion stage */

	/* Work area for frequency conversion.  */
	u_int		freq_step;	/* src/dst ratio */
	u_int		freq_current;	/* counter */
	u_int		freq_leap;	/* correction counter per block */
	aint_t		freq_prev[AUDIO_MAX_CHANNELS];	/* previous values */
	aint_t		freq_curr[AUDIO_MAX_CHANNELS];	/* current values */

	/* Per-channel volumes (0..256) */
	uint16_t ch_volume[AUDIO_MAX_CHANNELS];
#if defined(AUDIO_SUPPORT_TRACK_VOLUME)
	/* Track volume (0..256) */
	u_int		volume;
#endif

	audio_trackmixer_t *mixer;	/* connected track mixer */

	/* Sequence number picked up by track mixer. */
	uint64_t	seq;

	audio_state_t	pstate;		/* playback state */
	bool		is_pause;

	/* Statistic counters. */
	uint64_t	inputcounter;	/* # of frames input to track */
	uint64_t	outputcounter;	/* # of frames output from track */
	uint64_t	useriobytes;	/* # of bytes xfer to/from userland */
	uint64_t	dropframes;	/* # of frames dropped */
	int		eofcounter;	/* count of zero-sized write */

	/*
	 * Non-zero if the track is in use.
	 * Must access atomically.
	 */
	volatile uint	lock;

	int		id;		/* track id for debug */
} audio_track_t;

struct audio_file {
	struct audio_softc *sc;
	dev_t		dev;

	/*
	 * Playback and recording track, or NULL if the track is unavailable.
	 */
	audio_track_t	*ptrack;
	audio_track_t	*rtrack;

	/*
	 * Indicates the operation mode of this file.
	 * AUMODE_PLAY means playback is requested.
	 * AUMODE_RECORD means recording is requested.
	 * AUMODE_PLAY_ALL affects nothing but can be get/set for backward
	 * compatibility.
	 */
	int		mode;

	/* process who wants audio SIGIO. */
	pid_t		async_audio;

	/*
	 * Non-zero if some thread context is using this file structure
	 * (including ptrack and rtrack) now.
	 * Must be protected by sc_lock.
	 */
	volatile int lock;

	SLIST_ENTRY(audio_file) entry;
};

struct audio_trackmixer {
	struct audio_softc *sc;

	int		mode;		/* AUMODE_PLAY or AUMODE_RECORD */
	audio_format2_t	track_fmt;	/* track <-> trackmixer format */

	int		frames_per_block; /* number of frames in a block */

	u_int		volume;		/* software master volume (0..256) */

	audio_format2_t	mixfmt;
	void		*mixsample;	/* mixing buf in double-sized int */

	/*
	 * true if trackmixer does LE<->BE conversion.
	 * Generally an encoding conversion should be done by each hardware
	 * driver but for most modern little endian drivers which support
	 * only linear PCM it's troublesome issue to consider about big endian
	 * arch.  Therefore, we do this conversion here only if the hardware
	 * format is SLINEAR_OE:16.
	 */
	bool		swap_endian;

	audio_filter_t	codec;		/* hardware codec */
	audio_filter_arg_t codecarg;	/* and its argument */
	audio_ring_t	codecbuf;	/* also used for wide->int conversion */

	audio_ring_t	hwbuf;		/* HW I/O buf */

	void		*sih;		/* softint cookie */

	/* Must be protected by sc_lock. */
	kcondvar_t	outcv;

	uint64_t	mixseq;		/* seq# currently being mixed */
	uint64_t	hwseq;		/* seq# HW output completed */

	/* initial blktime n/d = AUDIO_BLK_MS / 1000 */
	int		blktime_n;	/* blk time numerator */
	int		blktime_d;	/* blk time denominator */

	/* XXX */
	uint64_t	hw_complete_counter;
};

/*
 * Audio Ring Buffer.
 */

#ifdef DIAGNOSTIC
#define DIAGNOSTIC_ring(ring)	audio_diagnostic_ring(__func__, (ring))
extern void audio_diagnostic_ring(const char *, const audio_ring_t *);
#else
#define DIAGNOSTIC_ring(ring)
#endif

/*
 * Convert number of frames to number of bytes.
 */
static __inline int
frametobyte(const audio_format2_t *fmt, int frames)
{
	return frames * fmt->channels * fmt->stride / NBBY;
}

/*
 * Return the number of frames per block.
 */
static __inline int
frame_per_block(const audio_trackmixer_t *mixer, const audio_format2_t *fmt)
{
	return (fmt->sample_rate * mixer->blktime_n + mixer->blktime_d - 1) /
	    mixer->blktime_d;
}

/*
 * Round idx.  idx must be non-negative and less than 2 * capacity.
 */
static __inline int
auring_round(const audio_ring_t *ring, int idx)
{
	DIAGNOSTIC_ring(ring);
	KASSERT(idx >= 0);
	KASSERT(idx < ring->capacity * 2);

	if (idx < ring->capacity) {
		return idx;
	} else {
		return idx - ring->capacity;
	}
}

/*
 * Return ring's tail (= head + used) position.
 * This position indicates next frame of the last valid frames.
 */
static __inline int
auring_tail(const audio_ring_t *ring)
{
	return auring_round(ring, ring->head + ring->used);
}

/*
 * Return ring's head pointer.
 * This function can be used only if the stride of the 'ring' is equal to
 * the internal stride.  Don't use this for hw buffer.
 */
static __inline aint_t *
auring_headptr_aint(const audio_ring_t *ring)
{
	KASSERT(ring->fmt.stride == sizeof(aint_t) * NBBY);

	return (aint_t *)ring->mem + ring->head * ring->fmt.channels;
}

/*
 * Return ring's tail (= head + used) pointer.
 * This function can be used only if the stride of the 'ring' is equal to
 * the internal stride.  Don't use this for hw buffer.
 */
static __inline aint_t *
auring_tailptr_aint(const audio_ring_t *ring)
{
	KASSERT(ring->fmt.stride == sizeof(aint_t) * NBBY);

	return (aint_t *)ring->mem + auring_tail(ring) * ring->fmt.channels;
}

/*
 * Return ring's head pointer.
 * This function can be used even if the stride of the 'ring' is equal to
 * or not equal to the internal stride.
 */
static __inline uint8_t *
auring_headptr(const audio_ring_t *ring)
{
	return (uint8_t *)ring->mem +
	    ring->head * ring->fmt.channels * ring->fmt.stride / NBBY;
}

/*
 * Return ring's tail pointer.
 * It points the next position of the last valid frames.
 * This function can be used even if the stride of the 'ring' is equal to
 * or not equal to the internal stride.
 */
static __inline uint8_t *
auring_tailptr(audio_ring_t *ring)
{
	return (uint8_t *)ring->mem +
	    auring_tail(ring) * ring->fmt.channels * ring->fmt.stride / NBBY;
}

/*
 * Return ring's capacity in bytes.
 */
static __inline int
auring_bytelen(const audio_ring_t *ring)
{
	return frametobyte(&ring->fmt, ring->capacity);
}

/*
 * Take out n frames from head of ring.
 * This function only manipurates counters.  It doesn't manipurate any
 * actual buffer data.
 */
#define auring_take(ring, n)	auring_take_(__func__, __LINE__, ring, n)
static __inline void
auring_take_(const char *func, int line, audio_ring_t *ring, int n)
{
	DIAGNOSTIC_ring(ring);
	KASSERTMSG(n >= 0, "called from %s:%d: n=%d", func, line, n);
	KASSERTMSG(ring->used >= n, "called from %s:%d: ring->used=%d n=%d",
	    func, line, ring->used, n);

	ring->head = auring_round(ring, ring->head + n);
	ring->used -= n;
}

/*
 * Append n frames into tail of ring.
 * This function only manipurates counters.  It doesn't manipurate any
 * actual buffer data.
 */
#define auring_push(ring, n)	auring_push_(__func__, __LINE__, ring, n)
static __inline void
auring_push_(const char *func, int line, audio_ring_t *ring, int n)
{
	DIAGNOSTIC_ring(ring);
	KASSERT(n >= 0);
	KASSERTMSG(ring->used + n <= ring->capacity,
	    "called from %s:%d: ring->used=%d n=%d ring->capacity=%d",
	    func, line, ring->used, n, ring->capacity);

	ring->used += n;
}

/*
 * Return the number of contiguous frames in used.
 */
static __inline int
auring_get_contig_used(const audio_ring_t *ring)
{
	DIAGNOSTIC_ring(ring);

	if (ring->head + ring->used <= ring->capacity) {
		return ring->used;
	} else {
		return ring->capacity - ring->head;
	}
}

/*
 * Return the number of contiguous free frames.
 */
static __inline int
auring_get_contig_free(const audio_ring_t *ring)
{
	DIAGNOSTIC_ring(ring);

	if (ring->head + ring->used < ring->capacity) {
		return ring->capacity - (ring->head + ring->used);
	} else {
		return ring->capacity - ring->used;
	}
}

#endif /* !_SYS_DEV_AUDIO_AUDIODEF_H_ */
