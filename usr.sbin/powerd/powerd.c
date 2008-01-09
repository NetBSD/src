/*	$NetBSD: powerd.c,v 1.8.4.2 2008/01/09 02:02:16 matt Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Power management daemon for sysmon.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/event.h>
#include <sys/power.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>
#include <prop/proplib.h>

int	debug;

static int kq;

#define	_PATH_DEV_POWER		"/dev/power"
#define	_PATH_POWERD_SCRIPTS	"/etc/powerd/scripts"

static void usage(void) __dead;
static void run_script(const char *[]);
static struct kevent *allocchange(void);
static int wait_for_events(struct kevent *, size_t);
static void dispatch_dev_power(struct kevent *);
static void dispatch_power_event_state_change(int, power_event_t *);

static const char *script_paths[] = {
	NULL,
	_PATH_POWERD_SCRIPTS
};

int
main(int argc, char *argv[])
{
	struct kevent *ev, events[16];
	struct power_type power_type;
	char *cp;
	int ch, fd;

	setprogname(*argv);

	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	if (debug == 0)
		(void)daemon(0, 0);

	openlog("powerd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);
	(void)pidfile(NULL);

	if ((kq = kqueue()) == -1) {
		syslog(LOG_ERR, "kqueue: %m");
		exit(EX_OSERR);
	}

	if ((fd = open(_PATH_DEV_POWER, O_RDONLY|O_NONBLOCK, 0600)) == -1) {
		syslog(LOG_ERR, "open %s: %m", _PATH_DEV_POWER);
		exit(EX_OSERR);
	}

	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
		syslog(LOG_ERR, "Cannot set close on exec in power fd: %m");
		exit(EX_OSERR);
	}

	if (ioctl(fd, POWER_IOC_GET_TYPE, &power_type) == -1) {
		syslog(LOG_ERR, "POWER_IOC_GET_TYPE: %m");
		exit(EX_OSERR);
	}

	(void)asprintf(&cp, "%s/%s", _PATH_POWERD_SCRIPTS,
	    power_type.power_type);
	if (cp == NULL) {
		syslog(LOG_ERR, "allocating script path: %m");
		exit(EX_OSERR);
	}
	script_paths[0] = cp;

	ev = allocchange();
	EV_SET(ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE,
	    0, 0, (intptr_t) dispatch_dev_power);

	for (;;) {
		void (*handler)(struct kevent *);
		int i, rv;

		rv = wait_for_events(events, __arraycount(events));
		for (i = 0; i < rv; i++) {
			handler = (void *) events[i].udata;
			(*handler)(&events[i]);
		}
	}
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-d]\n", getprogname());
	exit(EX_USAGE);
}

static void
run_script(const char *argv[])
{
	char path[MAXPATHLEN+1];
	size_t i, j;

	for (i = 0; i < __arraycount(script_paths); i++) {
		(void)snprintf(path, sizeof(path), "%s/%s", script_paths[i],
		    argv[0]);
		if (access(path, R_OK|X_OK) == 0) {
			int status;
			pid_t pid;

			argv[0] = path;

			if (debug) {
				(void)fprintf(stderr, "running script: %s",
				    argv[0]);
				for (j = 1; argv[j] != NULL; j++)
					(void)fprintf(stderr, " %s", argv[j]);
				(void)fprintf(stderr, "\n");
			}

			switch ((pid = vfork())) {
			case -1:
				syslog(LOG_ERR, "fork to run script: %m");
				return;

			case 0:
				/* Child. */
				(void) execv(path, __UNCONST(argv));
				_exit(1);
				/* NOTREACHED */

			default:
				/* Parent. */
				if (waitpid(pid, &status, 0) == -1) {
					syslog(LOG_ERR, "waitpid for %s: %m",
					    path);
					break;
				}
				if (WIFEXITED(status) &&
				    WEXITSTATUS(status) != 0) {
					syslog(LOG_ERR,
					    "%s exited with status %d",
					    path, WEXITSTATUS(status));
				} else if (!WIFEXITED(status)) {
					syslog(LOG_ERR,
					    "%s terminated abnormally", path);
				}
				break;
			}
			
			return;
		}
	}

	syslog(LOG_ERR, "no script for %s", argv[0]);
	if (debug)
		(void)fprintf(stderr, "no script for %s\n", argv[0]);
}

static struct kevent changebuf[8];
static size_t nchanges;

static struct kevent *
allocchange(void)
{

	if (nchanges == __arraycount(changebuf)) {
		(void)wait_for_events(NULL, 0);
		nchanges = 0;
	}

	return &changebuf[nchanges++];
}

static int
wait_for_events(struct kevent *events, size_t nevents)
{
	int rv;

	while ((rv = kevent(kq, nchanges ? changebuf : NULL, nchanges,
	    events, nevents, NULL)) < 0) {
		nchanges = 0;
		if (errno != EINTR) {
			syslog(LOG_ERR, "kevent: %m");
			exit(EX_OSERR);
		}
	}

	return rv;
}

static void
dispatch_dev_power(struct kevent *ev)
{
	power_event_t pev;
	int fd = ev->ident;

	if (debug)
		(void)fprintf(stderr, "%s: %" PRId64 
		    " event%s available\n", __func__,
		    ev->data, ev->data > 1 ? "s" : "");

 again:
	if (read(fd, &pev, sizeof(pev)) != sizeof(pev)) {
		if (errno == EWOULDBLOCK)
			return;
		syslog(LOG_ERR, "read of %s: %m", _PATH_DEV_POWER);
		exit(EX_OSERR);
	}

	if (debug)
		(void)fprintf(stderr, "%s: event type %d\n",
		    __func__, pev.pev_type);

	switch (pev.pev_type) {
	case POWER_EVENT_ENVSYS_STATE_CHANGE:
	case POWER_EVENT_SWITCH_STATE_CHANGE:
		dispatch_power_event_state_change(fd, &pev);
		break;
	default:
		syslog(LOG_INFO, "unknown %s event type: %d",
		    _PATH_DEV_POWER, pev.pev_type);
	}

	goto again;
}

static void
dispatch_power_event_state_change(int fd, power_event_t *pev)
{
	prop_dictionary_t dict;
	prop_object_t obj;
	const char *argv[6];
	char *buf = NULL;
	int error;

	error = prop_dictionary_recv_ioctl(fd, POWER_EVENT_RECVDICT, &dict);
	if (error) {
		if (debug)
			printf("%s: prop_dictionary_recv_ioctl error=%d\n",
			    __func__, error);
		return;
	}
	
	if (debug) {
		buf = prop_dictionary_externalize(dict);
		printf("%s", buf);
		free(buf);
	}

	obj = prop_dictionary_get(dict, "powerd-script-name");
	argv[0] = prop_string_cstring_nocopy(obj);

	obj = prop_dictionary_get(dict, "driver-name");
	argv[1] = prop_string_cstring_nocopy(obj);

	obj = prop_dictionary_get(dict, "powerd-event-name");
	argv[2] = prop_string_cstring_nocopy(obj);

	obj = prop_dictionary_get(dict, "sensor-name");
	argv[3] = prop_string_cstring_nocopy(obj);

	obj = prop_dictionary_get(dict, "state-description");
	argv[4] = prop_string_cstring_nocopy(obj);

	argv[5] = NULL;

	run_script(argv);
}
