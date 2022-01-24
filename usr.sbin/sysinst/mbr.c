/*	$NetBSD: mbr.c,v 1.41 2022/01/24 09:14:38 andvar Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Following applies to the geometry guessing code
 */

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/* mbr.c -- DOS Master Boot Record editing code */

#ifdef HAVE_MBR

#include <sys/param.h>
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <paths.h>
#include <sys/ioctl.h>
#include "defs.h"
#include "mbr.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "defsizes.h"
#include "endian.h"

#define NO_BOOTMENU (-0x100)

#define MAXCYL		1023    /* Possibly 1024 */
#define MAXHEAD		255     /* Possibly 256 */
#define MAXSECTOR	63


#define	MBR_UNKNOWN_PTYPE	94	/* arbitrary not widely used value */


/* A list of predefined partition types */
const struct {
	unsigned int ptype;
	char short_desc[12];
	const char *desc;
} mbr_part_types_src[] = {
	{ .ptype=MBR_PTYPE_NETBSD, .desc="NetBSD" },
	{ .ptype=MBR_PTYPE_FAT32L, .desc="Windows FAT32, LBA" },
	{ .ptype=MBR_PTYPE_EXT_LBA, .desc="Extended partition, LBA" },
	{ .ptype=MBR_PTYPE_386BSD, .desc="FreeBSD/386BSD" },
	{ .ptype=MBR_PTYPE_OPENBSD, .desc="OpenBSD"  },
	{ .ptype=MBR_PTYPE_LNXEXT2, .desc="Linux native" },
	{ .ptype=MBR_PTYPE_LNXSWAP, .desc="Linux swap" },
	{ .ptype=MBR_PTYPE_NTFSVOL, .desc="NTFS volume set" },
	{ .ptype=MBR_PTYPE_NTFS, .desc="NTFS" },
	{ .ptype=MBR_PTYPE_PREP, .desc="PReP Boot" },
#ifdef MBR_PTYPE_SOLARIS
	{ .ptype=MBR_PTYPE_SOLARIS, .desc="Solaris" },
#endif
	{ .ptype=MBR_PTYPE_FAT12, .desc="DOS FAT12" },
	{ .ptype=MBR_PTYPE_FAT16S, .desc="DOS FAT16, <32M" },
	{ .ptype=MBR_PTYPE_FAT16B, .desc="DOS FAT16, >32M" },
	{ .ptype=MBR_PTYPE_FAT16L, .desc="Windows FAT16, LBA" },
	{ .ptype=MBR_PTYPE_FAT32, .desc="Windows FAT32" },
	{ .ptype=MBR_PTYPE_EFI, .desc="(U)EFI Boot" },
};

/* bookeeping of available partition types (including custom ones) */
struct mbr_part_type_info {
	struct part_type_desc gen;	/* generic description */
	char desc_buf[40], short_buf[10];
	size_t next_ptype;		/* user interface order */
};

const struct disk_partitioning_scheme mbr_parts;
struct mbr_disk_partitions {
	struct disk_partitions dp, *dlabel;
	mbr_info_t mbr;
	uint ptn_alignment, ptn_0_offset, ext_ptn_alignment,
	    geo_sec, geo_head, geo_cyl, target;
};

const struct disk_partitioning_scheme mbr_parts;

static size_t known_part_types = 0, last_added_part_type = 0;
static const size_t first_part_type = MBR_PTYPE_NETBSD;

/* all partition types (we are lucky, only a fixed number is possible) */
struct mbr_part_type_info mbr_gen_type_desc[256];

extern const struct disk_partitioning_scheme disklabel_parts;

static void convert_mbr_chs(int, int, int, uint8_t *, uint8_t *,
				 uint8_t *, uint32_t);

static part_id mbr_add_part(struct disk_partitions *arg,
    const struct disk_part_info *info, const char **errmsg);

static size_t mbr_get_free_spaces(const struct disk_partitions *arg,
    struct disk_part_free_space *result, size_t max_num_result,
    daddr_t min_size, daddr_t align, daddr_t lower_bound, daddr_t ignore);

static size_t mbr_type_from_gen_desc(const struct part_type_desc *desc);

/*
 * Notes on the extended partition editor.
 *
 * The extended partition structure is actually a singly linked list.
 * Each of the 'mbr' sectors can only contain 2 items, the first describes
 * a user partition (relative to that mbr sector), the second describes
 * the following partition (relative to the start of the extended partition).
 *
 * The 'start' sector for the user partition is always the size of one
 * track - very often 63.  The extended partitions themselves should
 * always start on a cylinder boundary using the BIOS geometry - often
 * 16065 sectors per cylinder.
 *
 * The disk is also always described in increasing sector order.
 *
 * During editing we keep the mbr sectors accurate (it might have been
 * easier to use absolute sector numbers though), and keep a copy of the
 * entire sector - to preserve any information any other OS has tried
 * to squirrel away in the (apparently) unused space.
 *
 * Typical disk (with some small numbers):
 *
 *      0 -> a       63       37	dos
 *           b      100     1000	extended LBA (type 15)
 *
 *    100 -> a       63       37        user
 *           b      100      200	extended partition (type 5)
 *
 *    200 -> a       63       37        user
 *           b      200      300	extended partition (type 5)
 *
 *    300 -> a       63       37	user
 *           b        0        0        0 (end of chain)
 *
 */

#ifndef debug_extended
#define dump_mbr(mbr, msg)
#else
static void
dump_mbr(mbr_info_t *m, const char *label)
{
	int i;
	uint ext_base = 0;

	fprintf(stderr, "\n%s: bsec %d\n", label, bsec);
	do {
		fprintf(stderr, "%p: %12u %p\n",
		    m, m->sector, m->extended);
		for (i = 0; i < MBR_PART_COUNT; i++) {
			fprintf(stderr, " %*d %12u %12u %12" PRIu64,
			    10,
			    m->mbr.mbr_parts[i].mbrp_type,
			    m->mbr.mbr_parts[i].mbrp_start,
			    m->mbr.mbr_parts[i].mbrp_size,
			    (uint64_t)m->mbr.mbr_parts[i].mbrp_start +
				(uint64_t)m->mbr.mbr_parts[i].mbrp_size);
			if (m->sector == 0 &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				ext_base = m->mbr.mbr_parts[i].mbrp_start;
			if (m->sector > 0 &&
			    m->mbr.mbr_parts[i].mbrp_size > 0) {
				uint off = MBR_IS_EXTENDED(
				    m->mbr.mbr_parts[i].mbrp_type)
				    ? ext_base : m->sector;
				fprintf(stderr, " -> [%u .. %u]",
				    m->mbr.mbr_parts[i].mbrp_start + off,
				    m->mbr.mbr_parts[i].mbrp_size +
				    m->mbr.mbr_parts[i].mbrp_start + off);
			}
			fprintf(stderr, ",\n");
			if (m->sector > 0 && i >= 1)
				break;
		}
	} while ((m = m->extended));
}
#endif

/*
 * Like pread, but handles re-blocking for non 512 byte sector disks
 */
static ssize_t
blockread(int fd, size_t secsize, void *buf, size_t nbytes, off_t offset)
{
        ssize_t nr;
	off_t sector = offset / 512;
        off_t offs = sector * (off_t)secsize;
        off_t mod = offs & (secsize - 1);
        off_t rnd = offs & ~(secsize - 1);
	char *iobuf;

	assert(nbytes <= 512);

	if (secsize == 512)
		return pread(fd, buf, nbytes, offset);

	iobuf = malloc(secsize);
	if (iobuf == NULL)
		return -1;
	nr = pread(fd, iobuf, secsize, rnd);
	if (nr == (off_t)secsize)
		memcpy(buf, &iobuf[mod], nbytes);
	free(iobuf);

	return nr == (off_t)secsize ? (off_t)nbytes : -1;
}

/*
 * Same for pwrite
 */
static ssize_t
blockwrite(int fd, size_t secsize, const void *buf, size_t nbytes,
    off_t offset)
{
        ssize_t nr;
	off_t sector = offset / secsize;
        off_t offs = sector * (off_t)secsize;
        off_t mod = offs & (secsize - 1);
        off_t rnd = offs & ~(secsize - 1);
	char *iobuf;

	assert(nbytes <= 512);

	if (secsize == 512)
		return pwrite(fd, buf, nbytes, offset);

	iobuf = malloc(secsize);
	if (iobuf == NULL)
		return -1;
	nr = pread(fd, iobuf, secsize, rnd);
	if (nr == (off_t)secsize) {
		memcpy(&iobuf[mod], buf, nbytes);
		nr = pwrite(fd, iobuf, secsize, rnd);
	}
	free(iobuf);

	return nr == (off_t)secsize ? (off_t)nbytes : -1;
}

static void
free_last_mounted(mbr_info_t *m)
{
	size_t i;

	for (i = 0; i < MBR_PART_COUNT; i++)
		free(__UNCONST(m->last_mounted[i]));
}

static void
free_mbr_info(mbr_info_t *m)
{
	if (m == NULL)
		return;
	free_last_mounted(m);
	free(m);
}

/*
 * To be used only on ports which cannot provide any bios geometry
 */
int
set_bios_geom_with_mbr_guess(struct disk_partitions *parts)
{
	int cyl, head, sec;

	msg_fmt_display(MSG_nobiosgeom, "%d%d%d",
	    pm->dlcyl, pm->dlsec, pm->dlhead);
	if (guess_biosgeom_from_parts(parts, &cyl, &head, &sec) >= 0)
		msg_fmt_display_add(MSG_biosguess, "%d%d%d", cyl, head, sec);
	set_bios_geom(parts, &cyl, &head, &sec);
	if (parts->pscheme->change_disk_geom)
		parts->pscheme->change_disk_geom(parts, cyl, head, sec);

	return edit_outer_parts(parts);
}

static void
mbr_init_chs(struct mbr_disk_partitions *parts, int ncyl, int nhead, int nsec)
{
	if (ncyl > MAXCYL)
		ncyl = MAXCYL;
	pm->current_cylsize = nhead*nsec;
	pm->max_chs = (unsigned long)ncyl*nhead*nsec;
	parts->geo_sec = nsec;
	parts->geo_head = nhead;
	parts->geo_cyl = ncyl;
}

/*
 * get C/H/S geometry from user via menu interface and
 * store in globals.
 */
void
set_bios_geom(struct disk_partitions *parts, int *cyl, int *head, int *sec)
{
	char res[80];
	int bsec, bhead, bcyl;
	daddr_t s;

	if (parts->pscheme->change_disk_geom == NULL)
		return;

	msg_display_add(MSG_setbiosgeom);

	do {
		snprintf(res, 80, "%d", *sec);
		msg_prompt_add(MSG_sectors, res, res, 80);
		bsec = atoi(res);
	} while (bsec <= 0 || bsec > MAXSECTOR);

	do {
		snprintf(res, 80, "%d", *head);
		msg_prompt_add(MSG_heads, res, res, 80);
		bhead = atoi(res);
	} while (bhead <= 0 || bhead > MAXHEAD);
	s = min(pm->dlsize, mbr_parts.size_limit);
	bcyl = s / bsec / bhead;
	if (s != bcyl * bsec * bhead)
		bcyl++;
	if (bcyl > MAXCYL)
		bcyl = MAXCYL;
	pm->max_chs = (unsigned long)bcyl * bhead * bsec;
	pm->current_cylsize = bhead * bsec;
	parts->pscheme->change_disk_geom(parts, bcyl, bhead, bsec);
	*cyl = bcyl;
	*head = bhead;
	*sec = bsec;
}

static int
find_mbr_space(const struct mbr_info_t *mbrs, uint *start, uint *size,
    uint from, uint end_of_disk, uint ignore_at, bool primary_only)
{
	uint sz;
	int i, j;
	uint s, e;
	const mbr_info_t *m, *me;
	bool is_extended;

    check_again:
	m = mbrs;
	sz = end_of_disk-from;
	if (from >= end_of_disk)
		return -1;

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
			continue;

		is_extended = MBR_IS_EXTENDED(
		    m->mbr.mbr_parts[i].mbrp_type);

		s = m->mbr.mbr_parts[i].mbrp_start + m->sector;
		if (s == ignore_at)
			continue;
		e = s + m->mbr.mbr_parts[i].mbrp_size;
		if (s <= from && e > from
		    && (!is_extended || primary_only)) {
			if (e - 1 >= end_of_disk)
				break;
			if (e >= UINT_MAX) {
				sz = 0;
				break;
			}
			from = e + 1;
			goto check_again;
		}
		if (s <= from && e > from && is_extended) {
			/*
			 * if we start in the extended partition,
			 * we must end before its end
			 */
			sz = e - from;
		}
		if (s > from && s - from < sz)
			sz = s - from;

		if (is_extended) {
			for (me = m->extended; me != NULL; me = me->extended) {
				for (j = 0; j < MBR_PART_COUNT; j++) {
					if (me->mbr.mbr_parts[j].mbrp_type ==
					    MBR_PTYPE_UNUSED)
						continue;

					is_extended = MBR_IS_EXTENDED(
					    me->mbr.mbr_parts[j].mbrp_type);

					if (is_extended && i > 0)
						break;

					s = me->mbr.mbr_parts[j].mbrp_start +
					    me->sector;
					if (s == ignore_at)
						continue;
					e = s + me->mbr.mbr_parts[j].mbrp_size;
					if (me->sector != 0 && me->sector< s)
						/*
						 * can not allow to overwrite
						 * the ext mbr
						 */
						s = me->sector;
					if (s <= from && e > from) {
						if (e - 1 >= end_of_disk)
							break;
						from = e + 1;
						goto check_again;
					}
					if (s > from && s - from < sz)
						sz = s - from;

				}
			}
		}
	}

	if (sz == 0)
		return -1;
	if (start != NULL)
		*start = from;
	if (size != NULL)
		*size = sz;
	return 0;
}

#ifdef BOOTSEL
static int
validate_and_set_names(mbr_info_t *mbri, const struct mbr_bootsel *src,
    uint32_t ext_base)
{
	size_t i, l;
	const unsigned char *p;

	/*
	 * The 16 bit magic used to detect whether mbr_bootsel is valid
	 * or not is pretty week - collisions have been seen in the wild;
	 * but maybe it is just foreign tools corruption reminiscents
	 * of NetBSD MBRs. Anyway, before accepting a boot menu definition,
	 * make sure it is kinda "sane".
	 */

	for (i = 0; i < MBR_PART_COUNT; i++) {
		/*
		 * Make sure the name does not contain controll chars
		 * (not using iscntrl due to minimalistic locale support
		 * in miniroot environments) and is properly 0-terminated.
		 */
		for (l = 0, p = (const unsigned char *)&src->mbrbs_nametab[i];
		    *p != 0; l++, p++) {
			if (l >	MBR_BS_PARTNAMESIZE)
				return 0;
			if (*p < ' ')	/* hacky 'iscntrl' */
				return 0;
		}
	}

	memcpy(&mbri->mbrb, src, sizeof(*src));

	if (ext_base == 0)
		return mbri->mbrb.mbrbs_defkey - SCAN_1;
	return 0;
}
#endif

static int
valid_mbr(struct mbr_sector *mbrs)
{

	return (le16toh(mbrs->mbr_magic) == MBR_MAGIC);
}

static int
read_mbr(const char *disk, size_t secsize, mbr_info_t *mbri,
     struct mbr_disk_partitions *parts)
{
	struct mbr_partition *mbrp;
	struct mbr_sector *mbrs = &mbri->mbr;
	mbr_info_t *ext = NULL;
	char diskpath[MAXPATHLEN];
	int fd, i;
	uint32_t ext_base = 0, next_ext = 0;
	int rval = -1;
#ifdef BOOTSEL
	mbr_info_t *ombri = mbri;
	int bootkey = 0;
#endif

	memset(mbri, 0, sizeof *mbri);

	/* Open the disk. */
	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		goto bad_mbr;

	for (;;) {
		if (blockread(fd, secsize, mbrs, sizeof *mbrs,
		    (ext_base + next_ext) * (off_t)MBR_SECSIZE)
		    - sizeof *mbrs != 0)
			break;

		if (!valid_mbr(mbrs))
			break;

		mbrp = &mbrs->mbr_parts[0];
		if (ext_base != 0) {
			/* sanity check extended chain */
			if (MBR_IS_EXTENDED(mbrp[0].mbrp_type))
				break;
			if (mbrp[1].mbrp_type != MBR_PTYPE_UNUSED &&
			    !MBR_IS_EXTENDED(mbrp[1].mbrp_type))
				break;
			if (mbrp[2].mbrp_type != MBR_PTYPE_UNUSED
			    || mbrp[3].mbrp_type != MBR_PTYPE_UNUSED)
				break;
			/* Looks ok, link into extended chain */
			mbri->extended = ext;
			ext->extended = NULL;
			mbri = ext;
			ext = NULL;
		}
#if BOOTSEL
		if (mbrs->mbr_bootsel_magic == htole16(MBR_MAGIC)) {
			/* old bootsel, grab bootsel info */
			bootkey = validate_and_set_names(mbri,
				(struct mbr_bootsel *)
				((uint8_t *)mbrs + MBR_BS_OLD_OFFSET),
				ext_base);
		} else if (mbrs->mbr_bootsel_magic == htole16(MBR_BS_MAGIC)) {
			/* new location */
			bootkey = validate_and_set_names(mbri,
			    &mbrs->mbr_bootsel, ext_base);
		}
		/* Save original flags for mbr code update tests */
		mbri->oflags = mbri->mbrb.mbrbs_flags;
#endif
		mbri->sector = next_ext + ext_base;
		next_ext = 0;
		rval = 0;
		for (i = 0; i < MBR_PART_COUNT; mbrp++, i++) {
			if (mbrp->mbrp_type == MBR_PTYPE_UNUSED) {
				/* type is unused, discard scum */
				memset(mbrp, 0, sizeof *mbrp);
				continue;
			}
			mbrp->mbrp_start = le32toh(mbrp->mbrp_start);
			mbrp->mbrp_size = le32toh(mbrp->mbrp_size);
			if (MBR_IS_EXTENDED(mbrp->mbrp_type)) {
				next_ext = mbrp->mbrp_start;
			} else {
				uint flags = 0;
				if (mbrp->mbrp_type == MBR_PTYPE_NETBSD) {
					flags |= GLM_LIKELY_FFS;
					if (parts->target == ~0U)
						parts->target =
						    mbri->sector +
						    mbrp->mbrp_start;
				} else if (mbrp->mbrp_type == MBR_PTYPE_FAT12 ||
				    mbrp->mbrp_type == MBR_PTYPE_FAT16S ||
				    mbrp->mbrp_type == MBR_PTYPE_FAT16B ||
				    mbrp->mbrp_type == MBR_PTYPE_FAT32 ||
				    mbrp->mbrp_type == MBR_PTYPE_FAT32L ||
				    mbrp->mbrp_type == MBR_PTYPE_FAT16L)
					flags |= GLM_MAYBE_FAT32;
				else if (mbrp->mbrp_type == MBR_PTYPE_NTFS)
					flags |= GLM_MAYBE_NTFS;
				if (flags != 0) {
					const char *mount = get_last_mounted(
					    fd, mbri->sector + mbrp->mbrp_start,
					    &mbri->fs_type[i],
					    &mbri->fs_sub_type[i],
					    flags);
					char *p = strdup(mount);
					canonicalize_last_mounted(p);
					mbri->last_mounted[i] = p;
				}
			}
#if BOOTSEL
			if (mbri->mbrb.mbrbs_nametab[i][0] != 0
			    && bootkey-- == 0)
				ombri->bootsec = mbri->sector +
							mbrp->mbrp_start;
#endif
		}

		if (next_ext == 0 || ext_base + next_ext <= mbri->sector)
			break;
		if (ext_base == 0) {
			ext_base = next_ext;
			next_ext = 0;
		}
		ext = calloc(1, sizeof *ext);
		if (!ext)
			break;
		mbrs = &ext->mbr;
	}

    bad_mbr:
	free_mbr_info(ext);
	if (fd >= 0)
		close(fd);
	if (rval == -1) {
		memset(&mbrs->mbr_parts, 0, sizeof mbrs->mbr_parts);
		mbrs->mbr_magic = htole16(MBR_MAGIC);
	}
	dump_mbr(ombri, "read");
	return rval;
}

static int
write_mbr(const char *disk, size_t secsize, mbr_info_t *mbri, int bsec,
    int bhead, int bcyl)
{
	char diskpath[MAXPATHLEN];
	int fd, i, ret = 0, bits = 0;
	struct mbr_partition *mbrp;
	u_int32_t pstart, psize;
#ifdef BOOTSEL
	struct mbr_sector *mbrs;
#endif
	struct mbr_sector mbrsec;
	mbr_info_t *ext;
	uint sector;

	dump_mbr(mbri, "write");

	/* Open the disk. */
	fd = opendisk(disk, secsize == 512 ? O_WRONLY : O_RDWR,
	    diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return -1;

	/* Remove all wedges */
	if (ioctl(fd, DIOCRMWEDGES, &bits) == -1)
		return -1;

#ifdef BOOTSEL
	/*
	 * If the main boot code (appears to) contain the netbsd bootcode,
	 * copy in all the menu strings and set the default keycode
	 * to be that for the default partition.
	 * Unfortunately we can't rely on the user having actually updated
	 * to the new mbr code :-(
	 */
	if (mbri->mbr.mbr_bootsel_magic == htole16(MBR_BS_MAGIC)
	    || mbri->mbr.mbr_bootsel_magic == htole16(MBR_MAGIC)) {
		int8_t key = SCAN_1;
		uint offset = MBR_BS_OFFSET;
		if (mbri->mbr.mbr_bootsel_magic == htole16(MBR_MAGIC))
			offset = MBR_BS_OLD_OFFSET;
		mbri->mbrb.mbrbs_defkey = SCAN_ENTER;
		if (mbri->mbrb.mbrbs_timeo == 0)
			mbri->mbrb.mbrbs_timeo = 182;	/* 10 seconds */
		for (ext = mbri; ext != NULL; ext = ext->extended) {
			mbrs = &ext->mbr;
			mbrp = &mbrs->mbr_parts[0];
			/* Ensure marker is set in each sector */
			mbrs->mbr_bootsel_magic = mbri->mbr.mbr_bootsel_magic;
			/* and copy in bootsel parameters */
			*(struct mbr_bootsel *)((uint8_t *)mbrs + offset) =
								    ext->mbrb;
			for (i = 0; i < MBR_PART_COUNT; i++) {
				if (ext->mbrb.mbrbs_nametab[i][0] == 0)
					continue;
				if (ext->sector + mbrp->mbrp_start ==
								mbri->bootsec)
					mbri->mbrb.mbrbs_defkey = key;
				key++;
			}
		}
		/* copy main data (again) since we've put the 'key' in */
		*(struct mbr_bootsel *)((uint8_t *)&mbri->mbr + offset) =
								    mbri->mbrb;
	}
#endif

	for (ext = mbri; ext != NULL; ext = ext->extended) {
		memset(mbri->wedge, 0, sizeof mbri->wedge);
		sector = ext->sector;
		mbrsec = ext->mbr;	/* copy sector */
		mbrp = &mbrsec.mbr_parts[0];

		if (sector != 0 && ext->extended != NULL
		    && ext->extended->mbr.mbr_parts[0].mbrp_type
		    == MBR_PTYPE_UNUSED) {

			/*
			 * This should never happen nowadays, old code
			 * inserted empty ext sectors in the chain to
			 * help the gui out - we do not do that anymore.
			 */
			assert(false);

			/* We are followed by an empty slot, collapse out */
			ext = ext->extended;
			/* Make us describe the next non-empty partition */
			mbrp[1] = ext->mbr.mbr_parts[1];
		}

		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (mbrp[i].mbrp_start == 0 && mbrp[i].mbrp_size == 0) {
				mbrp[i].mbrp_scyl = 0;
				mbrp[i].mbrp_shd = 0;
				mbrp[i].mbrp_ssect = 0;
				mbrp[i].mbrp_ecyl = 0;
				mbrp[i].mbrp_ehd = 0;
				mbrp[i].mbrp_esect = 0;
				continue;
			}
			pstart = mbrp[i].mbrp_start;
			psize = mbrp[i].mbrp_size;
			mbrp[i].mbrp_start = htole32(pstart);
			mbrp[i].mbrp_size = htole32(psize);
			if (bsec && bcyl && bhead) {
				convert_mbr_chs(bcyl, bhead, bsec,
				    &mbrp[i].mbrp_scyl, &mbrp[i].mbrp_shd,
				    &mbrp[i].mbrp_ssect, pstart);
				convert_mbr_chs(bcyl, bhead, bsec,
				    &mbrp[i].mbrp_ecyl, &mbrp[i].mbrp_ehd,
				    &mbrp[i].mbrp_esect, pstart + psize - 1);
			}
		}

		mbrsec.mbr_magic = htole16(MBR_MAGIC);
		if (blockwrite(fd, secsize, &mbrsec, sizeof mbrsec,
					    sector * (off_t)MBR_SECSIZE) < 0) {
			ret = -1;
			break;
		}
	}

	(void)close(fd);
	return ret;
}

static void
convert_mbr_chs(int cyl, int head, int sec,
		uint8_t *cylp, uint8_t *headp, uint8_t *secp,
		uint32_t relsecs)
{
	unsigned int tcyl, temp, thead, tsec;

	temp = head * sec;
	tcyl = relsecs / temp;
	relsecs -= tcyl * temp;

	thead = relsecs / sec;
	tsec = relsecs - thead * sec + 1;

	if (tcyl > MAXCYL)
		tcyl = MAXCYL;

	*cylp = MBR_PUT_LSCYL(tcyl);
	*headp = thead;
	*secp = MBR_PUT_MSCYLANDSEC(tcyl, tsec);
}

/*
 * This function is ONLY to be used as a last resort to provide a
 * hint for the user. Ports should provide a more reliable way
 * of getting the BIOS geometry. The i386 code, for example,
 * uses the BIOS geometry as passed on from the bootblocks,
 * and only uses this as a hint to the user when that information
 * is not present, or a match could not be made with a NetBSD
 * device.
 */
int
guess_biosgeom_from_parts(struct disk_partitions *parts,
    int *cyl, int *head, int *sec)
{

	/*
	 * The physical parameters may be invalid as bios geometry.
	 * If we cannot determine the actual bios geometry, we are
	 * better off picking a likely 'faked' geometry than leaving
	 * the invalid physical one.
	 */

	int xcylinders = pm->dlcyl;
	int xheads = pm->dlhead;
	daddr_t xsectors = pm->dlsec;
	daddr_t xsize = min(pm->dlsize, mbr_parts.size_limit);
	if (xcylinders > MAXCYL || xheads > MAXHEAD || xsectors > MAXSECTOR) {
		xsectors = MAXSECTOR;
		xheads = MAXHEAD;
		xcylinders = xsize / (MAXSECTOR * MAXHEAD);
		if (xcylinders > MAXCYL)
			xcylinders = MAXCYL;
	}
	*cyl = xcylinders;
	*head = xheads;
	*sec = xsectors;

	if (parts->pscheme->guess_disk_geom == NULL)
		return -1;

	return parts->pscheme->guess_disk_geom(parts, cyl, head, sec);
}

static int
mbr_comp_part_entry(const void *a, const void *b)
{
	const struct mbr_partition *part_a = a,
		*part_b = b;

	if (part_a->mbrp_type == MBR_PTYPE_UNUSED
	    && part_b->mbrp_type != MBR_PTYPE_UNUSED)
		return 1;

	if (part_b->mbrp_type == MBR_PTYPE_UNUSED
	    && part_a->mbrp_type != MBR_PTYPE_UNUSED)
		return -1;

	return part_a->mbrp_start < part_b->mbrp_start ? -1 : 1;
}

static void
mbr_sort_main_mbr(struct mbr_sector *m)
{
	qsort(&m->mbr_parts[0], MBR_PART_COUNT,
	    sizeof(m->mbr_parts[0]), mbr_comp_part_entry);
}

static void
mbr_init_default_alignments(struct mbr_disk_partitions *parts, uint track)
{
	if (track == 0)
		track = 16065;

	assert(parts->dp.disk_size > 0);
	if (parts->dp.disk_size < 0)
		return;

	parts->ptn_0_offset = parts->geo_sec;

	/* Use 1MB offset/alignemnt for large (>128GB) disks */
	if (parts->dp.disk_size > HUGE_DISK_SIZE) {
		parts->ptn_alignment = 2048;
	} else if (parts->dp.disk_size > TINY_DISK_SIZE) {
		parts->ptn_alignment = 64;
	} else {
		parts->ptn_alignment = 1;
	}
	parts->ext_ptn_alignment = track;
}

static struct disk_partitions *
mbr_create_new(const char *disk, daddr_t start, daddr_t len,
    bool is_boot_drive, struct disk_partitions *parent)
{
	struct mbr_disk_partitions *parts;
	struct disk_geom geo;

	assert(start == 0);
	if (start != 0)
		return NULL;

	parts = calloc(1, sizeof(*parts));
	if (!parts)
		return NULL;

	parts->dp.pscheme = &mbr_parts;
	parts->dp.disk = strdup(disk);
	if (len > mbr_parts.size_limit)
		len = mbr_parts.size_limit;
	parts->dp.disk_start = start;
	parts->dp.disk_size = len;
	parts->dp.free_space = len-1;
	parts->dp.bytes_per_sector = 512;
	parts->geo_sec = MAXSECTOR;
	parts->geo_head = MAXHEAD;
	parts->geo_cyl = len/MAXHEAD/MAXSECTOR+1;
	parts->target = ~0U;

	if (get_disk_geom(disk, &geo)) {
		parts->geo_sec = geo.dg_nsectors;
		parts->geo_head = geo.dg_ntracks;
		parts->geo_cyl = geo.dg_ncylinders;
		parts->dp.bytes_per_sector = geo.dg_secsize;
	}

	mbr_init_default_alignments(parts, 0);

	return &parts->dp;
}

static void
mbr_calc_free_space(struct mbr_disk_partitions *parts)
{
	size_t i;
	mbr_info_t *m;

	parts->dp.free_space = parts->dp.disk_size - 1;
	parts->dp.num_part = 0;
	m = &parts->mbr;
	do {
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;

			if (m != &parts->mbr && i > 0 &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				break;

			parts->dp.num_part++;
			if (MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				continue;

			daddr_t psize = m->mbr.mbr_parts[i].mbrp_size;
			if (m != &parts->mbr)
				psize += m->mbr.mbr_parts[i].mbrp_start;

			if (psize > parts->dp.free_space)
				parts->dp.free_space = 0;
			else
				parts->dp.free_space -= psize;
		}
	} while ((m = m->extended));
}

static struct disk_partitions *
mbr_read_from_disk(const char *disk, daddr_t start, daddr_t len, size_t bps,
    const struct disk_partitioning_scheme *scheme)
{
	struct mbr_disk_partitions *parts;

	assert(start == 0);
	if (start != 0)
		return NULL;

	parts = calloc(1, sizeof(*parts));
	if (!parts)
		return NULL;

	parts->dp.pscheme = scheme;
	parts->dp.disk = strdup(disk);
	if (len >= mbr_parts.size_limit)
		len = mbr_parts.size_limit;
	parts->dp.disk_start = start;
	parts->dp.disk_size = len;
	parts->geo_sec = MAXSECTOR;
	parts->geo_head = MAXHEAD;
	parts->geo_cyl = len/MAXHEAD/MAXSECTOR+1;
	parts->dp.bytes_per_sector = bps;
	parts->target = ~0U;
	mbr_init_default_alignments(parts, 0);
	if (read_mbr(disk, parts->dp.bytes_per_sector, &parts->mbr, parts)
	     == -1) {
		free(parts);
		return NULL;
	}
	mbr_calc_free_space(parts);
	if (parts->dp.num_part == 1 &&
	    parts->dp.free_space < parts->ptn_alignment) {
		struct disk_part_info info;

		/*
		 * Check if this is a GPT protective MBR
		 */
		if (parts->dp.pscheme->get_part_info(&parts->dp, 0, &info)
		    && info.nat_type != NULL
		    && mbr_type_from_gen_desc(info.nat_type) == 0xEE) {
			parts->dp.pscheme->free(&parts->dp);
			return NULL;
		}
	}

	return &parts->dp;
}

static bool
mbr_write_to_disk(struct disk_partitions *new_state)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions *)new_state;
	unsigned long bsec, bhead, bcyl;
	daddr_t t;

	assert(parts->geo_sec != 0);
	if (parts->geo_sec != 0) {
		bsec = parts->geo_sec;
		bhead = parts->ext_ptn_alignment / bsec;
	} else {
		bsec = MAXSECTOR;
		bhead = MAXHEAD;
	}
	t = bsec * bhead;
	assert(t != 0);
	if ((daddr_t)(1UL<<10) * t <= parts->dp.disk_size)
		bcyl = (1UL<<10) - 1;
	else
		bcyl = (unsigned long)(parts->dp.disk_size / t);

	return write_mbr(parts->dp.disk, parts->dp.bytes_per_sector,
	    &parts->mbr, bsec, bhead, bcyl) == 0;
}

static bool
mbr_change_disk_geom(struct disk_partitions *arg, int ncyl, int nhead,
    int nsec)
{
	struct mbr_disk_partitions *parts = (struct mbr_disk_partitions *)arg;
	daddr_t ptn_0_base, ptn_0_limit;
	struct disk_part_info info;

	/* Default to using 'traditional' cylinder alignment */
	mbr_init_chs(parts, ncyl, nhead, nsec);
	mbr_init_default_alignments(parts, nhead * nsec);

	if (parts->dp.disk_size <= TINY_DISK_SIZE) {
		set_default_sizemult(arg->disk,
		    parts->dp.bytes_per_sector, parts->dp.bytes_per_sector);
		return true;
	}

	if (parts->dp.num_part > 0 &&
	    parts->dp.pscheme->get_part_info(arg, 0, &info)) {

		/* Try to copy offset of first partition */
		ptn_0_base = info.start;
		ptn_0_limit = info.start + info.size;
		if (!(ptn_0_limit & 2047)) {
			/* Partition ends on a 1MB boundary, align to 1MB */
			parts->ptn_alignment = 2048;
			if ((ptn_0_base <= 2048
			    && !(ptn_0_base & (ptn_0_base - 1)))
			    || (ptn_0_base < parts->ptn_0_offset)) {
				/*
				 * If ptn_base is a power of 2, use it.
				 * Also use it if the first partition
				 * already is close to the beginning
				 * of the disk and we can't enforce
				 * better alignment.
				 */
				parts->ptn_0_offset = ptn_0_base;
			}
		}
	}
	set_default_sizemult(arg->disk, MEG, parts->dp.bytes_per_sector);
	return true;
}

static size_t
mbr_type_from_gen_desc(const struct part_type_desc *desc)
{
	for (size_t i = 0; i < __arraycount(mbr_gen_type_desc); i++)
		if (&mbr_gen_type_desc[i].gen == desc)
			return i;

	return SIZE_T_MAX;
}

static enum part_type
mbr_map_part_type(unsigned int t)
{
	/* Map some special MBR partition types */
	switch (t) {
	case MBR_PTYPE_FAT32:
	case MBR_PTYPE_FAT32L:
	case MBR_PTYPE_FAT16S:
	case MBR_PTYPE_FAT16B:
	case MBR_PTYPE_FAT16L:
	case MBR_PTYPE_FAT12:
	case MBR_PTYPE_FT_FAT32:
	case MBR_PTYPE_FT_FAT32_EXT:
		return PT_FAT;
	case MBR_PTYPE_EFI:
		return PT_EFI_SYSTEM;
	case MBR_PTYPE_LNXEXT2:
		return PT_EXT2;
	case MBR_PTYPE_NETBSD:
		return PT_root;
	}

	return PT_unknown;
}

static void
map_mbr_part_types(void)
{

	for (size_t i = 0; i < __arraycount(mbr_part_types_src); i++) {
		unsigned int v = mbr_part_types_src[i].ptype;

		snprintf(mbr_gen_type_desc[v].short_buf,
		    sizeof(mbr_gen_type_desc[v].short_buf), "%u", v);
		mbr_gen_type_desc[v].gen.short_desc =
		    mbr_gen_type_desc[v].short_buf;
		mbr_gen_type_desc[v].gen.description =
		    mbr_part_types_src[i].desc;
		mbr_gen_type_desc[v].gen.generic_ptype = mbr_map_part_type(v);
		mbr_gen_type_desc[v].next_ptype = ~0U;
		mbr_gen_type_desc[last_added_part_type].next_ptype = v;
		known_part_types++;
		last_added_part_type = v;
	}
}

static size_t
mbr_get_part_type_count(void)
{
	if (known_part_types == 0)
		map_mbr_part_types();

	return known_part_types;
}

static const struct part_type_desc *
mbr_get_fs_part_type(enum part_type pt, unsigned fs_type, unsigned sub_type)
{
	if (known_part_types == 0)
		map_mbr_part_types();

	switch (fs_type) {
	case FS_BSDFFS:
		return &mbr_gen_type_desc[MBR_PTYPE_NETBSD].gen;
	case FS_EX2FS:
		return &mbr_gen_type_desc[MBR_PTYPE_LNXEXT2].gen;
	case FS_MSDOS:
		if (sub_type == 0)
			sub_type = MBR_PTYPE_FAT32L;

		switch (sub_type) {
		case MBR_PTYPE_FAT12:
		case MBR_PTYPE_FAT16S:
		case MBR_PTYPE_FAT16B:
		case MBR_PTYPE_FAT32:
		case MBR_PTYPE_FAT32L:
		case MBR_PTYPE_FAT16L:
			return &mbr_gen_type_desc[sub_type].gen;
		}
		break;
	}

	return NULL;
}

static const struct part_type_desc *
mbr_get_part_type(size_t index)
{
	size_t i, no;

	if (known_part_types == 0)
		map_mbr_part_types();

	if (index >= known_part_types)
		return NULL;

	for (i = first_part_type, no = 0; i < __arraycount(mbr_gen_type_desc)
	    && no != index;  no++)
		i = mbr_gen_type_desc[i].next_ptype;

	if (i >= __arraycount(mbr_gen_type_desc))
		return NULL;
	return &mbr_gen_type_desc[i].gen;
}

static const struct part_type_desc *
mbr_new_custom_part_type(unsigned int v)
{
	snprintf(mbr_gen_type_desc[v].short_buf,
	    sizeof(mbr_gen_type_desc[v].short_buf), "%u", v);
	snprintf(mbr_gen_type_desc[v].desc_buf,
	     sizeof(mbr_gen_type_desc[v].desc_buf), "%s (%u)",
	    msg_string(MSG_custom_type), v);
	mbr_gen_type_desc[v].gen.short_desc = mbr_gen_type_desc[v].short_buf;
	mbr_gen_type_desc[v].gen.description = mbr_gen_type_desc[v].desc_buf;
	mbr_gen_type_desc[v].gen.generic_ptype = mbr_map_part_type(v);
	mbr_gen_type_desc[v].next_ptype = ~0U;
	mbr_gen_type_desc[last_added_part_type].next_ptype = v;
	known_part_types++;
	last_added_part_type = v;

	return &mbr_gen_type_desc[v].gen;
}

static const struct part_type_desc *
mbr_custom_part_type(const char *custom, const char **err_msg)
{
	unsigned long v;
	char *endp;

	if (known_part_types == 0)
		map_mbr_part_types();

	v = strtoul(custom, &endp, 10);
	if (v > 255 || (v == 0 && *endp != 0)) {
		if (err_msg != NULL)
			*err_msg = msg_string(MSG_mbr_type_invalid);
		return NULL;
	}

	if (mbr_gen_type_desc[v].gen.short_desc != NULL)
		return &mbr_gen_type_desc[v].gen;

	return mbr_new_custom_part_type(v);
}

static const struct part_type_desc *
mbr_create_unknown_part_type(void)
{

	if (mbr_gen_type_desc[MBR_UNKNOWN_PTYPE].gen.short_desc != NULL)
		return &mbr_gen_type_desc[MBR_UNKNOWN_PTYPE].gen;

	return mbr_new_custom_part_type(MBR_UNKNOWN_PTYPE);
}

static const struct part_type_desc *
mbr_get_gen_type_desc(unsigned int pt)
{

	if (known_part_types == 0)
		map_mbr_part_types();

	if (pt >= __arraycount(mbr_gen_type_desc))
		return NULL;

	if (mbr_gen_type_desc[pt].gen.short_desc != NULL)
		return &mbr_gen_type_desc[pt].gen;

	return mbr_new_custom_part_type(pt);
}

static const struct part_type_desc *
mbr_get_generic_part_type(enum part_type pt)
{
	switch (pt) {
	case PT_root:
		return mbr_get_gen_type_desc(MBR_PTYPE_NETBSD);
	case PT_FAT:
		return mbr_get_gen_type_desc(MBR_PTYPE_FAT32L);
	case PT_EXT2:
		return mbr_get_gen_type_desc(MBR_PTYPE_LNXEXT2);
	case PT_EFI_SYSTEM:
		return mbr_get_gen_type_desc(MBR_PTYPE_EFI);
	default:
		break;
	}
	assert(false);
	return NULL;
}

static void
mbr_partition_to_info(const struct mbr_partition *mp, daddr_t start_off,
    struct disk_part_info *info)
{
	memset(info, 0, sizeof(*info));
	info->start = mp->mbrp_start + start_off;
	info->size = mp->mbrp_size;
	info->nat_type = mbr_get_gen_type_desc(mp->mbrp_type);
	if (mp->mbrp_type == MBR_PTYPE_NETBSD) {
		info->flags |= PTI_SEC_CONTAINER;
	} else if (MBR_IS_EXTENDED(mp->mbrp_type))
		info->flags |= PTI_PSCHEME_INTERNAL;
}

static bool
mbr_part_apply(const struct disk_partitions *arg, part_id id,
    bool (*func)(const struct disk_partitions *arg, part_id id,
	const mbr_info_t *mb, int i, bool primary,
	const struct mbr_partition *mp, void *),
    void *cookie)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	part_id i, j, no;
	const mbr_info_t *m = &parts->mbr, *me;

	no = 0;
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
			continue;

		if (no == id) {
			return func(arg, id, m, i, true,
			    &m->mbr.mbr_parts[i], cookie);
		}
		no++;

		if (MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type)) {
			for (me = m->extended; me != NULL; me = me->extended) {
				for (j = 0; j < MBR_PART_COUNT; j++) {
					if (me->mbr.mbr_parts[j].mbrp_type ==
					    MBR_PTYPE_UNUSED)
						continue;
					if (j > 0 && MBR_IS_EXTENDED(
					    me->mbr.mbr_parts[j].mbrp_type))
						break;
					if (no == id) {
						return func(arg, id, me, j,
						    false,
						    &me->mbr.mbr_parts[j],
						    cookie);
					}
					no++;
				}
			}
		}
	}


	return false;
}

static bool
mbr_do_get_part_info(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	struct disk_part_info *info = cookie;
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;

	mbr_partition_to_info(mp, mb->sector, info);
	if (mp->mbrp_start + mb->sector == parts->target)
		info->flags |= PTI_INSTALL_TARGET;
	if (mb->last_mounted[i] != NULL && mb->last_mounted[i][0] != 0)
		info->last_mounted = mb->last_mounted[i];
	if (mb->fs_type[i] != FS_UNUSED) {
		info->fs_type = mb->fs_type[i];
		info->fs_sub_type = mb->fs_sub_type[i];
	} else {
		info->fs_sub_type = 0;
		switch (mp->mbrp_type) {
		case MBR_PTYPE_FAT12:
		case MBR_PTYPE_FAT16S:
		case MBR_PTYPE_FAT16B:
		case MBR_PTYPE_FAT32:
		case MBR_PTYPE_FAT32L:
		case MBR_PTYPE_FAT16L:
		case MBR_PTYPE_OS2_DOS12:
		case MBR_PTYPE_OS2_DOS16S:
		case MBR_PTYPE_OS2_DOS16B:
		case MBR_PTYPE_HID_FAT32:
		case MBR_PTYPE_HID_FAT32_LBA:
		case MBR_PTYPE_HID_FAT16_LBA:
		case MBR_PTYPE_MDOS_FAT12:
		case MBR_PTYPE_MDOS_FAT16S:
		case MBR_PTYPE_MDOS_EXT:
		case MBR_PTYPE_MDOS_FAT16B:
		case MBR_PTYPE_SPEEDSTOR_16S:
		case MBR_PTYPE_EFI:
			info->fs_type = FS_MSDOS;
			info->fs_sub_type = mp->mbrp_type;
			break;
		case MBR_PTYPE_LNXEXT2:
			info->fs_type = FS_EX2FS;
			break;
		case MBR_PTYPE_XENIX_ROOT:
		case MBR_PTYPE_XENIX_USR:
			info->fs_type = FS_SYSV;
			break;
		case MBR_PTYPE_NTFS:
			info->fs_type = FS_NTFS;
			break;
		case MBR_PTYPE_APPLE_HFS:
			info->fs_type = FS_HFS;
			break;
		case MBR_PTYPE_VMWARE:
			info->fs_type = FS_VMFS;
			break;
		case MBR_PTYPE_AST_SWAP:
		case MBR_PTYPE_DRDOS_LSWAP:
		case MBR_PTYPE_LNXSWAP:
		case MBR_PTYPE_BSDI_SWAP:
		case MBR_PTYPE_HID_LNX_SWAP:
		case MBR_PTYPE_VMWARE_SWAP:
			info->fs_type = FS_SWAP;
			break;
		}
	}
	return true;
}

static bool
get_wedge_devname(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	char **res = cookie;

	if (!res)
		return false;

	*res = __UNCONST(mb->wedge[i]);
	return true;
}

static bool
mbr_part_get_wedge(const struct disk_partitions *arg, part_id id,
    char **res)
{
	return mbr_part_apply(arg, id, get_wedge_devname, res);
}

static bool
mbr_get_part_info(const struct disk_partitions *arg, part_id id,
    struct disk_part_info *info)
{
	return mbr_part_apply(arg, id, mbr_do_get_part_info, info);
}

static bool
type_can_change(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	/*
	 * The extended partition can only change type or be
	 * deleted if it is empty
	 */
	if (!MBR_IS_EXTENDED(mp->mbrp_type))
		return true;
	return primary && mb->extended == NULL;
}

static bool
mbr_part_type_can_change(const struct disk_partitions *arg, part_id id)
{
	return mbr_part_apply(arg, id, type_can_change, NULL);
}

struct part_get_str_data {
	char *str;
	size_t avail_space;
	size_t col;
};


static bool
mbr_get_part_table_str(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	struct part_get_str_data *data = cookie;
	char *str = data->str;
	const struct part_type_desc *ptype;

	switch (data->col) {
	case 0:
		ptype = mbr_get_gen_type_desc(mp->mbrp_type);
		if (ptype != NULL)
			strncpy(str, ptype->description, data->avail_space);
		else
			snprintf(str, data->avail_space, "%u", mp->mbrp_type);
		str[data->avail_space-1] = 0;
		break;
	case 1:
		if (mb->last_mounted[i])
			strlcpy(str, mb->last_mounted[i], data->avail_space);
		else
			*str = 0;
		break;
#ifdef BOOTSEL
	case 2:
		if (mb->mbrb.mbrbs_nametab[i][0] != 0)
			strlcpy(str, mb->mbrb.mbrbs_nametab[i],
			    data->avail_space);
		else
			*str = 0;
		break;
#endif
	}

	return true;
}

static bool
mbr_table_str(const struct disk_partitions *arg, part_id id, size_t col,
    char *str, size_t avail_space)
{
	struct part_get_str_data data;

	data.str = str;
	data.avail_space = avail_space;
	data.col = col;
	return mbr_part_apply(arg, id, mbr_get_part_table_str, &data);
}

static bool
mbr_get_part_attr_str(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
#ifdef BOOTSEL
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
#endif
	struct part_get_str_data *data = cookie;
	static const char *flags = NULL;
	char *str = data->str;

	if (flags == NULL)
		flags = msg_string(MSG_mbr_flags);

	if (mp->mbrp_flag & MBR_PFLAG_ACTIVE)
		*str++ = flags[0];
#ifdef BOOTSEL
	if (parts->mbr.bootsec == mb->sector+mp->mbrp_start)
		*str++ = flags[1];
#endif
	*str = 0;
	return true;
}

static bool
mbr_part_attr_str(const struct disk_partitions *arg, part_id id,
    char *str, size_t avail_space)
{
	struct part_get_str_data data;

	if (avail_space < 3)
		return false;

	data.str = str;
	data.avail_space = avail_space;
	return mbr_part_apply(arg, id, mbr_get_part_attr_str, &data);
}

static bool
mbr_info_to_partitition(const struct disk_part_info *info,
   struct mbr_partition *mp, uint sector,
   struct mbr_info_t *mb, size_t index, const char **err_msg)
{
	size_t pt = mbr_type_from_gen_desc(info->nat_type);
	if (info->start + info->size > UINT_MAX
	    || pt > __arraycount(mbr_gen_type_desc)) {
		if (err_msg)
			*err_msg = err_outofmem;
		return false;
	}
	mp->mbrp_start = info->start - sector;
	mp->mbrp_size = info->size;
	mp->mbrp_type = pt;
	if (info->flags & PTI_INSTALL_TARGET) {
		mp->mbrp_flag |= MBR_PFLAG_ACTIVE;
#ifdef BOOTSEL
		strcpy(mb->mbrb.mbrbs_nametab[index], "NetBSD");
#endif
	}

	return true;
}

static bool
inside_ext_part(mbr_info_t *m, daddr_t start)
{
	size_t i;
	struct mbr_partition *mp = NULL;

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (!MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
			continue;
		mp = &m->mbr.mbr_parts[i];
		break;
	}

	if (mp == NULL) {
		assert(false);
		return false;
	}

	if (mp->mbrp_start > start)
		return false;

	return true;
}

static void
adjust_ext_part(mbr_info_t *m, daddr_t start, daddr_t size)
{
	size_t i;
	struct mbr_partition *mp = NULL;

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (!MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
			continue;
		mp = &m->mbr.mbr_parts[i];
		break;
	}

	if (mp == NULL) {
		assert(false);
		return;
	}

	if (mp->mbrp_start + mp->mbrp_size >= start + size)
		return;

	daddr_t new_end = start + size;
	mp->mbrp_size = new_end - mp->mbrp_start;
}

static bool
ext_part_good(mbr_info_t *m, daddr_t ext_start, daddr_t ext_size)
{
	for (m = m->extended; m != NULL; m = m->extended) {
		for (size_t i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;

			if (i > 0 &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				break;

			daddr_t pstart = m->mbr.mbr_parts[i].mbrp_start +
			    m->sector;
			daddr_t pend = pstart + m->mbr.mbr_parts[i].mbrp_size;

			if (pstart < ext_start || pend > ext_start+ext_size)
				return false;
		}
	}

	return true;
}

static bool
mbr_set_part_info(struct disk_partitions *arg, part_id id,
    const struct disk_part_info *info, const char **err_msg)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;
	struct disk_part_info data = *info;
	part_id i, j, no, ext_ndx, t;
	mbr_info_t *m = &parts->mbr, *me;
	uint pt = mbr_type_from_gen_desc(info->nat_type);

	if (MBR_IS_EXTENDED(pt)) {
		/* check for duplicate ext part */
		no = 0;
		t = ext_ndx = MBR_PART_COUNT;
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;
			if (MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				ext_ndx = i;
			if (no == id)
				t = i;
			no++;
		}
		if (ext_ndx < MBR_PART_COUNT && t != ext_ndx) {
			if (err_msg)
				*err_msg =
				    msg_string(MSG_Only_one_extended_ptn);
			return false;
		}
		/* this partition becomes an extended one, apply alignment */
		data.start = max(roundup(data.start, parts->ext_ptn_alignment),
			parts->ext_ptn_alignment);
	}

	no = 0;
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
			continue;

		if (no == id)
			goto found;
		no++;

		if (MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type)) {
			for (me = m->extended; me != NULL; me = me->extended) {
				for (j = 0; j < MBR_PART_COUNT; j++) {
					if (me->mbr.mbr_parts[j].mbrp_type ==
					    MBR_PTYPE_UNUSED)
						continue;
					if (j > 0 && MBR_IS_EXTENDED(
					    me->mbr.mbr_parts[j].mbrp_type))
						break;
					if (no == id) {
						i = j;
						m = me;
						goto found;
					}
					no++;
				}
			}
		}
	}

	if (err_msg)
		*err_msg = INTERNAL_ERROR;
	return false;

found:
	/*
	 * We assume that m is the mbr we want to update and
	 * i is the local partition index into it.
	 */
	if (m == &parts->mbr) {
		if (MBR_IS_EXTENDED(
		    m->mbr.mbr_parts[i].mbrp_type) &&
		    !ext_part_good(&parts->mbr, data.start, data.size)) {
			if (err_msg)
				*err_msg =
				    MSG_mbr_ext_nofit;
			return false;
		}
	} else if (!inside_ext_part(&parts->mbr, data.start)) {
		if (err_msg)
			*err_msg = msg_string(MSG_mbr_inside_ext);
		return false;
	}
	uint start = data.start, size = data.size;
	uint oldstart = m->mbr.mbr_parts[i].mbrp_start + m->sector;
	if (parts->ptn_0_offset > 0 &&
	    start < parts->ptn_0_offset)
		start = parts->ptn_0_offset;
	if (find_mbr_space(m, &start, &size, start, parts->dp.disk_size,
	    oldstart, false) < 0) {
		if (err_msg != NULL)
			*err_msg = INTERNAL_ERROR;
		return false;
	}
	data.start = start;
	if (size < data.size)
		data.size = size;
	uint old_start = m->mbr.mbr_parts[i].mbrp_start;
	if (!mbr_info_to_partitition(&data,
	   &m->mbr.mbr_parts[i], m->sector, m, i, err_msg))
		return false;
	if (data.flags & PTI_INSTALL_TARGET)
		parts->target = start;
	else if (old_start == parts->target)
		parts->target = -1;
	if (data.last_mounted && m->last_mounted[i] &&
	    data.last_mounted != m->last_mounted[i]) {
		free(__UNCONST(m->last_mounted[i]));
		m->last_mounted[i] = strdup(data.last_mounted);
	}
	if (data.fs_type != 0)
		m->fs_type[i] = data.fs_type;
	if (data.fs_sub_type != 0)
		m->fs_sub_type[i] = data.fs_sub_type;

	if (m == &parts->mbr) {
		if (m->mbr.mbr_parts[i].mbrp_start !=
		    old_start)
			mbr_sort_main_mbr(&m->mbr);
	} else {
		adjust_ext_part(&parts->mbr,
		    data.start, data.size);
	}
	mbr_calc_free_space(parts);
	return true;
}

static bool
mbr_find_netbsd(const struct mbr_info_t *m, uint start,
    struct disk_part_info *info)
{
	size_t i;
	bool prim = true;

	do {
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;

			if (!prim && i > 0 &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				break;

			const struct mbr_partition *mp = &m->mbr.mbr_parts[i];
			if (mp->mbrp_type != MBR_PTYPE_NETBSD)
				continue;

			mbr_partition_to_info(mp, m->sector, info);
			if (m->last_mounted[i] && *m->last_mounted[i] != 0)
					info->last_mounted =
					    m->last_mounted[i];
			info->fs_type = m->fs_type[i];
			info->fs_sub_type = m->fs_sub_type[i];
			if (start > 0 && start != info->start)
				continue;
			return true;
		}
		prim = false;
	} while ((m = m->extended));

	return false;
}

static struct disk_partitions *
mbr_read_disklabel(struct disk_partitions *arg, daddr_t start, bool force_empty)
{
	struct mbr_disk_partitions *myparts =
	    (struct mbr_disk_partitions*)arg;
	struct disk_part_info part;
	struct disk_part_free_space space;

	if (force_empty && myparts->dlabel)
		myparts->dlabel->pscheme->delete_all_partitions(
		    myparts->dlabel);

	if (myparts->dlabel == NULL) {
		/*
		 * Find the NetBSD MBR partition
		 */
		if (!mbr_find_netbsd(&myparts->mbr, start, &part)) {
			if (!force_empty)
				return NULL;

			/* add a "whole disk" NetBSD partition */
			memset(&part, 0, sizeof part);
			part.start = min(myparts->ptn_0_offset,start);
			if (!mbr_get_free_spaces(arg, &space, 1,
			    part.start, myparts->ptn_alignment, -1, -1))
				return NULL;
			part.start = space.start;
			part.size = space.size;
			part.nat_type = &mbr_gen_type_desc[MBR_PTYPE_NETBSD].gen;
			mbr_add_part(arg, &part, NULL);
			if (!mbr_find_netbsd(&myparts->mbr, start, &part))
				return NULL;
		}

		if (!force_empty) {
			myparts->dlabel = disklabel_parts.read_from_disk(
			    myparts->dp.disk, part.start, part.size,
			    myparts->dp.bytes_per_sector, &disklabel_parts);
			if (myparts->dlabel != NULL)
				myparts->dlabel->parent = &myparts->dp;
		}

		if (myparts->dlabel == NULL && part.size > 0) {
			/* we just created the outer partitions? */
			myparts->dlabel =
			    disklabel_parts.create_new_for_disk(
			    myparts->dp.disk, part.start, part.size,
			    false, &myparts->dp);
		}

		if (myparts->dlabel != NULL)
			myparts->dlabel->pscheme->change_disk_geom(
			    myparts->dlabel, myparts->geo_cyl,
			    myparts->geo_head, myparts->geo_sec);
	}
	return myparts->dlabel;
}

static int
get_mapping(struct mbr_partition *parts, int i,
	    int *cylinder, int *head, int *sector, daddr_t *absolute)
{
	struct mbr_partition *apart = &parts[i / 2];

	if (apart->mbrp_type == MBR_PTYPE_UNUSED)
		return -1;
	if (i % 2 == 0) {
		*cylinder = MBR_PCYL(apart->mbrp_scyl, apart->mbrp_ssect);
		*head = apart->mbrp_shd;
		*sector = MBR_PSECT(apart->mbrp_ssect) - 1;
		*absolute = le32toh(apart->mbrp_start);
	} else {
		*cylinder = MBR_PCYL(apart->mbrp_ecyl, apart->mbrp_esect);
		*head = apart->mbrp_ehd;
		*sector = MBR_PSECT(apart->mbrp_esect) - 1;
		*absolute = le32toh(apart->mbrp_start)
			+ le32toh(apart->mbrp_size) - 1;
	}
	/* Sanity check the data against max values */
	if ((((*cylinder * MAXHEAD) + *head) * (uint32_t)MAXSECTOR + *sector) < *absolute)
		/* cannot be a CHS mapping */
		return -1;

	return 0;
}

static bool
mbr_delete_all(struct disk_partitions *arg)
{
	struct mbr_disk_partitions *myparts = (struct mbr_disk_partitions*)arg;
	struct mbr_sector *mbrs = &myparts->mbr.mbr;
	struct mbr_info_t *mbri = &myparts->mbr;
	mbr_info_t *ext;
	struct mbr_partition *part;

	part = &mbrs->mbr_parts[0];
	/* Set the partition information for full disk usage. */
	while ((ext = mbri->extended)) {
		mbri->extended = ext->extended;
		free_mbr_info(ext);
	}
	memset(part, 0, MBR_PART_COUNT * sizeof *part);
#ifdef BOOTSEL
	memset(&mbri->mbrb, 0, sizeof mbri->mbrb);
#endif

	/*
	 * We may have changed alignment settings due to partitions
	 * ending on an MB boundary - undo that, now that the partitions
	 * are gone.
	 */
	mbr_change_disk_geom(arg, myparts->geo_cyl, myparts->geo_head,
	    myparts->geo_sec);

	return true;
}

/*
 * helper function to fix up mbrp_start and mbrp_size for the
 * extended MBRs "partition b" entries after addition/deletion
 * of some partition.
 */
static void
mbr_fixup_ext_chain(mbr_info_t *primary, uint ext_start, uint ext_end)
{
	for (mbr_info_t *m = primary->extended; m != NULL; m = m->extended) {
		if (m->extended == NULL) {
			m->mbr.mbr_parts[1].mbrp_type = MBR_PTYPE_UNUSED;
			m->mbr.mbr_parts[1].mbrp_start = 0;
			m->mbr.mbr_parts[1].mbrp_size = 0;
		} else {
			uint n_end, n_start = m->extended->sector;
			if (m->extended->extended)
				n_end = m->extended->extended->sector;
			else
				n_end = ext_end;
			m->mbr.mbr_parts[1].mbrp_type = MBR_PTYPE_EXT;
			m->mbr.mbr_parts[1].mbrp_start = n_start - ext_start;
			m->mbr.mbr_parts[1].mbrp_size = n_end - n_start;
		}
	}
}

struct delete_part_args {
	struct mbr_disk_partitions *parts;
	daddr_t start, size;
	const char **err_msg;
};

static bool
mbr_do_delete_part(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	struct delete_part_args *marg = cookie;
	bool is_ext_part = MBR_IS_EXTENDED(mp->mbrp_type);

	/* can not delete non-empty extended partitions */
	if (MBR_IS_EXTENDED(mp->mbrp_type)
	    && marg->parts->mbr.extended != NULL) {
		if (marg->err_msg)
			*marg->err_msg = msg_string(MSG_mbr_ext_not_empty);
		return false;
	}

	/* return position/size to caller */
	marg->start = mb->sector + mp->mbrp_start;
	marg->size = mp->mbrp_size;

	if (primary) {
		/* if deleting the primary extended partition, just kill it */
		struct mbr_partition *md = &marg->parts->mbr.mbr.mbr_parts[i];
		md->mbrp_size = 0;
		md->mbrp_start = 0;
		md->mbrp_type = MBR_PTYPE_UNUSED;
		if (marg->parts->mbr.last_mounted[i]) {
			free(__UNCONST(marg->parts->mbr.last_mounted[i]));
			marg->parts->mbr.last_mounted[i] = NULL;
		}
		if (is_ext_part) {
			for (mbr_info_t *m = marg->parts->mbr.extended;
			    m != NULL; ) {
				mbr_info_t *n = m->extended;
				free_mbr_info(m);
				m = n;
			}
			marg->parts->mbr.extended = NULL;
		}
	} else {
		/* find the size of the primary extended partition */
		uint ext_start = 0, ext_size = 0;
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (!MBR_IS_EXTENDED(marg->parts->mbr.mbr.mbr_parts[i]
			    .mbrp_type))
				continue;
			ext_start = marg->parts->mbr.mbr.mbr_parts[i]
			    .mbrp_start;
			ext_size = marg->parts->mbr.mbr.mbr_parts[i]
			    .mbrp_size;
			break;
		}

		/*
		 * If we are in an extended partition chain, unlink this MBR,
		 * unless it is the very first one at the start of the extended
		 * partition (we would have no previos ext mbr to fix up
		 * the chain in that case)
		 */
		if (marg->parts->mbr.extended == mb) {
			struct mbr_partition *part =
			    &marg->parts->mbr.extended->mbr.mbr_parts[0];
			part->mbrp_type = MBR_PTYPE_UNUSED;
			part->mbrp_start = 0;
			part->mbrp_size = 0;
		} else {
			mbr_info_t *p, *last;
			for (last = NULL, p = &marg->parts->mbr; p != NULL;
			    last = p, p = p->extended)
				if (p == mb)
					break;
			if (last == NULL) {
				if (marg->err_msg != NULL)
					*marg->err_msg= INTERNAL_ERROR;
				return false;
			}
			last->extended = p->extended;
			free_mbr_info(p);
			if (last == &marg->parts->mbr && last->extended &&
			    last->extended->extended == NULL &&
			    last->extended->mbr.mbr_parts[0].mbrp_type ==
			    MBR_PTYPE_UNUSED) {
				/*
				 * we deleted the last extended sector,
				 * remove the whole chain
				 */
				free_mbr_info(last->extended);
				last->extended = NULL;
			}
		}
		mbr_fixup_ext_chain(&marg->parts->mbr, ext_start,
		    ext_start+ext_size);
	}
	mbr_calc_free_space(marg->parts);
	return true;
}

static bool
mbr_delete_part(struct disk_partitions *arg, part_id pno, const char **err_msg)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;
	struct delete_part_args data = { .parts = parts, .err_msg = err_msg };

	if (!mbr_part_apply(arg, pno, mbr_do_delete_part, &data)) {
		if (err_msg)
			*err_msg = INTERNAL_ERROR;
		return false;
	}

	if (parts->target == data.start)
		parts->target = ~0U;

	if (parts->dlabel) {
		/*
		 * If we change the mbr partitioning, the we must
		 * remove any references in the netbsd disklabel
		 * to the part we changed.
		 */
		parts->dlabel->pscheme->delete_partitions_in_range(
		    parts->dlabel, data.start, data.size);
	}

	if (err_msg)
		*err_msg = NULL;

	dump_mbr(&parts->mbr, "after delete");
	return true;
}

static struct mbr_partition *
mbr_ptr_from_start(mbr_info_t *m, daddr_t start)
{
	bool primary = true;

	do {
		for (uint i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;
			if (!primary &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				break;

			daddr_t pstart = m->sector +
			    m->mbr.mbr_parts[i].mbrp_start;
			if (pstart == start)
				return &m->mbr.mbr_parts[i];

		}
		primary = false;
	} while ((m = m->extended));

	return NULL;
}

static uint8_t
mbr_type_from_start(const mbr_info_t *m, daddr_t start)
{
	bool primary = true;

	do {
		for (uint i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;
			if (!primary &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				break;

			daddr_t pstart = m->sector +
			    m->mbr.mbr_parts[i].mbrp_start;
			if (pstart == start)
				return m->mbr.mbr_parts[i].mbrp_type;

		}
		primary = false;
	} while ((m = m->extended));

	return MBR_PTYPE_UNUSED;
}

static part_id
mbr_add_part(struct disk_partitions *arg,
    const struct disk_part_info *info, const char **errmsg)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;
	part_id i, j, no, free_primary = UINT_MAX;
	mbr_info_t *m = &parts->mbr, *me, *last, *t;
	daddr_t ext_start = 0, ext_size = 0;
	uint start, size;
	struct disk_part_info data = *info;
	struct mbr_partition *newp;

	if (errmsg != NULL)
		*errmsg = NULL;

	assert(info->nat_type != NULL);
	if (info->nat_type == NULL) {
		if (errmsg != NULL)
			*errmsg = INTERNAL_ERROR;
		return NO_PART;
	}
	if (mbr_type_from_gen_desc(info->nat_type) == MBR_PTYPE_UNUSED) {
		if (errmsg != NULL)
			*errmsg = INTERNAL_ERROR;
		return NO_PART;
	}

	/* do we have free primary slots and/or an extended partition? */
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED
		    && free_primary > MBR_PART_COUNT)
			free_primary = i;
		if (MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type)) {
			ext_start = m->mbr.mbr_parts[i].mbrp_start+m->sector;
			ext_size = m->mbr.mbr_parts[i].mbrp_size;
			continue;
		}
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED
		    && m->mbr.mbr_parts[i].mbrp_size == 0)
			continue;
	}
	if (ext_start > 0 && ext_size > 0 &&
	    MBR_IS_EXTENDED(mbr_type_from_gen_desc(info->nat_type))) {
		/*
		 * Do not allow a second extended partition
		 */
		if (errmsg)
			*errmsg = MSG_Only_one_extended_ptn;
		return NO_PART;
	}

	/* should this go into the extended partition? */
	if (ext_size > 0 && info->start >= ext_start
	    && info->start < ext_start + ext_size) {

		/* must fit into the extended partition */
		if (info->start + info->size > ext_start + ext_size) {
			if (errmsg != NULL)
				*errmsg = MSG_mbr_ext_nofit;
			return NO_PART;
		}

		/* walk the chain untill we find a proper insert position */
		daddr_t e_end, e_start;
		for (last = m, m = m->extended; m != NULL;
		    last = m, m = m->extended) {
			e_start = m->mbr.mbr_parts[1].mbrp_start
			    + ext_start;
			e_end = e_start + m->mbr.mbr_parts[1].mbrp_size;
			if (data.start <= e_start)
				break;
		}
		if (m == NULL) {
			/* add new tail record */
			e_end = ext_start + ext_size;
			/* new part needs to fit inside primary extended one */
			if (data.start + data.size > e_end) {
				if (errmsg)
					*errmsg = MSG_No_free_space;
				return NO_PART;
			}
		} else if (data.start + data.size > e_start) {
			/* new part needs to fit before next extended */
			if (errmsg)
				*errmsg = MSG_No_free_space;
			return NO_PART;
		}
		/*
		 * now last points to previous mbr (maybe primary), m
		 * points to the one that should take the new partition
		 * or we have to insert a new mbr between the two, or
		 * m needs to be split and we go into the one after it.
		 */
		if (m && m->mbr.mbr_parts[0].mbrp_type == MBR_PTYPE_UNUSED) {
			/* empty slot, we can just use it */
			newp = &m->mbr.mbr_parts[0];
			mbr_info_to_partitition(&data, &m->mbr.mbr_parts[0],
			    m->sector, m, 0, errmsg);
			if (data.last_mounted && m->last_mounted[0] &&
			    data.last_mounted != m->last_mounted[0]) {
				free(__UNCONST(m->last_mounted[0]));
				m->last_mounted[0] = strdup(data.last_mounted);
			}
		} else {
			mbr_info_t *new_mbr;
			if (m == NULL)
				m = last;
			daddr_t p_start = m->mbr.mbr_parts[0].mbrp_start
			    + m->sector;
			daddr_t p_end = p_start
			    + m->mbr.mbr_parts[0].mbrp_size;
			bool before;
			if (m == last || data.start > p_end)
				before = false;
			else if (data.start + data.size < p_start)
				before = true;
			else {
				if (errmsg)
					*errmsg = MSG_No_free_space;
				return NO_PART;
			}
			new_mbr = calloc(1, sizeof *new_mbr);
			if (!new_mbr) {
				if (errmsg)
					*errmsg = err_outofmem;
				return NO_PART;
			}
			new_mbr->mbr.mbr_magic = htole16(MBR_MAGIC);
			new_mbr->mbr.mbr_parts[1].mbrp_type = MBR_PTYPE_EXT;
			if (before) {
				/*
				 * This is a hypthetical case where
				 * an extended MBR uses an unusual high
				 * offset (m->sector to parts[0].mbrp_start)
				 * and we want to go into that space.
				 * Should not happen in the real world (tm)
				 * and is untested....
				 */

				/* make sure the aligned new mbr fits */
				uint mbrsec = rounddown(p_start,
				    parts->ext_ptn_alignment);
				if (mbrsec <= data.start + data.size)
					data.size = mbrsec-1-data.start;

				/* now the new partition data is ready,
				 * write out to old position */
				new_mbr->sector = m->sector;
				newp = &new_mbr->mbr.mbr_parts[0];
				mbr_info_to_partitition(&data,
				    &new_mbr->mbr.mbr_parts[0],
				    new_mbr->sector, new_mbr, 0, errmsg);
				if (data.last_mounted && m->last_mounted[0] &&
				    data.last_mounted != m->last_mounted[0]) {
					free(__UNCONST(m->last_mounted[0]));
					m->last_mounted[0] =
					    strdup(data.last_mounted);
				}
				new_mbr->extended = m;
			} else {
				new_mbr->sector = max(roundup(data.start,
				    parts->ext_ptn_alignment),
				    parts->ext_ptn_alignment);
				uint off = new_mbr->sector - data.start;
				data.start += parts->ptn_0_offset+off;
				if (data.start + data.size > e_end)
					data.size = e_end - data.start;
				newp = &new_mbr->mbr.mbr_parts[0];
				mbr_info_to_partitition(&data,
				    &new_mbr->mbr.mbr_parts[0],
				    new_mbr->sector, new_mbr, 0, errmsg);
				if (data.last_mounted && m->last_mounted[0] &&
				    data.last_mounted != m->last_mounted[0]) {
					free(__UNCONST(m->last_mounted[0]));
					m->last_mounted[0] =
					    strdup(data.last_mounted);
				}
				/*
				 * Special case: if we are creating the
				 * first extended mbr, but do not start
				 * at the beginning of the primary
				 * extended partition, we need to insert
				 * another extended mbr at the start.
				 */
				if (m == &parts->mbr && m->extended == NULL
				    && new_mbr->sector > ext_start) {
					t = calloc(1, sizeof *new_mbr);
					if (!t) {
						free_mbr_info(new_mbr);
						if (errmsg)
							*errmsg = err_outofmem;
						return NO_PART;
					}
					t->sector = ext_start;
					t->mbr.mbr_magic = htole16(MBR_MAGIC);
					t->mbr.mbr_parts[1].mbrp_type =
					    MBR_PTYPE_EXT;
					m->extended = t;
					m = t;
				}
				new_mbr->extended = m->extended;
				m->extended = new_mbr;
			}
		}
		mbr_fixup_ext_chain(&parts->mbr, ext_start, ext_start+ext_size);
		dump_mbr(&parts->mbr, "after adding in extended");
		goto find_rval;
	}

	/* this one is for the primary boot block */
	if (free_primary > MBR_PART_COUNT) {
		if (errmsg != NULL)
			*errmsg = ext_size > 0 ?
				MSG_mbr_no_free_primary_have_ext
				: MSG_mbr_no_free_primary_no_ext;
		return NO_PART;
	}

	start = max(info->start, parts->ptn_0_offset);
	size = info->size;
	if (find_mbr_space(m, &start, &size, start, parts->dp.disk_size,
	    start, true) < 0 || size < info->size) {
		if (errmsg != NULL)
			*errmsg = MSG_No_free_space;
		return NO_PART;
	}
	data.start = start;
	if (MBR_IS_EXTENDED(mbr_type_from_gen_desc(info->nat_type))) {
		data.start = max(roundup(data.start, parts->ext_ptn_alignment),
		   parts->ext_ptn_alignment);
	}
	if (data.start + data.size > start + size)
		data.size = start + size - data.start;
	mbr_info_to_partitition(&data, &m->mbr.mbr_parts[free_primary],
	     m->sector, m, free_primary, errmsg);
	if (data.last_mounted && m->last_mounted[free_primary] &&
	    data.last_mounted != m->last_mounted[free_primary]) {
		free(__UNCONST(m->last_mounted[free_primary]));
		m->last_mounted[free_primary] = strdup(data.last_mounted);
	}
	start = m->mbr.mbr_parts[free_primary].mbrp_start;
	mbr_sort_main_mbr(&m->mbr);

	/* find the partition again after sorting */
	newp = NULL;
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
			continue;
		if (m->mbr.mbr_parts[i].mbrp_start != start)
			continue;
		newp = &m->mbr.mbr_parts[i];
		break;
	}

	dump_mbr(&parts->mbr, "after adding in primary");

find_rval:
	mbr_calc_free_space(parts);
	if (newp == NULL)
		return 0;

	/*
	 * Now newp points to the modified partition entry but we do not know
	 * a good part_id for it.
	 * Iterate from start and find it.
	 */
	no = 0;
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
			continue;

		if (newp == &m->mbr.mbr_parts[i])
			return no;
		no++;

		if (MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type)) {
			for (me = m->extended; me != NULL; me = me->extended) {
				for (j = 0; j < MBR_PART_COUNT; j++) {
					if (me->mbr.mbr_parts[j].mbrp_type ==
					    MBR_PTYPE_UNUSED)
						continue;
					if (j > 0 && MBR_IS_EXTENDED(
					    me->mbr.mbr_parts[j].mbrp_type))
						break;
					if (newp == &me->mbr.mbr_parts[j])
						return no;
					no++;
				}
			}
		}
	}
	return 0;
}

static int
mbr_guess_geom(struct disk_partitions *arg, int *cyl, int *head, int *sec)
{
	struct mbr_disk_partitions *myparts = (struct mbr_disk_partitions*)arg;
	struct mbr_sector *mbrs = &myparts->mbr.mbr;
	struct mbr_partition *parts = &mbrs->mbr_parts[0];
	int xcylinders, xheads, i, j;
	daddr_t xsectors, xsize;
	int c1, h1, s1, c2, h2, s2;
	daddr_t a1, a2;
	uint64_t num, denom;

	xheads = -1;

	/* Try to deduce the number of heads from two different mappings. */
	for (i = 0; i < MBR_PART_COUNT * 2 - 1; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		a1 -= s1;
		for (j = i + 1; j < MBR_PART_COUNT * 2; j++) {
			if (get_mapping(parts, j, &c2, &h2, &s2, &a2) < 0)
				continue;
			a2 -= s2;
			num = (uint64_t)h1 * a2 - (quad_t)h2 * a1;
			denom = (uint64_t)c2 * a1 - (quad_t)c1 * a2;
			if (num != 0 && denom != 0 && num % denom == 0) {
				xheads = (int)(num / denom);
				xsectors = a1 / (c1 * xheads + h1);
				break;
			}
		}
		if (xheads != -1)
			break;
	}

	if (xheads == -1)
		return -1;

	/*
	 * Estimate the number of cylinders.
	 * XXX relies on get_disks having been called.
	 */
	xsize = min(pm->dlsize, mbr_parts.size_limit);
	xcylinders = xsize / xheads / xsectors;
	if (xsize != xcylinders * xheads * xsectors)
		xcylinders++;

	/*
	 * Now verify consistency with each of the partition table entries.
	 * Be willing to shove cylinders up a little bit to make things work,
	 * but translation mismatches are fatal.
	 */
	for (i = 0; i < MBR_PART_COUNT * 2; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		if (c1 >= MAXCYL - 1)
			/* Ignore anything that is near the CHS limit */
			continue;
		if (xsectors * (c1 * xheads + h1) + s1 != a1)
			return -1;
	}

	/*
	 * Everything checks out.  Reset the geometry to use for further
	 * calculations.
	 */
	*cyl = MIN(xcylinders, MAXCYL);
	*head = xheads;
	*sec = xsectors;
	return 0;
}

static size_t
mbr_get_cylinder(const struct disk_partitions *arg)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;

	return parts->geo_cyl;
}

static daddr_t
mbr_max_part_size(const struct disk_partitions *arg, daddr_t fp_start)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	uint start = fp_start, size = 0;
	uint8_t pt;

	start = fp_start;
	pt = mbr_type_from_start(&parts->mbr, start);
	if (find_mbr_space(&parts->mbr, &start, &size, start,
	    parts->dp.disk_size, start, MBR_IS_EXTENDED(pt)) < 0)
		return 0;

	return size;
}

static size_t
mbr_get_free_spaces(const struct disk_partitions *arg,
    struct disk_part_free_space *result, size_t max_num_result,
    daddr_t min_size, daddr_t align, daddr_t lower_bound, daddr_t ignore)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	uint start = 0, size = 0, from, next;
	size_t spaces = 0;

	if (min_size < 1)
		min_size = 1;
	from = parts->ptn_0_offset;
	if (lower_bound > from)
		from = lower_bound;
	for ( ; from < parts->dp.disk_size && spaces < max_num_result; ) {
		if (find_mbr_space(&parts->mbr, &start, &size, from,
		    parts->dp.disk_size, ignore > 0 ? (uint)ignore : UINT_MAX,
		    false) < 0)
			break;
		next = start + size + 1;
		if (align > 0) {
			uint nv = max(roundup(start, align), align);
			uint off = nv - start;
			start = nv;
			if (size > off)
				size -= off;
			else
				size = 0;
		}
		if (size > min_size) {
			result[spaces].start = start;
			result[spaces].size = size;
			spaces++;
		}
		if ((daddr_t)start + (daddr_t)size + 1 >= mbr_parts.size_limit)
			break;
		from = next;
	}

	return spaces;
}

static bool
mbr_can_add_partition(const struct disk_partitions *arg)
{
	const struct mbr_disk_partitions *myparts =
	    (const struct mbr_disk_partitions*)arg;
	struct disk_part_free_space space;
	bool free_primary, have_extended;

	if (arg->free_space < myparts->ptn_alignment)
		return false;

	if (mbr_get_free_spaces(arg, &space, 1, myparts->ptn_alignment,
	    myparts->ptn_alignment, 0, -1) < 1)
		return false;

	for (int i = 0; i < MBR_PART_COUNT; i++) {
		uint8_t t = myparts->mbr.mbr.mbr_parts[i].mbrp_type;

		if (t == MBR_PTYPE_UNUSED &&
		     myparts->mbr.mbr.mbr_parts[i].mbrp_size == 0)
			free_primary = true;

		if (MBR_IS_EXTENDED(t))
			have_extended = true;
	}

	if (have_extended)
		return true;

	return free_primary;
}

static void
mbr_free_wedge(int *fd, const char *disk, const char *wedge)
{
	struct dkwedge_info dkw;
	char diskpath[MAXPATHLEN];

	if (*fd == -1)
		*fd = opendisk(disk, O_RDWR, diskpath,
		    sizeof(diskpath), 0);
	if (*fd != -1) {
		memset(&dkw, 0, sizeof(dkw));
		strlcpy(dkw.dkw_devname, wedge,
		    sizeof(dkw.dkw_devname));
		ioctl(*fd, DIOCDWEDGE, &dkw);
	}
}

static void
mbr_free(struct disk_partitions *arg)
{
	struct mbr_disk_partitions *parts = (struct mbr_disk_partitions*)arg;
	mbr_info_t *m;
	int i, fd;

	assert(parts != NULL);

	fd = -1;
	m = &parts->mbr;
	do {
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (m->wedge[i][0] != 0)
				mbr_free_wedge(&fd, arg->disk, m->wedge[i]);
		}
	} while ((m = m->extended));

	if (fd != -1)
		close(fd);

	if (parts->dlabel)
		parts->dlabel->pscheme->free(parts->dlabel);

	free_mbr_info(parts->mbr.extended);
	free_last_mounted(&parts->mbr);
	free(__UNCONST(parts->dp.disk));
	free(parts);
}

static void
mbr_destroy_part_scheme(struct disk_partitions *arg)
{
	struct mbr_disk_partitions *parts = (struct mbr_disk_partitions*)arg;
	char diskpath[MAXPATHLEN];
	int fd;

	if (parts->dlabel != NULL)
		parts->dlabel->pscheme->destroy_part_scheme(parts->dlabel);
	fd = opendisk(arg->disk, O_RDWR, diskpath, sizeof(diskpath), 0);
	if (fd != -1) {
		char *buf;

		buf = calloc(arg->bytes_per_sector, 1);
		if (buf != NULL) {
			write(fd, buf, arg->bytes_per_sector);
			free(buf);
		}
		close(fd);
	}
	mbr_free(arg);
}

static bool
mbr_verify_for_update(struct disk_partitions *arg)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;

	return md_mbr_update_check(arg, &parts->mbr);
}

static int
mbr_verify(struct disk_partitions *arg, bool quiet)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;
	mbr_info_t *m = &parts->mbr;
	int i;
	bool active_found = false;

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_flag & MBR_PFLAG_ACTIVE) {
			active_found = true;
			break;
		}
	}

	if (!active_found && pm->ptstart > 0) {
		struct mbr_partition *mp = mbr_ptr_from_start(m, pm->ptstart);

		if (mp) {
			if (!quiet)
				msg_display(MSG_noactivepart);
			if (quiet || ask_yesno(MSG_fixactivepart)) {
				mp->mbrp_flag |= MBR_PFLAG_ACTIVE;
				active_found = true;
			}
		}
	}
	if (!active_found && !quiet) {
		msg_display(MSG_noactivepart);
		i = ask_reedit(arg);
		if (i <= 1)
			return i;
	}

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (m->mbr.mbr_parts[i].mbrp_type != MBR_PTYPE_NETBSD)
			continue;
		m->mbr.mbr_parts[i].mbrp_flag |= MBR_PFLAG_ACTIVE;
		break;
	}

	return md_check_mbr(arg, &parts->mbr, quiet);
}

static bool
mbr_guess_root(const struct disk_partitions *arg,
    daddr_t *start, daddr_t *size)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	const mbr_info_t *m = &parts->mbr;
	size_t i, num_found;
	bool prim = true;
	daddr_t pstart, psize;

	num_found = 0;
	do {
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (m->mbr.mbr_parts[i].mbrp_type == MBR_PTYPE_UNUSED)
				continue;

			if (!prim && i > 0 &&
			    MBR_IS_EXTENDED(m->mbr.mbr_parts[i].mbrp_type))
				break;

			const struct mbr_partition *mp = &m->mbr.mbr_parts[i];
			if (mp->mbrp_type != MBR_PTYPE_NETBSD)
				continue;

			if (num_found == 0) {
				pstart = m->sector + mp->mbrp_start;
				psize = mp->mbrp_size;
			}
			num_found++;

			if (m->last_mounted[i] != NULL &&
			    strcmp(m->last_mounted[i], "/") == 0) {
				*start = pstart;
				*size = psize;
				return true;
			}
		}
		prim = false;
	} while ((m = m->extended));

	if (num_found == 1) {
		*start = pstart;
		*size = psize;
		return true;
	}

	return false;
}

struct part_attr_fmt_data {
	char *str;
	size_t avail_space, attr_no;
	const struct mbr_disk_partitions *parts;
	const struct disk_part_info *info;
};

struct part_attr_set_data {
	size_t attr_no;
	const struct mbr_disk_partitions *parts;
	const char *str;
	mbr_info_t *mbr;
};

static bool
part_attr_fornat_str(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	struct part_attr_fmt_data *data = cookie;
	const char *attrtype = parts->dp.pscheme
	    ->custom_attributes[data->attr_no].label;

	if (attrtype == MSG_ptn_active) {
		strlcpy(data->str,
		    msg_string(primary && (mp->mbrp_flag & MBR_PFLAG_ACTIVE) ?
		    MSG_Yes : MSG_No), data->avail_space);
		return true;
#if BOOTSEL
	} else if (attrtype == MSG_boot_dflt) {
		strlcpy(data->str,
		    msg_string(
			(parts->mbr.bootsec == mb->sector+mp->mbrp_start) ?
		    MSG_Yes : MSG_No), data->avail_space);
		return true;
	} else if (attrtype == MSG_bootmenu) {
		strlcpy(data->str, mb->mbrb.mbrbs_nametab[i],
		    data->avail_space);
#endif
	}

	return false;
}

static bool
part_attr_set_str(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	struct part_attr_set_data *data = cookie;
	const char *str = data->str;
#ifdef BOOTSEL
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	const char *attrtype = parts->dp.pscheme
	    ->custom_attributes[data->attr_no].label;
	mbr_info_t *m;
#endif

	while (*str == ' ')
		str++;

#if BOOTSEL
	if (attrtype == MSG_bootmenu) {
		for (m = data->mbr; m != mb; m = m->extended)
			;
		strncpy(m->mbrb.mbrbs_nametab[i], str,
		    sizeof(m->mbrb.mbrbs_nametab[i]));
	}
#endif

	return false;
}

static bool
part_attr_toggle(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	struct part_attr_set_data *data = cookie;
	const char *attrtype = parts->dp.pscheme
	    ->custom_attributes[data->attr_no].label;
	int j;

	if (attrtype == MSG_ptn_active) {
		if (!primary)
			return false;

		data->mbr->mbr.mbr_parts[i].mbrp_flag ^= MBR_PFLAG_ACTIVE;
		for (j = 0; j < MBR_PART_COUNT; j++) {
			if (j == i)
				continue;
			data->mbr->mbr.mbr_parts[j].mbrp_flag
			    &= ~MBR_PFLAG_ACTIVE;
		}
		return true;
#ifdef BOOTSEL
	} else if (attrtype == MSG_boot_dflt) {
		if (data->mbr->bootsec == mb->sector+mp->mbrp_start)
			data->mbr->bootsec = 0;
		else
			data->mbr->bootsec = mb->sector+mp->mbrp_start;
		return true;
#endif
	}

	return false;
}

static bool
mbr_custom_attribute_format(const struct disk_partitions *arg,
    part_id id, size_t attr_no, const struct disk_part_info *info,
    char *res, size_t space)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	struct part_attr_fmt_data data;

	data.str = res;
	data.avail_space = space;
	data.attr_no = attr_no;
	data.parts = parts;
	data.info = info;

	return mbr_part_apply(arg, id, part_attr_fornat_str, &data);
}

static bool
mbr_custom_attribute_toggle(struct disk_partitions *arg,
    part_id id, size_t attr_no)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;
	struct part_attr_set_data data;

	data.attr_no = attr_no;
	data.parts = parts;
	data.str = NULL;
#ifdef BOOTSEL
	data.mbr = &parts->mbr;
#endif

	return mbr_part_apply(arg, id, part_attr_toggle, &data);
}

static bool
mbr_custom_attribute_set_str(struct disk_partitions *arg,
    part_id id, size_t attr_no, const char *new_val)
{
	struct mbr_disk_partitions *parts =
	    (struct mbr_disk_partitions*)arg;
	struct part_attr_set_data data;

	data.attr_no = attr_no;
	data.parts = parts;
	data.str = new_val;
#ifdef BOOTSEL
	data.mbr = &parts->mbr;
#endif

	return mbr_part_apply(arg, id, part_attr_set_str, &data);
}

static daddr_t
mbr_part_alignment(const struct disk_partitions *arg)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;

	return parts->ptn_alignment;
}

static bool
add_wedge(const char *disk, daddr_t start, daddr_t size,
    char *wname, size_t max_len)
{
	struct dkwedge_info dkw;
	char diskpath[MAXPATHLEN];
	int fd;

	memset(&dkw, 0, sizeof(dkw));
	dkw.dkw_offset = start;
	dkw.dkw_size = size;
	snprintf((char*)dkw.dkw_wname, sizeof dkw.dkw_wname,
	    "%s_%" PRIi64 "@%" PRIi64, disk, size, start);

	*wname = 0;

	fd = opendisk(disk, O_RDWR, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return false;
	if (ioctl(fd, DIOCAWEDGE, &dkw) == -1) {
		close(fd);
		return false;
	}
	close(fd);
	strlcpy(wname, dkw.dkw_devname, max_len);
	return true;
}

static bool
mbr_get_part_device(const struct disk_partitions *arg,
    part_id ptn, char *devname, size_t max_devname_len, int *part,
    enum dev_name_usage usage, bool with_path, bool life)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	struct disk_part_info info, tmp;
	part_id dptn;
	char *wedge_dev;

	if (!mbr_get_part_info(arg, ptn, &info))
		return false;

	if (!mbr_part_get_wedge(arg, ptn, &wedge_dev) || wedge_dev == NULL)
		return false;

	if (wedge_dev[0] == 0) {
		/*
		 * If we have secondary partitions, try to find a match there
		 * and use that...
		 */
		if (parts->dlabel != NULL) {
			for (dptn = 0; dptn < parts->dlabel->num_part; dptn++) {
				if (!parts->dlabel->pscheme->get_part_info(
				    parts->dlabel, dptn, &tmp))
					continue;
				if (tmp.start != info.start ||
				    tmp.size != info.size)
					continue;
				return parts->dlabel->pscheme->get_part_device(
				    parts->dlabel, dptn, devname,
				     max_devname_len,
				    part, usage, with_path, life);
			}
		}

		/*
		 * Configure a new wedge and remember the name
		 */
		if (!add_wedge(arg->disk, info.start, info.size, wedge_dev,
		    MBR_DEV_LEN))
			return false;
	}

	assert(wedge_dev[0] != 0);

	switch (usage) {
	case logical_name:
	case plain_name:
		if (with_path)
			snprintf(devname, max_devname_len, _PATH_DEV "%s",
			    wedge_dev);
		else
			strlcpy(devname, wedge_dev, max_devname_len);
		return true;
	case raw_dev_name:
		if (with_path)
			snprintf(devname, max_devname_len, _PATH_DEV "r%s",
			    wedge_dev);
		else
			snprintf(devname, max_devname_len, "r%s",
			    wedge_dev);
		return true;
	default:
		return false;
	}
}

static bool
is_custom_attribute_writable(const struct disk_partitions *arg, part_id id,
    const mbr_info_t *mb, int i, bool primary,
    const struct mbr_partition *mp, void *cookie)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	struct part_attr_set_data *data = cookie;
	const char *attrtype = parts->dp.pscheme
	    ->custom_attributes[data->attr_no].label;

	if (attrtype == MSG_ptn_active)
	        /* Only 'normal' partitions can be 'Active' */
		return primary && !MBR_IS_EXTENDED(mp->mbrp_type);
#ifdef BOOTSEL
	else if (attrtype == MSG_boot_dflt)
	        /* Only partitions with bootmenu names can be default */
		return mb->mbrb.mbrbs_nametab[i][0] != 0;
	else if (attrtype == MSG_bootmenu)
        	/* The extended partition isn't bootable */
		return !MBR_IS_EXTENDED(mp->mbrp_type);
#endif

	return false;
}

static bool
mbr_custom_attribute_writable(const struct disk_partitions *arg,
    part_id id, size_t attr_no)
{
	const struct mbr_disk_partitions *parts =
	    (const struct mbr_disk_partitions*)arg;
	struct part_attr_set_data data;

	data.attr_no = attr_no;
	data.parts = parts;
	data.str = NULL;
#ifdef BOOTSEL
	data.mbr = NULL;
#endif

	return mbr_part_apply(arg, id, is_custom_attribute_writable, &data);
}

const struct disk_part_edit_column_desc mbr_edit_columns[] = {
	{ .title = MSG_mbr_part_header_1,
#if BOOTSEL
	  .width = 16U
#else
	  .width = 26U
#endif
	 },
	{ .title = MSG_mbr_part_header_2, .width = 8U },
#if BOOTSEL
	{ .title = MSG_mbr_part_header_3, .width = 9U },
#endif
};

const struct disk_part_custom_attribute mbr_custom_attrs[] = {
	{ .label = MSG_ptn_active,	.type = pet_bool },
#if BOOTSEL
	{ .label = MSG_boot_dflt,	.type = pet_bool },
	{ .label = MSG_bootmenu,	.type = pet_str,
					.strlen = MBR_BS_PARTNAMESIZE },
#endif
};

const struct disk_partitioning_scheme
mbr_parts = {
	.name = MSG_parttype_mbr,
	.short_name = MSG_parttype_mbr_short,
	.new_type_prompt = MSG_mbr_get_ptn_id,
	.part_flag_desc = MSG_mbr_flag_desc,
	.size_limit = (daddr_t)UINT32_MAX,
	.secondary_scheme = &disklabel_parts,
	.edit_columns_count = __arraycount(mbr_edit_columns),
	.edit_columns = mbr_edit_columns,
	.custom_attribute_count = __arraycount(mbr_custom_attrs),
	.custom_attributes = mbr_custom_attrs,
	.get_part_alignment = mbr_part_alignment,
	.get_part_info = mbr_get_part_info,
	.get_part_attr_str = mbr_part_attr_str,
	.format_partition_table_str = mbr_table_str,
	.part_type_can_change = mbr_part_type_can_change,
	.can_add_partition = mbr_can_add_partition,
	.custom_attribute_writable = mbr_custom_attribute_writable,
	.format_custom_attribute = mbr_custom_attribute_format,
	.custom_attribute_toggle = mbr_custom_attribute_toggle,
	.custom_attribute_set_str = mbr_custom_attribute_set_str,
	.get_part_types_count = mbr_get_part_type_count,
	.adapt_foreign_part_info = generic_adapt_foreign_part_info,
	.get_part_type = mbr_get_part_type,
	.get_fs_part_type = mbr_get_fs_part_type,
	.get_generic_part_type = mbr_get_generic_part_type,
	.create_custom_part_type = mbr_custom_part_type,
	.create_unknown_part_type = mbr_create_unknown_part_type,
	.secondary_partitions = mbr_read_disklabel,
	.write_to_disk = mbr_write_to_disk,
	.read_from_disk = mbr_read_from_disk,
	.create_new_for_disk = mbr_create_new,
	.guess_disk_geom = mbr_guess_geom,
	.get_cylinder_size = mbr_get_cylinder,
	.change_disk_geom = mbr_change_disk_geom,
	.get_part_device = mbr_get_part_device,
	.max_free_space_at = mbr_max_part_size,
	.get_free_spaces = mbr_get_free_spaces,
	.set_part_info = mbr_set_part_info,
	.delete_all_partitions = mbr_delete_all,
	.delete_partition = mbr_delete_part,
	.add_partition = mbr_add_part,
	.guess_install_target = mbr_guess_root,
	.post_edit_verify = mbr_verify,
	.pre_update_verify = mbr_verify_for_update,
	.free = mbr_free,
	.destroy_part_scheme = mbr_destroy_part_scheme,
};

#endif
