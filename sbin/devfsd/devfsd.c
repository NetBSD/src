/* 	$NetBSD: devfsd.c,v 1.1.8.6 2008/04/14 16:23:57 mjf Exp $ */

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
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <dev/devfsctl/devfsctlio.h>

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
static int update(struct devfs_dev *, struct devfsctl_msg *);
static void device_found(struct devfs_dev *);
static void device_create_nodes(struct devfs_dev *);
static void push_nodes(struct devfs_dev *);
static void push_node(struct devfs_node *);
static void device_disappear(struct devfs_dev *);
static void mount_found(struct devfs_mount *);
static void mount_disappear(struct devfs_mount *);
static bool rule_match(struct devfs_rule *, struct devfs_dev *);
static void usage(void);

/* can userland see device nodes? */
static int default_visibility = DEVFS_VISIBLE;

/* kqueue stuff */
static struct kevent *allocevchange(void);
static int wait_for_events(struct kevent *, size_t, bool);
static void dispatch_read_devfsctl(struct kevent *);
static int fkq;
static int devfsctl_fd;

#define A_CNT(x)        (sizeof((x)) / sizeof((x)[0]))

int 
main(int argc, char **argv)
{
	int ch;
	bool single_run = false;
	struct kevent events[16], *ev;

	(void)setlocale(LC_ALL, "");

	while ((ch = getopt(argc, argv, "isv")) != -1) {
		switch(ch) {
		case 'i':
			default_visibility = DEVFS_INVISIBLE;
			break;
		case 's':
			single_run = true;
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
		err(EXIT_FAILURE, "%s", _PATH_CONFIG);

	if ((devfsctl_fd = open(_PATH_DEVFSCTL, O_RDONLY, 0)) < 0)
		err(EXIT_FAILURE, "%s: could not open", _PATH_DEVFSCTL);
	
	if (single_run != true) {
		/* Don't close stdin/stdout/stderr when we goto bg */
		if (daemon(0, 1) != 0)
			err(EXIT_FAILURE, "could not demonize");
	}

	if ((fkq = kqueue()) < 0)
		err(EXIT_FAILURE, "cannot create event queue");

	ev = allocevchange();
	EV_SET(ev, devfsctl_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
	    (intptr_t) dispatch_read_devfsctl);

	openlog("devfsd", LOG_PID, LOG_DAEMON);

	/* Handle events from the device driver */
	for (;;) {
		void (*handler)(struct kevent *);
		int i, rv;
		rv = wait_for_events(events, A_CNT(events), single_run);
		if (rv == 0) {
			if (single_run == true)
				break;
			continue;
		}
		if (rv < 0) {
			syslog(LOG_ERR, "kevent() failed");
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
 * Dispatch routine for reading /dev/devfsctl
 */
static void
dispatch_read_devfsctl(struct kevent *ev)
{
	ssize_t rv;
	int fd = ev->ident;
	struct devfsctl_msg msg;
	struct devfs_dev *de;
	struct devfs_mount *dmp;

	memset(&msg, 0, sizeof(msg));

	rv = ioctl(fd, DEVFSCTL_IOC_NEXTEVENT, &msg);
	if (rv >= 0) {
		switch(msg.d_type) {
		case DEVFSCTL_NEW_DEVICE:
			de = dev_create(&msg.d_kdev, msg.d_dc);
			if (de == NULL) {
				syslog(LOG_ERR, 
				    "could not internalize device\n");
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
			 * nodes. Pass in some extra info.
			 */
			device_found(de);

			/*
			 * Push the new nodes into their respective
			 * file systems at once!
			 */
			push_nodes(de);
			break;
		case DEVFSCTL_UPDATE_NODE_ATTR:
			de = dev_lookup(msg.d_sn);
			if (de == NULL) {
				syslog(LOG_ERR, 
				    "could not find device to update\n");
				return;
			}
			if (update(de, &msg) == -1)
				syslog(LOG_WARNING, 
				    "your changes will be lost on reboot\n");
			break;

		case DEVFSCTL_UPDATE_NODE_NAME:
			break;

		case DEVFSCTL_NEW_MOUNT:
			dmp = mount_create(msg.d_mc,
			    msg.d_mp.m_pathname, msg.d_mp.m_visibility);

			if (dmp == NULL) {
				syslog(LOG_ERR,
				    "failed to internalize mount\n");
				return;
			}

			mount_found(dmp);
			break;
		case DEVFSCTL_UNMOUNT:
			dmp = mount_lookup(msg.d_mc);

			if (dmp == NULL) {
				syslog(LOG_ERR, "unknown mount removed\n");
				return;
			}

			mount_disappear(dmp);
			mount_destroy(dmp);

			break;
		case DEVFSCTL_REMOVED_DEVICE:
			de = dev_lookup(msg.d_sn);
			if (de == NULL) {
				syslog(LOG_ERR, 
				    "could not find device to update\n");
				return;
			}
			device_disappear(de);
			break;
		default:
			syslog(LOG_ERR, "Unknown event: %d\n", msg.d_type);
			break;
		}
	} else if (rv < 0 && errno != EINTR) {
		/*      
		* /dev/devfsctl has croaked.  Disable the event
		* so it won't bother us again.
		*/
		struct kevent *cev = allocevchange();
		EV_SET(cev, fd, EVFILT_READ, EV_DISABLE,
		    0, 0, (intptr_t) dispatch_read_devfsctl);
	}
}

/*      
 * Push all device nodes for device 'dev' into their respective
 * devfs file systems.
 */
static void
push_nodes(struct devfs_dev *dev)
{
	struct devfs_node *node;

	SLIST_FOREACH(node, &dev->d_node_head, n_next)
		push_node(node);
}

static void
push_node(struct devfs_node *node)
{
	int rv;
	struct devfsctl_msg msg;

	memset(&msg, 0, sizeof(msg));

	msg.d_sn = node->n_cookie;
	msg.d_attr = node->n_attr;
	rv = ioctl(devfsctl_fd, DEVFSCTL_IOC_CREATENODE, &msg);

	if (rv < 0)
		syslog(LOG_WARNING, "could not push node into devfs");
}

/*
 * Delete a node from it's mount point.
 */
static void
delete_node(struct devfs_node *node)
{
	int rv;
	struct devfsctl_msg msg;

	memset(&msg, 0, sizeof(msg));

	msg.d_sn = node->n_cookie;
	msg.d_attr = node->n_attr;
	rv = ioctl(devfsctl_fd, DEVFSCTL_IOC_DELETENODE, &msg);

	if (rv < 0)
		syslog(LOG_WARNING, "could not delete node");
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
update(struct devfs_dev *dev, struct devfsctl_msg *msg)
{
	struct devfs_node *node;

	/* Figure out which node for this device is being updated */
	SLIST_FOREACH(node, &dev->d_node_head, n_next) {
		if (NODE_COOKIE_MATCH(node->n_cookie, msg->d_sn))
			break;
	}

	if (node == NULL) {
		syslog(LOG_WARNING, "node not on device node list\n");
		return -1;
	}

	if (msg->d_type != DEVFSCTL_UPDATE_NODE_ATTR &&
	    msg->d_type != DEVFSCTL_UPDATE_NODE_NAME) {
		syslog(LOG_WARNING, "%s: node not being updated\n", 
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
		syslog(LOG_WARNING, "%s: could not write to file", _PATH_CONFIG);
		return -1;
	}

	return 0;
}

/*
 * We've learnt about a new device from /dev/devfsctl so run it against our
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
	SLIST_FOREACH(dmp, &mount_list, m_next) {
		if (dev_add_node(dev, dmp) == -1)
			syslog(LOG_WARNING, "could not add node to devfs_dev");
	}	
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
			syslog(LOG_WARNING, "could not add node to device");
			continue;
		}

		/* This node has just been placed at the head */
		node = SLIST_FIRST(&dev->d_node_head);
		TAILQ_FOREACH(r2d, &dev->d_pairing, r_next_rule)
			dev_apply_rule_node(node, dmp, r2d->r_rule);

		push_node(node);
	}		
}

void
device_disappear(struct devfs_dev *dev)
{
	struct devfs_node *node;

	/* First delete the node in the file systems */
	SLIST_FOREACH(node, &dev->d_node_head, n_next)
		delete_node(node);

	/* Destroy the device (which also removes and free()s all nodes) */
	dev_destroy(dev);
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

		/* Does the device node exist on this mount? */
		if (node == NULL)
			continue;

		dev_del_node(dev, node);
	}
}

/*
 * This function tries to match a rule against a device. It returns
 * true if the rule matches the device and false otherwise.
 *
 * => NOTE: The truth value of a match is a conjunction of the match
 *    criteria.
 *
 * XXX: Find some way to implement subsystem specific matching functions.
 * 	For example, we should be able to match a network adapter by its
 *	MAC address. This probably needs to be an ioctl for the devfsctl driver.
 */
static bool
rule_match(struct devfs_rule *rule, struct devfs_dev *dev)
{
	int error;
	bool match = false;

	/* Simplest way to match is by driver name */
	if (rule->r_drivername != NULL) {
		if (strcmp(rule->r_drivername, dev->d_kname) == 0)
			match = true;
	}

	/* Handle disks */
	if (dev->d_type == DEV_DISK) {
		struct devfsctl_ioctl_data idata;
		memset(&idata, 0, sizeof(idata));

		idata.d_dev = dev->d_cookie;
		idata.d_type = DEV_DISK;

		error = ioctl(devfsctl_fd, DEVFSCTL_IOC_INNERIOCTL, &idata);
		if (rule->r_disk.r_fstype != NULL) {
			if (strcmp(rule->r_disk.r_fstype, 
			    idata.i_args.di.di_fstype) == 0)
				match = true;	
		}

		/* Handle disks that have wedges */
		if (rule->r_disk.r_wident != NULL) {
			if (strcmp(rule->r_disk.r_wident,
			    (char *)&idata.i_args.di.di_winfo.dkw_wname) == 0)
				match = true;
		}
	}
	return match;
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
		(void) wait_for_events(NULL, 0, false);
	}

	return &changebuf[nchanges++];
}

static int
wait_for_events(struct kevent *events, size_t nevents, bool timeout)
{
	int rv;

	struct timespec tout = { 1, 0 };	/* 1sec timeout */

	if (timeout != true)
		rv = kevent(fkq, nchanges ? changebuf : NULL, nchanges,
	    	    events, nevents, NULL);
	else
		rv = kevent(fkq, nchanges ? changebuf : NULL, nchanges,
	    	    events, nevents, &tout);

	nchanges = 0;
	return rv;
}       
