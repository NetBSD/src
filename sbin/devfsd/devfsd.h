/* 	$NetBSD: devfsd.h,v 1.1.8.5 2008/04/14 16:23:57 mjf Exp $ */

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

#ifndef _DEVFSD_H_
#define _DEVFSD_H_

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/condvar.h>
#include <sys/statvfs.h>
#include <syslog.h>

#include <prop/proplib.h>
#include <dev/devfsctl/devfsctlio.h>

/*       
 * A rule can be broken into two parts:
 *      1) How to match a device
 *      2) What attributes to set once a device is matched 
 *      
 * Matching a device:   
 *      We can match a device by any one of these characteristics (starting
 * from specific ones, which may only be implemented by certain subsystems,
 * to the more generic ones which all devices will have).
 *       
 * Attributes:
 *      Once we've matched a rule to a device this section specifices the
 * attributes to set on this device node. For example, we can set the perms
 * and owner/group.     
 */     

struct devfs_rule {
        const char	*r_name;           /* rule name */
	SLIST_ENTRY(devfs_rule) r_next;	/* next rule in list */

	TAILQ_HEAD(,rule2dev) r_pairing;

	/* which devfs mount this rule should be applied to */
	const char	r_mntpath[_VFS_MNAMELEN];

        /*                                  
         * How to match (generic).
         */
        const char      *r_label;
        const char      *r_manufacturer;        /* manufacturer of device */
        const char      *r_product;             /* product name */
        const char      *r_drivername;		/* e.g. "wd" for wd(4) */

	/* 
	 * How we can match a disk type device.
	 */
	struct {
		const char	*r_fstype;		/* file system type */
		const char 	*r_wident;		/* wedge identifier */
	} r_disk;

        /*
         * Attributes to set when we find a match.
         */
        char      	*r_filename;    /* filename to use */
        mode_t          r_mode;         
        uid_t           r_uid;		/* owner */
        gid_t           r_gid;
        int             r_flags; 	/* see below */
};

#define DEVFS_INVISIBLE         0x000           /* hide entry from userland */
#define DEVFS_VISIBLE           0x001           /* make entry visible to
                                                 * userland */
#define DEVFS_RULE_ATTR_UNSET	0xdeadbeef	/* attribute is not set */

#define ATTR_ISSET(x) (x != DEVFS_RULE_ATTR_UNSET)

SLIST_HEAD(rlist_head, devfs_rule) rule_list;

struct devfs_node {
	SLIST_ENTRY(devfs_node) n_next;
	struct devfsctl_specnode_attr n_attr;
	struct devfsctl_node_cookie n_cookie; /* cookie to uniquely identify node */
};

/*
 * This structure is our understanding of a device that is connected
 * to the system. Each device has a unique cookie that allows devfsctl(4)
 * to understand what device we're talking about when we request some
 * action for a device.
 *
 * Because there can be multiple devfs mounts on the system we need to
 * keep track of all the different special nodes in use for this device.
 * We keep a list of all device special nodes for this device in the 
 * 'd_node_head' member.
 */
struct devfs_dev {      
	int32_t			d_cookie;	/* cookie for this device */
	char			d_kname[16];	/* device driver name */
	int			d_char;		/* block or char */
	enum devtype 		d_type;		/* dev type, see devfsctlio.h  */
	SLIST_ENTRY(devfs_dev) 	d_next;  	/* next device in list */
	SLIST_HEAD(, devfs_node) d_node_head; 	/* nodes for this device */
	TAILQ_HEAD(, rule2dev) d_pairing;
};

SLIST_HEAD(dlist_head, devfs_dev) dev_list;

/*
 * This is allows us to pair up devices with rules that have
 * been applied to that it and also pair up rules with devices. 
 * This is a many-to-many relationship.
 */
struct rule2dev {
	TAILQ_ENTRY(rule2dev) r_next_rule;
	TAILQ_ENTRY(rule2dev) r_next_dev;
	struct devfs_rule *r_rule;
	struct devfs_dev *r_dev;
};

struct devfs_mount {
	char m_pathname[_VFS_MNAMELEN];
	int32_t m_id;
	int m_visibility;
	SLIST_ENTRY(devfs_mount) m_next;
};
SLIST_HEAD(mlist_head, devfs_mount) mount_list;

int rule_parsefile(const char *);
int rule_writefile(const char *);
int rule_init(void);
int rule_rebuild_attr(struct devfs_dev *);

int dev_init(void);
struct devfs_dev *dev_create(struct devfsctl_kerndev *, intptr_t);
void dev_apply_rule(struct devfs_dev *, struct devfs_rule *);
void dev_apply_rule_node(struct devfs_node *, struct devfs_mount *,
	struct devfs_rule *);
void dev_destroy(struct devfs_dev *);
struct devfs_dev *dev_lookup(struct devfsctl_node_cookie);
int dev_add_node(struct devfs_dev *, struct devfs_mount *);
void dev_del_node(struct devfs_dev *, struct devfs_node *);

struct devfs_mount *mount_create(int32_t, const char *, int);
struct devfs_mount *mount_lookup(int32_t);
void mount_destroy(struct devfs_mount *);

#endif	/* _DEVFSD_H_ */
