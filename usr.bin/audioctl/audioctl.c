/*	$NetBSD: audioctl.c,v 1.1 1997/05/13 17:35:53 augustss Exp $	*/

/*
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>

FILE *out = stdout;

char *prog;

audio_device_t adev;

audio_info_t info;

char encbuf[1000];

int fullduplex, rerror;

struct field {
	char *name;
	void *valp;
	int format;
#define STRING 1
#define INT 2
#define UINT 3
#define P_R 4
#define ULONG 5
#define UCHAR 6
#define ENC 7
	char readonly;
} fields[] = {
    { "name", 			&adev.name, 		STRING, 1 },
    { "version",		&adev.version,		STRING, 1 },
    { "config",			&adev.config,		STRING, 1 },
    { "encodings",		encbuf,			STRING, 1 },
    { "full_duplex",		&fullduplex,		INT,    0 },
    { "blocksize",		&info.blocksize,	UINT,	0 },
    { "hiwat",			&info.hiwat,		UINT,	0 },
    { "lowat",			&info.lowat,		UINT,	0 },
    { "backlog",		&info.backlog,		UINT,	0 },
    { "mode",			&info.mode,		P_R,	1 },
    { "play.rate",		&info.play.sample_rate,	UINT,	0 },
    { "play.channels",		&info.play.channels,	UINT,	0 },
    { "play.precision",		&info.play.precision,	UINT,	0 },
    { "play.encoding",		&info.play.encoding,	ENC,	0 },
    { "play.gain",		&info.play.gain,	UINT,	0 },
    { "play.port",		&info.play.port,	UINT,	0 },
    { "play.seek",		&info.play.seek,	ULONG,	1 },
    { "play.samples",		&info.play.samples,	UINT,	1 },
    { "play.eof",		&info.play.eof,		UINT,	1 },
    { "play.pause",		&info.play.pause,	UCHAR,	0 },
    { "play.error",		&info.play.error,	UCHAR,	1 },
    { "play.waiting",		&info.play.waiting,	UCHAR,	1 },
    { "play.open",		&info.play.open,	UCHAR,	1 },
    { "play.active",		&info.play.active,	UCHAR,	1 },
    { "record.rate",		&info.record.sample_rate,UINT,	0 },
    { "record.channels",	&info.record.channels,	UINT,	0 },
    { "record.precision",	&info.record.precision,	UINT,	0 },
    { "record.encoding",	&info.record.encoding,	ENC,	0 },
    { "record.gain",		&info.record.gain,	UINT,	0 },
    { "record.port",		&info.record.port,	UINT,	0 },
    { "record.seek",		&info.record.seek,	ULONG,	1 },
    { "record.samples",		&info.record.samples,	UINT,	1 },
    { "record.eof",		&info.record.eof,	UINT,	1 },
    { "record.pause",		&info.record.pause,	UCHAR,	0 },
    { "record.error",		&info.record.error,	UCHAR,	1 },
    { "record.waiting",		&info.record.waiting,	UCHAR,	1 },
    { "record.open",		&info.record.open,	UCHAR,	1 },
    { "record.active",		&info.record.active,	UCHAR,	1 },
    { "record.errors",		&rerror,		INT,	1 },
    { 0 }
};

struct {
	char *ename;
	int eno;
} encs[] = {
	{ "ulaw", AUDIO_ENCODING_ULAW },
	{ "alaw", AUDIO_ENCODING_ALAW },
	{ "linear", AUDIO_ENCODING_LINEAR },
	{ "ulinear", AUDIO_ENCODING_ULINEAR },
	{ "adpcm", AUDIO_ENCODING_ADPCM },
	{ "ADPCM", AUDIO_ENCODING_ADPCM },
	{ "mulaw", AUDIO_ENCODING_ULAW },
	{ "linear_le", AUDIO_ENCODING_LINEAR_LE },
	{ "ulinear_le", AUDIO_ENCODING_ULINEAR_LE },
	{ "linear_be", AUDIO_ENCODING_LINEAR_BE },
	{ "ulinear_be", AUDIO_ENCODING_ULINEAR_BE },
	{ 0 }
};

struct field *
findfield(char *name)
{
	int i;
	for(i = 0; fields[i].name; i++)
		if (strcmp(fields[i].name, name) == 0)
			return &fields[i];
	return 0;
}

void
prfield(struct field *p, char *sep)
{
	u_int v;
	char *cm;
	int i;

	if (sep)
		fprintf(out, "%s%s", p->name, sep);
	switch(p->format) {
	case STRING:
		fprintf(out, "%s", (char*)p->valp);
		break;
	case INT:
		fprintf(out, "%d", *(int*)p->valp);
		break;
	case UINT:
		fprintf(out, "%u", *(u_int*)p->valp);
		break;
	case UCHAR:
		fprintf(out, "%u", *(u_char*)p->valp);
		break;
	case ULONG:
		fprintf(out, "%lu", *(u_long*)p->valp);
		break;
	case P_R:
		v = *(u_int*)p->valp;
		cm = "";
		if (v & AUMODE_PLAY) {
			if (v & AUMODE_PLAY_ALL)
				fprintf(out, "play");
			else
				fprintf(out, "playsync");
			cm = ",";
		}
		if (v & AUMODE_RECORD)
			fprintf(out, "%srecord", cm);
		break;
	case ENC:
		v = *(u_int*)p->valp;
		for(i = 0; encs[i].ename; i++)
			if (encs[i].eno == v)
				break;
		if (encs[i].ename)
			fprintf(out, "%s", encs[i].ename);
		else
			fprintf(out, "%u", v);
		break;
	default:
		errx(1, "Invalid format.");
	}
}

void
rdfield(struct field *p, char *q, char *sep)
{
	u_int v;
	int i;

#if 0
	if (sep)
		prfield(p, ": ");
#else
	if (sep)
		fprintf(out, "%s: ", p->name);
#endif
	switch(p->format) {
	case UINT:
		if (sscanf(q, "%u", p->valp) != 1)
			warnx("Bad number %s", q);
		break;
	case ENC:
		for(i = 0; encs[i].ename; i++)
			if (strcmp(encs[i].ename, q) == 0)
				break;
		if (encs[i].ename)
			*(u_int*)p->valp = encs[i].eno;
		else
			warnx("Unknown encoding: %s", q);
		break;
	default:
		errx(1, "Invalid format.");
	}
	if (sep) {
		fprintf(out, " -> ");
		prfield(p, 0);
		fprintf(out, "\n");
	}
}

void
main(int argc, char **argv)
{
	int fd, r, i, ch, pos;
	int aflag = 0, wflag = 0;
	char *file = "/dev/sound";
	char *sep = "=";

	prog = *argv;

	while ((ch = getopt(argc, argv, "af:nw")) != -1) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'n':
			sep = 0;;
			break;
		case 'f':
			file = optarg;
			 /* XXX this test should be more clever */
			if (strcmp(file, "/dev/stdout") == 0)
				out = stderr;
			break;
		case '?':
		default:
		usage:
		fprintf(out, "%s [-f file] [-n] name ...\n", prog);
		fprintf(out, "%s [-f file] [-n] -w name=value ...\n", prog);
		fprintf(out, "%s [-f file] [-n] -a\n", prog);
		exit(0);
		}
	}
	argc -= optind;
	argv += optind;
    
	fd = open(file, O_RDWR);
	if (fd < 0)
		fd = open(file, O_WRONLY);
	if (fd < 0)
		fd = open(file, O_RDONLY);
	if (fd < 0)
		err(1, "%s", file);

	if (!wflag) {
		if (ioctl(fd, AUDIO_GETDEV, &adev) < 0)
			err(1, NULL);
		for(pos = 0, i = 0;; i++) {
			audio_encoding_t enc;
			enc.index = i;
			if (ioctl(fd, AUDIO_GETENC, &enc) < 0)
				break;
			if (pos)
				encbuf[pos++] = ',';
			sprintf(encbuf+pos, "%s:%d%s", enc.name, 
			        enc.precision, 
				enc.flags & AUDIO_ENCODINGFLAG_EMULATED ? "*" : "");
			pos += strlen(encbuf+pos);
		}
		if (ioctl(fd, AUDIO_GETFD, &fullduplex) < 0)
			err(1, NULL);
		if (ioctl(fd, AUDIO_RERROR, &rerror) < 0)
			err(1, NULL);
		if (ioctl(fd, AUDIO_GETINFO, &info) < 0)
			err(1, NULL);
	}

	if (argc == 0 && aflag && !wflag) {
		for(i = 0; fields[i].name; i++) {
			prfield(&fields[i], sep);
			fprintf(out, "\n");
		}
	} else if (argc > 0 && !aflag) {
		struct field *p;
		if (wflag) {
			AUDIO_INITINFO(&info);
			while(argc--) {
				char *q;

				q = strchr(*argv, '=');
				if (q) {
					*q++ = 0;
					p = findfield(*argv);
					if (p == 0)
						warnx("field %s does not exist", *argv);
					else {
						if (p->readonly)
							warnx("%s is read only", *argv);
						else {
							rdfield(p, q, sep);
							if (p->valp == &fullduplex)
								if (ioctl(fd, AUDIO_SETFD, &fullduplex) < 0)
									err(1, "set failed");
						}
					}
				} else {
					warnx("No `=' in %s", *argv);
				}
				argv++;
			}
			if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
				err(1, "set failed");
		} else {
			while(argc--) {
				p = findfield(*argv);
				if (p == 0)
					warnx("field %s does not exist", *argv);
				else
					prfield(p, sep), fprintf(out, "\n");
				argv++;
			}
		}
	} else
		goto usage;
	exit(0);
}

