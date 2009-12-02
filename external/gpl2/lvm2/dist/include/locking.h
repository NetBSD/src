/*	$NetBSD: locking.h,v 1.1.1.2 2009/12/02 00:25:44 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_LOCKING_H
#define _LVM_LOCKING_H

#include "uuid.h"
#include "config.h"

int init_locking(int type, struct cmd_context *cmd);
void fin_locking(void);
void reset_locking(void);
int vg_write_lock_held(void);
int locking_is_clustered(void);

int remote_lock_held(const char *vol);

/*
 * LCK_VG:
 *   Lock/unlock on-disk volume group data.
 *   Use VG_ORPHANS to lock all orphan PVs.
 *   Use VG_GLOBAL as a global lock and to wipe the internal cache.
 *   char *vol holds volume group name.
 *   Set the LCK_CACHE flag to invalidate 'vol' in the internal cache.
 *   If more than one lock needs to be held simultaneously, they must be
 *   acquired in alphabetical order of 'vol' (to avoid deadlocks).
 *
 * LCK_LV:
 *   Lock/unlock an individual logical volume
 *   char *vol holds lvid
 */
int lock_vol(struct cmd_context *cmd, const char *vol, uint32_t flags);

/*
 * Internal locking representation.
 *   LCK_VG: Uses prefix V_ unless the vol begins with # (i.e. #global or #orphans)
 *           or the LCK_CACHE flag is set when it uses the prefix P_.
 * If LCK_CACHE is set, we do not take out a real lock.
 */

/*
 * Does the LVM1 driver have this VG active?
 */
int check_lvm1_vg_inactive(struct cmd_context *cmd, const char *vgname);

/*
 * Lock type - these numbers are the same as VMS and the IBM DLM
 */
#define LCK_TYPE_MASK	0x00000007U

#define LCK_NULL	0x00000000U	/* LCK$_NLMODE */
#define LCK_READ	0x00000001U	/* LCK$_CRMODE */
					/* LCK$_CWMODE */
#define LCK_PREAD       0x00000003U	/* LCK$_PRMODE */
#define LCK_WRITE	0x00000004U	/* LCK$_PWMODE */
#define LCK_EXCL	0x00000005U	/* LCK$_EXMODE */
#define LCK_UNLOCK      0x00000006U	/* This is ours */

/*
 * Lock scope
 */
#define LCK_SCOPE_MASK	0x00000008U
#define LCK_VG		0x00000000U
#define LCK_LV		0x00000008U

/*
 * Lock bits
 */
#define LCK_NONBLOCK	0x00000010U	/* Don't block waiting for lock? */
#define LCK_HOLD	0x00000020U	/* Hold lock when lock_vol returns? */
#define LCK_LOCAL	0x00000040U	/* Don't propagate to other nodes */
#define LCK_CLUSTER_VG	0x00000080U	/* VG is clustered */
#define LCK_CACHE	0x00000100U	/* Operation on cache only using P_ lock */

/*
 * Additional lock bits for cluster communication
 */
#define LCK_PARTIAL_MODE        0x00000001U	/* Partial activation? */
#define LCK_MIRROR_NOSYNC_MODE	0x00000002U	/* Mirrors don't require sync */
#define LCK_DMEVENTD_MONITOR_MODE	0x00000004U	/* Register with dmeventd */
#define LCK_CONVERT		0x00000008U	/* Convert existing lock */

/*
 * Special cases of VG locks.
 */
#define VG_ORPHANS	"#orphans"
#define VG_GLOBAL	"#global"

/*
 * Common combinations
 */
#define LCK_NONE		(LCK_VG | LCK_NULL)

#define LCK_VG_READ		(LCK_VG | LCK_READ | LCK_HOLD)
#define LCK_VG_WRITE		(LCK_VG | LCK_WRITE | LCK_HOLD)
#define LCK_VG_UNLOCK		(LCK_VG | LCK_UNLOCK)
#define LCK_VG_DROP_CACHE	(LCK_VG | LCK_WRITE | LCK_CACHE)
#define LCK_VG_BACKUP		(LCK_VG | LCK_CACHE)

#define LCK_LV_EXCLUSIVE	(LCK_LV | LCK_EXCL)
#define LCK_LV_SUSPEND		(LCK_LV | LCK_WRITE)
#define LCK_LV_RESUME		(LCK_LV | LCK_UNLOCK)
#define LCK_LV_ACTIVATE		(LCK_LV | LCK_READ)
#define LCK_LV_DEACTIVATE	(LCK_LV | LCK_NULL)

#define LCK_MASK (LCK_TYPE_MASK | LCK_SCOPE_MASK)

#define LCK_LV_CLUSTERED(lv)	\
	(vg_is_clustered((lv)->vg) ? LCK_CLUSTER_VG : 0)

#define lock_lv_vol(cmd, lv, flags)	\
	lock_vol(cmd, (lv)->lvid.s, flags | LCK_LV_CLUSTERED(lv))

#define unlock_vg(cmd, vol)	lock_vol(cmd, vol, LCK_VG_UNLOCK)
#define unlock_and_release_vg(cmd, vg, vol) \
	do { \
		unlock_vg(cmd, vol); \
		vg_release(vg); \
	} while (0)

#define resume_lv(cmd, lv)	lock_lv_vol(cmd, lv, LCK_LV_RESUME)
#define suspend_lv(cmd, lv)	lock_lv_vol(cmd, lv, LCK_LV_SUSPEND | LCK_HOLD)
#define deactivate_lv(cmd, lv)	lock_lv_vol(cmd, lv, LCK_LV_DEACTIVATE)
#define activate_lv(cmd, lv)	lock_lv_vol(cmd, lv, LCK_LV_ACTIVATE | LCK_HOLD)
#define activate_lv_excl(cmd, lv)	\
				lock_lv_vol(cmd, lv, LCK_LV_EXCLUSIVE | LCK_HOLD)
#define activate_lv_local(cmd, lv)	\
	lock_lv_vol(cmd, lv, LCK_LV_ACTIVATE | LCK_HOLD | LCK_LOCAL)
#define deactivate_lv_local(cmd, lv)	\
	lock_lv_vol(cmd, lv, LCK_LV_DEACTIVATE | LCK_LOCAL)
#define drop_cached_metadata(vg)	\
	lock_vol((vg)->cmd, (vg)->name, LCK_VG_DROP_CACHE)
#define remote_backup_metadata(vg)	\
	lock_vol((vg)->cmd, (vg)->name, LCK_VG_BACKUP)

/* Process list of LVs */
int suspend_lvs(struct cmd_context *cmd, struct dm_list *lvs);
int resume_lvs(struct cmd_context *cmd, struct dm_list *lvs);
int activate_lvs(struct cmd_context *cmd, struct dm_list *lvs, unsigned exclusive);

/* Interrupt handling */
void sigint_clear(void);
void sigint_allow(void);
void sigint_restore(void);
int sigint_caught(void);

#endif
