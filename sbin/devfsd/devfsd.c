/* 	$NetBSD: devfsd.c,v 1.1.2.1 2007/12/08 22:05:04 mjf Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <sys/dev/dctlvar.h>

#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "devfsd.h"
#include "pathnames.h"

#define A_CNT(x)	(sizeof((x)) / sizeof((x)[0]))

extern struct rlist_head rule_list;
extern struct dlist_head dev_list;

static void init(void);
static int update(void);
static bool device_found(device_t);
static bool rule_match(struct devfs_rule *, struct devfs_dev *);
static void usage(void);

/* kevent(2) stuff */
static struct kevent *allocevchange(int);
static int wait_for_events(struct kevent *, size_t, int);
static void read_dctl(struct kevent *);

/* can userland see device nodes? */
static int default_visibility = DEVFS_VISIBLE;

int 
main(int argc, char **argv)
{
	int ch, fd, fkq;
	struct kevent events[16], *ev;

	(void)setlocale(LC_ALL, "");

	while ((ch = getopt(argc, argv, "iv")) != -1) {
		switch(ch) {
		case 'i':
			default_visibility = DEVFS_INVISIBLE;
			break;
		case 'v':
			default_visibility = DEVFS_VISIBLE;
			break;
		case '?':	
		default:
			usage();
		}
	}

	init();

	/* 
	 * Read the config file and construct a list of rules in the global
	 * rule list, 'rule_list'.
	 */
	if (rule_parsefile(_PATH_TEST_CONFIG) != 0)
		err(EXIT_FAILURE, "%s: could not parse\n", _PATH_TEST_CONFIG);

	if ((fd = open(_PATH_DCTL, O_RDWR, 0)) < 0)
		err(EXIT_FAILURE, "%s: could not open", _PATH_DCTL);
	
	if (daemon(0, 0) != 0)
		err(EXIT_FAILURE, "could not demonize");

	if ((fkq = kqueue() < 0))
		err(EXIT_FAILURE, "cannot create event queue");

	ev = allocevchange(fkq);
	EV_SET(ev, fd, EVFILT_READ, EV_ADD, 0, 0, (intptr_t) read_dctl);

	for (;;) {
		void (*handler)(struct kevent *);
		int i, rv;

		rv = wait_for_events(events, A_CNT(events), fkq);
		if (rv == 0)
			continue;

		if (rv < 0) {
			warnx("kqueue failed\n");
			continue;
		}

		for (i = 0; i < rv; i++) {
			handler = (void *) events[i].udata;
			(*handler)(&events[i]);
		}			
	}

	return 0;
}

static void
init(void)
{
	rule_init();
	dev_init();
}

/*
 * The attributes of some devices have changed (mode/permissions) so we
 * must update the corresponding device in the device list and rule
 * in the rules list. Then update the config file to reflect this.
 *
 * XXX: We call this function before going back to wait for kevent()
 */
static int
update(void)
{
	/* TODO: Reconstruct rules for devices into proplib objects */

	if (rule_writefile(_PATH_TEST_CONFIG) != 0) {
		err(1, "%s: could not write to file", _PATH_TEST_CONFIG);
		return -1;
	}

	return 0;
}

/*
 * We've learnt about a new device from /dev/dctl so run it against our
 * list of rules and apply any rules that are found to match. 
 *
 * Returns true if we successfully created and installed the device on the
 * lists, false otherwise.
 */
static bool
device_found(device_t kernel_dev)
{
	struct devfs_dev *dev;
	struct devfs_rule *rule;

	if ((dev = dev_create(kernel_dev, default_visibility)) == NULL)
		return false;

	SLIST_FOREACH(rule, &rule_list, r_next) {
		if (rule_match(rule, dev)) {
			/* got a match: apply rule-specific attrs */
			dev_apply_rule(dev, rule);
		}
	}
	SLIST_INSERT_HEAD(&dev_list, dev, d_next);

	return true;
}

/*
 * This function tries to match a rule against a device. It returns
 * true if the rule matches the device and false otherwise.
 *
 * XXX: Find some way to implement subsystem specific matching functions.
 * 	For example, we should be able to match a network adapter by its
 *	MAC address.
 */
static bool
rule_match(struct devfs_rule *rule, struct devfs_dev *dev)
{
	return false;
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-iv]\n", getprogname());
	exit(1);
}

static struct kevent changebuf[8];
static int nchanges;

static struct kevent *
allocevchange(int fkq)
{

	if (nchanges == A_CNT(changebuf)) {
		/* XXX Error handling could be improved. */
		(void) wait_for_events(NULL, 0, fkq);
	}

	return (&changebuf[nchanges++]);
}

static int
wait_for_events(struct kevent *events, size_t nevents, int fkq)
{
	int rv;

	rv = kevent(fkq, nchanges ? changebuf : NULL, nchanges,
		    events, nevents, NULL);
	nchanges = 0;
	return (rv);
}

static void
read_dctl(struct kevent *ev)
{
}
