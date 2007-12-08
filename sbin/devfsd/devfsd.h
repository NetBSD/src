/* 	$NetBSD: devfsd.h,v 1.1.2.1 2007/12/08 22:05:04 mjf Exp $ */

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

#ifndef _DEVFSD_H_
#define _DEVFSD_H_

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/param.h>

#include <prop/proplib.h>

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

	/* 
	 * This is used by the devfs_dev structure to keep track of which
	 * rules have been applied to a device.
	 */
	SLIST_ENTRY(devfs_rule) r_dev_next;

        /*                                  
         * How to match.
         */
        const char      *r_label;
        const char      *r_manufacturer;        /* manufacturer of device */
        const char      *r_product;             /* product name */
        /* Driver name given by kernel, i.e. "sd" for sd(4). */
        const char      *r_drivername;

        /*
         * Attributes to set.
         */
        const char      *r_filename;    /* filename to use */
        mode_t          r_mode;         
        uid_t           r_owner;	/* owner */
        gid_t           r_group;
        int             r_flags; 	/* see below */
};

#define DEVFS_INVISIBLE         0x001           /* hide entry from userland */
#define DEVFS_VISIBLE           0x002           /* make entry visible to
                                                 * userland */
#define DEVFS_RULE_ATTR_UNSET	0xdeadbeef	/* attribute is not set */

SLIST_HEAD(rlist_head, devfs_rule) rule_list;

int rule_parsefile(const char *);
int rule_writefile(const char *);
int rule_init(void);

/*       
 * This is the devfsd daemon's internal representation of a device. This
 * data structure is used to create a device special file for this device.
 *
 * We also keep some extra information such as whether or not the device
 * is the boot device.
 */     

struct devfs_dev {
	device_t	d_dev;
	SLIST_ENTRY(devfs_dev) d_next;	/* next device in list */
	char 		d_filename[MAXPATHLEN];	/* filename to use */
	mode_t		d_mode;
        uid_t           d_owner;
        gid_t           d_group;
        int             d_flags;

	/* 
	 * We maintain a list of rules that have been applied to this device.
	 */
	SLIST_HEAD(, devfs_rule) d_rule_head;
};

SLIST_HEAD(dlist_head, devfs_dev) dev_list;

int 	dev_init(void);
const char *device_xname(device_t);
struct devfs_dev *dev_create(device_t, int);
void	dev_apply_rule(struct devfs_dev *, struct devfs_rule *);
void	dev_destroy(struct devfs_dev *);

#endif	/* _DEVFSD_H_ */
