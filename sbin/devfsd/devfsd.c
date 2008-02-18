/* 	$NetBSD: devfsd.c,v 1.1.2.2 2008/02/18 22:07:02 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

#include <dev/dctl/dctlio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "devfsd.h"
#include "pathnames.h"

extern struct rlist_head rule_list;
extern struct dlist_head dev_list;

static int init(void);
static int update(struct devfs_dev *, struct dctl_msg *);
static void device_found(struct devfs_dev *);
static void device_create_nodes(struct devfs_dev *);
static void push_nodes(struct devfs_dev *, struct devfs_mount *);
static void mount_found(struct devfs_mount *);
static void mount_disappear(struct devfs_mount *);
static bool rule_match(struct devfs_rule *, struct devfs_dev *);
static void usage(void);

/* can userland see device nodes? */
static int default_visibility = DEVFS_VISIBLE;

/* kqueue stuff */
static struct kevent *allocevchange(void);
static int wait_for_events(struct kevent *, size_t);
static void dispatch_read_dctl(struct kevent *);
static int fkq;
static int dctl_fd;

#define A_CNT(x)        (sizeof((x)) / sizeof((x)[0]))

int 
main(int argc, char **argv)
{
	int ch;
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
	if (rule_parsefile(_PATH_CONFIG) != 0)
		err(EXIT_FAILURE, "%s: could not parse\n", _PATH_CONFIG);

	if ((dctl_fd = open(_PATH_DCTL, O_RDONLY, 0)) < 0)
		err(EXIT_FAILURE, "%s: could not open", _PATH_DCTL);
	
	/* Don't close stdin/stdout/stderr when we goto bg 
	if (daemon(0, 1) != 0)
		err(EXIT_FAILURE, "could not demonize");
	*/

	if ((fkq = kqueue()) < 0)
		err(EXIT_FAILURE, "cannot create event queue");

	ev = allocevchange();
	EV_SET(ev, dctl_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
	    (intptr_t) dispatch_read_dctl);

	/* Handle events from the device driver */
	for (;;) {
		void (*handler)(struct kevent *);
		int i, rv;
		rv = wait_for_events(events, A_CNT(events));
		if (rv == 0)
			continue;
		if (rv < 0) {
			warnx("kevent() failed");
			continue;
		}
		for (i = 0; i < rv; i++) {
			handler = (void *) events[i].udata;
			(*handler)(&events[i]);
		}
	}
	return 0;
}

/*      
 * Dispatch routine for reading /dev/dctl
 */
static void
dispatch_read_dctl(struct kevent *ev)
{
	ssize_t rv;
	int fd = ev->ident;
	struct dctl_msg msg;
	struct devfs_dev *de;
	struct devfs_mount *dmp;

	memset(&msg, 0, sizeof(msg));

	rv = ioctl(fd, DCTL_IOC_NEXTEVENT, &msg);
	if (rv >= 0) {
		switch(msg.d_type) {
			case DCTL_NEW_DEVICE:
				printf("dctl: new device %s\n", 
				    msg.d_kdev.k_name);
				de = dev_create(&msg.d_kdev, msg.d_dc);
				if (de == NULL) {
					warnx("could not internalize device\n");
					return;
				}

				/*
				 * We need to create special files for this
				 * device on all devfs file systems (whether
				 * they are visible or not in a file system is
				 * not our concern, they still need to be
				 * attached to it).
				 */
				device_create_nodes(de);
				
				/* 
				 * Run rules against our device and it's
				 * nodes.
				 */
				device_found(de);

				/*
				 * Push the new nodes into their respective
				 * file systems at once!
				 */
				push_nodes(de, NULL);
				break;
			case DCTL_UPDATE_NODE_ATTR:
				printf("dctl: updated device attrs\n");
				de = dev_lookup(msg.d_sn);
				if (de == NULL) {
					warnx("could not find device to update\n");
					return;
				}
				if (update(de, &msg) == -1)
					warnx("your changes will be lost on reboot\n");
				break;

			case DCTL_UPDATE_NODE_NAME:
				printf("dctl: filename for node updated\n");
				break;

			case DCTL_NEW_MOUNT:
				printf("dctl: new devfs mount: %s\n",
				    msg.d_mp.m_pathname);

				dmp = mount_create(msg.d_mc,
				    msg.d_mp.m_pathname, msg.d_mp.m_visibility);

				if (dmp == NULL) {
					warnx("failed to internalize mount\n");
					return;
				}

				mount_found(dmp);
				break;
			case DCTL_UNMOUNT:
				dmp = mount_lookup(msg.d_mc);

				if (dmp == NULL) {
					warnx("unknown mount removed\n");
					return;
				}

				printf("dctl: devfs mount gone: %s\n",
				    dmp->m_pathname);

				mount_disappear(dmp);
				mount_destroy(dmp);

				break;
			case DCTL_REMOVED_DEVICE:
				printf("dctl: device removed: %s\n",
				    msg.d_kdev.k_name);
				break;
		}
	} else if (rv < 0 && errno != EINTR) {
		/*      
		* /dev/dctl has croaked.  Disable the event
		* so it won't bother us again.
		*/
		struct kevent *cev = allocevchange();
		EV_SET(cev, fd, EVFILT_READ, EV_DISABLE,
		0, 0, (intptr_t) dispatch_read_dctl);
	}
}

/*      
 * Dispatch routine for writing /dev/dctl
 *
 * If 'dmp' is NULL we push these nodes in to _ALL_ device file systems.
 */
static void
push_nodes(struct devfs_dev *dev, struct devfs_mount *dmp)
{
	int rv;
	struct dctl_msg msg;
	struct devfs_node *node;

	SLIST_FOREACH(node, &dev->d_node_head, n_next) {

		if (dmp != NULL) {
			/* Ensure we push nodes into the correct fs */
			if (node->n_cookie.sc_mount != dmp->m_id)
				continue;
		}

		memset(&msg, 0, sizeof(msg));

		msg.d_sn = node->n_cookie;
		msg.d_attr = node->n_attr;
		printf("creating node: %s\n", 
	    	    msg.d_attr.d_component.in_pthcmp.d_filename);

		rv = ioctl(dctl_fd, DCTL_IOC_CREATENODE, &msg);

		if (rv < 0)
			warn("could not push node into devfs");
	}
}

static int
init(void)
{
	int error = 0;

	rule_init();
	dev_init();

	return error;
}

/*
 * The attributes of some devices have changed (mode/permissions) so we
 * must update the corresponding device in the device list and rule
 * in the rules list. Then update the config file to reflect this.
 *
 * NOTE: We handle filename updates, i.e. mv(1), differently from other
 * attribute updates, i.e. chmod(1). This is because in the future we
 * may do radically different things for either case. Right now they're
 * pretty much the same, though.
 */
static int
update(struct devfs_dev *dev, struct dctl_msg *msg)
{
	struct devfs_node *node;

	/* Figure out which node for this device is being updated */
	SLIST_FOREACH(node, &dev->d_node_head, n_next) {
		if (NODE_COOKIE_MATCH(node->n_cookie, msg->d_sn))
			break;
	}

	if (node == NULL) {
		warnx("node not on device node list\n");
		return -1;
	}

	if (msg->d_type != DCTL_UPDATE_NODE_ATTR &&
	    msg->d_type != DCTL_UPDATE_NODE_NAME) {
		warnx("%s: node not being updated\n", 
		    node->n_attr.d_component.out_pthcmp.d_pthcmp);
		return -1;
	}

	/*
	 * XXX: The simplest way I can think of doing this is just by
	 * letting the current attributes overwrite whatever we have for
	 * this entry in dev_list, because _we_ specified the attributes
	 * originally and anything that has changed needs to persist. 
	 * It also seems sensible to rebuild the rules based on the
	 * new values (old rules that differ will therefore be discarded),
	 * this way we don't have a two rules in the config file that 
	 * contradict each other.
	 */
	node->n_attr = msg->d_attr;

	/* construct new rules for dev->d_dev */
	rule_rebuild_attr(dev);

	if (rule_writefile(_PATH_CONFIG) != 0) {
		warnx("%s: could not write to file", _PATH_CONFIG);
		return -1;
	}

	return 0;
}

/*
 * We've learnt about a new device from /dev/dctl so run it against our
 * list of rules and apply any rules that are found to match. 
 */
static void
device_found(struct devfs_dev *dev)
{
	struct devfs_rule *rule;

	SLIST_FOREACH(rule, &rule_list, r_next) {
		if (rule_match(rule, dev)) {
			/* got a match: apply rule-specific attrs */
			dev_apply_rule(dev, rule);
		}
	}
	SLIST_INSERT_HEAD(&dev_list, dev, d_next);
}

static void
device_create_nodes(struct devfs_dev *dev)
{
	struct devfs_mount *dmp;
	SLIST_FOREACH(dmp, &mount_list, m_next)
		dev_add_node(dev, dmp);
}

/*
 * For every device we must now create a new node for that
 * device for this devfs mount
 */
static void
mount_found(struct devfs_mount *dmp)
{
	int error;
	struct devfs_dev *dev;
	struct devfs_node *node;
	struct rule2dev *r2d;

	SLIST_FOREACH(dev, &dev_list, d_next) {
		if ((error = dev_add_node(dev, dmp)) != 0) {
			warnx("could not add node to device");
			continue;
		}

		/* This node has just been placed at the head */
		node = SLIST_FIRST(&dev->d_node_head);
		TAILQ_FOREACH(r2d, &dev->d_pairing, r_next_rule)
			dev_apply_rule_node(node, dmp, r2d->r_rule);

		printf("Pushing nodes to kernel from daemon: %s\n", dmp->m_pathname);
		push_nodes(dev, dmp);
	}		
}

/*
 * This devfs mount is about to be destroy and disappear
 * from the system. We must reap all device nodes that are
 * associated with it.
 */
void
mount_disappear(struct devfs_mount *dmp)
{
	struct devfs_dev *dev;
	struct devfs_node *node;

	SLIST_FOREACH(dev, &dev_list, d_next) {
		SLIST_FOREACH(node, &dev->d_node_head, n_next) {
			if (node->n_cookie.sc_mount != dmp->m_id)
				continue;
		}
	}
}

/*
 * This function tries to match a rule against a device. It returns
 * true if the rule matches the device and false otherwise.
 *
 * XXX: Find some way to implement subsystem specific matching functions.
 * 	For example, we should be able to match a network adapter by its
 *	MAC address. This probably needs to be an ioctl for the dctl driver.
 */
static bool
rule_match(struct devfs_rule *rule, struct devfs_dev *dev)
{
	return false;
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-iv]\n", getprogname());
	exit(EXIT_FAILURE);
}

static struct kevent changebuf[8];
static int nchanges;

static struct kevent *
allocevchange(void)
{               
	if (nchanges == A_CNT(changebuf)) {  
		/* XXX Error handling could be improved. */
		(void) wait_for_events(NULL, 0);
	}

	return &changebuf[nchanges++];
}

static int
wait_for_events(struct kevent *events, size_t nevents)
{
	int rv;

	rv = kevent(fkq, nchanges ? changebuf : NULL, nchanges,
	    events, nevents, NULL);
	nchanges = 0;
	return rv;
}       
