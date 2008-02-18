/*      $NetBSD: devfsd_dev.c,v 1.1.2.2 2008/02/18 22:07:02 mjf Exp $ */

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

struct devfs_dev *
dev_create(struct dctl_kerndev *kerndev, intptr_t cookie)
{
	struct devfs_dev *ddev;

	if ((ddev = malloc(sizeof(*ddev))) == NULL) {
		warn("could not alloc memory for dev\n");
		return NULL;
	}

	ddev->d_cookie = cookie;

	strlcpy(ddev->d_kname, kerndev->k_name, sizeof(ddev->d_kname));

	SLIST_INIT(&ddev->d_node_head);
	TAILQ_INIT(&ddev->d_pairing);

	return ddev;
}

int
dev_add_node(struct devfs_dev *dev, struct devfs_mount *dmp)
{
	size_t len;
	struct devfs_node *dnode;
	char *d_filename;

	if ((dnode = malloc(sizeof(*dnode))) == NULL)
		return -1;

	/* Fill in default attributes */
	dnode->n_attr.d_mode = 0644;
	dnode->n_attr.d_uid = 0;	/* root */
	dnode->n_attr.d_gid = 0;	/* wheel */
	dnode->n_attr.d_flags = dmp->m_visibility;

	/*
	 * By default, we use the driver name for the node
	 * (this may be overridden later with a rule).
	 */
	len = strlen(dev->d_kname) + 1;
	d_filename = malloc(len);
	strlcpy(d_filename, dev->d_kname, len);

	dnode->n_attr.d_component.in_pthcmp.d_filename = d_filename;

	dnode->n_cookie.sc_dev = (intptr_t) dev->d_cookie;
	dnode->n_cookie.sc_mount = dmp->m_id;

	SLIST_INSERT_HEAD(&dev->d_node_head, dnode, n_next);

	return 0;
}

int
dev_del_node(struct devfs_dev *dev, struct devfs_mount *dmp)
{
	struct devfs_node *node;

	SLIST_FOREACH(node, &dev->d_node_head, n_next) {
		if (node->n_cookie.sc_mount != dmp->m_id)
			continue;
	}
	return 0;
}

/*
 * Remove this device from the global device list and free the memory.
 */
void
dev_destroy(struct devfs_dev *dev)
{
	bool removed = false;
	struct devfs_dev *tmp_dev;
	struct devfs_node *node;

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
		
	/* Free all nodes allocated for this device */
	SLIST_FOREACH(node, &dev->d_node_head, n_next) {
		SLIST_REMOVE(&dev->d_node_head, node, devfs_node, n_next);
		free(node);
	}

	/*
	 * TODO: Must remove this device from every rule's list of devices
	 */

	free(dev);
}

/*
 * We have already matched this rule against this device, so apply
 * the rule to the appropriate device nodes.
 */
void
dev_apply_rule(struct devfs_dev *dd, struct devfs_rule *r)
{
	struct rule2dev *rd;
	struct devfs_node *node;
	struct devfs_mount *dmp;

	if ((rd = malloc(sizeof(*rd))) == NULL) {
		warn("could not allocate rule2dev structure");	
		return;
	}

	SLIST_FOREACH(node, &dd->d_node_head, n_next) {
		/* 
		 * Check if this rule should be applied to nodes on
		 * this node's mount path.
		 */
		dmp = mount_lookup(node->n_cookie.sc_mount);
		dev_apply_rule_node(node, dmp, r);
	}

	/* 
	 * Add this device to the rule's devices list and add this rule
	 * to the device's rules list. See devfsd.h for more information.
	 *
	 * XXX: Adding the rules and devices to the appropriate lists is always
	 * done, even if no nodes were actually matched, a new node may be
	 * created for this device where this rule does apply.
	 */
	rd->r_rule = r;
	rd->r_dev = dd;
	TAILQ_INSERT_TAIL(&dd->d_pairing, rd, r_next_rule);
	TAILQ_INSERT_TAIL(&r->r_pairing, rd, r_next_dev);
}

/*
 * Apply a rule to a devfs node.
 */
void
dev_apply_rule_node(struct devfs_node *node, struct devfs_mount *dmp, 
	struct devfs_rule *r)
{
	if (strncmp(r->r_mntpath, 
    	    dmp->m_pathname, sizeof(r->r_mntpath)) != 0)
		return;

	if (r->r_filename != NULL)
		strlcpy(node->n_attr.d_component.out_pthcmp.d_pthcmp,
		    r->r_filename, 
		    sizeof(node->n_attr.d_component.out_pthcmp.d_pthcmp));

	if (r->r_mode != DEVFS_RULE_ATTR_UNSET)
		node->n_attr.d_mode = r->r_mode;

	if (r->r_uid != DEVFS_RULE_ATTR_UNSET)
		node->n_attr.d_uid = r->r_uid;

	if (r->r_gid != DEVFS_RULE_ATTR_UNSET)
		node->n_attr.d_gid = r->r_gid;

	node->n_attr.d_flags = r->r_flags;
}

struct devfs_dev *
dev_lookup(struct dctl_node_cookie c)
{
	struct devfs_dev *dev;

	SLIST_FOREACH(dev, &dev_list, d_next) {
		/* Found */
		if (c.sc_dev == dev->d_cookie)
			break;
	}
	return dev;
}
