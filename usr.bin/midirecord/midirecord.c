/*	$NetBSD: midirecord.c,v 1.10 2015/09/23 05:31:01 mrg Exp $	*/

/*
 * Copyright (c) 2014 Matthew R. Green
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

/*
 * midirecord(1), similar to audiorecord(1).
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: midirecord.c,v 1.10 2015/09/23 05:31:01 mrg Exp $");
#endif

#include <sys/param.h>
#include <sys/midiio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <assert.h>
#include <stdbool.h>

#include "libaudio.h"

static const char *midi_device;
static unsigned	*filt_devnos = NULL;
static unsigned	*filt_chans = NULL;
static unsigned num_filt_devnos, num_filt_chans;
static char	*raw_output;
static int	midifd;
static int	aflag, qflag, oflag;
static bool debug = false;
int	verbose;
static int	outfd, rawfd = -1;
static ssize_t	data_size;
static struct timeval record_time;
static struct timeval start_time;
static int	tempo = 120;
static unsigned	round_beats = 1;
static unsigned	notes_per_beat = 24;
static bool ignore_timer_fail = false;
static bool stdout_mode = false;

static void debug_log(const char *, size_t, const char *, ...)
    __printflike(3, 4);
static size_t midi_event_local_to_output(seq_event_t, u_char *, size_t);
static size_t midi_event_timer_wait_abs_to_output(seq_event_t, u_char *,
						  size_t);
static size_t midi_event_timer_to_output(seq_event_t, u_char *, size_t);
static size_t midi_event_chn_common_to_output(seq_event_t, u_char *, size_t);
static size_t midi_event_chn_voice_to_output(seq_event_t, u_char *, size_t);
static size_t midi_event_sysex_to_output(seq_event_t, u_char *, size_t);
static size_t midi_event_fullsize_to_output(seq_event_t, u_char *, size_t);
static size_t midi_event_to_output(seq_event_t, u_char *, size_t);
static int timeleft(struct timeval *, struct timeval *);
static bool filter_array(unsigned, unsigned *, size_t);
static bool filter_dev(unsigned);
static bool filter_chan(unsigned);
static bool filter_devchan(unsigned, unsigned);
static void parse_ints(const char *, unsigned **, unsigned *, const char *);
static void cleanup(int) __dead;
static void rewrite_header(void);
static void write_midi_header(void);
static void write_midi_trailer(void);
static void usage(void) __dead;

#define PATH_DEV_MUSIC "/dev/music"

int
main(int argc, char *argv[])
{
	u_char	*buffer;
	size_t	bufsize = 0;
	int	ch, no_time_limit = 1;

	while ((ch = getopt(argc, argv, "aB:c:Dd:f:hn:oqr:t:T:V")) != -1) {
		switch (ch) {
		case 'a':
			aflag++;
			break;
		case 'B':
			bufsize = strsuftoll("read buffer size", optarg,
					     1, UINT_MAX);
			break;
		case 'c':
			parse_ints(optarg, &filt_chans, &num_filt_chans,
			    "channels");
			break;
		case 'D':
			debug = true;
			break;
		case 'd':
			parse_ints(optarg, &filt_devnos, &num_filt_devnos,
			    "devices");
			break;
		case 'f':
			midi_device = optarg;
			ignore_timer_fail = true;
			break;
		case 'n':
			decode_uint(optarg, &notes_per_beat);
			break;
		case 'o':
			oflag++;	/* time stamp starts at proc start */
			break;
		case 'q':
			qflag++;
			break;
		case 'r':
			raw_output = optarg;
			break;
		case 'R':
			decode_uint(optarg, &round_beats);
			if (round_beats == 0)
				errx(1, "-R <round_beats> must be a positive integer");
			break;
		case 't':
			no_time_limit = 0;
			decode_time(optarg, &record_time);
			break;
		case 'T':
			decode_int(optarg, &tempo);
			break;
		case 'V':
			verbose++;
			break;
		/* case 'h': */
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	/*
	 * work out the buffer size to use, and allocate it.  don't allow it
	 * to be too small.
	 */
	if (bufsize < 32)
		bufsize = 32 * 1024;
	buffer = malloc(bufsize);
	if (buffer == NULL)
		err(1, "couldn't malloc buffer of %d size", (int)bufsize);

	/*
	 * open the music device
	 */
	if (midi_device == NULL && (midi_device = getenv("MIDIDEVICE")) == NULL)
		midi_device = PATH_DEV_MUSIC;
	midifd = open(midi_device, O_RDONLY);
	if (midifd < 0)
		err(1, "failed to open %s", midi_device);

	/* open the output file */
	if (argv[0][0] != '-' || argv[0][1] != '\0') {
		int mode = O_CREAT|(aflag ? O_APPEND : O_TRUNC)|O_WRONLY;

		outfd = open(*argv, mode, 0666);
		if (outfd < 0)
			err(1, "could not open %s", *argv);
	} else {
		stdout_mode = true;
		outfd = STDOUT_FILENO;
	}

	/* open the raw output file */
	if (raw_output) {
		int mode = O_CREAT|(aflag ? O_APPEND : O_TRUNC)|O_WRONLY;

		rawfd = open(raw_output, mode, 0666);
		if (rawfd < 0)
			err(1, "could not open %s", raw_output);
	}

	/* start the midi timer */
	if (ioctl(midifd, SEQUENCER_TMR_START, NULL) < 0) {
		if (ignore_timer_fail)
			warn("failed to start midi timer");
		else
			err(1, "failed to start midi timer");
	}

	/* set the timebase */
	if (ioctl(midifd, SEQUENCER_TMR_TIMEBASE, &notes_per_beat) < 0) {
		if (ignore_timer_fail)
			warn("SEQUENCER_TMR_TIMEBASE: notes_per_beat %d",
			     notes_per_beat);
		else
			err(1, "SEQUENCER_TMR_TIMEBASE: notes_per_beat %d",
			    notes_per_beat);
	}

	/* set the tempo */
	if (ioctl(midifd, SEQUENCER_TMR_TEMPO, &tempo) < 0) {
		if (ignore_timer_fail)
			warn("SEQUENCER_TMR_TIMEBASE: tempo %d", tempo);
		else
			err(1, "SEQUENCER_TMR_TIMEBASE: tempo %d", tempo);
	}

	signal(SIGINT, cleanup);

	data_size = 0;

	if (verbose)
		fprintf(stderr, "tempo=%d notes_per_beat=%u\n",
		   tempo, notes_per_beat);

	if (!no_time_limit && verbose)
		fprintf(stderr, "recording for %lu seconds, %lu microseconds\n",
		    (u_long)record_time.tv_sec, (u_long)record_time.tv_usec);

	/* Create a midi header. */
	write_midi_header();

	(void)gettimeofday(&start_time, NULL);
	while (no_time_limit || timeleft(&start_time, &record_time)) {
		seq_event_t e;
		size_t wrsize;
		size_t rdsize;

		rdsize = (size_t)read(midifd, &e, sizeof e);
		if (rdsize == 0)
			break;

		if (rdsize != sizeof e)
			err(1, "read failed");

		if (rawfd != -1 && write(rawfd, &e, sizeof e) != sizeof e)
			err(1, "write to raw file failed");

		/* convert 'e' into something useful for output */
		wrsize = midi_event_to_output(e, buffer, bufsize);

		if (wrsize) {
			if ((size_t)write(outfd, buffer, wrsize) != wrsize)
				err(1, "write failed");
			data_size += wrsize;
		}
	}
	cleanup(0);
}

static void
debug_log(const char *file, size_t line, const char *fmt, ...)
{
	va_list ap;

	if (!debug)
		return;
	fprintf(stderr, "%s:%zu: ", file, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

#define LOG(fmt...) \
	debug_log(__func__, __LINE__, fmt)


/*
 * handle a SEQ_LOCAL event.  NetBSD /dev/music doesn't generate these.
 */
static size_t
midi_event_local_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t	size = 0;

	LOG("UNHANDLED SEQ_LOCAL");

	return size;
}

/*
 * convert a midi absolute time event to a variable length delay
 */
static size_t
midi_event_timer_wait_abs_to_output(
	seq_event_t e,
	u_char *buffer,
	size_t bufsize)
{
	static unsigned prev_div;
	static int prev_leftover;
	unsigned cur_div;
	unsigned val = 0, xdiv;
	int vallen = 0, i;

	if (prev_div == 0 && !oflag)
		prev_div = e.t_WAIT_ABS.divisions;
	cur_div = e.t_WAIT_ABS.divisions;

	xdiv = cur_div - prev_div + prev_leftover;
	if (round_beats != 1) {
		// round to closest
		prev_leftover = xdiv % round_beats;
		xdiv -= prev_leftover;
		if (verbose)
			fprintf(stderr, "adjusted beat value to %x (leftover = %d)\n",
			    xdiv, prev_leftover);
	}
	if (xdiv) {
		while (xdiv) {
			uint32_t extra = val ? 0x80 : 0;

			val <<= 8;
			val |= (xdiv & 0x7f) | extra;
			xdiv >>= 7;
			vallen++;
		}
	} else
		vallen = 1;

	for (i = 0; i < vallen; i++) {
		buffer[i] = val & 0xff;
		val >>= 8;
	}
	for (; i < 4; i++)
		buffer[i] = 0;
	LOG("TMR_WAIT_ABS: new div %x (cur %x prev %x): bufval (len=%u): "
	    "%02x:%02x:%02x:%02x",
	    cur_div - prev_div, cur_div, prev_div,
	    vallen, buffer[0], buffer[1], buffer[2], buffer[3]); 

	prev_div = cur_div;

	return vallen;
}

/*
 * handle a SEQ_TIMING event.
 */
static size_t
midi_event_timer_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t	size = 0;

	LOG("SEQ_TIMING");
	switch (e.timing.op) {
	case TMR_WAIT_REL:
		/* NetBSD /dev/music doesn't generate these. */
		LOG("UNHANDLED TMR_WAIT_REL: divisions: %x", e.t_WAIT_REL.divisions);
		break;

	case TMR_WAIT_ABS:
		size = midi_event_timer_wait_abs_to_output(e, buffer, bufsize);
		break;

	case TMR_STOP:
	case TMR_START:
	case TMR_CONTINUE:
	case TMR_TEMPO:
	case TMR_ECHO:
	case TMR_CLOCK:
	case TMR_SPP:
	case TMR_TIMESIG:
		/* NetBSD /dev/music doesn't generate these. */
		LOG("UNHANDLED timer op: %x", e.timing.op);
		break;
	
	default:
		LOG("unknown timer op: %x", e.timing.op);
		break;
	}

	return size;
}

/*
 * handle a SEQ_CHN_COMMON event.
 */
static size_t
midi_event_chn_common_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t	size = 0;

	assert(e.common.channel < 16);
	LOG("SEQ_CHN_COMMON");

	if (filter_devchan(e.common.device, e.common.channel))
		return 0;

	switch (e.common.op) {
	case MIDI_CTL_CHANGE:
		buffer[0] = MIDI_CTL_CHANGE | e.c_CTL_CHANGE.channel;
		buffer[1] = e.c_CTL_CHANGE.controller;
		buffer[2] = e.c_CTL_CHANGE.value;
		LOG("MIDI_CTL_CHANGE: channel %x ctrl %x val %x",
		    e.c_CTL_CHANGE.channel, e.c_CTL_CHANGE.controller,
		    e.c_CTL_CHANGE.value);
		size = 3;
		break;

	case MIDI_PGM_CHANGE:
		buffer[0] = MIDI_PGM_CHANGE | e.c_PGM_CHANGE.channel;
		buffer[1] = e.c_PGM_CHANGE.program;
		LOG("MIDI_PGM_CHANGE: channel %x program %x",
		    e.c_PGM_CHANGE.channel, e.c_PGM_CHANGE.program);
		size = 2;
		break;

	case MIDI_CHN_PRESSURE:
		buffer[0] = MIDI_CHN_PRESSURE | e.c_CHN_PRESSURE.channel;
		buffer[1] = e.c_CHN_PRESSURE.pressure;
		LOG("MIDI_CHN_PRESSURE: channel %x pressure %x",
		    e.c_CHN_PRESSURE.channel, e.c_CHN_PRESSURE.pressure);
		size = 2;
		break;

	case MIDI_PITCH_BEND:
		buffer[0] = MIDI_PITCH_BEND | e.c_PITCH_BEND.channel;
		/* 14 bits split over 2 data bytes, lsb first */
		buffer[1] = e.c_PITCH_BEND.value & 0x7f;
		buffer[2] = (e.c_PITCH_BEND.value >> 7) & 0x7f;
		LOG("MIDI_PITCH_BEND: channel %x val %x",
		    e.c_PITCH_BEND.channel, e.c_PITCH_BEND.value);
		size = 3;
		break;

	default:
		LOG("unknown common op: %x", e.voice.op);
		break;
	}

	return size;
}

/*
 * handle a SEQ_CHN_VOICE event.
 */
static size_t
midi_event_chn_voice_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t	size = 0;

	assert(e.common.channel < 16);
	LOG("SEQ_CHN_VOICE");

	if (filter_devchan(e.voice.device, e.voice.channel))
		return 0;

	switch (e.voice.op) {
	case MIDI_NOTEOFF:
		buffer[0] = MIDI_NOTEOFF | e.c_NOTEOFF.channel;
		buffer[1] = e.c_NOTEOFF.key;
		buffer[2] = e.c_NOTEOFF.velocity;

		LOG("MIDI_NOTEOFF: channel %x key %x velocity %x",
		    e.c_NOTEOFF.channel, e.c_NOTEOFF.key, e.c_NOTEOFF.velocity);
		size = 3;
		break;

	case MIDI_NOTEON:
		buffer[0] = MIDI_NOTEON | e.c_NOTEON.channel;
		buffer[1] = e.c_NOTEON.key;
		buffer[2] = e.c_NOTEON.velocity;

		LOG("MIDI_NOTEON: channel %x key %x velocity %x",
		    e.c_NOTEON.channel, e.c_NOTEON.key, e.c_NOTEON.velocity);
		size = 3;
		break;

	case MIDI_KEY_PRESSURE:
		buffer[0] = MIDI_KEY_PRESSURE | e.c_KEY_PRESSURE.channel;
		buffer[1] = e.c_KEY_PRESSURE.key;
		buffer[2] = e.c_KEY_PRESSURE.pressure;

		LOG("MIDI_KEY_PRESSURE: channel %x key %x pressure %x",
		    e.c_KEY_PRESSURE.channel, e.c_KEY_PRESSURE.key,
		    e.c_KEY_PRESSURE.pressure);
		size = 3;
		break;

	default:
		LOG("unknown voice op: %x", e.voice.op);
		break;
	}

	return size;
}

/*
 * handle a SEQ_SYSEX event.  NetBSD /dev/music doesn't generate these.
 */
static size_t
midi_event_sysex_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t	size = 0;

	LOG("UNHANDLED SEQ_SYSEX");

	return size;
}

/*
 * handle a SEQ_FULLSIZE event.  NetBSD /dev/music doesn't generate these.
 */
static size_t
midi_event_fullsize_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t	size = 0;

	LOG("UNHANDLED SEQ_FULLSIZE");

	return size;
}

/*
 * main handler for MIDI events.
 */
static size_t
midi_event_to_output(seq_event_t e, u_char *buffer, size_t bufsize)
{
	size_t size = 0;

	/* XXX so far we only process 4 byte returns */
	assert(bufsize >= 4);

	LOG("event: %02x:%02x:%02x:%02x %02x:%02x:%02x:%02x", e.tag,
	     e.unknown.byte[0], e.unknown.byte[1],
	     e.unknown.byte[2], e.unknown.byte[3],
	     e.unknown.byte[4], e.unknown.byte[5],
	     e.unknown.byte[6]);

	switch (e.tag) {
	case SEQ_LOCAL:
		size = midi_event_local_to_output(e, buffer, bufsize);
		break;

	case SEQ_TIMING:
		size = midi_event_timer_to_output(e, buffer, bufsize);
		break;

	case SEQ_CHN_COMMON:
		size = midi_event_chn_common_to_output(e, buffer, bufsize);
		break;

	case SEQ_CHN_VOICE:
		size = midi_event_chn_voice_to_output(e, buffer, bufsize);
		break;

	case SEQ_SYSEX:
		size = midi_event_sysex_to_output(e, buffer, bufsize);
		break;

	case SEQ_FULLSIZE:
		size = midi_event_fullsize_to_output(e, buffer, bufsize);
		break;

	default:
		errx(1, "don't understand midi tag %x", e.tag);
	}

	return size;
}

static bool
filter_array(unsigned val, unsigned *array, size_t arraylen)
{

	if (array == NULL)
		return false;

	for (; arraylen; arraylen--)
		if (array[arraylen - 1] == val)
			return false;

	return true;
}

static bool
filter_dev(unsigned device)
{

	if (filter_array(device, filt_devnos, num_filt_devnos))
		return true;

	return false;
}

static bool
filter_chan(unsigned channel)
{

	if (filter_array(channel, filt_chans, num_filt_chans))
		return true;

	return false;
}

static bool
filter_devchan(unsigned device, unsigned channel)
{

	if (filter_dev(device) || filter_chan(channel))
		return true;

	return false;
}

static int
timeleft(struct timeval *start_tvp, struct timeval *record_tvp)
{
	struct timeval now, diff;

	(void)gettimeofday(&now, NULL);
	timersub(&now, start_tvp, &diff);
	timersub(record_tvp, &diff, &now);

	return (now.tv_sec > 0 || (now.tv_sec == 0 && now.tv_usec > 0));
}

static void
parse_ints(const char *str, unsigned **arrayp, unsigned *sizep, const char *msg)
{
	unsigned count = 1, u, longest = 0, c = 0;
	unsigned *ip;
	const char *s, *os;
	char *num_buf;

	/*
	 * count all the comma separated values, and figre out
	 * the longest one.
	 */
	for (s = str; *s; s++) {
		c++;
		if (*s == ',') {
			count++;
			if (c > longest)
				longest = c;
			c = 0;
		}
	}
	*sizep = count;

	num_buf = malloc(longest + 1);
	ip = malloc(sizeof(*ip) * count);
	if (!ip || !num_buf)
		errx(1, "malloc failed");

	for (count = 0, s = os = str, u = 0; *s; s++) {
		if (*s == ',') {
			num_buf[u] = '\0';
			decode_uint(num_buf, &ip[count++]);
			os = s + 1;
			u = 0;
		} else
			num_buf[u++] = *s;
	}
	num_buf[u] = '\0';
	decode_uint(num_buf, &ip[count++]);
	*arrayp = ip;

	if (verbose) {
		fprintf(stderr, "Filtering %s in:", msg);
		for (size_t i = 0; i < *sizep; i++)
			fprintf(stderr, " %u", ip[i]);
		fprintf(stderr, "\n");
	}

	free(num_buf);
}

static void
cleanup(int signo)
{

	write_midi_trailer();
	rewrite_header();

	if (ioctl(midifd, SEQUENCER_TMR_STOP, NULL) < 0) {
		if (ignore_timer_fail)
			warn("failed to stop midi timer");
		else
			err(1, "failed to stop midi timer");
	}

	close(outfd);
	close(midifd);
	if (signo != 0)
		(void)raise_default_signal(signo);

	exit(0);
}

static void
rewrite_header(void)
{

	/* can't do this here! */
	if (stdout_mode)
		return;

	if (lseek(outfd, (off_t)0, SEEK_SET) == (off_t)-1)
		err(1, "could not seek to start of file for header rewrite");
	write_midi_header();
}

#define BYTE1(x)        ((x) & 0xff)
#define BYTE2(x)        (((x) >> 8) & 0xff)
#define BYTE3(x)        (((x) >> 16) & 0xff)
#define BYTE4(x)        (((x) >> 24) & 0xff)

static void
write_midi_header(void)
{
	unsigned char header[] = {
		'M', 'T', 'h', 'd',
		0, 0, 0, 6,
		0, 1,
		0, 0, /* ntracks */
		0, 0, /* notes per beat */
	};
	/* XXX only support one track so far */
	unsigned ntracks = 1;
	unsigned char track[] = {
		'M', 'T', 'r', 'k',
		0, 0, 0, 0,
	};
	unsigned char bpm[] = {
		0, 0xff, 0x51, 0x3,
		0, 0, 0, /* inverse tempo */
	};
	unsigned total_size = data_size + sizeof header + sizeof track + sizeof bpm;

	header[10] = BYTE2(ntracks);
	header[11] = BYTE1(ntracks);
	header[12] = BYTE2(notes_per_beat);
	header[13] = BYTE1(notes_per_beat);

	track[4] = BYTE4(total_size);
	track[5] = BYTE3(total_size);
	track[6] = BYTE2(total_size);
	track[7] = BYTE1(total_size);

#define TEMPO_INV(x)    (60000000UL / (x))
	bpm[4] = BYTE3(TEMPO_INV(tempo));
	bpm[5] = BYTE2(TEMPO_INV(tempo));
	bpm[6] = BYTE1(TEMPO_INV(tempo));

	if (write(outfd, header, sizeof header) != sizeof header)
		err(1, "write of header failed");
	if (write(outfd, track, sizeof track) != sizeof track)
		err(1, "write of track header failed");
	if (write(outfd, bpm, sizeof bpm) != sizeof bpm)
		err(1, "write of bpm header failed");

	LOG("wrote header: ntracks=%u notes_per_beat=%u tempo=%d total_size=%u",
	    ntracks, notes_per_beat, tempo, total_size);
}

static void
write_midi_trailer(void)
{
	unsigned char trailer[] = {
		0, 0xff, 0x2f, 0,
	};

	if (write(outfd, trailer, sizeof trailer) != sizeof trailer)
		err(1, "write of trailer failed");
}

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-aDfhqV] [options] {outfile|-}\n",
	    getprogname());
	fprintf(stderr, "Options:\n"
	    "\t-B buffer size\n"
	    "\t-c channels\n"
	    "\t-d devices\n"
	    "\t-f sequencerdev\n"
	    "\t-n notesperbeat\n"
	    "\t-r raw_output\n"
	    "\t-T tempo\n"
	    "\t-t recording time\n");
	exit(EXIT_FAILURE);
}
