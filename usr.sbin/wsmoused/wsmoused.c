/* $NetBSD: wsmoused.c,v 1.12 2003/08/06 22:11:50 jmmv Exp $ */

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 2002, 2003\n"
"The NetBSD Foundation, Inc.  All rights reserved.\n");
__RCSID("$NetBSD: wsmoused.c,v 1.12 2003/08/06 22:11:50 jmmv Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/tty.h>
#include <dev/wscons/wsconsio.h>

#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "pathnames.h"
#include "wsmoused.h"

/* --------------------------------------------------------------------- */

/*
 * Global variables.
 */

static struct mouse Mouse;
static char *Pid_File = NULL;
static int X_Console = -1;

#ifdef WSMOUSED_SELECTION_MODE
extern struct mode_bootstrap Action_Mode;
#endif
#ifdef WSMOUSED_SELECTION_MODE
extern struct mode_bootstrap Selection_Mode;
#endif

#define MAX_MODES 2
static struct mode_bootstrap *Modes[MAX_MODES];
static struct mode_bootstrap *Avail_Modes[] = {
#ifdef WSMOUSED_ACTION_MODE
	&Action_Mode,
#endif
#ifdef WSMOUSED_SELECTION_MODE
	&Selection_Mode,
#endif
};

/* --------------------------------------------------------------------- */

/*
 * Prototypes for functions private to this module.
 */

static void usage(void);
static void open_device(unsigned int);
static void init_mouse(void);
static void event_loop(void);
static void generic_wscons_event(struct wscons_event);
static int  attach_mode(const char *);
static void attach_modes(char *);
static void detach_mode(const char *);
static void detach_modes(void);
static void signal_terminate(int);
int main(int, char **);

/* --------------------------------------------------------------------- */

/* Shows program usage information and exits. */
static void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-d device] [-f config_file] [-m modes] [-n]\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

/* --------------------------------------------------------------------- */

/* Initializes mouse information.  Basically, it opens required files
 * for the daemon to work. */
static void
init_mouse(void)
{

	Mouse.m_devfd = -1;
	open_device(0);

	/* Open FIFO, if wanted */
	Mouse.m_fifofd = -1;
	if (Mouse.m_fifoname != NULL) {
		Mouse.m_fifofd = open(Mouse.m_fifoname,
		    O_RDWR | O_NONBLOCK, 0);
		if (Mouse.m_fifofd == -1)
			err(EXIT_FAILURE, "cannot open %s", Mouse.m_fifoname);
	}
}

/* --------------------------------------------------------------------- */

/* Opens the mouse device (if not already opened).  The argument `secs'
 * specifies how much seconds the function will wait before trying to
 * open the device; this is used when returning from the X console. */
static void
open_device(unsigned int secs)
{

	if (Mouse.m_devfd != -1)
		return;

	sleep(secs);

	/* Open mouse file descriptor */
	Mouse.m_devfd = open(Mouse.m_devname, O_RDONLY | O_NONBLOCK, 0);
	if (Mouse.m_devfd == -1)
		err(EXIT_FAILURE, "cannot open %s", Mouse.m_devname);
}

/* --------------------------------------------------------------------- */

/* Main program event loop.  This function polls the wscons status
 * device and the mouse device; whenever an event is received, the
 * appropiate callback is fired for all attached modes.  If the polls
 * times out (which only appens when the mouse is disabled), another
 * callback is launched. */
static void
event_loop(void)
{
	int i, res;
	struct pollfd fds[2];
	struct wscons_event event;

	fds[0].fd = Mouse.m_statfd;
	fds[0].events = POLLIN;

	for (;;) {
		fds[1].fd = Mouse.m_devfd;
		fds[1].events = POLLIN;
		if (Mouse.m_disabled)
			res = poll(fds, 1, INFTIM);
		else
			res = poll(fds, 2, 300);

		if (res < 0)
			warn("failed to read from devices");

		if (fds[0].revents & POLLIN) {
			res = read(Mouse.m_statfd, &event, sizeof(event));
			if (res != sizeof(event))
				warn("failed to read from mouse stat");

			generic_wscons_event(event);

			for (i = 0; i < MAX_MODES && Modes[i] != NULL; i++)
				if (Modes[i]->mb_wscons_event != NULL)
					Modes[i]->mb_wscons_event(event);

		} else if (fds[1].revents & POLLIN) {
			res = read(Mouse.m_devfd, &event, sizeof(event));
			if (res != sizeof(event))
				warn("failed to read from mouse");

			if (Mouse.m_fifofd >= 0) {
				res = write(Mouse.m_fifofd, &event,
				            sizeof(event));
				if (res != sizeof(event))
					warn("failed to write to fifo");
			}

			for (i = 0; i < MAX_MODES && Modes[i] != NULL; i++)
				if (Modes[i]->mb_wsmouse_event != NULL)
					Modes[i]->mb_wsmouse_event(event);
		} else {
			for (i = 0; i < MAX_MODES && Modes[i] != NULL; i++)
				if (Modes[i]->mb_poll_timeout != NULL)
					Modes[i]->mb_poll_timeout();
		}
	}
}

/* --------------------------------------------------------------------- */

/* This function parses generic wscons status events.  Actually, it
 * handles the screen switch event to enable or disable the mouse,
 * depending if we are entering or leaving the X console. */
static void
generic_wscons_event(struct wscons_event evt)
{

	switch (evt.type) {
	case WSCONS_EVENT_SCREEN_SWITCH:
		if (evt.value == X_Console) {
			Mouse.m_disabled = 1;
			(void)close(Mouse.m_devfd);
			Mouse.m_devfd = -1;
		} else {
			if (Mouse.m_disabled) {
				open_device(5);
				Mouse.m_disabled = 0;
			} else {
				(void)close(Mouse.m_devfd);
				Mouse.m_devfd = -1;
				open_device(0);
			}
		}
		break;
	}
}

/* --------------------------------------------------------------------- */

/* Attaches a mode to the list of active modes, based on its name.
 * Returns 1 on success or 0 if the mode fails to initialize or there is
 * any other problem. */
static int
attach_mode(const char *name)
{
	int i, pos;
	struct mode_bootstrap *mb;

	for (i = 0, pos = -1; i < MAX_MODES; i++)
		if (Modes[i] == NULL) {
			pos = i;
			break;
		}
	if (pos == -1) {
		warnx("modes table full; cannot register `%s'", name);
		return 0;
	}

	for (i = 0; i < MAX_MODES; i++) {
		mb = Avail_Modes[i];
		if (mb != NULL && strcmp(name, mb->mb_name) == 0) {
			int res;

			res = mb->mb_startup(&Mouse);
			if (res == 0) {
				warnx("startup failed for `%s' mode",
				    mb->mb_name);
				return 0;
			} else {
				Modes[pos] = mb;
				return 1;
			}
		}
	}

	warnx("unknown mode `%s' (see the `modes' directive)", name);
	return 0;
}

/* --------------------------------------------------------------------- */

/* Attaches all modes given in the whitespace separated string `list'.
 * A fatal error is produced if no active modes can be attached. */
static void
attach_modes(char *list)
{
	char *last, *p;
	int count;

	/* Attach all requested modes */
	(void)memset(&Modes, 0, sizeof(struct mode_bootstrap) * MAX_MODES);
	for (count = 0, (p = strtok_r(list, " ", &last)); p;
	    (p = strtok_r(NULL, " ", &last))) {
		if (attach_mode(p))
			count++;
	}

	if (count == 0)
		errx(EXIT_FAILURE, "no active modes found; exiting...");
}

/* --------------------------------------------------------------------- */

/* Detaches a mode from the active modes list based on its name. */
static void
detach_mode(const char *name)
{
	int i;
	struct mode_bootstrap *mb;

	for (i = 0; i < MAX_MODES; i++) {
		mb = Modes[i];
		if (mb != NULL && strcmp(name, mb->mb_name) == 0) {
			int res;

			res = mb->mb_cleanup();
			if (res == 0) {
				warnx("cleanup failed for `%s' mode",
				    mb->mb_name);
				return;
			} else {
				Modes[i] = NULL;
				return;
			}
		}
	}

	warnx("unknown mode `%s' (see the `modes' directive)", name);
}

/* --------------------------------------------------------------------- */

/* Detaches all active modes. */
static void
detach_modes(void)
{
	int i;

	for (i = 0; i < MAX_MODES && Modes[i] != NULL; i++)
		detach_mode(Modes[i]->mb_name);
}

/* --------------------------------------------------------------------- */

/* Signal handler for close signals.  The program can only be exited
 * through this function. */
/* ARGSUSED */
static void
signal_terminate(int sig)
{

	detach_modes();
	config_free();
	exit(EXIT_SUCCESS);
}

/* --------------------------------------------------------------------- */

/* Main program.  Parses command line options, reads the configuration
 * file, initializes the mouse and associated files and launches the main
 * event loop. */
int
main(int argc, char **argv)
{
	char *conffile, *modelist, *tstat;
	int needconf, nodaemon, opt;
	struct block *conf;

	setprogname(argv[0]);

	(void)memset(&Mouse, 0, sizeof(struct mouse));
	conffile = _PATH_CONF;
	modelist = NULL;
	needconf = 0;
	nodaemon = -1;

	/* Parse command line options */
	while ((opt = getopt(argc, argv, "d:f:m:n")) != -1) {
		switch (opt) {
		case 'd': /* Mouse device name */
			Mouse.m_devname = optarg;
			break;
		case 'f': /* Configuration file name */
			needconf = 1;
			conffile = optarg;
			break;
		case 'm': /* List of modes to activate */
			modelist = optarg;
			break;
		case 'n': /* No daemon */
			nodaemon = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	/* Read the configuration file and get some basic properties */
	config_read(conffile, needconf);
	conf = config_get_mode("Global");
	if (nodaemon == -1)
		nodaemon = block_get_propval_int(conf, "nodaemon", 0);
	X_Console = block_get_propval_int(conf, "xconsole", -1);

	/* Open wsdisplay status device */
	tstat = block_get_propval(conf, "ttystat", _PATH_TTYSTAT);
	Mouse.m_statfd = open(tstat, O_RDONLY | O_NONBLOCK, 0);
	if (Mouse.m_statfd == -1)
		err(EXIT_FAILURE, "cannot open %s", tstat);

	/* Initialize mouse information and attach modes */
	if (Mouse.m_devname == NULL)
		Mouse.m_devname = block_get_propval(conf, "device",
		    _PATH_DEFAULT_MOUSE);
	init_mouse();
	if (modelist != NULL)
		attach_modes(modelist);
	else
		attach_modes(block_get_propval(conf, "modes", "selection"));

	/* Setup signal handlers */
	(void)signal(SIGINT,  signal_terminate);
	(void)signal(SIGKILL, signal_terminate);
	(void)signal(SIGQUIT, signal_terminate);
	(void)signal(SIGTERM, signal_terminate);

	if (!nodaemon) {
		/* Become a daemon */
		if (daemon(0, 0) == -1)
			err(EXIT_FAILURE, "failed to become a daemon");

		/* Create the pidfile, if wanted */
		Pid_File = block_get_propval(conf, "pidfile", NULL);
		if (pidfile(Pid_File) == -1)
			warn("pidfile %s", Pid_File);
	}

	event_loop();

	/* NOTREACHED */
	return EXIT_SUCCESS;
}
