/*	$NetBSD: mixerctl.c,v 1.30 2021/12/26 15:36:49 rillig Exp $	*/

/*
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org) and Chuck Cranor.
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
__RCSID("$NetBSD: mixerctl.c,v 1.30 2021/12/26 15:36:49 rillig Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>

#include <paths.h>

static FILE *out = stdout;
static int vflag = 0;

static struct field {
	char *name;
	mixer_ctrl_t *valp;
	mixer_devinfo_t *infp;
	char changed;
} *fields, *rfields;

static mixer_ctrl_t *values;
static mixer_devinfo_t *infos;

static const char mixer_path[] = _PATH_MIXER;

static char *
catstr(char *p, char *q)
{
	char *r;

	asprintf(&r, "%s.%s", p, q);
	if (!r)
		err(EXIT_FAILURE, "malloc");
	return r;
}

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
prfield(struct field *p, const char *sep, int prvalset)
{
	mixer_ctrl_t *m;
	int i, n;

	if (sep)
		fprintf(out, "%s%s", p->name, sep);
	m = p->valp;
	switch(m->type) {
	case AUDIO_MIXER_ENUM:
		for (i = 0; i < p->infp->un.e.num_mem; i++)
			if (p->infp->un.e.member[i].ord == m->un.ord)
				fprintf(out, "%s",
				    p->infp->un.e.member[i].label.name);
		if (prvalset) {
			fprintf(out, "  [ ");
			for (i = 0; i < p->infp->un.e.num_mem; i++)
				fprintf(out, "%s ",
				    p->infp->un.e.member[i].label.name);
			fprintf(out, "]");
		}
		break;
	case AUDIO_MIXER_SET:
		for (n = i = 0; i < p->infp->un.s.num_mem; i++)
			if (m->un.mask & p->infp->un.s.member[i].mask)
				fprintf(out, "%s%s", n++ ? "," : "",
				    p->infp->un.s.member[i].label.name);
		if (prvalset) {
			fprintf(out, "  { ");
			for (i = 0; i < p->infp->un.s.num_mem; i++)
				fprintf(out, "%s ",
				    p->infp->un.s.member[i].label.name);
			fprintf(out, "}");
		}
		break;
	case AUDIO_MIXER_VALUE:
		if (m->un.value.num_channels == 1)
			fprintf(out, "%d", m->un.value.level[0]);
		else
			fprintf(out, "%d,%d", m->un.value.level[0], 
			    m->un.value.level[1]);
		if (prvalset) {
			fprintf(out, " %s", p->infp->un.v.units.name);
			if (p->infp->un.v.delta)
				fprintf(out, " delta=%d", p->infp->un.v.delta);
		}
		break;
	default:
		printf("\n");
		errx(EXIT_FAILURE, "Invalid format %d", m->type);
	}
}

static int
clip(int vol)
{
	if (vol <= AUDIO_MIN_GAIN)
		return AUDIO_MIN_GAIN;
	if (vol >= AUDIO_MAX_GAIN)
		return AUDIO_MAX_GAIN;
	return vol;
}

static int
rdfield(struct field *p, char *q)
{
	mixer_ctrl_t *m;
	int v, v0, v1, mask;
	int i;
	char *s;

	m = p->valp;
	switch(m->type) {
	case AUDIO_MIXER_ENUM:
		for (i = 0; i < p->infp->un.e.num_mem; i++)
			if (strcmp(p->infp->un.e.member[i].label.name, q) == 0)
				break;
		if (i < p->infp->un.e.num_mem)
			m->un.ord = p->infp->un.e.member[i].ord;
		else {
			warnx("Bad enum value %s", q);
			return 0;
		}
		break;
	case AUDIO_MIXER_SET:
		mask = 0;
		for (v = 0; q && *q; q = s) {
			s = strchr(q, ',');
			if (s)
				*s++ = 0;
			for (i = 0; i < p->infp->un.s.num_mem; i++)
				if (strcmp(p->infp->un.s.member[i].label.name,
				    q) == 0)
					break;
			if (i < p->infp->un.s.num_mem) {
				mask |= p->infp->un.s.member[i].mask;
			} else {
				warnx("Bad set value %s", q);
				return 0;
			}
		}
		m->un.mask = mask;
		break;
	case AUDIO_MIXER_VALUE:
		if (m->un.value.num_channels == 1) {
			if (sscanf(q, "%d", &v) == 1) {
				m->un.value.level[0] = clip(v);
			} else {
				warnx("Bad number %s", q);
				return 0;
			}
		} else {
			if (sscanf(q, "%d,%d", &v0, &v1) == 2) {
				m->un.value.level[0] = clip(v0);
				m->un.value.level[1] = clip(v1);
			} else if (sscanf(q, "%d", &v) == 1) {
				m->un.value.level[0] =
				    m->un.value.level[1] = clip(v);
			} else {
				warnx("Bad numbers %s", q);
				return 0;
			}
		}
		break;
	default:
		errx(EXIT_FAILURE, "Invalid format %d", m->type);
	}
	p->changed = 1;
	return 1;
}

static int
incfield(struct field *p, int inc)
{
	mixer_ctrl_t *m;
	int i, v;

	m = p->valp;
	switch(m->type) {
	case AUDIO_MIXER_ENUM:
		m->un.ord += inc;
		if (m->un.ord < 0)
			m->un.ord = p->infp->un.e.num_mem - 1;
		if (m->un.ord >= p->infp->un.e.num_mem)
			m->un.ord = 0;
		break;
	case AUDIO_MIXER_SET:
		m->un.mask += inc;
		if (m->un.mask < 0)
			m->un.mask = (1 << p->infp->un.s.num_mem) - 1;
		if (m->un.mask >= (1 << p->infp->un.s.num_mem))
			m->un.mask = 0;
		warnx("Can't ++/-- %s", p->name);
		return 0;
	case AUDIO_MIXER_VALUE:
		if (p->infp->un.v.delta)
			inc *= p->infp->un.v.delta;
		for (i = 0; i < m->un.value.num_channels; i++) {
			v = m->un.value.level[i];
			v += inc;
			m->un.value.level[i] = clip(v);
		}
		break;
	default:
		errx(EXIT_FAILURE, "Invalid format %d", m->type);
	}
	p->changed = 1;
	return 1;
}

static void
wrarg(int fd, char *arg, const char *sep)
{
	char *q;
	struct field *p;
	mixer_ctrl_t val;
	int incdec, r;

	q = strchr(arg, '=');
	if (q == NULL) {
		int l = strlen(arg);
		incdec = 0;
		if (l > 2 && arg[l-2] == '+' && arg[l-1] == '+')
			incdec = 1;
		else if (l > 2 && arg[l-2] == '-' && arg[l-1] == '-')
			incdec = -1;
		else {
			warnx("No `=' in %s", arg);
			return;
		}
		arg[l-2] = 0;
	} else if (q > arg && (*(q-1) == '+' || *(q-1) == '-')) {
		if (sscanf(q+1, "%d", &incdec) != 1) {
			warnx("Bad number %s", q+1);
			return;
		}
		if (*(q-1) == '-')
			incdec *= -1;
		*(q-1) = 0;
		q = NULL;
	} else
		*q++ = 0;

	p = findfield(arg);
	if (p == NULL) {
		warnx("field %s does not exist", arg);
		return;
	}

	val = *p->valp;
	if (q != NULL)
		r = rdfield(p, q);
	else
		r = incfield(p, incdec);
	if (r) {
		if (ioctl(fd, AUDIO_MIXER_WRITE, p->valp) == -1)
			warn("AUDIO_MIXER_WRITE");
		else if (sep) {
			*p->valp = val;
			prfield(p, ": ", 0);
			if (ioctl(fd, AUDIO_MIXER_READ, p->valp) == -1)
				warn("AUDIO_MIXER_READ");
			printf(" -> ");
			prfield(p, 0, 0);
			printf("\n");
		}
	}
}

static void
prarg(int fd, char *arg, const char *sep)
{
	struct field *p;

	p = findfield(arg);
	if (p == NULL)
		warnx("field %s does not exist", arg);
	else
		prfield(p, sep, vflag), fprintf(out, "\n");
}

static inline void __dead
usage(void)
{
	const char *prog = getprogname();

	fprintf(stderr, "Usage:\t%s [-d file] [-v] [-n] name ...\n", prog);
	fprintf(stderr, "\t%s [-d file] [-v] [-n] -w name=value ...\n", prog);
	fprintf(stderr, "\t%s [-d file] [-v] [-n] -a\n", prog);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int fd, i, j, ch, pos;
	int aflag = 0, wflag = 0;
	const char *file;
	const char *sep = "=";
	mixer_devinfo_t dinfo;
	int ndev;

	file = getenv("MIXERDEVICE");
	if (file == NULL)
		file = mixer_path;


	while ((ch = getopt(argc, argv, "ad:f:nvw")) != -1) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'v':
			vflag++;
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

	if (aflag ? (argc != 0 || wflag) : argc == 0)
		usage();

	fd = open(file, O_RDWR);
	/* Try with mixer0 but only if using the default device. */
	if (fd == -1 && file == mixer_path) {
		file = _PATH_MIXER0;
		fd = open(file, O_RDWR);
	}

	if (fd == -1)
		err(EXIT_FAILURE, "Can't open `%s'", file);

	for (ndev = 0; ; ndev++) {
		dinfo.index = ndev;
		if (ioctl(fd, AUDIO_MIXER_DEVINFO, &dinfo) == -1)
			break;
	}
	rfields = calloc(ndev, sizeof *rfields);
	fields = calloc(ndev, sizeof *fields);
	infos = calloc(ndev, sizeof *infos);
	values = calloc(ndev, sizeof *values);

	for (i = 0; i < ndev; i++) {
		infos[i].index = i;
		if (ioctl(fd, AUDIO_MIXER_DEVINFO, &infos[i]) == -1)
			warn("AUDIO_MIXER_DEVINFO for %d", i);
	}

	for (i = 0; i < ndev; i++) {
		rfields[i].name = infos[i].label.name;
		rfields[i].valp = &values[i];
		rfields[i].infp = &infos[i];
	}

	for (i = 0; i < ndev; i++) {
		values[i].dev = i;
		values[i].type = infos[i].type;
		if (infos[i].type != AUDIO_MIXER_CLASS) {
			values[i].un.value.num_channels = 2;
			if (ioctl(fd, AUDIO_MIXER_READ, &values[i]) == -1) {
				values[i].un.value.num_channels = 1;
				if (ioctl(fd, AUDIO_MIXER_READ, &values[i])
				    == -1)
					err(EXIT_FAILURE, "AUDIO_MIXER_READ");
			}
		}
	}

	for (j = i = 0; i < ndev; i++) {
		if (infos[i].type != AUDIO_MIXER_CLASS &&
		    infos[i].type != -1) {
			fields[j++] = rfields[i];
			for (pos = infos[i].next; pos != AUDIO_MIXER_LAST;
			    pos = infos[pos].next) {
				fields[j] = rfields[pos];
				fields[j].name = catstr(rfields[i].name,
				    infos[pos].label.name);
				infos[pos].type = -1;
				j++;
			}
		}
	}

	for (i = 0; i < j; i++) {
		int cls = fields[i].infp->mixer_class;
		if (cls >= 0 && cls < ndev)
			fields[i].name = catstr(infos[cls].label.name, 
			    fields[i].name);
	}

	if (argc == 0 && aflag && !wflag) {
		for (i = 0; i < j; i++) {
			prfield(&fields[i], sep, vflag);
			fprintf(out, "\n");
		}
	} else if (argc > 0 && !aflag) {
		while (argc--) {
			if (wflag)
				wrarg(fd, *argv, sep);
			else
				prarg(fd, *argv, sep);
			argv++;
		}
	} else
		usage();
	return EXIT_SUCCESS;
}
