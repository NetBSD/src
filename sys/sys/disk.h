/*	$NetBSD: disk.h,v 1.33.6.1 2006/04/22 11:40:18 simonb Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Header: disk.h,v 1.5 92/11/19 04:33:03 torek Exp  (LBL)
 *
 *	@(#)disk.h	8.2 (Berkeley) 1/9/95
 */

#ifndef _SYS_DISK_H_
#define _SYS_DISK_H_

/*
 * Disk device structures.
 */

#include <sys/dkio.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/iostat.h>

struct buf;
struct disk;
struct disklabel;
struct cpu_disklabel;
struct vnode;

/*
 * dkwedge_info:
 *
 *	Information needed to configure (or query configuration of) a
 *	disk wedge.
 */
struct dkwedge_info {
	char		dkw_devname[16];/* device-style name (e.g. "dk0") */
	uint8_t		dkw_wname[128];	/* wedge name (Unicode, UTF-8) */
	char		dkw_parent[16];	/* parent disk device name */
	daddr_t		dkw_offset;	/* LBA offset of wedge in parent */
	uint64_t	dkw_size;	/* size of wedge in blocks */
	char		dkw_ptype[32];	/* partition type string */
};

/*
 * dkwedge_list:
 *
 *	Structure used to query a list of wedges.
 */
struct dkwedge_list {
	void		*dkwl_buf;	/* storage for dkwedge_info array */
	size_t		dkwl_bufsize;	/* size of that buffer */
	u_int		dkwl_nwedges;	/* total number of wedges */
	u_int		dkwl_ncopied;	/* number actually copied */
};

#ifdef _KERNEL
/*
 * dkwedge_discovery_method:
 *
 *	Structure used to describe partition map parsing schemes
 *	used for wedge autodiscovery.
 */
struct dkwedge_discovery_method {
					/* link in wedge driver's list */
	LIST_ENTRY(dkwedge_discovery_method) ddm_list;
	const char	*ddm_name;	/* name of this method */
	int		ddm_priority;	/* search priority */
	int		(*ddm_discover)(struct disk *, struct vnode *);
};

#define	DKWEDGE_DISCOVERY_METHOD_DECL(name, prio, discover)		\
static struct dkwedge_discovery_method name ## _ddm = {			\
	{ 0 },								\
	#name,								\
	prio,								\
	discover							\
};									\
__link_set_add_data(dkwedge_methods, name ## _ddm)
#endif /* _KERNEL */

/* Some common partition types */
#define	DKW_PTYPE_UNKNOWN	""
#define	DKW_PTYPE_UNUSED	"unused"
#define	DKW_PTYPE_SWAP		"swap"
#define	DKW_PTYPE_FFS		"ffs"
#define	DKW_PTYPE_LFS		"lfs"
#define	DKW_PTYPE_EXT2FS	"ext2fs"
#define	DKW_PTYPE_ISO9660	"cd9660"
#define	DKW_PTYPE_AMIGADOS	"ados"
#define	DKW_PTYPE_APPLEHFS	"hfs"
#define	DKW_PTYPE_FAT		"msdos"
#define	DKW_PTYPE_FILECORE	"filecore"
#define	DKW_PTYPE_RAIDFRAME	"raidframe"
#define	DKW_PTYPE_CCD		"ccd"
#define	DKW_PTYPE_APPLEUFS	"appleufs"
#define	DKW_PTYPE_NTFS		"ntfs"

/*
 * Disk geometry information.
 *
 * NOTE: Not all geometry information is relevant for every kind of disk.
 */
struct disk_geom {
	int64_t		dg_secperunit;	/* # of data sectors per unit */
	uint32_t	dg_secsize;	/* # of bytes per sector */
	uint32_t	dg_nsectors;	/* # of data sectors per track */
	uint32_t	dg_ntracks;	/* # of tracks per cylinder */
	uint32_t	dg_ncylinders;	/* # of data cylinders per unit */
	uint32_t	dg_secpercyl;	/* # of data sectors per cylinder */
	uint32_t	dg_pcylinders;	/* # of physical cylinders per unit */

	/*
	 * Spares (bad sector replacements) below are not counted in
	 * dg_nsectors or dg_secpercyl.  Spare sectors are assumed to
	 * be physical sectors which occupy space at the end of each
	 * track and/or cylinder.
	 */
	uint32_t	dg_sparespertrack;
	uint32_t	dg_sparespercyl;
	/*
	 * Alternative cylinders include maintenance, replacement,
	 * configuration description areas, etc.
	 */
	uint32_t	dg_acylinders;
};

struct disk {
	TAILQ_ENTRY(disk) dk_link;	/* link in global disklist */
	char		*dk_name;	/* disk name */
	int		dk_bopenmask;	/* block devices open */
	int		dk_copenmask;	/* character devices open */
	int		dk_openmask;	/* composite (bopen|copen) */
	int		dk_state;	/* label state   ### */
	int		dk_blkshift;	/* shift to convert DEV_BSIZE to blks */
	int		dk_byteshift;	/* shift to convert bytes to blks */

	/*
	 * Metrics data; note that some metrics may have no meaning
	 * on certain types of disks.
	 */
	struct io_stats	*dk_stats;

	struct	dkdriver *dk_driver;	/* pointer to driver */

	/*
	 * Information required to be the parent of a disk wedge.
	 */
	struct lock	dk_rawlock;	/* lock on these fields */
	struct vnode	*dk_rawvp;	/* vnode for the RAW_PART bdev */
	u_int		dk_rawopens;	/* # of openes of rawvp */

	struct lock	dk_openlock;	/* lock on these and openmask */
	u_int		dk_nwedges;	/* # of configured wedges */
					/* all wedges on this disk */
	LIST_HEAD(, dkwedge_softc) dk_wedges;

	/*
	 * Disk label information.  Storage for the in-core disk label
	 * must be dynamically allocated, otherwise the size of this
	 * structure becomes machine-dependent.
	 */
	daddr_t		dk_labelsector;		/* sector containing label */
	struct disklabel *dk_label;	/* label */
	struct cpu_disklabel *dk_cpulabel;
};

struct dkdriver {
	void	(*d_strategy)(struct buf *);
	void	(*d_minphys)(struct buf *);
#ifdef notyet
	int	(*d_open)(dev_t, int, int, struct proc *);
	int	(*d_close)(dev_t, int, int, struct proc *);
	int	(*d_ioctl)(dev_t, u_long, caddr_t, int, struct proc *);
	int	(*d_dump)(dev_t);
	void	(*d_start)(struct buf *, daddr_t);
	int	(*d_mklabel)(struct disk *);
#endif
};

/* states */
#define	DK_CLOSED	0		/* drive is closed */
#define	DK_WANTOPEN	1		/* drive being opened */
#define	DK_WANTOPENRAW	2		/* drive being opened */
#define	DK_RDLABEL	3		/* label being read */
#define	DK_OPEN		4		/* label read, drive open */
#define	DK_OPENRAW	5		/* open without label */

/*
 * Bad sector lists per fixed disk
 */
struct disk_badsectors {
	SLIST_ENTRY(disk_badsectors)	dbs_next;
	daddr_t		dbs_min;	/* min. sector number */
	daddr_t		dbs_max;	/* max. sector number */
	struct timeval	dbs_failedat;	/* first failure at */
};

struct disk_badsecinfo {
	uint32_t	dbsi_bufsize;	/* size of region pointed to */
	uint32_t	dbsi_skip;	/* how many to skip past */
	uint32_t	dbsi_copied;	/* how many got copied back */
	uint32_t	dbsi_left;	/* remaining to copy */
	caddr_t		dbsi_buffer;	/* region to copy disk_badsectors to */
};

#define	DK_STRATEGYNAMELEN	32
struct disk_strategy {
	char dks_name[DK_STRATEGYNAMELEN]; /* name of strategy */
	char *dks_param;		/* notyet; should be NULL */
	size_t dks_paramlen;		/* notyet; should be 0 */
};

#ifdef _KERNEL
extern	int disk_count;			/* number of disks in global disklist */

struct device;
struct proc;

void	disk_attach(struct disk *);
void	disk_detach(struct disk *);
void	pseudo_disk_init(struct disk *);
void	pseudo_disk_attach(struct disk *);
void	pseudo_disk_detach(struct disk *);
void	disk_busy(struct disk *);
void	disk_unbusy(struct disk *, long, int);
struct disk *disk_find(const char *);

int	dkwedge_add(struct dkwedge_info *);
int	dkwedge_del(struct dkwedge_info *);
void	dkwedge_delall(struct disk *);
int	dkwedge_list(struct disk *, struct dkwedge_list *, struct lwp *);
void	dkwedge_discover(struct disk *);
void	dkwedge_set_bootwedge(struct device *, daddr_t, uint64_t);
int	dkwedge_read(struct disk *, struct vnode *, daddr_t, void *, size_t);
#endif

#endif /* _SYS_DISK_H_ */
