/*	$NetBSD: midiplay.c,v 1.22.12.4 2006/06/02 02:36:13 chap Exp $	*/

/*
 * Copyright (c) 1998, 2002 The NetBSD Foundation, Inc.
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

#ifndef lint
__RCSID("$NetBSD: midiplay.c,v 1.22.12.4 2006/06/02 02:36:13 chap Exp $");
#endif


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/midiio.h>

#define DEVMUSIC "/dev/music"

struct track {
	struct track *indirect; /* for fast swaps in heap code */
	u_char *start, *end;
	u_long delta;
	u_char status;
};

#define MIDI_META 0xff

#define META_SEQNO	0x00
#define META_TEXT	0x01
#define META_COPYRIGHT	0x02
#define META_TRACK	0x03
#define META_INSTRUMENT	0x04
#define META_LYRIC	0x05
#define META_MARKER	0x06
#define META_CUE	0x07
#define META_CHPREFIX	0x20
#define META_EOT	0x2f
#define META_SET_TEMPO	0x51
#define META_KEY	0x59
#define META_SMPTE	0x54
#define META_TIMESIGN	0x58

char *metanames[] = { 
	"", "Text", "Copyright", "Track", "Instrument", 
	"Lyric", "Marker", "Cue",
};

static int midi_lengths[] = { 2,2,2,2,1,1,2,0 };
/* Number of bytes in a MIDI command */
#define MIDI_LENGTH(d) (midi_lengths[((d) >> 4) & 7])

void usage(void);
void send_event(seq_event_t *);
void dometa(u_int, u_char *, u_int);
void midireset(void);
void send_sysex(u_char *, u_int);
u_long getvar(struct track *);
u_long getlen(struct track *);
void playfile(FILE *, char *);
void playdata(u_char *, u_int, char *);
int main(int argc, char **argv);

void Heapify(struct track *, int, int);
void BuildHeap(struct track *, int);
int ShrinkHeap(struct track *, int);

/*
 * This sample plays at an apparent tempo of 120 bpm when the BASETEMPO is 150
 * bpm, because the quavers are 5 divisions (4 on 1 off) rather than 4 total.
 */
#define P(c) 1,0x90,c,0x7f,4,0x80,c,0
#define PL(c) 1,0x90,c,0x7f,8,0x80,c,0
#define C 0x3c
#define D 0x3e
#define E 0x40
#define F 0x41

u_char sample[] = { 
	'M','T','h','d',  0,0,0,6,  0,1,  0,1,  0,8,
	'M','T','r','k',  0,0,0,4+13*8,
	P(C), P(C), P(C), P(E), P(D), P(D), P(D), 
	P(F), P(E), P(E), P(D), P(D), PL(C),
	0, 0xff, 0x2f, 0
};
#undef P
#undef PL
#undef C
#undef D
#undef E
#undef F

#define MARK_HEADER "MThd"
#define MARK_TRACK "MTrk"
#define MARK_LEN 4

#define	RMID_SIG "RIFF"
#define	RMID_MIDI_ID "RMID"
#define	RMID_DATA_ID "data"

#define SIZE_LEN 4
#define HEADER_LEN 6

#define GET8(p) ((p)[0])
#define GET16(p) (((p)[0] << 8) | (p)[1])
#define GET24(p) (((p)[0] << 16) | ((p)[1] << 8) | (p)[2])
#define GET32(p) (((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])
#define GET32_LE(p) (((p)[3] << 24) | ((p)[2] << 16) | ((p)[1] << 8) | (p)[0])

void
usage(void)
{
	printf("usage: %s [-d unit] [-f file] [-l] [-m] [-p pgm] [-q] "
	       "[-t %%tempo] [-v] [-x] [file ...]\n",
		getprogname());
	exit(1);
}

int showmeta = 0;
int verbose = 0;
#define BASETEMPO 400000		/* us/beat(=24 clks or qn) (150 bpm) */
u_int tempo_set = 0;
u_int tempo_abs = 0;
u_int ttempo = 100;
int unit = 0;
int play = 1;
int fd = -1;
int sameprogram = 0;
int insysex = 0;
int svsysex = 0; /* number of sysex bytes saved internally */

void
send_event(seq_event_t *ev)
{
	/*
	printf("%02x %02x %02x %02x %02x %02x %02x %02x\n",
	       ev->arr[0], ev->arr[1], ev->arr[2], ev->arr[3], 
	       ev->arr[4], ev->arr[5], ev->arr[6], ev->arr[7]);
	*/
	if (play)
		write(fd, ev, sizeof *ev);
}

u_long
getvar(struct track *tp)
{
	u_long r, c;

	r = 0;
	do {
		c = *tp->start++;
		r = (r << 7) | (c & 0x7f);
	} while ((c & 0x80) && tp->start < tp->end);
	return r;
}

u_long
getlen(struct track *tp)
{
	u_long len;
	len = getvar(tp);
	if (tp->start + len > tp->end)
		errx(1, "bogus item length exceeds remaining track size");
	return len;
}

void
dometa(u_int meta, u_char *p, u_int len)
{
	static char const * const keys[] = {
	        "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F",
		"C",
		"G", "D", "A", "E", "B", "F#", "C#",
		"G#", "D#", "A#" /* for minors */
	};
	seq_event_t ev;
	uint32_t usperbeat;
	
	switch (meta) {
	case META_TEXT:
	case META_COPYRIGHT:
	case META_TRACK:
	case META_INSTRUMENT:
	case META_LYRIC:
	case META_MARKER:
	case META_CUE:
		if (showmeta) {
			printf("%s: ", metanames[meta]);
			fwrite(p, len, 1, stdout);
			printf("\n");
		}
		break;
	case META_SET_TEMPO:
		usperbeat = GET24(p);
		ev = SEQ_MK_TIMING(TEMPO,
		    .bpm=(60000000. / usperbeat) * (ttempo / 100.) + 0.5);
		if (showmeta)
			printf("Tempo: %u us/'beat'(24 midiclks)"
			       " at %u%%; adjusted bpm = %u\n",
			       usperbeat, ttempo, ev.t_TEMPO.bpm);
		if (tempo_abs)
			warnx("tempo event ignored"
			      " in absolute-timed MIDI file");
		else {
			send_event(&ev);
			if (!tempo_set) {
				tempo_set = 1;
				send_event(&SEQ_MK_TIMING(START));
			}
		}
		break;
	case META_TIMESIGN:
		ev = SEQ_MK_TIMING(TIMESIG,
		    .numerator=p[0],      .lg2denom=p[1],
		    .clks_per_click=p[2], .dsq_per_24clks=p[3]);
		if (showmeta) {
			printf("Time signature: %d/%d."
			       " Click every %d midiclk%s"
			       " (24 midiclks = %d 32nd note%s)\n",
			       ev.t_TIMESIG.numerator,
			       1 << ev.t_TIMESIG.lg2denom,
			       ev.t_TIMESIG.clks_per_click,
			       1 == ev.t_TIMESIG.clks_per_click ? "" : "s",
			       ev.t_TIMESIG.dsq_per_24clks,
			       1 == ev.t_TIMESIG.dsq_per_24clks ? "" : "s");
		}
		/* send_event(&ev); not implemented in sequencer */
		break;
	case META_KEY:
		if (showmeta)
			printf("Key: %s %s\n",
			       keys[((char)p[0]) + p[1] ? 10 : 7],
			       p[1] ? "minor" : "major");
		break;
	default:
		break;
	}
}

void
midireset(void)
{
	/* General MIDI reset sequence */
	send_event(&SEQ_MK_SYSEX(unit,[0]=0x7e, 0x7f, 0x09, 0x01, 0xf7));
}

#define SYSEX_CHUNK 6
void
send_sysex(u_char *p, u_int l)
{
	seq_event_t event;
	static u_char bf[6];
	
	if ( 0 == l ) {
		warnx("zero-length system-exclusive event");
		return;
	}
	
	/*
	 * This block is needed only to handle the possibility that a sysex
	 * message is broken into multiple events in a MIDI file that do not
	 * have length six; the /dev/music sequencer assumes a sysex message is
	 * finished with the first SYSEX event carrying fewer than six bytes,
	 * even if the last is not MIDI_SYSEX_END. So, we need to be careful
	 * not to send a short sysex event until we have seen the end byte.
	 * Instead, save some straggling bytes in bf, and send when we have a
	 * full six (or an end byte). Note bf/saved/insysex should be per-
	 * device, if we supported output to more than one device at a time.
	 */
	if ( svsysex > 0 ) {
		if ( l > sizeof bf - svsysex ) {
			memcpy(bf + svsysex, p, sizeof bf - svsysex);
			l -= sizeof bf - svsysex;
			p += sizeof bf - svsysex;
			send_event(&SEQ_MK_SYSEX(unit,[0]=
			    bf[0],bf[1],bf[2],bf[3],bf[4],bf[5]));
			svsysex = 0;
		} else {
			memcpy(bf + svsysex, p, l);
			svsysex += l;
			p += l;
			if ( MIDI_SYSEX_END == bf[svsysex-1] ) {
				event = SEQ_MK_SYSEX(unit);
				memcpy(event.sysex.buffer, bf, svsysex);
				send_event(&event);
				svsysex = insysex = 0;
			} else
				insysex = 1;
			return;
		}
	}
	
	/*
	 * l > 0. May as well test now whether we will be left 'insysex'
	 * after processing this event.
	 */	
	insysex = ( MIDI_SYSEX_END != p[l-1] );
	
	/*
	 * If not for multi-event sysexes and chunk-size weirdness, this
	 * function could pretty much start here. :)
	 */
	while ( l >= SYSEX_CHUNK ) {
		send_event(&SEQ_MK_SYSEX(unit,[0]=
		    p[0],p[1],p[2],p[3],p[4],p[5]));
		p += SYSEX_CHUNK;
		l -= SYSEX_CHUNK;
	}
	if ( l > 0 ) {
		if ( insysex ) {
			memcpy(bf, p, l);
			svsysex = l;
		} else { /* a <6 byte chunk is ok if it's REALLY the end */
			event = SEQ_MK_SYSEX(unit);
			memcpy(event.sysex.buffer, p, l);
			send_event(&event);
		}
	}
}

void
playfile(FILE *f, char *name)
{
	u_char *buf, *nbuf;
	u_int tot, n, size, nread;

	/* 
	 * We need to read the whole file into memory for easy processing.
	 * Using mmap() would be nice, but some file systems do not support
	 * it, nor does reading from e.g. a pipe.  The latter also precludes
	 * finding out the file size without reading it.
	 */
	size = 1000;
	buf = malloc(size);
	if (buf == 0)
		errx(1, "malloc() failed");
	nread = size;
	tot = 0;
	for (;;) {
		n = fread(buf + tot, 1, nread, f);
		tot += n;
		if (n < nread)
			break;
		/* There must be more to read. */
		nread = size;
		nbuf = realloc(buf, size * 2);
		if (nbuf == NULL)
			errx(1, "realloc() failed");
		buf = nbuf;
		size *= 2;
	}
	playdata(buf, tot, name);
	free(buf);
}

void
playdata(u_char *buf, u_int tot, char *name)
{
	int format, ntrks, divfmt, ticks, t;
	u_int len, mlen, status, chan;
	u_char *p, *end, byte, meta, *msg;
	struct track *tracks;
	struct track *tp;

	end = buf + tot;
	if (verbose)
		printf("Playing %s (%d bytes) ... \n", name, tot);

	if (tot < MARK_LEN + 4) {
		warnx("Not a MIDI file, too short");
		return;
	}

	if (memcmp(buf, RMID_SIG, MARK_LEN) == 0) {
		u_char *eod;
		/* Detected a RMID file, let's just check if it's
		 * a MIDI file */
		if (GET32_LE(buf + MARK_LEN) != tot - 8) {
			warnx("Not a RMID file, bad header");
			return;
		}

		buf += MARK_LEN + 4;
		if (memcmp(buf, RMID_MIDI_ID, MARK_LEN) != 0) {
			warnx("Not a RMID file, bad ID");
			return;
		}

		/* Now look for the 'data' chunk, which contains
		 * MIDI data */
		buf += MARK_LEN;

		/* Test against end-8 since we must have at least 8 bytes
		 * left to read */
		while(buf < end-8 && memcmp(buf, RMID_DATA_ID, MARK_LEN))
			buf += GET32_LE(buf+4) + 8; /* MARK_LEN + 4 */

		if (buf >= end-8) {
			warnx("Not a valid RMID file, no data chunk");
			return;
		}

		buf += MARK_LEN; /* "data" */
		eod = buf + 4 + GET32_LE(buf);
		if (eod >= end) {
			warnx("Not a valid RMID file, bad data chunk size");
			return;
		}

		end = eod;
		buf += 4;
	}

	if (memcmp(buf, MARK_HEADER, MARK_LEN) != 0) {
		warnx("Not a MIDI file, missing header");
		return;
	}

	if (GET32(buf + MARK_LEN) != HEADER_LEN) {
		warnx("Not a MIDI file, bad header");
		return;
	}
	format = GET16(buf + MARK_LEN + SIZE_LEN);
	ntrks = GET16(buf + MARK_LEN + SIZE_LEN + 2);
	divfmt = GET8(buf + MARK_LEN + SIZE_LEN + 4);
	ticks = GET8(buf + MARK_LEN + SIZE_LEN + 5);
	p = buf + MARK_LEN + SIZE_LEN + HEADER_LEN;
	/*
	 * Set the timebase (or timebase and tempo, for absolute-timed files).
	 * PORTABILITY: some sequencers actually check the timebase against
	 * available timing sources and may adjust it accordingly (storing a
	 * new value in the ioctl arg) which would require us to compensate
	 * somehow. That possibility is ignored for now, as NetBSD's sequencer
	 * currently synthesizes all timebases, for better or worse, from the
	 * system clock.
	 *
	 * For a non-absolute file, if timebase is set to the file's divisions
	 * value, and tempo set in the obvious way, then the timing deltas in
	 * the MTrks require no scaling. A downside to this approach is that
	 * the sequencer API wants tempo in (integer) beats per minute, which
	 * limits how finely tempo can be specified. That might be got around
	 * in some cases by frobbing tempo and timebase more obscurely, but this
	 * player is meant to be simple and clear.
	 */
	if ((divfmt & 0x80) == 0) {
		ticks |= divfmt << 8;
		if (ioctl(fd, SEQUENCER_TMR_TIMEBASE, &(int){ticks}) < 0)
			err(1, "SEQUENCER_TMR_TIMEBASE");
	} else {
		tempo_abs = tempo_set = 1;
		divfmt = -(int8_t)divfmt;
		/*
		 * divfmt is frames per second; multiplying by 60 to set tempo
		 * in frames per minute could exceed sequencer's (arbitrary)
		 * tempo limits, so factor 60 as 12*5, set tempo in frames per
		 * 12 seconds, and account for the 5 in timebase.
		 */
		send_event(&SEQ_MK_TIMING(TEMPO,
		    .bpm=(12*divfmt) * (ttempo/100.) + 0.5));
		if (ioctl(fd, SEQUENCER_TMR_TIMEBASE, &(int){5*ticks}) < 0)
			err(1, "SEQUENCER_TMR_TIMEBASE");
	}
	if (verbose > 1)
		printf(tempo_abs ?
		       "format=%d ntrks=%d abs fps=%u subdivs=%u\n" :
		       "format=%d ntrks=%d divisions=%u\n",
		       format, ntrks, tempo_abs ? divfmt : ticks, ticks);
	if (format != 0 && format != 1) {
		warnx("Cannot play MIDI file of type %d", format);
		return;
	}
	if (ntrks == 0)
		return;
	tracks = malloc(ntrks * sizeof(struct track));
	if (tracks == NULL)
		errx(1, "malloc() tracks failed");
	for (t = 0; t < ntrks; ) {
		if (p >= end - MARK_LEN - SIZE_LEN) {
			warnx("Cannot find track %d", t);
			goto ret;
		}
		len = GET32(p + MARK_LEN);
		if (len > 1000000) { /* a safe guard */
			warnx("Crazy track length");
			goto ret;
		}
		if (memcmp(p, MARK_TRACK, MARK_LEN) == 0) {
			tracks[t].start = p + MARK_LEN + SIZE_LEN;
			tracks[t].end = tracks[t].start + len;
			tracks[t].delta = getvar(&tracks[t]);
			tracks[t].indirect = &tracks[t]; /* -> self for now */
			t++;
		}
		p += MARK_LEN + SIZE_LEN + len;
	}

	/*
	 * Force every channel to the same patch if requested by the user.
	 */
	if (sameprogram) {
		for(t = 0; t < 16; t++) {
			send_event(&SEQ_MK_CHN(PGM_CHANGE, .device=unit,
			    .channel=t, .program=sameprogram-1));
		}
	}
	/* 
	 * Play MIDI events by selecting the track with the lowest
	 * delta.  Execute the event, update the delta and repeat.
	 *
	 * The ticks variable is the number of ticks that make up a beat
	 * (beat: 24 MIDI clocks always, a quarter note by usual convention)
	 * and is used as a reference value for the delays between
	 * the MIDI events.
	 */
	BuildHeap(tracks, ntrks); /* tracks[0].indirect is always next */
	for (;;) {
		tp = tracks[0].indirect;
		if ((verbose > 2 && tp->delta > 0)  ||  verbose > 3) {
			printf("DELAY %4ld TRACK %2d%s",
			       tp->delta, tp - tracks, verbose>3?" ":"\n");
			fflush(stdout);
		}
		if (tp->delta > 0) {
			if (!tempo_set) {
				if (verbose || showmeta)
					printf("No initial tempo;"
					       " defaulting:\n");
				dometa(META_SET_TEMPO, (u_char[]){
				    BASETEMPO >> 16,
				    (BASETEMPO >> 8) & 0xff,
				    BASETEMPO & 0xff},
				    3);
			}
			send_event(&SEQ_MK_TIMING(WAIT_REL,
			    .divisions=tp->delta));
		}
		byte = *tp->start++;
		if (byte == MIDI_META) {
			meta = *tp->start++;
			mlen = getlen(tp);
			if (verbose > 3)
				printf("META %02x (%d)\n", meta, mlen);
			dometa(meta, tp->start, mlen);
			tp->start += mlen;
		} else {
			if (MIDI_IS_STATUS(byte))
				tp->status = byte;
			else
				tp->start--;
			mlen = MIDI_LENGTH(tp->status);
			msg = tp->start;
			if (verbose > 3) {
			    if (mlen == 1)
				printf("MIDI %02x (%d) %02x\n",
				       tp->status, mlen, msg[0]);
			    else   
				printf("MIDI %02x (%d) %02x %02x\n",
				       tp->status, mlen, msg[0], msg[1]);
			}
			if (insysex && tp->status != MIDI_SYSEX_END) {
				warnx("incomplete system exclusive message"
				      " aborted");
				svsysex = insysex = 0;
			}
			status = MIDI_GET_STATUS(tp->status);
			chan = MIDI_GET_CHAN(tp->status);
			switch (status) {
			case MIDI_NOTEOFF:
				send_event(&SEQ_MK_CHN(NOTEOFF, .device=unit,
				.channel=chan, .key=msg[0], .velocity=msg[1]));
				break;
			case MIDI_NOTEON:
				send_event(&SEQ_MK_CHN(NOTEON, .device=unit,
				.channel=chan, .key=msg[0], .velocity=msg[1]));
				break;
			case MIDI_KEY_PRESSURE:
				send_event(&SEQ_MK_CHN(KEY_PRESSURE,
				.device=unit, .channel=chan,
				.key=msg[0], .pressure=msg[1]));
				break;
			case MIDI_CTL_CHANGE:
				send_event(&SEQ_MK_CHN(CTL_CHANGE,
				.device=unit, .channel=chan,
				.controller=msg[0], .value=msg[1]));
				break;
			case MIDI_PGM_CHANGE:
				if (!sameprogram)
					send_event(&SEQ_MK_CHN(PGM_CHANGE,
					.device=unit, .channel=chan,
					.program=msg[0]));
				break;
			case MIDI_CHN_PRESSURE:
				send_event(&SEQ_MK_CHN(CHN_PRESSURE,
				.device=unit, .channel=chan, .pressure=msg[0]));
				break;
			case MIDI_PITCH_BEND:
				send_event(&SEQ_MK_CHN(PITCH_BEND,
				.device=unit, .channel=chan,
				.value=(msg[0] & 0x7f) | ((msg[1] & 0x7f)<<7)));
				break;
			case MIDI_SYSTEM_PREFIX:
				mlen = getlen(tp);
				if (tp->status == MIDI_SYSEX_START) {
					send_sysex(tp->start, mlen);
					break;
				} else if (tp->status == MIDI_SYSEX_END) {
				/* SMF uses SYSEX_END as CONTINUATION/ESCAPE */
					if (insysex) { /* CONTINUATION */
						send_sysex(tp->start, mlen);
					} else { /* ESCAPE */
						for ( ; mlen > 0 ; -- mlen ) {
							send_event(
							    &SEQ_MK_EVENT(putc,
							    SEQOLD_MIDIPUTC,
							    .device=unit,
							    .byte=*(tp->start++)
							    ));
						}
					}
					break;
				}
				/* Sorry, can't do this yet; FALLTHROUGH */
			default:
				if (verbose)
					printf("MIDI event 0x%02x ignored\n",
					       tp->status);
			}
			tp->start += mlen;
		}
		if (tp->start >= tp->end) {
			ntrks = ShrinkHeap(tracks, ntrks); /* track gone */
			if (0 == ntrks)
				break;
		} else
			tp->delta = getvar(tp);
		Heapify(tracks, ntrks, 0);
	}
	if (ioctl(fd, SEQUENCER_SYNC, 0) < 0)
		err(1, "SEQUENCER_SYNC");

 ret:
	free(tracks);
}

int
main(int argc, char **argv)
{
	int ch;
	int listdevs = 0;
	int example = 0;
	int nmidi;
	const char *file = DEVMUSIC;
	const char *sunit;
	struct synth_info info;
	FILE *f;

	if ((sunit = getenv("MIDIUNIT")))
		unit = atoi(sunit);

	while ((ch = getopt(argc, argv, "?d:f:lmp:qt:vx")) != -1) {
		switch(ch) {
		case 'd':
			unit = atoi(optarg);
			break;
		case 'f':
			file = optarg;
			break;
		case 'l':
			listdevs++;
			break;
		case 'm':
			showmeta++;
			break;
		case 'p':
			sameprogram = atoi(optarg);
			break;
		case 'q':
			play = 0;
			break;
		case 't':
			ttempo = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		case 'x':
			example++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
    
	if (!play)
		goto output;

	fd = open(file, O_WRONLY);
	if (fd < 0)
		err(1, "%s", file);
	if (ioctl(fd, SEQUENCER_NRMIDIS, &nmidi) < 0)
		err(1, "ioctl(SEQUENCER_NRMIDIS) failed, ");
	if (nmidi == 0)
		errx(1, "Sorry, no MIDI devices available");
	if (listdevs) {
		for (info.device = 0; info.device < nmidi; info.device++) {
			if (ioctl(fd, SEQUENCER_INFO, &info) < 0)
				err(1, "ioctl(SEQUENCER_INFO) failed, ");
			printf("%d: %s\n", info.device, info.name);
		}
		exit(0);
	}

 output:
	if (example)
		while (example--)
			playdata(sample, sizeof sample, "<Gubben Noa>");
	else if (argc == 0)
		playfile(stdin, "<stdin>");
	else
		while (argc--) {
			f = fopen(*argv, "r");
			if (f == NULL)
				err(1, "%s", *argv);
			else {
				playfile(f, *argv);
				fclose(f);
			}
			argv++;
		}

	exit(0);
}

/*
 * relative-time priority queue (min-heap). Properties:
 * 1. The delta time at a node is relative to the node's parent's time.
 * 2. When an event is dequeued from a track, the delta time of the new head
 *    event is relative to the time of the event just dequeued.
 * Therefore:
 * 3. After dequeueing the head event from the track at heap root, the next
 *    event's time is directly comparable to the root's children.
 * These properties allow the heap to be maintained with delta times throughout.
 * Insert is also implementable, but not needed: all the tracks are present
 * at first; they just go away as they end.
 */

#define PARENT(i) ((i-1)>>1)
#define LEFT(i)   ((i<<1)+1)
#define RIGHT(i)  ((i+1)<<1)
#define DTIME(i)  (t[i].indirect->delta)
#define SWAP(i,j) do { \
    struct track *_t = t[i].indirect; \
    t[i].indirect = t[j].indirect; \
    t[j].indirect = _t; \
    } while ( /*CONSTCOND*/ 0 )

void
Heapify(struct track *t, int ntrks, int node)
{
	int lc, rc, mn;
	
	lc = LEFT(node);
	rc = RIGHT(node);
	
	if (rc >= ntrks) {			/* no right child */
		if (lc >= ntrks)		/* node is a leaf */
			return;
		if (DTIME(node) > DTIME(lc))
			SWAP(node,lc);
		DTIME(lc) -= DTIME(node);
		return;				/* no rc ==> lc is a leaf */
	}

	mn = lc;
	if (DTIME(lc) > DTIME(rc))
		mn = rc;
	if (DTIME(node) <= DTIME(mn)) {
		DTIME(rc) -= DTIME(node);
		DTIME(lc) -= DTIME(node);
		return;
	}
	
	SWAP(node,mn);
	DTIME(rc) -= DTIME(node);
	DTIME(lc) -= DTIME(node);
	Heapify(t, ntrks, mn); /* gcc groks tail recursion */
}

void
BuildHeap(struct track *t, int ntrks)
{
	int node;
	
	for ( node = PARENT(ntrks-1); node --> 0; )
		Heapify(t, ntrks, node);
}

/*
 * Make the heap 1 item smaller by discarding the track at the root.
 * Move the rightmost leaf to the root and decrement ntrks. It remains to
 * run Heapify, which the caller is expected to do. Returns the new ntrks.
 */
int
ShrinkHeap(struct track *t, int ntrks)
{
	int ancest;
	
	-- ntrks;
	for ( ancest = PARENT(ntrks); ancest > 0; ancest = PARENT(ancest) )
		DTIME(ntrks) += DTIME(ancest);
	t[0].indirect = t[ntrks].indirect;
	return ntrks;
}
