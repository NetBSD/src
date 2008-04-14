/* 	$NetBSD: devfsctlio.h,v 1.1.2.2 2008/04/14 16:23:57 mjf Exp $ */

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

#ifndef _DEV_DEVFSCTL_DEVFSCTLIO_H_
#define _DEV_DEVFSCTL_DEVFSCTLIO_H_

#include <sys/statvfs.h>
#include <sys/fstypes.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/disk.h>

enum dmsgtype {
	DEVFSCTL_NEW_MOUNT,
	DEVFSCTL_UNMOUNT,
	DEVFSCTL_NEW_DEVICE,
	DEVFSCTL_REMOVED_DEVICE,
	DEVFSCTL_UPDATE_NODE_NAME,
	DEVFSCTL_UPDATE_NODE_ATTR
};

struct devfsctl_specnode_attr {
	mode_t	d_mode;
	uid_t	d_uid;
	gid_t	d_gid;
	int	d_flags;
	int	d_char;

	union {
		struct {
			char	*d_filename;
		} in_pthcmp;

		struct {
			ino_t	d_pino;
			char	d_pthcmp[90];
		} out_pthcmp;
	} d_component;
};

struct devfsctl_mount {
	char m_pathname[_VFS_MNAMELEN];
	int32_t m_id;
	int m_visibility;
};

struct devfsctl_kerndev {
	char k_name[16];
	enum devtype k_type;
	int k_char;
};

/*
 * This cookie references device special nodes by a device
 * and a mount point. For instance, a device may be multiple special
 * nodes that refer to it.
 */
struct devfsctl_node_cookie {
	dev_t sc_dev;		/* cookie for device */
	int32_t sc_mount;	/* cookie for mount point */
};

#define NODE_COOKIE_MATCH(a, b)					\
	((a.sc_dev == b.sc_dev) && (a.sc_mount == b.sc_mount))

/*
 * This defines the structure of a message that is passed between devfsctl(4)
 * and devfsd(8). The mesasge type in 'd_type' dictates what type of cookie
 * and message-specific data should be used.
 */
struct devfsctl_msg {
	enum dmsgtype d_type;
	union {
		dev_t c_dev;		/* device cookie */
		int32_t c_mount;	/* mount point cookie */
		struct devfsctl_node_cookie c_specnode; /* dev spec node cookie */
	} d_cookie;
#define d_dc d_cookie.c_dev
#define d_mc d_cookie.c_mount
#define d_sn d_cookie.c_specnode
	union {
		struct devfsctl_specnode_attr a_attr;
		struct devfsctl_kerndev a_kdev;
		struct devfsctl_mount a_mount;
	} d_args;
#define d_attr d_args.a_attr
#define d_kdev d_args.a_kdev
#define d_mp d_args.a_mount
};

struct devfsctl_event_entry {
	struct devfsctl_msg *de_msg;
	SIMPLEQ_ENTRY(devfsctl_event_entry)	de_entries;
	SIMPLEQ_ENTRY(devfsctl_event_entry) dm_entries;	/* mount list */
	int de_on_mount;
};

/*
 * A structure that contains information about a disk device that
 * is suitable for use by devfsd(8) for matching rules.
 */
struct devfsctl_disk_info {
	const char *di_fstype;	/* file system type */
	struct dkwedge_info di_winfo;
};

/*
 * The generic structure used by devfsd(8) to gather information about
 * a device, which will later be used for matching.
 */
struct devfsctl_ioctl_data {
	enum devtype d_type;
	dev_t d_dev;		/* device cookie */
	u_long d_cmd;
	union {
		struct devfsctl_disk_info di;
	} i_args;
};

/* fetch event */
#define	DEVFSCTL_IOC_NEXTEVENT _IOR('A', 0, struct devfsctl_msg) 

/* create node */
#define DEVFSCTL_IOC_CREATENODE _IOW('A', 1, struct devfsctl_msg) 

/* delete node */
#define DEVFSCTL_IOC_DELETENODE _IOW('A', 2, struct devfsctl_msg) 

#define DEVFSCTL_IOC_INNERIOCTL _IOWR('A', 3, struct devfsctl_ioctl_data)

#endif /* _DEV_DEVFSCTL_DEVFSCTLIO_H_ */
