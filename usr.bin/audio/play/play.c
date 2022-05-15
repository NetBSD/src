/*	$NetBSD: play.c,v 1.61 2022/05/15 02:16:06 mrg Exp $	*/

/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2010, 2015, 2019, 2021 Matthew R. Green
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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: play.c,v 1.61 2022/05/15 02:16:06 mrg Exp $");
#endif

#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <paths.h>

#include "libaudio.h"

typedef size_t (*convert)(void *inbuf, void *outbuf, size_t len);

static void usage(void) __dead;
static void play(char *);
static void play_fd(const char *, int);
static ssize_t audioctl_write_fromhdr(void *, size_t, int,
				      off_t *, const char *,
				      convert *);
static void cleanup(int) __dead;

static audio_info_t	info;
static int	volume;
static int	balance;
static int	port;
static int	fflag;
static int	qflag;
int	verbose;
static int	sample_rate;
static int	encoding;
static char	*encoding_str;
static int	precision;
static int	channels;

static char	const *play_errstring = NULL;
static size_t	bufsize;
static int	audiofd;
static int	exitstatus = EXIT_SUCCESS;

int
main(int argc, char *argv[])
{
	size_t	len;
	int	ch;
	int	iflag = 0;
	const char *defdevice = _PATH_SOUND;
	const char *device = NULL;

	while ((ch = getopt(argc, argv, "b:B:C:c:d:e:fhip:P:qs:Vv:")) != -1) {
		switch (ch) {
		case 'b':
			decode_int(optarg, &balance);
			if (balance < 0 || balance > 64)
				errx(1, "balance must be between 0 and 63");
			break;
		case 'B':
			bufsize = strsuftoll("write buffer size", optarg,
					     1, UINT_MAX);
			break;
		case 'c':
			decode_int(optarg, &channels);
			if (channels < 0)
				errx(1, "channels must be positive");
			break;
		case 'C':
			/* Ignore, compatibility */
			break;
		case 'd':
			device = optarg;
			break;
		case 'e':
			encoding_str = optarg;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'i':
			iflag++;
			break;
		case 'q':
			qflag++;
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

			if (strncmp(optarg, "speaker", len) == 0)
				port |= AUDIO_SPEAKER;
			else if (strncmp(optarg, "headphone", len) == 0)
				port |= AUDIO_HEADPHONE;
			else if (strncmp(optarg, "line", len) == 0)
				port |= AUDIO_LINE_OUT;
			else
				errx(1,
			    "port must be `speaker', `headphone', or `line'");
			break;
		case 's':
			decode_int(optarg, &sample_rate);
			if (sample_rate < 0 || sample_rate > 48000 * 2)	/* XXX */
				errx(1, "sample rate must be between 0 and 96000");
			break;
		case 'V':
			verbose++;
			break;
		case 'v':
			volume = atoi(optarg);
			if (volume < 0 || volume > 255)
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

	if (encoding_str) {
		encoding = audio_enc_to_val(encoding_str);
		if (encoding == -1)
			errx(1, "unknown encoding, bailing...");
	}

	if (device == NULL && (device = getenv("AUDIODEVICE")) == NULL &&
	    (device = getenv("AUDIODEV")) == NULL) /* Sun compatibility */
		device = defdevice;

	audiofd = open(device, O_WRONLY);
	if (audiofd < 0 && device == defdevice) {
		device = _PATH_SOUND0;
		audiofd = open(device, O_WRONLY);
	}

	if (audiofd < 0)
		err(1, "failed to open %s", device);

	if (ioctl(audiofd, AUDIO_GETINFO, &info) < 0)
		err(1, "failed to get audio info");
	if (bufsize == 0) {
		bufsize = info.play.buffer_size;
		if (bufsize < 32 * 1024)
			bufsize = 32 * 1024;
	}

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGHUP, cleanup);

	if (*argv)
		do
			play(*argv++);
		while (*argv);
	else
		play_fd("standard input", STDIN_FILENO);

	cleanup(0);
}

static void
cleanup(int signo)
{

	(void)ioctl(audiofd, AUDIO_FLUSH, NULL);
	(void)ioctl(audiofd, AUDIO_SETINFO, &info);
	close(audiofd);
	if (signo != 0) {
		(void)raise_default_signal(signo);
	}
	exit(exitstatus);
}

#ifndef __vax__
static size_t
float32_to_linear32(void *inbuf, void *outbuf, size_t len)
{
	uint8_t *inbuf8 = inbuf, *end = inbuf8 + len;
	uint8_t *outbuf8 = outbuf;
	float f;
	int32_t i;

	while (inbuf8 + sizeof f <= end) {
		memcpy(&f, inbuf8, sizeof f);

		/* saturate */
		if (f < -1.0)
			f = -1.0;
		if (f > 1.0)
			f = 1.0;

		/* Convert -1.0 to +1.0 into a 32 bit signed value */
		i = f * (float)INT32_MAX;

		memcpy(outbuf8, &i, sizeof i);

		inbuf8 += sizeof f;
		outbuf8 += sizeof i;
	}

	return len;
}

static size_t
float64_to_linear32(void *inbuf, void *outbuf, size_t len)
{
	uint8_t *inbuf8 = inbuf, *end = inbuf8 + len;
	uint8_t *outbuf8 = outbuf;
	double f;
	int32_t i;

	while (inbuf8 + sizeof f <= end) {
		memcpy(&f, inbuf8, sizeof f);

		/* saturate */
		if (f < -1.0)
			f = -1.0;
		if (f > 1.0)
			f = 1.0;

		/* Convert -1.0 to +1.0 into a 32 bit signed value */
		i = f * ((1u << 31) - 1);

		memcpy(outbuf8, &i, sizeof i);

		inbuf8 += sizeof f;
		outbuf8 += sizeof i;
	}

	return len / 2;
}
#endif /* __vax__ */

static size_t
audio_write(int fd, void *buf, size_t len, convert conv)
{
	static void *convert_buffer;
	static size_t convert_buffer_size;

	if (conv == NULL)
		return write(fd, buf, len);

	if (convert_buffer == NULL || convert_buffer_size < len) {
		free(convert_buffer);
		convert_buffer = malloc(len);
		if (convert_buffer == NULL)
			err(1, "malloc of convert buffer failed");
		convert_buffer_size = len;
	}
	len = conv(buf, convert_buffer, len);
	return write(fd, convert_buffer, len);
}

static void
play(char *file)
{
	convert conv = NULL;
	struct stat sb;
	void *addr, *oaddr;
	off_t	filesize;
	size_t	sizet_filesize;
	off_t datasize = 0;
	ssize_t	hdrlen;
	int fd;
	int nw;

	if (file[0] == '-' && file[1] == 0) {
		play_fd("standard input", STDIN_FILENO);
		return;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		if (!qflag)
			warn("could not open %s", file);
		exitstatus = EXIT_FAILURE;
		return;
	}

	if (fstat(fd, &sb) < 0)
		err(1, "could not fstat %s", file);
	filesize = sb.st_size;
	sizet_filesize = (size_t)filesize;

	/*
	 * if the file is not a regular file, doesn't fit in a size_t,
	 * or if we failed to mmap the file, try to read it instead, so
	 * that filesystems, etc, that do not support mmap() work
	 */
	if (!S_ISREG(sb.st_mode) || 
	    ((off_t)sizet_filesize != filesize) ||
	    (oaddr = addr = mmap(0, sizet_filesize, PROT_READ,
	    MAP_SHARED, fd, 0)) == MAP_FAILED) {
		play_fd(file, fd);
		close(fd);
		return;
	}

	/*
	 * give the VM system a bit of a hint about the type
	 * of accesses we will make.  we don't care about errors.
	 */
	madvise(addr, sizet_filesize, MADV_SEQUENTIAL);

	/*
	 * get the header length and set up the audio device, and
	 * determine any conversion needed.
	 */
	if ((hdrlen = audioctl_write_fromhdr(addr,
	    sizet_filesize, audiofd, &datasize, file, &conv)) < 0) {
		if (play_errstring)
			errx(1, "%s: %s", play_errstring, file);
		else
			errx(1, "unknown audio file: %s", file);
	}

	filesize -= hdrlen;
	addr = (char *)addr + hdrlen;
	if (filesize < datasize || datasize == 0) {
		if (filesize < datasize)
			warnx("bogus datasize: %ld", (u_long)datasize);
		datasize = filesize;
	}

	while ((uint64_t)datasize > bufsize) {
		nw = audio_write(audiofd, addr, bufsize, conv);
		if (nw == -1)
			err(1, "write failed");
		if ((size_t)nw != bufsize)
			errx(1, "write failed");
		addr = (char *)addr + bufsize;
		datasize -= bufsize;
	}
	nw = audio_write(audiofd, addr, datasize, conv);
	if (nw == -1)
		err(1, "final write failed");
	if ((off_t)nw != datasize)
		errx(1, "final write failed");

	if (ioctl(audiofd, AUDIO_DRAIN) < 0 && !qflag)
		warn("audio drain ioctl failed");
	if (munmap(oaddr, sizet_filesize) < 0)
		err(1, "munmap failed");

	close(fd);
}

/*
 * play the file on the file descriptor fd
 */
static void
play_fd(const char *file, int fd)
{
	convert conv = NULL;
	char    *buffer = malloc(bufsize);
	ssize_t hdrlen;
	int     nr, nw;
	off_t	datasize = 0;
	off_t	dataout = 0;

	if (buffer == NULL)
		err(1, "malloc of read buffer failed");

	nr = read(fd, buffer, bufsize);
	if (nr < 0)
		goto read_error;
	if (nr == 0) {
		if (fflag) {
			free(buffer);
			return;
		}
		err(1, "unexpected EOF");
	}
	hdrlen = audioctl_write_fromhdr(buffer, nr, audiofd, &datasize, file, &conv);
	if (hdrlen < 0) {
		if (play_errstring)
			errx(1, "%s: %s", play_errstring, file);
		else
			errx(1, "unknown audio file: %s", file);
	}
	if (hdrlen > 0) {
		if (hdrlen > nr)	/* shouldn't happen */
			errx(1, "header seems really large: %lld", (long long)hdrlen);
		memmove(buffer, buffer + hdrlen, nr - hdrlen);
		nr -= hdrlen;
	}
	while (datasize == 0 || dataout < datasize) {
		if (datasize != 0 && dataout + nr > datasize)
			nr = datasize - dataout;
		nw = audio_write(audiofd, buffer, nr, conv);
		if (nw == -1)
			err(1, "audio device write failed");
		if (nw != nr)
			errx(1, "audio device write failed");
		dataout += nw;
		nr = read(fd, buffer, bufsize);
		if (nr == -1)
			goto read_error;
		if (nr == 0)
			break;
	}
	/* something to think about: no message given for dataout < datasize */
	if (ioctl(audiofd, AUDIO_DRAIN) < 0 && !qflag)
		warn("audio drain ioctl failed");
	return;
read_error:
	err(1, "read of standard input failed");
}

/*
 * only support sun and wav audio files so far ...
 *
 * XXX this should probably be mostly part of libaudio, but it
 * uses the local "info" variable. blah... fix me!
 */
static ssize_t
audioctl_write_fromhdr(void *hdr, size_t fsz, int fd, off_t *datasize, const char *file, convert *conv)
{
	sun_audioheader	*sunhdr;
	ssize_t	hdr_len = 0;

	*conv = NULL;

	AUDIO_INITINFO(&info);
	sunhdr = hdr;
	if (ntohl(sunhdr->magic) == AUDIO_FILE_MAGIC) {
		if (audio_sun_to_encoding(ntohl(sunhdr->encoding),
		    &info.play.encoding, &info.play.precision)) {
			if (!qflag)
				warnx("unknown unsupported Sun audio encoding"
				      " format %d", ntohl(sunhdr->encoding));
			if (fflag)
				goto set_audio_mode;
			return (-1);
		}

		info.play.sample_rate = ntohl(sunhdr->sample_rate);
		info.play.channels = ntohl(sunhdr->channels);
		hdr_len = ntohl(sunhdr->hdr_size);

		*datasize = (off_t)ntohl(sunhdr->data_size);
		goto set_audio_mode;
	}

	hdr_len = audio_wav_parse_hdr(hdr, fsz, &info.play.encoding,
	    &info.play.precision, &info.play.sample_rate, &info.play.channels,
	    datasize);

	switch (hdr_len) {
	case AUDIO_ESHORTHDR:
	case AUDIO_EWAVUNSUPP:
	case AUDIO_EWAVBADPCM:
	case AUDIO_EWAVNODATA:
		play_errstring = audio_errstring(hdr_len);
		/* FALL THROUGH */
	case AUDIO_ENOENT:
		break;
	default:
		if (hdr_len < 1)
			break;
		goto set_audio_mode;
	}
	/*
	 * if we don't know it, bail unless we are forcing.
	 */
	if (fflag == 0)
		return (-1);
set_audio_mode:
	if (port)
		info.play.port = port;
	if (volume)
		info.play.gain = volume;
	if (balance)
		info.play.balance = balance;
	if (fflag) {
		if (sample_rate)
			info.play.sample_rate = sample_rate;
		if (channels)
			info.play.channels = channels;
		if (encoding)
			info.play.encoding = encoding;
		if (precision)
			info.play.precision = precision;
		hdr_len = 0;
	}
	info.mode = AUMODE_PLAY_ALL;

	if (verbose) {
		const char *enc = audio_enc_from_val(info.play.encoding);

		printf("%s: sample_rate=%d channels=%d "
		   "datasize=%lld "
		   "precision=%d%s%s\n", file,
		   info.play.sample_rate,
		   info.play.channels,
		   (long long)*datasize,
		   info.play.precision,
		   enc ? " encoding=" : "", 
		   enc ? enc : "");
	}

#ifndef __vax__
	if (info.play.encoding == AUDIO_ENCODING_LIBAUDIO_FLOAT32 ||
	    info.play.encoding == AUDIO_ENCODING_LIBAUDIO_FLOAT64) {
		const char *msg;

		if (info.play.encoding == AUDIO_ENCODING_LIBAUDIO_FLOAT32) {
			if (sizeof(float) * CHAR_BIT != 32)
				return -1;
			*conv = float32_to_linear32;
			msg = "32";
		} else {
			if (sizeof(double) * CHAR_BIT != 64)
				return -1;
			*conv = float64_to_linear32;
			msg = "64";
		}
		info.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
		info.play.precision = 32;
		if (verbose)
			printf("%s: converting IEEE float%s to precision=%d "
			       "encoding=%s\n", file, msg,
			       info.play.precision,
			       audio_enc_from_val(info.play.encoding));
	}
#endif /* __vax__ */

	if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
		err(1, "failed to set audio info");

	return (hdr_len);
}

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-hiqV] [options] files\n", getprogname());
	fprintf(stderr, "Options:\n\t"
	    "-B buffer size\n\t"
	    "-b balance (0-63)\n\t"
	    "-d audio device\n\t"
	    "-f force settings\n\t"
	    "\t-c forced channels\n\t"
	    "\t-e forced encoding\n\t"
	    "\t-P forced precision\n\t"
	    "\t-s forced sample rate\n\t"
	    "-i header information\n\t"
	    "-p output port\n\t"
	    "-v volume\n");
	exit(EXIT_FAILURE);
}
