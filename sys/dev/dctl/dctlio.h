/* 	$NetBSD: dctlio.h,v 1.1.6.4 2008/03/29 16:17:57 mjf Exp $ */

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

#ifndef _DEV_DCTL_DCTLIO_H_
#define _DEV_DCTL_DCTLIO_H_

#include <sys/statvfs.h>
#include <sys/fstypes.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/conf.h>

enum dmsgtype {
	DCTL_NEW_MOUNT,
	DCTL_UNMOUNT,
	DCTL_NEW_DEVICE,
	DCTL_REMOVED_DEVICE,
	DCTL_UPDATE_NODE_NAME,
	DCTL_UPDATE_NODE_ATTR
};

struct dctl_specnode_attr {
	mode_t	d_mode;
	uid_t	d_uid;
	gid_t	d_gid;
	int	d_flags;

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

struct dctl_mount {
	char m_pathname[_VFS_MNAMELEN];
	int32_t m_id;
	int m_visibility;
};

struct dctl_kerndev {
	char k_name[16];
	enum devtype k_type;
};

/*
 * This cookie references device special nodes by a device
 * and a mount point. For instance, a device may be multiple special
 * nodes that refer to it.
 */
struct dctl_node_cookie {
	dev_t sc_dev;		/* cookie for device */
	int32_t sc_mount;	/* cookie for mount point */
};

#define NODE_COOKIE_MATCH(a, b)					\
	((a.sc_dev == b.sc_dev) && (a.sc_mount == b.sc_mount))

/*
 * This defines the structure of a message that is passed between dctl(4)
 * and devfsd(8). The mesasge type in 'd_type' dictates what type of cookie
 * and message-specific data should be used.
 */
struct dctl_msg {
	enum dmsgtype d_type;
	union {
		dev_t c_dev;		/* device cookie */
		int32_t c_mount;	/* mount point cookie */
		struct dctl_node_cookie c_specnode; /* dev spec node cookie */
	} d_cookie;
#define d_dc d_cookie.c_dev
#define d_mc d_cookie.c_mount
#define d_sn d_cookie.c_specnode
	union {
		struct dctl_specnode_attr a_attr;
		struct dctl_kerndev a_kdev;
		struct dctl_mount a_mount;
	} d_args;
#define d_attr d_args.a_attr
#define d_kdev d_args.a_kdev
#define d_mp d_args.a_mount
};

struct dctl_event_entry {
	struct dctl_msg *de_msg;
	SIMPLEQ_ENTRY(dctl_event_entry)	de_entries;
	SIMPLEQ_ENTRY(dctl_event_entry) dm_entries;	/* mount list */
	int de_on_mount;
};

/*
 * A structure that contains information about a disk device that
 * is suitable for use by devfsd(8) for matching rules.
 */
struct dctl_disk_info {
	const char *di_fstype;	/* file system type */
};

/*
 * The generic structure used by devfsd(8) to gather information about
 * a device, which will later be used for matching.
 */
struct dctl_ioctl_data {
	enum devtype d_type;
	dev_t d_dev;		/* device cookie */
	u_long d_cmd;
	union {
		struct dctl_disk_info di;
	} i_args;
};

#define	DCTL_IOC_NEXTEVENT _IOR('A', 0, struct dctl_msg) /* fetch event */
#define DCTL_IOC_CREATENODE _IOW('A', 1, struct dctl_msg) /* create node */
#define DCTL_IOC_DELETENODE _IOW('A', 2, struct dctl_msg) /* delete node */
#define DCTL_IOC_INNERIOCTL _IOR('A', 3, struct dctl_ioctl_data)

#endif /* _DEV_DCTL_DCTLIO_H_ */
