/*	$NetBSD: defaults.h,v 1.1.1.2 2009/12/02 00:26:27 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_DEFAULTS_H
#define _LVM_DEFAULTS_H

#define DEFAULT_ARCHIVE_ENABLED 1
#define DEFAULT_BACKUP_ENABLED 1

#define DEFAULT_ARCHIVE_SUBDIR "archive"
#define DEFAULT_BACKUP_SUBDIR "backup"
#define DEFAULT_CACHE_SUBDIR "cache"
#define DEFAULT_CACHE_FILE_PREFIX ""

#define DEFAULT_ARCHIVE_DAYS 30
#define DEFAULT_ARCHIVE_NUMBER 10

#define DEFAULT_SYS_DIR "/etc/lvm"
#define DEFAULT_DEV_DIR "/dev"
#define DEFAULT_PROC_DIR "/proc"
#define DEFAULT_SYSFS_SCAN 1
#define DEFAULT_MD_COMPONENT_DETECTION 1
#define DEFAULT_MD_CHUNK_ALIGNMENT 1
#define DEFAULT_IGNORE_SUSPENDED_DEVICES 1
#define DEFAULT_DATA_ALIGNMENT_OFFSET_DETECTION 1
#define DEFAULT_DATA_ALIGNMENT_DETECTION 1

#define DEFAULT_LOCK_DIR "/var/lock/lvm"
#define DEFAULT_LOCKING_LIB "liblvm2clusterlock.so"
#define DEFAULT_FALLBACK_TO_LOCAL_LOCKING 1
#define DEFAULT_FALLBACK_TO_CLUSTERED_LOCKING 1
#define DEFAULT_WAIT_FOR_LOCKS 1
#define DEFAULT_PRIORITISE_WRITE_LOCKS 1

#define DEFAULT_MIRRORLOG "disk"
#define DEFAULT_MIRROR_LOG_FAULT_POLICY "allocate"
#define DEFAULT_MIRROR_DEV_FAULT_POLICY "remove"
#define DEFAULT_DMEVENTD_MIRROR_LIB "libdevmapper-event-lvm2mirror.so"
#define DEFAULT_DMEVENTD_MONITOR 1

#define DEFAULT_UMASK 0077

#ifdef LVM1_FALLBACK
#  define DEFAULT_FALLBACK_TO_LVM1 1
#else
#  define DEFAULT_FALLBACK_TO_LVM1 0
#endif

#define DEFAULT_FORMAT "lvm2"

#define DEFAULT_STRIPESIZE 64	/* KB */
#define DEFAULT_PVMETADATASIZE 255
#define DEFAULT_PVMETADATACOPIES 1
#define DEFAULT_LABELSECTOR UINT64_C(1)
#define DEFAULT_READ_AHEAD "auto"
#define DEFAULT_UDEV_SYNC 0
#define DEFAULT_EXTENT_SIZE 4096	/* In KB */
#define DEFAULT_MAX_PV 0
#define DEFAULT_MAX_LV 0
#define DEFAULT_ALLOC_POLICY ALLOC_NORMAL
#define DEFAULT_CLUSTERED 0

#define DEFAULT_MSG_PREFIX "  "
#define DEFAULT_CMD_NAME 0
#define DEFAULT_OVERWRITE 0

#ifndef DEFAULT_LOG_FACILITY
#  define DEFAULT_LOG_FACILITY LOG_USER
#endif

#define DEFAULT_SYSLOG 1
#define DEFAULT_VERBOSE 0
#define DEFAULT_LOGLEVEL 0
#define DEFAULT_INDENT 1
#define DEFAULT_UNITS "h"
#define DEFAULT_SUFFIX 1
#define DEFAULT_HOSTTAGS 0

#ifndef DEFAULT_SI_UNIT_CONSISTENCY
#  define DEFAULT_SI_UNIT_CONSISTENCY 1
#endif

#ifdef DEVMAPPER_SUPPORT
#  define DEFAULT_ACTIVATION 1
#  define DEFAULT_RESERVED_MEMORY 8192
#  define DEFAULT_RESERVED_STACK 256
#  define DEFAULT_PROCESS_PRIORITY -18
#else
#  define DEFAULT_ACTIVATION 0
#endif

#define DEFAULT_STRIPE_FILLER "error"
#define DEFAULT_MIRROR_REGION_SIZE 512	/* KB */
#define DEFAULT_INTERVAL 15

#ifdef READLINE_SUPPORT
#  define DEFAULT_MAX_HISTORY 100
#endif

#define DEFAULT_REP_ALIGNED 1
#define DEFAULT_REP_BUFFERED 1
#define DEFAULT_REP_COLUMNS_AS_ROWS 0
#define DEFAULT_REP_HEADINGS 1
#define DEFAULT_REP_PREFIXES 0
#define DEFAULT_REP_QUOTED 1
#define DEFAULT_REP_SEPARATOR " "

#define DEFAULT_LVS_COLS "lv_name,vg_name,lv_attr,lv_size,origin,snap_percent,move_pv,mirror_log,copy_percent,convert_lv"
#define DEFAULT_VGS_COLS "vg_name,pv_count,lv_count,snap_count,vg_attr,vg_size,vg_free"
#define DEFAULT_PVS_COLS "pv_name,vg_name,pv_fmt,pv_attr,pv_size,pv_free"
#define DEFAULT_SEGS_COLS "lv_name,vg_name,lv_attr,stripes,segtype,seg_size"
#define DEFAULT_PVSEGS_COLS "pv_name,vg_name,pv_fmt,pv_attr,pv_size,pv_free,pvseg_start,pvseg_size"

#define DEFAULT_LVS_COLS_VERB "lv_name,vg_name,seg_count,lv_attr,lv_size,lv_major,lv_minor,lv_kernel_major,lv_kernel_minor,origin,snap_percent,move_pv,copy_percent,mirror_log,convert_lv,lv_uuid"
#define DEFAULT_VGS_COLS_VERB "vg_name,vg_attr,vg_extent_size,pv_count,lv_count,snap_count,vg_size,vg_free,vg_uuid"
#define DEFAULT_PVS_COLS_VERB "pv_name,vg_name,pv_fmt,pv_attr,pv_size,pv_free,dev_size,pv_uuid"
#define DEFAULT_SEGS_COLS_VERB "lv_name,vg_name,lv_attr,seg_start,seg_size,stripes,segtype,stripesize,chunksize"
#define DEFAULT_PVSEGS_COLS_VERB "pv_name,vg_name,pv_fmt,pv_attr,pv_size,pv_free,pvseg_start,pvseg_size,lv_name,seg_start_pe,segtype,seg_pe_ranges"

#define DEFAULT_LVS_SORT "vg_name,lv_name"
#define DEFAULT_VGS_SORT "vg_name"
#define DEFAULT_PVS_SORT "pv_name"
#define DEFAULT_SEGS_SORT "vg_name,lv_name,seg_start"
#define DEFAULT_PVSEGS_SORT "pv_name,pvseg_start"

#define DEFAULT_MIRROR_DEVICE_FAULT_POLICY "remove"
#define DEFAULT_MIRROR_LOG_FAULT_POLICY "allocate"

#endif				/* _LVM_DEFAULTS_H */
