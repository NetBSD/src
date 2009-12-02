/*	$NetBSD: lvmcache.h,v 1.1.1.2 2009/12/02 00:25:44 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_CACHE_H
#define _LVM_CACHE_H

#include "dev-cache.h"
#include "uuid.h"
#include "label.h"

#define ORPHAN_PREFIX "#"
#define ORPHAN_VG_NAME(fmt) ORPHAN_PREFIX "orphans_" fmt

#define CACHE_INVALID	0x00000001
#define CACHE_LOCKED	0x00000002

/* LVM specific per-volume info */
/* Eventual replacement for struct physical_volume perhaps? */

struct cmd_context;
struct format_type;
struct volume_group;

/* One per VG */
struct lvmcache_vginfo {
	struct dm_list list;	/* Join these vginfos together */
	struct dm_list infos;	/* List head for lvmcache_infos */
	const struct format_type *fmt;
	char *vgname;		/* "" == orphan */
	uint32_t status;
	char vgid[ID_LEN + 1];
	char _padding[7];
	struct lvmcache_vginfo *next; /* Another VG with same name? */
	char *creation_host;
	char *vgmetadata;	/* Copy of VG metadata as format_text string */
	unsigned precommitted;	/* Is vgmetadata live or precommitted? */
};

/* One per device */
struct lvmcache_info {
	struct dm_list list;	/* Join VG members together */
	struct dm_list mdas;	/* list head for metadata areas */
	struct dm_list das;	/* list head for data areas */
	struct lvmcache_vginfo *vginfo;	/* NULL == unknown */
	struct label *label;
	const struct format_type *fmt;
	struct device *dev;
	uint64_t device_size;	/* Bytes */
	uint32_t status;
};

int lvmcache_init(void);
void lvmcache_destroy(struct cmd_context *cmd, int retain_orphans);

/* Set full_scan to 1 to reread every filtered device label or
 * 2 to rescan /dev for new devices */
int lvmcache_label_scan(struct cmd_context *cmd, int full_scan);

/* Add/delete a device */
struct lvmcache_info *lvmcache_add(struct labeller *labeller, const char *pvid,
				   struct device *dev,
				   const char *vgname, const char *vgid,
				   uint32_t vgstatus);
int lvmcache_add_orphan_vginfo(const char *vgname, struct format_type *fmt);
void lvmcache_del(struct lvmcache_info *info);

/* Update things */
int lvmcache_update_vgname_and_id(struct lvmcache_info *info,
				  const char *vgname, const char *vgid,
				  uint32_t vgstatus, const char *hostname);
int lvmcache_update_vg(struct volume_group *vg, unsigned precommitted);

void lvmcache_lock_vgname(const char *vgname, int read_only);
void lvmcache_unlock_vgname(const char *vgname);
int lvmcache_verify_lock_order(const char *vgname);

/* Queries */
const struct format_type *fmt_from_vgname(const char *vgname, const char *vgid);
struct lvmcache_vginfo *vginfo_from_vgname(const char *vgname,
					   const char *vgid);
struct lvmcache_vginfo *vginfo_from_vgid(const char *vgid);
struct lvmcache_info *info_from_pvid(const char *pvid, int valid_only);
const char *vgname_from_vgid(struct dm_pool *mem, const char *vgid);
struct device *device_from_pvid(struct cmd_context *cmd, struct id *pvid);
int vgs_locked(void);
int vgname_is_locked(const char *vgname);

/* Returns list of struct str_lists containing pool-allocated copy of vgnames */
/* Set full_scan to 1 to reread every filtered device label */
struct dm_list *lvmcache_get_vgnames(struct cmd_context *cmd, int full_scan);

/* Returns list of struct str_lists containing pool-allocated copy of vgids */
/* Set full_scan to 1 to reread every filtered device label */
struct dm_list *lvmcache_get_vgids(struct cmd_context *cmd, int full_scan);

/* Returns list of struct str_lists containing pool-allocated copy of pvids */
struct dm_list *lvmcache_get_pvids(struct cmd_context *cmd, const char *vgname,
				const char *vgid);

/* Returns cached volume group metadata. */
struct volume_group *lvmcache_get_vg(const char *vgid, unsigned precommitted);
void lvmcache_drop_metadata(const char *vgname);

#endif
