/*	$NetBSD: play.c,v 1.4 1999/03/27 05:14:38 mrg Exp $	*/

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

int main __P((int, char *[]));
void usage __P((void));
ssize_t audioctl_write_fromhdr __P((void *, size_t, int, int));

audio_info_t	info;
int	volume = 0;
int	balance = 0;
int	port = 0;
char	const *play_errstring = NULL;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	size_t	len, bufsize;
	ssize_t	hdrlen;
	off_t	filesize;
	int	ch, audiofd, ctlfd;
	int	iflag = 0;
	int	qflag = 0;
	int	verbose = 0;
	char	*device = 0;
	char	*ctldev = 0;

	while ((ch = getopt(argc, argv, "b:C:d:hiqp:Vv:")) != -1) {
		switch (ch) {
		case 'b':
			decode_int(optarg, &balance);
			if (balance < 0 || balance > 64)
				errx(1, "balance must be between 0 and 64\n");
			break;
		case 'C':
			ctldev = optarg;
			break;
		case 'd':
			device = optarg;
			break;
		case 'i':
			iflag++;
			break;
		case 'q':
			qflag++;
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

	if (device == NULL)
		device = _PATH_AUDIO;
	if (ctldev == NULL)
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

	if (*argv) {
		int fd;
		struct stat sb;
		void *addr, *oaddr;

		do {
			fd = open(*argv, O_RDONLY);
			if (fd < 0)
				err(1, "could not open %s", *argv);

			if (fstat(fd, &sb) < 0)
				err(1, "could not fstat %s", *argv);
			filesize = sb.st_size;

			oaddr = addr = mmap(0, (size_t)filesize, PROT_READ,
			    MAP_SHARED, fd, 0);
			if (addr == (void *)-1)
				err(1, "could not mmap %s", *argv);

			if ((hdrlen = audioctl_write_fromhdr(addr,
			    (size_t)filesize, ctlfd, 1)) < 0) {
play_error:
				if (play_errstring)
					errx(1, "%s: %s", play_errstring, *argv);
				else
					errx(1, "unknown audio file: %s", *argv);
			}

			filesize -= hdrlen;
			(char *)addr += hdrlen;

			while (filesize > bufsize) {
				if (write(audiofd, addr, bufsize) != bufsize)
					err(1, "write failed");
				(char *)addr += bufsize;
				filesize -= bufsize;
			}
			if (write(audiofd, addr, (size_t)filesize) != (ssize_t)filesize)
				err(1, "final write failed");

			if (munmap(oaddr, (size_t)filesize) < 0)
				err(1, "munmap failed");

			close(fd);
			
		} while (*++argv);
	} else {
		char	*buffer = malloc(bufsize);
		int	n, m;

		if (buffer == NULL)
			err(1, "malloc of read buffer failed");

		n = read(STDIN_FILENO, buffer, bufsize);

		if (n < 0)
			err(1, "read of standard input failed");
		if (n == 0)
			errx(1, "EOF on standard input");

		hdrlen = audioctl_write_fromhdr(buffer, n, ctlfd, 1);
		if (hdrlen < 0)
			goto play_error;

		/* advance the buffer if we have to */
		if (hdrlen > 0) {
			/* shouldn't happen */
			if (hdrlen > n)
				err(1, "bogus hdrlen %d > length %d?", (int)hdrlen, n);

			memmove(buffer, buffer + hdrlen, n - hdrlen);

			m = read(STDIN_FILENO, buffer + n, hdrlen);
			n += m;
		}
		/* read until EOF or error */
		do {
			if (n == -1)
				err(1, "read of standard input failed");
			if (write(audiofd, buffer, n) != n)
				err(1, "write failed");
		} while ((n = read(STDIN_FILENO, buffer, bufsize)));
	}

	exit(0);
}

/*
 * only support sun and wav audio files so far ...
 *
 * XXX this should probably be mostly part of libaudio, but it
 * uses the local "info" variable. blah... fix me!
 */
ssize_t
audioctl_write_fromhdr(hdr, fsz, fd, unknown_ok)
	void	*hdr;
	size_t	fsz;
	int	fd;
	int	unknown_ok;
{
	sun_audioheader	*sunhdr;
	ssize_t	hdr_len;

	AUDIO_INITINFO(&info);
	sunhdr = hdr;
	if (ntohl(sunhdr->magic) == AUDIO_FILE_MAGIC) {
		if (audio_get_sun_encoding(ntohl(sunhdr->encoding), 
		    &info.play.encoding, &info.play.precision)) {
			warnx("unknown supported Sun audio encoding format %d",
			    sunhdr->encoding);
			return (-1);
		}

		info.play.sample_rate = ntohl(sunhdr->sample_rate);
		info.play.channels = ntohl(sunhdr->channels);
		if (port)
			info.play.port = port;
		if (volume)
			info.play.gain = volume;
		if (balance)
			info.play.balance = balance;
		info.mode = AUMODE_PLAY_ALL;

		if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
			err(1, "failed to set audio info");

		return (ntohl(sunhdr->hdr_size));
	}

	hdr_len = audio_parse_wav_hdr(hdr, fsz, &info.play.encoding,
	    &info.play.precision, &info.play.sample_rate, &info.play.channels);

	switch (hdr_len) {
	case AUDIO_ESHORTHDR:
	case AUDIO_EWAVUNSUPP:
	case AUDIO_EWAVBADPCM:
	case AUDIO_EWAVNODATA:
		if (unknown_ok == 0)
			play_errstring = audio_errstring(hdr_len);
		/* FALL THROUGH */
	case AUDIO_ENOENT:
		break;
	default:
		if (hdr_len < 1)
			break;
		if (port)
			info.play.port = port;
		if (volume)
			info.play.gain = volume;
		if (balance)
			info.play.balance = balance;
		info.mode = AUMODE_PLAY_ALL;

		if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
			err(1, "failed to set audio info");

		return (hdr_len);
	}
	return (unknown_ok ? 0 : -1);
}

void
usage()
{
	extern char *__progname;

	fprintf(stderr, "Usage: %s [-iqVh] [-v vol] [-b bal] [-p port] [-d dev]\n\t[-c ctl] [file ...]\n", __progname);
	exit(0);
}
