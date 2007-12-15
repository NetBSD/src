/*	$NetBSD: record.c,v 1.45 2007/12/15 19:44:49 perry Exp $	*/

/*
 * Copyright (c) 1999, 2002 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__RCSID("$NetBSD: record.c,v 1.45 2007/12/15 19:44:49 perry Exp $");
#endif


#include <sys/types.h>
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

audio_info_t info, oinfo;
ssize_t	total_size = -1;
const char *device;
int	format = AUDIO_FORMAT_DEFAULT;
char	*header_info;
char	default_info[8] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
int	audiofd, outfd;
int	qflag, aflag, fflag;
int	verbose;
int	monitor_gain, omonitor_gain;
int	gain;
int	balance;
int	port;
int	encoding;
char	*encoding_str;
int	precision;
int	sample_rate;
int	channels;
struct timeval record_time;
struct timeval start_time;

void (*conv_func) (u_char *, int);

void usage (void);
int main (int, char *[]);
int timeleft (struct timeval *, struct timeval *);
void cleanup (int) __dead;
int write_header_sun (void **, size_t *, int *);
int write_header_wav (void **, size_t *, int *);
void write_header (void);
void rewrite_header (void);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	u_char	*buffer;
	size_t	len, bufsize;
	int	ch, no_time_limit = 1;
	const char *defdevice = _PATH_SOUND;

	while ((ch = getopt(argc, argv, "ab:C:F:c:d:e:fhi:m:P:p:qt:s:Vv:")) != -1) {
		switch (ch) {
		case 'a':
			aflag++;
			break;
		case 'b':
			decode_int(optarg, &balance);
			if (balance < 0 || balance > 63)
				errx(1, "balance must be between 0 and 63");
			break;
		case 'C':
			/* Ignore, compatibility */
			break;
		case 'F':
			format = audio_format_from_str(optarg);
			if (format < 0)
				errx(1, "Unknown audio format; supported "
				    "formats: \"sun\", \"wav\", and \"none\"");
			break;
		case 'c':
			decode_int(optarg, &channels);
			if (channels < 0 || channels > 16)
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
			header_info = optarg;
			break;
		case 'm':
			decode_int(optarg, &monitor_gain);
			if (monitor_gain < 0 || monitor_gain > 255)
				errx(1, "monitor volume must be between 0 and 255");
			break;
		case 'P':
			decode_int(optarg, &precision);
			if (precision != 4 && precision != 8 &&
			    precision != 16 && precision != 24 &&
			    precision != 32)
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
			qflag++;
			break;
		case 's':
			decode_int(optarg, &sample_rate);
			if (sample_rate < 0 || sample_rate > 48000 * 2)	/* XXX */
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
		encoding = audio_enc_to_val(encoding_str);
		if (encoding == -1)
			errx(1, "unknown encoding, bailing...");
	}
#if 0
	else
		encoding = AUDIO_ENCODING_ULAW;
#endif

	/*
	 * open the output file
	 */
	if (argv[0][0] != '-' || argv[0][1] != '\0') {
		/* intuit the file type from the name */
		if (format == AUDIO_FORMAT_DEFAULT)
		{
			size_t flen = strlen(*argv);
			const char *arg = *argv;

			if (strcasecmp(arg + flen - 3, ".au") == 0)
				format = AUDIO_FORMAT_SUN;
			else if (strcasecmp(arg + flen - 4, ".wav") == 0)
				format = AUDIO_FORMAT_WAV;
		}
		outfd = open(*argv, O_CREAT|(aflag ? O_APPEND : O_TRUNC)|O_WRONLY, 0666);
		if (outfd < 0)
			err(1, "could not open %s", *argv);
	} else
		outfd = STDOUT_FILENO;

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
	bufsize = oinfo.record.buffer_size;
	if (bufsize < 32 * 1024)
		bufsize = 32 * 1024;
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
#define SETINFO(x)	if (x) \
				info.record.x = x; \
			else \
				info.record.x = x = oinfo.record.x;
	SETINFO (sample_rate)
	SETINFO (channels)
	SETINFO (precision)
	SETINFO (encoding)
	SETINFO (gain)
	SETINFO (port)
	SETINFO (balance)
#undef SETINFO

	if (monitor_gain)
		info.monitor_gain = monitor_gain;
	else
		monitor_gain = oinfo.monitor_gain;

	info.mode = AUMODE_RECORD;
	if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0)
		err(1, "failed to set audio info");

	signal(SIGINT, cleanup);
	write_header();
	total_size = 0;

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
		if (read(audiofd, buffer, bufsize) != bufsize)
			err(1, "read failed");
		if (conv_func)
			(*conv_func)(buffer, bufsize);
		if (write(outfd, buffer, bufsize) != bufsize)
			err(1, "write failed");
		total_size += bufsize;
	}
	cleanup(0);
}

int
timeleft(start_tvp, record_tvp)
	struct timeval *start_tvp;
	struct timeval *record_tvp;
{
	struct timeval now, diff;

	(void)gettimeofday(&now, NULL);
	timersub(&now, start_tvp, &diff);
	timersub(record_tvp, &diff, &now);

	return (now.tv_sec > 0 || (now.tv_sec == 0 && now.tv_usec > 0));
}

void
cleanup(signo)
	int signo;
{

	rewrite_header();
	close(outfd);
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

int
write_header_sun(hdrp, lenp, leftp)
	void **hdrp;
	size_t *lenp;
	int *leftp;
{
	static int warned = 0;
	static sun_audioheader auh;
	int sunenc, oencoding = encoding;

	/* only perform conversions if we don't specify the encoding */
	switch (encoding) {
	case AUDIO_ENCODING_ULINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (precision == 16)
			conv_func = change_sign16_swap_bytes_le;
		else if (precision == 32)
			conv_func = change_sign32_swap_bytes_le;
		if (conv_func)
			encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;

	case AUDIO_ENCODING_ULINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (precision == 16)
			conv_func = change_sign16_be;
		else if (precision == 32)
			conv_func = change_sign32_be;
		if (conv_func)
			encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;

	case AUDIO_ENCODING_SLINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (precision == 16)
			conv_func = swap_bytes;
		else if (precision == 32)
			conv_func = swap_bytes32;
		if (conv_func)
			encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;

#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
		encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;
#endif
	}
	
	/* if we can't express this as a Sun header, don't write any */
	if (audio_encoding_to_sun(encoding, precision, &sunenc) != 0) {
		if (!qflag && !warned) {
			const char *s = audio_enc_from_val(oencoding);

			if (s == NULL)
				s = "(unknown)";
			warnx("failed to convert to sun encoding from %s "
			      "(precision %d);\nSun audio header not written",
			      s, precision);
		}
		format = AUDIO_FORMAT_NONE;
		conv_func = 0;
		warned = 1;
		return -1;
	}

	auh.magic = htonl(AUDIO_FILE_MAGIC);
	if (outfd == STDOUT_FILENO)
		auh.data_size = htonl(AUDIO_UNKNOWN_SIZE);
	else if (total_size != -1)
		auh.data_size = htonl(total_size);
	else
		auh.data_size = 0;
	auh.encoding = htonl(sunenc);
	auh.sample_rate = htonl(sample_rate);
	auh.channels = htonl(channels);
	if (header_info) {
		int 	len, infolen;

		infolen = ((len = strlen(header_info)) + 7) & 0xfffffff8;
		*leftp = infolen - len;
		auh.hdr_size = htonl(sizeof(auh) + infolen);
	} else {
		*leftp = sizeof(default_info);
		auh.hdr_size = htonl(sizeof(auh) + *leftp);
	}
	*(sun_audioheader **)hdrp = &auh;
	*lenp = sizeof auh;
	return 0;
}

int
write_header_wav(hdrp, lenp, leftp)
	void **hdrp;
	size_t *lenp;
	int *leftp;
{
	/*
	 * WAV header we write looks like this:
	 *
	 *      bytes   purpose
	 *      0-3     "RIFF"
	 *      4-7     file length (minus 8)
	 *      8-15    "WAVEfmt "
	 *      16-19   format size
	 *      20-21   format tag
	 *      22-23   number of channels
	 *      24-27   sample rate
	 *      28-31   average bytes per second
	 *      32-33   block alignment
	 *      34-35   bits per sample
	 *
	 * then for ULAW and ALAW outputs, we have an extended chunk size
	 * and a WAV "fact" to add:
	 *
	 *      36-37   length of extension (== 0)
	 *      38-41   "fact"
	 *      42-45   fact size
	 *      46-49   number of samples written
	 *      50-53   "data"
	 *      54-57   data length
	 *      58-     raw audio data
	 *
	 * for PCM outputs we have just the data remaining:
	 *
	 *      36-39   "data"
	 *      40-43   data length
	 *      44-     raw audio data
	 *
	 *	RIFF\^@^C^@WAVEfmt ^P^@^@^@^A^@^B^@D<AC>^@^@^P<B1>^B^@^D^@^P^@data^@^@^C^@^@^@^@^@^@^@^@^@^@
	 */
	char	wavheaderbuf[64], *p = wavheaderbuf;
	const char *riff = "RIFF",
	    *wavefmt = "WAVEfmt ",
	    *fact = "fact",
	    *data = "data";
	u_int32_t filelen, fmtsz, sps, abps, factsz = 4, nsample, datalen;
	u_int16_t fmttag, nchan, align, bps, extln = 0;

	if (header_info)
		warnx("header information not supported for WAV");
	*leftp = 0;

	switch (precision) {
	case 8:
		bps = 8;
		break;
	case 16:
		bps = 16;
		break;
	case 32:
		bps = 32;
		break;
	default:
		{
			static int warned = 0;

			if (warned == 0) {
				warnx("can not support precision of %d", precision);
				warned = 1;
			}
		}
		return (-1);
	}

	switch (encoding) {
	case AUDIO_ENCODING_ULAW:
		fmttag = WAVE_FORMAT_MULAW;
		fmtsz = 18;
		align = channels;
		break;

	case AUDIO_ENCODING_ALAW:
		fmttag = WAVE_FORMAT_ALAW;
		fmtsz = 18;
		align = channels;
		break;

	/*
	 * we could try to support RIFX but it seems to be more portable
	 * to output little-endian data for WAV files.
	 */
	case AUDIO_ENCODING_ULINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (bps == 16)
			conv_func = change_sign16_swap_bytes_be;
		else if (bps == 32)
			conv_func = change_sign32_swap_bytes_be;
		goto fmt_pcm;

	case AUDIO_ENCODING_SLINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (bps == 8)
			conv_func = change_sign8;
		else if (bps == 16)
			conv_func = swap_bytes;
		else if (bps == 32)
			conv_func = swap_bytes32;
		goto fmt_pcm;

	case AUDIO_ENCODING_ULINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (bps == 16)
			conv_func = change_sign16_le;
		else if (bps == 32)
			conv_func = change_sign32_le;
		/* FALLTHROUGH */

	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_PCM16:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (bps == 8)
			conv_func = change_sign8;
fmt_pcm:
		fmttag = WAVE_FORMAT_PCM;
		fmtsz = 16;
		align = channels * (bps / 8);
		break;

	default:
		{
			static int warned = 0;

			if (warned == 0) {
				const char *s = wav_enc_from_val(encoding);

				if (s == NULL)
					warnx("can not support encoding of %s", s);
				else
					warnx("can not support encoding of %d", encoding);
				warned = 1;
			}
		}
		format = AUDIO_FORMAT_NONE;
		return (-1);
	}

	nchan = channels;
	sps = sample_rate;

	/* data length */
	if (outfd == STDOUT_FILENO)
		datalen = 0;
	else if (total_size != -1)
		datalen = total_size;
	else
		datalen = 0;

	/* file length */
	filelen = 4 + (8 + fmtsz) + (8 + datalen);
	if (fmttag != WAVE_FORMAT_PCM)
		filelen += 8 + factsz;

	abps = (double)align*sample_rate / (double)1 + 0.5;

	nsample = (datalen / bps) / sample_rate;
	
	/*
	 * now we've calculated the info, write it out!
	 */
#define put32(x) do { \
	u_int32_t _f; \
	putle32(_f, (x)); \
	memcpy(p, &_f, 4); \
} while (0)
#define put16(x) do { \
	u_int16_t _f; \
	putle16(_f, (x)); \
	memcpy(p, &_f, 2); \
} while (0)
	memcpy(p, riff, 4);
	p += 4;				/* 4 */
	put32(filelen);
	p += 4;				/* 8 */
	memcpy(p, wavefmt, 8);
	p += 8;				/* 16 */
	put32(fmtsz);
	p += 4;				/* 20 */
	put16(fmttag);
	p += 2;				/* 22 */
	put16(nchan);
	p += 2;				/* 24 */
	put32(sps);
	p += 4;				/* 28 */
	put32(abps);
	p += 4;				/* 32 */
	put16(align);
	p += 2;				/* 34 */
	put16(bps);
	p += 2;				/* 36 */
	/* NON PCM formats have an extended chunk; write it */
	if (fmttag != WAVE_FORMAT_PCM) {
		put16(extln);
		p += 2;			/* 38 */
		memcpy(p, fact, 4);
		p += 4;			/* 42 */
		put32(factsz);
		p += 4;			/* 46 */
		put32(nsample);
		p += 4;			/* 50 */
	}
	memcpy(p, data, 4);
	p += 4;				/* 40/54 */
	put32(datalen);
	p += 4;				/* 44/58 */
#undef put32
#undef put16

	*hdrp = wavheaderbuf;
	*lenp = (p - wavheaderbuf);

	return 0;
}

void
write_header()
{
	struct iovec iv[3];
	int veclen, left, tlen;
	void *hdr;
	size_t hdrlen;

	switch (format) {
	case AUDIO_FORMAT_DEFAULT:
	case AUDIO_FORMAT_SUN:
		if (write_header_sun(&hdr, &hdrlen, &left) != 0)
			return;
		break;
	case AUDIO_FORMAT_WAV:
		if (write_header_wav(&hdr, &hdrlen, &left) != 0)
			return;
		break;
	case AUDIO_FORMAT_NONE:
		return;
	default:
		errx(1, "unknown audio format");
	}

	veclen = 0;
	tlen = 0;
		
	if (hdrlen != 0) {
		iv[veclen].iov_base = hdr;
		iv[veclen].iov_len = hdrlen;
		tlen += iv[veclen++].iov_len;
	}
	if (header_info) {
		iv[veclen].iov_base = header_info;
		iv[veclen].iov_len = (int)strlen(header_info) + 1;
		tlen += iv[veclen++].iov_len;
	}
	if (left) {
		iv[veclen].iov_base = default_info;
		iv[veclen].iov_len = left;
		tlen += iv[veclen++].iov_len;
	}

	if (tlen == 0)
		return;

	if (writev(outfd, iv, veclen) != tlen)
		err(1, "could not write audio header");
}

void
rewrite_header()
{

	/* can't do this here! */
	if (outfd == STDOUT_FILENO)
		return;

	if (lseek(outfd, SEEK_SET, 0) < 0)
		err(1, "could not seek to start of file for header rewrite");
	write_header();
}

void
usage()
{

	fprintf(stderr, "Usage: %s [-afhqV] [options] {files ...|-}\n",
	    getprogname());
	fprintf(stderr, "Options:\n\t"
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
