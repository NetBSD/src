/*	$NetBSD: play.c,v 1.25 2001/03/28 03:18:39 simonb Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
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

#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <paths.h>

#include "libaudio.h"

int main (int, char *[]);
void usage (void);
void play (char *);
void play_fd (char *, int);
ssize_t audioctl_write_fromhdr (void *, size_t, int, size_t *);

audio_info_t	info;
int	volume;
int	balance;
int	port;
int	fflag;
int	qflag;
int	sample_rate;
int	encoding;
char	*encoding_str;
int	precision;
int	channels;

char	const *play_errstring = NULL;
size_t	bufsize;
int	audiofd, ctlfd;
int	exitstatus = EXIT_SUCCESS;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	size_t	len;
	int	ch;
	int	iflag = 0;
	int	verbose = 0;
	char	*device = 0;
	char	*ctldev = 0;

	while ((ch = getopt(argc, argv, "b:C:c:d:e:fhip:P:qs:Vv:")) != -1) {
		switch (ch) {
		case 'b':
			decode_int(optarg, &balance);
			if (balance < 0 || balance > 64)
				errx(1, "balance must be between 0 and 64");
			break;
		case 'c':
			decode_int(optarg, &channels);
			if (channels < 0)
				errx(1, "channels must be positive");
			break;
		case 'C':
			ctldev = optarg;
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
			if (precision != 8 && precision != 16 &&
			    precision != 24 && precision != 32)
				errx(1, "precision must be between 8, 16, 24 or 32");
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
				errx(1, "sample rate must be between 0 and 96000\n");
			break;
		case 'V':
			verbose++;
			break;
		case 'v':
			volume = atoi(optarg);
			if (volume < 0 || volume > 255)
				errx(1, "volume must be between 0 and 255\n");
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
		device = _PATH_AUDIO;
	if (ctldev == NULL && (ctldev = getenv("AUDIOCTLDEVICE")) == NULL)
		ctldev = _PATH_AUDIOCTL;

	audiofd = open(device, O_WRONLY);
#ifdef _PATH_OAUDIO
	/* Allow the non-unit device to be used. */
	if (audiofd < 0 && device == _PATH_AUDIO) {
		device = _PATH_OAUDIO;
		ctldev = _PATH_OAUDIOCTL;
		audiofd = open(device, O_WRONLY);
	}
#endif
	if (audiofd < 0)
		err(1, "failed to open %s", device);
	ctlfd = open(ctldev, O_RDWR);
	if (ctlfd < 0)
		err(1, "failed to open %s", ctldev);

	if (ioctl(ctlfd, AUDIO_GETINFO, &info) < 0)
		err(1, "failed to get audio info");
	bufsize = info.play.buffer_size;
	if (bufsize < 32 * 1024)
		bufsize = 32 * 1024;

	if (*argv)
		do
			play(*argv++);
		while (*argv);
	else
		play_fd("standard input", STDIN_FILENO);

	exit(exitstatus);
}

void
play(file)
	char *file;
{
	struct stat sb;
	void *addr, *oaddr;
	off_t	filesize;
	size_t datasize;
	ssize_t	hdrlen;
	int fd;

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

	oaddr = addr = mmap(0, (size_t)filesize, PROT_READ,
	    MAP_SHARED, fd, 0);

	/*
	 * if we failed to mmap the file, try to read it
	 * instead, so that filesystems, etc, that do not
	 * support mmap() work
	 */
	if (addr == MAP_FAILED) {
		play_fd(file, fd);
		close(fd);
		return;
	}

	/*
	 * give the VM system a bit of a hint about the type
	 * of accesses we will make.
	 */
	if (madvise(addr, filesize, MADV_SEQUENTIAL) < 0 &&
	    !qflag)
		warn("madvise failed, ignoring");

	/*
	 * get the header length and set up the audio device
	 */
	if ((hdrlen = audioctl_write_fromhdr(addr,
	    (size_t)filesize, ctlfd, &datasize)) < 0) {
		if (play_errstring)
			errx(1, "%s: %s", play_errstring, file);
		else
			errx(1, "unknown audio file: %s", file);
	}

	filesize -= hdrlen;
	addr = (char *)addr + hdrlen;
	if (filesize < datasize || datasize == 0) {
		warn("bogus datasize: %lu", (long)datasize);
		datasize = filesize;
	}

	while (datasize > bufsize) {
		if (write(audiofd, addr, bufsize) != bufsize)
			err(1, "write failed");
		addr = (char *)addr + bufsize;
		datasize -= bufsize;
	}
	if (write(audiofd, addr, (size_t)datasize) != (ssize_t)datasize)
		err(1, "final write failed");

	if (ioctl(audiofd, AUDIO_DRAIN) < 0 && !qflag)
		warn("audio drain ioctl failed");
	if (munmap(oaddr, (size_t)filesize) < 0)
		err(1, "munmap failed");

	close(fd);
}

/*
 * play the file on on the file descriptor fd
 */
void
play_fd(file, fd)
	char    *file;
	int     fd;
{
	char    *buffer = malloc(bufsize);
	ssize_t hdrlen;
	int     n;
	size_t	datasize;
	size_t	datainbuf;

	if (buffer == NULL)
		err(1, "malloc of read buffer failed");

	n = read(fd, buffer, bufsize);

	if (n < 0)
		err(1, "read of standard input failed");
	if (n == 0)
		errx(1, "EOF on standard input");

	hdrlen = audioctl_write_fromhdr(buffer, n, ctlfd, &datasize);
	if (hdrlen < 0) {
		if (play_errstring)
			errx(1, "%s: %s", play_errstring, file);
		else
			errx(1, "unknown audio file: %s", file);
	}

	/* advance the buffer if we have to */
	if (hdrlen > 0) {
		/* shouldn't happen */
		if (hdrlen > n)
			err(1, "bogus hdrlen %d > length %d?", (int)hdrlen, n);

		memmove(buffer, buffer + hdrlen, n - hdrlen);
	}

	datainbuf = n;
	do {
		if (datasize < datainbuf) {
			datainbuf = datasize;
		}
		else {
			n = read(fd, buffer + datainbuf, MIN(bufsize - datainbuf, datasize));
			if (n == -1)
				err(1, "read of %s failed", file);
			datainbuf += n;
		}
		if (write(audiofd, buffer, datainbuf) != datainbuf)
			err(1, "write failed");

		datasize -= datainbuf;
		datainbuf = 0;
	}
	while (datasize);

	if (ioctl(audiofd, AUDIO_DRAIN) < 0 && !qflag)
		warn("audio drain ioctl failed");
}

/*
 * only support sun and wav audio files so far ...
 *
 * XXX this should probably be mostly part of libaudio, but it
 * uses the local "info" variable. blah... fix me!
 */
ssize_t
audioctl_write_fromhdr(hdr, fsz, fd, datasize)
	void	*hdr;
	size_t	fsz;
	int	fd;
	size_t	*datasize;
{
	sun_audioheader	*sunhdr;
	ssize_t	hdr_len;

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

		*datasize = ntohl(sunhdr->data_size);
		goto set_audio_mode;
	}

	hdr_len = audio_parse_wav_hdr(hdr, fsz, &info.play.encoding,
	    &info.play.precision, &info.play.sample_rate, &info.play.channels, datasize);

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

	if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
		err(1, "failed to set audio info");

	return (hdr_len);
}

void
usage()
{

	fprintf(stderr, "Usage: %s [-hiqV] [options] files\n", getprogname());
	fprintf(stderr, "Options:\n\t"
	    "-C audio control device\n\t"
	    "-b balance (0-63)\n\t"
	    "-d audio device\n\t"
	    "-f force settings\n\t"
	    "\t-c forced channels\n\t"
	    "\t-e forced encoding\n\t"
	    "\t-P forced precision\n\t"
	    "\t-s forced sample rate\n\t"
	    "-i header information\n\t"
	    "-m monitor volume\n\t"
	    "-p output port\n\t"
	    "-v volume\n");
	exit(EXIT_FAILURE);
}
