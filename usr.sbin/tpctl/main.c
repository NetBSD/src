/*	$NetBSD: main.c,v 1.4.6.1 2009/05/13 19:20:42 jym Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMRUA Shin
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "tpctl.h"

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: main.c,v 1.4.6.1 2009/05/13 19:20:42 jym Exp $");
#endif /* not lint */

void load_data(const char *, struct tpctl_data *);
void save_data(const char *, struct tpctl_data *);
int do_calibration(const char *, struct tp *, struct wsmouse_calibcoords *);
void drawcross(struct fb *, int, int, int, fb_pixel_t);
int check_esc(void *);

int opt_verbose;
int opt_noupdate;
int opt_forceupdate;

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-D dispdev] [-d dev] [-f file] [-hnuv]\n",
	    getprogname());
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

int
main(int argc, char *argv[])
{
	int tpfd, ch;
	struct tp tp;
	struct wsmouse_calibcoords *pref;
	struct tpctl_data data;
	const char *data_file;
	const char *dev_name;
	const char *dispdev_name;

	/* set default values */
	opt_verbose = 0;
	opt_noupdate = 0;
	opt_forceupdate = 0;
	dev_name = TPCTL_TP_DEVICE;
	dispdev_name = TPCTL_FB_DEVICE;
	data_file = TPCTL_DB_FILENAME;

	/* parse command line */
	while ((ch = getopt(argc, argv, "d:D:f:hnuv")) != -1) {
		switch (ch) {
		case 'D':
			dispdev_name = optarg;
			break;
		case 'd':
			dev_name = optarg;
			break;
		case 'f':
			data_file = optarg;
			break;
		case 'h':
			usage();
			/* NOTREACHED */
		case 'n':
			opt_noupdate = 1;
			break;
		case 'u':
			opt_forceupdate = 1;
			break;
		case 'v':
			opt_verbose = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (argv[optind] != NULL) {
		usage();
		/* NOTREACHED */
	}

	/* load calibrarion parameters from specified file */
	load_data(data_file, &data);

	/* open touch panel device and initialize touch panel routines */
	if ((tpfd = open(dev_name, O_RDWR)) < 0)
		errx(EXIT_FAILURE, "can't open touch panel");
	if (tp_init(&tp, tpfd) < 0)
		errx(EXIT_FAILURE, "can't initialize touch panel");

	/* find out saved parameters for the touch panel */
	pref = search_data(&data, tp.id);
	if (opt_forceupdate || pref == NULL) {
		/* if the parameters wasn't found or '-f' options was 
		   specified, do 'calibrarion' */
		struct wsmouse_calibcoords coords;

		/* draw cursors and collect samples */
		if (do_calibration(dispdev_name, &tp, &coords) < 0) {
			/* ESC key was pressed to abort */
			exit(EXIT_FAILURE);
		}
		/* update parameters with new one */
		replace_data(&data, tp.id, &coords);
		pref = search_data(&data, tp.id);
	} else {
		/* nothing is updated, 
		   so you don't have to write back the data */
		opt_noupdate = 1;
	}

	if (opt_verbose)
		write_coords(stdout, tp.id, pref);

	/* set calibration parameters into touch panel device */
	if (tp_setcalibcoords(&tp, pref) < 0)
		errx(EXIT_FAILURE, "can't set samples");

	/* save calibrarion parameters from specified file */
	if (!opt_noupdate)
		save_data(data_file, &data);
	/* dispose data */
	free_data(&data);

	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

/*
 * load calibrarion parameters from specified file
 *
 * return:	none (it won't return if some error occurs)
 */
void
load_data(const char *data_file, struct tpctl_data *data)
{
	int error;

	init_data(data);
	error = read_data(data_file, data);
	switch (error) {
	case ERR_NONE:
		break;
	case ERR_NOFILE:
		fprintf(stderr, "%s: can't open %s\n", getprogname(),
		    data_file);
		/* it might be OK... */
		break;
	case ERR_IO:
		fprintf(stderr, "%s: I/O error on %s\n", getprogname(),
		    data_file);
		exit(EXIT_FAILURE);
		break;
	case ERR_SYNTAX:
		fprintf(stderr, "%s: format error at %s, line %d\n",
		    getprogname(), data_file, data->lineno);
		exit(EXIT_FAILURE);
		break;
	case ERR_DUPNAME:
		fprintf(stderr, "%s: duplicate entry at %s, line %d\n",
		    getprogname(), data_file, data->lineno);
		exit(EXIT_FAILURE);
		break;
	default:
		fprintf(stderr, "%s: internal error\n", getprogname());
		exit(EXIT_FAILURE);
		break;
	}
}

/*
 * save calibrarion parameters to specified file
 *
 * return:	none (it won't return if some error occurs)
 */
void
save_data(const char *data_file, struct tpctl_data *data)
{
	int error;

	error = write_data(data_file, data);
	switch (error) {
	case ERR_NONE:
		break;
	case ERR_NOFILE:
		fprintf(stderr, "%s: can't open %s\n", getprogname(),
		    data_file);
		exit(EXIT_FAILURE);
		break;
	case ERR_IO:
		fprintf(stderr, "%s: I/O error on %s\n", getprogname(),
		    data_file);
		exit(EXIT_FAILURE);
		break;
	default:
		fprintf(stderr, "%s: internal error\n", getprogname());
		exit(EXIT_FAILURE);
		break;
	}
}

/*
 * draw cursors on frame buffer and collect samples in 
 * wamouse_calibcoords structure.
 *
 * return:	0	succeeded
 *		-1	aborted by user (ESC key was pressed)
 *		(it won't return if some error occurs)
 */
int
do_calibration(const char *dev, struct tp *tp,
    struct wsmouse_calibcoords *coords)
{
	int fbfd;
	struct fb fb;
	int i, x, y, xm, ym, cursize, error, res;

	/* open frame buffer device and initialize frame buffer routine */
	if ((fbfd = open(dev, O_RDWR)) < 0)
		errx(EXIT_FAILURE, "can't open frame buffer");
	if (fb_init(&fb, fbfd) < 0)
		errx(EXIT_FAILURE, "can't map frame buffer");

	memset(coords, 0, sizeof(*coords));
	coords->minx = 0;
	coords->miny = 0;
	coords->maxx = fb.conf.hf_width - 1;
	coords->maxy = fb.conf.hf_height - 1;
	coords->samplelen = 5;

	cursize = 20;
	xm = fb.conf.hf_width/10;
	ym = fb.conf.hf_height/10;

	/* center */
	coords->samples[0].x = fb.conf.hf_width/2;
	coords->samples[0].y = fb.conf.hf_height/2;

	/* top left */
	coords->samples[1].x = xm;
	coords->samples[1].y = ym;

	/* bottom left */
	coords->samples[2].x = xm;
	coords->samples[2].y = fb.conf.hf_height - ym;

	/* bottom right */
	coords->samples[3].x = fb.conf.hf_width - xm;
	coords->samples[3].y = fb.conf.hf_height - ym;

	/* top right */
	coords->samples[4].x = fb.conf.hf_width - xm;
	coords->samples[4].y = ym;

	tp_setrawmode(tp);
	error = 0;
	for (i = 0; i < coords->samplelen; i++) {
		drawcross(&fb,
		    coords->samples[i].x,
		    coords->samples[i].y,
		    cursize, fb.white);
		fb_flush(&fb);
		tp_flush(tp);
		res = tp_get(tp, &x, &y, check_esc, 0 /* stdin */);
		if (res < 0) {
			error = errno;
			break;
		}
		if (0 < res) {
			fb_dispmode(&fb, WSDISPLAYIO_MODE_EMUL);
			return (-1); /* aborted by user */
		}
		coords->samples[i].rawx = x;
		coords->samples[i].rawy = y;
		drawcross(&fb,
		    coords->samples[i].x,
		    coords->samples[i].y,
		    cursize, fb.black);
		fb_flush(&fb);
		tp_waitup(tp, 200, check_esc, 0 /* stdin */);
	}

	fb_dispmode(&fb, WSDISPLAYIO_MODE_EMUL);
	close(fbfd);

	if (opt_verbose) {
		printf("%s: %dx%d (%dbytes/line) %dbit offset=0x%lx\n",
		    getprogname(),
		    fb.conf.hf_width,
		    fb.conf.hf_height,
		    fb.conf.hf_bytes_per_line,
		    fb.conf.hf_pixel_width,
		    fb.conf.hf_offset);
	}

	if (error) {
		errno = error;
		errx(EXIT_FAILURE, "can't get samples");
	}

	return (0);
}

/*
 * draw cross cursor on frame buffer
 *
 * return:	none
 */
void
drawcross(struct fb *fb, int x, int y, int size, fb_pixel_t pixel)
{
	size /= 2;

	fb_drawline(fb, x,     y - size + 1, x,     y - 1,    pixel);
	fb_drawline(fb, x + 1, y - size + 1, x + 1, y - 1,    pixel);
	fb_drawline(fb, x,     y + 2,        x,     y + size, pixel);
	fb_drawline(fb, x + 1, y + 2,        x + 1, y + size, pixel);

	fb_drawline(fb, x - size + 1, y,     x - 1,    y,     pixel);
	fb_drawline(fb, x - size + 1, y + 1, x - 1,    y + 1, pixel);
	fb_drawline(fb, x + 2,        y,     x + size, y,     pixel);
	fb_drawline(fb, x + 2,        y + 1, x + size, y + 1, pixel);
}

/*
 * check ESC key
 *
 * date:	input file descriptor
 *
 * return:	0	nothing has occurred
 *		1	ESC key was pressed
 *		-1	error
 */
int
check_esc(void *data)
{
	int fd = (int)data;
	int flg, n, error;
	char buf[1];
	struct termios tm, raw;

	if (tcgetattr(fd, &tm) < 0)
		return (-1);
	raw = tm;
	cfmakeraw(&raw);
	if (tcsetattr(fd, TCSANOW, &raw) < 0)
		return (-1);
	if ((flg = fcntl(fd, F_GETFL)) == -1)
		return (-1);
	if (fcntl(fd, F_SETFL, flg | O_NONBLOCK) == -1)
		return (-1);
	n = read(fd, buf, 1);
	error = errno;
	fcntl(fd, F_SETFL, flg);
	tcsetattr(fd, TCSANOW, &tm);
	if (n < 0)
		return (error == EWOULDBLOCK ? 0 : -1);
	if (n == 0)
		return (0); /* EOF */
	if (*buf == 0x1b)
		return (1); /* ESC */

	return (0);
}
