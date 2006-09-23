/*	$NetBSD: screenblank.c,v 1.24 2006/09/23 20:12:15 elad Exp $	*/

/*-
 * Copyright (c) 1996-2002 The NetBSD Foundation, Inc.
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
 * Screensaver daemon for the Sun 3 and SPARC, and platforms using WSCONS.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1996-2002 \
	The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: screenblank.c,v 1.24 2006/09/23 20:12:15 elad Exp $");
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
#include <syslog.h>
#include <unistd.h>
#include <util.h>

#include <dev/wscons/wsconsio.h>

#ifdef HAVE_FBIO
#include <dev/sun/fbio.h>
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

int	main(int, char *[]);
static	void add_dev(const char *, int);
static	void change_state(int);
static	void cvt_arg(char *, struct timespec *);
static	void sighandler(int);
static	int is_graphics_fb(struct dev_stat *);
static	void usage(void);

int
main(int argc, char *argv[])
{
	struct dev_stat *dsp;
	struct timespec timo_on, timo_off, *tvp, tv;
	struct sigaction sa;
	struct stat st;
	int ch, change, fflag = 0, kflag = 0, mflag = 0, state;
	int bflag = 0, uflag = 0;
	const char *kbd, *mouse, *display;

	LIST_INIT(&ds_list);

	/*
	 * Set the default timeouts: 10 minutes on, .25 seconds off.
	 */
	timo_on.tv_sec = 600;
	timo_on.tv_nsec = 0;
	timo_off.tv_sec = 0;
	timo_off.tv_nsec = 250000000;

	while ((ch = getopt(argc, argv, "bd:e:f:i:kmu")) != -1) {
		switch (ch) {
		case 'b':
			bflag = 1;
			uflag = 0;
			break;

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

		case 'i':
			add_dev(optarg, 0);
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

		case 'u':
			uflag = 1;
			bflag = 0;
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
	 * Handle -b and -u modes.
	 */
	if (bflag || uflag) {
		change_state(bflag ? videooff : videoon);
		exit(0);
	}

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

	openlog("screenblank", LOG_PID, LOG_DAEMON);
	/* Detach. */
	if (daemon(0, 0))
		err(1, "daemon");
	pidfile(NULL);

	/* Start the state machine. */
	for (;;) {
		change = 0;
		LIST_FOREACH(dsp, &ds_list, ds_link) {
			/* Don't check framebuffers in graphics mode. */
			if (is_graphics_fb(dsp))
				continue;
			if (stat(dsp->ds_path, &st) == -1) {
				syslog(LOG_CRIT,
				    "Can't stat `%s' (%m)", dsp->ds_path);
				exit(1);
			}
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

		tv = *tvp;
		if (nanosleep(&tv, NULL) == -1)
			err(1, "nanosleep");
	}
	/* NOTREACHED */
}

static void
add_dev(const char *path, int isfb)
{
	struct dev_stat *dsp;
	struct stat sb;

	/* Make sure we can stat the device. */
	if (stat(path, &sb) == -1) {
		warn("Can't stat `%s'", path);
		return;
	}

#ifdef HAVE_FBIO
	/*
	 * We default to WSCONS.  If this is a frame buffer
	 * device, check to see if it responds to the old
	 * Sun-style fbio ioctls.  If so, switch to fbio mode.
	 */
	if (isfb && setvideo != FBIOSVIDEO) {
		int onoff, fd;

		if ((fd = open(path, O_RDWR, 0666)) == -1) {
			warn("Can't open `%s'", path);
			return;
		}
		if ((ioctl(fd, FBIOGVIDEO, &onoff)) == 0)
			setvideo = FBIOSVIDEO;
		(void)close(fd);
	}
#endif

	/* Create the entry... */
	dsp = malloc(sizeof(struct dev_stat));
	if (dsp == NULL)
		err(1, "Can't allocate memory for `%s'", path);
	(void)memset(dsp, 0, sizeof(struct dev_stat));
	dsp->ds_path = path;
	dsp->ds_isfb = isfb;

	/* ...and put it in the list. */
	LIST_INSERT_HEAD(&ds_list, dsp, ds_link);
}

/* ARGSUSED */
static void
sighandler(int sig)
{

	/* Kill the pid file and re-enable the framebuffer before exit. */
	change_state(videoon);
	exit(0);
}

/*
 * Return 1 if we are a framebuffer in graphics mode or a framebuffer
 * where we cannot tell the mode. Return 0 if we are not a framebuffer
 * device, or a wscons framebuffer in text mode.
 */
static int
is_graphics_fb(struct dev_stat *dsp)
{
	int fd;
	int state;

	if (dsp->ds_isfb == 0)
		return 0;

	/* We can't tell if we are not a wscons device */
	if (setvideo != WSDISPLAYIO_SVIDEO)
		return 1;

	if ((fd = open(dsp->ds_path, O_RDWR, 0)) == -1) {
		syslog(LOG_WARNING, "Cannot open `%s' (%m)", dsp->ds_path);
		return 1;
	}

	if (ioctl(fd, WSDISPLAYIO_GMODE, &state) == -1) {
		syslog(LOG_WARNING, "Cannot get mode on `%s' (%m)",
		    dsp->ds_path);
		/* We can't tell, so we say we are mapped */
		state = WSDISPLAYIO_MODE_MAPPED;
	}

	(void)close(fd);

	return state != WSDISPLAYIO_MODE_EMUL;
}

static void
change_state(int state)
{
	struct dev_stat *dsp;
	int fd;
	int fail = 1;

	LIST_FOREACH(dsp, &ds_list, ds_link) {
		/* Don't change the state of non-framebuffers! */
		if (dsp->ds_isfb == 0)
			continue;
		if ((fd = open(dsp->ds_path, O_RDWR, 0)) == -1) {
			syslog(LOG_WARNING, "Can't open `%s' (%m)",
			    dsp->ds_path);
			continue;
		}
		if (ioctl(fd, setvideo, &state) == -1)
			syslog(LOG_WARNING, "Can't set video on `%s' (%m)",
			    dsp->ds_path);
		else
			fail = 0;
		(void)close(fd);
	}
	if (fail) {
		syslog(LOG_CRIT, "No frame buffer devices, exiting\n");
		exit(1);
	}
}

static void
cvt_arg(char *arg, struct timespec *tvp)
{
	char *cp;
	int seconds, nanoseconds, factor;
	int period = 0;
	factor = 1000000000;
	nanoseconds = 0;
	seconds = 0;

	for (cp = arg; *cp != '\0'; ++cp) {
		if (*cp == '.') {
			if (period)
				errx(1, "Invalid argument: %s", arg);
			period = 1;
			continue;
		}

		if (!isdigit((unsigned char)*cp))
			errx(1, "Invalid argument: %s", arg);

		if (period) {
			if (factor > 1) {
				nanoseconds = nanoseconds * 10 + (*cp - '0');
				factor /= 10;
			}
		} else
			seconds = (seconds * 10) + (*cp - '0');
	}

	tvp->tv_sec = seconds;
	if (factor > 1)
		nanoseconds *= factor;

	tvp->tv_nsec = nanoseconds;
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-k | -m] [-d inactivity-timeout] [-e wakeup-delay]\n"
	    "\t\t[-f framebuffer] [-i input-device]\n",
	    getprogname());
	exit(1);
}
