/*	$NetBSD: powerd.c,v 1.18 2013/02/09 01:16:39 christos Exp $	*/

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

#define SYSLOG_NAMES

#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/event.h>
#include <sys/power.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>
#include <prop/proplib.h>
#include <stdarg.h>
#include <string.h>

#include "prog_ops.h"

int	debug, no_scripts;

static int kq;

#define	_PATH_POWERD_SCRIPTS	"/etc/powerd/scripts"

static void usage(void) __dead;
static void run_script(const char *[]);
static struct kevent *allocchange(void);
static int wait_for_events(struct kevent *, size_t);
static void dispatch_dev_power(struct kevent *);
static void dispatch_power_event_state_change(int, power_event_t *);
static void powerd_log(int, const char *, ...) __printflike(2, 3);

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

	if (prog_init && prog_init() == -1)
		err(1, "init failed");

	while ((ch = getopt(argc, argv, "dn")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;

		case 'n':
			no_scripts = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	if (debug == 0) {
		(void)daemon(0, 0);

		openlog("powerd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);
		(void)pidfile(NULL);
	}

	if ((kq = prog_kqueue()) == -1) {
		powerd_log(LOG_ERR, "kqueue: %s", strerror(errno));
		exit(EX_OSERR);
	}

	if ((fd = prog_open(_PATH_POWER, O_RDONLY|O_NONBLOCK, 0600)) == -1) {
		powerd_log(LOG_ERR, "open %s: %s", _PATH_POWER,
		    strerror(errno));
		exit(EX_OSERR);
	}

	if (prog_fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
		powerd_log(LOG_ERR, "Cannot set close on exec in power fd: %s",
		    strerror(errno));
		exit(EX_OSERR);
	}

	if (prog_ioctl(fd, POWER_IOC_GET_TYPE, &power_type) == -1) {
		powerd_log(LOG_ERR, "POWER_IOC_GET_TYPE: %s", strerror(errno));
		exit(EX_OSERR);
	}

	(void)asprintf(&cp, "%s/%s", _PATH_POWERD_SCRIPTS,
	    power_type.power_type);
	if (cp == NULL) {
		powerd_log(LOG_ERR, "allocating script path: %s",
		    strerror(errno));
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

	(void)fprintf(stderr, "usage: %s [-dn]\n", getprogname());
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
				(void)fprintf(stderr, "%srunning script: %s",
				    no_scripts?"not ":"", argv[0]);
				for (j = 1; argv[j] != NULL; j++)
					(void)fprintf(stderr, " %s", argv[j]);
				(void)fprintf(stderr, "\n");
			}
			if (no_scripts != 0)
				return;

			switch ((pid = vfork())) {
			case -1:
				powerd_log(LOG_ERR, "fork to run script: %s",
				    strerror(errno));
				return;

			case 0:
				/* Child. */
				(void) execv(path, __UNCONST(argv));
				_exit(1);
				/* NOTREACHED */

			default:
				/* Parent. */
				if (waitpid(pid, &status, 0) == -1) {
					powerd_log(LOG_ERR,
					    "waitpid for %s: %s", path,
					    strerror(errno));
					break;
				}
				if (WIFEXITED(status) &&
				    WEXITSTATUS(status) != 0) {
					powerd_log(LOG_ERR,
					    "%s exited with status %d",
					    path, WEXITSTATUS(status));
				} else if (!WIFEXITED(status)) {
					powerd_log(LOG_ERR,
					    "%s terminated abnormally", path);
				}
				break;
			}
			
			return;
		}
	}

	powerd_log(LOG_ERR, "no script for %s", argv[0]);
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

	while ((rv = prog_kevent(kq, nchanges ? changebuf : NULL, nchanges,
	    events, nevents, NULL)) < 0) {
		nchanges = 0;
		if (errno != EINTR) {
			powerd_log(LOG_ERR, "kevent: %s", strerror(errno));
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
	if (prog_read(fd, &pev, sizeof(pev)) != sizeof(pev)) {
		if (errno == EWOULDBLOCK)
			return;
		powerd_log(LOG_ERR, "read of %s: %s", _PATH_POWER,
		    strerror(errno));
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
		powerd_log(LOG_INFO, "unknown %s event type: %d",
		    _PATH_POWER, pev.pev_type);
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

static void
powerd_log(int pri, const char *msg, ...)
{
	va_list arglist;
	unsigned int i;

	va_start(arglist, msg);
	if (debug == 0)
		vsyslog(pri, msg, arglist);
	else {
		for (i = 0; i < __arraycount(prioritynames); i++)
			if (prioritynames[i].c_val == pri)
				break;
		fprintf(stderr, "%s: ",
		    (prioritynames[i].c_val == -1) ?
			    "UNKNOWN" : prioritynames[i].c_name);
		vfprintf(stderr, msg, arglist);
	}
	va_end(arglist);
}
