/*	$NetBSD: devpubd.c,v 1.2.20.1 2015/02/17 14:45:31 martin Exp $	*/

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2011\
Jared D. McNeill <jmcneill@invisible.ca>. All rights reserved.");
__RCSID("$NetBSD: devpubd.c,v 1.2.20.1 2015/02/17 14:45:31 martin Exp $");

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/drvctlio.h>
#include <sys/wait.h>
#include <sys/poll.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static int drvctl_fd = -1;
static const char devpubd_script[] = DEVPUBD_RUN_HOOKS;

struct devpubd_probe_event {
	char *device;
	TAILQ_ENTRY(devpubd_probe_event) entries;
};

static TAILQ_HEAD(, devpubd_probe_event) devpubd_probe_events;

#define	DEVPUBD_ATTACH_EVENT	"device-attach"
#define	DEVPUBD_DETACH_EVENT	"device-detach"

__dead static void
devpubd_exec(const char *path, char * const *argv)
{
	int error;

	error = execv(path, argv);
	if (error) {
		syslog(LOG_ERR, "couldn't exec '%s': %m", path);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

static void
devpubd_eventhandler(const char *event, const char **device)
{
	char **argv;
	pid_t pid;
	int status;
	size_t i, ndevs;

	for (ndevs = 0, i = 0; device[i] != NULL; i++) {
		++ndevs;
		syslog(LOG_DEBUG, "event = '%s', device = '%s'", event,
		    device[i]);
	}

	argv = calloc(3 + ndevs, sizeof(*argv));
	argv[0] = __UNCONST(devpubd_script);
	argv[1] = __UNCONST(event);
	for (i = 0; i < ndevs; i++) {
		argv[2 + i] = __UNCONST(device[i]);
	}
	argv[2 + i] = NULL;

	pid = fork();
	switch (pid) {
	case -1:
		syslog(LOG_ERR, "fork failed: %m");
		break;
	case 0:
		devpubd_exec(devpubd_script, argv);
		/* NOTREACHED */
	default:
		if (waitpid(pid, &status, 0) == -1) {
			syslog(LOG_ERR, "waitpid(%d) failed: %m", pid);
			break;
		}
		if (WIFEXITED(status) && WEXITSTATUS(status)) {
			syslog(LOG_WARNING, "%s exited with status %d",
			    devpubd_script, WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			syslog(LOG_WARNING, "%s received signal %d",
			    devpubd_script, WTERMSIG(status));
		}
		break;
	}

	free(argv);
}

__dead static void
devpubd_eventloop(void)
{
	const char *event, *device[2];
	prop_dictionary_t ev;
	int res;

	assert(drvctl_fd != -1);

	device[1] = NULL;

	for (;;) {
		res = prop_dictionary_recv_ioctl(drvctl_fd, DRVGETEVENT, &ev);
		if (res)
			err(EXIT_FAILURE, "DRVGETEVENT failed");
		prop_dictionary_get_cstring_nocopy(ev, "event", &event);
		prop_dictionary_get_cstring_nocopy(ev, "device", &device[0]);

		printf("%s: event='%s', device='%s'\n", __func__,
		    event, device[0]);

		devpubd_eventhandler(event, device);

		prop_object_release(ev);
	}
}

static void
devpubd_probe(const char *device)
{
	struct devlistargs laa;
	size_t len, children, n;
	void *p;
	int error;

	assert(drvctl_fd != -1);

	memset(&laa, 0, sizeof(laa));
	if (device)
		strlcpy(laa.l_devname, device, sizeof(laa.l_devname));

	/* Get the child device count for this device */
	error = ioctl(drvctl_fd, DRVLISTDEV, &laa);
	if (error) {
		syslog(LOG_ERR, "DRVLISTDEV failed: %m");
		return;
	}

child_count_changed:
	/* If this device has no children, return */
	if (laa.l_children == 0)
		return;

	/* Allocate a buffer large enough to hold the child device names */
	p = laa.l_childname;
	children = laa.l_children;

	len = children * sizeof(laa.l_childname[0]);
	laa.l_childname = realloc(laa.l_childname, len);
	if (laa.l_childname == NULL) {
		syslog(LOG_ERR, "couldn't allocate %zu bytes", len);
		laa.l_childname = p;
		goto done;
	}

	/* Get a list of child devices */
	error = ioctl(drvctl_fd, DRVLISTDEV, &laa);
	if (error) {
		syslog(LOG_ERR, "DRVLISTDEV failed: %m");
		goto done;
	}

	/* If the child count changed between DRVLISTDEV calls, retry */
	if (children != laa.l_children)
		goto child_count_changed;

	/*
	 * For each child device, queue an attach event and
	 * then scan each one for additional devices.
	 */
	for (n = 0; n < laa.l_children; n++) {
		struct devpubd_probe_event *ev = calloc(1, sizeof(*ev));
		ev->device = strdup(laa.l_childname[n]);
		TAILQ_INSERT_TAIL(&devpubd_probe_events, ev, entries);
	}
	for (n = 0; n < laa.l_children; n++)
		devpubd_probe(laa.l_childname[n]);

done:
	free(laa.l_childname);
	return;
}

static void
devpubd_init(void)
{
	struct devpubd_probe_event *ev;
	const char **devs;
	size_t ndevs, i;

	TAILQ_INIT(&devpubd_probe_events);
	devpubd_probe(NULL);
	ndevs = 0;
	TAILQ_FOREACH(ev, &devpubd_probe_events, entries) {
		++ndevs;
	}
	devs = calloc(ndevs + 1, sizeof(*devs));
	i = 0;
	TAILQ_FOREACH(ev, &devpubd_probe_events, entries) {
		devs[i++] = ev->device;
	}
	devpubd_eventhandler(DEVPUBD_ATTACH_EVENT, devs);
	free(devs);
	while ((ev = TAILQ_FIRST(&devpubd_probe_events)) != NULL) {
		TAILQ_REMOVE(&devpubd_probe_events, ev, entries);
		free(ev->device);
		free(ev);
	}
}

__dead static void
usage(void)
{
	fprintf(stderr, "usage: %s [-f]\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	bool fflag = false;
	int ch;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "fh")) != -1) {
		switch (ch) {
		case 'f':
			fflag = true;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();
		/* NOTREACHED */

	drvctl_fd = open(DRVCTLDEV, O_RDWR);
	if (drvctl_fd == -1)
		err(EXIT_FAILURE, "couldn't open " DRVCTLDEV);

	/* Look for devices that are already present */
	devpubd_init();

	if (!fflag) {
		if (daemon(0, 0) == -1) {
			err(EXIT_FAILURE, "couldn't fork");
		}
	}

	devpubd_eventloop();

	return EXIT_SUCCESS;
}
