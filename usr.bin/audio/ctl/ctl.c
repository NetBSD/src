/*	$NetBSD: ctl.c,v 1.6 1997/07/27 01:28:04 augustss Exp $	*/

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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>

FILE *out = stdout;

char *prog;

audio_device_t adev;

audio_info_t info;

char encbuf[1000];

int properties, fullduplex, rerror;

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
#define PROPS 8
	char flags;
#define READONLY 1
#define ALIAS 2
#define SET 4
} fields[] = {
    { "name", 			&adev.name, 		STRING, READONLY },
    { "version",		&adev.version,		STRING, READONLY },
    { "config",			&adev.config,		STRING, READONLY },
    { "encodings",		encbuf,			STRING, READONLY },
    { "properties",		&properties,		PROPS,	READONLY },
    { "full_duplex",		&fullduplex,		INT,    0 },
    { "buffersize",		&info.buffersize,	UINT,	0 },
    { "blocksize",		&info.blocksize,	UINT,	0 },
    { "hiwat",			&info.hiwat,		UINT,	0 },
    { "lowat",			&info.lowat,		UINT,	0 },
    { "backlog",		&info.backlog,		UINT,	0 },
    { "mode",			&info.mode,		P_R,	READONLY },
    { "play.rate",		&info.play.sample_rate,	UINT,	0 },
    { "play.sample_rate",	&info.play.sample_rate,	UINT,	ALIAS },
    { "play.channels",		&info.play.channels,	UINT,	0 },
    { "play.precision",		&info.play.precision,	UINT,	0 },
    { "play.encoding",		&info.play.encoding,	ENC,	0 },
    { "play.gain",		&info.play.gain,	UINT,	0 },
    { "play.port",		&info.play.port,	UINT,	0 },
    { "play.seek",		&info.play.seek,	ULONG,	READONLY },
    { "play.samples",		&info.play.samples,	UINT,	READONLY },
    { "play.eof",		&info.play.eof,		UINT,	READONLY },
    { "play.pause",		&info.play.pause,	UCHAR,	0 },
    { "play.error",		&info.play.error,	UCHAR,	READONLY },
    { "play.waiting",		&info.play.waiting,	UCHAR,	READONLY },
    { "play.open",		&info.play.open,	UCHAR,	READONLY },
    { "play.active",		&info.play.active,	UCHAR,	READONLY },
    { "record.rate",		&info.record.sample_rate,UINT,	0 },
    { "record.sample_rate",	&info.record.sample_rate,UINT,	ALIAS },
    { "record.channels",	&info.record.channels,	UINT,	0 },
    { "record.precision",	&info.record.precision,	UINT,	0 },
    { "record.encoding",	&info.record.encoding,	ENC,	0 },
    { "record.gain",		&info.record.gain,	UINT,	0 },
    { "record.port",		&info.record.port,	UINT,	0 },
    { "record.seek",		&info.record.seek,	ULONG,	READONLY },
    { "record.samples",		&info.record.samples,	UINT,	READONLY },
    { "record.eof",		&info.record.eof,	UINT,	READONLY },
    { "record.pause",		&info.record.pause,	UCHAR,	0 },
    { "record.error",		&info.record.error,	UCHAR,	READONLY },
    { "record.waiting",		&info.record.waiting,	UCHAR,	READONLY },
    { "record.open",		&info.record.open,	UCHAR,	READONLY },
    { "record.active",		&info.record.active,	UCHAR,	READONLY },
    { "record.errors",		&rerror,		INT,	READONLY },
    { 0 }
};

struct {
	char *ename;
	int eno;
} encs[] = {
    { "ulaw",		AUDIO_ENCODING_ULAW },
    { "mulaw",		AUDIO_ENCODING_ULAW },
    { "alaw", 		AUDIO_ENCODING_ALAW },
    { "slinear",	AUDIO_ENCODING_SLINEAR },
    { "linear",		AUDIO_ENCODING_SLINEAR },
    { "ulinear",	AUDIO_ENCODING_ULINEAR },
    { "adpcm",		AUDIO_ENCODING_ADPCM },
    { "ADPCM",		AUDIO_ENCODING_ADPCM },
    { "slinear_le",	AUDIO_ENCODING_SLINEAR_LE },
    { "linear_le",	AUDIO_ENCODING_SLINEAR_LE },
    { "ulinear_le",	AUDIO_ENCODING_ULINEAR_LE },
    { "slinear_be",	AUDIO_ENCODING_SLINEAR_BE },
    { "linear_be",	AUDIO_ENCODING_SLINEAR_BE },
    { "ulinear_be",	AUDIO_ENCODING_ULINEAR_BE },
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
    case PROPS:
	v = *(u_int*)p->valp;
	cm = "";
	if (v & AUDIO_PROP_FULLDUPLEX) {
		fprintf(out, "%sfull_duplex", cm);
		cm = ",";
	}
	if (v & AUDIO_PROP_MMAP) {
		fprintf(out, "%smmap", cm);
		cm = ",";
	}
	break;
    default:
	errx(1, "Invalid format.");
    }
}

void
rdfield(struct field *p, char *q)
{
    int i;

    switch(p->format) {
    case UINT:
	if (sscanf(q, "%u", (unsigned int *)p->valp) != 1)
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
    p->flags |= SET;
}

void
getinfo(int fd)
{
    int pos, i;

    if (ioctl(fd, AUDIO_GETDEV, &adev) < 0)
	err(1, NULL);
    for(pos = 0, i = 0; ; i++) {
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
    if (ioctl(fd, AUDIO_GETPROPS, &properties) < 0)
	err(1, NULL);
    if (ioctl(fd, AUDIO_RERROR, &rerror) < 0)
	err(1, NULL);
    if (ioctl(fd, AUDIO_GETINFO, &info) < 0)
	err(1, NULL);
}

void
usage(void)
{
    fprintf(out, "%s [-f file] [-n] name ...\n", prog);
    fprintf(out, "%s [-f file] [-n] -w name=value ...\n", prog);
    fprintf(out, "%s [-f file] [-n] -a\n", prog);
    exit(1);
}

void
main(int argc, char **argv)
{
    int fd, i, ch;
    int aflag = 0, wflag = 0;
    struct stat dstat, ostat;
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
	    sep = 0;
	    break;
	case 'f':
	    file = optarg;
	    break;
	case '?':
	default:
	    usage();
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
    
    /* Check is stdout is the same device as the audio device. */
    if (fstat(fd, &dstat) < 0)
	err(1, NULL);
    if (fstat(STDOUT_FILENO, &ostat) < 0)
		err(1, NULL);
    if (S_ISCHR(dstat.st_mode) && S_ISCHR(ostat.st_mode) &&
	major(dstat.st_dev) == major(ostat.st_dev) &&
	minor(dstat.st_dev) == minor(ostat.st_dev))
	/* We can't write to stdout so use stderr */
	out = stderr;

    if (!wflag)
	getinfo(fd);

    if (argc == 0 && aflag && !wflag) {
	for(i = 0; fields[i].name; i++) {
	    if (!(fields[i].flags & ALIAS)) {
		prfield(&fields[i], sep);
		fprintf(out, "\n");
	    }
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
			warnx("field `%s' does not exist", *argv);
		    else {
			if (p->flags & READONLY)
			    warnx("`%s' is read only", *argv);
			else {
			    rdfield(p, q);
			    if (p->valp == &fullduplex)
				if (ioctl(fd, AUDIO_SETFD, &fullduplex) < 0)
				    err(1, "set failed");
			}
		    }
		} else
		    warnx("No `=' in %s", *argv);
		argv++;
	    }
	    if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
		err(1, "set failed");
	    if (sep) {
		getinfo(fd);
		for(i = 0; fields[i].name; i++) {
		    if (fields[i].flags & SET) {
			fprintf(out, "%s: -> ", fields[i].name);
			prfield(&fields[i], 0);
			fprintf(out, "\n");
		    }
		}
	    }
	} else {
	    while(argc--) {
		p = findfield(*argv);
		if (p == 0) {
		    if (strchr(*argv, '='))
			warnx("field %s does not exist (use -w to set a variable)", *argv);
		    else
			warnx("field %s does not exist", *argv);
		} else {
		    prfield(p, sep);
		    fprintf(out, "\n");
		}
		argv++;
	    }
	}
    } else
	usage();
    exit(0);
}
