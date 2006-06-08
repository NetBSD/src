/*	$NetBSD: midisynvar.h,v 1.9.14.14 2006/06/08 04:55:23 chap Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org).
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

#ifndef _SYS_DEV_MIDISYNVAR_H_
#define _SYS_DEV_MIDISYNVAR_H_

#include "midictl.h"

typedef struct midisyn midisyn;

/*
 * Important: any synth driver that uses midisyn must set up its methods
 * structure in a way that refers to members /by name/ and zeroes the rest
 * (as is the effect of C99 initializers with designators). This discipline
 * will allow the structure to evolve methods for newly implemented
 * functionality or to exploit capabilities of new drivers with a minimal
 * versioning burden.  Because midisyn is at present a very rudimentary and
 * partial implementation, change should be expected in this set of methods.
 * Do not hesitate to add one in the course of implementing new stuff, as
 * long as it will be generally useful and there is some reasonable fallback
 * for drivers without it.
 */
struct midisyn_methods {
	int  (*open)	(midisyn *, int /* flags */);
	void (*close)   (midisyn *);
	int  (*ioctl)   (midisyn *, u_long, caddr_t, int, struct lwp *);
	/*
	 * allocv(midisyn *ms, uint_fast8_t chan, uint_fast8_t key);
	 * Allocate one of the devices actual voices (stealing one if
	 * necessary) to play note number 'key' (the MIDI note number, not
	 * a frequency) associated with MIDI channel chan. An implementation
	 * might want to choose a voice already last used on chan, to save
	 * shuffling of patches.
	 * One day a variant of this method will probably be needed, with an
	 * extra argument indicating whether a melodic or percussive voice is
	 * wanted.
	 */
	int  (*allocv)  (midisyn *, uint_fast8_t, uint_fast8_t);
	void (*noteon)  (midisyn *, uint32_t, uint32_t, uint32_t);
	void (*noteoff) (midisyn *, uint32_t, uint32_t, uint32_t);
	void (*pgmchg)  (midisyn *, uint32_t, uint32_t);
};

struct voice {
	u_int chan_note;	/* channel and note */
#define MS_CHANNOTE(chan, note) ((chan) * 256 + (note))
#define MS_GETCHAN(v) ((v)->chan_note >> 8)
	u_int seqno;		/* allocation index (increases with time) */
	u_char inuse;
};

#define MIDI_MAX_CHANS 16

struct midisyn {
	/* Filled by synth driver */
	struct midisyn_methods *mets;
	char name[32];
	int nvoice;
	int flags;
#define MS_DOALLOC	1 /* obsolescent: implied if driver has no allocv */
#define MS_FREQXLATE	2
	void *data;

	/* Set up by midisyn but available to synth driver for reading ctls */
	/*
	 * Note - there is currently no locking on this structure; if the synth
	 * driver interacts with midictl it should do so synchronously, when
	 * processing a call from midisyn, and not at other times such as upon
	 * an interrupt. (may revisit locking if problems crop up.)
	 */
	midictl ctl;
	
	/* Used by midisyn driver */
	struct voice *voices;
	u_int seqno;
	u_int16_t pgms[MIDI_MAX_CHANS]; /* ref'd if driver uses MS_GETPGM */
};

#define MS_GETPGM(ms, vno) ((ms)->pgms[MS_GETCHAN(&(ms)->voices[vno])])

struct midi_softc;

extern const struct midi_hw_if midisyn_hw_if;

void	midisyn_attach (struct midi_softc *, midisyn *);

/*
 * Convert a 14-bit volume or expression controller value to centibels using
 * the General MIDI formula. The maximum controller value translates to 0 cB
 * (no attenuation), a half-range controller to -119 cB (level cut by 11.9 dB)
 * and a zero controller to INT16_MIN. If you are converting a 7-bit value
 * just shift it 7 bits left first.
 */
extern int16_t midisyn_vol2cB(uint_fast16_t);

/*
 * MIDI RP-012 constitutes a MIDI Tuning Specification. The units are
 * fractional-MIDIkeys, that is, the key number 00 - 7f left shifted
 * 14 bits to provide a 14-bit fraction that divides each semitone. The
 * whole thing is just a 21-bit number that is bent and tuned simply by
 * adding and subtracting--the same offset is the same pitch change anywhere
 * on the scale. One downside is that a cent is 163.84 of these units, so
 * you can't expect a lengthy integer sum of cents to come out in tune; if you
 * do anything in cents it is best to use them only for local adjustment of
 * a pitch.
 * 
 * This function converts a pitch in MIDItune units to Hz left-shifted 18 bits.
 * That should leave you enough to shift down to whatever precision the hardware
 * supports. You can generate a float representation in Hz (where float is
 * supported) with scalbn(midisyn_mt2hz18(mt),-18).
 */
extern uint32_t midisyn_mt2hz18(uint32_t);

#define MIDISYN_HZ18_TO_HZ(f) ((f) >> 18)
#define MIDISYN_KEY_TO_MT(k) ((k)<<14)
#define MIDISYN_MT_TO_KEY(m) (((m)+(1<<13))>>14)

#define MIDISYN_FREQ_TO_HZ(f) ((f) >> 16) /* deprecated; going away RSN */

#endif /* _SYS_DEV_MIDISYNVAR_H_ */
