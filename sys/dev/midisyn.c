/*	$NetBSD: midisyn.c,v 1.17.2.17 2006/06/09 17:05:28 chap Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: midisyn.c,v 1.17.2.17 2006/06/09 17:05:28 chap Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/audioio.h>
#include <sys/midiio.h>
#include <sys/device.h>

#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/midivar.h>
#include <dev/midisynvar.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (midisyndebug) printf x
#define DPRINTFN(n,x)	if (midisyndebug >= (n)) printf x
int	midisyndebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

int	midisyn_findvoice(midisyn *, int, int);
void	midisyn_freevoice(midisyn *, int);
uint_fast16_t	midisyn_allocvoice(midisyn *, uint_fast8_t, uint_fast8_t);
static void	midisyn_attackv_vel(midisyn *, uint_fast16_t, midipitch_t,
                                    int16_t, uint_fast8_t);
static int16_t midisyn_adj_level(midisyn *, uint_fast8_t);
static midipitch_t midisyn_adj_pitch(midisyn *, uint_fast8_t);

static midictl_notify midisyn_notify;

int	midisyn_open(void *, int,
		     void (*iintr)(void *, int),
		     void (*ointr)(void *), void *arg);
void	midisyn_close(void *);
int	midisyn_sysrt(void *, int);
void	midisyn_getinfo(void *, struct midi_info *);
int	midisyn_ioctl(void *, u_long, caddr_t, int, struct lwp *);

const struct midi_hw_if midisyn_hw_if = {
	midisyn_open,
	midisyn_close,
	midisyn_sysrt,
	midisyn_getinfo,
	midisyn_ioctl,
};

int	midisyn_channelmsg(void *, int, int, u_char *, int);
int	midisyn_commonmsg(void *, int, u_char *, int);
int	midisyn_sysex(void *, u_char *, int);

struct midi_hw_if_ext midisyn_hw_if_ext = {
	.channel = midisyn_channelmsg,
	.common  = midisyn_commonmsg,
	.sysex   = midisyn_sysex,
};

int
midisyn_open(void *addr, int flags, void (*iintr)(void *, int),
	     void (*ointr)(void *), void *arg)
{
	midisyn *ms = addr;
	int rslt;
	uint_fast8_t chan;

	DPRINTF(("midisyn_open: ms=%p ms->mets=%p\n", ms, ms->mets));
	
	midictl_open(&ms->ctl);
	
	rslt = 0;
	if (ms->mets->open)
		rslt = (ms->mets->open(ms, flags));
	
	/*
	 * Make the right initial things happen by faking receipt of RESET on
	 * all channels. The hw driver's ctlnotice() will be called in turn.
	 */
	for ( chan = 0 ; chan < MIDI_MAX_CHANS ; ++ chan )
		midisyn_notify(ms, MIDICTL_RESET, chan, 0);
	
	return rslt;
}

void
midisyn_close(void *addr)
{
	midisyn *ms = addr;
	struct midisyn_methods *fs;
	int v;

	DPRINTF(("midisyn_close: ms=%p ms->mets=%p\n", ms, ms->mets));
	fs = ms->mets;
	/* TODO
	 * This loop is much like what midisyn_notify should do for NOTES_OFF
	 * or SOUND_OFF. It should be moved there, and here we should call
	 * notify faking one of those events.
	 */
	for (v = 0; v < ms->nvoice; v++)
		if (ms->voices[v].inuse) {
			fs->releasev(ms, v, 127);
			midisyn_freevoice(ms, v);
		}
	if (fs->close)
		fs->close(ms);

	midictl_close(&ms->ctl);
}

void
midisyn_getinfo(void *addr, struct midi_info *mi)
{
	midisyn *ms = addr;

	mi->name = ms->name;
	/*
	 * I was going to add a property here to suppress midi(4)'s warning
	 * about an output device that uses no transmit interrupt, on the
	 * assumption that as an onboard synth we handle "output" internally
	 * with nothing like the 320 us per byte busy wait of a dumb UART.
	 * Then I noticed that opl (at least as currently implemented) seems
	 * to need 40 us busy wait to set each register on an OPL2, and sets
	 * about 21 registers for every note-on. (Half of that is patch loading
	 * and could probably be reduced by different management of voices and
	 * patches.) For now I won't bother suppressing that warning....
	 */
	mi->props = 0;
	
	midi_register_hw_if_ext(&midisyn_hw_if_ext);
}

int
midisyn_ioctl(void *maddr, u_long cmd, caddr_t addr, int flag, struct lwp *l)
{
	midisyn *ms = maddr;

	if (ms->mets->ioctl)
		return (ms->mets->ioctl(ms, cmd, addr, flag, l));
	else
		return (EINVAL);
}

int
midisyn_findvoice(midisyn *ms, int chan, int note)
{
	u_int cn;
	int v;

	cn = MS_CHANNOTE(chan, note);
	for (v = 0; v < ms->nvoice; v++)
		if (ms->voices[v].chan_note == cn && ms->voices[v].inuse)
			return (v);
	return (-1);
}

void
midisyn_attach(struct midi_softc *sc, midisyn *ms)
{
	if (ms->mets->allocv == 0) {
		ms->voices = malloc(ms->nvoice * sizeof (struct voice),
				    M_DEVBUF, M_WAITOK|M_ZERO);
		ms->seqno = 1;
		ms->mets->allocv = midisyn_allocvoice;
	}
	
	if (ms->mets->attackv_vel == 0 && ms->mets->attackv != 0)
		ms->mets->attackv_vel = midisyn_attackv_vel;
	
	ms->ctl = (midictl) {
		.base_channel = 16,
		.cookie = ms,
		.notify = midisyn_notify
	};
	
	sc->hw_if = &midisyn_hw_if;
	sc->hw_hdl = ms;
	DPRINTF(("midisyn_attach: ms=%p\n", sc->hw_hdl));
}

void
midisyn_freevoice(midisyn *ms, int voice)
{
	if (ms->mets->allocv != midisyn_allocvoice)
		return;
	ms->voices[voice].inuse = 0;
}

uint_fast16_t
midisyn_allocvoice(midisyn *ms, uint_fast8_t chan, uint_fast8_t note)
{
	int bestv, v;
	u_int bestseq, s;

	/* Find a free voice, or if no free voice is found the oldest. */
	bestv = 0;
	bestseq = ms->voices[0].seqno + (ms->voices[0].inuse ? 0x40000000 : 0);
	for (v = 1; v < ms->nvoice; v++) {
		s = ms->voices[v].seqno;
		if (ms->voices[v].inuse)
			s += 0x40000000;
		if (s < bestseq) {
			bestseq = s;
			bestv = v;
		}
	}
	DPRINTFN(10,("midisyn_allocvoice: v=%d seq=%d cn=%x inuse=%d\n",
		     bestv, ms->voices[bestv].seqno,
		     ms->voices[bestv].chan_note,
		     ms->voices[bestv].inuse));
#ifdef AUDIO_DEBUG
	if (ms->voices[bestv].inuse)
		DPRINTFN(1,("midisyn_allocvoice: steal %x\n",
			    ms->voices[bestv].chan_note));
#endif
	ms->voices[bestv].chan_note = MS_CHANNOTE(chan, note);
	ms->voices[bestv].seqno = ms->seqno++;
	ms->voices[bestv].inuse = 1;
	return (bestv);
}

/* dummy attackv_vel that just adds vel into level for simple drivers */
static void
midisyn_attackv_vel(midisyn *ms, uint_fast16_t voice, midipitch_t mp,
                    int16_t level_cB, uint_fast8_t vel)
{
	ms->mets->attackv(ms, voice, mp,
	                  level_cB + midisyn_vol2cB((uint_fast16_t)vel << 7));
}

int
midisyn_sysrt(void *addr, int b)
{
	return 0;
}

int midisyn_channelmsg(void *addr, int status, int chan, u_char *buf, int len)
{
	midisyn *ms = addr;
	int voice = 0;		/* initialize to keep gcc quiet */
	struct midisyn_methods *fs;

	DPRINTF(("midisyn_channelmsg: ms=%p status=%#02x chan=%d\n",
	       ms, status, chan));
	fs = ms->mets;

	switch (status) {
	case MIDI_NOTEOFF:
		/*
		 * for a device that leaves voice allocation to us--and that's
		 * all of 'em at the moment--the voice and release velocity
		 * should be the only necessary arguments to noteoff. what use
		 * are they making of note? checking... None. Cool.
		 * IF there is ever a device added that does its own allocation,
		 * extend the interface; this findvoice won't be what to do...
		 */
		voice = midisyn_findvoice(ms, chan, buf[1]);
		if (voice >= 0) {
			fs->releasev(ms, voice, buf[2]);
			midisyn_freevoice(ms, voice);
		}
		break;
	case MIDI_NOTEON:
		/*
		 * opl combines vel with a 'mainvol' that has no setter
		 * anywhere. pcppi punts volume entirely. cms uses vel alone.
		 * what's called for here, given current drivers, is an i/f
		 * where midisyn computes a volume from vel*volume*expression*
		 * mastervolume and passes that result as a single arg. It can
		 * evolve later to support drivers that expose some of those
		 * bits separately (e.g. a driver could expose a mixer register
		 * on its sound card and use that for mastervolume).
		 */
		voice = fs->allocv(ms, chan, buf[1]);
		fs->attackv_vel(ms, voice,
		                MIDIPITCH_FROM_KEY(buf[1])
				+ midisyn_adj_pitch(ms, chan),
				midisyn_adj_level(ms,chan), buf[2]);
		break;
	case MIDI_KEY_PRESSURE:
		/*
		 * unimplemented by the existing drivers. if we are doing
		 * voice allocation, find the voice that corresponds to this
		 * chan/note and define a method that passes the voice and
		 * pressure to the driver ... not the note, /it/ doesn't matter.
		 * For a driver that does its own allocation, a different
		 * method may be needed passing pressure, chan, note so it can
		 * find the right voice on its own. Be sure that whatever is
		 * done here is undone when midisyn_notify sees MIDICTL_RESET.
		 */
		break;
	case MIDI_CTL_CHANGE:
		midictl_change(&ms->ctl, chan, buf+1);
		break;
	case MIDI_PGM_CHANGE:
		if (fs->pgmchg)
			fs->pgmchg(ms, chan, buf[1]);
		break;
	case MIDI_CHN_PRESSURE:
		/*
		 * unimplemented by the existing drivers. if driver exposes no
		 * distinct method, can use KEY_PRESSURE method for each voice
		 * on channel. Be sure that whatever is
		 * done here is undone when midisyn_notify sees MIDICTL_RESET.
		 */
		break;
	case MIDI_PITCH_BEND:
		/*
		 * unimplemented by existing drivers. it is good that most
		 * existing drivers take a /frequency/ rather than a /note no/
		 * for control; midisyn should use this event to update some
		 * internal state and use it (along with tuning) to derive the
		 * frequency passed to the driver for any note on. A driver that
		 * can change freq of a sounding voice should expose a method
		 * for that, i.e. m(voice,newfreq) and we should call that too,
		 * for each voice on the affected channel, when a pitch bend
		 * change is received. Note RPN 0 must be used in scaling the
		 * pitch bend. Be sure that whatever is
		 * done here is undone when midisyn_notify sees MIDICTL_RESET.
		 */
		break;
	}
	return 0;
}

int midisyn_commonmsg(void *addr, int status, u_char *buf, int len)
{
	return 0;
}

int midisyn_sysex(void *addr, u_char *buf, int len)
{
	/*
	 * unimplemented by existing drivers. it is surely more sensible
	 * to do some parsing of well-defined sysex messages here, either
	 * handling them internally or calling specific methods on the
	 * driver after parsing out the details, than to ask every driver
	 * to deal with sysex messages poked at it a byte at a time.
	 */
	return 0;
}

static void
midisyn_notify(void *cookie, midictl_evt evt,
               uint_fast8_t chan, uint_fast16_t key)
{
	struct midisyn *ms;
	int drvhandled;
	
	ms = (struct midisyn *)cookie;
	drvhandled = 0;
	if ( ms->mets->ctlnotice )
		drvhandled = ms->mets->ctlnotice(ms, evt, chan, key);

	switch ( evt ) {
	case MIDICTL_RESET:
		/* re-read all ctls we use, revert pitchbend state */
		break;
	case MIDICTL_NOTES_OFF:
		if ( drvhandled )
			break;
		/* releasev all voices sounding on chan; use normal vel 64 */
		break;
	case MIDICTL_SOUND_OFF:
		if ( drvhandled )
			break;
		/* releasev all voices sounding on chan; use max vel 127 */
		/* it is really better for driver to handle this, instantly */
		break;
	default:
		break;
	}
}

static int16_t
midisyn_adj_level(midisyn *ms, uint_fast8_t chan)
{
	return 0; /* XXX for now. Use Volume and Expression ctlrs. */
}

static midipitch_t
midisyn_adj_pitch(midisyn *ms, uint_fast8_t chan)
{
	return 0; /* XXX for now. Use Pitchbend, PBrange, fine/coarse tuning. */
}

int16_t
midisyn_vol2cB(uint_fast16_t vol)
{
	int16_t cB = 0;
	int32_t v;
	
	if ( 0 == vol )
		return INT16_MIN;
	/*
	 * Adjust vol to fall in the range 8192..16383. Each doubling is
	 * worth 12 dB.
	 */
	while ( vol < 8192 ) {
		vol <<= 1;
		cB -= 120;
	}
	v = vol; /* ensure evaluation in signed 32 bit below */
	/*
	 * The GM vol-to-dB formula is dB = 40 log ( v / 127 ) for 7-bit v.
	 * The vol and expression controllers are in 14-bit space so the
	 * equivalent is 40 log ( v / 16256 ) - that is, MSB 127 LSB 0 because
	 * the LSB is commonly unused. MSB 127 LSB 127 would then be a tiny
	 * bit over.
	 * 1 dB resolution is a little coarser than we'd like, so let's shoot
	 * for centibels, i.e. 400 log ( v / 16256 ), and shift everything left
	 * as far as will fit in 32 bits, which turns out to be a shift of 22.
	 * This minimax polynomial approximation is good to about a centibel
	 * on the range 8192..16256, a shade worse (1.4 or so) above that.
	 * 26385/10166 is the 6th convergent of the coefficient for v^2.
	 */
	cB += ( v * ( 124828 - ( v * 26385 ) / 10166 ) - 1347349038 ) >> 22;
	return cB;
}

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
 * supports.
 *
 * Its prototype is exposed in <sys/midiio.h>.
 */
midihz18_t
midisyn_mp2hz18(midipitch_t mp)
{
	int64_t t64a, t64b;
	uint_fast8_t shift;
	
	/*
	 * Scale from the logarithmic MIDI-Tuning units to Hz<<18. Uses the
	 * continued-fraction form of a 2/2 rational function derived to
	 * cover the highest octave (mt 1900544..2097151 or 74.00.00..7f.7f.7f
	 * in RP-012-speak, the dotted bits are 7 wide) to produce Hz shifted
	 * left just as far as the maximum Hz will fit in a uint32, which
	 * turns out to be 18. Just shift off the result for lower octaves.
	 * Fit is within 1/4 MIDI tuning unit throughout (disclaimer: the
	 * comparison relied on the double-precision log in libm).
	 */

	if ( 0 == mp )
		return 2143236;
	
	for ( shift = 0; mp < 1900544; ++ shift )
		mp += MIDIPITCH_OCTAVE;

	if ( 1998848 == mp )
		return UINT32_C(2463438621) >> shift;
	
	t64a  = 0x5a1a0ee4; /* INT64_C(967879298788) gcc333: spurious warning */
	t64a |= (int64_t)0xe1 << 32;
	t64a /= mp - 1998848; /* here's why 1998848 is special-cased above ;) */
	t64a += mp - 3704981;
	t64b  = 0x6763759d; /* INT64_C(8405905567872413) and here too */
	t64b |= (int64_t)0x1ddd20 << 32;
	t64b /= t64a;
	t64b += UINT32_C(2463438619);
	return (uint32_t)t64b >> shift;
}
