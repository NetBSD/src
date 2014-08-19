/*	$NetBSD: record.c,v 1.52.8.1 2014/08/20 00:04:56 tls Exp $	*/

/*
 * Copyright (c) 1999, 2002, 2003, 2005, 2010 Matthew R. Green
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
 * SunOS compatible audiorecord(1)
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: record.c,v 1.52.8.1 2014/08/20 00:04:56 tls Exp $");
#endif


#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "libaudio.h"
#include "auconv.h"

static audio_info_t info, oinfo;
static const char *device;
static int	audiofd;
static int	aflag, fflag;
int	verbose;
static int	monitor_gain, omonitor_gain;
static int	gain;
static int	balance;
static int	port;
static char	*encoding_str;
static struct write_info wi;
static struct timeval record_time;
static struct timeval start_time;

static void (*conv_func) (u_char *, int);

static void usage (void) __dead;
static int timeleft (struct timeval *, struct timeval *);
static void cleanup (int) __dead;
static void rewrite_header (void);

int
main(int argc, char *argv[])
{
	u_char	*buffer;
	size_t	len, bufsize = 0;
	int	ch, no_time_limit = 1;
	const char *defdevice = _PATH_SOUND;

	/*
	 * Initialise the write_info.
	 */
	wi.format = AUDIO_FORMAT_DEFAULT;
	wi.total_size = -1;

	while ((ch = getopt(argc, argv, "ab:B:C:F:c:d:e:fhi:m:P:p:qt:s:Vv:")) != -1) {
		switch (ch) {
		case 'a':
			aflag++;
			break;
		case 'b':
			decode_int(optarg, &balance);
			if (balance < 0 || balance > 63)
				errx(1, "balance must be between 0 and 63");
			break;
		case 'B':
			bufsize = strsuftoll("read buffer size", optarg,
					     1, UINT_MAX);
			break;
		case 'C':
			/* Ignore, compatibility */
			break;
		case 'F':
			wi.format = audio_format_from_str(optarg);
			if (wi.format < 0)
				errx(1, "Unknown audio format; supported "
				    "formats: \"sun\", \"wav\", and \"none\"");
			break;
		case 'c':
			decode_int(optarg, &wi.channels);
			if (wi.channels < 0 || wi.channels > 16)
				errx(1, "channels must be between 0 and 16");
			break;
		case 'd':
			device = optarg;
			break;
		case 'e':
			encoding_str = optarg;
			break;
		case 'f':
			fflag++;
			break;
		case 'i':
			wi.header_info = optarg;
			break;
		case 'm':
			decode_int(optarg, &monitor_gain);
			if (monitor_gain < 0 || monitor_gain > 255)
				errx(1, "monitor volume must be between 0 and 255");
			break;
		case 'P':
			decode_int(optarg, &wi.precision);
			if (wi.precision != 4 && wi.precision != 8 &&
			    wi.precision != 16 && wi.precision != 24 &&
			    wi.precision != 32)
				errx(1, "precision must be between 4, 8, 16, 24 or 32");
			break;
		case 'p':
			len = strlen(optarg);

			if (strncmp(optarg, "mic", len) == 0)
				port |= AUDIO_MICROPHONE;
			else if (strncmp(optarg, "cd", len) == 0 ||
			           strncmp(optarg, "internal-cd", len) == 0)
				port |= AUDIO_CD;
			else if (strncmp(optarg, "line", len) == 0)
				port |= AUDIO_LINE_IN;
			else
				errx(1,
			    "port must be `cd', `internal-cd', `mic', or `line'");
			break;
		case 'q':
			wi.qflag++;
			break;
		case 's':
			decode_int(optarg, &wi.sample_rate);
			if (wi.sample_rate < 0 || wi.sample_rate > 48000 * 2)	/* XXX */
				errx(1, "sample rate must be between 0 and 96000");
			break;
		case 't':
			no_time_limit = 0;
			decode_time(optarg, &record_time);
			break;
		case 'V':
			verbose++;
			break;
		case 'v':
			decode_int(optarg, &gain);
			if (gain < 0 || gain > 255)
				errx(1, "volume must be between 0 and 255");
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
	 * convert the encoding string into a value.
	 */
	if (encoding_str) {
		wi.encoding = audio_enc_to_val(encoding_str);
		if (wi.encoding == -1)
			errx(1, "unknown encoding, bailing...");
	}

	/*
	 * open the output file
	 */
	if (argv[0][0] != '-' || argv[0][1] != '\0') {
		/* intuit the file type from the name */
		if (wi.format == AUDIO_FORMAT_DEFAULT)
		{
			size_t flen = strlen(*argv);
			const char *arg = *argv;

			if (strcasecmp(arg + flen - 3, ".au") == 0)
				wi.format = AUDIO_FORMAT_SUN;
			else if (strcasecmp(arg + flen - 4, ".wav") == 0)
				wi.format = AUDIO_FORMAT_WAV;
		}
		wi.outfd = open(*argv, O_CREAT|(aflag ? O_APPEND : O_TRUNC)|O_WRONLY, 0666);
		if (wi.outfd < 0)
			err(1, "could not open %s", *argv);
	} else
		wi.outfd = STDOUT_FILENO;

	/*
	 * open the audio device
	 */
	if (device == NULL && (device = getenv("AUDIODEVICE")) == NULL &&
	    (device = getenv("AUDIODEV")) == NULL) /* Sun compatibility */
		device = defdevice;

	audiofd = open(device, O_RDONLY);
	if (audiofd < 0 && device == defdevice) {
		device = _PATH_SOUND0;
		audiofd = open(device, O_RDONLY);
	}
	if (audiofd < 0)
		err(1, "failed to open %s", device);

	/*
	 * work out the buffer size to use, and allocate it.  also work out
	 * what the old monitor gain value is, so that we can reset it later.
	 */
	if (ioctl(audiofd, AUDIO_GETINFO, &oinfo) < 0)
		err(1, "failed to get audio info");
	if (bufsize == 0) {
		bufsize = oinfo.record.buffer_size;
		if (bufsize < 32 * 1024)
			bufsize = 32 * 1024;
	}
	omonitor_gain = oinfo.monitor_gain;

	buffer = malloc(bufsize);
	if (buffer == NULL)
		err(1, "couldn't malloc buffer of %d size", (int)bufsize);

	/*
	 * set up audio device for recording with the speified parameters
	 */
	AUDIO_INITINFO(&info);

	/*
	 * for these, get the current values for stuffing into the header
	 */
#define SETINFO2(x, y)	if (x) \
				info.record.y = x; \
			else \
				info.record.y = x = oinfo.record.y;
#define SETINFO(x)	SETINFO2(wi.x, x)

	SETINFO (sample_rate)
	SETINFO (channels)
	SETINFO (precision)
	SETINFO (encoding)
	SETINFO2 (gain, gain)
	SETINFO2 (port, port)
	SETINFO2 (balance, balance)
#undef SETINFO
#undef SETINFO2

	if (monitor_gain)
		info.monitor_gain = monitor_gain;
	else
		monitor_gain = oinfo.monitor_gain;

	info.mode = AUMODE_RECORD;
	if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0)
		err(1, "failed to set audio info");

	signal(SIGINT, cleanup);

	wi.total_size = 0;

	write_header(&wi);
	if (wi.format == AUDIO_FORMAT_NONE)
		errx(1, "unable to determine audio format");
	conv_func = write_get_conv_func(&wi);

	if (verbose && conv_func) {
		const char *s = NULL;

		if (conv_func == swap_bytes)
			s = "swap bytes (16 bit)";
		else if (conv_func == swap_bytes32)
			s = "swap bytes (32 bit)";
		else if (conv_func == change_sign16_be)
			s = "change sign (big-endian, 16 bit)";
		else if (conv_func == change_sign16_le)
			s = "change sign (little-endian, 16 bit)";
		else if (conv_func == change_sign32_be)
			s = "change sign (big-endian, 32 bit)";
		else if (conv_func == change_sign32_le)
			s = "change sign (little-endian, 32 bit)";
		else if (conv_func == change_sign16_swap_bytes_be)
			s = "change sign & swap bytes (big-endian, 16 bit)";
		else if (conv_func == change_sign16_swap_bytes_le)
			s = "change sign & swap bytes (little-endian, 16 bit)";
		else if (conv_func == change_sign32_swap_bytes_be)
			s = "change sign (big-endian, 32 bit)";
		else if (conv_func == change_sign32_swap_bytes_le)
			s = "change sign & swap bytes (little-endian, 32 bit)";
		
		if (s)
			fprintf(stderr, "%s: converting, using function: %s\n",
			    getprogname(), s);
		else
			fprintf(stderr, "%s: using unnamed conversion "
					"function\n", getprogname());
	}

	if (verbose)
		fprintf(stderr,
		   "sample_rate=%d channels=%d precision=%d encoding=%s\n",
		   info.record.sample_rate, info.record.channels,
		   info.record.precision,
		   audio_enc_from_val(info.record.encoding));

	if (!no_time_limit && verbose)
		fprintf(stderr, "recording for %lu seconds, %lu microseconds\n",
		    (u_long)record_time.tv_sec, (u_long)record_time.tv_usec);

	(void)gettimeofday(&start_time, NULL);
	while (no_time_limit || timeleft(&start_time, &record_time)) {
		if ((size_t)read(audiofd, buffer, bufsize) != bufsize)
			err(1, "read failed");
		if (conv_func)
			(*conv_func)(buffer, bufsize);
		if ((size_t)write(wi.outfd, buffer, bufsize) != bufsize)
			err(1, "write failed");
		wi.total_size += bufsize;
	}
	cleanup(0);
}

int
timeleft(struct timeval *start_tvp, struct timeval *record_tvp)
{
	struct timeval now, diff;

	(void)gettimeofday(&now, NULL);
	timersub(&now, start_tvp, &diff);
	timersub(record_tvp, &diff, &now);

	return (now.tv_sec > 0 || (now.tv_sec == 0 && now.tv_usec > 0));
}

void
cleanup(int signo)
{

	rewrite_header();
	close(wi.outfd);
	if (omonitor_gain) {
		AUDIO_INITINFO(&info);
		info.monitor_gain = omonitor_gain;
		if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0)
			err(1, "failed to reset audio info");
	}
	close(audiofd);
	if (signo != 0) {
		(void)raise_default_signal(signo);
	}
	exit(0);
}

static void
rewrite_header(void)
{

	/* can't do this here! */
	if (wi.outfd == STDOUT_FILENO)
		return;

	if (lseek(wi.outfd, (off_t)0, SEEK_SET) == (off_t)-1)
		err(1, "could not seek to start of file for header rewrite");
	write_header(&wi);
}

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-afhqV] [options] {files ...|-}\n",
	    getprogname());
	fprintf(stderr, "Options:\n\t"
	    "-B buffer size\n\t"
	    "-b balance (0-63)\n\t"
	    "-c channels\n\t"
	    "-d audio device\n\t"
	    "-e encoding\n\t"
	    "-F format\n\t"
	    "-i header information\n\t"
	    "-m monitor volume\n\t"
	    "-P precision (4, 8, 16, 24, or 32 bits)\n\t"
	    "-p input port\n\t"
	    "-s sample rate\n\t"
	    "-t recording time\n\t"
	    "-v volume\n");
	exit(EXIT_FAILURE);
}
