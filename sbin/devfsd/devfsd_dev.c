/* 	$NetBSD: devfsd_dev.c,v 1.1.2.1 2007/12/08 22:05:05 mjf Exp $ */

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

/*
 * Functions to manipulate the device list
 */

#include <sys/queue.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "devfsd.h"

extern struct dlist_head  dev_list;

int
dev_init(void)
{
	SLIST_INIT(&dev_list);
	return 0;
}

/*
 * Create a new devfsd device and fill it with default attributes.
 */
struct devfs_dev *
dev_create(device_t kernel_dev, int visibility)
{
	struct devfs_dev *dev;

	if ((dev = (struct devfs_dev *)malloc(sizeof(dev))) == NULL)
		return NULL;

	/* Fill in default attributes */
	dev->d_dev = kernel_dev; 

	strlcpy(dev->d_filename, device_xname(kernel_dev), 
	    sizeof(dev->d_filename));

	dev->d_mode = 0644;
	dev->d_owner = 0;	/* root */
	dev->d_group = 0;	/* wheel */
	dev->d_flags = visibility;

	SLIST_INIT(&dev->d_rule_head);

	return dev;
}

/*
 * Remove this device from the global device list and free the memory.
 */
void
dev_destroy(struct devfs_dev *dev)
{
	bool removed = false;
	struct devfs_dev *tmp_dev;

	SLIST_FOREACH(tmp_dev, &dev_list, d_next) {
		if (tmp_dev == dev) {
			SLIST_REMOVE(&dev_list, dev, devfs_dev, d_next);
			removed = true;
			tmp_dev = NULL;
			break;
		}
	}

	if (removed == false) {
		warnx("%p not on device list\n", dev);
		return;
	}
		
	free(dev);
}

/*
 * We have already matched this rule against this device, so apply
 * the rule to the device node.
 */
void
dev_apply_rule(struct devfs_dev *d, struct devfs_rule *r)
{
	if (r->r_filename != NULL)
		strlcpy(d->d_filename, r->r_filename, sizeof(d->d_filename));

	if (r->r_mode != DEVFS_RULE_ATTR_UNSET)
		d->d_mode = r->r_mode;

	if (r->r_owner != DEVFS_RULE_ATTR_UNSET)
		d->d_owner = r->r_owner;

	if (r->r_group != DEVFS_RULE_ATTR_UNSET)
		d->d_group = r->r_group;

	d->d_flags = r->r_flags;

	/* 
	 * Add this device to the list of devices this rule applies
	 * to
	 */
	SLIST_INSERT_HEAD(&d->d_rule_head, r, r_dev_next);
}

/* 
 * XXX: this is a userland definition of device_xname and as such will break
 * if the kernel version of device_xname changes. 
 */
const char *
device_xname(device_t d)
{
	return d->dv_xname;
}
