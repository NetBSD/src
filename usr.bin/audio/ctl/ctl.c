/*	$NetBSD: ctl.c,v 1.44 2019/05/08 14:44:42 isaki Exp $	*/

/*
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: ctl.c,v 1.44 2019/05/08 14:44:42 isaki Exp $");
#endif


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <paths.h>

#include "libaudio.h"

static struct field *findfield(const char *name);
static void prfield(const struct field *p, const char *sep);
static void rdfield(struct field *p, char *q);
static void getinfo(int fd);
static void audioctl_write(int, int, char *[]);
__dead static void usage(void);

static audio_device_t adev;

static audio_info_t info;

static char encbuf[1000];

static int properties, fullduplex, rerror;

int verbose;

static struct field {
	const char *name;
	void *valp;
	int format;
#define STRING 1
#define INT 2
#define UINT 3
#define P_R 4
#define ULONG 5	/* XXX obsolete now */
#define UCHAR 6
#define ENC 7
#define PROPS 8
#define XINT 9
#define	FORMAT 10
	char flags;
#define READONLY 1
#define ALIAS 2
#define SET 4
} fields[] = {
	{ "name", 		&adev.name, 		STRING, READONLY },
	{ "version",		&adev.version,		STRING, READONLY },
	{ "config",		&adev.config,		STRING, READONLY },
	{ "encodings",		encbuf,			STRING, READONLY },
	{ "properties",		&properties,		PROPS,	READONLY },
	{ "full_duplex",	&fullduplex,		UINT,   0 },
	{ "fullduplex",		&fullduplex,		UINT,   0 },
	{ "blocksize",		&info.blocksize,	UINT,	0 },
	{ "hiwat",		&info.hiwat,		UINT,	0 },
	{ "lowat",		&info.lowat,		UINT,	0 },
	{ "monitor_gain",	&info.monitor_gain,	UINT,	0 },
	{ "mode",		&info.mode,		P_R,	READONLY },
	{ "play",		&info.play,		FORMAT,	ALIAS },
	{ "play.rate",		&info.play.sample_rate,	UINT,	0 },
	{ "play.sample_rate",	&info.play.sample_rate,	UINT,	ALIAS },
	{ "play.channels",	&info.play.channels,	UINT,	0 },
	{ "play.precision",	&info.play.precision,	UINT,	0 },
	{ "play.encoding",	&info.play.encoding,	ENC,	0 },
	{ "play.gain",		&info.play.gain,	UINT,	0 },
	{ "play.balance",	&info.play.balance,	UCHAR,	0 },
	{ "play.port",		&info.play.port,	XINT,	0 },
	{ "play.avail_ports",	&info.play.avail_ports,	XINT,	0 },
	{ "play.seek",		&info.play.seek,	UINT,	READONLY },
	{ "play.samples",	&info.play.samples,	UINT,	READONLY },
	{ "play.eof",		&info.play.eof,		UINT,	READONLY },
	{ "play.pause",		&info.play.pause,	UCHAR,	0 },
	{ "play.error",		&info.play.error,	UCHAR,	READONLY },
	{ "play.waiting",	&info.play.waiting,	UCHAR,	READONLY },
	{ "play.open",		&info.play.open,	UCHAR,	READONLY },
	{ "play.active",	&info.play.active,	UCHAR,	READONLY },
	{ "play.buffer_size",	&info.play.buffer_size,	UINT,	0 },
	{ "record",		&info.record,		FORMAT,	ALIAS },
	{ "record.rate",	&info.record.sample_rate,UINT,	0 },
	{ "record.sample_rate",	&info.record.sample_rate,UINT,	ALIAS },
	{ "record.channels",	&info.record.channels,	UINT,	0 },
	{ "record.precision",	&info.record.precision,	UINT,	0 },
	{ "record.encoding",	&info.record.encoding,	ENC,	0 },
	{ "record.gain",	&info.record.gain,	UINT,	0 },
	{ "record.balance",	&info.record.balance,	UCHAR,	0 },
	{ "record.port",	&info.record.port,	XINT,	0 },
	{ "record.avail_ports",	&info.record.avail_ports,XINT,	0 },
	{ "record.seek",	&info.record.seek,	UINT,	READONLY },
	{ "record.samples",	&info.record.samples,	UINT,	READONLY },
	{ "record.eof",		&info.record.eof,	UINT,	READONLY },
	{ "record.pause",	&info.record.pause,	UCHAR,	0 },
	{ "record.error",	&info.record.error,	UCHAR,	READONLY },
	{ "record.waiting",	&info.record.waiting,	UCHAR,	READONLY },
	{ "record.open",	&info.record.open,	UCHAR,	READONLY },
	{ "record.active",	&info.record.active,	UCHAR,	READONLY },
	{ "record.buffer_size",	&info.record.buffer_size,UINT,	0 },
	{ "record.errors",	&rerror,		INT,	READONLY },
	{ .name = NULL },
};

static const struct {
	const char *name;
	u_int prop;
} props[] = {
	{ "full_duplex",	AUDIO_PROP_FULLDUPLEX },
	{ "mmap",		AUDIO_PROP_MMAP },
	{ "independent",	AUDIO_PROP_INDEPENDENT },
	{ .name = NULL },
};

static struct field *
findfield(const char *name)
{
	int i;
	for (i = 0; fields[i].name; i++)
		if (strcmp(fields[i].name, name) == 0)
			return &fields[i];
	return 0;
}

static void
prfield(const struct field *p, const char *sep)
{
	u_int v;
	const char *cm, *encstr;
	int i;

	if (sep)
		printf("%s%s", p->name, sep);
	switch(p->format) {
	case STRING:
		printf("%s", (const char*)p->valp);
		break;
	case INT:
		printf("%d", *(const int*)p->valp);
		break;
	case UINT:
		printf("%u", *(const u_int*)p->valp);
		break;
	case XINT:
		printf("0x%x", *(const u_int*)p->valp);
		break;
	case UCHAR:
		printf("%u", *(const u_char*)p->valp);
		break;
	case ULONG:
		printf("%lu", *(const u_long*)p->valp);
		break;
	case P_R:
		v = *(const u_int*)p->valp;
		cm = "";
		if (v & AUMODE_PLAY) {
			if (v & AUMODE_PLAY_ALL)
				printf("play");
			else
				printf("playsync");
			cm = ",";
		}
		if (v & AUMODE_RECORD)
			printf("%srecord", cm);
		break;
	case ENC:
		v = *(u_int*)p->valp;
		encstr = audio_enc_from_val(v);
		if (encstr)
			printf("%s", encstr);
		else
			printf("%u", v);
		break;
	case PROPS:
		v = *(u_int*)p->valp;
		for (cm = "", i = 0; props[i].name; i++) {
			if (v & props[i].prop) {
				printf("%s%s", cm, props[i].name);
				cm = ",";
			}
		}
		break;
	case FORMAT:
		prfield(p + 1, 0);
		printf(",");
		prfield(p + 3, 0);
		printf(",");
		prfield(p + 4, 0);
		printf(",");
		prfield(p + 5, 0);
		break;
	default:
		errx(1, "Invalid print format.");
	}
}

static void
rdfield(struct field *p, char *q)
{
	int enc;
	u_int u;
	char *s;

	switch(p->format) {
	case UINT:
		if (sscanf(q, "%u", (unsigned int *)p->valp) != 1)
			errx(1, "Bad number: %s", q);
		break;
	case UCHAR:
		if (sscanf(q, "%u", &u) != 1)
			errx(1, "Bad number: %s", q);
		else
			*(u_char *)p->valp = u;
		break;
	case XINT:
		if (sscanf(q, "0x%x", (unsigned int *)p->valp) != 1 &&
		    sscanf(q, "%x", (unsigned int *)p->valp) != 1)
			errx(1, "Bad number: %s", q);
		break;
	case ENC:
		enc = audio_enc_to_val(q);
		if (enc >= 0)
			*(u_int*)p->valp = enc;
		else
			errx(1, "Unknown encoding: %s", q);
		break;
	case FORMAT:
		s = strsep(&q, ",");
		if (s)
			rdfield(p + 1, s);
		s = strsep(&q, ",");
		if (s)
			rdfield(p + 3, s);
		s = strsep(&q, ",");
		if (s)
			rdfield(p + 4, s);
		s = strsep(&q, ",");
		if (s)
			rdfield(p + 5, s);
		if (!s || q)
			errx(1, "Bad format");
		break;
	default:
		errx(1, "Invalid read format.");
	}
	p->flags |= SET;
}

static void
getinfo(int fd)
{
	int pos, i;

	if (ioctl(fd, AUDIO_GETDEV, &adev) < 0)
		err(1, "AUDIO_GETDEV");
	for (pos = 0, i = 0; ; i++) {
		audio_encoding_t enc;
		enc.index = i;
		if (ioctl(fd, AUDIO_GETENC, &enc) < 0)
			break;
		if (pos >= (int)sizeof(encbuf)-1)
			break;
		if (pos)
			encbuf[pos++] = ',';
		if (pos >= (int)sizeof(encbuf)-1)
			break;
		pos += snprintf(encbuf+pos, sizeof(encbuf)-pos, "%s:%d%s",
			enc.name, enc.precision,
			enc.flags & AUDIO_ENCODINGFLAG_EMULATED ? "*" : "");
	}
	if (ioctl(fd, AUDIO_GETFD, &fullduplex) < 0)
		err(1, "AUDIO_GETFD");
	if (ioctl(fd, AUDIO_GETPROPS, &properties) < 0)
		err(1, "AUDIO_GETPROPS");
	if (ioctl(fd, AUDIO_RERROR, &rerror) < 0)
		err(1, "AUDIO_RERROR");
	if (ioctl(fd, AUDIO_GETINFO, &info) < 0)
		err(1, "AUDIO_GETINFO");
}

static void
usage(void)
{
	const char *prog = getprogname();

	fprintf(stderr, "Usage: %s [-d file] [-n] name ...\n", prog);
	fprintf(stderr, "Usage: %s [-d file] [-n] -w name=value ...\n", prog);
	fprintf(stderr, "Usage: %s [-d file] [-n] -a\n", prog);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int fd, i, ch;
	int aflag = 0, wflag = 0;
	const char *deffile = _PATH_AUDIOCTL;
	const char *file;
	const char *sep = "=";

	file = getenv("AUDIOCTLDEVICE");
	if (file == NULL)
		file = deffile;

	while ((ch = getopt(argc, argv, "ad:f:nw")) != -1) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'n':
			sep = 0;
			break;
		case 'f': /* compatibility */
		case 'd':
			file = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	fd = open(file, O_WRONLY);
	if (fd < 0)
		fd = open(file, O_RDONLY);
        if (fd < 0 && file == deffile) {
        	file = _PATH_AUDIOCTL0;
                fd = open(file, O_WRONLY);
		if (fd < 0)
			fd = open(file, O_RDONLY);
        }

	if (fd < 0)
		err(1, "%s", file);

	if (!wflag)
		getinfo(fd);

	if (argc == 0 && aflag && !wflag) {
		for (i = 0; fields[i].name; i++) {
			if (!(fields[i].flags & ALIAS)) {
				prfield(&fields[i], sep);
				printf("\n");
			}
		}
	} else if (argc > 0 && !aflag) {
		if (wflag) {
			audioctl_write(fd, argc, argv);
			if (sep) {
				getinfo(fd);
				for (i = 0; fields[i].name; i++) {
					if (fields[i].flags & SET) {
						printf("%s: -> ",
						    fields[i].name);
						prfield(&fields[i], 0);
						printf("\n");
					}
				}
			}
		} else {
			struct field *p;
			while (argc--) {
				p = findfield(*argv);
				if (p == 0) {
					if (strchr(*argv, '='))
						warnx("field %s does not exist "
						      "(use -w to set a "
						      "variable)", *argv);
					else
						warnx("field %s does not exist",
						      *argv);
				} else {
					prfield(p, sep);
					printf("\n");
				}
				argv++;
			}
		}
	} else
		usage();
	exit(0);
}

static void
audioctl_write(int fd, int argc, char *argv[])
{
	struct field *p;

	AUDIO_INITINFO(&info);
	while (argc--) {
		char *q;

		q = strchr(*argv, '=');
		if (q) {
			*q++ = 0;
			p = findfield(*argv);
			if (p == 0)
				warnx("field `%s' does not exist", *argv);
			else {
				if (p->flags & READONLY)
					warnx("`%s' is read only", *argv);
				else {
					rdfield(p, q);
					if (p->valp == &fullduplex)
						if (ioctl(fd, AUDIO_SETFD,
							   &fullduplex) < 0)
							err(1, "set failed");
				}
			}
		} else
			warnx("No `=' in %s", *argv);
		argv++;
	}
	if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
		err(1, "set failed");
}
