/*	$NetBSD: screenblank.c,v 1.10 1999/06/06 03:35:36 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Screensaver daemon for the Sun 3 and SPARC.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1996, 1998 \
	The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: screenblank.c,v 1.10 1999/06/06 03:35:36 thorpej Exp $");
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <paths.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <util.h>

#include <dev/wscons/wsconsio.h>

#ifdef HAVE_FBIO
#include <machine/fbio.h>
#endif

#include "pathnames.h"

u_long	setvideo = WSDISPLAYIO_SVIDEO;		/* "set video" ioctl */
int	videoon  = WSDISPLAYIO_VIDEO_ON;	/* value for "on" */
int	videooff = WSDISPLAYIO_VIDEO_OFF;	/* value for "off" */

struct	dev_stat {
	LIST_ENTRY(dev_stat) ds_link;	/* linked list */
	const char *ds_path;		/* path to device */
	int	ds_isfb;		/* boolean; framebuffer? */
	time_t	ds_atime;		/* time device last accessed */
	time_t	ds_mtime;		/* time device last modified */
};
LIST_HEAD(ds_list, dev_stat) ds_list;

extern	char *__progname;

int	main __P((int, char *[]));
static	void add_dev __P((const char *, int));
static	void change_state __P((int));
static	void cvt_arg __P((char *, struct timeval *));
static	void sighandler __P((int));
static	void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct dev_stat *dsp;
	struct timeval timo_on, timo_off, *tvp;
	struct sigaction sa;
	struct stat st;
	int ch, change, fflag = 0, kflag = 0, mflag = 0, state;
	const char *kbd, *mouse, *display;

	LIST_INIT(&ds_list);

	/*
	 * Set the default timeouts: 10 minutes on, .25 seconds off.
	 */
	timo_on.tv_sec = 600;
	timo_on.tv_usec = 0;
	timo_off.tv_sec = 0;
	timo_off.tv_usec = 250000;

	while ((ch = getopt(argc, argv, "d:e:f:km")) != -1) {
		switch (ch) {
		case 'd':
			cvt_arg(optarg, &timo_on);
			break;

		case 'e':
			cvt_arg(optarg, &timo_off);
			break;

		case 'f':
			fflag = 1;
			add_dev(optarg, 1);
			break;

		case 'k':
			if (mflag || kflag)
				usage();
			kflag = 1;
			break;

		case 'm':
			if (kflag || mflag)
				usage();
			mflag = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	if (argc)
		usage();

	/*
	 * Default to WSCONS support.
	 */
	kbd = _PATH_WSKBD;
	mouse = _PATH_WSMOUSE;
	display = _PATH_WSDISPLAY;

#ifdef HAVE_FBIO
	/*
	 * If a display device wasn't specified, check to see which we
	 * have.  If we can't open the WSCONS display, fall back to fbio.
	 */
	if (!fflag) {
		int fd;

		if ((fd = open(display, O_RDONLY, 0666)) == -1)
			setvideo = FBIOSVIDEO;
		else
			(void) close(fd);
	}

	/*
	 * Do this here so that -f ... args above can influence us.
	 */
	if (setvideo == FBIOSVIDEO) {
		videoon = FBVIDEO_ON;
		videooff = FBVIDEO_OFF;
		kbd = _PATH_KEYBOARD;
		mouse = _PATH_MOUSE;
		display = _PATH_FB;
	}
#endif

	/*
	 * Add the keyboard, mouse, and default framebuffer devices
	 * as necessary.  We _always_ check the console device.
	 */
	add_dev(_PATH_CONSOLE, 0);
	if (!kflag)
		add_dev(kbd, 0);
	if (!mflag)
		add_dev(mouse, 0);
	if (!fflag)
		add_dev(display, 1);

	/* Ensure that the framebuffer is on. */
	state = videoon;
	change_state(state);
	tvp = &timo_on;

	/*
	 * Make sure the framebuffer gets turned back on when we're
	 * killed.
	 */
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_NOCLDSTOP;
	if (sigemptyset(&sa.sa_mask))
		err(1, "sigemptyset");
	if (sigaction(SIGINT, &sa, NULL) || sigaction(SIGTERM, &sa, NULL) ||
	    sigaction(SIGHUP, &sa, NULL))
		err(1, "sigaction");

	/* Detach. */
	if (daemon(0, 0))
		err(1, "daemon");
	pidfile(NULL);

	/* Start the state machine. */
	for (;;) {
		change = 0;
		for (dsp = ds_list.lh_first; dsp != NULL;
		    dsp = dsp->ds_link.le_next) {
			/* Don't check framebuffers. */
			if (dsp->ds_isfb)
				continue;
			if (stat(dsp->ds_path, &st) < 0)
				err(1, "stat: %s", dsp->ds_path);
			if (st.st_atime > dsp->ds_atime) {
				change = 1;
				dsp->ds_atime = st.st_atime;
			}
			if (st.st_mtime > dsp->ds_mtime) {
				change = 1;
				dsp->ds_mtime = st.st_mtime;
			}
		}

		if (state == videoon) {
			if (!change) {
				state = videooff;
				change_state(state);
				tvp = &timo_off;
			}
		} else {
			if (change) {
				state = videoon;
				change_state(state);
				tvp = &timo_on;
			}
		}

		if (select(0, NULL, NULL, NULL, tvp) < 0)
			err(1, "select");
	}
	/* NOTREACHED */
}

static void
add_dev(path, isfb)
	const char *path;
	int isfb;
{
	struct dev_stat *dsp;
	int fd;

	/* Make sure we can open the device. */
	if ((fd = open(path, O_RDWR, 0666)) == -1)
		err(1, "can't open %s", path);

#ifdef HAVE_FBIO
	/*
	 * We default to WSCONS.  If this is a frame buffer
	 * device, check to see if it responds to the old
	 * Sun-style fbio ioctls.  If so, switch to fbio mode.
	 */
	if (isfb && setvideo != FBIOSVIDEO) {
		int onoff;

		if ((ioctl(fd, FBIOGVIDEO, &onoff)) == 0)
			setvideo = FBIOSVIDEO;
	}
#endif

	(void) close(fd);

	/* Create the entry... */
	dsp = malloc(sizeof(struct dev_stat));
	if (dsp == NULL)
		errx(1, "can't allocate memory for %s", path);
	memset(dsp, 0, sizeof(struct dev_stat));
	dsp->ds_path = path;
	dsp->ds_isfb = isfb;

	/* ...and put it in the list. */
	LIST_INSERT_HEAD(&ds_list, dsp, ds_link);
}

/* ARGSUSED */
static void
sighandler(sig)
	int sig;
{

	/* Kill the pid file and re-enable the framebuffer before exit. */
	change_state(videoon);
	exit(0);
}

static void
change_state(state)
	int state;
{
	struct dev_stat *dsp;
	int fd;

	for (dsp = ds_list.lh_first; dsp != NULL; dsp = dsp->ds_link.le_next) {
		/* Don't change the state of non-framebuffers! */
		if (dsp->ds_isfb == 0)
			continue;
		if ((fd = open(dsp->ds_path, O_RDWR, 0)) < 0) {
			warn("open: %s", dsp->ds_path);
			continue;
		}
		if (ioctl(fd, setvideo, &state) < 0)
			warn("ioctl: %s", dsp->ds_path);
		(void)close(fd);
	}
}

static void
cvt_arg(arg, tvp)
	char *arg;
	struct timeval *tvp;
{
	char *cp;
	int seconds, microseconds, factor;
	int period = 0;
	factor = 1000000;
	microseconds = 0;
	seconds = 0;

	for (cp = arg; *cp != '\0'; ++cp) {
		if (*cp == '.') {
			if (period)
				errx(1, "invalid argument: %s", arg);
			period = 1;
			continue;
		}

		if (!isdigit(*cp))
			errx(1, "invalid argument: %s", arg);

		if (period) {
			if (factor > 1) {
				microseconds = microseconds * 10 + (*cp - '0');
				factor /= 10;
			}
		} else
			seconds = (seconds * 10) + (*cp - '0');
	}

	tvp->tv_sec = seconds;
	if (factor > 1)
		microseconds *= factor;
		
	tvp->tv_usec = microseconds;
}

static void
usage()
{

	fprintf(stderr, "usage: %s [-k | -m] [-d timeout] [-e timeout] %s\n",
	    __progname, "[-f framebuffer]");
	exit(1);
}
