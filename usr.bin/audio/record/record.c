/*	$NetBSD: record.c,v 1.5 1999/06/19 05:20:17 itohy Exp $	*/

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

/*
 * SunOS compatible audiorecord(1)
 */

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

#include "libaudio.h"

audio_info_t info, oinfo;
ssize_t	total_size = -1;
char	*device;
char	*ctldev;
char	*header_info;
char	default_info[8] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
int	audiofd, ctlfd, outfd;
int	qflag, aflag, fflag;
int	verbose;
int	monvol, omonvol;
int	volume;
int	balance;
int	port;
int	encoding;
char	*encoding_str;
int	precision;
int	sample_rate;
int	channels;
struct timeval record_time;
struct timeval start_time;	/* XXX because that's what gettimeofday returns */

void usage __P((void));
int main __P((int, char *[]));
int timeleft __P((struct timeval *, struct timeval *));
void cleanup __P((int)) __attribute__((__noreturn__));
void write_header __P((void));
void rewrite_header __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char	*buffer;
	size_t	len, bufsize;
	int	ch;

	while ((ch = getopt(argc, argv, "ab:C:c:d:e:fhi:m:P:p:qt:s:Vv:")) != -1) {
		switch (ch) {
		case 'a':
			aflag++;
			break;
		case 'b':
			decode_int(optarg, &balance);
			if (balance < 0 || balance > 63)
				errx(1, "balance must be between 0 and 63\n");
			break;
		case 'C':
			ctldev = optarg;
			break;
		case 'c':
			decode_int(optarg, &channels);
			if (channels < 0 || channels > 16)
				errx(1, "channels must be between 0 and 16\n");
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
			decode_int(optarg, &monvol);
			if (monvol < 0 || monvol > 255)
				errx(1, "monitor volume must be between 0 and 255\n");
			break;
		case 'P':
			decode_int(optarg, &precision);
			if (precision != 8 && precision != 16 &&
			    precision != 24 && precision != 32)
				errx(1, "precision must be between 8, 16, 24 or 32");
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
				errx(1, "sample rate must be between 0 and 96000\n");
			break;
		case 't':
			decode_time(optarg, &record_time);
			break;
		case 'V':
			verbose++;
			break;
		case 'v':
			decode_int(optarg, &volume);
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

	/*
	 * open the audio device, and control device
	 */
	if (device == NULL)
		device = _PATH_AUDIO;
	if (ctldev == NULL)
		ctldev = _PATH_AUDIOCTL;

	audiofd = open(device, O_RDONLY);
	if (audiofd < 0)
		err(1, "failed to open %s", device);
	ctlfd = open(ctldev, O_RDWR);
	if (ctlfd < 0)
		err(1, "failed to open %s", ctldev);

	/*
	 * work out the buffer size to use, and allocate it.  also work out
	 * what the old monitor gain value is, so that we can reset it later.
	 */
	if (ioctl(ctlfd, AUDIO_GETINFO, &oinfo) < 0)
		err(1, "failed to get audio info");
	bufsize = oinfo.record.buffer_size;
	if (bufsize < 32 * 1024)
		bufsize = 32 * 1024;
	omonvol = oinfo.monitor_gain;

	buffer = malloc(bufsize);
	if (buffer == NULL)
		err(1, "couldn't malloc buffer of %d size", (int)bufsize);

	/*
	 * open the output file
	 */
	if (argc != 1)
		usage();
	if (argv[0][0] != '-' && argv[0][1] != '\0') {
		outfd = open(*argv, O_CREAT|(aflag ? O_APPEND : O_TRUNC)|O_WRONLY, 0666);
		if (outfd < 0)
			err(1, "could not open %s", *argv);
	} else
		outfd = STDOUT_FILENO;

	/*
	 * set up audio device for recording with the speified parameters
	 */
	AUDIO_INITINFO(&info);

	/*
	 * for these, get the current values for stuffing into the header
	 **/
	if (sample_rate)
		info.record.sample_rate = sample_rate;
	else
		sample_rate = oinfo.record.sample_rate;
	if (channels)
		info.record.channels = channels;
	else
		channels = oinfo.record.channels;

	if (encoding_str) {
		encoding = audio_enc_to_val(encoding_str);
		if (encoding == -1)
			errx(1, "unknown encoding, bailing...");
	}

	if (precision)
		info.record.precision = precision;
	if (encoding)
		info.record.encoding = encoding;
	if (volume)
		info.record.gain = volume;
	if (port)
		info.record.port = port;
	if (balance)
		info.record.balance = (u_char)balance;
	if (monvol)
		info.monitor_gain = monvol;

	info.mode = AUMODE_RECORD;
	if (ioctl(ctlfd, AUDIO_SETINFO, &info) < 0)
		err(1, "failed to reset audio info");

	signal(SIGINT, cleanup);
	write_header();
	total_size = 0;

	(void)gettimeofday(&start_time, NULL);
	while (timeleft(&start_time, &record_time)) {
		if (read(audiofd, buffer, bufsize) != bufsize)
			err(1, "read failed");
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
	timersub(&diff, &now, start_tvp);
	timersub(&now, record_tvp, &diff);

	return (now.tv_sec > 0 || (now.tv_sec == 0 && now.tv_usec > 0));
}

void
cleanup(signo)
	int signo;
{

	close(audiofd);
	rewrite_header();
	close(outfd);
	if (omonvol) {
		AUDIO_INITINFO(&info);
		info.monitor_gain = omonvol;
		if (ioctl(ctlfd, AUDIO_SETINFO, &info) < 0)
			err(1, "failed to reset audio info");
	}
	close(ctlfd);
	exit(0);
}

sun_audioheader auh;

void
write_header()
{
	struct iovec iv[3];
	int veclen = 0, left, tlen = 0;

	auh.magic = htonl(AUDIO_FILE_MAGIC);
	if (outfd == STDOUT_FILENO)
		auh.data_size = htonl(AUDIO_UNKNOWN_SIZE);
	else
		auh.data_size = htonl(total_size);
	auh.encoding = htonl(encoding);
	auh.sample_rate = htonl(sample_rate);
	auh.channels = htonl(channels);
	if (header_info) {
		int 	len, infolen;

		infolen = ((len = strlen(header_info)) + 7) & 0xfffffff8;
		left = infolen - len;
		auh.hdr_size = htonl(sizeof(auh) + infolen);
	} else {
		left = sizeof(default_info);
		auh.hdr_size = htonl(sizeof(auh) + left);
	}

	iv[veclen].iov_base = &auh;
	iv[veclen].iov_len = sizeof(auh);
	tlen = iv[veclen++].iov_len;
	if (header_info) {
		iv[veclen].iov_base = header_info;
		iv[veclen].iov_len = (int)strlen(header_info);
		tlen += iv[veclen++].iov_len;
	}
	if (left) {
		iv[veclen].iov_base = default_info;
		iv[veclen].iov_len = left;
		tlen += iv[veclen++].iov_len;
	}

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
	extern char *__progname;

	fprintf(stderr, "Usage: %s [-afhqV] [options] {files ...|-}\n", __progname);
	fprintf(stderr, "Options:\n\t"
	    "-C audio control device\n\t"
	    "-b balance (0-63)\n\t"
	    "-c channels\n\t"
	    "-d audio device\n\t"
	    "-e encoding\n\t"
	    "-i header information\n\t"
	    "-m monitor volume\n\t"
	    "-P precision bits (8, 16, 24 or 32)\n\t"
	    "-p input port\n\t"
	    "-s sample rate\n\t"
	    "-t recording time\n\t"
	    "-v volume\n");
	exit(0);
}
